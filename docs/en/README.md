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
  memory, sprites, tile maps, scrolling, mode 7 and converting images.
- **[API reference](sdk/reference.md)** — every function of the SDK.

## Tools for the PC

They come with the emulator, for Windows and Linux:

- **[The graphics editor, `zxv-gfx`](sdk/graphics.md#the-graphics-editor-for-the-pc-zxv-gfx)** —
  sprites, tiles, maps and textures for BASIC and C programs.
- **[The tracker, `zxv-tracker`](sdk/README.md#the-tracker-for-the-pc-zxv-tracker)** —
  music for the two AY chips (PT3 modules) and AYFX sound effects for
  BASIC and C programs.
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
  Picos.
- **[On a protoboard](hardware/protoboard.md)** — building it without the PCB.

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
- [Stardust, a game in ZX Basic](../../examples/zxbasic/stardust) — double
  buffering, sprites and collisions with `zxv.bas`, at 60 frames per second.

---

This documentation is licensed under [CC BY 4.0](../LICENSE); the code examples in it
are public domain (CC0), so you can copy them into your programs freely.
