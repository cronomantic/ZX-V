[English](../../en/hardware/protoboard.md) · **Español**

# En una protoboard

Antes de que hubiera placa, ZX-V se montó en una protoboard con Pico 2 y unos
cuantos módulos, y así se ha probado su firmware, una Pico cada vez. **Las dos
Pico todavía no se han probado juntas**, así que la máquina completa que se
describe aquí está sin probar. Esta página explica cómo montarla de esa
manera.

> **Lo recomendado es la placa** ([La placa](board.md)): lo lleva todo, las
> señales rápidas (el HDMI y el enlace entre las Pico) van cortas y bien
> trazadas, y solo se sueldan unas pocas piezas. La protoboard está bien para
> experimentar, o si ya tienes las piezas, pero hay mucho cable y hay que
> hacerlo con cuidado.

Se puede montar por partes y probar cada una ([Paso a paso](#paso-a-paso)): la
Pico ULA sola ya da imagen.

## Piezas

| Pieza | Cant. | Notas |
|---|---|---|
| Raspberry Pi Pico 2 | 2 | Con las tiras de pines, para la protoboard |
| Placa con un conector HDMI hembra (*breakout*) | 1 | La salida de vídeo |
| Resistencias de 240 Ω | 8 | Una en serie con cada señal del HDMI |
| Resistencias de 33 Ω | 12 | En serie con las señales del enlace (con cables muy cortos, se puede probar sin ellas) |
| Resistencias de 1 kΩ, 10 kΩ y 100 kΩ; condensadores de 10 nF, 1 nF y 10 µF | 2 de cada | El filtro de la salida de audio |
| Jack estéreo de 3,5 mm hembra | 1 | La salida de audio |
| Módulo de microSD (SPI, 3,3 V) | 1 | |
| Conector USB-A hembra (o un *breakout*) y un hub USB | 1 + 1 | Para el teclado, el ratón y los mandos. Mejor un hub con alimentación |
| Pulsador | 1 | El del modo de vídeo |
| LED y resistencias de 330 Ω | 2 + 2 | Actividad de la tarjeta (CPU) y estado del enlace (ULA) |
| Reloj: un módulo DS3231, o un PCF8563 (o BM8563) con un cristal de 32,768 kHz, un BAT54C y una pila CR2032 con su portapilas | 1 | Opcional: la fecha y la hora, el idioma y la distribución del teclado |
| Diodos Schottky (1N5817 o parecidos) | 2 | Solo con una fuente de 5 V para todo (ver [La alimentación](#la-alimentación)) |
| Fusible rearmable de 500 mA | 1 | Para los 5 V del USB-A, con una fuente de 5 V para todo |
| Fuente de 5 V y 2 A | 1 | Solo con una fuente de 5 V para todo |
| Pulsador | 1 | Opcional: el reinicio |

## Los pines de la Pico

Las dos Pico usan **los mismos números de pin para el enlace** (GP4 con GP4,
GP26 con GP26...), así es más fácil de cablear. Los pines físicos se cuentan
alrededor de la Pico como siempre: el pin 1 (GP0) y el 40 (VBUS) están en el
extremo del micro-USB. Estos son todos los que usa ZX-V:

| Pin | GPIO | Pico CPU | Pico ULA |
|---|---|---|---|
| 1 | GP0 | Tarjeta: MISO | Pulsador del modo |
| 2 | GP1 | Tarjeta: CS | LED del enlace |
| 4 | GP2 | Tarjeta: SCK | Audio izquierda |
| 5 | GP3 | Tarjeta: MOSI | Audio derecha |
| 6, 7, 9, 10, 11, 12, 14, 15 | GP4–GP11 | Enlace D0–D7 (salida) | Enlace D0–D7 (entrada) |
| 16 | GP12 | Puerto serie de depuración: TX | HDMI D0+ |
| 17 | GP13 | Puerto serie de depuración: RX | HDMI D0− |
| 19 | GP14 | USB D+ | HDMI CLK+ |
| 20 | GP15 | USB D− | HDMI CLK− |
| 21 | GP16 | Reloj: SDA | HDMI D2+ |
| 22 | GP17 | Reloj: SCL | HDMI D2− |
| 24 | GP18 | Canal de vuelta (entrada) | HDMI D1+ |
| 25 | GP19 | — | HDMI D1− |
| 26 | GP20 | Línea del sonido (salida) | Canal de vuelta (salida) |
| 27 | GP21 | — | Línea del sonido (entrada) |
| 29 | GP22 | LED de la tarjeta | — |
| 30 | RUN | Reinicio (opcional) | Reinicio (opcional) |
| 31 | GP26 | Enlace: CLK (salida) | Enlace: CLK (entrada) |
| 32 | GP27 | Enlace: VBLANK (entrada) | Enlace: VBLANK (salida) |
| 36 | 3V3 | 3,3 V para la tarjeta y el reloj | — |
| 39 | VSYS | Entrada de 5 V, por un diodo (con una fuente para todo) | Entrada de 5 V, por un diodo (con una fuente para todo) |
| 40 | VBUS | 5 V para el USB-A (ver [La alimentación](#la-alimentación)) | — |
| 3, 8, 13, 18, 23, 28, 33, 38 | GND | Masa | Masa |

Fíjate en que de GP12 a GP19 son cosas distintas en cada Pico: en la ULA, la
salida HDMI; en la CPU, el puerto de depuración, el USB y el reloj. No los
unas.

## La alimentación

Todas las masas, unidas. Hay dos maneras de darle corriente:

**Lo más sencillo, para probar: cada Pico por su micro-USB**, del ordenador o
de dos cargadores USB. Une las masas de las dos Pico. El USB-A toma sus 5 V del
VBUS de la Pico CPU (pin 40), que da para un teclado y un ratón; con un hub con
alimentación, mejor.

**Con una sola fuente de 5 V (como la placa):**

- La fuente va al **VSYS (pin 39) de cada Pico por un diodo Schottky** (el
  ánodo a la fuente; el cátodo, el lado de la raya, a VSYS). Así se puede
  enchufar a la vez el micro-USB de una Pico (para actualizarla, o para el
  puerto serie) sin que sus 5 V vuelvan a la fuente.
- La fuente también da los **5 V del USB-A (VBUS) por el fusible rearmable de
  500 mA**.

Si en cambio metes la fuente por el **VBUS (pin 40) de la Pico CPU**, sin
diodos, **tienes que desconectarla antes de enchufar el micro-USB** de esa
Pico (para cargarle el firmware, o para el puerto serie), o sus 5 V llegarían
a tu ordenador.

## El enlace entre las dos Pico

Cables cortos (**como mucho, unos 20 cm**), con **al menos tres cables de
masa** repartidos entre los de datos. Cada señal lleva una resistencia de
33 Ω en serie, **junto a la Pico que la manda**. Con cables muy cortos se
puede probar sin ellas; si el enlace da errores (ver
[No hay imagen](firmware.md#no-hay-imagen)), ponlas.

| Pico CPU | Señal | Pico ULA | 33 Ω junto a |
|---|---|---|---|
| GP4 (pin 6) | D0 → | GP4 (pin 6) | La CPU |
| GP5 (pin 7) | D1 → | GP5 (pin 7) | La CPU |
| GP6 (pin 9) | D2 → | GP6 (pin 9) | La CPU |
| GP7 (pin 10) | D3 → | GP7 (pin 10) | La CPU |
| GP8 (pin 11) | D4 → | GP8 (pin 11) | La CPU |
| GP9 (pin 12) | D5 → | GP9 (pin 12) | La CPU |
| GP10 (pin 14) | D6 → | GP10 (pin 14) | La CPU |
| GP11 (pin 15) | D7 → | GP11 (pin 15) | La CPU |
| GP26 (pin 31) | CLK → | GP26 (pin 31) | La CPU |
| GP20 (pin 26) | Línea del sonido → | GP21 (pin 27) | La CPU |
| GP27 (pin 32) | ← VBLANK | GP27 (pin 32) | La ULA |
| GP18 (pin 24) | ← Canal de vuelta | GP20 (pin 26) | La ULA |
| GND (pines 8, 13, 18...) | Masa, 3 cables o más | GND | — |

- Sin la **línea del sonido** hay imagen pero no sonido.
- Sin el **canal de vuelta** todo funciona, pero a los programas siempre se
  les dice que la salida es HDMI a 60 Hz, y el informe de la CPU lo avisa.

## El HDMI

En la Pico ULA. Una resistencia de 240 Ω en serie con cada
una de las ocho señales, **lo más cerca posible de la Pico**. Si tu *breakout*
tiene los pines rotulados (D0+, CK−...), basta con seguir la columna Señal.

| Pico ULA | Señal | Pin del conector HDMI |
|---|---|---|
| GP12 (pin 16) | D0+ | 7 |
| GP13 (pin 17) | D0− | 9 |
| GP14 (pin 19) | CLK+ | 10 |
| GP15 (pin 20) | CLK− | 12 |
| GP16 (pin 21) | D2+ | 1 |
| GP17 (pin 22) | D2− | 3 |
| GP18 (pin 24) | D1+ | 4 |
| GP19 (pin 25) | D1− | 6 |
| GND (pin 18 o 23) | Masa | 2, 5, 8, 11 y 17 |
| 5 V (opcional) | Alimentación del chip de identificación de la pantalla | 18 |

- **Cables cortos** (menos de 10 cm) y cada pareja (+ y −) junta, o
  trenzada. Por estas señales pasan cientos de millones de bits por segundo.
- **El pin 18 (5 V):** muchas teles lo necesitan para darse cuenta de que hay
  algo conectado. Sácalo del VBUS de la ULA (pin 40) si la ULA va por su
  micro-USB, o de tu fuente de 5 V. La placa pone un diodo Schottky en esta
  línea para que la tele no le dé corriente a la placa.
- Un cable HDMI corto y sin adaptadores.

## El audio

En la Pico ULA: GP2 (pin 4) es el canal izquierdo y GP3 (pin 5), el derecho.
Cada canal pasa por un filtro de dos etapas y un condensador hasta el jack:

```
GP2 ──[1 kΩ]──┬──[10 kΩ]──┬──|+ 10 µF|──┬── jack: punta (izquierda)
            10 nF        1 nF         100 kΩ
              │           │            │
             GND         GND          GND
```

GP3, igual, a la **anilla** del jack (la derecha). La **camisa** del jack, a
GND (pin 3 u 8). El condensador de 10 µF tiene polaridad: su `+` va hacia la
Pico. La salida es de nivel de línea, para unos altavoces con amplificador o
la entrada de audio de una tele.

Para una prueba rápida basta con una resistencia de 1 kΩ y el condensador de
10 µF, aunque se oirá algo de silbido.

## El pulsador del modo y el LED del enlace

En la Pico ULA.

| Pico ULA | Se conecta a |
|---|---|
| GP0 (pin 1) | Un pulsador, con el otro lado a GND (pin 3) |
| GP1 (pin 2) | Una resistencia de 330 Ω y un LED (el ánodo); su cátodo, a GND |

El pulsador no necesita resistencia: la Pico la lleva dentro. Lo que hacen el
pulsador y el LED se explica en [El firmware](firmware.md#no-hay-imagen).

## La tarjeta SD

En la Pico CPU. Un módulo de microSD de 3,3 V (o uno de 5 V con regulador y
adaptador de niveles, alimentado a 3,3 V):

| Módulo de microSD | Pico CPU |
|---|---|
| MISO (DO) | GP0 (pin 1) |
| CS | GP1 (pin 2) |
| SCK (CLK) | GP2 (pin 4) |
| MOSI (DI) | GP3 (pin 5) |
| VCC | 3V3 (pin 36) |
| GND | GND (pin 3) |

**El LED de actividad:** de GP22 (pin 29), una resistencia de 330 Ω y un LED a
masa. Se enciende mientras la máquina usa la tarjeta o el reloj.

## El teclado, el ratón y los mandos (USB-A)

En la Pico CPU. Un conector USB-A hembra (un *breakout*, o la punta de un
alargador USB cortado):

| USB-A | Pico CPU |
|---|---|
| D+ (cable verde) | GP14 (pin 19) |
| D− (cable blanco) | GP15 (pin 20) |
| VBUS (cable rojo) | VBUS (pin 40), o la fuente de 5 V por el fusible de 500 mA |
| GND (cable negro) | GND (pin 18 o 23) |

D+ y D− van directos a la Pico, o por una resistencia de 22 Ω cada uno (la
placa las lleva); no hacen falta más resistencias. Enchufa un teclado, o un hub
con el teclado, un ratón y uno o dos mandos.

## El reloj (opcional)

En la Pico CPU. Sin reloj la máquina funciona, pero al apagarla olvida la
fecha y la hora, el idioma y la distribución del teclado.

Lo más fácil es un **módulo DS3231** de 3,3 V, con su pila CR2032 (también vale
uno de 5 V con regulador si sus resistencias de *pull-up* van a 3,3 V):

| Reloj | Pico CPU |
|---|---|
| SDA | GP16 (pin 21) |
| SCL | GP17 (pin 22) |
| VCC | 3V3 (pin 36) |
| GND | GND (pin 23) |

Un chip **PCF8563** (o BM8563), como el de la placa, se conecta igual, pero
necesita más piezas alrededor: un cristal de 32,768 kHz de 12,5 pF de carga
junto al chip, resistencias de *pull-up* de 4,7 kΩ de SDA y SCL a 3,3 V, y un
BAT54C (dos diodos) que alimenta el chip desde los 3,3 V y desde la pila
CR2032, con un condensador de 100 nF. El firmware busca primero un DS3231 y
luego un PCF8563, y usa el que encuentre.

## El reinicio y el puerto de depuración (opcionales)

- **El reinicio:** un pulsador del pin **RUN** (pin 30) de cada Pico a masa la
  reinicia. La placa une los RUN de las dos a un solo pulsador.
- **El puerto serie de depuración:** en la Pico CPU, GP12 (pin 16) es TX y
  GP13 (pin 17) es RX, a 921600 baudios y 3,3 V. Crúzalos con un adaptador
  USB-serie de 3,3 V (TX con RX). De todas formas, los mismos mensajes salen
  por el micro-USB.

## Paso a paso

1. **La Pico ULA sola.** Cablea el HDMI y el audio, y cárgale la prueba
   `zxv_ula.uf2` ([El firmware](firmware.md#programas-de-prueba)). Tienen que
   verse unas demos y oírse una música, por la pantalla y por el jack. Pon el
   pulsador del modo y el LED, y prueba los modos.
2. **Las dos Pico.** Cablea el enlace, carga `zxv_cpu.uf2` en la Pico CPU y
   `zxv_ula_link.uf2` en la Pico ULA. En la pantalla debería salir el BASIC.
   Abre el puerto serie de la Pico CPU con un terminal: lo que escribas ahí le
   llega a la máquina, y cada 5 segundos escribe un informe. (Este es el paso
   que todavía no se ha probado: si falla, empieza por el informe del puerto
   serie.)
3. **La tarjeta SD, el USB-A y el reloj**, de uno en uno, mirando cada uno en
   el informe (`tarjeta`, `USB`, `reloj`) y desde el BASIC: `CAT` para la
   tarjeta, escribir algo para el teclado, y `CLOCK 2026,10,20,18,30` y luego
   `PRINT CLOCK 3;":";CLOCK 4` para el reloj.

Si algo no va, mira
[Si no hay imagen o no hay sonido](firmware.md#si-no-hay-imagen-o-no-hay-sonido).
