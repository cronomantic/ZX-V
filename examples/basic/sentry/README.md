**English** · [Español](README.es.md)

# Sentry

A game in ZX-V BASIC in the style of *Missile Command*: missiles fall on six cities and you shoot them down
with the mouse, firing from three bases. Each explosion destroys the missiles
it touches, and each missile destroyed explodes in turn: a chain reaction. A
way to see the mouse, the fast graphics and the new BASIC structures (the
block IF, `SELECT CASE` and procedures) in a game that runs at 50 frames per
second. The texts follow the machine's language.

| File | Contents |
|---|---|
| `sentry.bas` | The program, as text with line numbers |

## Trying it

`sentry.bas` is a text listing: there is no need to convert it. Copy it to
the card and type:

```
LOAD "sentry"
RUN
```

It is also on the machine's ROM disk: `LOAD "ROM:sentry"`.

| Control | What it does |
|---|---|
| Mouse | Moves the sight |
| Mouse button | Fires from the nearest base that has missiles |
| Arrow keys and space | The same, with the keyboard |
| Joystick | The d-pad moves the sight and any button fires |
| Esc | Stops the program (BREAK) |

You don't need a mouse: on the title screen, space or a joystick button
starts the game. With the arrow keys or the d-pad, the sight starts slowly,
for aiming, and speeds up while you hold them down.

## The game

- **Waves:** each brings more missiles, faster and more at a time, and is
  worth more points (the multiplier goes up every two). From the third on,
  some missiles split in three halfway down.
- **Bases:** each has ten missiles per wave (the white marks underneath). An
  enemy missile that lands on a base leaves it without ammunition until the
  next wave, when it is rebuilt.
- **At the end of each wave**, points for the missiles left and for the cities
  still standing; every 10,000 points a destroyed city comes back. The game
  ends when none is left.
- **High score** in `SD:sentry.txt` (on the card, even when the game is
  loaded from `ROM:`).
- **Demo:** if you touch nothing on the title screen, after 10 seconds an
  autopilot defends the cities.

## How it works

- **The graphics** are drawn at start-up with `BOX`, `POLY` and `DISC` on the
  hidden page and copied with `GRAB` as sprites: the city and its ruins, the
  base, the sight, the target mark and the whole background (the stars, the
  moon, the mountains and the ground), which is drawn in one go every frame.
  The title's name is a `PRINT` text copied with `GRAB` and drawn three times
  its size with `SPRITE`.
- **Every frame**, on the hidden page: the background, the cities and bases,
  the missile trails (`POLY` with two points is a line), the explosions
  (`DISC`, growing, shrinking and changing colour), the sight and the score;
  then `PAGE` and `WAIT 1`.
- **Collisions** are distances: a missile is inside an explosion if
  `dx*dx+dy*dy` is less than the radius squared. (`x^2` will not do here: with
  a negative `x` it gives `A Invalid argument`, as on the Spectrum.)
- **The tables** of missiles, explosions and targets are two-dimensional
  arrays (array names are a single letter); each column is explained in a
  `REM`.
- **The language:** `PEEK 536872736` and `PEEK 536872737` are the two letters
  of the machine's language, and `DEF FN t$(a$,b$)` picks the text with the
  old Spectrum trick: `a$ AND n` is `a$` if `n` is not 0, and `""` if it is.
- **The sound:** `SOUND` on the second chip (registers 16 to 31): a falling
  tone for the shots and fading noise for the explosions. `PLAY` plays the
  fanfares between waves.

## Speed

Like the Spectrum's, BASIC looks for the line of each jump (`NEXT`,
`UNTIL`, `END PROC`, `PROC`) from the start of the program, and for
variables in the order they were created. Sentry was written when that
mattered (in ZX-V 0.3.0), and it is laid out to run fast; now the
interpreter remembers where it found what it last looked for and the order
hardly matters any more, except with `SPEED 0`, which keeps the 48K's pace.
This is how it is laid out:

- **What is done every frame goes first** (lines 100 to 1999), and what is done
  once (preparing the graphics, the title, the screens between waves), after
  it. With the game loop at the end of the program, it ran at half speed.
- **The arrays and the loop's variables are created first**, right at the
  start.
- **The loops only go through the used slots** of each table.

That way it runs at 50 frames per second up to the eighth wave; after that,
with nine missiles and several explosions at once, the game slows down a
little, as arcade machines did.
