**English** · [Español](../../es/hardware/protoboard.md)

# On a protoboard

Before the PCB existed, ZX-V was built on a protoboard with Pico 2 boards and
a few breakout modules, and that is how its firmware has been tested, one Pico
at a time. **The two Picos have not yet been tested working together**, so the
complete machine described here is still untested. This page explains how to
build it that way.

> **The PCB is the recommended way** ([The board](board.md)): it has
> everything on it, the fast signals (HDMI and the link between the Picos) are
> short and well routed, and you only solder a few parts. A protoboard is
> fine for experimenting, or if you already have the parts, but the wiring is
> long and it needs care.

You can build it in stages and test each one ([Building it step by
step](#building-it-step-by-step)): the ULA Pico alone already shows a picture.

## Parts

| Part | Qty | Notes |
|---|---|---|
| Raspberry Pi Pico 2 | 2 | With pin headers for a breadboard |
| HDMI female breakout board | 1 | The video output |
| 240 Ω resistors | 8 | One in series with each HDMI signal |
| 33 Ω resistors | 12 | In series with the link signals (optional with very short wires) |
| 1 kΩ, 10 kΩ and 100 kΩ resistors; 10 nF, 1 nF and 10 µF capacitors | 2 of each | The audio output filter |
| 3.5 mm stereo jack, female | 1 | The audio output |
| microSD module (SPI, 3.3 V) | 1 | |
| USB-A female connector or breakout, and a USB hub | 1 + 1 | For the keyboard, mouse and gamepads. Better a powered hub |
| Push button | 1 | The video mode button |
| LEDs and 330 Ω resistors | 2 + 2 | SD card activity (CPU) and link status (ULA) |
| Clock: a DS3231 module, or a PCF8563 (or BM8563) with a 32.768 kHz crystal, a BAT54C, a CR2032 cell and its holder | 1 | Optional: the date and time, the language and the keyboard layout |
| Schottky diodes (1N5817 or similar) | 2 | Only with a single 5 V supply (see [Power](#power)) |
| 500 mA resettable fuse | 1 | For the USB-A's 5 V, with a single 5 V supply |
| 5 V, 2 A power supply | 1 | Only with a single 5 V supply |
| Push button | 1 | Optional: reset |

## The Pico pins

Both Picos use **the same pin numbers for the link** (GP4 to GP4, GP26 to
GP26...), which makes the wiring easier. Physical pin numbers are counted
round the Pico as usual: pin 1 (GP0) and pin 40 (VBUS) are at the micro-USB
end. Here are all the pins ZX-V uses:

| Pin | GPIO | CPU Pico | ULA Pico |
|---|---|---|---|
| 1 | GP0 | SD card MISO | Mode button |
| 2 | GP1 | SD card CS | Link LED |
| 4 | GP2 | SD card SCK | Audio left |
| 5 | GP3 | SD card MOSI | Audio right |
| 6, 7, 9, 10, 11, 12, 14, 15 | GP4–GP11 | Link D0–D7 (out) | Link D0–D7 (in) |
| 16 | GP12 | Debug serial TX | HDMI D0+ |
| 17 | GP13 | Debug serial RX | HDMI D0− |
| 19 | GP14 | USB D+ | HDMI CLK+ |
| 20 | GP15 | USB D− | HDMI CLK− |
| 21 | GP16 | Clock SDA | HDMI D2+ |
| 22 | GP17 | Clock SCL | HDMI D2− |
| 24 | GP18 | Return channel (in) | HDMI D1+ |
| 25 | GP19 | — | HDMI D1− |
| 26 | GP20 | Sound line (out) | Return channel (out) |
| 27 | GP21 | — | Sound line (in) |
| 29 | GP22 | SD card LED | — |
| 30 | RUN | Reset (optional) | Reset (optional) |
| 31 | GP26 | Link CLK (out) | Link CLK (in) |
| 32 | GP27 | Link VBLANK (in) | Link VBLANK (out) |
| 36 | 3V3 | 3.3 V for the SD card and the clock | — |
| 39 | VSYS | 5 V in, through a diode (single supply) | 5 V in, through a diode (single supply) |
| 40 | VBUS | 5 V for the USB-A (see [Power](#power)) | — |
| 3, 8, 13, 18, 23, 28, 33, 38 | GND | Ground | Ground |

Note that GP12 to GP19 mean different things on each Pico: on the ULA they
are the HDMI output, on the CPU the debug port, USB and clock. Do not join
them.

## Power

All the grounds must be joined. There are two ways to power it:

**The simplest, for testing: each Pico from its own micro-USB**, from the
computer or from two USB chargers. Join the grounds of the two Picos. The
USB-A port takes its 5 V from the CPU Pico's VBUS (pin 40), which is enough
for a keyboard and a mouse; with a powered hub, better.

**A single 5 V supply (like the PCB):**

- The supply feeds **VSYS (pin 39) of each Pico through a Schottky diode**
  (the anode to the supply, the cathode, the end with the stripe, to VSYS).
  This way you can plug in a Pico's micro-USB at the same time (to update it,
  or for the serial port) without its 5 V going back into the supply.
- The supply also feeds the **USB-A's 5 V (VBUS) through the 500 mA
  resettable fuse**.

If you feed the supply into the CPU Pico's **VBUS (pin 40)** instead, without
diodes, you **must disconnect the supply before plugging in that Pico's
micro-USB** (to flash it, or for the serial port), or its 5 V would reach your
computer.

## The link between the two Picos

Short wires (**at most about 20 cm**), with **at least three ground wires**
spread among the data wires. Each signal has a 33 Ω resistor in series,
placed **next to the Pico that sends it**. With very short wires you can try
without the resistors; if the link shows errors (see
[No picture](firmware.md#no-picture)), add them.

| CPU Pico | Signal | ULA Pico | 33 Ω next to |
|---|---|---|---|
| GP4 (pin 6) | D0 → | GP4 (pin 6) | CPU |
| GP5 (pin 7) | D1 → | GP5 (pin 7) | CPU |
| GP6 (pin 9) | D2 → | GP6 (pin 9) | CPU |
| GP7 (pin 10) | D3 → | GP7 (pin 10) | CPU |
| GP8 (pin 11) | D4 → | GP8 (pin 11) | CPU |
| GP9 (pin 12) | D5 → | GP9 (pin 12) | CPU |
| GP10 (pin 14) | D6 → | GP10 (pin 14) | CPU |
| GP11 (pin 15) | D7 → | GP11 (pin 15) | CPU |
| GP26 (pin 31) | CLK → | GP26 (pin 31) | CPU |
| GP20 (pin 26) | Sound line → | GP21 (pin 27) | CPU |
| GP27 (pin 32) | ← VBLANK | GP27 (pin 32) | ULA |
| GP18 (pin 24) | ← Return channel | GP20 (pin 26) | ULA |
| GND (pins 8, 13, 18...) | Ground, 3 wires or more | GND | — |

- Without the **sound line** there is a picture but no sound.
- Without the **return channel** everything works, but programs are always
  told the output is HDMI at 50 Hz, and the CPU's report warns about it.

## HDMI

On the ULA Pico. One 240 Ω resistor in series with each of the
eight signals, **as close to the Pico as possible**. If your breakout has its
pins labelled (D0+, CK−...), just follow the Signal column.

| ULA Pico | Signal | HDMI connector pin |
|---|---|---|
| GP12 (pin 16) | D0+ | 7 |
| GP13 (pin 17) | D0− | 9 |
| GP14 (pin 19) | CLK+ | 10 |
| GP15 (pin 20) | CLK− | 12 |
| GP16 (pin 21) | D2+ | 1 |
| GP17 (pin 22) | D2− | 3 |
| GP18 (pin 24) | D1+ | 4 |
| GP19 (pin 25) | D1− | 6 |
| GND (pin 18 or 23) | Ground | 2, 5, 8, 11 and 17 |
| 5 V (optional) | Power for the screen's identification chip | 18 |

- **Keep the wires short** (under 10 cm) and each pair (+ and −) together, or
  twisted. These signals carry hundreds of millions of bits per second.
- **Pin 18 (5 V):** many TVs need it to detect that something is connected.
  Take it from the ULA's VBUS (pin 40) when the ULA is powered by its
  micro-USB, or from your 5 V supply. The PCB puts a Schottky diode in this
  line so the TV cannot feed power back into the board.
- Use a short HDMI cable, without adapters.

## Audio

On the ULA Pico: GP2 (pin 4) is the left channel and GP3 (pin 5) the right.
Each channel goes through a two-stage filter and a capacitor to the jack:

```
GP2 ──[1 kΩ]──┬──[10 kΩ]──┬──|+ 10 µF|──┬── jack: tip (left)
            10 nF        1 nF         100 kΩ
              │           │            │
             GND         GND          GND
```

The same for GP3, to the jack's **ring** (right). The jack's **sleeve** goes to
GND (pin 3 or 8). The 10 µF capacitor is polarised: its `+` goes towards the
Pico. The output is line level, for powered speakers or a TV's audio input.

For a quick test, a 1 kΩ resistor and the 10 µF capacitor are enough, although
you will hear some hiss.

## The mode button and the link LED

On the ULA Pico.

| ULA Pico | Connect to |
|---|---|
| GP0 (pin 1) | A push button, whose other side goes to GND (pin 3) |
| GP1 (pin 2) | A 330 Ω resistor, then an LED (anode), its cathode to GND |

The button needs no resistor: the Pico has one inside. What the button and the
LED do is explained in [The firmware](firmware.md#no-picture).

## The SD card

On the CPU Pico. A 3.3 V microSD module (or a 5 V one, with a regulator and
level shifters, powered from 3.3 V):

| microSD module | CPU Pico |
|---|---|
| MISO (DO) | GP0 (pin 1) |
| CS | GP1 (pin 2) |
| SCK (CLK) | GP2 (pin 4) |
| MOSI (DI) | GP3 (pin 5) |
| VCC | 3V3 (pin 36) |
| GND | GND (pin 3) |

**The activity LED:** from GP22 (pin 29), a 330 Ω resistor and an LED to
ground. It lights while the machine uses the card or the clock.

## The keyboard, mouse and gamepads (USB-A)

On the CPU Pico. A female USB-A connector (a breakout, or the end of a cut USB
extension cable):

| USB-A | CPU Pico |
|---|---|
| D+ (green wire) | GP14 (pin 19) |
| D− (white wire) | GP15 (pin 20) |
| VBUS (red wire) | VBUS (pin 40), or the 5 V supply through the 500 mA fuse |
| GND (black wire) | GND (pin 18 or 23) |

D+ and D− go straight to the Pico, or through a 22 Ω resistor each (the PCB
has them); no other resistors are needed. Plug in a keyboard, or a hub with the
keyboard, a mouse and one or two gamepads.

## The clock (optional)

On the CPU Pico. Without a clock the machine works, but forgets the date and
time, the language and the keyboard layout when switched off.

The easiest is a **DS3231 module** for 3.3 V, with its CR2032 cell (a 5 V
module with a regulator also works if its pull-up resistors go to 3.3 V):

| Clock | CPU Pico |
|---|---|
| SDA | GP16 (pin 21) |
| SCL | GP17 (pin 22) |
| VCC | 3V3 (pin 36) |
| GND | GND (pin 23) |

A **PCF8563** (or BM8563) chip, as on the PCB, connects the same way, but
needs more parts around it: a 32.768 kHz crystal with a 12.5 pF load next to
the chip, 4.7 kΩ pull-up resistors from SDA and SCL to 3.3 V, and a BAT54C
diode pair that feeds the chip from the 3.3 V and from the CR2032 cell, with a
100 nF capacitor. The firmware looks for a DS3231 first and then for a
PCF8563, and uses the one it finds.

## Reset button and debug port (optional)

- **Reset:** a push button from the **RUN** pin (pin 30) of each Pico to
  ground restarts it. The PCB joins both RUN pins to a single button.
- **Debug serial port:** the CPU Pico's GP12 (pin 16) is TX and GP13 (pin 17)
  is RX, at 921600 baud and 3.3 V. Cross them to a 3.3 V USB-serial adapter
  (TX to RX). The same messages come out of the micro-USB anyway.

## Building it step by step

1. **The ULA Pico alone.** Wire the HDMI and the audio, and flash the
   `zxv_ula.uf2` test ([The firmware](firmware.md#test-programs)). You should
   see some demos and hear music, on the screen and on the jack. Add the mode
   button and the LED, and try the modes.
2. **The two Picos.** Wire the link, flash `zxv_cpu.uf2` on the CPU Pico and
   `zxv_ula_link.uf2` on the ULA Pico. The BASIC should appear on the screen.
   Open the CPU Pico's serial port with a terminal: what you type there
   reaches the machine, and every 5 seconds it writes a report. (This is the
   step that has not been tested yet: if it fails, the report on the serial
   port is the place to start.)
3. **The SD card, the USB-A and the clock**, one at a time, checking each in
   the report (`tarjeta`, `USB`, `reloj`) and from the BASIC: `CAT` for the
   card, typing for the keyboard, and `CLOCK 2026,10,20,18,30` then
   `PRINT CLOCK 3;":";CLOCK 4` for the clock.

If something does not work, see
[If there is no picture or no sound](firmware.md#if-there-is-no-picture-or-no-sound).
