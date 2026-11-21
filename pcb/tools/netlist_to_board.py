"""netlist_to_board.py - Lleva el netlist del esquema a la placa, con el
Python de KiCad (lo que hace «Actualizar placa desde el esquema»).

    C:/KiCad/10.0/bin/python.exe pcb/tools/netlist_to_board.py zxv.net placa.kicad_pcb

- Cada huella se enlaza con su símbolo por el `path` (/<hoja>/<símbolo>), que
  es lo que KiCad empareja al actualizar; sin él, la siguiente actualización
  desde el esquema duplicaría todas las piezas.
- Si la placa ya existe, las huellas que siguen en el netlist conservan su
  sitio; se ponen al día el valor, los campos, DNP y las redes de los pads.
  Las nuevas se añaden fuera del contorno, en fila; las que ya no están se
  quitan (y se dicen).
- Si cambia la huella de una pieza, se cambia y se conserva su sitio; con
  `--recargar J4,J7`, también las de esas piezas (tras cambiar su biblioteca).
- Escribe con el escritor de KiCad (no con el `save_board` del MCP, que pierde
  la tabla de redes).
"""
import os
import sys

import pcbnew

KICAD = 'C:/KiCad/10.0/share/kicad/footprints'


def sexpr(text):
    """Lee una expresión S: listas de átomos y cadenas."""
    stack, cur, i, n = [], [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '(':
            stack.append(cur)
            cur = []
            i += 1
        elif c == ')':
            done, cur = cur, stack.pop()
            cur.append(done)
            i += 1
        elif c == '"':
            j, buf = i + 1, []
            while text[j] != '"':
                if text[j] == '\\':
                    j += 1
                buf.append(text[j])
                j += 1
            cur.append(''.join(buf))
            i = j + 1
        elif c.isspace():
            i += 1
        else:
            j = i
            while j < n and not text[j].isspace() and text[j] not in '()':
                j += 1
            cur.append(text[i:j])
            i = j
    return cur[0]


def get(node, key):
    for x in node[1:]:
        if isinstance(x, list) and x and x[0] == key:
            return x
    return None


def val(node, key):
    x = get(node, key)
    return x[1] if x and len(x) > 1 else None


def lib_path(lib, proj):
    if lib == 'zxv':
        return os.path.join(proj, 'zxv.pretty')
    return os.path.join(KICAD, lib + '.pretty')


RECARGAR = set()


def main(netfile, pcbfile):
    net = sexpr(open(netfile, encoding='utf-8').read())
    proj = os.path.dirname(os.path.abspath(pcbfile))
    comps = {}
    for c in get(net, 'components')[1:]:
        ref = val(c, 'ref')
        fields = {f[1][1]: (f[2] if len(f) > 2 else '') for f in (get(c, 'fields') or [None])[1:]}
        props = {val(p, 'name'): val(p, 'value') for p in c[1:] if isinstance(p, list) and p[0] == 'property'}
        comps[ref] = dict(value=val(c, 'value'), footprint=val(c, 'footprint'),
                          datasheet=val(c, 'datasheet') or '', description=val(c, 'description') or '',
                          path=val(get(c, 'sheetpath'), 'tstamps') + val(c, 'tstamps'),
                          sheetname=props.get('Sheetname', ''), sheetfile=props.get('Sheetfile', ''),
                          dnp='dnp' in props, lcsc=fields.get('LCSC', ''))
    nets = {}
    for n in get(net, 'nets')[1:]:
        name = val(n, 'name')
        for node in n[1:]:
            if isinstance(node, list) and node[0] == 'node':
                nets[(val(node, 'ref'), val(node, 'pin'))] = name

    board = pcbnew.LoadBoard(pcbfile) if os.path.exists(pcbfile) else pcbnew.CreateEmptyBoard()
    existing = {fp.GetPath().AsString(): fp for fp in board.GetFootprints() if not fp.IsBoardOnly()}   # los taladros no son del esquema
    netinfo = {}

    def netof(name):
        if name not in netinfo:
            ni = board.FindNet(name)
            if ni is None:
                ni = pcbnew.NETINFO_ITEM(board, name)
                board.Add(ni)
            netinfo[name] = ni
        return netinfo[name]

    seen, added, swapped = set(), [], []
    x0 = 0
    for ref in sorted(comps, key=lambda r: (r.rstrip('0123456789'), int(r[len(r.rstrip('0123456789')):] or 0))):
        c = comps[ref]
        lib, name = c['footprint'].split(':')
        fp = existing.get(c['path'])
        if fp is not None and (fp.GetFPID().GetUniStringLibItemName() != name or ref in RECARGAR):
            pos, rot, side = fp.GetPosition(), fp.GetOrientation(), fp.IsFlipped()
            board.Remove(fp)
            fp = None
            swapped.append(ref)
        else:
            pos = None
        if fp is None:
            fp = pcbnew.FootprintLoad(lib_path(lib, proj), name)
            if fp is None:
                raise SystemExit(f'{ref}: no encuentro la huella {c["footprint"]}')
            fp.SetFPID(pcbnew.LIB_ID(lib, name))
            if pos is not None:
                fp.SetPosition(pos)
                fp.SetOrientation(rot)
            else:
                fp.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x0), pcbnew.FromMM(-30)))
                x0 += 12
                added.append(ref)
            board.Add(fp)
        fp.SetReference(ref)
        fp.SetValue(c['value'])
        fp.SetPath(pcbnew.KIID_PATH(c['path']))
        fp.SetSheetname(c['sheetname'])
        fp.SetSheetfile(c['sheetfile'])
        fp.SetDNP(c['dnp'])
        for k in ('Datasheet', 'Description'):
            fp.SetField(k, c[k.lower()])
            fp.GetField(k).SetVisible(False)
        if c['lcsc']:
            fp.SetField('LCSC', c['lcsc'])
            fp.GetField('LCSC').SetVisible(False)
        for pad in fp.Pads():
            name_ = nets.get((ref, pad.GetNumber()))
            if name_ is not None:
                pad.SetNet(netof(name_))
            elif pad.GetNumber():
                pad.SetNet(board.GetNetInfo().OrphanedItem())
        seen.add(c['path'])

    gone = [fp.GetReference() for p, fp in existing.items() if p not in seen]
    for p, fp in existing.items():
        if p not in seen:
            board.Remove(fp)
    pcbnew.SaveBoard(pcbfile, board)
    print(f'{len(comps)} piezas; nuevas: {len(added)}; huella cambiada: {swapped or "-"}; quitadas: {gone or "-"}; '
          f'redes: {len(set(nets.values()))}')


if __name__ == '__main__':
    if len(sys.argv) > 3:                       # --recargar J4,J7: vuelve a cargar esas huellas de su biblioteca
        RECARGAR = set(sys.argv[4].split(',')) if sys.argv[3] == '--recargar' else set()
    main(*sys.argv[1:3])
