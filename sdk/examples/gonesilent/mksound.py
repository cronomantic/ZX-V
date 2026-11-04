#!/usr/bin/env python3
"""
mksound.py - La música y los efectos de Gone Silent, hechos con este script:

    python mksound.py

- gonesilent-title.pt3: el título (y la introducción), lento, en la menor.
- gonesilent-end.pt3: el final, del mismo tono pero más abierto.
- gonesilent.afb: los efectos AYFX, en el orden de FX_SHOT... (gonesilent.h).

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


def pt3(patterns, positions, samples, delay, loop=0, title="", author="Gone Silent"):
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


VIBRATO = [0, 1, 2, 1, 0, -1, -2, -1]

SAMPLES = {
    # 1: el colchón, que entra despacio, baja y se queda con un vibrato suave
    1: None,
    # 2: el bajo, con su golpe y una caída larga
    2: None,
    # 3: la campana de la melodía, que se apaga sola
    3: None,
}
pad = [line(v) for v in ramp([3, 5, 7, 9, 10, 11, 11, 10, 10, 9, 9, 8, 8, 7], 3)]
pad_loop = len(pad)
pad += [line(7, t) for t in VIBRATO for _ in range(2)]
SAMPLES[1] = (pad_loop, pad)
bass = [line(v) for v in ramp([14, 13, 12, 12, 11, 11, 10, 10, 10, 9, 9, 9], 2)]
SAMPLES[2] = (len(bass) - 1, bass)
bell = [line(v) for v in ramp([13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 3, 2, 2, 1, 1, 0], 3)]
SAMPLES[3] = (len(bell) - 1, bell)

ROWS = 64

# Los acordes, dos por patrón: su bajo y sus notas para el colchón.
CHORDS = {
    "Am": ("A2", ["A3", "C4", "E4", "C4"]),
    "F": ("F2", ["F3", "A3", "C4", "A3"]),
    "Dm": ("D3", ["D3", "F3", "A3", "F3"]),
    "E": ("E2", ["E3", "G#3", "B3", "G#3"]),
    "C": ("C3", ["C4", "E4", "G4", "E4"]),
    "G": ("G2", ["G3", "B3", "D4", "B3"]),
}


def pattern(chords, melody):
    """Dos acordes de 32 filas y la melodía (nota o None, filas), 64 filas."""
    pad_ev, bass_ev = [], []
    for name in chords:
        root, tones = CHORDS[name]
        bass_ev += [(root, 32)]
        pad_ev += [(t, 8) for t in tones]
    assert sum(r for _, r in melody) == ROWS
    return (channel(pad_ev, 1, 10), channel(bass_ev, 2, 12), channel(melody, 3, 9))


def title_song():
    p0 = pattern(["Am", "F"], [(None, 16), ("E5", 16), (None, 8), ("C5", 24)])
    p1 = pattern(["Dm", "E"], [("D5", 16), (None, 8), ("F5", 8), ("E5", 32)])
    p2 = pattern(["Am", "C"], [(None, 8), ("A4", 16), ("C5", 8), ("E5", 16), ("G5", 16)])
    p3 = pattern(["Dm", "E"], [("F5", 16), ("E5", 8), ("D5", 8), ("B4", 32)])
    return pt3([p0, p1, p2, p3], [0, 1, 2, 3], SAMPLES, delay=5, title="Gone Silent")


def end_song():
    p0 = pattern(["F", "C"], [(None, 8), ("A4", 16), ("C5", 8), ("F5", 32)])
    p1 = pattern(["G", "Am"], [("D5", 16), ("B4", 16), ("C5", 8), ("E5", 24)])
    p2 = pattern(["F", "G"], [("A5", 16), ("G5", 16), ("F5", 16), ("D5", 16)])
    p3 = pattern(["C", "Am"], [("E5", 32), (None, 8), ("A4", 24)])
    return pt3([p0, p1, p2, p3], [0, 1, 2, 3], SAMPLES, delay=6, title="Gone Silent - Lift-off")


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


def fx_shot():
    return effect([frame(lerp(15, 6, i, 8), lerp(0x040, 0x1C0, i, 8)) for i in range(8)])


def fx_bite():
    return effect([frame(lerp(15, 2, i, 12), 0x300 + (i & 1) * 0x80, lerp(8, 20, i, 12)) for i in range(12)])


def fx_turret():
    return effect([frame(lerp(14, 4, i, 14), 0x060 if i & 1 else 0x090) for i in range(14)])


def fx_kill():
    return effect([frame(lerp(14, 0, i, 22), lerp(0x200, 0x030, i, 22), lerp(4, 31, i, 22)) for i in range(22)])


def fx_pickup():
    return effect([frame(12, 0x0A0)] * 4 + [frame(lerp(13, 2, i, 10), 0x078) for i in range(10)])


def fx_use():
    return effect([frame(11, 0x100)] * 3 + [frame(lerp(11, 3, i, 5), 0x0C0) for i in range(5)])


def fx_log():
    return effect([frame(9, 0x050), frame(9, 0x050), frame(8, 0x060), frame(6, 0x060), frame(3, 0x060)])


def fx_denied():
    tone = [frame(12, 0x400)] * 4
    return effect(tone + [frame(0)] * 3 + tone + [frame(lerp(10, 2, i, 4), 0x400) for i in range(4)])


def fx_door():
    return effect([frame(lerp(6, 12, i, 6), None, lerp(31, 12, i, 6)) for i in range(6)] +
                  [frame(lerp(12, 0, i, 12), None, lerp(12, 4, i, 12)) for i in range(12)])


def fx_charge():
    return effect([frame(8, lerp(0x200, 0x080, i, 28)) for i in range(28)])


def fx_low():
    beep = [frame(12, 0x090)] * 6
    return effect(beep + [frame(0)] * 6 + beep)


def fx_siren():
    up = [frame(11, lerp(0x180, 0x080, i, 25)) for i in range(25)]
    return effect(up + up[::-1] + up + up[::-1])


def fx_wind(vol):
    # El viento: ruido que sube y baja despacio, dos segundos.
    seq = []
    for i in range(100):
        k = i % 50
        n = 24 + (k * 7 // 50 if k < 25 else (49 - k) * 7 // 50)
        v = vol - 2 + (k // 12 if k < 25 else (49 - k) // 12)
        seq.append(frame(max(v, 1), None, n))
    return effect(seq)


def fx_hum():
    # El zumbido de la planta: un tono grave (unos 120 Hz) que tiembla un poco,
    # dos segundos.
    return effect([frame(7, 0x380 + (4 if i % 6 < 3 else 0)) for i in range(100)])


def fx_lock():
    # La torreta que apunta: dos pitidos que suben.
    return effect([frame(10, 0x070)] * 3 + [frame(0)] * 2 + [frame(12, 0x050)] * 3)


def fx_screech():
    # La criatura que se yergue para atacar: un chillido agudo con ruido.
    return effect([frame(lerp(13, 5, i, 14), lerp(0x040, 0x028, i, 14), lerp(2, 14, i, 14))
                   for i in range(14)])


def fx_gurgle():
    # La incubadora, a punto de escupir: un borboteo grave que sube.
    return effect([frame(lerp(6, 12, i, 24), lerp(0x600, 0x300, i, 24) + (0x40 if i % 4 < 2 else 0), 24)
                   for i in range(24)])


def fx_spit():
    # El escupitajo: ruido que se apaga deprisa.
    return effect([frame(lerp(14, 3, i, 10), None, lerp(6, 20, i, 10)) for i in range(10)])


def fx_ping(tone):
    # El sensor de movimiento: un pitido corto que se apaga; más agudo cuanto
    # más cerca está lo que se mueve.
    return effect([frame(v, tone) for v in (13, 11, 8, 5, 3)])


def fx_flicker():
    # Un fluorescente que falla: un chasquido.
    return effect([frame(10, None, 2), frame(6, None, 8), frame(2, None, 14)])


def fx_death():
    return effect([frame(lerp(15, 0, i, 60), lerp(0x100, 0xF00, i, 60)) for i in range(60)])


EFFECTS = [
    fx_shot(),          # FX_SHOT
    fx_bite(),          # FX_BITE
    fx_turret(),        # FX_TURRET
    fx_kill(),          # FX_KILL
    fx_pickup(),        # FX_PICKUP
    fx_use(),           # FX_USE
    fx_log(),           # FX_LOG
    fx_denied(),        # FX_DENIED
    fx_door(),          # FX_DOOR
    fx_charge(),        # FX_CHARGE
    fx_low(),           # FX_LOW
    fx_siren(),         # FX_SIREN
    fx_wind(7),         # FX_WIND
    fx_wind(11),        # FX_WIND_STRONG
    fx_hum(),           # FX_HUM
    fx_death(),         # FX_DEATH
    fx_lock(),          # FX_LOCK
    fx_screech(),       # FX_SCREECH
    fx_ping(0x0A0),     # FX_PING_FAR
    fx_ping(0x078),     # FX_PING_MID
    fx_ping(0x058),     # FX_PING_NEAR
    fx_gurgle(),        # FX_GURGLE
    fx_spit(),          # FX_SPIT
    fx_flicker(),       # FX_FLICKER
]


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
    out = {
        "gonesilent-title.pt3": title_song(),
        "gonesilent-end.pt3": end_song(),
        "gonesilent.afb": bank(EFFECTS),
    }
    for name, data in out.items():
        with open(os.path.join(HERE, name), "wb") as f:
            f.write(data)
        print(name, len(data), "bytes")


if __name__ == "__main__":
    main()
