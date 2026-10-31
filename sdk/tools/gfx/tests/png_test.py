#!/usr/bin/env python3
"""
Importar imágenes en zxv-gfx da lo mismo que sdk/tools/png2gfx.py: con unas
PNG hechas aquí (una hoja de sprites con transparentes y colores que no son
de ZX-V, tiles, un nivel con tiles repetidos, espejados y transparentes, y
una textura y pantallas de los dos modos, con su borde o sin él, con los
colores de siempre o con otros, que llevan paleta, D90), png_test y
png2gfx.py, con las mismas órdenes, tienen que dar el mismo .gfx byte a
byte. Y una pantalla de más de 16 colores no la importa ninguno de los dos.

    png_test.py PNG_TEST CARPETA_TEMPORAL
"""
import os
import random
import struct
import subprocess
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
PNG2GFX = os.path.join(HERE, "..", "..", "png2gfx.py")
test, tmp = sys.argv[1], sys.argv[2]
os.makedirs(tmp, exist_ok=True)


def palette(i):
    lvl = 0xFF if i & 8 else 0xD7
    return ((lvl if i & 2 else 0), (lvl if i & 4 else 0), (lvl if i & 1 else 0))


def write_png(path, rows):
    """rows: filas de (r, g, b, a)."""
    h, w = len(rows), len(rows[0])
    raw = b"".join(b"\0" + bytes(c for p in row for c in p) for row in rows)

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    data = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))
    open(path, "wb").write(data)


r = random.Random(5)


def pixel(transp=0.1, inexact=0.05):
    k = r.random()
    if k < transp:
        return (0, 0, 0, 0)
    if k < transp + inexact:
        return (r.randrange(256), r.randrange(256), r.randrange(256), 255)
    return palette(r.choice([i for i in range(16) if i != 8])) + (255,)


def image(w, h, **kw):
    return [[pixel(**kw) for _ in range(w)] for _ in range(h)]


def tile(s, transp):
    return [[pixel(transp=transp, inexact=0) for _ in range(s)] for _ in range(s)]


def level(s, cols, rows):
    """Un nivel con pocos tiles distintos, espejados a veces."""
    base = [tile(s, 0.3 if i % 3 == 0 else 0) for i in range(5)]
    img = [[None] * (cols * s) for _ in range(rows * s)]
    for ty in range(rows):
        for tx in range(cols):
            t = base[r.randrange(len(base))]
            if r.random() < 0.3:
                t = [list(reversed(row)) for row in t]
            if r.random() < 0.3:
                t = list(reversed(t))
            for y in range(s):
                for x in range(s):
                    img[ty * s + y][tx * s + x] = t[y][x]
    return img


# Colores para las pantallas con paleta: unos de los de siempre y otros que
# no (dos que dan el mismo RGB565 cuentan como uno).
OWN = [(0x80, 0x40, 0x20), (0x12, 0x34, 0x56), (0xCD, 0, 0), (0xCC, 0, 0), (0xC0, 0xC0, 0xC0), (0x55, 0xAA, 0xFF)]


def own(cols, transp=0.0):
    k = r.random()
    if k < transp:
        return (0, 0, 0, 0)
    return r.choice(cols) + (255,)


STD = [palette(i) for i in (0, 2, 7, 13, 15)]
files = {
    "sheet.png": image(64, 32),
    "one.png": image(20, 13, transp=0.2),
    "tiles.png": image(32, 16, transp=0, inexact=0.1),
    "level8.png": level(8, 10, 6),
    "level16.png": level(16, 6, 4),
    "tex.png": image(32, 32, transp=0.05),
    "key.png": [[(255, 0, 255, 255) if (x + y) % 5 == 0 else palette(4) + (255,) for x in range(12)]
                for y in range(9)],
    "screen0.png": image(256, 192, transp=0.02, inexact=0),
    "screen1.png": image(512, 384, transp=0, inexact=0),
    "frame0.png": [[palette(2) + (255,) if x < 32 or x >= 288 or y < 24 or y >= 216 else pixel(inexact=0)
                    for x in range(320)] for y in range(240)],
    "pal0.png": [[own(STD + OWN, 0.03) for x in range(256)] for y in range(192)],
    "pal1.png": [[(0x40, 0x80, 0x40, 255) if x < 64 or x >= 576 or y < 48 or y >= 432 else own(STD[1:] + OWN[:3])
                  for x in range(640)] for y in range(480)],
    "pal16.png": [[(16 * k + 3, 200 - 9 * k, 40 + 5 * k, 255) for k in ((x // 16 + y // 8) % 16 for x in range(256))]
                  for y in range(192)],
    "many.png": [[(x, y, 9, 255) for x in range(256)] for y in range(192)],
}
for name, rows in files.items():
    write_png(os.path.join(tmp, name), rows)

cases = [
    ["--sprites", "0", "sheet.png", "16,16", "--sprite", "20", "one.png", "--sprite", "21", "one.png", "3,2,10,8"],
    ["--tiles", "8", "3", "tiles.png", "--tiles", "16", "0", "tiles.png", "0,0,32,16"],
    ["--level", "2", "8", "level8.png", "--level", "5", "16", "level16.png"],
    ["--flip", "--tiles", "8", "0", "tiles.png", "--level", "0", "8", "level8.png", "--level", "1", "16", "level16.png"],
    ["--texture", "1", "tex.png", "--texture", "2", "tex.png", "5"],
    ["--key", "FF00FF", "--sprite", "0", "key.png"],
    ["--screen", "0", "screen0.png", "--screen", "7", "screen1.png", "12", "--screen", "3", "frame0.png"],
    ["--sprite", "1", "one.png", "--screen", "255", "frame0.png", "9"],
    ["--screen", "0", "pal0.png", "--screen", "1", "pal1.png", "--screen", "3", "pal1.png", "8"],
    ["--screen", "2", "pal16.png"],
    ["--key", "C0C0C0", "--screen", "4", "pal0.png"],
]
bad = 0
for i, args in enumerate(cases):
    want, got = os.path.join(tmp, "want%d.gfx" % i), os.path.join(tmp, "got%d.gfx" % i)
    paths = [os.path.join(tmp, a) if a.endswith(".png") else a for a in args]
    subprocess.run([sys.executable, PNG2GFX, want] + paths, check=True, stderr=subprocess.DEVNULL)
    res = subprocess.run([test, got] + paths)
    if res.returncode or open(want, "rb").read() != open(got, "rb").read():
        bad += 1
        print("distinto: %s" % " ".join(args))
# Más de 16 colores: ninguno de los dos la importa.
want, got = os.path.join(tmp, "want-many.gfx"), os.path.join(tmp, "got-many.gfx")
res = subprocess.run([sys.executable, PNG2GFX, want, "--screen", "0", os.path.join(tmp, "many.png")],
                     stderr=subprocess.DEVNULL)
res2 = subprocess.run([test, got, "--screen", "0", os.path.join(tmp, "many.png")], stderr=subprocess.DEVNULL)
if res.returncode == 0 or res2.returncode == 0 or os.path.exists(want) or os.path.exists(got):
    bad += 1
    print("más de 16 colores: se importa (%d, %d)" % (res.returncode, res2.returncode))
print("todo bien" if not bad else "%d fallos" % bad)
sys.exit(1 if bad else 0)
