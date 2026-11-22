#!/usr/bin/env python3
"""
La ventana de zxv-tracker, sin pantalla (QT_QPA_PLATFORM=offscreen, sin
sonido): abre un módulo de prueba de sdk/music y un banco de efectos y se
guarda la ventana en cada pestaña, en español y en inglés. Y --wav toca el
módulo una vez a un WAV: lo que dura tiene que ser lo de una vuelta.

    window_test.py ZXV_TRACKER CARPETA_TEMPORAL
"""
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "..", "emulator", "tests", "music"))
import mkmodules  # noqa: E402

prog, tmp = sys.argv[1], sys.argv[2]
os.makedirs(tmp, exist_ok=True)
mods = mkmodules.modules()
pt3 = os.path.join(tmp, "prueba.pt3")
open(pt3, "wb").write(mods["rand_005.pt3"])
ts = os.path.join(tmp, "turbo.pt3")
open(ts, "wb").write(mods["ts_000.pt3"])

# Un banco con dos efectos: un tono que baja y un ruido.
fx1 = bytearray()
for i in range(10):
    fx1 += bytes([0x20 | 0x80 | (15 - i), (0x80 + i * 16) & 0xFF, (0x80 + i * 16) >> 8])
fx1 += b"\xd0\x20" + b"baja\0"
fx2 = bytearray([0x40 | 0x10 | 15, 5]) + bytes([0x10 | (15 - i) for i in range(1, 12)]) + b"\xd0\x20" + b"ruido\0"
afb = bytearray([2, 0, 0, 0, 0]) + fx1 + fx2
struct.pack_into("<H", afb, 1, 5 - 2)
struct.pack_into("<H", afb, 3, 5 + len(fx1) - 4)
bank = os.path.join(tmp, "efectos.afb")
open(bank, "wb").write(afb)

env = dict(os.environ, QT_QPA_PLATFORM="offscreen", SDL_AUDIODRIVER="dummy", ZXV_HOME=tmp)
bad = 0
for lang in ("es", "en"):
    for tab, f in ((0, pt3), (0, ts), (1, pt3), (2, pt3), (3, pt3), (4, pt3), (5, bank)):
        shot = os.path.join(tmp, "ventana-%s-%d-%s.png" % (lang, tab, os.path.basename(f)[:-4]))
        if os.path.exists(shot):
            os.remove(shot)
        r = subprocess.run([prog, "--lang", lang, "--screenshot", shot, "--tab", str(tab), f], env=env, timeout=60)
        if r.returncode or not os.path.exists(shot) or os.path.getsize(shot) < 2000:
            bad += 1
            print("%s, pestaña %d, %s: no se ha guardado la ventana (%d)" % (lang, tab, f, r.returncode))

# El WAV: 48 kHz, estéreo, una vuelta del módulo (lo que dice sdk/music).
wav = os.path.join(tmp, "prueba.wav")
r = subprocess.run([prog, "--wav", wav, pt3], env=env, timeout=120)
if r.returncode or not os.path.exists(wav):
    bad += 1
    print("--wav: no se ha guardado (%d)" % r.returncode)
else:
    data = open(wav, "rb").read()
    rate, = struct.unpack_from("<I", data, 24)
    frames = (len(data) - 44) // 4
    loud = any(struct.unpack_from("<h", data, 44 + i * 2)[0] for i in range(0, min(len(data) - 44, 400000) // 2, 7))
    print("WAV: %d Hz, %.1f s" % (rate, frames / rate))
    if rate != 48000 or frames < 48000 or not loud:
        bad += 1
        print("--wav: no es lo que tiene que ser")
# Un MOD de 16 canales (más de los 12 de ZX-V), una nota en cada uno: sin
# nadie delante (--wav, --screenshot) no pregunta los canales; --channels
# los elige, y de más de 12 no vale.
mod = bytearray(1084 + 64 * 16 * 4)
mod[0:6] = b"canales"[:6]
struct.pack_into(">HBB", mod, 20 + 22, 50, 0, 64)      # muestra 1: 100 bytes, volumen 64
struct.pack_into(">HH", mod, 20 + 26, 0, 1)
mod[950], mod[951] = 1, 127
mod[1080:1084] = b"16CH"
for c in range(16):
    struct.pack_into(">BBBB", mod, 1084 + c * 4, 0x01, 0xAC, 0x10, 0)   # C-2 con la muestra 1
mod += bytes((i * 37) & 0xFF for i in range(100))
mod16 = os.path.join(tmp, "canales.mod")
open(mod16, "wb").write(mod)
for args, want in ((["--wav", os.path.join(tmp, "canales.wav")], 0),
                   (["--wav", os.path.join(tmp, "canales4.wav"), "--channels", "1-4"], 0),
                   (["--screenshot", os.path.join(tmp, "canales.png")], 0),
                   (["--wav", os.path.join(tmp, "canales13.wav"), "--channels", "1-13"], 2)):
    try:
        r = subprocess.run([prog] + args + [mod16], env=env, timeout=60)
        rc = r.returncode
    except subprocess.TimeoutExpired:
        rc = "se queda esperando"
    if rc != want:
        bad += 1
        print("MOD de 16 canales, %s: %s (y tenía que ser %d)" % (" ".join(args[2:] or args[:1]), rc, want))
print("todo bien" if not bad else "%d fallos" % bad)
sys.exit(1 if bad else 0)
