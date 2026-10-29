**English** · [Español](README.es.md)

# Stardust

A small game for ZX-V written in ZX Basic, Boriel's compiler, with its ZX-V
target: catch the falling gems and dodge the rocks. It is an example of the
`zxv.bas` library, which brings to ZX Basic the ZX-V BASIC's own commands with
the same names: double buffering (`SCREEN` and `PAGE`), `WAIT`, sprites from a
string (`GRABSTR`) with pixel-exact collisions (`HIT`), `BOX`, `KEY`, `JOY` and
`SOUND`. It runs at 60 frames per second (`HZ`). Everything moves a whole
number of pixels each frame, so it moves smoothly, a fifth quicker than when
the machine made 50; new gems and rocks still come at the same rate per
second, so there are slightly fewer of them on screen.

| File | Contents |
|---|---|
| `stardust.bas` | The program, in ZX Basic |

## Building it

You need the `zxv` branch of the ZX Basic fork (`cronomantic/zxbasic`) and
the RISC-V C compiler, the same one the SDK uses:

```
zxbc --arch zxv stardust.bas
```

That gives `stardust.zxv`: copy it to the card and load it with
`LOAD "stardust"`.

| Control | What it does |
|---|---|
| Cursor left and right, or O and P | Move the ship |
| Joystick | The D-pad moves; fire starts the game on the title screen |
| Space | Starts the game on the title screen |

Each gem gives 10 points; each rock takes one of your three lives. The more
points, the more things fall, and the faster.
