**English** · [Español](README.es.md)

# Cascade

A falling-gems puzzle in ZX-V BASIC. Pairs of gems drop into a well six
squares wide and twelve deep. Line up three or more of the same colour,
across, down or diagonally, and they vanish; the gems above fall, and if
they make new lines the chain scores more. It shows a tile map that changes
as you play, sprites for what moves, double buffering, and graphics, music
and effects loaded from files you can edit, all at 60 frames per second. The
texts follow the machine's language.

| File | Contents |
|---|---|
| `cascade.bas` | The program, as text with line numbers |
| `cascade.gfx` | The graphics: gems, the well, the wall and the title |
| `cascade.pt3` | The music, a Pro Tracker 3 module |
| `cascade.afb` | The sound effects, an AYFX bank |

## Trying it

`cascade.bas` is a text listing: there is no need to convert it. Copy the
four files to the same folder on the card and type:

```
LOAD "cascade"
RUN
```

It is also on the machine's ROM disk: `LOAD "ROM:cascade"`, then `RUN`.

| Control | What it does |
|---|---|
| Arrow keys left and right, or O and P | Move the pair (hold to keep moving) |
| Up arrow, Q or Space | Rotate clockwise |
| Z | Rotate anticlockwise |
| Down arrow or A | Drop faster |
| Joystick | The d-pad moves and drops, up and button 1 rotate clockwise, button 2 anticlockwise; any button starts the game on the title screen |
| M | Music on and off |
| Space or Enter | Starts the game on the title screen |
| Esc | Stops the program (BREAK) |

## The game

- **The pair** appears above the well and falls slowly. You can move it
  and rotate it while it falls; when it touches the bottom or other gems,
  you still have a third of a second to slide it before it settles. If the
  pair lies flat and only one gem is supported, the other one keeps falling
  down its own column.
- **Lines:** three or more gems of the same colour in a row, across, down or
  along either diagonal, flash and vanish. Everything above falls, and if
  that makes new lines they vanish too: a **chain**. Each gem is worth 10
  points, times the step of the chain (×2, ×3...) and times the level.
- **The prism** (a rainbow diamond) turns up now and then as one of the two
  gems of a pair. When it lands it clears every gem of the colour it lands
  on, and itself (100 points more). On the floor it only clears itself.
- **The gems** have different shapes as well as colours: a round ruby, a
  hexagonal emerald, a diamond sapphire, a triangular topaz, a square
  amethyst and a star aquamarine.
- **Levels:** every 30 gems cleared the level goes up and the pairs fall
  faster. Levels 1 and 2 use four colours, from level 3 five and from level 6
  six.
- **The end:** when a gem is left above the top of the well, the gems turn
  to stone from the bottom up.
- **High score** in `SD:cascade.txt` (on the card, even when the game is
  loaded from `ROM:`).
- **Demo:** if you touch nothing on the title screen, after 10 seconds an
  autopilot plays: for each pair it tries every column and every rotation,
  and keeps the one that makes the longest lines without piling up too high.

## Changing the graphics, the music and the effects

The program builds nothing itself: everything you see and hear is in the
three files, and it loads them at the start with `LOAD "cascade" GRAPHICS`,
`MUSIC` and `SFX`. `LOAD` looks for each one in the current folder first
and, if it is not there, in the program's folder (`ROM:`, if it was loaded
from there). So to play the ROM game with your
own graphics, put your `cascade.gfx` in the card's current folder and type
`LOAD "ROM:cascade"`. If a file is missing or damaged, the program says
which one and stops with the `LOAD` error (`T File not found` or `R Tape
loading error`).

`cascade.gfx` opens in `zxv-gfx` (on the PC) and in `gfxedit`
(`LOAD "ROM:gfxedit"`, on the machine). Keep the numbers, because the program
uses them:

| Object | What it is |
|---|---|
| Sprites 1 to 7 | The gems of the falling pair and of the title: 1 ruby, 2 emerald, 3 sapphire, 4 topaz, 5 amethyst, 6 aquamarine, 7 the prism (colour 8 is transparent) |
| Sprite 20 | The name on the title screen, drawn twice its size |
| 16 × 16 tiles 0 to 10 | The well: 0 empty, 1 to 7 the gems as above, 8 the flash and 9 the sparkle of a gem that vanishes, 10 the stone of the end |
| 8 × 8 tiles 1 and 2 | The bricks of the wall |
| Map 1 | The wall: one column of 24 bricks, drawn on both sides of the well |

`cascade.pt3` and `cascade.afb` open in `zxv-tracker`, the module in the
music tab and the bank in the effects tab. The effects are numbered:

| Effect | When |
|---|---|
| 0 | The pair moves |
| 1 | The pair rotates |
| 2 | Gems land |
| 3, 4, 5, 6 | A line vanishes: the first step of a chain, the second, the third, the fourth and beyond |
| 7 | The prism |
| 8 | Next level |
| 9 | Game over |
| 10 | New high score |
| 11 | *Ready!* at the start of a game |

The music plays on the first AY chip and the effects on the second, so they
do not get in each other's way.

## How it works

- **The well** is a table `g` of 8 × 14: columns 2 to 7 and rows 2 to 13 are
  the squares you see, row 1 is the one above the well (where the pairs
  appear) and columns 1 and 8 and row 14 are the walls and the floor (99), so
  that no search needs to check where the well ends. `n` counts the gems in
  each column: as the columns are always packed down, a gem coming down
  column `c` stops in row `13-n(c)`.
- **On screen** the well is map 0, of 6 × 12 tiles of 16 × 16 pixels:
  `MAP POKE` changes a square when a gem lands, flashes or vanishes, and
  `MAP 0` draws it all in one go every frame. On top go the falling pair (two
  sprites, pixel by pixel) and the gems that fall after a line vanishes.
- **Whether a gem fits** is a function, `DEF FN f(c,y)`: the lowest row the
  gem touches at height `y` (in pixels) is above the top of column `c`.
  Moving and rotating try it with the two gems; rotating against a wall moves
  the pair aside, and if there is no room to lie flat, the two gems swap.
- **Lines** are looked for from the first gem of each run (the one before it
  in that direction is another colour), in four directions held in the tables
  `w` and `z`. Gems in a line are marked in `m`, and only after looking at
  the whole well are they cleared, so a gem can be in two lines at once.
- **Gravity** packs each column down and remembers which gems change row;
  they fall as sprites, faster and faster, and go back into the map when
  they arrive.
- **The language:** `PEEK 536872736` and `PEEK 536872737` are the two letters
  of the machine's language, and `DEF FN t$(a$,b$)` picks the text with the
  old Spectrum trick: `a$ AND n` is `a$` if `n` is not 0, and `""` if it is.
- **The test** (`emulator/tests/cascade_test.py`) replaces line 5220 to start
  with the well already filled (`p$`) and a known first pair (`q$`), and so
  checks a chain of three steps.

## Speed

The game runs at 60 frames per second (measured in the emulator with
`TIME`). When a pair lands, looking for lines can make one frame late, about
once every 250 frames. To keep it so:

- **What is done every frame goes first** (lines 100 to 2430), and what is
  done once (loading, the title, the end of the game), after it.
- **Little work per frame:** the falling pair only moves two sprites; the
  well is only searched when something lands, and the autopilot thinks once
  per pair.
- **Variables in the hot loop are created first**, right at the start.
