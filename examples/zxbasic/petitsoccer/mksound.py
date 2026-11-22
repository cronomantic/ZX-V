#!/usr/bin/env python3
"""
mksound.py - La música y los efectos de Petit Soccer, hechos con este script:

    python mksound.py

- petitsoccer.pt3: la música del título, alegre, en do mayor.
- petitsoccer.afb: los efectos AYFX, en el orden que usa el programa
  (0 chut, 1 pase, 2 silbato, 3 gol, 4 bote, 5 el silbato del final).

Los ficheros que da se pueden retocar luego con zxv-tracker; este script
los vuelve a hacer desde cero.
"""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))

# --- PT3 --------------------------------------------------------------------

# Las notas: C-1 es 0; la octava sube de 12 en 12.
NOTES = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}


def note(name):
    """'A3' -> el número de nota de PT3 (0 = C-1)."""
    return NOTES[name[:-1]] + 12 * (int(name[-1]) - 1)


def channel(events, sample, volume):
    """events: (nota o None, filas). Da los bytes de un canal de un patrón."""
    out = [0xD0 + sample, 0xC0 + volume]
    skip = None
    for n, rows in events:
        if rows != skip:
            out += [0xB1, rows]
            skip = rows
        out.append(0xC0 if n is None else 0x50 + note(n))
    return out


def pt3(patterns, positions, samples, delay, loop=0, title="", author="Petit Soccer"):
    """Un módulo PT3 (versión 3.6, tabla 2): patterns, lista de (a, b, c)."""
    hdr = bytearray(b"ProTracker 3.6 compilation of ")
    hdr += title.encode().ljust(32)[:32] + b" by " + author.encode().ljust(32)[:32] + b" "
    hdr += bytes([2, delay, len(positions), loop])
    body = bytearray(hdr + bytes(2 + 64 + 32))
    body += bytes(p * 3 for p in positions) + b"\xff"
    pat_tab = len(body)
    body += bytes(6 * len(patterns))
    struct.pack_into("<H", body, 0x67, pat_tab)
    for i, chans in enumerate(patterns):
        for c, data in enumerate(chans):
            struct.pack_into("<H", body, pat_tab + i * 6 + c * 2, len(body))
            body += bytes(data) + (b"\x00" if c == 0 else b"")
    for n, (lp, lines) in samples.items():
        struct.pack_into("<H", body, 0x69 + n * 2, len(body))
        body += bytes([lp, len(lines)])
        for b0, b1, tone in lines:
            body += bytes([b0, b1]) + struct.pack("<h", tone)
    struct.pack_into("<H", body, 0xA9, len(body))       # el ornamento 0, vacío
    body += bytes([0, 1, 0])
    first = struct.unpack_from("<H", body, 0x69 + 2)[0]
    for n in range(32):
        if struct.unpack_from("<H", body, 0x69 + n * 2)[0] == 0:
            struct.pack_into("<H", body, 0x69 + n * 2, first)
    for n in range(16):
        if struct.unpack_from("<H", body, 0xA9 + n * 2)[0] == 0:
            struct.pack_into("<H", body, 0xA9 + n * 2, struct.unpack_from("<H", body, 0xA9)[0])
    return bytes(body)


def line(vol, tone=0):
    """Una línea de muestra: con tono, sin ruido ni envolvente."""
    return (0x01, 0x80 | vol, tone)


def ramp(vols, each):
    return [v for v in vols for _ in range(each)]


# 1: el acompañamiento, corto y seco; 2: el bajo, con su golpe; 3: la melodía,
# que se sostiene con un vibrato suave
pluck = [line(v) for v in ramp([12, 11, 9, 7, 5, 3, 2, 1, 0], 1)]
bass = [line(v) for v in ramp([14, 13, 12, 11, 10, 9, 8, 7, 6, 5], 1)]
lead = [line(v) for v in ramp([13, 13, 12, 12, 11, 11], 1)]
lead_loop = len(lead)
lead += [line(10, t) for t in (0, 1, 2, 1, 0, -1, -2, -1)]
SAMPLES = {1: (len(pluck) - 1, pluck), 2: (len(bass) - 1, bass), 3: (lead_loop, lead)}

ROWS = 64

# Los acordes, dos por patrón: su bajo y sus notas para el acompañamiento
CHORDS = {
    "C": ("C3", ["C4", "E4", "G4", "E4"]),
    "G": ("G2", ["B3", "D4", "G4", "D4"]),
    "Am": ("A2", ["A3", "C4", "E4", "C4"]),
    "F": ("F2", ["A3", "C4", "F4", "C4"]),
}


def pattern(chords, melody):
    """Dos acordes de 32 filas y la melodía (nota o None, filas), 64 filas."""
    comp, bass_ev = [], []
    for name in chords:
        root, tones = CHORDS[name]
        bass_ev += [(root, 8), (root, 4), (root, 4), (root, 8), (root, 8)]
        comp += [(t, 4) for t in tones] * 2
    assert sum(r for _, r in melody) == ROWS, sum(r for _, r in melody)
    return (channel(melody, 3, 12), channel(bass_ev, 2, 12), channel(comp, 1, 9))


def title_song():
    p0 = pattern(["C", "G"], [("E5", 8), ("G5", 8), ("E5", 4), ("D5", 4), ("C5", 8),
                              ("D5", 8), ("G5", 8), ("B4", 8), ("D5", 8)])
    p1 = pattern(["Am", "F"], [("C5", 8), ("E5", 8), ("A5", 8), ("E5", 8),
                               ("F5", 8), ("A5", 8), ("G5", 4), ("F5", 4), ("E5", 8)])
    p2 = pattern(["C", "F"], [("G5", 4), ("E5", 4), ("C5", 8), ("E5", 8), ("G5", 8),
                              ("A5", 8), ("F5", 8), ("C5", 8), ("F5", 8)])
    p3 = pattern(["G", "C"], [("B4", 8), ("D5", 8), ("G5", 8), ("F5", 8),
                              ("E5", 16), ("C5", 8), (None, 8)])
    return pt3([p0, p1, p2, p3], [0, 1, 2, 3], SAMPLES, delay=3, title="Petit Soccer")


# --- AYFX -------------------------------------------------------------------

def frame(vol, tone=None, noise=None):
    """Un paso de 1/50 s: el volumen, el tono (periodo) y el ruido, si los hay."""
    e = vol & 15
    out = bytearray()
    if tone is None:
        e |= 0x10                       # sin tono
    else:
        e |= 0x20
        out += struct.pack("<H", tone & 0xFFF)
    if noise is None:
        e |= 0x80                       # sin ruido
    else:
        e |= 0x40
        out.append(noise & 31)
    return bytes([e]) + out


def effect(frames):
    return b"".join(frames) + b"\xD0\x20"


def lerp(a, b, i, n):
    return a + (b - a) * i // max(n - 1, 1)


def fx_kick():
    # El chut: un golpe seco y grave, con algo de ruido
    return effect([frame(15, 0x280, 10), frame(14, 0x340, 14)] +
                  [frame(lerp(12, 0, i, 5), lerp(0x400, 0x600, i, 5)) for i in range(5)])


def fx_pass():
    return effect([frame(11, 0x300, 12)] + [frame(lerp(9, 0, i, 4), lerp(0x380, 0x480, i, 4)) for i in range(4)])


def whistle(steps, vol=13):
    # El silbato: un tono agudo que tiembla, como la bolita de dentro
    return [frame(vol, 0x02A if i % 4 < 2 else 0x026) for i in range(steps)]


def fx_whistle():
    return effect(whistle(18) + [frame(lerp(12, 0, i, 4), 0x02A) for i in range(4)])


def fx_goal():
    # El gol: el público, ruido que sube, se mantiene y se va apagando
    up = [frame(lerp(5, 14, i, 14), None, lerp(20, 12, i, 14)) for i in range(14)]
    hold = [frame(14 - (i % 3 == 0), None, 12 + i % 3) for i in range(40)]
    down = [frame(lerp(13, 0, i, 40), None, lerp(13, 22, i, 40)) for i in range(40)]
    return effect(up + hold + down)


def fx_bounce():
    return effect([frame(10, 0x500, 20), frame(6, 0x580), frame(2, 0x600)])


def fx_final():
    # Tres pitidos: el último, largo
    gap = [frame(0)] * 5
    return effect(whistle(10) + gap + whistle(10) + gap + whistle(30) +
                  [frame(lerp(12, 0, i, 5), 0x02A) for i in range(5)])


EFFECTS = [fx_kick(), fx_pass(), fx_whistle(), fx_goal(), fx_bounce(), fx_final()]


def bank(effects):
    """Un banco .afb: cuántos y, de cada uno, su desplazamiento desde el
    segundo byte del propio desplazamiento."""
    head = bytearray([len(effects)]) + bytes(2 * len(effects))
    body = bytearray()
    for i, fx in enumerate(effects):
        at = len(head) + len(body)
        struct.pack_into("<H", head, 1 + 2 * i, at - (2 + 2 * i))
        body += fx
    return bytes(head + body)


def main():
    out = {"petitsoccer.pt3": title_song(), "petitsoccer.afb": bank(EFFECTS)}
    for name, data in out.items():
        with open(os.path.join(HERE, name), "wb") as f:
            f.write(data)
        print(name, len(data), "bytes")


if __name__ == "__main__":
    main()
