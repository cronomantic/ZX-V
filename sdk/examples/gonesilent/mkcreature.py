#!/usr/bin/env python3
"""
mkcreature.py - Los sprites de las criaturas de Gone Silent, hechos con este
script como los de los juegos de entonces: la criatura se modela en 3D (con
formas que se funden unas con otras) y se pasa a pixel art de 80 x 56 desde
cinco ángulos y en cada postura.

    python mkcreature.py [--preview VISTA.png]

Da gonesilent.gfx, con los sprites en este orden (sdk/tools/png2gfx.py):

    0-4    andando, paso A        de frente, 3/4 de frente, de lado,
    5-9    andando, paso B        3/4 de espaldas y de espaldas (los otros
    10-14  erguida                tres ángulos son estos en espejo)
    15-19  golpeando
    20-24  herida
    25-27  muriendo (desde cualquier ángulo)
    30-35  la incubadora, de 88 x 64 y solo de frente (está pegada a la
           roca): latiendo (dos), a punto de escupir y muriendo (tres). En
           el juego, cada píxel suyo mide 6 cm: es más grande de lo que se
           modela aquí
    36-42  los objetos que se cogen, de 24 x 24 (mkitems.py)

Los colores son los de la paleta de la vista: 0 el negro (el contorno), 1, 5
y 6 el cuerpo, de oscuro a claro, y 12 el verde que brilla; el 8 es lo
transparente. Necesita numpy. El .gfx se puede retocar luego con zxv-gfx.
"""
import math
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
W, H = 80, 56                           # el sprite
CM = 4                                  # cm por píxel
PITCH = math.radians(12)                # se ve un poco desde arriba, como desde los ojos

# --- El esqueleto ---------------------------------------------------------------------

# Cada postura: los puntos (x a la derecha, y arriba, z adelante, en cm) del
# cuerpo, la cabeza, la cola y las patas (L, la izquierda; R, en espejo).
CROUCH = {
    'hip': (0, 70, -40), 'chest': (0, 82, 35), 'neck': (0, 90, 58),
    'skull': (0, 97, 64), 'snout': (0, 86, 132), 'jaw0': (0, 80, 70), 'jaw1': (0, 76, 120),
    'tail': [(0, 72, -55), (0, 84, -100), (0, 64, -145), (0, 34, -180)],
    'fl': [(-20, 78, 35), (-32, 44, 58), (-30, 3, 50), (-30, 0, 64)],
    'hl': [(-20, 68, -38), (-31, 58, -6), (-28, 24, -52), (-28, 0, -36)],
}
REAR = {
    'hip': (0, 80, -15), 'chest': (0, 150, 5), 'neck': (0, 162, 14),
    'skull': (0, 169, 18), 'snout': (0, 172, 86), 'jaw0': (0, 156, 26), 'jaw1': (0, 152, 76),
    'tail': [(0, 74, -28), (0, 46, -78), (0, 16, -126), (0, 4, -170)],
    'fl': [(-20, 145, 8), (-40, 166, 30), (-35, 196, 52), (-35, 206, 62)],
    'hl': [(-20, 78, -15), (-31, 50, 12), (-28, 22, -22), (-28, 0, -6)],
}
STRIKE = dict(REAR, **{
    'chest': (0, 138, 18), 'neck': (0, 148, 32), 'skull': (0, 152, 38), 'snout': (0, 150, 108),
    'jaw0': (0, 140, 44), 'jaw1': (0, 128, 100),
    'fl': [(-20, 134, 22), (-36, 136, 56), (-30, 112, 98), (-30, 102, 110)],
})
HURT = dict(CROUCH, **{
    'chest': (0, 92, 30), 'neck': (0, 104, 50), 'skull': (0, 112, 52), 'snout': (0, 126, 112),
    'jaw0': (0, 98, 58), 'jaw1': (0, 104, 104),
})


def walk(phase):
    """Un paso: la pata delantera izquierda y la trasera derecha adelante, o
    al revés; el cuerpo, un poco más bajo."""
    p = dict(CROUCH)
    s = 1 if phase == 0 else -1
    fl, hl = [list(v) for v in CROUCH['fl']], [list(v) for v in CROUCH['hl']]
    p['fl'] = [(x, y, z + (14 * s if i >= 1 else 0)) for i, (x, y, z) in enumerate(fl)]
    p['fr'] = [(-x, y, z - (14 * s if i >= 1 else 0)) for i, (x, y, z) in enumerate(fl)]
    p['hl'] = [(x, y, z - (12 * s if i >= 1 else 0)) for i, (x, y, z) in enumerate(hl)]
    p['hr'] = [(-x, y, z + (12 * s if i >= 1 else 0)) for i, (x, y, z) in enumerate(hl)]
    return p


def roll(pose, deg, drop):
    """La postura tumbada de lado: girada sobre su eje (z) y bajada al suelo."""
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)

    def t(v):
        x, y, z = v
        return (x * c - y * s, x * s + y * c - drop, z)
    out = {}
    for k, v in full(pose).items():
        out[k] = [t(p) for p in v] if isinstance(v, list) else t(v)
    return out


def full(p):
    """Las patas derechas, en espejo de las izquierdas si no las tiene."""
    p = dict(p)
    p.setdefault('fr', [(-x, y, z) for x, y, z in p['fl']])
    p.setdefault('hr', [(-x, y, z) for x, y, z in p['hl']])
    return p


def slump():
    p = full(CROUCH)
    out = {}
    for k, v in p.items():
        def low(q):
            x, y, z = q
            return (x * 1.3 if k in ('fl', 'fr', 'hl', 'hr') else x, max(4, y * 0.45), z)
        out[k] = [low(q) for q in v] if isinstance(v, list) else low(v)
    out['snout'] = (0, 12, 125)
    out['jaw1'] = (0, 8, 115)
    return out


# --- La criatura, como suma de formas ---------------------------------------------

def segments(pose):
    """Las formas: (a, b, radio en a, radio en b, brilla)."""
    p = full(pose)
    segs = [
        (p['hip'], p['chest'], 24, 20, 0),              # el cuerpo
        (p['chest'], p['neck'], 18, 13, 0),
        (p['neck'], p['skull'], 13, 14, 0),
        (p['skull'], p['snout'], 14, 5, 0),             # la cabeza, alargada
        (p['jaw0'], p['jaw1'], 7, 3, 0),
    ]
    tail = [p['hip']] + p['tail']
    rad = [12, 10, 7, 4, 2]
    for i in range(len(tail) - 1):
        segs.append((tail[i], tail[i + 1], rad[i], rad[i + 1], 0))
    for leg, r in (('fl', (9, 6, 4, 2)), ('fr', (9, 6, 4, 2)), ('hl', (11, 7, 5, 3)), ('hr', (11, 7, 5, 3))):
        pts = p[leg]
        for i in range(len(pts) - 1):
            segs.append((pts[i], pts[i + 1], r[i], r[i + 1], 0))
    # Las púas del lomo, que brillan, y los ojos.
    hip, chest = np.array(p['hip'], float), np.array(p['chest'], float)
    up = np.array([0.0, 1.0, 0.0])
    axis = chest - hip
    axis /= np.linalg.norm(axis)
    side = np.cross(axis, up)
    if np.linalg.norm(side) < 1e-3:
        side = np.array([1.0, 0.0, 0.0])
    side /= np.linalg.norm(side)
    back = np.cross(side, axis)             # hacia el lomo
    if back[1] < 0:
        back = -back
    for f, ln in ((0.75, 26), (0.45, 30), (0.15, 24)):
        base = hip + (chest - hip) * f + back * 16
        tip = base + back * ln - axis * 12
        segs.append((tuple(base), tuple(tip), 4, 1, 1))
    sk, sn = np.array(p['skull'], float), np.array(p['snout'], float)
    hd = sn - sk
    hd /= np.linalg.norm(hd)
    eye = sk + hd * 34
    for sx in (-1, 1):
        e = eye + side * 8 * sx + back * 5
        segs.append((tuple(e), tuple(e + hd * 5), 3, 3, 1))
    return segs


def smin(a, b, k):
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0, 1)
    return b + (a - b) * h - k * h * (1 - h)


def sdf(points, segs):
    """La distancia a la criatura (las formas oscuras se funden) y si brilla."""
    body = np.full(points.shape[0], 1e9)
    glow = np.full(points.shape[0], 1e9)
    for a, b, ra, rb, g in segs:
        a, b = np.array(a, float), np.array(b, float)
        ba = b - a
        pa = points - a
        h = np.clip((pa @ ba) / max(ba @ ba, 1e-6), 0, 1)
        d = np.linalg.norm(pa - h[:, None] * ba, axis=1) - (ra + (rb - ra) * h)
        if g:
            glow = np.minimum(glow, d)
        else:
            body = smin(body, d, 9)
    return np.minimum(body, glow), glow < body


# --- La incubadora -----------------------------------------------------------------

MW, MH = 88, 64                         # su sprite, más grande


def mother(beat=0.0, spit=False, death=0):
    """La incubadora: un cuerpo hinchado contra la roca (detrás, -z), con
    sacos que brillan y laten, una boca que se adelanta para escupir y
    raíces que la sujetan al suelo. beat, de 0 a 1, lo hinchados que están
    los sacos; death, de 0 a 3, lo que se ha hundido (y los sacos apagados)."""
    sink = (1.0, 0.8, 0.62, 0.45)[death]

    def p(x, y, z):
        return (x, y * sink, z)
    segs = [
        (p(0, 60, -40), p(0, 60, -40), 100, 100, 0),            # la masa
        (p(0, 150, -50), p(0, 150, -50), 84, 84, 0),
        (p(-105, 85, -45), p(-105, 85, -45), 58, 58, 0),        # los lóbulos
        (p(105, 85, -45), p(105, 85, -45), 58, 58, 0),
        (p(0, 210, -70), p(0, 210, -70), 46, 46, 0),
    ]
    for s in (-1, 1):                                           # las raíces
        segs.append((p(80 * s, 40, 10), p(170 * s, 0, 80), 18, 6, 0))
        segs.append((p(40 * s, 30, 50), p(75 * s, 0, 150), 14, 5, 0))
        segs.append((p(120 * s, 140, -30), p(175 * s, 60, 10), 16, 7, 0))
    head = ((0, 135, 45), (0, 160, 125)) if spit else ((0, 118, 30), (0, 108, 82))
    segs.append((p(*head[0]), p(*head[1]), 30, 17, 0))          # la boca
    if spit:                                                    # y lo que va a escupir
        segs.append(((0, 162, 140), (0, 162, 140), 15, 15, 1))
    sacs = ((-60, 170, 10, 20), (50, 190, 0, 24), (-92, 100, 30, 18), (86, 112, 25, 22),
            (0, 222, -25, 18), (22, 66, 62, 16), (-30, 62, 58, 13))
    lit = len(sacs) - 3 * death // 1 if death < 3 else 0
    for k, (x, y, z, r) in enumerate(sacs):
        rr = r * (1.0 + 0.22 * beat) * (0.8 if death else 1.0)
        segs.append((p(x, y, z), p(x, y, z), rr, rr, 1 if k < lit else 0))
    if not death:
        e = head[1]
        for s in (-1, 1):                                       # los ojos, sobre la boca
            segs.append(((10 * s, e[1] + 14, e[2] - 12), (10 * s, e[1] + 14, e[2] - 7), 4, 4, 1))
    return segs


# --- El paso a pixel art -----------------------------------------------------------

BAYER = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0
RAMP = [0, 1, 5, 6]                     # el cuerpo, de oscuro a claro
GREEN, TRANSPARENT = 12, 8


def render(pose, angle_deg, segs=None, W=W, H=H):
    """La criatura vista desde angle_deg (0: de frente, 180: de espaldas), en
    índices de color (H x W); con segs, esas formas en vez de la postura."""
    a = math.radians(angle_deg)
    f = np.array([-math.sin(a) * math.cos(PITCH), -math.sin(PITCH), -math.cos(a) * math.cos(PITCH)])
    r = np.array([-math.cos(a), 0.0, math.sin(a)])
    u = np.cross(r, f)
    u /= np.linalg.norm(u)
    if u[1] < 0:
        u = -u
    ys, xs = np.mgrid[0:H, 0:W]
    us = (xs + 0.5 - W / 2) * CM
    vs = (H - 0.5 - ys) * CM
    o = (r[None, None, :] * us[..., None] + u[None, None, :] * vs[..., None] - f * 800).reshape(-1, 3)
    if segs is None:
        segs = segments(pose)
    t = np.zeros(o.shape[0])
    hit = np.zeros(o.shape[0], bool)
    for _ in range(90):
        p = o + f * t[:, None]
        d, _g = sdf(p, segs)
        hit |= d < 0.4
        t = np.where(hit, t, t + np.maximum(d * 0.8, 0.3))
        if np.all(hit | (t > 1600)):
            break
    p = o + f * t[:, None]
    d, glow = sdf(p, segs)
    e = 1.0
    n = np.stack([sdf(p + np.array(v), segs)[0] - sdf(p - np.array(v), segs)[0]
                  for v in ((e, 0, 0), (0, e, 0), (0, 0, e))], axis=1)
    n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-9
    light = -f * 0.55 + u * 0.75 - r * 0.35
    light /= np.linalg.norm(light)
    lam = np.clip(n @ light, 0, 1)
    v = 0.12 + 0.88 * lam
    level = v * (len(RAMP) - 1)
    thr = BAYER[ys % 4, xs % 4].reshape(-1)
    idx = np.clip(np.floor(level + thr - 0.5).astype(int), 0, len(RAMP) - 1)
    img = np.array(RAMP)[idx]
    img = np.where(glow, GREEN, img)
    img = np.where(hit, img, TRANSPARENT).reshape(H, W)
    # El contorno: lo del cuerpo que toca lo transparente, en negro.
    solid = img != TRANSPARENT
    edge = np.zeros_like(solid)
    edge[1:, :] |= ~solid[:-1, :]
    edge[:-1, :] |= ~solid[1:, :]
    edge[:, 1:] |= ~solid[:, :-1]
    edge[:, :-1] |= ~solid[:, 1:]
    img = np.where(solid & edge & (img != GREEN), 0, img)
    return img


ANGLES = (0, 45, 90, 135, 180)


def frames():
    """Los sprites, en el orden del .gfx."""
    out = []
    for pose in (walk(0), walk(1), REAR, STRIKE, HURT):
        for ang in ANGLES:
            out.append(render(pose, ang))
    for pose in (slump(), roll(CROUCH, 55, 25), roll(CROUCH, 85, 40)):
        out.append(render(pose, 60))
    while len(out) % 5:
        out.append(np.full((H, W), TRANSPARENT))
    return out


def mother_frames():
    """Los de la incubadora, en el orden del .gfx (desde el 30)."""
    out = [render(None, 0, mother(0.0), MW, MH), render(None, 0, mother(1.0), MW, MH),
           render(None, 0, mother(0.5, spit=True), MW, MH)]
    for k in (1, 2, 3):
        out.append(render(None, 0, mother(0.0, death=k), MW, MH))
    return out


# --- Los ficheros ---------------------------------------------------------------------

sys.path.insert(0, os.path.join(HERE, '..', '..', 'tools'))
import png2gfx  # noqa: E402  (la paleta de siempre y el PNG)
import mkitems  # noqa: E402


def write_png(path, rgba, w, h):
    import struct
    import zlib
    raw = b''.join(b'\0' + bytes(rgba[y * w * 4:(y + 1) * w * 4]) for y in range(h))

    def chunk(t, data):
        c = struct.pack('>I', len(data)) + t + data
        return c + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff)
    with open(path, 'wb') as fo:
        fo.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) +
                 chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def sheet(imgs, colours, scale=1, bg=None, cols=5):
    """Las imágenes (todas del mismo tamaño) en una hoja de cols columnas, en RGBA."""
    H, W = imgs[0].shape
    rows = (len(imgs) + cols - 1) // cols
    w, h = cols * W * scale, rows * H * scale
    out = bytearray(w * h * 4)
    for k, img in enumerate(imgs):
        cx, cy = (k % cols) * W * scale, (k // cols) * H * scale
        for y in range(H * scale):
            for x in range(W * scale):
                c = int(img[y // scale, x // scale])
                at = ((cy + y) * w + cx + x) * 4
                if c == TRANSPARENT:
                    if bg:
                        out[at:at + 4] = bytes(bg) + b'\xff'
                    continue
                out[at:at + 4] = bytes(colours[c]) + b'\xff'
    return out, w, h


def main():
    imgs = frames()
    moth = mother_frames()
    items = mkitems.frames()
    std = [png2gfx.rgb888(png2gfx.STD565[i]) for i in range(16)]
    tmp = os.path.join(HERE, 'creature-sheet.png')
    tmp2 = os.path.join(HERE, 'mother-sheet.png')
    tmp3 = os.path.join(HERE, 'items-sheet.png')
    rgba, w, h = sheet(imgs, std)
    write_png(tmp, rgba, w, h)
    rgba, w, h = sheet(moth, std, cols=len(moth))
    write_png(tmp2, rgba, w, h)
    rgba, w, h = sheet(items, std, cols=len(items))
    write_png(tmp3, rgba, w, h)
    subprocess.run([sys.executable, os.path.join(HERE, '..', '..', 'tools', 'png2gfx.py'),
                    os.path.join(HERE, 'gonesilent.gfx'), '--sprites', '0', tmp, '%d,%d' % (W, H),
                    '--sprites', '30', tmp2, '%d,%d' % (MW, MH),
                    '--sprites', '36', tmp3, '%d,%d' % (mkitems.W, mkitems.H)], check=True)
    for t in (tmp, tmp2, tmp3):
        os.remove(t)
    if '--preview' in sys.argv:
        # Con los colores del nido, ampliadas al triple.
        pal = {0: (0, 0, 0), 1: (50, 24, 28), 5: (50, 52, 58), 6: (80, 82, 90), 12: (90, 230, 140)}
        cols = [pal.get(i, (255, 0, 255)) for i in range(16)]
        out = sys.argv[sys.argv.index('--preview') + 1]
        rgba, w, h = sheet(imgs, cols, 3, (130, 62, 60))
        write_png(out, rgba, w, h)
        rgba, w, h = sheet(moth, cols, 3, (130, 62, 60), cols=3)
        write_png(out[:-4] + '-mother.png', rgba, w, h)
        # Los objetos, con los colores de los habitáculos, al cuádruple.
        hab = {0: (0, 0, 0), 5: (60, 70, 80), 6: (90, 102, 114), 7: (126, 140, 152), 9: (150, 70, 30),
               10: (230, 130, 60), 11: (20, 90, 90), 12: (90, 230, 140), 15: (250, 240, 215)}
        rgba, w, h = sheet(items, [hab.get(i, (255, 0, 255)) for i in range(16)], 4, (52, 48, 46), cols=4)
        write_png(out[:-4] + '-items.png', rgba, w, h)
    print(len(imgs) + len(moth) + len(items), 'sprites')


if __name__ == '__main__':
    main()
