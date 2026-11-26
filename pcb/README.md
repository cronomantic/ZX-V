# La placa de ZX-V

> **Using ZX-V?** See the board guide: [English](../docs/en/hardware/board.md) · [Español](../docs/es/hardware/board.md). This page is developer documentation, in Spanish.  
> **¿Usas ZX-V?** Mira la guía de la placa: [English](../docs/en/hardware/board.md) · [Español](../docs/es/hardware/board.md). Esta página es documentación de desarrollo.

**Estado: revisión 1 (v0.1), fabricada y probada:** con las dos Pico pasa `boardtest`, usa la tarjeta microSD y, con la pila, el reloj guarda la hora. Licencia CERN-OHL-W v2 (§8). El esquema (85 piezas en
tres hojas: alimentación, Pico CPU y Pico ULA) lo dibuja `tools/schematic.py`,
con ERC sin errores y cada pieza con su número LCSC. Las huellas que no traía
KiCad están en `zxv.pretty`, comprobadas con los planos. La placa la ponen al
día `tools/netlist_to_board.py` (del netlist), `tools/place.py` (contorno,
taladros, reglas y el sitio de cada pieza) y `tools/silkscreen.py` (los
rótulos propios y las referencias, cada una en un hueco libre). El rutado, en
dos pasos (§4.6): `tools/pairs.py` traza a mano los pares diferenciales (los
cuatro del HDMI y el del USB), y `tools/route.py` pasa el resto a Freerouting y
pone la masa: un plano en cada cara y vías de cosido. DRC: paridad con el
esquema, 0; nada sin conectar, sin errores de margen ni de serigrafía. Solo
queda el aviso de que las Pico no coinciden con su huella de la biblioteca: es
la marca *DNP* que les pone el esquema. Las dos Pico 2 y todo lo de la
protoboard en una placa. Los pines, los valores y por qué son los que son
están en dev/hardware-pico.md; aquí, lo propio de
la placa. El proyecto de KiCad (10) está en `ZX-V/ZX-V/`.

---

## 1. Objetivos

- **Que la pueda montar cualquiera**, de tres maneras (decisiones D57):
  1. **Pedirla montada a JLCPCB:** todo es SMD de una cara, así que vale su
     montaje *Economic*, el barato. Llega con todo soldado salvo las dos
     Pico, que se sueldan con sus tiras de pines (THT, lo más fácil de
     soldar; §6.2).
  2. **Con soldador:** todo lo SMD se puede soldar a mano.
  3. **Con placa caliente u horno:** con el stencil, que se pide con las
     placas.
- **De 100 × 100 mm o menos**, el tamaño más barato de JLCPCB, y **de dos
  capas**, con plano de masa.
- **Todo lo de la protoboard**, y nada que el firmware no use, salvo lo que
  cuesta un par de pads (§3).

---

## 2. Reglas del diseño

- **Todo lo SMD en la cara de arriba:** una placa caliente no suelda las dos
  caras, y JLCPCB monta una sola más barato.
- **Nada pequeño:** pasivos de 0805 con las huellas `_HandSolder` (los pads
  más largos); chips en SOIC, SOT-23 o SOD-123, sin QFN ni pads escondidos
  debajo. Sitio alrededor de cada pieza para la punta del soldador. La
  excepción, el cristal del reloj (3,2 × 1,5 mm, dos pads): es la pieza
  *Basic*, y se suelda bien con flux.
- **Cada pieza SMD con su número LCSC**, en el campo `LCSC` del símbolo. Las
  *Basic* de JLCPCB siempre que se pueda: las *Extended* cuestan más al
  montar.
- **Nada THT en el montaje:** JLCPCB cobra aparte cada soldadura THT, y con
  una sola el pedido ya no vale para el *Economic*. Solo son THT las tiras
  de las Pico (no los hay SMD de 1 × 20), las tiras opcionales y los mandos
  y LED del frontal (en ángulo recto, para que salgan por la caja), y los
  suelda quien la monta: van marcados *DNP*.
- **La serigrafía**, para montarla sin el esquema: referencias y valores, la
  polaridad de diodos, LED, condensadores y pila, los nombres de los
  conectores y los de los pines de las tiras.
- **En el repositorio**, con cada versión: el esquema en PDF, el BOM y el CPL
  de JLCPCB, los Gerber y la guía de montaje (la de la placa, en
  [inglés](../docs/en/hardware/board.md) y en [español](../docs/es/hardware/board.md)).

---

## 3. Decisiones de la placa (propuestas)

| Qué | Propuesta | Por qué |
|---|---|---|
| Las Pico | **Con tiras macho de 1 × 20**, soldadas a la Pico y a los taladros de la placa, o **por sus bordes almenados**: la huella común de KiCad (`RaspberryPi_Pico_Common_Unspecified`) tiene a la vez los taladros y los pads, y una zona libre para el enchufe del micro-USB. Va copiada en `zxv:RaspberryPi_Pico2_Common`, sin la zona prohibida de la antena de la Pico W, que la Pico 2 no tiene | Con las tiras, lo de siempre: separada de la placa, quedan a mano el micro-USB, el BOOTSEL y el conector de depuración (SWD) para el cable de programar. Almenada, la mitad de soldaduras y más baja |
| Módulos | **Ninguno:** la microSD, el reloj y el HDMI, en la placa | Más barato, más bajo y sin cables |
| Alimentación | **USB-C solo para la corriente**, de 5 V | Vale cualquier cargador de 2 A; no hay que comprar una fuente aparte |
| Capas | **Dos**, con plano de masa en las dos caras | El HDMI sale de GPIO con una resistencia en serie y pistas de 2 a 3 cm: los pares van acoplados, igualados y con el plano entero debajo (§4.6), pero sin impedancia controlada (con el plano a 1,5 mm no se llega a 100 Ω), que a 252 Mbit/s no decide. El USB es de 12 Mbit/s |
| Reinicio | **Un pulsador** a los RUN de las dos Pico | Sin desenchufar; Ctrl+Alt+Supr reinicia toda la Pico CPU, pero no la ULA |
| Encendido | **Un deslizante** (SW3) que manda un MOSFET P (Q1) entre el USB-C y los 5 V | El deslizante no aguanta 1 A; Q1 sí, y con arranque suave |
| Los mandos | **Delante**, lo que toca el usuario: USB-A, pulsadores, encendido y LED, **THT en ángulo recto** que asoman por el borde (el botón de los pulsadores, 4 mm; la palanca del deslizante, 3,4; la punta de los LED, 1). La microSD, en el lateral derecho | Para la caja: salen por agujeros del frontal, sin guías de luz ni capuchones. Los SMD de pulsación lateral y los LED de 0805 se quedaban dentro. Detrás, lo que se enchufa una vez |
| Sujeción | **Cuatro taladros M3** en las esquinas (redondeadas, de 3 mm) | Para los separadores de una caja o unos pies |
| La tarjeta | Su **contacto de detección**, a GP21 de la CPU (libre) | El firmware de la CPU monta y desmonta la tarjeta al ponerla y quitarla; sin el cable (la protoboard), va como antes |
| El I2C | **Una tira de 4 pines** (3V3, GND, SDA, SCL), sin montar | Para mirar el bus del reloj o para un módulo DS3231 (tiene preferencia, §4.2). Los programas no tienen acceso al I2C: otro módulo no lo usaría nadie |

---

## 4. Bloques

### 4.1 Alimentación

- **J1, USB-C** (solo VBUS, GND y CC): una resistencia de 5,1 kΩ de cada CC
  a masa, para que el cargador dé los 5 V. 10 µF a la entrada.
- **El encendido:** el deslizante SW3 (ON con la palanca a la derecha, vista
  desde delante: su pin 3, el de la derecha, va a masa) pone a masa, por R41 (10 kΩ), la puerta
  de Q1 (AO3401A, MOSFET P), que conduce de la entrada al raíl de 5 V. R40
  (100 kΩ) la apaga, y C16 (100 nF) con R41 da un arranque suave de ~1 ms. A
  la salida, 10 µF y 100 nF.
- **El raíl de 5 V** alimenta:
  - **Las dos Pico por VSYS** (pin 39), cada una por un **Schottky** (D1 y
    D2, SOD-123): así se puede enchufar el micro-USB de una Pico a la vez
    sin que sus 5 V vuelvan al raíl.
  - **El VBUS del USB-A**, por un **fusible rearmable de 500 mA** (F1), con
    dos de 22 µF y 100 nF.
  - **El pin 18 del HDMI** (la EDID del monitor), por otro Schottky (D3):
    que la tele no alimente la placa; y por un **fusible rearmable de
    200 mA** (F2, salta a 0,5 A como pide la norma): un cable o un
    adaptador en corto no se lleva los 5 V de la placa.
- **El micro-USB de una Pico, solo con la placa encendida.** Con ella
  apagada, una Pico sola alimentaría a la otra por los pines del enlace (y
  el RESET de las dos va unido).
- **Los 3,3 V** salen de la Pico CPU (3V3 OUT, pin 36) y alimentan la
  microSD, el reloj y las *pull-up* del I2C. **Los 3V3 de las dos Pico no se
  unen.**
- **El consumo**, calculado: las dos Pico, unos 100–150 mA; la tarjeta, hasta
  unos 100 mA (más en picos); los aparatos USB, hasta los 500 mA del
  fusible. Menos de 1 A en total.

### 4.2 Pico CPU (U1)

| Bloque | Piezas | Notas |
|---|---|---|
| microSD (SPI0, GP0–GP3) | Zócalo *push-push* Hirose DM3AT (J3), 10 kΩ de *pull-up* en CMD, DAT0–DAT3, 100 nF y 10 µF | El contacto de detección, a GP21 (§3) |
| USB host (GP14 D+, GP15 D−) | USB-A hembra SMD en ángulo recto (J4), 22 Ω en serie en cada línea y un TVS de protección sobre cada una, justo antes del conector (D5, D6; SOD-323) | El par, diferencial (§4.6). Las *pull-down* son las de la Pico. Primero era un SRV05-4, pero con sus patas una línea tenía que cruzar a la otra |
| Reloj (I2C0, GP16 SDA, GP17 SCL) | PCF8563 o BM8563 (U3, SOIC-8), cristal de 32,768 kHz de 12,5 pF, BAT54C, 100 nF, portapilas CR2032 (BT1), 4,7 kΩ de *pull-up* en SDA y SCL | dev/hardware-pico.md §7. Cristal de 3,2 × 1,5 mm (§2). CLKOUT e INT, sin conectar |
| LED de la tarjeta (GP22) | LED THT de 3 mm, verde, tumbado, y 330 Ω | |
| Carga remota (UART0: GP12 TX, GP13 RX) | Tira de 3 pines con GND (J5), sin montar | Programas desde el PC con `zxv-link` y un adaptador USB-serie de 3,3 V (D101) |
| Tira I2C | 4 pines (J6), sin montar | §3 |

### 4.3 Pico ULA (U2)

| Bloque | Piezas | Notas |
|---|---|---|
| HDMI (GP12–GP19, HSTX) | Conector HDMI tipo A, SMD (J7), una resistencia en serie por señal (8) | 240 Ω, como en dev/hardware-pico.md. Junto a la Pico, pares juntos y cortos. GND en 2, 5, 8, 11 y 17; 5 V en 18 (§4.1); HPD (19) a GP22 de la ULA (1 kΩ, R42, y 100 kΩ a masa, R43: GP22 admite 5 V); CEC (13) y DDC (15, 16), sin conectar |
| Audio PWM (GP2 izquierda, GP3 derecha) | Por canal: 1 kΩ, 10 nF, 10 kΩ, 1 nF, 10 µF en serie y 100 kΩ a masa; jack estéreo de 3,5 mm PJ-320D (J8, SMD de pads grandes) | dev/hardware-pico.md §6.2. Lejos del HDMI y de la fuente de la Pico, con su masa |
| Pulsador del modo (GP0) | Pulsador THT de 6 × 6 mm en ángulo recto (SW2) a masa | La *pull-up* es la interna |
| LED del enlace (GP1) | LED THT de 3 mm, rojo, tumbado, y 330 Ω | Unos 4 mA: lo que da el GPIO con su fuerza por defecto |

### 4.4 El enlace

- **Las señales:** D0–D7 (GP4–GP11) y CLK (GP26) de la CPU a la ULA, VBLANK
  (GP27) de la ULA a la CPU, la línea del sonido (GP20 de la CPU → GP21 de la
  ULA) y el canal de vuelta (GP20 de la ULA → GP18 de la CPU).
- **33 Ω en serie junto al que envía** (12): diez en la CPU (datos, CLK y la
  línea del sonido) y dos en la ULA (VBLANK y el canal de vuelta).
- **Pistas cortas sobre el plano de masa:** en la misma placa son unos
  centímetros, muy por debajo de los 20 cm de la protoboard.

### 4.5 Los bordes

Placa de 100 × 100 mm (el tamaño más barato de JLCPCB), con las esquinas
redondeadas (3 mm) y cuatro taladros M3 en ellas, a 4 mm de los bordes.

- **Delante, lo que toca el usuario:** el USB-A en el centro; a la izquierda,
  el encendido y el pulsador del modo (ULA); a la derecha, el reinicio y los
  LED de la tarjeta y del enlace. Todo THT en ángulo recto, asomando por el
  borde para salir por el frontal de la caja: el cuerpo de los pulsadores
  llega al borde y el botón sale 4 mm; el deslizante sale 0,8 mm y su
  palanca, 3,4; la punta de los LED, 1 mm. Cada lado tiene 20,8 mm, del
  taladro de la esquina a la zona del cable de la Pico, y va casi lleno.
- **Detrás, lo que se enchufa una vez:** el USB-C de la alimentación, el HDMI
  (detrás de la ULA, que saca el vídeo por los pines de ese extremo) y el jack.
- **En el lateral derecho:** la microSD (junto a sus pines de la CPU) y la
  pila.
- **Las dos Pico**, una al lado de la otra (la ULA a la izquierda), con el
  micro-USB hacia delante y su zona libre de 20 mm para el cable de
  programar; entre las dos zonas, el USB-A. El conector SWD, en el otro
  extremo. Entre las Pico, las resistencias del enlace.

### 4.6 El rutado

Desde la placa colocada (`netlist_to_board.py`, `place.py`, `silkscreen.py`),
cada paso en su proceso (el Python de KiCad falla a veces si carga dos placas
seguidas):

```
C:/KiCad/10.0/bin/python.exe pcb/tools/pairs.py pcb/ZX-V/ZX-V/ZX-V.kicad_pcb
C:/KiCad/10.0/bin/python.exe pcb/tools/route.py dsn pcb/ZX-V/ZX-V/ZX-V.kicad_pcb
C:/KiCad/10.0/bin/python.exe pcb/tools/route.py fr  pcb/ZX-V/ZX-V/ZX-V.kicad_pcb
C:/KiCad/10.0/bin/python.exe pcb/tools/route.py ses pcb/ZX-V/ZX-V/ZX-V.kicad_pcb
kicad-cli pcb drc --schematic-parity --refill-zones --save-board pcb/ZX-V/ZX-V/ZX-V.kicad_pcb
```

- **Los pares diferenciales, por coordenadas** (`pairs.py`), porque
  Freerouting no sabe de pares. Cada tramo (de dos pads a otros dos: de la
  Pico a las resistencias, y de ellas al conector) es una línea central con
  giros de 45°, y las dos líneas del par, paralelas a ella: separación
  constante también en las esquinas. HDMI, a 0,2 mm de ancho y 0,15 de
  separación: de los pines de la ULA, que se juntan por igual, a sus 240 Ω y
  al conector, en abanico, sin cruces; D1 y D0 pasan bajo la Pico. USB, a
  0,25 y 0,15: de GP14 y GP15 por dentro de la columna de pines de la CPU a
  sus 22 Ω (una sobre otra, en el camino) y al USB-A, con cada diodo de
  protección sobre su línea, sin rabos.
- **Largo igualado:** en cada giro la línea de dentro se queda más corta (un
  giro de 90° deja 0,58 mm en el HDMI); la más corta lleva una onda de
  esquinas a 45° que la iguala, en cada tramo por separado. Diferencia: 0,000
  mm en todos.
- **Las reglas, en KiCad:** las redes de los pares acaban en P y N
  (`ULA_D2P`/`ULA_D2N`, `HDMI_D2P`/`HDMI_D2N`, `USB_DP`/`USB_DN`,
  `USB_PICO_DP`/`USB_PICO_DN`), que es como KiCad los reconoce; las clases
  HDMI y USB llevan su ancho y separación de par (`place.py`), y
  `ZX-V.kicad_dru` comprueba la separación (0,14–0,16 mm), el trozo sin
  acoplar (6 mm en el HDMI, 8 en el USB: las salidas de los pads y la onda) y
  la diferencia de largo (0,05 mm). Las salidas de los pads y las ondas van
  en zonas de reglas «Abanico», que la separación no mira.
- **El plano de debajo, entero:** bajo cada par, en B.Cu, una zona sin pistas
  ni vías (también bajo los pads: entre los dos de cada resistencia). Bajo el
  HDMI no pasa nada. El USB no puede: su camino va de la esquina de la CPU al
  centro del frente y parte la placa en dos, y las ocho líneas del enlace
  (que salen de los pines de su lado), RUN, LED_SD, SD_CD y 3V3 tienen que
  cruzarlo. Su zona tiene ventanas de 1,8 mm, a la altura de cada pin del
  enlace y unas pocas más, y lo cruzan por ahí, de través, por B.Cu. A los
  dos lados de los pares, vías de masa cada 2 mm (`route.py masa`).
- **Masa junto a los pares:** una vía junto a cada pad de masa del HDMI y del
  diodo del D+; puentes de masa del diodo del D− a C6 y entre los dos pads de
  masa de la microSD (las pistas de alrededor dejaban sin plano al de VSS).
  Todo lo de `pairs.py` queda fijo.
- **El resto, con Freerouting** (`route.py`; v2.5, en
  `%LOCALAPPDATA%\freerouting`): al DSN se le quita GND, lo ya rutado va
  protegido, las zonas sin pistas van como prohibidas (cada trozo, la suya: el
  DSN solo lleva el primero de cada zona) y el margen de todas las pistas es
  0,2 mm, el de GND, porque sus pads se quedan sin red y Freerouting no
  siempre se lo guarda. Al volver: fuera los tramos que pone entre los dos
  pads de un mismo pin de la Pico (el de taladro y el almenado, que ya se
  tocan); sus escaleras, una diagonal; sus esquinas a 90°, a 45° (donde la
  diagonal no se acerque a otra red), y lo que estrecha al llegar a un pad, al
  ancho mínimo (0,15 mm).
- **Lo añadido después, a mano** (con el Python de KiCad, antes del pedido):
  el PTC del HDMI (F2) entre el pin 18 y D3, y el HPD del pin 19 a R42/R43 y
  al pin 29 de la ULA, por B.Cu por la derecha de la ULA y por debajo de ella
  (entre los pines 14/15 y 26/27), fuera de la zona sin pistas de los pares;
  AUD_L, en B.Cu, va a x 146,53 para dejarle sitio. Freerouting no lo hace
  bien a trozos: aun con lo rutado protegido, rehace casi todo y deja redes
  sin conectar. Si algún día se vuelve a rutar desde cero, entran solos.
- **La masa** (`route.py masa`, que va con `ses`): un plano de GND en cada
  cara, con alivio térmico (un radio basta: así se sueldan a mano), una
  rejilla de vías de cosido cada 5 mm donde no estorban, las de junto a los
  pares, y una vía en cada isla que quede (un trozo de plano que solo toca
  pads) donde caiga sobre el plano de la otra cara.
- El DSN y el SES (`zxv.dsn`, `zxv.ses`) no van a git. Freerouting no da
  siempre el mismo resultado: si sale algo sin conectar, se vuelve a pasar.

---

## 5. Los pines de la Pico

Los mismos números de GPIO en las dos (dev/hardware-pico.md §2):

| Pin | CPU | ULA | Pin | CPU | ULA |
|---|---|---|---|---|---|
| 1 GP0 | SD MISO | Pulsador del modo | 40 VBUS | — | — |
| 2 GP1 | SD CS | LED del enlace | 39 VSYS | 5 V por D1 | 5 V por D2 |
| 3 GND | | | 38 GND | | |
| 4 GP2 | SD SCK | Audio izquierda | 37 3V3_EN | — | — |
| 5 GP3 | SD MOSI | Audio derecha | 36 3V3 | 3,3 V de la placa | — |
| 6 GP4 | Enlace D0 → | → D0 | 35 ADC_VREF | — | — |
| 7 GP5 | D1 → | → D1 | 34 GP28 | Libre | Libre |
| 8 GND | | | 33 AGND | | |
| 9 GP6 | D2 → | → D2 | 32 GP27 | ← VBLANK | VBLANK → |
| 10 GP7 | D3 → | → D3 | 31 GP26 | CLK → | → CLK |
| 11 GP8 | D4 → | → D4 | 30 RUN | Reinicio | Reinicio |
| 12 GP9 | D5 → | → D5 | 29 GP22 | LED de la tarjeta | HPD del HDMI |
| 13 GND | | | 28 GND | | |
| 14 GP10 | D6 → | → D6 | 27 GP21 | Detección de la tarjeta | ← Sonido |
| 15 GP11 | D7 → | → D7 | 26 GP20 | Sonido → | Canal de vuelta → |
| 16 GP12 | UART0 TX | HDMI D0+ | 25 GP19 | Libre | HDMI D1− |
| 17 GP13 | UART0 RX | HDMI D0− | 24 GP18 | ← Canal de vuelta | HDMI D1+ |
| 18 GND | | | 23 GND | | |
| 19 GP14 | USB D+ | HDMI CLK+ | 22 GP17 | Reloj SCL | HDMI D2− |
| 20 GP15 | USB D− | HDMI CLK− | 21 GP16 | Reloj SDA | HDMI D2+ |

---

## 6. Lista de material

### 6.1 Las piezas

Elegidas en el catálogo de JLCPCB del 22-10-2026 (precio de 1 a 9 unidades,
en dólares). **B:** *Basic*; **P:** *Extended* preferida; **E:** *Extended*
(§6.2). La huella es la de las bibliotecas de KiCad, salvo donde se dice.

| Ref. | Pieza | LCSC | Tipo | Precio | Huella |
|---|---|---|---|---|---|
| U1, U2 | Raspberry Pi Pico 2 | — | — | — | Las compra quien la monta |
| (U1, U2) | Tira macho de 1 × 20, 2,54 mm (4), THT | C50981 | — | 0,15 | Va en la huella de la Pico. Sin montar en JLCPCB: la suelda quien la monta (o vienen con la Pico) |
| U3 | PCF8563T (NXP), SO-8 | C7440 | P | 0,61 | `SOIC-8_3.9x4.9mm_P1.27mm` |
| Y1 | Cristal de 32,768 kHz, 12,5 pF, ±20 ppm, 70 kΩ (TKD SF32WK32768D31T002) | C271654 | E | 0,17 | `Crystal_SMD_3215-2Pin_3.2x1.5mm`. El Epson FC-135 (C32346) es *Basic*, pero JLCPCB lo da como de montaje difícil |
| D4 | BAT54C (Nexperia), SOT-23 | C37704 | P | 0,02 | `SOT-23` |
| BT1 | Portapilas CR2032, SMD (LX CR2032-BS-6) | C22363833 | E | 0,23 | `zxv:BatteryHolder_CR2032-BS-6-1`, de EasyEDA; polaridad comprobada con el plano (pad 1 +). Era el Q&J CR2032-BS-6-1 (C70377), con la misma huella, pero JLCPCB solo lo monta en *Standard* |
| D5, D6 | TVS H5VLD3B (5 V, 15 pF, bidireccional), protección del USB, SOD-323 | C20615805 | P | 0,02 | `D_SOD-323` |
| D1–D3 | Schottky B5819W (1 A, 40 V), SOD-123 | C8598 | B | 0,03 | `D_SOD-123` |
| F1 | Fusible rearmable de 500 mA, 1206 | C720075 | E | 0,03 | `Fuse_1206_3216Metric` |
| F2 | Fusible rearmable de 200 mA (salta a 0,5 A), 0805 (Littelfuse 0805L020YR) | C126816 | E | 0,27 | `Fuse_0805_2012Metric`, el pin 18 del HDMI |
| LED1, LED2 | LED THT de 3 mm, difuso, verde (Lite-On LTL-4231N) y rojo (LTL-4221N), tumbado | C125082, C125081 | E | 0,07, 0,06 | `zxv:LED_D3.0mm_Horizontal_O1.27mm_Z2.0mm`, la de KiCad sin la serigrafía de la cúpula (sale de la placa). Sin montar en JLCPCB |
| SW1, SW2 | Pulsador THT de 6 × 6 mm en ángulo recto (Omron B3F-3152) | C231320 | E | 0,27 | `zxv:SW_SPST_Omron_B3F-315x_Angled`, la de KiCad sin la serigrafía de delante. Sin montar en JLCPCB |
| SW3 | Deslizante THT en ángulo recto, el encendido (Shou Han SK12D07VG4) | C393937 | E | 0,03 | `zxv:SW_Slide_SK12D07VG4_Angled`, de EasyEDA, con las patillas de sujeción sin número. Sin montar en JLCPCB |
| Q1 | AO3401A (Alpha & Omega), MOSFET P, SOT-23 | C15127 | B | 0,10 | `SOT-23` |
| J1 | USB-C hembra, HRO TYPE-C-31-M-12 | C165948 | E | 0,19 | `USB_C_Receptacle_HRO_TYPE-C-31-M-12` |
| J3 | microSD *push-push*, Hirose DM3AT-SF-PEJM5 | C114218 | E | 1,40 | `microSD_HC_Hirose_DM3AT-SF-PEJM5` |
| J4 | USB-A hembra SMD en ángulo recto (Jing Extension 904-131A2032S10100) | C8328 | E | 0,07 | `zxv:USB_A_904-131A2032S10100`, de EasyEDA (era la de otro modelo): pivotes corregidos con el plano |
| J5 | Tira macho de 1 × 3, 2,54 mm (sin montar) | — | — | — | `PinHeader_1x03_P2.54mm_Vertical` |
| J6 | Tira macho de 1 × 4 (sin montar) | — | — | — | `PinHeader_1x04_P2.54mm_Vertical` |
| J7 | HDMI tipo A en ángulo recto, Hong Cheng HC-HDMI0019T-1 | C7501809 | E | 0,09 | `zxv:HDMI_A_HC-HDMI0019T-1`, de EasyEDA: fila de señales corregida 0,13 mm con el plano; el borde de la placa, 4,57 mm por delante de los taladros delanteros |
| J8 | Jack estéreo de 3,5 mm, SMD (Shou Han PJ-320D) | C431535 | E | 0,05 | `zxv:Jack_3.5mm_PJ-320D_TRS`, la de KiCad (`Jack_3.5mm_KoreanHropartsElec_PJ-320D-4A_Horizontal`) con dos pads renombrados para cablearlo como TRS |

**Resistencias y condensadores, 0805** (huellas `R_0805`/`C_0805`
`_HandSolder`):

| Valor | Cant. | LCSC | Tipo | Dónde |
|---|---|---|---|---|
| 33 Ω | 12 | C17634 | B | El enlace |
| 240 Ω | 8 | C17572 | P | El HDMI |
| 22 Ω | 2 | C17561 | B | USB D+ y D− |
| 330 Ω | 2 | C17630 | B | LED |
| 1 kΩ | 3 | C17513 | B | Audio (2) y el HPD del HDMI (R42) |
| 4,7 kΩ | 2 | C17673 | B | I2C |
| 5,1 kΩ | 2 | C27834 | B | CC del USB-C |
| 10 kΩ | 8 | C17414 | B | microSD (5), audio (2) y la puerta de Q1 (R41) |
| 100 kΩ | 4 | C149504 | B | Audio (2), la puerta de Q1 (R40) y el HPD (R43) |
| 100 nF, 50 V, X7R | 5 | C49678 | B | Desacoplo y arranque suave (C16) |
| 10 µF, 25 V, X5R | 5 | C15850 | B | Alimentación, microSD y audio |
| 10 nF, 50 V, X7R | 2 | C1710 | B | Audio |
| 1 nF, 50 V, X7R | 2 | C46653 | B | Audio |
| 22 µF, 25 V, X5R, **1206** | 2 | C12891 | B | VBUS del USB-A (en vez de un electrolítico de 100 µF) |

**Las piezas, por placa:** unos 5 $, con las tiras de las Pico (sin las Pico).

### 6.2 El montaje en JLCPCB

**Montaje *Economic*, todo SMD de la cara de arriba.** Según el catálogo,
JLCPCB monta por SMT todas las piezas de la lista, también los conectores con
patas de sujeción (el USB-C, la microSD, el HDMI y el USB-A). Las THT se
cobran aparte por soldadura y no entran en el *Economic*, así que no hay
ninguna.

- **Lo que suelda quien la monta:** las dos Pico con sus tiras (160
  soldaduras THT, las más fáciles: 80 en las Pico, que ya vienen hechas si
  se compran con las tiras, y 80 en la placa), los mandos y LED del frontal
  (SW1–SW3, LED1 y LED2: 17 soldaduras) y, si las quiere, las tiras J5 y J6.
  Después, la pila.
- **Lo que cuesta de más** (de memoria, sin comprobar: lo dice el
  presupuesto al pedirla): cada pieza *Extended* que no es preferida paga
  una vez **por pedido**, no por placa, la carga de su bobina (unos 3 $).
  Ningún conector ni fusible del catálogo es *Basic* ni preferido, así que
  no se pueden evitar sin soldarlos a mano en cada placa; con un pedido de
  5, salen a unos 5 $ por placa. Aquí son nueve: el
  portapilas, los dos fusibles, el USB-C, la microSD, el USB-A, el HDMI, el
  jack y el cristal.
- **Lo que JLCPCB da como de montaje difícil** («Assembly Difficulty:
  High», y al pedir: «El procesamiento de este componente es difícil»): el
  USB-A (C8328). Casi todos los USB-A SMD en ángulo recto lo son, y los que
  no tienen otra huella; se acepta el aviso. El cristal Epson (C32346) también
  lo era: va el TKD, con la misma huella.
- **Lo que se sube:** el BOM y el CPL que salen de KiCad, con el número LCSC
  de cada pieza en el campo `LCSC`. Las Pico, J5, J6 y los mandos y LED del
  frontal, marcados *DNP*.
- **El número de pedido:** el fabricante lo imprime en la serigrafía. Para
  que quede tapado, la placa lleva las marcas de dónde ponerlo, debajo de
  cada Pico: `JLCJLCJLCJLC` (bajo la CPU) para JLCPCB y `WayWayWay` (bajo la
  ULA) para PCBWay. Al pedir hay que elegir «especificar posición» (en
  JLCPCB, *Remove Order Number → Specify a location*); si no, lo ponen donde
  quieren.
- **Si se monta a mano:** las mismas piezas, de LCSC o de cualquier tienda.

### 6.3 Cómo pedirla

Todo lo que se sube está en [`fabrication/`](fabrication/), y lo genera y lo
comprueba (contra la placa, no contra lo que dice KiCad al terminar)
`tools/fabricate.py`:

```
python pcb/tools/fabricate.py
```

| Fichero | Qué es |
|---|---|
| `zxv-gerber.zip` | Los Gerber de las dos caras (cobre, máscara, serigrafía, pasta), el contorno, los taladros (metalizados y sin metalizar, en dos ficheros) y el fichero de trabajo |
| `zxv-bom.csv` | Las piezas del montaje, con su número LCSC: 76 piezas, 28 números distintos |
| `zxv-cpl.csv` | Dónde va cada una y cómo va girada |
| `zxv-schematic.pdf` | El esquema |

1. **La placa:** se sube `zxv-gerber.zip`. Dos capas, 100 × 100 mm,
   1,6 mm, FR-4; el acabado, el de siempre (HASL sin plomo). En el número
   de pedido, **especificar posición**: va debajo de una Pico (§6.2).
2. **El montaje:** *Economic*, por arriba. Se suben `zxv-bom.csv` y
   `zxv-cpl.csv`, y en la lista se mira que estén todas las piezas y con
   existencias. Las *DNP* no salen: las suelda quien la monta.
3. **La vista previa de la colocación:** JLCPCB coloca cada pieza según su
   propia huella, que no siempre está girada como la de KiCad ni tiene el
   origen en el mismo sitio. `fabricate.py` ya lo corrige (`GIRO_JLC`: los
   SOT-23, 180°; el SOIC-8, 270°; el USB-C, la microSD y el jack, su origen),
   medido comparando pad a pad nuestras huellas con las suyas. Aun así, hay
   que mirar en la vista previa la marca del pin 1 de U3, Q1 y D4, la raya
   de los diodos D1–D3 y que el USB-C, la microSD y el jack caigan sobre sus
   pads.
4. **Al llegar:** se sueldan las Pico, los mandos y los LED del frontal
   (el lado plano del LED, el cátodo, hacia el pad cuadrado), si se quieren
   las tiras J5 y J6, y se pone la pila.

## 7. Pendiente

1. **Las decisiones del §3**, confirmadas.
2. ~~Las huellas que faltan~~: hechas (HDMI, USB-A y portapilas de EasyEDA,
   corregidas con sus planos; el jack, de KiCad). Las piezas ya están elegidas
   (§6.1); con el pedido se mira que sigan en existencias.
3. ~~El esquema~~: hecho, en hojas (alimentación, Pico CPU, Pico ULA); ERC
   sin errores.
4. ~~La placa~~: colocada (§4.5) y rutada (§4.6); DRC limpio (solo el aviso
   *DNP* de las Pico). El encendido (SW3) es con la palanca a la derecha,
   vista desde delante: el deslizante une el pin del centro con el de su lado
   (lo dice su plano), y la masa va al de la derecha; la serigrafía dice OFF
   y ON a cada lado.
5. ~~La fabricación~~: Gerber, taladros, BOM, CPL y el esquema en PDF, con
   `tools/fabricate.py` (§6.3); la guía de montaje, en `docs/*/hardware/board.md`.
6. ~~Probarla~~: las placas llegaron el 16 de noviembre de 2026, montadas por
   JLCPCB; con las dos Pico pasa `boardtest` (vídeo, sonido, teclado, mandos,
   ratón, MODO, paletas) y usa la tarjeta; con la pila (desde el 22 de
   noviembre), el reloj guarda la hora.

## 8. Licencia

La placa es hardware abierto con la licencia **CERN-OHL-W v2** (débilmente
recíproca): se puede fabricar, cambiar y vender; quien reparta el diseño
cambiado lo reparte con la misma licencia, pero lo que se le añada como pieza
aparte (un módulo, una caja, el firmware) puede llevar la suya. El texto está
en [`LICENSE`](LICENSE) y el aviso, con las excepciones, en [`NOTICE`](NOTICE):

- `tools/` es MIT ([`tools/LICENSE`](tools/LICENSE)).
- Las huellas copiadas de las bibliotecas de KiCad (el jack, el LED, la Pico 2
  y el pulsador) siguen con su licencia, CC BY-SA 4.0 con su excepción para
  los diseños que las usan.
- Las de EasyEDA (portapilas, HDMI, USB-A y deslizante), corregidas aquí con
  los planos: EasyEDA no dice con qué licencia da su biblioteca, así que solo
  los cambios tienen la CERN-OHL-W. Sus modelos 3D (`zxv.3dshapes`) no se
  publican: el visor 3D enseña esas cuatro piezas sin cuerpo, y se sacan con
  `easyeda2kicad` del número LCSC de cada huella. Para fabricarla no hacen
  falta.
- La licencia no da el nombre: «ZX-V» y su logotipo no se licencian.
- El sitio del fuente, que la licencia pide dejar a la vista en lo que se
  fabrique, ya va en la serigrafía: `github.com/cronomantic/ZX-V`.

Se publica en el repositorio público con `dev/public/export.py` (decisión
D102), sin los modelos de EasyEDA.
