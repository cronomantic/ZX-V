[English](../en/zxbasic.md) · **Español**

# Compilar BASIC con ZX Basic

[ZX Basic](https://zxbasic.readthedocs.io) es un compilador de BASIC para el
ZX Spectrum, de José Rodríguez (Boriel), muy usado para hacer juegos. Hay una
versión con un target para ZX-V que convierte un programa en BASIC en un
programa nativo (`.zxv`), como los del [SDK](sdk/README.md) en C: se carga
con `LOAD "nombre"` y va unas 90 veces más rápido que el mismo programa en el
BASIC de la máquina.

No es una versión oficial de ZX Basic: está en un fork,
[cronomantic/zxbasic](https://github.com/cronomantic/zxbasic/tree/zxv),
basado en la versión 1.19.0.

## Qué BASIC es

ZX Basic **no es el BASIC de ZX-V**. Es el BASIC del Spectrum con añadidos de
los BASIC modernos: tipos (`UByte`, `Integer`, `Long`, `Float`, `String`...),
`SUB` y `FUNCTION` con variables locales, `DO ... LOOP`, `#include`, y no
hacen falta números de línea. Un programa del Spectrum suele compilar tal
cual: `PRINT AT`, `INK`, `PLOT`, `DRAW`, `CIRCLE`, `BEEP`, `INKEY$`, `PEEK` y
`POKE` funcionan como allí, también con las direcciones de la pantalla
(16384) y de las variables del sistema.

Lo propio de ZX-V (los modos, las dos páginas, los sprites, las figuras, el
mando y los AY) está en la librería `zxv.bas`, con los mismos nombres que en
el BASIC de ZX-V:

```basic
#include <zxv.bas>

GRABSTR 0, 4, "1111122112211111"
FOR x = 0 TO 240 STEP 16
    SPRITE 0, x, 80
NEXT x
BOX 4, 100, 20, 20, 20
```

| Órdenes | |
|---|---|
| `MODE`, `PAGE`, `SCREEN`, `SCROLL`, `REMAP`, `WAIT` | Los modos, la página que se ve y en la que se dibuja (el doble buffer), el scroll, los cambios de color y el ritmo de 60 imágenes por segundo |
| `GRAB`, `GRABSTR`, `SPRITE`, `HIT` | Los sprites, con espejo, otros colores o escalados, y los choques al píxel |
| `BOX`, `DISC`, `POLY`, `CLIP`, `PIXEL` | Figuras rellenas, el recorte y el color de un píxel |
| `KEY`, `JOY`, `SOUND`, `TIME` | Las teclas pulsadas, los mandos, los registros de los AY y el contador de frames |
| `MUSICLOAD`, `MUSICPLAY`, `MUSICSTOP`, `MUSICPAUSE`, `MUSICCONTINUE`, `MUSIC` | La música de un tracker (un `.pt3` de [`zxv-tracker`](sdk/README.md#el-tracker-para-el-pc-zxv-tracker)...), de fondo mientras el programa sigue |
| `SFXLOAD`, `SFX`, `SFXSTOP`, `SFXPLAYING` | Los efectos de sonido de un banco AYFX |

Las diferencias con el BASIC de ZX-V: el sprite de una cadena es `GRABSTR`
(`GRAB n, ancho, a$` en el de ZX-V); `KEY`, `JOY`, `HIT` y `PIXEL` llevan
paréntesis (`KEY(9)`); `REMAP` y `SOUND` van de par en par; `POLY` admite
hasta 8 puntos; y la música y los efectos se cargan con `MUSICLOAD f$` y
`SFXLOAD f$` (`LOAD f$ MUSIC` y `SFX` en el de ZX-V), se tocan con `MUSICPLAY`,
`MUSICSTOP`... y la función que dice si suena un efecto es `SFXPLAYING`.

`SAVE`, `LOAD` y `VERIFY` usan la tarjeta SD como el BASIC de ZX-V: sin
extensión, un `.tap` que también carga el BASIC de la máquina. Compilado con
`--enable-break`, Esc para el programa y los errores dicen la línea. Lo demás,
y las librerías que hay, está en su
[documentación del target](https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md).

## Instalarlo

**En Windows:** descarga `zxbasic-1.19.0-zxv.2-win64.zip` de la
[página de la versión](https://github.com/cronomantic/zxbasic/releases/tag/v1.19.0-zxv.2)
y descomprímelo donde quieras. Trae `zxbc.exe`, que no necesita Python.

**En Linux y macOS:** descarga el código de esa misma página (o clona la rama
`zxv` del fork). Hace falta Python 3.14; el compilador es `zxbc.py`.

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
queda abajo hasta que pulsas una tecla, como en el Spectrum.

**Ojo con los tipos.** Sin `DIM`, una variable toma el tipo más pequeño que
le vale: aquí, sin la primera línea, `x` sería un `UByte` (de 0 a 255). Y
entre enteros, `/` divide como enteros: `x / 20` daría 0, 0, 0... 1, 1, 1, y
la curva saldría a escalones. Y el `FOR` no acabaría nunca, porque un `UByte`
no llega a pasar de 255. Declara como `FLOAT` lo que necesite decimales, y
con un tipo más grande los contadores que lleguen al límite.

## Un ejemplo

[Stardust](../../examples/zxbasic/stardust/README.es.md) es un juego pequeño
hecho con `zxv.bas`: doble buffer, sprites de cadenas, choques, teclado o
mando y sonido, a 60 imágenes por segundo.

## La licencia de tus programas

El compilador tiene la licencia AGPL 3, pero lo que va dentro de tus programas
(el runtime y las librerías de ZX Basic, y las partes del SDK de ZX-V) es MIT:
tus programas pueden tener la licencia que quieras, también cerrada.
