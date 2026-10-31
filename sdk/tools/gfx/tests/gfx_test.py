#!/usr/bin/env python3
"""
El fichero .gfx de zxv-gfx (sdk/tools/gfx/doc.cpp): ficheros al
azar hechos con png2gfx.Store (sprites de ancho par e impar, los dos juegos
de tiles, mapas y texturas) y pantallas de los dos modos, con paleta o sin
ella (tipo 5, detrás de lo demás, como las graba png2gfx.py) se cargan y se
vuelven a grabar con doc.cpp, y tienen que salir iguales byte a byte;
también una pantalla sola, como la de SAVE "x" SCREEN$. Y un fichero dañado
se rechaza.

    gfx_test.py DOC_TEST CARPETA_TEMPORAL
"""
import os
import random
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", ".."))
import png2gfx  # noqa: E402

test, tmp = sys.argv[1], sys.argv[2]
os.makedirs(tmp, exist_ok=True)
bad = 0


def store(seed):
    r = random.Random(seed)
    st = png2gfx.Store()
    for n in r.sample(range(256), r.randrange(1, 12)):
        w, h = r.randrange(1, 40), r.randrange(1, 40)
        st.sprites[n] = (w, h, [[r.randrange(16) for _ in range(w)] for _ in range(h)])
    for s in (8, 16):
        if r.random() < 0.7:
            for i in r.sample(range(40), r.randrange(1, 10)):
                st.tiles[s][i] = tuple(tuple(r.randrange(16) for _ in range(s)) for _ in range(s))
    for m in r.sample(range(16), r.randrange(0, 4)):
        w, h = r.randrange(1, 50), r.randrange(1, 30)
        st.maps[m] = [w, h, r.choice((8, 16)), r.randrange(2), [r.randrange(65536) for _ in range(w * h)]]
    for t in r.sample(range(4), r.randrange(0, 3)):
        l2 = r.randrange(1, 7)
        st.textures[t] = (l2, r.randrange(2), r.randrange(16), [r.randrange(16) for _ in range(1 << 2 * l2)])
    return with_screens(st.serialize(), r)


def screen(r, mode, border, pal=False):
    """Un objeto pantalla: modo, borde, marcas, 0, la paleta si la lleva (D90)
    y la página como en la VRAM."""
    size = 98304 if mode else 24576
    return bytes([mode, border, 1 if pal else 0, 0]) + (r.randbytes(32) if pal else b"") + r.randbytes(size)


def with_screens(data, r, screens=None):
    """Añade pantallas al final del fichero (y las cuenta en la cabecera)."""
    if screens is None:
        screens = {n: screen(r, r.randrange(2), r.randrange(16), r.random() < 0.5)
                   for n in r.sample(range(256), r.randrange(0, 3))}
    count, = struct.unpack_from("<H", data, 6)
    out = data[:6] + struct.pack("<H", count + len(screens)) + data[8:]
    for n in sorted(screens):
        out += struct.pack("<BBHI", 5, 0, n, len(screens[n])) + screens[n]
    return out


EMPTY = b"ZXVG" + struct.pack("<BBH", 1, 0, 0)

for seed in range(40):
    data = store(seed)
    src, dst = os.path.join(tmp, "in.gfx"), os.path.join(tmp, "out.gfx")
    open(src, "wb").write(data)
    r = subprocess.run([test, src, dst])
    out = open(dst, "rb").read() if r.returncode == 0 else b""
    if out != data:
        bad += 1
        print("semilla %d: distinto (%d bytes, se esperaban %d)" % (seed, len(out), len(data)))

# Una pantalla sola, la 0, como la de SAVE "x" SCREEN$.
src, dst = os.path.join(tmp, "in.gfx"), os.path.join(tmp, "out.gfx")
for mode in (0, 1):
    for pal in (False, True):
        data = with_screens(EMPTY, None, {0: screen(random.Random(mode), mode, 7, pal)})
        open(src, "wb").write(data)
        r = subprocess.run([test, src, dst])
        if r.returncode or open(dst, "rb").read() != data:
            bad += 1
            print("pantalla sola de modo %d%s: distinta" % (mode, " con paleta" if pal else ""))

# Dañado: cortado, otra versión, un tipo que no existe, pantallas mal.
good = store(7)
scr = with_screens(EMPTY, None, {0: screen(random.Random(1), 0, 3)})
pscr = with_screens(EMPTY, None, {0: screen(random.Random(1), 0, 3, True)})
for name, data in (("cortado", good[:-3]), ("versión", good[:4] + b"\x02" + good[5:]),
                   ("tipo", good[:8] + b"\x09" + good[9:]), ("pantalla corta", scr[:-1]),
                   ("modo de pantalla", scr[:16] + b"\x01" + scr[17:]),
                   ("borde de pantalla", scr[:17] + b"\x10" + scr[18:]),
                   ("paleta sin la marca", pscr[:18] + b"\x00" + pscr[19:]),
                   ("marca sin la paleta", scr[:18] + b"\x01" + scr[19:]),
                   ("marca que no existe", pscr[:18] + b"\x03" + pscr[19:])):
    src = os.path.join(tmp, "bad.gfx")
    open(src, "wb").write(data)
    r = subprocess.run([test, src, os.path.join(tmp, "x.gfx")], stdout=subprocess.DEVNULL)
    if r.returncode != 1:
        bad += 1
        print("%s: no lo rechaza" % name)

print("%s" % ("todo bien" if not bad else "%d fallos" % bad))
sys.exit(1 if bad else 0)
