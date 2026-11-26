**English** · [Español](../../es/sdk/libraries.md)

# The SDK libraries

Besides the ROM routines, the SDK comes with **optional libraries** in
`sdk/lib` for what nearly every game ends up writing: fixed-point
arithmetic, moving things through a map without going through walls, the
keyboard and the gamepad at once, saving high scores, fonts, 3D... They are
not in the ROM: a program only carries the ones it uses, and what it does
not use takes no space. All of them are plain C (MIT licence, like the SDK)
and give the same results in the emulator and on the machine.

To use one, include its header and add its files to your program's. In the
Makefile from [Your first program](README.md#a-makefile-for-your-project):

```make
SOURCES := juego.c $(SDK)/lib/math.c $(SDK)/lib/input.c $(SDK)/lib/save.c
```

The headers are in `sdk/include`, the same folder as the system ones, so
no extra `-I` is needed.

The SDK examples use them, so they work as samples: the keyboard and the
gamepad with `zxv_input` and the high score with `zxv_save` in Estela, The
Mine, The Keep, Route 128, Umbra and Street Justice (each one, in the SDK
`Makefile`, with its own); Gone Silent, the keyboard and gamepad also with
`zxv_input` (its sixteen actions, as many as fit) and the saved game with
`zxv_save`; the square roots and angles of `zxv_math` in
Estela, Street Justice and Gone Silent; and the floor cones of `graphics`,
with `zxv_m7_project`.

| Library | Header | Files to add | What for |
|---|---|---|---|
| [Maths](#maths-zxv_mathh) | `zxv_math.h` | `lib/math.c` | 16.16 fixed point, sine and cosine, atan2, square roots, random numbers |
| [Objects on mode 7](#objects-on-the-mode-7-floor-zxv_m7h) | `zxv_m7.h` | `lib/m7.c`, `lib/math.c` | Where and at what size a point on the floor is seen |
| [Moving through a map](#moving-through-a-tile-map-zxv_mapmoveh) | `zxv_mapmove.h` | `lib/mapmove.c` | Bumping into walls, platforms, ladders |
| [Fades](#palette-fades-zxv_fadeh) | `zxv_fade.h` | `lib/fade.c` | Taking the palette to black, to white or to other colours |
| [Input](#keyboard-and-gamepads-at-once-zxv_inputh) | `zxv_input.h` | `lib/input.c` | Keyboard and gamepads as actions, with redefinable keys |
| [Saving](#saving-data-zxv_saveh) | `zxv_save.h` | `lib/save.c` | High scores, options and saved games, not lost if the power goes |
| [Decompressing](#decompressing-zxv_inflateh) | `zxv_inflate.h` | `lib/inflate.c` | gzip-compressed data inside the program |
| [Fonts](#fonts-zxv_fonth) | `zxv_font.h` | `lib/font.c`, `lib/font_rom.c` | Text of any size and colour, proportional |
| [3D](#simple-3d-zxv_3dh) | `zxv_3d.h` | `lib/3d.c`, `lib/math.c` | Flat-faced models with lighting and pattern shading |

As they are not in the ROM, the entries on this page have no "Since": the
library goes inside your program. All of them work with a ROM of
specification 0.1, except the in-between shades of the 3D one, which use
[`zxv_gfx_pattern`](reference.md#shapes) (specification 0.2: with an
older ROM, those faces come out in the light colour, with no pattern).

In [ZX Basic](../zxbasic.md) they are too: `zxvinput.bas`,
`zxvsave.bas`, `zxvfade.bas`, `zxvfont.bas`, `zxvinflate.bas`, `zxvmath.bas`,
`zxvmapmove.bas`, `zxvm7.bas` and `zxv3d.bas`, with the same C.

And three tools for the PC, in `sdk/tools`:
[`fontconv.py`](#fonts-from-elsewhere-fontconvpy) (fonts from other
formats), [`obj2c.py`](#models-from-blender-obj2cpy) (3D models) and, for
font sheets and textures, the usual `png2gfx.py`.

**How to read the entries.** Hardly any of these functions return error
codes: with a value out of range, most of them clip it or do nothing, and
each entry says which. The ones that do return them (`zxv_save`,
`zxv_load` and the decompressing ones) give them as negative numbers, like
the ROM's file functions. A `NULL` pointer where the entry does not say it
is accepted hangs the program like any other invalid pointer.

## Maths: `zxv_math.h`

The processor has no floating point: `float` works, but it is slow. Games
do their arithmetic in **fixed point**: an `int32_t` in which 65536 is 1
(16.16). `zxv_fix` is that type. **Angles** are in turns of 0x10000:
0x4000 is a quarter of a turn, as in the graphics engine.

```make
SOURCES := juego.c $(SDK)/lib/math.c
```

The macros, `zxv_fmul` and `zxv_fdiv` are entirely in the header: if you
only use those, `lib/math.c` is not needed.

```c
#include "zxv_math.h"

/* An enemy that goes towards the player at 1.5 pixels per frame. */
static zxv_fix ex, ey;                          /* its position, in 16.16 */

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

A 16.16 fixed-point number: the top 16 bits are the integer part (signed)
and the bottom 16, the fraction. 65536 (`ZXV_FIX_ONE`) is 1; 32768, a
half.

| Range | Step |
|---|---|
| from −32768 to 32767.99998 | 1/65536 |

It is added, subtracted and compared like an integer; to multiply and
divide two `zxv_fix`, [`zxv_fmul`](#zxv_fmul) and [`zxv_fdiv`](#zxv_fdiv).
Multiplying and dividing by an integer is done directly (`3 * v`, `v / 2`).
Nothing checks whether a calculation goes out of range: if it does, it
wraps round (from 32767 to −32768), like any `int32_t`.

**Example**

```c
#include "zxv_api.h"
#include "zxv_math.h"

static zxv_fix x = ZXV_FIX(10);                 /* 10 pixels */
static const zxv_fix vx = ZXV_FIX(0.75);        /* 0.75 pixels per frame */

void step(void)
{
    x += vx;                                    /* added like an integer */
    zxv_plot(ZXV_FIX_INT(x), 100, 7);           /* the pixel it falls on */
}
```

**See also:** [`ZXV_FIX`, `ZXV_FIX_ONE`](#zxv_fix-zxv_fix_one),
[`ZXV_FIX_INT`, `ZXV_FIX_ROUND`](#zxv_fix_int-zxv_fix_round).

### `ZXV_FIX`, `ZXV_FIX_ONE`

```c
#define ZXV_FIX_ONE  0x10000
#define ZXV_FIX(n)   ((zxv_fix)((n) * 65536))
```

`ZXV_FIX_ONE` is 1 in 16.16. `ZXV_FIX(n)` turns a number into 16.16. With
a floating-point constant (`ZXV_FIX(1.5)`, `ZXV_FIX(-0.25)`), the compiler
does the arithmetic: no floating point is left in the program.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | from −32768 to 32767.99998 | The number. Preferably a constant. |

**Returns:** `n` · 65536, with the decimals that do not fit cut towards
zero (`ZXV_FIX(0.00001)` is 0).

**Out of range:** with `n` out of range the result is not valid (it
overflows). With an integer variable it works (it is `n` · 65536); with a
`float` variable too, but it does the arithmetic in floating point, which
is slow: do not use it like that in the game loop.

**Example**

```c
#include "zxv_math.h"

static const zxv_fix gravity = ZXV_FIX(0.25);
static const zxv_fix jump = -ZXV_FIX(4);

zxv_fix tile_to_fix(int tile) { return ZXV_FIX(tile * 8); }   /* with an integer */
zxv_fix half(void) { return ZXV_FIX_ONE / 2; }
zxv_fix fall(zxv_fix vy) { return vy > jump ? vy + gravity : vy; }
```

**See also:** [`zxv_fix`](#zxv_fix),
[`ZXV_FIX_INT`, `ZXV_FIX_ROUND`](#zxv_fix_int-zxv_fix_round).

### `ZXV_FIX_INT`, `ZXV_FIX_ROUND`

```c
#define ZXV_FIX_INT(f)    ((int32_t)(f) >> 16)
#define ZXV_FIX_ROUND(f)  ((int32_t)((f) + 0x8000) >> 16)
```

They turn a `zxv_fix` into an integer: `ZXV_FIX_INT` rounding down
(towards −∞, not towards zero) and `ZXV_FIX_ROUND` to the nearest, with
halves going up.

| `f` | `ZXV_FIX_INT(f)` | `ZXV_FIX_ROUND(f)` |
|---|---|---|
| 2.5 | 2 | 3 |
| 2.4 | 2 | 2 |
| −1.5 | −2 | −1 |
| −0.2 | −1 | 0 |

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | a `zxv_fix` | The number. |

**Returns:** the integer, from −32768 to 32767.

**Out of range:** `ZXV_FIX_ROUND` overflows with `f` from 32767.5 upwards
(it gives a negative number).

**Example**

```c
#include "zxv_api.h"
#include "zxv_math.h"

void show(zxv_fix x, zxv_fix y)
{
    zxv_plot(ZXV_FIX_ROUND(x), ZXV_FIX_ROUND(y), 7);
}
```

**See also:** [`zxv_fix`](#zxv_fix),
[`ZXV_FIX`, `ZXV_FIX_ONE`](#zxv_fix-zxv_fix_one).

### `ZXV_TURN`, `ZXV_DEG`

```c
#define ZXV_TURN    0x10000
#define ZXV_DEG(d)  ((int32_t)((d) * 65536 / 360))
```

The angles of these libraries (and of the graphics engine) are integers in
turns of 0x10000 (`ZXV_TURN`): 0x4000 is a quarter of a turn, 0x8000 half
a turn. 0 points towards increasing x and 0x4000 towards increasing y,
which on the screen is downwards: angles grow clockwise. Only the bottom 16
bits count, so an angle can go past a turn or be negative and nothing
happens: 0x14000 and −0xC000 are the same as 0x4000. In
[3D](#simple-3d-zxv_3dh), y goes upwards and each angle says which way it
turns.

`ZXV_DEG(d)` turns degrees into those units, with the decimals cut towards
zero: `ZXV_DEG(90)` is 0x4000 and `ZXV_DEG(1)`, 182 (from 182.04).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `d` | from −32767 to 32767 with integers; any with floating point | The degrees. |

**Returns:** the angle, in turns of 0x10000.

**Out of range:** with an integer above 32767 (or below −32767), the
multiplication overflows. With `90.0` instead of `90` it does not, and the
compiler does the arithmetic just the same.

**Example**

```c
#include "zxv_math.h"

int32_t turn_right(int32_t angle)
{
    return (angle + ZXV_DEG(5)) & (ZXV_TURN - 1);   /* 5 degrees more, without going past a turn */
}
```

**See also:** [`zxv_sin`](#zxv_sin), [`zxv_atan2`](#zxv_atan2).

### `zxv_fmul`

```c
static inline zxv_fix zxv_fmul(zxv_fix a, zxv_fix b);
```

Multiplies two 16.16 numbers. The arithmetic is done in 64 bits, so it
does not overflow too early: only if the result does not fit in 16.16.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `a`, `b` | any `zxv_fix` | The factors. |

**Returns:** `a` · `b` in 16.16, rounded down (towards −∞) to the
1/65536.

**Out of range:** if the result goes beyond ±32768, its bottom 32 bits are
kept (a number that is not valid). There is no error.

**Example**

```c
#include "zxv_math.h"

zxv_fix brake(zxv_fix speed)
{
    return zxv_fmul(speed, ZXV_FIX(0.95));      /* 5 % less each frame */
}
```

**See also:** [`zxv_fdiv`](#zxv_fdiv), [`zxv_fix`](#zxv_fix).

### `zxv_fdiv`

```c
static inline zxv_fix zxv_fdiv(zxv_fix a, zxv_fix b);
```

Divides two 16.16 numbers, with 64 bits in between.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `a` | any `zxv_fix` | The dividend. |
| `b` | any `zxv_fix` except 0 | The divisor. |

**Returns:** `a` / `b` in 16.16, with the decimals that do not fit cut
towards zero.

**Out of range:** `b` = 0 is not defined: check it first. If the quotient
goes beyond ±32768, its bottom 32 bits are kept.

**Example**

```c
#include "zxv_math.h"

/* How many frames it takes to cover dist at speed v (both in 16.16). */
zxv_fix frames_to(zxv_fix dist, zxv_fix v)
{
    return v ? zxv_fdiv(dist, v) : 0;
}
```

**See also:** [`zxv_fmul`](#zxv_fmul).

### `zxv_sin`

```c
zxv_fix zxv_sin(int32_t angle);
```

The sine of an angle, in 16.16. It comes from a quarter-turn table with
256 steps and interpolation: it is **exactly the same sine as the graphics
engine uses** in [`zxv_m7_camera` and `zxv_m7_affine`](reference.md#mode-7),
so whatever you place with it matches the mode 7 floor.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `angle` | any `int32_t`; the bottom 16 bits count | The angle, in turns of 0x10000 ([`ZXV_DEG`](#zxv_turn-zxv_deg)). |

**Returns:** from −65536 to 65536 (from −1 to 1). `zxv_sin(0x4000)` is
exactly 65536. It is off from the exact sine by 1.7/65536 at most.

**Errors:** none.

**Example**

```c
#include "zxv_math.h"

/* An enemy that bobs up and down 16 pixels around base_y. */
int bob(int base_y, uint32_t frame)
{
    return base_y + ZXV_FIX_ROUND(16 * zxv_sin((int32_t)(frame * 512)));
}
```

**See also:** [`zxv_cos`](#zxv_cos), [`zxv_atan2`](#zxv_atan2).

### `zxv_cos`

```c
zxv_fix zxv_cos(int32_t angle);
```

The cosine of an angle, in 16.16: `zxv_sin(angle + 0x4000)`, with the same
table.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `angle` | any `int32_t`; the bottom 16 bits count | The angle, in turns of 0x10000. |

**Returns:** from −65536 to 65536. With the same precision as
[`zxv_sin`](#zxv_sin).

**Errors:** none.

**Example**

```c
#include "zxv_math.h"

/* The x and y speed of something going at v (16.16) in direction a. */
void velocity(int32_t a, zxv_fix v, zxv_fix *vx, zxv_fix *vy)
{
    *vx = zxv_fmul(v, zxv_cos(a));
    *vy = zxv_fmul(v, zxv_sin(a));
}
```

**See also:** [`zxv_sin`](#zxv_sin).

### `zxv_atan2`

```c
int32_t zxv_atan2(int32_t y, int32_t x);
```

The angle of the vector (x, y): which way to look to go from one point to
another. It is worked out with CORDIC, with integers only.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `y`, `x` | any `int32_t` | The vector, both in the same units (integers or 16.16, it does not matter). Mind the order: `y` first. |

**Returns:** the angle, from 0 to 0xFFFF: 0 towards increasing x, 0x4000
towards increasing y (down on the screen), 0x8000 towards decreasing x and
0xC000 towards decreasing y. `zxv_atan2(0, 0)` is 0.

It is off from the exact angle by 0.52/65536 of a turn at most: it gives
the exact value rounded or, in a few cases (about 3 in 1,000), one more or
one less. It is just as good with very small or very large vectors.

**Errors:** none.

**Example**

```c
#include "zxv_math.h"

/* Turns the turret towards the player, 2 degrees per frame at most. */
int32_t aim(int32_t turret, zxv_fix tx, zxv_fix ty, zxv_fix px, zxv_fix py)
{
    int32_t want = zxv_atan2(py - ty, px - tx);
    int16_t diff = (int16_t)(want - turret);    /* from -0x8000 to 0x7FFF: the short way round */
    if (diff > ZXV_DEG(2))
        diff = ZXV_DEG(2);
    if (diff < -ZXV_DEG(2))
        diff = -ZXV_DEG(2);
    return (turret + diff) & 0xFFFF;
}
```

**See also:** [`zxv_sin`](#zxv_sin), [`zxv_hypot`](#zxv_hypot).

### `zxv_isqrt`

```c
uint32_t zxv_isqrt(uint32_t v);
```

The integer square root of `v`, rounded down: `zxv_isqrt(10)` is 3.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `v` | from 0 to 4,294,967,295 | The number. |

**Returns:** the root, from 0 to 65535.

**Errors:** none.

**Example**

```c
#include "zxv_math.h"

/* The radius of a circle with an area of 'area' pixels (roughly). */
uint32_t radius(uint32_t area) { return zxv_isqrt(area * 100 / 314); }
```

**See also:** [`zxv_isqrt64`](#zxv_isqrt64), [`zxv_fsqrt`](#zxv_fsqrt),
[`zxv_hypot`](#zxv_hypot).

### `zxv_isqrt64`

```c
uint32_t zxv_isqrt64(uint64_t v);
```

Like [`zxv_isqrt`](#zxv_isqrt), with a 64-bit number: for the sum of two
large squares, for example.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `v` | any `uint64_t` | The number. |

**Returns:** the root rounded down, from 0 to 4,294,967,295.

**Errors:** none.

**Example**

```c
#include "zxv_math.h"

/* The distance between two points in 16.16 (without overflowing). */
zxv_fix dist(zxv_fix ax, zxv_fix ay, zxv_fix bx, zxv_fix by)
{
    int64_t dx = (int64_t)bx - ax, dy = (int64_t)by - ay;
    return (zxv_fix)zxv_isqrt64((uint64_t)(dx * dx) + (uint64_t)(dy * dy));
}
```

**See also:** [`zxv_hypot`](#zxv_hypot), which does this same calculation.

### `zxv_fsqrt`

```c
zxv_fix zxv_fsqrt(zxv_fix v);
```

The square root of a 16.16 number, in 16.16.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `v` | from 0 to 32767.99998 | The number. |

**Returns:** the root, rounded down to the 1/65536: `zxv_fsqrt(ZXV_FIX(2))`
is 92681 (1.41421). At most, 181.02.

**Out of range:** with `v` 0 or negative, it gives 0.

**Example**

```c
#include "zxv_math.h"

/* The speed to reach a height h (16.16) with gravity g: sqrt(2·g·h). */
zxv_fix jump_speed(zxv_fix g, zxv_fix h) { return zxv_fsqrt(2 * zxv_fmul(g, h)); }
```

**See also:** [`zxv_isqrt`](#zxv_isqrt).

### `zxv_hypot`

```c
uint32_t zxv_hypot(int32_t x, int32_t y);
```

The length of the vector (x, y), √(x² + y²), without overflowing even if
`x` and `y` are large (the arithmetic is done in 64 bits).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x`, `y` | any `int32_t` | The vector, in whatever units (integers or 16.16). |

**Returns:** the length rounded down, in the same units as `x` and `y`,
from 0 to 3,037,000,499. With two `zxv_fix`, the length is 16.16 too
(turn it into a `zxv_fix` if it is less than 32768).

**Errors:** none.

**Example**

```c
#include "zxv_math.h"

/* 1 if the shot (in 16.16) is less than 6 pixels from the enemy. */
int hit(zxv_fix sx, zxv_fix sy, zxv_fix ex, zxv_fix ey)
{
    return zxv_hypot(ex - sx, ey - sy) < (uint32_t)ZXV_FIX(6);
}
```

**See also:** [`zxv_atan2`](#zxv_atan2), [`zxv_isqrt64`](#zxv_isqrt64).

### `zxv_srand`

```c
void zxv_srand(uint32_t seed);
```

Sets the seed of [`zxv_rand`](#zxv_rand) and
[`zxv_rand_range`](#zxv_rand_range). The same seed always gives the same
sequence of numbers: it is useful to repeat a game or a generated level.

**Without calling it, the sequence always starts the same** (the initial
state is fixed): the game would be the same every time. A good seed is
[`zxv_time_us()`](reference.md#zxv_time_us) when the player presses a key
to start.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `seed` | any `uint32_t` | The seed. 0 works like 1. |

**Errors:** none.

**Example**

```c
#include "zxv_api.h"
#include "zxv_math.h"

void wait_start(void)
{
    while (zxv_key_get() < 0)
        zxv_wait_vblank();
    zxv_srand(zxv_time_us());                   /* every game, different */
}
```

**See also:** [`zxv_rand`](#zxv_rand), [`zxv_rand_next`](#zxv_rand_next).

### `zxv_rand`

```c
uint32_t zxv_rand(void);
```

The next random number (xorshift32: fast, 32-bit, and it does not repeat
until 4,294,967,295 have gone by).

**Returns:** from 1 to 4,294,967,295 (never 0).

**Errors:** none.

**Example**

```c
#include "zxv_math.h"

int coin(void) { return (int)(zxv_rand() >> 31); }            /* 0 or 1 */
int chance(int percent) { return zxv_rand_range(1, 100) <= percent; }
```

**See also:** [`zxv_rand_range`](#zxv_rand_range), [`zxv_srand`](#zxv_srand).

### `zxv_rand_range`

```c
int zxv_rand_range(int lo, int hi);
```

A random number between `lo` and `hi`, both included, all with the same
probability. It uses the same generator as [`zxv_rand`](#zxv_rand).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `lo`, `hi` | any `int` | The ends. If `hi` is less than `lo`, they are swapped. |

**Returns:** the number, from `lo` to `hi`. With `INT32_MIN` and
`INT32_MAX`, any `int`.

**Errors:** none.

**Example**

```c
#include "zxv_math.h"

int roll_dice(void) { return zxv_rand_range(1, 6); }
```

**See also:** [`zxv_rand`](#zxv_rand), [`zxv_srand`](#zxv_srand).

### `zxv_rand_next`

```c
uint32_t zxv_rand_next(uint32_t *state);
```

Another generator, with its state in your variable instead of
[`zxv_rand`](#zxv_rand)'s. It is useful to have independent sequences: one
to generate the level (always the same with the same seed) and another for
the rest of the game, which does not throw it off.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `state` | pointer to a `uint32_t` | The state: put the seed in it at the start. If it is 0, it is taken as 1. |

**Returns:** the next number, from 1 to 4,294,967,295, which is also left
in `*state`.

**Errors:** none.

**Example**

```c
#include "zxv_math.h"

/* Level n is always generated the same. */
void make_level(uint8_t *cells, int n_cells, uint32_t level)
{
    uint32_t st = 12345 + level;
    for (int i = 0; i < n_cells; i++)
        cells[i] = (uint8_t)(zxv_rand_next(&st) % 4);
}
```

**See also:** [`zxv_rand`](#zxv_rand).

## Objects on the mode 7 floor: `zxv_m7.h`

With the same camera that [`zxv_m7_camera`](reference.md#mode-7)
works out the floor with (see [Mode 7](graphics.md#mode-7)),
`zxv_m7_project` tells where a point on that floor (or at some height
above it) is seen and at what size: for the cars, trees and cones of a
racing game, drawn with [`zxv_spr_draw_scaled`](reference.md#sprites). And
`zxv_m7_floor_at` does the opposite: which point of the floor is at a
pixel of the screen.

```make
SOURCES := juego.c $(SDK)/lib/m7.c $(SDK)/lib/math.c
```

The header includes `zxv_gfx.h` (for `zxv_m7cam`) and `zxv_math.h`. The
arithmetic is the graphics engine's, so everything matches what it draws
to the pixel. Remember: in `zxv_m7cam`, everything is in 16.16 (`focal`
too) except `horizon`, which is the whole screen line.

For some objects to hide others, **draw them from far to near**, by their
`depth`.

### `zxv_m7pos`

```c
typedef struct {
    int32_t x, y;
    zxv_fix scale;
    zxv_fix depth;
} zxv_m7pos;
```

Where a point is seen, what [`zxv_m7_project`](#zxv_m7_project) fills in.

| Field | Values | What it is |
|---|---|---|
| `x` | pixels (integers) | The column of the target. |
| `y` | pixels (integers) | The line of the target. For a point on the floor, the line it stands on: draw the object with its base there. |
| `scale` | 16.16, greater than 0 | The pixels a texel measures at that distance: the object's size in texels times `scale` gives its size in pixels. |
| `depth` | 16.16, from 1 texel to 32767.99998 | How far away it is, forwards (the distance perpendicular to the screen). For sorting. |

**See also:** [`zxv_m7_project`](#zxv_m7_project).

### `zxv_m7_project`

```c
int zxv_m7_project(const zxv_m7cam *cam, int width, zxv_fix px, zxv_fix py, zxv_fix h, zxv_m7pos *p);
```

Where the point (`px`, `py`) of the floor, at height `h` above it, is seen
with the camera `cam`, and at what scale. It rounds to the nearest pixel.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `cam` | the camera from `zxv_m7_camera` | The same one the floor is worked out with. `focal` must be greater than 0. |
| `width` | pixels | The width of the target the floor is drawn in (256 in mode 0): the camera is at its column `width / 2`. |
| `px`, `py` | texels, 16.16 | The point on the floor. |
| `h` | texels, 16.16 | The height above the floor: 0, standing on it; more for what jumps or flies. At the height of the camera (`cam->height`) it falls on the horizon. |
| `p` | pointer to a `zxv_m7pos` | Where the result is left. |

**Returns:** 1 if the point is in front of the camera, and then it fills
in `*p`; 0 if not, and `*p` does not change. A point with 1 may fall
outside the screen (to the side, or below): the sprite is clipped by
itself when drawn.

It gives 0 when:

| Case | Why |
|---|---|
| The point is behind the camera or less than a texel in front of it | It cannot be seen there (or it would come out huge). |
| The position goes beyond ±2³⁰ pixels or `scale` beyond 32768 | Too close to draw it. |
| `cam` or `p` are `NULL`, or `cam->focal` is 0 or negative | There is no camera. |

**Example**

```c
#include "zxv_m7.h"

/* A tree 32 texels tall and 16 wide, standing at (tx, ty). */
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

**See also:** [`zxv_m7pos`](#zxv_m7pos), [`zxv_m7_floor_at`](#zxv_m7_floor_at),
[`zxv_m7_camera`](reference.md#mode-7).

### `zxv_m7_floor_at`

```c
int zxv_m7_floor_at(const zxv_m7cam *cam, int width, int sx, int sy, zxv_fix *px, zxv_fix *py);
```

The opposite of [`zxv_m7_project`](#zxv_m7_project): the point of the
floor seen at the pixel (`sx`, `sy`) of the target. It is exactly the texel
that `zxv_m7_draw` draws with the lines from `zxv_m7_camera`. It is useful
for the mouse, or to know what a car is driving on.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `cam` | the camera from `zxv_m7_camera` | The same one the floor is worked out with. |
| `width` | pixels | The width of the target, as in `zxv_m7_project`. |
| `sx` | pixels | The column. It is not checked: outside the target it gives the point that would be seen if it carried on. |
| `sy` | pixels, below `cam->horizon` | The line. |
| `px`, `py` | pointers, or `NULL` | Where the point is left, in 16.16 texels. Whichever is `NULL` is not filled in. |

**Returns:** 1 and the point; or 0 if `sy` is on the horizon or above it
(there is no floor there), if `cam` is `NULL` or if `cam->focal` is 0 or
negative. With 0, `*px` and `*py` do not change.

The point is not fitted to the size of the texture: if it repeats, the
texel is `(px >> 16) & (side - 1)`.

**Example**

```c
#include "zxv_m7.h"

/* The texel of the track under the car (drawn at column 128, line 170). */
int under_car(const zxv_m7cam *cam, const uint8_t *track, int log2size)
{
    zxv_fix u, v;
    if (!zxv_m7_floor_at(cam, 256, 128, 170, &u, &v))
        return -1;
    int mask = (1 << log2size) - 1;
    return track[(((v >> 16) & mask) << log2size) + ((u >> 16) & mask)];
}
```

**See also:** [`zxv_m7_project`](#zxv_m7_project).

## Moving through a tile map: `zxv_mapmove.h`

The core of a platform game: moving a rectangle through a
[tile map](graphics.md#tile-maps), stopping against walls, without going
through them however fast it goes, and knowing whether it is on the
ground. The game says what **kind** each tile is (empty, solid, platform
or one of its own: ladder, spikes, water, coins).

```make
SOURCES := juego.c $(SDK)/lib/mapmove.c
```

The header includes `zxv_gfx.h` (for `zxv_tilemap`) and `zxv_math.h` (for
`zxv_fix`), but `lib/math.c` is not needed.

**The measurements.** Positions are in map pixels, in 16.16: (0, 0) is the
top left corner of cell (0, 0). A rectangle with its corner at (x, y) and
w × h pixels covers from x to x + w (not included) and from y to y + h.
`w` and `h` are integers; less than 1 counts as 1. Of each cell only the
tile number counts (the bottom 12 bits): mirroring and priority make no
difference.

**Invalid map.** If `ms` or `ms->map` or its `cells` are `NULL`, if the
map is 0 wide or 0 tall, or if its tiles are neither 8 nor 16, the
functions do nothing: `zxv_map_move` does not move and gives 0,
`zxv_map_on_ground` and `zxv_map_find` give 0 and `zxv_map_kind`,
`ZXV_TILE_EMPTY`.

```c
#include "zxv_input.h"
#include "zxv_mapmove.h"

enum { LADDER = 3, COIN = 4 };
extern const zxv_tilemap level;
static const uint8_t kinds[] = { ZXV_TILE_EMPTY, ZXV_TILE_SOLID, ZXV_TILE_PLATFORM, LADDER, COIN };
static zxv_mapsolid ms = { &level, kinds, sizeof kinds, ZXV_TILE_SOLID };   /* outside the map, wall */

static zxv_fix px, py, vy;                      /* position and vertical speed, 16.16 */

void take_coin(int cx, int cy);                 /* the game's: removes the coin from the map */

void player_step(const zxv_input *in)
{
    zxv_fix vx = 0;
    if (in->held & ZXV_IN_LEFT)
        vx = -ZXV_FIX(1.5);
    if (in->held & ZXV_IN_RIGHT)
        vx = ZXV_FIX(1.5);
    unsigned through = (in->held & ZXV_IN_DOWN) ? ZXV_MOVE_THROUGH : 0;
    if (zxv_map_on_ground(&ms, px, py, 12, 16, through) && (in->pressed & ZXV_IN_FIRE))
        vy = -ZXV_FIX(4);                       /* jump */
    vy += ZXV_FIX(0.25);                        /* gravity */
    unsigned hit = zxv_map_move(&ms, &px, &py, 12, 16, vx, vy, through);
    if (hit & (ZXV_HIT_DOWN | ZXV_HIT_UP))
        vy = 0;
    int cx, cy;
    if (zxv_map_find(&ms, px, py, 12, 16, COIN, &cx, &cy))
        take_coin(cx, cy);
}
```

There are no slopes or diagonal tiles.

### `ZXV_TILE_EMPTY`, `ZXV_TILE_SOLID`, `ZXV_TILE_PLATFORM`

The kinds of tile the library understands. The game gives one to each tile
number with the `kinds` table of [`zxv_mapsolid`](#zxv_mapsolid).

| Kind | Value | What it does |
|---|---|---|
| `ZXV_TILE_EMPTY` | 0 | Nothing: it can be gone through. |
| `ZXV_TILE_SOLID` | 1 | Stops on all four sides. |
| `ZXV_TILE_PLATFORM` | 2 | Only stops when falling from above: you can go through it jumping from below and from the side, and going down with `ZXV_MOVE_THROUGH`. |
| 3 to 255 | | The game's own (ladders, spikes, water, coins): they do not stop anything, but [`zxv_map_find`](#zxv_map_find) and [`zxv_map_kind`](#zxv_map_kind) find them. |

**See also:** [`zxv_mapsolid`](#zxv_mapsolid).

### `zxv_mapsolid`

```c
typedef struct {
    const zxv_tilemap *map;
    const uint8_t *kinds;
    uint16_t nkinds;
    uint8_t outside;
} zxv_mapsolid;
```

The map with the kinds of its tiles: what all the functions take. It is
only read, so it can be `const` and shared by all the characters.

| Field | Values | What it is |
|---|---|---|
| `map` | a `zxv_tilemap` | The map ([Types](reference.md#types)): its cells, its size, tiles of 8 or 16 and `ZXV_MAP_WRAP` if it repeats. It is the same one that is drawn with `zxv_map_draw`. |
| `kinds` | table of `uint8_t`, or `NULL` | The kind of each tile, by its number (0 to 4095). `NULL`: tile 0 is empty and all the others are solid. |
| `nkinds` | 0 to 65535 | How many kinds there are in `kinds`. Tiles with a number greater than or equal to it are empty. |
| `outside` | a kind, 0 to 255 | The kind of what lies outside a map that does not repeat: `ZXV_TILE_SOLID` to make the edges walls, `ZXV_TILE_EMPTY` so that you can leave it. In a map that repeats it does not count. |

**Example**

```c
#include "zxv_mapmove.h"

extern const zxv_tilemap cave;
/* No table: tile 0 empty and the rest solid; the edges, open. */
const zxv_mapsolid cave_solid = { &cave, 0, 0, ZXV_TILE_EMPTY };
```

**See also:** [`ZXV_TILE_EMPTY`, `ZXV_TILE_SOLID`, `ZXV_TILE_PLATFORM`](#zxv_tile_empty-zxv_tile_solid-zxv_tile_platform).

### `ZXV_HIT_LEFT`, `ZXV_HIT_RIGHT`, `ZXV_HIT_UP`, `ZXV_HIT_DOWN`

The bits of what [`zxv_map_move`](#zxv_map_move) returns: the sides of the
rectangle that collided.

| Constant | Value | What happened |
|---|---|---|
| `ZXV_HIT_LEFT` | 0x01 | It collided on the left (it was going left). |
| `ZXV_HIT_RIGHT` | 0x02 | It collided on the right. |
| `ZXV_HIT_UP` | 0x04 | It hit its head. |
| `ZXV_HIT_DOWN` | 0x08 | It collided with its feet: it is on the ground. |

**See also:** [`zxv_map_move`](#zxv_map_move).

### `ZXV_MOVE_THROUGH`

```c
#define ZXV_MOVE_THROUGH  0x01u
```

The option of [`zxv_map_move`](#zxv_map_move) and
[`zxv_map_on_ground`](#zxv_map_on_ground) to go through platforms when
going down: to get off one (down + jump, in many games). With it,
platforms stop nothing.

**See also:** [`ZXV_TILE_EMPTY`, `ZXV_TILE_SOLID`, `ZXV_TILE_PLATFORM`](#zxv_tile_empty-zxv_tile_solid-zxv_tile_platform).

### `zxv_map_move`

```c
unsigned zxv_map_move(const zxv_mapsolid *ms, zxv_fix *x, zxv_fix *y, int w, int h,
                      zxv_fix dx, zxv_fix dy, unsigned flags);
```

Moves the rectangle first `dx` horizontally and then `dy` vertically (from
the new x), stopping each axis at the first cell that blocks it: it is
left touching it, without overlapping. It looks at every cell it goes
through, so **it does not go through any wall however fast it goes** (but
the further it goes, the more cells it looks at).

- Platforms only stop it when going down, and only the rows that were
  wholly below its feet before moving: "coming from above".
- If the rectangle was already inside something solid, the cells it
  covers do not stop it: it comes out the way it is moving.
- The speed is the game's: when it collides, set it to 0 yourself
  (`vy = 0` with `ZXV_HIT_DOWN` or `ZXV_HIT_UP`).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ms` | a `zxv_mapsolid` | The map. |
| `x`, `y` | pointers to the top left corner, 16.16 | Where it is; they are changed to where it ends up. |
| `w`, `h` | pixels (integers); less than 1 is 1 | The size of the rectangle. |
| `dx`, `dy` | 16.16, any value | How far it moves in this step. |
| `flags` | 0 or `ZXV_MOVE_THROUGH` | `ZXV_MOVE_THROUGH`: go through platforms. |

**Returns:** the `ZXV_HIT_` of the sides that collided, together; 0 if it
did not collide with anything. When it collides, the coordinate of that
axis is left exactly on the edge of the cell (with no fraction).

**Errors:** none. With an [invalid map](#moving-through-a-tile-map-zxv_mapmoveh)
or with `x` or `y` `NULL`, it moves nothing and gives 0.

**Example**

```c
#include "zxv_mapmove.h"

/* A ball that bounces off the walls. */
void ball_step(const zxv_mapsolid *ms, zxv_fix *x, zxv_fix *y, zxv_fix *vx, zxv_fix *vy)
{
    unsigned hit = zxv_map_move(ms, x, y, 6, 6, *vx, *vy, 0);
    if (hit & (ZXV_HIT_LEFT | ZXV_HIT_RIGHT))
        *vx = -*vx;
    if (hit & (ZXV_HIT_UP | ZXV_HIT_DOWN))
        *vy = -*vy;
}
```

**See also:** [`zxv_map_on_ground`](#zxv_map_on_ground),
[`ZXV_HIT_LEFT`...](#zxv_hit_left-zxv_hit_right-zxv_hit_up-zxv_hit_down),
[`ZXV_MOVE_THROUGH`](#zxv_move_through).

### `zxv_map_on_ground`

```c
int zxv_map_on_ground(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y, int w, int h, unsigned flags);
```

Tells whether the rectangle is standing on something, without moving it:
whether its feet are exactly on the top edge of a cell that stops it when
going down (solid, or a platform without `ZXV_MOVE_THROUGH`). It is how it
is left after [`zxv_map_move`](#zxv_map_move) stops it when falling; with
its feet halfway through a cell (falling, or on a ladder) it gives 0.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ms` | a `zxv_mapsolid` | The map. |
| `x`, `y` | 16.16 | The top left corner. |
| `w`, `h` | pixels; less than 1 is 1 | The size. |
| `flags` | 0 or `ZXV_MOVE_THROUGH` | With `ZXV_MOVE_THROUGH`, platforms do not count. |

**Returns:** 1 if it is standing on something; 0 if not (or with an
invalid map).

**Errors:** none.

**Example**

```c
#include "zxv_mapmove.h"

/* You can only jump from the ground. */
zxv_fix try_jump(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y, zxv_fix vy)
{
    return zxv_map_on_ground(ms, x, y, 12, 16, 0) ? -ZXV_FIX(4) : vy;
}
```

**See also:** [`zxv_map_move`](#zxv_map_move).

### `zxv_map_find`

```c
int zxv_map_find(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y, int w, int h, int kind, int *cx, int *cy);
```

Looks for a cell of kind `kind` among those the rectangle touches: for
ladders, water, spikes or coins. It looks at them from top to bottom and,
in each row, from left to right, and keeps the first one.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ms` | a `zxv_mapsolid` | The map. |
| `x`, `y` | 16.16 | The top left corner. |
| `w`, `h` | pixels; less than 1 is 1 | The size. |
| `kind` | 0 to 255 | The kind it looks for (the library's too: `ZXV_TILE_SOLID`...). |
| `cx`, `cy` | pointers, or `NULL` | Where the cell found is left (column and row). Whichever is `NULL` is not filled in. |

**Returns:** 1 if it finds it, with its cell in `*cx`, `*cy`; 0 if not (or
with an invalid map), and then they do not change. In a map that repeats,
the cell is always inside the map (0 to width − 1). In one that does not
repeat, if `kind` is the `outside` kind, the cell may be outside (negative,
or beyond the edge).

**Errors:** none.

**Example**

```c
#include "zxv_mapmove.h"

enum { SPIKES = 5 };

int touching_spikes(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y)
{
    return zxv_map_find(ms, x, y, 12, 16, SPIKES, 0, 0);
}
```

**See also:** [`zxv_map_kind`](#zxv_map_kind).

### `zxv_map_kind`

```c
int zxv_map_kind(const zxv_mapsolid *ms, int px, int py);
```

The kind of the cell at a pixel of the map.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ms` | a `zxv_mapsolid` | The map. |
| `px`, `py` | **integer** pixels (not 16.16) | The point. It may be outside the map. |

**Returns:** the kind, 0 to 255. Outside a map that does not repeat, the
`outside` kind; in one that repeats, that of the cell it falls on. With an
invalid map, `ZXV_TILE_EMPTY`.

**Errors:** none.

**Example**

```c
#include "zxv_mapmove.h"

enum { WATER = 6 };

/* Is its head under water? (x, y in 16.16; 12 wide) */
int underwater(const zxv_mapsolid *ms, zxv_fix x, zxv_fix y)
{
    return zxv_map_kind(ms, ZXV_FIX_INT(x) + 6, ZXV_FIX_INT(y)) == WATER;
}
```

**See also:** [`zxv_map_find`](#zxv_map_find).

## Palette fades: `zxv_fade.h`

They take the colours of [the palettes](graphics.md#colour-remapping) to
others over a few frames: to black at the end of a level, from black to
its colours at the start of the next, a white flash when hit. The pixels
are not touched: only how they look changes, so it costs nothing in
drawing. It changes the colours of the 4 palette tables (16 RGB565 colours
each, see [Palette](reference.md#palette)); the remapping tables, not.

```make
SOURCES := juego.c $(SDK)/lib/fade.c
```

```c
#include "zxv.h"                                /* ZXV_RGB */
#include "zxv_api.h"
#include "zxv_fade.h"

void load_level(int n);                         /* the game's */
void game_frame(void);

void next_level(int n)
{
    zxv_fade_to_colour(ZXV_RGB(0, 0, 0), 30, ZXV_FADE_ALL);  /* to black in half a second */
    zxv_fade_wait();
    load_level(n);
    zxv_fade_to(NULL, 30, ZXV_FADE_ALL);                     /* to the usual colours */
    while (zxv_fade_busy()) {
        zxv_fade_step();                                     /* one step per frame... */
        game_frame();                                        /* ...while the game goes on */
        zxv_wait_vblank();
    }
}
```

- **One fade at a time:** starting another replaces whichever there was,
  and it starts from the colours where that one had got to. A fade and the
  one back go through the same colours.
- The machine has a single [frame hook](README.md#the-frame-hook) and
  music usually uses it, so the library does not take it: call
  `zxv_fade_step()` once per frame, in your loop or in your hook.
- `zxv_fade.h` does not include `zxv.h`: for `ZXV_RGB` (a colour from r,
  g, b from 0 to 255), include it yourself.

### `ZXV_FADE_ALL`

```c
#define ZXV_FADE_ALL  0x0Fu
```

The four palette tables. The `tables` argument of the fades is a mask:
bit `t` is table `t` (0 to 3). With only some of them, the game fades and
the scoreboard does not, for example (if the scoreboard uses another
table, see [The line table](graphics.md#the-line-table)).

| Value | Tables |
|---|---|
| `0x01` | Only 0. |
| `0x0E` | 1, 2 and 3. |
| `ZXV_FADE_ALL` (0x0F) | All of them. |

**See also:** [`zxv_fade_to`](#zxv_fade_to).

### `zxv_fade_to`

```c
void zxv_fade_to(const uint16_t to[16], int frames, unsigned tables);
```

Starts taking the tables in `tables` to the colours `to`, from the ones
they have now, in `frames` steps of [`zxv_fade_step`](#zxv_fade_step). The
chosen tables all end with the same 16 colours. Until the first step
nothing changes (except with `frames` 0).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `to` | 16 RGB565 colours, or `NULL` | Where it goes. They are copied: it can be a local variable. `NULL`: the usual 16 colours (`ZXV_STD_RGB565`), not those of a `.gfx` you have loaded; for those, pass it their palette. |
| `frames` | 1 or more; 0 or less, at once | How many steps it lasts. With 0 or less, it sets the colours at once and no fade is left going. |
| `tables` | 4-bit mask ([`ZXV_FADE_ALL`](#zxv_fade_all)) | Which tables it changes. Higher bits do not count; with 0 nothing changes, but the fade still "lasts" `frames` steps. |

**Errors:** none.

**Example**

```c
#include "zxv_api.h"
#include "zxv_fade.h"

/* From black to the level's palette (16 colours, for example those of its .gfx). */
void fade_in(const uint16_t level_pal[16])
{
    zxv_fade_to(level_pal, 25, ZXV_FADE_ALL);
    zxv_fade_wait();
}
```

**See also:** [`zxv_fade_to_colour`](#zxv_fade_to_colour),
[`zxv_fade_step`](#zxv_fade_step), [`zxv_rgb_mix`](#zxv_rgb_mix).

### `zxv_fade_to_colour`

```c
void zxv_fade_to_colour(uint16_t rgb, int frames, unsigned tables);
```

Like [`zxv_fade_to`](#zxv_fade_to), with the 16 colours the same: to
black, to white, to red...

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `rgb` | an RGB565 colour | Where they all go: `ZXV_RGB(0, 0, 0)` (or 0), black; `ZXV_RGB(255, 255, 255)`, white. |
| `frames` | 1 or more; 0 or less, at once | How many steps it lasts. |
| `tables` | 4-bit mask | Which tables it changes. |

**Errors:** none.

**Example**

```c
#include "zxv.h"
#include "zxv_fade.h"

/* A white flash when hit: from white to the usual colours in 8 frames. */
void flash(void)
{
    zxv_fade_to_colour(ZXV_RGB(255, 255, 255), 0, ZXV_FADE_ALL);   /* white, at once */
    zxv_fade_to(0, 8, ZXV_FADE_ALL);                              /* and back (0: NULL) */
}
```

**See also:** [`zxv_fade_to`](#zxv_fade_to).

### `zxv_fade_step`

```c
void zxv_fade_step(void);
```

Takes one step of the fade: puts the colours of the next step in the
tables. The last step leaves exactly the target colours. If there is no
fade, it does nothing.

Call it once per frame: in your loop or in your
[frame hook](README.md#the-frame-hook) (it does not wait for anything, so
it can go there).

**Errors:** none.

**Example**

```c
#include "zxv_fade.h"

void music_tick(void);                          /* the game's */

/* The game's frame hook: the music and the fade. */
void frame_hook(void)
{
    music_tick();
    zxv_fade_step();
}
```

**See also:** [`zxv_fade_busy`](#zxv_fade_busy), [`zxv_fade_wait`](#zxv_fade_wait).

### `zxv_fade_busy`

```c
int zxv_fade_busy(void);
```

**Returns:** 1 while there are steps of the fade left; 0 when it has
finished or there is none.

**Errors:** none.

**Example**

```c
#include "zxv_fade.h"

/* You cannot pause while fading. */
int can_pause(void) { return !zxv_fade_busy(); }
```

**See also:** [`zxv_fade_step`](#zxv_fade_step).

### `zxv_fade_wait`

```c
void zxv_fade_wait(void);
```

Waits for the fade to finish: on each VBLANK
([`zxv_wait_vblank`](reference.md#zxv_wait_vblank)) it takes a step. With
no fade, it returns at once. Meanwhile, the game is stopped (the frame
hook, with the music, carries on). **Do not call it from a frame hook:**
it waits for the VBLANK.

**Errors:** none.

**Example**

```c
#include "zxv_fade.h"

void fade_out(void)
{
    zxv_fade_to_colour(0, 30, ZXV_FADE_ALL);    /* 0 is black */
    zxv_fade_wait();
}
```

**See also:** [`zxv_fade_step`](#zxv_fade_step).

### `zxv_rgb_mix`

```c
uint16_t zxv_rgb_mix(uint16_t a, uint16_t b, int k, int n);
```

A colour partway between `a` and `b`: the one the fades use, for your own
effects. Each component (red, green and blue, separately) is
a + (b − a) · k / n, to the nearest, with halves going up.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `a`, `b` | RGB565 colours | The two ends. |
| `k` | 0 to `n` | The step. |
| `n` | 1 or more | The steps from `a` to `b`. |

**Returns:** the RGB565 colour. With `k` 0 or less, `a`; with `k` equal
to or greater than `n`, or with `n` 0 or negative, `b`.

**Errors:** none.

**Example**

```c
#include "zxv.h"
#include "zxv_fade.h"

/* The colour of a life bar: from green (full) to red (empty). */
uint16_t life_colour(int life, int max)
{
    return zxv_rgb_mix(ZXV_RGB(215, 0, 0), ZXV_RGB(0, 215, 0), life, max);
}
```

**See also:** [`zxv_fade_to`](#zxv_fade_to).

## Keyboard and gamepads at once: `zxv_input.h`

The game thinks in **actions** (left, fire, pause: up to 16, one bit
each) and a **map** says which keys (up to 3) and which gamepad buttons do
each one. Once per frame, `zxv_input_update` looks at the keyboard and the
gamepad and says which actions are held, which have just been pressed or
released and for how long. With two maps, two players.

```make
SOURCES := juego.c $(SDK)/lib/input.c
```

It reads the map of [keys held down](README.md#keys-held-down) directly
(by their HID code) and the [gamepads](README.md#joysticks), without
calling the ROM.

```c
#include "zxv_api.h"
#include "zxv_input.h"

void game(void)
{
    zxv_input in;
    int x = 128;
    zxv_input_init(&in, NULL);                  /* the default map */
    for (;;) {
        zxv_wait_vblank();
        zxv_input_update(&in);                  /* once per frame */
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

**Redefining the keys:** [`zxv_input_new_key`](#zxv_input_new_key) gives
the key just pressed and [`zxv_input_key_name`](#zxv_input_key_name) its
name. The map is data: it can be copied, changed and saved with
[`zxv_save`](#zxv_save).

### `ZXV_IN_ACTIONS`, `ZXV_IN_KEYS`

```c
#define ZXV_IN_ACTIONS  16
#define ZXV_IN_KEYS     3
```

`ZXV_IN_ACTIONS`: how many actions there are (16; action `a` is bit
`1 << a`). `ZXV_IN_KEYS`: how many keys each one can have in a map (3).

**See also:** [`zxv_input_map`](#zxv_input_map).

### `ZXV_IN_RIGHT`, `ZXV_IN_LEFT`, `ZXV_IN_DOWN`, `ZXV_IN_UP`, `ZXV_IN_FIRE`, `ZXV_IN_FIRE2`, `ZXV_IN_FIRE3`, `ZXV_IN_START`, `ZXV_IN_SELECT`, `ZXV_IN_ESC`, `ZXV_IN_FIRE4`, `ZXV_IN_L`, `ZXV_IN_R`

The actions of the [default map](#zxv_input_default). The first nine are
the same bits as the gamepad buttons (`ZXV_JOY_...`); then come Esc and
fire 4, L and R (these three only on the gamepad). The bits it does not use
(0x2000 to 0x8000) are for the game, with a map of its own; and a game can
give these whatever meaning it likes.

| Action | Value | Keys | Gamepad button |
|---|---|---|---|
| `ZXV_IN_RIGHT` | 0x0001 | Cursor right, P | `ZXV_JOY_RIGHT` |
| `ZXV_IN_LEFT` | 0x0002 | Cursor left, O | `ZXV_JOY_LEFT` |
| `ZXV_IN_DOWN` | 0x0004 | Cursor down, A | `ZXV_JOY_DOWN` |
| `ZXV_IN_UP` | 0x0008 | Cursor up, Q | `ZXV_JOY_UP` |
| `ZXV_IN_FIRE` | 0x0010 | Space, M | `ZXV_JOY_FIRE1` |
| `ZXV_IN_FIRE2` | 0x0020 | N, Z | `ZXV_JOY_FIRE2` |
| `ZXV_IN_FIRE3` | 0x0040 | B, X | `ZXV_JOY_FIRE3` |
| `ZXV_IN_START` | 0x0080 | Enter | `ZXV_JOY_START` |
| `ZXV_IN_SELECT` | 0x0100 | Tab | `ZXV_JOY_SELECT` |
| `ZXV_IN_ESC` | 0x0200 | Esc | none |
| `ZXV_IN_FIRE4` | 0x0400 | none | `ZXV_JOY_FIRE4` |
| `ZXV_IN_L` | 0x0800 | none | `ZXV_JOY_L` |
| `ZXV_IN_R` | 0x1000 | none | `ZXV_JOY_R` |

The letters are those at their place on a US keyboard (on a Spanish one
they are the same).

**See also:** [`zxv_input_default`](#zxv_input_default).

### `ZXV_IN_KEEP_QUEUE`

```c
#define ZXV_IN_KEEP_QUEUE  0x01u
```

An option of the map (field `flags`). Without it,
[`zxv_input_update`](#zxv_input_update) empties the key queue of
[`zxv_key_get`](reference.md#zxv_key_get) every frame, so that what was
typed during the game does not reach BASIC on the way back. With it, the
queue stays: for a game that also reads keys with `zxv_key_get` (typing a
name, for example).

**Example**

```c
#include "zxv_input.h"

static zxv_input_map typing_map;

/* The default map, without emptying the key queue. */
void make_typing_map(void)
{
    typing_map = zxv_input_default;
    typing_map.flags |= ZXV_IN_KEEP_QUEUE;
}
```

**See also:** [`zxv_input_map`](#zxv_input_map).

### `zxv_input_map`

```c
typedef struct {
    uint8_t keys[ZXV_IN_ACTIONS][ZXV_IN_KEYS];
    uint16_t joy[ZXV_IN_ACTIONS];
    int8_t pad;
    uint8_t flags;
} zxv_input_map;
```

Which keys and which buttons do each action. It is data (82 bytes): it can
be copied, changed and saved.

| Field | Values | What it is |
|---|---|---|
| `keys[a][k]` | HID codes from 1 to 255; 0, none | The keys (up to 3) of action `a`. Any of them does it. |
| `joy[a]` | `ZXV_JOY_...` bits; 0, none | The gamepad buttons that do action `a` (any of them). |
| `pad` | 0 or 1; another (−1), none | The gamepad that is looked at. If it is not connected, only the keys count. |
| `flags` | 0 or `ZXV_IN_KEEP_QUEUE` | Options. |

**Example**

```c
#include "zxv.h"                                /* ZXV_JOY_... */
#include "zxv_input.h"

/* The second player: W A S D, fire with Left Shift, and gamepad 1. */
const zxv_input_map player2 = {
    .keys = {
        [0] = { 0x07 },                         /* ZXV_IN_RIGHT: D */
        [1] = { 0x04 },                         /* LEFT: A */
        [2] = { 0x16 },                         /* DOWN: S */
        [3] = { 0x1A },                         /* UP: W */
        [4] = { 0xE1 },                         /* FIRE: Left Shift */
    },
    .joy = { ZXV_JOY_RIGHT, ZXV_JOY_LEFT, ZXV_JOY_DOWN, ZXV_JOY_UP, ZXV_JOY_FIRE1 },
    .pad = 1,
};
```

`zxv_input.h` does not include `zxv.h`: for the `ZXV_JOY_...`, include it
yourself.

**See also:** [`zxv_input_default`](#zxv_input_default),
[`zxv_input_init`](#zxv_input_init).

### `zxv_input_default`

```c
extern const zxv_input_map zxv_input_default;
```

The default map: cursor keys or Q A O P, fire with Space or M, fire 2 with
N or Z, fire 3 with B or X, Enter for START, Tab for SELECT, Esc, fire 4,
L and R only on the gamepad, and gamepad 0. The table of [the actions](#zxv_in_right-zxv_in_left-zxv_in_down-zxv_in_up-zxv_in_fire-zxv_in_fire2-zxv_in_fire3-zxv_in_start-zxv_in_select-zxv_in_esc-zxv_in_fire4-zxv_in_l-zxv_in_r)
says what each one does. It is the one used with
`zxv_input_init(&in, NULL)`.

It is `const`: to change it, copy it.

**Example**

```c
#include "zxv_input.h"

static zxv_input_map my_map;
static zxv_input in;

/* Like the default one, but with fire on Left Ctrl too. */
void setup_input(void)
{
    my_map = zxv_input_default;
    my_map.keys[4][2] = 0xE0;                   /* FIRE is action 4; its third key */
    zxv_input_init(&in, &my_map);
}
```

**See also:** [`zxv_input_map`](#zxv_input_map).

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

The state of a player. [`zxv_input_init`](#zxv_input_init) sets it up and
[`zxv_input_update`](#zxv_input_update) updates it; the game only reads
it.

| Field | Values | What it is |
|---|---|---|
| `map` | a `zxv_input_map` | The map it uses. |
| `held` | action bits | Those held now. |
| `pressed` | action bits | Those pressed in this frame (they were not in the previous one). |
| `released` | action bits | Those released in this frame. |
| `frames[a]` | 0 to 65535 | How many frames action `a` (that of bit `1 << a`) has been held: 1 in the frame it is pressed, 0 if it is not held. It does not go past 65535. |

**See also:** [`zxv_input_update`](#zxv_input_update),
[`zxv_input_repeat`](#zxv_input_repeat).

### `zxv_input_init`

```c
void zxv_input_init(zxv_input *in, const zxv_input_map *map);
```

Sets up the state of a player with a map, with nothing held. It can be
called again to change the map (what was held is forgotten too).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `in` | pointer to a `zxv_input` | The state. |
| `map` | a map, or `NULL` | The map. `NULL`: [`zxv_input_default`](#zxv_input_default). **It is not copied:** it must go on existing while it is in use (global or `static`, not a local variable that goes away). |

**Errors:** none.

**Example**

```c
#include "zxv_input.h"

extern const zxv_input_map player2;             /* the one from the zxv_input_map example */
static zxv_input p1, p2;

void setup_players(void)
{
    zxv_input_init(&p1, 0);                     /* 0 (NULL): the default map */
    zxv_input_init(&p2, &player2);
}
```

**See also:** [`zxv_input_update`](#zxv_input_update).

### `zxv_input_update`

```c
void zxv_input_update(zxv_input *in);
```

Reads the keyboard and the gamepad and updates `held`, `pressed`,
`released` and `frames`. An action is held if any of its keys is or, with
the gamepad connected, any of its buttons.

Call it **once per frame** and per player (before looking at the state):
`pressed` and `released` compare with the previous call, so with two calls
in a frame the second would see nothing just pressed.

Afterwards, except with [`ZXV_IN_KEEP_QUEUE`](#zxv_in_keep_queue) in the
map, **it empties the key queue**: `zxv_key_get` will give nothing of what
was typed in the meantime. With two players, it is enough for one of the
maps not to have the option for it to be emptied.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `in` | a `zxv_input` set up with `zxv_input_init` | The state. |

**Errors:** none.

**Example**

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

**See also:** [`zxv_input`](#zxv_input), [`zxv_input_repeat`](#zxv_input_repeat).

### `zxv_input_repeat`

```c
int zxv_input_repeat(const zxv_input *in, uint16_t actions, int delay, int rate);
```

For moving through a menu: it gives 1 when pressed and, if it is still
held, again every `rate` frames after `delay`. With `delay` 10 and `rate`
3, on frames 1, 13, 16, 19... of the press. It keeps nothing: it looks at
`frames`, so call it right after `zxv_input_update`, once per frame.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `in` | a `zxv_input` | The state. |
| `actions` | action bits | The ones looked at (one is enough). |
| `delay` | frames, 0 or more | How long to wait before repeating. |
| `rate` | frames; less than 1 is 1 | How often it repeats. |

**Returns:** 1 if any of `actions` has just been pressed or is due to
repeat in this frame; 0 if not.

**Errors:** none.

**Example**

```c
#include "zxv_input.h"

/* The chosen option of a menu of n, with down and up. */
int menu_move(const zxv_input *in, int sel, int n)
{
    if (zxv_input_repeat(in, ZXV_IN_DOWN, 10, 3))
        sel = (sel + 1) % n;
    if (zxv_input_repeat(in, ZXV_IN_UP, 10, 3))
        sel = (sel + n - 1) % n;
    return sel;
}
```

**See also:** [`zxv_input`](#zxv_input).

### `zxv_input_key_down`

```c
int zxv_input_key_down(int hid);
```

Tells whether a key is held now, by its USB HID code (see
[Keys held down](README.md#keys-held-down)). It needs no `zxv_input`: it
is for a single key that is not an action (F1 for help, for example).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `hid` | 0 to 255 | The HID code of the key (0xE0 to 0xE7, the Ctrl, Shift, Alt and Windows ones). |

**Returns:** 1 if it is held; 0 if not, or if `hid` is outside 0–255.

**Errors:** none.

**Example**

```c
#include "zxv_input.h"

int help_key(void) { return zxv_input_key_down(0x3A); }      /* F1 */
```

**See also:** [`zxv_input_new_key`](#zxv_input_new_key).

### `zxv_input_new_key`

```c
int zxv_input_new_key(void);
```

The key pressed since the previous call: for redefining the keys. Call it
once per frame while you wait.

The first time it is called (in the whole program) it only takes note of
what is already held and gives −1. A key that was already held does not
count until it is released and pressed again. There is a single state for
the whole program.

**Returns:** the HID code of the key just pressed (if there are several,
the one with the lowest code), or −1 if there is none.

**Errors:** none.

**Example**

```c
#include "zxv_api.h"
#include "zxv_input.h"

/* Waits for a new key and gives it to action a of the map. */
void redefine(zxv_input_map *m, int a)
{
    int k;
    zxv_input_new_key();                        /* forgets what was already held */
    while ((k = zxv_input_new_key()) < 0)
        zxv_wait_vblank();
    m->keys[a][0] = (uint8_t)k;
    m->keys[a][1] = m->keys[a][2] = 0;
    zxv_print_str(zxv_input_key_name(k));
}
```

**See also:** [`zxv_input_key_name`](#zxv_input_key_name),
[`zxv_input_key_down`](#zxv_input_key_down).

### `zxv_input_key_name`

```c
const char *zxv_input_key_name(int hid);
```

The name of a key to show it, in UTF-8 and in the machine's language
([`zxv_lang_get`](reference.md#zxv_lang_get-zxv_lang_set)): in Spanish if
it is `ZXV_LANG_ES` and in English with any other.

| Codes | Name |
|---|---|
| 0x04–0x1D | The letter, "A" to "Z" (by its place on a US keyboard) |
| 0x1E–0x27 | The digit, "1" to "9" and "0" |
| 0x3A–0x45 | "F1" to "F12" |
| 0x59–0x62 | "Keypad 1"... "Keypad 0" ("Numérico 1"...) |
| 0x28, 0x29, 0x2A, 0x2B, 0x2C | "Enter", "Esc", "Backspace", "Tab", "Space" |
| 0x39, 0x46, 0x47, 0x48 | "Caps Lock", "Print Screen", "Scroll Lock", "Pause" |
| 0x49–0x4E | "Insert", "Home", "Page Up", "Delete", "End", "Page Down" |
| 0x4F–0x52 | "Right", "Left", "Down", "Up" |
| 0x58 | "Keypad Enter" |
| 0xE0–0xE7 | "Left Ctrl", "Left Shift", "Left Alt", "Left GUI", "Right Ctrl", "Right Shift", "Right Alt", "Right GUI" |
| the rest | "Key" and its code in hexadecimal: "Key 33" ("Tecla 33") |

Symbol keys (Ñ, comma, hyphen...) come out by their code because they
move around depending on the keyboard: the Ñ of a Spanish keyboard is
"Key 33".

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `hid` | 0 to 255 | The HID code. Outside that range, its bottom 8 bits count (−1 comes out as "Key FF"). |

**Returns:** the name. Letters, digits, the F keys, the keypad ones and
the "Key XX" ones go in a buffer that is overwritten on the next call: if
you want two names at once, copy the first.

**Errors:** none.

**Example**

```c
#include "zxv_api.h"
#include "zxv_input.h"

/* "Fire: Space" */
void show_fire_key(const zxv_input_map *m)
{
    zxv_print_str("Fire: ");
    zxv_print_str(zxv_input_key_name(m->keys[4][0]));
}
```

**See also:** [`zxv_input_new_key`](#zxv_input_new_key).

## Saving data: `zxv_save.h`

It saves a structure of your game (the high scores, the options, a saved
game) and reads it back, checking that it is yours and that it is not
damaged.

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
    if (zxv_load("juego.sav", 1, &data, sizeof data) < 0) {    /* not there, or not valid */
        data.music = 1;
        data.level = 0;
    }
}

void game_over(uint32_t score, const char *name)
{
    if (zxv_score_add(data.top, 5, score, name) >= 0)          /* it made the table */
        zxv_save("juego.sav", 1, &data, sizeof data);
}
```

- The file carries the **version** of your data (the 1 above): when you
  change the structure, raise it, and older files will give
  `ZXV_SAVE_E_VERSION` instead of being read wrongly. If `zxv_load` fails,
  **your data does not change** (with more than 256 bytes, but for one
  case: see [`zxv_load`](#zxv_load)): fill it in with the usual values
  before, or after it fails.
- **If the power goes**: it is written to `juego.sav~` and then swapped
  for the real one: if the machine is switched off halfway, the previous
  one remains. If it is switched off right between deleting the old one
  and renaming, `zxv_load` reads the `~` one and the next `zxv_save` puts
  it in its place.
- **With no card**, on `ROM:` or with no server on `EXT:`, it returns the
  system error and the game goes on: it just does not save. There is no
  need to warn about anything if you do not want to.
- **Where it is saved.** The name is a [path](reference.md#files-and-folders):
  with no drive or folder, the file goes in the **current folder** (the
  one from `CD` or [`zxv_dir_change`](reference.md#folders)), not in the
  program's: loading a `.zxv` does not change the current folder. If the
  game is loaded with `LOAD "SD:/juegos/nave"` from another folder, it
  saves in that other folder; and if the current folder is on `ROM:`, it
  cannot save (`ZXV_E_ACCESS`). For it to always go to the same place, give
  the whole path (`"SD:/juegos/nave.sav"`).

**Common errors.** `zxv_save` and `zxv_load` use the
[file](reference.md#files-and-folders) functions, and besides their own
they can give those functions' errors:

| Error | When |
|---|---|
| `ZXV_E_NOCARD` | The path goes to `SD:` and there is no card. |
| `ZXV_E_OFFLINE` | The path goes to `EXT:` and there is no server. |
| `ZXV_E_IO` | The card failed when reading or writing. |
| `ZXV_E_NOTDIR`, `ZXV_E_ISDIR`, `ZXV_E_MFILE` | As in [`zxv_file_open`](reference.md#zxv_file_open). |

### `ZXV_SAVE_E_DAMAGED`, `ZXV_SAVE_E_VERSION`

The errors of [`zxv_load`](#zxv_load) itself (the rest are the file
`ZXV_E_` ones, in `zxv.h`).

| Error | Value | When |
|---|---|---|
| `ZXV_SAVE_E_DAMAGED` | −64 | The file is not from `zxv_save` (it does not start with `ZXVS`), it is cut short, it has bytes to spare or its CRC does not match. |
| `ZXV_SAVE_E_VERSION` | −65 | It is from `zxv_save`, but of another version or another length: it was saved with another structure. |

**See also:** [`zxv_load`](#zxv_load).

### `zxv_save`

```c
int zxv_save(const char *name, uint16_t version, const void *data, uint32_t len);
```

Saves `len` bytes of `data` in the file `name`, with a header (the
version, the length and a CRC) for [`zxv_load`](#zxv_load) to check them.
It does so without losing the previous one if the power goes:

1. If only `name~` is left (from a time it was switched off between steps
   3 and 4), it renames it to `name`.
2. It writes everything to `name~`.
3. It deletes `name`.
4. It renames `name~` to `name`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | path of up to 254 characters; the file name, up to 62 | Where it is saved (`name~` is used too, hence one less than usual). See [Where it is saved](#saving-data-zxv_saveh). |
| `version` | 0 to 65535 | The version of your data. |
| `data` | pointer to the data | What is saved. It can be `NULL` with `len` 0. |
| `len` | bytes | How much is saved. |

**Returns:** `ZXV_OK` (0), or an error (negative).

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NAME` | `name` is `NULL` or empty, ends in `/` or `:`, the path goes over 254 characters or the name over 62; or `data` is `NULL` with `len` greater than 0. Nothing is touched. |
| `ZXV_E_ACCESS` | The path goes to `ROM:`; `name` is open; on `EXT:`, the server does not allow the file to be replaced. |
| `ZXV_E_NOENT` | A folder in the path does not exist. |
| `ZXV_E_NOSPC` | It does not fit on the card. `name~` is deleted and `name` stays as it was. |
| `ZXV_E_FAULT` | `data` is not wholly in the program's memory or in the ROM. `name~` is deleted. |
| (common) | See [Common errors](#saving-data-zxv_saveh). |

If writing `name~` fails, it is deleted and `name` stays as it was. If
renaming fails (step 4), `name~` is left with the new data and
[`zxv_load`](#zxv_load) finds it.

**Example**

```c
#include "zxv_save.h"

static struct { uint8_t music, sfx, lang; } options;

int save_options(void)
{
    return zxv_save("SD:/opciones.cfg", 2, &options, sizeof options);
}
```

**See also:** [`zxv_load`](#zxv_load), [The file](#the-zxv_save-file).

### `zxv_load`

```c
int zxv_load(const char *name, uint16_t version, void *data, uint32_t len);
```

Reads into `data` what [`zxv_save`](#zxv_save) saved with the same version
and the same length. Before touching `data` it reads it all and checks the
CRC: **if it fails, `data` does not change**. Up to 256 bytes it reads it
on the stack and copies it; with more there is no room for a copy, so it
reads it again into `data` and checks the CRC again (if that read fails,
it tries once more). Only if the card fails right between the two reads
(it is pulled out at that moment) does it return the error with `data`
half done: on any error, put in the usual values, as in the example.

If `name` is not there or is not valid (for any error except
`ZXV_E_NOCARD` and `ZXV_E_OFFLINE`), it tries `name~`, in case the machine
was switched off halfway through saving.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | path, as in `zxv_save` | The file. |
| `version` | 0 to 65535 | The version expected. |
| `data` | pointer | Where the data is read into. It can be `NULL` with `len` 0. |
| `len` | bytes | How much is expected: it must be the same as was saved. |

**Returns:** `ZXV_OK` (0) if it has read the data (from `name` or from
`name~`), or an error (negative). If both fail, the error is `name`'s.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOENT` | There is no file (the first time the game is played), or a folder in the path does not exist. |
| `ZXV_SAVE_E_DAMAGED` | The file is not from `zxv_save`, or it is damaged or cut short. |
| `ZXV_SAVE_E_VERSION` | It is of another version or another length. |
| `ZXV_E_NAME` | As in `zxv_save`. |
| (common) | See [Common errors](#saving-data-zxv_saveh). |

**Example**

```c
#include "zxv_save.h"

static struct { uint8_t music, sfx, lang; } options;

void load_options(void)
{
    if (zxv_load("SD:/opciones.cfg", 2, &options, sizeof options) < 0) {
        options.music = 1;                      /* the usual ones */
        options.sfx = 1;
        options.lang = 0;
    }
}
```

**See also:** [`zxv_save`](#zxv_save),
[`ZXV_SAVE_E_DAMAGED`, `ZXV_SAVE_E_VERSION`](#zxv_save_e_damaged-zxv_save_e_version).

### `zxv_crc32`

```c
uint32_t zxv_crc32(const void *data, uint32_t n);
```

The CRC-32 of `n` bytes: the one of zip, PNG and gzip (polynomial
0xEDB88320, starting at 0xFFFFFFFF and inverted at the end). It is the one
the `zxv_save` files carry; it is also useful for checking your own data.
It goes bit by bit, with no table: it takes no memory, but it is slower
than with one.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `data` | pointer | The data. |
| `n` | bytes | How many. With 0, the CRC is 0. |

**Returns:** the CRC. That of the nine bytes `"123456789"` is 0xCBF43926.

**Errors:** none.

**Example**

```c
#include "zxv_save.h"

/* 1 if the level loaded is the one expected. */
int level_ok(const uint8_t *lvl, uint32_t size, uint32_t expected)
{
    return zxv_crc32(lvl, size) == expected;
}
```

**See also:** [The file](#the-zxv_save-file).

### `zxv_score`

```c
typedef struct {
    uint32_t score;
    char name[8];
} zxv_score;
```

A line of a high-score table (12 bytes). A table is an array of them,
highest first, which is saved with the rest.

| Field | Values | What it is |
|---|---|---|
| `score` | 0 to 4,294,967,295 | The points. |
| `name` | up to 7 bytes and a 0 | The name. In UTF-8, an accented letter takes 2 bytes. |

A table that starts as zeros (like that of a global variable) is valid:
they are lines of 0 points with no name.

**See also:** [`zxv_score_add`](#zxv_score_add).

### `zxv_score_add`

```c
int zxv_score_add(zxv_score *table, int n, uint32_t score, const char *name);
```

Puts a score in its place in the table (highest first) and drops the last
one. On a tie, the new one goes after those already there.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `table` | a table of `n` lines, highest first | The table. |
| `n` | 1 or more | How many lines it has. |
| `score` | any `uint32_t` | The points. |
| `name` | text, or `NULL` | The name. The first 7 bytes are kept (it may cut an accented letter in half); `NULL`, no name. |

**Returns:** the position it got into (0 is the best), or −1 if it does
not get in: if it does not beat the last one (or only equals it), or if
`table` is `NULL` or `n` is 0 or negative. With −1 the table does not
change.

**Errors:** none.

**Example**

```c
#include "zxv_save.h"

static zxv_score top[10];

/* The position, to highlight it in the table; -1 if it did not get in. */
int new_score(uint32_t score, const char *initials)
{
    int at = zxv_score_add(top, 10, score, initials);
    if (at >= 0)
        zxv_save("records.sav", 1, top, sizeof top);
    return at;
}
```

**See also:** [`zxv_score`](#zxv_score), [`zxv_save`](#zxv_save).

### The `zxv_save` file

In case you want to read or write it from the PC. Everything is low byte
first:

| Offset | Size | What it is |
|---|---|---|
| 0 | 4 | `ZXVS` |
| 4 | 2 | The version of the data. |
| 6 | 2 | 0 |
| 8 | 4 | The length of the data. |
| 12 | 4 | The CRC-32 of the data ([`zxv_crc32`](#zxv_crc32)). |
| 16 | the length | The data, just as it was in memory. |

The data is the structure as the compiler lays it out (with the alignment
gaps): a file saved by a program is read correctly by the same program, or
by another with the same structure.

## Decompressing: `zxv_inflate.h`

To carry compressed graphics, maps or text inside the program (or in a
file), and unpack them when they are needed. The output of any program
that compresses with deflate will do: a `.gz` from gzip, 7-Zip or Python.

```make
SOURCES := juego.c $(SDK)/lib/inflate.c
```

```c
#include "zxv_inflate.h"

extern const unsigned char level1_gz[];         /* from "xxd -i nivel1.bin.gz" */
extern const unsigned int level1_gz_len;
static uint8_t level[16384];

int unpack_level(void)
{
    return zxv_gunzip(level1_gz, level1_gz_len, level, sizeof level);   /* < 0: damaged, or does not fit */
}
```

- It needs about 2 KB of stack and nothing else: it decompresses straight
  into the destination, with no other buffer.
- **It is fast:** in the emulator, 64 KB come out in 11 ms (repeated
  tiles), 23 ms (text) or 54 ms (random texels): from 1.2 to 6 MB per
  second.
- **Compressing is done on the PC:** `gzip -9 nivel1.bin`. To put it in
  the program, `xxd -i nivel1.bin.gz` (a C array), or read it from the
  card with [`zxv_file_load`](reference.md#zxv_file_load).

### `ZXV_INFLATE_E_DATA`, `ZXV_INFLATE_E_SPACE`

The errors of the decompressing functions.

| Error | Value | When |
|---|---|---|
| `ZXV_INFLATE_E_DATA` | −66 | The data is not deflate (or gzip), it is damaged or truncated, or the CRC or the length of a `.gz` do not match. Also with `NULL` pointers or an `out_max` above 0x7FFFFFFF. |
| `ZXV_INFLATE_E_SPACE` | −67 | The decompressed data does not fit in `out_max` bytes. |

With either of them, `out` may have part of the decompressed data written
in it, but never beyond `out_max`.

**See also:** [`zxv_inflate`](#zxv_inflate), [`zxv_gunzip`](#zxv_gunzip).

### `zxv_inflate`

```c
int zxv_inflate(const void *in, uint32_t in_len, void *out, uint32_t out_max);
```

Decompresses plain deflate (RFC 1951), with no header or check: that of
Python's `zlib.compressobj(9, zlib.DEFLATED, -15)`. Whatever is in `in`
after the end of the compressed data is not looked at.

A zlib stream with a header (that of Python's `zlib.compress`) does not
work as it is: it is deflate with 2 bytes in front and 4 behind; pass it
`in + 2`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `in` | pointer | The compressed data. |
| `in_len` | bytes | How much there is. |
| `out` | pointer | Where it is decompressed. It can be `NULL` with `out_max` 0. |
| `out_max` | 0 to 0x7FFFFFFF bytes | The room there is in `out`. Nothing is ever written beyond it. |

**Returns:** the decompressed bytes, or an error (negative).

**Errors**

| Error | When |
|---|---|
| `ZXV_INFLATE_E_DATA` | It is not deflate, or it is damaged or truncated; `in` is `NULL`; `out` is `NULL` with `out_max` greater than 0; `out_max` is above 0x7FFFFFFF. |
| `ZXV_INFLATE_E_SPACE` | It does not fit in `out_max` bytes. |

**Example**

```c
#include "zxv_inflate.h"

extern const uint8_t title_z[];                 /* zlib.compress(data, 9), from Python */
extern const uint32_t title_z_len;
static uint8_t title[6144];

int unpack_title(void)
{
    /* Without the 2 bytes of the zlib header (the 4 at the end are not looked at). */
    return zxv_inflate(title_z + 2, title_z_len - 2, title, sizeof title);
}
```

**See also:** [`zxv_gunzip`](#zxv_gunzip).

### `zxv_gunzip`

```c
int zxv_gunzip(const void *in, uint32_t in_len, void *out, uint32_t out_max);
```

Decompresses a whole `.gz` (RFC 1952) with a single member (what gzip and
7-Zip make of a file), and checks the CRC and the length it carries at the
end. It skips the name, the comment and the extra fields of the header.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `in` | pointer | The `.gz`. |
| `in_len` | bytes | Its **exact** size (the end carries the CRC and the length). |
| `out` | pointer | Where it is decompressed. It can be `NULL` with `out_max` 0. |
| `out_max` | 0 to 0x7FFFFFFF bytes | The room there is in `out`. |

**Returns:** the decompressed bytes, or an error (negative).

**Errors**

| Error | When |
|---|---|
| `ZXV_INFLATE_E_DATA` | It is not a `.gz` (it is shorter than 18 bytes, it does not start with 1F 8B 08, it has reserved bits set); the deflate data is damaged; the CRC or the length do not match (also if `in_len` is not the exact size, or if the `.gz` has several members); pointers or `out_max` as in `zxv_inflate`. |
| `ZXV_INFLATE_E_SPACE` | It does not fit in `out_max` bytes. |

**Example**

```c
#include "zxv_api.h"
#include "zxv_inflate.h"

static uint8_t packed[8192], map[32768];

/* A compressed map on the card. */
int load_map(const char *path)
{
    int n = zxv_file_load(path, packed, sizeof packed);
    if (n < 0)
        return n;
    return zxv_gunzip(packed, (uint32_t)n, map, sizeof map);
}
```

**See also:** [`zxv_gunzip_size`](#zxv_gunzip_size), [`zxv_inflate`](#zxv_inflate).

### `zxv_gunzip_size`

```c
int zxv_gunzip_size(const void *in, uint32_t in_len);
```

How much a `.gz` will take decompressed, without decompressing it: its
last 4 bytes say so. It only looks at the header and the end; whether the
data is sound is for [`zxv_gunzip`](#zxv_gunzip) to say.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `in` | pointer | The `.gz`. |
| `in_len` | bytes | Its exact size. |

**Returns:** the decompressed size in bytes, or `ZXV_INFLATE_E_DATA`.

**Errors**

| Error | When |
|---|---|
| `ZXV_INFLATE_E_DATA` | It does not look like a `.gz` (as in `zxv_gunzip`), `in` is `NULL`, or the size it gives is above 0x7FFFFFFF. |

**Example**

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

**See also:** [`zxv_gunzip`](#zxv_gunzip).

## Fonts: `zxv_font.h`

Text of **any size and colour**, proportional or fixed-width, with
accented letters and ñ. A font comes from:

- **the ROM one**, the machine's 8 × 8, ready-made:
  [`zxv_font_rom`](#zxv_font_rom);
- **a sheet**: a bitmap with one character in each cell (of any size, in
  colour and with 8 transparent), for example a sprite from a `.gfx`
  drawn in `zxv-gfx` or made with `png2gfx`:
  [`zxv_font_sheet`](#zxv_font_sheet);
- **a 1-bit table** made with [`fontconv.py`](#fonts-from-elsewhere-fontconvpy):
  [`zxv_font_bits`](#zxv_font_bits).

```make
SOURCES := juego.c $(SDK)/lib/font.c $(SDK)/lib/font_rom.c
```

**Always add both files**, even if you do not use the ROM font: `font.c`
refers to it and, with the options of the SDK's Makefile, it does not link
without `font_rom.c`. (`font_rom.c` takes a little over 1 KB. If you
compile with `-ffunction-sections` and do not call `zxv_font_rom`, the
linker removes it and you can leave it out.)

```c
#include "zxv_font.h"

static zxv_font f;

void game_over_title(void)
{
    zxv_font_rom(&f, 0);
    const char *t = "Game over!";
    int w = zxv_font_width(&f, t, 2);
    zxv_font_draw(&f, (256 - w) / 2 + 1, 81, t, 0, 2);     /* the shadow */
    zxv_font_draw(&f, (256 - w) / 2, 80, t, 14, 2);        /* twice as big, centred */
}
```

- **The text** is in UTF-8 and may have `\n` (new line). Whatever is not
  valid UTF-8 is taken byte by byte as Latin-1. What is not in the font
  comes out as `?` (and if `?` is not there either, as a gap of half a
  cell).
- **It is drawn with [`zxv_gfx_rect`](reference.md#shapes)**, on the
  current target: it respects clipping and also the
  [pattern](graphics.md#shapes-with-patterns) (patterned text). Each run of
  consecutive pixels of the same colour in a row is a rectangle, so
  enlarging costs no more.
- In proportional fonts, each character starts at its first column with
  ink and advances its width plus `spacing` (1 pixel); a character with no
  ink (the space) advances half a cell.

### `ZXV_FONT_CHARS`, `ZXV_FONT_FIXED`, `ZXV_FONT_OWN`

| Constant | Value | What it is |
|---|---|---|
| `ZXV_FONT_CHARS` | 224 | The most characters that fit in a font. |
| `ZXV_FONT_FIXED` | 0x01 | Option when creating the font: fixed-width, all of them advance the width of the cell and with no space between letters. Without it, proportional. |
| `ZXV_FONT_OWN` | −1 | The colour for `zxv_font_draw` to draw with the sheet's colours (or 15, in 1-bit ones). |

**See also:** [`zxv_font`](#zxv_font), [`zxv_font_draw`](#zxv_font_draw).

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

A font ready to draw with. [`zxv_font_rom`](#zxv_font_rom),
[`zxv_font_sheet`](#zxv_font_sheet) or [`zxv_font_bits`](#zxv_font_bits)
fill it in, measuring each character. It takes about 480 bytes: better
global or `static` than on the stack. The graphics are not copied: the
sheet or the table must go on existing.

| Field | What it is |
|---|---|
| `sheet` | The sheet (4 bpp), in sheet fonts; `sheet.pix` is `NULL` in 1-bit ones. |
| `bits` | The 1-bit table (`h` bytes per character, bit 7 on the left); `NULL` in sheet fonts. |
| `codes` | The Unicode code of each character, from lowest to highest; `NULL`: `first`, `first + 1`... |
| `first`, `count` | The code of the first one and how many there are (up to 224). |
| `w`, `h` | The size of the cell, in pixels (in 1-bit ones, `w` is 8). |
| `spacing` | The pixels between letters: 1 in proportional fonts, 0 in fixed-width ones. **It can be changed** after creating the font (`f.spacing = 2;`). |
| `flags` | The options it was created with. |
| `left[i]`, `adv[i]` | For each character, the empty columns on its left and how far it advances (without `spacing`). The library works them out. |

**See also:** [`ZXV_FONT_CHARS`...](#zxv_font_chars-zxv_font_fixed-zxv_font_own).

### `zxv_font_rom`

```c
void zxv_font_rom(zxv_font *f, unsigned flags);
```

The machine's 8 × 8 font, with 112 characters: ASCII 32 to 126, 127 (©),
and ¡ ¿ Á É Í Ñ Ó Ú Ü á é í ñ ó ú ü. It is in `lib/font_rom.c`; it is not
read from the ROM.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | pointer to a `zxv_font` | Where it is set up. With `NULL`, it does nothing. |
| `flags` | 0 or `ZXV_FONT_FIXED` | 0: proportional, with 1 pixel between letters; `ZXV_FONT_FIXED`: every 8 pixels, like the machine's text. |

**Errors:** none.

**Example**

```c
#include "zxv_font.h"

static zxv_font hud;

void hud_init(void) { zxv_font_rom(&hud, ZXV_FONT_FIXED); }   /* the numbers do not jiggle */
```

**See also:** [`zxv_font_draw`](#zxv_font_draw).

### `zxv_font_sheet`

```c
int zxv_font_sheet(zxv_font *f, const zxv_bitmap *sheet, int cell_w, int cell_h, int first, unsigned flags);
```

A font from a sheet: a 4 bpp bitmap (like those in video memory: the low
nibble is the even pixel) with one character in each cell of `cell_w` ×
`cell_h`, from left to right and from top to bottom, with consecutive
codes from `first`. The characters can be in colour; 8 is the transparent
one (and what counts as empty when measuring them).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | pointer to a `zxv_font` | Where it is set up. |
| `sheet` | a `zxv_bitmap` with `pix` | The sheet. It is not copied: it must go on existing. |
| `cell_w`, `cell_h` | 1 to 255, and no more than the sheet | The size of each cell. |
| `first` | 0 to 65535 | The code of the character in the first cell (32 to start at the space). |
| `flags` | 0 or `ZXV_FONT_FIXED` | Proportional or fixed-width. |

It holds as many whole cells as there are in the sheet (up to 224); those
cut off by the right or bottom edge do not count.

**Returns:** 0 if the font is valid; −1 if not, and then `*f` does not
change.

**Errors**

| Returns | When |
|---|---|
| −1 | `f` or `sheet` are `NULL`, `sheet->pix` is `NULL`, `cell_w` or `cell_h` are outside 1–255 or larger than the sheet, or `first` is negative. |

**Example**

```c
#include "zxv_font.h"

extern zxv_bitmap digits_sheet;                 /* from the .gfx: "0123456789" in 12 x 16 cells */
static zxv_font digits;

int score_font(void) { return zxv_font_sheet(&digits, &digits_sheet, 12, 16, '0', ZXV_FONT_FIXED); }
```

**See also:** [`zxv_font_bits`](#zxv_font_bits), [`zxv_font_draw`](#zxv_font_draw).

### `zxv_font_bits`

```c
int zxv_font_bits(zxv_font *f, const uint8_t *bits, const uint16_t *codes, int n, int h, unsigned flags);
```

A font from a 1-bit table 8 pixels wide: the one
[`fontconv.py`](#fonts-from-elsewhere-fontconvpy) writes. Each character is
`h` bytes, one per row, with the left column in bit 7.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | pointer to a `zxv_font` | Where it is set up. |
| `bits` | `n` · `h` bytes | The characters. They are not copied. |
| `codes` | `n` codes from lowest to highest, or `NULL` | The Unicode code of each character. They must be in order: if not, some are not found. `NULL`: from 32 onwards, consecutive. |
| `n` | 1 to 224 | How many characters there are. |
| `h` | 1 to 255 | The rows of each one. |
| `flags` | 0 or `ZXV_FONT_FIXED` | Proportional or fixed-width. |

**Returns:** 0 if the font is valid; −1 if not, and `*f` does not change.

**Errors**

| Returns | When |
|---|---|
| −1 | `f` or `bits` are `NULL`, `n` is outside 1–224 or `h` outside 1–255. |

**Example:** see [`fontconv.py`](#fonts-from-elsewhere-fontconvpy).

**See also:** [`zxv_font_sheet`](#zxv_font_sheet), [`zxv_font_draw`](#zxv_font_draw).

### `zxv_font_draw`

```c
int zxv_font_draw(const zxv_font *f, int x, int y, const char *text, int colour, int scale);
```

Draws the text with the top left corner of its first cell at (`x`, `y`).
A `\n` goes back to `x`, `h` · `scale` pixels further down.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | a font set up | The font. |
| `x`, `y` | pixels of the target, any value | Where. What falls outside the clipping is not drawn. |
| `text` | UTF-8 | The text. |
| `colour` | 0–15, or `ZXV_FONT_OWN` | 0–15: all the ink in that colour. Any other value (`ZXV_FONT_OWN`): the sheet's colours; in 1-bit ones, 15. |
| `scale` | 1 to 8 | How many times bigger. Less than 1 is 1 and more than 8 is 8. |

**Returns:** the x where the last line ends (counting the `spacing` after
the last letter): to carry on writing after it. With `f` or `text` `NULL`,
it draws nothing and returns `x`.

**Errors:** none.

**Example**

```c
#include "zxv_font.h"

static zxv_font f;

/* "Score: " in white and the number in yellow, after it. */
void draw_score(const char *digits)
{
    zxv_font_rom(&f, 0);
    int x = zxv_font_draw(&f, 8, 4, "Score: ", 15, 1);
    zxv_font_draw(&f, x, 4, digits, 14, 1);
}
```

**See also:** [`zxv_font_width`](#zxv_font_width),
[`zxv_font_height`](#zxv_font_height).

### `zxv_font_width`

```c
int zxv_font_width(const zxv_font *f, const char *text, int scale);
```

How wide the text is: the width of its longest line, without the
`spacing` after the last letter. To centre it or align it to the right.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | a font set up | The font. |
| `text` | UTF-8 | The text. |
| `scale` | 1 to 8 | As in `zxv_font_draw`. |

**Returns:** the width in pixels; 0 with `f` or `text` `NULL` or with an
empty text.

**Errors:** none.

**Example**

```c
#include "zxv_font.h"

/* Aligned to the right, ending at column 248. */
void draw_right(const zxv_font *f, int y, const char *t)
{
    zxv_font_draw(f, 248 - zxv_font_width(f, t, 1), y, t, 7, 1);
}
```

**See also:** [`zxv_font_height`](#zxv_font_height).

### `zxv_font_height`

```c
int zxv_font_height(const zxv_font *f, const char *text, int scale);
```

How tall the text is: its lines times the height of the cell times
`scale`. Each `\n` is one more line, at the end too.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | a font set up | The font. |
| `text` | UTF-8 | The text. |
| `scale` | 1 to 8 | As in `zxv_font_draw`. |

**Returns:** the height in pixels; 0 with `f` or `text` `NULL` or with an
empty text.

**Errors:** none.

**Example**

```c
#include "zxv_font.h"

/* A text of several lines, centred on the 256 x 192 screen. */
void draw_centred(const zxv_font *f, const char *t)
{
    int w = zxv_font_width(f, t, 1), h = zxv_font_height(f, t, 1);
    zxv_font_draw(f, (256 - w) / 2, (192 - h) / 2, t, 15, 1);
}
```

**See also:** [`zxv_font_width`](#zxv_font_width).

### Fonts from elsewhere: `fontconv.py`

`sdk/tools/fontconv.py` converts bitmap fonts up to 8 columns wide and
from 1 to 32 rows tall into C for [`zxv_font_bits`](#zxv_font_bits), or
into text for touching them up:

```sh
python3 sdk/tools/fontconv.py mifuente.ch8 mifuente.c mifuente
python3 sdk/tools/fontconv.py mifuente.ch8 mifuente.txt
```

| Format | How it is recognised |
|---|---|
| ZX-V's text format (`== CODE` and the rows with `#` and `.`, like `rom/font`) | `.txt` |
| ZX Spectrum (96 characters, 768 bytes): its ↑, £ and © go to their codes (`--ascii`, not) | `.ch8`, or 768 bytes |
| A dump of 8 bytes per character (PC, MSX, Amstrad CPC...): `--first`, `--height`, `--charset` (CP437 for PC ones: accented letters and ñ in their place) | `.bin`, `.rom`, `.fnt`... |
| PSF 1 and 2 (the Linux console ones), with their Unicode table | by its signature |
| BDF (X11) | `.bdf` |
| `.hex` (GNU Unifont: `CODE:BYTES`): wide characters (the 16-column ones) are left out, with a warning | `.hex` |

| Option | What it does |
|---|---|
| `--format F` | `txt`, `zx`, `raw`, `psf`, `bdf` or `hex`, if the extension is not enough. |
| `--first N` | In a dump, the code of the first character (0; 32 if there are 96). |
| `--height H` | In a dump, the rows of each character (8). In a `.hex`, those of the 8-column ones (if not given, those of the narrowest in the file; if all have the same size and it is more than 16 bytes, it must be given: 16 for Unifont's wide ones). |
| `--charset C` | In a dump, how to turn codes above 127 into Unicode: `cp437` (the default if there are 256 from 0), `latin-1`... |
| `--ascii` | In a Spectrum one, leaves `^`, `` ` `` and 127 as in ASCII. |
| `--range R` | The characters that are kept, in hexadecimal: by default `20-7E,A0-FF` (ASCII and Latin-1, with accented letters, ñ, ¿ and ¡). |

The `--range` ones are picked first: whatever is not in it does not count
(nor do its rows). At most 224 characters are kept: if there are more in
`--range`, it gives an error. In a dump, codes are one byte: one of more
than 256 characters (or going past 255 from `--first`) gives an error;
usually `--height` is not the font's. Codes are 16-bit: a `--range` with anything above FFFF (or a
reversed interval) gives an error. A
font's licence is still its own: being able to convert it does not mean
you may distribute it.

**What it writes.** The `.c` defines four things, with the name you give
it (the third argument) in front:

| Definition | What it is |
|---|---|
| `const unsigned NAME_n` | How many characters there are. |
| `const unsigned NAME_h` | The rows of each one. |
| `const uint16_t NAME_codes[]` | Their Unicode codes, from lowest to highest. |
| `const uint8_t NAME_bits[]` | The characters, `NAME_h` bytes each. |

It writes no header: add the `.c` to `SOURCES` and declare what you use
with `extern`:

```c
#include "zxv_font.h"

/* From "fontconv.py mifuente.ch8 mifuente.c mifuente" (mifuente.c, in SOURCES). */
extern const unsigned mifuente_n, mifuente_h;
extern const uint16_t mifuente_codes[];
extern const uint8_t mifuente_bits[];

static zxv_font f;

int load_font(void)
{
    return zxv_font_bits(&f, mifuente_bits, mifuente_codes, (int)mifuente_n, (int)mifuente_h, 0);
}
```

## Simple 3D: `zxv_3d.h`

**Flat-faced** models with lighting and pattern shading, like the 3D games
of 8- and 16-bit computers: each face has a dark colour and a light one,
and depending on how the light falls on it, it is seen in one, the other
or a mix of both with [patterns](graphics.md#shapes-with-patterns) (five
shades). Drawing with a pattern costs the same as with a colour.

```make
SOURCES := juego.c $(SDK)/lib/3d.c $(SDK)/lib/math.c
```

The in-between shades need a ROM of specification 0.2 (they use
[`zxv_gfx_pattern`](reference.md#shapes); with an older one, those faces
come out in the light colour, with no pattern). It takes about 29 KB of
memory (the frame's tables, fixed). 3D gains a lot from optimisation: the
Gone Silent example compiles `3d.c` with `-O2` instead of the usual `-Os`.

```c
#include "zxv_3d.h"
#include "zxv_gfx.h"

extern const zxv_model house;                   /* from obj2c.py (below) */

int main(void)
{
    zxv_camera cam = { .pos = { ZXV_FIX(-3), ZXV_FIX(2), ZXV_FIX(-7) },   /* where it is */
                       .yaw = ZXV_DEG(20), .pitch = -ZXV_DEG(10),         /* turns, looks down */
                       .focal = 128, .cx = 128, .cy = 96, .w = 256, .h = 192 };
    zxv_xform pos;
    for (int32_t a = 0; zxv_key_get() < 0; a += 256) {
        zxv_gfx_rect(0, 0, 256, 80, 5);                            /* sky and ground */
        zxv_gfx_rect(0, 80, 256, 112, 4);
        zxv_xform_set(&pos, a, 0, 0, 0, ZXV_FIX(1), 0);            /* the house, turning */
        zxv_3d_begin(&cam);
        zxv_3d_add(&house, &pos);
        zxv_3d_draw();
        zxv_wait_vblank();
    }
    return 0;
}
```

(To avoid flicker, draw on the hidden page: see [Pages and double
buffering](graphics.md#pages-and-double-buffering).)

- **Coordinates:** x to the right, y up, z forwards (where the camera
  looks without turning), in 16.16. The world fits in ±32,768 units.
  Angles in turns of 0x10000: `yaw` turns right, `pitch` looks up and
  `roll` tilts right.
- **Every frame:** [`zxv_3d_begin(&cam)`](#zxv_3d_begin), one
  [`zxv_3d_add(&model, &pos)`](#zxv_3d_add) per object (with a
  [`zxv_xform`](#zxv_xform): its rotation and position) and
  [`zxv_3d_draw()`](#zxv_3d_draw). Faces turned away are not drawn,
  everything is clipped against the view and faces are drawn from far to
  near (the painter's algorithm).
- **The painter gets it wrong** with faces that cross, or with very long
  faces that overlap others: a large floor is better done with
  `zxv_gfx_rect` or with mode 7 before the 3D. See also the tips for
  [`obj2c.py`](#models-from-blender-obj2cpy).

### `ZXV_3D_MAX_FACES`, `ZXV_3D_MAX_VERTS`, `ZXV_3D_FACE_SIDES`

| Constant | Value | What it is |
|---|---|---|
| `ZXV_3D_MAX_FACES` | 400 | The faces seen in a frame, among all the objects. |
| `ZXV_3D_MAX_VERTS` | 512 | The vertices of a model. |
| `ZXV_3D_FACE_SIDES` | 32 | The most sides a face can have. A non-convex face is split into triangles when drawn. |

The first two are changed when compiling `lib/3d.c` with `-D`
(`EXTRA_CFLAGS=-DZXV_3D_MAX_FACES=800`): with the usual values, the
tables take about 29 KB; each extra face, about 43 bytes, and each vertex,
24. Besides the faces, there is room for 6 points per face on average
(clipped ones have more): a frame full of many-sided faces may run out of
room sooner.

**See also:** [`zxv_3d_add`](#zxv_3d_add).

### `ZXV_3D_MAT`, `ZXV_3D_TWO_SIDED`, `ZXV_3D_FLAT`, `ZXV_3D_DECAL`

```c
#define ZXV_3D_MAT(dark, light, flags) ((uint16_t)((dark) | (light) << 4 | (flags)))
#define ZXV_3D_TWO_SIDED  0x0100u
#define ZXV_3D_FLAT       0x0200u
#define ZXV_3D_DECAL      0x0400u
```

The material of a face, the number that goes in the model after the
number of sides: its two colours and its options.

| Part | Values | What it is |
|---|---|---|
| `dark` | 0–15 | The dark colour: that of the face turned away from the light. |
| `light` | 0–15 | The light colour: that of the face facing the light. If it is the same as `dark`, the face is plain, with no patterns. |
| `ZXV_3D_TWO_SIDED` | 0x0100 | It is seen from both sides (a sheet, a fence). From behind, the light falls on it from that side. |
| `ZXV_3D_FLAT` | 0x0200 | No lighting: always in the dark colour. |
| `ZXV_3D_DECAL` | 0x0400 | It lies on top of another (a door on a wall): it is sorted by its nearest vertex, so it is drawn after the face it is on. |

**Example:** see [`zxv_model`](#zxv_model).

**See also:** [`zxv_model`](#zxv_model), [`zxv_3d_light`](#zxv_3d_light).

### `zxv_vec3`

```c
typedef struct {
    zxv_fix x, y, z;
} zxv_vec3;
```

A point or a vector of the world, in 16.16: x to the right, y up, z
forwards.

**See also:** [`zxv_xform_apply`](#zxv_xform_apply), [`zxv_3d_project`](#zxv_3d_project).

### `zxv_xform`

```c
typedef struct {
    zxv_fix m[3][3];
    zxv_vec3 t;
} zxv_xform;
```

Where an object is and how it is turned: a rotation (a 3 × 3 matrix in
16.16) and a translation. It is set up with
[`zxv_xform_set`](#zxv_xform_set) and combined with
[`zxv_xform_mul`](#zxv_xform_mul). A point `p` of the object goes to
`m` · `p` + `t` in the world.

| Field | What it is |
|---|---|
| `m[i][j]` | The rotation, row `i`, column `j`, in 16.16. It can carry a scale too (multiply the matrix by it). |
| `t` | Where the object's origin goes, in the world. |

**See also:** [`zxv_xform_set`](#zxv_xform_set).

### `zxv_model`

```c
typedef struct {
    const int16_t (*verts)[3];
    const uint16_t *faces;
    uint16_t nverts, nfaces;
    zxv_fix scale;
} zxv_model;
```

A model: what [`obj2c.py`](#models-from-blender-obj2cpy) writes, or one
made by hand.

| Field | Values | What it is |
|---|---|---|
| `verts` | `nverts` vertices of 3 16-bit integers | x, y, z of each vertex, in model units. |
| `faces` | a list of `uint16_t` | The faces, one after another: the number of vertices (3 to 32), the [material](#zxv_3d_mat-zxv_3d_two_sided-zxv_3d_flat-zxv_3d_decal) and the indices of its vertices (0 to `nverts` − 1). |
| `nverts` | 0 to 512 | How many vertices. |
| `nfaces` | 0 or more | How many faces. |
| `scale` | 16.16 | How big one model unit is in the world: with `ZXV_FIX_ONE`, each unit is one of the world; with `ZXV_FIX_ONE / 256`, 256 model units are one of the world. |

A face **is seen from the front if its vertices go clockwise when looked
at from outside**; from behind it is not drawn (except with
`ZXV_3D_TWO_SIDED`). The lighting and whether it is seen come from the
normal of the whole face, so it does not matter which vertex it starts at.
A face is **flat and does not cross itself**, but it may be non-convex (an
L, a saw): it is split into triangles when drawn, every frame
([`obj2c.py`](#models-from-blender-obj2cpy) already gives them split,
which is faster).

**Example**

```c
#include "zxv_3d.h"

/* A cube with sides of 2 units, blue (1 and 9). */
static const int16_t cube_v[8][3] = {
    { -1, -1, -1 }, { 1, -1, -1 }, { -1, 1, -1 }, { 1, 1, -1 },
    { -1, -1, 1 },  { 1, -1, 1 },  { -1, 1, 1 },  { 1, 1, 1 },
};
#define BLUE ZXV_3D_MAT(1, 9, 0)
static const uint16_t cube_f[] = {
    4, BLUE, 0, 2, 3, 1,                        /* front (z = -1) */
    4, BLUE, 5, 7, 6, 4,                        /* back */
    4, BLUE, 1, 3, 7, 5,                        /* right */
    4, BLUE, 4, 6, 2, 0,                        /* left */
    4, BLUE, 2, 6, 7, 3,                        /* top */
    4, BLUE, 0, 1, 5, 4,                        /* bottom */
};
const zxv_model cube = { cube_v, cube_f, 8, 6, ZXV_FIX_ONE };
```

**See also:** [`zxv_3d_add`](#zxv_3d_add), [`obj2c.py`](#models-from-blender-obj2cpy).

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

Where it is looked from and how it is seen on the screen.

| Field | Values | What it is |
|---|---|---|
| `pos` | 16.16 | Where the camera is. |
| `yaw`, `pitch`, `roll` | turns of 0x10000 | Which way it looks: `yaw` turns right, `pitch` looks up, `roll` tilts right. With all three at 0, it looks towards increasing z. |
| `focal` | pixels (integer), up to 32767 (more counts as 32767); 0 or less, 128 | The focal length: with 128 and a view 256 wide, about 90° from side to side. More, more zoom. |
| `cx`, `cy` | pixels | The centre of the view on the target (128, 96 for the whole mode 0 screen). **No default value:** set them. |
| `w`, `h` | pixels; 0 or less, 256 and 192 | The size of the view, for clipping. What is actually drawn is also clipped by the graphics engine's [clipping](reference.md#target-and-clipping): for a window, set both. |
| `near` | 16.16; 0 or less, 0.25 | The nearest that is drawn: whatever is nearer is clipped. |

**See also:** [`zxv_3d_begin`](#zxv_3d_begin), [`zxv_3d_project`](#zxv_3d_project).

### `zxv_xform_set`

```c
void zxv_xform_set(zxv_xform *x, int32_t yaw, int32_t pitch, int32_t roll, zxv_fix px, zxv_fix py, zxv_fix pz);
```

Sets up a position: the rotation of the three angles and the place. When
it is applied, the object is first tilted (`roll`), then raised or lowered
(`pitch`) and lastly turned (`yaw`), and moved to (`px`, `py`, `pz`).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x` | pointer to a `zxv_xform` | Where it is left. |
| `yaw` | turns of 0x10000 | Turns right (around y). |
| `pitch` | turns of 0x10000 | Looks up (around x). |
| `roll` | turns of 0x10000 | Tilts right (around z). |
| `px`, `py`, `pz` | 16.16 | The place. |

**Errors:** none.

**Example**

```c
#include "zxv_3d.h"

/* A ship at (x, 1, z) that faces 'heading' and banks in the bends. */
void place_ship(zxv_xform *pos, zxv_fix x, zxv_fix z, int32_t heading, int32_t bank)
{
    zxv_xform_set(pos, heading, 0, bank, x, ZXV_FIX(1), z);
}
```

**See also:** [`zxv_xform_mul`](#zxv_xform_mul), [`zxv_3d_add`](#zxv_3d_add).

### `zxv_xform_apply`

```c
zxv_vec3 zxv_xform_apply(const zxv_xform *x, zxv_vec3 p);
```

Where a point goes with a position: `m` · `p` + `t`. To know where in the
world a point of an object ends up (the muzzle of the gun, for the shot to
come out of it).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x` | a `zxv_xform` | The position. |
| `p` | 16.16 | The point, in the object's coordinates. |

**Returns:** the point in the world, in 16.16. If it goes beyond ±32768,
it is not valid (it overflows).

**Errors:** none.

**Example**

```c
#include "zxv_3d.h"

/* The gun is 1.5 units in front of the centre of the ship. */
zxv_vec3 gun_position(const zxv_xform *ship)
{
    zxv_vec3 gun = { 0, 0, ZXV_FIX(1.5) };
    return zxv_xform_apply(ship, gun);
}
```

**See also:** [`zxv_xform_set`](#zxv_xform_set), [`zxv_3d_project`](#zxv_3d_project).

### `zxv_xform_mul`

```c
void zxv_xform_mul(zxv_xform *out, const zxv_xform *a, const zxv_xform *b);
```

Combines two positions: `out` = `a` · `b`, which is applying first `b` and
then `a`. It is useful to put one object on another: an arm on a body is
`body · arm`, with the position of the arm relative to the body.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `out` | pointer to a `zxv_xform` | Where it is left. It can be the same as `a` or `b`. |
| `a` | a `zxv_xform` | The outer one (the body). |
| `b` | a `zxv_xform` | The inner one (the arm, relative to the body). |

**Errors:** none.

**Example**

```c
#include "zxv_3d.h"

extern const zxv_model tank_body, tank_turret;

/* The turret turns on the tank, 0.5 units above its centre. */
void add_tank(const zxv_xform *body, int32_t turret_angle)
{
    zxv_xform rel, turret;
    zxv_xform_set(&rel, turret_angle, 0, 0, 0, ZXV_FIX(0.5), 0);
    zxv_xform_mul(&turret, body, &rel);
    zxv_3d_add(&tank_body, body);
    zxv_3d_add(&tank_turret, &turret);
}
```

**See also:** [`zxv_xform_set`](#zxv_xform_set).

### `zxv_3d_light`

```c
void zxv_3d_light(zxv_fix x, zxv_fix y, zxv_fix z, int ambient);
```

Sets the light: where it comes from and the shade of what gets none. It
stays for the following frames. **It counts from the next
[`zxv_3d_begin`](#zxv_3d_begin)**, which is what turns it with the camera.

Each face has five shades, from 0 (the dark colour) to 4 (the light one),
going through patterns of a quarter, a half and three quarters of the
light one. A face turned away from the light has shade `ambient`; one
facing it, 4; and those in between, depending on the angle.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x`, `y`, `z` | 16.16, any length | A vector **towards** the light, in the world. Only the direction counts. With (0, 0, 0), all faces have shade `ambient`. |
| `ambient` | 0 to 4 | The shade of what gets no light. Less than 0 is 0 and more than 4 is 4. |

Without calling it, the light comes from above, from the left and from
the front (from the vector −1, 1.85, −1) with `ambient` 1.

**Errors:** none.

**Example**

```c
#include "zxv_3d.h"

void night(void) { zxv_3d_light(0, ZXV_FIX(1), 0, 0); }         /* from above, black shadows */
void noon(void) { zxv_3d_light(ZXV_FIX(1), ZXV_FIX(2), 0, 2); }  /* plenty of light everywhere */
```

**See also:** [`ZXV_3D_MAT`...](#zxv_3d_mat-zxv_3d_two_sided-zxv_3d_flat-zxv_3d_decal).

### `zxv_3d_wireframe`

```c
void zxv_3d_wireframe(int colour);
```

Draws the edges of the faces instead of filling them, with
[`zxv_gfx_line`](reference.md#shapes): for debugging, or for a line-drawn
game. It stays until it is changed, and counts from the next
`zxv_3d_draw`. Faces turned away are still not drawn.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `colour` | 0–15, or −1 | The colour of the edges. Any other value (−1): faces again. |

**Errors:** none.

**Example**

```c
#include "zxv_3d.h"

void debug_view(int on) { zxv_3d_wireframe(on ? 4 : -1); }      /* green */
```

**See also:** [`zxv_3d_draw`](#zxv_3d_draw).

### `zxv_3d_begin`

```c
void zxv_3d_begin(const zxv_camera *cam);
```

Starts a frame: empties the list of faces and takes the camera (and the
light). The fields of the camera that are 0 (`focal`, `w`, `h`, `near`)
take their default values; `cx` and `cy` do not.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `cam` | a `zxv_camera` | The camera. It is copied. |

**Errors:** none.

**Example:** see the one in the [introduction](#simple-3d-zxv_3dh).

**See also:** [`zxv_3d_add`](#zxv_3d_add), [`zxv_camera`](#zxv_camera).

### `zxv_3d_add`

```c
int zxv_3d_add(const zxv_model *m, const zxv_xform *pos);
```

Adds a model to the frame: puts it in its place, removes the faces turned
away, works out the shade of each one with the light, clips them against
the view and keeps them for [`zxv_3d_draw`](#zxv_3d_draw). It does not
draw anything yet.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `m` | a `zxv_model` | The model. |
| `pos` | a `zxv_xform`, or `NULL` | Where it goes. `NULL`: at the origin, not turned. |

**Returns:** the faces of the model that are seen (0 if none is seen), or
−1 if the model is not valid or they do not fit.

**Errors**

| Returns | When |
|---|---|
| −1 | `m` is `NULL`, or its `verts` or `faces`; it has more than 512 vertices (`ZXV_3D_MAX_VERTS`); a face has fewer than 3 or more than 32 sides, or a vertex index that does not exist. |
| −1 | There are already `ZXV_3D_MAX_FACES` faces in the frame, or there is no room left for their points. |

With −1, the faces of that model that had already been added stay (it
will be drawn half-done); those of earlier models too.

**Example**

```c
#include "zxv_3d.h"

extern const zxv_model rock;
static zxv_xform rocks[20];

/* The rocks, until no more fit. */
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

**See also:** [`zxv_3d_begin`](#zxv_3d_begin), [`zxv_3d_draw`](#zxv_3d_draw),
[`ZXV_3D_MAX_FACES`...](#zxv_3d_max_faces-zxv_3d_max_verts-zxv_3d_face_sides).

### `zxv_3d_draw`

```c
void zxv_3d_draw(void);
```

Draws everything added since `zxv_3d_begin`, from far to near (by the
average distance of the vertices of each face; `ZXV_3D_DECAL` ones, by
that of the nearest), with [`zxv_gfx_poly`](reference.md#shapes) on the
current target and with its clipping. It does not clear anything first:
the background is yours.

If it has used any pattern, when it finishes **it leaves the engine's
pattern removed** (`zxv_gfx_pattern(NULL, 0)`): if you draw with a pattern
afterwards, set it again. The list of faces is not emptied until the next
`zxv_3d_begin`: calling it twice draws the same thing twice.

**Errors:** none.

**Example**

```c
#include "zxv_3d.h"
#include "zxv_gfx.h"

/* The 3D only in a 160 x 120 window, with its centre at (128, 80). */
void draw_window(const zxv_camera *cam, const zxv_model *m, const zxv_xform *pos)
{
    zxv_gfx_clip(48, 20, 208, 140);
    zxv_gfx_rect(48, 20, 160, 120, 0);
    zxv_3d_begin(cam);                          /* with cx 128, cy 80, w 160, h 120 */
    zxv_3d_add(m, pos);
    zxv_3d_draw();
    zxv_gfx_clip(0, 0, 256, 192);
}
```

**See also:** [`zxv_3d_add`](#zxv_3d_add), [`zxv_3d_wireframe`](#zxv_3d_wireframe).

### `zxv_3d_project`

```c
int zxv_3d_project(const zxv_camera *cam, zxv_vec3 p, int *sx, int *sy, zxv_fix *scale);
```

Where a point of the world is seen with a camera: to place sprites in the
3D world (an explosion, a sign) or a crosshair. It needs no
`zxv_3d_begin` and changes nothing in the frame.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `cam` | a `zxv_camera` | The camera (with the same default values as in `zxv_3d_begin`). |
| `p` | 16.16 | The point, in the world. |
| `sx`, `sy` | pointers, or `NULL` | Where the pixel is left. |
| `scale` | pointer, or `NULL` | Where the pixels that one world unit measures at that distance are left, in 16.16 (at most 32767.99998). |

**Returns:** 1 if the point is in front of the camera (beyond `near`),
with the result; 0 if not, or if it falls more than 2³⁰ pixels away, and
then nothing is filled in. With 1, the point may be outside the view: it
is not clipped.

**Errors:** none.

**Example**

```c
#include "zxv_3d.h"
#include "zxv_gfx.h"

/* An explosion (a 16 x 16 sprite) at point p, the size of 1 unit. */
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

**See also:** [`zxv_xform_apply`](#zxv_xform_apply).

### Models from Blender: `obj2c.py`

`sdk/tools/obj2c.py` converts an `.obj` model (Blender: File > Export >
Wavefront) into C:

```sh
python3 sdk/tools/obj2c.py casa.obj casa.c house --size 2 --mat puerta=1,9,flat,decal
```

It writes `const zxv_model house` (the third argument) with its vertices
and its faces. Add `casa.c` to `SOURCES` and declare the model where you
use it:

```c
#include "zxv_3d.h"

extern const zxv_model house;

void add_house(const zxv_xform *where) { zxv_3d_add(&house, where); }
```

- It centres the model (except with `--keep-origin`) and scales it so
  that its longest side measures `--size` world units (2 by default). The
  vertices end up as integers of up to ±4096 and `scale` says how big each
  one is.
- It turns the z of the `.obj` (towards the camera) into that of `zxv_3d`
  (forwards) and flips the faces so that they are still seen from
  outside.
- The colours come from each face's material: a material called
  `zxv_DARK_LIGHT` (`zxv_2_10`) uses those, and with `_two`, `_flat` or
  `_decal` after them (`zxv_2_10_two`), those options; otherwise, the
  diffuse colour (`Kd`) of the `.mtl` gives the closest of the 16 and its
  bright or non-bright partner. With no material, `--default DARK,LIGHT`
  (1,9: blue).
- `--mat name=dark,light[,two][,flat][,decal]` changes a material;
  `--two-sided`, all faces two-sided.
- Non-convex faces (an L, a saw, a crown) and faces of more than 32 sides
  are split into triangles, which are then joined into convex pieces of up
  to 32 sides: they draw correctly and faster than if `zxv_3d_add` had to
  split them every frame. Faces with no area (repeated vertices or in a
  line) are dropped, and it warns about one that crosses itself. With more
  than 512 vertices it warns (`zxv_3d_add` would not accept it).
- Each face starts at the corner that gives its normal best, so that the
  lighting comes out right even if it has vertices in a line.
- With an `.obj` or options that are not valid (a vertex without its three
  numbers, an option without a value...), a message that says what and
  where. Comments (`#`) may also go at the end of a line.
- **For the painter to get it right:** make the pieces touch without going
  into each other, and split a big face where a small one stands on it
  (like the top of the ship of the Nave 3D example, split under the fin).
- With `--bas` it writes ZX Basic for `zxv3d.bas` instead of C (below).

### In ZX Basic: `zxv3d.bas`

In the [ZX Basic fork](../zxbasic.md), `zxv3d.bas` does the same with BASIC
commands, and `obj2c.py --bas` writes the model as two arrays and a
`SUB casa_model(n)` that sets it as model `n` (0 to 15):

```sh
python3 sdk/tools/obj2c.py casa.obj casa.bas casa --size 2 --bas
```

```basic
#include <zxv3d.bas>
#include "casa.bas"

casa_model(0)
CAMERA3D -3, 2, -7, 3640, -1820             ' where it is; turned, looking down
FOR a = 0 TO 65535 STEP 256
    BOX 5, 0, 0, 256, 80: BOX 4, 0, 80, 256, 112
    BEGIN3D: LET n = ADD3D(0, 0, 1, 0, a): DRAW3D: WAIT
NEXT a
```

The commands (`CAMERA3D`, `LIGHT3D`, `WIREFRAME3D`, `BEGIN3D`, `ADD3D`,
`DRAW3D`, `PROJECT3D`, `MAT3D`, `MODEL3D`) are the functions of this page
with the coordinates in `FIXED` and the angles in turns of 65536; the
arrays of the model are not copied and must be global. Their reference is
on the [ZX Basic page](../zxbasic.md). A whole example, with double
buffering: [Nave 3D](../../../examples/zxbasic/nave3d/README.md).
