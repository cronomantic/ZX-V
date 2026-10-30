#!/usr/bin/env python3
"""
El módulo de zxv-tracker con los módulos de prueba de sdk/music
(emulator/tests/music/mkmodules.py): leídos y guardados otra vez, las notas
y los volúmenes suenan igual que los originales (tracker_song_test los toca
con sdk/music), y todos, leídos otra vez, dan lo mismo. Los rand_ y ts_
juntan órdenes que el editor no tiene (una envolvente que se quita en la
misma fila...): esos no suenan igual.

    pt3_test.py TRACKER_SONG_TEST MKMODULES.PY CARPETA_TEMPORAL
"""
import glob
import os
import subprocess
import sys

test, mkmodules, tmp = sys.argv[1:4]
subprocess.run([sys.executable, mkmodules, tmp], check=True, stdout=subprocess.DEVNULL)
mods = sorted(glob.glob(os.path.join(tmp, "*.pt3")))
exact = [m for m in mods if os.path.basename(m).startswith(("note_", "vol_"))]
stable = [m for m in mods if m not in exact]
bad = subprocess.run([test] + exact).returncode
bad |= subprocess.run([test, "--estable"] + stable).returncode
print("%d módulos: %s" % (len(mods), "bien" if not bad else "MAL"))
sys.exit(1 if bad else 0)
