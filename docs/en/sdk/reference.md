**English** · [Español](../../es/sdk/reference.md)

# SDK reference

Everything the SDK offers, grouped by topic: the functions of the system
(the API), the graphics engine, the registers and constants of the
machine, the memory map and the file formats. To learn how to use them,
read the [SDK guide](README.md) and [Graphics](graphics.md). The optional
libraries of `sdk/lib` (fixed-point maths, keyboard and gamepads at once,
saving data, fonts, 3D...) are not in the ROM and have their own
reference: [The SDK libraries](libraries.md).

**Contents**

- [The files of the SDK](#the-files-of-the-sdk)
- [Conventions](#conventions)
- [System and timing](#system-and-timing)
- [Screen and text](#screen-and-text)
- [Keyboard, joystick and mouse](#keyboard-joystick-and-mouse)
- [Sound](#sound)
- [Music and effects](#music-and-effects)
- [Files and folders](#files-and-folders)
- [Banks](#banks)
- [Clock and language](#clock-and-language)
- [Ending a program and running others](#ending-a-program-and-running-others)
- [Graphics engine](#graphics-engine)
- [`.gfx` graphics files](#gfx-graphics-files)
- [Video registers](#video-registers)
- [Input registers](#input-registers)
- [Sound registers](#sound-registers)
- [System variables](#system-variables)
- [Error codes](#error-codes)
- [API function numbers](#api-function-numbers)
- [Memory map](#memory-map)
- [The `.zxv` file format](#the-zxv-file-format)
- [Tools](#tools)
- [The standard C library (picolibc)](#the-standard-c-library-picolibc)
- [Internal definitions](#internal-definitions)

## The files of the SDK

Everything is in the `sdk` folder of the repository.

**The headers and the code that goes into your program**

| File | Contents |
|---|---|
| `include/zxv.h` | Addresses, registers and constants of the machine, as `#define` only, so it also works in assembly (`.S` files) |
| `include/zxv_api.h` | The API as `static inline` C functions, which call the ROM through the [API table](#api-function-numbers); the sample channels, the mouse pointer (`zxv_mouse_t`) and the types `zxv_dirent_t` and `zxv_clock_t`. Includes `zxv.h`, `<stdint.h>` and `<stddef.h>` (so `NULL` and `size_t` are available) |
| `include/zxv_gfx.h` | The [graphics engine](#graphics-engine): types, constants and functions. Includes `zxv_api.h` (define `ZXV_GFX_NO_API` first to get only the types and constants) |
| `include/zxv_gfxfile.h`, `gfxfile.c` | Reads [`.gfx` graphics files](#gfx-graphics-files) (from `zxv-gfx`, the ROM's editor and `png2gfx.py`): sprites, tiles, maps and textures, in the `zxv_gfx.h` formats, and screens |
| `include/zxv_music.h`, `music/` | [Music and effects](#music-and-effects): the own `.zxm` format, PT3, TurboSound, PT2, STC, ASC, PSG and VGM modules, and AYFX effects |
| `include/zxv_bank.h`, `zxvbank.c` | Data [banks](#banks): `zxv_bank_load` and `zxv_bank_size` |
| `zxvbankcode.c` | Code banks: what the bridges and `zxv_bank_preload` do. `tools/zxvbanks.py` links it; there is no need to add it by hand |
| `include/zxv_version.h` | The ZX-V version this SDK was released with (`ZXV_VERSION`, for example `"0.12.0"`, and `ZXV_VERSION_MAJOR`, `_MINOR` and `_PATCH`); see [`zxv_api_version`](#zxv_api_version) |
| `include/sys/dirent.h` | `struct dirent` and `DIR` for picolibc's `<dirent.h>` |
| `lib/` and its headers | The [optional libraries](libraries.md): `zxv_math.h` (`lib/math.c`), `zxv_m7.h` (`lib/m7.c`), `zxv_mapmove.h` (`lib/mapmove.c`), `zxv_fade.h` (`lib/fade.c`), `zxv_input.h` (`lib/input.c`), `zxv_save.h` (`lib/save.c`), `zxv_inflate.h` (`lib/inflate.c`), `zxv_font.h` (`lib/font.c` and `lib/font_rom.c`) and `zxv_3d.h` (`lib/3d.c`) |
| `sound/` | The machine's sound, the same code that plays in the Pico ULA, in the emulator and in `zxv-tracker`: the two AY chips (`aysynth`), the twelve sample channels (`smpsynth`, with the ADPCM encoder `smp_adpcm_encode`) and the mix (`sndsynth`, with the registers of `sndregs.h`) |
| `crt0.S` | Start-up code: clears the variables in the hidden VRAM (`ZXV_VRAM0_DATA`, `ZXV_VRAM1_DATA`), sets the `tp` register (thread variables, used by picolibc's `errno`) and jumps to `main` |
| `zxv.ld` | Linker script: everything contiguous at the start of the user RAM, the hidden VRAM sections and the heap for picolibc |
| `zxvlib.c` | `memcpy`, `memmove`, `memset`, `memcmp`, for programs without the C library |
| `zxvc.c` | The glue between picolibc and the API (files, folders, console, heap, time, exit): see [The standard C library](#the-standard-c-library-picolibc) |

**The tools** (see also [Tools](#tools))

| File | Contents |
|---|---|
| `tools/elf2zxv.py` | Converts the linked `.elf` into a `.zxv`, with the loading screen and the data banks (`--bank`, `--pack`) |
| `tools/zxvbanks.py` | Builds a program with code banks: compiles each bank separately, generates the bridges, links and calls `elf2zxv.py` |
| `tools/zxvbanks-gdb.py` | For GDB: loads and drops the symbols of each code bank as the banks are loaded on the machine |
| `tools/png2gfx.py` | Converts PNG images into a `.gfx` graphics file |
| `tools/obj2c.py` | Converts an `.obj` model (Blender and almost every 3D program) into C for `zxv_3d.h` |
| `tools/fontconv.py` | Converts bitmap fonts (Spectrum `.ch8`, `.psf`, `.bdf`, `.hex`, dumps...) into C for `zxv_font.h` |
| `tools/mkweb.py` | The web page of a single game (see [Publishing your game on the web](web.md)) |
| `tools/gfx/` | `zxv-gfx`, the graphics editor for the PC |
| `tools/tracker/` | `zxv-tracker`, the tracker for the PC: `.zxm` and PT3 music, and AYFX effects |
| `tools/music/` | `zxv-music`, the converter from PT3, TurboSound, MOD, S3M and XM to the `.zxm` format |
| `tools/link/` | `zxv-link` and `zxv-link-gui` (with a window), remote loading: they send programs to the board through the J5 header and run them, and serve a folder on the PC as the `EXT:` drive |

**Everything else**

| File | Contents |
|---|---|
| `examples/` | The examples, one per folder (`hello`, `graphics`, `samples`, `fases`...) |
| `Makefile` | Builds the examples (`make`, `make run SD=folder`, `make debug SD=folder`) |
| `README.md`, `LICENSE` | A short introduction and the licence (MIT): your programs can use the SDK however you like |

## Conventions

- All functions follow the standard RISC-V `ilp32` calling convention.
  Those of `zxv_api.h` are `inline`: they call the ROM through the
  [API table](#api-function-numbers), or do the work themselves with the
  registers (the mouse, the sample channels).
- Functions that can fail return an `int`: **0 or more** on success and a
  **negative** [error code](#error-codes) on failure.
- "The drawing page" is the video page chosen with `zxv_gfx_draw_page` (0
  at the start of a program); "the visible page" is the one chosen with
  `zxv_set_page`.
- Registers are memory: read and write them through `volatile` pointers,
  for example `*(volatile uint8_t *)(uintptr_t)ZXV_V_BORDER = 2;`. Video
  registers take effect at the next VBLANK.
- Each entry says **since** which version of the machine specification it
  exists and, if it is an API function, its number. A program that uses
  something from specification 0.2 must check the ROM's version first:
  see [`zxv_api_version`](#zxv_api_version). A function the ROM does not
  have does nothing and returns −1. What arrived in a ROM later than the
  first published version (0.4.0) says so too: "ROM 0.6.0 or later".
- What is not in the ROM (the banks, the music, the libraries of
  `sdk/lib`) is compiled with your program: it has no "Since", but says
  which ROM it needs if it needs a particular one.

## System and timing

### `zxv_wait_vblank`

```c
void zxv_wait_vblank(void);
```

Waits for the next VBLANK, the start of a frame: there are 60 per second
(`ZXV_FRAME_HZ`). It is a game's clock: do one frame's work and wait for
the next.

It turns interrupts on if they were off (it needs them to notice the
VBLANK). **Do not call it from a frame hook**: the hook runs with
interrupts off, and it would wait for ever.

**Example**

```c
for (;;) {
    move_everything();
    draw_everything();
    zxv_wait_vblank();              /* 60 rounds a second */
}
```

**Since:** specification 0.1 · API function no. 1.

**See also:** [`zxv_set_frame_hook`](#zxv_set_frame_hook),
[`zxv_time_us`](#zxv_time_us), the system variable `ZXV_SV_FRAMES`
([System variables](#system-variables)).

### `zxv_set_frame_hook`

```c
void zxv_set_frame_hook(void (*fn)(void));
```

Makes the system call `fn` at every VBLANK, from the interrupt (with
interrupts off), even if the program is busy with something else. It is
for what has to happen every frame without fail: the music, a counter.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `fn` | a `void f(void)` function, or `0` | The function called at every VBLANK; `0` removes the one there was. There is only one hook: setting another replaces the previous one. |

**Rules of the hook:**

- Keep it short: while it runs, there are no other interrupts.
- It cannot call `zxv_wait_vblank` or anything that uses it (such as
  `zxv_smp_upload`), nor load a code bank: the program would stop with an
  error.
- Variables it shares with the rest of the program must be `volatile`.
- Remove it before your program ends. The ROM removes it when going back
  to BASIC, but not when going back to the system monitor.

**Example**

```c
static volatile uint32_t frames;

static void every_frame(void)
{
    frames++;
}

zxv_set_frame_hook(every_frame);
/* ... the game ... */
zxv_set_frame_hook(0);              /* before ending */
```

**Since:** specification 0.1 · function no. 2.

**See also:** [`zxv_wait_vblank`](#zxv_wait_vblank),
[`zxv_music_update`](#zxv_music_update), the system variable
`ZXV_SV_FRAME_HOOK`.

### `zxv_time_us`

```c
uint32_t zxv_time_us(void);
```

A microsecond counter, which never stops and is never set. It is for
measuring times and for giving the time to the sound's [timed
queue](#zxv_ay_event).

**Returns:** the microseconds of the counter. It wraps round every 2³² µs
(about 71 minutes): compare times by subtracting them as unsigned numbers,
and the subtraction comes out right even if it wrapped round in between.

**Errors:** none.

**Example**

```c
uint32_t t0 = zxv_time_us();
do_something();
uint32_t elapsed = zxv_time_us() - t0;     /* in µs, even if it wraps round */
```

**Since:** specification 0.1 · function no. 3.

**See also:** [`zxv_wait_vblank`](#zxv_wait_vblank),
[`zxv_clock_get`](#zxv_clock_get-zxv_clock_set) (the date and time).

### `zxv_api_version`

```c
uint32_t zxv_api_version(void);
```

The version of the machine specification that the ROM the program runs on
meets. It is what says which functions and which system variables there
are.

**Returns:** `major << 16 | minor`. The current ROM gives **3** (version
0.3); earlier ones, 2 (0.2) or 1 (0.1). It is the same as the system variable
`ZXV_SV_VERSION`. `ZXV_SPEC_VERSION` (3) is the version of the headers you
compile with, not that of the machine.

**Errors:** none.

**Checking the version.** A program compiled with this SDK can end up on a
machine with an older ROM. Before using what arrived with specification
0.2 (the pattern of the shapes, `zxv_m7_draw_map`, `zxv_m7_affine`, the
banks, `zxv_report_error` messages of more than 47 bytes...), check that
the ROM has it. With a 0.1 ROM, a 0.2 function does nothing (returns −1),
and the banks give `ZXV_E_ACCESS`. The same with what arrived with 0.3 (`JOY2`,
`JOY3`, fire 4, L and R): with an older ROM it reads 0. Specification 0.3 is
that of ZX-V 0.12.0 and later; 0.2, that of 0.11.0; 0.1, that of the earlier
ones.

**The ZX-V version is not this.** `ZXV_VERSION` (from `zxv_version.h`, for
example `"0.12.0"`) is the version of the ZX-V programs (the ROM, the
emulator and the firmware, which are released together) this SDK came out
with. It is a fixed string compiled into your program: it does not say
which ROM the machine has, and many ZX-V versions meet the same
specification. To know what the machine can do, use `zxv_api_version`.

**Example**

```c
static const uint8_t pattern[8] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };

if (zxv_api_version() >= 2)         /* specification 0.2 or later */
    zxv_gfx_pattern(pattern, 0);    /* patterned shapes */
```

**Since:** specification 0.1 · function no. 0.

**See also:** [Constants](#constants),
[API function numbers](#api-function-numbers).

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_FRAME_HZ` | 60 | Frames per second (the VBLANKs) |
| `ZXV_CPU_HZ_REF` | 144000000 | Reference speed of the processor (Hz); the real one is in the system variable `ZXV_SV_CPU_HZ` |
| `ZXV_SPEC_VERSION` | 3 | Version of the specification of these headers (0.3) |
| `ZXV_VERSION` | `"0.12.0"` | ZX-V version the SDK came out with, in `zxv_version.h` (with `ZXV_VERSION_MAJOR`, `_MINOR`, `_PATCH`) |

## Screen and text

### `zxv_set_mode`

```c
void zxv_set_mode(int mode);
```

Changes the video mode from the next VBLANK and clears the drawing page
with the paper colour of the text (see
[`zxv_text_colors`](#zxv_text_colors-zxv_text_at)). The text cursor goes
back to the top left.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `mode` | 0 or 1 | 0: 256 × 192 pixels; 1: 512 × 384. Only bit 0 counts (2 is mode 0, 3 is mode 1). |

Mode 1 uses the whole video page (96 KB): whatever the program has in the
hidden VRAM (`ZXV_VRAM0_DATA`, `ZXV_VRAM1_DATA`, see [Memory
map](#memory-map)) is lost. In mode 0 with the virtual playfield
(`ZXV_V_VSIZE`), the whole virtual page is cleared.

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_MODE0_W`, `ZXV_MODE0_H` | 256, 192 | The mode 0 screen |
| `ZXV_MODE1_W`, `ZXV_MODE1_H` | 512, 384 | The mode 1 screen |
| `ZXV_SCREEN_PIXELS(mode)` | 24,576 or 98,304 | The bytes of the pixels of a screen (4 bits per pixel) |
| `ZXV_SCREEN_SIZE(mode)` | 24,580 or 98,308 | The size of a screen saved without palette: the header (`ZXV_SCREEN_HEAD`, 4) and the pixels; with palette, `ZXV_SCREEN_SIZE_F` (see [The `.zxv` file format](#the-zxv-file-format)) |

**Example**

```c
zxv_text_colors(7, 1);              /* the blue paper... */
zxv_set_mode(1);                    /* ...fills the 512 x 384 screen */
```

**Since:** specification 0.1 · function no. 4.

**See also:** [`zxv_set_page`](#zxv_set_page), [`zxv_cls`](#zxv_cls),
the `ZXV_V_MODE` register ([Video registers](#video-registers)).

### `zxv_set_page`

```c
void zxv_set_page(int page);
```

Chooses the video page that is shown, from the next VBLANK (writes
`ZXV_V_PAGE`). It does not change the page you draw on: for double
buffering, you draw on one and show the other.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `page` | 0 or 1 | The page. Only bit 0 counts. |

**Example**

```c
int drawing = 1;
for (;;) {
    zxv_gfx_draw_page(drawing);     /* draw on the one not shown */
    draw_everything();
    zxv_set_page(drawing);          /* and show it at the VBLANK */
    zxv_wait_vblank();
    drawing ^= 1;
}
```

**Since:** specification 0.1 · function no. 5.

**See also:** [`zxv_set_mode`](#zxv_set_mode),
`zxv_gfx_draw_page` ([Graphics engine](#graphics-engine)).

### `zxv_cls`

```c
void zxv_cls(int colour);
```

Fills the drawing page with a colour and moves the text cursor to the top
left. It does not change the text colours.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `colour` | 0–15 | The colour ([the palette](#palette)). Only the low 4 bits count. |

In mode 0 with the virtual playfield (`ZXV_V_VSIZE`) it fills the whole
virtual page.

**Example**

```c
zxv_cls(1);                         /* all blue */
```

**Since:** specification 0.1 · function no. 6.

**See also:** [`zxv_set_mode`](#zxv_set_mode), `zxv_gfx_rect`
([Graphics engine](#graphics-engine)).

### `zxv_plot`

```c
void zxv_plot(int x, int y, int colour);
```

Sets one pixel of the drawing page. It is simple and slow: to draw a lot,
use the [graphics engine](#graphics-engine).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x`, `y` | inside the page | The pixel, counting from the top left. A point outside the page is not drawn (it is not an error). |
| `colour` | 0–15 | The colour. Only the low 4 bits count. |

**Example**

```c
for (int x = 0; x < ZXV_MODE0_W; x++)
    zxv_plot(x, 96, 2);             /* a red line */
```

**Since:** specification 0.1 · function no. 7.

**See also:** `zxv_gfx_line` ([Graphics engine](#graphics-engine)).

### `zxv_print_char`, `zxv_print_str`

```c
void zxv_print_char(int c);
void zxv_print_str(const char *s);
```

Print a character or a string at the text cursor of the drawing page, in
cells of 8 × 8 pixels, with the colours of `zxv_text_colors`, and move the
cursor past it. At the end of a line they carry on in the next one, and
when the text goes past the last row, the screen scrolls up one row.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `c` | a Unicode code point | The character (`'A'`, `0xF1` for `ñ`...). |
| `s` | zero-terminated UTF-8 string | The text. UTF-8 is the encoding of C source files, so you can write `"¡Hola!"` in your program. |

The characters that are drawn:

| Character | What comes out |
|---|---|
| ASCII 32 to 126 | The character |
| `©` (0xA9) | `©` (also with 127) |
| `á é í ó ú ü ñ`, `Á É Í Ó Ú Ü Ñ`, `¿`, `¡` | The letter, from the ROM's font |
| `'\n'` or `'\r'` | Goes to the start of the next line. Each one is a new line: `"\r\n"` leaves a blank line |
| `'\b'` | Deletes the previous character and moves back (in column 0, nothing) |
| Any other (`'\t'`, other control characters, other letters) | `?` |

A malformed UTF-8 sequence comes out as `?`.

The rows and columns depend on the mode: 32 × 24 cells in mode 0 and
64 × 48 in mode 1 (in mode 0 with the virtual playfield, those of the
virtual page).

**Example**

```c
zxv_print_str("Lives: ");
zxv_print_char('0' + lives);
zxv_print_char('\n');
```

**Since:** specification 0.1 · functions no. 8 and 9.

**See also:** [`zxv_text_colors`, `zxv_text_at`](#zxv_text_colors-zxv_text_at),
the fonts of [`zxv_font.h`](libraries.md#fonts-zxv_fonth).

### `zxv_text_colors`, `zxv_text_at`

```c
void zxv_text_colors(int ink, int paper);
void zxv_text_at(int col, int row);
```

`zxv_text_colors` sets the colours of the text that `zxv_print_char` and
`zxv_print_str` write from then on: each cell is filled with the paper and
the character drawn in the ink. `zxv_text_at` moves the text cursor to
another cell.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ink` | 0–15 | The ink. Only the low 4 bits count. |
| `paper` | 0–15 | The paper. It is also the colour `zxv_set_mode` clears the screen with. |
| `col` | 0–31 in mode 0, 0–63 in mode 1 | The column, from 0 on the left. |
| `row` | 0–23 in mode 0, 0–47 in mode 1 | The row, from 0 at the top. |

A position off the screen is not an error: what falls outside is not
drawn, and if the cursor is below the last row, the next character
scrolls the screen up and carries on in the last row.

Every program starts with black on white (0 and 7) and the cursor at the
top left, whatever the previous one left: the ROM sets them like that when
it loads each `.zxv`. `zxv_cls` and `zxv_set_mode` also put the cursor back
at the top left.

**Example**

```c
zxv_text_colors(14, 1);             /* bright yellow on blue */
zxv_text_at(8, 10);
zxv_print_str("¡Hola, señor Núñez!");
```

**Since:** specification 0.1 · functions no. 53 and 54.

**See also:** [`zxv_print_char`, `zxv_print_str`](#zxv_print_char-zxv_print_str),
[Palette](#palette).

### Palette

| Number | Colour | RGB | Number | Colour | RGB |
|---|---|---|---|---|---|
| 0 | Black | `000000` | 8 | Black (transparent in the engine) | `000000` |
| 1 | Blue | `0000D7` | 9 | Bright blue | `0000FF` |
| 2 | Red | `D70000` | 10 | Bright red | `FF0000` |
| 3 | Magenta | `D700D7` | 11 | Bright magenta | `FF00FF` |
| 4 | Green | `00D700` | 12 | Bright green | `00FF00` |
| 5 | Cyan | `00D7D7` | 13 | Bright cyan | `00FFFF` |
| 6 | Yellow | `D7D700` | 14 | Bright yellow | `FFFF00` |
| 7 | White | `D7D7D7` | 15 | Bright white | `FFFFFF` |

The bits of the number are bright, green, red and blue (from bit 3 to
bit 0).

These are the palettes' colours at the start of every program. Each
remapping table has its own palette of 16 RGB565 colours (u16: red in bits
15–11, green in 10–5 and blue in 4–0) at `ZXV_PALETTE(t)`, which can be
changed: see [Video registers](#video-registers). `ZXV_RGB(r, g, b)` gives
the RGB565 of a colour with components from 0 to 255 (215, `D7`, is the
normal level: `ZXV_RGB(215, 0, 0)` is red) and `ZXV_STD_RGB565(i)`, index
`i`'s usual colour. When it loads each `.zxv`, the ROM puts the four
palettes and the remapping tables back as they were at the start.

## Keyboard, joystick and mouse

### `zxv_key_get`

```c
int zxv_key_get(void);
```

Takes the next key from the key queue. The queue keeps the key presses in
order (64 at most; if it fills up, new ones are lost), so none is missed
even if the program is slow to look. To know which keys **are held down**
now (a game), read the [input registers](#input-registers) `ZXV_I_KEYMAP`
or use
[`zxv_input.h`](libraries.md#keyboard-and-gamepads-at-once-zxv_inputh).

**Returns:** the code of the key (table), or −1 if the queue is empty.

| Code | Key |
|---|---|
| Unicode code of the character | A key that types a character, with the keyboard layout (`'a'`, `'A'`, `'ñ'` = 0xF1, `'@'`...) |
| 0x0D, 0x08, 0x09, 0x1B | Enter, Backspace, Tab, Esc |
| `ZXV_KEY_SPECIAL` (0xF000) \| HID code | The keys that do not type (below) |

| Code | Key | Code | Key |
|---|---|---|---|
| 0xF03A–0xF045 | F1–F12 | 0xF04C | Delete |
| 0xF046 | Print Screen | 0xF04D | End |
| 0xF047 | Scroll Lock | 0xF04E | Page Down |
| 0xF048 | Pause | 0xF04F | → |
| 0xF049 | Insert | 0xF050 | ← |
| 0xF04A | Home | 0xF051 | ↓ |
| 0xF04B | Page Up | 0xF052 | ↑ |
| 0xF068–0xF073 | F13–F24 | | |

Keys held down repeat (after 500 ms, and then 20 times a second): those
that type, Enter, Backspace, Tab, Delete, End, Page Down and the cursors.
Accented letters cannot be typed: the accent and the letter give the plain
letter. Ctrl+Alt+Delete never arrives (it restarts the machine), nor does
Ctrl+Alt+F12 (the board's diagnostics page).

**Errors:** none.

**Example**

```c
int k;
while ((k = zxv_key_get()) >= 0) {
    if (k == 0x1B)
        quit = 1;                           /* Esc */
    else if (k == (int)(ZXV_KEY_SPECIAL | 0x52))
        go_up();                            /* cursor up */
}
```

**Since:** specification 0.1 · function no. 10.

**See also:** [Input registers](#input-registers),
[`zxv_joy_read`](#zxv_joy_read).

### `zxv_joy_read`

```c
uint32_t zxv_joy_read(int n);
```

The state of a USB gamepad: which directions and buttons are pressed right
now. It is the same as reading the registers `ZXV_I_JOY0` to
`ZXV_I_JOY3`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–3 | The gamepad: the first one connected is 0, the second, 1, and so on up to 3; any other value reads 0. 2 and 3 since specification 0.3 (before, they always read 0); up to ZX-V 0.11, any value other than 0 read 1. |

**Returns:** the bits of the table, set for those pressed;
`ZXV_JOY_CONNECTED` says there is a gamepad. A disconnected gamepad reads
0.

| Constant | Bit | Constant | Bit |
|---|---|---|---|
| `ZXV_JOY_RIGHT` | 0 | `ZXV_JOY_START` | 7 |
| `ZXV_JOY_LEFT` | 1 | `ZXV_JOY_SELECT` | 8 |
| `ZXV_JOY_DOWN` | 2 | `ZXV_JOY_FIRE4` | 9 |
| `ZXV_JOY_UP` | 3 | `ZXV_JOY_L` | 10 |
| `ZXV_JOY_FIRE1` | 4 | `ZXV_JOY_R` | 11 |
| `ZXV_JOY_FIRE2` | 5 | `ZXV_JOY_CONNECTED` | 15 |
| `ZXV_JOY_FIRE3` | 6 | | |

`ZXV_JOY_FIRE4`, `ZXV_JOY_L` and `ZXV_JOY_R` since specification 0.3 (before, 0).

Direction comes from the d-pad or the left stick (past a quarter of its
travel). On a PlayStation pad, ✕, ○, □ and △ are fire 1 to 4, L1 is L and
R1 is R; Share/Create is Select and Options is Start. On an Xbox pad (360,
One or Series), A, B, X and Y are the fire buttons, LB and RB are L and R.
On a generic pad, buttons 1–4 are the fire buttons, 5 is L, 6 is R, 9 is
Select and 10 Start.

**Errors:** none.

**Example**

```c
uint32_t j = zxv_joy_read(0);
if (j & ZXV_JOY_LEFT)
    x--;
if (j & ZXV_JOY_FIRE1)
    fire();
```

**Since:** specification 0.1 · function no. 11.

**See also:** [`zxv_key_get`](#zxv_key_get),
[`zxv_input.h`](libraries.md#keyboard-and-gamepads-at-once-zxv_inputh).

### The mouse: `zxv_mouse_t`, `zxv_mouse_init`, `zxv_mouse_update`

```c
typedef struct {
    int x, y;                   /* the pointer, in pixels */
    int wheel;                  /* the wheel, accumulated (positive upwards) */
    uint8_t buttons;            /* bits 0-2: left, right and middle */
    uint16_t rx, ry;            /* internal: the counters at the last call */
    uint8_t rw;
} zxv_mouse_t;

void zxv_mouse_init(zxv_mouse_t *p, int x, int y);
void zxv_mouse_update(zxv_mouse_t *p, int w, int h);
```

A pointer in pixels that follows the USB mouse, like `MOUSE` in BASIC.
`zxv_mouse_init` puts the pointer at (x, y) and remembers the mouse's
counters. `zxv_mouse_update`, once per frame, adds the movement since the
last call (one mouse unit is one pixel, without acceleration), reads the
buttons and the wheel, and keeps the pointer inside the area. They read
the [input registers](#input-registers) `ZXV_I_MOUSE_...` directly;
without a mouse, the pointer does not move.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `p` | | The pointer, a structure of the program. |
| `x`, `y` | | Where the pointer starts. |
| `w`, `h` | 1 or more | The size of the area: the pointer stays in [0, w) × [0, h). Usually that of the screen. |

**Example**

```c
zxv_mouse_t m;
zxv_mouse_init(&m, ZXV_MODE0_W / 2, ZXV_MODE0_H / 2);
for (;;) {
    zxv_mouse_update(&m, ZXV_MODE0_W, ZXV_MODE0_H);
    if (m.buttons & 1)
        zxv_plot(m.x, m.y, 2);              /* paint with the left button */
    zxv_wait_vblank();
}
```

**Since:** specification 0.1 (they are in `zxv_api.h`; they are not API
functions).

**See also:** [Input registers](#input-registers).

## Sound

The sound is two AY chips, the beeper and twelve [sample
channels](#sample-channels), mixed at the output. The AY and the sample
channels are written through two queues: one that plays as soon as it can
(`zxv_ay_write`) and another in which each write carries its time
(`zxv_ay_event`). The volumes and the stereo are in the [sound
registers](#sound-registers).

### `zxv_ay_write`

```c
void zxv_ay_write(int reg, int val);
```

Writes to a register of an AY (or of a sample channel). The write goes
through a queue of 256 entries and sounds a few milliseconds later, in
order with the others; if the queue is full, the function waits. Writing
register 13 restarts the envelope, as on the real chip.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `reg` | 0–15, the first AY; `ZXV_AY2` + 0–15 (16–31), the second | The register (table). The numbers of the sample channels (`ZXV_SMP_REG(ch, reg)`, 64–159) and `ZXV_SMP_KEYON_HI` and `ZXV_SMP_KEYON` (36, 37) also work. Other numbers are ignored. |
| `val` | 0–255 | The value. Bits the register does not have are dropped (register 1 only keeps 4, for example). |

| Register | Contents |
|---|---|
| 0, 1 | Tone period of channel A: low 8 bits, high 4 bits |
| 2, 3 | Tone period of channel B |
| 4, 5 | Tone period of channel C |
| 6 | Noise period (5 bits) |
| 7 | Mixer: bits 0–2 = 1 turn off the tone of A, B, C; bits 3–5 = 1 turn off the noise of A, B, C |
| 8, 9, 10 | Volume of A, B, C (0–15); bit 4 = follow the envelope |
| 11, 12 | Envelope period: low and high byte |
| 13 | Envelope shape (0–15) |

The tone frequency is 1,773,447.5 / (16 × period) Hz; the envelope
repeats every 256 × period / 1,773,447.5 seconds. Both chips start silent,
and the ROM silences them when a program goes back to BASIC.

**Example**

```c
zxv_ay_write(0, 252);               /* A 440 Hz on channel A: period 252 */
zxv_ay_write(1, 0);
zxv_ay_write(7, 0x3E);              /* only the tone of A */
zxv_ay_write(8, 15);                /* at full volume */
```

**Since:** specification 0.1 · function no. 12.

**See also:** [`zxv_ay_event`](#zxv_ay_event),
[Constants](#constants-1), [Music and effects](#music-and-effects).

### `zxv_beeper_event`

```c
void zxv_beeper_event(int level);
```

Sets the beeper to a level at exactly the current time plus
`ZXV_BEEP_LATENCY_US` (2000 µs). The changes go through the timed queue
(256 entries, shared with `zxv_ay_event`); if it is full, the function
waits. That way the beeper sounds with microsecond precision, even if the
program does not call at exactly the right time.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `level` | 0, or anything else | 0, low; any other value, high (1). |

For simple sounds you can write the `ZXV_S_BEEPER` register instead; both
ways add up. With the music of `zxv_music.h` playing, use
[`zxv_beeper_event_at`](#zxv_beeper_event_at): this one uses the current
time, and the music has already queued its own times ahead.

**Example**

```c
for (int i = 0; i < 200; i++) {     /* a 1 kHz beep */
    zxv_beeper_event(i & 1);
    uint32_t t = zxv_time_us();
    while (zxv_time_us() - t < 500)
        ;
}
```

**Since:** specification 0.1 · function no. 13.

**See also:** [`zxv_beeper_event_at`](#zxv_beeper_event_at),
[Sound registers](#sound-registers).

### `zxv_ay_event`

```c
void zxv_ay_event(uint32_t t_us, int reg, int val);
```

Writes to a register of an AY (or of a sample channel) at an exact time:
`t_us` plus 2 ms (`ZXV_BEEP_LATENCY_US`). It goes through the timed queue,
the same as the beeper's. It is meant for digitised sound and for music:
see the [`digital`](../../../sdk/examples/digital/digital.c) example.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `t_us` | in the clock of `zxv_time_us` | The time. Times must go in increasing order: a change waits for the ones before it. If it is already past, it sounds at once. |
| `reg` | as in [`zxv_ay_write`](#zxv_ay_write) | The register. |
| `val` | 0–255 | The value. |

If the queue is full, the function waits. `zxv_smp_write` has its timed
version: `zxv_ay_event(t, ZXV_SMP_REG(ch, reg), val)`.

**Example**

```c
uint32_t t = zxv_time_us() + 5000;  /* in 5 ms */
for (int v = 15; v >= 0; v--) {     /* channel A fades out in 160 ms */
    zxv_ay_event(t, 8, v);
    t += 10000;
}
```

**Since:** specification 0.1 · function no. 51.

**See also:** [`zxv_ay_write`](#zxv_ay_write),
[`zxv_beeper_event_at`](#zxv_beeper_event_at),
[`zxv_music_update_until`](#zxv_music_update_until).

### `zxv_beeper_event_at`

```c
void zxv_beeper_event_at(uint32_t t_us, int level);
```

Like [`zxv_beeper_event`](#zxv_beeper_event), but at time `t_us` (plus
2 ms), with the same rules as `zxv_ay_event`: times in increasing order,
and if the queue is full, it waits.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `t_us` | in the clock of `zxv_time_us` | The time. |
| `level` | 0, or anything else | 0, low; any other, high. |

With the music of `zxv_music.h` playing, which goes `ZXV_MUSIC_AHEAD_US`
(35 ms) ahead, the beeper has to go just as far ahead so as not to put the
queue out of order.

**Example**

```c
zxv_beeper_event_at(zxv_time_us() + ZXV_MUSIC_AHEAD_US, 1);
```

**Since:** specification 0.1, ROM 0.6.0 or later · function no. 55.

**See also:** [`zxv_beeper_event`](#zxv_beeper_event),
[`zxv_ay_event`](#zxv_ay_event).

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_AY2` | 16 | Add to a register number to reach the second AY |
| `ZXV_AY_FIFO_LEN` | 256 | Entries in the AY queue (`zxv_ay_write`) |
| `ZXV_BEEP_QUEUE_LEN` | 256 | Entries in the timed queue (`zxv_ay_event`, the beeper) |
| `ZXV_BEEP_LATENCY_US` | 2000 | Fixed delay of the timed queue, in µs |
| `ZXV_AY_STEREO_MONO`, `_ABC`, `_ACB` | 0, 1, 2 | Values of `ZXV_S_AY_STEREO` (ABC at power-on) |
| `ZXV_EV_AY`, `ZXV_EV_AY_WRITE(reg, val)` | 0x80000000, | The data word of an AY write in the timed queue (with bit 31 set); only needed if you fill the queue yourself |

### Sample channels

Twelve channels that play samples from the **sound memory**, which is in
the Pico ULA, like an Amiga's Paula: music from MOD files, recorded
effects and voices, or a byte you write that sounds at once (the DAC).
Each channel has its note (its step), its volume and its pan, and mixes
with the two AYs. In the emulator they sound the same, with the same code.

**The sound memory** has two parts, and the program fills it with
[`zxv_smp_upload`](#zxv_smp_upload):

| From | To | What |
|---|---|---|
| 0 | `ZXV_SMP_MEM` (0x18000, 96 KB) | The samples. A sample has to be entirely here |
| `ZXV_SMP_TABLE` (0x18000) | `ZXV_SMP_SOUND_MEM` (0x18C00) | The **sample table**: 256 entries (`ZXV_SMP_SLOTS`) of 12 bytes (`ZXV_SMP_SLOT_SIZE`), filled with [`zxv_smp_define`](#zxv_smp_define) |

An entry of the table (all with the low byte first):

| Bytes | Field |
|---|---|
| 0–2 | Where the sample starts in the sound memory (24 bits) |
| 3 | The format: `ZXV_SMP_PCM8` (0) or `ZXV_SMP_ADPCM` (1) |
| 4–7 | How many samples it has |
| 8–11 | The sample the loop starts again from; if it is the length or more, there is no loop |

An entry that is not valid (another format, length 0, a sample that does
not fit in the 96 KB) gives no error: the sample does not sound.

**The formats:**

- **`ZXV_SMP_PCM8`**: one signed byte per sample (−128 to 127). It takes
  any step.
- **`ZXV_SMP_ADPCM`**: IMA ADPCM, in independent blocks of 64 samples
  (`ZXV_SMP_ADPCM_BLOCK`) in 36 bytes (`ZXV_SMP_ADPCM_BYTES`): the first
  sample in 16 signed bits (bytes 0–1), the step index (byte 2, 0–88), a
  zero byte and the other 63 in 4 bits, from byte 4, the even-numbered one
  in the low 4 bits. It sounds like 16 bits and takes a little over half
  of 8 bits. Each sample is worked out from the one before, so a high note
  also decodes the ones it skips: an ADPCM channel goes **no higher than
  step 2.0** (`ZXV_SMP_ADPCM_MAX_STEP`, an octave above how it was
  recorded); a higher step counts as 2.0. A sample of `n` samples takes
  `(n + 63) / 64 × 36` bytes. `smp_adpcm_encode`, from
  `sdk/sound/smpsynth.c`, encodes; the
  [`samples`](../../../sdk/examples/samples/samples.c) example has its own
  encoder.

**The sound of a channel:**

- **Interpolation**, on by default, smooths the low notes; the last sample
  of a loop goes towards the first sample of the loop, so a short waveform
  in a loop sounds round. `ZXV_SMP_NOINTERP` turns it off (the raw sound of
  an Amiga).
- **Direct mode** (`ZXV_SMP_DIRECT`) plays the value of `ZXV_SMP_OFFSET`
  as it is written, with no sample: a signed 8-bit DAC, like the SpecDrum.
  With `zxv_ay_event`, each value sounds at its time.
- **The volume of all of them** is the `ZXV_S_SMP_VOLUME` register (0–255,
  255 at power-on; the ROM does not reset it between programs): one
  channel at full volume is as loud as one AY channel, and twelve at full
  volume saturate.

**The registers of a channel.** They are written through the AY queues
with `zxv_smp_write(ch, reg, val)` (or `ZXV_SMP_REG(ch, reg)` with
`zxv_ay_write` and `zxv_ay_event`), and read at `ZXV_S_SMP_REGS` +
8 × channel + register ([Sound registers](#sound-registers)).

| Register | Number | What it is |
|---|---|---|
| `ZXV_SMP_SLOT` | 0 | The sample: an entry of the table (0–255). Counts when it is started |
| `ZXV_SMP_VOL` | 1 | Volume, 0–64 (more counts as 64). Changes at once |
| `ZXV_SMP_PAN` | 2 | Pan: 0 left, 128 centre, 255 right. Changes at once |
| `ZXV_SMP_CTRL` | 3 | Writing it: `ZXV_SMP_KEY` (0x01) starts the sample from `ZXV_SMP_OFFSET` and `ZXV_SMP_STOP` (0x02) stops it. `ZXV_SMP_NOINTERP` (0x04) and `ZXV_SMP_DIRECT` (0x08) stay until the next write of `CTRL`: each write sets or clears them |
| `ZXV_SMP_STEP` | 4–6 | Three bytes, from low to high (8.16): how far it moves for each output sample, which runs at 48 kHz. 0x10000 (1.0) is the sample at 48 kHz. Changes at once, also with the sample playing |
| `ZXV_SMP_OFFSET` | 7 | Where it starts when started, in steps of 256 samples (a MOD's `9xx`). In direct mode, the value that sounds (signed) |

#### `zxv_smp_upload`

```c
int zxv_smp_upload(uint32_t dst, const void *src, uint32_t len);
```

Copies bytes from the RAM to the sound memory, and waits until they are
there. The copy goes through the link with the ULA, a piece every frame,
so a big one takes several frames. It also serves to write the sample
table directly (from `ZXV_SMP_TABLE`).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `dst` | 0 to `ZXV_SMP_SOUND_MEM` | Where, in the sound memory. |
| `src` | in the RAM (0x2000_0000–0x2006_FFFF) | Where from. Your program's data (`const` too) is in the RAM; the ROM does not work (the emulator accepts it, the board does not). |
| `len` | a multiple of 4; 0 is valid | How many bytes. `dst + len` cannot go past `ZXV_SMP_SOUND_MEM`. |

**Returns:** `ZXV_OK`, or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_RANGE` | `len` is not a multiple of 4; it does not fit (`dst + len` goes past `ZXV_SMP_SOUND_MEM`); `src` (all of it) is not in the RAM; or the ULA does not answer within a second (a ULA firmware older than the sample channels). Nothing is copied, or not all of it. |

It uses `zxv_wait_vblank`: do not call it from a frame hook.

**Example**

```c
static const int8_t wave[32] = { 0, 24, 48, 70, 90, 106, 117, 125,
                                 127, 125, 117, 106, 90, 70, 48, 24,
                                 0, -24, -48, -70, -90, -106, -117, -125,
                                 -127, -125, -117, -106, -90, -70, -48, -24 };

if (zxv_smp_upload(0, wave, sizeof wave) < 0)
    zxv_report_error(0, "No sample channels");
```

**Since:** specification 0.1, ROM 0.10.0 or later (it is in `zxv_api.h`;
it uses the registers `ZXV_S_SMP_SRC`, `_DST`, `_LEN` and `_UPLOAD`).

**See also:** [`zxv_smp_define`](#zxv_smp_define),
[`zxv_music_samples`](#zxv_music_samples).

#### `zxv_smp_define`

```c
int zxv_smp_define(int slot, uint32_t addr, int fmt, uint32_t len, uint32_t loop);
```

Fills an entry of the sample table: where a sample that is already in the
sound memory is, its format and its loop. It is a 12-byte
`zxv_smp_upload`. A channel already playing it carries on with the old
one: the entry is read when the sample is started.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `slot` | 0–255 | The entry of the table. |
| `addr` | 0 to 0x17FFF | Where the sample starts in the sound memory. |
| `fmt` | `ZXV_SMP_PCM8` or `ZXV_SMP_ADPCM` | The format. |
| `len` | 1 or more | How many **samples** it has (not bytes). It has to fit entirely in the 96 KB. |
| `loop` | 0 to `len` − 1, or `len` or more | The sample it starts again from when it reaches the end; `len` or more: no loop, it stops at the end. |

**Returns:** `ZXV_OK`, or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_RANGE` | `slot` is not between 0 and 255; or the ULA does not answer (as in [`zxv_smp_upload`](#zxv_smp_upload)). |

The values of the entry are not checked here: one that is not valid does
not sound.

**Example**

```c
zxv_smp_define(0, 0, ZXV_SMP_PCM8, 32, 0);          /* the wave, all of it in a loop */
zxv_smp_define(1, 1024, ZXV_SMP_ADPCM, 8000, 8000); /* a hit, no loop */
```

**Since:** specification 0.1, ROM 0.10.0 or later (it is in `zxv_api.h`).

**See also:** [`zxv_smp_upload`](#zxv_smp_upload),
[`zxv_smp_play`](#zxv_smp_play).

#### `zxv_smp_play`

```c
void zxv_smp_play(int ch, int slot, uint32_t step, int vol, int pan);
```

Plays a sample on a channel, now, from the start. It is
[`zxv_smp_prepare`](#zxv_smp_prepare-zxv_smp_start) and a write of
`ZXV_SMP_KEY` to `ZXV_SMP_CTRL`: through the AY queue, in order with
everything else. If the channel was playing something else, it cuts it.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ch` | 0–11 | The channel. Another value writes where it should not: do not use it. |
| `slot` | 0–255 | The entry of the table. |
| `step` | 0 to 0xFFFFFF (8.16) | The step: [`zxv_smp_step(hz)`](#zxv_smp_step) for a sample recorded at `hz` to sound at its own pitch; twice that, an octave higher. In ADPCM, up to 0x20000. |
| `vol` | 0–64 | The volume (more counts as 64). |
| `pan` | 0–255 | The pan: 0 left, 128 centre, 255 right. |

Since it writes `ZXV_SMP_CTRL` with only `ZXV_SMP_KEY`, it clears
`ZXV_SMP_NOINTERP` and `ZXV_SMP_DIRECT` on the channel. To play without
interpolation, write `zxv_smp_write(ch, ZXV_SMP_CTRL, ZXV_SMP_NOINTERP)`
afterwards.

**Example**

```c
zxv_smp_play(0, 0, zxv_smp_step(440 * 32), 64, 128);   /* the 32-sample wave: 440 Hz */
```

**Since:** specification 0.1, ROM 0.10.0 or later (it is in `zxv_api.h`).

**See also:** [`zxv_smp_stop`](#zxv_smp_stop),
[`zxv_smp_prepare`, `zxv_smp_start`](#zxv_smp_prepare-zxv_smp_start),
[`zxv_music_smp_fx`](#zxv_music_smp_fx) (with the music playing).

#### `zxv_smp_prepare`, `zxv_smp_start`

```c
void zxv_smp_prepare(int ch, int slot, uint32_t step, int vol, int pan);
void zxv_smp_start(uint32_t mask);
```

`zxv_smp_prepare` sets up a channel just like `zxv_smp_play` (sample,
step, volume, pan and from the start), but without starting it.
`zxv_smp_start` starts the channels it is given **together**, on the same
output sample, whether prepared or not (from `ZXV_SMP_OFFSET`). Use them
for a chord or a row of music: with `zxv_smp_play` on each channel, if
there are many, some may start slightly before others, and with the same
sample on two channels you can hear it.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ch`, `slot`, `step`, `vol`, `pan` | | As in [`zxv_smp_play`](#zxv_smp_play). |
| `mask` | 0 to 0xFFF | The channels: bit n, channel n. |

`zxv_smp_start` writes `ZXV_SMP_KEYON_HI` (channels 8–11, if any) and
`ZXV_SMP_KEYON` one after the other, as they have to go. Unlike
`zxv_smp_play`, it does not touch `ZXV_SMP_CTRL`: `ZXV_SMP_NOINTERP` and
`ZXV_SMP_DIRECT` stay as they are.

**Example**

```c
/* A chord: A, C# and E, starting together. */
zxv_smp_prepare(1, 0, zxv_smp_step(440 * 32), 40, 64);
zxv_smp_prepare(2, 0, zxv_smp_step(554 * 32), 40, 128);
zxv_smp_prepare(3, 0, zxv_smp_step(659 * 32), 40, 192);
zxv_smp_start(1 << 1 | 1 << 2 | 1 << 3);
```

**Since:** specification 0.1, ROM 0.10.0 or later (they are in
`zxv_api.h`).

**See also:** [`zxv_smp_play`](#zxv_smp_play).

#### `zxv_smp_stop`

```c
void zxv_smp_stop(int ch);
```

Stops the channel: writes `ZXV_SMP_STOP` to its `ZXV_SMP_CTRL`, through
the AY queue. It also clears its `ZXV_SMP_NOINTERP` and `ZXV_SMP_DIRECT`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ch` | 0–11 | The channel. |

When the program goes back to BASIC (or to the monitor), the ROM stops
every sample channel and silences the AYs, a looped sample too.

**Example**

```c
for (int ch = 0; ch < ZXV_SMP_CHANNELS; ch++)
    zxv_smp_stop(ch);
```

**Since:** specification 0.1, ROM 0.10.0 or later (it is in `zxv_api.h`).

**See also:** [`zxv_smp_play`](#zxv_smp_play).

#### `zxv_smp_write`

```c
void zxv_smp_write(int ch, int reg, int val);
```

Writes a [register of a channel](#sample-channels), through the AY queue,
in order with the AY writes. If the queue is full, it waits. To do it at
an exact time: `zxv_ay_event(t, ZXV_SMP_REG(ch, reg), val)`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ch` | 0–11 | The channel. |
| `reg` | 0–7 | The register: `ZXV_SMP_SLOT`, `_VOL`, `_PAN`, `_CTRL`, `ZXV_SMP_STEP` + 0–2, `ZXV_SMP_OFFSET`. |
| `val` | 0–255 | The value. |

**Example**

```c
for (int p = 0; p < 256; p += 4) {          /* the sample moves from left to right */
    zxv_smp_write(0, ZXV_SMP_PAN, p);
    zxv_wait_vblank();
}
```

**Since:** specification 0.1, ROM 0.10.0 or later (it is in `zxv_api.h`).

**See also:** [`zxv_ay_event`](#zxv_ay_event).

#### `zxv_smp_step`

```c
uint32_t zxv_smp_step(uint32_t sample_hz);
```

The step for a sample recorded at `sample_hz` to sound at its own pitch:
the sum, in integers, is `sample_hz × 65536 / 48000`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `sample_hz` | up to 16,777,215 | The samples per second it was recorded at (or, for a waveform of `n` samples, `n ×` the frequency of the note). |

**Returns:** the step (8.16). 48000 gives 0x10000; 24000, 0x8000.

**Errors:** none.

**Example**

```c
uint32_t step = zxv_smp_step(22050);        /* a sample recorded at 22,050 Hz */
zxv_smp_play(2, 1, step * 2, 64, 128);      /* an octave higher */
```

**Since:** specification 0.1, ROM 0.10.0 or later (it is in `zxv_api.h`).

**See also:** [`zxv_smp_play`](#zxv_smp_play).

#### Sample channel constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_SMP_CHANNELS` | 12 | Sample channels |
| `ZXV_SMP_MEM` | 0x18000 (96 KB) | Bytes for the samples in the sound memory |
| `ZXV_SMP_TABLE` | 0x18000 | Where the sample table starts, after the samples |
| `ZXV_SMP_SLOTS` | 256 | Entries of the table |
| `ZXV_SMP_SLOT_SIZE` | 12 | Bytes per entry |
| `ZXV_SMP_SOUND_MEM` | 0x18C00 (101,376) | The whole sound memory, the samples and the table: the limit of `zxv_smp_upload` |
| `ZXV_SMP_PCM8`, `ZXV_SMP_ADPCM` | 0, 1 | The formats |
| `ZXV_SMP_ADPCM_BLOCK` | 64 | Samples per ADPCM block |
| `ZXV_SMP_ADPCM_BYTES` | 36 | Bytes per ADPCM block |
| `ZXV_SMP_ADPCM_MAX_STEP` | 0x20000 | The highest step in ADPCM (2.0) |
| `ZXV_SMP_REG(ch, reg)` | 64 + 8 × `ch` + `reg` | The number of a channel's register for the AY queues |
| `ZXV_SMP_SLOT` ... `ZXV_SMP_OFFSET` | 0–7 | The registers of a channel (table above) |
| `ZXV_SMP_KEY`, `_STOP`, `_NOINTERP`, `_DIRECT` | 0x01, 0x02, 0x04, 0x08 | The bits of `ZXV_SMP_CTRL` |
| `ZXV_SMP_KEYON` | 37 | Number for the queues: starts together channels 0–7 of its value (bits 0–7) and those `ZXV_SMP_KEYON_HI` stored |
| `ZXV_SMP_KEYON_HI` | 36 | Number for the queues: stores channels 8–11 (bits 0–3) for the next `ZXV_SMP_KEYON`, which has to come right after it |

## Music and effects

Declared in `zxv_music.h`. Build it with the files of `sdk/music`: it is
not in the ROM (BASIC has its own copy), and it works with any ROM. The
explanations and a complete example are in the
[guide](README.md#tracker-music-and-sound-effects).

The essentials:

- **It plays at its own tempo:** 50 steps per second, as on the Spectrum
  (a `.zxm`, at its tempo; a VGM, at its waits), even though the machine
  runs at 60 frames per second. Each step goes to the AYs at its time with
  [`zxv_ay_event`](#zxv_ay_event), and only the registers that change.
- **It has to be kept going:** [`zxv_music_update`](#zxv_music_update)
  prepares the next 35 ms (`ZXV_MUSIC_AHEAD_US`). Call it every frame, or
  from the frame hook (and then only from there). Without it nothing
  sounds, not the effects, nor the silence when stopping.
- **The library copies nothing:** the module, the effect bank and the work
  memory of a `.vgz` must stay in memory while they play.
- **The timed queue goes in order:** if the program uses it too, its times
  have to be 35 ms ahead, like the music's
  ([`zxv_beeper_event_at`](#zxv_beeper_event_at)), or be mixed in with
  [`zxv_music_update_until`](#zxv_music_update_until).
- **The channels:** a one-AY module plays on the first chip (or on the
  second, with `ZXV_MUSIC_AY2`); TurboSound, a `.zxm` or a two-AY VGM, on
  both. AYFX effects go on channels A, B and C of either chip, over the
  music. A `.zxm` plays its sample tracks on the sample channels from 0
  on.

### Modules

```c
int zxv_music_info(const void *data, uint32_t size, zxv_music_info_t *info);
int zxv_music_play(const void *data, uint32_t size, int flags);
void zxv_music_stop(void);
void zxv_music_samples(uint32_t base, int slot);
void zxv_music_work(void *buf);
void zxv_music_smp_fx(int ch, int slot, uint32_t step, int vol, int pan, uint32_t dur_us);
void zxv_music_smp_fx_stop(int ch);
int zxv_music_smp_tracks(void);
void zxv_music_pause(int on);
int zxv_music_playing(void);
uint32_t zxv_music_steps(void);
void zxv_music_moved(const void *song, const void *bank);
void zxv_music_update(void);
void zxv_music_update_until(uint32_t t_us);
```

None of the music functions gives errors: one that cannot do what it is
asked does nothing, or says so in what it returns.

#### `zxv_music_play`

```c
int zxv_music_play(const void *data, uint32_t size, int flags);
```

Starts playing a module, from the beginning, instead of whatever was
playing. It recognises it by its contents, not by the file name.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `data` | | The module, whole in memory, and it must stay there while it plays. |
| `size` | bytes | Its size. |
| `flags` | 0, `ZXV_MUSIC_LOOP`, `ZXV_MUSIC_AY2`, or both | `ZXV_MUSIC_LOOP`: at the end, it goes on from its loop position (without it, it stops). `ZXV_MUSIC_AY2`: a one-AY module plays on the second chip (on a two-AY one, it does not count). |

**Returns:** the format (`ZXV_MUSIC_PT3`...), or `ZXV_MUSIC_NONE` if it
does not recognise it; then it plays nothing (and whatever was playing has
already stopped).

- **A `.zxm`** uploads its samples to the sound memory before starting (it
  waits until they are there; do not call it from a frame hook), wherever
  [`zxv_music_samples`](#zxv_music_samples) says.
- **A `.vgz`** is decompressed while it plays, with the memory you give it
  with [`zxv_music_work`](#zxv_music_work): without it, it gives
  `ZXV_MUSIC_NONE`. Of a VGM only the AY chips play; writes to other chips
  are skipped.
- The samples of the previous song go silent at once.

**Example**

```c
extern const uint8_t song[];
extern const uint32_t song_len;

if (zxv_music_play(song, song_len, ZXV_MUSIC_LOOP) == ZXV_MUSIC_NONE)
    zxv_report_error(0, "The song is not valid");
```

**See also:** [`zxv_music_update`](#zxv_music_update),
[`zxv_music_stop`](#zxv_music_stop), [`zxv_music_info`](#zxv_music_info).

#### `zxv_music_info`

```c
int zxv_music_info(const void *data, uint32_t size, zxv_music_info_t *info);

typedef struct {
    int format;                 /* ZXV_MUSIC_... */
    int chips;                  /* the AY chips it uses (below) */
    char title[33];
    char author[33];
} zxv_music_info_t;
```

Says what a module is, without playing it: its format, the AY chips it
uses, its title and its author.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `data`, `size` | | The module, as in `zxv_music_play`. |
| `info` | | Where it leaves what it finds. |

**Returns:** the format, as `info->format`; `ZXV_MUSIC_NONE` if it is not
a known one.

- `chips`: 1, one AY; 3, both; 0, none (a `.zxm` with samples only, or a
  module that is not recognised).
- `title` and `author`: up to 32 ASCII characters (anything else, as
  spaces), without the trailing spaces; empty if the format does not have
  them (PSG, the author of PT2 and STC, an ASC without them) or, in a VGM,
  if it has no GD3 tag (a `.vgz`, not decompressed, does not show it). A
  `.vgz` is recognised even without `zxv_music_work`.

**Example**

```c
zxv_music_info_t mi;
if (zxv_music_info(song, song_len, &mi) != ZXV_MUSIC_NONE) {
    zxv_print_str(mi.title);
    zxv_print_str(mi.chips == 3 ? " (two AYs)\n" : "\n");
}
```

**See also:** [`zxv_music_play`](#zxv_music_play).

#### `zxv_music_stop`

```c
void zxv_music_stop(void);
```

Stops the song. The AY chips and the sample channels it used go silent at
the next [`zxv_music_update`](#zxv_music_update): call it afterwards. AYFX
effects keep playing.

**Example**

```c
zxv_music_stop();
zxv_music_update();                 /* the silence */
```

**See also:** [`zxv_music_pause`](#zxv_music_pause).

#### `zxv_music_samples`

```c
void zxv_music_samples(uint32_t base, int slot);
```

Where `zxv_music_play` puts the samples of a `.zxm`: in the sound memory
from `base`, and in the sample table from entry `slot`. By default, 0 and
0; the program's samples, after them. It counts for the next
`zxv_music_play`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `base` | 0 to 0x17FFF | The first byte of the sound memory. What does not fit in the 96 KB is not uploaded; with `base` of 96 KB or more, nothing. |
| `slot` | 0–255 | The first entry of the table. Those that would go past 255 are not set. |

**Example**

```c
zxv_music_samples(64 * 1024, 200);  /* the song, after the effects */
zxv_music_play(song, song_len, ZXV_MUSIC_LOOP);
```

**See also:** [Sample channels](#sample-channels).

#### `zxv_music_work`

```c
void zxv_music_work(void *buf);
```

Gives the music 32 KB of memory to decompress a `.vgz` while it plays.
Without them, a `.vgz` does not play. It counts for the next
`zxv_music_play`, and the memory must remain the music's while it plays.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `buf` | 32,768 bytes | The memory. |

**Example**

```c
ZXV_VRAM1_DATA static uint8_t window[32 * 1024];    /* in the hidden VRAM */

zxv_music_work(window);
zxv_music_play(theme_vgz, theme_vgz_len, ZXV_MUSIC_LOOP);
```

**See also:** [`zxv_music_play`](#zxv_music_play).

#### `zxv_music_smp_fx`

```c
void zxv_music_smp_fx(int ch, int slot, uint32_t step, int vol, int pan, uint32_t dur_us);
```

Plays a sample as an effect, together with the song, on a sample channel.
With the music running it goes in step with it, with the same lead (so it
does not put the timed queue out of order); without music, at once. If the
channel belongs to the song, it covers it while it plays, and the song
carries on there at its next note. It is what `SAMPLE` uses in BASIC.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `ch` | 0–11 | The channel. Another value does nothing. Effects are best on those the song does not use ([`zxv_music_smp_tracks`](#zxv_music_smp_tracks)). |
| `slot`, `step`, `vol`, `pan` | | As in [`zxv_smp_play`](#zxv_smp_play). |
| `dur_us` | µs, or 0 | How long the channel belongs to the effect. 0: until `zxv_music_smp_fx_stop`, for a looped sample. |

It plays the sample interpolated (it sets `ZXV_SMP_CTRL` to 0).

**Example**

```c
int ch = zxv_music_smp_tracks();            /* the first free one */
zxv_music_smp_fx(ch, 10, zxv_smp_step(22050), 64, 128, 400000);
```

**See also:** [`zxv_music_smp_fx_stop`](#zxv_music_smp_fx_stop),
[`zxv_smp_play`](#zxv_smp_play).

#### `zxv_music_smp_fx_stop`

```c
void zxv_music_smp_fx_stop(int ch);
```

Stops the effect on channel `ch` (0–11; another value does nothing), with
the music's lead if it is playing. If the channel belongs to the song, the
song carries on there at its next note.

**Example**

```c
zxv_music_smp_fx(11, 12, zxv_smp_step(16000), 48, 128, 0);  /* an engine, looped */
/* ... */
zxv_music_smp_fx_stop(11);
```

**See also:** [`zxv_music_smp_fx`](#zxv_music_smp_fx).

#### `zxv_music_smp_tracks`

```c
int zxv_music_smp_tracks(void);
```

**Returns:** how many sample channels the playing song uses: the first
ones, from 0. 0 if no `.zxm` is playing, if it is paused or if it has no
sample tracks. Put the effects on the ones above.

**Example:** see [`zxv_music_smp_fx`](#zxv_music_smp_fx).

#### `zxv_music_pause`

```c
void zxv_music_pause(int on);
```

`zxv_music_pause(1)` pauses the song where it is, and `zxv_music_pause(0)`
resumes it from there. As when stopping it, the AY chips go silent at the
next `zxv_music_update`. The effects carry on.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `on` | 0, or anything else | Any value other than 0: paused. |

**Example**

```c
if (key == 'p')
    zxv_music_pause(paused ^= 1);
```

**See also:** [`zxv_music_stop`](#zxv_music_stop).

#### `zxv_music_playing`

```c
int zxv_music_playing(void);
```

**Returns:** 1 while there is a song (also when paused); 0 if there is
none or if it has reached the end without `ZXV_MUSIC_LOOP`.

**Example**

```c
zxv_music_play(fanfare, fanfare_len, 0);
while (zxv_music_playing()) {       /* until it ends */
    zxv_music_update();
    zxv_wait_vblank();
}
```

#### `zxv_music_steps`

```c
uint32_t zxv_music_steps(void);
```

**Returns:** the steps of the song that have played since
`zxv_music_play`: of 1/50 s (in a `.zxm`, the ticks of its tempo; in a
VGM, those of its waits). Those while paused do not count. It is for
synchronising something with the music.

**Example**

```c
if (zxv_music_steps() >= 50 * 30)   /* 30 s into a PT3 */
    start_boss();
```

#### `zxv_music_moved`

```c
void zxv_music_moved(const void *song, const void *bank);
```

For a program that moves its data while it plays (like BASIC, which
compacts its memory): the module or the effect bank are now, whole, at
another address, and the music carries on where it was.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `song` | the new address, or `NULL` | Where the module is now; `NULL` if it has not moved. |
| `bank` | the new address, or `NULL` | Where the effect bank is now; `NULL` if it has not moved. |

**Example**

```c
memmove(new_addr, old_addr, song_len);      /* the module is moved... */
zxv_music_moved(new_addr, NULL);            /* ...and the library is told */
```

#### `zxv_music_update`

```c
void zxv_music_update(void);
```

Prepares the steps of the music and the effects for the next
`ZXV_MUSIC_AHEAD_US` (35 ms) and sends them at their time. Call it at
least every 35 ms: every frame, or from the frame hook (and then only from
there), which keeps it playing even if the program is busy. If a long time
goes by without calling it, the music does not rush to catch up: it
carries on from that moment.

**Example**

```c
static void hook(void)
{
    zxv_music_update();
}

zxv_set_frame_hook(hook);           /* the music plays by itself */
```

**See also:** [`zxv_music_update_until`](#zxv_music_update_until),
[`zxv_set_frame_hook`](#zxv_set_frame_hook).

#### `zxv_music_update_until`

```c
void zxv_music_update_until(uint32_t t_us);
```

Like `zxv_music_update`, but only the steps before `t_us` (never more than
35 ms ahead). To mix the music with your own `zxv_ay_event` calls, which
go in time order: before each one, `zxv_music_update_until(its time)`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `t_us` | in the clock of `zxv_time_us` | Up to when. |

**Example**

```c
uint32_t t = zxv_time_us() + ZXV_MUSIC_AHEAD_US;
zxv_music_update_until(t);          /* the music, up to there... */
zxv_ay_event(t, 10, 15);            /* ...and yours, in order */
```

**See also:** [`zxv_ay_event`](#zxv_ay_event).

### AYFX effects

```c
int zxv_sfx_bank(const void *data, uint32_t size);
int zxv_sfx_play(int n, int channel);
int zxv_sfx_play_data(const void *data, uint32_t size, int channel);
void zxv_sfx_stop(int channel);
void zxv_sfx_channels(int mask);
int zxv_sfx_playing(int channel);
```

The effects of AYFX Editor (an `.afb` bank or a single `.afx` effect), 50
steps per second. The channels are 0 to 2 (A, B and C of the first AY)
and 3 to 5 (those of the second). An effect on one of the music's channels
covers it while it plays. They play with
[`zxv_music_update`](#zxv_music_update), whether there is music or not.

#### `zxv_sfx_bank`

```c
int zxv_sfx_bank(const void *data, uint32_t size);
```

Sets the effect bank (`.afb`) for `zxv_sfx_play`. It must stay in memory
while it is used.

**Returns:** how many effects it has (0 if it is not a bank).

**Example**

```c
int n = zxv_sfx_bank(effects, effects_len);
```

**See also:** [`zxv_sfx_play`](#zxv_sfx_play).

#### `zxv_sfx_play`

```c
int zxv_sfx_play(int n, int channel);
```

Plays an effect of the bank on a channel, from the start. If that channel
already had an effect, it replaces it with this one.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0 to the bank's count − 1 | The effect. |
| `channel` | 0–5, or `ZXV_SFX_AUTO` | The channel. `ZXV_SFX_AUTO` (or any negative value): a free one of those of [`zxv_sfx_channels`](#zxv_sfx_channels) or, if there is none, the one that has been playing longest. |

**Returns:** the channel, or −1 if the effect does not exist, if `channel`
is above 5 or if no channel is chosen for `ZXV_SFX_AUTO`.

**Example**

```c
if (shot)
    zxv_sfx_play(0, ZXV_SFX_AUTO);
```

**See also:** [`zxv_sfx_play_data`](#zxv_sfx_play_data),
[`zxv_sfx_stop`](#zxv_sfx_stop).

#### `zxv_sfx_play_data`

```c
int zxv_sfx_play_data(const void *data, uint32_t size, int channel);
```

Like `zxv_sfx_play`, with a single effect (`.afx`) instead of one from the
bank. It must stay in memory while it plays.

**Returns:** the channel, or −1 (`size` 0, `channel` above 5 or no
channels).

**Example**

```c
zxv_sfx_play_data(explosion, explosion_len, 2);
```

#### `zxv_sfx_stop`

```c
void zxv_sfx_stop(int channel);
```

Stops the effect on a channel (0–5), or all of them with `ZXV_SFX_AUTO`
(or any negative value).

**Example**

```c
zxv_sfx_stop(ZXV_SFX_AUTO);         /* silence all effects */
```

#### `zxv_sfx_channels`

```c
void zxv_sfx_channels(int mask);
```

Chooses the channels for `ZXV_SFX_AUTO`: bit *n*, channel *n* (only bits
0–5 count). By default, those of the AY the music does not use (all six if
there is no music or if it uses both chips); once you call it, it no
longer goes back to that: `zxv_sfx_channels(0x3F)` gives all six.

**Example**

```c
zxv_sfx_channels(1 << 2 | 1 << 5); /* the effects, on the C channels of both AYs */
```

#### `zxv_sfx_playing`

```c
int zxv_sfx_playing(int channel);
```

**Returns:** 1 if an effect is playing on the channel (0–5), or on any,
with `ZXV_SFX_AUTO` (or any negative value); 0 if not.

**Example**

```c
while (zxv_sfx_playing(ZXV_SFX_AUTO)) {     /* let the explosion finish */
    zxv_music_update();
    zxv_wait_vblank();
}
```

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_MUSIC_NONE` | 0 | Not a known module |
| `ZXV_MUSIC_PT3`, `_TS`, `_PSG` | 1, 2, 3 | Pro Tracker 3 (and Vortex Tracker II), TurboSound (two PT3s, one per AY), a dump of the AY registers |
| `ZXV_MUSIC_PT2`, `_STC`, `_ASC` | 4, 5, 6 | Pro Tracker 2, Sound Tracker 1.x, ASC Sound Master |
| `ZXV_MUSIC_ZXM`, `_VGM` | 7, 8 | The own format (`.zxm`: the two AYs and the sample channels); a VGM (`.vgm` or `.vgz`) with AY chips |
| `ZXV_MUSIC_LOOP`, `ZXV_MUSIC_AY2` | 1, 2 | Options of `zxv_music_play` |
| `ZXV_MUSIC_AHEAD_US` | 35000 | How far ahead the music goes, in µs |
| `ZXV_SFX_AUTO` | −1 | A free channel (or all of them) |

## Files and folders

There are three drives: `SD:` (the SD card), `ROM:` (the built-in
read-only drive, in the flash) and `EXT:` (the external drive: a folder
served by another device, see [The EXT:
drive](../hardware/firmware.md#the-ext-drive)). The functions are the same
for all three.

- Up to **8 files and 4 folders can be open at once on each drive**. The
  file of a program with [banks](#banks) is open while it runs, and
  counts.
- File numbers say which drive they belong to: 0–7 on `SD:`, 8–15 on
  `ROM:` and 16–23 on `EXT:`; folder numbers, 0–3, 4–7 and 8–11.
- Each drive has its current folder, and one of them is the current drive
  (`SD:` at power-on).
- **Buffers** must be entirely in the RAM (from 0x2000_0000 to
  0x2006_FFFF: the system area, the VRAM and the user RAM) or, if the
  function only reads them (what is written to a file, the names), in the
  ROM. Otherwise the function returns `ZXV_E_FAULT` and does nothing.
- **On an SD card**, each `zxv_file_write` reaches the card before
  returning (if the card is taken out afterwards, what was written is
  there). A file that is open cannot be deleted, renamed or opened again
  for writing. If the card is taken out, whatever was open on it only gives
  `ZXV_E_NOCARD` until it is closed.
- **On `EXT:`** you read and write as on the card, but only while there is
  a server: otherwise every call returns `ZXV_E_OFFLINE`. If the server
  goes away, the first call waits 1.5 seconds and the next ones fail at
  once until it comes back. The server writes to a copy and replaces the
  file when it is closed, and it may refuse to replace or delete a file
  the machine did not write (`ZXV_E_ACCESS`).

**Common errors.** Besides the ones of each function, any file function
can give:

| Error | When |
|---|---|
| `ZXV_E_NOCARD` | The path goes to `SD:` and there is no card (or it does not respond); or the file or folder was opened before the card was taken out. |
| `ZXV_E_IO` | The card failed to read or write, or does not have a FAT format that can be understood. Whatever was open on it is no longer valid, as when taking it out. |
| `ZXV_E_OFFLINE` | The path goes to `EXT:` and there is no server, or it has stopped answering. |
| `ZXV_E_FAULT` | A buffer or a name is not entirely in the allowed memory (above). Nothing is touched. |

### Paths

- **The drive**: a path may start with `SD:`, `ROM:` or `EXT:`, upper or
  lower case. After the drive, the path always goes from its root:
  `ROM:graphics` is the same as `ROM:/graphics`.
- **Without a drive**, the path is on the current drive: relative to its
  current folder, or from its root if it starts with `/`.
- `.` and `..` work, and `..` at the root stays at the root. Case does not
  matter (`Level1.DAT` is `level1.dat`).
- **Names** have 1 to 63 (`ZXV_NAME_MAX`) printable ASCII characters,
  without `\ : * ? " < > |`, and not ending in a space or a dot. **Paths**,
  up to 255 characters (`ZXV_PATH_MAX`) and 64 folders deep. Otherwise,
  `ZXV_E_NAME`.
- On the card, hidden or system files, and those with a name that does not
  follow these rules (with accents, or longer than 63 characters), do not
  appear in listings.

### `zxv_file_open`

```c
int zxv_file_open(const char *name, uint32_t flags);
```

Opens a file to read it, write it or both. The position starts at the
beginning (at the end with `ZXV_O_APPEND`).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | path of up to 255 characters; names of up to 63 | The file. Without a drive, on the current one; see [Paths](#paths). |
| `flags` | a combination of `ZXV_O_*` (table) | What is going to be done with it. It must include `ZXV_O_READ`, `ZXV_O_WRITE` or both. |

| Flag | Value | Effect |
|---|---|---|
| `ZXV_O_READ` | 0x01 | Read. |
| `ZXV_O_WRITE` | 0x02 | Write. |
| `ZXV_O_CREATE` | 0x04 | Create it if it does not exist. Only with `ZXV_O_WRITE`. |
| `ZXV_O_TRUNC` | 0x08 | Empty it when opening. Only with `ZXV_O_WRITE`. |
| `ZXV_O_APPEND` | 0x10 | Start at the end. |

**Returns:** the file's number (0 or more), for the other file functions;
or an error (negative).

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOENT` | It does not exist and `ZXV_O_CREATE` was not asked for with `ZXV_O_WRITE`; a folder of the path does not exist; or the path goes to `ROM:` and the machine has no ROM disk. |
| `ZXV_E_ACCESS` | `flags` has neither `ZXV_O_READ` nor `ZXV_O_WRITE`, or has bits that are not `ZXV_O_*`; on `ROM:`, anything other than just `ZXV_O_READ`; on `SD:`, opening for writing a file that is already open, or a read-only one; on `EXT:`, the server does not allow replacing it. |
| `ZXV_E_INVAL` | On `EXT:`, `flags` has neither `ZXV_O_READ` nor `ZXV_O_WRITE` (the server says so). |
| `ZXV_E_ISDIR` | `name` is a folder (or the root). |
| `ZXV_E_NAME` | Invalid name or path (see [Paths](#paths)). |
| `ZXV_E_NOTDIR` | A part of the path is not a folder. |
| `ZXV_E_MFILE` | There are already 8 files open on that drive. |
| `ZXV_E_NOSPC` | It could not be created: the card is full. |
| (common) | See [Common errors](#files-and-folders). |

**Example**

```c
int f = zxv_file_open("records.dat", ZXV_O_WRITE | ZXV_O_CREATE | ZXV_O_TRUNC);
if (f < 0)
    zxv_report_error(f, "Cannot save the records");
zxv_file_write(f, &records, sizeof records);
zxv_file_close(f);          /* on EXT:, the file changes when it is closed */
```

**Since:** specification 0.1 · API function no. 14.

**See also:** [`zxv_file_close`](#zxv_file_close),
[`zxv_file_load`](#zxv_file_load),
[`zxv_save.h`](libraries.md#saving-data-zxv_saveh) (saving without losing
the previous data if the power goes).

### `zxv_file_read`, `zxv_file_write`

```c
int zxv_file_read(int f, void *buf, uint32_t n);
int zxv_file_write(int f, const void *buf, uint32_t n);
```

Read or write up to `n` bytes at the current position of the file, and
move it past what was read or written. If you read code to run it,
execute a `fence.i` instruction afterwards (or use
[`zxv_file_load`](#zxv_file_load), which executes it).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | a number from `zxv_file_open` | The file. |
| `buf` | `n` bytes in the RAM (when writing, also in the ROM) | Where to read into, or what is written. When reading, `buf` must have room for `n` bytes even if the file has fewer. |
| `n` | bytes | How many. |

**Returns:** how many bytes were read or written, or an error.
`zxv_file_read` returns fewer than `n` near the end and 0 at the end. If
the position is past the end, `zxv_file_write` fills the gap with zeros
(on the card).

**Errors**

| Error | When |
|---|---|
| `ZXV_E_BADF` | `f` is not an open file. |
| `ZXV_E_ACCESS` | Reading from a file opened without `ZXV_O_READ`; writing to one opened without `ZXV_O_WRITE`, or on `ROM:`. |
| `ZXV_E_NOSPC` | When writing, the card has filled up: part of it may have been written, and the position moves on by what was written. |
| (common) | See [Common errors](#files-and-folders). `ZXV_E_FAULT` if `buf` does not have `n` bytes in the allowed memory. |

On `EXT:`, if the server goes away halfway, they return what they managed
to read or write (or `ZXV_E_OFFLINE` if nothing got through).

**Example**

```c
uint8_t head[16];
int f = zxv_file_open("map.bin", ZXV_O_READ);
if (f >= 0) {
    int n = zxv_file_read(f, head, sizeof head);
    if (n == (int)sizeof head)
        read_rest(f, head);
    zxv_file_close(f);
}
```

**Since:** specification 0.1 · functions no. 15 and 16.

**See also:** [`zxv_file_seek`](#zxv_file_seek),
[`zxv_file_load`](#zxv_file_load).

### `zxv_file_seek`

```c
int zxv_file_seek(int f, int32_t off, int whence);
```

Moves the position of a file. It can go past the end: reading there gives
0 bytes, and writing makes the file longer.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | a number from `zxv_file_open` | The file. |
| `off` | bytes, signed | How far, from `whence`. |
| `whence` | `ZXV_SEEK_SET` (0), `ZXV_SEEK_CUR` (1), `ZXV_SEEK_END` (2) | From the start, from the current position or from the end. |

**Returns:** the new position, from the start; or an error.
`zxv_file_seek(f, 0, ZXV_SEEK_END)` gives the size of the file.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_BADF` | `f` is not an open file. |
| `ZXV_E_ACCESS` | `whence` is not 0, 1 or 2, or the position would be before the start. On `ROM:`, also if it would go past 2 GB. |
| `ZXV_E_INVAL` | On `EXT:`, the same (the server says so). |
| `ZXV_E_IO` | On `SD:`, the position would go past 2 GB (0x7FFFFFFF). |
| (common) | See [Common errors](#files-and-folders). |

**Example**

```c
int size = zxv_file_seek(f, 0, ZXV_SEEK_END);       /* the size... */
zxv_file_seek(f, 0, ZXV_SEEK_SET);                  /* ...and back to the start */
```

**Since:** specification 0.1 · function no. 21.

**See also:** [`zxv_file_read`, `zxv_file_write`](#zxv_file_read-zxv_file_write).

### `zxv_file_close`

```c
int zxv_file_close(int f);
```

Closes a file and frees its number. Always close what you open: there are
only 8 per drive, and on `EXT:` the file you write does not change until
you close it (if you do not close it, the old one stays).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | a number from `zxv_file_open` | The file. |

**Returns:** 0, or an error. A file on a card that has been taken out, or
on `EXT:` without a server, is closed all the same and gives 0.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_BADF` | `f` is not an open file. |
| `ZXV_E_IO`, `ZXV_E_NOCARD` | The card failed when closing it, or was taken out at that moment (the number is freed all the same). |

**Example:** see [`zxv_file_open`](#zxv_file_open).

**Since:** specification 0.1 · function no. 17.

**See also:** [`zxv_file_open`](#zxv_file_open).

### `zxv_file_load`

```c
int zxv_file_load(const char *name, void *addr, uint32_t max);
```

Reads a whole file into memory. If it fits, it is the same as opening it,
reading it and closing it, in one call. Afterwards it executes `fence.i`,
so what is loaded can be code.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | path | The file. |
| `addr` | in the RAM | Where it is loaded. |
| `max` | bytes | The room there is at `addr`. Pass the size of the buffer. |

**Returns:** the size of the file in bytes, or an error (negative).

**Errors**

| Error | When |
|---|---|
| `ZXV_E_RANGE` | The file is bigger than `max` bytes. Nothing is loaded. |
| `ZXV_E_NOENT`, `ZXV_E_ISDIR`, `ZXV_E_NAME`, `ZXV_E_NOTDIR`, `ZXV_E_MFILE` | As in [`zxv_file_open`](#zxv_file_open). |
| (common) | See [Common errors](#files-and-folders). `ZXV_E_FAULT` if `addr` does not have room in the RAM for the whole file. |

**Example**

```c
static uint8_t level[8192];
int size = zxv_file_load("level1.dat", level, sizeof level);
if (size < 0)
    zxv_report_error(size, "Cannot load level1.dat");
```

**Since:** specification 0.1 · function no. 18. (The `max` argument came
later: a program compiled without it must be compiled again.)

**See also:** [`zxv_bank_load`](#zxv_bank_load) (data inside the `.zxv`
itself).

### `zxv_file_stat`

```c
int zxv_file_stat(const char *name, zxv_dirent_t *e);

typedef struct {
    char name[64];              /* zero-terminated */
    uint32_t size;              /* in bytes (0 for a folder) */
    uint32_t attr;              /* ZXV_ATTR_DIR (bit 0): it is a folder */
} zxv_dirent_t;
```

Says whether a file or folder exists, and its name, its size and whether
it is a folder, without opening it.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | path | The file or folder. |
| `e` | 72 bytes in the RAM | Where it leaves what it finds: `name`, the name as it is written on the drive (the root, `/`). |

**Returns:** 0, or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOENT` | It does not exist (or a folder of the path does not exist). |
| `ZXV_E_NAME`, `ZXV_E_NOTDIR` | As in [`zxv_file_open`](#zxv_file_open). |
| (common) | See [Common errors](#files-and-folders). |

The same 72 bytes described for assembly: `ZXV_DIRENT_SIZE` (72),
`ZXV_DE_NAME` (0), `ZXV_DE_SIZE` (64), `ZXV_DE_ATTR` (68).

**Example**

```c
zxv_dirent_t e;
if (zxv_file_stat("game.sav", &e) == 0 && !(e.attr & ZXV_ATTR_DIR))
    saved_game = e.size > 0;
```

**Since:** specification 0.1 · function no. 22.

**See also:** [`zxv_dir_read`](#zxv_dir_read).

### `zxv_file_remove`, `zxv_file_rename`

```c
int zxv_file_remove(const char *name);
int zxv_file_rename(const char *from, const char *to);
```

`zxv_file_remove` deletes a file or an **empty** folder.
`zxv_file_rename` renames a file or folder, or moves it to another folder
on the same drive (from one drive to another it cannot: it has to be
copied).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | path | What is deleted. |
| `from` | path | What is renamed or moved. |
| `to` | path | The new name, with its folder. It cannot exist. If only the case changes, nothing is done (it gives 0). |

**Returns:** 0, or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOENT` | `name` or `from` does not exist, or a folder of the path of `to` does not exist. |
| `ZXV_E_EXIST` | `to` already exists. |
| `ZXV_E_NOTEMPTY` | The folder being deleted is not empty. |
| `ZXV_E_ACCESS` | It is the root; it is an open file; it is on `ROM:`; `from` and `to` are on different drives; on `EXT:`, the server does not allow it (a file the machine did not write). |
| `ZXV_E_IO` | On `SD:`, moving a folder inside itself. |
| `ZXV_E_NAME`, `ZXV_E_NOTDIR` | As in [`zxv_file_open`](#zxv_file_open). |
| (common) | See [Common errors](#files-and-folders). |

**Example**

```c
zxv_file_remove("game.bak");                    /* if it does not exist, ZXV_E_NOENT */
zxv_file_rename("game.sav", "game.bak");
```

**Since:** specification 0.1 · functions no. 23 and 24.

**See also:** [`zxv_dir_make`](#zxv_dir_make).

### Folders

```c
int zxv_dir_make(const char *name);             /* create a folder */
int zxv_dir_change(const char *name);           /* change the current folder */
int zxv_dir_current(char *buf, uint32_t len);   /* the current folder */
int zxv_dir_open(const char *name);             /* open a folder to list it */
int zxv_dir_read(int d, zxv_dirent_t *e);       /* the next entry */
int zxv_dir_close(int d);                       /* close it */
```

#### `zxv_dir_make`

```c
int zxv_dir_make(const char *name);
```

Creates a folder. The folder it goes in must exist.

**Returns:** 0, or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_EXIST` | There is already a file or folder with that name. |
| `ZXV_E_NOENT` | A folder of the path does not exist. |
| `ZXV_E_ACCESS` | On `ROM:`. |
| `ZXV_E_NOSPC` | The card is full. |
| `ZXV_E_NAME`, `ZXV_E_NOTDIR` | As in [`zxv_file_open`](#zxv_file_open). |
| (common) | See [Common errors](#files-and-folders). |

**Example**

```c
int r = zxv_dir_make("SD:/saves");
if (r < 0 && r != ZXV_E_EXIST)
    zxv_report_error(r, "Cannot create the folder");
```

**Since:** specification 0.1 · function no. 25.

#### `zxv_dir_change`

```c
int zxv_dir_change(const char *name);
```

Changes the current folder. With a drive in front, also the current drive:
`zxv_dir_change("ROM:")` enters the root of the ROM disk,
`zxv_dir_change("EXT:")` that of the external drive and
`zxv_dir_change("SD:")` goes back to the root of the card.

**Returns:** 0, or an error. With an error nothing changes.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOENT` | It does not exist (or a folder of the path does not exist, or there is no ROM disk). |
| `ZXV_E_NOTDIR` | It is a file, or a part of the path is not a folder. |
| `ZXV_E_NAME` | As in [`zxv_file_open`](#zxv_file_open). |
| (common) | See [Common errors](#files-and-folders). |

**Example**

```c
if (zxv_dir_change("ROM:/graphics") == 0)
    zxv_file_load("sprites.gfx", gfx, sizeof gfx);
```

**Since:** specification 0.1 · function no. 26.

#### `zxv_dir_current`

```c
int zxv_dir_current(char *buf, uint32_t len);
```

Copies the current folder into `buf`, zero-terminated: `/...` on the card,
`ROM:/...` on the ROM drive and `EXT:/...` on the external one.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `buf` | `len` bytes in the RAM | Where. |
| `len` | bytes | The room in `buf`, with the zero. With `ZXV_PATH_MAX + 1` (256) any path fits. |

**Returns:** the length (without the zero), or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_RANGE` | It does not fit in `len` bytes (or the path, with the drive in front, would go past 255). |
| `ZXV_E_FAULT` | `buf` does not have `len` bytes in the RAM. |

**Example**

```c
char cwd[ZXV_PATH_MAX + 1];
if (zxv_dir_current(cwd, sizeof cwd) >= 0)
    zxv_print_str(cwd);
```

**Since:** specification 0.1 · function no. 27.

#### `zxv_dir_open`

```c
int zxv_dir_open(const char *name);
```

Opens a folder to list it with `zxv_dir_read`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | path, `""` or `NULL` | The folder; `""` or `NULL`, the current one. |

**Returns:** the number of the open folder (0 or more), or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOENT` | It does not exist (or a folder of the path does not exist, or there is no ROM disk). |
| `ZXV_E_NOTDIR` | It is a file, or a part of the path is not a folder. |
| `ZXV_E_MFILE` | There are already 4 folders open on that drive. |
| `ZXV_E_NAME` | As in [`zxv_file_open`](#zxv_file_open). |
| (common) | See [Common errors](#files-and-folders). |

**Example:** see [`zxv_dir_read`](#zxv_dir_read).

**Since:** specification 0.1 · function no. 28.

#### `zxv_dir_read`

```c
int zxv_dir_read(int d, zxv_dirent_t *e);
```

Gives the next entry of an open folder (a file or a folder), in
alphabetical order, ignoring case. It does not give `.` or `..`. On the
card, hidden entries or entries with an invalid name do not appear
([Paths](#paths)).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `d` | a number from `zxv_dir_open` | The folder. |
| `e` | 72 bytes in the RAM | Where it leaves the entry, as in [`zxv_file_stat`](#zxv_file_stat). |

**Returns:** 1 if it leaves an entry in `*e`, 0 at the end, or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_BADF` | `d` is not an open folder. |
| (common) | See [Common errors](#files-and-folders). |

**Example**

```c
zxv_dirent_t e;
int d = zxv_dir_open("");
if (d >= 0) {
    while (zxv_dir_read(d, &e) == 1) {
        zxv_print_str(e.name);
        zxv_print_str(e.attr & ZXV_ATTR_DIR ? "/\n" : "\n");
    }
    zxv_dir_close(d);
}
```

**Since:** specification 0.1 · function no. 29.

#### `zxv_dir_close`

```c
int zxv_dir_close(int d);
```

Closes an open folder and frees its number.

**Returns:** 0, or an error. A folder on a card that has been taken out,
or on `EXT:` without a server, is closed all the same and gives 0.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_BADF` | `d` is not an open folder. |

**Example:** see [`zxv_dir_read`](#zxv_dir_read).

**Since:** specification 0.1 · function no. 30.

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_O_READ`, `_WRITE`, `_CREATE`, `_TRUNC`, `_APPEND` | 0x01, 0x02, 0x04, 0x08, 0x10 | Flags of `zxv_file_open` |
| `ZXV_SEEK_SET`, `_CUR`, `_END` | 0, 1, 2 | `whence` of `zxv_file_seek` |
| `ZXV_NAME_MAX` | 63 | Characters per name |
| `ZXV_PATH_MAX` | 255 | Characters per path |
| `ZXV_MAX_FILES` | 8 | Files open at once (per drive) |
| `ZXV_MAX_DIRS` | 4 | Folders open at once (per drive) |
| `ZXV_ATTR_DIR` | 0x01 | Attribute: folder |
| `ZXV_DIRENT_SIZE`, `ZXV_DE_NAME`, `_SIZE`, `_ATTR` | 72, 0, 64, 68 | `zxv_dirent_t` for assembly |

## Banks

`zxv_bank.h` and `zxvbank.c`: data that goes at the end of the `.zxv`
itself and is loaded when needed; and, with `tools/zxvbanks.py`, code too.
The explanations are in the guide: [Banks: more data than fits at
once](README.md#banks-more-data-than-fits-at-once) and [Code
banks](README.md#code-banks-more-code-than-fits-at-once).

- Data banks are added when converting the ELF:
  `elf2zxv.py game.elf game.zxv --bank stage1=stage1.gfx --pack` (with
  `--pack`, each one is compressed if that makes it smaller).
- They are read from the program's own file, which the ROM keeps open
  while it runs (`ZXV_SV_PROG_FILE`): the current folder or drive does not
  matter. That file takes one of the 8 of its drive.
- They use about 2 KB of stack.
- **They need a ROM with specification 0.2** or later; with an older one,
  everything gives `ZXV_E_ACCESS` (the program loads and runs all the
  same).

### `zxv_bank_load`

```c
int zxv_bank_load(const char *name, void *dst, uint32_t max);
```

Loads a data bank into memory: it unpacks it straight into `dst` if it is
compressed, and checks its CRC.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | up to 15 characters | The name of the bank, as in `--bank`. Here case does matter. |
| `dst` | in the RAM | Where it is loaded. |
| `max` | bytes | The room at `dst`. [`zxv_bank_size`](#zxv_bank_size) says how much is needed. |

**Returns:** the size of the bank (unpacked), or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOENT` | There is no data bank with that name (a code bank does not count: its bridges load it), or the program has no banks. |
| `ZXV_E_RANGE` | The bank is bigger than `max`. `dst` is not touched. |
| `ZXV_E_CHANGED` | The file is no longer the running program's (another version copied to the card, or rebuilt on the PC while it runs from `EXT:`): the program has to be loaded again. |
| `ZXV_E_FORMAT` | The bank is damaged: its CRC does not match, it does not unpack properly or the file ends too soon. |
| `ZXV_E_ACCESS` | The ROM is older than specification 0.2. |
| `ZXV_E_NOCARD`, `ZXV_E_OFFLINE`, `ZXV_E_IO` | Those of reading the file: no card, no `EXT:` server, a read error. |
| `ZXV_E_FAULT` | `dst` does not have room in the RAM for the bank (with a compressed one, the program stops with an exception). |

**With `ZXV_E_FORMAT` or a read error, whatever is in `dst` is no longer
valid:** it is written before the CRC is checked. With the other errors,
`dst` has not been touched.

**Example**

```c
#include "zxv_bank.h"

ZXV_VRAM1_DATA static uint8_t stage[64 * 1024];

int n = zxv_bank_load("stage2", stage, sizeof stage);
if (n < 0)
    zxv_report_error(n, "Cannot load stage 2");
```

**Needs:** a ROM with specification 0.2. Link `zxvbank.c`.

**See also:** [`zxv_bank_size`](#zxv_bank_size),
[`zxv_file_load`](#zxv_file_load), [The `.zxv` file
format](#the-zxv-file-format).

### `zxv_bank_size`

```c
int zxv_bank_size(const char *name);
```

The unpacked size of a bank, to make room for it before loading it. It
works for data banks and for code banks.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | up to 15 characters | The name of the bank. |

**Returns:** the bytes, or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOENT` | There is no bank with that name, or the program has no banks. |
| `ZXV_E_CHANGED`, `ZXV_E_ACCESS`, `ZXV_E_NOCARD`, `ZXV_E_OFFLINE`, `ZXV_E_IO` | As in [`zxv_bank_load`](#zxv_bank_load). |
| `ZXV_E_FORMAT` | The bank's entry in the index makes no sense. |

**Example**

```c
int size = zxv_bank_size("music");
if (size > 0 && (uint32_t)size <= sizeof buffer)
    zxv_bank_load("music", buffer, sizeof buffer);
```

**Needs:** a ROM with specification 0.2. Link `zxvbank.c`.

### `zxv_bank_preload`

```c
int zxv_bank_preload(const char *name);
```

Loads a code bank now, to choose when it shows (a stage's banner).
Without it, the bank loads by itself the first time one of its functions
is called, and then an error stops the program; with it, you handle the
error. It only exists in programs built with `tools/zxvbanks.py` (which
links it with `zxvbankcode.c`).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | up to 15 characters | The name of the bank, as in `--code`. |

**Returns:** 0 (also if it was already loaded), or an error. If reading it
fails, no bank is left in its region (it is filled with `ebreak`); with
the other errors, the region stays as it was.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOENT` | The program has no code bank with that name. |
| `ZXV_E_ACCESS` | Another bank in its region is in use (one of its functions is running); the frame hook points into its region; or the ROM is older than specification 0.2. |
| `ZXV_E_FORMAT` | The bank is damaged, or is not the one that was linked (another region, another address, bigger than its region). |
| `ZXV_E_CHANGED`, `ZXV_E_NOCARD`, `ZXV_E_OFFLINE`, `ZXV_E_IO` | As in [`zxv_bank_load`](#zxv_bank_load). |

**Example**

```c
banner("STAGE 2");
int r = zxv_bank_preload("valley");
if (r < 0)
    zxv_report_error(r, "Cannot load the valley");
valley_start();                     /* already there: it doesn't show */
```

**Needs:** a ROM with specification 0.2, and building with
`tools/zxvbanks.py`.

**See also:** [Code bank errors](#code-bank-errors).

### Code bank errors

A code bank is loaded into its **region** from its bridge, the first time
one of its functions is called. A bridge cannot return an error, so if
something goes wrong the program stops with report `W`, the message and
the error code (in the machine's language):

| Message | Error | When |
|---|---|---|
| "Bank a in use: cannot load b in its region" | `ZXV_E_ACCESS` | A function in `a` calls (directly or not) one in `b`, in the same region |
| "Bank b: the frame hook points into its region" | `ZXV_E_ACCESS` | The frame hook is in `b`'s region |
| "Bank b: not loaded with interrupts off" | `ZXV_E_ACCESS` | `b` is called with interrupts off (from the frame hook, for example), and it is not loaded |
| "Too many nested bank calls" | `ZXV_E_RANGE` | More than 32 nested bank calls |
| "Bank b: the program changed: load it again" | `ZXV_E_CHANGED` | As in `zxv_bank_load` |
| "Bank b: it is damaged" | `ZXV_E_FORMAT` | As in `zxv_bank_preload` |
| "Bank b: it is not in the file" | `ZXV_E_NOENT` | The `.zxv` does not have that bank |
| "Bank b: the ROM has no banks" | `ZXV_E_ACCESS` | The ROM is older than specification 0.2 |
| "Bank b: no card", "Bank b: no server for EXT:", "Bank b: cannot be read" | `ZXV_E_NOCARD`, `ZXV_E_OFFLINE`, another | Those of reading the file |

For example: `W Bank valley: it is damaged (-14), 0:1`.

A bank fills its region from the start. Its variables start again on
every load, and the region is filled with `ebreak` (`c.ebreak`, 0x9002)
before it loads, so a call to something that is gone stops at once.

### Bank constants

To read the banks of a `.zxv` yourself (in a PC program, for example). The
format and the entry of each bank (`ZXV_BANK_E_*`) are in [The `.zxv` file
format](#the-zxv-file-format).

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_BANK_MAGIC` | `"ZXVBANK1"` | The signature of the tail, at the end of the file |
| `ZXV_BANK_TAIL` | 16 | Bytes of the tail: the signature (8), where the index starts and its size |
| `ZXV_BANK_T_INDEX`, `ZXV_BANK_T_INDEX_LEN` | 8, 12 | In the tail: where the index starts and its size (u32) |
| `ZXV_BANK_HEAD` | 16 | Bytes of the index header |
| `ZXV_BANK_I_COUNT` | 0 | In the header: how many banks (u32) |
| `ZXV_BANK_I_PROG_CRC` | 4 | The CRC of the program image, its fingerprint (u32); it must be that of `ZXV_SV_PROG_CRC` |
| `ZXV_BANK_I_IMAGE_SIZE` | 8 | The size of the image (u32) |
| `ZXV_BANK_I_CRC` | 12 | The CRC-32 of the entries (u32) |
| `ZXV_BANK_ENTRY` | 48 | Bytes per entry, after the header |
| `ZXV_BANK_NAME_MAX` | 15 | Characters per name |
| `ZXV_BANK_DATA`, `ZXV_BANK_CODE` | 0, 1 | Bank kinds (`ZXV_BANK_E_KIND`) |
| `ZXV_BANK_STORED`, `ZXV_BANK_DEFLATE` | 0, 1 | How it is stored (`ZXV_BANK_E_METHOD`): as it is, or with raw deflate (RFC 1951) |

## Clock and language

### `zxv_clock_get`, `zxv_clock_set`

```c
typedef struct {
    uint16_t year;                      /* 2000-2099 */
    uint8_t month, day, hour, min, sec;
    uint8_t wday;                       /* 1 Monday ... 7 Sunday (set by clock_get) */
} zxv_clock_t;

int zxv_clock_get(zxv_clock_t *c);
int zxv_clock_set(const zxv_clock_t *c);
```

Read and set the date and time of the machine's clock: local time,
without time zones. With a clock module with a battery, the time is kept
while switched off; without it, it lasts until the machine is switched
off, and at power-on the clock is not set. Files created on the card get
this date.

Both are slow (they ask the clock): to measure time, use `zxv_time_us` or
`ZXV_SV_FRAMES`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `c` | 8 bytes in the RAM (when setting, also in the ROM) | The date and time. |
| `year` | 2000–2099 | The year. |
| `month`, `day` | 1–12; 1–28, 29, 30 or 31 | The month and the day, which must exist (29 February, only in leap years). |
| `hour`, `min`, `sec` | 0–23, 0–59, 0–59 | The time. |
| `wday` | 1 Monday ... 7 Sunday | The day of the week: `zxv_clock_get` sets it; `zxv_clock_set` ignores it. |

**Returns:** 0, or an error. `zxv_clock_get` does not touch `*c` if it
gives an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_NOCLOCK` | (`zxv_clock_get`) The clock is not set: it has not been set since power-on, or the module lost the time (no battery). With a firmware older than the clock, both functions. |
| `ZXV_E_INVAL` | (`zxv_clock_set`) The date does not exist or is outside 2000–2099, or the time is not valid. |
| `ZXV_E_FAULT` | `c` is not in the allowed memory. |

For assembly: `ZXV_CLOCK_SIZE` (8) and the offsets `ZXV_CK_YEAR` (0),
`ZXV_CK_MONTH` (2), `ZXV_CK_DAY` (3), `ZXV_CK_HOUR` (4), `ZXV_CK_MIN` (5),
`ZXV_CK_SEC` (6), `ZXV_CK_WDAY` (7).

**Example**

```c
zxv_clock_t c;
if (zxv_clock_get(&c) == 0 && c.month == 12 && c.day == 25)
    put_on_christmas_hat();
```

**Since:** specification 0.1 · functions no. 48 and 49.

**See also:** [`zxv_time_us`](#zxv_time_us).

### `zxv_lang_get`, `zxv_lang_set`

```c
uint32_t zxv_lang_get(void);
int zxv_lang_set(uint32_t lang);
```

The machine's language: a two-letter ISO 639-1 code in lower case, first
letter in the low byte, made with `ZXV_LANG(a, b)`. `ZXV_LANG_EN` is
`ZXV_LANG('e', 'n')` and `ZXV_LANG_ES` is `ZXV_LANG('e', 's')`.

The ROM has its texts in English and Spanish (with any other code, it
gives them in English), but any code is valid, so programs can use it to
choose their own texts. `zxv_lang_get` reads the system variable
`ZXV_SV_LANG`. `zxv_lang_set` changes it; with a clock module, the machine
remembers it while switched off (without one, until it is switched off).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `lang` | `ZXV_LANG(a, b)`, `a` and `b` from `'a'` to `'z'` | The language. |

**Returns:** `zxv_lang_get`, the code (English, `ZXV_LANG_EN`, if none has
ever been chosen); `zxv_lang_set`, 0 or an error.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_INVAL` | (`zxv_lang_set`) The code is not two lower-case letters (or has something in bits 16–31). Nothing changes. |

**Example**

```c
const char *hello = zxv_lang_get() == ZXV_LANG_ES ? "Hola" : "Hello";
```

**Since:** specification 0.1. `zxv_lang_get` is in `zxv_api.h` (it reads
the system variable); `zxv_lang_set` is function no. 50.

## Ending a program and running others

When a program ends (in whatever way), the memory belongs to BASIC again,
so the BASIC program that was in it is lost. The ROM removes the frame
hook, goes back to page 0 without scrolling or virtual playfield and
silences both AY chips. The next `.zxv` starts with the usual colours,
palettes and text.

### `zxv_exit_to_basic`

```c
void zxv_exit_to_basic(int code);
```

Ends the program from anywhere, however deep inside it is, and returns to
BASIC: the same as returning `code` from `main`. It does not return.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `code` | any `int` | **0 means success**: BASIC shows `0 OK, 0:1`. Any other value means an error, and BASIC shows `W Native code error (code), 0:1` (in Spanish, `W Error en el código nativo (code), 0:1`), with its sign if it is negative. |

The value `main` returns works the same way. Native code called from BASIC
with `USR` is different: what it returns is just the value of `USR`.

**Example**

```c
if (!card_present())
    zxv_exit_to_basic(2);           /* W Native code error (2), 0:1 */
```

**Since:** specification 0.1 · function no. 19.

**See also:** [`zxv_report_error`](#zxv_report_error).

### `zxv_exec`

```c
int zxv_exec(const char *name, int *result);
```

Loads and runs a `.zxv`, and returns when it ends. It is what BASIC
(`LOAD`) and the system monitor do.

**A native program cannot use it**: called from a `.zxv` (whether BASIC
or the monitor started it), it returns `ZXV_E_ACCESS` and does nothing. Every program is loaded at the same
address, at the start of the user RAM, so the new one would overwrite the
one that called it.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name` | path | The program. If it has no extension and does not exist, `.zxv` is added and tried. |
| `result` | | Where it leaves what the program returned. |

**Returns:** 0 if the program was loaded and ended by returning from
`main` (its value, in `*result`); or an error, and nothing is run. The
whole file is checked (the header, that it fits and the CRC) before the
memory is touched.

**Errors**

| Error | When |
|---|---|
| `ZXV_E_ACCESS` | A native program calls it. |
| `ZXV_E_FORMAT` | It is not a valid program: the header, the loading screen or the CRC do not match, or the file ends too soon. |
| `ZXV_E_RANGE` | It does not fit in the user RAM (or its stack is not valid). |
| `ZXV_E_CHANGED` | The file changed between the check and the load. |
| `ZXV_E_NOENT`, `ZXV_E_ISDIR`, `ZXV_E_NAME`, `ZXV_E_NOTDIR`, `ZXV_E_MFILE` | As in [`zxv_file_open`](#zxv_file_open). |
| (common) | See [Common errors](#files-and-folders). |

**Since:** specification 0.1 · function no. 31.

**See also:** [The `.zxv` file format](#the-zxv-file-format).

### `zxv_report_error`

```c
void zxv_report_error(int code, const char *msg);
```

Ends the program and returns to BASIC with the report `W` and your own
message instead of BASIC's. It does not return. The message is copied
before BASIC takes back the memory, so it can be in a local array.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `code` | any `int` | If it is not 0, it follows the message in brackets, with its sign. A `ZXV_E_*` error shows as it is: `(-1)`. |
| `msg` | UTF-8, up to 79 bytes | The message. Whatever goes past 79 bytes is not shown (with a ROM older than specification 0.2, past 47). Empty (`""`), or `NULL`, leaves BASIC's usual text. |

For example, `zxv_report_error(5, "Fichero dañado")` gives
`W Fichero dañado (5), 0:1`; `zxv_report_error(0, "No level files")`
gives `W No level files, 0:1`; and `zxv_report_error(5, "")`,
`W Native code error (5), 0:1`.

BASIC's screen font has the Spanish letters, so `ñ`, `¿` or `á` can go in
the message; other non-ASCII characters show as `?`. Count bytes, not
letters: in UTF-8, each accented letter takes 2.

**Example**

```c
int size = zxv_file_load("level1.dat", level, sizeof level);
if (size < 0)
    zxv_report_error(size, "Cannot load level1.dat");
/* if it does not exist: W Cannot load level1.dat (-1), 0:1 */
```

**Since:** specification 0.1 · function no. 20 (up to 79 bytes since
specification 0.2; before, 47).

**See also:** [`zxv_exit_to_basic`](#zxv_exit_to_basic),
[Error codes](#error-codes).

## Graphics engine

Declared in `zxv_gfx.h`. See [Graphics](graphics.md) for the explanations
and the longer examples. All functions draw on the current **target** (the
drawing page or a bitmap in memory) and clip to it and to the clip
rectangle. The result is defined pixel by pixel by the ROM code: the
emulator and the machine draw exactly the same pixels.

**Common behaviour.** No engine function returns an error. What goes
beyond what is accepted is always handled the same way:

| Case | What the ROM does |
|---|---|
| Coordinates outside the target or the clip rectangle | Clips: only what falls inside is drawn. Coordinates are signed, so a shape or a sprite can be half outside. Nothing is ever written outside the target. |
| A colour above 15 or negative | Its 4 low bits are used (`c & 15`): 16 is 0 and −1 is 15. Except `paper` in `zxv_gfx_pattern` and `out` in `zxv_m7_draw_map`, where it counts as `ZXV_CLEAR`. |
| A size of 0 or negative, a `NULL` pointer or a sprite that does not come from `zxv_spr_prepare` | Draws nothing (what each function does, in its entry). |
| A bitmap, tiles or a prepared sprite not aligned to 4 bytes | Not checked: the engine reads and writes 32-bit words, and an unaligned access stops the program. |
| A bitmap whose `pix`, `stride`, `w` or `h` do not match its memory | Not checked: it writes wherever they say. |

When a program starts (and each time `zxv_exec` starts another one), the
target is the drawing page 0, without a clip rectangle and without a
pattern. `zxv_cls`, `zxv_plot` and the text do not use the engine: they
always draw on the drawing page, even when the target is a bitmap, and
ignore the clip rectangle and the pattern.

### Types

```c
typedef struct {            /* a bitmap */
    uint8_t *pix;           /* first pixel, aligned to 4 */
    uint16_t stride;        /* bytes per line, a multiple of 4 */
    uint16_t w, h;          /* in pixels */
    uint16_t reserved;
} zxv_bitmap;

typedef struct {            /* a tile map */
    const uint16_t *cells;  /* cells, row by row */
    uint16_t w, h;          /* in cells */
    const uint8_t *tiles;   /* tile graphics, one after another (32 or 128 bytes) */
    uint8_t size;           /* tile side: 8 or 16 */
    uint8_t flags;          /* ZXV_MAP_WRAP */
} zxv_tilemap;

typedef struct {            /* a mode 7 texture */
    const uint8_t *texels;  /* 1 byte per texel (0-15), row by row */
    uint8_t log2size;       /* side = 2^log2size (1 to 10) */
    uint8_t wrap;           /* not 0: repeats; 0: outside is colour 'out' */
    uint8_t out;
    uint8_t flags;          /* ZXV_M7_TRANSPARENT; formerly 'reserved', 0 */
} zxv_m7tex;

typedef struct { int32_t u, v, du, dv; } zxv_m7line;   /* 16.16 fixed point */

typedef struct {            /* a rotated and scaled plane, 16.16 fixed point */
    int32_t x, y;           /* the point of the plane that goes at the centre */
    int32_t angle;          /* 0x10000 = a full turn, clockwise */
    int32_t zoom_x, zoom_y; /* scale on each axis of the plane: 0x10000 = 1 */
    int16_t cx, cy;         /* the centre, on the target (integers) */
} zxv_m7aff;

typedef struct {            /* a mode 7 camera, 16.16 fixed point */
    int32_t x, y;           /* position on the plane */
    int32_t height;         /* height above the floor */
    int32_t angle;          /* 0x10000 = a full turn; 0 looks towards growing u */
    int32_t focal;          /* focal distance, in pixels (also 16.16) */
    int16_t horizon;        /* target line of the horizon (integer) */
    int16_t reserved;
} zxv_m7cam;
```

Pixels are 4 bits, two per byte, the even pixel in the low 4 bits, as in
VRAM. A bitmap `w` pixels wide needs at least `w / 2` bytes per line,
rounded up to a multiple of 4: a 64 × 32 bitmap has 32 bytes per line and
1,024 bytes. Bitmaps, tiles and prepared sprites must be aligned to 4
bytes (an array of `uint32_t` always is).

An 8 × 8 tile is 32 bytes (4 per line) and a 16 × 16 one, 128 (8 per
line), one after another in `tiles`: tile *n* starts at `tiles + n·32` or
`tiles + n·128`. A map cell is a 16-bit number with the tile number and
its attributes (the `ZXV_CELL_*` constants, below).

### Graphics engine constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_TRANSPARENT` | 8 | The transparent colour (in `zxv_gfx_blit` with `ZXV_BLIT_TRANSPARENT`, in sprites, in cells with `ZXV_CELL_TRANSP` and in textures with `ZXV_M7_TRANSPARENT`) |
| `ZXV_CLEAR` | −1 | A paper (`zxv_gfx_pattern`) or an outside colour (`zxv_m7_draw_map`) that is not painted |
| `ZXV_FLIP_H`, `ZXV_FLIP_V` | 0x01, 0x02 | Mirror horizontally, vertically (`zxv_gfx_blit` and sprites) |
| `ZXV_BLIT_TRANSPARENT` | 0x04 | `zxv_gfx_blit`: do not copy colour 8 |
| `ZXV_MAP_WRAP` | 0x01 | `zxv_tilemap.flags`: the map repeats in both directions |
| `ZXV_CELL_TILE` | 0x0FFF | Cell: tile number (0–4095) |
| `ZXV_CELL_FLIP_H`, `ZXV_CELL_FLIP_V` | 0x1000, 0x2000 | Cell: mirrored horizontally, vertically |
| `ZXV_CELL_TRANSP` | 0x4000 | Cell: colour 8 of the tile shows what is underneath (without it, the tile is opaque and is drawn faster) |
| `ZXV_CELL_PRIO` | 0x8000 | Cell: priority (for the `zxv_map_draw` filter) |
| `ZXV_MAP_ALL`, `ZXV_MAP_BACK`, `ZXV_MAP_FRONT` | 0, 1, 2 | `zxv_map_draw` filter: all cells, only those without priority, only those with it |
| `ZXV_M7_TRANSPARENT` | 0x01 | `zxv_m7tex.flags`: colour 8 of the texture (and `out` if it is 8) is not drawn. Since specification 0.2 |

Angles (`zxv_m7cam.angle`, `zxv_m7aff.angle`) are in turns of 0x10000:
0x4000 is a quarter turn. The maths library,
[`zxv_math.h`](libraries.md#maths-zxv_mathh), calls it `ZXV_TURN`
(0x10000) and provides `ZXV_DEG(degrees)`, with the same sine as the ROM.

### Target and clipping

Where drawing goes: the drawing page (by default) or a bitmap in memory,
and the clip rectangle inside it.

### `zxv_gfx_target`

```c
void zxv_gfx_target(const zxv_bitmap *b);
```

Makes bitmap `b` the target: from now on, the whole engine draws on it.
With `NULL` (or `0`), the target goes back to the drawing page. It always
removes the clip rectangle. The structure is copied, so it can be a local
variable; the pixels are not, of course: they must stay there while
drawing on them.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `b` | a bitmap, or `NULL` | The new target. `NULL`: the drawing page, with whatever size it has at each moment (see [`zxv_gfx_draw_page`](#zxv_gfx_draw_page)). |

**Out of range**

| Case | What the ROM does |
|---|---|
| `b->pix` is `NULL`, or `w` or `h` is 0 | Accepts it, but nothing is drawn until the target changes. |
| `pix` not aligned to 4, `stride` not a multiple of 4 or smaller than `w / 2` | Not checked (see [Common behaviour](#graphics-engine)). |

**Example**

```c
static uint32_t buf_pix[64 * 32 / 8];           /* 64 x 32 pixels, aligned to 4 */
const zxv_bitmap buf = { (uint8_t *)buf_pix, 32, 64, 32, 0 };
zxv_gfx_target(&buf);                           /* draw in memory */
zxv_gfx_rect(0, 0, 64, 32, 1);
zxv_gfx_circle(32, 16, 12, 6, 1);
zxv_gfx_target(0);                              /* the drawing page again */
zxv_gfx_blit(&buf, 0, 0, 64, 32, 96, 80, 0);    /* and it is copied onto it */
```

**Since:** specification 0.1 · API function no. 32.

**See also:** [`zxv_gfx_draw_page`](#zxv_gfx_draw_page),
[`zxv_gfx_clip`](#zxv_gfx_clip), [`zxv_gfx_blit`](#zxv_gfx_blit).

### `zxv_gfx_draw_page`

```c
void zxv_gfx_draw_page(int page);
```

Chooses the drawing page: the VRAM page on which the engine (when the
target is the page), `zxv_cls`, `zxv_plot` and the text draw. It does not
change the visible page (that is [`zxv_set_page`](#zxv_set_page)), so it
is used for double buffering: draw on the page that is not shown and then
show it. It does not touch the target: if it is a bitmap, the engine keeps
drawing on it. Nor does it remove the clip rectangle.

The page size follows the video mode and `ZXV_V_VSIZE` on every call to
the engine: 256 × 192 in mode 0 (or the one from `ZXV_V_VSIZE`, see
[Video registers](#video-registers)) and 512 × 384 in mode 1.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `page` | 0 or 1 | The page: 0 at `ZXV_VRAM0_BASE` and 1 at `ZXV_VRAM1_BASE`. |

**Out of range**

| Case | What the ROM does |
|---|---|
| Another value | Its low bit counts (`page & 1`): 2 is page 0 and −1, page 1. |

**Example**

```c
int visible = 0;
for (int frame = 0; zxv_key_get() < 0; frame++) {
    int hidden = 1 - visible;
    zxv_gfx_draw_page(hidden);                  /* draw on the one not shown */
    zxv_cls(0);
    zxv_gfx_circle(frame % 256, 96, 20, 6, 1);
    zxv_set_page(hidden);                       /* and show it */
    zxv_wait_vblank();
    visible = hidden;
}
zxv_set_page(0);
zxv_gfx_draw_page(0);
```

**Since:** specification 0.1 · API function no. 33.

**See also:** [`zxv_set_page`](#zxv_set_page),
[`zxv_gfx_target`](#zxv_gfx_target),
[Graphics: pages and double buffering](graphics.md#pages-and-double-buffering).

### `zxv_gfx_clip`

```c
void zxv_gfx_clip(int x0, int y0, int x1, int y1);
```

Limits the engine's drawing to the rectangle from (x0, y0) to (x1, y1),
excluding x1 and y1, in target coordinates. What falls outside is not
touched. The rectangle is stored as given and on every drawing it is
intersected with the target, so it can go beyond it. It lasts until the
next `zxv_gfx_clip` or `zxv_gfx_target` (which removes it);
`zxv_gfx_draw_page` does not change it.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x0`, `y0` | any integer | The top left corner, included. |
| `x1`, `y1` | any integer | The bottom right corner, **excluded**: the rectangle is x1 − x0 × y1 − y0. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `x1 <= x0` or `y1 <= y0`, or the rectangle does not touch the target | Accepts it: nothing is drawn until the clip rectangle changes. |

To remove the clip rectangle without changing the target, set the same
target again: `zxv_gfx_target(0)` (or whichever bitmap it was).

**Example**

```c
zxv_gfx_clip(0, 0, 256, 160);           /* the score panel, below, is not touched */
zxv_gfx_circle(128, 150, 40, 2, 1);     /* comes out cut off at line 160 */
zxv_gfx_target(0);                      /* no clip rectangle again */
```

**Since:** specification 0.1 · API function no. 34.

**See also:** [`zxv_gfx_target`](#zxv_gfx_target).

### Shapes

Lines, rectangles, triangles, polygons and circles, in a plain colour or
with the pattern of [`zxv_gfx_pattern`](#zxv_gfx_pattern). Triangles and
polygons follow a **top-left rule**: a pixel is painted if its centre
falls inside, with the top and left sides included; so two of them
sharing a side neither overlap nor leave a gap.

### `zxv_gfx_line`

```c
void zxv_gfx_line(int x0, int y0, int x1, int y1, int c);
```

A line from (x0, y0) to (x1, y1), including both ends. At each step along
the longer axis, the other one moves to the pixel nearest to the line
(ties go towards the end). With a pattern set, each pixel takes the colour
the pattern gives it where it falls.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x0`, `y0` | any integer | One end. |
| `x1`, `y1` | any integer | The other. If it is the same point, one pixel. |
| `c` | 0–15 | The colour. |

**Out of range**

| Case | What the ROM does |
|---|---|
| Ends outside the target, even very far away | Clips: draws exactly the pixels it would draw without clipping that fall inside. |
| `c` outside 0–15 | (common) [Its 4 low bits](#graphics-engine). |

**Example**

```c
for (int i = 0; i < 16; i++)
    zxv_gfx_line(128, 191, i * 17, 0, i);       /* a fan with the 16 colours */
```

**Since:** specification 0.1 · API function no. 43.

**See also:** [`zxv_plot`](#zxv_plot), [`zxv_gfx_poly`](#zxv_gfx_poly),
[`zxv_gfx_pattern`](#zxv_gfx_pattern).

### `zxv_gfx_rect`

```c
void zxv_gfx_rect(int x, int y, int w, int h, int c);
```

A filled rectangle of `w` × `h` pixels with its top left corner at
(x, y). It is the fastest way to fill: 8 pixels at a time, also with a
pattern.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x`, `y` | any integer | The top left corner. |
| `w`, `h` | 1 or more | The width and the height, in pixels. |
| `c` | 0–15 | The colour. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `w` or `h` is 0 or less | Nothing. |
| `c` outside 0–15 | (common) [Its 4 low bits](#graphics-engine). |

**Example**

```c
zxv_gfx_rect(0, 160, 256, 32, 1);       /* a blue strip at the bottom, for the score panel */
```

**Since:** specification 0.1 · API function no. 44.

**See also:** [`zxv_cls`](#zxv_cls), [`zxv_gfx_poly`](#zxv_gfx_poly).

### `zxv_gfx_tri`

```c
void zxv_gfx_tri(const int16_t *p, int c);
```

A filled triangle. It is the same as `zxv_gfx_poly(p, 3, c)`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `p` | 6 numbers from −32768 to 32767 | The three vertices, as pairs x, y: `{ x0, y0, x1, y1, x2, y2 }`. The direction does not matter. |
| `c` | 0–15 | The colour. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `p` is `NULL` | Nothing. |
| The three vertices in a line | Nothing (it does not cover the centre of any pixel). |
| `c` outside 0–15 | (common) [Its 4 low bits](#graphics-engine). |

**Example**

```c
static const int16_t t[] = { 128, 20,  200, 170,  56, 170 };
zxv_gfx_tri(t, 4);                      /* a green triangle */
```

**Since:** specification 0.1 · API function no. 45.

**See also:** [`zxv_gfx_poly`](#zxv_gfx_poly).

### `zxv_gfx_poly`

```c
void zxv_gfx_poly(const int16_t *p, int n, int c);
```

A filled polygon of `n` vertices, with the even-odd rule: a pixel is
painted if a line from it crosses an odd number of sides. So a
five-pointed star drawn in one go has a hole in the middle, and a polygon
can have holes if its outline is traced there and back. The vertices are
joined in order and the last one with the first.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `p` | `2·n` numbers from −32768 to 32767 | The vertices, as pairs x, y. |
| `n` | 3–64 | How many vertices. |
| `c` | 0–15 | The colour. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `p` is `NULL`, or `n` is less than 3 or greater than 64 | Nothing. |
| `c` outside 0–15 | (common) [Its 4 low bits](#graphics-engine). |

**Example**

```c
static const int16_t star[] = { 128, 20,  168, 160,  56, 74,  200, 74,  88, 160 };
zxv_gfx_poly(star, 5, 6);               /* a yellow star, hollow in the middle */
```

**Since:** specification 0.1 · API function no. 46.

**See also:** [`zxv_gfx_tri`](#zxv_gfx_tri),
[`zxv_gfx_pattern`](#zxv_gfx_pattern).

### `zxv_gfx_circle`

```c
void zxv_gfx_circle(int x, int y, int r, int c, int fill);
```

A circle of radius `r` centred on (x, y): only the outline, or a filled
disc. It is 2·r + 1 pixels from side to side.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x`, `y` | any integer | The centre. |
| `r` | 0 or more | The radius, in pixels. With 0, one pixel. |
| `c` | 0–15 | The colour. |
| `fill` | 0 or other | 0: only the outline; any other value: a disc. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `r` negative | Nothing. |
| `c` outside 0–15 | (common) [Its 4 low bits](#graphics-engine). |

**Example**

```c
zxv_gfx_circle(128, 96, 50, 2, 1);      /* a red disc... */
zxv_gfx_circle(128, 96, 50, 7, 0);      /* ...with a white edge */
```

**Since:** specification 0.1 · API function no. 47.

**See also:** [`zxv_gfx_pattern`](#zxv_gfx_pattern).

### `zxv_gfx_pattern`

```c
void zxv_gfx_pattern(const uint8_t *pat, int paper);
```

Sets an 8 × 8 **pattern** for the shapes: `zxv_gfx_line`,
`zxv_gfx_rect`, `zxv_gfx_tri`, `zxv_gfx_poly` and `zxv_gfx_circle`. Where
the pattern has a 1 the shape's colour is painted, and where it has a 0,
colour `paper`, or nothing with `ZXV_CLEAR`. The pattern is anchored to
the target: pixel (x, y) uses bit 7 − (x mod 8) of `pat[y mod 8]`, so two
shapes that touch match up. Filling with a pattern costs the same as with
a plain colour. On lines and outlines, how it shows depends on where each
pixel falls: a 45° diagonal over a checkerboard comes out all one colour.

The pattern is copied: `pat` can be a local variable. It lasts until the
next call; every program starts with no pattern. It does not affect
copies, sprites, maps or mode 7, nor `zxv_cls`, `zxv_plot` or the text.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `pat` | 8 bytes, or `NULL` | One row per byte, from top to bottom; bit 7 is the leftmost pixel. `NULL` (or `0`): back to plain colour. |
| `paper` | 0–15, or `ZXV_CLEAR` | The colour of the zeros of the pattern, or `ZXV_CLEAR` not to paint them (what is underneath shows). Not used when `pat` is `NULL`. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `paper` negative or greater than 15 | Like `ZXV_CLEAR`: the zeros are not painted. |
| With a ROM of specification 0.1 | The function does not exist: it does nothing and shapes come out plain. |

**Example**

```c
static const uint8_t check[8] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
zxv_gfx_pattern(check, 0);              /* checkerboard of the shape's colour and black */
zxv_gfx_rect(16, 16, 96, 64, 2);
zxv_gfx_pattern(check, ZXV_CLEAR);      /* here, what is underneath shows between the dots */
zxv_gfx_circle(160, 100, 40, 6, 1);
zxv_gfx_pattern(0, 0);                  /* plain colour again */
```

**Since:** specification 0.2 · API function no. 56.

**See also:** [`PATTERN`](../basic/reference.md#pattern) in BASIC,
[Graphics: shapes with patterns](graphics.md#shapes-with-patterns).

### Copies

Copying a rectangle of a bitmap to the target, as it is.

### `zxv_gfx_blit`

```c
void zxv_gfx_blit(const zxv_bitmap *src, int sx, int sy, int w, int h, int x, int y, int flags);
```

Copies the rectangle (sx, sy) of `w` × `h` pixels of `src` to (x, y) of
the target, mirrored if asked, and without colour 8 with
`ZXV_BLIT_TRANSPARENT`. The part of the rectangle that falls outside `src`
is not copied (nothing is read outside the bitmap). With mirroring, the
whole rectangle is flipped and then placed at (x, y). For many copies of
the same picture with a transparent 8, a prepared sprite is faster.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `src` | a bitmap | Where to copy from. It can be a VRAM page described as a bitmap. |
| `sx`, `sy` | any integer | The corner of the rectangle in `src`. |
| `w`, `h` | 1 or more | The size of the rectangle. The part that falls inside `src` can be up to 1024 pixels wide. |
| `x`, `y` | any integer | Where the corner goes on the target. |
| `flags` | `ZXV_FLIP_H`, `ZXV_FLIP_V`, `ZXV_BLIT_TRANSPARENT`, combined with `\|`, or 0 | Mirroring and transparency. Other bits do not count. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `src` is `NULL`, or `w` or `h` is 0 or less | Nothing. |
| The rectangle does not touch `src` | Nothing. |
| The part inside `src` is more than 1024 pixels wide | Nothing. |
| `src` is the target and the two areas overlap | Not defined. |

**Example**

```c
static uint32_t strip_pix[64 * 16 / 8];         /* 4 frames of 16 x 16 */
const zxv_bitmap strip = { (uint8_t *)strip_pix, 32, 64, 16, 0 };
int f = 2, x = 100, y = 80;
/* Frame f, facing left, without its colour 8. */
zxv_gfx_blit(&strip, f * 16, 0, 16, 16, x, y, ZXV_FLIP_H | ZXV_BLIT_TRANSPARENT);
```

**Since:** specification 0.1 · API function no. 35.

**See also:** [`zxv_gfx_target`](#zxv_gfx_target),
[`zxv_spr_prepare`](#zxv_spr_prepare),
[`zxv_gfxf_sprite`](#zxv_gfxf_sprite).

### Sprites

A **prepared sprite** is a picture with its mask already worked out, to
draw it with a transparent 8, 8 pixels at a time. It is made once with
`zxv_spr_prepare` and drawn as many times as needed. Its internal format
belongs to the ROM: a sprite is used only with these functions.

### `zxv_spr_prepare`

```c
uint32_t zxv_spr_prepare(const zxv_bitmap *src, int sx, int sy, int w, int h, void *buf);
```

Converts the rectangle (sx, sy) of `w` × `h` pixels of `src` into a
prepared sprite, in `buf`. Pixels of colour 8, and those of the rectangle
that fall outside `src`, become transparent. With `buf` set to `NULL` it
writes nothing and only says how big it is: that is how you ask for the
size first.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `src` | a bitmap | The picture. It is also needed to ask for the size. |
| `sx`, `sy` | any integer | The corner of the rectangle in `src`. |
| `w`, `h` | 1–1024 | The size of the sprite. |
| `buf` | memory aligned to 4, or `NULL` | Where it is written, with room for what it returns (not checked). |

**Returns:** the size of the sprite in bytes, 8 + h × ⌈w / 8⌉ × 8 (264
for a 16 × 16 one), whether it wrote it or not; 0 if it cannot be
prepared.

**Out of range**

| Case | What the ROM does |
|---|---|
| `src` is `NULL`, or `w` or `h` outside 1–1024 | Returns 0 and writes nothing. |
| `buf` smaller than the size | Not checked: it writes past the end. |

**Example**

```c
static uint32_t sheet_pix[32 * 16 / 8];         /* two 16 x 16 pictures */
const zxv_bitmap sheet = { (uint8_t *)sheet_pix, 16, 32, 16, 0 };
static uint32_t ship[264 / 4];                  /* 8 + 16 · 2 · 8 bytes */
uint32_t n = zxv_spr_prepare(&sheet, 0, 0, 16, 16, NULL);    /* 264 */
if (n && n <= sizeof ship)
    zxv_spr_prepare(&sheet, 0, 0, 16, 16, ship);
```

**Since:** specification 0.1 · API function no. 36.

**See also:** [`zxv_spr_draw`](#zxv_spr_draw),
[`zxv_gfxf_sprite`](#zxv_gfxf_sprite),
[Graphics: sprites](graphics.md#sprites).

### `zxv_spr_draw`

```c
void zxv_spr_draw(const void *spr, int x, int y, int flags, const uint8_t *colours);
```

Draws a prepared sprite with its top left corner at (x, y). Transparent
pixels show what is underneath. With `colours`, it is **recoloured**:
each pixel of colour *c* is drawn as `colours[c]`, without changing the
transparent ones (an 8 in the table is drawn as colour 8, not as
transparent). Without mirroring or recolouring and wholly inside
horizontally, it takes a faster path.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `spr` | a sprite from `zxv_spr_prepare` | The sprite. |
| `x`, `y` | any integer | Where its corner goes. |
| `flags` | `ZXV_FLIP_H`, `ZXV_FLIP_V`, combined, or 0 | Mirroring. Other bits do not count. |
| `colours` | a table of 16 bytes (0–15), or `NULL` | The recolouring. `NULL`: as it is. Of each byte its 4 low bits are used. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `spr` is `NULL` or is not a prepared sprite (it does not carry its mark) | Nothing. |

**Example**

```c
/* ship, from zxv_spr_prepare */
uint8_t red[16];
for (int i = 0; i < 16; i++)
    red[i] = (uint8_t)i;
red[5] = 2;                                     /* cyan (5) shows as red (2) */
zxv_spr_draw(ship, 120, 90, 0, NULL);           /* as it is */
zxv_spr_draw(ship, 150, 90, ZXV_FLIP_H, red);   /* the other way round and another colour */
```

**Since:** specification 0.1 · API function no. 37.

**See also:** [`zxv_spr_draw_scaled`](#zxv_spr_draw_scaled),
[`zxv_spr_collide`](#zxv_spr_collide).

### `zxv_spr_draw_scaled`

```c
void zxv_spr_draw_scaled(const void *spr, int x, int y, int w, int h, int flags, const uint8_t *colours);
```

Draws a prepared sprite stretched or shrunk to `w` × `h` pixels, with its
corner at (x, y), for things that come closer or move away. Each pixel of
the target takes the sprite pixel that falls at its centre (nearest
neighbour): pixel *i* of the `w` is pixel ⌊(2·i + 1)·width / (2·w)⌋ of
the sprite, and the same vertically. With mirroring, pixel *i* is the one
that would be `w − 1 − i`. At its own size it does the same as
`zxv_spr_draw` (and calls it). It uses about 2.5 KB of the caller's stack
and costs about three times as much as a sprite of its final size.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `spr` | a sprite from `zxv_spr_prepare` | The sprite. |
| `x`, `y` | any integer | Where the corner goes. |
| `w`, `h` | 1–4096 | The size it is drawn at. |
| `flags` | `ZXV_FLIP_H`, `ZXV_FLIP_V`, or 0 | Mirroring, as in `zxv_spr_draw`. |
| `colours` | a table of 16 bytes, or `NULL` | The recolouring, as in `zxv_spr_draw`. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `w` or `h` outside 1–4096 | Nothing. |
| `spr` is `NULL` or is not a prepared sprite | Nothing. |

**Example**

```c
for (int s = 4; s <= 64; s += 12)               /* the ship, closer each time */
    zxv_spr_draw_scaled(ship, 128 - s / 2, 96 - s / 2, s, s, 0, NULL);
```

**Since:** specification 0.1 · API function no. 52.

**See also:** [`zxv_spr_draw`](#zxv_spr_draw),
[`zxv_m7_camera`](#zxv_m7_camera) (the `graphics` example puts scaled
sprites on a mode 7 floor).

### `zxv_spr_collide`

```c
int zxv_spr_collide(const void *a, int ax, int ay, const void *b, int bx, int by);
```

Tells whether two prepared sprites touch: whether some opaque pixel of
`a` drawn at (ax, ay) falls on the same spot as an opaque pixel of `b`
drawn at (bx, by). It compares them at their own size and without
mirroring, 8 pixels at a time; it does not look at the screen.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `a`, `b` | sprites from `zxv_spr_prepare` | The two sprites (they can be the same one). |
| `ax`, `ay` | any integer | Where the corner of `a` is. |
| `bx`, `by` | any integer | Where the corner of `b` is. |

**Returns:** 1 if they touch; 0 if not.

**Out of range**

| Case | What the ROM does |
|---|---|
| `a` or `b` is `NULL` or is not a prepared sprite | Returns 0. |

**Example**

```c
if (zxv_spr_collide(ship, sx, sy, bullet, bx, by))
    lives--;
```

**Since:** specification 0.1 · API function no. 38.

**See also:** [`zxv_map_cell`](#zxv_map_cell) (collisions with the
scenery), [`HIT`](../basic/reference.md#hit) in BASIC.

### Tile maps

A map is an array of cells; each cell says which tile goes in it and with
which attributes (mirroring, transparency and priority). See
[Graphics: tile maps](graphics.md#tile-maps).

### `zxv_map_draw`

```c
void zxv_map_draw(const zxv_tilemap *m, int mx, int my, int x, int y, int w, int h, int filter);
```

Draws, in the rectangle (x, y) of `w` × `h` pixels of the target, the part
of the map that starts at pixel (mx, my) of the map: pixel (X, Y) of the
target shows pixel (mx + X − x, my + Y − y) of the map. Each cell goes
with its mirroring, and with `ZXV_CELL_TRANSP` its colour 8 shows what is
underneath. With the filter, only the cells with priority or only those
without it are drawn: a typical frame is the background (`ZXV_MAP_BACK`),
the sprites and the front part (`ZXV_MAP_FRONT`). Cells that are not drawn
(because of the filter or because they fall outside a map that does not
repeat) leave what was there. With opaque tiles without mirroring and
aligned to 8 pixels, it takes a faster path.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `m` | a map | The map, with `size` 8 or 16 and `w` and `h` of 1 or more. |
| `mx`, `my` | any integer | The pixel of the map that goes at the corner of the rectangle. If the map repeats (`ZXV_MAP_WRAP`), it wraps round in both directions, also with negative values. |
| `x`, `y` | any integer | The corner of the rectangle on the target. |
| `w`, `h` | 1 or more | The size of the rectangle. |
| `filter` | `ZXV_MAP_ALL`, `ZXV_MAP_BACK` or `ZXV_MAP_FRONT` | Which cells are drawn. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `m` is `NULL`, its `w` or its `h` is 0, or `size` is neither 8 nor 16 | Nothing. |
| `w` or `h` is 0 or less | Nothing. |
| The visible part crosses more than 136 columns of tiles (more than about 1,080 pixels of width with 8-pixel tiles) | Nothing. It does not happen on the machine's pages (up to 1,024 wide). |
| `filter` with another value | Like `ZXV_MAP_ALL`. |
| A cell with a tile number that is not in `tiles` | Not checked: it reads whatever lies beyond. |

**Example**

```c
static uint32_t tiles[2 * 8];                   /* two 8 x 8 tiles (32 bytes each) */
static uint16_t cells[32 * 24];
for (int i = 0; i < 8; i++) {
    tiles[i] = 0x11111111u;                     /* tile 0: plain blue */
    tiles[8 + i] = i & 1 ? 0x66666666u : 0x44444444u;   /* tile 1: stripes */
}
for (int i = 0; i < 32 * 24; i++)
    cells[i] = i / 32 == 23 ? 1 : 0;            /* the last row, the ground */
const zxv_tilemap map = { cells, 32, 24, (const uint8_t *)tiles, 8, ZXV_MAP_WRAP };
zxv_map_draw(&map, scroll_x, 0, 0, 0, 256, 192, ZXV_MAP_ALL);
```

**Since:** specification 0.1 · API function no. 39.

**See also:** [`zxv_map_cell`](#zxv_map_cell),
[`zxv_gfxf_map`](#zxv_gfxf_map), [`zxv_m7_draw_map`](#zxv_m7_draw_map),
the [`zxv_mapmove.h`](libraries.md#moving-through-a-tile-map-zxv_mapmoveh)
library.

### `zxv_map_cell`

```c
uint16_t zxv_map_cell(const zxv_tilemap *m, int px, int py);
```

The map cell on which pixel (px, py) of the map falls: for collisions
with the scenery. It takes the map's repetition (`ZXV_MAP_WRAP`) into
account, also towards negative values.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `m` | a map | The map. |
| `px`, `py` | any integer | The pixel, in map coordinates (those of `mx` and `my` in `zxv_map_draw`). |

**Returns:** the cell, with its tile number (`& ZXV_CELL_TILE`) and its
attributes; 0 outside a map that does not repeat. As 0 is also tile 0
without attributes, it is best to make tile 0 the empty one.

**Out of range**

| Case | What the ROM does |
|---|---|
| `m` is `NULL`, its `w` or its `h` is 0, or `size` is neither 8 nor 16 | Returns 0. |

**Example**

```c
/* Is the player (16 x 16, at px, py of the map) standing on a ground tile (tile 1)? */
uint16_t c = zxv_map_cell(&map, px + 8, py + 16);
if ((c & ZXV_CELL_TILE) == 1)
    vy = 0;
```

**Since:** specification 0.1 · API function no. 40.

**See also:** [`zxv_map_draw`](#zxv_map_draw),
[`zxv_spr_collide`](#zxv_spr_collide).

### Mode 7

**Mode 7** draws a plane, a texture or a tile map, with its own
transformation on each line of the target: rotated, scaled or in
perspective, as on the SNES. Each line (`zxv_m7line`) holds the position
on the plane (u, v) of pixel x = 0 of the target and the step (du, dv) per
pixel, in 16.16 fixed point (65536 = one texel or one map pixel): pixel x
of the line shows the point (u + x·du, v + x·dv), taking the integer part
of each coordinate. `zxv_m7_camera` and `zxv_m7_affine` work out those
lines; they can also be written by hand. See
[Graphics: mode 7](graphics.md#mode-7).

With a texture it costs about 15 cycles per pixel (the whole mode 0
screen, a third of the frame); with a map, about 36 according to the
specification (about 33 have been measured on the board). In mode 1 at
full screen it does not fit in a frame: draw only the floor.

### `zxv_m7_draw`

```c
void zxv_m7_draw(const zxv_m7tex *t, const zxv_m7line *l, int y, int n);
```

Draws `n` lines of a texture from line `y` of the target: line `y + i`
uses `l[i]`. Each line covers the whole width of the clip rectangle.
Coordinates are taken as **unsigned**: negative values fall outside the
texture. If the texture repeats, everything falls inside (negative values
too, which wrap round); if not, the outside is colour `out`. With
`ZXV_M7_TRANSPARENT` in `flags`, texels of colour 8 are not drawn (nor is
the outside if `out` is 8).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `t` | a texture | Its `texels` (one byte per texel, 0–15, row by row), its `log2size` (1–10: from 2 × 2 to 1024 × 1024), `wrap`, `out` (0–15) and `flags`. Up to 256 × 256 (64 KB) fits in the user RAM. |
| `l` | `n` lines | The transformation of each line. |
| `y` | any integer | The first line of the target. Those that fall outside the clip rectangle are not drawn. |
| `n` | 1 or more | How many lines. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `t` or `l` is `NULL`, or `n` is 0 or less | Nothing. |
| `log2size` outside 1–10 | Nothing. |
| A texel or `out` above 15 | Its 4 low bits. |
| `flags` with `ZXV_M7_TRANSPARENT` on a ROM of specification 0.1 | The field was reserved and had to be left at 0: do not use it without first checking [`zxv_api_version`](#zxv_api_version) ≥ 2. |

**Example**

```c
static uint8_t texels[64 * 64];                 /* a 64 x 64 board */
static zxv_m7line rows[192];
for (int v = 0; v < 64; v++)
    for (int u = 0; u < 64; u++)
        texels[v * 64 + u] = ((u / 8) ^ (v / 8)) & 1 ? 12 : 4;
const zxv_m7tex tex = { texels, 6, 1, 0, 0 };   /* 2^6 = 64, repeats */
for (int i = 0; i < 192; i++)                   /* each line, a little further away */
    rows[i] = (zxv_m7line){ 0, i << 16, 0x4000 + i * 256, 0 };
zxv_m7_draw(&tex, rows, 0, 192);
```

**Since:** specification 0.1 · API function no. 41 (`ZXV_M7_TRANSPARENT`,
since specification 0.2).

**See also:** [`zxv_m7_camera`](#zxv_m7_camera),
[`zxv_m7_affine`](#zxv_m7_affine), [`zxv_m7_draw_map`](#zxv_m7_draw_map),
[`zxv_gfxf_texture`](#zxv_gfxf_texture),
[`FLOOR`](../basic/reference.md#floor) in BASIC.

### `zxv_m7_draw_map`

```c
void zxv_m7_draw_map(const zxv_tilemap *m, const zxv_m7line *l, int y, int n, int out);
```

The same as `zxv_m7_draw`, with a **tile map** as the plane, as on the
SNES: point (u, v) of the plane is that pixel of the map, with the
mirroring of each cell and its transparent 8 (`ZXV_CELL_TRANSP`), just as
`zxv_map_draw` would draw it. Here u and v are **signed**: below 0 is
outside the map, or the other side if the map repeats (`ZXV_MAP_WRAP`, in
both directions, with any size). The outside of a map that does not
repeat is colour `out`, or is not drawn. Cell priority does not count. A
1024 × 1024 pixel track is a map of 128 × 128 cells (32 KB) plus its
tiles, instead of the megabyte it would take as a texture.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `m` | a map | The plane, with `cells` and `tiles`, `size` 8 or 16 and `w` and `h` of 1 or more. |
| `l` | `n` lines | The transformation of each line. |
| `y` | any integer | The first line of the target. |
| `n` | 1 or more | How many lines. |
| `out` | 0–15, or `ZXV_CLEAR` | The colour of the outside of a map that does not repeat; `ZXV_CLEAR`, it is not drawn. |

**Out of range**

| Case | What the ROM does |
|---|---|
| `m` or `l` is `NULL`, `cells` or `tiles` is `NULL`, `w` or `h` is 0, `size` is neither 8 nor 16, or `n` is 0 or less | Nothing. |
| `out` negative or greater than 15 | Like `ZXV_CLEAR`: the outside is not drawn. |
| With a ROM of specification 0.1 | The function does not exist: it does nothing. |

**Example**

```c
/* map, from zxv_map_draw; rows, from zxv_m7_camera */
zxv_m7_draw_map(&map, rows + 72, 72, 120, ZXV_CLEAR);
```

**Since:** specification 0.2 · API function no. 57.

**See also:** [`zxv_m7_draw`](#zxv_m7_draw),
[`zxv_map_draw`](#zxv_map_draw).

### `zxv_m7_camera`

```c
void zxv_m7_camera(zxv_m7line *l, int n, const zxv_m7cam *c);
```

Fills `l[0]` to `l[n − 1]` with the lines of a **floor in perspective**
seen from a camera, for lines 0 to n − 1 of the target. Line *i* below
the horizon is at distance height × focal / (i − horizon), looking in
direction `angle` (0 towards growing u, 0x4000 towards growing v), with
the centre of the target at the camera's position. Lines from the horizon
upwards are set to zero. Draw with `zxv_m7_draw(t, l + y0, y0, n)` (or
`zxv_m7_draw_map`) from the first line of the floor, `y0`. The centre is
half the width of the current target (the whole target, not the clip
rectangle).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `l` | `n` lines | Where they are written. |
| `n` | 1 or more | How many lines, from line 0 of the target. |
| `c` | a camera | `x`, `y` (position on the plane), `height` and `focal` (greater than 0), in 16.16 fixed point; `angle` in turns of 0x10000; `horizon`, the target line of the horizon (integer, can be negative). |

**Out of range**

| Case | What the ROM does |
|---|---|
| `l` or `c` is `NULL`, `n` is 0 or less, or `focal` is 0 or less | Writes nothing. |
| The target has nothing visible (an empty clip rectangle) | Takes 256 as the width. |

**Example**

```c
/* tex, the texture from zxv_m7_draw */
static zxv_m7line rows[192];
zxv_m7cam cam = {
    .x = 32 << 16, .y = 32 << 16,               /* 16.16 fixed point */
    .height = 16 << 16,
    .angle = 0x2000,                            /* an eighth of a turn */
    .focal = 128 << 16,
    .horizon = 64,                              /* line of the horizon */
};
zxv_gfx_rect(0, 0, 256, 72, 9);                 /* the sky */
zxv_m7_camera(rows, 192, &cam);                 /* lines 0..64 are set to zero */
zxv_m7_draw(&tex, rows + 72, 72, 120);          /* the floor: lines 72..191 */
```

**Since:** specification 0.1 · API function no. 42.

**See also:** [`zxv_m7_draw`](#zxv_m7_draw),
[`zxv_m7_affine`](#zxv_m7_affine), the
[`zxv_m7.h`](libraries.md#objects-on-the-mode-7-floor-zxv_m7h) library
(objects on the floor).

### `zxv_m7_affine`

```c
void zxv_m7_affine(zxv_m7line *l, int n, const zxv_m7aff *a);
```

Fills `l[0]` to `l[n − 1]` for a **rotated and scaled** plane, like the
SNES matrix, for lines 0 to n − 1 of the target: point (x, y) of the
plane appears at (cx, cy) on the target, rotated `angle` clockwise and
scaled by `zoom_x` and `zoom_y` along the axes of the plane. That is,
pixel (X, Y) shows (x, y) + Z⁻¹·R(−angle)·(X − cx, Y − cy). It uses the
same sine as `zxv_m7_camera`. It is useful for rotating backgrounds, huge
bosses or a map that zooms in.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `l` | `n` lines | Where they are written. |
| `n` | 1 or more | How many lines, from line 0 of the target. |
| `a` | a plane | `x`, `y` (the point at the centre, 16.16); `angle` (turns of 0x10000, clockwise); `zoom_x`, `zoom_y` (greater than 0; 0x10000 is its own size and 0x20000, double); `cx`, `cy` (the centre on the target, integers). |

**Out of range**

| Case | What the ROM does |
|---|---|
| `l` or `a` is `NULL`, `n` is 0 or less, or `zoom_x` or `zoom_y` is 0 or less | Writes nothing. |
| With a ROM of specification 0.1 | The function does not exist: it writes nothing. |

**Example**

```c
/* tex, the 64 x 64 texture from zxv_m7_draw; rows, 192 lines */
zxv_m7aff a = { .x = 32 << 16, .y = 32 << 16,  /* the centre of the texture */
                .angle = frame * 256, .zoom_x = 0x20000, .zoom_y = 0x20000,
                .cx = 128, .cy = 96 };
zxv_m7_affine(rows, 192, &a);
zxv_m7_draw(&tex, rows, 0, 192);
```

**Since:** specification 0.2 · API function no. 58.

**See also:** [`zxv_m7_camera`](#zxv_m7_camera),
[`zxv_m7_draw`](#zxv_m7_draw).

## `.gfx` graphics files

Declared in `zxv_gfxfile.h`. They read `.gfx` files (those from
`png2gfx.py`, `zxv-gfx`, the editor on the machine and BASIC's
`LOAD "x" GRAPHICS`) already in memory. They are not in the ROM: they are
built from `gfxfile.c`, which goes inside the program (add it to its
sources), so they work with any ROM. The explanations and a complete
example are in [Graphics](graphics.md#reading-a-gfx-file-in-c).

```c
typedef struct {
    const uint8_t *data;
    uint32_t size;
} zxv_gfxfile;
```

The file can be anywhere: read with `zxv_file_load` or inside the
program. The objects in the file are not aligned, so each function copies
its object into `buf`, which must be aligned to 4, and **returns the bytes
`buf` needs**, whether it copied them or not. If `buf` is `NULL` or `room`
is smaller, it copies nothing: that is how you ask for the size. If the
object is not there, or `zxv_gfxf_open` did not accept the file, it
returns 0 (or `NULL`). None of them calls the ROM.

### `zxv_gfxf_open`

```c
int zxv_gfxf_open(zxv_gfxfile *f, const void *data, uint32_t size);
```

Checks that `data` is a whole, well-formed `.gfx` file and leaves `f`
ready for the other functions. It looks at the signature, the version and
all the objects: that they fit in `size` and that each one has the length
its dimensions say. Objects of kinds it does not know are skipped. The
file is not copied: it must stay in `data` while `f` is in use.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | a `zxv_gfxfile` | What gets filled in. |
| `data` | the file in memory | From its first byte (`ZXVG`). |
| `size` | bytes | How big it is. |

**Returns:** 1 if it accepts it; 0 if not, and then `f` is left empty (all
the other functions give 0 or `NULL` with it).

**When it returns 0**

| Case | Why |
|---|---|
| `size` less than 8, no `ZXVG` at the start or a version other than 1 | It is not a `.gfx` it understands. |
| An object goes beyond `size` | The file is cut short (it was not read whole: check the `max` of `zxv_file_load`). |
| A sprite with a side of 0 or more than 1024, tiles that are neither 8 nor 16 or with none, an empty map or one with tiles that are neither 8 nor 16, a texture with `log2size` outside 1–10, a screen with a mode, border or flags that are not valid, or any of them with a length that does not match its dimensions | The object is not valid. |

**Example**

```c
static uint8_t data[16 * 1024];                 /* the whole .gfx file */
zxv_gfxfile f;
int n = zxv_file_load("art.gfx", data, sizeof data);
if (n < 0 || !zxv_gfxf_open(&f, data, (uint32_t)n))
    zxv_report_error(n < 0 ? n : 0, "Cannot read art.gfx");
```

**See also:** [`zxv_file_load`](#zxv_file_load),
[The `.gfx` format](#the-gfx-format).

### `zxv_gfxf_find`

```c
const uint8_t *zxv_gfxf_find(const zxv_gfxfile *f, int kind, int num, uint32_t *len);
```

An object's data as it is in the file, neither aligned nor copied, to read
its dimensions or an object of a kind that has no function.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | an open file | The file. |
| `kind` | `ZXV_GFXF_SPRITE` ... `ZXV_GFXF_SCREEN` (table below) | The kind of object. |
| `num` | 0–65535 | Its number (for tiles, the side: 8 or 16). |
| `len` | a pointer, or `NULL` | If not `NULL`, it receives the length of the data. |

**Returns:** a pointer to the object's data (without the 8-byte header),
or `NULL` if it is not there.

**Example**

```c
uint32_t len;
const uint8_t *d = zxv_gfxf_find(&f, ZXV_GFXF_SPRITE, 3, &len);
int w = d ? d[0] | d[1] << 8 : 0;               /* the width of sprite 3 */
```

**See also:** [The `.gfx` format](#the-gfx-format).

### `zxv_gfxf_sprite`

```c
uint32_t zxv_gfxf_sprite(const zxv_gfxfile *f, int num, zxv_bitmap *b, void *buf, uint32_t room);
```

Sprite `num` as a bitmap: `b` receives its dimensions and points to `buf`,
where its pixels are copied with lines of a multiple of 4 bytes (the
padding on the right, transparent: colour 8). Ready for
`zxv_spr_prepare`, `zxv_gfx_blit` or `zxv_gfx_target`. `b` is filled in
even if nothing is copied (with `pix` = `buf`, which can be `NULL`).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | an open file | The file. |
| `num` | 0–255 (those of `png2gfx.py` and BASIC) | The sprite number. |
| `b` | a bitmap | What gets filled in. |
| `buf` | memory aligned to 4, or `NULL` | Where the pixels are copied. |
| `room` | bytes | The room there is in `buf`. |

**Returns:** the bytes `buf` needs: ⌈⌈width / 2⌉ / 4⌉ × 4 × height; 0 if
the sprite is not there (and then `b` is not touched).

**Example**

```c
static uint32_t pix[4096];                      /* up to 16 KB of pixels */
static uint32_t face[1024];                     /* the prepared sprite */
zxv_bitmap b;
uint32_t need = zxv_gfxf_sprite(&f, 0, &b, pix, sizeof pix);
if (!need || need > sizeof pix || zxv_spr_prepare(&b, 0, 0, b.w, b.h, NULL) > sizeof face)
    zxv_report_error(0, "Sprite 0 does not fit");
zxv_spr_prepare(&b, 0, 0, b.w, b.h, face);
```

**See also:** [`zxv_spr_prepare`](#zxv_spr_prepare),
[`zxv_gfx_blit`](#zxv_gfx_blit).

### `zxv_gfxf_tiles`

```c
uint32_t zxv_gfxf_tiles(const zxv_gfxfile *f, int size, void *buf, uint32_t room, int *count);
```

The tiles of side `size`, one after another, as `zxv_tilemap.tiles` wants
them.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | an open file | The file. |
| `size` | 8 or 16 | The side of the tiles. |
| `buf` | memory aligned to 4, or `NULL` | Where they are copied. |
| `room` | bytes | The room there is in `buf`. |
| `count` | a pointer, or `NULL` | If not `NULL`, it receives how many tiles there are (even if they are not copied). |

**Returns:** the bytes `buf` needs: 32 per 8 × 8 tile and 128 per 16 × 16
tile; 0 if the file has no tiles of that side (and then `count` is not
touched).

**Example**

```c
static uint32_t tiles[64 * 32 / 4];             /* up to 64 tiles of 8 x 8 */
int count;
uint32_t t = zxv_gfxf_tiles(&f, 8, tiles, sizeof tiles, &count);
if (!t || t > sizeof tiles)
    zxv_report_error(0, "The tiles do not fit");
```

**See also:** [`zxv_gfxf_map`](#zxv_gfxf_map).

### `zxv_gfxf_map`

```c
uint32_t zxv_gfxf_map(const zxv_gfxfile *f, int num, zxv_tilemap *m, const void *tiles,
                      void *buf, uint32_t room);
```

Map `num`: its cells are copied into `buf`, and `m` is left with them,
with its size, the side of its tiles, its repetition (`ZXV_MAP_WRAP`) and
`tiles`, the graphics of its tiles (those from `zxv_gfxf_tiles` of its
side). `m` is filled in even if nothing is copied.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | an open file | The file. |
| `num` | 0–15 (those of `png2gfx.py` and BASIC) | The map number. |
| `m` | a map | What gets filled in. |
| `tiles` | the tiles, aligned to 4 | Those of its side (the `size` of `m`); it is not checked that they are of that side or that they are all there. |
| `buf` | memory aligned to 4, or `NULL` | Where the cells are copied. |
| `room` | bytes | The room there is in `buf`. |

**Returns:** the bytes `buf` needs: 2 per cell; 0 if the map is not there
(and then `m` is not touched).

**Example**

```c
static uint16_t cells[1024];
zxv_tilemap level;
uint32_t c = zxv_gfxf_map(&f, 0, &level, tiles, cells, sizeof cells);
if (!c || c > sizeof cells)
    zxv_report_error(0, "The map does not fit");
zxv_map_draw(&level, 0, 0, 0, 0, 256, 192, ZXV_MAP_ALL);
```

**See also:** [`zxv_gfxf_tiles`](#zxv_gfxf_tiles),
[`zxv_map_draw`](#zxv_map_draw).

### `zxv_gfxf_texture`

```c
uint32_t zxv_gfxf_texture(const zxv_gfxfile *f, int num, zxv_m7tex *t, void *buf, uint32_t room);
```

Mode 7 texture `num`: its texels are copied into `buf`, and `t` is left
with them, its `log2size`, `wrap`, `out` and `flags`
(`ZXV_M7_TRANSPARENT`). `t` is filled in even if nothing is copied.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | an open file | The file. |
| `num` | 0–3 (those of `png2gfx.py` and BASIC) | The texture number. |
| `t` | a texture | What gets filled in. |
| `buf` | memory, or `NULL` | Where the texels are copied. |
| `room` | bytes | The room there is in `buf`. |

**Returns:** the bytes `buf` needs: side × side (one byte per texel); 0 if
the texture is not there (and then `t` is not touched).

**Example**

```c
static uint8_t texels[128 * 128];
zxv_m7tex floor_tex;
uint32_t n = zxv_gfxf_texture(&f, 0, &floor_tex, texels, sizeof texels);
if (!n || n > sizeof texels)
    zxv_report_error(0, "The texture does not fit");
```

**See also:** [`zxv_m7_draw`](#zxv_m7_draw).

### `zxv_gfxf_screen`

```c
uint32_t zxv_gfxf_screen(const zxv_gfxfile *f, int num, int *mode, int *border, void *buf, uint32_t room);
```

Screen `num` (those from `SAVE "x" SCREEN$`, `png2gfx.py --screen` and
loading screens): its pixels, like those of a VRAM page, are copied into
`buf`, and its mode and its border colour go to `*mode` and `*border`.
`buf` can be the page that is not shown: then the page is switched (and
the mode and the border set) at the VBLANK.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | an open file | The file. |
| `num` | 0–255 | The screen number. |
| `mode` | a pointer, or `NULL` | Receives its mode, 0 or 1. |
| `border` | a pointer, or `NULL` | Receives its border colour, 0–15. |
| `buf` | memory aligned to 4, or `NULL` | Where the pixels are copied. |
| `room` | bytes | The room there is in `buf`. |

**Returns:** the bytes `buf` needs: 24,576 in mode 0 and 98,304 in mode
1; 0 if the screen is not there (and then `*mode` and `*border` are not
touched).

**Example**

```c
int mode, border;
uint16_t pal[16];
if (!zxv_gfxf_screen(&f, 0, &mode, &border, NULL, 0))   /* first, its mode */
    zxv_report_error(0, "There is no screen 0");
zxv_set_mode(mode);
zxv_gfxf_screen(&f, 0, NULL, NULL, (void *)(uintptr_t)ZXV_VRAM1_BASE, ZXV_VRAM_PAGE_SIZE);
zxv_gfxf_screen_palette(&f, 0, pal);
zxv_wait_vblank();
for (int i = 0; i < 16; i++)
    ((volatile uint16_t *)(uintptr_t)ZXV_PALETTE(0))[i] = pal[i];
*(volatile uint8_t *)(uintptr_t)ZXV_V_BORDER = (uint8_t)border;
zxv_set_page(1);
```

**See also:** [`zxv_gfxf_screen_palette`](#zxv_gfxf_screen_palette),
[the loading screen](#the-loading-screen).

### `zxv_gfxf_screen_palette`

```c
int zxv_gfxf_screen_palette(const zxv_gfxfile *f, int num, uint16_t pal[16]);
```

The 16 RGB565 colours screen `num` was shown with, to put them in a
palette (`ZXV_PALETTE(t)`). A screen carries a palette if it was made with
colours other than the usual ones.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `f` | an open file | The file. |
| `num` | 0–255 | The screen number. |
| `pal` | 16 `uint16_t` | What gets filled in, always. |

**Returns:** 1 if the screen carries a palette (and `pal` is left with
it); 0 if it does not carry one or is not there, and then `pal` is left
with the usual colours (`ZXV_STD_RGB565`).

**Example:** the one of [`zxv_gfxf_screen`](#zxv_gfxf_screen).

**See also:** [Palette](#palette), [Video registers](#video-registers).

### `.gfx` file constants

| Constant | Value | Kind of object (`kind`) |
|---|---|---|
| `ZXV_GFXF_SPRITE` | 1 | Sprite |
| `ZXV_GFXF_TILES` | 2 | Tiles (its number is the side: 8 or 16) |
| `ZXV_GFXF_MAP` | 3 | Map |
| `ZXV_GFXF_TEXTURE` | 4 | Texture |
| `ZXV_GFXF_SCREEN` | 5 | Screen |

### The `.gfx` format

The 4 letters `ZXVG`, a version byte (1), a byte set to 0 and the number
of objects (16 bits). Each object has an 8-byte header (kind, 0, number in
16 bits and length of its data in 32 bits) followed by its data. All
numbers are little-endian.

| Kind | Object | Data |
|---|---|---|
| 1 | Sprite | Width and height (16 bits each, 1–1024) and its pixels: ⌈width / 2⌉ bytes per line |
| 2 | Tiles of S × S (S is the object's number: 8 or 16) | How many (16 bits, 1 or more), 0 (16 bits) and the tiles (32 or 128 bytes each) |
| 3 | Map | Width and height (16 bits each), tile side (8 or 16) and repetition (bytes), 0 (16 bits) and the cells (16 bits each) |
| 4 | Texture | `log2size` (1–10), repetition, outside colour and flags (bit 0: 8 is transparent), one byte each, and the texels (one byte each) |
| 5 | Screen | Mode (0 or 1), border colour, flags (bit 0: it has a palette) and 0, one byte each; if it has a palette, its 16 RGB565 colours (32 bytes); and the pixels of the page, as in VRAM (24,576 bytes in mode 0, 98,304 in mode 1) |

## Video registers

At `ZXV_VIDEO_REGS` (`0x2000_0400`). Write them at any time: the system
takes them just after each VBLANK and they show in the next frame.

| Register | Offset | Size | Contents |
|---|---|---|---|
| `ZXV_V_MODE` | 0x00 | 8 bits | Video mode, 0 or 1 (use `zxv_set_mode`, which also clears the drawing page) |
| `ZXV_V_PAGE` | 0x01 | 8 bits | Visible page, 0 or 1 (use `zxv_set_page`) |
| `ZXV_V_BORDER` | 0x02 | 8 bits | Border colour, 0–15 (not remapped; shown with the palette of the `ZXV_V_REMAP` table) |
| `ZXV_V_FLAGS` | 0x03 | 8 bits | `ZXV_VFLAG_LINETAB` (0x01): use the line table; `ZXV_VFLAG_BLANK` (0x02): show only the border |
| `ZXV_V_SCROLL_X` | 0x04 | 16 bits | Horizontal scroll of the picture, wrapping round |
| `ZXV_V_SCROLL_Y` | 0x06 | 16 bits | Vertical scroll, wrapping round |
| `ZXV_V_REMAP` | 0x08 | 8 bits | Colour remapping table in use, 0–3 |
| `ZXV_V_VSIZE` | 0x09 | 8 bits | Page size in mode 0 (see below). Every program starts with 0 |
| `ZXV_V_OUT_INFO` | 0x0C | 32 bits | Read only: the video output (see below) |

With scroll, screen pixel (x, y) shows page pixel
((x + SCROLL_X) mod width, (y + SCROLL_Y) mod height).

**The output** (`ZXV_V_OUT_INFO`). The user chooses it on the machine;
programs only read it:

| Bits | Constants | Contents |
|---|---|---|
| 0–3 | `ZXV_OUT_DVI` (1), `ZXV_OUT_HDMI` (2) | The kind of output: DVI, with no sound through the cable, or HDMI, with sound. Other values, reserved |
| 8–15 | | The refresh rate in Hz: always 60 |
| 16–17 | `ZXV_OUT_SCREEN_MASK` (`3 << 16`), `ZXV_OUT_SCREEN_SHIFT` (16) | The monitor, through the HDMI detection line: `ZXV_OUT_SCREEN_UNKNOWN` (`0 << 16`), not known (on a protoboard, or until one is connected); `ZXV_OUT_SCREEN_NONE` (`1 << 16`), it was connected and has been unplugged; `ZXV_OUT_SCREEN_ON` (`2 << 16`), connected. The emulator always says `ZXV_OUT_SCREEN_ON` |

```c
uint32_t o = *(volatile uint32_t *)(uintptr_t)ZXV_V_OUT_INFO;
int hdmi = (o & 15) == ZXV_OUT_HDMI;
int screen_on = (o & ZXV_OUT_SCREEN_MASK) == ZXV_OUT_SCREEN_ON;
```

**Page sizes** (`ZXV_V_VSIZE`, mode 0 only; ignored in mode 1). When a
`.zxv` is loaded, the ROM sets it to 0:

| Constant | Value | Page | Bytes per line |
|---|---|---|---|
| `ZXV_VSIZE_SCREEN` | 0 | 256 × 192 | 128 |
| `ZXV_VSIZE_2X2` | 1 | 512 × 384 | 256 |
| `ZXV_VSIZE_WIDE` | 2 | 1024 × 192 | 512 |
| `ZXV_VSIZE_TALL` | 3 | 256 × 768 | 128 |

With `ZXV_VSIZE_SCREEN` in mode 0, each page shows only its first 24 KB;
the 72 KB behind them (`ZXV_VRAM_HIDDEN`, `ZXV_VRAM_HIDDEN_SIZE`) are
memory for the program (`ZXV_VRAM0_DATA`, `ZXV_VRAM1_DATA`; see
[How the SDK lays out a program](#how-the-sdk-lays-out-a-program)), which
is lost on switching to mode 1 or to another page size.

**Remapping tables:** four tables of 16 bytes from `ZXV_REMAP_TABLES`
(table *t* at `ZXV_REMAP_TABLES + 16·t`). Entry *v* is the colour shown
for pixel value *v*. The ROM sets them to the identity when each program
starts.

**Palettes:** each table has its own, 16 RGB565 colours (u16) from
`ZXV_PALETTE(t)` (`ZXV_PALETTES + 32·t`, at `0x2000_0480`). A pixel of
value *v* on a line that uses table *t* is shown in colour
`palette[t][table[t][v]]`. They start with the usual colours
(`ZXV_STD_RGB565(i)`), and the ROM sets them again when each program
starts. Like the tables, they are read just after each VBLANK. To make
colours, `ZXV_RGB(r, g, b)` (see [Palette](#palette)).

**Line table:** at `ZXV_LINE_TABLE`, one entry of `ZXV_LINE_ENTRY_SIZE`
(8) bytes per line of the whole frame, border included, from the first
line of the top border: 240 lines in mode 0 (the picture starts at line
24) and 480 in mode 1 (the picture, from line 48):
`ZXV_FRAME_LINES(mode)` and `ZXV_BORDER_LINES(mode)`. The frame is
`ZXV_FRAME_W` × `ZXV_FRAME_H` (640 × 480), with a border of `ZXV_BORDER_X`
(64) pixels at each side and `ZXV_BORDER_Y` (48) lines above and below
(half that, in mode 0 pixels). It is only used with `ZXV_VFLAG_LINETAB`.

| Offset | Size | Field | Value for "as usual" |
|---|---|---|---|
| `ZXV_LT_BORDER` (0) | 8 bits | Border colour | `0xFF` (the one in `ZXV_V_BORDER`) |
| `ZXV_LT_REMAP` (1) | 8 bits | Remapping table | `0xFF` (the one in `ZXV_V_REMAP`) |
| `ZXV_LT_SCROLL_X` (2) | 16 bits | Horizontal scroll (0 to page width − 1) | `0xFFFF` (the one in `ZXV_V_SCROLL_X`) |
| `ZXV_LT_SRC_LINE` (4) | 16 bits | Page line shown (0 to page height − 1) | `0xFFFF` (= (y + SCROLL_Y) mod height) |
| 6 | 16 bits | Reserved, write 0 | |

On border lines only the border field counts.

**When changes are safe:** right after each VBLANK the system reads the
visible page, the registers and the tables. While bit
`ZXV_SYSF_LINK_BUSY` of `ZXV_SV_SYSFLAGS` is 1 (up to 1.5 ms in mode 0
and 6 ms in mode 1 or with a large page), do not write to the visible page
or the line table. The page that is not shown can be written at any time.

**Video memory:** `ZXV_VRAM0_BASE` (`0x2000_4000`) and `ZXV_VRAM1_BASE`
(`0x2001_C000`), `ZXV_VRAM_PAGE_SIZE` (96 KB) each. Mode 0 uses the first
24,576 bytes of each page with `ZXV_VSIZE_SCREEN`, and mode 1, all 98,304.
The picture sizes: `ZXV_MODE0_W` × `ZXV_MODE0_H` (256 × 192) and
`ZXV_MODE1_W` × `ZXV_MODE1_H` (512 × 384).

## Input registers

At `ZXV_INPUT_REGS` (`0x2000_0500`). The system writes them, except
`ZXV_I_KEY_TAIL`.

| Register | Offset | Size | Contents |
|---|---|---|---|
| `ZXV_I_KEYROWS` | 0x00 | 8 × 8 bits | The Spectrum keyboard matrix by half-rows: bits 0–4, 0 = pressed; bits 5–7 set to 1 |
| `ZXV_I_KEY_HEAD` | 0x08 | 8 bits | How many keys the system has written into the queue (a counter that wraps round) |
| `ZXV_I_KEY_TAIL` | 0x09 | 8 bits | How many the program has read (`zxv_key_get` advances it). If it equals `ZXV_I_KEY_HEAD`, the queue is empty; if not, the next key is at `ZXV_I_KEY_FIFO[TAIL mod 64]` |
| `ZXV_I_KEYMAP` | 0x10 | 32 bytes | 256 bits, one per HID key code; 1 = pressed |
| `ZXV_I_KEY_MODS` | 0x30 | 8 bits | Modifiers: bits 0–3 left Ctrl, Shift, Alt and GUI; bits 4–7, the same on the right |
| `ZXV_I_JOY0` to `ZXV_I_JOY3` | 0x40, 0x42, 0x44, 0x46 | 16 bits each | The gamepads of players 1 to 4 (the `ZXV_JOY_*` bits, as `zxv_joy_read`; `JOY2` and `JOY3` since specification 0.3, before at 0) |
| `ZXV_I_MOUSE_X`, `ZXV_I_MOUSE_Y` | 0x48, 0x4A | 16 bits signed each | Mouse movement counters, wrapping round (x to the right, y down) |
| `ZXV_I_MOUSE_BTN` | 0x4C | 8 bits | Bits 0–2: left, right and middle button |
| `ZXV_I_MOUSE_WHEEL` | 0x4D | 8 bits signed | Wheel counter (positive upwards) |
| `ZXV_I_KEY_FIFO` | 0x80 | 64 × 16 bits | The key queue (`ZXV_KEY_FIFO_LEN` = 64), what `zxv_key_get` reads |

**The mouse.** The system adds the mouse movement to the counters, with
no acceleration or limits: they are not a position. Keep the last reading
and look at how far they have moved, with 16- or 8-bit arithmetic so that
it does not matter when they wrap round. `zxv_mouse_update` does it for
you.

**HID codes** of common keys: A–Z 0x04–0x1D; 1–9 0x1E–0x26, 0 0x27;
Enter 0x28, Esc 0x29, Backspace 0x2A, Tab 0x2B, Space 0x2C; F1–F12
0x3A–0x45; Insert 0x49, Home 0x4A, Page Up 0x4B, Delete 0x4C, End 0x4D,
Page Down 0x4E; Right 0x4F, Left 0x50, Down 0x51, Up 0x52; left Ctrl,
Shift and Alt 0xE0–0xE2, the right ones 0xE4–0xE6. They identify the
position of the key on a US keyboard.

**The Spectrum matrix** (`ZXV_I_KEYROWS[n]`, bits 0 to 4):

| Half-row | Bit 0 | Bit 1 | Bit 2 | Bit 3 | Bit 4 |
|---|---|---|---|---|---|
| 0 | CAPS SHIFT | Z | X | C | V |
| 1 | A | S | D | F | G |
| 2 | Q | W | E | R | T |
| 3 | 1 | 2 | 3 | 4 | 5 |
| 4 | 0 | 9 | 8 | 7 | 6 |
| 5 | P | O | I | U | Y |
| 6 | ENTER | L | K | J | H |
| 7 | SPACE | SYMBOL SHIFT | M | N | B |

Shift is CAPS SHIFT and Ctrl is SYMBOL SHIFT; Backspace, Esc and the
cursor keys press their Spectrum combinations (CAPS SHIFT + 0, SPACE,
5–8), and punctuation signs press SYMBOL SHIFT with the right key.

## Sound registers

At `ZXV_SOUND_REGS` (`0x2000_0600`).

| Register | Offset | Size | Written by | Contents |
|---|---|---|---|---|
| `ZXV_S_BEEPER` | 0x00 | 8 bits | program | Bit 0: beeper level |
| `ZXV_S_AY_STEREO` | 0x01 | 8 bits | program | `ZXV_AY_STEREO_MONO` (0), `ZXV_AY_STEREO_ABC` (1, at start-up) or `ZXV_AY_STEREO_ACB` (2), for both chips |
| `ZXV_S_VOLUME` | 0x02 | 8 bits | program | Master volume 0–255 (200 at start-up) |
| `ZXV_S_AFLAGS` | 0x03 | 8 bits | program | Bit 0: mute |
| `ZXV_S_AY_HEAD` | 0x04 | 16 bits | program | Where the program writes in the AY queue (`ZXV_AY_FIFO`) |
| `ZXV_S_AY_TAIL` | 0x06 | 16 bits | system | Where the system reads |
| `ZXV_S_BEEP_HEAD` | 0x08 | 16 bits | program | Where the program writes in the timed queue (`ZXV_BEEP_QUEUE`) |
| `ZXV_S_BEEP_TAIL` | 0x0A | 16 bits | system | Where the system reads |
| `ZXV_S_SMP_VOLUME` | 0x0C | 8 bits | program | Volume of the sample channels, 0–255 (255 at start-up) |
| `ZXV_S_AY_REGS` | 0x10 | 16 × 8 bits | system | Read-only copy of the registers of the first AY, as already applied |
| `ZXV_S_AY2_REGS` | 0x20 | 16 × 8 bits | system | The same for the second AY |
| `ZXV_S_SMP_REGS` | 0x40 | 96 × 8 bits | system | Read-only copy of the registers of the 12 sample channels (8 each, in the order of `ZXV_SMP_SLOT`...; `ZXV_SMP_CTRL` without `KEY` and `STOP`) |
| `ZXV_S_SMP_SRC` | 0xA0 | 32 bits | program | An upload to the sound memory: from where (a RAM address) |
| `ZXV_S_SMP_DST` | 0xA4 | 32 bits | program | ... to where (an offset in the sound memory, from 0 to `ZXV_SMP_SOUND_MEM`) |
| `ZXV_S_SMP_LEN` | 0xA8 | 32 bits | program | ... and how many bytes (a multiple of 4) |
| `ZXV_S_SMP_UPLOAD` | 0xAC | 8 bits | both | 1 starts it; the system leaves 0 when it is done, or 0x80 if it could not |

The queues are easier to use through `zxv_ay_write`, `zxv_beeper_event`,
`zxv_ay_event` and `zxv_beeper_event_at`; the upload, through
`zxv_smp_upload` (see [Sample channels](#sample-channels)).

**The upload** copies within a few frames; the program must not touch the
source until `ZXV_S_SMP_UPLOAD` goes back to 0. With `ZXV_S_SMP_LEN` at 0
it finishes at once. It stays at 0x80, without copying anything, if
`ZXV_S_SMP_LEN` is not a multiple of 4, if it does not fit from
`ZXV_S_SMP_DST` in the sound memory, if the source is not in the
machine's RAM (from `ZXV_SRAM_BASE` to `ZXV_RESERVED_BASE`) or, on the
board, if the Pico ULA does not answer within a second (an older
firmware).

**The sound memory** belongs to the Pico ULA: programs only reach it
through the upload and the channel registers.

| Constant | Value | What it is |
|---|---|---|
| `ZXV_SMP_CHANNELS` | 12 | Sample channels |
| `ZXV_SMP_MEM` | 0x18000 (98,304) | Bytes for the samples, from offset 0 |
| `ZXV_SMP_TABLE` | 0x18000 | Where the sample table starts (after the samples) |
| `ZXV_SMP_SLOTS` | 256 | Entries in the sample table |
| `ZXV_SMP_SLOT_SIZE` | 12 | Bytes per entry |
| `ZXV_SMP_SOUND_MEM` | 0x18C00 (101,376) | The whole sound memory: samples and table |
| `ZXV_SMP_ADPCM_BLOCK` | 64 | Samples per IMA ADPCM block |
| `ZXV_SMP_ADPCM_BYTES` | 36 | Bytes per ADPCM block |
| `ZXV_SMP_KEYON` | 37 | Number for the queues: starts channels 0–7 (bits 0–7) at once, together with those of `ZXV_SMP_KEYON_HI` |
| `ZXV_SMP_KEYON_HI` | 36 | Number for the queues: channels 8–11 (bits 0–3) of the next `ZXV_SMP_KEYON` |
| `ZXV_AY_FIFO_LEN` | 256 | Entries in the AY queue (2 bytes each) |
| `ZXV_BEEP_QUEUE_LEN` | 256 | Entries in the timed queue (8 bytes each) |

`zxv_smp_define` writes the table entries and `zxv_smp_start`, the
`ZXV_SMP_KEYON`; the registers of each channel are in
[Sample channels](#sample-channels).

Stereo mixing: in ABC, left = A + half of B and right = C + half of B; in
ACB, left = A + half of C and right = B + half of C; in mono, all three on
both sides. The beeper goes to both sides. The channels of the two chips
with the same letter are added together.

## System variables

At `ZXV_SYSVARS` (`0x2000_0700`), 32 bits each. Programs only read them,
unless stated otherwise.

| Variable | Offset | Contents |
|---|---|---|
| `ZXV_SV_FRAMES` | 0x00 | Frame counter, +1 at every VBLANK (60 per second; not while interrupts are off) |
| `ZXV_SV_VERSION` | 0x04 | Version of the specification (`major << 16 \| minor`): 3, version 0.3, with this ROM (`ZXV_SPEC_VERSION`) |
| `ZXV_SV_FRAME_HOOK` | 0x08 | The current frame hook (change it with `zxv_set_frame_hook`) |
| `ZXV_SV_RAMTOP` | 0x0C | Top of the usable RAM; the system stack starts here (`ZXV_URAM_END`) |
| `ZXV_SV_URAM_START`, `ZXV_SV_URAM_END` | 0x10, 0x14 | Start and end (excluded) of the user RAM |
| `ZXV_SV_CPU_HZ` | 0x18 | Processor frequency (`ZXV_CPU_HZ_REF`, 144 MHz) |
| `ZXV_SV_SYSFLAGS` | 0x1C | `ZXV_SYSF_LINK_BUSY` (bit 0): the visible page is being sent; `ZXV_SYSF_BREAK` (bit 1): the BREAK key is pressed |
| `ZXV_SV_LANG` | 0x20 | The machine's language, `ZXV_LANG(a, b)` (use `zxv_lang_get` and `zxv_lang_set`) |
| `ZXV_SV_EDITOR` | 0x24 | 1 while BASIC waits for a command (in the editor or with a report on the screen); 0 while it runs something, native programs too. Written by the ROM; the web games page looks at it to start them and restart them |
| `ZXV_SV_PREFS` | 0x28 | The machine's preferences, which the user changes and which are kept when it is switched off (see below). Changed by the ROM |
| `ZXV_SV_PROG_FILE` | 0x2C | The file of the running `.zxv` program, kept open while it runs if it has [banks](#banks): its number + 1 (0 if not). Written by the loader. Since specification 0.2 |
| `ZXV_SV_PROG_CRC` | 0x30 | The CRC of the running program's image, the one in its header (0 with no program). Written by the loader. Since specification 0.2 |

**The preferences** (`ZXV_SV_PREFS`):

| Constant | Value | What it is |
|---|---|---|
| `ZXV_PREF_NO_CLICK` | 0x01 | The keys of the BASIC editor (and of `INPUT`, of the EDIT menu and of the waits for a key) do not click. It is changed in the editor's EDIT menu. Programs that read the keyboard never click |
| `ZXV_PREFS_MASK` | 0x0F | The bits that exist; the others are always 0. Bits 1–3 are reserved (0) |

The ROM reads them from the platform on a cold start and passes them to
it when they change; with an older firmware, they start at 0 every time
the machine is switched on.

## Error codes

The API functions that can fail return a negative number. `ZXV_OK` (0)
means everything went well; some also return a positive number (a size, a
file number).

| Code | Constant | Meaning |
|---|---|---|
| 0 | `ZXV_OK` | Success |
| −1 | `ZXV_E_NOENT` | It does not exist: the file, the folder, the program or the bank |
| −2 | `ZXV_E_EXIST` | It already exists (when creating a folder or renaming over something else) |
| −3 | `ZXV_E_NAME` | Name or path not valid |
| −4 | `ZXV_E_NOTDIR` | Part of the path is not a folder |
| −5 | `ZXV_E_ISDIR` | It is a folder (and a file was expected) |
| −6 | `ZXV_E_NOTEMPTY` | The folder is not empty |
| −7 | `ZXV_E_BADF` | File or folder number not valid (it is not open) |
| −8 | `ZXV_E_MFILE` | Too many open (`ZXV_MAX_FILES` files or `ZXV_MAX_DIRS` folders per drive) |
| −9 | `ZXV_E_ACCESS` | Not allowed: by the open mode, on `ROM:`, deleting the root, a file that is already open for writing, or `zxv_exec` from a native program |
| −10 | `ZXV_E_FAULT` | A buffer is outside the allowed memory |
| −11 | `ZXV_E_IO` | Read or write error |
| −12 | `ZXV_E_NOCARD` | No SD card (or the platform has no files) |
| −13 | `ZXV_E_NOSPC` | The card is full |
| −14 | `ZXV_E_FORMAT` | Not a valid `.zxv`: signature, header, loading screen or CRC |
| −15 | `ZXV_E_RANGE` | It does not fit: the program in the user RAM, or the file or the bank in the buffer (`zxv_file_load`, `zxv_dir_current`, `zxv_bank_load`), or an upload to the sound memory (`zxv_smp_upload`) |
| −16 | `ZXV_E_NOCLOCK` | The clock is not set, or the machine has no clock |
| −17 | `ZXV_E_INVAL` | Value not valid: a date that does not exist (`zxv_clock_set`) or a language that is not valid (`zxv_lang_set`) |
| −18 | `ZXV_E_OFFLINE` | The `EXT:` drive has no server, or it has stopped answering |
| −19 | `ZXV_E_CHANGED` | The program's file changed while it ran: its [banks](#banks) are no longer those of the running program; or, in `zxv_exec`, while it was being loaded |

If the card is taken out, or stops answering, while files or folders are
open, they keep their numbers (a file opened afterwards does not get one
of them) and every call on them returns `ZXV_E_NOCARD` or `ZXV_E_IO` until
you close them. Closing them returns `ZXV_OK`: what was already written is
on the card.

With the C library, these codes arrive in `errno` (see
[picolibc](#the-standard-c-library-picolibc)).

## API function numbers

The API is a table of 256 function addresses at `ZXV_API_TABLE`
(`0x2000_0000`). Entry *n* is at `ZXV_API_TABLE + 4·n`. From C, the
headers' functions use the macro `ZXV_API_FN(n, type)`, which reads the
entry and casts it to a function pointer of that type. In assembly:

```asm
    lui  t0, %hi(ZXV_API_TABLE)
    lw   t0, 4*ZXV_API_CLS(t0)
    jalr t0
```

| n | Constant | Function | Since |
|---|---|---|---|
| 0 | `ZXV_API_VERSION` | `zxv_api_version` | 0.1 |
| 1 | `ZXV_API_WAIT_VBLANK` | `zxv_wait_vblank` | 0.1 |
| 2 | `ZXV_API_SET_FRAME_HOOK` | `zxv_set_frame_hook` | 0.1 |
| 3 | `ZXV_API_TIME_US` | `zxv_time_us` | 0.1 |
| 4 | `ZXV_API_SET_MODE` | `zxv_set_mode` | 0.1 |
| 5 | `ZXV_API_SET_PAGE` | `zxv_set_page` | 0.1 |
| 6 | `ZXV_API_CLS` | `zxv_cls` | 0.1 |
| 7 | `ZXV_API_PLOT` | `zxv_plot` | 0.1 |
| 8 | `ZXV_API_PRINT_CHAR` | `zxv_print_char` | 0.1 |
| 9 | `ZXV_API_PRINT_STR` | `zxv_print_str` | 0.1 |
| 10 | `ZXV_API_KEY_GET` | `zxv_key_get` | 0.1 |
| 11 | `ZXV_API_JOY_READ` | `zxv_joy_read` | 0.1 |
| 12 | `ZXV_API_AY_WRITE` | `zxv_ay_write` | 0.1 |
| 13 | `ZXV_API_BEEPER_EVENT` | `zxv_beeper_event` | 0.1 |
| 14 | `ZXV_API_FILE_OPEN` | `zxv_file_open` | 0.1 |
| 15 | `ZXV_API_FILE_READ` | `zxv_file_read` | 0.1 |
| 16 | `ZXV_API_FILE_WRITE` | `zxv_file_write` | 0.1 |
| 17 | `ZXV_API_FILE_CLOSE` | `zxv_file_close` | 0.1 |
| 18 | `ZXV_API_FILE_LOAD` | `zxv_file_load` | 0.1 |
| 19 | `ZXV_API_EXIT_TO_BASIC` | `zxv_exit_to_basic` | 0.1 |
| 20 | `ZXV_API_REPORT_ERROR` | `zxv_report_error` | 0.1 |
| 21 | `ZXV_API_FILE_SEEK` | `zxv_file_seek` | 0.1 |
| 22 | `ZXV_API_FILE_STAT` | `zxv_file_stat` | 0.1 |
| 23 | `ZXV_API_FILE_REMOVE` | `zxv_file_remove` | 0.1 |
| 24 | `ZXV_API_FILE_RENAME` | `zxv_file_rename` | 0.1 |
| 25 | `ZXV_API_DIR_MAKE` | `zxv_dir_make` | 0.1 |
| 26 | `ZXV_API_DIR_CHANGE` | `zxv_dir_change` | 0.1 |
| 27 | `ZXV_API_DIR_CURRENT` | `zxv_dir_current` | 0.1 |
| 28 | `ZXV_API_DIR_OPEN` | `zxv_dir_open` | 0.1 |
| 29 | `ZXV_API_DIR_READ` | `zxv_dir_read` | 0.1 |
| 30 | `ZXV_API_DIR_CLOSE` | `zxv_dir_close` | 0.1 |
| 31 | `ZXV_API_ZXV_EXEC` | `zxv_exec` | 0.1 |
| 32 | `ZXV_API_GFX_TARGET` | `zxv_gfx_target` | 0.1 |
| 33 | `ZXV_API_GFX_DRAW_PAGE` | `zxv_gfx_draw_page` | 0.1 |
| 34 | `ZXV_API_GFX_CLIP` | `zxv_gfx_clip` | 0.1 |
| 35 | `ZXV_API_GFX_BLIT` | `zxv_gfx_blit` | 0.1 |
| 36 | `ZXV_API_SPR_PREPARE` | `zxv_spr_prepare` | 0.1 |
| 37 | `ZXV_API_SPR_DRAW` | `zxv_spr_draw` | 0.1 |
| 38 | `ZXV_API_SPR_COLLIDE` | `zxv_spr_collide` | 0.1 |
| 39 | `ZXV_API_MAP_DRAW` | `zxv_map_draw` | 0.1 |
| 40 | `ZXV_API_MAP_CELL` | `zxv_map_cell` | 0.1 |
| 41 | `ZXV_API_M7_DRAW` | `zxv_m7_draw` | 0.1 |
| 42 | `ZXV_API_M7_CAMERA` | `zxv_m7_camera` | 0.1 |
| 43 | `ZXV_API_GFX_LINE` | `zxv_gfx_line` | 0.1 |
| 44 | `ZXV_API_GFX_RECT` | `zxv_gfx_rect` | 0.1 |
| 45 | `ZXV_API_GFX_TRI` | `zxv_gfx_tri` | 0.1 |
| 46 | `ZXV_API_GFX_POLY` | `zxv_gfx_poly` | 0.1 |
| 47 | `ZXV_API_GFX_CIRCLE` | `zxv_gfx_circle` | 0.1 |
| 48 | `ZXV_API_CLOCK_GET` | `zxv_clock_get` | 0.1 |
| 49 | `ZXV_API_CLOCK_SET` | `zxv_clock_set` | 0.1 |
| 50 | `ZXV_API_LANG_SET` | `zxv_lang_set` | 0.1 |
| 51 | `ZXV_API_AY_EVENT` | `zxv_ay_event` | 0.1 |
| 52 | `ZXV_API_SPR_DRAW_SCALED` | `zxv_spr_draw_scaled` | 0.1 |
| 53 | `ZXV_API_TEXT_COLORS` | `zxv_text_colors` | 0.1 |
| 54 | `ZXV_API_TEXT_AT` | `zxv_text_at` | 0.1 |
| 55 | `ZXV_API_BEEPER_EVENT_AT` | `zxv_beeper_event_at` | 0.1 |
| 56 | `ZXV_API_GFX_PATTERN` | `zxv_gfx_pattern` | 0.2 |
| 57 | `ZXV_API_M7_DRAW_MAP` | `zxv_m7_draw_map` | 0.2 |
| 58 | `ZXV_API_M7_AFFINE` | `zxv_m7_affine` | 0.2 |

Numbers 32 to 63 are reserved for graphics. Unused entries return −1 and
do nothing: a program that calls a function of specification 0.2 on a ROM
of 0.1 does not hang. On a platform without files, the file functions
return `ZXV_E_NOCARD`, and on one without a clock, the clock functions
return `ZXV_E_NOCLOCK`.

These have no number, because they do not call the ROM: `zxv_lang_get`
and the mouse functions (they read memory directly), the sample channel
functions (`zxv_smp_*`, which write the registers through `zxv_ay_write`
and the upload), the [bank](#banks) functions (in `zxvbank.c`, inside the
program) and the [`.gfx` file](#gfx-graphics-files) functions (in
`gfxfile.c`).

Because the table is in RAM, a program can replace an entry to intercept
a routine.

## Memory map

| From | To | Size | Contents | Access |
|---|---|---|---|---|
| `0x1000_0000` | `0x1003_FFFF` | 256 KB | Platform firmware | None |
| `0x1004_0000` | `0x100B_FFFF` | 512 KB | The ROM (`ZXV_ROM_BASE`): BASIC and the API | Read, execute |
| `0x1040_0000` | `0x1067_AFFF` | 2,540 KB | The `ROM:` drive (`ZXV_ROMDISK_BASE`, `ZXV_ROMDISK_SIZE`) | Through the file functions |
| `0x2000_0000` | `0x2000_3FFF` | 16 KB | System area (`ZXV_SYS_BASE`) | See below |
| `0x2000_4000` | `0x2001_BFFF` | 96 KB | Video page 0 (`ZXV_VRAM0_BASE`) | Read, write |
| `0x2001_C000` | `0x2003_3FFF` | 96 KB | Video page 1 (`ZXV_VRAM1_BASE`) | Read, write |
| `0x2003_4000` | `0x2006_FFFF` | 240 KB | User RAM (`ZXV_URAM_BASE` to `ZXV_URAM_END`) | Read, write, execute |
| `0x2007_0000` | `0x2008_1FFF` | 72 KB | Reserved for the system (`ZXV_RESERVED_BASE`) | **Forbidden** |

**System area:**

| Address | Constant | Contents |
|---|---|---|
| `0x2000_0000` | `ZXV_API_TABLE` | API table (256 addresses) |
| `0x2000_0400` | `ZXV_VIDEO_REGS` | [Video registers](#video-registers) |
| `0x2000_0480` | `ZXV_PALETTES` | Palettes of the remapping tables (4 × 16 RGB565 colours) |
| `0x2000_0500` | `ZXV_INPUT_REGS` | [Input registers](#input-registers) |
| `0x2000_0600` | `ZXV_SOUND_REGS` | [Sound registers](#sound-registers) |
| `0x2000_0700` | `ZXV_SYSVARS` | [System variables](#system-variables) |
| `0x2000_0800` | `ZXV_AY_FIFO` | AY queue (256 × 2 bytes) |
| `0x2000_0A00` | `ZXV_BEEP_QUEUE` | Timed queue (256 × 8 bytes) |
| `0x2000_1200` | `ZXV_REMAP_TABLES` | Colour remapping tables (4 × 16 bytes) |
| `0x2000_1240` | `ZXV_PLATFORM_IF` | The ROM's interface with the platform ([internal](#internal-definitions)) |
| `0x2000_1800` | `ZXV_LINE_TABLE` | Line table (480 × 8 bytes) |
| `0x2000_2700` | `ZXV_DBG_BOX` | BASIC debugger mailbox ([internal](#internal-definitions): used by the ROM and the emulator) |
| `0x2000_2A00` | `ZXV_ROM_WORKSPACE` | The ROM's workspace, up to `ZXV_ROM_WORKSPACE_END` (`0x2000_4000`, [internal](#internal-definitions)) |

The rest of the system area is for the ROM's internal use.

The processor is a 32-bit RISC-V (RV32IMAC with the Zba, Zbb, Zbs, Zbkb
and Zcb extensions, `-march=rv32imac_zicsr_zifencei_zba_zbb_zbs_zbkb_zcb
-mabi=ilp32`), little-endian. **Unaligned accesses** to 16- or 32-bit data
stop the program. Programs may turn interrupts on and off (bit 3 of
`mstatus`) and read `mcycle`/`minstret`; other control registers belong to
the system.

### How the SDK lays out a program

`zxv.ld` puts, from `0x2003_4000`: the code (starting with `crt0.S`), the
constants, the initialised variables, then the zeroed variables (BSS).
It defines these symbols:

| Symbol | Meaning |
|---|---|
| `__image_end` | End of what is stored in the `.zxv` |
| `__bss_start`, `__bss_end` | The zeroed variables (the system clears them before starting) |
| `__heap_start`, `__heap_end` | Free memory for picolibc's `malloc`: from the end of the BSS to 32 KB below the end of the user RAM |
| `__tls_base` | Thread variables (picolibc's `errno`) |
| `__vram0_start`, `__vram0_end`, `__vram1_start`, `__vram1_end` | What is declared with `ZXV_VRAM0_DATA` and `ZXV_VRAM1_DATA`: in the hidden VRAM of each page, from `ZXV_VRAMn_BASE + ZXV_VRAM_HIDDEN` (72 KB, `ZXV_VRAM_HIDDEN_SIZE`); `crt0.S` clears it |

What is declared with `ZXV_VRAM0_DATA` or `ZXV_VRAM1_DATA` (for example,
`ZXV_VRAM0_DATA static uint8_t heights[18 * 1024];`) does not go in the
file: it starts at zero and cannot have another initial value
(`elf2zxv.py` rejects it). It is lost if the program switches to mode 1 or
uses another page size (`ZXV_V_VSIZE`), which take up the whole page, and
a program that uses it cannot start in mode 1.

The stack is the system's: it starts near `ZXV_URAM_END` and grows down.

## The `.zxv` file format

A 72-byte header, followed by the loading screen, if there is one, and the
program image, which is copied to the load address; after it, if there are
any, the banks. All numbers are little-endian.

| Offset | Constant | Size | Field |
|---|---|---|---|
| 0x00 | `ZXV_H_MAGIC` | 4 bytes | `ZXV1` (`ZXV_HDR_MAGIC`, 0x3156585A) |
| 0x04 | `ZXV_H_HDR_SIZE` | 16 bits | Header size: 72 (`ZXV_HDR_SIZE`) |
| 0x06 | `ZXV_H_PROFILE` | 16 bits | Specification profile: 1 (`ZXV_HDR_PROFILE`) |
| 0x08 | `ZXV_H_LOAD` | 32 bits | Load address |
| 0x0C | `ZXV_H_ENTRY` | 32 bits | Entry point |
| 0x10 | `ZXV_H_IMAGE_SIZE` | 32 bits | Size of the image |
| 0x14 | `ZXV_H_BSS_SIZE` | 32 bits | Bytes cleared to zero after the image (including the regions of the code banks) |
| 0x18 | `ZXV_H_STACK` | 32 bits | Stack top (0 = keep the system's stack) |
| 0x1C | `ZXV_H_MODE` | 8 bits | Initial video mode (its bit 0 counts) |
| 0x1D | `ZXV_H_FLAGS` | 8 bits | Reserved: written as 0, ignored when loading |
| 0x1E | | 16 bits | Reserved |
| 0x20 | `ZXV_H_CRC` | 32 bits | CRC-32 of the image (the zlib one) |
| 0x24 | `ZXV_H_NAME` | 28 bytes | Name (ASCII, zero-terminated) |
| 0x40 | `ZXV_H_SCREEN` | 32 bits | Size of the loading screen, which comes after the header (0 = none) |
| 0x44 | | 32 bits | Reserved (0) |

When loading it, the ROM checks:

| Check | If not |
|---|---|
| The signature, the header size (72) and the profile (1) | `ZXV_E_FORMAT` |
| That the load address is aligned to 4 and the entry point is even and inside the image | `ZXV_E_FORMAT` |
| That the image and the BSS start in the user RAM and fit leaving 4 KB below the ROM's stack | `ZXV_E_RANGE` |
| That a stack top other than 0 is aligned to 16 and lies between the end of the BSS and `ZXV_URAM_END` | `ZXV_E_RANGE` |
| The loading screen, if there is one (below) | `ZXV_E_FORMAT` |
| That the whole image is read, and its CRC | `ZXV_E_FORMAT` |

Then it clears the BSS, sets the video mode (if the loading screen is not
already in that mode; if it is, the program starts with it on page 0),
sets `ZXV_V_VSIZE` to 0 and calls the entry point as `int entry(void)`.
Before loading, it leaves the graphics engine on page 0 with no clip
rectangle or pattern, and the colours and the text as at the start. A
`.zxv` with the old 64-byte header does not load (`ZXV_E_FORMAT`): run it
through `elf2zxv.py` again.

### The loading screen

It is the data of a screen object of a `.gfx`,
`ZXV_SCREEN_SIZE_F(mode, flags)` bytes long:

| Offset | Size | Field |
|---|---|---|
| 0 | 8 bits | Mode, 0 or 1 |
| 1 | 8 bits | Border colour, 0–15 |
| 2 | 8 bits | Flags: `ZXV_SCREEN_F_PALETTE` (0x01), it has a palette; the others, 0 |
| 3 | 8 bits | 0 |
| 4 | 32 bytes | With a palette, its 16 RGB565 colours (`ZXV_SCREEN_PALETTE`, 32) |
| 4 or 36 | 24,576 or 98,304 bytes | The pixels of the page, as in VRAM (`ZXV_SCREEN_PIXELS(mode)`) |

`ZXV_SCREEN_HEAD` (4) is the size of the screen's header;
`ZXV_SCREEN_SIZE(mode)`, that of a screen without a palette, and
`ZXV_SCREEN_SIZE_F(mode, flags)`, that of the whole screen. The ROM checks
that the mode, the border, the flags and the 0 are valid and that the size
in `ZXV_H_SCREEN` is exactly that; if not, `ZXV_E_FORMAT`. It puts it up
before loading the image, with its mode, its border (with no scroll or
remapping) and, if it has one, its palette in table 0.

### The banks

The [banks](#banks) go after the image, which the loader ignores (so older
ROMs load the program all the same). At the end of the file comes the
trailer, `ZXV_BANK_TAIL` (16) bytes long:

| Offset | Constant | Size | Field |
|---|---|---|---|
| 0 | `ZXV_BANK_MAGIC` | 8 bytes | `ZXVBANK1` |
| 8 | `ZXV_BANK_T_INDEX` | 32 bits | Where the index starts, from the start of the file |
| 12 | `ZXV_BANK_T_INDEX_LEN` | 32 bits | The length of the index |

The index: a header of `ZXV_BANK_HEAD` (16) bytes and an entry of
`ZXV_BANK_ENTRY` (48) bytes per bank.

| Offset | Constant | Size | Field |
|---|---|---|---|
| 0 | `ZXV_BANK_I_COUNT` | 32 bits | How many banks |
| 4 | `ZXV_BANK_I_PROG_CRC` | 32 bits | The CRC of the program's image (its fingerprint) |
| 8 | `ZXV_BANK_I_IMAGE_SIZE` | 32 bits | The size of the image |
| 12 | `ZXV_BANK_I_CRC` | 32 bits | The CRC of the entries |

Each entry:

| Offset | Constant | Size | Field |
|---|---|---|---|
| 0 | `ZXV_BANK_E_NAME` | 16 bytes | Name (ASCII, up to `ZXV_BANK_NAME_MAX`, 15, zero-terminated) |
| 16 | `ZXV_BANK_E_KIND` | 8 bits | `ZXV_BANK_DATA` (0) or `ZXV_BANK_CODE` (1) |
| 17 | `ZXV_BANK_E_METHOD` | 8 bits | `ZXV_BANK_STORED` (0) or `ZXV_BANK_DEFLATE` (1, raw deflate without a wrapper, RFC 1951) |
| 18 | `ZXV_BANK_E_REGION` | 8 bits | The region, 0 to 7 (code banks) |
| 20 | `ZXV_BANK_E_OFFSET` | 32 bits | Where it starts, from the start of the file |
| 24 | `ZXV_BANK_E_STORED` | 32 bits | Its size in the file |
| 28 | `ZXV_BANK_E_SIZE` | 32 bits | Its real size |
| 32 | `ZXV_BANK_E_CRC` | 32 bits | CRC-32 of the real data |
| 36 | `ZXV_BANK_E_ADDR` | 32 bits | Code banks, where they go: the start of their region (they carry their code, constants, data and zeroed BSS) |

The rest is reserved (0). If the `.zxv` ends in that trailer, the ROM
keeps its file open while the program runs (`ZXV_SV_PROG_FILE`).

## Tools

The Python ones are in `sdk/tools` and need only Python 3 (no other
packages). The C and C++ ones (`zxv-music`, `zxv-link`, `zxv-link-gui`,
`zxv-gfx` and `zxv-tracker`) come compiled in the emulator packages.
`elf2zxv.py`, `zxvbanks.py`, `png2gfx.py`, `mkweb.py`, `zxv-link` and the
editors speak Spanish if the system is in Spanish (by `LC_ALL`,
`LC_MESSAGES` or `LANG` or, on Windows, the user's language), and English
otherwise; `obj2c.py`, `fontconv.py` and `zxv-music`, only Spanish.

### `elf2zxv.py`

```sh
python3 sdk/tools/elf2zxv.py INPUT.elf OUTPUT.zxv [--name NAME] [--mode 0|1]
        [--stack ADDRESS] [--screen SCREEN] [--bank NAME=FILE ...] [--pack]
```

Converts a 32-bit RISC-V ELF, linked with `sdk/zxv.ld`, into a `.zxv`
program: the header, the loading screen if there is one, the image and,
after it, the banks. The SDK's `Makefile` calls it by itself.

| Option | Effect |
|---|---|
| `--name NAME` | Name in the header: up to 27 ASCII characters (others come out as `?`). By default, the output file name without the extension (what comes after the last `/`) |
| `--mode 0\|1` | Initial video mode (default 0) |
| `--stack ADDRESS` | Stack top, in decimal or `0x...`; default 0, the system's stack. It must meet the rules of the [format](#the-zxv-file-format) or the program does not load |
| `--screen SCREEN` | The loading screen: a PNG (256 × 192 or 512 × 384, or the whole frame with its border, 320 × 240 or 640 × 480, like `png2gfx.py --screen`) or a `.gfx` with a screen (the first one). The ROM puts it up before loading the program, which starts with it if it is in its mode |
| `--bank NAME=FILE` | A data bank: the contents of `FILE`, which the program loads with `zxv_bank_load("NAME", ...)`. It can be repeated. The name, 1 to 15 ASCII characters with no spaces, not repeated |
| `--pack` | Compresses each bank (deflate, level 9) if that makes it smaller |

It writes `OUTPUT.zxv` and prints the load address, the entry point and
the sizes of the image and the BSS, and of each bank. It takes the code
banks from the ELF itself (the `.zxvbank.REGION.NAME` sections left by
[`zxvbanks.py`](#zxvbankspy)). It stops, without writing anything, if the
ELF is not 32-bit RISC-V, if the program does not fit in the user RAM, if
the load address is not aligned to 4 or the entry point is not in the
image, if there are initialised data in the hidden VRAM, if the program
uses the hidden VRAM and `--mode` is 1, or if there are more than 4096
banks.

### `zxvbanks.py`

```sh
python3 sdk/tools/zxvbanks.py -o OUTPUT.zxv [--cc GCC] [--cflags "..."] [--sdk FOLDER]
        RESIDENT.c ... --code NAME[@REGION]=A.c,B.c ... [--bank NAME=FILE ...]
        [--pack] [--name NAME] [--mode 0|1] [--screen SCREEN]
```

Builds a program with **code banks**: functions that are not in memory
all at once, but are loaded from the `.zxv` itself when they are called
(see [Banks](#banks)). The resident part (what is always there) goes in
RAM as usual; each bank is compiled separately and linked in its region,
which is as big as its largest bank and goes after the BSS. The program
calls the functions of a bank like any other: a resident "bridge" loads
the bank if needed and jumps to the function.

| Option | Effect |
|---|---|
| `-o OUTPUT.zxv` | The program to write (required) |
| `RESIDENT.c ...` | The files of the resident part (`.c`, `.S`). The SDK's `crt0.S`, `zxvlib.c`, `zxvbank.c` and `zxvbankcode.c` are added by themselves |
| `--code NAME[@REGION]=A.c,B.c` | A code bank and its files. The name, 1 to 15 letters, digits or `_`; the region, 0 to 7 (0 by default). It can be repeated; at least one is needed (without code banks, use `elf2zxv.py`) |
| `--bank NAME=FILE` | A data bank, as in `elf2zxv.py`. It can be repeated |
| `--pack` | Compresses the banks, as in `elf2zxv.py` |
| `--name`, `--mode`, `--screen` | As in `elf2zxv.py` |
| `--cc GCC` | The compiler (`riscv64-unknown-elf-gcc` by default); `objcopy`, `nm`, `objdump` and `readelf`, those with the same prefix |
| `--cflags "..."` | The compiler options (by default, the SDK's: `-march=... -mabi=ilp32 -Os -g -Wall -Wextra -std=gnu11 -ffreestanding ...`), split as the system's command line does: on Windows, the `\` of paths stay and quotes group (`-I"C:\my stuff\include"`) |
| `--sdk FOLDER` | The SDK folder (by default, the tool's) |

It writes `OUTPUT.zxv`, `OUTPUT.elf` (for debugging) and, in
`OUTPUT.bancos/`, the intermediate files and the symbols for GDB
(`gdb/residente.elf` and `gdb/BANK.elf`, for
[`zxvbanks-gdb.py`](#zxvbanks-gdbpy)). It stops if two banks have the same
name or the same function, if a bank exports a variable (make it `static`
or put it in the resident part: it would be wiped when another bank of its
region is loaded), has thread variables or has variables in the hidden
VRAM (`ZXV_VRAM0_DATA`, `ZXV_VRAM1_DATA`: declare them in the resident
part, where they take no user RAM either). It warns if a bank takes the
address of one of its functions: that pointer is valid only while the bank
is loaded.

### `zxvbanks-gdb.py`

```sh
gdb-multiarch game.elf -x sdk/tools/zxvbanks-gdb.py -ex "target remote :1234"
```

A GDB script (or, once inside, `source sdk/tools/zxvbanks-gdb.py`) to
debug a program with code banks: the banks of a region are at the same
addresses, and GDB can only have one function per address. It uses
`game.bancos/gdb/residente.elf` instead of the whole ELF and, every time a
bank is loaded and every time the program stops, adds the symbols of the
loaded banks and removes those of the others. So `break FUNCTION` in a
bank stays pending until it is loaded and stops only in it, and `bt`,
`list` and `print` work with what is really there. The bridges are called
`FUNCTION.puente`. It adds the `zxvbanks` command, which shows the banks
and which ones are loaded. With a program without code banks it does
nothing.

### `png2gfx.py`

```sh
python3 sdk/tools/png2gfx.py OUTPUT.gfx COMMAND...
python3 sdk/tools/png2gfx.py --list FILE.gfx
```

Converts PNG images into a `.gfx` file (the one of `LOAD "x" GRAPHICS`
and of [`zxv_gfxfile.h`](#gfx-graphics-files)). The commands, with
examples, are in
[Graphics](graphics.md#images-from-png-files-png2gfxpy):

| Command | Numbers | Effect |
|---|---|---|
| `--sprite N IMAGE [X,Y,W,H]` | N: 0–255 | Sprite N is the image or that rectangle (1 to 1024 per side) |
| `--sprites N IMAGE W,H` | N: 0–255 | A sheet: sprites N, N+1... of W × H, row by row (the image, a multiple of W × H) |
| `--tiles S N IMAGE [X,Y,W,H]` | S: 8 or 16; N: 0–4095 | The image or the rectangle (a multiple of S) cut into tiles, numbered from N |
| `--level M S IMAGE` | M: 0–15 | A level: cut into tiles of S × S, reusing repeated ones (and, with `--flip`, mirrored ones), which are added to those already there; map M gets its layout, with the transparency bit in the cells of tiles with transparent pixels |
| `--map M S FILE.csv` | M: 0–15 | Map M from a CSV with the value of each cell (0–65535, as in `MAP POKE`), one row per line |
| `--texture N IMAGE [C]` | N: 0–3; C: 0–15 | Texture N: a square with a power-of-2 side, from 2 to 256. It repeats or, with C, the outside is colour C. Transparent parts become black |
| `--wrap M` | | Map M repeats |
| `--transparent N` | | The transparent parts of the PNG of texture N are colour 8 and are not drawn (with C = 8, neither is the outside) |
| `--screen N IMAGE [B]` | N: 0–255; B: 0–15 | Screen N: 256 × 192 (mode 0), 512 × 384 (mode 1), or the whole frame (320 × 240 or 640 × 480) with the border in the colour of its corner; B, the border colour. With colours other than the usual ones (16 at most), it carries its palette |
| `--flip` | | `--level` also reuses mirrored tiles |
| `--key RRGGBB` | | That colour is transparent, as well as pixels with alpha below 128 |

Each pixel becomes the nearest ZX-V colour (with a warning if it was not
exact). It reads any PNG except interlaced ones. It warns if a map uses a
tile that is not there. `--list` shows what is in a `.gfx`. It exits with
an error, without writing anything, if a command is not valid, or an image
cannot be read or does not have the right size.

### `obj2c.py`

```sh
python3 sdk/tools/obj2c.py MODEL.obj OUTPUT.c NAME [options]
python3 sdk/tools/obj2c.py MODEL.obj OUTPUT.bas NAME --bas [options]
```

Converts a Wavefront `.obj` model (the one from Blender and almost every
3D program) into C for [`zxv_3d.h`](libraries.md#simple-3d-zxv_3dh):
`const zxv_model NAME`, with the vertices as 16-bit integers and the faces
with their colours. Or, with `--bas`, into ZX Basic for `zxv3d.bas`. It is
explained, with examples, in
[Models from Blender](libraries.md#models-from-blender-obj2cpy).

| Option | Effect |
|---|---|
| `--size S` | The longest side of the model, in world units (2 by default) |
| `--keep-origin` | Does not centre it: the origin of the `.obj` stays as the model's |
| `--mat NAME=DARK,LIGHT[,two][,flat][,decal]` | The colours (0–15) of a material, and its options: `two`, seen from both sides; `flat`, without light; `decal`, stuck on top of another face and drawn after it. It can be repeated |
| `--default DARK,LIGHT` | The colours of faces without a material (1,9 by default: blue) |
| `--two-sided` | All faces are seen from both sides |
| `--bas` | Outputs ZX Basic: the arrays `NAME_v` and `NAME_f` and `SUB NAME_model(n)`, which sets it as model `n` of `MODEL3D` |

The colours of a face come from its material: one called
`zxv_DARK_LIGHT[_two][_flat][_decal]` gives those; if not, the diffuse
colour (`Kd`) from its `.mtl`, the closest of the 16 and its pair (bright
and not bright); if not, `--default`. Non-convex faces and faces with
more than 32 sides are split into convex pieces of up to 32 sides; faces
with no area are dropped. It warns if there are more than 512 vertices
(what `zxv_3d` accepts) or if a face crosses itself. It stops, with a
message saying what and where, if a face uses a vertex that does not
exist, if a vertex lacks its three numbers, if there are no vertices or
faces, if some colours are not valid, if an option has no value or if
`--size` is not a number greater than 0 or is too small. Comments (`#`)
may go at the end of a line.

### `fontconv.py`

```sh
python3 sdk/tools/fontconv.py INPUT OUTPUT.c NAME [options]
python3 sdk/tools/fontconv.py INPUT OUTPUT.txt [options]
```

Converts bitmap fonts into C for
[`zxv_font.h`](libraries.md#fonts-zxv_fonth) (`NAME_n`, `NAME_h`,
`NAME_codes[]` and `NAME_bits[]`), or into text to touch them up by hand
(if the output ends in `.txt`). It is explained in
[Fonts from elsewhere](libraries.md#fonts-from-elsewhere-fontconvpy).

| Input | Format |
|---|---|
| `.txt` | The ZX-V one (`rom/font/zxv8x8.txt`): `== CODE` and the rows, `#` ink and `.` background |
| `.ch8`, or 768 bytes | ZX Spectrum: 96 characters, from 32 to 127 (its 0x5E is ↑, 0x60 £ and 0x7F ©) |
| `.bin`, `.rom`, `.fnt`, others | A dump of 8 bytes per character (or `--height`): PC (CP437), MSX, Amstrad CPC... |
| `.psf` | PC Screen Font 1 or 2 (the Linux console ones), with its Unicode table if it has one |
| `.bdf` | Glyph Bitmap Distribution Format (X11) |
| `.hex` | Like GNU Unifont: `CODE:BYTES`; wide characters (the 16-column ones) are left out, with a warning |

| Option | Effect |
|---|---|
| `--format F` | `txt`, `zx`, `raw`, `psf`, `bdf` or `hex`, if the extension (or the contents) is not enough |
| `--first N` | `raw`: the code of the first character (0 by default; 32 if there are 96) |
| `--height H` | `raw`: the rows of each character (8 by default). `hex`: those of the 8-column ones (by default, those of the narrowest in the file; if all have more than 16 bytes, it must be given) |
| `--charset C` | `raw`: the Python character set used to turn codes above 127 into Unicode (`cp437` if there are 256 from 0; otherwise `latin-1`) |
| `--ascii` | `zx`: leaves `^`, `` ` `` and 127 as in ASCII |
| `--range R` | The characters that are kept, in hexadecimal: `20-7E,A0-FF` by default (ASCII and Latin-1, with accents, ñ, ¿ and ¡) |

Fonts must be up to 8 columns wide and 1 to 32 rows high, all the
characters of `--range` the same (those outside do not count). It stops if
more than 224 characters remain (what `zxv_font` holds; use `--range`) or
none, or if a dump goes past code 255 (more than 256 characters from
`--first`: usually a `--height` that is not the font's). A font's licence is still its own: being
able to convert it does not mean you may distribute it.

### `mkweb.py`

```sh
python3 sdk/tools/mkweb.py OUTPUT GAME [FILE...] --kit KIT [options]
python3 sdk/tools/mkweb.py OUTPUT --config game.json [--kit KIT] [options]
```

Prepares a game for the web: a page that has only that game, which starts
by itself and starts again when it ends, without BASIC being seen. To
upload it to itch.io or any server; it also works opened from disk. It is
explained, with the touch controls and `game.json`, in
[Publishing your game on the web](web.md).

| Argument or option | Effect |
|---|---|
| `OUTPUT` | The folder of the page. It is created; if it already exists, it is deleted entirely first |
| `GAME` | The program that starts: `.zvb`, `.zxv`, `.tap`, `.tzx` or a `.bas` listing (or `.txt`). If it is a `.zxv` or a `.zvb` with a loading screen, the page shows it while starting |
| `FILE...` | What the game needs (`.gfx`, `.pt3`, `.afb`...): it goes with it on the `ROM:` drive, where BASIC finds it by itself. Names of up to 63 printable ASCII characters, without `\ : * ? " < > \|`, not repeated |
| `--kit KIT` | The `play/` folder of the web version (`zxv-web.zip`, or `emulator/build-web/web/play`): it must have `index.html`, `play.js`, `play.css`, `engine.js` and `zxv-core.js`. Required |
| `--config game.json` | The options from a file (below) |
| `--title TEXT` | The name on the cover and the tab (by default, the game's) |
| `--id NAME` | The name of its card in the browser: letters, digits, `-` and `_`, up to 40 (by default, the game's). Changing it means starting with an empty card |
| `--lang auto\|es\|en` | The language of the machine and the page (`auto`, the browser's, by default) |
| `--border 0\|1\|2` | The border: full (0, the default), small or none |
| `--fit aspect\|integer\|fill` | How the picture is scaled (also `proporcion`, `entera` and `llenar`); `aspect` by default |
| `--touch PRESET\|JSON` | The touch controls: `joystick` (the default), `arrows`, `qaop` or `none`, or a JSON file or text |
| `--zip FILE.zip` | Also, the page in a `.zip` (with `index.html` at the root) |
| `--kit-url URL` | Does not copy the kit: the page uses the one at `URL` (relative to it, like `../kit/`). Only `index.html` and `game.js` go in the folder. Not compatible with `--zip` |

`game.json` holds the same: `start`, `files`, `title`, `id`, `lang`,
`border`, `fit`, `touch`, `kit` and `kit_url`, with paths relative to it;
the command-line options override its own. It writes in `OUTPUT` the kit
(with its licences), `game.js` (the game, the `ROM:` files and the
options) and, with `--kit-url`, only `index.html` and `game.js`. It exits
with code 1 and a message if something is not valid, and with 2 without
arguments (it shows the help).

### `zxv-music`

```sh
zxv-music [--free KB] [--channels LIST] INPUT OUTPUT.zxm
```

Converts a PT3, a TurboSound (Pro Tracker 3 and Vortex Tracker II), a
MOD, an S3M or an XM (up to 4 MB) to the own format, `.zxm`, and says
what it could not carry over and how much the samples take in the sound
memory. What it keeps of each format is in the
[guide](README.md#the-own-format-zxm-and-zxv-music).

| Option | Effect |
|---|---|
| `--free KB` | Leaves that many KB (0–96) of the sound memory free for other samples, such as the sounds of `SAMPLE` in BASIC: the song's samples that do not fit go in ADPCM, at half the samples or, if even that fails, are left out |
| `--channels LIST` | Of a MOD, S3M or XM, the channels to keep, up to 12, with the tracker's numbers, from 1 (`1-8,11,13`). Without it, of a module with more than 12 channels, the 12 that play the most notes are kept |

It exits with code 2 if the options are not valid (or `--channels` with a
PT3), and with 1 if it cannot read, convert or write.

### `zxv-link` and `zxv-link-gui`

```sh
zxv-link ports
zxv-link --port PORT info | send FILE [DEST] | run FILE [MORE...] | run --pc FILE | log
zxv-link --port PORT exec PATH | ls [PATH] | get PATH [FILE] | rm PATH | mv PATH NEW | mkdir PATH
zxv-link --port PORT serve [--folder FOLDER [--run PROGRAM]] [--basic-debug TCP_PORT] [FILE...]
zxv-link --port PORT [--yes] update zxv-update.bin
zxv-link --port MACHINE pair
```

Remote loading: they talk to the machine through the J5 header, with a
3.3 V USB-serial adapter, or over the network if the Pico CPU is a Pico 2
W. They send programs, run them, manage the files on the card, serve a
folder on the PC as the `EXT:` drive and update the firmware.
`zxv-link-gui` does the same with a window. How to use them is in
[Loading programs from the PC (J5)](../hardware/firmware.md#loading-programs-from-the-pc-j5)
and [over Wi-Fi](../hardware/firmware.md#loading-programs-over-wi-fi-pico-2-w).

| Command | Effect |
|---|---|
| `ports` | The serial ports there are and the ZX-Vs on the network that answer, one per line on standard output, as `--port` takes them. Those that answer with another version of the network protocol are left out (they cannot be talked to): they go to standard error, after "Not compatible:", with their firmware and what to update, the machine or `zxv-link` |
| `info` | The version of the firmware, the specification and the protocol, and whether the machine is waiting for a command or has a program running |
| `send FILE [DEST]` | Sends the file to the card (with its name, or `DEST`) |
| `run FILE [MORE...]` | Sends the file (and the ones it loads) to the card and runs it; if a program is running, it stops it first |
| `run --pc FILE` | Runs it straight into memory, without copying it to the card: the machine reads it from the PC through `EXT:`. If the program keeps it open (a `.zxv` with code banks), it keeps serving it until the program ends or Ctrl+C, and writes the line `zxv-link: serving-ext` to standard output |
| `exec PATH` | Runs what is already on the machine: `SD:/games/a.zxv`, `ROM:/b.bas` or `EXT:/c.zxv` |
| `ls [PATH]` | What is in a folder of the card (`SD:/` if none is given) or of `ROM:` |
| `get PATH [FILE]` | Copies a file from the card (or from `ROM:`) to the PC |
| `rm PATH` | Deletes a file or an empty folder |
| `mv PATH NEW` | Renames or moves, on the same drive |
| `mkdir PATH` | Creates a folder |
| `log` | The machine's log, until Ctrl+C |
| `serve ...` | Serves `FOLDER` as the `EXT:` drive until Ctrl+C (with `--run`, first runs `PROGRAM`, from inside the folder, from `EXT:`), and/or the BASIC debugger over TCP on `TCP_PORT` (used by the VS Code extension), after sending the `FILE`s. It needs `--folder` or `--basic-debug`; `--run` only with `--folder` and without `--basic-debug` |
| `update FILE` | Updates the machine's firmware with a `zxv-update.bin`, without a card: the machine reads it from the PC. It shows the versions first and asks |
| `pair` | Pairs this PC with a ZX-V on the network (with EDIT > Wi-Fi > Pair a PC open on it): both show a code and, if it matches, it is accepted on the machine |

| Option | Effect |
|---|---|
| `--port PORT` | The adapter (`COM5` on Windows, `/dev/ttyUSB0` on Linux) or a ZX-V on the network (its name, `ZX-V-1A2B`, or its IP). `zxv-link ports` lists them. Needed by every command except `ports` |
| `--overwrite` | Also replaces on the card what `zxv-link` did not put there, and lets the machine replace or delete in the `EXT:` folder what it did not write itself |
| `--yes` | `update` without asking |
| `--lang es\|en` | The language of the messages (by default, the system's) |

What is already on the card unchanged (the same size and the same CRC) is
not sent again. It exits with code 2 if the command is not valid (it shows
the help), with 3 if it cannot open the port, with 4 if it would have to
replace on the card something it did not put there (without
`--overwrite`), and with 1 if anything else fails.

### `zxv-gfx`

```sh
zxv-gfx [FILE.gfx]
```

The graphics editor for the PC: sprites, tiles, maps, textures and
screens in `.gfx` files. It is explained in
[Graphics](graphics.md#the-graphics-editor-for-the-pc-zxv-gfx). With
`--lang es` or `--lang en` it speaks that language (otherwise, the
system's); `--help` and `--version`, the usual. For the tests,
`--screenshot CAPTURE.png [--section N]` opens the file, saves the window
(with tab N, 0 to 5) and exits.

### `zxv-tracker`

```sh
zxv-tracker [FILE.zxm | .pt3 | .mod | .s3m | .xm | .afb]
zxv-tracker --wav OUTPUT.wav FILE
```

The tracker for the PC: `.zxm` songs with the two AYs and up to 12 sample
tracks, PT3 modules and AYFX effects (`.afb`); it also opens MOD, S3M and
XM. It is explained in the
[guide](README.md#the-tracker-for-the-pc-zxv-tracker).

| Option | Effect |
|---|---|
| `--wav OUTPUT.wav` | Plays the song once into a WAV, without a window, and exits |
| `--channels LIST` | Of a MOD, S3M or XM with more than 12 channels, the ones to keep (`1,2,5-9`, from 1, up to 12), without asking |
| `--lang es\|en` | The language (by default, the system's) |
| `--screenshot CAPTURE.png [--tab N]` | For the tests: opens, saves the window (with tab N, 0 to 5) and exits |

## The standard C library (picolibc)

Programs compiled with picolibc and `zxvc.c` (see the
[guide](README.md#two-ways-to-write-c)) can use:

| What | Functions |
|---|---|
| Files | `fopen`, `fread`, `fwrite`, `fgets`, `fprintf`, `fseek`, `fclose`..., and `open`, `read`, `write`, `lseek`, `close` |
| Folders | `opendir`, `readdir`, `rewinddir`, `closedir`, `mkdir`, `rmdir`, `chdir`, `getcwd` |
| Names | `rename`, `unlink`/`remove`, `stat`, `fstat` |
| Console | `printf`, `puts`, `getchar`, `fgets(..., stdin)`: the screen and the keyboard (with echo; Enter is `'\n'`) |
| Memory | `malloc`, `free`, `realloc` |
| Time | `time`, `gettimeofday`, `localtime`: the machine's clock, local time (1 January 2026 at 0:00 if it is not set) |
| Exit | `exit`, `abort`: back to BASIC (`exit(0)` gives `0 OK` and any other code a `W` report, as in [`zxv_exit_to_basic`](#zxv_exit_to_basic)) |

- API errors arrive in `errno`:

  | ZX-V code | `errno` |
  |---|---|
  | `ZXV_E_NOENT` | `ENOENT` |
  | `ZXV_E_EXIST` | `EEXIST` |
  | `ZXV_E_NAME`, `ZXV_E_INVAL` | `EINVAL` |
  | `ZXV_E_NOTDIR` | `ENOTDIR` |
  | `ZXV_E_ISDIR` | `EISDIR` |
  | `ZXV_E_NOTEMPTY` | `ENOTEMPTY` |
  | `ZXV_E_BADF` | `EBADF` |
  | `ZXV_E_MFILE` | `EMFILE` |
  | `ZXV_E_ACCESS` | `EACCES` |
  | `ZXV_E_FAULT` | `EFAULT` |
  | `ZXV_E_NOCARD` | `ENODEV` |
  | `ZXV_E_NOSPC` | `ENOSPC` |
  | `ZXV_E_FORMAT` | `ENOEXEC` |
  | `ZXV_E_RANGE` | `ERANGE` |
  | The rest (`ZXV_E_IO`, `ZXV_E_NOCLOCK`, `ZXV_E_OFFLINE`, `ZXV_E_CHANGED`) | `EIO` |

- Descriptors 0, 1 and 2 are the console; those of files are the API's
  plus 3.
- Folders open at once, as in the API: `ZXV_MAX_DIRS` (4). `rewinddir`
  closes the folder and opens it again on the next `readdir`.
- `struct dirent` has `d_name`, `d_type` (`DT_DIR` or `DT_REG`) and, as an
  extra, the size in `d_size`.
- `printf` with decimals adds about 7 KB; `-DPICOLIBC_INTEGER_PRINTF_SCANF`
  when linking uses the smaller one, integers only.
- The console speaks UTF-8, like C strings: `printf("¿Qué tal?")` shows
  the accents, and `fgets` gives the keys typed in UTF-8 (`ñ` is two
  bytes). Letters the ROM font does not have come out as `?`.
- The heap (`malloc`) goes from the end of the program to 32 KB below the
  end of the user RAM, where the stack is.

## Internal definitions

`zxv.h` is shared with the ROM, the firmware and the emulator, so it also
defines things programs must not use. They are listed here only so that
you recognise them; they may change without notice:

| What | Constants |
|---|---|
| The header guards, and the one the ROM uses to get the `zxv_gfx.h` types without the calls | `ZXV_H`, `ZXV_GFX_H`, `ZXV_GFXFILE_H`...; `ZXV_GFX_NO_API` |
| The ROM's interface with the platform (the firmware or the emulator) | `ZXV_PLATFORM_IF` and every `ZXV_PIF_...`, including the versions (`ZXV_PIF_VERSION_FS`, `_CLOCK`, `_LANG`, `_PREFS`, `_UPDATE`, `_WIFI`) |
| Updating the firmware from the card (the EDIT menu) | `ZXV_UPDATE_FILE` (`"zxv-update.bin"`), `ZXV_UPDATE_INFO_SIZE` and `ZXV_UI_FILE`, `ZXV_UI_CPU`, `ZXV_UI_ULA` |
| The Wi-Fi of a Pico 2 W (the Wi-Fi box of the EDIT menu) | Every `ZXV_WIFI_...` (the commands, `ZXV_WIFI_INFO_SIZE`, `ZXV_WIFI_LIST_MAX`, `_ENTRY`, `_SIZE`), `ZXV_WI_...` (the fields of the information), `ZXV_WS_...` (the states) and `ZXV_WP_...` (pairing) |
| The BASIC debugger's mailbox | `ZXV_DBG_BOX` and every `ZXV_DBG_...`, `ZXV_DBGC_...`, `ZXV_DBGS_...`, `ZXV_DBGR_...` and `ZXV_DBGO_...` |
| The ROM's workspace | `ZXV_ROM_WORKSPACE`, `ZXV_ROM_WORKSPACE_END` |
| The flash addresses | `ZXV_FLASH_BASE`, `ZXV_FLASH_SIZE`, `ZXV_PLATFORM_BASE`, `ZXV_ROM_SIZE` |
| The SRAM ones | `ZXV_SRAM_BASE`, `ZXV_SRAM_SIZE`, `ZXV_RESERVED_END` |
| The arithmetic inside `ZXV_RGB` and `ZXV_STD_RGB565` | `ZXV_TO5`, `ZXV_TO6`, `ZXV_STD_CHAN5` (use the outer macros) |
