[English](../en/zxbasic.md) · **Español**

# Compilar BASIC con ZX Basic

[ZX Basic](https://zxbasic.readthedocs.io) es un compilador de BASIC para el
ZX Spectrum, de José Rodríguez (Boriel), muy usado para hacer juegos. Hay una
versión con un target para ZX-V que convierte un programa en BASIC en un
programa nativo (`.zxv`), como los del [SDK](sdk/README.md) en C: se carga
con `LOAD "nombre"` y va unas 90 veces más rápido que el mismo programa en el
BASIC de la máquina.

No es una versión oficial de ZX Basic: está en un fork,
[cronomantic/zxbasic](https://github.com/cronomantic/zxbasic/tree/zxv) (la
rama `zxv`), basado en la versión 1.19.0. Esta página es la introducción;
todo el detalle (cada orden con sus parámetros, sus errores y desde qué
versión está, las opciones del compilador, la memoria) está en la
[documentación del fork][fork], en inglés.

## Qué BASIC es

ZX Basic **no es el BASIC de ZX-V**. Es el BASIC del Spectrum con añadidos de
los BASIC modernos: tipos (`UByte`, `Integer`, `Long`, `Float`, `String`...),
`SUB` y `FUNCTION` con variables locales, `DO ... LOOP`, `#include`, y no
hacen falta números de línea. Un programa del Spectrum suele compilar tal
cual: `PRINT AT`, `INK`, `PLOT`, `DRAW`, `CIRCLE`, `BEEP`, `INKEY$`, `PEEK` y
`POKE` funcionan como allí, también con las direcciones de la pantalla
(16384) y de las variables del sistema.

Lo propio de ZX-V (los modos, las dos páginas, los sprites, las figuras, los
mapas de tiles, el modo 7, el mando, los AY, la música y los sonidos) está en
la biblioteca `zxv.bas`, con los mismos nombres que en el BASIC de ZX-V:

```basic
#include <zxv.bas>

GRABSTR 0, 4, "1111122112211111"
FOR x = 0 TO 240 STEP 16
    SPRITE 0, x, 80
NEXT x
BOX 4, 100, 20, 20, 20
```

Casi todo se escribe igual que en el BASIC de ZX-V. Lo que cambia son
detalles de la sintaxis de ZX Basic: `KEY` y `JOY` llevan paréntesis
(`KEY(9)`), y las órdenes de dos palabras tienen un nombre propio
(`MAPDIM` en vez de `MAP DIM`, `MUSICLOAD f$` en vez de `LOAD f$ MUSIC`...).
La [referencia de `zxv.bas`][zxvbas] tiene una entrada por orden y la
[lista de las diferencias][dif], y también [lo que el BASIC de ZX-V tiene y
ZX Basic no][nohay] (el ratón, `RASTER`, `PLAY`, el reloj, las órdenes de
carpetas y ficheros, `ON ERROR`...).

Además están las [bibliotecas del SDK](sdk/libraries.md), cada una en su
fichero (`#include <zxvinput.bas>`...) y con el mismo C que los programas en
C: el teclado y los mandos como acciones, guardar datos y récords sin
perderlos si se va la luz, fundidos de paleta, texto de cualquier tamaño,
datos comprimidos con gzip, matemáticas con enteros, moverse por un mapa sin
atravesar paredes, objetos sobre el suelo del modo 7 y modelos 3D. Un
programa que no las usa no ocupa ni un byte más. Sus órdenes, en la
[documentación del fork][libs].

`SAVE`, `LOAD` y `VERIFY` usan la tarjeta SD como el BASIC de ZX-V: sin
extensión, la pantalla va en un `.gfx` (la de ZX-V, con sus colores si no son
los de siempre) y `CODE` y `DATA` en un `.bin`, los bytes sin más; con la
extensión `.tap`, en una cinta del Spectrum. Valen las unidades `ROM:` y
`EXT:`. Los detalles, en [Files][files].

## Si vienes del Spectrum

El programa se escribe igual, pero la máquina es otra, y se nota en algunas
cosas (las explica [How it works][how]):

- **Las direcciones son de 32 bits.** `@x`, las cadenas, `PEEK`, `POKE`,
  `USR` y `LOAD ... CODE` usan direcciones `ULong`. Por debajo de 65536 la
  memoria es la del Spectrum, imitada: la pantalla en 16384, los atributos en
  22528, las variables del sistema (`FRAMES`, `CHARS`, `UDG`...) y la memoria
  libre hasta 65535, como en el BASIC de ZX-V. Por encima, las direcciones son
  de verdad: la RAM de ZX-V, su memoria de vídeo y sus registros. Lo cuenta
  [Memory below 64K][mem].
- **`Float` es otro.** Es un `double` de 8 bytes, con unos 15 dígitos, y no
  el número de 5 bytes de la ROM del Spectrum; da los mismos resultados que
  el BASIC de ZX-V. `Fixed` y los enteros son iguales que en el Spectrum.
- **No hay código Z80.** Los bloques `ASM` son de RISC-V y `USR` llama a
  código RISC-V: el código máquina del Spectrum y las bibliotecas de ZX Basic
  escritas en ensamblador Z80 no sirven. Las que trae el fork para ZX-V
  (`string.bas`, `keys.bas`, `attr.bas`...) se llaman y funcionan igual.
- **El tiempo.** `PAUSE` y `FRAMES` siguen contando cincuentavos de segundo,
  como en el Spectrum, aunque ZX-V hace 60 imágenes por segundo; `TIME` y
  `WAIT`, de `zxv.bas`, cuentan esas 60.
- **La pantalla del Spectrum** son los 256 × 192 píxeles de arriba a la
  izquierda de la página de dibujo, en cualquier modo: `PRINT`, `PLOT`,
  `DRAW`, `CIRCLE` y `CLS` no salen de ahí. Las figuras, los sprites y los
  mapas de `zxv.bas` llegan a toda la página.
- **La memoria.** Un programa tiene 244 KB (código, datos, montón y pila); el
  montón es todo lo que sobra, sin `-H`. Ver
  [How much memory a program has][cuanta].
- **Opciones del compilador.** No hay `tap`, `tzx` ni `sna`: sale un `.zxv`
  (o el ensamblador, con `-f asm`). `--org` y `-H` se aceptan pero no hacen
  nada. **Los índices de las matrices solo se comprueban con
  `--debug-array`** (da `3 Subscript wrong`); sin él, uno fuera de rango lee o
  escribe otra memoria sin dar error. La lista, en
  [Compiler options][opciones].

## Las versiones

Esta página es la de la **v1.19.0-zxv.7**, que sale junto con ZX-V 0.11.0:
`zxv.bas` con los modos, las páginas, los sprites, las figuras, los mapas de
tiles (`TILE`, `MAPDIM`, `MAP`, `CELL`...), el modo 7 (`TEXTURE`, `FLOOR`,
`FLOORMAP`, `ROTATE`, `ROTATEMAP`), las tramas (`PATTERN`, `PATTERNBITS`),
el teclado, los mandos, los AY, la música (`.zxm`, PT3, TurboSound, VGM...),
los efectos y los sonidos grabados; `SAVE`, `LOAD` y `VERIFY`; las
bibliotecas del SDK (`zxvinput.bas`... `zxv3d.bas`); y las opciones
`--loading-screen`, `--embed` y `--code-bank`. Sus programas necesitan ZX-V
0.7.0 o posterior, y los sonidos grabados (`SAMPLE`), ZX-V 0.10.0 o
posterior. Cada entrada de la [documentación del fork][fork] dice desde qué
versión existe; una versión anterior del fork no tiene lo que llegó después.

Algunas cosas necesitan además **una ROM de ZX-V de la especificación 0.2**,
la de ZX-V 0.11.0 o posterior (la 0.10.0 y las anteriores son de la
especificación 0.1): los bancos de código, `PATTERN`,
`PATTERNBITS`, `ROTATE`, `FLOORMAP`, `ROTATEMAP` y el sombreado de
`zxv3d.bas`. Con una ROM anterior el programa carga, pero un banco lo para
con «the ROM has no banks», las tramas salen de color liso y esos planos del
modo 7 no se dibujan bien. Cada entrada de la documentación del fork dice
qué ROM necesita.

## Instalarlo

**En Windows:** descarga `zxbasic-1.19.0-zxv.7-win64.zip` de la
[página de la versión](https://github.com/cronomantic/zxbasic/releases/tag/v1.19.0-zxv.7)
y descomprímelo donde quieras. Trae `zxbc.exe`, que no necesita Python.

**En Linux y macOS:** descarga el código de esa misma página, o clona la
rama `zxv` del fork
(`git clone -b zxv https://github.com/cronomantic/zxbasic`). Hace falta
Python 3.14; el compilador es `zxbc.py`.

**Además, en todos:** el compilador de C de RISC-V, el mismo que usa el SDK.
Cómo instalarlo está en la [guía del SDK](sdk/README.md#instalar-las-herramientas).

## Tu primer programa

Guarda esto como `hola.bas`:

```basic
DIM x AS FLOAT
PAPER 1: INK 7: CLS
PRINT AT 10, 6; "Hola desde ZX Basic"
FOR x = 0 TO 255
    PLOT x, 40 + 30 * SIN(x / 20)
NEXT x
```

Compílalo:

```
zxbc --arch=zxv hola.bas
```

(`zxbc.exe` en Windows, `python3 zxbc.py` con el código.) Sale `hola.zxv`:
cópialo a la tarjeta SD (en el emulador, a la carpeta que hace de tarjeta) y
cárgalo con `LOAD "hola"`. Al terminar, el informe (`0 OK` o el error) se
queda abajo hasta que pulsas una tecla, como en el Spectrum. Los informes
salen en inglés, como en ZX Basic. Compilado con `--enable-break`, Esc para
el programa y los errores dicen la línea.

**Ojo con los tipos.** Sin `DIM`, una variable toma el tipo más pequeño que
le vale: aquí, sin la primera línea, `x` sería un `UByte` (de 0 a 255). Y
entre enteros, `/` divide como enteros: `x / 20` daría 0, 0, 0... 1, 1, 1, y
la curva saldría a escalones. Y el `FOR` no acabaría nunca, porque un `UByte`
no llega a pasar de 255. Declara como `FLOAT` lo que necesite decimales, y
con un tipo más grande los contadores que lleguen al límite. Lo mismo vale
para los argumentos: si un parámetro de `zxv.bas` es un `UByte`, un 257 llega
como 1, sin error.

## Más opciones del compilador

Estas tres solo existen con `--arch=zxv`:

- **`--loading-screen titulo.png`** (desde la v1.19.0-zxv.5): una pantalla de
  carga, un PNG o un `.gfx` con una pantalla, como con `elf2zxv.py
  --screen`. La máquina la enseña mientras carga el programa. Ver
  [Loading screen][carga].
- **`--embed cancion.pt3`** (desde la v1.19.0-zxv.6; tantos como hagan falta:
  música, efectos, sonidos, `.gfx`, datos): el `.zxv` lleva esos ficheros
  dentro, como los juegos en C. `LOAD`, `VERIFY`, `MUSICLOAD`, `SFXLOAD` y
  `SAMPLELOAD` usan el de la carpeta actual si lo hay y, si no, el de dentro:
  el programa funciona solo, también desde `ROM:`, y un fichero de la tarjeta
  con el mismo nombre ocupa el lugar del de dentro. Ver
  [Files inside the program][dentro].
- **`--code-bank fase1=fase1.bas`** (desde la v1.19.0-zxv.7; necesita ZX-V
  0.11.0 o posterior): los `SUB` y `FUNCTION` de ese fichero van en un
  **banco de código**, que se carga del propio `.zxv` la primera vez que se
  llama a una de ellas, en una zona de la memoria que comparten los bancos de
  esa zona (`fase2@1=fase2.bas`, la zona 1). Así un juego puede tener el
  código de cada fase en su fichero, y solo ocupa memoria el de la que se
  juega. Sigue siendo un solo programa: las variables globales se quedan en
  la parte residente y pasan de una fase a otra. Si una función de un banco
  llama a una de otro banco de su misma zona, el programa para con «Bank a in
  use»: ponlos en zonas distintas. Ver [Code banks][bancos] y el
  [ejemplo `fases`](../../examples/zxbasic/fases/README.es.md).

## Los ejemplos

[Stardust](../../examples/zxbasic/stardust/README.es.md) es un juego pequeño
hecho con `zxv.bas`: doble buffer, sprites de cadenas, choques, teclado o
mando y sonido, a 60 imágenes por segundo.

[Petit Soccer](../../examples/zxbasic/petitsoccer/README.es.md) es uno
grande: fútbol 7 contra la máquina o entre dos jugadores, con un campo más
grande que la pantalla y scroll, los sprites de una hoja PNG, y la música y
los efectos de un tracker. Lleva dentro la música y los efectos
(`zxbc --embed`, desde la v1.19.0-zxv.6), así que también funciona desde la
unidad `ROM:`, donde viene incluido: `LOAD "ROM:petitsoccer"`.

[Fases](../../examples/zxbasic/fases/README.es.md) es el de los bancos de
código: dos fases, cada una en su fichero y en su banco. Necesita la
v1.19.0-zxv.7 del fork y ZX-V 0.11.0 o posterior.

[Nave 3D](../../examples/zxbasic/nave3d/README.es.md) es el del 3D: una nave
de caras planas, hecha en un `.obj` y pasada a BASIC con `obj2c.py --bas`,
que gira con luz entre estrellas. Necesita la v1.19.0-zxv.7 del fork; con
una ROM anterior a ZX-V 0.11.0, las caras salen sin las tramas del
sombreado.

## La licencia de tus programas

El compilador tiene la licencia AGPL 3, pero lo que va dentro de tus programas
(el runtime y las bibliotecas de ZX Basic, y las partes del SDK de ZX-V) es MIT:
tus programas pueden tener la licencia que quieras, también cerrada.

[fork]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md
[zxvbas]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#zxvbas
[dif]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#differences-with-the-zx-v-basic
[nohay]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#what-the-zx-v-basic-has-and-zx-basic-does-not
[libs]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#the-libraries-of-the-zx-v-sdk
[files]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#files
[dentro]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#files-inside-the-program
[bancos]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#code-banks
[carga]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#loading-screen
[how]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#how-it-works
[mem]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#memory-below-64k
[cuanta]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#how-much-memory-a-program-has
[opciones]: https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md#compiler-options
