**English** · [Español](../../es/hardware/README.md)

# Building the hardware

ZX-V is not only an emulator: you can build the real machine and plug it into
a TV. It is made from two **Raspberry Pi Pico 2** boards, a handful of
connectors and a few dozen small parts. You do not need to write any code: the
software for both Picos comes ready to copy onto them.

> **Status.** The firmware for both Picos is written, and **each Pico has been
> tested on its own**: the HDMI picture and sound on a single Pico 2, and the
> CPU, which runs programs exactly like the emulator. **The two Picos have not
> yet been tested working together**, so the complete machine has not been
> tested, neither on the PCB nor on a protoboard. The **printed circuit board
> (PCB) is fully designed, but it has not been manufactured or tested yet**.
> If you build one now, you will be among the first to try it: read
> [The board](board.md) carefully and expect that something may need fixing.

## What you need

- **Two Raspberry Pi Pico 2** boards. They must be the Pico **2** (with the
  RP2350 chip): the original Raspberry Pi Pico does not run the firmware.
- **The board**, ordered assembled from JLCPCB, and the few parts you solder
  yourself ([The board](board.md)). Or, instead, a protoboard or breadboard and
  the parts in [On a protoboard](protoboard.md).
- **The firmware**: two `.uf2` files, one for each Pico
  ([The firmware](firmware.md)).
- **A screen with an HDMI input** (a TV or a monitor) and an HDMI cable.
- **A USB keyboard.** Optionally a USB mouse, one or two USB gamepads and a USB
  hub to plug them all in.
- **A microSD card**, to save your programs and load others.
- **A USB-C phone charger** of 5 V that can give 2 A, with its cable.
- Optionally, **powered speakers** for the audio output, and a **CR2032 coin
  cell** so the clock keeps the date and time.
- A soldering iron, solder, and a computer with a micro-USB cable to put the
  firmware on the Picos.

## Two ways to build it

| | The PCB (recommended) | A protoboard or breadboard |
|---|---|---|
| What it is | A 100 × 100 mm board with everything on it | The two Picos and modules wired by hand |
| Soldering | Only the two Picos, two buttons, a switch, two LEDs and the coin cell: all through-hole | Many wires; HDMI and the link between the Picos need short, tidy wiring |
| Connectors | HDMI, USB-A, microSD, audio jack, USB-C for power, real-time clock | Breakout boards and modules |
| Status | Designed, **not yet made or tested** | Each Pico tested on its own; **not yet together** |
| Guide | [The board](board.md) | [On a protoboard](protoboard.md) |

Both run the same firmware and behave the same.

## What each Pico does

The two Picos split the work like the chips in the original Spectrum. The
**CPU Pico** is the computer itself: it runs the BASIC and your programs, and
looks after the keyboard, mouse and gamepads (through the USB-A port), the SD
card and the clock. The **ULA Pico** makes the picture and the sound: it
receives each finished image from the CPU Pico, sends it to the screen over
HDMI, and plays the sound both on the HDMI and on the audio jack. It also
carries the video mode button and the link LED. They talk over an 8-bit bus and
a few extra wires. For your programs there is only one machine: they cannot
tell that there are two chips.

## Connecting it

### The screen (HDMI)

Plug any HDMI TV or monitor into the **HDMI** connector. The picture is
always 640 × 480 at 60 Hz, a mode every HDMI screen must accept, and the ULA
Pico has two output modes:

| Mode | Output | Sound on the screen | The link LED blinks at start-up |
|---|---|---|---|
| 0 | HDMI (the default) | Yes | Once |
| 1 | DVI (the **safe mode**) | No | Twice |

- **A short press of the MODE button** switches to the other mode. The ULA
  Pico restarts with it (the BASIC carries on) and remembers it when you
  switch off.
- **Holding MODE down while you switch on** starts in the safe mode, just that
  once. Every screen should accept it, including old DVI monitors that reject
  HDMI.
- Programs cannot change the mode, only read it.

### Sound

The sound comes out on the HDMI (in modes 0 and 1) and, always, on the
**3.5 mm stereo jack** (labelled `AUDIO`). The jack gives a line-level signal:
connect powered speakers, an amplifier or your TV's audio input. Headphones
need an amplifier.

### Keyboard, mouse and gamepads

Plug them into the **USB-A** port at the front. For more than one device, use
a USB hub (a powered hub is best). ZX-V understands ordinary USB keyboards,
mice and most gamepads.

- **Ctrl+Alt+F1**, **F2** and **F3** choose the keyboard layout: Spanish,
  British or US. The machine remembers it.
- **Ctrl+Alt+Del** is the emergency reset: it takes you back to the BASIC even
  if a program has hung, and keeps your BASIC program.
- **Ctrl+Alt+F12** shows or hides the diagnostic page (see
  [The firmware](firmware.md#the-diagnostic-page)).

[The machine](../machine.md) explains how the keyboard, mouse and gamepads are
seen by programs.

### The SD card

Insert a microSD card in the slot on the right-hand side. It can be formatted
FAT, FAT32 or exFAT (cards come formatted like this), of any size. You can take
it out and put it back with the machine on. On the board the machine notices
at once: a card that goes in is ready, and a file that was open when it came
out can only be closed (what was already written to it is kept). Close your
files before taking the card out.
The green **SD** LED lights while the machine reads or writes the card.

The built-in examples are not on the card: they are in the `ROM:` drive, which
comes inside the CPU Pico's firmware.

### Power

Plug a USB-C charger into the **5 V** connector at the back, and slide the
power switch (at the front, on the left) to the right, to `ON`; to the left is
`OFF`. A 2 A charger is plenty: the whole machine uses
under 1 A, including the USB devices. The USB-C port only takes power; it does
not carry data.

### The clock and the language

The board has a real-time clock with a CR2032 coin cell. It keeps the date and
time (used to date the files you save), the machine's language and the
keyboard layout while the power is off. Set the time from the BASIC with
`CLOCK`, for example `CLOCK 2026,10,20,18,30` (year, month, day, hour and
minutes).

The machine starts in **English**. Type `LANGUAGE "es"` to switch to Spanish;
with the coin cell in place it is remembered.

## In this section

- **[The board](board.md)**: ordering the PCB assembled from JLCPCB, the parts
  you solder, a checklist, the first power-on, and what every connector and
  control is.
- **[The firmware](firmware.md)**: getting the `.uf2` files, putting them on
  each Pico, updating, building them yourself, and what to check if there is
  no picture or no sound.
- **[On a protoboard](protoboard.md)**: building ZX-V without the PCB, with the
  wiring between the two Picos and to each connector.
