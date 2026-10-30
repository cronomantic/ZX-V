**English** · [Español](README.es.md)

# Platformer: a game in BASIC

A small platform game written in ZX-V BASIC, to see its extensions working
together: a robot crosses a five-screen level with scrolling, jumps between
platforms, collects coins, avoids spikes and pits, and stomps on the bugs
that patrol it. If you touch nothing on the title screen, it plays by itself.

| File | Contents |
|---|---|
| `platformer.bas` | The program (text with line numbers) |
| `tiles.png` | The eight 8 × 8 tiles, in this order: sky, grass, earth, brick (the three solid ones), coin, spikes, goal and cloud |
| `sprites.png` | The 16 × 16 sprites: the robot (standing and walking) and the bug (two steps) |
| `level.png` | The whole level (1280 × 160) drawn with those tiles |
| `Makefile` | Makes `build/platformer.gfx` and copies the program to `build/` |

## Try it

It comes on the machine's `ROM:` drive: `LOAD "ROM:platformer"` (the program
loads its graphics from the current folder or, if they are not there, from
its own; a `platformer.gfx` in your folder comes before the ROM one).

To build it yourself you need Python 3 (for the graphics, with
`sdk/tools/png2gfx.py`); `run` opens the emulator, `zxv` if it is in your PATH
or the one you give with `ZXV=`:

```sh
make -C examples/basic/platformer run ZXV=path/to/zxv
```

Copy `build/platformer.gfx` and the program to the SD card to play it
elsewhere: `LOAD "platformer"` loads it, and `RUN` starts it.

| Keys | Joystick |
|---|---|
| O, P or the cursor keys: walk | Left and right |
| Q, Space or cursor up: jump | Up or fire |

## How it is made

- **Graphics:** `png2gfx` turns the three PNG files into a single `.gfx`. The
  tile sheet goes first, so the level reuses those tiles and each one keeps
  its number: the program knows that 1–3 are solid, 4 is a coin, 5 are spikes
  and 6 is the goal. To change the level, edit `level.png` with those tiles.
- **Drawing:** each frame is drawn on the hidden page (`SCREEN s`) and shown
  with `PAGE s`; `WAIT 2` keeps a steady 30 frames per second (the machine
  makes 60). Speeds are in pixels per turn of the loop: 2.5 when walking (75 a
  second) and, for jumps, the same parabola as when the game ran at 25. The
  map (`MAP`) is drawn shifted by the camera, then the bugs and the robot
  (`SPRITE`) on top, and the scoreboard (`PRINT AT`) above. `CLIP` limits
  drawing to the level area, which is what gets redrawn every frame: without
  it, a robot falling into a pit would be drawn below and stay there.
- **Collisions:** with the scenery, `CELL` at points on the robot's edge
  (`DEF FN s` says whether a point is solid); between the robot and the bugs,
  `HIT`. A coin is removed from the map with `MAP POKE`.
- **Sound:** the effects write to the sound chip directly with `SOUND` (and
  fade out little by little); the tunes of the title, of losing a life and of
  the ending are `PLAY` strings.
- **Best score:** in `SD:platformer.txt`, at the root of the card (also when
  playing from the `ROM:` drive, which cannot be written to), with `OPEN #4`
  and `INPUT #4`/`PRINT #4`; `ON ERROR` covers the case where it does not
  exist yet.
- **Structure:** procedures (`DEF PROC`), `REPEAT`/`UNTIL`, `ELSE` and labels
  (`@titulo`, `@bichos`).

In the emulator, one turn of the game loop (about 60 statements) fits easily
in two frames.

The program's names (variables, procedures, labels) and its messages are in
Spanish.
