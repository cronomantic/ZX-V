**English** · [Español](README.es.md)

# Petit Soccer

A seven-a-side football game for ZX-V written in ZX Basic, Boriel's compiler,
with its ZX-V target: against the machine or between two players, with two
halves, throw-ins, corners and goal kicks, eight teams and three levels. It is
the big example of the `zxv.bas` library:

- **A pitch bigger than the screen** (`MODE 0, 512, 384`), which the camera
  follows the ball over with `SCROLL`; every frame is drawn whole on the
  hidden page (`SCREEN`, `PAGE`, `CLIP`), 60 of them a second.
- **The players**, standing and in eight directions, with five drawings and
  their mirror (`SPRITE` mirrored); each one is painted with the colours of
  its team and its hair by changing three colours of the same sprite.
  Everything uses the 16 standard colours of ZX-V.
- **The scoreboard texts**, with the machine's font turned into sprites
  (`GRAB`), to write them anywhere on the pitch and at any size.
- **The music and the effects**, from a tracker module and an AYFX bank
  (`MUSICLOAD`, `MUSICPLAY`, `SFXLOAD`, `SFX`).
- Positions are kept in 1/256 of a pixel, with integers (`LONG`), so that
  everything moves smoothly without fractional numbers.

It comes up in the machine's language, English or Spanish.

| File | Contents |
|---|---|
| `petitsoccer.bas` | The program, in ZX Basic |
| `sprites.png` | The sprite sheet: the players, the ball, the shadows and the goal |
| `mksprites.py` | Makes `sprites.bas`, which the program includes, from `sprites.png` |
| `sprites.bas` | The sprites, as `GRABSTR` statements (not edited by hand) |
| `mksound.py` | Makes the music and the effects |
| `petitsoccer.pt3`, `petitsoccer.afb` | The title music and the effects |
| `build.py` | Compiles it, with the music and the effects inside |
| `petitsoccer.zxv`, `petitsoccer.sha256` | The program compiled, and the fingerprints of what it was compiled from |

## Building it

You need the `zxv` branch of the ZX Basic fork (`cronomantic/zxbasic`),
v1.19.0-zxv.6 or later, and the RISC-V C compiler, the same one the SDK uses:

```
python build.py path/to/zxbc
```

That is `zxbc --arch zxv --embed petitsoccer.pt3 --embed petitsoccer.afb
petitsoccer.bas`: it gives `petitsoccer.zxv`, with the music and the effects
inside, so it works alone, from the card or from `ROM:`, where it comes built
in (`LOAD "ROM:petitsoccer"`). A `petitsoccer.pt3` or `petitsoccer.afb` in the
current folder takes the place of the one inside.

The `.zxv` is kept in the repository, because the automatic builds cannot
compile ZX Basic: `build.py` also writes `petitsoccer.sha256`, and a test of the
emulator fails if the sources change and the `.zxv` is not compiled again
(`python build.py --check` says which file changed).

If you retouch `sprites.png` (with any image editor), `python mksprites.py`
makes `sprites.bas` again. The players are in 12 × 18 cells with their feet
on the last row; red is the shirt and the socks, blue the shorts and yellow
the hair, the three colours the game changes. On the right of the sheet is
the goal, standing and with its net: it is the one on the right of the
pitch, and the other one is its mirror. The music and the effects can
be retouched with `zxv-tracker`, or by changing `mksound.py`, which makes
them again from scratch.

## Playing

On the title, up and down choose an option and left and right change it: how
many players, the two teams, the minutes of the match and the level of the
machine. If nothing is touched, a demonstration match starts after a while.

| | Player 1 | Player 2 |
|---|---|---|
| Move | Cursor keys, or joystick 1 | W, A, S and D, or joystick 2 |
| Button | Space, or fire | F, or fire |

You control the player with the arrow: the one with the ball, the one about
to receive it or the nearest one.

| With the ball | |
|---|---|
| A tap of the button | A pass to the team-mate in front |
| The button held | A shot, harder and higher the longer it is held. Facing the goal it goes on target: straight for the centre, and diagonally for a post |

| Without the ball | |
|---|---|
| The button | A sliding tackle: if it reaches the ball, it takes it from the opponent |

At set pieces, choose the direction and release the button. The goalkeeper
plays by himself. There are no offsides and no fouls.
