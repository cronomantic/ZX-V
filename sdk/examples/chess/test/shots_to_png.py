#!/usr/bin/env python3
"""Convierte las capturas del arnés (PPM de 256x192 + texto) en PNG ampliados."""
import sys, glob, os
from PIL import Image, ImageDraw, ImageFont

PAL = [(0,0,0),(0,0,215),(215,0,0),(215,0,215),(0,215,0),(0,215,215),(215,215,0),(215,215,215),
       (0,0,0),(0,0,255),(255,0,0),(255,0,255),(0,255,0),(0,255,255),(255,255,0),(255,255,255)]
S = 3
font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 7 * S)

def convert(base):
    raw = open(base + ".ppm", "rb").read()
    img = Image.new("RGB", (256, 192))
    img.putdata([PAL[b & 15] for b in raw])
    img = img.resize((256 * S, 192 * S), Image.NEAREST)
    d = ImageDraw.Draw(img)
    for line in open(base + ".txt"):
        r, c, ink, cp = map(int, line.split())
        d.text((c * 8 * S + S // 2, r * 8 * S - S // 2), chr(cp), fill=PAL[ink & 15], font=font)
    img.save(base + ".png")

for ppm in sorted(glob.glob(os.path.join(sys.argv[1], "*.ppm"))):
    convert(ppm[:-4])
print("hecho")
