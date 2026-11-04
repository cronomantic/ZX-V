#!/usr/bin/env python3
"""
mkitems.py - Los objetos que se cogen en Gone Silent, en pixel art de 24 x 24:
la tarjeta de acceso, el filtro del traje, el cortador y las cuatro piezas
del módulo (la tobera, la placa de control, la célula de energía y la válvula
de combustible). Los dibuja con rectángulos y líneas, y les pone el contorno
negro. mkcreature.py los mete en gonesilent.gfx, desde el sprite 36, en ese
orden.

Los colores son los de la paleta de la vista que no cambian de una zona a
otra: 0 negro, 5-7 metal (de oscuro a claro; el 8 es lo transparente), 9-10
naranja, 11 cian, 12 el verde que brilla y 15 la luz.
"""
import numpy as np

W = H = 24
CLEAR = 8
BLACK, M5, M6, M7, O9, O10, CYAN, GREEN, LIGHT = 0, 5, 6, 7, 9, 10, 11, 12, 15


class Canvas:
    def __init__(self):
        self.p = np.full((H, W), CLEAR, dtype=np.uint8)

    def rect(self, x0, y0, x1, y1, c):
        """Lleno, con las dos esquinas dentro."""
        self.p[y0:y1 + 1, x0:x1 + 1] = c

    def px(self, x, y, c):
        self.p[y, x] = c

    def outline(self):
        """El contorno negro alrededor de lo que no es transparente."""
        solid = self.p != CLEAR
        edge = np.zeros_like(solid)
        edge[1:, :] |= solid[:-1, :]
        edge[:-1, :] |= solid[1:, :]
        edge[:, 1:] |= solid[:, :-1]
        edge[:, :-1] |= solid[:, 1:]
        self.p[edge & ~solid] = BLACK
        return self.p


def card():
    c = Canvas()
    c.rect(2, 9, 21, 20, M7)
    c.rect(2, 9, 21, 9, LIGHT)
    c.rect(2, 12, 21, 13, O10)
    c.rect(2, 13, 21, 13, O9)
    c.rect(4, 15, 7, 18, O10)                   # el chip
    c.rect(5, 16, 6, 17, O9)
    c.rect(15, 15, 19, 18, CYAN)                # la foto
    c.rect(9, 16, 13, 16, M5)                   # lo escrito
    c.rect(9, 18, 12, 18, M5)
    return c.outline()


def filter_():
    c = Canvas()
    for x, col in zip(range(7, 17), (M5, M6, M6, M7, M7, M7, M7, LIGHT, M6, M5)):
        c.rect(x, 5, x, 20, col)
    c.rect(8, 3, 15, 4, M5)                     # la tapa y la boca
    c.rect(10, 2, 13, 2, M6)
    c.rect(6, 21, 17, 22, M5)
    for y in (7, 9, 11, 13):                    # las aletas
        c.rect(7, y, 16, y, M5)
    c.rect(7, 15, 16, 17, O10)                  # la etiqueta
    c.rect(7, 17, 16, 17, O9)
    return c.outline()


def cutter():
    c = Canvas()
    c.rect(6, 6, 12, 8, O10)                    # la botella de gas
    c.rect(6, 8, 12, 8, O9)
    c.rect(3, 9, 15, 14, M6)                    # el cuerpo
    c.rect(3, 9, 15, 9, M7)
    c.rect(3, 14, 15, 14, M5)
    c.px(7, 11, GREEN)                          # el piloto
    c.rect(16, 10, 21, 12, M7)                  # la boquilla
    c.rect(16, 12, 21, 12, M6)
    c.rect(22, 10, 22, 12, CYAN)
    for k, y in enumerate(range(15, 22)):       # la empuñadura, inclinada
        c.rect(5 - k // 3, y, 8 - k // 3, y, M5)
    c.rect(10, 15, 11, 17, O10)                 # el gatillo
    return c.outline()


def nozzle():
    c = Canvas()
    c.rect(8, 3, 15, 4, M5)                     # la brida de arriba
    c.rect(9, 3, 14, 3, M6)
    c.rect(10, 5, 13, 6, M6)
    c.rect(9, 7, 14, 8, O10)                    # el cuello, quemado
    c.rect(9, 8, 14, 8, O9)
    for y in range(9, 22):                      # la campana, que se abre
        hw = 2 + (y - 9) * 8 // 12
        x0, x1 = 12 - hw, 11 + hw
        n = x1 - x0 + 1
        for k in range(n):
            col = M5 if k < n // 3 else M6 if k < 2 * n // 3 else M7
            if k == 2 * n // 3 + 1:
                col = LIGHT
            c.px(x0 + k, y, col)
    hw = 2 + 12 * 8 // 12
    c.rect(12 - hw + 2, 21, 11 + hw - 2, 21, BLACK)    # la boca, por dentro
    return c.outline()


def board():
    c = Canvas()
    c.rect(2, 7, 21, 19, CYAN)
    c.rect(2, 7, 21, 7, M7)
    c.rect(5, 9, 9, 12, BLACK)                  # los chips
    c.rect(13, 13, 18, 17, BLACK)
    for x in (6, 8):
        c.px(x, 13, M6)
    for y in (14, 16):
        c.px(12, y, M6)
        c.px(19, y, M6)
    c.rect(3, 15, 11, 15, GREEN)                # las pistas
    c.rect(11, 10, 11, 15, GREEN)
    c.rect(11, 10, 18, 10, GREEN)
    c.rect(18, 10, 18, 12, GREEN)
    c.rect(4, 18, 10, 18, GREEN)
    for x in range(6, 18, 2):                   # el conector
        c.rect(x, 20, x, 21, O10)
        c.rect(x + 1, 20, x + 1, 21, O9)
    return c.outline()


def cell():
    c = Canvas()
    for x, col in zip(range(7, 17), (M5, M6, M6, M7, M7, M7, M7, LIGHT, M6, M5)):
        c.rect(x, 4, x, 21, col)
    c.rect(10, 2, 13, 3, O10)                   # el borne
    c.rect(7, 4, 16, 5, M5)
    c.rect(9, 8, 14, 16, GREEN)                 # la ventana, que brilla
    c.rect(12, 8, 12, 16, LIGHT)
    for y in (10, 13):
        c.rect(9, y, 14, y, BLACK)
    for x in range(7, 17):                      # las rayas de aviso
        c.rect(x, 18, x, 19, O10 if (x // 2) % 2 else BLACK)
    c.rect(7, 21, 16, 21, M5)
    return c.outline()


def valve():
    c = Canvas()
    c.rect(2, 13, 21, 18, M6)                   # la tubería
    c.rect(2, 13, 21, 13, M7)
    c.rect(2, 17, 21, 18, M5)
    for x in (2, 20):                           # las bridas
        c.rect(x, 11, x + 1, 20, M5)
    c.rect(9, 9, 14, 19, M6)                    # el cuerpo
    c.rect(13, 9, 13, 19, M7)
    c.rect(9, 9, 9, 19, M5)
    c.rect(11, 5, 12, 8, M5)                    # el vástago
    c.rect(6, 3, 17, 4, O10)                    # la palanca
    c.rect(6, 4, 17, 4, O9)
    return c.outline()


def frames():
    """Los siete, en el orden del .gfx (desde el 36)."""
    return [card(), filter_(), cutter(), nozzle(), board(), cell(), valve()]
