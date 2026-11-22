#!/usr/bin/env python3
"""
La canción de zxv-tracker (tune.cpp): la hecha en tune_test, y los módulos
de prueba de sdk/music (emulator/tests/music/mkmodules.py): cada PT3,
pasado a la canción, guardado como PT3 y como .zxm, y el .zxm leído otra
vez (ver tests/tune_test.cpp).

    tune_test.py TRACKER_TUNE_TEST MKMODULES.PY CARPETA_TEMPORAL
"""
import glob
import os
import subprocess
import sys

test, mkmodules, tmp = sys.argv[1:4]
subprocess.run([sys.executable, mkmodules, tmp], check=True, stdout=subprocess.DEVNULL)
mods = sorted(glob.glob(os.path.join(tmp, "*.pt3")))
bad = subprocess.run([test]).returncode
bad |= subprocess.run([test] + mods, stdout=subprocess.DEVNULL if os.environ.get("QUIET") else None).returncode
print("%d módulos: %s" % (len(mods), "bien" if not bad else "MAL"))
sys.exit(1 if bad else 0)
