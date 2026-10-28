**English** · [Español](README.es.md)

# Muncher

A maze game in ZX-V BASIC in the style of *Pac-Man*: eat every dot in the maze while
four chasers hunt you down, each in its own way. A power pill turns the tables
for a few seconds. A way to see tile
maps, sprites over a still background, double buffering and the new BASIC
structures (the block IF, `REPEAT` and procedures) in a game that runs at 50
frames per second. The texts follow the machine's language.

| File | Contents |
|---|---|
| `muncher.bas` | The program, as text with line numbers |

## Trying it

`muncher.bas` is a text listing: there is no need to convert it. Copy it to
the card and type:

```
LOAD "muncher"
RUN
```

It is also on the machine's ROM disk: `LOAD "ROM:muncher"`.

| Control | What it does |
|---|---|
| Arrow keys or O, P, Q, A | Left, right, up and down |
| Joystick | The d-pad moves; any button starts the game on the title screen |
| Space or Enter | Starts the game on the title screen |
| Esc | Stops the program (BREAK) |

The last direction you press is remembered: press it before reaching a
crossing and you turn as soon as the corridor allows it.

## The game

- **The chasers** leave their house in the middle one after another, and
  each has a character:
  - **Rufo** (red) goes straight for you.
  - **Lila** (magenta) aims four squares ahead of you, to cut you off.
  - **Niebla** (cyan) guards her corner (bottom left): she only chases you
    when you come into her part of the maze.
  - **Chispa** (green) chases you, but at many crossings he takes any way.
- **Retreat and chase:** every so often (7, 20, 7, 20, 5, 20 and 5 seconds)
  they stop chasing and each goes off to its own corner for a while; at every
  change they turn round.
- **Power pills** (the four big dots, 50 points): the chasers turn blue, turn
  round and run away; you can eat them for 200, 400, 800 and 1600 points.
  They flash white just before they recover. An eaten chaser goes back home
  as a pair of eyes and comes out again.
- **The tunnel** on the sides takes you to the other side; the chasers slow
  down in it.
- **Fruit:** twice per level (after 70 and 170 dots) a fruit appears under the
  house for ten seconds: a cherry (100 points), a lemon (300), grapes (500), a
  mushroom (1000)... The one for the current level is shown at the top.
- **Levels:** after all 224 dots, the walls flash and the next level begins,
  with faster chasers and a shorter fright. Three lives, and one more at
  10,000 points.
- **High score** in `SD:muncher.txt` (on the card, even when the game is
  loaded from `ROM:`).
- **Demo:** if you touch nothing on the title screen, after 10 seconds an
  autopilot plays: at each crossing it looks down every corridor, counts the
  dots it sees and keeps away from the chasers.

## How it works

- **The maze** is written in `DATA` as text, one row per string (`#` wall,
  `.` dot, `o` power pill, `=` the door of the house), only the left half: the
  right half is its mirror image. From it come a table of where one can walk
  (`m`) and the tile map.
- **The walls** are drawn once at start-up on the hidden page: each wall
  square gets a line along the sides that face a corridor, set in two pixels,
  with rounded corners. `TILE 8,100,0,8,256,184` cuts the whole picture into
  tiles (one per square), and the map uses them for the walls and three more
  tiles for the empty corridor, the dot and the pill. Eating a dot is a
  `MAP POKE` with the empty tile; `CELL` says what there is in a square.
- **Every frame**, on the hidden page: `MAP 0` draws the whole maze in one go,
  then the five sprites and the score with `PRINT`; then `PAGE` and `WAIT 1`.
  Nothing has to be erased, because the map covers everything.
- **The sprites** are 10 by 10 pixels and are written in `DATA` as strings of
  hexadecimal digits (`GRAB n,10,a$`). The player looking up is the one
  looking right turned a quarter (`PROC gira`); left and down are mirror
  images (`SPRITE`'s fourth argument). The chasers are one red body with the
  eyes for each direction laid on top, and `SPRITE`'s colour string paints it
  in each one's colour. The disappearing player is a `DISC` with a mouth cut
  out with `POLY`, wider and wider.
- **Movement** goes pixel by pixel; in the centre of each square the chaser
  picks the neighbouring square closest to its target (never turning round),
  as in the arcade game, and the player turns if it can. Speeds are fractions
  of a pixel per frame, accumulated (`f(i)`).
- **The sound:** `SOUND` on the second chip: the dots and the effects on
  channel A, and a siren on channel B that changes with the fright and when
  there are eyes going home. `PLAY` plays the tune at the start of each level
  on the first chip.
- **The language:** `PEEK 536872736` and `PEEK 536872737` are the two letters
  of the machine's language, and `DEF FN t$(a$,b$)` picks the text with the
  old Spectrum trick: `a$ AND n` is `a$` if `n` is not 0, and `""` if it is.

## Speed

The game runs at 50 frames per second (measured in the emulator with
`TIME`); the demo, whose autopilot thinks more, at 49 or 50. To keep it so:

- **What is done every frame goes first** (lines 100 to 2640), and what is
  done once (the graphics, the title, the screens between levels), after it.
- **Little work per frame:** each actor moves one pixel and only thinks in
  the centre of a square, once every eight pixels; the maze, the sprites and
  the colour changes are graphics commands that the ROM does in one go.
- **Integer maths:** directions are tables (`u`, `v`, `o`), and distances are
  `dx*dx+dy*dy` in squares, without `SQR`.
- **Variables in the hot loop are created first**, right at the start.
