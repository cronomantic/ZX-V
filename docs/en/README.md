**English** · [Español](../es/README.md)

# ZX-V documentation

ZX-V is a 32-bit hobby computer inspired by the ZX Spectrum: switch it on and
you are in BASIC. These pages explain how to use it, how to program it in
BASIC or in C, and how to build the real machine.

## Start here

- **[Getting started](getting-started.md)** — get the emulator, type your
  first program and load the built-in examples.
- **[The emulator](emulator.md)** — the desktop and web versions: menus,
  keys, the SD card folder, snapshots and options.
- **[The machine](machine.md)** — what ZX-V is like inside: the screen and its
  colours, sound, input, memory and the files it uses.

## Programming in BASIC

- **[Learn BASIC](basic/README.md)** — a course from scratch: no programming
  or Spectrum experience needed. It ends with a complete game.
- **[The editor](basic/editor.md)** — typing and editing programs, the keys
  and the options menu.
- **[BASIC reference](basic/reference.md)** — every statement, function and
  error message.

## Programming in C

- **[Native programs (SDK)](sdk/README.md)** — the toolchain, your first
  program, graphics, sound, input and files.
- **[Graphics](sdk/graphics.md)** — the graphics engine in depth: video
  memory, sprites, tile maps, scrolling, patterned shapes, mode 7 and
  converting images.
- **[The SDK libraries](sdk/libraries.md)** — what nearly every game
  needs, in C and optional: fixed-point maths, moving through a map,
  keyboard and gamepad, saving, fonts, fades and 3D.
- **[API reference](sdk/reference.md)** — every function of the SDK.

## Tools for the PC

They come with the emulator, for Windows and Linux:

- **[The graphics editor, `zxv-gfx`](sdk/graphics.md#the-graphics-editor-for-the-pc-zxv-gfx)** —
  sprites, tiles, maps and textures for BASIC and C programs.
- **[The tracker, `zxv-tracker`](sdk/README.md#the-tracker-for-the-pc-zxv-tracker)** —
  music for the two AY chips and the sample channels (`.zxm` songs and PT3
  modules) and AYFX sound effects for BASIC and C programs. It also opens
  MOD, S3M and XM modules, converted; for one with more than 12 channels, it
  asks which ones to keep.
- **[The music converter, `zxv-music`](sdk/README.md#the-own-format-zxm-and-zxv-music)** —
  converts PT3, TurboSound, MOD, S3M and XM to the own format, `.zxm`.
- **[Remote loading, `zxv-link` and `zxv-link-gui`](hardware/firmware.md#loading-programs-from-the-pc-j5)** —
  they send programs from the PC to the board through the J5 header (or [over
  Wi-Fi](hardware/firmware.md#loading-programs-over-wi-fi-pico-2-w), with a
  Pico 2 W) and run them, show its card and ROM like an explorer (send,
  fetch, folders, delete) and serve a folder on the PC as the machine's
  [`EXT:` drive](hardware/firmware.md#the-ext-drive).
- **[Publishing your game on the web](sdk/web.md)** — `mkweb.py` (in the
  SDK) makes a page with only your game, in C or in BASIC, for itch.io or
  your website, with touch controls for phones.

## Compiling BASIC

- **[ZX Basic](zxbasic.md)** — Boriel's BASIC compiler, with a ZX-V target:
  BASIC programs that run like native ones.

## The real machine

- **[Build the hardware](hardware/README.md)** — what you need and how it all
  connects.
- **[The board](hardware/board.md)** — ordering the PCB assembled, and the
  parts you solder yourself.
- **[The firmware](hardware/firmware.md)** — putting the software on the two
  Picos and updating it, and loading programs from the PC through the J5
  header, with `zxv-link` and the `EXT:` drive.
- **[On a protoboard](hardware/protoboard.md)** — building it without the PCB.
- **[The protocol](hardware/protocol.md)** — the `EXT:` drive and remote
  loading through J5, to write your own server.

## Examples

- [A platform game in BASIC](../../examples/basic/platformer) — sprites, a
  tile map, scrolling and music.
- [Rock, Paper, Scissors in BASIC](../../examples/basic/rps) — the block IF,
  `SELECT CASE` and procedures, in a listing you load as text.
- [Sentry, a missile defence game in BASIC](../../examples/basic/sentry) —
  the mouse, fast graphics and chain explosions at 60 frames per second.
- [Estela, a shooter in C](../../sdk/examples/estela) — a complete native
  game.
- [Route 128, a racing game in C](../../sdk/examples/route128) — a pseudo-3D road
  with bends and crests, and scaled sprites.
- [Muncher in BASIC](../../examples/basic/muncher) — a maze with four
  chasers, each hunting you in its own way, with a tile map and sprites at 60
  frames per second.
- [Cascade in BASIC](../../examples/basic/cascade) — a falling-gems puzzle
  whose graphics, music and effects are editable files.
- [The Mine, a digging game in C](../../sdk/examples/mine) — boulders and
  diamonds that fall and roll, in caves bigger than the screen.
- [Street Justice, a brawler in C](../../sdk/examples/street) — through the
  streets against the gangs' thugs, with fighters drawn by the engine from a
  skeleton.
- [Umbra, a 3D maze in C](../../sdk/examples/umbra) — raycast walls, a
  mode 7 floor and a lamp whose light shrinks as its oil runs out.
- [The Keep, a dungeon adventure in C](../../sdk/examples/keep) — four floors
  of rooms seen from above, whose maps you can redraw in `zxv-gfx`.
- [ZXV Chess, chess in C](../../sdk/examples/chess), by ximoKom — against
  the machine or for two players, with the keyboard, a joystick or the mouse.
- [Gone Silent, a 3D adventure in C](../../sdk/examples/gonesilent) — solid
  dithered polygons, pixel-art creatures and a colony to explore in your
  suit.
- [Stardust, a game in ZX Basic](../../examples/zxbasic/stardust) — double
  buffering, sprites and collisions with `zxv.bas`, at 60 frames per second.
- [Petit Soccer, football in ZX Basic](../../examples/zxbasic/petitsoccer) — a
  pitch bigger than the screen with scroll, teams painted from one set of
  sprites, music and effects, against the machine or for two players.
- [Fases, in ZX Basic](../../examples/zxbasic/fases) — two stages of a game,
  each with its code in a bank that is loaded when the game gets there.
- [Nave 3D, in ZX Basic](../../examples/zxbasic/nave3d) — a ship of flat
  faces made as an `.obj`, turning with light among stars.

---

This documentation is licensed under [CC BY 4.0](../LICENSE); the code examples in it
are public domain (CC0), so you can copy them into your programs freely.
