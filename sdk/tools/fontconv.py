#!/usr/bin/env python3
# SPDX-License-Identifier: MIT (como el SDK, sdk/LICENSE).
"""Convierte fuentes de mapa de bits en C para zxv_font.h (zxv_font_bits), o
en texto para retocarlas a mano.

    fontconv.py ENTRADA SALIDA.c NOMBRE [opciones]
    fontconv.py ENTRADA SALIDA.txt [opciones]

Entradas (por la extensión o, si no, por lo que hay dentro):
  .txt          la de ZX-V (rom/font/zxv8x8.txt): "== CÓDIGO" y las filas,
                '#' tinta y '.' fondo
  .ch8, 768 B   del ZX Spectrum: 96 caracteres, del 32 al 127; su 0x5E es ↑,
                el 0x60 £ y el 0x7F © (con --ascii, ^, ` y el 127)
  .bin, .rom,   un volcado de 8 bytes por carácter (o --height): los de PC
  .fnt, otros   (CP437), MSX, Amstrad CPC... desde el carácter --first (0)
                y con el juego --charset (cp437 si son 256, latin-1 si no)
  .psf          PC Screen Font 1 o 2 (las de la consola de Linux), con su
                tabla Unicode si la lleva
  .bdf          Glyph Bitmap Distribution Format (X11)
  .hex          como GNU Unifont: "CÓDIGO:BYTES" (16 cifras para 8 filas);
                los caracteres anchos (los de 16 columnas de Unifont, el
                doble de cifras) se quedan fuera, con un aviso
Las fuentes tienen que ser de hasta 8 columnas y 32 filas (las más anchas,
como hoja de color: png2gfx y zxv_font_sheet). zxv_font guarda hasta 224
caracteres: se quedan los de --range (por defecto 20-7E,A0-FF: ASCII y
Latin-1, con las tildes, la ñ, ¿ y ¡) que estén en la fuente.

Opciones:
  --format F      txt, zx, raw, psf, bdf o hex (si la extensión no basta)
  --first N       raw: el código del primer carácter (por defecto, 0; 32
                  si son 96)
  --height H      raw: las filas de cada carácter (8); hex: las de los
                  caracteres de 8 columnas (si no, las de los más
                  estrechos del fichero)
  --charset C     raw: cp437, latin-1, ascii... (los de Python) para pasar
                  los códigos de más de 127 a Unicode
  --ascii         zx: deja ^, ` y el 127 como en ASCII
  --range R       los caracteres que se quedan: "20-7E,A0-FF,20AC"

Sale NOMBRE_n (cuántos), NOMBRE_h (las filas), NOMBRE_codes[] (sus códigos
Unicode, de menor a mayor) y NOMBRE_bits[] (una fila por byte, la columna
izquierda en el bit 7). La licencia de una fuente sigue siendo la suya: que
se pueda convertir no quiere decir que se pueda repartir.
"""
import os
import struct
import sys

MAX_CHARS = 224


def die(msg):
    raise SystemExit("fontconv: " + msg)


# --- Entradas: cada una da {código: [filas]} y el alto -------------------------------

def read_txt(path):
    glyphs, code, rows = {}, None, []
    with open(path, encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line or (line.startswith("#") and not set(line) <= set("#.")):
                continue
            if line.startswith("=="):
                if code is not None:
                    glyphs[code] = rows
                code = int(line.split()[1], 16)
                rows = []
                continue
            if code is None or len(line) > 8 or not set(line) <= set("#."):
                die(f"{path}:{lineno}: fila no válida: {line!r}")
            b = 0
            for ch in line.ljust(8, "."):
                b = (b << 1) | (ch == "#")
            rows.append(b)
    if code is not None:
        glyphs[code] = rows
    return glyphs


def read_raw(data, height, first, charset):
    if height < 1 or len(data) % height:
        die(f"{len(data)} bytes no son caracteres de {height} filas")
    n = len(data) // height
    if first + n > 256:
        # Un juego de 8 bits (--charset) no dice qué son los de más del 255:
        # antes se pisaban con los de abajo sin avisar.
        die(f"{n} caracteres de {height} filas desde el {first}: pasan del 255, y en un volcado los códigos son "
            f"de un byte; recorta el fichero a {256 - first} caracteres, o mira --height y --first")
    glyphs = {}
    for i in range(n):
        n = first + i
        if n < 128 or charset in ("latin-1", "latin1", "iso-8859-1"):
            code = n
        else:
            try:
                code = ord(bytes([n & 0xFF]).decode(charset))
            except (UnicodeDecodeError, LookupError) as e:
                die(f"--charset {charset}: {e}")
        glyphs.setdefault(code, list(data[i * height:(i + 1) * height]))
    return glyphs


def read_zx(data, ascii_):
    if len(data) != 768:
        die("una fuente del Spectrum son 768 bytes")
    glyphs = read_raw(data, 8, 32, "latin-1")
    if not ascii_:
        for old, new in ((0x5E, 0x2191), (0x60, 0x00A3), (0x7F, 0x00A9)):
            glyphs[new] = glyphs.pop(old)
    return glyphs


def read_psf(data):
    if data[:2] == b"\x36\x04":                         # PSF 1
        mode, size = data[2], data[3]
        n = 512 if mode & 1 else 256
        width, height, off, has_tab = 8, size, 4, bool(mode & 6)
        bytes_per = size
        tab = data[off + n * size:]
        unicode_ = []
        if has_tab:
            pos = 0
            for _ in range(n):
                codes, in_seq = [], False
                while pos + 1 < len(tab):
                    u = tab[pos] | tab[pos + 1] << 8
                    pos += 2
                    if u == 0xFFFF:
                        break
                    if u == 0xFFFE:                     # lo que sigue son secuencias: no valen
                        in_seq = True
                    elif not in_seq:
                        codes.append(u)
                unicode_.append(codes)
    elif data[:4] == b"\x72\xb5\x4a\x86":               # PSF 2
        _, hsize, flags, n, bytes_per, height, width = struct.unpack_from("<7I", data, 4)
        off = hsize
        unicode_ = []
        if flags & 1:
            tab = data[off + n * bytes_per:]
            pos = 0
            for _ in range(n):
                end = tab.index(b"\xff", pos)
                entry = tab[pos:end]
                pos = end + 1
                single = entry.split(b"\xfe")[0]        # sin las secuencias
                unicode_.append([ord(c) for c in single.decode("utf-8", "replace")])
    else:
        die("no es una fuente PSF")
    if width > 8:
        die(f"la fuente mide {width} de ancho: para zxv_font_bits, 8 como mucho")
    row_bytes = (width + 7) // 8
    glyphs = {}
    for i in range(n):
        g = data[off + i * bytes_per: off + (i + 1) * bytes_per]
        rows = [g[r * row_bytes] for r in range(height)]
        codes = unicode_[i] if unicode_ and unicode_[i] else [i]
        for c in codes:
            glyphs.setdefault(c, rows)
    return glyphs


def read_bdf(path):
    glyphs = {}
    cell = None
    ascent = descent = None
    with open(path, encoding="latin-1") as f:
        lines = [ln.strip() for ln in f]
    i = 0
    while i < len(lines):
        parts = lines[i].split()
        key = parts[0] if parts else ""
        if key == "FONTBOUNDINGBOX":
            cell = [int(v) for v in parts[1:5]]
        elif key == "FONT_ASCENT":
            ascent = int(parts[1])
        elif key == "FONT_DESCENT":
            descent = int(parts[1])
        elif key == "STARTCHAR":
            code, bbx, bitmap = None, None, []
            i += 1
            while i < len(lines) and lines[i] != "ENDCHAR":
                p = lines[i].split()
                if p and p[0] == "ENCODING":
                    code = int(p[1])
                elif p and p[0] == "BBX":
                    bbx = [int(v) for v in p[1:5]]
                elif p and p[0] == "BITMAP":
                    i += 1
                    while i < len(lines) and lines[i] != "ENDCHAR":
                        bitmap.append(int(lines[i], 16) if lines[i] else 0)
                        i += 1
                    break
                i += 1
            if code is None or code < 0 or bbx is None or cell is None:
                i += 1
                continue
            if cell[0] > 8:
                die(f"la fuente mide {cell[0]} de ancho: para zxv_font_bits, 8 como mucho")
            asc = ascent if ascent is not None else cell[1] + cell[3]
            height = cell[1] if ascent is None or descent is None else ascent + descent
            w, h, xo, yo = bbx
            rows = [0] * height
            nbytes = (w + 7) // 8
            for r, v in enumerate(bitmap[:h]):
                y = asc - (yo + h) + r                  # la fila en la celda
                if 0 <= y < height:
                    v >>= nbytes * 8 - 8                # las 8 columnas de la izquierda
                    shift = xo - cell[2]
                    rows[y] = (v >> shift if shift >= 0 else v << -shift) & 0xFF
            glyphs[code] = rows
        i += 1
    if cell is None:
        die("no es una fuente BDF")
    return glyphs


def read_hex(path, height):
    """Los caracteres de 8 columnas ({código: filas}) y los códigos de los
    anchos. En Unifont, cada carácter son 16 filas: 32 cifras si es de 8
    columnas y 64 si es de 16 (o 96 y 128, de 24 y 32). Las filas, height o,
    si no se dice, las de los más estrechos del fichero."""
    raw, where = {}, {}
    with open(path, encoding="ascii", errors="replace") as f:
        for lineno, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            code, _, digits = line.partition(":")
            try:
                c, b = int(code, 16), bytes.fromhex(digits)
            except ValueError:
                c, b = None, b""
            if c is None or not b:
                die(f"{path}:{lineno}: línea no válida (tiene que ser CÓDIGO:BYTES, en hexadecimal)")
            raw[c], where[c] = b, lineno
    if not raw:
        die(f"{path}: no hay ningún carácter")
    if height is None:
        lengths = {len(b) for b in raw.values()}
        height = min(lengths)
        if len(lengths) == 1 and height > 16:
            die(f"{path}: todos los caracteres son de {height} bytes: ¿de 8 columnas y {height} filas, o anchos "
                f"de 16 filas como en Unifont? Dilo con --height ({height} o 16)")
    glyphs, wide = {}, []
    for c, b in raw.items():
        if len(b) == height:
            glyphs[c] = list(b)
        elif len(b) % height == 0 and len(b) // height <= 4:
            wide.append(c)
        else:
            die(f"{path}:{where[c]}: el carácter {c:04X} tiene {len(b)} bytes: con {height} filas, "
                f"{height} (8 columnas) o {2 * height} (16 columnas)")
    return glyphs, wide


# --- Salidas ---------------------------------------------------------------------------

def parse_range(text):
    keep = set()
    for part in text.split(","):
        a, _, b = part.partition("-")
        try:
            lo, hi = int(a, 16), int(b or a, 16)
        except ValueError:
            die(f"--range: «{part}» no es un código o un intervalo en hexadecimal")
        if lo > hi:
            die(f"--range: «{part}»: el intervalo va al revés")
        if hi > 0xFFFF:
            # zxv_font guarda los códigos en 16 bits (uint16_t): los de más
            # arriba (emojis...) no caben
            die(f"--range: «{part}»: los códigos van de 0 a FFFF")
        keep.update(range(lo, hi + 1))
    return keep


def write_c(glyphs, height, out, name, src):
    codes = sorted(glyphs)
    lines = [f"/* Generado por sdk/tools/fontconv.py a partir de {os.path.basename(src)}. */",
             "#include <stdint.h>", "",
             f"const unsigned {name}_n = {len(codes)};",
             f"const unsigned {name}_h = {height};",
             f"const uint16_t {name}_codes[{len(codes)}] = {{"]
    for i in range(0, len(codes), 12):
        lines.append("    " + ", ".join(f"0x{c:02X}" for c in codes[i:i + 12]) + ",")
    lines.append("};")
    lines.append(f"const uint8_t {name}_bits[{len(codes)} * {height}] = {{")
    for c in codes:
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in glyphs[c]) + f",  /* U+{c:04X} */")
    lines.append("};")
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def write_txt(glyphs, out, src):
    lines = [f"# Generado por sdk/tools/fontconv.py a partir de {os.path.basename(src)}."]
    for c in sorted(glyphs):
        ch = chr(c) if c > 32 and c != 0x7F else ""
        lines.append(f"== 0x{c:02X} {ch}".rstrip())
        for b in glyphs[c]:
            lines.append("".join("#" if b >> (7 - k) & 1 else "." for k in range(8)))
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def number(opt, text):
    try:
        return int(text, 0)
    except ValueError:
        die(f"{opt} {text}: no es un número")


def main(argv):
    opts = {"--format": None, "--first": None, "--height": None, "--charset": None, "--range": "20-7E,A0-FF"}
    flags = set()
    pos = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in opts:
            if i + 1 >= len(argv):
                die(f"{a}: falta el valor")
            opts[a] = argv[i + 1]
            i += 2
        elif a == "--ascii":
            flags.add(a)
            i += 1
        elif a.startswith("--"):
            die(f"opción desconocida: {a}")
        else:
            pos.append(a)
            i += 1
    if len(pos) not in (2, 3) or (pos[1].lower().endswith(".c") and len(pos) != 3):
        print(__doc__.strip(), file=sys.stderr)
        return 2
    src, out = pos[0], pos[1]
    height_opt = number("--height", opts["--height"]) if opts["--height"] is not None else None
    if height_opt is not None and not 1 <= height_opt <= 32:
        die(f"--height {height_opt}: de 1 a 32")
    try:
        with open(src, "rb") as f:
            data = f.read()
    except OSError as e:
        die(f"no se puede leer {src}: {e.strerror}")
    keep = parse_range(opts["--range"])
    wide = []
    ext = os.path.splitext(src)[1].lower()
    fmt = opts["--format"]
    if not fmt:
        if data[:2] == b"\x36\x04" or data[:4] == b"\x72\xb5\x4a\x86":
            fmt = "psf"
        elif ext == ".txt":
            fmt = "txt"
        elif ext == ".bdf" or data[:9] == b"STARTFONT":
            fmt = "bdf"
        elif ext == ".hex":
            fmt = "hex"
        elif ext == ".ch8" or (len(data) == 768 and opts["--first"] is None):
            fmt = "zx"
        else:
            fmt = "raw"
    if fmt == "txt":
        glyphs = read_txt(src)
    elif fmt == "zx":
        glyphs = read_zx(data, "--ascii" in flags)
    elif fmt == "raw":
        height = height_opt or 8
        n = len(data) // height
        first = number("--first", opts["--first"]) if opts["--first"] is not None else (32 if n == 96 else 0)
        if first < 0:
            die(f"--first {first}: desde 0")
        charset = opts["--charset"] or ("cp437" if n == 256 and first == 0 else "latin-1")
        glyphs = read_raw(data, height, first, charset)
    elif fmt == "psf":
        glyphs = read_psf(data)
    elif fmt == "bdf":
        glyphs = read_bdf(src)
    elif fmt == "hex":
        glyphs, wide = read_hex(src, height_opt)
    else:
        die(f"formato desconocido: {fmt}")
    # Primero los de --range: lo que no se va a usar no cuenta (ni sus filas).
    glyphs = {c: r for c, r in glyphs.items() if c in keep}
    wide = sorted(c for c in wide if c in keep)
    if wide:
        print(f"fontconv: aviso: {len(wide)} caracteres de más de 8 columnas (como los de 16 de Unifont) no "
              f"caben en zxv_font_bits y se quedan fuera: {', '.join(f'U+{c:04X}' for c in wide[:8])}"
              f"{'...' if len(wide) > 8 else ''}", file=sys.stderr)
    if not glyphs:
        die("no queda ningún carácter (mira --range)" if not wide else
            "no queda ningún carácter de 8 columnas (los de --range son todos anchos)")
    heights = {len(r) for r in glyphs.values()}
    if len(heights) != 1:
        die(f"los caracteres no tienen todos las mismas filas: {sorted(heights)}")
    height = heights.pop()
    if not 1 <= height <= 32:
        die(f"{height} filas: de 1 a 32")
    if len(glyphs) > MAX_CHARS:
        die(f"{len(glyphs)} caracteres: zxv_font guarda {MAX_CHARS} como mucho (usa --range)")
    if out.lower().endswith(".txt"):
        write_txt(glyphs, out, src)
    else:
        write_c(glyphs, height, out, pos[2], src)
    print(f"fontconv: {len(glyphs)} caracteres de {height} filas ({fmt})")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
