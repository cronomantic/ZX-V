**English** · [Español](../../../es/basic/tutorial/10-sound-and-music.md)

# 10. Sound and music

**In this chapter you will learn** to make simple beeps with `BEEP`, to
play tunes of up to six voices with `PLAY`, and to create sound effects for
games with `SOUND`.

ZX-V has two kinds of sound: the **beeper**, a simple loudspeaker like the
one in the first Spectrum, and two **sound chips** (called AY), like the
one in the Spectrum 128, each able to play three notes at once.

## `BEEP`

`BEEP duration,pitch` plays one note. The duration is in seconds, and the
pitch is the number of semitones above middle C (0 is middle C, 12 is the C
an octave higher, -12 an octave lower):

```
10 BEEP 1,0
20 BEEP 0.5,12
30 BEEP 0.5,-12
```

The program waits while the note plays. With `READ` and `DATA` you can play
a scale:

```
10 FOR i=1 TO 8
20 READ p
30 BEEP 0.25,p
40 NEXT i
50 DATA 0,2,4,5,7,9,11,12
```

## `PLAY`: music in a string

`PLAY` uses the sound chips and takes the music written in a string. The
simplest strings are just note names:

```
10 PLAY "cdefgabC"
```

That is a scale: `c` is C (do), `d` is D (re), and so on; a capital letter
is one octave higher. These are the most useful signs:

| In the string | Meaning |
|---|---|
| `c d e f g a b` | Notes; in capitals, one octave higher |
| `#c`, `$c` | Sharp and flat (the sign goes before the note) |
| `&` | A rest (silence) |
| `1` to `9` | The length of the next notes: `3` a quaver, `5` a crotchet (the starting one), `7` a minim, `9` a semibreve |
| `O n` | Octave, from 0 to 8 (it starts at 5, where `c` is middle C) |
| `T n` | Tempo, in crotchets per minute (60 to 240; it starts at 120) |
| `V n` | Volume, from 0 to 15 |

A length stays in force for the next notes until you change it. Spaces are
ignored, so use them to make the tune easier to read:

```
10 LET m$="T100 O4 5ccgg aa7g 5ffee dd7c"
20 PLAY m$
```

### Several voices

Give `PLAY` several strings, separated by commas, and they sound at the same
time: a tune and its accompaniment.

```
10 LET a$="T120 O5 3cegCegCe 7C"
20 LET b$="O3 7c&e&"
30 PLAY a$,b$
```

The first three strings use the three voices of the first chip; a fourth,
fifth and sixth string use the second chip. Six voices at once:

```
10 PLAY "cdefg","efgab","gabCD","O3c","O3e","O3g"
```

The tempo is the one of the first string. `PLAY` waits until all the strings
have finished; **Esc** stops it.

**If you know the Spectrum:** `PLAY` is the one from the Spectrum 128, with
the same strings, extended to the second chip. The [reference](../reference.md#play)
lists the rest of the signs: envelopes, repeats, triplets and more.

## Sound effects with `SOUND`

Games need sounds that are not notes: explosions, jumps, laser shots.
`SOUND` talks to the sound chip directly. It writes a value into one of the
chip's **registers**, its control knobs: `SOUND register,value`. You can
write several at once, separated by `;`. These are enough to start:

| Register | What it controls |
|---|---|
| 0 and 1 | The pitch of voice A: a **period**, so bigger numbers are lower notes. Register 0 takes 0 to 255, and register 1 adds steps of 256 |
| 6 | The pitch of the noise (0 to 31) |
| 7 | The **mixer**: which voices play tone and which play noise. 62 means "voice A plays a tone"; 55 means "voice A plays noise" |
| 8 | The volume of voice A (0 to 15), or 16 to let the **envelope** control it |
| 11 and 12 | How long the envelope takes |
| 13 | The shape of the envelope: 0 fades out once |

A zap that rises:

```
10 REM A rising zap on channel A
20 SOUND 7,62;8,15
30 FOR p=200 TO 20 STEP -6
40 SOUND 0,p;1,0
50 WAIT 1
60 NEXT p
70 SOUND 8,0
```

Line 20 turns on the tone of voice A at full volume, the loop lowers the
period (so the note goes up) once every fiftieth of a second, and line 70
turns the volume down to 0. `WAIT 1` waits for the next frame of the screen:
you will see it again in the next chapters.

An explosion: noise that fades out by itself, thanks to the envelope:

```
10 REM An explosion: noise on channel A, with the envelope
20 SOUND 6,31;7,55;8,16;11,0;12,40;13,0
30 PAUSE 100
```

Unlike `BEEP` and `PLAY`, `SOUND` does not wait: the program goes on while
the chip plays. That is what games need. The sound keeps playing until you
change it, so remember to silence it with `SOUND 8,0`. Registers 16 to 31
are the same ones for the second chip.

## Try it

1. Play the beginning of a tune you know with `PLAY`.
2. Make a "coin" sound for a game: two quick high notes, one after the other,
   with `SOUND`.

<details>
<summary>Hint</summary>

For the coin, turn on voice A (`SOUND 7,62;8,15`), set a period such as 60,
wait a few frames with `WAIT 4`, set a smaller period (a higher note) such as
40, wait again, and turn the volume down with `SOUND 8,0`.

</details>

<details>
<summary>A solution for the second one</summary>

```
10 SOUND 7,62;8,15
20 SOUND 0,60;1,0: WAIT 4
30 SOUND 0,40;1,0: WAIT 8
40 SOUND 8,0
```

</details>

---

**Reference:** [`BEEP`](../reference.md#beep), [`PLAY`](../reference.md#play),
[`SOUND`](../reference.md#sound), [`WAIT`](../reference.md#wait).

**Next:** [11. Keyboard and joystick](11-keyboard-and-joystick.md) ·
**Previous:** [9. Drawing](09-drawing.md)
