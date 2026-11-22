**English** · [Español](../../es/hardware/firmware.md)

# The firmware

Each Pico needs its own program, called its firmware. It comes as a `.uf2`
file that you copy onto the Pico like a file onto a USB stick: no special
software is needed.

| Pico | File | What it contains |
|---|---|---|
| CPU Pico | `zxv_cpu.uf2` | The machine: the BASIC, the services (USB, SD card, clock) and the `ROM:` drive with the built-in examples |
| ULA Pico | `zxv_ula_link.uf2` | The picture and the sound, received from the CPU Pico |

Be careful with the ULA file: it is `zxv_ula_link.uf2`, **not** `zxv_ula.uf2`.
The second one is a test that shows some demos on its own, without the CPU
Pico.

## Getting the files

1. Go to the **Releases** page of the project on GitHub
   (<https://github.com/cronomantic/ZX-V/releases>).
2. In the latest version, download **`zxv-uf2.zip`** and unzip it. It contains
   all the `.uf2` files: you need `zxv_cpu.uf2` and `zxv_ula_link.uf2`. The
   others are tests (see [Test programs](#test-programs)). It also has
   `zxv-update.bin`, to update a machine from its card
   ([Updating](#updating)).


## Putting the firmware on a Pico

1. **Hold down the BOOTSEL button** on the Pico (the only button on it) and,
   while holding it, connect the Pico's micro-USB to your computer. Then
   release the button.
2. A drive called **`RP2350`** appears on the computer.
3. **Copy the `.uf2` file onto that drive** (drag and drop). When the copy
   finishes, the Pico restarts on its own with the new firmware and the drive
   disappears.

Do it once for each Pico, with its own file. Each file holds the Pico's whole
flash: the firmware, room for a second copy of it (used by updates from the
card), and on the CPU Pico the `ROM:` drive.

- **Before soldering.** On the ZX-V board it is easiest to flash each Pico
  before soldering it, and to label it (CPU or ULA): the two look identical.
  Remember: the **ULA Pico goes on the left** and the **CPU Pico on the right**,
  seen from the front.
- **Once on the board.** The micro-USB connector and the BOOTSEL button are
  still reachable. Plug the Pico into the computer **with the board switched
  on**: the board's diodes keep the two supplies apart. With the board off, a
  single Pico on USB would feed the other one through the link pins, and the
  RESET they share could hold it in reset.
- **On a protoboard** where your 5 V supply is connected to a Pico's VBUS pin
  (pin 40), **disconnect that supply first**, or its 5 V would reach your
  computer through the USB cable ([On a protoboard](protoboard.md#power)).

## Updating

**From the card**, with no cable:

1. Download the new `zxv-uf2.zip`, unzip it and copy **`zxv-update.bin`** to
   the main folder of the machine's microSD card.
2. Put the card in the machine and, in the BASIC editor, press **EDIT** (Tab)
   and choose **Update firmware**. The box shows the version on the card and
   the one in the machine. Press **Enter** to update, or Esc to leave it.
3. The machine restarts into an update screen with four steps: it checks the
   file, then updates the ULA Pico, the CPU Pico and the `ROM:` drive. It takes
   a minute or two; **do not switch it off**. The USB keyboard does not work on
   that screen: when it is done, it restarts by itself after five seconds with
   the new version. You can then delete `zxv-update.bin` from the card.

Each Pico keeps **two copies** of its firmware: the update is written in the
one that is not in use, and the Pico only moves to it once it has been
written and checked. So if the power goes off half way, or the card fails,
both Picos still start; at worst the `ROM:` drive is empty until you repeat
the update. If something goes wrong, the screen says what (and restarts by
itself after half a minute), and nothing has changed in the CPU Pico unless it
got to its step.

**The first time.** Picos with version 0.9.0 or older do not have the two
copies yet: load **both** of them once over USB as above, with the new
`zxv_cpu.uf2` and `zxv_ula_link.uf2`. From then on, the card is enough. The
box in the EDIT menu says so if a Pico needs it. In the emulator there is
nothing to update, and the box says that too.

**Over USB** it still works as before: repeat the steps above for **both**
Picos. Always use the two files from the same version: if the CPU and the ULA
have different versions of their link, the CPU says so in its report (see
below), and the picture may not appear.

**Without pressing BOOTSEL.** If you have `picotool` installed (it comes with
the Raspberry Pi Pico extension of VS Code), a Pico that is already running a
ZX-V firmware can be updated through its USB cable, without touching BOOTSEL:

```sh
picotool load -f -x zxv_cpu.uf2
```

`-f` restarts the Pico in loading mode and `-x` starts the new firmware when it
has finished. If the Pico has hung, or has never had ZX-V firmware, use
BOOTSEL.

## The report

**Every 5 seconds the CPU Pico writes a report**, in the machine's language
(the one `LANGUAGE` chooses), about the link, the sound, the SD card, the USB
devices, the clock and the ULA. It is the best place to look when something
does not work. There are two ways to see it:

- **Through J5**, with `zxv-link log` (see [Loading programs from the
  PC](#loading-programs-from-the-pc-j5)).
- **Through the CPU Pico's micro-USB**, which is also a serial port: with any
  terminal program (the Serial Monitor of VS Code, PuTTY on Windows, or
  `screen /dev/ttyACM0` on Linux; the speed does not matter). Plug it in with
  the board switched on.

The ULA Pico writes its own messages on its micro-USB.

## Loading programs from the PC (J5)

The board's `UART` header (J5) is for **sending programs from the PC and
running them**, without taking the card out. Solder its 3 pins and use a
**3.3 V** USB-serial adapter (many choose the voltage with a jumper or a
switch: set it to 3.3 V):

- `GND` to `GND`, the board's `TX` to the adapter's `RX`, and the board's
  `RX` to the adapter's `TX`. Leave the adapter's power pin (`VCC`)
  unconnected.
- Better with the board switched on: when it is off, the adapter could power
  it a little through `RX`.
- **On Linux**, you must be in the `dialout` group to open the port
  (`sudo usermod -aG dialout $USER`, then log in again). If not, `zxv-link`
  says it has no permission to open it.

On the PC, **`zxv-link-gui`**, with a window, which comes with the
emulator packages. It is used in steps, from top to bottom:

1. **Board:** choose the adapter's port; choosing it connects, and it tells
   which firmware the machine has and whether BASIC is waiting for a
   command.
2. **Program:** choose it, drop it on the window or click one of the recent
   ones. That only puts it there: nothing is sent yet.
3. **Where the board loads it from:** **EXT:**, the usual (the shared
   folder, without copying anything to the card: see [The EXT:
   drive](#the-ext-drive)), or **The card** (copies the program and its
   files first).
   - With **EXT:**, the box **EXT: is** tells which folder it is, and
     **Choose...** changes the folder. If the program is not inside it, it
     warns and offers **Copy it to that folder** (with its files, and uses
     that copy) or **Run from the card**.
   - With **The card**, the files of its folder it can load (graphics,
     music, sounds...), ticked, and the ones you add. With the board
     connected it tells what the card has with each name: nothing, the same
     (not sent), what `zxv-link` put there last time but has changed
     (replaced), or a different one that `zxv-link` did not put there (it
     asks before replacing it). It also tells when the board has no card.
4. **Run on the board** (sends everything and runs it) or **Only copy to
   the card** (without running anything; with no program, it only copies
   the files of step 3). They are the only buttons that send anything. If
   another program is running, it asks before stopping it. While sending,
   **Stop** stops it.
5. **Result:** how each file goes, the command typed on the machine and
   what the machine reads and writes on `EXT:`.

Below, the recent programs and the machine's report while it is connected.
**Disconnect** releases the port, to use it with another program (such as
the VS Code extension).

The same, on the command line, with `zxv-link`:

```
zxv-link ports
zxv-link --port COM5 run game.zxv
zxv-link --port COM5 send graphics.gfx
zxv-link --port COM5 info
zxv-link --port COM5 log
```

- **`ports`** lists the serial ports: the adapter's is the one that appears
  when you plug it in (`COM5` on Windows, `/dev/ttyUSB0` on Linux).
- **`run`** copies the program to the root of the card and loads it (a
  `.bas`, with `RUN`). The files it loads can go after the program
  (`run game.zxv game.gfx music.zxm`). What is already on the card,
  unchanged, is not sent again. If a program is running, it first restarts
  the whole machine, like Ctrl+Alt+Del, and waits until BASIC is back.
- **`send`** only copies it (with another name after it, somewhere else:
  `send x.gfx SD:game/x.gfx`).
- **`info`**, the machine's version; **`log`**, the report until you press
  Ctrl+C.

**What is on the card is not lost by accident:**

- A file arrives whole or not at all. The board writes it aside
  (`zxv-link.tmp`) and only at the end, with everything checked, gives it
  its name. If something cuts it short (**Stop**, the adapter unplugged,
  the board switched off), the one that was there with that name stays as
  it was.
- `zxv-link` notes on the PC what it puts on the card. What has changed
  since then (the program you edit and run again) it replaces without
  asking. What it did not put there (what you copied by hand, with other
  contents) it does not touch: it sends nothing and exits with code 4,
  unless you give `--overwrite`
  (`zxv-link --port COM5 --overwrite run game.zxv`). The VS Code extension
  and the window ask.

J5 speaks in frames with `zxv-link`: a terminal program will not do.

**From VS Code**, the [ZX-V extension](https://github.com/cronomantic/zxv-vscode)
does the same with **ZX-V: Run on the board** (from `EXT:` or from the
card: see below), and debugs the ROM BASIC on the board with
`"target": "board"`: for that it uses `zxv-link serve`, which offers the
debugger over TCP just like the emulator with `--basic-debug`.

### The EXT: drive

Besides `SD:` (the card) and `ROM:` (the ROM disk, read-only), the machine
has a third drive: **`EXT:`, the external drive**, the files of a folder
served by **another device** (the server: a PC, a Raspberry Pi, a phone,
another ZX-V...). Today it goes through J5, with the same adapter; later,
perhaps through Wi-Fi. It is **one drive whatever the link**: a program that
uses `EXT:data.dat` works the same by cable as by Wi-Fi (if there were a
server on both, the cable wins).

Programs and BASIC use it like the others, to read **and write**:
`LOAD "EXT:game.bas"`, `SAVE "EXT:copy"`, `CAT "EXT:"`, `CD "EXT:"`,
`OPEN #` and the C API file functions, with the same names as on the card.
**With no server**, everything on `EXT:` gives error −18
(`V Disk error (no server for EXT:)`); if the server goes away, the first
call waits 1.5 seconds and fails, and the next ones fail at once until it
comes back. Nothing hangs. Through J5 it goes at about **80 KB/s** (a
100 KB program loads in a little over a second); the card is faster.

**The folder is chosen on purpose**, it is never the program's folder: the
machine can create files in it, and the program's folder could be your
home folder, Downloads or a system folder. In `zxv-link-gui` it is, by
default, `Documents/ZX-V-EXT` (created the first time; it is not
`Documents/ZX-V`, which is the emulator's card), and **Choose...** sets
another. It is served while the board is connected (*Served as EXT: while
the board is connected.*), so the machine can also use it on its own: with
`SAVE "EXT:copy"` from BASIC you keep on the PC what you type on the
machine.

On the command line:

```
zxv-link --port COM5 serve --folder FOLDER
zxv-link --port COM5 serve --folder FOLDER --run FOLDER/games/game.zxv
```

- **`serve --folder`** serves the folder as `EXT:` until you press Ctrl+C,
  and tells what the machine reads and writes (`EXT:/record.txt (write)`).
  It can be combined with `--basic-debug`.
- **`--run`** first runs the program (which must be inside the folder) from
  `EXT:`: it types `CD "EXT:/games": LOAD "game.zxv"`, so that a `.zxv`
  finds its files, and keeps serving.
- It needs firmware with protocol 4 (`zxv-link info` tells).

**What is in the folder is not lost by accident** (with `zxv-link`, the
window and the VS Code extension):

- The machine does not leave the folder: `..` stays at the root.
- A file is written whole or not at all: it goes to a temporary one that
  only gets its name at the end.
- **It does not replace or delete files the machine did not write** (it
  gives it `ZXV_E_ACCESS`), unless you allow it: with `--overwrite`
  (`zxv-link --port COM5 --overwrite serve --folder FOLDER`) or, in the
  window, with the tick box **Let the machine replace or delete files it
  did not write** (unticked at first). What the machine writes is noted on
  the PC.

**In VS Code**, **ZX-V: Run on the board** loads from `EXT:` by default (the
setting `zxv.boardFrom`: `ext` or `card`). The folder is the workspace
folder, or another one with the setting `zxv.extFolder`; `zxv.extOverwrite`
allows replacing. `zxv-link` keeps serving while the program runs: the
status bar shows **EXT:**, and a click (or **ZX-V: Stop serving EXT:**)
stops it. If the program is outside the folder, it offers to run it from
the card. When **debugging on the board** (`"target": "board"`), the same:
the folder is served while the session lasts and the program is loaded
from `EXT:`.

To write another server (on another device, in another language), what
travels through J5 is in [The protocol](protocol.md).

## If there is no picture or no sound

### No picture

Look at the red **link LED** (`ENLACE` on the board) when you switch on.

- **It never blinks.** The ULA Pico is not running its firmware: check that it
  has `zxv_ula_link.uf2` and that it gets power.
- **It blinks, then stays off.** The ULA works but receives nothing from the
  CPU Pico. Check that the CPU Pico has `zxv_cpu.uf2` (and the ULA has not
  been given the CPU's file by mistake). On a protoboard, check the link
  wiring.
- **It blinks, then stays on.** Images arrive correctly: the problem is
  between the ULA and the screen. Try the safe mode (below), another HDMI
  input, or a shorter cable without adapters. On the board, the report on the
  serial port says whether the screen is detected (`screen: connected`): if it
  stays `unknown` with the screen plugged in and switched on, look at the HDMI
  cable and connector.
- **It keeps blinking.** Images arrive, but some with errors. On a protoboard,
  shorten the link wires, add the 33 Ω resistors and more ground wires.

The number of blinks at start-up is the video mode plus one (once for mode 0,
twice for mode 1).

**The safe mode.** Hold **MODE** down (`MODO` on the board) while switching on:
the ULA starts in DVI, which every screen should accept. This lasts only until
the next restart. To keep a mode, press MODE briefly with the machine running:
each short press switches between HDMI and DVI, saves it and restarts the ULA.
If your screen only shows the safe mode, press MODE until the link LED blinks
**twice** at start-up: that is mode 1, the same as the safe mode, and it will
be kept.

| Mode | Output | Sound on the screen |
|---|---|---|
| 0 | HDMI 640 × 480, 60 Hz (default) | Yes |
| 1 | DVI 640 × 480, 60 Hz (safe mode) | No |

**You see moving demos instead of the BASIC** (a scrolling game, coloured
stripes, circles): the ULA Pico has `zxv_ula.uf2`, the test. Put
`zxv_ula_link.uf2` on it.

**Dots that flicker over a recognisable picture** are transmission errors
between the ULA and the screen. On a protoboard, look for a loose wire and
keep the HDMI wires short and in pairs. Some TVs also need 5 V on pin 18 of the
HDMI connector ([On a protoboard](protoboard.md#hdmi)).

### No sound

- **In mode 1 (DVI) the screen has no sound.** Switch to mode 0 with MODE,
  or use the audio jack.
- **The audio jack is line level**: it needs powered speakers, an amplifier or
  a TV's audio input. Headphones need an amplifier.
- **Try a sound from the BASIC:** `BEEP 1,0` or `PLAY "cdefgab"`.
- **On a protoboard**, the sound travels on its own wire, from GP20 of the CPU
  Pico (pin 26) to GP21 of the ULA Pico (pin 27). Without it there is a
  picture but no sound.
- A little hiss on the audio jack is normal.

### Other things

- **The keyboard does nothing.** Look at the report on the serial port: the
  `USB:` part says how many keyboards, mice and gamepads it sees. Try without a hub, or with a powered hub.
- **The SD card does not work.** It must be FAT, FAT32 or exFAT. The report
  says `no card` until the first access (for example `CAT`), or if the card
  does not answer. On the board the card is mounted as soon as it goes in.
- **The report says `no ULA: timer`**: the CPU Pico
  is not receiving the ULA's frame signal. On a protoboard, check the wire
  between the two GP27 pins (pin 32).
- **The report says `no back channel`**: the wire
  from GP20 of the ULA (pin 26) to GP18 of the CPU (pin 24) is missing.
  Everything works without it, except that programs are always told the output
  is HDMI.

### The diagnostic page

**A long press of MODE** (between 1 and 5 seconds) or **Ctrl+Alt+F12** on the
keyboard shows a blue band across the middle of the screen with the state of
both Picos, updated every second: at the top the ULA (its mode, the images per
second, link errors, the CRC of the palettes it receives, the sound); at the
bottom the CPU (link, sound, SD card,
USB, clock). If the CPU's part does not arrive within 2.5 seconds, the page
says so. Do the same again to hide it. It is in the machine's language: the
CPU sends it to the ULA with every frame. The ULA Pico's own messages on its
micro-USB follow it too, once the link is working.

## Test programs

`zxv-uf2.zip` also contains test programs. Two are useful when building:

| File | On | What it does |
|---|---|---|
| `zxv_ula.uf2` | One Pico 2 alone | The ULA's picture and sound with some demos, without the CPU Pico. Proves the HDMI output and the audio jack. The MODE button works the same |
| `zxv_hdmi.uf2` | One Pico 2 alone | Colour bars and a test tone on each side (left, right, centre) over HDMI. `zxv_dvi.uf2` is the same in DVI, without sound |

The rest (`zxv_linktx`, `zxv_bench`, `zxv_bench_ram`, `zxv_isatest`) are for
the firmware's developers. Remember to put the right firmware back afterwards.

