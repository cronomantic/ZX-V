**English** · [Español](../es/zxbasic.md)

# Compiling BASIC with ZX Basic

[ZX Basic](https://zxbasic.readthedocs.io) is a BASIC compiler for the ZX
Spectrum, by José Rodríguez (Boriel), widely used to make games. There is a
version with a ZX-V target that turns a BASIC program into a native program
(`.zxv`), like the C ones of the [SDK](sdk/README.md): you load it with
`LOAD "name"` and it runs about 90 times faster than the same program in the
machine's BASIC.

It is not an official version of ZX Basic: it lives in a fork,
[cronomantic/zxbasic](https://github.com/cronomantic/zxbasic/tree/zxv),
based on version 1.19.0.

## Which BASIC it is

ZX Basic **is not ZX-V's BASIC**. It is the Spectrum's BASIC with additions
from modern BASICs: types (`UByte`, `Integer`, `Long`, `Float`, `String`...),
`SUB` and `FUNCTION` with local variables, `DO ... LOOP`, `#include`, and no
line numbers needed. A Spectrum program usually compiles as it is: `PRINT AT`,
`INK`, `PLOT`, `DRAW`, `CIRCLE`, `BEEP`, `INKEY$`, `PEEK` and `POKE` work as
there, also with the addresses of the screen (16384) and of the system
variables.

What is only in ZX-V (the modes, the two pages, sprites, shapes, the joystick
and the AY chips) is in the `zxv.bas` library, with the same names as in ZX-V's
BASIC:

```basic
#include <zxv.bas>

GRABSTR 0, 4, "1111122112211111"
FOR x = 0 TO 240 STEP 16
    SPRITE 0, x, 80
NEXT x
BOX 4, 100, 20, 20, 20
```

| Commands | |
|---|---|
| `MODE`, `PAGE`, `SCREEN`, `SCROLL`, `REMAP`, `WAIT` | The modes, the page shown and the one drawn on (double buffering), scrolling, colour changes and a steady 60 frames per second |
| `GRAB`, `GRABSTR`, `SPRITE`, `HIT` | Sprites, mirrored, recoloured or scaled, and pixel-exact collisions |
| `BOX`, `DISC`, `POLY`, `CLIP`, `PIXEL` | Filled shapes, clipping and the colour of a pixel |
| `KEY`, `JOY`, `SOUND`, `TIME` | Keys held down, joysticks, the AY registers and the frame counter |
| `MUSICLOAD`, `MUSICPLAY`, `MUSICSTOP`, `MUSICPAUSE`, `MUSICCONTINUE`, `MUSIC` | Music from a tracker (a `.pt3` from [`zxv-tracker`](sdk/README.md#the-tracker-for-the-pc-zxv-tracker)...), in the background while the program goes on |
| `SFXLOAD`, `SFX`, `SFXSTOP`, `SFXPLAYING` | Sound effects from an AYFX bank |

Where it differs from ZX-V's BASIC: the sprite of a string is `GRABSTR`
(`GRAB n, width, a$` in ZX-V's); `KEY`, `JOY`, `HIT` and `PIXEL` take
parentheses (`KEY(9)`); `REMAP` and `SOUND` take one pair at a time; `POLY`
takes up to 8 points; and music and effects are loaded with `MUSICLOAD f$` and
`SFXLOAD f$` (`LOAD f$ MUSIC` and `SFX` in ZX-V's), played with `MUSICPLAY`,
`MUSICSTOP`... and the function that says whether an effect plays is
`SFXPLAYING`.

`SAVE`, `LOAD` and `VERIFY` use the SD card as ZX-V's BASIC does: without an
extension, a `.tap` that the machine's BASIC loads too. Compiled with
`--enable-break`, Esc stops the program and errors say the line. The rest, and
the libraries there are, is in its
[target documentation](https://github.com/cronomantic/zxbasic/blob/zxv/docs/zxv.md).

## Installing it

**On Windows:** download `zxbasic-1.19.0-zxv.4-win64.zip` from the
[release page](https://github.com/cronomantic/zxbasic/releases/tag/v1.19.0-zxv.4)
and unzip it anywhere. It has `zxbc.exe`, which does not need Python.

**On Linux and macOS:** download the source code from that same page (or
clone the fork's `zxv` branch). You need Python 3.14; the compiler is
`zxbc.py`.

**Also, on all of them:** the RISC-V C compiler, the same one the SDK uses.
How to install it is in the [SDK guide](sdk/README.md#installing-the-tools).

## Your first program

Save this as `hello.bas`:

```basic
DIM x AS FLOAT
PAPER 1: INK 7: CLS
PRINT AT 10, 6; "Hello from ZX Basic"
FOR x = 0 TO 255
    PLOT x, 40 + 30 * SIN(x / 20)
NEXT x
```

Compile it:

```
zxbc --arch=zxv hello.bas
```

(`zxbc.exe` on Windows, `python3 zxbc.py` with the source code.) That gives
`hello.zxv`: copy it to the SD card (in the emulator, to the folder that acts
as the card) and load it with `LOAD "hello"`. When it ends, its report (`0 OK`
or the error) stays at the bottom until you press a key, as on the Spectrum.

**Mind the types.** Without `DIM`, a variable takes the smallest type that fits
it: here, without the first line, `x` would be a `UByte` (0 to 255). And
between integers, `/` divides as integers: `x / 20` would give 0, 0, 0... 1, 1,
1, and the curve would come out in steps. And the `FOR` would never end,
because a `UByte` never gets past 255. Declare as `FLOAT` whatever needs
decimals, and with a bigger type the counters that reach the limit.

## An example

[Stardust](../../examples/zxbasic/stardust/README.md) is a small game made
with `zxv.bas`: double buffering, sprites from strings, collisions, keyboard
or joystick and sound, at 60 frames per second.

## The licence of your programs

The compiler is AGPL 3, but what goes inside your programs (ZX Basic's runtime
and libraries, and the parts of the ZX-V SDK) is MIT: your programs can have
any licence you like, closed ones too.
