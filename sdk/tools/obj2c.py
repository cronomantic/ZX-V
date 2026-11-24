#!/usr/bin/env python3
# SPDX-License-Identifier: MIT (como el SDK, sdk/LICENSE).
"""Convierte un modelo Wavefront .obj (el de Blender y casi todos los
programas de 3D) en C para zxv_3d.h.

    obj2c.py MODELO.obj SALIDA.c NOMBRE [opciones]

Sale const zxv_model NOMBRE: los vértices en enteros de 16 bits y las caras
con sus colores. Las caras no convexas (una L, una sierra) y las de más de
32 lados se parten en triángulos (cortando orejas) que luego se juntan en
trozos convexos de hasta 32 lados; las que no tienen área se quitan. El
modelo se centra (sin --keep-origin) y se escala para que su lado más largo
mida --size (en unidades del mundo, por defecto 2). Se pasa del .obj (y
arriba, z hacia la cámara) al de zxv_3d (z hacia delante) dándole la vuelta
a la z; como así el modelo se ve igual, las caras del .obj (en el sentido
contrario al reloj vistas desde fuera) se dan la vuelta para quedar en el
del reloj.

Los colores de cada cara salen de su material (usemtl):
  - un material llamado zxv_OSCURO_CLARO (zxv_1_9, por ejemplo), esos
    colores de ZX-V; con _two, _flat o _decal detrás (zxv_2_10_two), esas
    opciones (abajo);
  - si no, el color difuso (Kd) de su .mtl (mtllib): el más parecido de los
    16 y su pareja (el mismo color con brillo y sin él) son el claro y el
    oscuro;
  - --mat NOMBRE=OSCURO,CLARO[,two][,flat][,decal] para cambiar uno: two, se
    ve por los dos lados; flat, sin luz; decal, va pegado encima de otra
    cara (una puerta en una pared) y se dibuja después de ella;
  - sin material, --default OSCURO,CLARO (por defecto 1,9: azul).

Opciones: --size S, --keep-origin, --mat ..., --default ..., --two-sided
(todas las caras, de los dos lados).

Con --bas, sale ZX Basic para zxv3d.bas (en el fork de ZX Basic, ZX-V D111)
en lugar de C: las matrices NOMBRE_v (x, y, z de cada vértice, Integer) y
NOMBRE_f (por cara: lados, material y vértices, UInteger) y el SUB
NOMBRE_model(n), que lo pone como el modelo n de MODEL3D:

    obj2c.py casa.obj casa.bas casa --bas
    #include "casa.bas"
    casa_model(0)
"""
import math
import os
import re
import sys

# Los 16 colores de siempre (spec §4.1): el nivel normal es 0xD7.
PALETTE = []
for i in range(16):
    lv = 0xFF if i & 8 else 0xD7
    PALETTE.append(((lv if i & 2 else 0), (lv if i & 4 else 0), (lv if i & 1 else 0)))


def die(msg):
    raise SystemExit("obj2c: " + msg)


def nearest(rgb):
    best, bi = None, 0
    for i, p in enumerate(PALETTE):
        if i == 8:                                      # el negro brillante es el negro
            continue
        d = sum((a - b) ** 2 for a, b in zip(rgb, p))
        if best is None or d < best:
            best, bi = d, i
    return bi


def colours_of_kd(kd):
    """El más parecido y su pareja del mismo color: con brillo, el claro (y sin
    brillo, el oscuro); sin brillo, el oscuro (y con brillo, el claro)."""
    c = nearest(tuple(max(0, min(255, round(v * 255))) for v in kd))
    if c == 0:
        return 0, 0
    return (c - 8, c) if c >= 9 else (c, c + 8)


def read_mtl(path):
    mats, cur = {}, None
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            for line in f:
                p = line.split()
                if not p:
                    continue
                if p[0] == "newmtl":
                    cur = " ".join(p[1:])
                elif p[0] == "Kd" and cur is not None and len(p) >= 4:
                    mats[cur] = tuple(float(v) for v in p[1:4])
    except OSError:
        print(f"obj2c: aviso: no se puede leer {path}", file=sys.stderr)
    return mats


# --- Caras: convexas o no, partirlas --------------------------------------------------

def newell(p):
    """La normal de toda la cara (Newell): vale también con vértices en línea
    o una cara no convexa."""
    n = [0, 0, 0]
    for i in range(len(p)):
        a, b = p[i], p[(i + 1) % len(p)]
        n[0] += (a[1] - b[1]) * (a[2] + b[2])
        n[1] += (a[2] - b[2]) * (a[0] + b[0])
        n[2] += (a[0] - b[0]) * (a[1] + b[1])
    return n


class Flat:
    """Los vértices de una cara vistos en el plano en que más se ven (sin el
    eje de la componente más larga de la normal; u y v, los otros dos en
    orden) y s, el sentido, como en zxv_3d (lib/3d.c), con enteros exactos."""

    def __init__(self, p):
        n = newell(p)
        drop = max(range(3), key=lambda k: abs(n[k]))
        self.p, self.u, self.v = p, (drop + 1) % 3, (drop + 2) % 3
        self.s = (n[drop] > 0) - (n[drop] < 0)

    def cross(self, ia, ib, ic):
        a, b, c, u, v = self.p[ia], self.p[ib], self.p[ic], self.u, self.v
        return ((b[u] - a[u]) * (c[v] - a[v]) - (b[v] - a[v]) * (c[u] - a[u])) * self.s

    def convex(self, ring):
        """¿Convexo el polígono de esos vértices? Todas las vueltas hacia el
        mismo lado y una sola vuelta entera (una estrella da dos)."""
        n, changes, first, last = len(ring), 0, 0, 0
        for i in range(n):
            if self.cross(ring[i - 1], ring[i], ring[(i + 1) % n]) < 0:
                return False
            du = self.p[ring[(i + 1) % n]][self.u] - self.p[ring[i]][self.u]
            sg = (du > 0) - (du < 0)
            if sg:
                if not first:
                    first = sg
                elif sg != last:
                    changes += 1
                last = sg
        if first and first != last:
            changes += 1
        return changes <= 2

    def in_tri(self, ip, ia, ib, ic):
        p = self.p[ip]
        if any(p == self.p[k] for k in (ia, ib, ic)):
            return False
        return self.cross(ia, ib, ip) >= 0 and self.cross(ib, ic, ip) >= 0 and self.cross(ic, ia, ip) >= 0

    def crosses_itself(self):
        """¿Se cruzan dos lados que no son vecinos?"""
        n, u, v = len(self.p), self.u, self.v

        def orient(a, b, c):
            d = (b[u] - a[u]) * (c[v] - a[v]) - (b[v] - a[v]) * (c[u] - a[u])
            return (d > 0) - (d < 0)
        for i in range(n):
            a, b = self.p[i], self.p[(i + 1) % n]
            for j in range(i + 2, n):
                if i == 0 and j == n - 1:
                    continue
                c, d = self.p[j], self.p[(j + 1) % n]
                if orient(a, b, c) * orient(a, b, d) < 0 and orient(c, d, a) * orient(c, d, b) < 0:
                    return True
        return False

    def triangles(self):
        """Cortando orejas (un vértice convexo sin otro dentro de su
        triángulo); los vértices en línea se quitan sin triángulo. Si no hay
        oreja (se cruza consigo misma), por cualquier vértice."""
        w, out = list(range(len(self.p))), []
        while len(w) >= 3:
            m, pick, any_ = len(w), None, None
            for i in range(m):
                ia, ib, ic = w[i - 1], w[i], w[(i + 1) % m]
                cr = self.cross(ia, ib, ic)
                if cr < 0:
                    continue
                if any_ is None:
                    any_ = i
                if cr == 0 or m == 3 or not any(self.in_tri(k, ia, ib, ic) for k in w if k not in (ia, ib, ic)):
                    pick = i
                    break
            if pick is None:
                pick = any_ if any_ is not None else 0
            ia, ib, ic = w[pick - 1], w[pick], w[(pick + 1) % m]
            if self.cross(ia, ib, ic):
                out.append([ia, ib, ic])
            if m == 3:
                break
            del w[pick]
        return out

    def pieces(self, most):
        """La cara en trozos convexos de hasta most lados: en triángulos y
        luego, mientras se pueda, dos vecinos juntos en uno."""
        parts = self.triangles()
        merged = True
        while merged:
            merged = False
            for i in range(len(parts)):
                for j in range(i + 1, len(parts)):
                    joined = self.join(parts[i], parts[j], most)
                    if joined:
                        parts[i] = joined
                        del parts[j]
                        merged = True
                        break
                if merged:
                    break
        return parts

    def join(self, a, b, most):
        """a y b en uno, si comparten un lado y juntos son convexos y de hasta
        most lados; si no, None."""
        if len(a) + len(b) - 2 > most:
            return None
        for k in range(len(a)):
            x, y = a[k], a[(k + 1) % len(a)]
            for t in range(len(b)):
                if b[t] == y and b[(t + 1) % len(b)] == x:
                    ra = a[k + 1:] + a[:k + 1]              # de y a x
                    rb = b[t + 1:] + b[:t + 1]              # de x a y
                    joined = ra + rb[1:-1]
                    return joined if self.convex(joined) else None
        return None


def write(out, lines):
    try:
        with open(out, "w", encoding="utf-8", newline="\n") as f:
            f.write("\n".join(lines) + "\n")
    except OSError as e:
        die(f"no se puede escribir {out}: {e.strerror}")


def main(argv):
    opts = {"--size": "2", "--default": "1,9"}
    mat_over = {}
    flags = set()
    pos = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ("--size", "--default", "--mat"):
            if i + 1 >= len(argv):
                die(f"{a}: falta el valor")
            if a == "--mat":
                name, eq, spec = argv[i + 1].partition("=")
                if not eq or not name:
                    die(f"--mat {argv[i + 1]}: tiene que ser NOMBRE=OSCURO,CLARO[,two][,flat][,decal]")
                mat_over[name] = spec
            else:
                opts[a] = argv[i + 1]
            i += 2
        elif a in ("--keep-origin", "--two-sided", "--bas"):
            flags.add(a)
            i += 1
        elif a.startswith("--"):
            die(f"opción desconocida: {a}")
        else:
            pos.append(a)
            i += 1
    if len(pos) != 3:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    src, out, name = pos
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
        die(f"{name}: el nombre tiene que valer en C (letras, cifras y _, sin empezar por cifra)")
    try:
        size = float(opts["--size"])
    except ValueError:
        die(f"--size {opts['--size']}: no es un número")
    if not 0 < size < float("inf"):
        die(f"--size {opts['--size']}: tiene que ser mayor que 0")
    verts, faces, kd = [], [], {}
    mat = None
    try:
        f = open(src, encoding="utf-8", errors="replace")
    except OSError as e:
        die(f"no se puede leer {src}: {e.strerror}")
    with f:
        for lineno, line in enumerate(f, 1):
            line = re.sub(r"(^|\s)#.*", "", line)     # los comentarios, también al final de una línea
            p = line.split()
            if not p:
                continue
            if p[0] == "v":
                try:
                    v = [float(x) for x in p[1:4]]
                except ValueError:
                    v = []
                if len(v) != 3 or not all(math.isfinite(x) for x in v):
                    die(f"{src}:{lineno}: un vértice son tres números (x, y, z): {line.strip()!r}")
                verts.append(v)
            elif p[0] == "f":
                idx = []
                for tok in p[1:]:
                    try:
                        n = int(tok.split("/")[0])
                    except ValueError:
                        die(f"{src}:{lineno}: «{tok}» no es un vértice de una cara")
                    idx.append(n - 1 if n > 0 else len(verts) + n)
                if any(k < 0 or k >= len(verts) for k in idx):
                    die(f"{src}:{lineno}: un vértice que no existe")
                if len(idx) >= 3:
                    faces.append((mat, idx[::-1], lineno))  # del contrario al reloj, al del reloj
            elif p[0] == "usemtl":
                mat = " ".join(p[1:])
            elif p[0] == "mtllib":
                kd.update(read_mtl(os.path.join(os.path.dirname(src), " ".join(p[1:]))))
    if not verts or not faces:
        die(f"{src}: sin vértices o sin caras")

    # Centrar y escalar: el lado más largo, --size; los enteros, hasta ±4096.
    lo = [min(v[k] for v in verts) for k in range(3)]
    hi = [max(v[k] for v in verts) for k in range(3)]
    centre = [0.0] * 3 if "--keep-origin" in flags else [(a + b) / 2 for a, b in zip(lo, hi)]
    reach = max(max(abs(v[k] - centre[k]) for v in verts) for k in range(3)) or 1.0
    unit = reach / 4096.0                               # lo que mide una unidad del modelo, en el .obj
    longest = max(b - a for a, b in zip(lo, hi)) or 1.0
    world_per_obj = size / longest
    iverts = [[round((v[0] - centre[0]) / unit), round((v[1] - centre[1]) / unit), round(-(v[2] - centre[2]) / unit)]
              for v in verts]
    scale = round(unit * world_per_obj * 65536)
    if scale < 1:
        die("--size demasiado pequeño para tanto detalle")

    def colours(m):
        spec = mat_over.get(m)
        extra = []
        if spec is None and m and m.startswith("zxv_"):
            spec = ",".join(m.split("_")[1:])          # zxv_2_10_two_flat...
        if spec is None and m in kd:
            d, l = colours_of_kd(kd[m])
            spec = f"{d},{l}"
        if spec is None:
            spec = opts["--default"]
        parts = spec.split(",")
        try:
            d, l = int(parts[0]), int(parts[1])
        except (ValueError, IndexError):
            die(f"colores no válidos: {spec!r}")
        if not (0 <= d <= 15 and 0 <= l <= 15):
            die(f"colores fuera de 0-15: {spec!r}")
        extra = parts[2:]
        fl, bits = [], 0
        if "two" in extra or "--two-sided" in flags:
            fl.append("ZXV_3D_TWO_SIDED")
            bits |= 0x100
        if "flat" in extra:
            fl.append("ZXV_3D_FLAT")
            bits |= 0x200
        if "decal" in extra:
            fl.append("ZXV_3D_DECAL")
            bits |= 0x400
        if "--bas" in flags:                            # ZXV_3D_MAT, ya calculado
            return str(d | l << 4 | bits)
        return f"ZXV_3D_MAT({d}, {l}, {' | '.join(fl) if fl else 0})"

    def corner_first(idx):
        """zxv_3d saca la normal (y si la cara se ve) de los tres primeros
        vértices: que la cara empiece por la esquina que mejor la da. Con
        tres en línea (un corte en T, normal en Blender) saldría cero."""
        n = len(idx)
        p = [iverts[k] for k in idx]
        newell = [0, 0, 0]                              # la normal de toda la cara
        for i in range(n):
            a, b = p[i], p[(i + 1) % n]
            newell[0] += (a[1] - b[1]) * (a[2] + b[2])
            newell[1] += (a[2] - b[2]) * (a[0] + b[0])
            newell[2] += (a[0] - b[0]) * (a[1] + b[1])
        best, at = None, 0
        for r in range(n):
            a, b, c = p[r], p[(r + 1) % n], p[(r + 2) % n]
            e1 = [b[k] - a[k] for k in range(3)]
            e2 = [c[k] - a[k] for k in range(3)]
            cr = (e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0])
            score = sum(cr[k] * newell[k] for k in range(3))
            if best is None or score > best:
                best, at = score, r
        return idx[at:] + idx[:at]

    out_faces, flat_faces = [], 0
    for m, idx, lineno in faces:
        c = colours(m)
        fl = Flat([iverts[k] for k in idx])
        if not fl.s:
            flat_faces += 1                             # sin área: no se vería
            continue
        if len(idx) <= 32 and fl.convex(list(range(len(idx)))):
            out_faces.append((c, corner_first(idx)))
            continue
        if fl.crosses_itself():
            print(f"obj2c: aviso: {src}:{lineno}: la cara se cruza consigo misma: no saldrá como en el .obj",
                  file=sys.stderr)
        for piece in fl.pieces(32):                     # no convexa o de más de 32 lados
            out_faces.append((c, corner_first([idx[k] for k in piece])))
    if flat_faces:
        print(f"obj2c: aviso: {flat_faces} caras sin área (vértices repetidos, en línea o que se cruzan) se quitan",
              file=sys.stderr)
    if not out_faces:
        die(f"{src}: ninguna cara tiene área")
    if len(iverts) > 512:
        print(f"obj2c: aviso: {len(iverts)} vértices; zxv_3d admite 512 por modelo (ZXV_3D_MAX_VERTS)",
              file=sys.stderr)

    if "--bas" in flags:
        if not name.replace("_", "a").isalnum() or name[0].isdigit():
            die(f"{name}: el nombre tiene que valer en BASIC (letras, cifras y _)")
        nums_v = [str(c) for v in iverts for c in v]
        nums_f = []
        for c, idx in out_faces:
            nums_f += [str(len(idx)), c] + [str(k) for k in idx]

        def array(var, typ, nums, per_line):
            out = [f"DIM {var}({len(nums) - 1}) AS {typ} => {{ _"]
            for k in range(0, len(nums), per_line):
                end = " _" if k + per_line < len(nums) else " }"
                out.append("    " + ", ".join(nums[k:k + per_line]) + ("," if k + per_line < len(nums) else "") + end)
            return out

        lines = [f"' Generado por sdk/tools/obj2c.py --bas a partir de {os.path.basename(src)}: para",
                 "' zxv3d.bas (los vértices, x, y, z; por cara: lados, material y vértices).", ""]
        lines += array(f"{name}_v", "INTEGER", nums_v, 12) + [""]
        lines += array(f"{name}_f", "UINTEGER", nums_f, 16) + [""]
        lines += [f"' {name}_model(n): el modelo n de MODEL3D, ADD3D...",
                  f"SUB {name}_model(n AS UBYTE)",
                  f"    MODEL3D n, @{name}_v(0), {len(iverts)}, @{name}_f(0), {len(out_faces)}, {scale}",
                  "END SUB"]
        write(out, lines)
        print(f"obj2c: {len(iverts)} vértices, {len(out_faces)} caras (BASIC)")
        return 0

    lines = [f"/* Generado por sdk/tools/obj2c.py a partir de {os.path.basename(src)}. */",
             '#include "zxv_3d.h"', "",
             f"static const int16_t {name}_v[{len(iverts)}][3] = {{"]
    for v in iverts:
        lines.append(f"    {{ {v[0]}, {v[1]}, {v[2]} }},")
    lines.append("};")
    lines.append(f"static const uint16_t {name}_f[] = {{")
    for c, idx in out_faces:
        lines.append(f"    {len(idx)}, {c}, " + ", ".join(str(k) for k in idx) + ",")
    lines.append("};")
    lines.append(f"const zxv_model {name} = {{ {name}_v, {name}_f, {len(iverts)}, {len(out_faces)}, {scale} }};")
    write(out, lines)
    print(f"obj2c: {len(iverts)} vértices, {len(out_faces)} caras")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
