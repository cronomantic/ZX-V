#!/usr/bin/env python3
"""
Convierte un ELF32 RISC-V (enlazado con sdk/zxv.ld) en un ejecutable .ZXV
(spec §9.2): cabecera de 72 bytes, la pantalla de carga (si la hay) y la
imagen contigua.

Uso: elf2zxv.py ENTRADA.elf SALIDA.zxv [--name NOMBRE] [--mode 0|1]
                [--stack DIRECCIÓN] [--screen PANTALLA]
                [--bank NOMBRE=FICHERO ...] [--pack]

La pantalla de carga es un PNG (de 256 x 192, 512 x 384 o el cuadro con el
borde) o la primera pantalla de un .gfx (png2gfx.py --screen, zxv-gfx).

Los bancos (D109, zxv_bank.h) van detrás de la imagen, con su índice y el
cierre ZXVBANK1 al final: cualquier fichero, que el programa carga con
zxv_bank_load(NOMBRE, ...). Con --pack, cada uno va comprimido (deflate) si
así ocupa menos.

Los mensajes van en español si el idioma del sistema lo es y, si no, en
inglés.
"""
import argparse
import os
import struct
import sys
import zlib

URAM_BASE, URAM_END = 0x20034000, 0x20070000
# La VRAM oculta de cada página en modo 0 (zxv.ld, D108): lo que el programa
# pone ahí empieza a cero (crt0.S) y no va en el fichero.
VRAM_HIDDEN = ((0x2000A000, 0x2001C000), (0x20022000, 0x20034000))
MAGIC = b"ZXV1"
HDR_SIZE = 72
PROFILE = 1
# Los bancos (zxv.h, ZXV_BANK_*).
BANK_MAGIC = b"ZXVBANK1"
BANK_NAME_MAX = 15
BANK_MAX = 4096
BANK_DATA, BANK_STORED, BANK_DEFLATE = 0, 0, 1
BANK_CODE = 1
# Los bancos de código (tools/zxvbanks.py): en el ELF, las secciones
# .zxvbank.ZONA.NOMBRE, enlazadas en su zona y «cargadas» desde aquí arriba
# (no van en la imagen).
BANK_LMA = 0x80000000


def system_spanish():
    """¿Es español el idioma del sistema? El de LC_ALL, LC_MESSAGES o LANG
    o, sin ellas, en Windows el del usuario."""
    for var in ("LC_ALL", "LC_MESSAGES", "LANG"):
        v = os.environ.get(var)
        if v:
            return v[:2].lower() == "es"
    if sys.platform == "win32":
        try:
            import ctypes
            buf = ctypes.create_unicode_buffer(85)
            if ctypes.windll.kernel32.GetUserDefaultLocaleName(buf, len(buf)):
                return buf.value[:2].lower() == "es"
        except (AttributeError, OSError):
            pass
    return False


SPANISH = system_spanish()


def tr(en, es):
    """El texto en el idioma del sistema: el español, o el inglés para cualquier otro."""
    return es if SPANISH else en


# Los textos del propio argparse, para la ayuda y sus errores en español.
ARGPARSE_ES = {
    "usage: ": "uso: ",
    "positional arguments": "argumentos",
    "options": "opciones",
    "optional arguments": "opciones",
    "show this help message and exit": "enseña esta ayuda y termina",
    "%(prog)s: error: %(message)s\n": "%(prog)s: error: %(message)s\n",
    "the following arguments are required: %s": "faltan estos argumentos: %s",
    "unrecognized arguments: %s": "argumentos desconocidos: %s",
    "argument %(argument_name)s: %(message)s": "argumento %(argument_name)s: %(message)s",
    "invalid choice: %(value)r (choose from %(choices)s)": "%(value)r no vale (puede ser %(choices)s)",
    "invalid %(type)s value: %(value)r": "valor no válido: %(value)r",
    "expected one argument": "falta su valor",
}
if SPANISH:
    argparse._ = lambda s: ARGPARSE_ES.get(s, s)


def code_banks(data):
    """Los bancos de código del ELF: (nombre, zona, dirección, contenido)."""
    shoff = struct.unpack_from("<I", data, 32)[0]
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 46)
    if not shoff or not shnum:
        return []
    def sh(i):
        return struct.unpack_from("<IIIIIIIIII", data, shoff + i * shentsize)
    names = sh(shstrndx)
    out = []
    for i in range(shnum):
        name_off, typ, flags, addr, off, size = sh(i)[:6]
        n = data[names[4] + name_off:data.index(b"\0", names[4] + name_off)].decode("ascii", "replace")
        if not n.startswith(".zxvbank."):
            continue
        region, _, bank = n[len(".zxvbank."):].partition(".")
        if not region.isdigit() or not bank:
            raise SystemExit(tr("section {}: not a code bank", "sección {}: no es un banco de código").format(n))
        body = data[off:off + size] if typ != 8 else bytes(size)          # SHT_NOBITS: a cero
        out.append((bank, int(region), addr, body))
    return out


def load_segments(data):
    if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 1:
        raise SystemExit(tr("not a little-endian ELF32", "no es un ELF32 little-endian"))
    if struct.unpack_from("<H", data, 18)[0] != 243:
        raise SystemExit(tr("not a RISC-V ELF", "no es un ELF RISC-V"))
    entry, phoff = struct.unpack_from("<II", data, 24)
    phentsize, phnum = struct.unpack_from("<HH", data, 42)
    segs = []
    for i in range(phnum):
        p_type, off, vaddr, paddr, filesz, memsz = struct.unpack_from("<IIIIII", data, phoff + i * phentsize)
        if p_type == 1 and memsz:
            segs.append((paddr, data[off:off + filesz], memsz))
    segs.sort()
    return entry, segs


def address(v):
    return int(v, 0)


def main():
    address.__name__ = tr("address", "dirección")     # para el mensaje de argparse si no vale
    ap = argparse.ArgumentParser(description=tr("Converts a RISC-V ELF (linked with sdk/zxv.ld) into a "
                                                ".ZXV program.",
                                                "Convierte un ELF RISC-V (enlazado con sdk/zxv.ld) en un "
                                                "programa .ZXV."))
    ap.add_argument("elf", help=tr("the ELF file", "el fichero ELF"))
    ap.add_argument("zxv", help=tr("the .ZXV file to write", "el fichero .ZXV que se escribe"))
    ap.add_argument("--name", default="", metavar=tr("NAME", "NOMBRE"),
                    help=tr("the program's name (by default, the file's)",
                            "el nombre del programa (si no, el del fichero)"))
    ap.add_argument("--mode", type=int, choices=(0, 1), default=0,
                    help=tr("the video mode it starts in", "el modo de vídeo con el que empieza"))
    ap.add_argument("--stack", type=address, default=0, metavar=tr("ADDRESS", "DIRECCIÓN"),
                    help=tr("the top of its stack (0: it keeps the current one)",
                            "el tope de su pila (0: sigue en la de ahora)"))
    ap.add_argument("--screen", metavar=tr("SCREEN", "PANTALLA"),
                    help=tr("its loading screen: a PNG or a .gfx with a screen",
                            "su pantalla de carga: un PNG o un .gfx con una pantalla"))
    ap.add_argument("--bank", action="append", default=[], metavar=tr("NAME=FILE", "NOMBRE=FICHERO"),
                    help=tr("a data bank: FILE, loaded with zxv_bank_load(NAME, ...) (repeatable)",
                            "un banco de datos: FICHERO, que se carga con zxv_bank_load(NOMBRE, ...) "
                            "(se puede repetir)"))
    ap.add_argument("--pack", action="store_true",
                    help=tr("compress each bank (deflate) when it gets smaller",
                            "comprime cada banco (deflate) si así ocupa menos"))
    a = ap.parse_args()

    with open(a.elf, "rb") as f:
        elf = f.read()
    entry, segs = load_segments(elf)
    code = code_banks(elf)
    segs = [s for s in segs if s[0] < BANK_LMA]                 # los bancos de código, aparte
    hidden = [s for s in segs if any(lo <= s[0] and s[0] + s[2] <= hi for lo, hi in VRAM_HIDDEN)]
    for addr, content, memsz in hidden:
        if content:
            raise SystemExit(tr("data with an initial value in the hidden VRAM ({:#x}): it is not loaded",
                                "datos con valor inicial en la VRAM oculta ({:#x}): no se cargan").format(addr))
    segs = [s for s in segs if s not in hidden]
    if hidden and a.mode == 1:
        raise SystemExit(tr("the program uses the hidden VRAM (ZXV_VRAMn_DATA), which mode 1 does not have",
                            "el programa usa la VRAM oculta (ZXV_VRAMn_DATA), que en el modo 1 no hay"))
    if not segs:
        raise SystemExit(tr("the ELF has no loadable segments", "el ELF no tiene segmentos cargables"))

    load = segs[0][0]
    image = bytearray()
    mem_end = load
    for addr, content, memsz in segs:
        if addr < load + len(image):
            raise SystemExit(tr("overlapping segments at {:#x}", "segmentos solapados en {:#x}").format(addr))
        if content:
            image += b"\0" * (addr - (load + len(image)))
            image += content
        mem_end = max(mem_end, addr + memsz)
    # Las zonas de los bancos de código, detrás del BSS: cuentan en lo que
    # ocupa el programa (el cargador comprueba que cabe y las borra).
    for _, _, addr, body in code:
        mem_end = max(mem_end, addr + len(body))
    bss = mem_end - (load + len(image))

    if load < URAM_BASE or mem_end > URAM_END:
        raise SystemExit(tr("the program ({:#x}-{:#x}) is not in user RAM",
                            "el programa ({:#x}-{:#x}) no está en la RAM de usuario").format(load, mem_end))
    if load % 4 or not (load <= entry < load + len(image)):
        raise SystemExit(tr("invalid load address or entry point",
                            "dirección de carga o punto de entrada no válidos"))

    name = (a.name or os.path.splitext(os.path.basename(a.zxv))[0]).encode("ascii", "replace")[:27]
    flags = 0                                       # reservado
    screen = b""
    if a.screen:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import png2gfx
        try:
            screen = png2gfx.screen_block(a.screen)
        except SystemExit as e:                     # png2gfx dice ya qué pasa
            raise SystemExit(str(e).replace("png2gfx: ", "", 1))
    hdr = struct.pack("<4sHHIIIIIBBHI28sII", MAGIC, HDR_SIZE, PROFILE, load, entry, len(image),
                      bss, a.stack, a.mode, flags, 0, zlib.crc32(image) & 0xFFFFFFFF, name,
                      len(screen), 0)
    assert len(hdr) == HDR_SIZE
    out = hdr + screen + bytes(image)
    out += banks(a.bank, a.pack, len(out), zlib.crc32(image) & 0xFFFFFFFF, len(image), code)
    with open(a.zxv, "wb") as f:
        f.write(out)
    print(tr("{}: load {:#010x}, entry {:#010x}, image {} B, BSS {} B",
             "{}: carga {:#010x}, entrada {:#010x}, imagen {} B, BSS {} B").format(
        a.zxv, load, entry, len(image), bss))
    return 0


def banks(specs, pack, at, prog_crc, image_size, code=()):
    """Los bancos (los de código del ELF y los de datos de --bank), su índice y
    el cierre, para poner detrás de la imagen (que acaba en at); b"" sin
    bancos."""
    if not specs and not code:
        return b""
    if len(specs) + len(code) > BANK_MAX:
        raise SystemExit(tr("too many banks (at most {})", "demasiados bancos (como mucho {})").format(BANK_MAX))
    data, entries, seen = b"", b"", set()
    items = [(name, BANK_CODE, region, addr, body) for name, region, addr, body in code]
    for spec in specs:
        name, sep, path = spec.partition("=")
        if not sep or not path:
            raise SystemExit(tr("--bank {}: it must be NAME=FILE", "--bank {}: tiene que ser NOMBRE=FICHERO").format(spec))
        items.append((name, BANK_DATA, 0, 0, path))
    for name, kind, region, addr, what in items:
        if not (0 < len(name) <= BANK_NAME_MAX) or not all(33 <= ord(c) < 127 for c in name):
            raise SystemExit(tr("bank {!r}: the name must have 1 to {} ASCII characters, with no spaces",
                                "banco {!r}: el nombre ha de tener de 1 a {} caracteres ASCII, sin espacios")
                             .format(name, BANK_NAME_MAX))
        if name in seen:
            raise SystemExit(tr("bank {!r} is repeated", "el banco {!r} está repetido").format(name))
        seen.add(name)
        if kind == BANK_CODE:
            raw = what
        else:
            try:
                with open(what, "rb") as f:
                    raw = f.read()
            except OSError as e:
                raise SystemExit(tr("bank {!r}: {}", "banco {!r}: {}").format(name, e.strerror or e))
        stored, method = raw, BANK_STORED
        if pack:
            c = zlib.compressobj(9, zlib.DEFLATED, -15)
            z = c.compress(raw) + c.flush()
            if len(z) < len(raw):
                stored, method = z, BANK_DEFLATE
        entries += struct.pack("<16sBBBBIIIIIII", name.encode("ascii"), kind, method, region, 0,
                               at + len(data), len(stored), len(raw), zlib.crc32(raw) & 0xFFFFFFFF, addr, 0, 0)
        data += stored
        print(tr("  bank {}: {} B{}{}", "  banco {}: {} B{}{}").format(
            name, len(raw), tr(" ({} B packed)", " ({} B comprimido)").format(len(stored))
            if method == BANK_DEFLATE else "",
            tr(", code in region {} at {:#010x}", ", de código en la zona {}, en {:#010x}").format(region, addr)
            if kind == BANK_CODE else ""))
    index = struct.pack("<IIII", len(items), prog_crc, image_size, zlib.crc32(entries) & 0xFFFFFFFF) + entries
    tail = BANK_MAGIC + struct.pack("<II", at + len(data), len(index))
    return data + index + tail


if __name__ == "__main__":
    sys.exit(main())
