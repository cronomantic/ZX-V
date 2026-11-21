"""schematic_lib.py - Lo que necesita schematic.py para escribir hojas de KiCad 10:
símbolos de biblioteca, posición de los pines, instancias, hilos, etiquetas
y uniones. Las coordenadas van en mm, con y hacia abajo, como en la hoja.
"""
import math
import os
import re
import uuid as uuidlib

KSYM = 'C:/KiCad/10.0/share/kicad/symbols'
GRID = 1.27


def nid():
    return str(uuidlib.uuid4())


def block(t, i):
    """El bloque entre paréntesis que empieza en t[i]."""
    d = 0
    j = i
    while True:
        c = t[j]
        if c == '(':
            d += 1
        elif c == ')':
            d -= 1
            if d == 0:
                return t[i:j + 1]
        j += 1


def g(v):
    """Redondea a la rejilla (y quita el -0)."""
    r = round(round(v / GRID) * GRID, 4)
    return 0.0 if r == 0 else r


def fmt(v):
    s = ('%.4f' % v).rstrip('0').rstrip('.')
    return '0' if s in ('-0', '') else s


# ---------------------------------------------------------------- bibliotecas

class Libs:
    """Los símbolos: de las hojas que ya existen (ya aplanados) o de la biblioteca."""

    def __init__(self, proj, old_sheets):
        self.proj = proj
        self.blocks = {}
        for f in old_sheets:
            if not os.path.exists(f):
                continue
            t = open(f, encoding='utf-8').read()
            i = t.find('(lib_symbols')
            if i < 0:
                continue
            ls = block(t, i)
            for m in re.finditer(r'\n\t\t\(symbol "([^"]+)"', ls):
                self.blocks.setdefault(m.group(1), block(ls, m.start() + 1).replace('\r\n', '\n'))

    def _from_file(self, lib, name):
        path = os.path.join(self.proj, 'zxv.kicad_sym') if lib == 'zxv' else f'{KSYM}/{lib}.kicad_sym'
        t = open(path, encoding='utf-8').read().replace('\r\n', '\n')
        i = t.find('\t(symbol "%s"\n' % name)
        if i < 0:
            raise SystemExit(f'no encuentro {lib}:{name}')
        b = block(t, i + 1)
        m = re.search(r'\(extends "([^"]+)"\)', b)
        if not m:
            return b
        # Derivado: el dibujo y los pines del padre, las propiedades del hijo
        parent = self._from_file(lib, m.group(1))
        props = [block(b, x.start()) for x in re.finditer(r'\(property ', b)]
        pb = re.sub(r'\n\t\t\(property .*?\n\t\t\)', '', parent, flags=re.S)
        head = pb.index('\n', pb.index('(symbol')) + 1
        pb = pb[:head] + ''.join('\t\t' + p + '\n' for p in props) + pb[head:]
        pb = pb.replace(f'"{m.group(1)}_', f'"{name}_')
        pb = pb.replace(f'(symbol "{m.group(1)}"', f'(symbol "{name}"', 1)
        return pb

    def get(self, lib_id):
        if lib_id not in self.blocks:
            lib, name = lib_id.split(':')
            b = self._from_file(lib, name)
            b = b.replace(f'(symbol "{name}"', f'(symbol "{lib_id}"', 1)
            self.blocks[lib_id] = b
        return self.blocks[lib_id]

    def pins(self, lib_id):
        """{número: (x, y, ángulo)} en coordenadas de la biblioteca (y hacia arriba)."""
        b = self.get(lib_id)
        out = {}
        for m in re.finditer(r'\(symbol "[^"]*_(\d+)_(\d+)"', b):
            unit, style = int(m.group(1)), int(m.group(2))
            if unit not in (0, 1) or style not in (0, 1):
                continue
            sb = block(b, m.start())
            for pm in re.finditer(r'\(pin \w+ \w+', sb):
                pb = block(sb, pm.start())
                at = re.search(r'\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)', pb)
                num = re.search(r'\(number "([^"]*)"', pb).group(1)
                out[num] = (float(at.group(1)), float(at.group(2)), float(at.group(3) or 0))
        return out

    def field(self, lib_id, name):
        """(x, y, justify) de una propiedad en la biblioteca."""
        b = self.get(lib_id)
        m = re.search(r'\(property "%s" "[^"]*"\s*\(at ([-\d.]+) ([-\d.]+)' % name, b)
        if not m:
            return 0.0, 0.0, None
        pb = block(b, m.start())
        j = re.search(r'\(justify ([^)]*)\)', pb)
        return float(m.group(1)), float(m.group(2)), j.group(1) if j else None


def xf(x, y, rot):
    """De la biblioteca (y arriba) a la hoja (y abajo), girado `rot` grados."""
    px, py = x, -y
    a = math.radians(rot)
    c, s = round(math.cos(a)), round(math.sin(a))
    return px * c + py * s, -px * s + py * c


# ---------------------------------------------------------------- instancias viejas

def old_instances(files):
    """ref -> {uuid, pins{num: uuid}, props{nombre: valor}, dnp}."""
    out = {}
    for f in files:
        if not os.path.exists(f):
            continue
        t = open(f, encoding='utf-8').read().replace('\r\n', '\n')
        for m in re.finditer(r'^\t\(symbol\n\t\t\(lib_id "([^"]+)"', t, re.M):
            b = block(t, m.start() + 1)
            ref = re.search(r'\(property "Reference" "([^"]+)"', b).group(1)
            if ref.startswith('#'):
                continue
            props = dict(re.findall(r'\(property "([^"]+)" "([^"]*)"', b))
            pins = dict(re.findall(r'\(pin "([^"]+)"\s*\(uuid "([^"]+)"\)', b))
            out[ref] = dict(uuid=re.search(r'\n\t\t\(uuid "([^"]+)"\)', b).group(1), pins=pins, props=props,
                            dnp='(dnp yes)' in b, lib_id=m.group(1))
    return out


# ---------------------------------------------------------------- la hoja

def eff(justify=None, hide=False, size=1.27):
    j = f'\n\t\t\t\t(justify {justify})' if justify else ''
    return (f'\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size {fmt(size)} {fmt(size)})\n\t\t\t\t){j}\n\t\t\t)')


class Sym:
    def __init__(self, sheet, ref, lib_id, x, y, rot):
        self.sheet, self.ref, self.lib_id, self.x, self.y, self.rot = sheet, ref, lib_id, g(x), g(y), rot
        self._pins = sheet.libs.pins(lib_id)
        self.value = self.footprint = self.lcsc = self.dnp = self.fields = None
        self.hide_value = False

    def p(self, num):
        px, py, _ = self._pins[str(num)]
        dx, dy = xf(px, py, self.rot)
        return (g(self.x + dx), g(self.y + dy))


class Sheet:
    def __init__(self, libs, old, path, file_uuid, paper, title, project='ZX-V'):
        self.libs, self.old, self.path, self.file_uuid = libs, old, path, file_uuid
        self.paper, self.title, self.project = paper, title, project
        self.syms, self.wires, self.labels, self.ncs, self.texts, self.power = [], [], [], [], [], []
        self.extra = []

    # -- símbolos
    def sym(self, ref, lib_id, x, y, rot=0, value=None, footprint=None, lcsc=None, dnp=None,
            hide_value=False, fields=None):
        s = Sym(self, ref, lib_id, x, y, rot)
        s.value, s.footprint, s.lcsc, s.dnp, s.hide_value, s.fields = value, footprint, lcsc, dnp, hide_value, fields
        self.syms.append(s)
        return s

    def pwr(self, kind, pt, rot=0):
        """Un símbolo de alimentación con su pin en `pt` (GND hacia abajo, +5V hacia arriba)."""
        s = Sym(self, None, 'power:' + kind, pt[0], pt[1], rot)
        self.power.append(s)
        return s

    # -- hilos
    def wire(self, *pts):
        pts = [(g(x), g(y)) for x, y in pts]
        for a, b in zip(pts, pts[1:]):
            if a != b:
                assert a[0] == b[0] or a[1] == b[1], ('hilo en diagonal', a, b)
                self.wires.append((a, b))

    def label(self, name, pt, side='R', kind='label', shape='bidirectional'):
        """Etiqueta en `pt`; el texto sale hacia `side` (R, L, U, D)."""
        self.labels.append((kind, name, (g(pt[0]), g(pt[1])), side, shape))

    def stub(self, name, pt, side, length=5.08, kind='label', shape='bidirectional'):
        """Un hilo corto desde un pin y la etiqueta al final."""
        dx = {'R': length, 'L': -length, 'U': 0, 'D': 0}[side]
        dy = {'U': -length, 'D': length, 'R': 0, 'L': 0}[side]
        end = (pt[0] + dx, pt[1] + dy)
        self.wire(pt, end)
        self.label(name, end, side, kind, shape)

    def nc(self, pt):
        self.ncs.append((g(pt[0]), g(pt[1])))

    def text(self, s, x, y, size=1.27):
        self.texts.append((s, g(x), g(y), size))

    # -- salida
    def _sym_text(self, s, pwr_ref=None):
        libs = self.libs
        if s.ref is None:                                   # alimentación
            ref, val = pwr_ref, s.lib_id.split(':')[1]
            o = None
        else:
            ref = s.ref
            o = self.old.get(ref)
        props = dict(o['props']) if o else {}
        if s.ref is not None:
            if s.value is not None:
                props['Value'] = s.value
            if s.footprint is not None:
                props['Footprint'] = s.footprint
            if s.lcsc is not None:
                props['LCSC'] = s.lcsc
            props['Reference'] = ref
            if 'Value' not in props:
                props['Value'] = libs.field(s.lib_id, 'Value') and re.search(
                    r'\(property "Value" "([^"]*)"', libs.get(s.lib_id)).group(1)
            for k in ('Footprint', 'Datasheet', 'Description'):
                if k not in props:
                    m = re.search(r'\(property "%s" "([^"]*)"' % k, libs.get(s.lib_id))
                    props[k] = m.group(1) if m else ''
        else:
            props = {'Reference': ref, 'Value': val, 'Footprint': '', 'Datasheet': '', 'Description': ''}
        dnp = s.dnp if s.ref is not None and s.dnp is not None else (o['dnp'] if o else False)
        u = o['uuid'] if o else nid()
        pins = s._pins
        X, Y = s.x, s.y
        # sitio de la referencia y el valor
        fl = self._field_pos(s)
        lines = [f'\t(symbol\n\t\t(lib_id "{s.lib_id}")\n\t\t(at {fmt(X)} {fmt(Y)} {fmt(s.rot)})\n\t\t(unit 1)\n'
                 f'\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n\t\t(in_bom {"no" if s.ref is None else "yes"})\n'
                 f'\t\t(on_board {"no" if s.ref is None else "yes"})\n\t\t(in_pos_files yes)\n'
                 f'\t\t(dnp {"yes" if dnp else "no"})\n\t\t(uuid "{u}")']
        order = ['Reference', 'Value', 'Footprint', 'Datasheet'] + [k for k in props if k not in (
            'Reference', 'Value', 'Footprint', 'Datasheet', 'Description')] + ['Description']
        for k in order:
            if k not in props:
                continue
            vis = k == 'Reference' and s.ref is not None or k == 'Value' and not s.hide_value
            if k == 'Reference' and s.ref is None:
                vis = False
            if k in fl and vis:
                fx, fy, j = fl[k]
            else:
                fx, fy, j = X, Y, None
            hide = '' if vis else '\n\t\t\t(hide yes)'
            fa = 90 if s.rot in (90, 270) and vis else 0     # KiCad gira el texto con el símbolo
            val = props[k].replace('\\', '\\\\').replace('"', '\\"')
            lines.append(f'\t\t(property "{k}" "{val}"\n\t\t\t(at {fmt(fx)} {fmt(fy)} {fa}){hide}\n'
                         f'\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n{eff(j)}\n\t\t)')
        for num in sorted(pins, key=lambda n: (len(n), n)):
            pu = (o['pins'].get(num) if o else None) or nid()
            lines.append(f'\t\t(pin "{num}"\n\t\t\t(uuid "{pu}")\n\t\t)')
        lines.append(f'\t\t(instances\n\t\t\t(project "{self.project}"\n\t\t\t\t(path "{self.path}"\n'
                     f'\t\t\t\t\t(reference "{ref}")\n\t\t\t\t\t(unit 1)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)')
        return '\n'.join(lines)

    def _field_pos(self, s):
        libs = self.libs
        pins = [s.p(n) for n in s._pins]
        X, Y = s.x, s.y
        if s.fields:
            return s.fields
        if s.ref is None:
            fx, fy, j = libs.field(s.lib_id, 'Value')
            dx, dy = xf(fx, fy, s.rot)
            return {'Value': (X + dx, Y + dy, None)}
        if len(pins) == 2:
            (x1, y1), (x2, y2) = pins
            big = s.lib_id.split(':')[1] in ('D_Schottky', 'LED', 'Crystal', 'SW_Push', 'Polyfuse', 'Battery_Cell')
            if x1 == x2:                                    # vertical: el texto a la derecha
                d = 3.81 if big else 2.54
                return {'Reference': (X + d, Y - 1.27, 'left'), 'Value': (X + d, Y + 1.27, 'left')}
            d = 3.81 if big else 2.54                       # horizontal: encima y debajo
            return {'Reference': (X, Y - d, None), 'Value': (X, Y + d, None)}
        out = {}
        for k in ('Reference', 'Value'):
            fx, fy, j = libs.field(s.lib_id, k)
            dx, dy = xf(fx, fy, s.rot)
            if s.rot in (90, 270):
                j = None
            out[k] = (X + dx, Y + dy, j)
        return out

    def _junctions(self):
        pts = {}

        def add(p, n=1):
            pts[p] = pts.get(p, 0) + n
        allpins = []
        for s in self.syms + self.power:
            for n in s._pins:
                allpins.append(s.p(n))
        # partir los hilos donde toca algo
        marks = set(allpins) | {l[2] for l in self.labels} | {p for w in self.wires for p in w}
        split = []
        for a, b in self.wires:
            inner = [p for p in marks if p != a and p != b and _on(p, a, b)]
            inner.sort(key=lambda p: abs(p[0] - a[0]) + abs(p[1] - a[1]))
            seq = [a] + inner + [b]
            split += list(zip(seq, seq[1:]))
        self.wires = split
        for a, b in self.wires:
            add(a)
            add(b)
        for p in allpins:
            add(p)
        return [p for p, n in pts.items() if n >= 3]     # las etiquetas parten hilos, pero no ponen punto

    def write(self, extra_head=''):
        juncs = self._junctions()
        libs_used = sorted({s.lib_id for s in self.syms + self.power})
        out = [f'(kicad_sch\n\t(version 20260306)\n\t(generator "eeschema")\n\t(generator_version "10.0")\n'
               f'\t(uuid "{self.file_uuid}")\n\t(paper "{self.paper}")\n'
               f'\t(title_block\n\t\t(title "{self.title}")\n\t\t(company "ZX-V")\n\t\t(rev "0.1")\n\t)\n\t(lib_symbols']
        for l in libs_used:
            b = self.libs.get(l)
            b = '\n'.join(('\t' + x if x else x) for x in b.split('\n'))
            out.append('\t' + b.lstrip('\t'))
        out.append('\t)')
        if extra_head:
            out.append(extra_head)
        for p in juncs:
            out.append(f'\t(junction\n\t\t(at {fmt(p[0])} {fmt(p[1])})\n\t\t(diameter 0)\n\t\t(color 0 0 0 0)\n'
                       f'\t\t(uuid "{nid()}")\n\t)')
        for p in self.ncs:
            out.append(f'\t(no_connect\n\t\t(at {fmt(p[0])} {fmt(p[1])})\n\t\t(uuid "{nid()}")\n\t)')
        for a, b in self.wires:
            out.append(f'\t(wire\n\t\t(pts\n\t\t\t(xy {fmt(a[0])} {fmt(a[1])}) (xy {fmt(b[0])} {fmt(b[1])})\n\t\t)\n'
                       f'\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n\t\t(uuid "{nid()}")\n\t)')
        for kind, name, p, side, shape in self.labels:
            ang = {'R': 0, 'L': 180, 'U': 90, 'D': 270}[side]
            if kind == 'label':
                j = {'R': 'left bottom', 'L': 'right bottom', 'U': 'left bottom', 'D': 'right bottom'}[side]
                out.append(f'\t(label "{name}"\n\t\t(at {fmt(p[0])} {fmt(p[1])} {ang})\n{eff(j)[1:]}\n'
                           f'\t\t(uuid "{nid()}")\n\t)')
            else:
                j = {'R': 'left', 'L': 'right', 'U': 'left', 'D': 'right'}[side]
                out.append(f'\t({kind} "{name}"\n\t\t(shape {shape})\n\t\t(at {fmt(p[0])} {fmt(p[1])} {ang})\n'
                           f'{eff(j)[1:]}\n\t\t(uuid "{nid()}")\n\t)')
        for s, x, y, size in self.texts:
            s = s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
            out.append(f'\t(text "{s}"\n\t\t(exclude_from_sim no)\n\t\t(at {fmt(x)} {fmt(y)} 0)\n'
                       f'{eff("left top", size=size)[1:]}\n\t\t(uuid "{nid()}")\n\t)')
        for i, s in enumerate(self.syms):
            out.append(self._sym_text(s))
        for s in self.power:
            Sheet.pwr_n += 1
            kind = s.lib_id.split(':')[1]
            pref = '#FLG' if kind == 'PWR_FLAG' else '#PWR'
            out.append(self._sym_text(s, pwr_ref=f'{pref}{Sheet.pwr_n:03d}'))
        out.append('\t(sheet_instances\n\t\t(path "/"\n\t\t\t(page "1")\n\t\t)\n\t)\n\t(embedded_fonts no)\n)\n')
        return '\n'.join(out)


Sheet.pwr_n = 0


def _on(p, a, b):
    if a[0] == b[0] == p[0]:
        return min(a[1], b[1]) < p[1] < max(a[1], b[1])
    if a[1] == b[1] == p[1]:
        return min(a[0], b[0]) < p[0] < max(a[0], b[0])
    return False
