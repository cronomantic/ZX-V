**English** · [Español](README.es.md)

# The Mine

A rocks-and-diamonds digging game in C for ZX-V. The miner digs through
the earth collecting the diamonds each cave asks for before time runs out;
then the exit opens. Boulders and diamonds fall and roll, and whatever
falls on the miner kills him. It uses the ROM's tile maps and sprites,
double buffering and both AY chips.

```sh
make -C sdk                       # build/mine.zxv
make -C sdk run SD=my_card        # and in BASIC: LOAD "mine"
```

It is also on the machine's ROM disk: `LOAD "ROM:mine"`.

## The game

- **Twelve caves**, ten of them normal and two bonus caves, each bigger
  than the screen (40 × 22 cells of 16 × 16 pixels, 640 × 352 pixels): the
  way in, the seams, the embers, the cascade, the rain (bonus), the bats,
  the moss, the sieve, the maze, the cellar (bonus), the avalanche and the
  heart of the mine. The first ones are easy: a lot of earth, few boulders
  and more diamonds than needed.
- **The rules:** boulders and diamonds fall into empty space and roll off
  anything round (boulders, diamonds and brick walls) when there is room at
  the side and below. A boulder resting on the miner's head is harmless;
  one that is already falling, or one he steps out from under and stays
  below, is deadly. Boulders can be pushed sideways (keep pushing for a
  moment). Steel walls are indestructible; everything else, except the
  exit, is blown away by explosions.
- **Two creatures** that follow the walls: embers keep to the wall on
  their left and explode into empty space; bats keep to the wall on their
  right and explode into nine diamonds. Both explode when they touch the
  miner or the moss, and when something falls on them.
- **Moss** grows slowly at first and then faster. If it is enclosed and
  cannot grow any more, it turns into diamonds; if it grows too big, into
  boulders.
- **The sieve** starts working with the first thing that falls on it: for a
  while, a boulder that goes through comes out below as a diamond, and a
  diamond as a boulder. Then it is just another wall.
- **Time, points and lives:** each cave has a time limit (the last ten
  seconds tick); the diamonds needed are worth some points and the extra
  ones more; the seconds left are added at the exit. Three lives, one more
  every 500 points. Losing in a bonus cave costs no life.
- **On the title,** left and right choose the starting cave among the first
  four.
- **High score** in `SD:mine.txt` (on the card, even if the game was loaded
  from `ROM:`, which cannot be written to). Leaving a game with Esc also
  counts. Without a card, it lasts for the session.
- **Demo:** if nothing is touched on the title screen, after 12 seconds an
  autopilot plays one of the first three caves (the next one each time),
  with the title music.
- **Texts in Spanish or English**, following the machine's language
  (`zxv_lang_get`).

| Key | Gamepad | |
|---|---|---|
| Cursor keys or OPQA | D-pad | Move (and dig, collect, push) |
| Space or M + direction | Button 1 or 2 + direction | Dig or collect without moving |
| Enter | Start | Pause |
| Esc | | To the title; on the title, to BASIC |

## How it is made

| File | |
|---|---|
| `mine.h` | What the parts share: sizes, tiles, sprites, sound |
| `cave.h`, `cave.c` | The rules of the cave and the demo's autopilot, with nothing from the machine (it also compiles on the host) |
| `caves.c` | The twelve caves, drawn with characters |
| `draw.c` | The screen: double buffering, the 5 × 7 font (with accents and Ñ) and the sprites |
| `art.c` | The cave tiles, drawn with the engine at start-up, and the miner, drawn with characters |
| `sound.c` | The music (first AY) and the effects (second AY) |
| `game.c` | The game, the scrolling, the title, the demo and the high score |

The source code and its comments are in Spanish.

- **Steps and pixels:** the cave moves in steps of 5 or 6 frames (it
  depends on the cave): it is scanned from top to bottom and each thing
  does its part once. But what moves in a step is drawn between its old
  cell and the new one, frame by frame, so the miner, the falling boulders
  and the creatures glide instead of jumping 16 pixels.
- **The scrolling:** the 256 × 176 window follows the miner pixel by pixel
  when he moves away from the centre, without leaving the cave. Every frame
  the visible cells of a tile map (`zxv_map_draw`, opaque 16 × 16 tiles) are
  drawn on the hidden page, then the sprites of what is moving, and the
  score line on top; the page is shown at the next VBLANK (`zxv_set_page`
  and `zxv_wait_vblank`).
- **Graphics without files:** the earth, the bricks, the steel, the boulders,
  the diamonds (in four sparkles), the exit, the moss, the sieve, the
  explosions and the creatures are drawn with the engine's rectangles,
  circles, lines and polygons on a bitmap (`zxv_gfx_target`), which is also
  the tile store. The earth and the bricks are redrawn with the colours of
  each cave. Boulders, diamonds and creatures have a transparent background
  (colour 8, which the opaque map draws as black), so the same drawings are
  prepared as sprites (`zxv_spr_prepare`). The miner (standing, blinking,
  walking, pushing, climbing and cheering) is drawn with characters.
- **The caves** are lines of characters: walls, steel, boulders, diamonds,
  creatures, moss, the sieve, the entrance and the exit, and `.` for earth
  that is filled at random with boulders, diamonds and holes in each cave's
  proportions, always the same (each cave has its seed). Each cave opens
  from the entrance outwards while its name is shown.
- **The autopilot** searches backwards from its goals (the open exit, the
  diamonds, a boulder to push, earth to dig) what it costs to get there from
  each cell, avoiding the creatures and what is about to fall, and making it
  dearer to dig under a boulder (when the miner leaves, it falls and blocks
  the way). It takes the best step only if a simulation of the next few
  steps (copying the cave) says the miner is still alive, standing still or
  stepping aside.
- **The sound:** the title music goes on the first AY (melody, bass and
  drums with the noise); the effects, on the second: channel A the
  warnings (diamond, exit open, time running out), B the noise of digging,
  pushing, boulders landing and explosions (with the envelope), and C the
  chimes of diamonds landing, the sieve and the bonus count.
- **The keyboard:** the map of keys held down (`ZXV_I_KEYMAP`) is read every
  frame; the last direction pressed wins, and a short press between two
  steps is not lost. The key queue is emptied every frame so that what was
  typed does not reach BASIC on the way back.

## The caves can be done

Built with `make -C sdk -B build/mine.zxv EXTRA_CFLAGS=-DMINA_BOT` and run
with `zxv-cli --test`, the demo's autopilot plays the twelve caves in a row,
with all the drawing, and at the end of each one writes whether it finished
it, in how many seconds, how many diamonds it took, the lives it lost and
the lost frames. It finishes all twelve without losing a life, in 5 to 30
seconds each (the limits are 20 to 170). Because `cave.c` does not depend
on the machine, the same autopilot was also run on the host, faster, and
with a "hesitant player" that only moves one step in two to six when
standing still is safe: it still finishes every cave in time, except, at
some of those speeds, the moss and the heart, where it ends up dithering
next to an ember until time runs out.

## Performance

Built with `make -C sdk -B build/mine.zxv EXTRA_CFLAGS=-DMINA_STATS`, the
score line shows the lost frames. On the emulator (at 144 MHz) no frame is
lost in the twelve caves, with the autopilot simulating the next steps: the
slowest frame takes about 4.4 ms of the 20 available. It has not been
measured on the Pico 2.

