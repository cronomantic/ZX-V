"""schematic.py - Dibuja el esquema de la placa de ZX-V (pcb/README.md §4).

    python pcb/tools/schematic.py pcb/ZX-V/ZX-V

Reescribe las cuatro hojas: la raíz (las tres hojas y los hilos del enlace
entre la CPU y la ULA), la alimentación, la Pico CPU y la Pico ULA. Cada
bloque va dibujado con sus hilos; la alimentación, con sus símbolos; las
señales entre bloques de una hoja, con etiquetas; y entre hojas, con
etiquetas jerárquicas. Conserva el identificador de cada símbolo (el de su
huella en la placa), así que la placa no pierde lo colocado.

Después: kicad-cli sch erc, y netlist_to_board.py para llevarlo a la placa.
"""
import os
import sys

from schematic_lib import Libs, Sheet, nid, old_instances, fmt

ROOT = 'db6611a1-da7c-415c-844a-3a9904c96aab'
HOJAS = {   # clave: (fichero, uuid de la hoja en la raíz, uuid del fichero, papel, título, página)
    'alim': ('power.kicad_sch', 'e5f5ae2d-e531-49d4-ac86-b89d74dd2468', 'b3f122ec-f6d2-4690-83b6-5ae01fdbda9f',
             'A4', 'Alimentación', '2', 'Alimentación'),
    'cpu': ('cpu.kicad_sch', '3fa6b6da-3548-4933-a148-84c68e58568c', '09d48379-5cbc-482f-ae51-b1df020be3a3',
            'A3', 'Pico CPU', '3', 'Pico CPU'),
    'ula': ('ula.kicad_sch', 'b1f49e46-0e16-4267-8a09-6426c288bb8b', '02ddb57b-28fb-45bb-8754-8b47d87f726a',
            'A3', 'Pico ULA', '4', 'Pico ULA'),
}

R0805 = 'Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder'
C0805 = 'Capacitor_SMD:C_0805_2012Metric_Pad1.18x1.45mm_HandSolder'
# Los mandos y los LED del frontal, THT en ángulo recto: asoman por el frontal de la caja (los suelda
# quien la monta, como las Pico: DNP en el montaje de JLCPCB)
PULSADOR = 'zxv:SW_SPST_Omron_B3F-315x_Angled'          # las de KiCad, sin la serigrafía que cae fuera
LED3 = 'zxv:LED_D3.0mm_Horizontal_O1.27mm_Z2.0mm'

# Las señales entre la CPU y la ULA: (nombre, sentido visto desde la CPU)
ENLACE = [(f'LINK_D{i}', 'output') for i in range(8)] + [
    ('LINK_CLK', 'output'), ('SND_LINK', 'output'), ('LINK_VBLANK', 'input'), ('LINK_BACK', 'input'),
    ('RUN', 'bidirectional')]


def pico_pins(s, U, left, right, nc):
    """Las conexiones de una Pico: {pin: (nombre, tipo)} a izquierda y derecha."""
    for pin, (name, kind) in left.items():
        if kind == 'h':
            s.stub(name, U.p(pin), 'L', kind='hierarchical_label', shape='input')
        else:
            s.stub(name, U.p(pin), 'L')
    for pin, (name, kind) in right.items():
        if kind == 'h':
            s.stub(name, U.p(pin), 'R', kind='hierarchical_label', shape='input')
        elif kind == 'ho':
            s.stub(name, U.p(pin), 'R', kind='hierarchical_label', shape='output')
        else:
            s.stub(name, U.p(pin), 'R')
    for pin in nc:
        s.nc(U.p(pin))


def pico_power(s, U, D, flag_ref_y, with_3v3):
    """VSYS por su Schottky desde +5V, 3V3 (si se usa), GND y AGND."""
    x, y = U.p('39')                                     # VSYS
    s.wire((x, y), (x, y - 12.7))
    a = D.p('2')
    s.wire(a, (a[0] - 5.08, a[1]))
    s.pwr('+5V', (a[0] - 5.08, a[1]))
    s.pwr('PWR_FLAG', (x, flag_ref_y))
    if with_3v3:
        p = U.p('36')
        s.wire(p, (p[0], p[1] - 5.08))
        s.pwr('+3V3', (p[0], p[1] - 5.08))
    else:
        s.nc(U.p('36'))
    s.nc(U.p('40'))                                      # VBUS: el micro-USB de la Pico, solo para programar
    g = U.p('3')
    s.wire(g, (g[0], g[1] + 2.54))
    s.pwr('GND', (g[0], g[1] + 2.54))
    a = U.p('33')                                        # AGND
    s.wire(a, (a[0] + 5.08, a[1]), (a[0] + 5.08, a[1] + 2.54))
    s.pwr('GND', (a[0] + 5.08, a[1] + 2.54))


def resistor_row(s, ref, x, y, left_name, right_name, value, right_kind='label', right_shape='output',
                 hide_value=True, lcsc=None, left_len=7.62, right_len=7.62):
    """Etiqueta — resistencia horizontal — etiqueta, en una fila."""
    R = s.sym(ref, 'Device:R', x, y, 90, value=value, footprint=R0805, lcsc=lcsc, hide_value=hide_value,
              fields={'Reference': (x, y - 2.54, None)})
    p1, p2 = R.p('1'), R.p('2')
    s.wire(p1, (p1[0] - left_len, p1[1]))
    s.label(left_name, (p1[0] - left_len, p1[1]), 'L')
    s.wire(p2, (p2[0] + right_len, p2[1]))
    s.label(right_name, (p2[0] + right_len, p2[1]), 'R', kind=right_kind, shape=right_shape)
    return R


def cap_down(s, ref, x, y_top, value, fp=C0805):
    """Un condensador colgado de un hilo en (x, y_top), a masa."""
    C = s.sym(ref, 'Device:C', x, y_top + 3.81, 0, value=value, footprint=fp)
    b = C.p('2')
    s.pwr('GND', (b[0], b[1] + 2.54))
    s.wire(b, (b[0], b[1] + 2.54))
    return C


# ======================================================================= alimentación

def hoja_alim(s):
    s.text('Alimentación: 5 V por el USB-C (solo la corriente) y el interruptor de encendido.', 25.4, 25.4, 2)
    s.text('R1 y R2 (5,1 kΩ en CC1 y CC2) piden 5 V al cargador.\n'
           'Encendido: con SW3 en ON, la puerta de Q1 va a masa por R41 y Q1 conduce. C16 con R41 da un\n'
           'arranque suave (~1 ms); R40 la apaga. El interruptor no lleva la corriente: la lleva Q1.', 25.4, 160.02)
    J1 = s.sym('J1', 'Connector:USB_C_Receptacle_USB2.0_16P', 60.96, 93.98)
    Q1 = s.sym('Q1', 'Transistor_FET:AO3401A', 139.7, 76.2, 270, value='AO3401A',
               footprint='Package_TO_SOT_SMD:SOT-23', lcsc='C15127',
               fields={'Reference': (139.7, 83.82, None), 'Value': (139.7, 86.36, None)})
    vb, src, drn = J1.p('A4'), Q1.p('2'), Q1.p('3')
    s.wire(vb, src)
    s.label('VBUS_IN', (vb[0] + 7.62, vb[1]), 'R')
    s.pwr('PWR_FLAG', (101.6, vb[1]))
    s.sym('C1', 'Device:C', 114.3, vb[1] + 3.81, 0, value='10uF', footprint=C0805)
    s.wire((114.3, vb[1] + 7.62), (114.3, vb[1] + 10.16))
    s.pwr('GND', (114.3, vb[1] + 10.16))
    # La puerta: R40 y C16 a la fuente, R41 al interruptor
    gy = 63.5
    for ref, x, val, kind in (('C16', 111.76, '100nF', 'C'), ('R40', 124.46, '100k', 'R')):
        P = s.sym(ref, 'Device:' + kind, x, vb[1] - 7.62, 0, value=val,
                  footprint=C0805 if kind == 'C' else R0805,
                  lcsc='C49678' if kind == 'C' else 'C149504')
        s.wire(P.p('2'), (x, vb[1]))
        s.wire(P.p('1'), (x, gy))
    gate = Q1.p('1')
    s.wire((111.76, gy), (gate[0], gy), gate)
    R41 = s.sym('R41', 'Device:R', 149.86, gy, 90, value='10k', footprint=R0805, lcsc='C17414')
    s.wire((gate[0], gy), R41.p('1'))
    SW3 = s.sym('SW3', 'Switch:SW_SPDT', 163.83, gy, 0, value='Encendido',
                footprint='zxv:SW_Slide_SK12D07VG4_Angled', lcsc='C393937', dnp=True)
    s.wire(R41.p('2'), SW3.p('2'))
    # ON con la palanca a la derecha (vista desde delante): el deslizante une el pin del centro con
    # el de su lado (plano del SK12D07VG4), y el 3 es el de la derecha en la placa
    a = SW3.p('3')
    s.wire(a, (a[0] + 7.62, a[1]), (a[0] + 7.62, a[1] + 7.62))
    s.pwr('GND', (a[0] + 7.62, a[1] + 7.62))
    s.nc(SW3.p('1'))
    s.text('ON', a[0] + 1.27, a[1] - 2.54)
    # Salida: +5V
    s.wire(drn, (193.04, drn[1]))
    s.pwr('+5V', (182.88, drn[1]))
    s.pwr('PWR_FLAG', (193.04, drn[1]))
    s.sym('C2', 'Device:C', 157.48, drn[1] + 3.81, 0, value='100nF', footprint=C0805)
    s.sym('C15', 'Device:C', 167.64, drn[1] + 3.81, 0, value='10uF', footprint=C0805, lcsc='C15850')
    for x in (157.48, 167.64):
        s.wire((x, drn[1] + 7.62), (x, drn[1] + 10.16))
        s.pwr('GND', (x, drn[1] + 10.16))
    # CC
    cc1, cc2 = J1.p('A5'), J1.p('B5')
    R1 = s.sym('R1', 'Device:R', 99.06, 96.52, 0, value='5.1k', footprint=R0805)
    R2 = s.sym('R2', 'Device:R', 91.44, 96.52, 0, value='5.1k', footprint=R0805)
    s.wire(cc1, (99.06, cc1[1]), R1.p('1'))
    s.wire(cc2, (91.44, cc2[1]), R2.p('1'))
    for R in (R1, R2):
        b = R.p('2')
        s.wire(b, (b[0], b[1] + 2.54))
        s.pwr('GND', (b[0], b[1] + 2.54))
    b = R1.p('2')                                        # la marca de GND, en un sitio libre
    s.wire((b[0], b[1] + 1.27), (b[0] + 7.62, b[1] + 1.27))
    s.pwr('PWR_FLAG', (b[0] + 7.62, b[1] + 1.27))
    # Masa del conector y lo que no se usa
    gp, sh = J1.p('A1'), J1.p('SH')
    s.wire(gp, (gp[0], gp[1] + 5.08))
    s.wire(sh, (sh[0], sh[1] + 2.54), (gp[0], sh[1] + 2.54))
    s.pwr('GND', (gp[0], gp[1] + 5.08))
    for p in ('A6', 'A7', 'B6', 'B7', 'A8', 'B8'):
        s.nc(J1.p(p))


# ======================================================================= Pico CPU

def hoja_cpu(s):
    s.text('Pico CPU: la CPU de ZX-V, con la tarjeta, el USB host, el reloj y el emisor del enlace.', 25.4, 20.32, 2)
    U = s.sym('U1', 'zxv:RaspberryPi_Pico', 203.2, 139.7, footprint='zxv:RaspberryPi_Pico2_Common')
    D1 = s.sym('D1', 'Device:D_Schottky', 194.31, 88.9, 180,
               fields={'Reference': (194.31, 82.55, None), 'Value': (194.31, 85.09, None)})
    left = {'1': ('SD_MISO', 'l'), '2': ('SD_CS', 'l'), '4': ('SD_SCK', 'l'), '5': ('SD_MOSI', 'l'),
            '16': ('UART_TX', 'l'), '17': ('UART_RX', 'l'), '19': ('USB_PICO_DP', 'l'), '20': ('USB_PICO_DN', 'l')}
    for i, pin in enumerate(('6', '7', '9', '10', '11', '12', '14', '15')):
        left[pin] = (f'CPU_D{i}', 'l')
    right = {'21': ('I2C_SDA', 'l'), '22': ('I2C_SCL', 'l'), '24': ('LINK_BACK', 'h'), '26': ('CPU_SND', 'l'),
             '27': ('SD_CD', 'l'), '29': ('LED_SD', 'l'), '31': ('CPU_CLK', 'l'), '32': ('LINK_VBLANK', 'h')}
    pico_pins(s, U, left, right, nc=('37', '25', '34', '35'))
    pico_power(s, U, D1, 95.25, with_3v3=True)
    # Reinicio de las dos Pico
    run = U.p('30')
    s.wire(run, (149.86, run[1]))
    s.label('RUN', (149.86, run[1]), 'L', kind='hierarchical_label', shape='bidirectional')
    sy = run[1] + 5.08
    SW1 = s.sym('SW1', 'Switch:SW_Push', 157.48, sy, 270, value='Reinicio', footprint=PULSADOR, lcsc='C231320', dnp=True,
                fields={'Reference': (153.67, sy - 1.27, 'right'), 'Value': (153.67, sy + 1.27, 'right')})
    b = SW1.p('2')
    s.wire(b, (b[0], b[1] + 2.54))
    s.pwr('GND', (b[0], b[1] + 2.54))
    s.text('SW1: reinicia\nlas dos Pico', 132.08, 129.54)

    # ---- microSD (SPI0)
    s.text('Tarjeta microSD (SPI0), con pull-up de 10 kΩ', 30.48, 40.64, 1.5)
    J3 = s.sym('J3', 'Connector:Micro_SD_Card_Det_Hirose_DM3AT', 135.89, 69.85)
    # (pin, etiqueta o None, resistencia o None): de arriba abajo; cada línea acaba más a la izquierda
    lines = [('1', None, 'R7'), ('2', 'SD_CS', 'R4'), ('3', 'SD_MOSI', 'R5'), ('5', 'SD_SCK', None),
             ('7', 'SD_MISO', 'R3'), ('8', None, 'R6'), ('10', 'SD_CD', None)]
    x_end = J3.p('1')[0] - 7.62
    for pin, name, rref in lines:
        p = J3.p(pin)
        s.wire(p, (x_end if name else x_end + 2.54, p[1]))   # sin etiqueta, el hilo acaba en su resistencia
        if rref:
            rx = x_end + 2.54
            ry = p[1] - 3.81
            R = s.sym(rref, 'Device:R', rx, ry, 0, value='10k', footprint=R0805,
                      fields={'Reference': (rx - 1.905, ry - 1.27, 'right'), 'Value': (rx - 1.905, ry + 1.27, 'right')})
            s.pwr('+3V3', R.p('1'))
        if name:
            s.label(name, (x_end, p[1]), 'L')
        x_end -= 12.7
    for pin, kind, rot in (('4', '+3V3', 90), ('6', 'GND', 270)):
        p = J3.p(pin)
        s.wire(p, (p[0] - 5.08, p[1]))
        s.pwr(kind, (p[0] - 5.08, p[1]), rot)
    p = J3.p('9')                                        # DET_B a masa: DET_A cierra con la tarjeta dentro
    s.wire(p, (p[0] - 5.08, p[1]))
    s.pwr('GND', (p[0] - 5.08, p[1]), 270)
    p = J3.p('SH')
    s.wire(p, (p[0], p[1] + 5.08))
    s.pwr('GND', (p[0], p[1] + 5.08))
    for i, (ref, val) in enumerate((('C3', '100nF'), ('C4', '10uF'))):
        x = 30.48 + i * 10.16
        C = s.sym(ref, 'Device:C', x, 55.88, 0, value=val, footprint=C0805)
        s.pwr('+3V3', C.p('1'))
        b = C.p('2')
        s.wire(b, (b[0], b[1] + 2.54))
        s.pwr('GND', (b[0], b[1] + 2.54))

    # ---- USB host
    s.text('USB host (GP14 y GP15 por PIO): teclado, ratón y mandos', 30.48, 190.5, 1.5)
    J4 = s.sym('J4', 'Connector:USB_A', 63.5, 228.6)
    dp, dm, vb = J4.p('3'), J4.p('2'), J4.p('1')
    R18 = s.sym('R18', 'Device:R', 91.44, dp[1], 90, value='22', footprint=R0805)
    R19 = s.sym('R19', 'Device:R', 91.44, dm[1] + 5.08, 90, value='22', footprint=R0805)
    s.wire(dp, R18.p('1'))
    s.wire(dm, (dm[0] + 5.08, dm[1]), (dm[0] + 5.08, dm[1] + 5.08), R19.p('1'))
    s.label('USB_DP', (dp[0] + 2.54, dp[1]), 'R')
    s.label('USB_DN', (dm[0] + 7.62, dm[1] + 5.08), 'R')
    for R, name in ((R18, 'USB_PICO_DP'), (R19, 'USB_PICO_DN')):
        p = R.p('2')
        s.wire(p, (p[0] + 5.08, p[1]))
        s.label(name, (p[0] + 5.08, p[1]), 'R')
    vy = 203.2
    s.wire(vb, (vb[0], vy), (104.14, vy))
    s.label('USB_VBUS', (vb[0] + 2.54, vy), 'R')
    s.pwr('PWR_FLAG', (99.06, vy))
    F1 = s.sym('F1', 'Device:Polyfuse', 107.95, vy, 270, value='500mA')
    s.wire(F1.p('1'), (116.84, vy))
    s.pwr('+5V', (116.84, vy))
    for ref, x, val, fp in (('C5', 78.74, '22uF', 'Capacitor_SMD:C_1206_3216Metric_Pad1.33x1.80mm_HandSolder'),
                            ('C6', 86.36, '22uF', 'Capacitor_SMD:C_1206_3216Metric_Pad1.33x1.80mm_HandSolder'),
                            ('C7', 93.98, '100nF', C0805)):
        cap_down(s, ref, x, vy, val, fp)
    g = J4.p('4')
    sh = J4.p('SH')
    s.wire(g, (g[0], g[1] + 2.54))
    s.wire(sh, (sh[0], g[1] + 2.54), (g[0], g[1] + 2.54))
    s.pwr('GND', (g[0], g[1] + 2.54))
    # La protección: un diodo por línea (en la placa, la línea pasa por su pad: sin rabos)
    for ref, x, name in (('D5', 124.46, 'USB_DN'), ('D6', 134.62, 'USB_DP')):
        D = s.sym(ref, 'Device:D_TVS', x, 228.6, 90, value='H5VLD3B', footprint='Diode_SMD:D_SOD-323',
                  lcsc='C20615805')
        s.stub(name, D.p('2'), 'U')
        p = D.p('1')
        s.wire(p, (p[0], p[1] + 2.54))
        s.pwr('GND', (p[0], p[1] + 2.54))
    s.text('D5, D6: protección\ncontra descargas', 119.38, 243.84)

    # ---- Enlace con la ULA
    s.text('Enlace con la ULA: 33 Ω junto al emisor', 279.4, 93.98, 1.5)
    rows = [(f'CPU_D{i}', f'LINK_D{i}') for i in range(8)] + [('CPU_CLK', 'LINK_CLK'), ('CPU_SND', 'SND_LINK')]
    for i, (a, b) in enumerate(rows):
        resistor_row(s, f'R{8 + i}', 304.8, 101.6 + 5.08 * i, a, b, '33', right_kind='hierarchical_label',
                     right_shape='output')
    s.text('R8–R17: 33 Ω', 297.18, 154.94)

    # ---- Reloj
    s.text('Reloj PCF8563 (I2C0), con su pila', 289.56, 33.02, 1.5)
    U3 = s.sym('U3', 'Timer_RTC:PCF8563T', 330.2, 76.2)
    Y1 = s.sym('Y1', 'Device:Crystal', 309.88, 76.2, 270, value='32.768kHz')
    oi, oo = U3.p('1'), U3.p('2')
    s.wire(oi, (Y1.p('1')[0], oi[1]), Y1.p('1'))
    s.wire(oo, (Y1.p('2')[0], oo[1]), Y1.p('2'))
    p = U3.p('4')
    s.wire(p, (p[0], p[1] + 2.54))
    s.pwr('GND', (p[0], p[1] + 2.54))
    s.nc(U3.p('3'))
    s.nc(U3.p('7'))
    vdd = U3.p('8')
    D4 = s.sym('D4', 'Diode:BAT54C', 330.2, 45.72)
    s.wire(vdd, D4.p('3'))
    a1, a2 = D4.p('1'), D4.p('2')
    s.wire(a1, (a1[0] - 5.08, a1[1]))
    s.pwr('+3V3', (a1[0] - 5.08, a1[1]))
    BT1 = s.sym('BT1', 'Device:Battery_Cell', 358.14, 55.88)
    s.wire(a2, (358.14, a2[1]), BT1.p('1'))
    s.label('VBAT', (a2[0] + 2.54, a2[1]), 'R')
    p = BT1.p('2')
    s.wire(p, (p[0], p[1] + 2.54))
    s.pwr('GND', (p[0], p[1] + 2.54))
    s.wire((vdd[0], 58.42), (304.8, 58.42))
    s.pwr('PWR_FLAG', (317.5, 58.42))
    C8 = s.sym('C8', 'Device:C', 304.8, 62.23, 0, value='100nF', footprint=C0805)
    p = C8.p('2')
    s.wire(p, (p[0], p[1] + 2.54))
    s.pwr('GND', (p[0], p[1] + 2.54))
    s.label('RTC_VDD', (vdd[0], 62.23), 'R')
    sda, scl = U3.p('5'), U3.p('6')
    s.wire(scl, (351.79, scl[1]))
    s.label('I2C_SCL', (351.79, scl[1]), 'R')
    s.wire(sda, (370.84, sda[1]))
    s.label('I2C_SDA', (370.84, sda[1]), 'R')
    for ref, x, line in (('R21', 347.98, scl), ('R20', 365.76, sda)):
        y = line[1] - 3.81
        R = s.sym(ref, 'Device:R', x, y, 0, value='4.7k', footprint=R0805,
                  fields={'Reference': (x - 2.54, y - 1.27, 'right'), 'Value': (x - 2.54, y + 1.27, 'right')})
        s.pwr('+3V3', R.p('1'))

    # ---- Depuración e I2C (tiras sin montar)
    s.text('Tiras sin montar: UART0 (depuración) e I2C', 340.36, 96.52, 1.5)
    J5 = s.sym('J5', 'Connector_Generic:Conn_01x03', 378.46, 106.68, value='UART0')
    J6 = s.sym('J6', 'Connector_Generic:Conn_01x04', 378.46, 124.46, value='I2C')
    p = J5.p('1')
    s.wire(p, (p[0] - 5.08, p[1]))
    s.pwr('GND', (p[0] - 5.08, p[1]), 270)
    s.stub('UART_TX', J5.p('2'), 'L', 12.7)
    s.stub('UART_RX', J5.p('3'), 'L', 12.7)
    p = J6.p('1')
    s.wire(p, (p[0] - 5.08, p[1]))
    s.pwr('+3V3', (p[0] - 5.08, p[1]), 90)
    p = J6.p('2')
    s.wire(p, (p[0] - 5.08, p[1]))
    s.pwr('GND', (p[0] - 5.08, p[1]), 270)
    s.stub('I2C_SDA', J6.p('3'), 'L', 12.7)
    s.stub('I2C_SCL', J6.p('4'), 'L', 12.7)

    # ---- LED de la tarjeta
    s.text('LED de actividad de la tarjeta', 279.4, 195.58, 1.5)
    resistor_led(s, 'R22', 'LED1', 'LED_SD', 284.48, 203.2, 'Verde (tarjeta)', 'C125082')


def resistor_led(s, rref, lref, name, x, y, lval, lcsc):
    R = s.sym(rref, 'Device:R', x + 7.62, y, 90, value='330', footprint=R0805)
    s.wire(R.p('1'), (x, y))
    s.label(name, (x, y), 'L')
    L = s.sym(lref, 'Device:LED', x + 20.32, y, 180, value=lval, footprint=LED3, lcsc=lcsc, dnp=True)
    s.wire(R.p('2'), L.p('2'))
    k = L.p('1')
    s.wire(k, (k[0] + 5.08, k[1]), (k[0] + 5.08, k[1] + 2.54))
    s.pwr('GND', (k[0] + 5.08, k[1] + 2.54))


# ======================================================================= Pico ULA

def hoja_ula(s):
    s.text('Pico ULA: el vídeo (HDMI por HSTX), el sonido (PWM) y el receptor del enlace.', 25.4, 20.32, 2)
    U = s.sym('U2', 'zxv:RaspberryPi_Pico', 203.2, 139.7, footprint='zxv:RaspberryPi_Pico2_Common')
    D2 = s.sym('D2', 'Device:D_Schottky', 194.31, 88.9, 180,
               fields={'Reference': (194.31, 82.55, None), 'Value': (194.31, 85.09, None)})
    left = {'1': ('MODE_BTN', 'l'), '2': ('LED_LINK', 'l'), '4': ('AUD_L_PWM', 'l'), '5': ('AUD_R_PWM', 'l'),
            '16': ('ULA_D0P', 'l'), '17': ('ULA_D0N', 'l'), '19': ('ULA_CKP', 'l'), '20': ('ULA_CKN', 'l')}
    for i, pin in enumerate(('6', '7', '9', '10', '11', '12', '14', '15')):
        left[pin] = (f'LINK_D{i}', 'h')
    right = {'21': ('ULA_D2P', 'l'), '22': ('ULA_D2N', 'l'), '24': ('ULA_D1P', 'l'), '25': ('ULA_D1N', 'l'),
             '26': ('ULA_BACK', 'l'), '27': ('SND_LINK', 'h'), '29': ('HDMI_HPD', 'l'), '31': ('LINK_CLK', 'h'),
             '32': ('ULA_VBLANK', 'l')}
    pico_pins(s, U, left, right, nc=('37', '34', '35'))
    pico_power(s, U, D2, 95.25, with_3v3=False)
    s.stub('RUN', U.p('30'), 'L', kind='hierarchical_label', shape='bidirectional')

    # ---- HDMI
    s.text('HDMI (HSTX, GP12–GP19): una resistencia en serie por señal', 279.4, 96.52, 1.5)
    J7 = s.sym('J7', 'Connector:HDMI_A', 340.36, 139.7)
    rows = [('R27', '1', 'D2P'), ('R28', '3', 'D2N'), ('R29', '4', 'D1P'), ('R30', '6', 'D1N'),
            ('R23', '7', 'D0P'), ('R24', '9', 'D0N'), ('R25', '10', 'CKP'), ('R26', '12', 'CKN')]
    jog = [327.66, 325.12, 322.58, 320.04, 320.04, 322.58, 325.12, 327.66]
    for i, (ref, pin, sig) in enumerate(rows):
        py = J7.p(pin)[1]
        ry = 128.27 + (i - 3.5) * 5.08
        R = s.sym(ref, 'Device:R', 304.8, ry, 90, value='240', footprint=R0805, hide_value=True,
                  fields={'Reference': (304.8, ry - 2.54, None)})
        p1, p2 = R.p('1'), R.p('2')
        s.wire(p1, (p1[0] - 7.62, ry))
        s.label('ULA_' + sig, (p1[0] - 7.62, ry), 'L')
        s.wire(p2, (jog[i], ry), (jog[i], py), J7.p(pin))
        s.label('HDMI_' + sig, (p2[0] + 1.27, ry), 'R')      # con P y N al final: KiCad lo ve como par
    s.text('R23–R30: 240 Ω', 297.18, 152.4)
    gy = J7.p('2')[1] + 2.54
    for pin in ('2', '5', '8', '11', '17', 'SH'):
        p = J7.p(pin)
        s.wire(p, (p[0], gy))
    s.wire((J7.p('2')[0], gy), (J7.p('SH')[0], gy))
    s.wire((340.36, gy), (340.36, gy + 2.54))
    s.pwr('GND', (340.36, gy + 2.54))
    v = J7.p('18')
    D3 = s.sym('D3', 'Device:D_Schottky', v[0] + 3.81, v[1] - 12.7, 0,
               fields={'Reference': (v[0] + 3.81, v[1] - 19.05, None), 'Value': (v[0] + 3.81, v[1] - 16.51, None)})
    # Un PTC entre el diodo y el pin 18: un cable o un adaptador en corto no
    # se lleva los 5 V de la placa (la norma pide limitarlo a 0,5 A).
    F2 = s.sym('F2', 'Device:Polyfuse', v[0], v[1] - 6.35, 0, value='200mA',
               footprint='Fuse:Fuse_0805_2012Metric_Pad1.15x1.40mm_HandSolder', lcsc='C126816')
    s.wire(v, F2.p('2'))
    s.wire(F2.p('1'), D3.p('1'))
    s.pwr('PWR_FLAG', (v[0], v[1] - 1.27))
    a = D3.p('2')
    s.wire(a, (a[0] + 5.08, a[1]))
    s.pwr('+5V', (a[0] + 5.08, a[1]))
    for pin in ('13', '14', '15', '16'):
        s.nc(J7.p(pin))
    s.text('CEC y DDC sin conectar;\nlos 5 V, para la EDID del monitor', 350.52, 165.1)
    # El HPD (el monitor lo sube a 5 V cuando está conectado) a GP22 de la ULA,
    # que admite 5 V: 1 kΩ en serie, y 100 kΩ a masa para que sin monitor lea 0.
    hp = J7.p('19')
    R42 = s.sym('R42', 'Device:R', hp[0] - 6.35, hp[1], 90, value='1k', footprint=R0805, lcsc='C17513',
                hide_value=True, fields={'Reference': (hp[0] - 6.35, hp[1] - 2.54, None)})
    s.wire(R42.p('2'), hp)
    k = R42.p('1')
    s.wire(k, (k[0] - 3.81, k[1]), (k[0] - 7.62, k[1]))
    s.label('HDMI_HPD', (k[0] - 7.62, k[1]), 'L')
    R43 = s.sym('R43', 'Device:R', k[0] - 3.81, k[1] + 6.35, 0, value='100k', footprint=R0805, lcsc='C149504',
                fields={'Reference': (k[0] - 1.27, k[1] + 5.08, 'left'), 'Value': (k[0] - 1.27, k[1] + 7.62, 'left')})
    s.wire((k[0] - 3.81, k[1]), R43.p('1'))
    s.pwr('GND', R43.p('2'))
    s.text('R42, R43: el HPD a GP22', 299.72, 172.72)

    # ---- VBLANK y canal de vuelta
    s.text('Al enlace: 33 Ω junto al emisor', 279.4, 191.77, 1.5)
    resistor_row(s, 'R31', 304.8, 200.66, 'ULA_VBLANK', 'LINK_VBLANK', '33', right_kind='hierarchical_label',
                 right_shape='output', hide_value=False)
    resistor_row(s, 'R32', 304.8, 210.82, 'ULA_BACK', 'LINK_BACK', '33', right_kind='hierarchical_label',
                 right_shape='output', hide_value=False)

    # ---- Audio
    s.text('Audio PWM (GP2 y GP3): filtro RC de dos etapas por canal', 30.48, 35.56, 1.5)
    J8 = s.sym('J8', 'Connector_Audio:AudioJack3', 129.54, 53.34, 180, value='PJ-320D')
    for y, side, refs, pin in ((50.8, 'L', ('R33', 'C9', 'R34', 'C10', 'C11', 'R35'), 'T'),
                               (76.2, 'R', ('R36', 'C12', 'R37', 'C13', 'C14', 'R38'), 'R')):
        r1, c1, r2, c2, cs, rl = refs
        s.label(f'AUD_{side}_PWM', (40.64, y), 'L')
        A = s.sym(r1, 'Device:R', 50.8, y, 90, value='1k', footprint=R0805)
        s.wire((40.64, y), A.p('1'))
        B = s.sym(r2, 'Device:R', 71.12, y, 90, value='10k', footprint=R0805)
        s.wire(A.p('2'), B.p('1'))
        cap_down(s, c1, 60.96, y, '10nF')
        Cs = s.sym(cs, 'Device:C', 91.44, y, 90, value='10uF', footprint=C0805)
        s.wire(B.p('2'), Cs.p('1'))
        cap_down(s, c2, 81.28, y, '1nF')
        R = s.sym(rl, 'Device:R', 101.6, y + 3.81, 0, value='100k', footprint=R0805)
        b = R.p('2')
        s.wire(b, (b[0], b[1] + 2.54))
        s.pwr('GND', (b[0], b[1] + 2.54))
        jp = J8.p(pin)
        if pin == 'T':
            s.wire(Cs.p('2'), jp)
        else:
            s.wire(Cs.p('2'), (114.3, y), (114.3, jp[1]), jp)
        s.label(f'AUD_{side}', (Cs.p('2')[0] + 2.54, y), 'R')
    p = J8.p('S')
    s.wire(p, (p[0] - 3.81, p[1]), (p[0] - 3.81, p[1] + 5.08))
    s.pwr('GND', (p[0] - 3.81, p[1] + 5.08))
    s.text('Punta: izquierda; anillo: derecha', 121.92, 71.12)

    # ---- Pulsador del modo y LED del enlace
    s.text('Modo de vídeo y LED del enlace', 30.48, 99.06, 1.5)
    SW2 = s.sym('SW2', 'Switch:SW_Push', 63.5, 109.22, 0, value='Modo de vídeo', footprint=PULSADOR, lcsc='C231320',
                dnp=True)
    s.wire(SW2.p('1'), (50.8, 109.22))
    s.label('MODE_BTN', (50.8, 109.22), 'L')
    p = SW2.p('2')
    s.wire(p, (p[0] + 5.08, p[1]), (p[0] + 5.08, p[1] + 2.54))
    s.pwr('GND', (p[0] + 5.08, p[1] + 2.54))
    resistor_led(s, 'R39', 'LED2', 'LED_LINK', 50.8, 124.46, 'Rojo (enlace)', 'C125081')


# ======================================================================= raíz

def raiz(proj):
    y0 = 50.8
    out = [f'(kicad_sch\n\t(version 20260306)\n\t(generator "eeschema")\n\t(generator_version "10.0")\n'
           f'\t(uuid "{ROOT}")\n\t(paper "A4")\n\t(title_block\n\t\t(title "ZX-V: la placa de las dos Pico 2")\n'
           f'\t\t(company "ZX-V")\n\t\t(rev "0.1")\n\t)\n\t(lib_symbols)']

    def sheet(key, x, y, w, h, pins, side):
        f, su, fu, paper, title, page, name = HOJAS[key]
        o = [f'\t(sheet\n\t\t(at {fmt(x)} {fmt(y)})\n\t\t(size {fmt(w)} {fmt(h)})\n\t\t(exclude_from_sim no)\n'
             f'\t\t(in_bom yes)\n\t\t(on_board yes)\n\t\t(dnp no)\n\t\t(fields_autoplaced yes)\n'
             f'\t\t(stroke\n\t\t\t(width 0.1524)\n\t\t\t(type solid)\n\t\t)\n\t\t(fill\n\t\t\t(color 0 0 0 0.0000)\n\t\t)\n'
             f'\t\t(uuid "{su}")\n'
             f'\t\t(property "Sheetname" "{name}"\n\t\t\t(at {fmt(x)} {fmt(y - 0.7116)} 0)\n'
             f'\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n\t\t\t\t(justify left bottom)\n\t\t\t)\n\t\t)\n'
             f'\t\t(property "Sheetfile" "{f}"\n\t\t\t(at {fmt(x)} {fmt(y + h + 0.5846)} 0)\n'
             f'\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n\t\t\t\t(justify left top)\n\t\t\t)\n\t\t)']
        for i, (pn, kind) in enumerate(pins):
            py = y + 7.62 + 2.54 * i
            if side == 'R':
                px, ang, j = x + w, 0, 'right'
            else:
                px, ang, j = x, 180, 'left'
                kind = {'output': 'input', 'input': 'output'}.get(kind, kind)
            o.append(f'\t\t(pin "{pn}" {kind}\n\t\t\t(at {fmt(px)} {fmt(py)} {ang})\n\t\t\t(uuid "{nid()}")\n'
                     f'\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n\t\t\t\t(justify {j})\n\t\t\t)\n\t\t)')
        o.append(f'\t\t(instances\n\t\t\t(project "ZX-V"\n\t\t\t\t(path "/{ROOT}"\n\t\t\t\t\t(page "{page}")\n'
                 f'\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)')
        return '\n'.join(o)

    h = 7.62 + 2.54 * len(ENLACE) + 5.08
    out.append(sheet('alim', 25.4, y0, 50.8, 25.4, [], 'R'))
    out.append(sheet('cpu', 101.6, y0, 50.8, h, ENLACE, 'R'))
    out.append(sheet('ula', 203.2, y0, 50.8, h, ENLACE, 'L'))
    for i in range(len(ENLACE)):
        y = y0 + 7.62 + 2.54 * i
        out.append(f'\t(wire\n\t\t(pts\n\t\t\t(xy 152.4 {fmt(y)}) (xy 203.2 {fmt(y)})\n\t\t)\n'
                   f'\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n\t\t(uuid "{nid()}")\n\t)')
    txt = ('El enlace entre las dos Pico (dev/hardware-pico.md §4): los datos (8 bits) y CLK de la CPU a la ULA,\n'
           'VBLANK y el canal de vuelta de la ULA a la CPU, la línea del sonido y el reinicio de las dos (SW1).')
    txt = txt.replace('\n', '\\n')
    out.append(f'\t(text "{txt}"\n\t\t(exclude_from_sim no)\n\t\t(at 25.4 {fmt(y0 + h + 12.7)} 0)\n'
               f'\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify left top)\n\t\t)\n'
               f'\t\t(uuid "{nid()}")\n\t)')
    out.append('\t(sheet_instances\n\t\t(path "/"\n\t\t\t(page "1")\n\t\t)\n\t)\n\t(embedded_fonts no)\n)\n')
    open(os.path.join(proj, 'ZX-V.kicad_sch'), 'w', encoding='utf-8', newline='\n').write('\n'.join(out))


def main(proj):
    files = [os.path.join(proj, h[0]) for h in HOJAS.values()]
    libs = Libs(proj, files)
    old = old_instances(files)
    for key, fn in (('alim', hoja_alim), ('cpu', hoja_cpu), ('ula', hoja_ula)):
        f, su, fu, paper, title, page, name = HOJAS[key]
        s = Sheet(libs, old, f'/{ROOT}/{su}', fu, paper, title)
        fn(s)
        text = s.write()
        open(os.path.join(proj, f), 'w', encoding='utf-8', newline='\n').write(text)
        print(f, len(s.syms), 'piezas,', len(s.wires), 'hilos,', len(s.labels), 'etiquetas')
    raiz(proj)


if __name__ == '__main__':
    main(sys.argv[1])
