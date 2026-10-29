**English** · [Español](../../../es/basic/tutorial/11-keyboard-and-joystick.md)

# 11. Keyboard and joystick

**In this chapter you will learn** to read the keyboard without stopping the
program, with `INKEY$` and `KEY`, to read a joystick with `JOY`, and to keep
a steady speed with `WAIT` and `TIME`. These are the controls of every
game.

`INPUT` is fine for questions, but a game cannot stop and wait for Enter: it
has to look at the keyboard and carry on.

## `INKEY$`: which key is pressed?

`INKEY$` gives the key that is being pressed at that moment, as a string, or
`""` (an empty string) if there is none. It never waits.

```
10 PRINT "Press keys (Q to quit)"
20 REPEAT
30 REPEAT: UNTIL INKEY$=""
40 REPEAT: LET k$=INKEY$: UNTIL k$<>""
50 PRINT "You pressed ";k$
60 UNTIL k$="q"
70 PAUSE 0
```

Line 30 waits until no key is pressed, and line 40 waits until one is. Why
both? Because the program is very fast: while your finger is on a key,
`INKEY$` gives that key again and again. Without line 30, a single press
would fill the screen.

A loop can be written on a single line, as in lines 30 and 40: `REPEAT:
UNTIL INKEY$=""` repeats nothing, it just waits.

### Why the programs end with `PAUSE 0`

The machine remembers the keys you press while a program runs, to type them
into the editor afterwards; `INKEY$` and `KEY` look at the keyboard but do
not take them out of that memory. Without line 70, the keys you pressed
would arrive at the editor as soon as the program ended, as if you had just
typed them. `PAUSE` forgets the keys pressed before it and waits for a new
one, so ending with `PAUSE 0` ("press a key to finish") solves it. All the
programs in this chapter that read the keyboard end like that.

## `KEY`: several keys at once

In a game you may hold "right" and "fire" at the same time. `KEY n` gives 1
if the key `n` is held down right now, and 0 if not. You can ask about as
many keys as you like:

| `n` | Key |
|---|---|
| `CODE "a"` ... `CODE "z"`, `CODE "0"` ... `CODE "9"` | Letters and digits |
| 8, 9, 10, 11 | The cursor keys: left, right, down, up |
| 13 | Enter |
| 32 | Space |

This program moves an `@` around the screen. Use **O**, **P**, **Q** and
**A**, or the cursor keys, and **Space** to stop:

```
10 BORDER 0: PAPER 0: INK 7: CLS
20 PRINT AT 21,0;"Move with OPQA or the cursors"
30 LET x=15: LET y=10
40 REPEAT
50 PRINT AT y,x;" "
60 IF (KEY CODE "o" OR KEY 8) AND x>0 THEN LET x=x-1
70 IF (KEY CODE "p" OR KEY 9) AND x<31 THEN LET x=x+1
80 IF (KEY CODE "q" OR KEY 11) AND y>0 THEN LET y=y-1
90 IF (KEY CODE "a" OR KEY 10) AND y<20 THEN LET y=y+1
100 PRINT AT y,x; INK 6;"@"
110 WAIT 3
120 UNTIL KEY 32
130 PAUSE 0
```

Each time round the loop, it rubs out the `@` (line 50), changes `x` and
`y` according to the keys, and prints it again in its new place. Holding two
keys moves it diagonally. The conditions `x>0`, `x<31`... stop it at the
edges.

## A steady speed: `WAIT` and `TIME`

The screen is redrawn 60 times a second; each redraw is a **frame**. `WAIT
n` waits until `n` frames have passed since the last `WAIT`. In the program
above, `WAIT 3` makes each turn of the loop last exactly three frames, so
the `@` moves at the same speed whether the loop has a lot or a little to
do. That is what you want in a game.

`TIME` gives the number of frames since the machine was switched on. Use it
to measure time. This game measures your reflexes:

```
10 RANDOMIZE
20 BORDER 2: PAPER 0: INK 7: CLS
30 PRINT "Wait for green, then press SPACE"
40 PAUSE 50+INT (RND*150)
50 BORDER 4: LET t=TIME
60 REPEAT: UNTIL KEY 32
70 LET t=TIME-t
80 PRINT "Your time: ";INT (t*1000/60);" milliseconds"
90 PAUSE 0
```

Each frame lasts a sixtieth of a second, hence the `t*1000/60`. (`PAUSE`
counts in fiftieths of a second, as on the Spectrum: `PAUSE 50` is one
second.) Without the `PAUSE 0`
of line 90, the space you pressed would send you back to the editor before
you could read your time.

## The joystick: `JOY`

`JOY 0` reads the first joystick or gamepad, and `JOY 1` the second. It gives
0 if nothing is pressed (or nothing is plugged in), and otherwise a number
that adds up these values:

| Value | Direction or button |
|---|---|
| 1 | right |
| 2 | left |
| 4 | down |
| 8 | up |
| 16 | fire |

For example, up and fire together give 8 + 16 = 24. To find out whether one
of those values is in the number, this function is handy: `FN b(j,n)` is 1
when the value 2^n is included in `j` (n = 0 for right, 1 left, 2 down,
3 up, 4 fire):

```
10 DEF FN b(j,n)=INT (j/2^n)-2*INT (j/2^(n+1))
20 CLS
30 REPEAT
40 LET j=JOY 0
50 PRINT AT 0,0;"Right ";FN b(j,0);"  Left ";FN b(j,1);"  Fire ";FN b(j,4)
60 WAIT 5
70 UNTIL INKEY$<>""
80 PAUSE 0
```

To control a game with the keyboard and the joystick, join them with `OR`:
`IF KEY 9 OR FN b(j,0) THEN ...`. In the emulator, a gamepad plugged into
the PC works as joystick 0.

**If you know the Spectrum:** the joystick is also seen as the number keys,
as the Sinclair interface did (6 left, 7 right, 8 down, 9 up, 0 fire), so
old programs that read the keyboard with `INKEY$` or `IN` can be played with
it. `KEY` only sees the real keyboard. The mouse is read with `MOUSE`; see
the reference.

## Try it

1. In the moving `@` program, leave a trail: instead of rubbing out the `@`,
   print a `.` where it was.
2. Turn the reflexes game into a contest: play five rounds and print the
   best time.

<details>
<summary>Hint</summary>

For the first, change the `" "` of line 50. For the second, put lines 20 to
80 inside a `FOR` loop, keep the best time in a variable that starts very
big (`LET best=1000`), and update it with `IF t<best THEN LET best=t`.

</details>

---

**Reference:** [`INKEY$`](../reference.md#inkey), [`KEY`](../reference.md#key),
[`JOY`](../reference.md#joy), [`WAIT`](../reference.md#wait),
[`TIME`](../reference.md#time), [`PAUSE`](../reference.md#pause),
[`MOUSE`](../reference.md#mouse).

**Next:** [12. Sprites and tile maps](12-sprites-and-tile-maps.md) ·
**Previous:** [10. Sound and music](10-sound-and-music.md)
