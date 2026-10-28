#!/usr/bin/env python3
"""
Convierte imágenes PNG en un fichero de gráficos .gfx para el BASIC de
ZX-V (LOAD "x" GRAPHICS; formato en dev/basic.md §9.8).

Uso: png2gfx.py SALIDA.gfx ORDEN...
     png2gfx.py --list FICHERO.gfx

Órdenes (se aplican en este orden; los números son los del BASIC):
  --sprite N IMAGEN [X,Y,AN,AL]   el sprite N es la imagen (o ese rectángulo)
  --sprites N IMAGEN AN,AL        una hoja: los sprites N, N+1... de AN x AL,
                                  fila a fila
  --tiles S N IMAGEN [X,Y,AN,AL]  la imagen (o el rectángulo) cortada en tiles
                                  de S x S (8 o 16), numerados desde N
  --level M S IMAGEN              un nivel: la imagen se corta en tiles de
                                  S x S, los repetidos se aprovechan (y se
                                  añaden los nuevos al juego de tiles) y el
                                  mapa M queda con su distribución
  --map M S FICHERO.csv           el mapa M de un CSV con el valor de cada
                                  celda (como en MAP POKE), una fila por línea
  --texture N IMAGEN [C]          la textura N (cuadrada, de lado potencia de
                                  2, de 2 a 256); se repite o, con C, fuera
                                  es del color C
  --wrap M                        el mapa M se repite
Opciones:
  --flip                          --level aprovecha también los tiles
                                  espejados (con los bits de espejo)
  --key RRGGBB                    ese color es transparente

Cada píxel toma el color de ZX-V más cercano (se avisa si no es uno
exacto); los píxeles con alfa < 128 son transparentes (el 8). En un nivel,
las celdas de los tiles con transparencias llevan el bit de transparencia.
Solo hace falta Python 3: el PNG se lee aquí mismo.
"""
import os
import struct
import sys
import zlib


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


# La ayuda en inglés (la española es la de arriba).
HELP_EN = """
Converts PNG images into a .gfx graphics file for the ZX-V BASIC
(LOAD "x" GRAPHICS; format in dev/basic.md section 9.8).

Usage: png2gfx.py OUTPUT.gfx COMMAND...
       png2gfx.py --list FILE.gfx

Commands (applied in this order; the numbers are the BASIC's):
  --sprite N IMAGE [X,Y,W,H]      sprite N is the image (or that rectangle)
  --sprites N IMAGE W,H           a sheet: sprites N, N+1... of W x H, row
                                  by row
  --tiles S N IMAGE [X,Y,W,H]     the image (or the rectangle) cut into S x S
                                  tiles (8 or 16), numbered from N
  --level M S IMAGE               a level: the image is cut into S x S tiles,
                                  repeated ones are reused (and new ones are
                                  added to the tile set) and map M gets their
                                  layout
  --map M S FILE.csv              map M from a CSV with the value of each
                                  cell (as in MAP POKE), one row per line
  --texture N IMAGE [C]           texture N (square, its side a power of 2
                                  from 2 to 256); it repeats or, with C,
                                  outside it is colour C
  --wrap M                        map M repeats
Options:
  --flip                          --level also reuses mirrored tiles (with
                                  the mirror bits)
  --key RRGGBB                    that colour is transparent

Each pixel takes the nearest ZX-V colour (with a warning if it is not an
exact one); pixels with alpha < 128 are transparent (colour 8). In a level,
the cells of tiles with transparent pixels get the transparency bit.
Only Python 3 is needed: the PNG is read right here.
"""

TRANSPARENT = 8
FLIP_H, FLIP_V, TRANSP = 0x1000, 0x2000, 0x4000


def die(msg):
    raise SystemExit("png2gfx: " + msg)


def palette():
    """Los 16 colores de ZX-V (spec §4.3): BRIGHT-G-R-B."""
    pal = []
    for i in range(16):
        lvl = 0xFF if i & 8 else 0xD7
        pal.append(((lvl if i & 2 else 0), (lvl if i & 4 else 0), (lvl if i & 1 else 0)))
    return pal


PALETTE = palette()

# --- PNG ------------------------------------------------------------------------


def read_png(path):
    """Devuelve (ancho, alto, filas de píxeles (r, g, b, a))."""
    try:
        data = open(path, "rb").read()
    except OSError as e:
        die("%s: %s" % (path, e.strerror))
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        die(tr("%s: not a PNG", "%s: no es un PNG") % path)
    pos, ihdr, plte, trns, idat = 8, None, None, None, []
    while pos + 8 <= len(data):
        n, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        pos += 12 + n
        if kind == b"IHDR":
            ihdr = body
        elif kind == b"PLTE":
            plte = body
        elif kind == b"tRNS":
            trns = body
        elif kind == b"IDAT":
            idat.append(body)
        elif kind == b"IEND":
            break
    if not ihdr or not idat:
        die(tr("%s: incomplete PNG", "%s: PNG incompleto") % path)
    w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", ihdr)
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(ctype)
    if channels is None or depth not in (1, 2, 4, 8, 16) or (ctype in (2, 4, 6) and depth < 8):
        die(tr("%s: invalid PNG type", "%s: tipo de PNG no válido") % path)
    if interlace:
        die(tr("%s: interlaced PNGs are not supported", "%s: los PNG entrelazados no se admiten") % path)
    if ctype == 3 and not plte:
        die(tr("%s: the palette is missing", "%s: falta la paleta") % path)
    try:
        raw = zlib.decompress(b"".join(idat))
    except zlib.error:
        die(tr("%s: corrupt data", "%s: datos dañados") % path)
    bits = channels * depth
    stride = (w * bits + 7) // 8
    bpp = max(1, bits // 8)
    if len(raw) < h * (stride + 1):
        die(tr("%s: incomplete data", "%s: datos incompletos") % path)

    rows, prev = [], bytearray(stride)
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        if f == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 0xFF
        elif f == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif f == 3:
            for i in range(stride):
                a = line[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + ((a + prev[i]) >> 1)) & 0xFF
        elif f == 4:
            for i in range(stride):
                a = line[i - bpp] if i >= bpp else 0
                b = prev[i]
                c = prev[i - bpp] if i >= bpp else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 0xFF
        elif f != 0:
            die(tr("%s: invalid filter %d", "%s: filtro %d no válido") % (path, f))
        rows.append(line)
        prev = line

    def samples(line):
        if depth == 8:
            return list(line)
        if depth == 16:
            return [(line[i] << 8) | line[i + 1] for i in range(0, len(line), 2)]
        per = 8 // depth
        mask = (1 << depth) - 1
        return [(line[i // per] >> (8 - depth * (i % per + 1))) & mask for i in range(w * channels)]

    top = (1 << depth) - 1
    alpha_pal = list(trns) if (ctype == 3 and trns) else []
    key = None
    if trns and ctype == 0:
        key = struct.unpack(">H", trns[:2])[0]
    elif trns and ctype == 2:
        key = struct.unpack(">HHH", trns[:6])
    out = []
    for line in rows:
        s = samples(line)
        px = []
        for x in range(w):
            v = s[x * channels:(x + 1) * channels]
            if ctype == 3:
                i = v[0]
                if 3 * i + 2 >= len(plte):
                    die(tr("%s: palette index outside the palette",
                           "%s: índice de paleta fuera de la paleta") % path)
                a = alpha_pal[i] if i < len(alpha_pal) else 255
                px.append((plte[3 * i], plte[3 * i + 1], plte[3 * i + 2], a))
                continue
            sc = [c * 255 // top for c in v]
            if ctype == 0:
                px.append((sc[0], sc[0], sc[0], 0 if key == v[0] else 255))
            elif ctype == 2:
                px.append((sc[0], sc[1], sc[2], 0 if key == tuple(v) else 255))
            elif ctype == 4:
                px.append((sc[0], sc[0], sc[0], sc[1]))
            else:
                px.append(tuple(sc))
        out.append(px)
    return w, h, out


# --- Colores ----------------------------------------------------------------------


class Image:
    """Una imagen ya convertida a los índices de ZX-V."""

    def __init__(self, path, key):
        self.path = path
        self.w, self.h, rgba = read_png(path)
        cache, inexact = {}, 0
        self.pix = []
        for row in rgba:
            r_out = []
            for p in row:
                hit = cache.get(p)
                if hit is None:
                    hit = cache[p] = colour(p, key)
                r_out.append(hit[0])
                inexact += not hit[1]
            self.pix.append(r_out)
        if inexact:
            print(tr("png2gfx: warning: %s: %d pixels are not ZX-V colours (the nearest one is used)",
                     "png2gfx: aviso: %s: %d píxeles no son colores de ZX-V (se usa el más cercano)")
                  % (path, inexact), file=sys.stderr)

    def region(self, x, y, w, h, outside=TRANSPARENT):
        return [[self.pix[j][i] if 0 <= i < self.w and 0 <= j < self.h else outside
                 for i in range(x, x + w)] for j in range(y, y + h)]


EXACT = {c: i for i, c in enumerate(PALETTE) if i != TRANSPARENT}


def colour(p, key):
    """Índice de ZX-V del píxel (r, g, b, a), y si era un color exacto."""
    r, g, b, a = p
    if a < 128 or (r, g, b) == key:
        return TRANSPARENT, True
    if (r, g, b) in EXACT:
        return EXACT[(r, g, b)], True
    best = min((i for i in range(16) if i != TRANSPARENT),
               key=lambda i: (PALETTE[i][0] - r) ** 2 + (PALETTE[i][1] - g) ** 2 + (PALETTE[i][2] - b) ** 2)
    return best, False


def pack(rows):
    """Filas de índices -> bytes de 4 bpp (nibble bajo = píxel par)."""
    out = bytearray()
    for row in rows:
        r = list(row) + ([TRANSPARENT] if len(row) & 1 else [])
        for i in range(0, len(r), 2):
            out.append((r[i] & 15) | (r[i + 1] & 15) << 4)
    return bytes(out)


def hmirror(t, s):
    return tuple(tuple(reversed(t[y])) for y in range(s))


def vmirror(t, s):
    return tuple(reversed(t))


# --- Gráficos -----------------------------------------------------------------------


class Store:
    def __init__(self):
        self.sprites = {}           # n -> (an, al, filas)
        self.tiles = {8: {}, 16: {}}  # lado -> {n: tile (tupla de filas)}
        self.maps = {}              # m -> [an, al, lado, repite, celdas]
        self.textures = {}          # n -> (log2, repite, fuera, texels)

    def tile_count(self, s):
        return max(self.tiles[s]) + 1 if self.tiles[s] else 0

    def serialize(self):
        recs = []
        for n in sorted(self.sprites):
            w, h, rows = self.sprites[n]
            recs.append((1, n, struct.pack("<HH", w, h) + pack(rows)))
        for s in (8, 16):
            if self.tiles[s]:
                count = self.tile_count(s)
                blank = tuple(tuple([0] * s) for _ in range(s))
                data = b"".join(pack(self.tiles[s].get(i, blank)) for i in range(count))
                recs.append((2, s, struct.pack("<HH", count, 0) + data))
        for m in sorted(self.maps):
            w, h, s, wrap, cells = self.maps[m]
            recs.append((3, m, struct.pack("<HHBBH", w, h, s, wrap, 0) + struct.pack("<%dH" % len(cells), *cells)))
        for n in sorted(self.textures):
            l2, wrap, out, texels = self.textures[n]
            recs.append((4, n, bytes([l2, wrap, out, 0]) + bytes(texels)))
        out = b"ZXVG" + struct.pack("<BBH", 1, 0, len(recs))
        for kind, num, data in recs:
            out += struct.pack("<BBHI", kind, 0, num, len(data)) + data
        return out


def ints(text, n, what):
    try:
        v = [int(x) for x in text.split(",")]
    except ValueError:
        v = []
    if len(v) != n:
        die(tr("%s: %d comma-separated numbers expected",
               "%s: se esperaban %d números separados por comas") % (what, n))
    return v


def number(text, lo, hi, what):
    try:
        v = int(text)
    except ValueError:
        die(tr("%s: %r is not a number", "%s: %r no es un número") % (what, text))
    if not lo <= v <= hi:
        die(tr("%s: %d out of range (%d to %d)", "%s: %d fuera de rango (%d a %d)") % (what, v, lo, hi))
    return v


def tile_size(text):
    s = number(text, 8, 16, tr("tile size", "lado del tile"))
    if s not in (8, 16):
        die(tr("the tile size is 8 or 16", "el lado del tile es 8 o 16"))
    return s


def put_sprite(st, n, rows, w, h):
    if not (1 <= w <= 1024 and 1 <= h <= 1024):
        die(tr("sprite %d: sides from 1 to 1024 pixels", "sprite %d: de 1 a 1024 píxeles de lado") % n)
    st.sprites[n] = (w, h, rows)


def cut_tiles(img, s, x, y, w, h):
    if w % s or h % s:
        die(tr("%s: %d x %d is not a multiple of %d", "%s: %d x %d no es múltiplo de %d") % (img.path, w, h, s))
    reg = img.region(x, y, w, h)
    for ty in range(h // s):
        for tx in range(w // s):
            yield tuple(tuple(reg[ty * s + j][tx * s:(tx + 1) * s]) for j in range(s))


def level(st, m, s, img, flip):
    tiles = st.tiles[s]
    known = {}
    for i in sorted(tiles, reverse=True):
        known[tiles[i]] = i
    nxt = st.tile_count(s)
    cells = []
    for t in cut_tiles(img, s, 0, 0, img.w, img.h):
        cell = None
        variants = [(t, 0)]
        if flip:
            variants += [(hmirror(t, s), FLIP_H), (vmirror(t, s), FLIP_V),
                         (vmirror(hmirror(t, s), s), FLIP_H | FLIP_V)]
        for v, bits in variants:
            if v in known:
                cell = known[v] | bits
                break
        if cell is None:
            if nxt > 4095:
                die(tr("%s: more than 4096 tiles", "%s: más de 4096 tiles") % img.path)
            tiles[nxt] = t
            known[t] = nxt
            cell = nxt
            nxt += 1
        if any(TRANSPARENT in row for row in t):
            cell |= TRANSP
        cells.append(cell)
    w, h = img.w // s, img.h // s
    st.maps[m] = [w, h, s, 0, cells]


def read_csv(path):
    try:
        lines = [l.strip() for l in open(path, encoding="utf-8-sig")]
    except OSError as e:
        die("%s: %s" % (path, e.strerror))
    rows = []
    for l in lines:
        if not l:
            continue
        try:
            rows.append([int(v) for v in l.rstrip(",").split(",")])
        except ValueError:
            die(tr("%s: %r are not numbers", "%s: %r no son números") % (path, l))
    if not rows or any(len(r) != len(rows[0]) for r in rows):
        die(tr("%s: all the rows must have the same cells",
               "%s: todas las filas deben tener las mismas celdas") % path)
    if any(not 0 <= v <= 65535 for r in rows for v in r):
        die(tr("%s: the cells go from 0 to 65535", "%s: las celdas van de 0 a 65535") % path)
    return rows


# --- Listado ---------------------------------------------------------------------


def list_gfx(path):
    data = open(path, "rb").read()
    if data[:4] != b"ZXVG" or len(data) < 8 or data[4] != 1:
        die(tr("%s: not a graphics file", "%s: no es un fichero de gráficos") % path)
    count, = struct.unpack_from("<H", data, 6)
    pos, tiles = 8, {}
    maps = []
    for _ in range(count):
        if pos + 8 > len(data):
            die(tr("%s: truncated file", "%s: fichero cortado") % path)
        kind, _, num, ln = struct.unpack_from("<BBHI", data, pos)
        d = data[pos + 8:pos + 8 + ln]
        pos += 8 + ln
        if len(d) != ln:
            die(tr("%s: truncated file", "%s: fichero cortado") % path)
        if kind == 1:
            w, h = struct.unpack_from("<HH", d)
            print("sprite %d: %d x %d" % (num, w, h))
        elif kind == 2:
            n, = struct.unpack_from("<H", d)
            tiles[num] = n
            print(tr("%d x %d tiles: %d", "tiles de %d x %d: %d") % (num, num, n))
        elif kind == 3:
            w, h, s, wrap = struct.unpack_from("<HHBB", d)
            cells = struct.unpack_from("<%dH" % (w * h), d, 8)
            top = max(c & 0xFFF for c in cells)
            maps.append((num, s, top))
            print(tr("map %d: %d x %d cells, tiles of %d%s, highest tile %d",
                     "mapa %d: %d x %d celdas, tiles de %d%s, tile más alto %d")
                  % (num, w, h, s, tr(", wraps", ", se repite") if wrap else "", top))
        elif kind == 4:
            side = 1 << d[0]
            how = (tr("wraps", "se repite") if d[1]
                   else tr("outside, colour %d", "fuera, el color %d") % d[2])
            print(tr("texture %d: %d x %d, %s", "textura %d: %d x %d, %s") % (num, side, side, how))
        else:
            die(tr("%s: unknown object type %d", "%s: objeto de tipo %d desconocido") % (path, kind))
    for num, s, top in maps:
        if tiles.get(s, 0) <= top:
            print(tr("warning: map %d uses tile %d and there are %d tiles of %d x %d",
                     "aviso: el mapa %d usa el tile %d y hay %d tiles de %d x %d")
                  % (num, top, tiles.get(s, 0), s, s))


# --- Órdenes ---------------------------------------------------------------------------

ARGS = {"--sprite": (2, 3), "--sprites": (3, 3), "--tiles": (3, 4), "--level": (3, 3),
        "--map": (3, 3), "--texture": (2, 3), "--wrap": (1, 1)}


def main(argv):
    if len(argv) == 2 and argv[0] == "--list":
        list_gfx(argv[1])
        return 0
    if not argv or argv[0].startswith("-"):
        print(tr(HELP_EN, __doc__).strip())
        return 2
    out, rest = argv[0], argv[1:]
    flip, key, orders = False, None, []
    i = 0
    while i < len(rest):
        a = rest[i]
        if a == "--flip":
            flip = True
            i += 1
        elif a == "--key":
            if i + 1 >= len(rest) or len(rest[i + 1]) != 6:
                die("--key RRGGBB")
            try:
                v = int(rest[i + 1], 16)
            except ValueError:
                die("--key RRGGBB")
            key = (v >> 16, (v >> 8) & 255, v & 255)
            i += 2
        elif a in ARGS:
            lo, hi = ARGS[a]
            j = i + 1
            while j < len(rest) and j - i - 1 < hi and not rest[j].startswith("--"):
                j += 1
            if j - i - 1 < lo:
                die(tr("%s: missing arguments", "%s: faltan argumentos") % a)
            orders.append((a, rest[i + 1:j]))
            i = j
        else:
            die(tr("unknown command: %s", "orden desconocida: %s") % a)
    if not orders:
        die(tr("nothing to convert", "no hay nada que convertir"))

    st = Store()
    images = {}

    def image(path):
        if path not in images:
            images[path] = Image(path, key)
        return images[path]

    wraps = []
    for cmd, a in orders:
        if cmd == "--sprite":
            n = number(a[0], 0, 255, "sprite")
            img = image(a[1])
            x, y, w, h = ints(a[2], 4, cmd) if len(a) > 2 else (0, 0, img.w, img.h)
            put_sprite(st, n, img.region(x, y, w, h), w, h)
        elif cmd == "--sprites":
            n = number(a[0], 0, 255, "sprite")
            img = image(a[1])
            w, h = ints(a[2], 2, cmd)
            if w < 1 or h < 1 or img.w % w or img.h % h:
                die(tr("%s: %d x %d is not a multiple of %d x %d",
                       "%s: %d x %d no es múltiplo de %d x %d") % (img.path, img.w, img.h, w, h))
            for sy in range(img.h // h):
                for sx in range(img.w // w):
                    if n > 255:
                        die(tr("%s: more sprites than fit (up to 255)",
                               "%s: más sprites de los que caben (hasta el 255)") % img.path)
                    put_sprite(st, n, img.region(sx * w, sy * h, w, h), w, h)
                    n += 1
        elif cmd == "--tiles":
            s = tile_size(a[0])
            n = number(a[1], 0, 4095, "tile")
            img = image(a[2])
            x, y, w, h = ints(a[3], 4, cmd) if len(a) > 3 else (0, 0, img.w, img.h)
            for t in cut_tiles(img, s, x, y, w, h):
                if n > 4095:
                    die(tr("%s: more than 4096 tiles", "%s: más de 4096 tiles") % img.path)
                st.tiles[s][n] = t
                n += 1
        elif cmd == "--level":
            m = number(a[0], 0, 15, tr("map", "mapa"))
            level(st, m, tile_size(a[1]), image(a[2]), flip)
        elif cmd == "--map":
            m = number(a[0], 0, 15, tr("map", "mapa"))
            rows = read_csv(a[2])
            st.maps[m] = [len(rows[0]), len(rows), tile_size(a[1]), 0, [v for r in rows for v in r]]
        elif cmd == "--texture":
            n = number(a[0], 0, 3, tr("texture", "textura"))
            img = image(a[1])
            if img.w != img.h or img.w not in [1 << k for k in range(1, 9)]:
                die(tr("%s: a texture is a square, its side a power of 2 from 2 to 256",
                       "%s: una textura es un cuadrado de lado potencia de 2, de 2 a 256") % img.path)
            out_c = number(a[2], 0, 15, tr("colour", "color")) if len(a) > 2 else None
            texels = [0 if c == TRANSPARENT else c for row in img.pix for c in row]
            st.textures[n] = (img.w.bit_length() - 1, 1 if out_c is None else 0, out_c or 0, texels)
        elif cmd == "--wrap":
            wraps.append(number(a[0], 0, 15, tr("map", "mapa")))
    for m in wraps:
        if m not in st.maps:
            die(tr("--wrap %d: there is no map %d", "--wrap %d: no hay mapa %d") % (m, m))
        st.maps[m][3] = 1
    for m, (w, h, s, _, cells) in sorted(st.maps.items()):
        if not (1 <= w <= 32767 and 1 <= h <= 32767):
            die(tr("map %d: sides from 1 to 32767 cells", "mapa %d: de 1 a 32767 celdas de lado") % m)
        top = max(c & 0xFFF for c in cells)
        if st.tile_count(s) <= top:
            print(tr("png2gfx: warning: map %d uses tile %d and there are %d tiles of %d x %d",
                     "png2gfx: aviso: el mapa %d usa el tile %d y hay %d tiles de %d x %d")
                  % (m, top, st.tile_count(s), s, s), file=sys.stderr)
    data = st.serialize()
    try:
        open(out, "wb").write(data)
    except OSError as e:
        die("%s: %s" % (out, e.strerror))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
