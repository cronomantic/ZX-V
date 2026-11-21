"""route.py - El rutado de la placa de ZX-V, con Freerouting y el Python de KiCad.

    C:/KiCad/10.0/bin/python.exe pcb/tools/route.py dsn  placa.kicad_pcb   # exporta zxv.dsn (sin GND)
    C:/KiCad/10.0/bin/python.exe pcb/tools/route.py fr   placa.kicad_pcb   # solo Freerouting: zxv.dsn → zxv.ses
    C:/KiCad/10.0/bin/python.exe pcb/tools/route.py auto placa.kicad_pcb   # dsn + Freerouting + ses + masa
    C:/KiCad/10.0/bin/python.exe pcb/tools/route.py ses  placa.kicad_pcb   # importa zxv.ses y pone la masa
    C:/KiCad/10.0/bin/python.exe pcb/tools/route.py masa placa.kicad_pcb   # solo los planos y el cosido

- GND no se ruta: es un plano en las dos caras, unidas por vías de cosido en
  una rejilla, donde no molestan a ninguna otra red. Al DSN se le quita, y el
  margen de todas las pistas es allí el suyo (0,2 mm), que sus pads no se queden
  con menos.
- Los tramos que Freerouting estrecha al llegar a un pad vuelven al ancho mínimo;
  sus escaleras (pasos cortos en horizontal y vertical), una diagonal, y sus
  esquinas a 90°, a 45°.
- Las pistas que ya hay en la placa (las rutadas a mano) van al DSN como
  protegidas: el autorrutador no las toca.
- El DSN y el SES van junto a la placa (zxv.dsn, zxv.ses); no se guardan en git.
"""
import os
import re
import subprocess
import sys

import pcbnew

# Freerouting 2.5, donde lo deja su instalador (o donde diga FREEROUTING)
FREEROUTING = os.environ.get('FREEROUTING', os.path.expandvars(r'%LOCALAPPDATA%\freerouting\freerouting.exe'))
PASADAS = 100
MM = pcbnew.FromMM

PLANO_SEP = 0.3         # separación del plano a lo demás
COSIDO_PASO = 5.0       # rejilla de las vías de cosido
COSIDO_SEP = 0.35       # separación de una vía de cosido a cualquier otra red
COSIDO_PAR = 2.0        # entre dos vías de masa junto a los pares
COSIDO_PAR_SEP = 1.0    # de cada línea del par a su vía
MARGEN = 0.2            # el de Potencia (y GND): el de todas las pistas en el DSN
CHAFLAN = 0.5          # lo que se corta de cada lado en una esquina a 90° (esquinas)
ESCALON = 1.0          # el paso más largo de una escalera de Freerouting (escaleras)
ANCHO_MIN = 0.15        # el de la placa: Freerouting estrecha alguna pista al llegar a un pad


def dsn(pcbfile, out):
    b = pcbnew.LoadBoard(pcbfile)
    # Las zonas de reglas que no prohíben nada (las «Abanico» de pairs.py) irían como prohibidas
    for z in list(b.Zones()):
        if z.GetIsRuleArea() and not (z.GetDoNotAllowTracks() or z.GetDoNotAllowVias() or z.GetDoNotAllowPads()):
            b.Delete(z)
    if not pcbnew.ExportSpecctraDSN(b, out):
        raise SystemExit('no se pudo exportar el DSN')
    s = open(out, encoding='utf-8', errors='replace').read()
    # GND fuera de la red: lo hacen los planos
    s, n = re.subn(r'\n\s*\(net "?GND"?\s*\n\s*\(pins[^)]*\)\s*\)', '', s)
    s = re.sub(r'(\(class \S+(?: [^()\s]+)*?) "?GND"?(?=[\s)])', r'\1', s)
    # Sus pads se quedan sin red, y Freerouting no siempre les guarda el margen
    # de GND (el de Potencia): ese margen, para todo
    s = s.replace('(clearance 150)', f'(clearance {round(MARGEN * 1000)})')
    # Lo que ya está rutado, protegido
    s = s.replace('(type route)', '(type protect)')
    open(out, 'w', encoding='utf-8').write(s)
    print(f'DSN: {out} (GND quitado: {n})')


def freerouting(dsnfile, sesfile):
    if os.path.exists(sesfile):
        os.remove(sesfile)
    cmd = [FREEROUTING, '-de', dsnfile, '-do', sesfile, '-mp', str(PASADAS), '--gui.enabled=false']
    print('Freerouting:', ' '.join(cmd))
    subprocess.run(cmd, check=False)
    if not os.path.exists(sesfile):
        raise SystemExit('Freerouting no dejó el SES')


def ses(pcbfile, sesfile):
    b = pcbnew.LoadBoard(pcbfile)
    if not pcbnew.ImportSpecctraSES(b, sesfile):
        raise SystemExit('no se pudo importar el SES')
    # Freerouting une los dos pads de cada pin de la Pico (el de taladro y el almenado), que ya
    # se tocan: esos tramos sobran (y en un par, cuentan como trozo sin acoplar)
    pads = [p for f in b.GetFootprints() for p in f.Pads()]
    sobran = []
    for t in b.GetTracks():
        if isinstance(t, pcbnew.PCB_VIA) or t.IsLocked() or t.GetLength() > MM(1.0):
            continue
        suyos = [p for p in pads if p.GetNetCode() == t.GetNetCode() and p.IsOnLayer(t.GetLayer())]
        a = [p for p in suyos if p.HitTest(t.GetStart())]
        c = [p for p in suyos if p.HitTest(t.GetEnd())]
        if any(p.m_Uuid != q.m_Uuid and p.GetParentFootprint() == q.GetParentFootprint()
               and p.GetNumber() == q.GetNumber() for p in a for q in c):
            sobran.append(t)
    for t in sobran:
        b.Delete(t)
    if sobran:
        print(f'{len(sobran)} tramos entre los dos pads de un mismo pin, fuera')
    print(f'{escaleras(b)} escaleras, en diagonal')
    print(f'{esquinas(b)} esquinas a 90°, a 45°')
    n = 0
    for t in b.GetTracks():
        if not isinstance(t, pcbnew.PCB_VIA) and t.GetWidth() < MM(ANCHO_MIN):
            t.SetWidth(MM(ANCHO_MIN))
            n += 1
    if n:
        print(f'{n} tramos estrechos, a {ANCHO_MIN} mm')
    pcbnew.SaveBoard(pcbfile, b)
    print('SES importado:', sesfile)


def escaleras(b):
    """Freerouting deja a veces escaleras (pasos cortos, horizontal y vertical, uno tras otro)
    donde iría una diagonal: cada tramo de tres o más pasos de menos de ESCALON mm que en total
    avanza a 45° se cambia por la diagonal, con el ancho mayor de los pasos."""
    def k(v):
        return (round(v.x / 1000), round(v.y / 1000))              # al µm
    pads = [p for f in b.GetFootprints() for p in f.Pads()]
    vias = {k(v.GetPosition()) for v in b.GetTracks() if isinstance(v, pcbnew.PCB_VIA)}
    grupos = {}
    for t in b.GetTracks():
        if type(t) is pcbnew.PCB_TRACK and not t.IsLocked():
            grupos.setdefault((t.GetNetCode(), t.GetLayer()), []).append(t)
    hechas = 0
    for ts in grupos.values():
        nodo = {}
        for t in ts:
            for p in (k(t.GetStart()), k(t.GetEnd())):
                nodo.setdefault(p, []).append(t)

        def paso(p):
            """¿Se puede pasar por p sin dejar nada colgado? (dos pistas, sin vía ni pad)"""
            return (len(nodo[p]) == 2 and p not in vias
                    and not any(q.HitTest(pcbnew.VECTOR2I(p[0] * 1000, p[1] * 1000)) for q in pads))
        vistos = set()
        for t0 in ts:
            if id(t0) in vistos:
                continue
            # La cadena entera por la que pasa t0
            cad, pts = [t0], [k(t0.GetStart()), k(t0.GetEnd())]
            vistos.add(id(t0))
            for lado in (0, 1):
                while True:
                    p = pts[-1] if lado else pts[0]
                    if not paso(p):
                        break
                    sig = [t for t in nodo[p] if id(t) not in vistos]
                    if not sig:
                        break
                    t = sig[0]
                    vistos.add(id(t))
                    q = k(t.GetEnd()) if k(t.GetStart()) == p else k(t.GetStart())
                    if lado:
                        cad.append(t)
                        pts.append(q)
                    else:
                        cad.insert(0, t)
                        pts.insert(0, q)
            # Los tramos en escalera, dentro de la cadena
            i = 0
            while i < len(cad):
                j = i
                while j < len(cad):
                    a, c = pts[j], pts[j + 1]
                    hv = 'h' if a[1] == c[1] else 'v' if a[0] == c[0] else None
                    if hv is None or cad[j].GetLength() > MM(ESCALON):
                        break
                    if j > i:
                        pa, pc = pts[j - 1], pts[j]
                        if hv == ('h' if pa[1] == pc[1] else 'v'):
                            break
                    j += 1
                a, c = pts[i], pts[j]
                dx, dy = abs(c[0] - a[0]), abs(c[1] - a[1])
                if j - i >= 3 and abs(dx - dy) <= 20:              # 45°, a 0,02 mm
                    w = max(t.GetWidth() for t in cad[i:j])
                    nt = pcbnew.PCB_TRACK(b)
                    nt.SetStart(pcbnew.VECTOR2I(a[0] * 1000, a[1] * 1000))
                    nt.SetEnd(pcbnew.VECTOR2I(c[0] * 1000, c[1] * 1000))
                    nt.SetWidth(w)
                    nt.SetLayer(cad[i].GetLayer())
                    nt.SetNet(cad[i].GetNet())
                    b.Add(nt)
                    for t in cad[i:j]:
                        b.Delete(t)
                    hechas += 1
                    i = j
                else:
                    i += 1
    return hechas


def esquinas(b):
    """Las esquinas a 90° (dos pistas de la misma red y cara, sin pad ni vía en el vértice), a 45°:
    cada pista se acorta lo mismo (CHAFLAN mm, o el 40 % de la más corta) y se unen en diagonal."""
    def k(v):
        return (round(v.x / 1000), round(v.y / 1000))
    pads = [p for f in b.GetFootprints() for p in f.Pads()]
    vias = {k(v.GetPosition()) for v in b.GetTracks() if isinstance(v, pcbnew.PCB_VIA)}
    ns = b.GetDesignSettings().m_NetSettings

    def margen(it):
        return ns.GetNetClassByName(it.GetNetClassName()).GetClearance()
    nodo, mismas = {}, {}
    for t in b.GetTracks():
        if type(t) is pcbnew.PCB_TRACK:
            mismas.setdefault((t.GetNetCode(), t.GetLayer()), []).append(t)
        if type(t) is pcbnew.PCB_TRACK and not t.IsLocked():
            for p in (t.GetStart(), t.GetEnd()):
                nodo.setdefault((t.GetNetCode(), t.GetLayer(), k(p)), []).append(t)
    hechas = 0
    for (_, _, p), ts in nodo.items():
        if len(ts) != 2 or p in vias:
            continue
        P = pcbnew.VECTOR2I(p[0] * 1000, p[1] * 1000)
        if any(q.HitTest(P) for q in pads):
            continue
        a, c = ts
        if any(t.HitTest(P) for t in mismas.get((a.GetNetCode(), a.GetLayer()), []) if t not in (a, c)):
            continue                                 # una T: otra pista de la red pasa por el vértice
        # el otro extremo de cada una, y su dirección desde el vértice
        oa = a.GetEnd() if k(a.GetStart()) == p else a.GetStart()
        oc = c.GetEnd() if k(c.GetStart()) == p else c.GetStart()
        u = (oa.x - P.x, oa.y - P.y)
        v = (oc.x - P.x, oc.y - P.y)
        lu, lv = (u[0] ** 2 + u[1] ** 2) ** 0.5, (v[0] ** 2 + v[1] ** 2) ** 0.5
        if not lu or not lv or abs(u[0] * v[0] + u[1] * v[1]) > 0.05 * lu * lv:
            continue
        # La diagonal recorta la esquina por dentro: si ahí hay algo de otra red, más pequeña
        w = min(a.GetWidth(), c.GetWidth())
        mio = margen(a)
        ajenos = [(q.GetEffectiveShape(a.GetLayer()), max(mio, margen(q)))
                  for q in pads + list(b.GetTracks())
                  if q.GetNetCode() != a.GetNetCode() and q.IsOnLayer(a.GetLayer())
                  and q.GetBoundingBox().Distance(P) < MM(3)]
        s = min(MM(CHAFLAN), 0.4 * lu, 0.4 * lv)
        while s >= MM(0.1):
            na = pcbnew.VECTOR2I(int(P.x + u[0] / lu * s), int(P.y + u[1] / lu * s))
            nc = pcbnew.VECTOR2I(int(P.x + v[0] / lv * s), int(P.y + v[1] / lv * s))
            seg = pcbnew.SHAPE_SEGMENT(na, nc, w)
            if not any(sh.Collide(seg, m) for sh, m in ajenos):
                break
            s /= 2
        else:
            continue
        for t, n in ((a, na), (c, nc)):
            if k(t.GetStart()) == p:
                t.SetStart(n)
            else:
                t.SetEnd(n)
        d = pcbnew.PCB_TRACK(b)
        d.SetStart(na)
        d.SetEnd(nc)
        d.SetWidth(w)
        d.SetLayer(a.GetLayer())
        d.SetNet(a.GetNet())
        b.Add(d)
        hechas += 1
    return hechas


def _seg_dist(p, a, b):
    ax, ay, bx, by, px, py = a.x, a.y, b.x, b.y, p[0], p[1]
    dx, dy = bx - ax, by - ay
    L = dx * dx + dy * dy
    t = 0 if L == 0 else max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / L))
    qx, qy = ax + t * dx, ay + t * dy
    return ((px - qx) ** 2 + (py - qy) ** 2) ** 0.5


def masa(pcbfile):
    b = pcbnew.LoadBoard(pcbfile)
    gnd = b.FindNet('GND')
    # Fuera los planos y el cosido de antes
    for z in list(b.Zones()):
        if not z.GetIsRuleArea() and z.GetNetname() == 'GND':
            b.Delete(z)
    # (las de cosido son las de GND fijas sin pista: las que llevan, de pairs.py, se quedan)
    puntas = {(p.x, p.y) for t in b.GetTracks() if not isinstance(t, pcbnew.PCB_VIA) and t.GetNetname() == 'GND'
              for p in (t.GetStart(), t.GetEnd())}
    for t in list(b.GetTracks()):
        if (isinstance(t, pcbnew.PCB_VIA) and t.GetNetname() == 'GND' and t.IsLocked()
                and (t.GetPosition().x, t.GetPosition().y) not in puntas):
            b.Delete(t)
    # Un pad con un solo radio de alivio térmico está conectado: los pequeños
    # quedan así a menudo entre pistas, y con alivio se sueldan a mano
    b.GetDesignSettings().m_MinResolvedSpokes = 1
    edge = b.GetBoardEdgesBoundingBox()
    # Los planos: todo el contorno, en las dos caras
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        z = pcbnew.ZONE(b)
        z.SetLayer(layer)
        z.SetNet(gnd)
        z.SetLocalClearance(MM(PLANO_SEP))
        z.SetMinThickness(MM(0.25))
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        z.SetThermalReliefGap(MM(0.3))
        z.SetThermalReliefSpokeWidth(MM(0.4))
        z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
        z.SetAssignedPriority(0)
        o = z.Outline()
        o.NewOutline()
        for x, y in ((edge.GetLeft(), edge.GetTop()), (edge.GetRight(), edge.GetTop()),
                     (edge.GetRight(), edge.GetBottom()), (edge.GetLeft(), edge.GetBottom())):
            o.Append(x, y)
        b.Add(z)
    # El cosido: una rejilla de vías de GND donde no haya nada de otra red cerca
    via_d, via_drill = MM(0.6), MM(0.3)
    sep = MM(COSIDO_SEP) + via_d // 2
    obst = []                                            # (tipo, caja, red)
    for fp in b.GetFootprints():
        for p in fp.Pads():
            obst.append(('pad', p.GetBoundingBox(), p.GetNetname()))
        cy = fp.GetCourtyard(pcbnew.F_CrtYd)
        if cy.OutlineCount() and not fp.GetReference().startswith(('U1', 'U2', 'H')):
            obst.append(('patio', cy.BBox(), '*'))       # nada de vías debajo de las piezas SMD
        for z in fp.Zones():
            if z.GetIsRuleArea():
                obst.append(('veda', z.GetBoundingBox(), '*'))
    areas = [z for z in b.Zones() if z.GetIsRuleArea() and z.GetDoNotAllowVias()]   # las de la placa, por su forma
    tracks = [t for t in b.GetTracks() if not isinstance(t, pcbnew.PCB_VIA)]
    vias = [t for t in b.GetTracks() if isinstance(t, pcbnew.PCB_VIA)]

    def libre(x, y, patios=True, sep=sep):
        for kind, bb, net in obst:
            if net == 'GND' or (kind == 'patio' and not patios):
                continue
            if bb.GetLeft() - sep < x < bb.GetRight() + sep and bb.GetTop() - sep < y < bb.GetBottom() + sep:
                return False
        for z in areas:
            if z.Outline().Collide(pcbnew.VECTOR2I(int(x), int(y)), via_d // 2):
                return False
        for t in tracks:
            if t.GetNetname() != 'GND' and _seg_dist((x, y), t.GetStart(), t.GetEnd()) < sep + t.GetWidth() // 2:
                return False
        for v in vias:
            if ((v.GetPosition().x - x) ** 2 + (v.GetPosition().y - y) ** 2) ** 0.5 < sep + v.GetWidth(pcbnew.F_Cu) // 2:
                return False
        return True

    def poner(x, y):
        v = pcbnew.PCB_VIA(b)
        v.SetPosition(pcbnew.VECTOR2I(int(x), int(y)))
        v.SetWidth(via_d)
        v.SetDrill(via_drill)
        v.SetNet(gnd)
        v.SetLocked(True)                                # las de cosido, marcadas como fijas
        b.Add(v)
        vias.append(v)

    n = 0
    step = MM(COSIDO_PASO)
    x = edge.GetLeft() + MM(3)
    while x < edge.GetRight() - MM(3):
        y = edge.GetTop() + MM(3)
        while y < edge.GetBottom() - MM(3):
            if libre(x, y):
                poner(x, y)
                n += 1
            y += step
        x += step
    # La costura junto a los pares: a los dos lados, cada COSIDO_PAR mm, que el cobre de masa de
    # al lado del par quede unido al plano de debajo
    par = 0
    for t in tracks:
        if not (t.IsLocked() and t.GetNetClassName() in ('HDMI', 'USB')):
            continue
        a, c = t.GetStart(), t.GetEnd()
        L = ((c.x - a.x) ** 2 + (c.y - a.y) ** 2) ** 0.5
        if L < MM(0.5):
            continue
        ux, uy = (c.x - a.x) / L, (c.y - a.y) / L
        s = MM(0.25)
        while s < L:
            for lado in (1, -1):
                x = a.x + ux * s + uy * lado * MM(COSIDO_PAR_SEP)
                y = a.y + uy * s - ux * lado * MM(COSIDO_PAR_SEP)
                lejos = all(((v.GetPosition().x - x) ** 2 + (v.GetPosition().y - y) ** 2) ** 0.5 >= MM(COSIDO_PAR)
                            for v in vias if v.GetNetname() == 'GND')
                if lejos and libre(x, y):
                    poner(x, y)
                    par += 1
            s += MM(0.25)
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    # Las islas: los trozos de plano que no son el grande de su cara (los que
    # quedan, porque tocan algún pad de GND) llevan una vía donde caiga sobre
    # el plano de la otra cara
    planos = {z.GetLayer(): z for z in b.Zones() if not z.GetIsRuleArea() and z.GetNetname() == 'GND'}
    r = via_d // 2
    islas, sin = 0, []
    for layer, z in planos.items():
        mio = z.GetFilledPolysList(layer)
        otro = planos[pcbnew.B_Cu if layer == pcbnew.F_Cu else pcbnew.F_Cu].GetFilledPolysList(
            pcbnew.B_Cu if layer == pcbnew.F_Cu else pcbnew.F_Cu)
        if not mio.OutlineCount():
            continue
        grande = max(range(mio.OutlineCount()), key=lambda i: mio.Outline(i).Area())
        for i in range(mio.OutlineCount()):
            if i == grande:
                continue
            bb = mio.Outline(i).BBox()
            hecho = False
            yy = bb.GetTop()
            while yy <= bb.GetBottom() and not hecho:
                xx = bb.GetLeft()
                while xx <= bb.GetRight() and not hecho:
                    pts = [pcbnew.VECTOR2I(int(xx + dx), int(yy + dy))
                           for dx, dy in ((0, 0), (r, 0), (-r, 0), (0, r), (0, -r))]
                    if (all(mio.Contains(q, i) for q in pts) and all(otro.Contains(q) for q in pts)
                            and libre(xx, yy, patios=False, sep=MM(MARGEN) + via_d // 2)):   # el margen justo
                        poner(xx, yy)
                        islas += 1
                        hecho = True
                    xx += MM(0.1)
                yy += MM(0.1)
            if not hecho:
                c = bb.Centre()
                sin.append(f'{b.GetLayerName(layer)} ({pcbnew.ToMM(c.x) - 100:.1f}, {pcbnew.ToMM(c.y) - 50:.1f})')
    if islas:
        pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    pcbnew.SaveBoard(pcbfile, b)
    print(f'masa: planos en F.Cu y B.Cu, {n} vías de cosido, {par} junto a los pares, {islas} en islas')
    if sin:
        print('islas sin sitio para una vía:', ', '.join(sin))


def main(cmd, pcbfile):
    here = os.path.dirname(os.path.abspath(pcbfile))
    d, s = os.path.join(here, 'zxv.dsn'), os.path.join(here, 'zxv.ses')
    if cmd in ('dsn', 'auto'):
        dsn(pcbfile, d)
    if cmd in ('fr', 'auto'):
        freerouting(d, s)
    if cmd in ('ses', 'auto'):
        ses(pcbfile, s)
    if cmd in ('ses', 'auto', 'masa'):
        masa(pcbfile)


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
