#!/usr/bin/env python3
"""
La ventana de zxv-gfx, sin pantalla (QT_QPA_PLATFORM=offscreen): abre un
.gfx con de todo (hecho con png2gfx.py) y se guarda la ventana en cada
pestaña. Tiene que abrirlo sin errores y dibujar algo en cada una, en
español y en inglés.

    window_test.py ZXV_GFX CARPETA_TEMPORAL
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", ".."))
import png2gfx  # noqa: E402

prog, tmp = sys.argv[1], sys.argv[2]
os.makedirs(tmp, exist_ok=True)
st = png2gfx.Store()
st.sprites[3] = (12, 10, [[(x + y) % 16 for x in range(12)] for y in range(10)])
st.tiles[8] = {i: tuple(tuple((i + x * y) % 16 for x in range(8)) for y in range(8)) for i in range(5)}
st.tiles[16] = {0: tuple(tuple((x ^ y) % 16 for x in range(16)) for y in range(16))}
st.maps[2] = [20, 12, 8, 0, [(x + y) % 5 | (0x1000 if x % 3 == 0 else 0) for y in range(12) for x in range(20)]]
st.textures[1] = (5, 1, 0, [(x // 4 + y // 4) % 16 for y in range(32) for x in range(32)])
gfx = os.path.join(tmp, "todo.gfx")
open(gfx, "wb").write(st.serialize())

env = dict(os.environ, QT_QPA_PLATFORM="offscreen", ZXV_HOME=tmp)
bad = 0
for lang in ("es", "en"):
    for section in range(5):
        shot = os.path.join(tmp, "ventana-%s-%d.png" % (lang, section))
        if os.path.exists(shot):
            os.remove(shot)
        r = subprocess.run([prog, "--lang", lang, "--screenshot", shot, "--section", str(section), gfx], env=env,
                           timeout=60)
        if r.returncode or not os.path.exists(shot) or os.path.getsize(shot) < 2000:
            bad += 1
            print("%s, pestaña %d: no se ha guardado la ventana (%d)" % (lang, section, r.returncode))
print("todo bien" if not bad else "%d fallos" % bad)
sys.exit(1 if bad else 0)
