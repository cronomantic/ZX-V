"""place.py - El contorno, los taladros, las reglas y el sitio de cada pieza
de la placa de ZX-V (pcb/README.md §4.5), con el Python de KiCad.

    C:/KiCad/10.0/bin/python.exe pcb/tools/place.py pcb/ZX-V/ZX-V/ZX-V.kicad_pcb

Las coordenadas son en mm desde la esquina trasera izquierda de la placa (x a
la derecha, y hacia delante). Delante (y = 100), lo que toca el usuario: el
USB-A en el centro, los pulsadores, el encendido y los LED, THT en ángulo
recto, que asoman por el borde para salir por el frontal de la caja. Detrás (y = 0), lo que se enchufa una vez: USB-C, HDMI y jack. En el
lateral derecho, la microSD y la pila. Cuatro taladros M3 en las esquinas
(redondeadas), para la caja o unos pies. Las Pico, con el micro-USB hacia
delante y su zona libre para el cable (la de su huella).

No toca el cobre (pistas, vías y zonas); se puede volver a pasar.
"""
import sys

import pcbnew

X0, Y0 = 100.0, 50.0            # dónde cae la esquina trasera izquierda en la hoja
W, H = 100.0, 100.0

ULA_X, CPU_X, PICO_Y = 34.0, 66.0, 50.0     # separadas: entre sus zonas de cable cabe el USB-A, en el centro
RADIO = 3.0                                 # las esquinas, redondeadas

# ref: (x, y, ángulo). El ángulo es el de KiCad (antihorario).
SITIO = {
    # Las Pico, con el USB hacia delante y su zona libre para el cable (20 mm)
    'U2': (ULA_X, PICO_Y, 180), 'U1': (CPU_X, PICO_Y, 180),
    # Detrás, lo que se enchufa una vez: USB-C, HDMI (detrás de la ULA) y jack
    'J1': (17.5, 4.2, 180),             # USB-C de la alimentación
    'J7': (ULA_X, 8.01, 180),           # HDMI: su frente, en el borde (plano: 4,57 mm delante de los taladros)
    'J8': (66.0, 8.9, 270),             # jack
    # Delante, lo que toca el usuario: el USB-A en el centro; a la izquierda, el encendido y el modo
    # (ULA); a la derecha, el reinicio y los dos LED (la tarjeta y el enlace). Cada lado tiene 20,8 mm,
    # del taladro de la esquina a la zona del cable de la Pico, y va casi lleno
    'J4': (50.0, 87.38, 0),             # USB-A
    'SW3': (12.8, 98.6, 0), 'R41': (11.5, 89.0, 0),        # encendido: el cuerpo asoma 0,8 mm y la palanca, 3,4
    'SW2': (25.3, 97.8, 180),           # pulsadores: el cuerpo, hasta el borde; el botón asoma 4 mm
    'SW1': (79.05, 97.8, 180),
    'LED1': (83.1, 94.38, 0), 'R22': (84.4, 89.0, 0),      # LED de 3 mm tumbados: la punta, 1 mm fuera
    'LED2': (88.2, 94.38, 0), 'R39': (89.5, 89.0, 0),
    # El lateral derecho: la microSD (entra por la derecha) y, detrás, la pila
    'J3': (91.07, 66.0, 90),
    'BT1': (88.0, 39.0, 90),            # 31,3 mm con las pestañas (plano)
    # Alimentación: CC junto al USB-C; el interruptor de 5 V (Q1) y sus condensadores, en el lateral izquierdo
    'R1': (16.5, 12.5, 90), 'R2': (13.5, 12.5, 90), 'C1': (19.5, 12.5, 90),   # CC2 a la izquierda: no se cruzan
    'Q1': (7.0, 18.0, 0), 'R40': (11.0, 18.0, 90), 'C16': (14.0, 18.0, 90),
    'C2': (7.0, 26.0, 90), 'C15': (11.0, 26.0, 90),
    'D1': (50.0, 69.0, 0),              # VSYS de la CPU (pin 39), en el hueco
    'D2': (19.5, 72.0, 90),             # VSYS de la ULA
    'D3': (43.5, 10.0, 90),             # 5 V del HDMI
    'F2': (41.0, 14.35, 180),           # y su PTC, entre D3 y el pin 18 (a 45° de los dos)
    # HDMI: las 240 Ω entre el conector y los pines de la ULA
    # (por pares y a 2 mm de paso, bajo el conector, con sitio para que los pares lleguen a él
    # en diagonal: los pares del HDMI los ruta pairs.py)
    'R27': (25.11, 19.5, 90), 'R28': (27.1, 19.5, 90), 'R29': (29.1, 19.5, 90), 'R30': (31.1, 19.5, 90),
    'R23': (33.3, 19.5, 90), 'R24': (35.3, 19.5, 90), 'R25': (37.3, 19.5, 90), 'R26': (39.3, 19.5, 90),
    # El enlace: en el hueco entre las Pico, en dos columnas, cada una en su carril entre pines
    'R8': (48.0, 60.1, 0), 'R10': (48.0, 52.5, 0), 'R12': (48.0, 47.4, 0), 'R14': (48.0, 39.8, 0), 'R16': (48.0, 55.1, 0),
    'R9': (52.0, 57.6, 0), 'R11': (52.0, 50.0, 0), 'R13': (52.0, 44.9, 0), 'R15': (52.0, 37.3, 0), 'R17': (52.0, 42.4, 0),
    'R31': (19.5, 54.5, 90), 'R32': (19.5, 39.3, 90),      # VBLANK y canal de vuelta, junto a la ULA
    'R42': (19.5, 44.3, 90), 'R43': (19.5, 48.3, 270),     # el HPD del HDMI, a la altura del pin 29 (GP22)
    # Audio: el filtro entre el HDMI y el jack (el PWM viaja, la señal filtrada no), en tres filas
    'R33': (47.3, 13.5, 0), 'R34': (51.1, 13.5, 0), 'C11': (54.9, 13.5, 0), 'R35': (58.7, 13.5, 0),
    'C9': (47.3, 17.0, 0), 'C10': (51.1, 17.0, 0), 'R36': (54.9, 17.0, 0), 'R37': (58.7, 17.0, 0),
    'C12': (47.3, 20.5, 0), 'C13': (51.1, 20.5, 0), 'C14': (54.9, 20.5, 0), 'R38': (58.7, 20.5, 0),
    # Reloj, detrás a la derecha, con las dos tiras sin montar
    'U3': (83.0, 9.0, 0), 'Y1': (77.0, 9.0, 90), 'D4': (89.5, 10.0, 0), 'C8': (82.0, 14.5, 0),
    'R20': (75.0, 14.5, 90), 'R21': (77.5, 14.5, 90),
    'J5': (86.5, 16.0, 90),             # UART (sin montar)
    'J6': (86.5, 20.8, 90),             # I2C (sin montar)
    # USB: el par baja por debajo de la CPU, pasa por sus 22 Ω (una sobre otra, en el camino) y
    # baja al conector; cada diodo de protección, con el pad de la señal sobre su línea, justo
    # antes del conector (pairs.py); el VBUS, a la izquierda
    'R19': (57.0, 78.6, 0), 'R18': (57.0, 80.6, 0),
    'D5': (47.94, 82.9, 0), 'D6': (52.05, 82.9, 180),
    'F1': (43.2, 78.3, 0), 'C5': (43.2, 80.8, 0), 'C6': (43.2, 83.3, 0), 'C7': (46.8, 78.0, 90),
    # microSD: las pull-up y los condensadores, entre la CPU y el zócalo
    'R3': (80.0, 56.5, 0), 'R4': (80.0, 59.5, 0), 'R5': (80.0, 62.5, 0), 'R6': (80.0, 65.5, 0), 'R7': (80.0, 68.5, 0),
    'C3': (80.0, 71.5, 0), 'C4': (80.0, 74.5, 0),
}
TALADROS = [(4.0, 4.0), (96.0, 4.0), (4.0, 96.0), (96.0, 96.0)]   # M3, para separadores de caja o pies


# (clase, ancho, separación, vía, taladro, patrones de nombre de red)
CLASES = [
    ('Potencia', 0.8, 0.2, 0.8, 0.4, ['+5V', 'GND', '*VBUS*', '*VSYS*']),
    ('3V3', 0.5, 0.2, 0.6, 0.3, ['+3V3', '*RTC_VDD', '*VBAT', '*HDMI_5V']),
    ('HDMI', 0.2, 0.15, 0.6, 0.3, ['*ULA_D?P', '*ULA_D?N', '*ULA_CK?', '*HDMI_D?P', '*HDMI_D?N', '*HDMI_CK?']),
    ('USB', 0.25, 0.15, 0.6, 0.3, ['*USB_DP', '*USB_DN', '*USB_PICO_D?']),
]
# Los pares (KiCad los ve por la P y la N del final del nombre): (ancho, separación entre las dos)
PARES = {'HDMI': (0.2, 0.15), 'USB': (0.25, 0.15)}


def pt(x, y):
    return pcbnew.VECTOR2I(pcbnew.FromMM(X0 + x), pcbnew.FromMM(Y0 + y))


def main(pcbfile):
    b = pcbnew.LoadBoard(pcbfile)
    # Contorno y taladros: se rehacen
    for d in list(b.GetDrawings()):
        if d.GetLayer() == pcbnew.Edge_Cuts:
            b.Delete(d)                     # Delete y no Remove: con Remove, el SWIG deja la placa rota
    holes = {fp.GetReference(): fp for fp in b.GetFootprints() if fp.GetReference().startswith('H') and fp.IsBoardOnly()}
    def edge(kind, *pts):
        e = pcbnew.PCB_SHAPE(b, kind)
        e.SetLayer(pcbnew.Edge_Cuts)
        e.SetWidth(pcbnew.FromMM(0.1))
        if kind == pcbnew.SHAPE_T_ARC:
            e.SetArcGeometry(*[pt(*p) for p in pts])
        else:
            e.SetStart(pt(*pts[0]))
            e.SetEnd(pt(*pts[1]))
        b.Add(e)
    r, k = RADIO, RADIO * (1 - 0.5 ** 0.5)
    edge(pcbnew.SHAPE_T_SEGMENT, (r, 0), (W - r, 0))
    edge(pcbnew.SHAPE_T_SEGMENT, (W, r), (W, H - r))
    edge(pcbnew.SHAPE_T_SEGMENT, (W - r, H), (r, H))
    edge(pcbnew.SHAPE_T_SEGMENT, (0, H - r), (0, r))
    edge(pcbnew.SHAPE_T_ARC, (W - r, 0), (W - k, k), (W, r))
    edge(pcbnew.SHAPE_T_ARC, (W, H - r), (W - k, H - k), (W - r, H))
    edge(pcbnew.SHAPE_T_ARC, (r, H), (k, H - k), (0, H - r))
    edge(pcbnew.SHAPE_T_ARC, (0, r), (k, k), (r, 0))
    for i, (x, y) in enumerate(TALADROS, 1):
        h = holes.get(f'H{i}')
        if h is None:       # (cargar huellas después de tocar la placa falla a veces en el SWIG: solo si falta)
            h = pcbnew.FootprintLoad('C:/KiCad/10.0/share/kicad/footprints/MountingHole.pretty', 'MountingHole_3.2mm_M3')
            h.SetReference(f'H{i}')
            h.SetBoardOnly(True)
            h.SetExcludedFromBOM(True)
            h.SetExcludedFromPosFiles(True)
            b.Add(h)
        h.SetPosition(pt(x, y))

    # Reglas (JLCPCB, dos capas): pistas de 0,2, separación de 0,15 (el HDMI va a 0,5 mm de paso)
    ds = b.GetDesignSettings()
    ds.m_TrackMinWidth = pcbnew.FromMM(0.15)
    ds.m_MinClearance = pcbnew.FromMM(0.15)
    ds.m_ViasMinSize = pcbnew.FromMM(0.5)
    ds.m_MinThroughDrill = pcbnew.FromMM(0.3)
    ds.m_CopperEdgeClearance = pcbnew.FromMM(0.3)
    nc = ds.m_NetSettings.GetDefaultNetclass()
    nc.SetClearance(pcbnew.FromMM(0.15))
    nc.SetTrackWidth(pcbnew.FromMM(0.2))
    nc.SetViaDiameter(pcbnew.FromMM(0.6))
    nc.SetViaDrill(pcbnew.FromMM(0.3))
    # Las clases de red (el .kicad_pro las pierde cada vez que pcbnew guarda: se ponen aquí, siempre)
    ns = ds.m_NetSettings
    ns.ClearNetclassPatternAssignments()
    for name, width, clear, via, drill, patterns in CLASES:
        c = pcbnew.NETCLASS(name)
        c.SetTrackWidth(pcbnew.FromMM(width))
        c.SetClearance(pcbnew.FromMM(clear))
        c.SetViaDiameter(pcbnew.FromMM(via))
        c.SetViaDrill(pcbnew.FromMM(drill))
        if name in PARES:
            c.SetDiffPairWidth(pcbnew.FromMM(PARES[name][0]))
            c.SetDiffPairGap(pcbnew.FromMM(PARES[name][1]))
        ns.SetNetclass(name, c)
        for pat in patterns:
            ns.SetNetclassPatternAssignment(pat, name)
    ns.RecomputeEffectiveNetclasses()

    falta = []
    for fp in b.GetFootprints():
        ref = fp.GetReference()
        if ref in SITIO:
            x, y, a = SITIO[ref]
            fp.SetPosition(pt(x, y))
            fp.SetOrientationDegrees(a)
        elif not fp.IsBoardOnly():
            falta.append(ref)
    ds.SetAuxOrigin(pt(0, H))
    ds.SetGridOrigin(pt(0, 0))
    pcbnew.SaveBoard(pcbfile, b)
    print('sin sitio:', falta or '-')


if __name__ == '__main__':
    main(sys.argv[1])
