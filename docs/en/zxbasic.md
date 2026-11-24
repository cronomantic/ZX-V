**English** · [Español](../es/zxbasic.md)

# Compiling BASIC with ZX Basic

[ZX Basic](https://zxbasic.readthedocs.io) is a BASIC compiler for the ZX
Spectrum, by José Rodríguez (Boriel), widely used to make games. There is a
version with a ZX-V target that turns a BASIC program into a native program
(`.zxv`), like the C ones of the [SDK](sdk/README.md): you load it with
`LOAD "name"` and it runs about 90 times faster than the same program in the
machine's BASIC.

It is not an official version of ZX Basic: it lives in a fork,
[cronomantic/zxbasic](https://github.com/cronomantic/zxbasic/tree/zxv) (the
`zxv` branch), based on version 1.19.0. This page is the introduction; all
the detail (each command with its parameters, its errors and the version it
appeared in, the compiler options, the memory) is in the
[fork's documentation][fork].

## Which BASIC it is

ZX Basic **is not ZX-V's BASIC**. It is the Spectrum's BASIC with additions
from modern BASICs: types (`UByte`, `Integer`, `Long`, `Float`, `String`...),
`SUB` and `FUNCTION` with local variables, `DO ... LOOP`, `#include`, and no
line numbers needed. A Spectrum program usually compiles as it is: `PRINT AT`,
`INK`, `PLOT`, `DRAW`, `CIRCLE`, `BEEP`, `INKEY$`, `PEEK` and `POKE` work as
there, also with the addresses of the screen (16384) and of the system
variables.

What is only in ZX-V (the modes, the two pages, sprites, shapes, tile maps,
mode 7, the joystick, the AY chips, music and sounds) is in the `zxv.bas`
library, with the same names as in ZX-V's BASIC:

```basic
#include <zxv.bas>

GRABSTR 0, 4, "1111122112211111"
FOR x = 0 TO 240 STEP 16
    SPRITE 0, x, 80
NEXT x
BOX 4, 100, 20, 20, 20
```

Almost everything is written as in ZX-V's BASIC. What changes are details of
ZX Basic's syntax: `KEY` and `JOY` take parentheses (`KEY(9)`), and the
two-word commands have a name of their own (`MAPDIM` instead of `MAP DIM`,
`MUSICLOAD f$` instead of `LOAD f$ MUSIC`...). The
[`zxv.bas` reference][zxvbas] has an entry per command and the
[list of the differences][dif], and also [what ZX-V's BASIC has and ZX Basic
does not][nohay] (the mouse, `RASTER`, `PLAY`, the clock, the folder and file
commands, `ON ERROR`...).

Also, the [SDK libraries](sdk/libraries.md), each one in its own file
(`#include <zxvinput.bas>`...), with the same C as the programs in C: keyboard
and joysticks as actions, saving data and scores without losing them in a
power cut, palette fades, text of any size, data compressed with gzip,
integer maths, moving through a map without going through walls, things on
the mode 7 floor and 3D models. A program that does not use them does not
take a byte more. Their commands, in the [fork's documentation][libs].

`SAVE`, `LOAD` and `VERIFY` use the SD card as ZX-V's BASIC does: without an
extension, the screen goes in a `.gfx` (the ZX-V one, with its colours if they
are not the standard ones) and `CODE` and `DATA` in a `.bin`, just the bytes;
with the extension `.tap`, in a Spectrum tape. The `ROM:` and `EXT:` drives
work. The details, in [Files][files].

## If you come from the Spectrum

The program is written the same way, but the machine is a different one, and
it shows in a few things (explained in [How it works][how]):

- **Addresses are 32-bit.** `@x`, strings, `PEEK`, `POKE`, `USR` and
  `LOAD ... CODE` use `ULong` addresses. Below 65536 the memory is the
  Spectrum's, imitated: the screen at 16384, the attributes at 22528, the
  system variables (`FRAMES`, `CHARS`, `UDG`...) and the free memory up to
  65535, as in ZX-V's BASIC. Above it, the addresses are real: ZX-V's RAM,
  its video memory and its registers. It is explained in
  [Memory below 64K][mem].
- **`Float` is different.** It is an 8-byte `double`, with about 15 digits,
  not the 5-byte number of the Spectrum's ROM; it gives the same results as
  ZX-V's BASIC. `Fixed` and the integers are the same as on the Spectrum.
- **There is no Z80 code.** `ASM` blocks are RISC-V and `USR` calls RISC-V
  code: Spectrum machine code and the ZX Basic libraries written in Z80
  assembler do not work. The ones the fork brings for ZX-V (`string.bas`,
  `keys.bas`, `attr.bas`...) are called and work the same.
- **Time.** `PAUSE` and `FRAMES` still count fiftieths of a second, as on the
  Spectrum, even though ZX-V makes 60 frames per second; `TIME` and `WAIT`,
  from `zxv.bas`, count those 60.
- **The Spectrum's screen** is the top-left 256 × 192 pixels of the drawing
  page, in any mode: `PRINT`, `PLOT`, `DRAW`, `CIRCLE` and `CLS` do not go
  outside it. The shapes, sprites and maps of `zxv.bas` reach the whole page.
- **Memory.** A program has 244 KB (code, data, heap and stack); the heap is
  all that is left over, without `-H`. See
  [How much memory a program has][cuanta].
- **Compiler options.** There is no `tap`, `tzx` or `sna`: the output is a
  `.zxv` (or the assembler, with `-f asm`). `--org` and `-H` are accepted but
  do nothing. **Array indices are only checked with `--debug-array`** (it
  gives `3 Subscript wrong`); without it, one out of range reads or writes
  other memory without an error. The list, in [Compiler options][opciones].

## The versions

This page is the one for **v1.19.0-zxv.7**, which comes out together with
ZX-V 0.11.0: `zxv.bas` with the modes, the pages, sprites, shapes, tile maps
(`TILE`, `MAPDIM`, `MAP`, `CELL`...), mode 7 (`TEXTURE`, `FLOOR`,
`FLOORMAP`, `ROTATE`, `ROTATEMAP`), patterns (`PATTERN`, `PATTERNBITS`), the
keyboard, the joysticks, the AY chips, music (`.zxm`, PT3, TurboSound,
VGM...), effects and recorded sounds; `SAVE`, `LOAD` and `VERIFY`; the SDK
libraries (`zxvinput.bas`... `zxv3d.bas`); and the `--loading-screen`,
`--embed` and `--code-bank` options. Its programs need ZX-V 0.7.0 or later,
and recorded sounds (`SAMPLE`), ZX-V 0.10.0 or later. Each entry of the
[fork's documentation][fork] says which version it exists since; an earlier
version of the fork does not have what came later.

Some things also need **a ZX-V ROM of specification 0.2**, that of ZX-V
0.11.0 or later (0.10.0 and the earlier ones are specification 0.1): code
banks, `PATTERN`, `PATTERNBITS`, `ROTATE`, `FLOORMAP`, `ROTATEMAP`
and the shading of `zxv3d.bas`. With an earlier ROM the program loads, but a
bank stops it with "the ROM has no banks", patterns come out in a plain
colour and those mode 7 planes are not drawn properly. Each entry of the
fork's documentation says which ROM it needs.

## Installing it

**On Windows:** download `zxbasic-1.19.0-zxv.7-win64.zip` from the
[release page](https://github.com/cronomantic/zxbasic/releases/tag/v1.19.0-zxv.7)
and unzip it anywhere. It has `zxbc.exe`, which does not need Python.

**On Linux and macOS:** download the source code from that same page, or
clone the fork's `zxv` branch (`git clone -b zxv https://github.com/cronomantic/zxbasic`). You need
Python 3.14; the compiler is `zxbc.py`.

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
The reports are in English, as in ZX Basic. Compiled with `--enable-break`,
Esc stops the program and errors say the line.

**Mind the types.** Without `DIM`, a variable takes the smallest type that fits
it: here, without the first line, `x` would be a `UByte` (0 to 255). And
between integers, `/` divides as integers: `x / 20` would give 0, 0, 0... 1, 1,
1, and the curve would come out in steps. And the `FOR` would never end,
because a `UByte` never gets past 255. Declare as `FLOAT` whatever needs
decimals, and with a bigger type the counters that reach the limit. The same
goes for arguments: if a parameter of `zxv.bas` is a `UByte`, a 257 arrives
as 1, without an error.

## More compiler options

These three only exist with `--arch=zxv`:

- **`--loading-screen title.png`** (since v1.19.0-zxv.5): a loading screen, a
  PNG or a `.gfx` with a screen, as with `elf2zxv.py --screen`. The machine
  shows it while it loads the program. See [Loading screen][carga].
- **`--embed song.pt3`** (since v1.19.0-zxv.6; as many as needed: music,
  effects, sounds, `.gfx`, data): the `.zxv` carries those files inside, as
  the C games do. `LOAD`, `VERIFY`, `MUSICLOAD`, `SFXLOAD` and `SAMPLELOAD`
  use the file in the current folder if there is one and, if not, the one
  inside: the program works alone, also from `ROM:`, and a file on the card
  with the same name takes the place of the one inside. See
  [Files inside the program][dentro].
- **`--code-bank stage1=stage1.bas`** (since v1.19.0-zxv.7; it needs ZX-V
  0.11.0 or later): the `SUB`s and `FUNCTION`s of that file go in a
  **code bank**, which is loaded from the `.zxv` itself the first time one of
  them is called, into a region of memory that the banks of that region share
  (`stage2@1=stage2.bas`, region 1). So a game can keep the code of each stage
  in its own file, and only the one being played takes memory. It is still
  one program: the global variables stay in the resident part and go from one
  stage to the next. If a function of a bank calls one of another bank in the
  same region, the program stops with "Bank a in use": put them in different
  regions. See [Code banks][bancos] and the
  [`fases` example](../../examples/zxbasic/fases/README.md).

## The examples

[Stardust](../../examples/zxbasic/stardust/README.md) is a small game made
with `zxv.bas`: double buffering, sprites from strings, collisions, keyboard
or joystick and sound, at 60 frames per second.

[Petit Soccer](../../examples/zxbasic/petitsoccer/README.md) is a big one:
seven-a-side football against the machine or between two players, with a
pitch bigger than the screen and scroll, sprites from a PNG sheet, and music
and effects from a tracker. It carries its music and effects inside
(`zxbc --embed`, since v1.19.0-zxv.6), so it also runs from the `ROM:` drive,
where it comes built in: `LOAD "ROM:petitsoccer"`.

[Fases](../../examples/zxbasic/fases/README.md) is the one about code banks:
two stages, each one in its own file and its own bank. It needs v1.19.0-zxv.7
of the fork and ZX-V 0.11.0 or later.

[Nave 3D](../../examples/zxbasic/nave3d/README.md) is the 3D one: a ship of
flat faces, made as an `.obj` and turned into BASIC with `obj2c.py --bas`,
that turns with light among stars. It needs v1.19.0-zxv.7 of the fork; with a
ROM older than ZX-V 0.11.0, the faces come out without the shading patterns.

## The licence of your programs

The compiler is AGPL 3, but what goes inside your programs (ZX Basic's runtime
and libraries, and the parts of the ZX-V SDK) is MIT: your programs can have
any licence you like, closed ones too.

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
