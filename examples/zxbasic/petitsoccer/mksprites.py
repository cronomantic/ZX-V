#!/usr/bin/env python3
"""
mksprites.py - Los sprites de Petit Soccer: de la hoja sprites.png a
sprites.bas, lo que incluye el programa.

    python mksprites.py

La hoja se retoca con cualquier editor de imágenes; cada píxel toma el color
de ZX-V más cercano y lo transparente (alfa) no se dibuja. Los jugadores van
en casillas de 12 x 18, con los pies en la última fila, y el juego les cambia
tres colores a cada uno: el rojo (2) es su camiseta y sus medias, el azul (1)
su pantalón y el amarillo (6) su pelo. Debajo, lo pequeño: el balón en sus
tres posiciones, su sombra, la de un jugador y la flecha del que se maneja.
A la derecha, la portería, de pie y con su red.

Solo hace falta Python 3 (el PNG lo lee png2gfx.py, del SDK).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "sdk", "tools"))
from png2gfx import colour, read_png  # noqa: E402

CW, CH = 12, 18
# Los jugadores, en el orden de la hoja (diez por fila): el número de sprite
# es su posición. 0 es parado; 1 y 2, los dos pasos de la carrera.
PLAYERS = ["de frente 0", "de frente 1", "de frente 2",
           "de espaldas 0", "de espaldas 1", "de espaldas 2",
           "de lado 0", "de lado 1", "de lado 2",
           "en diagonal, de frente 0", "en diagonal, de frente 1", "en diagonal, de frente 2",
           "en diagonal, de espaldas 0", "en diagonal, de espaldas 1", "en diagonal, de espaldas 2",
           "entrada, de lado", "entrada, de frente", "entrada, de espaldas",
           "en el suelo", "el portero, estirado"]
# Lo pequeño: nombre, x, y, ancho y alto en la hoja
SMALL = [("el balón 0", 0, 38, 6, 6), ("el balón 1", 8, 38, 6, 6), ("el balón 2", 16, 38, 6, 6),
         ("la sombra del balón", 24, 38, 6, 3), ("la sombra de un jugador", 32, 38, 10, 3),
         ("la flecha", 44, 38, 5, 3),
         # La portería de la derecha (la otra es su espejo): los postes y el
         # larguero en sus dos primeras columnas, y la red, que baja hacia
         # atrás. Mide 2 * GH + BAR + 1 de alto, las medidas del programa.
         ("la portería", 121, 0, 18, 95)]


def main():
    w, h, rows = read_png(os.path.join(HERE, "sprites.png"))
    cells = [(name, (i % 10) * CW, (i // 10) * CH, CW, CH) for i, name in enumerate(PLAYERS)] + SMALL
    out = ["REM Los sprites de Petit Soccer. Lo escribe mksprites.py desde sprites.png:",
           "REM no lo cambies a mano.",
           "SUB LoadSprites()"]
    for n, (name, x0, y0, cw, ch) in enumerate(cells):
        if x0 + cw > w or y0 + ch > h:
            sys.exit("sprites.png es pequeño para «%s»" % name)
        out.append("    REM %d: %s" % (n, name))
        out.append("    GRABSTR %d, %d, _" % (n, cw))
        for y in range(ch):
            line = ""
            for x in range(cw):
                c = colour(rows[y0 + y][x0 + x], None)[0]
                line += "." if c == 8 else "%x" % c
            out.append('        "%s"%s' % (line, " + _" if y < ch - 1 else ""))
    out.append("END SUB")
    with open(os.path.join(HERE, "sprites.bas"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out) + "\n")
    print("sprites.bas: %d sprites" % len(cells))


if __name__ == "__main__":
    main()
