#!/usr/bin/env python3
"""
Prepara un PNG sacado de un editor de imágenes para usarlo como gráfico del juego.

    python3 import_png.py ENTRADA.png [SALIDA.png] [--fondo RRGGBB] [--celda N] [--tolerancia T]

Editores como Photoshop guardan los PNG indexados sin transparencia y con colores
a veces desviados un punto (D8 en vez de D7). Este script lo arregla:

  * El FONDO pasa a transparente. Es el color del píxel de arriba a la izquierda
    (o el de --fondo) y solo se borra el que está conectado con el borde de cada
    casilla: ese mismo color DENTRO de una pieza se conserva como dibujo. Si el PNG
    ya trae transparencia, se respeta y no se busca fondo.
  * Cada color se ajusta al más cercano de los 15 de ZX-V. Si alguno estaba a más
    de --tolerancia (12 por defecto) en algún componente, avisa de cuál y cuántos.

Las casillas son de 24 x 24 en pieces.png (--celda 24, que se usa si el tamaño lo
permite). Para una imagen suelta, como el puntero, se trata entera (--celda 0).

Si no se da SALIDA, se escribe junto a la ENTRADA como <nombre>_zxv.png.
"""
import argparse
import os
import struct
import sys
import zlib
from collections import Counter, deque

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "tools"))
sys.path.insert(0, HERE)

import png2gfx                      # el lector de PNG del SDK (sin librerías externas)


# Los 16 colores de ZX-V: el 8 (negro brillante) es el transparente.
def zxv_color(i):
    lvl = 0xFF if i & 8 else 0xD7
    return (lvl if i & 2 else 0, lvl if i & 4 else 0, lvl if i & 1 else 0)


TRANSPARENT = 8
PALETTE = [(i, zxv_color(i)) for i in range(16) if i != TRANSPARENT]


def write_png(path, rows):
    """rows: lista de filas de índices de color de ZX-V. RGBA de 8 bits, sin entrelazar."""
    h, w = len(rows), len(rows[0])
    raw = bytearray()
    for row in rows:
        raw.append(0)                           # filtro: ninguno
        for c in row:
            if c == TRANSPARENT:
                raw += bytes((0, 0, 0, 0))
            else:
                raw += bytes(zxv_color(c)) + b"\xff"

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def nearest(rgb):
    best = min(PALETTE, key=lambda p: sum((p[1][k] - rgb[k]) ** 2 for k in range(3)))
    return best[0], max(abs(best[1][k] - rgb[k]) for k in range(3))


def main():
    ap = argparse.ArgumentParser(description="Prepara un PNG de un editor para usarlo en el juego")
    ap.add_argument("entrada")
    ap.add_argument("salida", nargs="?")
    ap.add_argument("--fondo", help="color del fondo, RRGGBB (por defecto, el del píxel de arriba a la izquierda)")
    ap.add_argument("--celda", type=int, help="tamaño de casilla; 0 = la imagen entera")
    ap.add_argument("--tolerancia", type=int, default=12)
    a = ap.parse_args()

    w, h, rows = png2gfx.read_png(a.entrada)
    cell = a.celda if a.celda is not None else (24 if w % 24 == 0 and h % 24 == 0 else 0)
    cw, ch = (cell, cell) if cell else (w, h)
    if w % cw or h % ch:
        raise SystemExit("import_png: %dx%d no es múltiplo de la casilla de %dx%d" % (w, h, cw, ch))

    has_alpha = any(p[3] < 128 for row in rows for p in row)
    bg = None
    if not has_alpha:
        if a.fondo:
            v = int(a.fondo, 16)
            bg = ((v >> 16) & 255, (v >> 8) & 255, v & 255)
        else:
            bg = rows[0][0][:3]
        print("fondo: %02X%02X%02X (se borra solo el conectado con el borde de cada casilla)" % bg)
    else:
        print("el PNG ya trae transparencia: se respeta")

    out = [[None] * w for _ in range(h)]
    made_transparent = kept_inside = 0
    if bg:
        for oy in range(0, h, ch):
            for ox in range(0, w, cw):
                seen = set()
                q = deque()
                for i in range(max(cw, ch)):
                    for x, y in ((i, 0), (i, ch - 1), (0, i), (cw - 1, i)):
                        if x < cw and y < ch and (x, y) not in seen and rows[oy + y][ox + x][:3] == bg:
                            seen.add((x, y))
                            q.append((x, y))
                while q:
                    x, y = q.popleft()
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < cw and 0 <= ny < ch and (nx, ny) not in seen \
                                and rows[oy + ny][ox + nx][:3] == bg:
                            seen.add((nx, ny))
                            q.append((nx, ny))
                for x, y in seen:
                    out[oy + y][ox + x] = TRANSPARENT
                made_transparent += len(seen)
                inside = sum(1 for y in range(ch) for x in range(cw)
                             if rows[oy + y][ox + x][:3] == bg and (x, y) not in seen)
                kept_inside += inside

    snapped = Counter()
    worst = Counter()
    for y in range(h):
        for x in range(w):
            if out[y][x] is not None:
                continue
            r, g, b, al = rows[y][x]
            if al < 128:
                out[y][x] = TRANSPARENT
                continue
            idx, dist = nearest((r, g, b))
            out[y][x] = idx
            if dist:
                snapped[(r, g, b)] += 1
                worst[(r, g, b)] = dist

    if bg:
        print("píxeles de fondo pasados a transparente: %d" % made_transparent)
        if kept_inside:
            print("píxeles de ese mismo color conservados dentro de las piezas: %d" % kept_inside)
    far = [(c, n) for c, n in snapped.items() if worst[c] > a.tolerancia]
    near = sum(n for c, n in snapped.items() if worst[c] <= a.tolerancia)
    if near:
        print("píxeles con colores ajustados a la paleta (desvío pequeño): %d" % near)
    for (r, g, b), n in far:
        print("AVISO: %d píxeles de color %02X%02X%02X no están en la paleta de ZX-V (desvío %d); "
              "se usa el más cercano" % (n, r, g, b, worst[(r, g, b)]))

    dst = a.salida or os.path.splitext(a.entrada)[0] + "_zxv.png"
    write_png(dst, out)
    used = sorted({c for row in out for c in row})
    print("escrito %s (%dx%d), colores usados: %s" % (dst, w, h, ", ".join(
        "transparente" if c == TRANSPARENT else "%d (%02X%02X%02X)" % ((c,) + zxv_color(c)) for c in used)))


if __name__ == "__main__":
    main()
