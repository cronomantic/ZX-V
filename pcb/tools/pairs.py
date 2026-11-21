"""pairs.py - Los pares diferenciales de la placa de ZX-V, rutados por
coordenadas, que Freerouting no sabe hacer: los cuatro del HDMI y el del USB.

    C:/KiCad/10.0/bin/python.exe pcb/tools/pairs.py pcb/ZX-V/ZX-V/ZX-V.kicad_pcb

- Cada tramo (de dos pads a otros dos) es una línea central con giros de 45°,
  y las dos líneas del par, paralelas a ella a la misma distancia (con
  inglete en cada esquina: la separación no cambia al girar). De cada pad a
  su línea, y de su línea al pad, un codo de 45°, igual en las dos.
- En cada giro, la línea de dentro se queda más corta; la que sale más corta
  lleva una onda de esquinas a 45° (por fuera, lejos de la otra) que la
  iguala. Cada tramo, por separado: el desfase se corrige donde se produce.
- HDMI: de los pines de la ULA (GP12–GP19) a sus 240 Ω y al conector, sin
  cruces. USB: de GP14 y GP15 de la CPU, por debajo de ella, a sus 22 Ω y al
  USB-A, con cada diodo de protección sobre su línea.
- Donde las dos líneas no van juntas (al salir de los pads y en las ondas),
  una zona de reglas «Abanico», en F.Cu y sin prohibir nada: las reglas del
  .kicad_dru miden la separación del par fuera de ellas.
- Debajo de los pares, en B.Cu, una zona sin pistas ni vías: el plano de
  masa, entero. La del USB tiene ventanas: su camino parte la placa en dos
  (de la esquina de la CPU al centro del frente), y las ocho líneas del
  enlace, entre otras, tienen que cruzarlo; lo cruzan por ahí, de través.
- Una vía a masa junto a cada pad de masa del conector HDMI, del diodo del D+
  de R1 y R2 (las del CC del USB-C, que el VBUS deja sin plano) y de C8 (el
  reloj, entre RTC_VDD y el I2C), y puentes de masa (el del microSD, entre sus dos pads; el diodo del D−,
  al condensador de al lado).

Las pistas quedan bloqueadas (y el DSN de route.py las protege). Se puede
volver a pasar: quita antes lo suyo. Dice el largo de cada línea.
"""
import math
import sys

import pcbnew

X0, Y0 = 100.0, 50.0
MM = pcbnew.FromMM
R2 = math.sqrt(2)

HDMI = (0.2, 0.15)          # ancho y separación del par (las de place.py, PARES)
USB = (0.25, 0.15)
ONDA_C = 0.15               # el chaflán de las esquinas de la onda
ONDA_T = 0.2                # lo llano de arriba de la onda
ONDA_H = 0.6                # lo más que sale una onda
ONDA_HUECO = 0.3            # entre dos ondas seguidas
TOLERANCIA = 0.02           # la diferencia de largo que no se iguala
PLANO_SEP = 0.5             # lo que la zona de B.Cu sobresale del cobre del par

# Cada tramo: de dos pads (P, N) a otros dos, por la línea central (giros de 45°).
# P va a la izquierda en el sentido de la marcha (como lo están los pads).
# sal/lle: el codo de cada pad a su línea, 'recto' o 'diag' primero.
# onda: (tramo recto de la línea central, distancia desde su principio) donde va la onda.
TRAMOS = [
    # HDMI, de la ULA a las resistencias: de dos pines seguidos, que se juntan por igual
    dict(par='D2', medidas=HDMI, de=(('U2', '21'), ('U2', '22')), a=(('R27', '1'), ('R28', '1')),
         centro=[(26.4, 27.14), (27.0, 27.14), (27.6, 26.54), (27.6, 24.8), (26.105, 23.305), (26.105, 22.2)],
         sal='recto', lle='diag', onda=(2, 0.4)),
    dict(par='D1', medidas=HDMI, de=(('U2', '24'), ('U2', '25')), a=(('R29', '1'), ('R30', '1')),
         centro=[(26.4, 34.76), (28.0, 34.76), (28.6, 34.16), (28.6, 23.4), (29.2, 22.8), (29.5, 22.8),
                 (30.1, 22.2), (30.1, 22.0)],
         sal='recto', lle='diag', onda=(2, 2.5)),
    dict(par='D0', medidas=HDMI, de=(('U2', '16'), ('U2', '17')), a=(('R23', '1'), ('R24', '1')),
         centro=[(41.6, 34.76), (40.0, 34.76), (39.4, 34.16), (39.4, 23.6), (38.8, 23.0), (34.9, 23.0),
                 (34.3, 22.4), (34.3, 22.0)],
         sal='recto', lle='diag', onda=(2, 2.5)),
    dict(par='CK', medidas=HDMI, de=(('U2', '19'), ('U2', '20')), a=(('R25', '1'), ('R26', '1')),
         centro=[(41.6, 27.14), (41.0, 27.14), (40.4, 26.54), (40.4, 22.8), (39.8, 22.2), (38.9, 22.2),
                 (38.3, 21.6), (38.3, 21.5)],
         sal='recto', lle='diag', onda=(2, 1.2)),
    # HDMI, de las resistencias al conector: en diagonal, y se abren al llegar (masa en medio)
    dict(par='D2', medidas=HDMI, de=(('R27', '2'), ('R28', '2')), a=(('J7', '1'), ('J7', '3')),
         centro=[(26.105, 17.6), (26.105, 17.4), (30.0, 13.505), (30.0, 13.4)], sal='diag', lle='diag'),
    dict(par='D1', medidas=HDMI, de=(('R29', '2'), ('R30', '2')), a=(('J7', '4'), ('J7', '6')),
         centro=[(30.1, 17.6), (30.1, 15.0), (31.5, 13.6), (31.5, 13.4)], sal='diag', lle='diag'),
    dict(par='D0', medidas=HDMI, de=(('R23', '2'), ('R24', '2')), a=(('J7', '7'), ('J7', '9')),
         centro=[(34.3, 17.6), (34.3, 15.0), (33.0, 13.7), (33.0, 13.4)], sal='diag', lle='diag'),
    dict(par='CK', medidas=HDMI, de=(('R25', '2'), ('R26', '2')), a=(('J7', '10'), ('J7', '12')),
         centro=[(38.3, 17.6), (38.3, 17.4), (34.5, 13.6), (34.5, 13.4)], sal='diag', lle='diag'),
    # USB: de la CPU, por dentro de su columna de pines, a sus 22 Ω; de ahí, al USB-A
    dict(par='USB', medidas=USB, de=(('U1', '19'), ('U1', '20')), a=(('R18', '2'), ('R19', '2')),
         centro=[(73.7, 27.14), (73.5, 27.14), (72.9, 27.74), (72.9, 79.0), (72.3, 79.6), (59.9, 79.6)],
         sal='recto', lle='diag', onda=(2, 20.0)),
    dict(par='USB', medidas=USB, de=(('R18', '1'), ('R19', '1')), a=(('J4', '3'), ('J4', '2')),
         centro=[(54.9, 79.6), (50.6, 79.6), (49.995, 80.205), (49.995, 81.6)],
         sal='diag', lle='diag', onda=(0, 1.0)),
]
# La zona sin pistas bajo el USB, con ventanas para lo que tiene que cruzarlo: una a la altura
# de cada pin del enlace de la CPU (sale de él por B.Cu) y otras para lo demás
VENTANA_X = (71.6, 74.2)
VENTANA_LARGO = 1.8
VENTANAS_Y = [33.0, 38.57, 41.11, 44.0, 46.19, 48.73, 51.27, 53.81, 56.35, 58.89, 61.43, 66.0, 70.5, 75.0]

# Masa: (x del pad, y del pad, x de la vía, y de la vía)
MASA = [(30.0, 10.6, 30.0, 9.3), (31.5, 10.6, 31.5, 9.3), (33.0, 10.6, 33.0, 9.3), (34.5, 10.6, 34.5, 9.3),
        (37.5, 10.6, 37.5, 9.3),
        (53.1, 82.9, 54.3, 82.9),                   # el diodo del D+
        (16.5, 11.5, 16.5, 10.2), (13.5, 11.5, 13.5, 10.2),   # R1 y R2 (CC del USB-C): el VBUS las encierra
        (83.04, 14.5, 84.3, 14.5)]                  # C8 (el reloj): RTC_VDD y el I2C la encierran
# Puentes de masa, sin vía: el pad 6 (VSS) del microSD, al 9 (también GND), por la derecha de
# los contactos, bajo el zócalo (las pistas de alrededor le dejaban sin plano); el diodo del
# D−, al pad de masa de C6 (no cabe una vía entre el VBUS y el conector); los pads de masa del
# USB-C, a su carcasa, de taladro (el VBUS los encierra)
PUENTES = [[(83.34, 68.72), (84.2, 68.72), (84.6, 69.12), (84.6, 71.48), (84.2, 71.88), (83.34, 71.88)],
           [(46.89, 82.9), (45.1, 82.9)],
           [(20.75, 8.245), (21.6, 7.4)], [(14.25, 8.245), (13.4, 7.4)]]   # USB-C: GND a la carcasa (THT)
ZONA = 'Pares: plano de masa entero'
ABANICO = 'Abanico'           # el nombre que usa el .kicad_dru


def pt(x, y):
    return pcbnew.VECTOR2I(MM(X0 + x), MM(Y0 + y))


def izquierda(a, b):
    """La normal a la izquierda de a→b, en el sentido de la marcha (y crece hacia abajo)."""
    dx, dy = b[0] - a[0], b[1] - a[1]
    L = math.hypot(dx, dy)
    return dy / L, -dx / L


def paralela(c, d):
    """La línea a d de la central c (a su izquierda si d > 0), con inglete en cada vértice."""
    out = []
    for i, p in enumerate(c):
        if i == 0:
            n = izquierda(c[0], c[1])
            out.append((p[0] + d * n[0], p[1] + d * n[1]))
        elif i == len(c) - 1:
            n = izquierda(c[-2], c[-1])
            out.append((p[0] + d * n[0], p[1] + d * n[1]))
        else:
            n1, n2 = izquierda(c[i - 1], p), izquierda(p, c[i + 1])
            k = d / (1 + n1[0] * n2[0] + n1[1] * n2[1])
            out.append((p[0] + k * (n1[0] + n2[0]), p[1] + k * (n1[1] + n2[1])))
    return out


def codo(a, b, primero):
    """De a a b con un tramo recto (horizontal o vertical) y uno a 45°."""
    dx, dy = b[0] - a[0], b[1] - a[1]
    m = min(abs(dx), abs(dy))
    diag = (math.copysign(m, dx), math.copysign(m, dy))
    mid = (a[0] + diag[0], a[1] + diag[1]) if primero == 'diag' else (b[0] - diag[0], b[1] - diag[1])
    pts = [a]
    for p in (mid, b):
        if math.dist(p, pts[-1]) > 1e-4:
            pts.append(p)
    return pts


def largo(pts):
    return sum(math.dist(a, b) for a, b in zip(pts, pts[1:]))


def ondas(a, b, en, extra, fuera):
    """Los puntos de las ondas que alargan en `extra` el tramo a→b, empezando a `en` de a,
    hacia el lado `fuera` (+1 izquierda, −1 derecha). Cada onda, un rectángulo de esquinas
    achaflanadas a 45°: alarga 2h − 4(2 − √2)c."""
    L = math.dist(a, b)
    u = ((b[0] - a[0]) / L, (b[1] - a[1]) / L)
    nl = izquierda(a, b)
    n = (nl[0] * fuera, nl[1] * fuera)
    perdida = 4 * (2 - R2)
    cuantas = max(1, math.ceil(extra / (2 * ONDA_H - perdida * ONDA_C)))
    e1 = extra / cuantas
    c = ONDA_C
    h = (e1 + perdida * c) / 2
    if h < 2 * c:                                    # muy poco: una onda más pequeña, con h = 2c
        c = e1 / (4 - perdida)
        h = 2 * c
    paso = 4 * c + ONDA_T
    if en + cuantas * paso + (cuantas - 1) * ONDA_HUECO > L - 0.3:
        raise SystemExit(f'no caben las ondas ({extra:.2f} mm) en un tramo de {L:.2f} mm')
    pts = []
    s = en
    for _ in range(cuantas):
        for al, fu in ((0, 0), (c, c), (c, h - c), (2 * c, h), (2 * c + ONDA_T, h), (3 * c + ONDA_T, h - c),
                       (3 * c + ONDA_T, c), (4 * c + ONDA_T, 0)):
            pts.append((a[0] + u[0] * (s + al) + n[0] * fu, a[1] + u[1] * (s + al) + n[1] * fu))
        s += paso + ONDA_HUECO
    return pts


def pad_xy(fps, ref, num):
    """El centro del pad (el de taladro, si la huella tiene dos con ese número) y su red."""
    pads = [p for p in fps[ref].Pads() if p.GetNumber() == num]
    pads.sort(key=lambda p: p.GetAttribute() != pcbnew.PAD_ATTRIB_PTH)
    p = pads[0].GetPosition()
    return (round(pcbnew.ToMM(p.x) - X0, 4), round(pcbnew.ToMM(p.y) - Y0, 4)), pads[0].GetNet()


def lineas(t, fps):
    """Las dos líneas del tramo (P, N), con la onda en la más corta, y los trozos en que no van
    juntas (las salidas de los pads, la onda): listas de puntos."""
    w, g = t['medidas']
    d = (w + g) / 2
    c = t['centro']
    ini = {'P': pad_xy(fps, *t['de'][0])[0], 'N': pad_xy(fps, *t['de'][1])[0]}
    fin = {'P': pad_xy(fps, *t['a'][0])[0], 'N': pad_xy(fps, *t['a'][1])[0]}
    medio = {'P': paralela(c, d), 'N': paralela(c, -d)}

    def entera(k, m):
        return codo(ini[k], m[0], t['sal'])[:-1] + m + codo(m[-1], fin[k], t['lle'])[1:]

    sueltos = [codo(ini['P'], medio['P'][0], t['sal']) + codo(ini['N'], medio['N'][0], t['sal']),
               codo(medio['P'][-1], fin['P'], t['lle']) + codo(medio['N'][-1], fin['N'], t['lle'])]
    L = {k: largo(entera(k, medio[k])) for k in 'PN'}
    corta = min(L, key=L.get)
    extra = abs(L['P'] - L['N'])
    if extra > TOLERANCIA:
        if 'onda' not in t:
            raise SystemExit(f"{t['par']}: {extra:.2f} mm de diferencia, y el tramo no tiene sitio para la onda")
        i, en = t['onda']
        m = medio[corta]
        o = ondas(m[i], m[i + 1], en, extra, +1 if corta == 'P' else -1)
        medio[corta] = m[:i + 1] + o + m[i + 1:]
        # La otra línea, partida enfrente de la onda: ese trozo tampoco va acoplado
        larga = 'N' if corta == 'P' else 'P'
        q = medio[larga]
        u = ((q[i + 1][0] - q[i][0]) / math.dist(q[i], q[i + 1]), (q[i + 1][1] - q[i][1]) / math.dist(q[i], q[i + 1]))
        frente = [(q[i][0] + u[0] * s, q[i][1] + u[1] * s)
                  for s in ((p[0] - q[i][0]) * u[0] + (p[1] - q[i][1]) * u[1] for p in (o[0], o[-1]))]
        medio[larga] = q[:i + 1] + frente + q[i + 1:]
        sueltos.append(o + frente)
    return {k: entera(k, medio[k]) for k in 'PN'}, sueltos


def main(pcbfile):
    b = pcbnew.LoadBoard(pcbfile)
    fps = {f.GetReference(): f for f in b.GetFootprints()}
    gnd = b.FindNet('GND')
    # Fuera lo de antes: las pistas de estas redes, los puentes y vías de masa, las zonas
    nets = set()
    for t in TRAMOS:
        for ref, num in t['de'] + t['a']:
            nets.add(pad_xy(fps, ref, num)[1].GetNetname())
    for t in list(b.GetTracks()):
        if t.GetNetname() in nets or (t.GetNetname() == 'GND' and t.IsLocked() and not isinstance(t, pcbnew.PCB_VIA)):
            b.Delete(t)
    for v in list(b.GetTracks()):
        if isinstance(v, pcbnew.PCB_VIA) and v.GetNetname() == 'GND':
            p = v.GetPosition()
            if any(abs(p.x - pt(vx, vy).x) < MM(0.05) and abs(p.y - pt(vx, vy).y) < MM(0.05) for _, _, vx, vy in MASA):
                b.Delete(v)
    for z in list(b.Zones()):
        if z.GetIsRuleArea() and (z.GetZoneName().startswith(ZONA) or z.GetZoneName() == ABANICO):
            b.Delete(z)

    def pista(a, c, w, net):
        tr = pcbnew.PCB_TRACK(b)
        tr.SetStart(pt(*a))
        tr.SetEnd(pt(*c))
        tr.SetWidth(MM(w))
        tr.SetLayer(pcbnew.F_Cu)
        tr.SetNet(net)
        tr.SetLocked(True)
        b.Add(tr)
        return tr

    def abanico(pts, w):
        m = w / 2 + 0.05
        x0, x1 = min(p[0] for p in pts) - m, max(p[0] for p in pts) + m
        y0, y1 = min(p[1] for p in pts) - m, max(p[1] for p in pts) + m
        z = pcbnew.ZONE(b)
        z.SetIsRuleArea(True)
        z.SetLayer(pcbnew.F_Cu)
        z.SetZoneName(ABANICO)
        for f in (z.SetDoNotAllowTracks, z.SetDoNotAllowVias, z.SetDoNotAllowPads, z.SetDoNotAllowZoneFills,
                  z.SetDoNotAllowFootprints):
            f(False)
        o = z.Outline()
        o.NewOutline()
        for x, y in ((x0, y0), (x1, y0), (x1, y1), (x0, y1)):
            o.Append(MM(X0 + x), MM(Y0 + y))
        z.SetLocked(True)
        b.Add(z)

    grupos = {'HDMI': [], 'USB': []}
    pads = {'HDMI': [], 'USB': []}                 # también bajo los pads: entre los dos de una resistencia
    informe = []
    for t in TRAMOS:
        w = t['medidas'][0]
        ls, sueltos = lineas(t, fps)
        for ref, num in t['de'] + t['a']:
            pads['USB' if t['par'] == 'USB' else 'HDMI'] += [p for p in fps[ref].Pads() if p.GetNumber() == num]
        for pts in sueltos:
            abanico(pts, w)
        for k, de in zip('PN', t['de']):
            net = pad_xy(fps, *de)[1]
            # Un vértice en el centro de cada pad de la red por el que pase la línea (el de los
            # diodos del USB): Freerouting no da el pad por conectado si la pista solo lo cruza
            centros = [pad_xy(fps, f.GetReference(), p.GetNumber())[0] for f in fps.values() for p in f.Pads()
                       if p.GetNetCode() == net.GetNetCode()]
            pts = [ls[k][0]]
            for p, q in zip(ls[k], ls[k][1:]):
                L = math.dist(p, q)
                for c in sorted(centros, key=lambda c: math.dist(p, c)):
                    s = ((c[0] - p[0]) * (q[0] - p[0]) + (c[1] - p[1]) * (q[1] - p[1])) / (L * L)
                    if 0.01 < s < 0.99 and math.dist(c, (p[0] + s * (q[0] - p[0]), p[1] + s * (q[1] - p[1]))) < 0.005:
                        pts.append(c)
                pts.append(q)
            ls[k] = pts
            for p, q in zip(ls[k], ls[k][1:]):
                if math.dist(p, q) > 1e-4:
                    grupos['USB' if t['par'] == 'USB' else 'HDMI'].append(pista(p, q, w, net))
        lp, ln = largo(ls['P']), largo(ls['N'])
        informe.append(f"{t['par']:3} {t['de'][0][0]:>3} → {t['a'][0][0]:<3}  P {lp:6.2f}  N {ln:6.2f}  "
                       f"diferencia {abs(lp - ln):.3f} mm")
    for px, py, vx, vy in MASA:
        pista((px, py), (vx, vy), 0.25, gnd)
        v = pcbnew.PCB_VIA(b)
        v.SetPosition(pt(vx, vy))
        v.SetWidth(MM(0.6))
        v.SetDrill(MM(0.3))
        v.SetNet(gnd)
        v.SetLocked(True)
        b.Add(v)
    for pts in PUENTES:
        for a, c in zip(pts, pts[1:]):
            pista(a, c, 0.25, gnd)

    # Debajo, en B.Cu, sin pistas ni vías (el plano sí)
    for nombre, tracks in grupos.items():
        ps = pcbnew.SHAPE_POLY_SET()
        for it in tracks + pads[nombre]:
            it.TransformShapeToPolygon(ps, pcbnew.F_Cu, MM(PLANO_SEP), MM(0.005), pcbnew.ERROR_OUTSIDE)
        ps.Simplify()
        if nombre == 'USB':
            for yc in VENTANAS_Y:
                r = pcbnew.SHAPE_POLY_SET()
                r.NewOutline()
                for x, y in ((VENTANA_X[0], yc - VENTANA_LARGO / 2), (VENTANA_X[1], yc - VENTANA_LARGO / 2),
                             (VENTANA_X[1], yc + VENTANA_LARGO / 2), (VENTANA_X[0], yc + VENTANA_LARGO / 2)):
                    r.Append(MM(X0 + x), MM(Y0 + y))
                ps.BooleanSubtract(r)
        for i in range(ps.OutlineCount()):         # una zona por trozo: el DSN solo lleva el primero
            z = pcbnew.ZONE(b)
            z.SetIsRuleArea(True)
            z.SetLayer(pcbnew.B_Cu)
            z.SetZoneName(f'{ZONA} ({nombre})')
            z.SetDoNotAllowTracks(True)
            z.SetDoNotAllowVias(True)
            z.SetDoNotAllowPads(False)
            z.SetDoNotAllowZoneFills(False)
            z.SetDoNotAllowFootprints(False)
            o = z.Outline()
            o.AddOutline(ps.Outline(i))
            for j in range(ps.HoleCount(i)):
                o.AddHole(ps.Hole(i, j))
            z.SetLocked(True)
            b.Add(z)

    pcbnew.SaveBoard(pcbfile, b)
    print('\n'.join(informe))


if __name__ == '__main__':
    main(sys.argv[1])
