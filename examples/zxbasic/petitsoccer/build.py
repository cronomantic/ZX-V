#!/usr/bin/env python3
# SPDX-License-Identifier: MIT (como el SDK, sdk/LICENSE).
"""Compila Petit Soccer con ZX Basic, con la música y los efectos dentro.

    python build.py [ZXBC]      compila petitsoccer.zxv y apunta sus huellas
    python build.py --check     comprueba que petitsoccer.zxv es el de las fuentes

ZXBC es zxbc (o zxbc.py) de la rama zxv del fork de ZX Basic, v1.19.0-zxv.6 o
posterior (--embed); sin él, la variable ZXBC o zxbc en el PATH.

El .zxv va en el repositorio porque la CI no tiene zxbc (necesita Python 3.14):
el disco ROM y los paquetes lo copian tal cual. petitsoccer.sha256 apunta las
huellas de las fuentes y del .zxv (como sha256sum); la prueba
petitsoccer_test.py falla si alguien cambia las fuentes y no lo vuelve a
compilar con este programa.
"""
import hashlib
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCES = ['petitsoccer.bas', 'sprites.bas', 'petitsoccer.pt3', 'petitsoccer.afb']
OUTPUT = 'petitsoccer.zxv'
STAMP = 'petitsoccer.sha256'


def digest(name):
    """La huella de un fichero; la de un listado, con LF (en Windows, git lo
    deja con CR LF)."""
    with open(os.path.join(HERE, name), 'rb') as f:
        data = f.read()
    if name.endswith('.bas'):
        data = data.replace(b'\r\n', b'\n')
    return hashlib.sha256(data).hexdigest()


def stamp_text():
    return ''.join(f'{digest(n)}  {n}\n' for n in SOURCES + [OUTPUT])


def check():
    path = os.path.join(HERE, STAMP)
    if not os.path.exists(path):
        print(f'falta {STAMP}: compila con build.py')
        return 1
    want = open(path, encoding='ascii').read()
    got = stamp_text()
    if want == got:
        print(f'{OUTPUT} es el de las fuentes')
        return 0
    old = dict(reversed(l.split('  ', 1)) for l in want.splitlines())
    new = dict(reversed(l.split('  ', 1)) for l in got.splitlines())
    for n in SOURCES + [OUTPUT]:
        if old.get(n) != new.get(n):
            print(f'{n} ha cambiado desde la última compilación')
    print('vuelve a compilarlo con build.py')
    return 1


def build(zxbc):
    cmd = [zxbc] if not zxbc.endswith('.py') else [sys.executable, zxbc]
    cmd += ['--arch', 'zxv', '--embed', 'petitsoccer.pt3', '--embed', 'petitsoccer.afb',
            '-o', OUTPUT, 'petitsoccer.bas']
    r = subprocess.run(cmd, cwd=HERE)
    if r.returncode:
        return r.returncode
    with open(os.path.join(HERE, STAMP), 'w', encoding='ascii', newline='\n') as f:
        f.write(stamp_text())
    print(f'{OUTPUT} y {STAMP}, al día')
    return 0


def main():
    args = sys.argv[1:]
    if args == ['--check']:
        return check()
    if len(args) > 1 or args and args[0].startswith('-'):
        print(__doc__)
        return 2
    zxbc = args[0] if args else os.environ.get('ZXBC') or shutil.which('zxbc')
    if not zxbc:
        print('no encuentro zxbc: dilo en la línea de órdenes o en la variable ZXBC')
        return 2
    return build(zxbc)


if __name__ == '__main__':
    sys.exit(main())
