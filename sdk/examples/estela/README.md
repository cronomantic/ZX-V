**English** · [Español](README.es.md)

# Estela

A three-stage shoot'em up in C for ZX-V: the SDK's example native game. It
uses the ROM's graphics engine (sprites, tile maps, circles, polygons),
both AY chips, and AY writes with a timestamp for a digitised explosion.

```sh
make -C sdk                       # build/estela.zxv
make -C sdk run SD=my_card        # and in BASIC: LOAD "estela"
```

It is also on the machine's built-in `ROM:` drive: `LOAD "ROM:estela"`.

## The game

- **Three stages:** the asteroid belt (with a planet in the background),
  the canyon (rock walls above and below, with turrets) and the fortress
  (metal walls). At the end of each one, after the "DANGER!" warning, a
  boss is waiting: the mother rock (rings of bullets and stones), the
  guardian (aimed bursts and darts) and the core, which can only be hurt
  while it is open.
- **Enemies:** darts that weave, wheels that shoot, rocks that split in
  two, turrets on the floor and the ceiling, mines that chase the ship, and
  carriers that drop power-ups: the weapon (double, triple) and the shield,
  which takes one hit.
- **Three lives**, one more every 20,000 points, and a bonus for those
  left at the end. Losing a life keeps the weapon.
- **High score** in `SD:estela.txt` (on the card, even if the game was
  loaded from `ROM:`, which cannot be written to). Without a card, it lasts
  for the session.
- **Demo:** if nothing is touched on the title screen, after 12 seconds an
  autopilot plays the end of a stage and its boss (the next stage each
  time).
- **Texts in Spanish or English**, following the machine's language
  (`zxv_lang_get`).

| Key | Gamepad | |
|---|---|---|
| Cursor keys or OPQA | D-pad | Move |
| Space or M | Button 1 or 2 | Fire (hold it down for rapid fire) |
| Enter | Start | Pause |
| S | | Music on or off |
| Esc | | To the title; on the title, to BASIC |

## Its graphics and music, your way

The graphics are in [`estela.gfx`](estela.gfx), edited with `zxv-gfx` on
the PC or with `gfxedit` on the machine itself, and the music in Pro
Tracker 3 modules edited with `zxv-tracker`:
[`estela-title.pt3`](estela-title.pt3) (the title),
[`-stage1`](estela-stage1.pt3), [`-stage2`](estela-stage2.pt3) and
[`-stage3`](estela-stage3.pt3) (one for each stage),
[`-boss`](estela-boss.pt3) (the bosses), [`-over`](estela-over.pt3) (game
over) and [`-clear`](estela-clear.pt3) (stage clear). The program carries
them inside, but if there is one with that name in the folder it is loaded
from, it uses that one: changing a drawing or a tune needs no compiling.
One that is not valid (or does not fit) gives a report and goes back to
BASIC.

| Sprite | |
|---|---|
| 0 and 1 | The ship and its shield |
| 2, 3 and 4 | The dart and the wheel (in two positions) |
| 5 and 6 | The big rock and the small one |
| 7, 8 and 9 | The turret, the mine and the power-up carrier |
| 10 | The enemy bullets |
| 11 and 12 | The power-ups: the weapon and the shield |
| 13–16 | The explosion, in four steps |
| 17–19 | The bosses of the three stages |

And the 8 × 8 tiles of the terrain: empty, rock, the top and bottom edges
of the rock, metal, its two edges and a light.

## How it is made

| File | |
|---|---|
| `estela.h` | What the parts share: sizes, sprites, sound, the random number generator and the sine table |
| `draw.c` | The screen: double buffering, the 5 × 7 font (with accents and Ñ), the sprites of `estela.gfx` (with `zxv_gfxfile.h`), collisions and the stars in three layers |
| `sound.c` | The music (first AY, with `zxv_music.h`), the effects (second AY) and the digitised samples |
| `game.c` | The stages, the terrain, the enemies, the bosses, the title, the demo and the high score |

The source code and its comments are in Spanish.

- **Double buffering:** every frame is drawn in full on the hidden page
  (`zxv_gfx_draw_page`), which is shown at the next VBLANK
  (`zxv_set_page` and `zxv_wait_vblank`), 60 times a second. A lost frame
  shows up in `ZXV_SV_FRAMES`.
- **Time:** the game was tuned when the machine ran at 50 frames per
  second, and at 60 (`ZXV_FRAME_HZ`) everything takes the same time in
  seconds: speeds are still written in 1/16 of a pixel every 1/50 of a
  second and `V()` turns them into a step per frame; times are in
  milliseconds with `MS()`. The terrain moves one pixel per frame, so the
  stages are measured in fiftieths of a second (their pixels at 50 Hz) and
  `PX()` turns that into today's pixels: at 60 the stages are 20% longer,
  and the waves, the turrets and the boss come at the same seconds.
- **The sprites** are those of `estela.gfx`, prepared for the engine
  (`zxv_spr_prepare`) at start-up, in a 48 KB store.
- **The terrain** is a tile map of 512 × 24 cells made when the stage
  starts (a random path with a minimum gap and a flat area for the boss).
  Only the bands at the top and bottom where there is wall are drawn, and
  collisions are checked with `zxv_map_cell`.
- **Positions** are in 1/96 of a pixel (so that `V()` is exact at 50 and
  60 Hz), and angles in 64 steps per turn with a sine table: no floating
  point at all.
- **The sound:** the music goes on the first AY, with the SDK's music
  library; the effects, on the second (shots and power-ups on channel A,
  explosions with noise and envelope on B), at 50 steps per second, as when
  they were made. Big explosions also have a 0.6 s sample at 8 kHz through
  the volume of channel C, sent a little at a time, ahead, with
  `zxv_ay_event`. It goes through the same queue as the music, which must
  be in time order: the sample starts after what the music already has
  and, while it plays, the music goes up to each sample and no further
  (`zxv_music_update_until`).
- **The keyboard:** the map of keys held down (`ZXV_I_KEYMAP`) is read to
  move and fire at the same time, and the key queue is emptied every frame
  so that what was typed does not reach BASIC on the way back.

## Difficulty

Built with `make -C sdk -B build/estela.zxv EXTRA_CFLAGS=-DESTELA_BOT=1`
(the number is the seed) and run with `zxv-cli --test`, the demo's pilot
plays the three stages from the start, takes hits like a player and, at the
end of each stage, writes the lives it lost, to what (walls, bullets,
enemies or the boss), how many against the boss and how long it took. It
never runs out of ships, so that the whole game can be measured.

The difficulty was tuned with it. Before, the pilot lost 15 to 27 lives in
a game, 9 to 15 in stage 3, and the last boss lasted almost four minutes:
each life lost took the weapon down, and with the weapon at its lowest
everything dragged on. Now the weapon is kept, the bosses have less life and
fire less, the wheels, turrets and darts are slower, the ship stays
invulnerable a little longer after a hit, and only its centre (7 × 3 pixels)
counts for bullets. Over six seeds, the pilot loses 4 to 7 lives in the whole
game (2 to 7 when the machine ran at 50 frames per second), and the last boss
lasts about 40 seconds; each stage and each boss take as long as they did
then. At 60 frames per second the pilot also looks at the wall that is still
above or below the ship: with shorter steps, it moved into the next gap too
early and hit the edge.

## Performance

Built with `make -C sdk -B build/estela.zxv EXTRA_CFLAGS=-DESTELA_STATS`,
the score line shows the lost frames, and the demo plays the three stages
in full, with their bosses. On the emulator (at 144 MHz and 60 frames per
second) no frame is lost while playing, even with dozens of bullets on
screen; only one at start-up, with what is prepared before the first frame. It has not been measured on the
Pico 2.

