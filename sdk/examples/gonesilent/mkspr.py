#!/usr/bin/env python3
"""
mkspr.py - Los sprites de gonesilent.gfx, comprimidos para el juego: de
cada fila, solo sus tramos sin transparente (el color 8), que es lo que se
dibuja. Ocupan la tercera parte. Lo hace el Makefile; gonesilent.gfx sigue
siendo el original, el que hace mkcreature.py y se retoca con zxv-gfx.

    python mkspr.py gonesilent.gfx SALIDA.spr

El fichero, todo en little endian:

    u16 n                   cuántos sprites (los números de 0 a n - 1)
    u32 pos[n]              dónde empieza cada uno (0: no está)

y cada sprite:

    u8 w, h                 sus medidas (de 1 a 255)
    u16 fila[h]             dónde empieza cada fila, desde el sprite (las
                            filas iguales, una sola vez)

y cada fila, su número de tramos y, de cada uno, la x, el ancho y los
píxeles, de dos en dos por byte (el primero en el nibble bajo).
"""
import struct
import sys

TRANSPARENT = 8


def read_sprites(path):
    data = open(path, 'rb').read()
    if data[:4] != b'ZXVG' or len(data) < 8 or data[4] != 1:
        sys.exit('%s: no es un fichero de gráficos' % path)
    count, = struct.unpack_from('<H', data, 6)
    pos, out = 8, {}
    for _ in range(count):
        kind, _, num, ln = struct.unpack_from('<BBHI', data, pos)
        d = data[pos + 8:pos + 8 + ln]
        pos += 8 + ln
        if kind != 1:
            continue
        w, h = struct.unpack_from('<HH', d)
        row = (w + 1) // 2
        out[num] = (w, h, [[(d[4 + y * row + x // 2] >> (x & 1) * 4) & 15 for x in range(w)]
                           for y in range(h)])
    return out


def pack_row(r):
    segs, x = [], 0
    while x < len(r):
        if r[x] == TRANSPARENT:
            x += 1
            continue
        n = 0
        while x + n < len(r) and r[x + n] != TRANSPARENT:
            n += 1
        segs.append((x, r[x:x + n]))
        x += n
    out = bytearray([len(segs)])
    for x, px in segs:
        out += bytes((x, len(px)))
        for k in range(0, len(px), 2):
            out.append(px[k] | (px[k + 1] << 4 if k + 1 < len(px) else 0))
    return bytes(out)


def pack_sprite(w, h, rows):
    if not (0 < w < 256 and 0 < h < 256):
        sys.exit('sprite de %d x %d: no cabe (como mucho, 255 x 255)' % (w, h))
    body, at, offs = bytearray(), {}, []
    head = 2 + 2 * h
    for r in rows:
        p = pack_row(r)
        if p not in at:
            at[p] = head + len(body)
            body += p
        offs.append(at[p])
    if head + len(body) > 0xFFFF:
        sys.exit('sprite de %d x %d: demasiado grande' % (w, h))
    return bytes((w, h)) + struct.pack('<%dH' % h, *offs) + bytes(body)


def main():
    src, dst = sys.argv[1:3]
    spr = read_sprites(src)
    n = max(spr) + 1 if spr else 0
    out = bytearray(2 + 4 * n)
    struct.pack_into('<H', out, 0, n)
    for k in range(n):
        if k in spr:
            struct.pack_into('<I', out, 2 + 4 * k, len(out))
            out += pack_sprite(*spr[k])
    open(dst, 'wb').write(out)
    print('%s: %d sprites, %d bytes' % (dst, len(spr), len(out)))


if __name__ == '__main__':
    main()
