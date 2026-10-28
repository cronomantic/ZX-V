**English** · [Español](README.es.md)

# Rock, Paper, Scissors

A short game in ZX-V BASIC, to see the new structures in a real program: the
**block IF** (`IF ... THEN`, `ELSE IF`, `ELSE`, `END IF`), **`SELECT CASE`**
and procedures (`DEF PROC`). You play against the machine, which chooses at
random; the score is kept until you quit. The texts on screen are in
Spanish.

| File | Contents |
|---|---|
| `rps.bas` | The program, as text with line numbers |

## Trying it

`rps.bas` is a text listing: there is no need to convert it. Copy it to the
card and type:

```
LOAD "rps"
RUN
```

It is also on the machine's ROM disk: `CD "ROM:"` and `LOAD "rps"`. `LIST`
shows the program, with its blocks indented.

| Key | What it does |
|---|---|
| P | Rock (*piedra*) |
| A | Paper (*papel*) |
| T | Scissors (*tijera*) |
| S | Ends the game and says who won |

## How it works

- **The key:** `SELECT CASE k$` sends the key pressed (`INKEY$`) to one of the
  three moves, quitting, no key (`CASE ""`, which waits for a frame) or any
  other (`CASE ELSE`, a beep).
- **Who wins:** with the moves numbered 1, 2 and 3, the difference between
  yours and the machine's, modulo 3, gives the result: 0 a draw, 1 you win,
  2 the machine wins. Another `SELECT CASE`, in `PROC ronda`, shows it and
  adds the point.
- **The end:** a block IF with `ELSE IF` picks one of the three messages.
- **Procedures:** `ronda(t)` gets your move and uses a local variable
  (`LOCAL m`) for the machine's; `marcador` repaints the score. After each
  round, the program waits for you to let go of the key, so as not to play
  several rounds in a row.
