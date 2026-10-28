#!/usr/bin/env python3
"""
Convierte un ELF32 RISC-V (enlazado con sdk/zxv.ld) en un ejecutable .ZXV
(spec §9.2): cabecera de 64 bytes + imagen contigua.

Uso: elf2zxv.py ENTRADA.elf SALIDA.zxv [--name NOMBRE] [--mode 0|1]
                [--stack DIRECCIÓN]

Los mensajes van en español si el idioma del sistema lo es y, si no, en
inglés.
"""
import argparse
import os
import struct
import sys
import zlib

URAM_BASE, URAM_END = 0x20034000, 0x20072000
MAGIC = b"ZXV1"
HDR_SIZE = 64
PROFILE = 1


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
    a = ap.parse_args()

    with open(a.elf, "rb") as f:
        entry, segs = load_segments(f.read())
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
    bss = mem_end - (load + len(image))

    if load < URAM_BASE or mem_end > URAM_END:
        raise SystemExit(tr("the program ({:#x}-{:#x}) is not in user RAM",
                            "el programa ({:#x}-{:#x}) no está en la RAM de usuario").format(load, mem_end))
    if load % 4 or not (load <= entry < load + len(image)):
        raise SystemExit(tr("invalid load address or entry point",
                            "dirección de carga o punto de entrada no válidos"))

    name = (a.name or a.zxv.rsplit("/", 1)[-1].rsplit(".", 1)[0]).encode("ascii", "replace")[:27]
    flags = 0                                       # reservado
    hdr = struct.pack("<4sHHIIIIIBBHI28s", MAGIC, HDR_SIZE, PROFILE, load, entry, len(image),
                      bss, a.stack, a.mode, flags, 0, zlib.crc32(image) & 0xFFFFFFFF, name)
    assert len(hdr) == HDR_SIZE
    with open(a.zxv, "wb") as f:
        f.write(hdr + bytes(image))
    print(tr("{}: load {:#010x}, entry {:#010x}, image {} B, BSS {} B",
             "{}: carga {:#010x}, entrada {:#010x}, imagen {} B, BSS {} B").format(
        a.zxv, load, entry, len(image), bss))
    return 0


if __name__ == "__main__":
    sys.exit(main())
