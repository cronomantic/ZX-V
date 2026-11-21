"""fabricate.py - Lo que se sube a JLCPCB para pedir la placa de ZX-V montada,
y la comprobación de que está bien (contra la placa, no contra lo que dice
kicad-cli al terminar).

    python pcb/tools/fabricate.py            genera pcb/fabrication y lo comprueba
    python pcb/tools/fabricate.py --check    solo comprueba lo que ya hay

Sale en `pcb/fabrication/`:

- `zxv-gerber.zip`: los Gerber (dos capas, máscara, serigrafía, pasta y el
  contorno), los taladros (Excellon, mm) y el fichero de trabajo.
- `zxv-bom.csv`: la lista para el montaje, con las columnas de JLCPCB
  (Comment, Designator, Footprint, JLCPCB Part #), una fila por número LCSC.
- `zxv-cpl.csv`: dónde va cada pieza (Designator, Mid X, Mid Y, Layer,
  Rotation), en el mismo sistema que los Gerber.
- `zxv-schematic.pdf`: el esquema.

Entran en el montaje las piezas SMD que no son *DNP*: lo que suelda quien la
monta (las Pico, los mandos y LED del frontal, las tiras J5 y J6) va marcado
*DNP* en el esquema.

**El giro y el origen de cada pieza.** JLCPCB coloca cada pieza según su
propia huella (la de EasyEDA), que no siempre está girada como la de KiCad ni
tiene el origen en el mismo sitio. GIRO_JLC dice, por huella, cuánto hay que
girar la de JLCPCB para que coincida con la nuestra y dónde cae su origen en
la nuestra: se midió comparando pad a pad las dos huellas (y, en los
conectores, sus patillas de sujeción, que son las que fijan el cuerpo). Las
huellas que vienen de EasyEDA (el HDMI, el USB-A y el portapilas) ya
coinciden. Las piezas de dos pads sin polaridad dan igual a 0 que a 180°.
Aun así, en el pedido hay que mirar la vista previa de la colocación.

Lo que hace falta para JLCPCB: el CPL con las columnas que lee
JLCPCB (con las de kicad-cli da «failed processing cpl file»), y que
`--exclude-fp-th` tiraría los conectores SMD con patillas de sujeción THT.
"""
import csv
import io
import json
import math
import os
import re
import subprocess
import sys
import zipfile

sys.stdout.reconfigure(encoding='utf-8')

TOOLS = os.path.dirname(os.path.abspath(__file__))
PROY = os.path.join(TOOLS, '..', 'ZX-V', 'ZX-V')
FAB = os.path.normpath(os.path.join(TOOLS, '..', 'fabrication'))
PCB = os.path.join(PROY, 'ZX-V.kicad_pcb')
SCH = os.path.join(PROY, 'ZX-V.kicad_sch')
CLI = os.environ.get('KICAD_CLI', r'C:\KiCad\10.0\bin\kicad-cli.exe')
PY = os.environ.get('KICAD_PYTHON', r'C:\KiCad\10.0\bin\python.exe')

CAPAS = 'F.Cu,B.Cu,F.Paste,B.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts'
CPL_CAB = 'Designator,Mid X,Mid Y,Layer,Rotation'
# El paréntesis de la columna del BOM es el ancho (U+FF08/FF09), como en su plantilla
BOM_PIEZA = 'JLCPCB Part #' + chr(0xFF08) + 'optional' + chr(0xFF09)
BOM_CAB = 'Comment,Designator,Footprint,' + BOM_PIEZA

# huella: (giro que se suma, (x, y) del origen de la de JLCPCB en la nuestra, en mm)
GIRO_JLC = {
    'SOT-23': (180, (0.0, 0.0)),                                   # Q1, D4
    'SOIC-8_3.9x4.9mm_P1.27mm': (270, (0.0, 0.0)),                 # U3
    'D_SOD-123': (0, (0.0, 0.0)),                                  # D1–D3
    'Crystal_SMD_3215-2Pin_3.2x1.5mm': (180, (0.0, 0.0)),          # Y1 (sin polaridad)
    'USB_C_Receptacle_HRO_TYPE-C-31-M-12': (0, (0.0, -1.39)),      # J1: por sus patillas
    'microSD_HC_Hirose_DM3AT-SF-PEJM5': (0, (-0.075, -0.175)),     # J3
    'Jack_3.5mm_PJ-320D_TRS': (0, (0.95, 0.0)),                    # J8: por sus patillas
}


def cli(args):
    r = subprocess.run([CLI] + args, capture_output=True)
    if r.returncode != 0:
        print(r.stdout.decode('utf-8', 'replace')[-600:], r.stderr.decode('utf-8', 'replace')[-600:])
        raise SystemExit('kicad-cli falló: ' + ' '.join(args[:3]))


PIEZAS = r'''
import pcbnew, json, sys
b = pcbnew.LoadBoard(sys.argv[1])
out = []
for f in b.GetFootprints():
    if f.IsBoardOnly():
        continue
    lc = f.GetField('LCSC')
    p = f.GetPosition()
    out.append(dict(ref=f.GetReference(), valor=f.GetValue(), huella=f.GetFPID().GetUniStringLibItemName(),
                    lcsc=lc.GetText() if lc else '', smd=bool(f.GetAttributes() & pcbnew.FP_SMD),
                    dnp=f.IsDNP(), x=pcbnew.ToMM(p.x), y=pcbnew.ToMM(p.y), giro=f.GetOrientationDegrees(),
                    cara='Top' if f.GetLayer() == pcbnew.F_Cu else 'Bottom'))
print(json.dumps(out))
'''


def piezas():
    """Las piezas de la placa, con el Python de KiCad (el de este script puede ser otro)."""
    r = subprocess.run([PY, '-c', PIEZAS, PCB], capture_output=True)
    linea = [l for l in r.stdout.decode('utf-8', 'replace').splitlines() if l.startswith('[')]
    if not linea:
        print(r.stderr.decode('utf-8', 'replace')[-800:])
        raise SystemExit('no se pudo leer la placa')
    return json.loads(linea[-1])


def montadas(ps):
    return sorted((p for p in ps if p['smd'] and not p['dnp']), key=lambda p: clave(p['ref']))


def clave(ref):
    m = re.match(r'([A-Z]+)(\d+)', ref)
    return (m.group(1), int(m.group(2))) if m else (ref, 0)


def girar(x, y, a):
    """El giro de KiCad (y hacia abajo, positivo en sentido antihorario a la vista)."""
    c, s = math.cos(math.radians(a)), math.sin(math.radians(a))
    return x * c + y * s, -x * s + y * c


def cpl(ps, ruta):
    """Coordenadas de página de KiCad, con la Y cambiada de signo: las de los Gerber y los taladros."""
    with io.open(ruta, 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(CPL_CAB.split(','))
        for p in montadas(ps):
            giro, (tx, ty) = GIRO_JLC.get(p['huella'], (0, (0.0, 0.0)))
            dx, dy = girar(tx, ty, p['giro'])
            r = (p['giro'] + giro) % 360
            w.writerow([p['ref'], '%.4fmm' % (p['x'] + dx), '%.4fmm' % -(p['y'] + dy), p['cara'],
                        ('%d' % r) if r == int(r) else '%.2f' % r])


def bom(ps, ruta):
    filas = {}
    for p in montadas(ps):
        filas.setdefault(p['lcsc'], []).append(p)
    with io.open(ruta, 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(BOM_CAB.split(','))
        for lcsc, grupo in sorted(filas.items(), key=lambda kv: clave(kv[1][0]['ref'])):
            w.writerow([grupo[0]['valor'], ','.join(p['ref'] for p in grupo), grupo[0]['huella'], lcsc])


def generar():
    os.makedirs(FAB, exist_ok=True)
    gerb = os.path.join(FAB, 'gerber')
    os.makedirs(gerb, exist_ok=True)
    for f in os.listdir(gerb):
        os.remove(os.path.join(gerb, f))
    cli(['pcb', 'export', 'gerbers', '--layers', CAPAS, '--subtract-soldermask', '--no-protel-ext', '-o', gerb, PCB])
    # Los metalizados y los que no, en dos ficheros (ZX-V-PTH.drl y ZX-V-NPTH.drl): como los pide JLCPCB
    cli(['pcb', 'export', 'drill', '--format', 'excellon', '--drill-origin', 'absolute', '--excellon-units', 'mm',
         '--excellon-separate-th', '--generate-map', '--map-format', 'gerberx2', '-o', gerb, PCB])
    z = os.path.join(FAB, 'zxv-gerber.zip')
    with zipfile.ZipFile(z, 'w', zipfile.ZIP_DEFLATED) as zf:
        for f in sorted(os.listdir(gerb)):
            zf.write(os.path.join(gerb, f), f)
    ps = piezas()
    cpl(ps, os.path.join(FAB, 'zxv-cpl.csv'))
    bom(ps, os.path.join(FAB, 'zxv-bom.csv'))
    cli(['sch', 'export', 'pdf', '-o', os.path.join(FAB, 'zxv-schematic.pdf'), SCH])


def comprobar():
    ok = True

    def di(nombre, bien, detalle):
        print('   %-34s %-3s %s' % (nombre, 'ok' if bien else 'NO', detalle))
        return bien

    gerb = os.path.join(FAB, 'gerber')
    for c in ('F_Cu', 'B_Cu', 'F_Paste', 'B_Paste', 'F_Silkscreen', 'B_Silkscreen', 'F_Mask', 'B_Mask', 'Edge_Cuts'):
        p = os.path.join(gerb, 'ZX-V-%s.gbr' % c)
        n = os.path.getsize(p) if os.path.exists(p) else 0
        ops = len(re.findall(r'D0[123]\*', io.open(p, encoding='utf-8', errors='replace').read())) if n else 0
        # B_Paste va vacía: detrás no se monta nada
        ok &= di('capa %s' % c, n > 0 and (ops > 0 or c == 'B_Paste'), '%d bytes, %d trazos' % (n, ops))

    ps = piezas()
    g = io.open(os.path.join(gerb, 'ZX-V-Edge_Cuts.gbr'), encoding='utf-8').read()
    dec = int(re.search(r'%FSLAX\d(\d)', g).group(1))
    pts = [(int(x) / 10 ** dec, int(y) / 10 ** dec) for x, y in re.findall(r'^X(-?\d+)Y(-?\d+)D0[123]\*', g, re.M)]
    an = max(p[0] for p in pts) - min(p[0] for p in pts)
    al = max(p[1] for p in pts) - min(p[1] for p in pts)
    ok &= di('contorno', abs(an - 100) < 0.15 and abs(al - 100) < 0.15, '%.2f x %.2f mm' % (an, al))

    taladros = sum(len(re.findall(r'^X[-\d.]+Y[-\d.]+', io.open(os.path.join(gerb, f), encoding='utf-8').read(),
                                  re.M)) for f in os.listdir(gerb) if f.endswith('.drl'))
    ok &= di('taladros', taladros > 300, '%d en los ficheros de taladro' % taladros)
    drl = sorted(f for f in os.listdir(gerb) if f.endswith('.drl'))
    ok &= di('metalizados y sin metalizar', drl == ['ZX-V-NPTH.drl', 'ZX-V-PTH.drl'], ', '.join(drl))

    j = json.load(io.open(os.path.join(gerb, 'ZX-V-job.gbrjob'), encoding='utf-8'))
    gs = j['GeneralSpecs']
    ok &= di('fichero de trabajo', gs['LayerNumber'] == 2,
             '%d capas, %.1f x %.1f mm, %s mm de grueso' % (gs['LayerNumber'], gs['Size']['X'], gs['Size']['Y'],
                                                            gs['BoardThickness']))
    z = os.path.join(FAB, 'zxv-gerber.zip')
    with zipfile.ZipFile(z) as zf:
        dentro = set(zf.namelist())
    ok &= di('zip', dentro == set(os.listdir(gerb)), '%d ficheros' % len(dentro))

    # El CPL: su formato, y que esté exactamente lo que monta la máquina
    ruta = os.path.join(FAB, 'zxv-cpl.csv')
    cab = io.open(ruta, encoding='utf-8').readline().strip()
    filas = list(csv.DictReader(io.open(ruta, encoding='utf-8'))) if cab == CPL_CAB else []
    giros = [float(r['Rotation']) for r in filas] or [0.0]
    ok &= di('CPL: formato', cab == CPL_CAB and all(r['Mid X'].endswith('mm') for r in filas)
             and 0 <= min(giros) and max(giros) < 360 and {r['Layer'] for r in filas} <= {'Top'},
             'cabecera %s' % ('ok' if cab == CPL_CAB else repr(cab)))
    van = {p['ref'] for p in montadas(ps)}
    hay = {r['Designator'] for r in filas}
    ok &= di('CPL: piezas', hay == van, '%d; faltan %s, sobran %s' % (len(hay), sorted(van - hay) or '-',
                                                                      sorted(hay - van) or '-'))
    sin = sorted(p['ref'] for p in montadas(ps) if not p['lcsc'])
    ok &= di('LCSC', not sin, 'sin número: %s' % (sin or '-'))

    ruta = os.path.join(FAB, 'zxv-bom.csv')
    cab = io.open(ruta, encoding='utf-8').readline().strip()
    en = {}
    if cab == BOM_CAB:
        for r in csv.DictReader(io.open(ruta, encoding='utf-8')):
            for ref in r['Designator'].split(','):
                en[ref] = r[BOM_PIEZA]
    ok &= di('BOM', cab == BOM_CAB and set(en) == van,
             '%d números LCSC para %d piezas; sin BOM %s, de más %s'
             % (len(set(en.values())), len(en), sorted(van - set(en)) or '-', sorted(set(en) - van) or '-'))
    pdf = os.path.join(FAB, 'zxv-schematic.pdf')
    ok &= di('esquema en PDF', os.path.exists(pdf) and os.path.getsize(pdf) > 50000,
             '%d bytes' % (os.path.getsize(pdf) if os.path.exists(pdf) else 0))
    return ok


if __name__ == '__main__':
    if '--check' not in sys.argv:
        generar()
    print('PAQUETE DE FABRICACIÓN, comprobado contra la placa')
    raise SystemExit(0 if comprobar() else 1)
