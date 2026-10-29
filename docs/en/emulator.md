**English** · [Español](../es/emulator.md)

# The emulator

The emulator is ZX-V in software. It runs the same ROM (the BASIC) as the real
machine, so a program that works in the emulator works on the hardware, and the
other way round. It comes in several forms:

| Program | What it is |
|---|---|
| **`zxv`** | The desktop application, with menus, a debugger and settings that are remembered. The one most people want |
| **`zxv-sdl`** | A plain window with no menus, controlled with keyboard shortcuts |
| **`zxv-cli`** | No window at all: runs as fast as it can, for scripts, tests and screenshots |
| **The web version** | The same machine in a web browser |
| **`sdimg`** | A small tool to create and fill SD card images |

The Windows and Linux packages contain `zxv`, `zxv-sdl`, `zxv-cli` and `sdimg`,
plus the examples (the `ejemplos` folder) and the `ROM:` drive
(`romdisk.bin`). How to download them is explained in
[Getting started](getting-started.md#download).

## The desktop application (`zxv`)

`zxv` opens a window with the machine's screen, menus and a status bar. The
picture is scaled without distortion when you resize the window. What you
choose (window size, SD card, recent files, options) is remembered for the next
time.

### The menus

**File**

| Item | What it does |
|---|---|
| Open... (Alt+O) | Open a program, a listing, a snapshot or a card image ([see below](#opening-files)) |
| Reopen (Alt+L) | Open the last file again (handy after recompiling your program) |
| Recent | The files opened recently |
| Save program... (Alt+G) | Save the BASIC program to the card: you pick the name and the emulator types the `SAVE` for you (BASIC must be in its editor, not running a program) |
| Save a snapshot... (Alt+I) | Save the whole state of the machine in a `.zxvs` file |
| Load a snapshot... | Continue from a snapshot |
| Back to the snapshot (Alt+U) | Go back to the last snapshot saved or loaded |
| SD card | Choose the card: a folder, a card image, the usual one (Documents/ZX-V) or no card; and open the card folder on your PC |
| Save screenshot (Alt+S) | Save the screen as a PNG in your *Pictures/ZX-V* folder |
| Record sound... | Record everything the machine plays to a `.wav` file, until you choose the item again to stop |
| Quit (Alt+Q) | Close the emulator |

**Machine**

| Item | What it does |
|---|---|
| Paused (Alt+P) | Pause or continue the machine |
| Reset (Alt+R) | A full reset, like switching it off and on: the program in memory is lost |
| Ctrl+Alt+Del (back to BASIC) | The emergency reset: stops whatever is running and goes back to BASIC, keeping your BASIC program |
| Paste text (Alt+V) | Type the text on the clipboard into the machine, as if you typed it |

**View**: the window size (x1 is 640 × 480, up to x4), the border, the scaling, full screen
(Alt+Enter, without menus) and the status bar. The border is what surrounds
the 512 × 384 drawing area: **Full** shows what the real machine sends to
the screen (640 × 480), **Small** (576 × 432) crops it, and **No border**
leaves just the drawing area. It is remembered for next time. **Scaling**
says how the picture fills the window when you make it bigger: **Keep the
aspect ratio** (the usual: as big as fits without distortion), **Integer
scale** (only x1, x2, x3..., with every pixel the same size) or **Fill the
window**, even if the picture gets stretched. When the scale is not a whole
number, the picture is enlarged with the pixel edges smoothed, without some
rows thicker than others.

**Options**

| Item | What it does |
|---|---|
| Language | English, Spanish or automatic (the language of your system). It changes the menus and the machine ([see below](#language)) |
| Mute | No sound |
| Unlimited CPU speed | Run the machine as fast as your PC allows ([see below](#speed)) |
| Strict mode | Warn about anything a program does that the real machine would not allow (for programmers) |
| Hide the PC pointer over the picture | For programs that draw their own mouse pointer |
| Programs outside the card | What to do when you open a program that is not on the SD card: ask, copy it to the card, or use its folder as the card |

**Debug**: the debugger window (Alt+D), loading a program's symbols, one
instruction (Alt+N), to the end of the frame (Alt+F), the GDB server and the
strict mode warnings. See [Debugging](#debugging).

**Help**: the keyboard and shortcuts, this documentation (the copy in the
package, which works offline, or else the one on GitHub), and the version.

### The keyboard

You type as on any PC, with your own keyboard layout: what is printed on the
key is what you get. BASIC and its editor use the PC keys directly:

| Key | On ZX-V |
|---|---|
| Enter | Store the line, or run a command |
| Esc | **BREAK**: stop a running program. In the editor, undo the changes to the line |
| Tab | **EDIT**: the editor's options menu |
| Arrow keys, Home, End, Page Up, Page Down, Ctrl+Left/Right | Move around the listing |
| Backspace, Delete | Delete to the left or under the cursor |
| Ctrl+Alt+Del | Emergency reset: back to BASIC, keeping the program |

Windows keeps Ctrl+Alt+Del for itself, and some Linux desktops do too: if
that happens, use **Machine > Ctrl+Alt+Del (back to BASIC)**.

The editor is explained in [The editor](basic/editor.md).

Because Ctrl belongs to the machine, all the emulator's shortcuts use **Alt**.
Alt on its own does not open the menu bar.

| Shortcut | What it does |
|---|---|
| Alt+O, Alt+L | Open; reopen the last file |
| Alt+G | Save the BASIC program to the card |
| Alt+I, Alt+U | Save a snapshot; go back to it |
| Alt+V | Paste text |
| Alt+R | Full reset |
| Alt+P | Pause or continue |
| Alt+S | Save a screenshot |
| Alt+Enter | Full screen |
| Alt+D | The debugger |
| Alt+N, Alt+F | One instruction; to the end of the frame |
| Alt+Q | Quit |

**The Spectrum keys.** Some programs, especially those that come from the
ZX Spectrum, read the keyboard as the Spectrum's grid of keys (with `IN` or
`INKEY$`). For them, your PC keyboard also presses the Spectrum keys: letters
and digits are themselves, Shift is CAPS SHIFT, Ctrl is SYMBOL SHIFT,
Backspace is DELETE (CAPS SHIFT + 0), Esc is BREAK (CAPS SHIFT + SPACE) and the
arrow keys are CAPS SHIFT + 5, 6, 7 and 8. Punctuation follows the symbol your
key produces: the key that types `"` presses SYMBOL SHIFT + P, whatever your
layout. You do not need to think about this in normal use.

### Mouse and gamepads

**The mouse** does not need to be captured: while the PC pointer is over the
picture, the machine's pointer goes to the same place, and the buttons and the
wheel are the machine's. When it leaves the picture, it is your PC's pointer
again. If a program draws its own pointer, **Options > Hide the PC pointer over
the picture** removes the duplicate.

**Gamepads** work as soon as you connect them; the first is player 1 and the
second, player 2. The d-pad or the left stick are the directions, and the
buttons are:

| Gamepad (Xbox names) | ZX-V |
|---|---|
| A | Fire 1 |
| B | Fire 2 |
| X | Fire 3 |
| Start (Menu) | Start |
| Back (View) | Select |

On a PlayStation pad that is ✕, ○ and □. The PC keyboard is not turned into
a gamepad; most games, like *Estela*, accept both the keyboard and a gamepad.

### The SD card

The machine keeps your files on its SD card. In `zxv` you can choose, in
**File > SD card**:

- **The usual one (Documents/ZX-V):** the default. The first time, `zxv`
  creates this folder and copies the examples into it.
- **A folder...:** any folder of your PC acts as the card. Everything the
  machine saves goes into it, and anything you copy there with your file
  manager is on the card. **Open the folder** shows it in your file manager.
- **A card image...:** a `.img` file containing a whole card, formatted as
  FAT, FAT32 or exFAT. It can be a copy of a real ZX-V card, which you can make
  with `dd` on Linux or a tool such as Win32 Disk Imager on Windows, and you
  can write an image back to a card.
- **No card:** as if the card had been taken out. BASIC then reports
  `U No SD card`.

A folder card behaves like a real card: names are not case sensitive
(`GAME.TAP` and `game.tap` are the same file) and programs cannot see anything
outside the folder. File names can use only unaccented letters, digits and
some symbols, up to 63 characters; files with other names do not show up.

**`sdimg`** creates and fills card images from the command line:

```sh
sdimg new card.img 64 fat32          # a new 64 MB image (fat, fat32 or exfat)
sdimg put card.img game.tap /game.tap   # copy a file into the image
sdimg get card.img /game.tap game.tap   # copy a file out of it
sdimg mkdir card.img /games          # create a folder
sdimg ls card.img /                  # list a folder, with dates
```

(`sdimg` speaks only Spanish for now.)

**The `ROM:` drive**, with the built-in examples, is the `romdisk.bin` file that
sits next to the emulator. If it is missing, `ROM:` is empty.

### Opening files

**File > Open...** (Alt+O), dragging a file onto the window, or naming it on
the command line (`zxv game.tap`) all do the same:

| File | What happens |
|---|---|
| `.zxv`, `.tap`, `.tzx` | The machine is reset and the program is loaded with `LOAD`, exactly as you would on the machine. If it is in a subfolder of the card, the emulator first changes to that folder with `CD`, in case the program loads more files from it |
| `.bas` | A BASIC listing in plain text: like a program, it is loaded with `LOAD` (the BASIC turns it into a program, checking each line) |
| `.txt` | A text, typed into the editor for you, line by line: a listing, or commands such as `RUN` |
| `.zxvs` | A snapshot: the machine continues from where it was saved |
| `.img`, or a folder | It becomes the SD card |

**Programs from outside the card.** A program has to be on the card to be
loaded, so if it is somewhere else the emulator asks you to choose:

- **Copy it to the card**, to its main folder, together with the files next to
  it that have the same name (such as a game's `.gfx` graphics).
- **Use its folder** as the card until you choose another one. Anything the
  program saves goes into that folder.

**Options > Programs outside the card** can make the choice for you every time.

A `.bas` listing is the easiest way to write programs in your favourite text
editor: a text file with numbered lines, keywords spelt out (`PRINT`, `GO TO`),
in capitals or not. It loads on the real machine too, from the card
([Listings in text](basic/reference.md#listings-in-text)). Type `RUN` once it
is loaded.

### Snapshots

A **snapshot** (`.zxvs`) stores the complete state of the machine: memory, the
screen, the sound chips, the clock, the language and the current folder.
Loading it continues exactly from that moment, as if the machine had never
stopped. It is a quick way to save a game in the middle, or to go back to a
known point while you experiment:

- **Alt+I** saves a snapshot: it asks for a file name, suggesting the last
  one. **Alt+U** goes back to the last snapshot saved or loaded.
- **File > Load a snapshot...**, Open, or dragging the file onto the window
  load any snapshot.

A snapshot does **not** contain the SD card: when you load one, the card is the
one in use now, and files the program had open are closed (as with
Ctrl+Alt+Del). The emulator's settings are not saved in it either.

### Screenshots and sound

- **Alt+S** (File > Save screenshot) saves the screen as you see it, with the
  border from **View > Border**, as `zxv-DATE-TIME.png` in *Pictures/ZX-V*.
- **File > Record sound...** records everything the machine plays to a WAV file
  (48 kHz, stereo, 16 bits) until you stop it.

There is no video recording; to capture video, use a screen recorder on your
PC.

### Speed

The emulator runs the machine at its real speed: 60 frames per second, with a
144 MHz processor. **Options > Unlimited CPU speed** lets the processor run as
fast as your PC can (up to twenty times more work per frame), while the
screen and the sound stay at their normal 60 frames per second. Programs that
synchronise with the screen (most games) look the same; long BASIC
calculations finish much sooner.

The emulator counts one instruction per clock cycle, so a native program that
just fits in a frame in the emulator may be a little slower on the real
hardware. See the [SDK guide](sdk/README.md) for measuring.

### Language

**Options > Language** chooses English or Spanish (or *Automatic*, your
system's language, or English if it is neither). It changes:

- the emulator's menus and windows, the next time you open `zxv`;
- the machine's language (BASIC's messages and editor), when you reset it
  (Alt+R).

A program can also change the machine's language with `LANGUAGE "en"` or
`LANGUAGE "es"`; `zxv` remembers that choice, just as the real machine does.

New translations of the menus can be added without recompiling: a `zxv_XX.qm`
file in a folder called `idiomas` next to the program.

## The simple window (`zxv-sdl`)

`zxv-sdl` shows the machine in a window without menus. You type and use the
mouse as in `zxv`, and you control the emulator with these keys:

| Key | What it does |
|---|---|
| Alt+Enter | Full screen |
| Alt+S | Save a screenshot (`zxv-NNNN.bmp`, in the current folder) |
| Alt+R | Full reset |
| Alt+I, Alt+U | Save a snapshot (in `zxv.zxvs`, or the last one opened); go back to it |
| Alt+P, Alt+N, Alt+F | Pause or continue; one instruction; to the end of the frame |
| Alt+D | Debug panel over the picture: CPU, VRAM, sound, line table, none |
| Alt+B | The border: full (640 × 480), small, none (with the debug panel it is always shown whole) |
| Ctrl+Alt+Del | Emergency reset: back to BASIC |
| Alt+Q | Quit |

`zxv-sdl` has **no SD card** unless you give it one with `--sd FOLDER` or
`--sd-image FILE`. If you open a program from outside, its folder becomes the
card. You can drop files on the window, as in `zxv`. By default its machine and
messages use your system's language.

## Command-line options

`zxv`, `zxv-sdl` and `zxv-cli` accept the same options (and `zxv-cli --help`
lists them all). These are the ones you are most likely to use:

| Option | What it does |
|---|---|
| `FILE` | Open a file at start-up, as with File > Open (`zxv`, `zxv-sdl`) |
| `--sd FOLDER` | Use FOLDER as the SD card |
| `--sd-image FILE` | Use a card image as the SD card |
| `--romdisk FILE` | The `ROM:` drive (by default, `romdisk.bin` next to the program) |
| `--lang XX` | The machine's language, `en` or `es`, for this session only. In `zxv-sdl` and `zxv-cli` it is also the language of the emulator's messages |
| `--scale N` | Window size, 1 to 4 (`zxv`, `zxv-sdl`) |
| `--border SIZE` | The border: `full` (`completo`, 640 × 480), `small` (`pequeño`) or `none` (`ninguno`) (`zxv`, `zxv-sdl`) |
| `--no-sound` | No sound (`zxv`, `zxv-sdl`) |
| `--turbo` | Unlimited CPU speed |
| `--cpu-hz N` | Emulated processor speed in Hz (default 144000000) |
| `--clock DATE` | The clock: `host` (your PC's time, the default), `"2026-01-31 12:00:00"` (that time at start-up, then running), or `off` (no time until a program sets it) |
| `--type TEXT` | Type TEXT at start-up. `\n` or `{ENTER}` is Enter; also `{ESC}`, `{TAB}`, `{BS}`, `{DEL}`, `{UP}`, `{DOWN}`, `{LEFT}`, `{RIGHT}`, `{HOME}`, `{END}`, `{PGUP}`, `{PGDN}`, `{F1}`...`{F12}`, `{CTRL+END}`, `{SHIFT+UP}`; `{WAIT n}` waits n frames, `{RESET}` presses Ctrl+Alt+Del and `{JOY bits}` holds gamepad 1 with those buttons (those of [`JOY`](basic/reference.md#joy)) until the next `{JOY}`: `{JOY 1}{WAIT 60}{JOY 0}` pushes it right for one second |
| `--paste FILE` | Type the contents of a text file (a BASIC listing, for example) after `--type`, much faster than key by key |
| `--load-snapshot F.zxvs` | Start from a snapshot |
| `--save-snapshot F.zxvs` | Save a snapshot at the end (`zxv-cli`) |
| `--frames N` | Stop after N frames (60 per second) |
| `--screenshot F.bmp` | Save the screen at the end |
| `--wav F.wav` | Record the sound to a WAV file |
| `--strict` | Strict mode: warn about anything the real machine would not allow |
| `--version` | Print the ZX-V version (`ZX-V 0.5.0`) and exit. On Windows `zxv` has no console: its version is in **Help > About ZX-V** |

The rest (`--rom`, `--elf`, `--load`, `--gdb`, `--debug`, `--profile`,
`--panel`, `--test`, `--fail-on-warning`) are for programmers and are explained
in the [SDK guide](sdk/README.md).

### `zxv-cli`: the machine without a window

`zxv-cli` has no window, no sound output and no keyboard: it runs as fast as it
can, typing what you tell it to, and saves what you ask for. It is useful for
scripts, automatic tests and making screenshots for documentation. Like
`zxv-sdl`, it has no SD card unless you give it one, and its machine is in
English unless you use `--lang`.

Run a BASIC listing and take a screenshot after six seconds. `--paste` types
the file after anything given with `--type`, so make `RUN` the last line of
`program.bas`:

```sh
zxv-cli --paste program.bas --frames 300 --screenshot result.bmp
```

Load a game from the card, wait, and record ten seconds of its music:

```sh
zxv-cli --sd mycard --type 'LOAD "game"\n' --frames 500 --wav music.wav
```

## The web version

The web version is the same machine running in your browser, with the same
ROM, BASIC and `ROM:` drive. Unzip `zxv-web.zip` and open `index.html`: it
works straight from the disk, or from any web server. It is four files, 1 MB in
total (with the examples and the ROM disk).

Click on the picture so that it receives the keyboard. Browsers do not allow
any sound until you press a key or click, so the machine starts silent.

**The menus** are like those of `zxv`, and so are the Alt shortcuts (with the
left Alt key):

| Menu | Items |
|---|---|
| File | Open... (Alt+O), The SD card..., Save the program... (Alt+G), Save a snapshot (Alt+I), Back to the snapshot (Alt+U), Download the snapshot, Save the image (Alt+S) |
| Machine | Reset (Alt+R), Ctrl+Alt+Del (back to BASIC), Stop/Continue (Alt+P), One instruction (Alt+N), Debug panel (Alt+D) |
| View | Sound on or off, Change the border (Alt+B), Full screen (Alt+Enter), Options..., Open in a separate tab (only when the page is inside another one) |
| Help | The keyboard and shortcuts |

Ctrl+Alt+Del is in the Machine menu because your operating system keeps those
keys for itself.

**The SD card** is stored in the browser, so it is still there the next time
you open the page. The first time, it contains the examples. **File > The SD
card...** shows it: you can browse folders, **upload files** from your PC,
**download** files to your PC, create folders, delete files and open them.
Files you drag onto the page are copied to the card (to the folder shown in the
card window) and the first one is opened. What programs save is written to the
browser's storage every three seconds and when you leave the page.

The card lives in that browser only: another browser, or another computer, has
its own card. If you clear the site's data in your browser, the card is
erased. Download the files you want to keep.

**Snapshots**: Alt+I keeps a snapshot inside the page and Alt+U goes back to
it; **File > Download the snapshot** saves it as a `.zxvs` file, which you can
open later in the web version or in `zxv`.

**Options** (View > Options...): the language (of the page and of the machine;
by default, your browser's), the border and the scaling (the same as in `zxv`:
the picture takes all the room of the page, keeping the aspect ratio, in whole
multiples or filling it), strict mode, unlimited CPU speed and hiding the
pointer over the picture. The machine's language changes when you reset it.
The border and the scaling are remembered, and Save the image (Alt+S) saves
what you see, with that border.

**What it does not have:** card images (`.img`), the GDB server and the
debugger windows. The debug panel (Alt+D) is there.

### Putting it on your website

The contents of `zxv-web.zip` can be uploaded as they are to any web server, or
to sites like **itch.io**: create a project of type *HTML*, upload the zip and
mark it as *This file will be played in the browser*. The page adapts to the
size it is given (960 × 720, for example), and View > Full screen works if you
enable itch.io's full-screen button.

On sites like itch.io the page runs inside a frame from another domain, and
some browsers (Safari, and possibly Firefox) do not let such frames store
anything. Then the card only lasts while the page is open, and the page tells
you so. **View > Open in a separate tab** opens the emulator on its own, where
the card is stored normally. Downloads from inside a frame depend on what the
site allows.

## Debugging

The emulator is also a complete development environment for native programs:

- in `zxv`, the **debugger** window (Alt+D): disassembly, breakpoints, watches,
  registers, memory, stack, the video and sound chips, and the symbols of your
  program;
- in `zxv-sdl` and the web version, a **debug panel** drawn over the picture;
- in `zxv-cli`, a console debugger (`--debug`);
- a **GDB server** (`--gdb PORT`, or Debug > GDB server... in `zxv`) to debug C
  programs with GDB, line by line;
- **strict mode**, which warns about anything that would fail on the real
  machine.

All of this is explained in the [SDK guide](sdk/README.md).

