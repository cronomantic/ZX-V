"""silkscreen.py - La serigrafía de la placa de ZX-V, con el Python de KiCad.

    C:/KiCad/10.0/bin/python.exe pcb/tools/silkscreen.py pcb/ZX-V/ZX-V/ZX-V.kicad_pcb

- Los textos propios (TEXTOS): qué es cada Pico, los conectores, los mandos y
  los LED, los pines de las tiras, el nombre, el autor y la versión; detrás,
  el título grande. Se rehacen cada vez.
- Las referencias de las piezas, de 0,8 mm, en el primer hueco alrededor de
  cada pieza que no pise un pad, un taladro, la serigrafía de otra pieza,
  otro texto o el borde. Si no cabe en ningún sitio, la referencia pasa a la
  capa de montaje (F.Fab) y se dice.

Después de place.py, que mueve las piezas.
"""
import sys

import pcbnew

X0, Y0 = 100.0, 50.0            # la esquina trasera izquierda (la de place.py)
W, H = 100.0, 100.0
F, B = pcbnew.F_SilkS, pcbnew.B_SilkS
MM = pcbnew.FromMM

# (texto, x, y, altura, ángulo, capa)
TEXTOS = [
    # El proyecto, en el lateral izquierdo
    ('ZX-V', 9.5, 52.0, 4.0, 90, F),
    ('Cronomantic · v0.1 · 2026', 15.0, 52.0, 1.0, 90, F),
    # Qué es cada Pico (delante de cada una, junto a su micro-USB)
    ('ULA', 34.0, 80.6, 2.0, 0, F),
    ('vídeo y sonido', 34.0, 83.3, 1.0, 0, F),
    ('CPU', 66.0, 80.6, 2.0, 0, F),
    ('BASIC y programas', 67.0, 83.3, 1.0, 0, F),
    # Conectores
    ('5 V', 10.0, 10.0, 1.0, 0, F),
    ('HDMI', 34.0, 14.0, 1.2, 0, F),
    ('AUDIO', 66.0, 17.6, 1.0, 0, F),
    ('USB', 50.0, 76.3, 1.2, 0, F),
    ('microSD', 91.0, 56.2, 1.0, 0, F),
    ('CR2032', 88.0, 39.0, 1.2, 90, F),
    ('+', 93.5, 25.0, 1.5, 0, F),
    # Mandos y LED, encima de cada uno
    ('OFF', 10.6, 95.3, 0.8, 0, F),               # el encendido: la palanca a la derecha es ON
    ('ON', 15.0, 95.3, 0.8, 0, F),
    ('MODO', 23.05, 92.4, 0.8, 0, F),
    ('RESET', 76.8, 92.4, 0.8, 0, F),
    ('SD', 84.37, 92.4, 0.8, 0, F),
    ('ENLACE', 89.47, 92.4, 0.8, 0, F),
    # Las tiras sin montar: el nombre de cada pin
    ('UART', 95.6, 16.0, 0.8, 0, F),
    ('GND', 86.5, 13.9, 0.8, 0, F), ('TX', 89.04, 13.9, 0.8, 0, F), ('RX', 91.58, 13.9, 0.8, 0, F),
    ('I2C', 97.0, 20.8, 0.8, 0, F),
    ('3V3', 86.5, 18.4, 0.8, 0, F), ('GND', 89.04, 18.4, 0.8, 0, F),
    ('SDA', 91.58, 18.4, 0.8, 0, F), ('SCL', 94.12, 18.4, 0.8, 0, F),
    # Dónde va el número de pedido del fabricante, debajo de cada Pico (queda tapado): la marca de
    # JLCPCB y la de PCBWay (al pedir, «especificar posición»; si no, lo ponen donde quieren)
    ('JLCJLCJLCJLC', 66.0, 50.0, 1.0, 0, F),
    ('WayWayWay', 34.0, 50.0, 1.0, 0, F),
    # Detrás
    ('ZX-V', 50.0, 81.5, 4.0, 0, B),
    ('v0.1 · 2026 · Cronomantic', 50.0, 86.2, 1.2, 0, B),
    ('github.com/cronomantic/ZX-V', 50.0, 88.4, 1.0, 0, B),
]
# Referencias en un sitio fijo (x, y, ángulo), donde el reparto automático las dejaría ambiguas:
# los diodos del USB, uno junto al otro, cada uno encima del suyo
REF_FIJAS = {'D5': (47.94, 81.3, 0), 'D6': (52.05, 81.3, 0)}
REF = 0.8           # altura de las referencias
SEP = 0.2           # holgura alrededor de cada cosa


def box(b, grow=0.0):
    g = MM(grow)
    return (b.GetLeft() - g, b.GetTop() - g, b.GetRight() + g, b.GetBottom() + g)


def hit(a, b):
    return a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]


def shape_boxes(sh, grow):
    """Las cajas de un dibujo: un segmento por cada lado si es un contorno."""
    w = sh.GetWidth() / 2 + MM(grow)
    if sh.GetShape() == pcbnew.SHAPE_T_SEGMENT:
        s, e = sh.GetStart(), sh.GetEnd()
        return [(min(s.x, e.x) - w, min(s.y, e.y) - w, max(s.x, e.x) + w, max(s.y, e.y) + w)]
    if sh.GetShape() == pcbnew.SHAPE_T_RECTANGLE and not sh.IsSolidFill():
        c = sh.GetRectCorners()
        out = []
        for a, b in zip(c, list(c[1:]) + [c[0]]):
            out.append((min(a.x, b.x) - w, min(a.y, b.y) - w, max(a.x, b.x) + w, max(a.y, b.y) + w))
        return out
    return [box(sh.GetBoundingBox(), grow)]


def main(pcbfile):
    b = pcbnew.LoadBoard(pcbfile)
    # Los textos propios se rehacen
    for d in list(b.GetDrawings()):
        if isinstance(d, pcbnew.PCB_TEXT) and d.GetLayer() in (F, B):
            b.Delete(d)
    edge = (MM(X0 + 0.6), MM(Y0 + 0.6), MM(X0 + W - 0.6), MM(Y0 + H - 0.6))
    obst = {F: [], B: []}
    for fp in b.GetFootprints():
        for p in fp.Pads():
            for layer in (F, B):
                if p.IsOnLayer(pcbnew.F_Cu if layer == F else pcbnew.B_Cu) or p.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH:
                    obst[layer].append(box(p.GetBoundingBox(), SEP))
        for g in fp.GraphicalItems():
            if isinstance(g, pcbnew.PCB_SHAPE) and g.GetLayer() in (F, B):
                obst[g.GetLayer()] += shape_boxes(g, 0.1)
            elif isinstance(g, pcbnew.PCB_TEXT) and g.GetLayer() in (F, B) and g.IsVisible():
                obst[g.GetLayer()].append(box(g.GetBoundingBox(), SEP))
    # Los textos propios
    for txt, x, y, h, ang, layer in TEXTOS:
        t = pcbnew.PCB_TEXT(b)
        t.SetText(txt)
        t.SetLayer(layer)
        t.SetTextSize(pcbnew.VECTOR2I(MM(h), MM(h)))
        t.SetTextThickness(MM(max(0.12, h * 0.15)))
        t.SetTextAngleDegrees(ang)
        t.SetMirrored(layer == B)
        t.SetPosition(pcbnew.VECTOR2I(MM(X0 + x), MM(Y0 + y)))
        b.Add(t)
        bb = box(t.GetBoundingBox(), SEP)
        for o in obst[layer]:
            if hit(bb, o):
                print(f'aviso: el texto «{txt}» toca algo')
                break
        obst[layer].append(bb)
    # Las referencias: primero las fijas, luego las piezas grandes
    fps = sorted(b.GetFootprints(), key=lambda f: (f.GetReference() not in REF_FIJAS, -f.GetBoundingBox(False).GetArea()))
    fab = []
    for fp in fps:
        if fp.IsBoardOnly():
            fp.Reference().SetVisible(False)
            continue
        r = fp.Reference()
        if fp.GetReference() in REF_FIJAS:
            x, y, ang = REF_FIJAS[fp.GetReference()]
            r.SetLayer(F)
            r.SetVisible(True)
            r.SetTextSize(pcbnew.VECTOR2I(MM(REF), MM(REF)))
            r.SetTextThickness(MM(0.12))
            r.SetKeepUpright(True)
            r.SetTextAngleDegrees(ang)
            r.SetPosition(pcbnew.VECTOR2I(MM(X0 + x), MM(Y0 + y)))
            obst[F].append(box(r.GetBoundingBox(), SEP))
            continue
        r.SetLayer(F)
        r.SetVisible(True)
        r.SetTextSize(pcbnew.VECTOR2I(MM(REF), MM(REF)))
        r.SetTextThickness(MM(0.12))
        r.SetKeepUpright(True)
        cy = fp.GetCourtyard(pcbnew.F_CrtYd)
        fb = box(cy.BBox()) if cy.OutlineCount() else box(fp.GetBoundingBox(False))
        cx, cyy = (fb[0] + fb[2]) // 2, (fb[1] + fb[3]) // 2
        done = False
        for ang in (0, 90):
            r.SetTextAngleDegrees(ang)
            r.SetPosition(pcbnew.VECTOR2I(cx, cyy))
            tb = box(r.GetBoundingBox())
            tw, th = tb[2] - tb[0], tb[3] - tb[1]
            g = MM(SEP)
            cands = []
            for d in (0, th, 2 * th):                      # cada vez un poco más lejos
                for k in (0, -0.5, 0.5, -1, 1, -1.5, 1.5):
                    cands += [(cx + int(k * tw), fb[1] - g - d - th // 2), (cx + int(k * tw), fb[3] + g + d + th // 2),
                              (fb[2] + g + d + tw // 2, cyy + int(k * th)), (fb[0] - g - d - tw // 2, cyy + int(k * th))]
            if (fb[2] - fb[0]) > 3 * tw and (fb[3] - fb[1]) > 3 * th:
                cands.append((cx, cyy))                      # en las piezas grandes, también dentro
            for px, py in cands:
                r.SetPosition(pcbnew.VECTOR2I(px, py))
                bb = box(r.GetBoundingBox(), SEP)
                if bb[0] < edge[0] or bb[1] < edge[1] or bb[2] > edge[2] or bb[3] > edge[3]:
                    continue
                if any(hit(bb, o) for o in obst[F]):
                    continue
                obst[F].append(bb)
                done = True
                break
            if done:
                break
        if not done:
            r.SetLayer(pcbnew.F_Fab)
            fab.append(fp.GetReference())
    pcbnew.SaveBoard(pcbfile, b)
    print('referencias solo en F.Fab (no caben):', fab or '-')


if __name__ == '__main__':
    main(sys.argv[1])
