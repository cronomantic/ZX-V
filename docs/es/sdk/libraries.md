[English](../../en/sdk/libraries.md) · **Español**

# Las bibliotecas del SDK

Además de las rutinas de la ROM, el SDK trae unas **bibliotecas opcionales**
en `sdk/lib` para lo que casi todos los juegos acaban escribiendo: cuentas
en coma fija, mover cosas por un mapa sin atravesar paredes, el teclado y el
mando a la vez, guardar los récords, fuentes de letra, 3D... No están en la
ROM: un programa solo lleva las que usa, y lo que no usa no ocupa nada.
Todas son C normal (licencia MIT, como el SDK) y dan lo mismo en el
emulador y en la máquina.

Para usar una, incluye su cabecera y añade sus ficheros a los de tu
programa. En el Makefile de [Tu primer programa](README.md#un-makefile-para-tu-proyecto):

```make
SOURCES := juego.c $(SDK)/lib/math.c $(SDK)/lib/input.c $(SDK)/lib/save.c
```

Las cabeceras están en `sdk/include`, la misma carpeta que las del sistema,
así que no hace falta ningún `-I` más.

Los ejemplos del SDK las usan, así que sirven de muestra: el teclado y el
mando con `zxv_input` y el récord con `zxv_save` en Estela, The Mine, The
Keep, Route 128, Umbra y Street Justice (cada uno, en el `Makefile` del SDK,
con las suyas); Gone Silent, el teclado y el mando también con `zxv_input`
(sus dieciséis acciones, las que caben) y la partida guardada con
`zxv_save`; las raíces y los ángulos de `zxv_math` en Estela, Street
Justice y Gone Silent; y los conos del suelo de `graphics`, con
`zxv_m7_project`.

| Biblioteca | Cabecera | Ficheros que se añaden | Para qué |
|---|---|---|---|
| [Cuentas](#cuentas-zxv_mathh) | `zxv_math.h` | `lib/math.c` | Coma fija 16.16, seno y coseno, atan2, raíces, números al azar |
| [Objetos sobre el modo 7](#objetos-sobre-el-suelo-del-modo-7-zxv_m7h) | `zxv_m7.h` | `lib/m7.c`, `lib/math.c` | Dónde y a qué tamaño se ve un punto del suelo |
| [Mover por un mapa](#mover-por-un-mapa-de-tiles-zxv_mapmoveh) | `zxv_mapmove.h` | `lib/mapmove.c` | Chocar con las paredes, plataformas, escaleras |
| [Fundidos](#fundidos-de-paleta-zxv_fadeh) | `zxv_fade.h` | `lib/fade.c` | Llevar la paleta a negro, a blanco o a otros colores |
| [Entrada](#teclado-y-mandos-a-la-vez-zxv_inputh) | `zxv_input.h` | `lib/input.c` | Teclado y mandos como acciones, con teclas que se pueden cambiar |
| [Guardar](#guardar-datos-zxv_saveh) | `zxv_save.h` | `lib/save.c` | Récords, opciones y partidas, sin perderlos si se va la luz |
| [Descomprimir](#descomprimir-zxv_inflateh) | `zxv_inflate.h` | `lib/inflate.c` | Datos comprimidos con gzip dentro del programa |
| [Fuentes](#fuentes-de-letra-zxv_fonth) | `zxv_font.h` | `lib/font.c`, `lib/font_rom.c` | Texto de cualquier tamaño y color, proporcional |
| [3D](#3d-sencillo-zxv_3dh) | `zxv_3d.h` | `lib/3d.c`, `lib/math.c` | Modelos de caras planas con luz y sombras de trama |

Como no están en la ROM, las entradas de esta página no llevan «Desde»: la
biblioteca va dentro de tu programa. Todas funcionan con una ROM de la
especificación 0.1, salvo los tonos intermedios de la de 3D, que usan
[`zxv_gfx_pattern`](reference.md#figuras) (especificación 0.2: con una ROM
anterior, esas caras salen del color claro, sin trama).

En [ZX Basic](../zxbasic.md) están también: `zxvinput.bas`,
`zxvsave.bas`, `zxvfade.bas`, `zxvfont.bas`, `zxvinflate.bas`, `zxvmath.bas`,
`zxvmapmove.bas`, `zxvm7.bas` y `zxv3d.bas`, con el mismo C.

Y tres herramientas para el PC, en `sdk/tools`:
[`fontconv.py`](#fuentes-de-otros-sitios-fontconvpy) (fuentes de otros
formatos), [`obj2c.py`](#modelos-de-blender-obj2cpy)
(modelos 3D) y, para las hojas de letras y las texturas, el `png2gfx.py` de
siempre.

**Cómo leer las entradas.** Casi ninguna de estas funciones devuelve
códigos de error: con un valor fuera de rango, la mayoría lo recorta o no
hace nada, y cada entrada dice qué. Las que sí los devuelven (`zxv_save`,
`zxv_load` y las de descomprimir) los dan negativos, como las funciones de
ficheros de la ROM. Un puntero `NULL` donde la entrada no dice que se
admite cuelga el programa como cualquier puntero no válido.

## Cuentas: `zxv_math.h`

El procesador no tiene coma flotante: `float` funciona, pero es lento. Los
juegos hacen las cuentas en **coma fija**: un `int32_t` en el que 65536 es
1 (16.16). `zxv_fix` es ese tipo. Los **ángulos** van en vueltas de
0x10000: 0x4000 es un cuarto de vuelta, igual que en el motor gráfico.

```make
SOURCES := juego.c $(SDK)/lib/math.c
```

Las macros, `zxv_fmul` y `zxv_fdiv` están enteras en la cabecera: si solo
usas eso, no hace falta `lib/math.c`.

```c
#include "zxv_math.h"

/* Un enemigo que va hacia el jugador a 1,5 píxeles por frame. */
static zxv_fix ex, ey;                          /* su posición, en 16.16 */

void chase(zxv_fix px, zxv_fix py)
{
    int32_t a = zxv_atan2(py - ey, px - ex);
    ex += zxv_fmul(ZXV_FIX(1.5), zxv_cos(a));
    ey += zxv_fmul(ZXV_FIX(1.5), zxv_sin(a));
}
```

### `zxv_fix`

```c
typedef int32_t zxv_fix;
```

Un número en coma fija 16.16: los 16 bits altos son la parte entera (con
signo) y los 16 bajos, la fracción. 65536 (`ZXV_FIX_ONE`) es 1; 32768, un
medio.

| Rango | Paso |
|---|---|
| de −32768 a 32767,99998 | 1/65536 |

Se suma, se resta y se compara como un entero; para multiplicar y dividir
dos `zxv_fix`, [`zxv_fmul`](#zxv_fmul) y [`zxv_fdiv`](#zxv_fdiv). Por un
entero se multiplica y se divide directamente (`3 * v`, `v / 2`). Nada
comprueba que una cuenta se salga del rango: si se sale, da la vuelta (de
32767 a −32768), como cualquier `int32_t`.

**Ejemplo**

```c
#include "zxv_api.h"
#include "zxv_math.h"

static zxv_fix x = ZXV_FIX(10);                 /* 10 píxeles */
static const zxv_fix vx = ZXV_FIX(0.75);        /* 0,75 píxeles por frame */

void step(void)
{
    x += vx;                                    /* se suma como un entero */
    zxv_plot(ZXV_FIX_INT(x), 100, 7);           /* el píxel donde cae */
}
```

**Ver también:** [`ZXV_FIX`, `ZXV_FIX_ONE`](#zxv_fix-zxv_fix_one),
[`ZXV_FIX_INT`, `ZXV_FIX_ROUND`](#zxv_fix_int-zxv_fix_round).

### `ZXV_FIX`, `ZXV_FIX_ONE`

```c
#define ZXV_FIX_ONE  0x10000
#define ZXV_FIX(n)   ((zxv_fix)((n) * 65536))
```

`ZXV_FIX_ONE` es el 1 en 16.16. `ZXV_FIX(n)` pasa un número a 16.16. Con
una constante de coma flotante (`ZXV_FIX(1.5)`, `ZXV_FIX(-0.25)`), la
cuenta la hace el compilador: no queda nada de coma flotante en el programa.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | de −32768 a 32767,99998 | El número. Mejor una constante. |

**Devuelve:** `n` · 65536, con los decimales que no caben cortados hacia el
cero (`ZXV_FIX(0.00001)` es 0).

**Fuera de rango:** con `n` fuera del rango el resultado no vale (se
desborda). Con una variable entera funciona (es `n` · 65536); con una
variable `float` también, pero hace la cuenta en coma flotante, que es
lenta: no la uses así en el bucle del juego.

**Ejemplo**

```c
#include "zxv_math.h"

static const zxv_fix gravity = ZXV_FIX(0.25);
static const zxv_fix jump = -ZXV_FIX(4);

zxv_fix tile_to_fix(int tile) { return ZXV_FIX(tile * 8); }   /* con un entero */
zxv_fix half(void) { return ZXV_FIX_ONE / 2; }
zxv_fix fall(zxv_fix vy) { return vy > jump ? vy + gravity : vy; }
```

**Ver también:** [`zxv_fix`](#zxv_fix),
[`ZXV_FIX_INT`, `ZXV_FIX_ROUND`](#zxv_fix_int-zxv_fix_round).

### `ZXV_FIX_INT`, `ZXV_FIX_ROUND`

```c
#define ZXV_FIX_INT(f)    ((int32_t)(f) >> 16)
#define ZXV_FIX_ROUND(f)  ((int32_t)((f) + 0x8000) >> 16)
```

Pasan un `zxv_fix` a entero: `ZXV_FIX_INT` hacia abajo (hacia −∞, no hacia
el cero) y `ZXV_FIX_ROUND` al más cercano, con los medios hacia arriba.

| `f` | `ZXV_FIX_INT(f)` | `ZXV_FIX_ROUND(f)` |
|---|---|---|
| 2,5 | 2 | 3 |
| 2,4 | 2 | 2 |
| −1,5 | −2 | −1 |
| −0,2 | −1 | 0 |

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un `zxv_fix` | El número. |

**Devuelve:** el entero, de −32768 a 32767.

**Fuera de rango:** `ZXV_FIX_ROUND` se desborda con `f` de 32767,5 en
adelante (da un número negativo).

**Ejemplo**

```c
#include "zxv_api.h"
#include "zxv_math.h"

void show(zxv_fix x, zxv_fix y)
{
    zxv_plot(ZXV_FIX_ROUND(x), ZXV_FIX_ROUND(y), 7);
}
```

**Ver también:** [`zxv_fix`](#zxv_fix),
[`ZXV_FIX`, `ZXV_FIX_ONE`](#zxv_fix-zxv_fix_one).

### `ZXV_TURN`, `ZXV_DEG`

```c
#define ZXV_TURN    0x10000
#define ZXV_DEG(d)  ((int32_t)((d) * 65536 / 360))
```

Los ángulos de estas bibliotecas (y del motor gráfico) son enteros en
vueltas de 0x10000 (`ZXV_TURN`): 0x4000 es un cuarto de vuelta, 0x8000
media. El 0 mira hacia x creciente y 0x4000 hacia y creciente, que en la
pantalla es hacia abajo: los ángulos crecen en el sentido del reloj. Solo
cuentan los 16 bits bajos, así que un ángulo puede pasar de una vuelta o ser
negativo sin hacer nada: 0x14000 y −0xC000 son lo mismo que 0x4000. En el
[3D](#3d-sencillo-zxv_3dh), la y va hacia arriba y cada ángulo dice hacia
dónde gira.

`ZXV_DEG(d)` pasa grados a esas unidades, con los decimales cortados hacia
el cero: `ZXV_DEG(90)` es 0x4000 y `ZXV_DEG(1)`, 182 (de 182,04).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `d` | de −32767 a 32767 con enteros; cualquiera con coma flotante | Los grados. |

**Devuelve:** el ángulo, en vueltas de 0x10000.

**Fuera de rango:** con un entero de más de 32767 (o menos de −32767), la
multiplicación se desborda. Con `90.0` en vez de `90` no pasa, y el
compilador hace la cuenta igual.

**Ejemplo**

```c
#include "zxv_math.h"

int32_t turn_right(int32_t angle)
{
    return (angle + ZXV_DEG(5)) & (ZXV_TURN - 1);   /* 5 grados más, sin pasar de una vuelta */
}
```

**Ver también:** [`zxv_sin`](#zxv_sin), [`zxv_atan2`](#zxv_atan2).

### `zxv_fmul`

```c
static inline zxv_fix zxv_fmul(zxv_fix a, zxv_fix b);
```

Multiplica dos números 16.16. La cuenta se hace con 64 bits, así que no se
desborda antes de tiempo: solo si el resultado no cabe en 16.16.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `a`, `b` | cualquier `zxv_fix` | Los factores. |

**Devuelve:** `a` · `b` en 16.16, redondeado hacia abajo (hacia −∞) al
1/65536.

**Fuera de rango:** si el resultado pasa de ±32768, se quedan sus 32 bits
bajos (un número que no vale). No hay error.

**Ejemplo**

```c
#include "zxv_math.h"

zxv_fix brake(zxv_fix speed)
{
    return zxv_fmul(speed, ZXV_FIX(0.95));      /* un 5 % menos cada frame */
}
```

**Ver también:** [`zxv_fdiv`](#zxv_fdiv), [`zxv_fix`](#zxv_fix).

### `zxv_fdiv`

```c
static inline zxv_fix zxv_fdiv(zxv_fix a, zxv_fix b);
```

Divide dos números 16.16, con 64 bits en medio.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `a` | cualquier `zxv_fix` | El dividendo. |
| `b` | cualquier `zxv_fix` menos 0 | El divisor. |

**Devuelve:** `a` / `b` en 16.16, con los decimales que no caben cortados
hacia el cero.

**Fuera de rango:** `b` = 0 no está definido: compruébalo antes. Si el
cociente pasa de ±32768, se quedan sus 32 bits bajos.

**Ejemplo**

```c
#include "zxv_math.h"

/* Cuántos frames tarda en recorrer dist a la velocidad v (los dos en 16.16). */
zxv_fix frames_to(zxv_fix dist, zxv_fix v)
{
    return v ? zxv_fdiv(dist, v) : 0;
}
```

**Ver también:** [`zxv_fmul`](#zxv_fmul).

### `zxv_sin`

```c
zxv_fix zxv_sin(int32_t angle);
```

El seno de un ángulo, en 16.16. Sale de una tabla de un cuarto de vuelta
con 256 pasos e interpolación: es **exactamente el mismo seno que usa el
motor gráfico** en [`zxv_m7_camera` y `zxv_m7_affine`](reference.md#modo-7),
así que lo que pongas con él cuadra con el suelo del modo 7.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `angle` | cualquier `int32_t`; cuentan los 16 bits bajos | El ángulo, en vueltas de 0x10000 ([`ZXV_DEG`](#zxv_turn-zxv_deg)). |

**Devuelve:** de −65536 a 65536 (de −1 a 1). `zxv_sin(0x4000)` es 65536
justo. Se aparta del seno exacto 1,7/65536 como mucho.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_math.h"

/* Un enemigo que sube y baja 16 píxeles alrededor de base_y. */
int bob(int base_y, uint32_t frame)
{
    return base_y + ZXV_FIX_ROUND(16 * zxv_sin((int32_t)(frame * 512)));
}
```

**Ver también:** [`zxv_cos`](#zxv_cos), [`zxv_atan2`](#zxv_atan2).

### `zxv_cos`

```c
zxv_fix zxv_cos(int32_t angle);
```

El coseno de un ángulo, en 16.16: `zxv_sin(angle + 0x4000)`, con la misma
tabla.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `angle` | cualquier `int32_t`; cuentan los 16 bits bajos | El ángulo, en vueltas de 0x10000. |

**Devuelve:** de −65536 a 65536. Con la misma precisión que
[`zxv_sin`](#zxv_sin).

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_math.h"

/* La velocidad en x y en y de algo que va a v (16.16) en la dirección a. */
void velocity(int32_t a, zxv_fix v, zxv_fix *vx, zxv_fix *vy)
{
    *vx = zxv_fmul(v, zxv_cos(a));
    *vy = zxv_fmul(v, zxv_sin(a));
}
```

**Ver también:** [`zxv_sin`](#zxv_sin).

### `zxv_atan2`

```c
int32_t zxv_atan2(int32_t y, int32_t x);
```

El ángulo del vector (x, y): hacia dónde hay que mirar para ir de un punto
a otro. Lo calcula con CORDIC, solo con enteros.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `y`, `x` | cualquier `int32_t` | El vector, los dos en las mismas unidades (enteros o 16.16, da igual). Ojo al orden: primero `y`. |

**Devuelve:** el ángulo, de 0 a 0xFFFF: 0 hacia x creciente, 0x4000 hacia
y creciente (abajo en la pantalla), 0x8000 hacia x decreciente y 0xC000
hacia y decreciente. `zxv_atan2(0, 0)` es 0.

Se aparta del ángulo exacto 0,52/65536 de vuelta como mucho: da el valor
exacto redondeado o, en unos pocos casos (unos 3 de cada 1.000), uno más o uno
menos. Vale igual con vectores muy pequeños o muy grandes.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_math.h"

/* Gira la torreta hacia el jugador, 2 grados por frame como mucho. */
int32_t aim(int32_t turret, zxv_fix tx, zxv_fix ty, zxv_fix px, zxv_fix py)
{
    int32_t want = zxv_atan2(py - ty, px - tx);
    int16_t diff = (int16_t)(want - turret);    /* de -0x8000 a 0x7FFF: por el lado corto */
    if (diff > ZXV_DEG(2))
        diff = ZXV_DEG(2);
    if (diff < -ZXV_DEG(2))
        diff = -ZXV_DEG(2);
    return (turret + diff) & 0xFFFF;
}
```

**Ver también:** [`zxv_sin`](#zxv_sin), [`zxv_hypot`](#zxv_hypot).

### `zxv_isqrt`

```c
uint32_t zxv_isqrt(uint32_t v);
```

La raíz cuadrada entera de `v`, hacia abajo: `zxv_isqrt(10)` es 3.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `v` | de 0 a 4.294.967.295 | El número. |

**Devuelve:** la raíz, de 0 a 65535.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_math.h"

/* El radio de un círculo de área 'area' píxeles (más o menos). */
uint32_t radius(uint32_t area) { return zxv_isqrt(area * 100 / 314); }
```

**Ver también:** [`zxv_isqrt64`](#zxv_isqrt64), [`zxv_fsqrt`](#zxv_fsqrt),
[`zxv_hypot`](#zxv_hypot).

### `zxv_isqrt64`

```c
uint32_t zxv_isqrt64(uint64_t v);
```

Como [`zxv_isqrt`](#zxv_isqrt), con un número de 64 bits: para la suma de
dos cuadrados grandes, por ejemplo.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `v` | cualquier `uint64_t` | El número. |

**Devuelve:** la raíz hacia abajo, de 0 a 4.294.967.295.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_math.h"

/* La distancia entre dos puntos en 16.16 (sin desbordarse). */
zxv_fix dist(zxv_fix ax, zxv_fix ay, zxv_fix bx, zxv_fix by)
{
    int64_t dx = (int64_t)bx - ax, dy = (int64_t)by - ay;
    return (zxv_fix)zxv_isqrt64((uint64_t)(dx * dx) + (uint64_t)(dy * dy));
}
```

**Ver también:** [`zxv_hypot`](#zxv_hypot), que hace esta misma cuenta.

### `zxv_fsqrt`

```c
zxv_fix zxv_fsqrt(zxv_fix v);
```

La raíz cuadrada de un número 16.16, en 16.16.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `v` | de 0 a 32767,99998 | El número. |

**Devuelve:** la raíz, hacia abajo al 1/65536: `zxv_fsqrt(ZXV_FIX(2))` es
92681 (1,41421). Como mucho, 181,02.

**Fuera de rango:** con `v` de 0 o negativo, da 0.

**Ejemplo**

```c
#include "zxv_math.h"

/* La velocidad para llegar a una altura h (16.16) con la gravedad g: sqrt(2·g·h). */
zxv_fix jump_speed(zxv_fix g, zxv_fix h) { return zxv_fsqrt(2 * zxv_fmul(g, h)); }
```

**Ver también:** [`zxv_isqrt`](#zxv_isqrt).

### `zxv_hypot`

```c
uint32_t zxv_hypot(int32_t x, int32_t y);
```

La longitud del vector (x, y), √(x² + y²), sin desbordarse aunque `x` e `y`
sean grandes (las cuentas van en 64 bits).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x`, `y` | cualquier `int32_t` | El vector, en las unidades que sean (enteros o 16.16). |

**Devuelve:** la longitud hacia abajo, en las mismas unidades que `x` e
`y`, de 0 a 3.037.000.499. Con dos `zxv_fix`, la longitud es también 16.16
(pásala a `zxv_fix` si es menor de 32768).

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_math.h"

/* 1 si el disparo (en 16.16) está a menos de 6 píxeles del enemigo. */
int hit(zxv_fix sx, zxv_fix sy, zxv_fix ex, zxv_fix ey)
{
    return zxv_hypot(ex - sx, ey - sy) < (uint32_t)ZXV_FIX(6);
}
```

**Ver también:** [`zxv_atan2`](#zxv_atan2), [`zxv_isqrt64`](#zxv_isqrt64).

### `zxv_srand`

```c
void zxv_srand(uint32_t seed);
```

Pone la semilla de [`zxv_rand`](#zxv_rand) y
[`zxv_rand_range`](#zxv_rand_range). La misma semilla da siempre la misma
serie de números: sirve para repetir una partida o un nivel generado.

**Sin llamarla, la serie empieza siempre igual** (el estado inicial es fijo):
el juego sería el mismo cada vez. Una buena semilla es
[`zxv_time_us()`](reference.md#zxv_time_us) cuando el jugador pulsa una
tecla para empezar.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `seed` | cualquier `uint32_t` | La semilla. El 0 vale como el 1. |

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_api.h"
#include "zxv_math.h"

void wait_start(void)
{
    while (zxv_key_get() < 0)
        zxv_wait_vblank();
    zxv_srand(zxv_time_us());                   /* cada partida, distinta */
}
```

**Ver también:** [`zxv_rand`](#zxv_rand), [`zxv_rand_next`](#zxv_rand_next).

### `zxv_rand`

```c
uint32_t zxv_rand(void);
```

El siguiente número al azar (xorshift32: rápido, de 32 bits, y no se repite
hasta pasados 4.294.967.295).

**Devuelve:** de 1 a 4.294.967.295 (nunca 0).

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_math.h"

int coin(void) { return (int)(zxv_rand() >> 31); }            /* 0 o 1 */
int chance(int percent) { return zxv_rand_range(1, 100) <= percent; }
```

**Ver también:** [`zxv_rand_range`](#zxv_rand_range), [`zxv_srand`](#zxv_srand).

### `zxv_rand_range`

```c
int zxv_rand_range(int lo, int hi);
```

Un número al azar entre `lo` y `hi`, los dos incluidos, todos con la misma
probabilidad. Usa el mismo generador que [`zxv_rand`](#zxv_rand).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `lo`, `hi` | cualquier `int` | Los extremos. Si `hi` es menor que `lo`, se dan la vuelta. |

**Devuelve:** el número, de `lo` a `hi`. Con `INT32_MIN` e `INT32_MAX`,
cualquier `int`.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_math.h"

int roll_dice(void) { return zxv_rand_range(1, 6); }
```

**Ver también:** [`zxv_rand`](#zxv_rand), [`zxv_srand`](#zxv_srand).

### `zxv_rand_next`

```c
uint32_t zxv_rand_next(uint32_t *state);
```

Otro generador, con el estado en tu variable en vez del de
[`zxv_rand`](#zxv_rand). Sirve para tener series independientes: una para
generar el nivel (siempre la misma con la misma semilla) y otra para el
resto del juego, que no la descoloca.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `state` | puntero a un `uint32_t` | El estado: ponle la semilla al principio. Si vale 0, se toma como 1. |

**Devuelve:** el siguiente número, de 1 a 4.294.967.295, que queda también
en `*state`.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_math.h"

/* El nivel n se genera siempre igual. */
void make_level(uint8_t *cells, int n_cells, uint32_t level)
{
    uint32_t st = 12345 + level;
    for (int i = 0; i < n_cells; i++)
        cells[i] = (uint8_t)(zxv_rand_next(&st) % 4);
}
```

**Ver también:** [`zxv_rand`](#zxv_rand).

## Objetos sobre el suelo del modo 7: `zxv_m7.h`

Con la misma cámara con la que [`zxv_m7_camera`](reference.md#modo-7)
calcula el suelo (ver [Modo 7](graphics.md#modo-7)), `zxv_m7_project` dice
dónde se ve un punto de ese suelo (o a cierta altura sobre él) y a qué
tamaño: para los coches, los árboles y los conos de un juego de carreras,
dibujados con [`zxv_spr_draw_scaled`](reference.md#sprites). Y
`zxv_m7_floor_at` hace lo contrario: qué punto del suelo hay en un píxel de
la pantalla.

```make
SOURCES := juego.c $(SDK)/lib/m7.c $(SDK)/lib/math.c
```

La cabecera incluye `zxv_gfx.h` (por `zxv_m7cam`) y `zxv_math.h`. Las
cuentas son las del motor gráfico, así que todo cuadra al píxel con lo que
dibuja. Recuerda: en `zxv_m7cam`, todo va en 16.16 (también `focal`) salvo
`horizon`, que es la línea entera de la pantalla.

Para que unos objetos tapen a otros, **dibújalos de lejos a cerca**, por su
`depth`.

### `zxv_m7pos`

```c
typedef struct {
    int32_t x, y;
    zxv_fix scale;
    zxv_fix depth;
} zxv_m7pos;
```

Dónde se ve un punto, lo que rellena [`zxv_m7_project`](#zxv_m7_project).

| Campo | Valores | Qué es |
|---|---|---|
| `x` | píxeles (enteros) | La columna del destino. |
| `y` | píxeles (enteros) | La línea del destino. Para un punto del suelo, la línea donde se apoya: dibuja el objeto con su base ahí. |
| `scale` | 16.16, mayor que 0 | Los píxeles que mide un texel a esa distancia: el tamaño del objeto en texels por `scale` da su tamaño en píxeles. |
| `depth` | 16.16, de 1 texel a 32767,99998 | Lo lejos que está, hacia delante (la distancia perpendicular a la pantalla). Para ordenar. |

**Ver también:** [`zxv_m7_project`](#zxv_m7_project).

### `zxv_m7_project`

```c
int zxv_m7_project(const zxv_m7cam *cam, int width, zxv_fix px, zxv_fix py, zxv_fix h, zxv_m7pos *p);
```

Dónde se ve el punto (`px`, `py`) del suelo, a la altura `h` sobre él, con
la cámara `cam`, y a qué escala. Redondea al píxel más cercano.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `cam` | la cámara de `zxv_m7_camera` | La misma con la que se calcula el suelo. `focal` tiene que ser mayor que 0. |
| `width` | píxeles | El ancho del destino en el que se dibuja el suelo (256 en el modo 0): la cámara está en su columna `width / 2`. |
| `px`, `py` | texels, 16.16 | El punto del suelo. |
| `h` | texels, 16.16 | La altura sobre el suelo: 0, apoyado; para lo que salta o vuela, más. A la altura de la cámara (`cam->height`) cae en el horizonte. |
| `p` | puntero a un `zxv_m7pos` | Donde se deja el resultado. |

**Devuelve:** 1 si el punto está delante de la cámara, y entonces rellena
`*p`; 0 si no, y `*p` no cambia. Un punto con 1 puede caer fuera de la
pantalla (de lado, o por debajo): el sprite se recorta solo al dibujarlo.

Da 0 cuando:

| Caso | Por qué |
|---|---|
| El punto está detrás de la cámara o a menos de un texel por delante | Allí no se puede ver (o saldría enorme). |
| La posición pasa de ±2³⁰ píxeles o `scale`, de 32768 | Demasiado cerca para dibujarlo. |
| `cam` o `p` son `NULL`, o `cam->focal` es 0 o negativo | No hay cámara. |

**Ejemplo**

```c
#include "zxv_m7.h"

/* Un árbol de 32 texels de alto y 16 de ancho, apoyado en (tx, ty). */
void draw_tree(const zxv_m7cam *cam, const void *tree_spr, zxv_fix tx, zxv_fix ty)
{
    zxv_m7pos p;
    if (zxv_m7_project(cam, 256, tx, ty, 0, &p)) {
        int h = (int)((32 * (int64_t)p.scale) >> 16);
        int w = (int)((16 * (int64_t)p.scale) >> 16);
        if (w > 0 && h > 0)
            zxv_spr_draw_scaled(tree_spr, p.x - w / 2, p.y - h, w, h, 0, 0);
    }
}
```

**Ver también:** [`zxv_m7pos`](#zxv_m7pos), [`zxv_m7_floor_at`](#zxv_m7_floor_at),
[`zxv_m7_camera`](reference.md#modo-7).

### `zxv_m7_floor_at`

```c
int zxv_m7_floor_at(const zxv_m7cam *cam, int width, int sx, int sy, zxv_fix *px, zxv_fix *py);
```

Lo contrario de [`zxv_m7_project`](#zxv_m7_project): el punto del suelo que
se ve en el píxel (`sx`, `sy`) del destino. Es exactamente el texel que
dibuja `zxv_m7_draw` con las líneas de `zxv_m7_camera`. Sirve para el
ratón, o para saber qué pisa un coche.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `cam` | la cámara de `zxv_m7_camera` | La misma con la que se calcula el suelo. |
| `width` | píxeles | El ancho del destino, como en `zxv_m7_project`. |
| `sx` | píxeles | La columna. No se comprueba: fuera del destino da el punto que se vería si siguiera. |
| `sy` | píxeles, por debajo de `cam->horizon` | La línea. |
| `px`, `py` | punteros, o `NULL` | Donde se deja el punto, en texels 16.16. El que sea `NULL` no se rellena. |

**Devuelve:** 1 y el punto; o 0 si `sy` está en el horizonte o por encima
(allí no hay suelo), si `cam` es `NULL` o si `cam->focal` es 0 o
negativo. Con 0, `*px` y `*py` no cambian.

El punto no se ajusta al tamaño de la textura: si se repite, el texel es
`(px >> 16) & (lado - 1)`.

**Ejemplo**

```c
#include "zxv_m7.h"

/* El texel de la pista que hay bajo el coche (dibujado en la columna 128, línea 170). */
int under_car(const zxv_m7cam *cam, const uint8_t *track, int log2size)
{
    zxv_fix u, v;
    if (!zxv_m7_floor_at(cam, 256, 128, 170, &u, &v))
        return -1;
    int mask = (1 << log2size) - 1;
    return track[(((v >> 16) & mask) << log2size) + ((u >> 16) & mask)];
}
```

**Ver también:** [`zxv_m7_project`](#zxv_m7_project).

## Mover por un mapa de tiles: `zxv_mapmove.h`

El núcleo de un juego de plataformas: mover un rectángulo por un
[mapa de tiles](graphics.md#mapas-de-tiles) parando contra las paredes, sin
atravesarlas aunque vaya muy deprisa, y saber si está en el suelo. El juego
dice de qué **tipo** es cada tile (vacío, sólido, plataforma o uno suyo:
escalera, pinchos, agua, monedas).

```make
SOURCES := juego.c $(SDK)/lib/mapmove.c
```

La cabecera incluye `zxv_gfx.h` (por `zxv_tilemap`) y `zxv_math.h` (por
`zxv_fix`), pero `lib/math.c` no hace falta.

**Las medidas.** Las posiciones van en píxeles del mapa, en 16.16: (0, 0)
es la esquina de arriba a la izquierda de la celda (0, 0). Un rectángulo
con la esquina en (x, y) y w × h píxeles ocupa de x a x + w (sin incluir) y
de y a y + h. `w` y `h` son enteros; menos de 1 cuenta como 1. De cada
celda solo cuenta el número de tile (los 12 bits bajos): los espejos y la
prioridad dan igual.

**Mapa no válido.** Si `ms` o `ms->map` o sus `cells` son `NULL`, si el
mapa mide 0 de ancho o de alto, o si sus tiles no son de 8 ni de 16, las
funciones no hacen nada: `zxv_map_move` no mueve y da 0,
`zxv_map_on_ground` y `zxv_map_find` dan 0 y `zxv_map_kind`,
`ZXV_TILE_EMPTY`.

```c
#include "zxv_input.h"
#include "zxv_mapmove.h"

enum { LADDER = 3, COIN = 4 };
extern const zxv_tilemap level;
static const uint8_t kinds[] = { ZXV_TILE_EMPTY, ZXV_TILE_SOLID, ZXV_TILE_PLATFORM, LADDER, COIN };
static zxv_mapsolid ms = { &level, kinds, sizeof kinds, ZXV_TILE_SOLID };   /* fuera del mapa, pared */

static zxv_fix px, py, vy;                      /* posición y velocidad vertical, 16.16 */

void take_coin(int cx, int cy);                 /* del juego: quita la moneda del mapa */

void player_step(const zxv_input *in)
{
    zxv_fix vx = 0;
    if (in->held & ZXV_IN_LEFT)
        vx = -ZXV_FIX(1.5);
    if (in->held & ZXV_IN_RIGHT)
        vx = ZXV_FIX(1.5);
    unsigned through = (in->held & ZXV_IN_DOWN) ? ZXV_MOVE_THROUGH : 0;
    if (zxv_map_on_ground(&ms, px, py, 12, 16, through) && (in->pressed & ZXV_IN_FIRE))
        vy = -ZXV_FIX(4);                       /* salto */
    vy += ZXV_FIX(0.25);                        /* gravedad */
    unsigned hit = zxv_map_move(&ms, &px, &py, 12, 16, vx, vy, through);
    if (hit & (ZXV_HIT_DOWN | ZXV_HIT_UP))
        vy = 0;
    int cx, cy;
    if (zxv_map_find(&ms, px, py, 12, 16, COIN, &cx, &cy))
        take_coin(cx, cy);
}
```

No hay rampas ni tiles en diagonal.

### `ZXV_TILE_EMPTY`, `ZXV_TILE_SOLID`, `ZXV_TILE_PLATFORM`

Los tipos de tile que entiende la biblioteca. El juego le da uno a cada
número de tile con la tabla `kinds` de [`zxv_mapsolid`](#zxv_mapsolid).

| Tipo | Valor | Qué hace |
|---|---|---|
| `ZXV_TILE_EMPTY` | 0 | Nada: se atraviesa. |
| `ZXV_TILE_SOLID` | 1 | Para por los cuatro lados. |
| `ZXV_TILE_PLATFORM` | 2 | Solo para al caer desde encima: se atraviesa saltando por debajo y de lado, y bajando con `ZXV_MOVE_THROUGH`. |
| del 3 al 255 | | Del juego (escaleras, pinchos, agua, monedas): no paran nada, pero [`zxv_map_find`](#zxv_map_find) y [`zxv_map_kind`](#zxv_map_kind) los encuentran. |

**Ver también:** [`zxv_mapsolid`](#zxv_mapsolid).

### `zxv_mapsolid`

```c
typedef struct {
    const zxv_tilemap *map;
    const uint8_t *kinds;
    uint16_t nkinds;
    uint8_t outside;
} zxv_mapsolid;
```

El mapa con los tipos de sus tiles: lo que reciben todas las funciones.
Solo se lee, así que puede ser `const` y compartirse entre todos los
personajes.

| Campo | Valores | Qué es |
|---|---|---|
| `map` | un `zxv_tilemap` | El mapa ([Tipos](reference.md#tipos)): sus celdas, su tamaño, tiles de 8 o de 16 y `ZXV_MAP_WRAP` si se repite. Es el mismo que se dibuja con `zxv_map_draw`. |
| `kinds` | tabla de `uint8_t`, o `NULL` | El tipo de cada tile, por su número (de 0 a 4095). `NULL`: el tile 0 está vacío y todos los demás son sólidos. |
| `nkinds` | de 0 a 65535 | Cuántos tipos hay en `kinds`. Los tiles con un número mayor o igual están vacíos. |
| `outside` | un tipo, de 0 a 255 | El tipo de lo que hay fuera de un mapa que no se repite: `ZXV_TILE_SOLID` para que los bordes sean paredes, `ZXV_TILE_EMPTY` para que se pueda salir. En un mapa que se repite no cuenta. |

**Ejemplo**

```c
#include "zxv_mapmove.h"

extern const zxv_tilemap cave;
/* Sin tabla: el tile 0 vacío y los demás sólidos; los bordes, abiertos. */
const zxv_mapsolid cave_solid = { &cave, 0, 0, ZXV_TILE_EMPTY };
```

**Ver también:** [`ZXV_TILE_EMPTY`, `ZXV_TILE_SOLID`, `ZXV_TILE_PLATFORM`](#zxv_tile_empty-zxv_tile_solid-zxv_tile_platform).

### `ZXV_HIT_LEFT`, `ZXV_HIT_RIGHT`, `ZXV_HIT_UP`, `ZXV_HIT_DOWN`

Los bits de lo que devuelve [`zxv_map_move`](#zxv_map_move): los lados del
rectángulo que han chocado.

| Constante | Valor | Qué ha pasado |
|---|---|---|
| `ZXV_HIT_LEFT` | 0x01 | Ha chocado por la izquierda (iba hacia la izquierda). |
| `ZXV_HIT_RIGHT` | 0x02 | Ha chocado por la derecha. |
| `ZXV_HIT_UP` | 0x04 | Ha chocado con la cabeza. |
| `ZXV_HIT_DOWN` | 0x08 | Ha chocado con los pies: está en el suelo. |

**Ver también:** [`zxv_map_move`](#zxv_map_move).

### `ZXV_MOVE_THROUGH`

```c
#define ZXV_MOVE_THROUGH  0x01u
```

La opción de [`zxv_map_move`](#zxv_map_move) y
[`zxv_map_on_ground`](#zxv_map_on_ground) para atravesar las plataformas al
bajar: para bajarse de una (abajo + saltar, en muchos juegos). Con ella, las
plataformas no paran nada.

**Ver también:** [`ZXV_TILE_EMPTY`, `ZXV_TILE_SOLID`, `ZXV_TILE_PLATFORM`](#zxv_tile_empty-zxv_tile_solid-zxv_tile_platform).

### `zxv_map_move`

```c
unsigned zxv_map_move(const zxv_mapsolid *ms, zxv_fix *x, zxv_fix *y, int w, int h,
                      zxv_fix dx, zxv_fix dy, unsigned flags);
```

Mueve el rectángulo primero `dx` en horizontal y luego `dy` en vertical
(desde la x nueva), parando cada eje en la primera celda que lo bloquea: se
queda tocándola, sin solaparse. Mira todas las celdas por las que pasa, así
que **no atraviesa ninguna pared por rápido que vaya** (pero cuanto más
lejos va, más celdas mira).

- Las plataformas solo paran al bajar, y solo las filas que estaban enteras
  por debajo de los pies antes de moverse: «venir de encima».
- Si el rectángulo ya estaba metido en algo sólido, las celdas que ocupa no
  lo paran: sale por donde se mueve.
- La velocidad es del juego: al chocar, ponla a 0 tú (`vy = 0` con
  `ZXV_HIT_DOWN` o `ZXV_HIT_UP`).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ms` | un `zxv_mapsolid` | El mapa. |
| `x`, `y` | punteros a la esquina de arriba a la izquierda, 16.16 | Dónde está; se cambian por dónde queda. |
| `w`, `h` | píxeles (enteros); menos de 1 es 1 | El tamaño del rectángulo. |
| `dx`, `dy` | 16.16, cualquier valor | Lo que se mueve en este paso. |
| `flags` | 0 o `ZXV_MOVE_THROUGH` | `ZXV_MOVE_THROUGH`: atravesar las plataformas. |

**Devuelve:** los `ZXV_HIT_` de los lados que han chocado, juntos; 0 si no
ha chocado con nada. Al chocar, la coordenada de ese eje queda justo en el
borde de la celda (sin fracción).

**Errores:** ninguno. Con un [mapa no válido](#mover-por-un-mapa-de-tiles-zxv_mapmoveh)
o con `x` o `y` `NULL`, no mueve nada y da 0.

**Ejemplo**

```c
#include "zxv_mapmove.h"

/* Una pelota que rebota en las paredes. */
void ball_step(const zxv_mapsolid *ms, zxv_fix *x, zxv_fix *y, zxv_fix *vx, zxv_fix *vy)
{
    unsigned hit = zxv_map_move(ms, x, y, 6, 6, *vx, *vy, 0);
    if (hit & (ZXV_HIT_LEFT | ZXV_HIT_RIGHT))
        *vx = -*vx;
    if (hit & (ZXV_HIT_UP | ZXV_HIT_DOWN))
        *vy = -*vy;
}
```

**Ver también:** [`zxv_map_on_ground`](#zxv_map_on_ground),
[`ZXV_HIT_LEFT`...](#zxv_hit_left-zxv_hit_right-zxv_hit_up-zxv_hit_down),
[`ZXV_MOVE_THROUGH`](#zxv_move_through).

### `zxv_map_on_ground`

```c
int zxv_map_on_ground(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y, int w, int h, unsigned flags);
```

Dice si el rectángulo está apoyado, sin moverlo: si sus pies están justo
en el borde de arriba de una celda que para al bajar (sólida, o una
plataforma sin `ZXV_MOVE_THROUGH`). Es como queda después de que
[`zxv_map_move`](#zxv_map_move) lo pare al caer; con los pies a media celda
(cayendo, o en una escalera) da 0.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ms` | un `zxv_mapsolid` | El mapa. |
| `x`, `y` | 16.16 | La esquina de arriba a la izquierda. |
| `w`, `h` | píxeles; menos de 1 es 1 | El tamaño. |
| `flags` | 0 o `ZXV_MOVE_THROUGH` | Con `ZXV_MOVE_THROUGH`, las plataformas no cuentan. |

**Devuelve:** 1 si está apoyado; 0 si no (o con un mapa no válido).

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_mapmove.h"

/* Solo se puede saltar desde el suelo. */
zxv_fix try_jump(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y, zxv_fix vy)
{
    return zxv_map_on_ground(ms, x, y, 12, 16, 0) ? -ZXV_FIX(4) : vy;
}
```

**Ver también:** [`zxv_map_move`](#zxv_map_move).

### `zxv_map_find`

```c
int zxv_map_find(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y, int w, int h, int kind, int *cx, int *cy);
```

Busca una celda del tipo `kind` entre las que toca el rectángulo: para las
escaleras, el agua, los pinchos o las monedas. Las mira de arriba abajo y,
en cada fila, de izquierda a derecha, y se queda con la primera.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ms` | un `zxv_mapsolid` | El mapa. |
| `x`, `y` | 16.16 | La esquina de arriba a la izquierda. |
| `w`, `h` | píxeles; menos de 1 es 1 | El tamaño. |
| `kind` | de 0 a 255 | El tipo que se busca (también los de la biblioteca: `ZXV_TILE_SOLID`...). |
| `cx`, `cy` | punteros, o `NULL` | Donde se deja la celda encontrada (columna y fila). El que sea `NULL` no se rellena. |

**Devuelve:** 1 si la encuentra, con su celda en `*cx`, `*cy`; 0 si no
(o con un mapa no válido), y entonces no cambian. En un mapa que se repite,
la celda está siempre dentro del mapa (de 0 a ancho − 1). En uno que no se
repite, si `kind` es el tipo `outside`, la celda puede estar fuera
(negativa, o más allá del borde).

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_mapmove.h"

enum { SPIKES = 5 };

int touching_spikes(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y)
{
    return zxv_map_find(ms, x, y, 12, 16, SPIKES, 0, 0);
}
```

**Ver también:** [`zxv_map_kind`](#zxv_map_kind).

### `zxv_map_kind`

```c
int zxv_map_kind(const zxv_mapsolid *ms, int px, int py);
```

El tipo de la celda que hay en un píxel del mapa.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ms` | un `zxv_mapsolid` | El mapa. |
| `px`, `py` | píxeles **enteros** (no 16.16) | El punto. Puede estar fuera del mapa. |

**Devuelve:** el tipo, de 0 a 255. Fuera de un mapa que no se repite, el
tipo `outside`; en uno que se repite, el de la celda que toca. Con un mapa
no válido, `ZXV_TILE_EMPTY`.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_mapmove.h"

enum { WATER = 6 };

/* ¿Tiene la cabeza bajo el agua? (x, y en 16.16; 12 de ancho) */
int underwater(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y)
{
    return zxv_map_kind(ms, ZXV_FIX_INT(x) + 6, ZXV_FIX_INT(y)) == WATER;
}
```

**Ver también:** [`zxv_map_find`](#zxv_map_find).

## Fundidos de paleta: `zxv_fade.h`

Llevan los colores de [las paletas](graphics.md#el-remapeo-de-colores) a
otros en unos frames: a negro al acabar una fase, de negro a sus colores al
empezar la siguiente, un destello blanco al recibir un golpe. Los píxeles
no se tocan: solo cambia cómo se ven, así que no cuesta nada dibujar.
Cambia los colores de las 4 tablas de paleta (16 colores RGB565 cada una,
ver [La paleta](reference.md#la-paleta)); las tablas de remapeo no.

```make
SOURCES := juego.c $(SDK)/lib/fade.c
```

```c
#include "zxv.h"                                /* ZXV_RGB */
#include "zxv_api.h"
#include "zxv_fade.h"

void load_level(int n);                         /* del juego */
void game_frame(void);

void next_level(int n)
{
    zxv_fade_to_colour(ZXV_RGB(0, 0, 0), 30, ZXV_FADE_ALL);  /* a negro en medio segundo */
    zxv_fade_wait();
    load_level(n);
    zxv_fade_to(NULL, 30, ZXV_FADE_ALL);                     /* a los colores de siempre */
    while (zxv_fade_busy()) {
        zxv_fade_step();                                     /* un paso por frame... */
        game_frame();                                        /* ...mientras el juego sigue */
        zxv_wait_vblank();
    }
}
```

- **Un fundido a la vez:** empezar otro sustituye al que hubiera, y sale de
  los colores por donde iba. Uno y el de vuelta pasan por los mismos
  colores.
- La máquina tiene un solo [gancho de frame](README.md#el-gancho-de-frame) y
  la música suele usarlo, así que la biblioteca no lo toma: llama a
  `zxv_fade_step()` una vez por frame, en tu bucle o en tu gancho.
- `zxv_fade.h` no incluye `zxv.h`: para `ZXV_RGB` (un color de r, g, b de
  0 a 255), inclúyelo tú.

### `ZXV_FADE_ALL`

```c
#define ZXV_FADE_ALL  0x0Fu
```

Las cuatro tablas de paleta. El argumento `tables` de los fundidos es una
máscara: el bit `t` es la tabla `t` (de 0 a 3). Con solo algunas, se funde
el juego y no el marcador, por ejemplo (si el marcador usa otra tabla, ver
[La tabla por línea](graphics.md#la-tabla-por-línea)).

| Valor | Tablas |
|---|---|
| `0x01` | Solo la 0. |
| `0x0E` | La 1, la 2 y la 3. |
| `ZXV_FADE_ALL` (0x0F) | Todas. |

**Ver también:** [`zxv_fade_to`](#zxv_fade_to).

### `zxv_fade_to`

```c
void zxv_fade_to(const uint16_t to[16], int frames, unsigned tables);
```

Empieza a llevar las tablas de `tables` a los colores `to`, desde los que
tienen ahora, en `frames` pasos de [`zxv_fade_step`](#zxv_fade_step). Las
tablas elegidas acaban todas con los mismos 16 colores. Hasta el primer
paso no cambia nada (salvo con `frames` 0).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `to` | 16 colores RGB565, o `NULL` | Adónde se va. Se copian: puede ser una variable local. `NULL`: los 16 colores de siempre (`ZXV_STD_RGB565`), no los de un `.gfx` que hayas cargado; para esos, pásale su paleta. |
| `frames` | 1 o más; 0 o menos, ya | Cuántos pasos dura. Con 0 o menos, pone los colores enseguida y no queda fundido. |
| `tables` | máscara de 4 bits ([`ZXV_FADE_ALL`](#zxv_fade_all)) | Qué tablas cambia. Los bits de más arriba no cuentan; con 0 no cambia nada, pero el fundido «dura» igual `frames` pasos. |

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_api.h"
#include "zxv_fade.h"

/* De negro a la paleta del nivel (16 colores, por ejemplo los de su .gfx). */
void fade_in(const uint16_t level_pal[16])
{
    zxv_fade_to(level_pal, 25, ZXV_FADE_ALL);
    zxv_fade_wait();
}
```

**Ver también:** [`zxv_fade_to_colour`](#zxv_fade_to_colour),
[`zxv_fade_step`](#zxv_fade_step), [`zxv_rgb_mix`](#zxv_rgb_mix).

### `zxv_fade_to_colour`

```c
void zxv_fade_to_colour(uint16_t rgb, int frames, unsigned tables);
```

Como [`zxv_fade_to`](#zxv_fade_to), con los 16 colores iguales: a negro, a
blanco, a rojo...

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `rgb` | un color RGB565 | Adónde van todos: `ZXV_RGB(0, 0, 0)` (o 0), negro; `ZXV_RGB(255, 255, 255)`, blanco. |
| `frames` | 1 o más; 0 o menos, ya | Cuántos pasos dura. |
| `tables` | máscara de 4 bits | Qué tablas cambia. |

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv.h"
#include "zxv_fade.h"

/* Un destello blanco al recibir un golpe: de blanco a lo de siempre en 8 frames. */
void flash(void)
{
    zxv_fade_to_colour(ZXV_RGB(255, 255, 255), 0, ZXV_FADE_ALL);   /* blanco, ya */
    zxv_fade_to(0, 8, ZXV_FADE_ALL);                              /* y de vuelta (0: NULL) */
}
```

**Ver también:** [`zxv_fade_to`](#zxv_fade_to).

### `zxv_fade_step`

```c
void zxv_fade_step(void);
```

Da un paso del fundido: pone en las tablas los colores del paso siguiente.
El último paso deja exactamente los colores de destino. Si no hay fundido,
no hace nada.

Llámala una vez por frame: en tu bucle o en tu
[gancho de frame](README.md#el-gancho-de-frame) (no espera nada, así que
puede ir ahí).

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_fade.h"

void music_tick(void);                          /* del juego */

/* El gancho de frame del juego: la música y el fundido. */
void frame_hook(void)
{
    music_tick();
    zxv_fade_step();
}
```

**Ver también:** [`zxv_fade_busy`](#zxv_fade_busy), [`zxv_fade_wait`](#zxv_fade_wait).

### `zxv_fade_busy`

```c
int zxv_fade_busy(void);
```

**Devuelve:** 1 mientras quedan pasos del fundido; 0 cuando ha acabado o
no hay ninguno.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_fade.h"

/* No se puede pausar mientras se funde. */
int can_pause(void) { return !zxv_fade_busy(); }
```

**Ver también:** [`zxv_fade_step`](#zxv_fade_step).

### `zxv_fade_wait`

```c
void zxv_fade_wait(void);
```

Espera a que acabe el fundido: en cada VBLANK
([`zxv_wait_vblank`](reference.md#zxv_wait_vblank)) da un paso. Sin
fundido, vuelve enseguida. Mientras, el juego está parado (el gancho de
frame, con la música, sigue). **No la llames desde un gancho de frame:**
espera al VBLANK.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_fade.h"

void fade_out(void)
{
    zxv_fade_to_colour(0, 30, ZXV_FADE_ALL);    /* 0 es el negro */
    zxv_fade_wait();
}
```

**Ver también:** [`zxv_fade_step`](#zxv_fade_step).

### `zxv_rgb_mix`

```c
uint16_t zxv_rgb_mix(uint16_t a, uint16_t b, int k, int n);
```

Un color a medio camino entre `a` y `b`: el que usan los fundidos, para tus
propios efectos. Cada componente (rojo, verde y azul, por separado) es
a + (b − a) · k / n, al más cercano, con los medios hacia arriba.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `a`, `b` | colores RGB565 | Los dos extremos. |
| `k` | de 0 a `n` | El paso. |
| `n` | 1 o más | Los pasos de `a` a `b`. |

**Devuelve:** el color RGB565. Con `k` 0 o menos, `a`; con `k` igual o
mayor que `n`, o con `n` 0 o negativo, `b`.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv.h"
#include "zxv_fade.h"

/* El color de una barra de vida: de verde (llena) a rojo (vacía). */
uint16_t life_colour(int life, int max)
{
    return zxv_rgb_mix(ZXV_RGB(215, 0, 0), ZXV_RGB(0, 215, 0), life, max);
}
```

**Ver también:** [`zxv_fade_to`](#zxv_fade_to).

## Teclado y mandos a la vez: `zxv_input.h`

El juego piensa en **acciones** (izquierda, fuego, pausa: hasta 16, un bit
cada una) y un **mapa** dice qué teclas (hasta 3) y qué botones del mando
hacen cada una. Una vez por frame, `zxv_input_update` mira el teclado y el
mando y dice qué acciones están pulsadas, cuáles se acaban de pulsar o de
soltar y cuánto llevan. Con dos mapas, dos jugadores.

```make
SOURCES := juego.c $(SDK)/lib/input.c
```

Lee directamente el mapa de [teclas mantenidas](README.md#las-teclas-mantenidas)
(por su código HID) y los [mandos](README.md#los-mandos), sin llamar a la
ROM.

```c
#include "zxv_api.h"
#include "zxv_input.h"

void game(void)
{
    zxv_input in;
    int x = 128;
    zxv_input_init(&in, NULL);                  /* el mapa de siempre */
    for (;;) {
        zxv_wait_vblank();
        zxv_input_update(&in);                  /* una vez por frame */
        if (in.held & ZXV_IN_LEFT)
            x--;
        if (in.held & ZXV_IN_RIGHT)
            x++;
        if (in.pressed & ZXV_IN_ESC)
            break;
        zxv_plot(x, 100, 7);
    }
}
```

**Redefinir las teclas:** [`zxv_input_new_key`](#zxv_input_new_key) da la
tecla que se acaba de pulsar y [`zxv_input_key_name`](#zxv_input_key_name)
su nombre. El mapa son datos: se copia, se cambia y se guarda con
[`zxv_save`](#zxv_save).

### `ZXV_IN_ACTIONS`, `ZXV_IN_KEYS`

```c
#define ZXV_IN_ACTIONS  16
#define ZXV_IN_KEYS     3
```

`ZXV_IN_ACTIONS`: cuántas acciones hay (16; la acción `a` es el bit
`1 << a`). `ZXV_IN_KEYS`: cuántas teclas puede tener cada una en un mapa
(3).

**Ver también:** [`zxv_input_map`](#zxv_input_map).

### `ZXV_IN_RIGHT`, `ZXV_IN_LEFT`, `ZXV_IN_DOWN`, `ZXV_IN_UP`, `ZXV_IN_FIRE`, `ZXV_IN_FIRE2`, `ZXV_IN_FIRE3`, `ZXV_IN_START`, `ZXV_IN_SELECT`, `ZXV_IN_ESC`, `ZXV_IN_FIRE4`, `ZXV_IN_L`, `ZXV_IN_R`

Las acciones del [mapa de siempre](#zxv_input_default). Las nueve primeras
son los mismos bits que los botones del mando (`ZXV_JOY_...`); luego vienen
Esc y el fuego 4, L y R (estos tres, solo con el mando). Los bits que no usa
(de 0x2000 a 0x8000) son para el juego, con un mapa suyo; y un juego puede
dar a estos el sentido que quiera.

| Acción | Valor | Teclas | Botón del mando |
|---|---|---|---|
| `ZXV_IN_RIGHT` | 0x0001 | Cursor derecha, P | `ZXV_JOY_RIGHT` |
| `ZXV_IN_LEFT` | 0x0002 | Cursor izquierda, O | `ZXV_JOY_LEFT` |
| `ZXV_IN_DOWN` | 0x0004 | Cursor abajo, A | `ZXV_JOY_DOWN` |
| `ZXV_IN_UP` | 0x0008 | Cursor arriba, Q | `ZXV_JOY_UP` |
| `ZXV_IN_FIRE` | 0x0010 | Espacio, M | `ZXV_JOY_FIRE1` |
| `ZXV_IN_FIRE2` | 0x0020 | N, Z | `ZXV_JOY_FIRE2` |
| `ZXV_IN_FIRE3` | 0x0040 | B, X | `ZXV_JOY_FIRE3` |
| `ZXV_IN_START` | 0x0080 | Intro | `ZXV_JOY_START` |
| `ZXV_IN_SELECT` | 0x0100 | Tab | `ZXV_JOY_SELECT` |
| `ZXV_IN_ESC` | 0x0200 | Esc | ninguno |
| `ZXV_IN_FIRE4` | 0x0400 | ninguna | `ZXV_JOY_FIRE4` |
| `ZXV_IN_L` | 0x0800 | ninguna | `ZXV_JOY_L` |
| `ZXV_IN_R` | 0x1000 | ninguna | `ZXV_JOY_R` |

Las letras son las de su sitio en un teclado de EE. UU. (en uno español
son las mismas).

**Ver también:** [`zxv_input_default`](#zxv_input_default).

### `ZXV_IN_KEEP_QUEUE`

```c
#define ZXV_IN_KEEP_QUEUE  0x01u
```

Una opción del mapa (campo `flags`). Sin ella,
[`zxv_input_update`](#zxv_input_update) vacía cada frame la cola de teclas
de [`zxv_key_get`](reference.md#zxv_key_get), para que lo tecleado durante
la partida no le llegue al BASIC al volver. Con ella, la cola se queda: para
un juego que también lee teclas con `zxv_key_get` (escribir un nombre, por
ejemplo).

**Ejemplo**

```c
#include "zxv_input.h"

static zxv_input_map typing_map;

/* El mapa de siempre, sin vaciar la cola de teclas. */
void make_typing_map(void)
{
    typing_map = zxv_input_default;
    typing_map.flags |= ZXV_IN_KEEP_QUEUE;
}
```

**Ver también:** [`zxv_input_map`](#zxv_input_map).

### `zxv_input_map`

```c
typedef struct {
    uint8_t keys[ZXV_IN_ACTIONS][ZXV_IN_KEYS];
    uint16_t joy[ZXV_IN_ACTIONS];
    int8_t pad;
    uint8_t flags;
} zxv_input_map;
```

Qué teclas y qué botones hacen cada acción. Son datos (82 bytes): se
pueden copiar, cambiar y guardar.

| Campo | Valores | Qué es |
|---|---|---|
| `keys[a][k]` | códigos HID de 1 a 255; 0, ninguna | Las teclas (hasta 3) de la acción `a`. Cualquiera de ellas la hace. |
| `joy[a]` | bits `ZXV_JOY_...`; 0, ninguno | Los botones del mando que hacen la acción `a` (cualquiera de ellos). |
| `pad` | 0 o 1; otro (−1), ninguno | El mando que se mira. Si no está conectado, solo cuentan las teclas. |
| `flags` | 0 o `ZXV_IN_KEEP_QUEUE` | Opciones. |

**Ejemplo**

```c
#include "zxv.h"                                /* ZXV_JOY_... */
#include "zxv_input.h"

/* El segundo jugador: W A S D, fuego con Mayús izquierda, y el mando 1. */
const zxv_input_map player2 = {
    .keys = {
        [0] = { 0x07 },                         /* ZXV_IN_RIGHT: D */
        [1] = { 0x04 },                         /* LEFT: A */
        [2] = { 0x16 },                         /* DOWN: S */
        [3] = { 0x1A },                         /* UP: W */
        [4] = { 0xE1 },                         /* FIRE: Mayús izquierda */
    },
    .joy = { ZXV_JOY_RIGHT, ZXV_JOY_LEFT, ZXV_JOY_DOWN, ZXV_JOY_UP, ZXV_JOY_FIRE1 },
    .pad = 1,
};
```

`zxv_input.h` no incluye `zxv.h`: para los `ZXV_JOY_...`, inclúyelo tú.

**Ver también:** [`zxv_input_default`](#zxv_input_default),
[`zxv_input_init`](#zxv_input_init).

### `zxv_input_default`

```c
extern const zxv_input_map zxv_input_default;
```

El mapa de siempre: cursores o Q A O P, fuego con Espacio o M, fuego 2 con
N o Z, fuego 3 con B o X, Intro para START, Tab para SELECT, Esc, el fuego
4, L y R solo con el mando, y el mando 0. La tabla de [las acciones](#zxv_in_right-zxv_in_left-zxv_in_down-zxv_in_up-zxv_in_fire-zxv_in_fire2-zxv_in_fire3-zxv_in_start-zxv_in_select-zxv_in_esc-zxv_in_fire4-zxv_in_l-zxv_in_r)
dice qué hace cada una. Es el que se usa con `zxv_input_init(&in, NULL)`.

Es `const`: para cambiarlo, cópialo.

**Ejemplo**

```c
#include "zxv_input.h"

static zxv_input_map my_map;
static zxv_input in;

/* Como el de siempre, pero con fuego también en Ctrl izquierdo. */
void setup_input(void)
{
    my_map = zxv_input_default;
    my_map.keys[4][2] = 0xE0;                   /* FIRE es la acción 4; su tercera tecla */
    zxv_input_init(&in, &my_map);
}
```

**Ver también:** [`zxv_input_map`](#zxv_input_map).

### `zxv_input`

```c
typedef struct {
    const zxv_input_map *map;
    uint16_t held;
    uint16_t pressed;
    uint16_t released;
    uint16_t frames[ZXV_IN_ACTIONS];
} zxv_input;
```

El estado de un jugador. Lo pone a punto [`zxv_input_init`](#zxv_input_init)
y lo actualiza [`zxv_input_update`](#zxv_input_update); el juego solo lo
lee.

| Campo | Valores | Qué es |
|---|---|---|
| `map` | un `zxv_input_map` | El mapa que usa. |
| `held` | bits de acciones | Las que están pulsadas ahora. |
| `pressed` | bits de acciones | Las que se han pulsado en este frame (no lo estaban en el anterior). |
| `released` | bits de acciones | Las que se han soltado en este frame. |
| `frames[a]` | de 0 a 65535 | Cuántos frames lleva pulsada la acción `a` (la del bit `1 << a`): 1 en el frame en que se pulsa, 0 si no lo está. No pasa de 65535. |

**Ver también:** [`zxv_input_update`](#zxv_input_update),
[`zxv_input_repeat`](#zxv_input_repeat).

### `zxv_input_init`

```c
void zxv_input_init(zxv_input *in, const zxv_input_map *map);
```

Prepara el estado de un jugador con un mapa, sin nada pulsado. Se puede
volver a llamar para cambiar de mapa (también se olvida lo pulsado).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `in` | puntero a un `zxv_input` | El estado. |
| `map` | un mapa, o `NULL` | El mapa. `NULL`: [`zxv_input_default`](#zxv_input_default). **No se copia:** tiene que seguir existiendo mientras se use (global o `static`, no una variable local que se acaba). |

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_input.h"

extern const zxv_input_map player2;             /* el del ejemplo de zxv_input_map */
static zxv_input p1, p2;

void setup_players(void)
{
    zxv_input_init(&p1, 0);                     /* 0 (NULL): el mapa de siempre */
    zxv_input_init(&p2, &player2);
}
```

**Ver también:** [`zxv_input_update`](#zxv_input_update).

### `zxv_input_update`

```c
void zxv_input_update(zxv_input *in);
```

Lee el teclado y el mando y actualiza `held`, `pressed`, `released` y
`frames`. Una acción está pulsada si lo está cualquiera de sus teclas o,
con el mando conectado, cualquiera de sus botones.

Llámala **una vez por frame** y por jugador (antes de mirar el estado):
`pressed` y `released` comparan con la llamada anterior, así que con dos
llamadas en un frame la segunda no vería nada recién pulsado.

Después, salvo con [`ZXV_IN_KEEP_QUEUE`](#zxv_in_keep_queue) en el mapa,
**vacía la cola de teclas**: `zxv_key_get` no dará nada de lo tecleado
mientras tanto. Con dos jugadores, basta con que uno de los mapas no lleve
la opción para que se vacíe.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `in` | un `zxv_input` preparado con `zxv_input_init` | El estado. |

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_api.h"
#include "zxv_input.h"

static zxv_input p1, p2;

void read_players(void)
{
    zxv_wait_vblank();
    zxv_input_update(&p1);
    zxv_input_update(&p2);
}
```

**Ver también:** [`zxv_input`](#zxv_input), [`zxv_input_repeat`](#zxv_input_repeat).

### `zxv_input_repeat`

```c
int zxv_input_repeat(const zxv_input *in, uint16_t actions, int delay, int rate);
```

Para moverse por un menú: da 1 al pulsar y, si se deja pulsado, otra vez
cada `rate` frames después de `delay`. Con `delay` 10 y `rate` 3, en los
frames 1, 13, 16, 19... de la pulsación. No guarda nada: mira `frames`, así
que llámala justo después de `zxv_input_update`, una vez por frame.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `in` | un `zxv_input` | El estado. |
| `actions` | bits de acciones | Las que se miran (basta una). |
| `delay` | frames, 0 o más | Cuánto esperar antes de repetir. |
| `rate` | frames; menos de 1 es 1 | Cada cuánto se repite. |

**Devuelve:** 1 si alguna de `actions` se acaba de pulsar o le toca
repetir en este frame; 0 si no.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_input.h"

/* La opción elegida de un menú de n, con abajo y arriba. */
int menu_move(const zxv_input *in, int sel, int n)
{
    if (zxv_input_repeat(in, ZXV_IN_DOWN, 10, 3))
        sel = (sel + 1) % n;
    if (zxv_input_repeat(in, ZXV_IN_UP, 10, 3))
        sel = (sel + n - 1) % n;
    return sel;
}
```

**Ver también:** [`zxv_input`](#zxv_input).

### `zxv_input_key_down`

```c
int zxv_input_key_down(int hid);
```

Dice si una tecla está pulsada ahora, por su código USB HID (ver
[Las teclas mantenidas](README.md#las-teclas-mantenidas)). No necesita
ningún `zxv_input`: sirve para una tecla suelta que no es una acción
(F1 para la ayuda, por ejemplo).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `hid` | de 0 a 255 | El código HID de la tecla (de 0xE0 a 0xE7, las de Ctrl, Mayús, Alt y Windows). |

**Devuelve:** 1 si está pulsada; 0 si no, o si `hid` está fuera de 0–255.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_input.h"

int help_key(void) { return zxv_input_key_down(0x3A); }      /* F1 */
```

**Ver también:** [`zxv_input_new_key`](#zxv_input_new_key).

### `zxv_input_new_key`

```c
int zxv_input_new_key(void);
```

La tecla que se ha pulsado desde la llamada anterior: para redefinir las
teclas. Llámala una vez por frame mientras esperas.

La primera vez que se llama (en todo el programa) solo toma nota de lo que
ya está pulsado y da −1. Una tecla que ya estaba pulsada no cuenta hasta
que se suelta y se vuelve a pulsar. El estado es uno solo para todo el
programa.

**Devuelve:** el código HID de la tecla recién pulsada (si son varias, la
de código más bajo), o −1 si no hay ninguna.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_api.h"
#include "zxv_input.h"

/* Espera una tecla nueva y se la pone a la acción a del mapa. */
void redefine(zxv_input_map *m, int a)
{
    int k;
    zxv_input_new_key();                        /* olvida lo que ya estaba pulsado */
    while ((k = zxv_input_new_key()) < 0)
        zxv_wait_vblank();
    m->keys[a][0] = (uint8_t)k;
    m->keys[a][1] = m->keys[a][2] = 0;
    zxv_print_str(zxv_input_key_name(k));
}
```

**Ver también:** [`zxv_input_key_name`](#zxv_input_key_name),
[`zxv_input_key_down`](#zxv_input_key_down).

### `zxv_input_key_name`

```c
const char *zxv_input_key_name(int hid);
```

El nombre de una tecla para enseñarlo, en UTF-8 y en el idioma de la
máquina ([`zxv_lang_get`](reference.md#zxv_lang_get-zxv_lang_set)): en
castellano si es `ZXV_LANG_ES` y en inglés con cualquier otro.

| Códigos | Nombre |
|---|---|
| 0x04–0x1D | La letra, de «A» a «Z» (por su sitio en un teclado de EE. UU.) |
| 0x1E–0x27 | La cifra, de «1» a «9» y «0» |
| 0x3A–0x45 | De «F1» a «F12» |
| 0x59–0x62 | «Numérico 1»... «Numérico 0» («Keypad 1»...) |
| 0x28, 0x29, 0x2A, 0x2B, 0x2C | «Intro», «Esc», «Retroceso», «Tab», «Espacio» |
| 0x39, 0x46, 0x47, 0x48 | «Bloq Mayús», «Impr Pant», «Bloq Despl», «Pausa» |
| 0x49–0x4E | «Insertar», «Inicio», «Re Pág», «Supr», «Fin», «Av Pág» |
| 0x4F–0x52 | «Cursor derecha», «Cursor izquierda», «Cursor abajo», «Cursor arriba» |
| 0x58 | «Intro numérico» |
| 0xE0–0xE7 | «Ctrl izquierdo», «Mayús izquierda», «Alt», «Windows izquierda», «Ctrl derecho», «Mayús derecha», «Alt Gr», «Windows derecha» |
| las demás | «Tecla» y su código en hexadecimal: «Tecla 33» («Key 33») |

Las teclas de símbolos (la Ñ, la coma, el guion...) salen por su código
porque cambian de sitio según el teclado: la Ñ de un teclado español es
«Tecla 33».

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `hid` | de 0 a 255 | El código HID. Fuera de ese rango, cuentan sus 8 bits bajos (−1 sale como «Tecla FF»). |

**Devuelve:** el nombre. Las letras, las cifras, las F, las del teclado
numérico y las de «Tecla XX» van en un buffer que se sobrescribe en la
siguiente llamada: si quieres dos nombres a la vez, copia el primero.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_api.h"
#include "zxv_input.h"

/* «Fuego: Espacio» */
void show_fire_key(const zxv_input_map *m)
{
    zxv_print_str("Fuego: ");
    zxv_print_str(zxv_input_key_name(m->keys[4][0]));
}
```

**Ver también:** [`zxv_input_new_key`](#zxv_input_new_key).

## Guardar datos: `zxv_save.h`

Guarda una estructura de tu juego (los récords, las opciones, una partida)
y la lee de vuelta, comprobando que es la tuya y que no está dañada.

```make
SOURCES := juego.c $(SDK)/lib/save.c
```

```c
#include "zxv_save.h"

static struct {
    uint8_t music, level;
    zxv_score top[5];
} data;

void load_data(void)
{
    if (zxv_load("juego.sav", 1, &data, sizeof data) < 0) {    /* no hay, o no vale */
        data.music = 1;
        data.level = 0;
    }
}

void game_over(uint32_t score, const char *name)
{
    if (zxv_score_add(data.top, 5, score, name) >= 0)          /* ha entrado en la tabla */
        zxv_save("juego.sav", 1, &data, sizeof data);
}
```

- El fichero lleva la **versión** de tus datos (el 1 de arriba): cuando
  cambies la estructura, súbela, y los ficheros de antes darán
  `ZXV_SAVE_E_VERSION` en vez de leerse mal. Si `zxv_load` falla, **tus
  datos no cambian** (con más de 256 bytes, salvo un caso: ver
  [`zxv_load`](#zxv_load)): rellénalos antes con los valores de siempre o
  después de que falle.
- **Si se va la luz**: se escribe en `juego.sav~` y luego se cambia por el
  de verdad: si la máquina se apaga a medias, se queda el de antes. Si se
  apaga justo entre borrar el viejo y renombrar, `zxv_load` lee el `~` y el
  siguiente `zxv_save` lo pone en su sitio.
- **Sin tarjeta**, en `ROM:` o sin servidor en `EXT:`, devuelve el error del
  sistema y el juego sigue: solo que no se guarda. No hace falta avisar de
  nada si no quieres.
- **Dónde se guarda.** El nombre es una [ruta](reference.md#ficheros-y-carpetas):
  sin unidad ni carpeta, el fichero va en la **carpeta actual** (la de `CD`
  o [`zxv_dir_change`](reference.md#carpetas)), no en la del programa:
  cargar un `.zxv` no cambia la carpeta actual. Si el juego se carga con
  `LOAD "SD:/juegos/nave"` desde otra carpeta, guarda en esa otra carpeta;
  y si la carpeta actual está en `ROM:`, no puede guardar (`ZXV_E_ACCESS`).
  Para que vaya siempre al mismo sitio, pon la ruta entera
  (`"SD:/juegos/nave.sav"`).

**Errores comunes.** `zxv_save` y `zxv_load` usan las funciones de
[ficheros](reference.md#ficheros-y-carpetas), y además de los suyos pueden
dar sus errores:

| Error | Cuándo |
|---|---|
| `ZXV_E_NOCARD` | La ruta va a `SD:` y no hay tarjeta. |
| `ZXV_E_OFFLINE` | La ruta va a `EXT:` y no hay servidor. |
| `ZXV_E_IO` | La tarjeta falló al leer o escribir. |
| `ZXV_E_NOTDIR`, `ZXV_E_ISDIR`, `ZXV_E_MFILE` | Como en [`zxv_file_open`](reference.md#zxv_file_open). |

### `ZXV_SAVE_E_DAMAGED`, `ZXV_SAVE_E_VERSION`

Los errores propios de [`zxv_load`](#zxv_load) (los demás son los
`ZXV_E_` de los ficheros, en `zxv.h`).

| Error | Valor | Cuándo |
|---|---|---|
| `ZXV_SAVE_E_DAMAGED` | −64 | El fichero no es de `zxv_save` (no empieza por `ZXVS`), está cortado, le sobran bytes o su CRC no cuadra. |
| `ZXV_SAVE_E_VERSION` | −65 | Es de `zxv_save`, pero de otra versión o de otra longitud: se guardó con otra estructura. |

**Ver también:** [`zxv_load`](#zxv_load).

### `zxv_save`

```c
int zxv_save(const char *name, uint16_t version, const void *data, uint32_t len);
```

Guarda `len` bytes de `data` en el fichero `name`, con una cabecera (la
versión, la longitud y un CRC) para que [`zxv_load`](#zxv_load) los
compruebe. Lo hace sin perder el de antes si se va la luz:

1. Si solo queda `name~` (de una vez que se apagó entre los pasos 3 y 4),
   lo renombra a `name`.
2. Escribe todo en `name~`.
3. Borra `name`.
4. Renombra `name~` a `name`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | ruta de hasta 254 caracteres; el nombre del fichero, hasta 62 | Dónde se guarda (se usa también `name~`, por eso uno menos de lo normal). Ver [Dónde se guarda](#guardar-datos-zxv_saveh). |
| `version` | de 0 a 65535 | La versión de tus datos. |
| `data` | puntero a los datos | Lo que se guarda. Puede ser `NULL` con `len` 0. |
| `len` | bytes | Cuánto se guarda. |

**Devuelve:** `ZXV_OK` (0), o un error (negativo).

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NAME` | `name` es `NULL` o está vacío, acaba en `/` o en `:`, la ruta pasa de 254 caracteres o el nombre de 62; o `data` es `NULL` con `len` mayor que 0. No se toca nada. |
| `ZXV_E_ACCESS` | La ruta va a `ROM:`; `name` está abierto; en `EXT:`, el servidor no deja sustituir el fichero. |
| `ZXV_E_NOENT` | No existe una carpeta de la ruta. |
| `ZXV_E_NOSPC` | No cabe en la tarjeta. Se borra `name~` y `name` sigue como estaba. |
| `ZXV_E_FAULT` | `data` no está entero en la memoria del programa ni en la ROM. Se borra `name~`. |
| (comunes) | Ver [Errores comunes](#guardar-datos-zxv_saveh). |

Si falla al escribir `name~`, se borra y `name` sigue como estaba. Si falla
al renombrar (paso 4), `name~` se queda con los datos nuevos y
[`zxv_load`](#zxv_load) los encuentra.

**Ejemplo**

```c
#include "zxv_save.h"

static struct { uint8_t music, sfx, lang; } options;

int save_options(void)
{
    return zxv_save("SD:/opciones.cfg", 2, &options, sizeof options);
}
```

**Ver también:** [`zxv_load`](#zxv_load), [El fichero](#el-fichero-de-zxv_save).

### `zxv_load`

```c
int zxv_load(const char *name, uint16_t version, void *data, uint32_t len);
```

Lee en `data` lo que guardó [`zxv_save`](#zxv_save) con la misma versión y
la misma longitud. Antes de tocar `data` lo lee entero y comprueba el CRC:
**si falla, `data` no cambia**. Con hasta 256 bytes lo lee en la pila y lo
copia; con más no hay sitio para una copia, así que lo lee otra vez en
`data` y comprueba el CRC otra vez (si esa lectura falla, lo intenta de
nuevo). Solo si la tarjeta falla justo entre las dos lecturas (se saca en
ese momento), devuelve el error con `data` a medias: con cualquier error,
pon los valores de siempre, como en el ejemplo.

Si `name` no está o no vale (por cualquier error salvo `ZXV_E_NOCARD` y
`ZXV_E_OFFLINE`), prueba con `name~`, por si la máquina se apagó a medio
guardar.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | ruta, como en `zxv_save` | El fichero. |
| `version` | de 0 a 65535 | La versión que se espera. |
| `data` | puntero | Donde se leen los datos. Puede ser `NULL` con `len` 0. |
| `len` | bytes | Cuánto se espera: tiene que ser lo mismo que se guardó. |

**Devuelve:** `ZXV_OK` (0) si ha leído los datos (de `name` o de `name~`),
o un error (negativo). Si los dos fallan, el error es el de `name`.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOENT` | No hay fichero (la primera vez que se juega), o no existe una carpeta de la ruta. |
| `ZXV_SAVE_E_DAMAGED` | El fichero no es de `zxv_save`, está dañado o cortado. |
| `ZXV_SAVE_E_VERSION` | Es de otra versión o de otra longitud. |
| `ZXV_E_NAME` | Como en `zxv_save`. |
| (comunes) | Ver [Errores comunes](#guardar-datos-zxv_saveh). |

**Ejemplo**

```c
#include "zxv_save.h"

static struct { uint8_t music, sfx, lang; } options;

void load_options(void)
{
    if (zxv_load("SD:/opciones.cfg", 2, &options, sizeof options) < 0) {
        options.music = 1;                      /* los de siempre */
        options.sfx = 1;
        options.lang = 0;
    }
}
```

**Ver también:** [`zxv_save`](#zxv_save),
[`ZXV_SAVE_E_DAMAGED`, `ZXV_SAVE_E_VERSION`](#zxv_save_e_damaged-zxv_save_e_version).

### `zxv_crc32`

```c
uint32_t zxv_crc32(const void *data, uint32_t n);
```

El CRC-32 de `n` bytes: el de zip, PNG y gzip (polinomio 0xEDB88320,
empezando en 0xFFFFFFFF y con la vuelta al final). Es el que llevan los
ficheros de `zxv_save`; sirve también para comprobar tus propios datos.
Va bit a bit, sin tabla: no ocupa memoria, pero es más lento que con una.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `data` | puntero | Los datos. |
| `n` | bytes | Cuántos. Con 0, el CRC es 0. |

**Devuelve:** el CRC. El de los nueve bytes `"123456789"` es 0xCBF43926.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_save.h"

/* 1 si el nivel cargado es el que se esperaba. */
int level_ok(const uint8_t *lvl, uint32_t size, uint32_t expected)
{
    return zxv_crc32(lvl, size) == expected;
}
```

**Ver también:** [El fichero](#el-fichero-de-zxv_save).

### `zxv_score`

```c
typedef struct {
    uint32_t score;
    char name[8];
} zxv_score;
```

Una línea de una tabla de récords (12 bytes). Una tabla es un array de
ellas, de mayor a menor, que se guarda con lo demás.

| Campo | Valores | Qué es |
|---|---|---|
| `score` | de 0 a 4.294.967.295 | Los puntos. |
| `name` | hasta 7 bytes y un 0 | El nombre. En UTF-8, una letra con tilde ocupa 2 bytes. |

Una tabla que empieza a ceros (como la de una variable global) vale: son
líneas de 0 puntos sin nombre.

**Ver también:** [`zxv_score_add`](#zxv_score_add).

### `zxv_score_add`

```c
int zxv_score_add(zxv_score *table, int n, uint32_t score, const char *name);
```

Mete una puntuación en su sitio de la tabla (de mayor a menor) y quita la
última. Con un empate, la nueva va detrás de las que ya estaban.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `table` | una tabla de `n` líneas, de mayor a menor | La tabla. |
| `n` | 1 o más | Cuántas líneas tiene. |
| `score` | cualquier `uint32_t` | Los puntos. |
| `name` | texto, o `NULL` | El nombre. Se quedan los 7 primeros bytes (puede cortar una letra con tilde por la mitad); `NULL`, sin nombre. |

**Devuelve:** la posición en la que ha entrado (0 es la mejor), o −1 si no
entra: si no supera a la última (o la iguala), o si `table` es `NULL` o
`n` es 0 o negativo. Con −1 la tabla no cambia.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_save.h"

static zxv_score top[10];

/* La posición, para resaltarla en la tabla; -1 si no ha entrado. */
int new_score(uint32_t score, const char *initials)
{
    int at = zxv_score_add(top, 10, score, initials);
    if (at >= 0)
        zxv_save("records.sav", 1, top, sizeof top);
    return at;
}
```

**Ver también:** [`zxv_score`](#zxv_score), [`zxv_save`](#zxv_save).

### El fichero de `zxv_save`

Por si quieres leerlo o escribirlo desde el PC. Todo va con el byte bajo
primero:

| Posición | Tamaño | Qué es |
|---|---|---|
| 0 | 4 | `ZXVS` |
| 4 | 2 | La versión de los datos. |
| 6 | 2 | 0 |
| 8 | 4 | La longitud de los datos. |
| 12 | 4 | El CRC-32 de los datos ([`zxv_crc32`](#zxv_crc32)). |
| 16 | la longitud | Los datos, tal cual estaban en la memoria. |

Los datos son la estructura como la deja el compilador (con los huecos de
alineación): un fichero guardado por un programa se lee bien con el mismo
programa, o con otro con la misma estructura.

## Descomprimir: `zxv_inflate.h`

Para llevar dentro del programa (o en un fichero) gráficos, mapas o textos
comprimidos, y sacarlos cuando hacen falta. Vale lo de cualquier programa
que comprime en deflate: un `.gz` de gzip, 7-Zip o Python.

```make
SOURCES := juego.c $(SDK)/lib/inflate.c
```

```c
#include "zxv_inflate.h"

extern const unsigned char level1_gz[];         /* de «xxd -i nivel1.bin.gz» */
extern const unsigned int level1_gz_len;
static uint8_t level[16384];

int unpack_level(void)
{
    return zxv_gunzip(level1_gz, level1_gz_len, level, sizeof level);   /* < 0: dañado, o no cabe */
}
```

- Necesita unos 2 KB de pila y nada más: descomprime directamente en el
  destino, sin otro buffer.
- **Es rápido:** en el emulador, 64 KB salen en 11 ms (tiles repetidos),
  23 ms (texto) o 54 ms (texels al azar): de 1,2 a 6 MB por segundo.
- **Comprimir se hace en el PC:** `gzip -9 nivel1.bin`. Para meterlo en el
  programa, `xxd -i nivel1.bin.gz` (un array de C) o lo lees de la tarjeta
  con [`zxv_file_load`](reference.md#zxv_file_load).

### `ZXV_INFLATE_E_DATA`, `ZXV_INFLATE_E_SPACE`

Los errores de las funciones de descomprimir.

| Error | Valor | Cuándo |
|---|---|---|
| `ZXV_INFLATE_E_DATA` | −66 | Los datos no son deflate (o gzip), están dañados o cortados, o el CRC o la longitud de un `.gz` no cuadran. También con punteros `NULL` o un `out_max` de más de 0x7FFFFFFF. |
| `ZXV_INFLATE_E_SPACE` | −67 | Lo descomprimido no cabe en `out_max` bytes. |

Con cualquiera de los dos, `out` puede tener escrito parte de lo
descomprimido, pero nunca más allá de `out_max`.

**Ver también:** [`zxv_inflate`](#zxv_inflate), [`zxv_gunzip`](#zxv_gunzip).

### `zxv_inflate`

```c
int zxv_inflate(const void *in, uint32_t in_len, void *out, uint32_t out_max);
```

Descomprime deflate sin más (RFC 1951), sin cabecera ni comprobación: el de
`zlib.compressobj(9, zlib.DEFLATED, -15)` de Python. Lo que hay en `in`
detrás del final de los datos comprimidos no se mira.

Un zlib con cabecera (el de `zlib.compress` de Python) no vale tal cual: es
deflate con 2 bytes delante y 4 detrás; pásale `in + 2`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `in` | puntero | Los datos comprimidos. |
| `in_len` | bytes | Cuántos hay. |
| `out` | puntero | Donde se descomprime. Puede ser `NULL` con `out_max` 0. |
| `out_max` | de 0 a 0x7FFFFFFF bytes | El sitio que hay en `out`. Nunca se escribe más allá. |

**Devuelve:** los bytes descomprimidos, o un error (negativo).

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_INFLATE_E_DATA` | No es deflate, está dañado o cortado; `in` es `NULL`; `out` es `NULL` con `out_max` mayor que 0; `out_max` pasa de 0x7FFFFFFF. |
| `ZXV_INFLATE_E_SPACE` | No cabe en `out_max` bytes. |

**Ejemplo**

```c
#include "zxv_inflate.h"

extern const uint8_t title_z[];                 /* zlib.compress(datos, 9), de Python */
extern const uint32_t title_z_len;
static uint8_t title[6144];

int unpack_title(void)
{
    /* Sin los 2 bytes de la cabecera de zlib (los 4 del final no se miran). */
    return zxv_inflate(title_z + 2, title_z_len - 2, title, sizeof title);
}
```

**Ver también:** [`zxv_gunzip`](#zxv_gunzip).

### `zxv_gunzip`

```c
int zxv_gunzip(const void *in, uint32_t in_len, void *out, uint32_t out_max);
```

Descomprime un `.gz` (RFC 1952) entero, de un solo miembro (lo que hacen
gzip y 7-Zip con un fichero), y comprueba el CRC y la longitud que lleva al
final. Se salta el nombre, el comentario y los campos de más de la
cabecera.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `in` | puntero | El `.gz`. |
| `in_len` | bytes | Su tamaño **exacto** (el final lleva el CRC y la longitud). |
| `out` | puntero | Donde se descomprime. Puede ser `NULL` con `out_max` 0. |
| `out_max` | de 0 a 0x7FFFFFFF bytes | El sitio que hay en `out`. |

**Devuelve:** los bytes descomprimidos, o un error (negativo).

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_INFLATE_E_DATA` | No es un `.gz` (mide menos de 18 bytes, no empieza por 1F 8B 08, tiene bits reservados); el deflate está dañado; el CRC o la longitud no cuadran (también si `in_len` no es el tamaño exacto, o si el `.gz` tiene varios miembros); punteros o `out_max` como en `zxv_inflate`. |
| `ZXV_INFLATE_E_SPACE` | No cabe en `out_max` bytes. |

**Ejemplo**

```c
#include "zxv_api.h"
#include "zxv_inflate.h"

static uint8_t packed[8192], map[32768];

/* Un mapa comprimido en la tarjeta. */
int load_map(const char *path)
{
    int n = zxv_file_load(path, packed, sizeof packed);
    if (n < 0)
        return n;
    return zxv_gunzip(packed, (uint32_t)n, map, sizeof map);
}
```

**Ver también:** [`zxv_gunzip_size`](#zxv_gunzip_size), [`zxv_inflate`](#zxv_inflate).

### `zxv_gunzip_size`

```c
int zxv_gunzip_size(const void *in, uint32_t in_len);
```

Lo que ocupará un `.gz` descomprimido, sin descomprimirlo: lo dicen sus 4
últimos bytes. Solo mira la cabecera y el final; que los datos estén bien
lo dice [`zxv_gunzip`](#zxv_gunzip).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `in` | puntero | El `.gz`. |
| `in_len` | bytes | Su tamaño exacto. |

**Devuelve:** el tamaño descomprimido en bytes, o `ZXV_INFLATE_E_DATA`.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_INFLATE_E_DATA` | No parece un `.gz` (como en `zxv_gunzip`), `in` es `NULL`, o el tamaño que dice pasa de 0x7FFFFFFF. |

**Ejemplo**

```c
#include "zxv_inflate.h"

static uint8_t buf[16384];

int unpack_if_fits(const uint8_t *gz, uint32_t len)
{
    int size = zxv_gunzip_size(gz, len);
    if (size < 0 || size > (int)sizeof buf)
        return ZXV_INFLATE_E_SPACE;
    return zxv_gunzip(gz, len, buf, sizeof buf);
}
```

**Ver también:** [`zxv_gunzip`](#zxv_gunzip).

## Fuentes de letra: `zxv_font.h`

Texto de **cualquier tamaño y color**, proporcional o de ancho fijo, con
tildes y eñes. Una fuente sale de:

- **la de la ROM**, la 8 × 8 de la máquina, ya hecha:
  [`zxv_font_rom`](#zxv_font_rom);
- **una hoja**: un bitmap con un carácter en cada celda (de cualquier
  tamaño, en color y con el 8 transparente), por ejemplo un sprite de un
  `.gfx` dibujado en `zxv-gfx` o hecho con `png2gfx`:
  [`zxv_font_sheet`](#zxv_font_sheet);
- **una tabla de 1 bit** hecha con [`fontconv.py`](#fuentes-de-otros-sitios-fontconvpy):
  [`zxv_font_bits`](#zxv_font_bits).

```make
SOURCES := juego.c $(SDK)/lib/font.c $(SDK)/lib/font_rom.c
```

**Añade siempre los dos ficheros**, aunque no uses la fuente de la ROM:
`font.c` hace referencia a ella y, con las opciones del Makefile del SDK,
sin `font_rom.c` no enlaza. (`font_rom.c` ocupa algo más de 1 KB. Si
compilas con `-ffunction-sections` y no llamas a `zxv_font_rom`, el
enlazador lo quita y puedes no ponerlo.)

```c
#include "zxv_font.h"

static zxv_font f;

void game_over_title(void)
{
    zxv_font_rom(&f, 0);
    const char *t = "¡Fin de la partida!";
    int w = zxv_font_width(&f, t, 2);
    zxv_font_draw(&f, (256 - w) / 2 + 1, 81, t, 0, 2);     /* la sombra */
    zxv_font_draw(&f, (256 - w) / 2, 80, t, 14, 2);        /* el doble de grande, centrado */
}
```

- **El texto** va en UTF-8 y puede tener `\n` (cambia de línea). Lo que no
  es UTF-8 válido se toma byte a byte como Latin-1. Lo que no está en la
  fuente sale como `?` (y si tampoco está el `?`, como un hueco de media
  celda).
- **Se dibuja con [`zxv_gfx_rect`](reference.md#figuras)**, en el destino
  actual: respeta el recorte y también la [trama](graphics.md#figuras-con-trama)
  (texto con trama). Cada tramo de píxeles seguidos del mismo color de una
  fila es un rectángulo, así que ampliar no cuesta más.
- En las proporcionales, cada carácter empieza en su primera columna con
  tinta y avanza lo que mide más `spacing` (1 píxel); un carácter sin tinta
  (el espacio) avanza media celda.

### `ZXV_FONT_CHARS`, `ZXV_FONT_FIXED`, `ZXV_FONT_OWN`

| Constante | Valor | Qué es |
|---|---|---|
| `ZXV_FONT_CHARS` | 224 | Los caracteres que caben en una fuente, como mucho. |
| `ZXV_FONT_FIXED` | 0x01 | Opción al crear la fuente: de ancho fijo, todos avanzan el ancho de la celda y sin espacio entre letras. Sin ella, proporcional. |
| `ZXV_FONT_OWN` | −1 | El color de `zxv_font_draw` para dibujar con los colores de la hoja (o el 15, en las de 1 bit). |

**Ver también:** [`zxv_font`](#zxv_font), [`zxv_font_draw`](#zxv_font_draw).

### `zxv_font`

```c
typedef struct {
    zxv_bitmap sheet;
    const uint8_t *bits;
    const uint16_t *codes;
    uint16_t first, count;
    uint8_t w, h;
    uint8_t spacing;
    uint8_t flags;
    uint8_t left[ZXV_FONT_CHARS];
    uint8_t adv[ZXV_FONT_CHARS];
} zxv_font;
```

Una fuente lista para dibujar. La rellenan [`zxv_font_rom`](#zxv_font_rom),
[`zxv_font_sheet`](#zxv_font_sheet) o [`zxv_font_bits`](#zxv_font_bits),
que miden cada carácter. Ocupa unos 480 bytes: mejor global o `static` que
en la pila. Los gráficos no se copian: la hoja o la tabla tienen que seguir
existiendo.

| Campo | Qué es |
|---|---|
| `sheet` | La hoja (4 bpp), en las de hoja; `sheet.pix` es `NULL` en las de 1 bit. |
| `bits` | La tabla de 1 bit (`h` bytes por carácter, el bit 7 a la izquierda); `NULL` en las de hoja. |
| `codes` | El código Unicode de cada carácter, de menor a mayor; `NULL`: `first`, `first + 1`... |
| `first`, `count` | El código del primero y cuántos hay (hasta 224). |
| `w`, `h` | El tamaño de la celda, en píxeles (en las de 1 bit, `w` es 8). |
| `spacing` | Los píxeles entre letras: 1 en las proporcionales, 0 en las de ancho fijo. **Se puede cambiar** después de crear la fuente (`f.spacing = 2;`). |
| `flags` | Las opciones con que se creó. |
| `left[i]`, `adv[i]` | De cada carácter, las columnas vacías a su izquierda y lo que avanza (sin `spacing`). Las calcula la biblioteca. |

**Ver también:** [`ZXV_FONT_CHARS`...](#zxv_font_chars-zxv_font_fixed-zxv_font_own).

### `zxv_font_rom`

```c
void zxv_font_rom(zxv_font *f, unsigned flags);
```

La fuente de 8 × 8 de la máquina, con 112 caracteres: del 32 al 126 de
ASCII, el 127 (©), y ¡ ¿ Á É Í Ñ Ó Ú Ü á é í ñ ó ú ü. Está en
`lib/font_rom.c`, no se lee de la ROM.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | puntero a un `zxv_font` | Donde se prepara. Con `NULL`, no hace nada. |
| `flags` | 0 o `ZXV_FONT_FIXED` | 0: proporcional, con 1 píxel entre letras; `ZXV_FONT_FIXED`: de 8 en 8, como el texto de la máquina. |

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_font.h"

static zxv_font hud;

void hud_init(void) { zxv_font_rom(&hud, ZXV_FONT_FIXED); }   /* los números no bailan */
```

**Ver también:** [`zxv_font_draw`](#zxv_font_draw).

### `zxv_font_sheet`

```c
int zxv_font_sheet(zxv_font *f, const zxv_bitmap *sheet, int cell_w, int cell_h, int first, unsigned flags);
```

Una fuente de una hoja: un bitmap de 4 bpp (como los de la memoria de
vídeo: el nibble bajo es el píxel par) con un carácter en cada celda de
`cell_w` × `cell_h`, de izquierda a derecha y de arriba abajo, con códigos
seguidos desde `first`. Los caracteres pueden ser de colores; el 8 es el
transparente (y lo que cuenta como vacío para medirlos).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | puntero a un `zxv_font` | Donde se prepara. |
| `sheet` | un `zxv_bitmap` con `pix` | La hoja. No se copia: tiene que seguir existiendo. |
| `cell_w`, `cell_h` | de 1 a 255, y no más que la hoja | El tamaño de cada celda. |
| `first` | de 0 a 65535 | El código del carácter de la primera celda (32 para empezar en el espacio). |
| `flags` | 0 o `ZXV_FONT_FIXED` | Proporcional o de ancho fijo. |

Caben tantas celdas enteras como haya en la hoja (hasta 224); las que
sobran por el borde derecho o por abajo no cuentan.

**Devuelve:** 0 si la fuente vale; −1 si no, y entonces `*f` no cambia.

**Errores**

| Devuelve | Cuándo |
|---|---|
| −1 | `f` o `sheet` son `NULL`, `sheet->pix` es `NULL`, `cell_w` o `cell_h` están fuera de 1–255 o son mayores que la hoja, o `first` es negativo. |

**Ejemplo**

```c
#include "zxv_font.h"

extern zxv_bitmap digits_sheet;                 /* del .gfx: «0123456789» en celdas de 12 x 16 */
static zxv_font digits;

int score_font(void) { return zxv_font_sheet(&digits, &digits_sheet, 12, 16, '0', ZXV_FONT_FIXED); }
```

**Ver también:** [`zxv_font_bits`](#zxv_font_bits), [`zxv_font_draw`](#zxv_font_draw).

### `zxv_font_bits`

```c
int zxv_font_bits(zxv_font *f, const uint8_t *bits, const uint16_t *codes, int n, int h, unsigned flags);
```

Una fuente de una tabla de 1 bit de 8 píxeles de ancho: la que escribe
[`fontconv.py`](#fuentes-de-otros-sitios-fontconvpy). Cada carácter son
`h` bytes, uno por fila, con la columna de la izquierda en el bit 7.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | puntero a un `zxv_font` | Donde se prepara. |
| `bits` | `n` · `h` bytes | Los caracteres. No se copian. |
| `codes` | `n` códigos de menor a mayor, o `NULL` | El código Unicode de cada carácter. Tienen que ir ordenados: si no, algunos no se encuentran. `NULL`: del 32 en adelante, seguidos. |
| `n` | de 1 a 224 | Cuántos caracteres hay. |
| `h` | de 1 a 255 | Las filas de cada uno. |
| `flags` | 0 o `ZXV_FONT_FIXED` | Proporcional o de ancho fijo. |

**Devuelve:** 0 si la fuente vale; −1 si no, y `*f` no cambia.

**Errores**

| Devuelve | Cuándo |
|---|---|
| −1 | `f` o `bits` son `NULL`, `n` está fuera de 1–224 o `h` fuera de 1–255. |

**Ejemplo:** ver [`fontconv.py`](#fuentes-de-otros-sitios-fontconvpy).

**Ver también:** [`zxv_font_sheet`](#zxv_font_sheet), [`zxv_font_draw`](#zxv_font_draw).

### `zxv_font_draw`

```c
int zxv_font_draw(const zxv_font *f, int x, int y, const char *text, int colour, int scale);
```

Dibuja el texto con la esquina de arriba a la izquierda de su primera
celda en (`x`, `y`). Un `\n` vuelve a `x`, `h` · `scale` píxeles más abajo.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | una fuente preparada | La fuente. |
| `x`, `y` | píxeles del destino, cualquier valor | Dónde. Lo que cae fuera del recorte no se dibuja. |
| `text` | UTF-8 | El texto. |
| `colour` | 0–15, o `ZXV_FONT_OWN` | 0–15: toda la tinta de ese color. Cualquier otro valor (`ZXV_FONT_OWN`): los colores de la hoja; en las de 1 bit, el 15. |
| `scale` | de 1 a 8 | Cuántas veces más grande. Menos de 1 es 1 y más de 8 es 8. |

**Devuelve:** la x en la que acaba la última línea (contando el `spacing`
tras la última letra): para seguir escribiendo detrás. Con `f` o `text`
`NULL`, no dibuja nada y devuelve `x`.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_font.h"

static zxv_font f;

/* «Puntos: » en blanco y el número en amarillo, detrás. */
void draw_score(const char *digits)
{
    zxv_font_rom(&f, 0);
    int x = zxv_font_draw(&f, 8, 4, "Puntos: ", 15, 1);
    zxv_font_draw(&f, x, 4, digits, 14, 1);
}
```

**Ver también:** [`zxv_font_width`](#zxv_font_width),
[`zxv_font_height`](#zxv_font_height).

### `zxv_font_width`

```c
int zxv_font_width(const zxv_font *f, const char *text, int scale);
```

Lo que mide de ancho el texto: el de su línea más larga, sin el `spacing`
de detrás de la última letra. Para centrarlo o alinearlo a la derecha.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | una fuente preparada | La fuente. |
| `text` | UTF-8 | El texto. |
| `scale` | de 1 a 8 | Como en `zxv_font_draw`. |

**Devuelve:** el ancho en píxeles; 0 con `f` o `text` `NULL` o con un
texto vacío.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_font.h"

/* Alineado a la derecha, acabando en la columna 248. */
void draw_right(const zxv_font *f, int y, const char *t)
{
    zxv_font_draw(f, 248 - zxv_font_width(f, t, 1), y, t, 7, 1);
}
```

**Ver también:** [`zxv_font_height`](#zxv_font_height).

### `zxv_font_height`

```c
int zxv_font_height(const zxv_font *f, const char *text, int scale);
```

Lo que mide de alto el texto: sus líneas por el alto de la celda por
`scale`. Cada `\n` es una línea más, también al final.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | una fuente preparada | La fuente. |
| `text` | UTF-8 | El texto. |
| `scale` | de 1 a 8 | Como en `zxv_font_draw`. |

**Devuelve:** el alto en píxeles; 0 con `f` o `text` `NULL` o con un texto
vacío.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_font.h"

/* Un texto de varias líneas, centrado en la pantalla de 256 x 192. */
void draw_centred(const zxv_font *f, const char *t)
{
    int w = zxv_font_width(f, t, 1), h = zxv_font_height(f, t, 1);
    zxv_font_draw(f, (256 - w) / 2, (192 - h) / 2, t, 15, 1);
}
```

**Ver también:** [`zxv_font_width`](#zxv_font_width).

### Fuentes de otros sitios: `fontconv.py`

`sdk/tools/fontconv.py` convierte fuentes de mapa de bits de hasta 8
columnas y de 1 a 32 filas en C para [`zxv_font_bits`](#zxv_font_bits), o
en texto para retocarlas:

```sh
python3 sdk/tools/fontconv.py mifuente.ch8 mifuente.c mifuente
python3 sdk/tools/fontconv.py mifuente.ch8 mifuente.txt
```

| Formato | Cómo lo reconoce |
|---|---|
| El de ZX-V en texto (`== CÓDIGO` y las filas con `#` y `.`, como `rom/font`) | `.txt` |
| ZX Spectrum (96 caracteres, 768 bytes): su ↑, £ y © van a sus códigos (`--ascii`, no) | `.ch8`, o 768 bytes |
| Un volcado de 8 bytes por carácter (PC, MSX, Amstrad CPC...): `--first`, `--height`, `--charset` (CP437 para los de PC: las tildes y la ñ en su sitio) | `.bin`, `.rom`, `.fnt`... |
| PSF 1 y 2 (las de la consola de Linux), con su tabla Unicode | por su firma |
| BDF (X11) | `.bdf` |
| `.hex` (GNU Unifont: `CÓDIGO:BYTES`): los caracteres anchos (los de 16 columnas) se quedan fuera, con un aviso | `.hex` |

| Opción | Qué hace |
|---|---|
| `--format F` | `txt`, `zx`, `raw`, `psf`, `bdf` o `hex`, si la extensión no basta. |
| `--first N` | En un volcado, el código del primer carácter (0; 32 si son 96). |
| `--height H` | En un volcado, las filas de cada carácter (8). En un `.hex`, las de los de 8 columnas (si no, las de los más estrechos del fichero; si todos miden lo mismo y son de más de 16 bytes, hay que decirlo: 16 para los anchos de Unifont). |
| `--charset C` | En un volcado, cómo pasar los códigos de más de 127 a Unicode: `cp437` (el de siempre si son 256 desde el 0), `latin-1`... |
| `--ascii` | En una del Spectrum, deja `^`, `` ` `` y el 127 como en ASCII. |
| `--range R` | Los caracteres que se quedan, en hexadecimal: por defecto `20-7E,A0-FF` (ASCII y Latin-1, con las tildes, la ñ, ¿ y ¡). |

Primero se quedan los de `--range`: lo que no está no cuenta (ni sus
filas). Se quedan como mucho 224 caracteres: si hay más en `--range`, da un
error. En un volcado, los códigos son de un byte: uno de más de 256
caracteres (o que pase del 255 desde `--first`) da un error; suele ser que
`--height` no es el de la fuente.
Los códigos van en 16 bits: un `--range` con algo por encima de FFFF (o
un intervalo al revés) da un error. La licencia de una fuente sigue siendo la suya: que se pueda
convertir no quiere decir que se pueda repartir.

**Lo que escribe.** El `.c` define cuatro cosas, con el nombre que le das
(el tercer argumento) delante:

| Definición | Qué es |
|---|---|
| `const unsigned NOMBRE_n` | Cuántos caracteres hay. |
| `const unsigned NOMBRE_h` | Las filas de cada uno. |
| `const uint16_t NOMBRE_codes[]` | Sus códigos Unicode, de menor a mayor. |
| `const uint8_t NOMBRE_bits[]` | Los caracteres, `NOMBRE_h` bytes cada uno. |

No escribe ninguna cabecera: añade el `.c` a `SOURCES` y declara lo que
usas con `extern`:

```c
#include "zxv_font.h"

/* De «fontconv.py mifuente.ch8 mifuente.c mifuente» (mifuente.c, en SOURCES). */
extern const unsigned mifuente_n, mifuente_h;
extern const uint16_t mifuente_codes[];
extern const uint8_t mifuente_bits[];

static zxv_font f;

int load_font(void)
{
    return zxv_font_bits(&f, mifuente_bits, mifuente_codes, (int)mifuente_n, (int)mifuente_h, 0);
}
```

## 3D sencillo: `zxv_3d.h`

Modelos de **caras planas** con luz y sombras de trama, como los juegos 3D
de los 8 y los 16 bits: cada cara tiene un color oscuro y uno claro, y
según le dé la luz se ve de uno, del otro o de una mezcla con
[las tramas](graphics.md#figuras-con-trama) (cinco tonos). Dibujar con trama
cuesta lo mismo que con un color.

```make
SOURCES := juego.c $(SDK)/lib/3d.c $(SDK)/lib/math.c
```

Los tonos intermedios necesitan una ROM de la especificación 0.2 (usan
[`zxv_gfx_pattern`](reference.md#figuras); con una anterior, esas caras
salen del color claro, sin trama). Ocupa unos 29 KB de memoria
(las tablas del frame, fijas). El 3D nota mucho la optimización: el ejemplo
Gone Silent compila `3d.c` con `-O2` en vez del `-Os` de siempre.

```c
#include "zxv_3d.h"
#include "zxv_gfx.h"

extern const zxv_model house;                   /* de obj2c.py (abajo) */

int main(void)
{
    zxv_camera cam = { .pos = { ZXV_FIX(-3), ZXV_FIX(2), ZXV_FIX(-7) },   /* dónde está */
                       .yaw = ZXV_DEG(20), .pitch = -ZXV_DEG(10),         /* gira, mira abajo */
                       .focal = 128, .cx = 128, .cy = 96, .w = 256, .h = 192 };
    zxv_xform pos;
    for (int32_t a = 0; zxv_key_get() < 0; a += 256) {
        zxv_gfx_rect(0, 0, 256, 80, 5);                            /* cielo y suelo */
        zxv_gfx_rect(0, 80, 256, 112, 4);
        zxv_xform_set(&pos, a, 0, 0, 0, ZXV_FIX(1), 0);            /* la casa, girando */
        zxv_3d_begin(&cam);
        zxv_3d_add(&house, &pos);
        zxv_3d_draw();
        zxv_wait_vblank();
    }
    return 0;
}
```

(Para que no parpadee, dibuja en la página oculta: ver [Las páginas y el
doble buffer](graphics.md#las-páginas-y-el-doble-buffer).)

- **Coordenadas:** x a la derecha, y hacia arriba, z hacia delante (adonde
  mira la cámara sin girar), en 16.16. El mundo cabe en ±32768 unidades.
  Los ángulos, en vueltas de 0x10000: `yaw` gira a la derecha, `pitch` mira
  hacia arriba y `roll` inclina a la derecha.
- **Cada frame:** [`zxv_3d_begin(&cam)`](#zxv_3d_begin), un
  [`zxv_3d_add(&modelo, &pos)`](#zxv_3d_add) por objeto (con un
  [`zxv_xform`](#zxv_xform): su giro y su sitio) y
  [`zxv_3d_draw()`](#zxv_3d_draw). Las caras que dan la espalda no se
  dibujan, todo se recorta contra la vista y las caras se dibujan de lejos
  a cerca (el algoritmo del pintor).
- **El pintor se equivoca** con caras que se cruzan, o con caras muy largas
  que se solapan con otras: un suelo grande, mejor con `zxv_gfx_rect` o con
  el modo 7 antes del 3D. Ver también los consejos de
  [`obj2c.py`](#modelos-de-blender-obj2cpy).

### `ZXV_3D_MAX_FACES`, `ZXV_3D_MAX_VERTS`, `ZXV_3D_FACE_SIDES`

| Constante | Valor | Qué es |
|---|---|---|
| `ZXV_3D_MAX_FACES` | 400 | Las caras que se ven en un frame, entre todos los objetos. |
| `ZXV_3D_MAX_VERTS` | 512 | Los vértices de un modelo. |
| `ZXV_3D_FACE_SIDES` | 32 | Los lados de una cara, como mucho. Una cara no convexa se parte en triángulos al dibujarla. |

Las dos primeras se cambian al compilar `lib/3d.c` con `-D`
(`EXTRA_CFLAGS=-DZXV_3D_MAX_FACES=800`): con los valores de siempre, las
tablas ocupan unos 29 KB; cada cara de más, unos 43 bytes, y cada vértice,
24. Además de las caras, hay sitio para 6 puntos por cara de media (las
recortadas tienen más): un frame lleno de caras de muchos lados puede
quedarse sin sitio antes.

**Ver también:** [`zxv_3d_add`](#zxv_3d_add).

### `ZXV_3D_MAT`, `ZXV_3D_TWO_SIDED`, `ZXV_3D_FLAT`, `ZXV_3D_DECAL`

```c
#define ZXV_3D_MAT(dark, light, flags) ((uint16_t)((dark) | (light) << 4 | (flags)))
#define ZXV_3D_TWO_SIDED  0x0100u
#define ZXV_3D_FLAT       0x0200u
#define ZXV_3D_DECAL      0x0400u
```

El material de una cara, el número que va en el modelo detrás del número de
lados: sus dos colores y sus opciones.

| Parte | Valores | Qué es |
|---|---|---|
| `dark` | 0–15 | El color oscuro: el de la cara de espaldas a la luz. |
| `light` | 0–15 | El color claro: el de la cara de frente a la luz. Si es igual que `dark`, la cara es lisa, sin tramas. |
| `ZXV_3D_TWO_SIDED` | 0x0100 | Se ve por los dos lados (una hoja, una valla). Por detrás, la luz le da por ese lado. |
| `ZXV_3D_FLAT` | 0x0200 | Sin luz: siempre del color oscuro. |
| `ZXV_3D_DECAL` | 0x0400 | Va pegada encima de otra (una puerta en una pared): se ordena por su vértice más cercano, así que se dibuja después de la cara en la que está. |

**Ejemplo:** ver [`zxv_model`](#zxv_model).

**Ver también:** [`zxv_model`](#zxv_model), [`zxv_3d_light`](#zxv_3d_light).

### `zxv_vec3`

```c
typedef struct {
    zxv_fix x, y, z;
} zxv_vec3;
```

Un punto o un vector del mundo, en 16.16: x a la derecha, y hacia arriba,
z hacia delante.

**Ver también:** [`zxv_xform_apply`](#zxv_xform_apply), [`zxv_3d_project`](#zxv_3d_project).

### `zxv_xform`

```c
typedef struct {
    zxv_fix m[3][3];
    zxv_vec3 t;
} zxv_xform;
```

Dónde está un objeto y cómo está girado: una rotación (matriz de 3 × 3 en
16.16) y una traslación. Se prepara con [`zxv_xform_set`](#zxv_xform_set)
y se combina con [`zxv_xform_mul`](#zxv_xform_mul). Un punto `p` del
objeto va a `m` · `p` + `t` en el mundo.

| Campo | Qué es |
|---|---|
| `m[i][j]` | La rotación, fila `i`, columna `j`, en 16.16. Puede llevar también una escala (multiplica la matriz por ella). |
| `t` | Dónde va el origen del objeto, en el mundo. |

**Ver también:** [`zxv_xform_set`](#zxv_xform_set).

### `zxv_model`

```c
typedef struct {
    const int16_t (*verts)[3];
    const uint16_t *faces;
    uint16_t nverts, nfaces;
    zxv_fix scale;
} zxv_model;
```

Un modelo: lo que escribe [`obj2c.py`](#modelos-de-blender-obj2cpy), o uno
hecho a mano.

| Campo | Valores | Qué es |
|---|---|---|
| `verts` | `nverts` vértices de 3 enteros de 16 bits | x, y, z de cada vértice, en unidades del modelo. |
| `faces` | una lista de `uint16_t` | Las caras, una tras otra: el número de vértices (3 a 32), el [material](#zxv_3d_mat-zxv_3d_two_sided-zxv_3d_flat-zxv_3d_decal) y los índices de sus vértices (de 0 a `nverts` − 1). |
| `nverts` | de 0 a 512 | Cuántos vértices. |
| `nfaces` | 0 o más | Cuántas caras. |
| `scale` | 16.16 | Lo que mide una unidad del modelo en el mundo: con `ZXV_FIX_ONE`, cada unidad es una del mundo; con `ZXV_FIX_ONE / 256`, 256 unidades del modelo son una del mundo. |

Una cara **se ve por delante si sus vértices van en el sentido del reloj
mirándola desde fuera**; por detrás no se dibuja (salvo con
`ZXV_3D_TWO_SIDED`). La luz y si se ve salen de la normal de toda la cara,
así que da igual por qué vértice empiece. Una cara es **plana y no se cruza
consigo misma**, pero puede no ser convexa (una L, una sierra): se parte en
triángulos al dibujarla, en cada frame ([`obj2c.py`](#modelos-de-blender-obj2cpy)
ya las da partidas, que es más rápido).

**Ejemplo**

```c
#include "zxv_3d.h"

/* Un cubo de 2 unidades de lado, azul (1 y 9). */
static const int16_t cube_v[8][3] = {
    { -1, -1, -1 }, { 1, -1, -1 }, { -1, 1, -1 }, { 1, 1, -1 },
    { -1, -1, 1 },  { 1, -1, 1 },  { -1, 1, 1 },  { 1, 1, 1 },
};
#define BLUE ZXV_3D_MAT(1, 9, 0)
static const uint16_t cube_f[] = {
    4, BLUE, 0, 2, 3, 1,                        /* delante (z = -1) */
    4, BLUE, 5, 7, 6, 4,                        /* detrás */
    4, BLUE, 1, 3, 7, 5,                        /* derecha */
    4, BLUE, 4, 6, 2, 0,                        /* izquierda */
    4, BLUE, 2, 6, 7, 3,                        /* arriba */
    4, BLUE, 0, 1, 5, 4,                        /* abajo */
};
const zxv_model cube = { cube_v, cube_f, 8, 6, ZXV_FIX_ONE };
```

**Ver también:** [`zxv_3d_add`](#zxv_3d_add), [`obj2c.py`](#modelos-de-blender-obj2cpy).

### `zxv_camera`

```c
typedef struct {
    zxv_vec3 pos;
    int32_t yaw, pitch, roll;
    int32_t focal;
    int16_t cx, cy;
    int16_t w, h;
    zxv_fix near;
} zxv_camera;
```

Desde dónde se mira y cómo se ve en la pantalla.

| Campo | Valores | Qué es |
|---|---|---|
| `pos` | 16.16 | Dónde está la cámara. |
| `yaw`, `pitch`, `roll` | vueltas de 0x10000 | Hacia dónde mira: `yaw` gira a la derecha, `pitch` mira hacia arriba, `roll` inclina a la derecha. Con los tres a 0, mira hacia z creciente. |
| `focal` | píxeles (entero), hasta 32767 (más cuenta como 32767); 0 o menos, 128 | La distancia focal: con 128 y una vista de 256 de ancho, unos 90° de lado a lado. Más, más zoom. |
| `cx`, `cy` | píxeles | El centro de la vista en el destino (128, 96 para la pantalla entera del modo 0). **Sin valor por defecto:** ponlos. |
| `w`, `h` | píxeles; 0 o menos, 256 y 192 | El tamaño de la vista, para recortar. Lo que se dibuja de verdad lo recorta además el [recorte](reference.md#destino-y-recorte) del motor gráfico: para una ventana, pon los dos. |
| `near` | 16.16; 0 o menos, 0,25 | Lo más cerca que se dibuja: lo que está más cerca se recorta. |

**Ver también:** [`zxv_3d_begin`](#zxv_3d_begin), [`zxv_3d_project`](#zxv_3d_project).

### `zxv_xform_set`

```c
void zxv_xform_set(zxv_xform *x, int32_t yaw, int32_t pitch, int32_t roll, zxv_fix px, zxv_fix py, zxv_fix pz);
```

Prepara una posición: el giro de los tres ángulos y el sitio. Al aplicarla,
el objeto se inclina primero (`roll`), luego se levanta o se baja
(`pitch`) y por último gira (`yaw`), y se lleva a (`px`, `py`, `pz`).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x` | puntero a un `zxv_xform` | Donde se deja. |
| `yaw` | vueltas de 0x10000 | Gira a la derecha (alrededor de y). |
| `pitch` | vueltas de 0x10000 | Mira hacia arriba (alrededor de x). |
| `roll` | vueltas de 0x10000 | Se inclina a la derecha (alrededor de z). |
| `px`, `py`, `pz` | 16.16 | El sitio. |

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_3d.h"

/* Una nave en (x, 1, z) que mira hacia 'heading' y se inclina en las curvas. */
void place_ship(zxv_xform *pos, zxv_fix x, zxv_fix z, int32_t heading, int32_t bank)
{
    zxv_xform_set(pos, heading, 0, bank, x, ZXV_FIX(1), z);
}
```

**Ver también:** [`zxv_xform_mul`](#zxv_xform_mul), [`zxv_3d_add`](#zxv_3d_add).

### `zxv_xform_apply`

```c
zxv_vec3 zxv_xform_apply(const zxv_xform *x, zxv_vec3 p);
```

Dónde va un punto con una posición: `m` · `p` + `t`. Para saber dónde
queda en el mundo un punto de un objeto (la boca del cañón, para que salga
de ahí el disparo).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x` | un `zxv_xform` | La posición. |
| `p` | 16.16 | El punto, en las coordenadas del objeto. |

**Devuelve:** el punto en el mundo, en 16.16. Si se sale de ±32768, no vale
(se desborda).

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_3d.h"

/* El cañón está 1,5 unidades por delante del centro de la nave. */
zxv_vec3 gun_position(const zxv_xform *ship)
{
    zxv_vec3 gun = { 0, 0, ZXV_FIX(1.5) };
    return zxv_xform_apply(ship, gun);
}
```

**Ver también:** [`zxv_xform_set`](#zxv_xform_set), [`zxv_3d_project`](#zxv_3d_project).

### `zxv_xform_mul`

```c
void zxv_xform_mul(zxv_xform *out, const zxv_xform *a, const zxv_xform *b);
```

Combina dos posiciones: `out` = `a` · `b`, que es aplicar primero `b` y
luego `a`. Sirve para poner un objeto sobre otro: un brazo sobre un cuerpo
es `cuerpo · brazo`, con la posición del brazo relativa al cuerpo.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `out` | puntero a un `zxv_xform` | Donde se deja. Puede ser el mismo que `a` o que `b`. |
| `a` | un `zxv_xform` | La de fuera (el cuerpo). |
| `b` | un `zxv_xform` | La de dentro (el brazo, relativo al cuerpo). |

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_3d.h"

extern const zxv_model tank_body, tank_turret;

/* La torreta gira sobre el tanque, 0,5 unidades por encima de su centro. */
void add_tank(const zxv_xform *body, int32_t turret_angle)
{
    zxv_xform rel, turret;
    zxv_xform_set(&rel, turret_angle, 0, 0, 0, ZXV_FIX(0.5), 0);
    zxv_xform_mul(&turret, body, &rel);
    zxv_3d_add(&tank_body, body);
    zxv_3d_add(&tank_turret, &turret);
}
```

**Ver también:** [`zxv_xform_set`](#zxv_xform_set).

### `zxv_3d_light`

```c
void zxv_3d_light(zxv_fix x, zxv_fix y, zxv_fix z, int ambient);
```

Pone la luz: de dónde viene y el tono de lo que no la recibe. Se queda
para los frames siguientes. **Cuenta desde el siguiente
[`zxv_3d_begin`](#zxv_3d_begin)**, que es quien la gira con la cámara.

Cada cara tiene cinco tonos, del 0 (el color oscuro) al 4 (el claro),
pasando por tramas de un cuarto, la mitad y tres cuartos del claro. Una
cara de espaldas a la luz tiene el tono `ambient`; una de frente, el 4; y
las de en medio, según el ángulo.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x`, `y`, `z` | 16.16, cualquier largo | Un vector **hacia** la luz, en el mundo. Solo cuenta la dirección. Con (0, 0, 0), todas las caras tienen el tono `ambient`. |
| `ambient` | de 0 a 4 | El tono de lo que no recibe luz. Menos de 0 es 0 y más de 4 es 4. |

Sin llamarla, la luz viene de arriba, de la izquierda y de delante (del
vector −1, 1,85, −1) con `ambient` 1.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_3d.h"

void night(void) { zxv_3d_light(0, ZXV_FIX(1), 0, 0); }         /* desde arriba, sombras negras */
void noon(void) { zxv_3d_light(ZXV_FIX(1), ZXV_FIX(2), 0, 2); }  /* mucha luz por todas partes */
```

**Ver también:** [`ZXV_3D_MAT`...](#zxv_3d_mat-zxv_3d_two_sided-zxv_3d_flat-zxv_3d_decal).

### `zxv_3d_wireframe`

```c
void zxv_3d_wireframe(int colour);
```

Dibuja las aristas de las caras en vez de rellenarlas, con
[`zxv_gfx_line`](reference.md#figuras): para depurar, o para un juego de
líneas. Se queda hasta que se cambie, y cuenta desde el siguiente
`zxv_3d_draw`. Las caras de espaldas siguen sin dibujarse.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `colour` | 0–15, o −1 | El color de las aristas. Cualquier otro valor (−1): caras otra vez. |

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_3d.h"

void debug_view(int on) { zxv_3d_wireframe(on ? 4 : -1); }      /* verde */
```

**Ver también:** [`zxv_3d_draw`](#zxv_3d_draw).

### `zxv_3d_begin`

```c
void zxv_3d_begin(const zxv_camera *cam);
```

Empieza un frame: vacía la lista de caras y toma la cámara (y la luz). Los
campos de la cámara que valen 0 (`focal`, `w`, `h`, `near`) toman sus
valores por defecto; `cx` y `cy`, no.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `cam` | una `zxv_camera` | La cámara. Se copia. |

**Errores:** ninguno.

**Ejemplo:** ver el de la [introducción](#3d-sencillo-zxv_3dh).

**Ver también:** [`zxv_3d_add`](#zxv_3d_add), [`zxv_camera`](#zxv_camera).

### `zxv_3d_add`

```c
int zxv_3d_add(const zxv_model *m, const zxv_xform *pos);
```

Añade un modelo al frame: lo pone en su sitio, quita las caras que dan la
espalda, calcula el tono de cada una con la luz, las recorta contra la
vista y las guarda para [`zxv_3d_draw`](#zxv_3d_draw). Todavía no dibuja
nada.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `m` | un `zxv_model` | El modelo. |
| `pos` | un `zxv_xform`, o `NULL` | Dónde va. `NULL`: en el origen, sin girar. |

**Devuelve:** las caras del modelo que se ven (0 si no se ve ninguna), o
−1 si el modelo no vale o no caben.

**Errores**

| Devuelve | Cuándo |
|---|---|
| −1 | `m` es `NULL`, o sus `verts` o `faces`; tiene más de 512 vértices (`ZXV_3D_MAX_VERTS`); una cara tiene menos de 3 o más de 32 lados, o un índice de vértice que no existe. |
| −1 | Ya hay `ZXV_3D_MAX_FACES` caras en el frame, o no queda sitio para sus puntos. |

Con −1, las caras de ese modelo que ya se habían añadido se quedan (se
dibujará a medias); las de los modelos de antes, también.

**Ejemplo**

```c
#include "zxv_3d.h"

extern const zxv_model rock;
static zxv_xform rocks[20];

/* Las rocas, hasta que no quepan más. */
int add_rocks(int n)
{
    int faces = 0;
    for (int i = 0; i < n; i++) {
        int r = zxv_3d_add(&rock, &rocks[i]);
        if (r < 0)
            break;
        faces += r;
    }
    return faces;
}
```

**Ver también:** [`zxv_3d_begin`](#zxv_3d_begin), [`zxv_3d_draw`](#zxv_3d_draw),
[`ZXV_3D_MAX_FACES`...](#zxv_3d_max_faces-zxv_3d_max_verts-zxv_3d_face_sides).

### `zxv_3d_draw`

```c
void zxv_3d_draw(void);
```

Dibuja todo lo añadido desde `zxv_3d_begin`, de lejos a cerca (por la
distancia media de los vértices de cada cara; las `ZXV_3D_DECAL`, por la
del más cercano), con [`zxv_gfx_poly`](reference.md#figuras) en el destino
actual y con su recorte. No borra nada antes: el fondo es tuyo.

Si ha usado alguna trama, al acabar **deja la trama del motor quitada**
(`zxv_gfx_pattern(NULL, 0)`): si dibujas con trama después, vuelve a
ponerla. La lista de caras no se vacía hasta el siguiente `zxv_3d_begin`:
llamarla dos veces dibuja dos veces lo mismo.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_3d.h"
#include "zxv_gfx.h"

/* El 3D solo en una ventana de 160 x 120, con el centro en (128, 80). */
void draw_window(const zxv_camera *cam, const zxv_model *m, const zxv_xform *pos)
{
    zxv_gfx_clip(48, 20, 208, 140);
    zxv_gfx_rect(48, 20, 160, 120, 0);
    zxv_3d_begin(cam);                          /* con cx 128, cy 80, w 160, h 120 */
    zxv_3d_add(m, pos);
    zxv_3d_draw();
    zxv_gfx_clip(0, 0, 256, 192);
}
```

**Ver también:** [`zxv_3d_add`](#zxv_3d_add), [`zxv_3d_wireframe`](#zxv_3d_wireframe).

### `zxv_3d_project`

```c
int zxv_3d_project(const zxv_camera *cam, zxv_vec3 p, int *sx, int *sy, zxv_fix *scale);
```

Dónde se ve un punto del mundo con una cámara: para poner sprites en el
mundo 3D (una explosión, un letrero) o un punto de mira. No necesita
`zxv_3d_begin` ni cambia nada del frame.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `cam` | una `zxv_camera` | La cámara (con los mismos valores por defecto que en `zxv_3d_begin`). |
| `p` | 16.16 | El punto, en el mundo. |
| `sx`, `sy` | punteros, o `NULL` | Donde se deja el píxel. |
| `scale` | puntero, o `NULL` | Donde se dejan los píxeles que mide una unidad del mundo a esa distancia, en 16.16 (como mucho 32767,99998). |

**Devuelve:** 1 si el punto está delante de la cámara (más allá de
`near`), con el resultado; 0 si no, o si cae a más de 2³⁰ píxeles, y
entonces no se rellena nada. Con 1, el punto puede estar fuera de la
vista: no se recorta.

**Errores:** ninguno.

**Ejemplo**

```c
#include "zxv_3d.h"
#include "zxv_gfx.h"

/* Una explosión (un sprite de 16 x 16) en el punto p, del tamaño de 1 unidad. */
void draw_blast(const zxv_camera *cam, zxv_vec3 p, const void *spr)
{
    int x, y;
    zxv_fix s;
    if (zxv_3d_project(cam, p, &x, &y, &s)) {
        int size = ZXV_FIX_INT(s);
        if (size > 0)
            zxv_spr_draw_scaled(spr, x - size / 2, y - size / 2, size, size, 0, 0);
    }
}
```

**Ver también:** [`zxv_xform_apply`](#zxv_xform_apply).

### Modelos de Blender: `obj2c.py`

`sdk/tools/obj2c.py` convierte un modelo `.obj` (Blender: Archivo >
Exportar > Wavefront) en C:

```sh
python3 sdk/tools/obj2c.py casa.obj casa.c house --size 2 --mat puerta=1,9,flat,decal
```

Escribe `const zxv_model house` (el tercer argumento) con sus vértices y
sus caras. Añade `casa.c` a `SOURCES` y declara el modelo donde lo uses:

```c
#include "zxv_3d.h"

extern const zxv_model house;

void add_house(const zxv_xform *where) { zxv_3d_add(&house, where); }
```

- Centra el modelo (salvo con `--keep-origin`) y lo escala para que su lado
  más largo mida `--size` unidades del mundo (2 por defecto). Los vértices
  quedan como enteros de hasta ±4096 y `scale` dice cuánto mide cada uno.
- Pasa la z del `.obj` (hacia la cámara) a la de `zxv_3d` (hacia delante) y
  da la vuelta a las caras para que sigan viéndose por fuera.
- Los colores salen del material de cada cara: un material llamado
  `zxv_OSCURO_CLARO` (`zxv_2_10`) usa esos, y con `_two`, `_flat` o
  `_decal` detrás (`zxv_2_10_two`), esas opciones; si no, el color difuso
  (`Kd`) del `.mtl` da el más parecido de los 16 y su pareja con brillo o
  sin él. Sin material, `--default OSCURO,CLARO` (1,9: azul).
- `--mat nombre=oscuro,claro[,two][,flat][,decal]` cambia un material;
  `--two-sided`, todas las caras de los dos lados.
- Las caras no convexas (una L, una sierra, una corona) y las de más de 32
  lados se parten en triángulos, que luego se juntan en trozos convexos de
  hasta 32 lados: se dibujan bien y más deprisa que si `zxv_3d_add` tuviera
  que partirlas en cada frame. Las caras sin área (vértices repetidos o en
  línea) se quitan, y de una que se cruza consigo misma avisa. Con más de
  512 vértices avisa (`zxv_3d_add` no lo aceptaría).
- Cada cara empieza por la esquina que mejor da su normal, para que la luz
  salga bien aunque tenga vértices en línea.
- Con un `.obj` o unas opciones que no valen (un vértice sin sus tres
  números, una opción sin valor...), un mensaje que dice qué y dónde. Los
  comentarios (`#`) pueden ir también al final de una línea.
- **Para que el pintor acierte:** que las piezas se toquen sin meterse unas
  en otras, y parte una cara grande por donde se apoya en ella una
  pequeña (como el lomo de la nave del ejemplo Nave 3D, partido bajo la
  deriva).
- Con `--bas` sale ZX Basic para `zxv3d.bas` en lugar de C (abajo).

### En ZX Basic: `zxv3d.bas`

En el [fork de ZX Basic](../zxbasic.md), `zxv3d.bas` hace lo mismo con
órdenes de BASIC, y `obj2c.py --bas` escribe el modelo como dos matrices y
un `SUB casa_model(n)` que lo pone como el modelo `n` (de 0 a 15):

```sh
python3 sdk/tools/obj2c.py casa.obj casa.bas casa --size 2 --bas
```

```basic
#include <zxv3d.bas>
#include "casa.bas"

casa_model(0)
CAMERA3D -3, 2, -7, 3640, -1820             ' dónde está; gira y mira abajo
FOR a = 0 TO 65535 STEP 256
    BOX 5, 0, 0, 256, 80: BOX 4, 0, 80, 256, 112
    BEGIN3D: LET n = ADD3D(0, 0, 1, 0, a): DRAW3D: WAIT
NEXT a
```

Las órdenes (`CAMERA3D`, `LIGHT3D`, `WIREFRAME3D`, `BEGIN3D`, `ADD3D`,
`DRAW3D`, `PROJECT3D`, `MAT3D`, `MODEL3D`) son las funciones de esta
página con las coordenadas en `FIXED` y los ángulos en vueltas de 65536;
las matrices del modelo no se copian y tienen que ser globales. Su
referencia está en la [página de ZX Basic](../zxbasic.md). Un ejemplo
entero, con doble buffer: [Nave 3D](../../../examples/zxbasic/nave3d/README.es.md).
