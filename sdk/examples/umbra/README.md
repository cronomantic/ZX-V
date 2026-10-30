**English** · [Español](README.es.md)

# Umbra

A 3D maze game in C for ZX-V, played in the dark. The only light is your
lamp, and it is slowly going out. Gather each level's runes to open the
exit gate before the oil runs out, and run from the shadows that roam the
corridors. The walls are drawn by raycasting, and the floor and ceiling with
the graphics engine's mode 7. The lamp's fog is made with colour tables.

```sh
make -C sdk                       # build/umbra.zxv
make -C sdk run SD=my_card        # then in BASIC: LOAD "umbra"
```

It is also on the machine's ROM disk: `LOAD "ROM:umbra"`.

## The game

- **Five mazes**, each one bigger than the last and with more shadows: the
  stone crypts, the brick cellars, the moss caves, the crystal grotto and
  the temple. Each one hides some runes. With all of them, the exit gate
  opens and leads to the next maze.
- **The lamp:** a full one lasts 80 seconds. The less oil it has, the less
  you can see. The colours fade, the fog comes closer, and in the end you
  only see what is next to you. Oil flasks refill it. When it runs low, you
  hear a heartbeat.
- **The shadows** roam the maze and smell you eight steps away. Then they
  chase you, and you hear their whisper, louder the closer they are. They
  are faster when your light is dim and in the later levels. A touch drinks
  part of your oil (the border flashes red). The shadow then vanishes and
  comes back somewhere far away. If the lamp goes out completely, you light
  one of your spares; with none left, the game is over. Their red eyes show
  even in the dark.
- **The map:** what you have seen is mapped. At the bottom left you see
  your surroundings; M shows the whole maze (the game stops while you look).
- **Points** for each rune (more in later levels), for flasks and for each
  level cleared, plus the oil you have left. The **high score** is saved in
  `SD:umbra.txt`, on the card even when the game is loaded from `ROM:`.
- **Demo:** leave the title alone for 12 seconds and an autopilot plays. It
  goes for the nearest rune, for oil when it runs low, and for the gate once
  it is open.
- **Texts in English or Spanish**, following the machine's language.

| Key | Gamepad | |
|---|---|---|
| Cursor up or Q | Up | Walk |
| Cursor down or A | Down | Back |
| Cursor left and right, or O and P | Left and right | Turn |
| Z and X | | Sidestep left and right |
| M | Button 2 | The map |
| Space | Button 1 | Start |
| Enter | Start | Pause |
| S | | Music on or off |
| Esc | | To the title; on the title, to BASIC |

## Its levels, graphics and sound, your way

Everything you see and hear is in files that you edit with the ZX-V tools,
and the program carries them inside. If the folder it is loaded from has a
file with the same name, the game uses that one instead. To change a maze, a
drawing or a tune you don't need to compile anything. A file that isn't
valid (or doesn't fit) gives a report and returns to BASIC.

| File | What it holds | Edit it with |
|---|---|---|
| [`umbra.gfx`](umbra.gfx) | The levels, the textures and the sprites | `zxv-gfx` on the PC or `gfxedit` on the machine |
| [`umbra-title.pt3`](umbra-title.pt3) | The title music | `zxv-tracker` |
| [`umbra-maze.pt3`](umbra-maze.pt3) | The maze music | `zxv-tracker` |
| [`umbra-clear.pt3`](umbra-clear.pt3) | Level clear | `zxv-tracker` |
| [`umbra-over.pt3`](umbra-over.pt3) | Game over | `zxv-tracker` |
| [`umbra.afb`](umbra.afb) | The sound effects, an AYFX bank | `zxv-tracker` |

### The levels

Each level is a **map** in `umbra.gfx`. Map 0 is the first level, map 1 the
second, and so on for as many maps as there are in a row (up to 16, each up
to 32 × 32 cells). Each maze cell is a map cell, and the number of its tile
says what is in it. The file's 8 × 8 tiles have a picture of each thing, so
you can see the maze in the editor and paint with them:

| Tile | In the cell |
|---|---|
| 0 | Floor |
| 1 | Stone wall |
| 2 | Brick wall |
| 3 | Moss wall |
| 4 | Crystal wall |
| 5 | Temple wall |
| 6 | Wall with a carved rune |
| 7 | The exit gate (in a wall, reachable from one side) |
| 8 | A rune |
| 9 | An oil flask |
| 10 | A shadow, where it starts |
| 11–14 | The start: where the player begins, facing east, south, west or north |

Outside the map is wall, and so is any number not on this list. Every level
needs one start and one gate. Without runes, the gate is open from the
beginning. The floor and ceiling take the colours of the most common wall in
the level. A level holds up to 24 runes and flasks and 8 shadows.

### Textures and sprites

| Texture | |
|---|---|
| 0 | Walls, 128 × 128, holding four 64 × 64 walls: stone, brick, moss and crystal |
| 1 | Walls, 128 × 128, holding the temple wall, the closed gate, the open gate and the rune wall |
| 2 | The floor, 128 × 128: each 64 × 64 is one cell |
| 3 | The ceiling, 64 × 64 |

The floor and ceiling are drawn with colours 1 (dark), 7 (mid) and 15
(light), which each level replaces with its own; 0 stays black.

| Sprite | |
|---|---|
| 0 | The rune |
| 1 | The oil flask |
| 2–4 | The shadow, animated through all three; colour 10 (bright red) is its eyes, which show in the dark |
| 5 | The lamp in the status bar (also the smaller spare lamps) |
| 6 | The small rune in the status bar |

In the maze, a cell is 64 pixels of a shadow or 96 pixels of an object.

### The effects

By their number in `umbra.afb`:

| Effect | |
|---|---|
| 0 | Taking a rune |
| 1 | Taking a flask |
| 2 | A shadow touches you |
| 3 | The gate opens |
| 4 | The lamp goes out |
| 5 | A step |
| 6 | A beep |
| 7 | The heartbeat |
| 8–10 | A shadow's whisper, from far to near |

The effects play on the second AY; the music plays on the first.

## How it is made

| File | |
|---|---|
| `umbra.h` | What the parts share: sizes, cells, objects and shadows, sound |
| `video.c` | The screen: double buffer, the 5 × 7 font and sprites |
| `art.c` | The graphics and levels from `umbra.gfx` (with `zxv_gfxfile.h`) |
| `view.c` | The 3D view: walls, floor and ceiling, fog and objects |
| `maze.c` | The mazes (the maps), the paths and the shadows |
| `sound.c` | Music and effects, with `zxv_music.h` |
| `game.c` | The player, the lamp, the status bar, the map, the title and the demo |

- **Walls by raycasting:** the view is the top 144 lines. Each 2-pixel
  column (128 in all) casts a ray that steps through the maze grid, from one
  cell edge to the next, until it hits a wall. The distance gives the
  column's height, and where the ray hits gives the texture column. The
  column goes straight into video memory, one byte per line: both pixels of
  the byte in the same colour.
- **Floor and ceiling with mode 7:** the same camera as the rays gives the
  floor's lines (`zxv_m7_camera`, with the engine's own sine table). The
  ceiling uses the lines on the other side of the horizon. Only lines that
  some wall leaves visible are drawn.
- **The fog:** each fog level is a colour table. First the brightness goes,
  then half the pixels turn black (a checkerboard), then three in four, and
  finally everything is black.
  - The walls get their fog column by column.
  - The floor and ceiling get it line by line, with a 256-entry table for
    each byte (its two pixels). The same table also gives them the level's
    colours.
  - The sprites get it through `zxv_spr_draw_scaled`'s colour table.

  The remaining oil sets how far the light reaches.
- **Objects and shadows** are scaled sprites, drawn from far to near. The
  walls in front of them cut them off: knowing each column's distance, a
  sprite is drawn only over the runs of columns where the wall is farther
  away than the sprite (`zxv_gfx_clip`).
- **Paths:** a breadth-first search from the player gives each cell its step
  count to the player. The shadows use it to chase you, and the demo runs
  its own search from what it is looking for.
- **Spare video memory:** in mode 0 the screen only uses 24 KB of each 96 KB
  video page.
  - Page 0's spare memory holds the textures, ready for the engine.
  - Page 1's holds the prepared sprites and whatever is loaded from the card.
- **No floating point:** positions are 16.16 fixed point, in cells; angles
  are in 1/1024 of a turn.
- **At 60 frames per second:** speeds are per second (`ZXV_FRAME_HZ`), and
  whatever is left over in each frame carries to the next.

## Performance

Built with `make -C sdk -B build/umbra.zxv EXTRA_CFLAGS=-DUMBRA_STATS`, the
status bar shows the lost frames and the slowest frame of the last second,
in microseconds. In this build the demo plays every level without using oil. With
`EXTRA_CFLAGS=-DUMBRA_SCORE=9876543` a game starts with that score, to see
the status bar with its largest numbers.
In the emulator (at 144 MHz) a maze frame takes under 7 ms of the 16.7 ms
frame at 60 frames per second. The slowest frame, the one that starts a
level (it builds the floor and ceiling colour tables), takes about 12.5 ms.
No frame is lost. It hasn't been
measured on the Pico 2.

