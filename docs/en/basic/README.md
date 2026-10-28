**English** · [Español](../../es/basic/README.md)

# ZX-V BASIC

When you switch ZX-V on, it is ready for you to type in **BASIC**, a
programming language made for learning. You type a line, press Enter, and
the machine does it. A few lines more and you have a program: a drawing, a
tune, a game.

ZX-V BASIC is the BASIC of the ZX Spectrum, the home computer of the 1980s,
with many additions. It runs the programs written for the Spectrum 48K and
the `PLAY` music of the Spectrum 128, and it adds what was missing:

- **Structured programming**: `WHILE` and `REPEAT` loops, `IF` ... `ELSE`,
  procedures with parameters and local variables, labels instead of line
  numbers, and error catching with `ON ERROR`.
- **Graphics for games**: sprites, tile maps with scrolling, filled shapes,
  two screen pages for smooth animation, and a high-resolution mode of
  512 × 384 pixels. Every pixel has its own colour.
- **Sound**: the beeper, plus two sound chips with six voices for music and
  effects.
- **Files**: programs and data on an SD card, in folders, and a `ROM:` drive
  with examples.
- **A full-screen editor**, where you type keywords letter by letter and
  move around the listing with the arrow keys.

## The course

The course teaches BASIC from zero: you do not need to have programmed
before, or to have seen a Spectrum. Each chapter builds on the previous
ones, with small programs to type and try, and exercises at the end.

| Chapter | What you learn |
|---|---|
| [1. First steps](tutorial/01-first-steps.md) | Type a line, `PRINT`, `RUN`, the editor in two minutes, error messages |
| [2. Variables and maths](tutorial/02-variables-and-maths.md) | `LET`, sums and functions, arranging `PRINT`, random numbers |
| [3. Text and input](tutorial/03-text-and-input.md) | Strings, joining and slicing text, asking questions with `INPUT` |
| [4. Decisions](tutorial/04-decisions.md) | `IF`, `THEN`, `ELSE`, comparisons, `GO TO` and labels, a guessing game |
| [5. Loops](tutorial/05-loops.md) | `FOR`, `WHILE`, `REPEAT`, loops inside loops, `PAUSE` |
| [6. Arrays and data](tutorial/06-arrays-and-data.md) | `DIM`, lists of values with `READ` and `DATA` |
| [7. Procedures](tutorial/07-procedures.md) | `DEF PROC`, parameters, `LOCAL`, recursion, `DEF FN`, `GO SUB` |
| [8. Colours and the screen](tutorial/08-colours-and-the-screen.md) | `INK`, `PAPER`, `BORDER`, `PRINT AT`, `MODE 1` |
| [9. Drawing](tutorial/09-drawing.md) | `PLOT`, `DRAW`, `CIRCLE`, and filled shapes with `BOX`, `DISC`, `POLY` |
| [10. Sound and music](tutorial/10-sound-and-music.md) | `BEEP`, music with `PLAY`, sound effects with `SOUND` |
| [11. Keyboard and joystick](tutorial/11-keyboard-and-joystick.md) | `INKEY$`, `KEY`, `JOY`, steady speed with `WAIT` and `TIME` |
| [12. Sprites and tile maps](tutorial/12-sprites-and-tile-maps.md) | `GRAB`, `SPRITE`, smooth animation, `HIT`, tiles, maps and scrolling |
| [13. Files and the SD card](tutorial/13-files-and-the-sd-card.md) | `SAVE`, `LOAD`, folders, the `ROM:` drive, text files, `ON ERROR` |
| [14. A complete game](tutorial/14-a-complete-game.md) | *Star Catcher*, built step by step |

## Also about BASIC

- [The BASIC reference](reference.md): every statement and function, with
  its syntax, details and examples, plus the error messages.
- [The editor](editor.md): all the keys, the EDIT menu, renumbering and
  other editing commands.
- [Getting started](../getting-started.md): how to start ZX-V, the emulator
  or the real machine, for the first time.
