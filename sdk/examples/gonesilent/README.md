**English** · [Español](README.es.md)

# Gone Silent

A 3D adventure in C for ZX-V. The Better Worlds mining colony has not
answered for weeks; the Colonial Rescue Service sends you to find out what
happened, but a storm forces you into a crash landing and the lander will
not take off without four parts. Explore the colony in your suit, read what
the colonists left behind and find your way home.

The world is built from solid polygons with dithered shading, like the 3D
games of the eighties, and the creatures and items are pixel-art sprites.

```sh
make -C sdk                       # build/gonesilent.zxv
make -C sdk run SD=my_card        # and in BASIC: LOAD "gonesilent"
```

It is also on the machine's ROM disk: `LOAD "ROM:gonesilent"`.

## The game

- **The colony:** the outside, with the lander and the storms, and four
  installations: the habitation block (with the greenhouse and the
  workshop), control and communications, the mine (three levels down) and the
  terraforming plant. Some doors need a card, a code, power or something to
  open them with.
- **The suit:** its energy drains when you walk outside, fire, get hit,
  breathe toxic air or stand in a storm, and it recharges on the charging
  pads. If it runs out you go back to the last pad you used, keeping
  everything you found.
- **The creatures** each hunt in their own way (stalking, ambushing or
  chasing you) and rear up before they strike. The defence turrets fire while
  they are active.
- **The motion tracker** in the visor beeps at whatever moves in front of
  you; storms confuse it.
- **The logs** left by the colonists tell what happened and sometimes give
  you what you need to go on. The ending depends on what you did.
- **The map** (M) shows what you have explored of the zone you are in.
- **The game is saved** when you reach a charging pad, in
  `SD:gonesilent.sav`; the title lets you continue or start again.
- **Two video modes:** Tab switches between 256 × 192 and 512 × 384.
- **Text in English or Spanish**, following the machine's language.

| Key | Joystick | |
|---|---|---|
| Cursor up and down, or Q and A | Up and down | Walk |
| Cursor left and right, or O and P | Left and right | Turn |
| Z and X | | Sidestep left and right |
| W and S (C, straight ahead) | | Look up and down |
| Space | Button 1 | Fire |
| Enter or U | Button 2 | Use or read what is in front of you |
| I | | What you carry |
| M | | The map |
| H | | Help |
| Tab | | Video mode |
| Esc | | Back to BASIC |

## Its files

The music and the effects are built into the program, but if the folder it
is loaded from has a file with the same name, it uses that one: you can
change them without compiling.

| File | What it holds | Made with |
|---|---|---|
| [`gonesilent.gfx`](gonesilent.gfx) | The sprites: the creatures, the incubator and the items | [`mkcreature.py`](mkcreature.py) and [`mkitems.py`](mkitems.py); touch up with `zxv-gfx` |
| [`gonesilent-title.pt3`](gonesilent-title.pt3) | The title music | [`mksound.py`](mksound.py); edit with `zxv-tracker` |
| [`gonesilent-end.pt3`](gonesilent-end.pt3) | The ending music | `mksound.py`; `zxv-tracker` |
| [`gonesilent.afb`](gonesilent.afb) | The effects, an AYFX bank | `mksound.py`; `zxv-tracker` |

`mkcreature.py` models each creature in 3D, with shapes that blend into each
other, and turns it into pixel art from five angles in every pose (it needs
numpy); `mkitems.py` draws the items. Both rebuild the whole of
`gonesilent.gfx`, so a hand-made touch-up is lost if you run them again. At
build time [`mkspr.py`](mkspr.py) keeps only the non-transparent part of each
sprite row: the sprites take a third of the space and are drawn from there.

## How it is made

- [`world.c`](world.c): the zones, as lists of pieces (boxes, pyramids,
  wedges, prisms and octahedra) with a material on each face; doors, items,
  consoles, lights and furniture are pieces too. And the texts: the logs, the
  introduction and the endings.
- [`render.c`](render.c): the view. Faces are shaded by the way they face,
  with 4 × 4 dither patterns between the tones of each material, and drawn
  back to front (the painter's algorithm); far things fade into the haze.
  Sprites scale with distance.
- [`main.c`](main.c): the game: walking and collisions, the creatures and the
  turrets, the consoles, the lights, the tracker, the map, the saved game and
  the HUD.
- [`video.c`](video.c): the screen, with two palettes through the line table:
  the zone's for the view and the visor's for the HUD.
- [`sound.c`](sound.c): the music on the first AY and the effects on the
  second.
