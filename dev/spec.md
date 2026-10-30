# Especificación de la máquina ZX-V

**Versión 0.1 (borrador).** Todo lo que aparece aquí puede cambiar hasta la v1.0. Las
direcciones, desplazamientos y tamaños marcados como *provisional* se fijarán cuando
existan el emulador y las pruebas de hardware.
Licencia CC BY 4.0, como el resto de la documentación ([docs/LICENSE](../docs/LICENSE)).

Este documento describe la máquina **tal como la ve el software**. La forma de
construirla con dos Pico 2 está en hardware-pico.md, y el BASIC
en [basic.md](basic.md).

---

## 1. Principios

1. **Una sola máquina, varias implementaciones.** La referencia hardware es la
   pareja de Pico 2 (CPU + ULA). El emulador implementa exactamente esta
   especificación. Un programa que la respete funciona igual en ambos.
2. **Registros sin efectos laterales.** Los registros de ZX-V son posiciones de
   memoria normales: leer no cambia nada, escribir solo guarda el valor, y el
   sistema los aplica en el siguiente VBLANK. Las operaciones que sí necesitan
   efecto por escritura (como el AY) usan **colas** en memoria.
3. **Sincronización por frame, no por ciclos.** La máquina funciona a 60 Hz lógicos
   (D81). Lo que imita al Spectrum (`PAUSE`, la variable `FRAMES` del 48K
   emulado) va en cincuentavos de segundo, con su propio reloj.
   Los programas se sincronizan con el VBLANK, nunca contando ciclos.
4. **Los periféricos del RP2350 no forman parte de la máquina.** Los programas solo
   usan los registros ZX-V, la API de la ROM y la propia CPU.

---

## 2. CPU

### 2.1 Perfil de instrucciones

La CPU es un **Hazard3** (núcleo RISC-V del RP2350) en modo máquina (M-mode).

| Elemento | Valor |
|---|---|
| ISA | RV32IMAC + Zicsr + Zifencei + Zba + Zbb + Zbs + Zbkb + Zcb + Zcmp |
| Opción de GCC | `-march=rv32imac_zicsr_zifencei_zba_zbb_zbs_zbkb_zcb_zcmp -mabi=ilp32` |
| Excluidas | Las extensiones propias del Hazard3 (`Xh3*`) y cualquier CSR no listada en 2.3 |
| Endianness | Little-endian |
| Accesos desalineados | Generan excepción (como en el Hazard3) |
| Frecuencia de referencia | 144 MHz, definitiva: dentro de los 150 MHz del RP2350 sin subir el voltaje, y múltiplo de 12 MHz por el USB host por PIO (ver hardware-pico.md §7) |

> **Comprobado en el chip:** `pico/isatest` ejecuta en el Hazard3 de una Pico 2
> los 200 programas aleatorios con los que el emulador se compara con QEMU, y
> los 200 dan lo mismo que en el emulador. Es también el perfil con el que la
> toolchain del SDK de la Pico compila para el RP2350
> (`rv32ima_zicsr_zifencei_zba_zbb_zbs_zbkb_zca_zcb_zcmp`).

> **Toolchain:** el binutils de Ubuntu 24.04 (2.42) todavía no conoce Zcmp. Mientras
> tanto, el SDK y la ROM se compilan con
> `-march=rv32imac_zicsr_zifencei_zba_zbb_zbs_zbkb_zcb`. El código sin Zcmp
> también es válido en el Hazard3.

### 2.2 Convención de llamada

Es la ABI estándar **ilp32**: argumentos en `a0`–`a7`, resultado en `a0` (y `a1`),
registros `s0`–`s11` preservados por el llamado, y `sp` alineado a 16 bytes.

### 2.3 CSR que pueden usar los programas

| CSR | Uso permitido |
|---|---|
| `mstatus` | Solo el bit `MIE` (bit 3): `csrci mstatus, 8` desactiva las interrupciones y `csrsi mstatus, 8` las activa |
| `mcycle`, `mcycleh`, `minstret`, `minstreth` | Lectura, para medir rendimiento. **No** sirven para temporizar (usa `time_us`) |
| `mepc`, `mcause`, `mtval` | Solo lectura, y solo dentro de un manejador instalado con la API |

Los programas **no** deben escribir en `mtvec`, en las CSR de interrupción del
Hazard3 ni en las de PMP. Esas pertenecen a la ROM.

### 2.4 Instrucciones especiales

- `wfi`: espera a la siguiente interrupción. Es el equivalente del `HALT` del Z80.
- `ecall`: reservada para usos futuros de la ROM. La API se llama mediante la tabla
  de vectores (sección 7).
- `ebreak`: punto de ruptura. Con un depurador activo en el emulador, para la CPU
  en él; si no, es una excepción como las de 7.5.

---

## 3. Mapa de memoria

### 3.1 Vista general

| Desde | Hasta | Tamaño | Contenido | Acceso del programa |
|---|---|---|---|---|
| `0x1000_0000` | `0x1003_FFFF` | 256 KB | Firmware de plataforma (arranque y servicios del núcleo 1) | Ninguno |
| `0x1004_0000` | `0x100B_FFFF` | 512 KB | **ROM ZX-V** (BASIC, editor y API) | Lectura y ejecución |
| `0x100C_0000` | `0x103F_FFFF` | 3,25 MB | **Disco ROM**: ficheros de solo lectura, la unidad `ROM:` (7.7) | Lectura (mediante la API) |
| `0x2000_0000` | `0x2000_3FFF` | 16 KB | **Área de sistema**: tabla API, registros y colas | Según cada registro |
| `0x2000_4000` | `0x2001_BFFF` | 96 KB | **VRAM, página 0** | Lectura y escritura |
| `0x2001_C000` | `0x2003_3FFF` | 96 KB | **VRAM, página 1** | Lectura y escritura |
| `0x2003_4000` | `0x2007_1FFF` | 248 KB | **RAM de usuario** (programas y datos del BASIC) | Lectura, escritura y ejecución |
| `0x2007_2000` | `0x2008_1FFF` | 64 KB | Reservada para el sistema (núcleo 1, USB, audio, ficheros) | **Prohibido** (protegida por PMP) |

Los tamaños son definitivos. El firmware de la Pico CPU usa de los 64 KB
reservados unos 53 KB para sus datos (USB, tarjeta, disco ROM, sonido, enlace)
y 0,5 KB de montón, deja unos 3 KB libres, y los dos bancos de 4 KB del final
son las pilas de sus dos núcleos y algo de código que tiene que ir rápido.

- La flash es de **solo lectura en tiempo de ejecución**. Escribir en ella detiene
  ambos núcleos del RP2350, así que los ficheros se guardan siempre en la tarjeta SD.
- En **modo 0** cada página de VRAM solo muestra sus primeros 24 KB. Los 72 KB
  restantes de cada página no se envían a la pantalla y el programa puede usarlos
  como RAM, salvo si usa el campo de juego virtual (4.9).
- La pila del núcleo 0 empieza en `RAMTOP` (variable del sistema) y crece hacia
  abajo, como en el Spectrum.

### 3.2 Área de sistema (`0x2000_0000`)

| Desplazamiento | Tamaño | Contenido |
|---|---|---|
| `0x0000` | 1 KB | Tabla de vectores de la API (256 punteros) |
| `0x0400` | 256 B | Registros de vídeo |
| `0x0500` | 256 B | Registros de entrada |
| `0x0600` | 256 B | Registros de sonido |
| `0x0700` | 256 B | Variables del sistema |
| `0x0800` | 512 B | Cola de comandos del AY (256 × 2 B) |
| `0x0A00` | 2 KB | Cola de eventos del beeper (256 × 8 B) |
| `0x1200` | 64 B | Tablas de remapeo de color (4 × 16 B) |
| `0x1240` | ~1,5 KB | Interfaz interna ROM ↔ plataforma (no usar desde programas) |
| `0x1800` | 4,5 KB | Tabla por línea (480 × 8 B; el resto, reservado) |
| `0x2A00` | 5,5 KB | Espacio de trabajo interno de la ROM |

---

## 4. Vídeo

### 4.1 Paleta

Cada píxel es un índice de 4 bits con el mismo orden que la tinta del Spectrum:

```
bit 3   bit 2   bit 1   bit 0
BRIGHT    G       R       B
```

| Índice | Color | RGB | Índice | Color | RGB |
|---|---|---|---|---|---|
| 0 | Negro | `000000` | 8 | Negro (BRIGHT) | `000000` |
| 1 | Azul | `0000D7` | 9 | Azul brillante | `0000FF` |
| 2 | Rojo | `D70000` | 10 | Rojo brillante | `FF0000` |
| 3 | Magenta | `D700D7` | 11 | Magenta brillante | `FF00FF` |
| 4 | Verde | `00D700` | 12 | Verde brillante | `00FF00` |
| 5 | Cian | `00D7D7` | 13 | Cian brillante | `00FFFF` |
| 6 | Amarillo | `D7D700` | 14 | Amarillo brillante | `FFFF00` |
| 7 | Blanco | `D7D7D7` | 15 | Blanco brillante | `FFFFFF` |

- Hay **15 colores distintos**: el 0 y el 8 son el mismo negro.
- **Convención:** el índice 8 se usa como **transparente** en las rutinas de copia
  de gráficos (sprites, fuentes). En pantalla se ve negro.
- El nivel "normal" (`D7`) es una calibración de la salida, no un dato del
  software. Cada salida debe acercarse a esa proporción (84 %).

### 4.2 Modos

| Modo | Resolución | Bytes por línea | Tamaño de página usado | Celdas de texto (8×8) |
|---|---|---|---|---|
| 0 | 256 × 192 | 128 | 24.576 B (o la página entera, ver 4.9) | 32 × 24 |
| 1 | 512 × 384 | 256 | 98.304 B | 64 × 48 |

### 4.3 Formato de píxel

Es **4 bpp empaquetado** y lineal: no hay entrelazado de tercios como en el
Spectrum. Para el píxel `(x, y)`:

```
desplazamiento = y * bytes_por_línea + x / 2
nibble         = bajo si x es par, alto si x es impar
```

Visto como palabras de 32 bits (little-endian), cada palabra contiene 8 píxeles
consecutivos, y **el píxel `x` ocupa los bits `4·(x mod 8)` a `4·(x mod 8)+3`**. Así
las operaciones de desplazamiento y máscara en registros de 32 bits son directas.

### 4.4 Cuadro lógico y borde

La pantalla se define como un **cuadro lógico** con borde, de 640×480 píxeles
de salida, lo mismo que sale por la pantalla (D81):

| Modo | Cuadro lógico | Área activa | Posición del área activa |
|---|---|---|---|
| 0 | 320 × 240 | 256 × 192 | (32, 24) |
| 1 | 640 × 480 | 512 × 384 | (64, 48) |

El borde es de 64 píxeles a cada lado y 48 líneas arriba y abajo (la mitad en
píxeles de modo 0). En `zxv.h`: `ZXV_FRAME_W`, `ZXV_FRAME_H`, `ZXV_BORDER_X`,
`ZXV_BORDER_Y`, `ZXV_FRAME_LINES(modo)` y `ZXV_BORDER_LINES(modo)`.

### 4.5 Registros de vídeo (`0x2000_0400`, *provisional*)

| Desp. | Nombre | Tipo | Descripción |
|---|---|---|---|
| `+0x00` | `MODE` | u8 | Modo de vídeo: 0 o 1 |
| `+0x01` | `PAGE` | u8 | Página visible: 0 o 1 |
| `+0x02` | `BORDER` | u8 | Color del borde (índice 0–15, sin remapeo) |
| `+0x03` | `VFLAGS` | u8 | Bit 0 `LINETAB`: usar la tabla por línea. Bit 1 `BLANK`: área activa apagada (solo borde) |
| `+0x04` | `SCROLL_X` | u16 | Desplazamiento horizontal del área activa (circular) |
| `+0x06` | `SCROLL_Y` | u16 | Desplazamiento vertical del área activa (circular) |
| `+0x08` | `REMAP` | u8 | Tabla de remapeo activa (0–3) |
| `+0x09` | `VSIZE` | u8 | Tamaño de la página en modo 0 (4.9): 0 = el de la pantalla |
| `+0x0C` | `OUT_INFO` | u32 | Solo lectura: tipo de salida (bits 0–3: 1 DVI, sin audio; 2 HDMI, con audio; los demás valores, reservados) y frecuencia de refresco en Hz (bits 8–15: siempre 60). Bits 16–17, el monitor, por el HPD del HDMI: 0 no se sabe (la protoboard, o aún no se ha conectado ninguno), 1 desconectado, 2 conectado (el emulador dice siempre 2). La salida la elige el usuario en la máquina (4.10); los programas solo la leen |

Con scroll, el píxel activo `(x, y)` muestra el píxel de la página
`((x + SCROLL_X) mod ancho, (y + SCROLL_Y) mod alto)`, donde `ancho` y `alto` son
los de la página: 512 × 384 en modo 1, y en modo 0 los que indique `VSIZE`.

### 4.6 Tablas de remapeo (`0x2000_1200`)

Hay 4 tablas de 16 bytes. La entrada `v` indica qué índice de color se muestra para
el valor de píxel `v`. Inicialmente son la identidad. Sirven para fundidos, ciclos
de color, FLASH y efectos, siempre dentro de los 15 colores.

### 4.7 Tabla por línea (`0x2000_1800`)

Cuando `VFLAGS.LINETAB = 1`, cada línea del cuadro lógico (240 en modo 0, 480 en
modo 1, empezando por la primera línea del borde superior) tiene una entrada de 8
bytes:

| Desp. | Tipo | Campo | Valor especial |
|---|---|---|---|
| `+0` | u8 | Color de borde de esta línea | `0xFF` = usar `BORDER` |
| `+1` | u8 | Tabla de remapeo de esta línea | `0xFF` = usar `REMAP` |
| `+2` | u16 | Scroll horizontal de esta línea | `0xFFFF` = usar `SCROLL_X` |
| `+4` | u16 | Línea de la página que se muestra aquí (0 a `alto` − 1) | `0xFFFF` = `(y + SCROLL_Y) mod alto` |
| `+6` | u16 | Reservado (escribir 0) | |

En las líneas de borde solo se usa el campo de borde. Con esta tabla se consiguen
las rayas de borde, las pantallas partidas con scroll independiente y los
degradados, **sin tener que cambiar registros a mitad de frame**.

### 4.8 Cuándo se aplican los cambios

En cada VBLANK el sistema toma una **instantánea** de los registros de vídeo, las
tablas de remapeo, la tabla por línea (si está activa) y la página visible, y la
envía a la pantalla. Esa instantánea se muestra en el frame siguiente.

- Escribir en una página **que no es la visible** es siempre seguro.
- Mientras `SYSFLAGS.LINK_BUSY = 1` (ver 8.1), el sistema todavía está leyendo la
  página que era visible en el último VBLANK, junto con los registros y tablas. **No
  escribas en esa página ni en la tabla por línea hasta que el bit vuelva a 0.** La
  lectura dura como máximo unos 6 ms en modo 1 y 1,5 ms en modo 0.
- **Doble buffer recomendado:** dibujar en la página oculta, cambiar `PAGE`, llamar
  a `wait_vblank` y seguir dibujando en la otra página.
- En el emulador, el modo estricto avisa si se escribe en una página mientras se
  está transfiriendo.

### 4.9 Campo de juego virtual (modo 0)

En modo 0 la página puede ser **mayor que la pantalla**. El área activa es
entonces una ventana sobre la página, que se mueve con `SCROLL_X`, `SCROLL_Y` y
la tabla por línea:

| `VSIZE` | Página | Bytes por línea | Uso |
|---|---|---|---|
| 0 | 256 × 192 | 128 | Normal; los 72 KB restantes quedan como RAM |
| 1 | 512 × 384 | 256 | Scroll en las dos direcciones (2 × 2 pantallas) |
| 2 | 1024 × 192 | 512 | Scroll horizontal (4 pantallas de ancho) |
| 3 | 256 × 768 | 128 | Scroll vertical (4 pantallas de alto) |

- Vale para las dos páginas. En modo 1 se ignora, porque la página ya es de
  512 × 384.
- El formato de píxel es el de 4.3, con los bytes por línea de la tabla.
- Como el scroll es circular, un juego puede desplazarse sin fin, igual que en
  las consolas: dibuja la columna o fila de tiles que va a entrar justo fuera de
  la ventana y luego mueve el scroll.
- Con `VSIZE` distinto de 0 se envía la página entera a la pantalla, como en
  modo 1, así que `LINK_BUSY` dura lo mismo que en modo 1 (unos 6 ms).
- En el BASIC, `MODE 0, ancho, alto` elige el tamaño de la página (y `MODE n`
  vuelve a `VSIZE = 0`); el editor siempre usa `VSIZE = 0`.
- La línea de origen de la tabla por línea (4.7) va de 0 a `alto` − 1 de la
  página, y el scroll de cada línea, de 0 a `ancho` − 1.
- Ejemplo: `sdk/examples/scroll` (página de 1024 × 192, solo se dibuja la
  columna de tiles que entra, y parallax con la tabla por línea).

### 4.10 Salidas (informativo)

| Salida | Señal | Notas |
|---|---|---|
| DVI/HDMI | 640×480@60 | Digital: HDMI con el audio (48 kHz, estéreo), o DVI sin audio para las pantallas que no acepten HDMI |

640×480 a 60 Hz es el modo que toda pantalla HDMI tiene que aceptar, y el VBLANK
llega en cada imagen de la salida: la máquina y la pantalla van al mismo ritmo
(D81). Se ve el cuadro lógico entero (4.4).

**HDMI o DVI lo elige el usuario**, no los programas: en la Pico, con un
pulsador de la placa, y se conserva al apagar. Un programa lo lee en `OUT_INFO`
(4.5).

No hay salida de vídeo analógica (decisiones D36). El sonido sale también por una
salida analógica de línea, en la placa.


---

## 5. Sonido

### 5.1 Componentes

- **Beeper** de 1 bit, como el del Spectrum.
- **Dos AY-3-8912** (como TurboSound), compatibles con el del 128K: reloj de
  1,7734475 MHz y los 16 registros de cada uno con la semántica original,
  incluido que **escribir R13 reinicia la envolvente**. El primero es el del
  128K; el segundo usa los registros 16–31 (5.3) y empieza en silencio, así que
  los programas de un solo AY no notan que existe.
- **Salida:** estéreo a 48 kHz, 16 bits. El AY admite mezcla mono, ABC o ACB.

### 5.2 Registros de sonido (`0x2000_0600`, *provisional*)

| Desp. | Nombre | Tipo | Escribe | Descripción |
|---|---|---|---|---|
| `+0x00` | `BEEPER` | u8 | programa | Bit 0 = nivel del beeper. Se muestrea a 48 kHz |
| `+0x01` | `AY_STEREO` | u8 | programa | 0 mono, 1 ABC, 2 ACB |
| `+0x02` | `VOLUME` | u8 | programa | Volumen general (0–255) |
| `+0x03` | `AFLAGS` | u8 | programa | Bit 0: silencio |
| `+0x04` | `AY_HEAD` | u16 | programa | Índice de escritura de la cola del AY |
| `+0x06` | `AY_TAIL` | u16 | sistema | Índice de lectura de la cola del AY |
| `+0x08` | `BEEP_HEAD` | u16 | programa | Índice de escritura de la cola del beeper |
| `+0x0A` | `BEEP_TAIL` | u16 | sistema | Índice de lectura de la cola del beeper |
| `+0x10` | `AY_REGS` | 16 × u8 | sistema | Copia de solo lectura de los registros del AY ya aplicados |
| `+0x20` | `AY2_REGS` | 16 × u8 | sistema | Lo mismo, del segundo AY |

### 5.3 Cola del AY (`0x2000_0800`)

Es un buffer circular de 256 entradas `{u8 registro, u8 valor}`. Cada entrada
equivale exactamente a **una escritura** en un AY: los registros 0–15 son los del
primero y los 16–31 los del segundo (`ZXV_AY2 + n`); los demás se ignoran.

```
entrada[AY_HEAD mod 256] = {reg, valor}
fence w, w
AY_HEAD = AY_HEAD + 1
```

La cola está llena cuando `AY_HEAD - AY_TAIL == 256`. El sistema consume las
entradas con una latencia de unos pocos milisegundos; el emulador lo hace en la
siguiente muestra de audio. La función de la API `ay_write` lo hace por ti.

### 5.4 Beeper y la cola con marca de tiempo

Hay dos formas de usar el beeper:

1. **Directa:** escribir en `BEEPER`. Basta para efectos normales. La resolución es
   la de la frecuencia de muestreo (unos 21 µs).
2. **Precisa:** la cola con marca de tiempo (`0x2000_0A00`), con 256 entradas
   `{u32 tiempo_us, u32 dato}`, donde el tiempo se obtiene con `time_us()`. Con
   el bit 31 del dato a 0, es un cambio del beeper: cualquier valor distinto de
   0 cuenta como 1. El sistema reproduce cada cambio en su instante exacto, con
   una latencia fija de **2 ms** (`ZXV_BEEP_LATENCY_US`). Los eventos con una
   marca de tiempo ya pasada se aplican de inmediato. Está pensada para motores
   de beeper multicanal. La función de la API `beeper_event` añade la marca de
   tiempo automáticamente, la hora actual; `beeper_event_at` (55), la que se le
   da: para ir por delante, como la música que va con `ay_event`.

Si se usan las dos formas a la vez, se suman.

**Escrituras del AY con su hora.** Con el bit 31 del dato a 1, la entrada es una
escritura en un AY: el registro en los bits 8–15 (0–31, como en la cola del AY;
los demás se ignoran) y el valor en los bits 0–7 (`ZXV_EV_AY_WRITE(reg, valor)`
en el SDK). Suena a su hora, con la misma latencia de 2 ms, antes de la muestra
de 48 kHz en la que cae (unos 21 µs de resolución), y en orden con los cambios
del beeper. Sirve para la **música digitalizada**: muestras por el volumen de
un canal con el tono y el ruido apagados, o efectos que necesitan su momento
exacto. La función de la API `ay_event` (51) pone la hora que se le da, así que
el programa puede ir por delante: la cola, de 256 entradas, son 32 ms de
muestras a 8 kHz. Lo escrito así sale también en `AY_REGS`.

- Las entradas se aplican en orden: una con la hora en el futuro hace esperar
  a las que van detrás, así que las horas deben ir de menor a mayor.
- Caben unas 36.000 escrituras por segundo entre las dos colas (las de la cola
  del AY y las de esta); las que no caben en una muestra salen en la siguiente.
- El orden entre la cola del AY y esta no está definido: un mismo registro se
  escribe por una o por otra.

### 5.5 Mezcla

| Fuente | Izquierda | Derecha |
|---|---|---|
| AY en mono (`AY_STEREO = 0`) | A, B y C al 50 % | A, B y C al 50 % |
| AY en ABC (`AY_STEREO = 1`) | A + 50 % de B | C + 50 % de B |
| AY en ACB (`AY_STEREO = 2`) | A + 50 % de C | B + 50 % de C |
| Beeper | 100 % | 100 % |

Con los dos AY, los canales del mismo nombre se suman antes de la panorámica
(`AY_STEREO` vale para los dos). Seis canales a todo volumen a la vez pueden
saturar: basta con bajar `VOLUME` o el volumen de los canales.

- Los niveles del AY siguen su DAC logarítmico.
- `VOLUME` escala la mezcla final; con `VOLUME = 0` no suena nada. La ROM lo pone
  a 200, con estéreo ABC, al arrancar.
- La salida pasa por un filtro paso alto de unos 3,7 Hz (coeficiente 1 − 2⁻¹¹ a
  48 kHz), como el condensador de acoplo de una salida real, que quita la
  continua del beeper.

---

## 6. Entrada

El sistema actualiza estos registros con la información de los dispositivos USB
(teclado, mandos y ratón, a través de un hub).

### 6.1 Registros de entrada (`0x2000_0500`, *provisional*)

| Desp. | Nombre | Tipo | Descripción |
|---|---|---|---|
| `+0x00` | `KEYROWS` | 8 × u8 | Matriz del Spectrum por semifilas (ver 6.2). Bits 0–4 a **0 = pulsada**, bits 5–7 a 1 |
| `+0x08` | `KEY_HEAD` | u8 | Índice de escritura de la cola de teclas (sistema) |
| `+0x09` | `KEY_TAIL` | u8 | Índice de lectura de la cola de teclas (programa) |
| `+0x10` | `KEYMAP` | 32 × u8 | Mapa de 256 bits indexado por código de uso HID. 1 = pulsada |
| `+0x30` | `KEY_MODS` | u8 | Modificadores HID (Ctrl, Shift, Alt, GUI, izquierda y derecha) |
| `+0x40` | `JOY0` | u16 | Mando del jugador 1 (ver 6.3) |
| `+0x42` | `JOY1` | u16 | Mando del jugador 2 |
| `+0x48` | `MOUSE_X` | i16 | Movimiento acumulado (circular), positivo a la derecha |
| `+0x4A` | `MOUSE_Y` | i16 | Movimiento acumulado (circular), positivo hacia abajo |
| `+0x4C` | `MOUSE_BTN` | u8 | Bits 0–2: izquierdo, derecho y central |
| `+0x4D` | `MOUSE_WHEEL` | i8 | Rueda acumulada, positiva hacia arriba |
| `+0x80` | `KEY_FIFO` | 64 × u16 | Cola de teclas tecleadas (ver 6.4) |

**Ratón.** El sistema suma a `MOUSE_X`, `MOUSE_Y` y `MOUSE_WHEEL` lo que
trae cada informe HID, sin aceleración ni límites: son contadores, no una
posición. Quien los usa guarda la última lectura y mira cuánto han avanzado
(con aritmética de 16 u 8 bits, para que dé igual que den la vuelta). El
BASIC (`MOUSE`) y el SDK (`zxv_mouse_update`) llevan así un puntero en
píxeles.

### 6.2 Matriz del Spectrum

| Semifila | Puerto original | Bit 0 | Bit 1 | Bit 2 | Bit 3 | Bit 4 |
|---|---|---|---|---|---|---|
| 0 | `0xFEFE` | CAPS SHIFT | Z | X | C | V |
| 1 | `0xFDFE` | A | S | D | F | G |
| 2 | `0xFBFE` | Q | W | E | R | T |
| 3 | `0xF7FE` | 1 | 2 | 3 | 4 | 5 |
| 4 | `0xEFFE` | 0 | 9 | 8 | 7 | 6 |
| 5 | `0xDFFE` | P | O | I | U | Y |
| 6 | `0xBFFE` | ENTER | L | K | J | H |
| 7 | `0x7FFE` | SPACE | SYMBOL SHIFT | M | N | B |

Traducción del teclado USB:
- **Shift** → CAPS SHIFT; **Ctrl** → SYMBOL SHIFT.
- **Retroceso** → CAPS SHIFT + 0 (DELETE). **Esc** → CAPS SHIFT + SPACE
  (BREAK). **Cursores** → CAPS SHIFT + 5/6/7/8.
- La matriz es para los programas que leen las semifilas, como en el Spectrum.
  El sistema no la usa: el BASIC y su editor trabajan con las teclas del PC
  (la cola de teclas y el mapa HID), y su BREAK es la tecla Esc en el mapa
  HID, no CAPS SHIFT + SPACE.
- **Los signos de puntuación** se traducen a su combinación con SYMBOL SHIFT (por
  ejemplo `,` → SYMBOL SHIFT + N), según el signo que da la tecla en la
  distribución del teclado, no su posición: `"` es SYMBOL SHIFT + P tanto con
  Shift + 2 (ES) como con Shift + ' (US). Mientras se mantiene, el Shift que
  hizo falta para el signo no pulsa CAPS SHIFT.
- Se traducen `! @ # $ % & ' ( ) _ < > ; " ^ - + = : £ ? / * , .`. Los que en
  el Spectrum piden el modo extendido (`[ ] { } \ | ~ ©`) no pulsan nada en la
  matriz (sí llegan a la cola de teclas).
- La distribución del teclado (ES, UK, US) es configurable. En el emulador la
  da el sistema anfitrión. En la Pico, **Ctrl+Alt+F1, F2 y F3** eligen la española, la
  británica y la de EE. UU.; la elegida se guarda en el reloj y es la de
  arranque (sin él, la española). Los acentos muertos dan la
  letra sin acento.
- **Ctrl+Alt+F12** no llega al programa: en la Pico enseña o quita la página de
  diagnóstico de la ULA (dev/hardware-pico.md §5.4). En el emulador no hace nada.
- Mantener una tecla la repite en la cola de teclas (a los 500 ms, y luego 20 veces
  por segundo); en la matriz sigue pulsada.

### 6.3 Mandos

| Bit | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|
| Función | Derecha | Izquierda | Abajo | Arriba | Fuego 1 | Fuego 2 | Fuego 3 | Start | Select | Conectado |

Los 5 bits bajos coinciden con la **interfaz Kempston**. Se admiten mandos HID
genéricos y DualShock 4 o DualSense; XInput (Xbox, 8BitDo en modo X) está previsto.

- **Dirección:** la cruceta o el stick izquierdo, pasado un cuarto de su recorrido.
- **Botones de un mando genérico:** los botones 1, 2 y 3 son los fuegos; el 9 es
  Select y el 10, Start.
- **DualShock 4 y DualSense:**

  | Botón | Bit |
  |---|---|
  | ✕ | Fuego 1 |
  | ○ | Fuego 2 |
  | □ | Fuego 3 |
  | Share o Create | Select |
  | Options | Start |
- **JOY0 y JOY1:** el primer mando que se conecta va a JOY0 y el segundo a JOY1.
  Si se desconecta uno, su registro se queda a 0 (sin el bit «conectado»).

### 6.4 Cola de teclas

Cada entrada es un u16:
- el **código Unicode** del carácter tecleado (BMP);
- un código de control ASCII para Enter (`0x0D`), Retroceso (`0x08`), Tab
  (`0x09`) y Esc (`0x1B`);
- `0xF000 | código_HID` para el resto de teclas sin carácter (F1–F12,
  cursores, Inicio…).

Para leer:
mientras `KEY_TAIL != KEY_HEAD`, toma `KEY_FIFO[KEY_TAIL mod 64]` e incrementa
`KEY_TAIL`. La función de la API `key_get` lo hace por ti.

**En el BASIC** (el editor e `INPUT`) se escriben los códigos del 32 al 126,
`£` (que es el 96) y `©` (el 127). El juego de caracteres del BASIC es el del
Spectrum, sin sitio para más letras (del `0x80` en adelante van los bloques,
los UDG y los tokens), así que:
- las letras de Latin-1 con tilde, diéresis, cedilla o virgulilla
  (U+00C0–U+00FF) se escriben sin ella: `á` → `a`, `Ü` → `U`, `ñ` → `n`,
  `ç` → `c`, `ý`/`ÿ` → `y` (y también `æ` → `a`, `ø` → `o`, `ð` → `d`);
- `¿` → `?` y `¡` → `!`;
- los demás caracteres que no son ASCII (`€`, `ß`, `þ`, `×`…) se ignoran.

---

## 7. ROM y API

### 7.1 Contenido de la ROM ZX-V

La ROM (`0x1004_0000`) contiene el intérprete BASIC, el editor, las rutinas de la
API y el manejador de interrupciones y excepciones. **Es el mismo binario en el
hardware y en el emulador.** Todo lo que depende de la plataforma (USB, SD, audio,
reconocer interrupciones) lo hace la capa de plataforma a través de la interfaz
interna (`0x2000_1240`): en la Pico la implementa el firmware y en el emulador el
propio emulador.

La plataforma rellena la interfaz antes de saltar a la ROM. Versión 4:

| Desp. | Contenido |
|---|---|
| `+0x00` | Versión de la interfaz (4) |
| `+0x04` | `void irq_ack(void)`: reconoce la interrupción de VBLANK; la ROM la llama al principio del manejador |
| `+0x08` | `u32 time_us(void)`: contador de microsegundos |
| `+0x0C`…`+0x40` | Sistema de ficheros de la tarjeta (desde la versión 2), en este orden: `fs_open`, `fs_read`, `fs_write`, `fs_close`, `fs_seek`, `fs_stat`, `fs_remove`, `fs_rename`, `fs_mkdir`, `fs_chdir`, `fs_getcwd`, `fs_opendir`, `fs_readdir`, `fs_closedir`. Mismas firmas que las funciones 14–17 y 21–30 de la API (§7.3) |
| `+0x44`, `+0x48` | El reloj (desde la versión 3): `clock_get` y `clock_set`, con las firmas de las funciones 48 y 49 (§7.9) |
| `+0x4C`, `+0x50` | El idioma (desde la versión 4): `u32 lang_get(void)` y `int lang_set(u32 idioma)` (§7.10) |

En la Pico, el sistema de ficheros es FatFs en el núcleo de servicios: la
función de la plataforma deja la petición en un buzón y espera la respuesta. La
ROM rellena las entradas de ficheros de la API con estos mismos punteros.

En el emulador, estas funciones apuntan a ranuras de la zona de firmware que
contienen una instrucción *custom-0* (opcode `0x0B`), que el emulador ejecuta
en C. En la Pico, su código está en la propia interfaz (desde `+0x100`), porque
la flash del firmware está cerrada con PMP para la CPU.

### 7.2 Tabla de vectores

La API es una **tabla de 256 punteros a función** en `0x2000_0000`, que la ROM
rellena al arrancar. Para llamar a la función `n`:

```asm
    lui  t0, 0x20000          # t0 = 0x2000_0000
    lw   t0, 4*n(t0)          # puntero a la función n
    jalr ra, t0               # llamada con la ABI ilp32
```

Como la tabla está en RAM, un programa puede **redirigir** una entrada para
interceptar una rutina del sistema, igual que se hacía con los ganchos del Spectrum.

### 7.3 Funciones iniciales (*provisional*)

| n | Función | Descripción |
|---|---|---|
| 0 | `u32 api_version(void)` | Versión de la API |
| 1 | `void wait_vblank(void)` | Espera al siguiente VBLANK (equivale a `HALT`) |
| 2 | `void set_frame_hook(void (*fn)(void))` | Instala (o quita, con NULL) la función que se llama en cada VBLANK |
| 3 | `u32 time_us(void)` | Contador de microsegundos (circular) |
| 4 | `void set_mode(int modo)` | Cambia el modo de vídeo |
| 5 | `void set_page(int página)` | Cambia la página visible |
| 6 | `void cls(int color)` | Borra la página de dibujo y lleva el cursor del texto arriba a la izquierda |
| 7 | `void plot(int x, int y, int color)` | Dibuja un píxel (origen arriba a la izquierda) |
| 8 | `void print_char(int c)` | Imprime un carácter (su código Unicode) en la posición del cursor, con los colores de `text_colors`. Dibuja ASCII, `©` y las letras del español de la fuente de la ROM (§7.10); los demás salen como `?` |
| 9 | `void print_str(const char *s)` | Imprime una cadena en UTF-8, como `print_char` |
| 10 | `int key_get(void)` | Siguiente tecla de la cola, o −1 si está vacía |
| 11 | `u32 joy_read(int n)` | Estado del mando `n` |
| 12 | `void ay_write(int reg, int valor)` | Escribe en el AY mediante la cola |
| 13 | `void beeper_event(int nivel)` | Cambio del beeper con marca de tiempo |
| 14 | `int file_open(const char *nombre, int modo)` | Abre un fichero de la SD |
| 15 | `int file_read(int f, void *buf, u32 n)` | Lee de un fichero |
| 16 | `int file_write(int f, const void *buf, u32 n)` | Escribe en un fichero |
| 17 | `int file_close(int f)` | Cierra un fichero |
| 18 | `int file_load(const char *nombre, void *dir, u32 max)` | Carga un fichero entero en `dir` si cabe en `max` bytes (si no, `ZXV_E_RANGE` y no carga nada); devuelve su tamaño |
| 19 | `void exit_to_basic(int código)` | Vuelve al BASIC: con 0, el informe `0 OK`; con otro código, `W` con ese número (§9.2) |
| 20 | `void report_error(int código, const char *msg)` | Vuelve al BASIC con el informe `W` y el texto `msg` (UTF-8, hasta 47 bytes, que se copian), más ` (código)` si no es 0 (§9.2) |
| 21 | `int file_seek(int f, i32 desp, int desde)` | Mueve la posición (`desde`: 0 inicio, 1 actual, 2 final); devuelve la nueva |
| 22 | `int file_stat(const char *nombre, dirent *e)` | Nombre, tamaño y atributos |
| 23 | `int file_remove(const char *nombre)` | Borra un fichero o una carpeta vacía |
| 24 | `int file_rename(const char *de, const char *a)` | Renombra o mueve |
| 25 | `int dir_make(const char *nombre)` | Crea una carpeta |
| 26 | `int dir_change(const char *nombre)` | Cambia la carpeta actual |
| 27 | `int dir_current(char *buf, u32 n)` | Copia la carpeta actual; devuelve su longitud |
| 28 | `int dir_open(const char *nombre)` | Abre una carpeta para listarla (`""` = la actual) |
| 29 | `int dir_read(int d, dirent *e)` | Siguiente entrada: 1, o 0 al final |
| 30 | `int dir_close(int d)` | Cierra la carpeta |
| 31 | `int zxv_exec(const char *nombre, int *resultado)` | Carga y ejecuta un `.ZXV` (§9.2); deja en `*resultado` lo que devuelve. Desde un programa `.ZXV` da `ZXV_E_ACCESS`: todos se cargan en la misma dirección |
| 32–47 | Gráficos | Ver §7.8 |
| 48 | `int clock_get(zxv_clock *c)` | La fecha y la hora (§7.9) |
| 49 | `int clock_set(const zxv_clock *c)` | Pone el reloj en hora (§7.9) |
| 50 | `int lang_set(u32 idioma)` | Cambia el idioma de la máquina (§7.10) |
| 51 | `void ay_event(u32 tiempo_us, int reg, int valor)` | Escritura en el AY a su hora, por la cola con marca de tiempo (§5.4); con la cola llena, espera |
| 52 | Gráficos | Ver §7.8 |
| 53 | `void text_colors(int tinta, int papel)` | Colores (0–15) del texto de `print_char` y `print_str`. Al arrancar, 0 y 7. `set_mode` borra con el papel |
| 54 | `void text_at(int col, int fila)` | Lleva el cursor del texto a esa celda (desde 0, arriba a la izquierda) |
| 55 | `void beeper_event_at(u32 tiempo_us, int nivel)` | Cambio del beeper a esa hora, por la cola con marca de tiempo (§5.4); con la cola llena, espera |

Las funciones de ficheros devuelven un valor ≥ 0 si todo va bien o un código
de error negativo (§7.7).

Los números de función no se reordenarán a partir de la v1.0.

### 7.4 Interrupciones

- La ROM es dueña de `mtvec`. En cada VBLANK (60 Hz):
  1. Incrementa `FRAMES`.
  2. Llama al gancho de frame, si hay uno, con las interrupciones desactivadas.
  3. Vuelve.
- El gancho es una función C normal: debe ser breve y no puede llamar a
  `wait_vblank`.
- Un programa puede desactivar las interrupciones (`mstatus.MIE`) durante el tiempo
  que quiera. Mientras estén desactivadas no se incrementa `FRAMES`, pero el vídeo,
  el sonido y la entrada **siguen funcionando**, porque los atiende otro núcleo.

### 7.5 Excepciones

Una instrucción ilegal, un acceso indebido (incluida la zona protegida), un acceso
desalineado o un `ebreak` vuelven al BASIC, que lo cuenta con un informe al estilo
Spectrum:

- Si fallaba código nativo llamado desde el BASIC (`USR`), el informe es
  `W Error en el código nativo, línea:sentencia`, y el programa BASIC sigue ahí.
- Si fallaba un programa `.ZXV`, el informe es
  `W Error en el código nativo, 0:1` (o el texto de `report_error`, si lo
  dio). La memoria del BASIC empieza de nuevo: el programa nativo la ocupaba.
- En el MONITOR, el informe dice la causa y el PC, por ejemplo
  `Instrucción ilegal, PC=2003A1F0`.

### 7.6 Reset de emergencia

**Ctrl + Alt + Supr** lo vigila el núcleo de servicios, no la CPU. Reinicia el
núcleo 0 y vuelve al BASIC aunque el programa se haya colgado con las
interrupciones desactivadas.

- **Cómo entra la ROM:** la CPU vuelve a entrar por la segunda entrada de la ROM,
  `ZXV_ROM_BASE + 4`. Con la memoria como estaba, la ROM vuelve al BASIC igual que
  tras una excepción.
- **Lo que queda:**
  - Si estaba un programa en BASIC, se conserva, con el informe `L BREAK into
    program`.
  - Si era un `.ZXV`, el informe es el mismo, `L BREAK into program, 0:1`,
    y la memoria del BASIC empieza de nuevo, porque el programa la ocupaba.
  - En el monitor, se ve «Ctrl+Alt+Supr».
- **Lo que se para:** se callan los dos AY y el beeper, y se cierran los ficheros
  abiertos.
- **Lo que no recibe el programa:** la tecla Supr de esa combinación no llega a la
  cola de teclas.
- **En el emulador** se hace lo mismo, con las teclas de verdad o con `{RESET}` en
  `--type`.

### 7.7 Ficheros (tarjeta SD y disco ROM)

La tarjeta usa FAT, FAT32 o exFAT. El emulador la imita sobre una carpeta del
anfitrión (`--sd`), o usa una imagen de tarjeta con el mismo código que la Pico
(`--sd-image`).

**Las unidades.** Las mismas funciones sirven para dos unidades:

- `SD:`, la tarjeta, donde se lee y se escribe.
- `ROM:`, el **disco ROM**: ficheros de solo lectura en la flash
  (`0x100C_0000`, 3,25 MB) que vienen con la máquina, con los ejemplos. En el
  emulador es un fichero (`romdisk.bin`, o `--romdisk`). Lo que escribe en él
  (abrir para escribir, borrar, renombrar, crear carpetas) da `ZXV_E_ACCESS`.

Una ruta que empieza por `ROM:` o `SD:` (sin distinguir mayúsculas) va a esa
unidad **desde su raíz**: `ROM:graphics` es `ROM:/graficos`, y `ROM:` sola, la raíz.
Sin prefijo, la ruta es de la unidad en la que está la carpeta actual. La
carpeta actual puede estar en cualquiera de las dos: `dir_change("ROM:")`
entra en el disco ROM y `dir_change("SD:")` vuelve a la raíz de la tarjeta.
`dir_current` da `/...` en la tarjeta (como siempre) y `ROM:/...` en el disco
ROM. Si no hay disco ROM, lo que va a `ROM:` da `ZXV_E_NOENT`.

**La carpeta del programa (el BASIC).** El BASIC apunta la carpeta del
fichero del que cargó el programa (`.tap`, `.tzx` o `.bas`; `NEW` y los
`.zxv` la borran). `LOAD`, `VERIFY`, `MERGE` y `OPEN #` para leer, con un
nombre relativo que no está en la carpeta actual (probando las extensiones
de cada orden), lo buscan ahí. La carpeta actual va antes; lo que se graba
no cambia. Los programas nativos no la usan: llevan sus ficheros dentro y
miran la carpeta actual (D82).

**Rutas:**
- El separador es `/`. Una ruta que empieza por `/` es absoluta; si no, es
  relativa a la carpeta actual.
- `.` y `..` funcionan. `..` en la raíz se queda en la raíz: no se puede salir de
  la tarjeta.
- **No se distinguen mayúsculas y minúsculas**, pero se conserva la grafía al crear.
- Nombres de hasta 63 caracteres ASCII imprimibles, sin `\ : * ? " < > |` y sin
  acabar en espacio ni en punto. Rutas de hasta 255 caracteres.

**Modos de `file_open`** (se combinan):

| Bit | Nombre | Efecto |
|---|---|---|
| `0x01` | `ZXV_O_READ` | Lectura |
| `0x02` | `ZXV_O_WRITE` | Escritura |
| `0x04` | `ZXV_O_CREATE` | Crea el fichero si no existe (requiere escritura) |
| `0x08` | `ZXV_O_TRUNC` | Lo vacía al abrir |
| `0x10` | `ZXV_O_APPEND` | Empieza al final |

**Límites:** 8 ficheros y 4 carpetas abiertos a la vez en cada unidad. Los
números de los ficheros del disco ROM son el 8 y siguientes, y los de sus
carpetas, el 4 y siguientes.

**En una tarjeta de verdad** (y con `--sd-image`):
- No se puede borrar ni renombrar un fichero abierto, ni abrir para escribir uno que
  ya está abierto (`ZXV_E_ACCESS`).
- No salen en los listados los ficheros y carpetas ocultos o de sistema (los que deja
  Windows o macOS), aunque se pueden abrir por su nombre.
- Tampoco salen los nombres que no cumplen las reglas de arriba (por ejemplo, con
  acentos), y esos no se pueden abrir.

**Entrada de carpeta (`dirent`, 72 bytes):** nombre (`char[64]`, terminado en
cero), tamaño (u32, en el desplazamiento 64) y atributos (u32 en el 68; bit 0 =
carpeta). `dir_read` devuelve las entradas en orden alfabético, sin distinguir
mayúsculas.

**Buffers:** deben estar enteros en la SRAM accesible (o en la flash de la ROM,
desde `0x10040000`, si solo se leen). Si no, la función devuelve `ZXV_E_FAULT` sin tocar nada. Leer un fichero
sobre memoria que contiene código es válido: después hay que ejecutar `fence.i`.

**Códigos de error:**

| Código | Nombre | Significado |
|---|---|---|
| −1 | `ZXV_E_NOENT` | No existe |
| −2 | `ZXV_E_EXIST` | Ya existe |
| −3 | `ZXV_E_NAME` | Nombre o ruta no válidos |
| −4 | `ZXV_E_NOTDIR` | Una parte de la ruta no es una carpeta |
| −5 | `ZXV_E_ISDIR` | Es una carpeta |
| −6 | `ZXV_E_NOTEMPTY` | Carpeta no vacía |
| −7 | `ZXV_E_BADF` | Descriptor no válido |
| −8 | `ZXV_E_MFILE` | Demasiados abiertos |
| −9 | `ZXV_E_ACCESS` | Operación no permitida (por el modo, borrar la raíz, o `zxv_exec` desde un programa `.ZXV`) |
| −10 | `ZXV_E_FAULT` | Buffer fuera de la memoria permitida |
| −11 | `ZXV_E_IO` | Error de lectura o escritura |
| −12 | `ZXV_E_NOCARD` | No hay tarjeta |
| −13 | `ZXV_E_NOSPC` | Tarjeta llena |
| −14 | `ZXV_E_FORMAT` | Fichero `.ZXV` no válido |
| −15 | `ZXV_E_RANGE` | No cabe (en la memoria de usuario, o en el buffer de `dir_current` o `file_load`) |
| −16 | `ZXV_E_NOCLOCK` | El reloj no está en hora (§7.9) |
| −17 | `ZXV_E_INVAL` | Valor no válido (una fecha que no existe, §7.9) |

### 7.8 Gráficos para juegos

ZX-V no tiene hardware de sprites ni de tiles. El «chip gráfico» es un
**motor de la ROM** que se ejecuta en la CPU y dibuja en la VRAM o en memoria; la
pantalla solo muestra la página terminada (4.8). Así:

- El emulador ejecuta el mismo código, así que los píxeles son idénticos en el
  hardware y en el PC, y el motor se prueba como el BASIC.
- La carga del enlace no depende de lo que se dibuje: se envía la página haya un
  sprite o mil.
- Los formatos se pueden mejorar en la ROM sin cambiar la máquina.

El hardware de vídeo aporta lo que no se puede hacer bien por software:

| Recurso | Uso en juegos |
|---|---|
| Dos páginas | Doble buffer: dibujar en la oculta y cambiar `PAGE`, sin parpadeo |
| Scroll y campo virtual (4.9) | Recorrer un escenario mayor que la pantalla dibujando solo la franja que entra |
| Tabla por línea (4.7) | Parallax, pantallas partidas, agua y carreteras pseudo-3D: cada línea elige qué línea de la página muestra y con qué scroll (escala vertical y desplazamiento, sin coste de CPU) |
| Tablas de remapeo (4.6) | Ciclos de color, fundidos y destellos sin tocar la VRAM |

#### Formatos

Todos los gráficos son de **4 bpp como la VRAM** (4.3), y el índice **8 es
transparente** donde se indica. Los tipos y las llamadas están en
`sdk/include/zxv_gfx.h` (con el prefijo `zxv_`). Los bitmaps, los tiles
y los sprites preparados van alineados a 4 bytes.

```c
typedef struct {            // Bitmap: una pantalla, una hoja de sprites, un buffer
    u8  *pix;               // primer píxel, alineado a 4
    u16  stride;            // bytes por línea, múltiplo de 4
    u16  w, h;              // en píxeles
    u16  reservado;
} bitmap;

typedef struct {            // Mapa de tiles
    const u16 *cells;       // celdas, fila a fila
    u16  w, h;              // en celdas
    const u8 *tiles;        // gráficos de los tiles, seguidos (32 o 128 B cada uno)
    u8   size;              // lado del tile: 8 o 16
    u8   flags;             // bit 0: el mapa se repite en las dos direcciones
} tilemap;

typedef struct {            // Textura del modo 7
    const u8 *texels;       // 1 byte por texel (0–15), fila a fila
    u8   log2size;          // lado = 2^log2size (de 1 a 10: de 2 a 1024; en la
                            // RAM de usuario caben hasta 256 × 256, 64 KB)
    u8   wrap;              // 1: se repite; 0: fuera se ve el color 'out'
    u8   out;
} m7tex;

typedef struct { i32 u, v, du, dv; } m7line;   // coma fija 16.16

typedef struct {            // Cámara para m7_camera (coma fija 16.16)
    i32  x, y;              // posición sobre la textura
    i32  height;            // altura sobre el suelo
    i32  angle;             // 0x10000 = una vuelta completa
    i32  focal;             // distancia focal, en píxeles
    i16  horizon;           // línea del destino donde está el horizonte (entera)
    i16  reservado;
} m7cam;
```

- **Sprite preparado:** `spr_prepare` convierte un rectángulo de un bitmap en un
  bloque opaco con la máscara de cada palabra de 32 bits ya calculada, para
  dibujar 8 píxeles por operación con `dst = (dst & ~m) | (src & m)`. Su formato
  interno lo decide la ROM.
- **Sprite escalado:** `spr_draw_scaled` lo dibuja en `w × h` píxeles
  eligiendo el más cercano: el píxel `i` de los `w` del destino es el
  `floor((2·i + 1)·ancho / (2·w))` del sprite (el que cae en su centro), y lo
  mismo en vertical. Con espejo, el píxel `i` es el que sería el `w − 1 − i`
  (la imagen sin espejo, al revés). A su tamaño da lo mismo que `spr_draw`.
  Usa unos 2,5 KB de la pila del que la llama.
- **Celda de mapa (u16):** bits 0–11, número de tile (hasta 4096); bit 12, espejo
  horizontal; bit 13, espejo vertical; bit 14, transparente (el 8 deja ver lo de
  debajo; si no, el tile es opaco y se dibuja más deprisa); bit 15, prioridad.
- **Modo 7:** en cada línea del destino, el píxel `x` muestra el texel
  `(u + x·du, v + x·dv)`, con la parte entera de cada coordenada (aritmética
  de 32 bits sin signo: lo negativo cae fuera). Cambiando los parámetros línea
  a línea salen la rotación, el zoom y la perspectiva, como con el HDMA de la
  SNES. Los texels valen 0–15.
- **Cámara:** `m7_camera` rellena `l[i]` para la línea `i` del destino (desde
  la 0): a la distancia `height·focal / (i − horizon)`, mirando en la
  dirección `angle` (0 = hacia `u` creciente), con el centro del destino en
  `x`. Las líneas del horizonte para arriba quedan a cero; se dibuja con
  `m7_draw(t, l + y0, y0, n)`.
- **Líneas:** en cada paso del eje mayor, el otro avanza
  `floor((2·k·dm + dM) / (2·dM))` (el píxel más cercano a la recta).
- **Seno:** el motor usa una tabla de un cuarto de vuelta con interpolación
  lineal, solo con enteros.

#### Funciones

Los números 32–63 de la tabla de vectores quedan reservados para los gráficos.

| n | Función | Descripción |
|---|---|---|
| 32 | `void gfx_target(const bitmap *b)` | Destino de dibujo; `NULL` = la página de dibujo |
| 33 | `void gfx_draw_page(int página)` | Elige la página de dibujo (también para `cls`, `plot` y el texto). Su tamaño sigue a `MODE` y `VSIZE` |
| 34 | `void gfx_clip(int x0, int y0, int x1, int y1)` | Rectángulo de recorte del destino (x1 e y1 excluidos) |
| 35 | `void gfx_blit(const bitmap *src, int sx, int sy, int w, int h, int x, int y, int flags)` | Copia un rectángulo. `flags`: bit 0 espejo horizontal, bit 1 espejo vertical, bit 2 transparente |
| 36 | `u32 spr_prepare(const bitmap *src, int sx, int sy, int w, int h, void *buf)` | Prepara un sprite (hasta 1024 × 1024; lo que cae fuera del bitmap es transparente); con `buf = NULL` solo devuelve los bytes que necesita: 8 + h·⌈w/8⌉·8 |
| 37 | `void spr_draw(const void *spr, int x, int y, int flags, const u8 *colores)` | Dibuja un sprite con máscara. `flags` como en `gfx_blit`; `colores` es una tabla de 16 para recolorearlo (`NULL` = tal cual) |
| 38 | `int spr_collide(const void *a, int ax, int ay, const void *b, int bx, int by)` | 1 si algún píxel opaco de los dos sprites coincide (a su tamaño) |
| 39 | `void map_draw(const tilemap *m, int mx, int my, int x, int y, int w, int h, int filtro)` | Dibuja la parte del mapa que empieza en el píxel `(mx, my)` del mapa en el rectángulo `(x, y, w, h)` del destino. `filtro`: 0 todas las celdas, 1 sin prioridad, 2 solo con prioridad |
| 40 | `u16 map_cell(const tilemap *m, int px, int py)` | Celda que hay en el píxel `(px, py)` del mapa (para colisiones con el escenario) |
| 41 | `void m7_draw(const m7tex *t, const m7line *l, int y, int n)` | Dibuja `n` líneas en modo 7 a partir de la línea `y` del destino |
| 42 | `void m7_camera(m7line *l, int n, const m7cam *c)` | Calcula las líneas de un suelo en perspectiva a partir de una cámara (posición, altura, ángulo, horizonte y distancia focal) |
| 43 | `void gfx_line(int x0, int y0, int x1, int y1, int c)` | Línea |
| 44 | `void gfx_rect(int x, int y, int w, int h, int c)` | Rectángulo relleno |
| 45 | `void gfx_tri(const i16 *p, int c)` | Triángulo relleno (3 pares x, y) |
| 46 | `void gfx_poly(const i16 *p, int n, int c)` | Polígono relleno de 3 a 64 vértices (regla par-impar) |
| 47 | `void gfx_circle(int x, int y, int r, int c, int relleno)` | Círculo o disco |
| 52 | `void spr_draw_scaled(const void *spr, int x, int y, int w, int h, int flags, const u8 *colores)` | Dibuja un sprite estirado o encogido a `w × h` (de 1 a 4096; si no, nada), con `flags` y `colores` como `spr_draw`: para lo que se acerca o se aleja |

Reglas comunes:

- Las coordenadas son con signo y todo se recorta contra el rectángulo de
  recorte: un sprite puede estar a medias fuera de la pantalla, y nunca se
  escribe fuera del destino.
- El orden de dibujo da las prioridades. Un frame típico es: mapa de fondo
  (`filtro` 1), sprites y la parte delantera del mapa (`filtro` 2).
- Los triángulos y polígonos usan la regla de cobertura superior-izquierda, así
  que dos triángulos con un lado común no se solapan ni dejan huecos.
- **El resultado está definido píxel a píxel por el código de la ROM.** Si una
  implementación acelera una función, debe dar exactamente los mismos píxeles, y
  las pruebas lo comprueban.
- Copiar un bitmap sobre sí mismo con zonas que se solapan no está definido.

Desde el BASIC se usa con `GRAB`, `TILE`, `MAP`, `TEXTURE`, `SPRITE`,
`FLOOR`, `POLY`, `BOX`, `DISC`, `CLIP`, `WAIT`, `HIT`, `CELL` y `PIXEL` (ver
[basic.md §9.8](basic.md)).

El motor está en `rom/src/gfx/`, en C portable. Las pruebas lo comparan con
versiones de referencia escritas píxel a píxel (con parámetros y recortes
aleatorios) y comprueban que la ROM, compilada para RISC-V, dibuja la misma
escena que el motor compilado para el PC, bit a bit.

#### Rendimiento (medido en una Pico 2)

A 144 MHz y 60 fps hay **2,4 millones de ciclos por frame**. Estas cifras son
ciclos reales en una Pico 2 (medidos a 150 MHz: en ciclos apenas cambia), con
el motor ejecutándose desde la flash como la ROM (`pico/bench`). El emulador cuenta una instrucción por
ciclo; en la Pico cuesta lo mismo (modo 7) o hasta 1,8 veces más (mapa
desplazado), por las esperas de las cargas y los saltos. Desde la RAM no
mejora más de un 16 % (y en las operaciones largas, nada): la caché de la
flash basta.

| Operación | Modo 0 (256×192) | Modo 1 (512×384) |
|---|---|---|
| Borrar la página | 28.000 ciclos (1,2 %) | 64.000 (2,7 %) |
| Mapa de tiles de 8×8 completo, alineado | 93.000 (3,9 %) | 280.000 (11,7 %) |
| Mapa de tiles de 8×8 completo, desplazado | 315.000 (13,1 %) | 1,14 M (48 %) |
| Mapa de tiles de 16×16 completo, alineado | 52.000 (2,2 %) | 153.000 (6,4 %) |
| 100 sprites de 16×16 con máscara | 146.000 (6,1 %) | 134.000 (5,6 %) |
| 100 sprites de 16×16 escalados a 32×32 | 1,50 M (63 %) | 1,48 M (62 %) |
| 100 sprites de 16×16 escalados a 12×12 | 668.000 (28 %) | 653.000 (27 %) |
| Modo 7 a pantalla completa (~15 ciclos por píxel) | 738.000 (31 %) | 2,90 M (121 %) |
| 100 triángulos de unos 470 píxeles | 688.000 (29 %) | 675.000 (28 %) |
| 100 líneas al azar | 333.000 (14 %) | 610.000 (25 %) |

- Con el campo virtual (4.9) el escenario no se redibuja: el scroll lo hace el
  vídeo y solo se dibuja la franja que entra (una columna de 8 × 144 píxeles
  cuesta unos 19.000 ciclos en modo 0, el 0,8 %). En modo 1 es casi obligatorio:
  redibujar el mapa desplazado entero se lleva el 48 % del frame.
- En modo 1, el modo 7 a pantalla completa no cabe en un frame: hay que
  limitarlo al suelo (en `sdk/examples/graphics`, 56 líneas en modo 0 cuestan
  unos 215.000 ciclos) o ir a 30 fps.
- Un sprite escalado cuesta unas tres veces lo que uno de su tamaño final, y
  algo más en los pequeños (unos 6.700 ciclos por sprite de 12×12): para unos
  pocos objetos que se acercan, no para todos. En la Pico va a 1,08 ciclos
  por instrucción, casi lo que cuenta el emulador.
- `zxv --profile` da un perfil por muestreo del programa (ver
  [la guía del emulador](../docs/es/emulator.md)).

#### Aceleración (opcional)

El RP2350 tiene interpoladores (pensados para el modo 7 y para buscar tiles, a
unos 3–5 ciclos por píxel) y DMA, pero los programas no pueden usarlos (9.1).
Si las medidas lo piden, algunas funciones de esta sección pasarán a la
interfaz de plataforma (7.1), como los ficheros: en la Pico usarán esos
periféricos, y en el emulador habrá una versión en C con los mismos píxeles.
Para el programa no cambia nada: llama a la misma entrada de la tabla.

### 7.9 Reloj

La máquina tiene **fecha y hora**: en la Pico, un reloj con su pila (un PCF8563 o un
DS3231);
en el emulador, el reloj del ordenador. Con ellas se fechan los ficheros que
se crean o se cambian en la tarjeta.

**`zxv_clock` (8 bytes):**

| Desp. | Tipo | Contenido |
|---|---|---|
| `+0` | u16 | Año, de 2000 a 2099 |
| `+2` | u8 | Mes, 1–12 |
| `+3` | u8 | Día, 1–31 |
| `+4` | u8 | Hora, 0–23 |
| `+5` | u8 | Minutos, 0–59 |
| `+6` | u8 | Segundos, 0–59 |
| `+7` | u8 | Día de la semana, 1 (lunes) a 7 (domingo). Lo pone `clock_get`; `clock_set` no lo mira |

- **`clock_get`** devuelve 0, o `ZXV_E_NOCLOCK` si el reloj no está en hora: no hay
  reloj, se le acabó la pila o nunca se puso en hora. Entonces no toca `*c`.
- **`clock_set`** devuelve 0, o `ZXV_E_INVAL` si la fecha no existe o está fuera
  de 2000–2099. Sin reloj, la hora se mantiene hasta que se apaga la máquina.
- **La hora es la local**, sin zonas ni cambio de horario: la que se le pone.
- Las dos son lentas (pasan por la plataforma): para medir tiempos, `time_us` o
  `FRAMES`.
- Los ficheros sin reloj en hora se fechan el 1 de enero de 2026.

### 7.10 Idioma

La máquina tiene un **idioma**: el de los textos de la ROM (los informes del
BASIC, la barra y el menú del editor). Es un código ISO 639-1 de dos letras
minúsculas guardado en un u32, la primera letra en el byte bajo:
`ZXV_LANG('e', 's')` = `0x7365`.

- **Dónde está.** La variable `LANG` (§8.1) tiene el actual; los programas la
  leen ahí (`zxv_lang_get()` en el SDK). Para cambiarlo, `lang_set` (función 50),
  que lo guarda también en la plataforma: devuelve 0, o `ZXV_E_INVAL` si no son
  dos letras minúsculas. En el BASIC, `LANGUAGE "es"`.
- **Al arrancar en frío**, la ROM lo pide a la plataforma (`lang_get`, desde la
  versión 4 de la interfaz); en caliente (Ctrl+Alt+Supr) se queda el que había.
  Con una plataforma anterior, o si lo que da no vale, el inglés.
- **Persistencia.** En la Pico se guarda con la hora en el reloj (en registros
  de la alarma que ZX-V no usa, dev/hardware-pico.md §7): sobrevive al
  apagado mientras dure la pila. Sin reloj se queda hasta que se apaga; el de
  fábrica es el inglés, como cuando la ROM no sabe el idioma. En el emulador
  lo da `--lang` o la aplicación, que lo guarda en sus ajustes.
- **Idiomas de la ROM:** inglés (`en`) y español (`es`). Cualquier otro código vale,
  y la ROM saca sus textos en inglés; así un programa puede usar `LANG` para
  elegir los suyos.
- **Los textos** van en UTF-8. La fuente de la ROM tiene, además de ASCII, las
  letras del español (`á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿ ¡`) en una tabla aparte:
  no cambian los códigos de los caracteres del BASIC (de `0x80` en adelante
  siguen siendo los bloques, los UDG y los tokens). En la impresora y en los
  ficheros, esas letras salen sin tilde.

---

## 8. Variables del sistema

### 8.1 Variables (`0x2000_0700`, *provisional*)

| Desp. | Nombre | Tipo | Descripción |
|---|---|---|---|
| `+0x00` | `FRAMES` | u32 | Contador de frames (60 Hz). No es el `FRAMES` del Spectrum (23672 en el BASIC), que cuenta cincuentavos de segundo |
| `+0x04` | `VERSION` | u32 | Versión de la especificación (`mayor << 16 \| menor`) |
| `+0x08` | `FRAME_HOOK` | u32 | Gancho de frame actual (cámbialo con `set_frame_hook`) |
| `+0x0C` | `RAMTOP` | u32 | Tope de la RAM usable; la pila del núcleo 0 empieza aquí |
| `+0x10` | `URAM_START` | u32 | Inicio de la RAM de usuario |
| `+0x14` | `URAM_END` | u32 | Fin de la RAM de usuario (exclusivo) |
| `+0x18` | `CPU_HZ` | u32 | Frecuencia de la CPU |
| `+0x1C` | `SYSFLAGS` | u32 | Bit 0 `LINK_BUSY` (ver 4.8). Bit 1 `BREAK` (tecla de interrupción pulsada) |
| `+0x20` | `LANG` | u32 | El idioma de la máquina (§7.10). Solo lectura: se cambia con `lang_set` |
| `+0x24` | `EDITOR` | u32 | 1 mientras el BASIC espera una orden (en el editor o con un informe en la pantalla); 0 mientras ejecuta algo, también un programa nativo. La escribe la ROM; la página de los juegos para la web (D85) la mira para empezarlos y volver a empezarlos |

---

## 9. Programas nativos

### 9.1 Reglas

1. Solo usan la CPU, la memoria de usuario, la VRAM, los registros ZX-V y la
   API. **No acceden a los periféricos del RP2350** (`0x4000_0000`–`0x5FFF_FFFF`,
   `0xD000_0000` y superiores). El emulador lo detecta y lo trata como error.
2. Se sincronizan con el VBLANK (`wfi`, `wait_vblank` o el gancho de frame), nunca
   contando ciclos.
3. Pueden usar toda la RAM de usuario, desactivar las interrupciones y quedarse con
   el núcleo 0 indefinidamente.
4. No escriben en la página de VRAM que se está transfiriendo (ver 4.8).

### 9.2 Formato `.ZXV`

Es un ejecutable con una cabecera de 64 bytes (little-endian), seguida de la imagen
que se copia en `load_addr`:

| Desp. | Tipo | Campo |
|---|---|---|
| `0x00` | 4 B | Firma `ZXV1` |
| `0x04` | u16 | Tamaño de la cabecera (64) |
| `0x06` | u16 | Versión del perfil de la especificación |
| `0x08` | u32 | `load_addr`: dirección de carga |
| `0x0C` | u32 | `entry`: punto de entrada |
| `0x10` | u32 | Tamaño de la imagen |
| `0x14` | u32 | Tamaño del BSS (se rellena con ceros después de la imagen) |
| `0x18` | u32 | Tope de pila (0 = seguir en la pila actual) |
| `0x1C` | u8 | Modo de vídeo inicial |
| `0x1D` | u8 | Reservado: se escribe a 0 y se ignora al leer (fue el indicador «puede volver») |
| `0x1E` | u16 | Reservado |
| `0x20` | u32 | CRC-32 de la imagen (el de zlib / IEEE 802.3) |
| `0x24` | 28 B | Nombre (ASCII, terminado en cero) |

- **Carga** (`zxv_exec`, `LOAD`). Si el nombre no existe y no tiene extensión,
  se prueba con `.zxv`. La ROM comprueba:
  - la firma, el tamaño de la cabecera y el perfil;
  - que la dirección de carga esté alineada a 4 y el punto de entrada dentro de
    la imagen;
  - que la imagen y el BSS quepan en la RAM de usuario, por debajo de la pila
    actual de la ROM con 4 KB de margen;
  - el CRC.

  Después borra el BSS, ejecuta `fence.i` y pone el modo de vídeo de la
  cabecera.
- **Pila.** Con tope 0, el programa sigue en la pila actual, por debajo de la de
  la ROM (que empieza en `RAMTOP`), como hacía `USR` en el Spectrum. Con otro
  tope (alineado a 16, por encima del BSS), usa el suyo, y la ROM recupera la
  suya al volver.
- **Ejecución.** El punto de entrada se llama como `int entry(void)`, con la ABI
  ilp32. Para retornar debe conservar `s0`–`s11`, como pide la ABI.
- **La vuelta al BASIC.** El programa ocupaba la memoria del BASIC, que
  empieza de nuevo (el programa BASIC y sus variables se pierden). El informe
  depende de cómo acabe:
  - retorno de `entry`, o `exit_to_basic`, con **0**: `0 OK, 0:1`;
  - con otro código: `W Error en el código nativo (código), 0:1`
    (`W Native code error (código)` en inglés);
  - `report_error(código, msg)`: `W` y el texto del programa, con
    ` (código)` si no es 0; por ejemplo, `W Fichero dañado (5), 0:1`. El
    texto (UTF-8, hasta 47 bytes) se copia antes de reiniciar la memoria.
    Con `msg` vacío, el texto de siempre;
  - una excepción (§7.5): `W Error en el código nativo, 0:1`;
  - Ctrl+Alt+Supr (§7.6): `L BREAK into program, 0:1`.

  El código nativo llamado con `USR` es distinto: el valor de `a0` es el
  resultado de `USR`, y el programa BASIC sigue ahí.
- **Un `.ZXV` no lanza otro.** Todos se cargan en la misma dirección, así que
  `zxv_exec` desde un programa `.ZXV` devuelve `ZXV_E_ACCESS`; solo el BASIC
  (`LOAD`) y el monitor lanzan programas.
- También se pueden cargar binarios en bruto con `LOAD "x" CODE dirección` y
  ejecutarlos con `RANDOMIZE USR dirección` (ver [basic.md](basic.md)).

### 9.3 SDK

Ver [sdk/README.md](../sdk/README.md):
- `zxv.h`: direcciones y constantes.
- `zxv_api.h`: la API desde C (incluye `<stdint.h>` y `<stddef.h>`, así que
  `NULL` y `size_t` están a mano); `zxv_gfx.h`, el motor gráfico.
- `zxv.ld` y `crt0.S`: enlazado y arranque.
- La biblioteca estándar de C (picolibc), con los ficheros y carpetas sobre la API.
- `tools/elf2zxv.py`: convierte un ELF en `.ZXV`; `tools/png2gfx.py`, los gráficos
  en PNG en sprites, tiles y mapas.
- Ejemplos en C: `hello`, `graphics`, `scroll`, `digital`, `files`, `boardtest`,
  `estela`, `route128`, `mine`, `street`, `umbra` y `keep`.
  Todos devuelven 0 al acabar bien.

Previsto: `zxv.inc` con macros para ensamblador (`ZXV_CALL`, `WAIT_VBLANK`,
`AY_WRITE`…) y un juego de ejemplo en ensamblador puro.

---

## 10. Emulador

El emulador es la implementación de referencia en software y el entorno de
desarrollo principal: un núcleo en C11 con cuatro frontales, la aplicación `zxv`
(Qt 6, Windows y Linux), `zxv-sdl`, `zxv-cli` (sin ventana, para pruebas) y la
versión web (WebAssembly). Ver [la guía del emulador](../docs/es/emulator.md).

- **Implementa esta especificación**, no el RP2350: CPU con el perfil de 2.1, mapa
  de memoria, registros, vídeo, los dos AY, beeper, entrada (teclado, ratón y
  mandos del sistema anfitrión) y una carpeta del anfitrión o una imagen como
  tarjeta SD.
- **Instantáneas**: guardan la máquina entera y la continúan exactamente.
- **Ejecuta el mismo binario de la ROM ZX-V** que el hardware. La capa de
  plataforma la implementa el propio emulador.
- **Modo estricto**: avisa de instrucciones fuera del perfil, accesos a periféricos
  del RP2350, accesos a la zona reservada, accesos desalineados y escrituras en una
  página de VRAM en transferencia. Este último aviso solo se da para código fuera
  de la ROM, porque la ROM dibuja con un solo buffer a propósito.
- **Velocidad**: por defecto, tantas instrucciones por frame como ciclos tiene la
  CPU de referencia (`--cpu-hz`), y `--turbo` va mucho más deprisa. Cuenta una
  instrucción por ciclo, así que da una idea del rendimiento real, no el exacto
  del Hazard3.
- **Depuración**: desensamblador, puntos de ruptura, vigilancias, paso a paso,
  visor de VRAM, de las colas del AY y de la tabla por línea, y **servidor GDB
  remoto** (`--gdb PUERTO`) para depurar con `riscv gdb`. Los puntos de ruptura
  no cambian la memoria del programa. Con un depurador activo, `ebreak` para la
  CPU en él (2.4). Ver [la guía del emulador](../docs/es/emulator.md).

---

## 11. Pendiente

Nada por ahora: los puntos abiertos de esta versión están cerrados (decisiones
D51 a D53, y el escalado de sprites en D55). Lo que surja, aquí.
