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
   others are tests (see [Test programs](#test-programs)).


## Putting the firmware on a Pico

1. **Hold down the BOOTSEL button** on the Pico (the only button on it) and,
   while holding it, connect the Pico's micro-USB to your computer. Then
   release the button.
2. A drive called **`RP2350`** appears on the computer.
3. **Copy the `.uf2` file onto that drive** (drag and drop). When the copy
   finishes, the Pico restarts on its own with the new firmware and the drive
   disappears.

Do it once for each Pico, with its own file.

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

Download the new `zxv-uf2.zip` and repeat the steps above for **both** Picos.
Always use the two files from the same version: if the CPU and the ULA have
different versions of their link, the CPU says so in its report (see below),
and the picture may not appear.

**Without pressing BOOTSEL.** If you have `picotool` installed (it comes with
the Raspberry Pi Pico extension of VS Code), a Pico that is already running a
ZX-V firmware can be updated through its USB cable, without touching BOOTSEL:

```sh
picotool load -f -x zxv_cpu.uf2
```

`-f` restarts the Pico in loading mode and `-x` starts the new firmware when it
has finished. If the Pico has hung, or has never had ZX-V firmware, use
BOOTSEL.

## The serial port and the report

The CPU Pico's micro-USB is also a **serial port**. Open it with any terminal
program (for example the Serial Monitor of VS Code, PuTTY on Windows, or
`screen /dev/ttyACM0` on Linux; the speed does not matter). Two things happen
there:

- **What you type in the terminal reaches ZX-V** as if typed on its keyboard.
  Handy to try the machine before you have a USB keyboard.
- **Every 5 seconds the CPU Pico writes a report**, in the machine's language
  (the one `LANGUAGE` chooses), about the link,
  the sound, the SD card, the USB devices, the clock and the ULA. It is the
  best place to look when something does not work.

The same messages also come out of the board's `UART` header (J5) at 921600
baud. The ULA Pico writes its own messages on its micro-USB.

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
four times for mode 3).

**The safe mode.** Hold **MODE** down (`MODO` on the board) while switching on:
the ULA starts in DVI at 640 × 480 and 60 Hz, which every screen should accept.
This lasts only until the next restart. To keep a mode, press MODE briefly with
the machine running: each short press moves to the next mode (0, 1, 2, 3, then
0 again), saves it and restarts the ULA. If your screen only shows the safe
mode, press MODE until the link LED blinks **four times** at start-up: that is
mode 3, the same as the safe mode, and it will be kept.

| Mode | Output | Sound on the screen |
|---|---|---|
| 0 | HDMI 720 × 576, 50 Hz (default) | Yes |
| 1 | HDMI 640 × 480, 60 Hz | Yes |
| 2 | DVI 720 × 576, 50 Hz | No |
| 3 | DVI 640 × 480, 60 Hz (safe mode) | No |

**You see moving demos instead of the BASIC** (a scrolling game, coloured
stripes, circles): the ULA Pico has `zxv_ula.uf2`, the test. Put
`zxv_ula_link.uf2` on it.

**Dots that flicker over a recognisable picture** are transmission errors
between the ULA and the screen. On a protoboard, look for a loose wire and
keep the HDMI wires short and in pairs. Some TVs also need 5 V on pin 18 of the
HDMI connector ([On a protoboard](protoboard.md#hdmi)).

### No sound

- **In modes 2 and 3 (DVI) the screen has no sound.** Switch to mode 0 or 1
  with MODE, or use the audio jack.
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
  is HDMI at 50 Hz.

### The diagnostic page

**A long press of MODE** (between 1 and 5 seconds) or **Ctrl+Alt+F12** on the
keyboard shows a blue band across the middle of the screen with the state of
both Picos, updated every second: at the top the ULA (its mode, the images per
second, link errors, the sound); at the bottom the CPU (link, sound, SD card,
USB, clock). If the CPU's part does not arrive within 2.5 seconds, the page
says so. Do the same again to hide it. It is in the machine's language: the
CPU sends it to the ULA with every frame. The ULA Pico's own messages on its
micro-USB follow it too, once the link is working.

## Test programs

`zxv-uf2.zip` also contains test programs. Two are useful when building:

| File | On | What it does |
|---|---|---|
| `zxv_ula.uf2` | One Pico 2 alone | The ULA's picture and sound with some demos, without the CPU Pico. Proves the HDMI output and the audio jack. The MODE button works the same |
| `zxv_hdmi.uf2` | One Pico 2 alone | Colour bars and a test tone on each side (left, right, centre) over HDMI. `zxv_dvi.uf2`, `zxv_dvi_480p60.uf2` and `zxv_hdmi_480p60.uf2` are the same in other modes |

The rest (`zxv_linktx`, `zxv_bench`, `zxv_bench_ram`, `zxv_isatest`) are for
the firmware's developers. Remember to put the right firmware back afterwards.

