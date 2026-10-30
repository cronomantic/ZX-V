#!/usr/bin/env python3
"""
Importar imágenes en zxv-gfx da lo mismo que sdk/tools/png2gfx.py: con unas
PNG hechas aquí (una hoja de sprites con transparentes y colores que no son
de ZX-V, tiles, un nivel con tiles repetidos, espejados y transparentes, y
una textura), png_test y png2gfx.py, con las mismas órdenes, tienen que dar
el mismo .gfx byte a byte.

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


files = {
    "sheet.png": image(64, 32),
    "one.png": image(20, 13, transp=0.2),
    "tiles.png": image(32, 16, transp=0, inexact=0.1),
    "level8.png": level(8, 10, 6),
    "level16.png": level(16, 6, 4),
    "tex.png": image(32, 32, transp=0.05),
    "key.png": [[(255, 0, 255, 255) if (x + y) % 5 == 0 else palette(4) + (255,) for x in range(12)]
                for y in range(9)],
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
print("todo bien" if not bad else "%d fallos" % bad)
sys.exit(1 if bad else 0)
