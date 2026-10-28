[English](../../en/hardware/board.md) · **Español**

# La placa

La placa de ZX-V lleva las dos Pico y todo lo que necesitan: HDMI, USB-A para
el teclado, microSD, jack de audio, USB-C para la corriente, un reloj con su
pila y los mandos del frontal. Mide 100 × 100 mm, es de dos capas, y está
pensada para que JLCPCB la haga **con todas las piezas pequeñas de montaje
superficial (SMD) ya soldadas**. Cuando llega, solo te quedan por soldar las
dos Pico y unas pocas piezas grandes de agujero pasante.

> **La placa todavía no se ha fabricado ni probado.** El diseño está
> terminado y pasa las comprobaciones de KiCad, pero nadie ha montado una
> todavía. Si la pides, comprueba todo lo que dice esta página y cuenta con
> alguna sorpresa. El diseño se publicará cuando se haya montado y probado una; hasta entonces, esta página cuenta lo que supondrá pedirla.

![La cara de arriba de la placa, sin las Pico. El frontal, con los mandos,
está abajo.](../../images/hardware-board-top.png)

*La placa sin las Pico, dibujada por KiCad. El frontal queda abajo; algunos
conectores solo se ven por sus pads. Los rótulos de la serigrafía están en
español (ver [Conectores y mandos](#conectores-y-mandos)).*

## Cómo pedirla a JLCPCB

Estos son los ficheros que habrá que subir, cuando se publiquen;
para pedirla no hará falta KiCad.

| Fichero | Qué es | Dónde va |
|---|---|---|
| `zxv-gerber.zip` | Los Gerber: cobre, máscara, serigrafía y pasta de las dos caras, el contorno y los taladros | La placa («Add gerber file») |
| `zxv-bom.csv` | La lista de piezas del montaje: 76 piezas, 28 números LCSC distintos | El montaje, «BOM file» |
| `zxv-cpl.csv` | Dónde va cada pieza y cómo va girada | El montaje, «CPL file» |
| `zxv-schematic.pdf` | El esquema, para consultarlo | No se sube |

Los nombres de los botones y opciones de abajo son los que usa la web de
JLCPCB (en inglés) mientras se escribe esto; pueden cambiar un poco.

### 1. La placa

1. En jlcpcb.com, empieza un pedido de PCB y sube `zxv-gerber.zip`. La web lee
   el tamaño de los ficheros: tiene que decir **2 capas, 100 × 100 mm**.
2. Elige estas opciones (lo demás, como venga):

   | Opción | Qué elegir |
   |---|---|
   | Base material | FR-4 |
   | Layers | 2 |
   | PCB thickness | 1,6 mm |
   | Surface finish | HASL sin plomo (*lead-free*) |
   | PCB color | El que quieras |
   | **Remove order number** | **Specify a location** |

3. **El número de pedido.** JLCPCB imprime su número de pedido en la
   serigrafía. La placa tiene marcado el sitio **debajo de la Pico CPU**: el
   texto `JLCJLCJLCJLC`. Con *Specify a location* lo imprimen ahí, donde la
   Pico lo tapará; si dejas lo que viene, lo ponen donde quieren, a lo mejor
   en pleno frontal. (Debajo de la Pico ULA hay otra marca, `WayWayWay`, para
   pedirla a PCBWay.)

### 2. El montaje

1. Activa **PCB Assembly** en el mismo pedido.
2. Elige el montaje **Economic**, por la cara de arriba (**Top side**). Todo
   lo que se monta es SMD y está arriba, así que vale el barato. Elige cuántas
   placas quieres montadas.
3. Sube `zxv-bom.csv` como BOM y `zxv-cpl.csv` como CPL (el fichero de
   colocación).
4. En la lista de piezas, mira que **las 28 líneas tengan su pieza** y que
   todas estén **en existencias**. Las Pico, los pulsadores, el interruptor,
   los LED y las tiras de pines no salen: los sueldas tú, así que no van en
   los ficheros.

Si una pieza está agotada, busca en el catálogo de JLCPCB una igual, del mismo
valor y encapsulado. Por ejemplo, si se agotaran las ocho resistencias de
240 Ω del HDMI (C17572), valdría cualquier otra de 240 Ω con el mismo
encapsulado 0805.

### 3. La vista previa de la colocación

JLCPCB coloca cada pieza según su propia huella, que no siempre está girada
como la de KiCad ni tiene el centro en el mismo sitio. El CPL ya corrige las
diferencias conocidas, pero **mira la vista previa de la colocación** antes de
pagar. Amplía y comprueba que:

- el punto del pin 1 de **U3** (el chip del reloj, de 8 patas), de **Q1** y de
  **D4** (de 3 patas cada uno) cae sobre la marca del pin 1 de la placa;
- la raya de los diodos **D1, D2 y D3** queda del lado de la raya dibujada en
  la placa;
- el **USB-C** (J1), el zócalo de la **microSD** (J3) y el **jack** (J8)
  caen bien sobre sus pads, y también el HDMI (J7), el USB-A (J4) y el
  portapilas (BT1).

Si una pieza sale girada, normalmente se puede girar en la propia vista
previa, o pedírselo a JLCPCB en las notas del pedido.

### Lo que cuesta

Las piezas salen por unos 5 dólares por placa (sin las Pico). Además, JLCPCB
cobra una vez **por pedido** (no por placa) cada pieza *Extended* que no sea
*preferida*: aquí son ocho (el portapilas, los dos fusibles, el USB-C, la
microSD, el USB-A, el HDMI y el jack), a unos 3 dólares cada una. En un pedido
de 5 placas, unos 5 dólares por placa. Son cifras aproximadas: las de verdad salen
en el presupuesto de la web.

### Sin el montaje de JLCPCB

Todas las piezas SMD se pueden soldar a mano: las resistencias y los
condensadores son de 0805 (un tamaño cómodo) con pads largos, los chips tienen
las patas a la vista, y solo el cristal del reloj es pequeño (3,2 × 1,5 mm, dos
pads; con flux se suelda bien). También puedes pedir el stencil con las placas
y usar una placa caliente o un horno: todo lo SMD está en la cara de arriba.
Los mismos números LCSC de `zxv-bom.csv` valen en LCSC o en cualquier tienda.

## Las piezas que sueldas tú

| Ref. | Pieza | Cant. | LCSC | Notas |
|---|---|---|---|---|
| U1, U2 | Raspberry Pi Pico 2 | 2 | — | U1 es la Pico CPU; U2, la Pico ULA |
| (U1, U2) | Tira de pines macho de 1 × 20, 2,54 mm | 4 | C50981 | No hacen falta si sueldas las Pico por sus bordes, o si tus Pico ya las traen |
| SW1, SW2 | Pulsador de 6 × 6 mm en ángulo recto (Omron B3F-3152) | 2 | C231320 | RESET y MODO |
| SW3 | Interruptor deslizante en ángulo recto (Shou Han SK12D07VG4) | 1 | C393937 | El encendido (`OFF` / `ON`) |
| LED1 | LED de 3 mm, verde difuso (Lite-On LTL-4231N) | 1 | C125082 | SD |
| LED2 | LED de 3 mm, rojo difuso (Lite-On LTL-4221N) | 1 | C125081 | ENLACE |
| J5 | Tira de pines macho de 1 × 3, 2,54 mm (opcional) | 1 | — | UART |
| J6 | Tira de pines macho de 1 × 4, 2,54 mm (opcional) | 1 | — | I2C |
| — | Pila de botón CR2032 | 1 | — | Va en BT1, que viene montado |

## Cómo se sueldan

Primero lo bajo del frontal, luego las tiras opcionales y, al final, las Pico.

### Los mandos y LED del frontal

Los cinco van en el borde de delante y asoman por él, para salir por los
agujeros del frontal de una caja. Son piezas en ángulo recto que van tumbadas
sobre la placa.

- **Los pulsadores (SW1, SW2)**: mételos hasta el fondo, con el cuerpo hasta
  el borde; el botón asoma unos 4 mm. Las patas solo entran de una manera.
- **El interruptor (SW3)**: mételo hasta el fondo y suelda sus tres patas y
  sus dos pestañas de sujeción. La palanca asoma unos 3,4 mm. Visto desde
  delante, con la palanca a la **derecha** está encendido y a la **izquierda**,
  apagado: la placa pone `OFF` encima de la mitad izquierda y `ON` encima de
  la derecha.
- **Los LED (LED1 verde, LED2 rojo)**: van tumbados, apuntando hacia fuera
  del borde. Dobla las dos patas en ángulo recto para que el LED quede
  paralelo a la placa, a unos 2 mm de ella, con la punta saliendo 1 mm del
  borde. **Ojo con la polaridad:** el cátodo (la pata más corta, del lado
  plano del reborde del LED) va al **pad cuadrado**. No los cambies: el verde
  es LED1 (`SD`) y el rojo, LED2 (`ENLACE`).

### Las tiras opcionales

**J5** (3 pines, `UART`) y **J6** (4 pines, `I2C`) están detrás a la derecha.
Solo hacen falta para depurar (J5) o para conectar un módulo I2C, como un
reloj DS3231 (J6). Tiras macho rectas de 2,54 mm.

### Las Pico

La huella admite las Pico de dos maneras:

- **Con tiras de pines** (lo más fácil de soldar): suelda dos tiras de 1 × 20
  a la Pico, o cómpralas con las tiras ya puestas, y luego suelda las tiras en
  los agujeros de la placa. La Pico queda un poco separada de la placa.
- **Por sus bordes almenados**: apoya la Pico sobre la placa, cada medio
  agujero sobre su pad, y suelda los dos lados. La mitad de soldaduras y más
  baja; más difícil de quitar.

**La orientación:** los componentes de la Pico hacia arriba y su **conector
micro-USB hacia el frontal** (hacia el USB-A y los pulsadores). La **Pico ULA
va a la izquierda** y la **Pico CPU a la derecha**, vistas desde delante; la
placa pone `ULA` y `CPU` delante de cada una.

Las dos Pico son iguales hasta que llevan su firmware. Lo más cómodo es
**cargar el firmware en cada una antes de soldarla** y marcarla
([El firmware](firmware.md)). También se puede hacer después: el micro-USB y
el botón BOOTSEL quedan a mano.

### La pila

Mete una CR2032 en el portapilas (`CR2032`, en el lateral derecho) con el
**lado `+` (el plano, el de las letras) hacia arriba**, como indica el `+` de
la placa. Sin ella la máquina funciona, pero al apagarla olvida la hora, el
idioma y la distribución del teclado.

## Lista de comprobación

- [ ] La placa montada no tiene puentes de estaño a la vista, sobre todo en
      el HDMI y el USB-C.
- [ ] SW1 (`RESET`) y SW2 (`MODO`), hasta el fondo y soldados.
- [ ] SW3 (`OFF` / `ON`) soldado, con sus pestañas.
- [ ] LED1 verde en `SD`, LED2 rojo en `ENLACE`, con el cátodo en el pad
      cuadrado.
- [ ] Si las quieres: J5 y J6.
- [ ] Cada Pico con su firmware: `zxv_cpu.uf2` en la Pico CPU y
      `zxv_ula_link.uf2` en la Pico ULA.
- [ ] La Pico ULA a la izquierda, la Pico CPU a la derecha, con el micro-USB
      hacia delante.
- [ ] Los 80 pines de las Pico soldados (40 cada una).
- [ ] La CR2032 puesta, con el `+` hacia arriba.

## El primer encendido

Las primeras comprobaciones, **antes de soldar las Pico**, con un polímetro.

1. **Que no haya un cortocircuito.** Sin nada enchufado, mide la resistencia
   entre los dos contactos de los extremos del puerto USB-A (5 V y masa). No
   tiene que estar cerca de 0 Ω.
2. **El interruptor.** Enchufa el cargador USB-C y mide la tensión entre esos
   mismos dos contactos del USB-A: unos 5 V con la palanca a la derecha (`ON`)
   y 0 V a la izquierda (`OFF`). También puedes medir, en la huella de cada
   Pico, entre el pin 39 (VSYS) y el 38 (GND), el segundo y el tercero desde el
   extremo del micro-USB por el lado del pin 40: algo menos de 5 V encendida
   (un diodo de protección se come unas décimas de voltio). Desenchufa el
   cargador.
3. **Suelda las Pico**, mete una tarjeta microSD y conecta la pantalla HDMI y
   un teclado USB.
4. **Enciende.** El LED rojo `ENLACE` parpadea una vez (el modo 0) y luego se
   queda encendido mientras la Pico ULA recibe imágenes de la Pico CPU. En la
   pantalla sale el BASIC.
5. **Pruébala:** `PRINT 6*7`, luego `CAT` (el LED verde `SD` se enciende
   mientras lee la tarjeta) y `BEEP 1,0` para el sonido.
6. **La prueba de la placa:** `LOAD "ROM:boardtest"` comprueba, uno a uno, lo que
   ve un programa: el monitor (por su HPD), el reloj, la tarjeta, el teclado,
   los mandos, el ratón y el sonido (teclas 1, 2 y 3: beeper, AY por la
   izquierda y por la derecha). Pulsa MODO para ver cambiar la línea del vídeo,
   y Esc para volver.

Las Pico se pueden conectar al ordenador por su micro-USB con la placa
encendida: los diodos de protección separan las dos alimentaciones. **Con la
placa apagada, no**: una Pico sola alimentaría a la otra por los pines del
enlace. Si algo no funciona, mira [Qué mirar](firmware.md#si-no-hay-imagen-o-no-hay-sonido).

## Conectores y mandos

| Rótulo | Dónde | Pieza | Qué es |
|---|---|---|---|
| `5 V` | Detrás, a la izquierda | J1, USB-C | La corriente: cualquier cargador USB-C de 5 V y 2 A. Solo corriente, sin datos |
| `HDMI` | Detrás, tras la ULA | J7 | La pantalla: la imagen, y el sonido en los modos HDMI |
| `AUDIO` | Detrás, tras la CPU | J8, jack de 3,5 mm | El sonido estéreo, a nivel de línea, siempre |
| `OFF` `ON` | Delante, en la esquina izquierda | SW3 | El encendido: con la palanca a la derecha, encendida; a la izquierda, apagada |
| `MODO` | Delante, a la izquierda | SW2 | Una pulsación corta cambia el modo de vídeo; una larga (de 1 a 5 s) enseña o quita la página de diagnóstico; apretado al encender, el modo seguro |
| `USB` | Delante, en el centro | J4, USB-A | Teclado, ratón y mandos (varios, con un hub). Con un fusible de 500 mA |
| `RESET` | Delante, a la derecha | SW1 | Reinicia las dos Pico, como apagar y encender |
| `SD` | Delante, a la derecha | LED1, verde | Encendido mientras la máquina usa la tarjeta o el reloj |
| `ENLACE` | Delante, en la esquina derecha | LED2, rojo | Al arrancar parpadea el número del modo más uno; luego, encendido mientras llegan imágenes de la Pico CPU, parpadeando si algunas llegan con errores y apagado si no llega ninguna |
| `microSD` | Lateral derecho | J3 | La tarjeta SD (se empuja para meterla y otra vez para sacarla) |
| `CR2032` `+` | Lateral derecho | BT1 | La pila del reloj, con el `+` hacia arriba |
| `UART` `GND` `TX` `RX` | Detrás, a la derecha | J5 (sin montar) | El puerto serie de depuración de la Pico CPU, a 3,3 V y 921600 baudios. `TX` es la salida de la Pico |
| `I2C` `3V3` `GND` `SDA` `SCL` | Detrás, a la derecha | J6 (sin montar) | El bus I2C del reloj, a 3,3 V, para otro módulo (si hay un módulo de reloj DS3231, se usa en lugar del de la placa) |
| `ULA` *vídeo y sonido* | En el centro, a la izquierda | U2 | La Pico ULA |
| `CPU` *BASIC y programas* | En el centro, a la derecha | U1 | La Pico CPU |

Para arrancar en el modo de vídeo seguro con esta placa, mantén apretado
`MODO` mientras enciendes, o mientras pulsas y sueltas `RESET`.
