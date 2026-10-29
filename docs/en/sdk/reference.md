**English** · [Español](../../es/sdk/reference.md)

# SDK reference

Everything the SDK offers, grouped by topic: the functions of the system
(the API), the graphics engine, the registers and constants of the
machine, the memory map and the file formats. To learn how to use them,
read the [SDK guide](README.md) and [Graphics](graphics.md).

**Contents**

- [The files of the SDK](#the-files-of-the-sdk)
- [Conventions](#conventions)
- [System and timing](#system-and-timing)
- [Screen and text](#screen-and-text)
- [Keyboard, joystick and mouse](#keyboard-joystick-and-mouse)
- [Sound](#sound)
- [Files and folders](#files-and-folders)
- [Clock and language](#clock-and-language)
- [Ending a program and running others](#ending-a-program-and-running-others)
- [Graphics engine](#graphics-engine)
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

| File | Contents |
|---|---|
| `include/zxv_api.h` | The API as `static inline` C functions; the mouse pointer (`zxv_mouse_t`); types `zxv_dirent_t` and `zxv_clock_t`. Includes `zxv.h`, `<stdint.h>` and `<stddef.h>` (so `NULL` and `size_t` are available) |
| `include/zxv_gfx.h` | The graphics engine: types, constants and functions. Includes `zxv_api.h` (define `ZXV_GFX_NO_API` to get only the types and constants) |
| `include/zxv.h` | Addresses, registers and constants, as `#define` only, so it also works in assembly (`.S` files) |
| `include/sys/dirent.h` | `struct dirent` and `DIR` for picolibc's `<dirent.h>` |
| `crt0.S` | Start-up code: sets the `tp` register (thread variables, used by picolibc's `errno`) and jumps to `main` |
| `zxv.ld` | Linker script: everything at the start of the user RAM, contiguous; heap for picolibc |
| `zxvlib.c` | `memcpy`, `memmove`, `memset`, `memcmp`, for programs without the C library |
| `zxvc.c` | The glue between picolibc and the API (files, folders, console, heap, time, exit) |
| `tools/elf2zxv.py` | Converts the linked `.elf` into a `.zxv` |
| `tools/png2gfx.py` | Converts PNG images into a `.gfx` graphics file |
| `Makefile` | Builds the examples (`make`, `make run SD=folder`, `make debug SD=folder`) |

## Conventions

- All functions follow the standard RISC-V `ilp32` calling convention; the
  inline functions of the headers call the ROM through the
  [API table](#api-function-numbers).
- Functions that can fail return an `int`: **0 or more** on success and a
  **negative** [error code](#error-codes) on failure.
- "The drawing page" is the video page chosen with `zxv_gfx_draw_page` (0
  at the start of a program); "the visible page" is the one chosen with
  `zxv_set_page`.
- Registers are memory: read and write them through `volatile` pointers,
  for example `*(volatile uint8_t *)(uintptr_t)ZXV_V_BORDER = 2;`. Video
  registers take effect at the next VBLANK.

## System and timing

### `zxv_wait_vblank`

```c
void zxv_wait_vblank(void);
```

Waits for the next VBLANK, the start of a frame (60 per second). It turns
interrupts on if they were off. Do not call it from a frame hook.

### `zxv_set_frame_hook`

```c
void zxv_set_frame_hook(void (*fn)(void));
```

Makes the system call `fn` at every VBLANK, with interrupts off; `0`
removes it. The function must be short and cannot call
`zxv_wait_vblank`. Remove it before your program ends. Variables shared
with the hook must be `volatile`.

### `zxv_time_us`

```c
uint32_t zxv_time_us(void);
```

A microsecond counter. It wraps round every 2³² µs (about 71 minutes):
compare times by subtracting them as unsigned numbers.

```c
uint32_t t0 = zxv_time_us();
do_something();
uint32_t elapsed = zxv_time_us() - t0;
```

### `zxv_api_version`

```c
uint32_t zxv_api_version(void);
```

The version of the machine specification, as `major << 16 | minor`
(currently 1, version 0.1); the same as `ZXV_SPEC_VERSION` and the system
variable `ZXV_SV_VERSION`.

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_FRAME_HZ` | 60 | Frames per second |
| `ZXV_CPU_HZ_REF` | 144000000 | Reference speed of the processor (Hz) |
| `ZXV_SPEC_VERSION` | 1 | Version of the specification (0.1) |

## Screen and text

### `zxv_set_mode`

```c
void zxv_set_mode(int mode);
```

Changes the video mode (0: 256 × 192; 1: 512 × 384) and clears the
drawing page with the paper colour of the text (see
[`zxv_text_colors`](#zxv_text_colors-zxv_text_at)). Only bit 0 of `mode`
counts.

| Constant | Value |
|---|---|
| `ZXV_MODE0_W`, `ZXV_MODE0_H` | 256, 192 |
| `ZXV_MODE1_W`, `ZXV_MODE1_H` | 512, 384 |

### `zxv_set_page`

```c
void zxv_set_page(int page);
```

Makes page 0 or 1 visible from the next VBLANK (writes `ZXV_V_PAGE`).

### `zxv_cls`

```c
void zxv_cls(int colour);
```

Fills the drawing page with a colour (0–15) and moves the text cursor to
the top left.

### `zxv_plot`

```c
void zxv_plot(int x, int y, int colour);
```

Sets one pixel of the drawing page. Points outside the page are ignored.

### `zxv_print_char`, `zxv_print_str`

```c
void zxv_print_char(int c);
void zxv_print_str(const char *s);
```

Print a character or a zero-terminated string at the text cursor of the
drawing page, in cells of 8 × 8 pixels, with the colours of
`zxv_text_colors`. `'\n'` and `'\r'` go to the start of the next line,
`'\b'` deletes the previous character, and the screen scrolls up when the
text reaches the bottom.

`zxv_print_char` takes a Unicode code point and `zxv_print_str` a string in
UTF-8 (the encoding of C source files, so you can write `"¡Hola!"` in your
program). They draw the ASCII characters 32 to 126, `©`, and the Spanish
letters of the ROM's font: `á é í ó ú ü ñ`, their capitals `Á É Í Ó Ú Ü Ñ`,
`¿` and `¡`. Any other character prints as `?`.

### `zxv_text_colors`, `zxv_text_at`

```c
void zxv_text_colors(int ink, int paper);
void zxv_text_at(int col, int row);
```

`zxv_text_colors` sets the colours (0–15) of the text that
`zxv_print_char` and `zxv_print_str` write from then on: each cell is
filled with `paper` and the character drawn in `ink`. Every program
starts with black on white (0 and 7), whatever the previous one left.

`zxv_text_at` moves the text cursor to column `col` and row `row`, counted
from 0 at the top left: 32 × 24 cells in mode 0, 64 × 48 in mode 1.
`zxv_cls` and `zxv_set_mode` also put the cursor back at the top left.

```c
zxv_text_colors(14, 1);                 /* bright yellow on blue */
zxv_text_at(8, 10);
zxv_print_str("¡Hola, señor Núñez!");
```

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

The bits of the number are bright, green, red, blue (from bit 3 to bit 0).

## Keyboard, joystick and mouse

### `zxv_key_get`

```c
int zxv_key_get(void);
```

Takes the next key from the key queue (64 entries), or returns −1 if it
is empty. Keys held down repeat (after 500 ms, 20 times a second).

| Code | Key |
|---|---|
| Unicode code of the character | A key that types a character (`'a'`, `'ñ'` = 0xF1...) |
| 0x0D, 0x08, 0x09, 0x1B | Enter, Backspace, Tab, Esc |
| `ZXV_KEY_SPECIAL` (0xF000) \| HID code | Any other key: F1–F12, cursors, Home, End... |

Ctrl+Alt+Delete never arrives: it returns to BASIC.

### `zxv_joy_read`

```c
uint32_t zxv_joy_read(int n);
```

The state of gamepad `n` (0 or 1; the first one connected is 0). A
disconnected gamepad reads 0.

| Constant | Bit | Constant | Bit |
|---|---|---|---|
| `ZXV_JOY_RIGHT` | 0 | `ZXV_JOY_FIRE2` | 5 |
| `ZXV_JOY_LEFT` | 1 | `ZXV_JOY_FIRE3` | 6 |
| `ZXV_JOY_DOWN` | 2 | `ZXV_JOY_START` | 7 |
| `ZXV_JOY_UP` | 3 | `ZXV_JOY_SELECT` | 8 |
| `ZXV_JOY_FIRE1` | 4 | `ZXV_JOY_CONNECTED` | 15 |

Direction comes from the d-pad or the left stick. On a PlayStation pad, ✕,
○ and □ are fire 1, 2 and 3; Share/Create is Select and Options is Start.
On a generic pad, buttons 1–3 are the fire buttons, 9 is Select and 10
Start.

### The mouse: `zxv_mouse_t`, `zxv_mouse_init`, `zxv_mouse_update`

```c
typedef struct {
    int x, y;                   /* the pointer, in pixels */
    int wheel;                  /* the wheel, accumulated (positive upwards) */
    uint8_t buttons;            /* bits 0-2: left, right and middle */
    uint16_t rx, ry;            /* internal: the counters at the last update */
    uint8_t rw;
} zxv_mouse_t;

void zxv_mouse_init(zxv_mouse_t *p, int x, int y);
void zxv_mouse_update(zxv_mouse_t *p, int w, int h);
```

`zxv_mouse_init` puts the pointer at (x, y) and remembers the mouse's
counters. `zxv_mouse_update`, once per frame, adds the movement since the
last call (one mouse unit is one pixel, without acceleration) and keeps
the pointer inside [0, w) × [0, h). It reads the
[input registers](#input-registers) `ZXV_I_MOUSE_...` directly.

## Sound

### `zxv_ay_write`

```c
void zxv_ay_write(int reg, int val);
```

Writes `val` to register `reg` of an AY: 0–15 for the first chip,
`ZXV_AY2` + 0–15 (16–31) for the second. The write goes through a queue
of 256 entries and sounds a few milliseconds later; if the queue is full,
the function waits. Writing register 13 restarts the envelope, as on the
real chip.

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
repeats every 256 × period / 1,773,447.5 seconds. Both chips start silent.

### `zxv_beeper_event`

```c
void zxv_beeper_event(int level);
```

Changes the beeper to `level` (0, or anything else for 1) at exactly the
current time plus `ZXV_BEEP_LATENCY_US` (2000 µs). Events go through a
queue of 256 entries (shared with `zxv_ay_event`); if it is full, the
function waits. For simple sounds you can write the `ZXV_S_BEEPER`
register instead; both ways add up.

### `zxv_ay_event`

```c
void zxv_ay_event(uint32_t t_us, int reg, int val);
```

Writes `val` to AY register `reg` (0–31, as in `zxv_ay_write`) at time
`t_us` (in the clock of `zxv_time_us`) plus 2 ms. Times must go in
increasing order: an event waits for the ones before it. If `t_us` is
already past, it sounds at once. If the queue is full, the function waits.
It is meant for digitised sound: see the
[`digital`](../../../sdk/examples/digital/digital.c) example.

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_AY2` | 16 | Add to a register number to reach the second AY |
| `ZXV_AY_FIFO_LEN` | 256 | Entries in the AY queue |
| `ZXV_BEEP_QUEUE_LEN` | 256 | Entries in the timed queue |
| `ZXV_BEEP_LATENCY_US` | 2000 | Fixed delay of the timed queue, in µs |
| `ZXV_AY_STEREO_MONO`, `_ABC`, `_ACB` | 0, 1, 2 | Values of `ZXV_S_AY_STEREO` |
| `ZXV_EV_AY`, `ZXV_EV_AY_WRITE(reg, val)` | | The data word of an AY write in the timed queue (bit 31 set); only needed if you fill the queue yourself |

## Files and folders

Paths may start with a drive: `SD:` (the SD card) or `ROM:` (the
read-only built-in drive), upper or lower case; `ROM:game` is the same as
`ROM:/game`. Without a drive, a path is relative to the current folder, or
absolute if it starts with `/`. `.` and `..` work, and `..` at the root
stays at the root. Case does not matter. Names have up to 63 printable
ASCII characters (not `\ : * ? " < > |`, not ending in a space or a dot);
paths, up to 255.

Up to 8 files and 4 folders can be open at once on each drive. Buffers
must be in RAM (or, for reading, in the ROM); otherwise the function
returns `ZXV_E_FAULT`. On a real SD card, a file that is open cannot be
deleted, renamed or opened again for writing, and hidden or system files
do not appear in listings.

### `zxv_file_open`

```c
int zxv_file_open(const char *name, uint32_t flags);
```

Opens a file and returns its number, or an error. `flags` combines:

| Flag | Value | Effect |
|---|---|---|
| `ZXV_O_READ` | 0x01 | Read |
| `ZXV_O_WRITE` | 0x02 | Write |
| `ZXV_O_CREATE` | 0x04 | Create it if it does not exist (needs `ZXV_O_WRITE`) |
| `ZXV_O_TRUNC` | 0x08 | Empty it when opening |
| `ZXV_O_APPEND` | 0x10 | Start at the end |

Opening for writing on `ROM:` returns `ZXV_E_ACCESS`.

### `zxv_file_read`, `zxv_file_write`

```c
int zxv_file_read(int f, void *buf, uint32_t n);
int zxv_file_write(int f, const void *buf, uint32_t n);
```

Read or write up to `n` bytes at the current position and return how
many, or an error. `zxv_file_read` returns 0 at the end of the file. If you
read code into memory to run it, execute a `fence.i` instruction
afterwards.

### `zxv_file_seek`

```c
int zxv_file_seek(int f, int32_t off, int whence);
```

Moves the position to `off` bytes from the start (`ZXV_SEEK_SET`, 0), the
current position (`ZXV_SEEK_CUR`, 1) or the end (`ZXV_SEEK_END`, 2), and
returns the new position.

### `zxv_file_close`

```c
int zxv_file_close(int f);
```

Closes the file. Always close files you wrote: that is when everything
reaches the card.

### `zxv_file_load`

```c
int zxv_file_load(const char *name, void *addr, uint32_t max);
```

Reads a whole file into memory at `addr` and returns its size. `max` is the
room you have there: if the file is bigger than `max` bytes, nothing is
loaded and the function returns `ZXV_E_RANGE`. Pass the size of your buffer:

```c
static uint8_t level[8192];
int size = zxv_file_load("level1.dat", level, sizeof(level));
if (size < 0)
    zxv_report_error(size, "Cannot load level1.dat");
```

Earlier versions of the SDK had no `max` argument; a program compiled with
them must be compiled again.

### `zxv_file_stat`

```c
int zxv_file_stat(const char *name, zxv_dirent_t *e);
```

Fills `*e` with the name, size and attributes of a file or folder.

```c
typedef struct {
    char name[64];              /* zero-terminated */
    uint32_t size;              /* in bytes */
    uint32_t attr;              /* ZXV_ATTR_DIR (bit 0): it is a folder */
} zxv_dirent_t;
```

The same 72 bytes described for assembly: `ZXV_DIRENT_SIZE` (72),
`ZXV_DE_NAME` (0), `ZXV_DE_SIZE` (64), `ZXV_DE_ATTR` (68).

### `zxv_file_remove`, `zxv_file_rename`

```c
int zxv_file_remove(const char *name);
int zxv_file_rename(const char *from, const char *to);
```

Delete a file or an **empty** folder; rename or move a file or folder.

### Folders

```c
int zxv_dir_make(const char *name);             /* create a folder */
int zxv_dir_change(const char *name);           /* change the current folder */
int zxv_dir_current(char *buf, uint32_t len);   /* the current folder; returns its length */
int zxv_dir_open(const char *name);             /* open a folder to list it ("" = the current one) */
int zxv_dir_read(int d, zxv_dirent_t *e);       /* next entry: 1, or 0 at the end */
int zxv_dir_close(int d);
```

- `zxv_dir_change("ROM:")` enters the ROM drive and `zxv_dir_change("SD:")`
  goes back to the root of the card.
- `zxv_dir_current` gives `/...` on the card and `ROM:/...` on the ROM
  drive; if `buf` is too small it returns `ZXV_E_RANGE`.
- `zxv_dir_read` returns the entries in alphabetical order, ignoring case.

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_NAME_MAX` | 63 | Characters per name |
| `ZXV_PATH_MAX` | 255 | Characters per path |
| `ZXV_MAX_FILES` | 8 | Files open at once (per drive) |
| `ZXV_MAX_DIRS` | 4 | Folders open at once (per drive) |
| `ZXV_ATTR_DIR` | 0x01 | Attribute: folder |

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

`zxv_clock_get` returns 0, or `ZXV_E_NOCLOCK` if the clock has not been
set (then `*c` is unchanged). `zxv_clock_set` returns 0, or `ZXV_E_INVAL`
for a date that does not exist or is outside 2000–2099; it ignores
`wday`. The time is local time, without time zones. Both are slow: to
measure time, use `zxv_time_us` or `ZXV_SV_FRAMES`. Files created on the
card get this date.

For assembly: `ZXV_CLOCK_SIZE` (8) and the offsets `ZXV_CK_YEAR` (0),
`ZXV_CK_MONTH` (2), `ZXV_CK_DAY` (3), `ZXV_CK_HOUR` (4), `ZXV_CK_MIN` (5),
`ZXV_CK_SEC` (6), `ZXV_CK_WDAY` (7).

### `zxv_lang_get`, `zxv_lang_set`

```c
uint32_t zxv_lang_get(void);
int zxv_lang_set(uint32_t lang);
```

The machine's language: a two-letter ISO 639-1 code in lower case, first
letter in the low byte, made with `ZXV_LANG(a, b)`. `ZXV_LANG_EN` is
`ZXV_LANG('e', 'n')` and `ZXV_LANG_ES` is `ZXV_LANG('e', 's')`. The ROM
has English and Spanish texts, but any code is valid, so programs can use
it to choose their own texts. `zxv_lang_set` changes it (and the machine
remembers it); it returns `ZXV_E_INVAL` if the code is not two lower-case
letters.

```c
const char *hello = zxv_lang_get() == ZXV_LANG_ES ? "Hola" : "Hello";
```

## Ending a program and running others

### `zxv_exit_to_basic`

```c
void zxv_exit_to_basic(int code);
```

Ends the program from anywhere and returns to BASIC, as returning `code`
from `main` does. **0 means success**: BASIC shows `0 OK, 0:1`. Any other
code means an error, and BASIC shows `W Native code error (code), 0:1`
(in Spanish, `W Error en el código nativo (code), 0:1`). In both cases the
BASIC program that was in memory is gone, because the native program used
that memory.

The value `main` returns works the same way. Native code called from BASIC
with `USR` is different: what it returns is just the value of `USR`.

### `zxv_exec`

```c
int zxv_exec(const char *name, int *result);
```

Loads and runs a `.zxv` (if `name` has no extension and does not exist,
`.zxv` is added), stores in `*result` what it returned, and returns 0 or an
error (`ZXV_E_FORMAT` if it is not a valid program, `ZXV_E_RANGE` if it
does not fit).

**A native program cannot use it**: called from a `.zxv`, it returns
`ZXV_E_ACCESS` (−9) and does nothing. Every program is loaded at the same
address, at the start of the user RAM, so the new one would overwrite the
one that called it. Only BASIC (with `LOAD`) and the system's monitor
launch programs.

### `zxv_report_error`

```c
void zxv_report_error(int code, const char *msg);
```

Ends the program and returns to BASIC with the report `W` and your own
message instead of BASIC's: `msg` is a string in UTF-8, of which BASIC
shows up to 47 bytes. If `code` is not 0, it follows in brackets. For
example, `zxv_report_error(5, "Fichero dañado")` gives
`W Fichero dañado (5), 0:1`, and `zxv_report_error(0, "No level files")`
gives `W No level files, 0:1`. The message is copied before BASIC takes
back the memory, so it can be in a local array. An empty message shows
BASIC's usual text (`W Native code error (5), 0:1`).

BASIC's screen font has the Spanish letters, so `ñ`, `¿` or `á` can go in
the message; other non-ASCII characters show as `?`.

## Graphics engine

Declared in `zxv_gfx.h`. See [Graphics](graphics.md) for explanations and
examples. All functions draw on the current target and clip to it and to
the clip rectangle.

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
    uint8_t wrap;           /* 1: repeats; 0: outside is colour 'out' */
    uint8_t out;
    uint8_t reserved;
} zxv_m7tex;

typedef struct { int32_t u, v, du, dv; } zxv_m7line;   /* 16.16 fixed point */

typedef struct {            /* a mode 7 camera, 16.16 fixed point */
    int32_t x, y;           /* position on the texture */
    int32_t height;         /* height above the floor */
    int32_t angle;          /* 0x10000 = a full turn; 0 looks towards growing u */
    int32_t focal;          /* focal distance, in pixels */
    int16_t horizon;        /* screen line of the horizon (integer) */
    int16_t reserved;
} zxv_m7cam;
```

Pixels are 4 bits, two per byte, the even pixel in the low 4 bits.
Bitmaps, tiles and prepared sprites must be aligned to 4 bytes.

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `ZXV_TRANSPARENT` | 8 | The transparent colour |
| `ZXV_FLIP_H`, `ZXV_FLIP_V` | 0x01, 0x02 | Mirror horizontally, vertically (blit and sprites) |
| `ZXV_BLIT_TRANSPARENT` | 0x04 | `zxv_gfx_blit`: do not copy colour 8 |
| `ZXV_MAP_WRAP` | 0x01 | `zxv_tilemap.flags`: the map repeats |
| `ZXV_CELL_TILE` | 0x0FFF | Cell: tile number |
| `ZXV_CELL_FLIP_H`, `ZXV_CELL_FLIP_V` | 0x1000, 0x2000 | Cell: mirrored |
| `ZXV_CELL_TRANSP` | 0x4000 | Cell: colour 8 is transparent |
| `ZXV_CELL_PRIO` | 0x8000 | Cell: priority |
| `ZXV_MAP_ALL`, `ZXV_MAP_BACK`, `ZXV_MAP_FRONT` | 0, 1, 2 | `zxv_map_draw` filter: all cells, without priority, with priority |

### Target and clipping

```c
void zxv_gfx_target(const zxv_bitmap *b);
void zxv_gfx_draw_page(int page);
void zxv_gfx_clip(int x0, int y0, int x1, int y1);
```

- `zxv_gfx_target(b)` makes bitmap `b` the target; `0` returns to the
  drawing page. It removes the clip rectangle. The bitmap structure is
  copied, so it can be a local variable.
- `zxv_gfx_draw_page(page)` chooses the drawing page (0 or 1), also for
  `zxv_cls`, `zxv_plot` and the text. Its size follows the video mode and
  `ZXV_V_VSIZE`.
- `zxv_gfx_clip` limits drawing to the rectangle from (x0, y0) to
  (x1, y1), excluding x1 and y1, in target coordinates.

When a program starts, the target is the drawing page 0, without a clip
rectangle.

### Shapes

```c
void zxv_gfx_line(int x0, int y0, int x1, int y1, int c);
void zxv_gfx_rect(int x, int y, int w, int h, int c);
void zxv_gfx_tri(const int16_t *p, int c);
void zxv_gfx_poly(const int16_t *p, int n, int c);
void zxv_gfx_circle(int x, int y, int r, int c, int fill);
```

- `zxv_gfx_line`: a line including both ends.
- `zxv_gfx_rect`: a filled rectangle of w × h with its top left corner at
  (x, y). Nothing if w or h is 0 or less.
- `zxv_gfx_tri`: a filled triangle; `p` has 3 pairs x, y.
- `zxv_gfx_poly`: a filled polygon of `n` vertices (3 to 64) given as
  pairs x, y, with the even-odd rule (a five-pointed star drawn in one go
  has a hole in the middle).
- `zxv_gfx_circle`: a circle of radius `r` centred on (x, y): only the
  outline if `fill` is 0, a disc otherwise.

Triangles and polygons follow a top-left rule: two of them sharing a side
neither overlap nor leave a gap.

### Copies

```c
void zxv_gfx_blit(const zxv_bitmap *src, int sx, int sy, int w, int h, int x, int y, int flags);
```

Copies the rectangle (sx, sy, w, h) of `src` to (x, y) of the target, up
to 1024 pixels wide. `flags`: `ZXV_FLIP_H`, `ZXV_FLIP_V`,
`ZXV_BLIT_TRANSPARENT`. The part of the rectangle outside `src` is not
copied. Copying a bitmap onto itself with overlapping areas is not
defined.

### Sprites

```c
uint32_t zxv_spr_prepare(const zxv_bitmap *src, int sx, int sy, int w, int h, void *buf);
void zxv_spr_draw(const void *spr, int x, int y, int flags, const uint8_t *colours);
void zxv_spr_draw_scaled(const void *spr, int x, int y, int w, int h, int flags, const uint8_t *colours);
int zxv_spr_collide(const void *a, int ax, int ay, const void *b, int bx, int by);
```

- `zxv_spr_prepare` converts the rectangle (sx, sy, w, h) of `src` (up to
  1024 × 1024; what falls outside `src` is transparent) into a sprite in
  `buf`, which must be aligned to 4 bytes. It returns the size in bytes,
  8 + h × ⌈w / 8⌉ × 8; with `buf` = `NULL` it only returns the size. It
  returns 0 if the size is not valid.
- `zxv_spr_draw` draws a sprite with its top left corner at (x, y).
  `flags`: `ZXV_FLIP_H`, `ZXV_FLIP_V`. `colours`: `NULL`, or a table of
  16 bytes; colour *c* is drawn as `colours[c]` (transparent pixels stay
  transparent).
- `zxv_spr_draw_scaled` draws it stretched or shrunk to w × h (1 to 4096
  each; otherwise nothing), taking for each pixel the nearest one of the
  sprite. At its own size it is the same as `zxv_spr_draw`. It uses about
  2.5 KB of stack.
- `zxv_spr_collide` returns 1 if some opaque pixel of `a` at (ax, ay)
  coincides with an opaque pixel of `b` at (bx, by), at their own size.

### Tile maps

```c
void zxv_map_draw(const zxv_tilemap *m, int mx, int my, int x, int y, int w, int h, int filter);
uint16_t zxv_map_cell(const zxv_tilemap *m, int px, int py);
```

- `zxv_map_draw` draws, in the rectangle (x, y, w, h) of the target, the
  part of the map that starts at pixel (mx, my) of the map. `filter`:
  `ZXV_MAP_ALL`, `ZXV_MAP_BACK`, `ZXV_MAP_FRONT`. Cells outside a map
  without `ZXV_MAP_WRAP` are not drawn. A single call can cover up to
  1024 pixels of width.
- `zxv_map_cell` returns the cell at pixel (px, py) of the map (0 outside
  a map that does not repeat).

### Mode 7

```c
void zxv_m7_draw(const zxv_m7tex *t, const zxv_m7line *l, int y, int n);
void zxv_m7_camera(zxv_m7line *l, int n, const zxv_m7cam *c);
```

- `zxv_m7_draw` draws `n` lines from line `y` of the target; line `y + i`
  uses `l[i]`. Pixel x of a line shows the texel at
  (u + x·du, v + x·dv), taking the integer part of each coordinate.
- `zxv_m7_camera` fills `l[0..n-1]` for lines 0 to n−1 of the target:
  line *i* below the horizon is at distance height × focal / (i − horizon),
  with the centre of the target at the camera's x. Lines at or above the
  horizon are set to zero. Draw with `zxv_m7_draw(t, l + y0, y0, n)`.

## Video registers

Write them at any time; the system applies them at the next VBLANK.

| Register | Size | Contents |
|---|---|---|
| `ZXV_V_MODE` | 8 bits | Video mode, 0 or 1 (use `zxv_set_mode`) |
| `ZXV_V_PAGE` | 8 bits | Visible page, 0 or 1 (use `zxv_set_page`) |
| `ZXV_V_BORDER` | 8 bits | Border colour, 0–15 |
| `ZXV_V_FLAGS` | 8 bits | `ZXV_VFLAG_LINETAB` (0x01): use the line table; `ZXV_VFLAG_BLANK` (0x02): show only the border |
| `ZXV_V_SCROLL_X` | 16 bits | Horizontal scroll of the picture, wrapping round |
| `ZXV_V_SCROLL_Y` | 16 bits | Vertical scroll, wrapping round |
| `ZXV_V_REMAP` | 8 bits | Colour remapping table in use, 0–3 |
| `ZXV_V_VSIZE` | 8 bits | Page size in mode 0 (see below) |
| `ZXV_V_OUT_INFO` | 32 bits | Read only: bits 0–3, the output (`ZXV_OUT_DVI` 1, no sound; `ZXV_OUT_HDMI` 2, with sound); bits 8–15, the refresh rate (always 60 Hz); bits 16–17, the screen (`ZXV_OUT_SCREEN_ON` connected, `ZXV_OUT_SCREEN_NONE` unplugged, `ZXV_OUT_SCREEN_UNKNOWN` not known: on a protoboard, or before one has been connected). The user chooses the output, not programs |

With scroll, screen pixel (x, y) shows page pixel ((x + SCROLL_X) mod
width, (y + SCROLL_Y) mod height).

**Page sizes** (`ZXV_V_VSIZE`, mode 0 only; ignored in mode 1):

| Constant | Value | Page | Bytes per line |
|---|---|---|---|
| `ZXV_VSIZE_SCREEN` | 0 | 256 × 192 | 128 |
| `ZXV_VSIZE_2X2` | 1 | 512 × 384 | 256 |
| `ZXV_VSIZE_WIDE` | 2 | 1024 × 192 | 512 |
| `ZXV_VSIZE_TALL` | 3 | 256 × 768 | 128 |

**Remapping tables:** four tables of 16 bytes from `ZXV_REMAP_TABLES`
(table *t* at `ZXV_REMAP_TABLES + 16·t`). Entry *v* is the colour shown
for pixel value *v*. They start as the identity.

**Line table:** at `ZXV_LINE_TABLE`, one entry of `ZXV_LINE_ENTRY_SIZE`
(8) bytes per line of the whole frame, border included: 240 lines in mode
0 (the picture starts at line 24), 480 in mode 1 (picture from line 48):
`ZXV_FRAME_LINES(mode)` and `ZXV_BORDER_LINES(mode)`. The frame is
`ZXV_FRAME_W` × `ZXV_FRAME_H` (640 × 480), with a border of `ZXV_BORDER_X`
(64) pixels at each side and `ZXV_BORDER_Y` (48) lines above and below.

| Offset | Size | Field | Value for "as usual" |
|---|---|---|---|
| `ZXV_LT_BORDER` (0) | 8 bits | Border colour | `0xFF` |
| `ZXV_LT_REMAP` (1) | 8 bits | Remapping table | `0xFF` |
| `ZXV_LT_SCROLL_X` (2) | 16 bits | Horizontal scroll (0 to page width − 1) | `0xFFFF` |
| `ZXV_LT_SRC_LINE` (4) | 16 bits | Page line shown (0 to page height − 1) | `0xFFFF` (= (y + SCROLL_Y) mod height) |
| 6 | 16 bits | Reserved, write 0 | |

On border lines only the border field counts.

**When changes are safe:** right after each VBLANK the system reads the
visible page, the registers and the tables. While bit
`ZXV_SYSF_LINK_BUSY` of `ZXV_SV_SYSFLAGS` is 1 (up to 1.5 ms in mode 0 and
6 ms in mode 1 or with a large page), do not write to the visible page or
the line table.

**Video memory:** `ZXV_VRAM0_BASE` (`0x2000_4000`) and `ZXV_VRAM1_BASE`
(`0x2001_C000`), `ZXV_VRAM_PAGE_SIZE` (96 KB) each.

## Input registers

| Register | Size | Contents |
|---|---|---|
| `ZXV_I_KEYROWS` | 8 × 8 bits | The Spectrum keyboard matrix by half-rows; bits 0–4, 0 = pressed |
| `ZXV_I_KEY_HEAD`, `ZXV_I_KEY_TAIL` | 8 bits each | Write and read positions of the key queue (use `zxv_key_get`) |
| `ZXV_I_KEY_FIFO` | 64 × 16 bits | The key queue (`ZXV_KEY_FIFO_LEN` = 64) |
| `ZXV_I_KEYMAP` | 32 bytes | 256 bits, one per HID key code; 1 = pressed |
| `ZXV_I_KEY_MODS` | 8 bits | Modifiers: bits 0–3 left Ctrl, Shift, Alt, GUI; bits 4–7 the same on the right |
| `ZXV_I_JOY0`, `ZXV_I_JOY1` | 16 bits each | Gamepads (as `zxv_joy_read`) |
| `ZXV_I_MOUSE_X`, `ZXV_I_MOUSE_Y` | 16 bits each | Mouse movement counters, wrapping round (x to the right, y down) |
| `ZXV_I_MOUSE_BTN` | 8 bits | Bits 0–2: left, right, middle button |
| `ZXV_I_MOUSE_WHEEL` | 8 bits | Wheel counter (positive upwards) |

**HID codes** of common keys: A–Z 0x04–0x1D; 1–9 0x1E–0x26, 0 0x27;
Enter 0x28, Esc 0x29, Backspace 0x2A, Tab 0x2B, Space 0x2C; F1–F12
0x3A–0x45; Insert 0x49, Home 0x4A, Page Up 0x4B, Delete 0x4C, End 0x4D,
Page Down 0x4E; Right 0x4F, Left 0x50, Down 0x51, Up 0x52; left Ctrl,
Shift, Alt 0xE0–0xE2, right ones 0xE4–0xE6. They identify the position of
the key on a US keyboard.

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
cursor keys press their Spectrum combinations (CAPS SHIFT + 0, SPACE, 5–8),
and punctuation signs press SYMBOL SHIFT with the right key.

## Sound registers

| Register | Size | Written by | Contents |
|---|---|---|---|
| `ZXV_S_BEEPER` | 8 bits | program | Bit 0: beeper level |
| `ZXV_S_AY_STEREO` | 8 bits | program | 0 mono, 1 ABC, 2 ACB (both chips) |
| `ZXV_S_VOLUME` | 8 bits | program | Master volume 0–255 (200 at start-up) |
| `ZXV_S_AFLAGS` | 8 bits | program | Bit 0: mute |
| `ZXV_S_AY_HEAD`, `ZXV_S_AY_TAIL` | 16 bits each | program, system | Positions of the AY queue (`ZXV_AY_FIFO`) |
| `ZXV_S_BEEP_HEAD`, `ZXV_S_BEEP_TAIL` | 16 bits each | program, system | Positions of the timed queue (`ZXV_BEEP_QUEUE`) |
| `ZXV_S_AY_REGS` | 16 × 8 bits | system | Read-only copy of the registers of the first AY, as already applied |
| `ZXV_S_AY2_REGS` | 16 × 8 bits | system | The same for the second AY |

The queues are easier to use through `zxv_ay_write`, `zxv_beeper_event` and
`zxv_ay_event`.

Stereo mixing: in ABC, left = A + half of B and right = C + half of B; in
ACB, left = A + half of C, right = B + half of C; in mono, all three on
both sides. The beeper goes to both sides. The channels of the two chips
with the same letter are added together.

## System variables

At `ZXV_SYSVARS` (`0x2000_0700`), 32 bits each:

| Variable | Contents |
|---|---|
| `ZXV_SV_FRAMES` | Frame counter, +1 at every VBLANK (not while interrupts are off) |
| `ZXV_SV_VERSION` | Version of the specification (`major << 16 \| minor`) |
| `ZXV_SV_FRAME_HOOK` | The current frame hook (change it with `zxv_set_frame_hook`) |
| `ZXV_SV_RAMTOP` | Top of the usable RAM; the system stack starts here |
| `ZXV_SV_URAM_START`, `ZXV_SV_URAM_END` | Start and end (excluded) of the user RAM |
| `ZXV_SV_CPU_HZ` | Processor frequency |
| `ZXV_SV_SYSFLAGS` | `ZXV_SYSF_LINK_BUSY` (bit 0): the visible page is being sent; `ZXV_SYSF_BREAK` (bit 1): the BREAK key is pressed |
| `ZXV_SV_LANG` | The machine's language (read only: use `zxv_lang_set`) |

## Error codes

| Code | Constant | Meaning |
|---|---|---|
| 0 | `ZXV_OK` | Success |
| −1 | `ZXV_E_NOENT` | It does not exist |
| −2 | `ZXV_E_EXIST` | It already exists |
| −3 | `ZXV_E_NAME` | Name or path not valid |
| −4 | `ZXV_E_NOTDIR` | Part of the path is not a folder |
| −5 | `ZXV_E_ISDIR` | It is a folder |
| −6 | `ZXV_E_NOTEMPTY` | The folder is not empty |
| −7 | `ZXV_E_BADF` | File or folder number not valid |
| −8 | `ZXV_E_MFILE` | Too many open |
| −9 | `ZXV_E_ACCESS` | Not allowed (by the open mode, on `ROM:`, deleting the root, or `zxv_exec` from a native program) |
| −10 | `ZXV_E_FAULT` | The buffer is outside the allowed memory |
| −11 | `ZXV_E_IO` | Read or write error |
| −12 | `ZXV_E_NOCARD` | No SD card |
| −13 | `ZXV_E_NOSPC` | The card is full |
| −14 | `ZXV_E_FORMAT` | Not a valid `.zxv` |
| −15 | `ZXV_E_RANGE` | It does not fit (in the user RAM, or in the buffer: `zxv_file_load`, `zxv_dir_current`) |
| −16 | `ZXV_E_NOCLOCK` | The clock is not set |
| −17 | `ZXV_E_INVAL` | Value not valid |

If the card is taken out, or stops answering, while files or folders are
open, they keep their numbers (a file opened afterwards does not get one of
them) and every call on them returns `ZXV_E_NOCARD` or `ZXV_E_IO` until you
close them. Closing them returns `ZXV_OK`: what was already written is on
the card.

## API function numbers

The API is a table of 256 function addresses at `ZXV_API_TABLE`
(`0x2000_0000`). Entry *n* is at `ZXV_API_TABLE + 4·n`. From C, the
headers' functions use the macro `ZXV_API_FN(n, type)`, which reads the
entry and casts it to a function pointer type. In assembly:

```asm
    lui  t0, %hi(ZXV_API_TABLE)
    lw   t0, 4*ZXV_API_CLS(t0)
    jalr t0
```

| n | Constant | Function |
|---|---|---|
| 0 | `ZXV_API_VERSION` | `zxv_api_version` |
| 1 | `ZXV_API_WAIT_VBLANK` | `zxv_wait_vblank` |
| 2 | `ZXV_API_SET_FRAME_HOOK` | `zxv_set_frame_hook` |
| 3 | `ZXV_API_TIME_US` | `zxv_time_us` |
| 4 | `ZXV_API_SET_MODE` | `zxv_set_mode` |
| 5 | `ZXV_API_SET_PAGE` | `zxv_set_page` |
| 6 | `ZXV_API_CLS` | `zxv_cls` |
| 7 | `ZXV_API_PLOT` | `zxv_plot` |
| 8 | `ZXV_API_PRINT_CHAR` | `zxv_print_char` |
| 9 | `ZXV_API_PRINT_STR` | `zxv_print_str` |
| 10 | `ZXV_API_KEY_GET` | `zxv_key_get` |
| 11 | `ZXV_API_JOY_READ` | `zxv_joy_read` |
| 12 | `ZXV_API_AY_WRITE` | `zxv_ay_write` |
| 13 | `ZXV_API_BEEPER_EVENT` | `zxv_beeper_event` |
| 14–17 | `ZXV_API_FILE_OPEN`, `_READ`, `_WRITE`, `_CLOSE` | `zxv_file_open`, `zxv_file_read`, `zxv_file_write`, `zxv_file_close` |
| 18 | `ZXV_API_FILE_LOAD` | `zxv_file_load` |
| 19 | `ZXV_API_EXIT_TO_BASIC` | `zxv_exit_to_basic` |
| 20 | `ZXV_API_REPORT_ERROR` | `zxv_report_error` |
| 21–24 | `ZXV_API_FILE_SEEK`, `_STAT`, `_REMOVE`, `_RENAME` | `zxv_file_seek`, `zxv_file_stat`, `zxv_file_remove`, `zxv_file_rename` |
| 25–30 | `ZXV_API_DIR_MAKE`, `_CHANGE`, `_CURRENT`, `_OPEN`, `_READ`, `_CLOSE` | `zxv_dir_make` ... `zxv_dir_close` |
| 31 | `ZXV_API_ZXV_EXEC` | `zxv_exec` |
| 32 | `ZXV_API_GFX_TARGET` | `zxv_gfx_target` |
| 33 | `ZXV_API_GFX_DRAW_PAGE` | `zxv_gfx_draw_page` |
| 34 | `ZXV_API_GFX_CLIP` | `zxv_gfx_clip` |
| 35 | `ZXV_API_GFX_BLIT` | `zxv_gfx_blit` |
| 36 | `ZXV_API_SPR_PREPARE` | `zxv_spr_prepare` |
| 37 | `ZXV_API_SPR_DRAW` | `zxv_spr_draw` |
| 38 | `ZXV_API_SPR_COLLIDE` | `zxv_spr_collide` |
| 39 | `ZXV_API_MAP_DRAW` | `zxv_map_draw` |
| 40 | `ZXV_API_MAP_CELL` | `zxv_map_cell` |
| 41 | `ZXV_API_M7_DRAW` | `zxv_m7_draw` |
| 42 | `ZXV_API_M7_CAMERA` | `zxv_m7_camera` |
| 43 | `ZXV_API_GFX_LINE` | `zxv_gfx_line` |
| 44 | `ZXV_API_GFX_RECT` | `zxv_gfx_rect` |
| 45 | `ZXV_API_GFX_TRI` | `zxv_gfx_tri` |
| 46 | `ZXV_API_GFX_POLY` | `zxv_gfx_poly` |
| 47 | `ZXV_API_GFX_CIRCLE` | `zxv_gfx_circle` |
| 48 | `ZXV_API_CLOCK_GET` | `zxv_clock_get` |
| 49 | `ZXV_API_CLOCK_SET` | `zxv_clock_set` |
| 50 | `ZXV_API_LANG_SET` | `zxv_lang_set` |
| 51 | `ZXV_API_AY_EVENT` | `zxv_ay_event` |
| 52 | `ZXV_API_SPR_DRAW_SCALED` | `zxv_spr_draw_scaled` |
| 53 | `ZXV_API_TEXT_COLORS` | `zxv_text_colors` |
| 54 | `ZXV_API_TEXT_AT` | `zxv_text_at` |

`zxv_lang_get` and the mouse functions read memory directly and have no
number. Unused entries return −1. Because the table is in RAM, a program
can replace an entry to intercept a routine.

## Memory map

| From | To | Size | Contents | Access |
|---|---|---|---|---|
| `0x1000_0000` | `0x1003_FFFF` | 256 KB | Platform firmware | None |
| `0x1004_0000` | `0x100B_FFFF` | 512 KB | The ROM (`ZXV_ROM_BASE`): BASIC and the API | Read, execute |
| `0x100C_0000` | `0x103F_FFFF` | 3.25 MB | The `ROM:` drive (`ZXV_ROMDISK_BASE`) | Through the file functions |
| `0x2000_0000` | `0x2000_3FFF` | 16 KB | System area (`ZXV_SYS_BASE`) | See below |
| `0x2000_4000` | `0x2001_BFFF` | 96 KB | Video page 0 (`ZXV_VRAM0_BASE`) | Read, write |
| `0x2001_C000` | `0x2003_3FFF` | 96 KB | Video page 1 (`ZXV_VRAM1_BASE`) | Read, write |
| `0x2003_4000` | `0x2007_1FFF` | 248 KB | User RAM (`ZXV_URAM_BASE` to `ZXV_URAM_END`) | Read, write, execute |
| `0x2007_2000` | `0x2008_1FFF` | 64 KB | Reserved for the system (`ZXV_RESERVED_BASE`) | **Forbidden** |

**System area:**

| Address | Constant | Contents |
|---|---|---|
| `0x2000_0000` | `ZXV_API_TABLE` | API table (256 addresses) |
| `0x2000_0400` | `ZXV_VIDEO_REGS` | [Video registers](#video-registers) |
| `0x2000_0500` | `ZXV_INPUT_REGS` | [Input registers](#input-registers) |
| `0x2000_0600` | `ZXV_SOUND_REGS` | [Sound registers](#sound-registers) |
| `0x2000_0700` | `ZXV_SYSVARS` | [System variables](#system-variables) |
| `0x2000_0800` | `ZXV_AY_FIFO` | AY queue (256 × 2 bytes) |
| `0x2000_0A00` | `ZXV_BEEP_QUEUE` | Timed queue (256 × 8 bytes) |
| `0x2000_1200` | `ZXV_REMAP_TABLES` | Colour remapping tables (4 × 16 bytes) |
| `0x2000_1800` | `ZXV_LINE_TABLE` | Line table (480 × 8 bytes) |

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

The stack is the system's: it starts near `ZXV_URAM_END` and grows down.

## The `.zxv` file format

A 64-byte header followed by the program image, which is copied to the
load address. All numbers are little-endian.

| Offset | Constant | Size | Field |
|---|---|---|---|
| 0x00 | `ZXV_H_MAGIC` | 4 bytes | `ZXV1` (`ZXV_HDR_MAGIC`) |
| 0x04 | `ZXV_H_HDR_SIZE` | 16 bits | Header size: 64 (`ZXV_HDR_SIZE`) |
| 0x06 | `ZXV_H_PROFILE` | 16 bits | Specification profile: 1 (`ZXV_HDR_PROFILE`) |
| 0x08 | `ZXV_H_LOAD` | 32 bits | Load address |
| 0x0C | `ZXV_H_ENTRY` | 32 bits | Entry point |
| 0x10 | `ZXV_H_IMAGE_SIZE` | 32 bits | Size of the image |
| 0x14 | `ZXV_H_BSS_SIZE` | 32 bits | Bytes cleared to zero after the image |
| 0x18 | `ZXV_H_STACK` | 32 bits | Stack top (0 = keep the system's stack) |
| 0x1C | `ZXV_H_MODE` | 8 bits | Initial video mode |
| 0x1D | `ZXV_H_FLAGS` | 8 bits | Reserved: written as 0, ignored when loading |
| 0x1E | | 16 bits | Reserved |
| 0x20 | `ZXV_H_CRC` | 32 bits | CRC-32 of the image (the zlib one) |
| 0x24 | `ZXV_H_NAME` | 28 bytes | Name (ASCII, zero-terminated) |

When loading, the system checks the signature, the header size, the
profile, that the load address is aligned to 4 and the entry point inside
the image, that image and BSS fit in the user RAM leaving 4 KB below the
system stack, and the CRC. Then it clears the BSS, sets the video mode and
calls the entry point as `int entry(void)`. A stack top other than 0 must
be aligned to 16 and lie between the end of the BSS and `ZXV_URAM_END`.

## Tools

### `elf2zxv.py`

```sh
python3 sdk/tools/elf2zxv.py INPUT.elf OUTPUT.zxv [--name NAME] [--mode 0|1] [--stack ADDRESS]
```

| Option | Effect |
|---|---|
| `--name NAME` | Name in the header (up to 27 characters; by default the output file name without extension) |
| `--mode 0\|1` | Initial video mode (default 0) |
| `--stack ADDRESS` | Stack top (decimal or `0x...`; default 0, the system's stack) |

It checks that the program is inside the user RAM and prints the load
address, the entry point and the sizes. Like `png2gfx.py`, it speaks
English, or Spanish if your system is in Spanish.

### `png2gfx.py`

```sh
python3 sdk/tools/png2gfx.py OUTPUT.gfx COMMAND...
python3 sdk/tools/png2gfx.py --list FILE.gfx
```

The commands are listed in [Graphics](graphics.md#images-from-png-files-png2gfxpy),
together with the `.gfx` format. Sprite numbers go from 0 to 255, tiles
from 0 to 4095, maps from 0 to 15 and textures from 0 to 3, as in BASIC.
It reads any PNG except interlaced ones, without other Python packages.

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
| Time | `time`, `gettimeofday`, `localtime`: the machine's clock (1 January 2026 if it is not set) |
| Exit | `exit`, `abort`: back to BASIC (`exit(0)` gives `0 OK`, any other code a `W` report, as in [`zxv_exit_to_basic`](#zxv_exit_to_basic)) |

- API errors arrive in `errno` (`ENOENT`, `EEXIST`, `ENOTEMPTY`, `ENOSPC`...;
  no card is `ENODEV`).
- Descriptors 0, 1 and 2 are the console; files start at 3.
- `struct dirent` has `d_name`, `d_type` (`DT_DIR` or `DT_REG`) and, as an
  extra, the size in `d_size`.
- `printf` with decimals adds about 7 KB; `-DPICOLIBC_INTEGER_PRINTF_SCANF`
  when linking uses the smaller one, integers only.
- The console speaks UTF-8, like C strings: `printf("¿Qué tal?")` shows
  the accents, and `fgets` gives the keys typed in UTF-8 (`ñ` is two
  bytes). Letters the ROM font does not have come out as `?`.

## Internal definitions

`zxv.h` is shared with the ROM and the emulator, so it also defines
things programs must not use: the platform interface (`ZXV_PLATFORM_IF`
and every `ZXV_PIF_...`), the ROM's workspace (`ZXV_ROM_WORKSPACE`,
`ZXV_ROM_WORKSPACE_END`), the flash addresses (`ZXV_FLASH_BASE`,
`ZXV_FLASH_SIZE`, `ZXV_PLATFORM_BASE`, `ZXV_ROM_SIZE`) and the SRAM ones
(`ZXV_SRAM_BASE`, `ZXV_SRAM_SIZE`, `ZXV_RESERVED_END`). They are listed
here only so that you recognise them.
