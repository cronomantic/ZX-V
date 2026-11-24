#!/usr/bin/env python3
"""
Los fondos de las dos fases del ejemplo fases (256 x 96, 4 bits por píxel,
como en la VRAM: el píxel par en el nibble bajo): el valle (el cielo, el sol
y dos filas de montañas) y el espacio (estrellas y un planeta con anillo).
Van en el .zxv como bancos de datos.

    mkfondos.py FONDO1.bin FONDO2.bin [--png CARPETA]

Con --png, también en PNG con la paleta del Spectrum, para verlos.
"""
import math
import random
import sys

W, H = 256, 96
# La paleta de siempre (los 16 colores del Spectrum), para --png.
PAL = [(0, 0, 0), (0, 0, 0xD7), (0xD7, 0, 0), (0xD7, 0, 0xD7), (0, 0xD7, 0), (0, 0xD7, 0xD7), (0xD7, 0xD7, 0),
       (0xD7, 0xD7, 0xD7), (0, 0, 0), (0, 0, 0xFF), (0xFF, 0, 0), (0xFF, 0, 0xFF), (0, 0xFF, 0), (0, 0xFF, 0xFF),
       (0xFF, 0xFF, 0), (0xFF, 0xFF, 0xFF)]


def valley():
    p = [[0] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            # El cielo, en bandas que se tramean en el cambio.
            band = 13 if y < 34 else 5
            if y in (32, 33) and (x + y) % 2:
                band = 5
            p[y][x] = band
    for y in range(H):                  # el sol
        for x in range(W):
            if (x - 196) ** 2 + (y - 22) ** 2 <= 13 ** 2:
                p[y][x] = 14
    for x in range(W):                  # las montañas del fondo y las de delante
        far = int(50 + 14 * math.sin(x / 23.0) + 7 * math.sin(x / 9.0 + 1))
        near = int(68 + 10 * math.sin(x / 31.0 + 2) + 4 * math.sin(x / 7.0))
        for y in range(far, H):
            p[y][x] = 1
        for y in range(far, min(far + 3, H)):
            p[y][x] = 15 if (x // 3) % 2 == 0 and far < 46 else 1     # nieve en los picos
        for y in range(near, H):
            p[y][x] = 4 if (x + y) % 2 else 12
    return p


def space():
    rnd = random.Random(7)
    p = [[0] * W for _ in range(H)]
    for _ in range(140):                # estrellas
        x, y = rnd.randrange(W), rnd.randrange(H)
        p[y][x] = rnd.choice((7, 7, 15, 13))
    cx, cy, r = 70, 50, 26              # el planeta, con su luz desde la derecha
    for y in range(H):
        for x in range(W):
            d = (x - cx) ** 2 + (y - cy) ** 2
            if d <= r * r:
                lit = (x - cx) * 0.7 - (y - cy) * 0.3
                p[y][x] = 10 if lit > 6 else 2 if lit > -10 or (x + y) % 2 else 0
    for t in range(720):                # el anillo, por delante y por detrás
        a = t * math.pi / 360
        x, y = int(cx + 44 * math.cos(a)), int(cy + 9 * math.sin(a))
        behind = math.sin(a) < 0 and (x - cx) ** 2 + (y - cy) ** 2 <= r * r
        if 0 <= x < W and 0 <= y < H and not behind:
            p[y][x] = 14
    for y in range(H):                  # una luna pequeña
        for x in range(W):
            if (x - 200) ** 2 + (y - 30) ** 2 <= 6 ** 2:
                p[y][x] = 7 if x < 202 else 15
    return p


def pack(p):
    out = bytearray()
    for row in p:
        for x in range(0, W, 2):
            out.append(row[x] | row[x + 1] << 4)
    return bytes(out)


def main():
    args = sys.argv[1:]
    png = None
    if "--png" in args:
        i = args.index("--png")
        png = args[i + 1]
        del args[i:i + 2]
    if len(args) != 2:
        sys.exit(__doc__)
    for path, pic in zip(args, (valley(), space())):
        with open(path, "wb") as f:
            f.write(pack(pic))
        if png:
            from PIL import Image
            im = Image.new("RGB", (W, H))
            im.putdata([PAL[c] for row in pic for c in row])
            im.resize((W * 2, H * 2), Image.NEAREST).save("%s/%s.png" % (png, path.replace("\\", "/").split("/")[-1]))


if __name__ == "__main__":
    main()
