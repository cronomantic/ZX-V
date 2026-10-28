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
| Esc | | To the title; on the title, to BASIC |

## How it is made

| File | |
|---|---|
| `estela.h` | What the parts share: sizes, sprites, sound, the random number generator and the sine table |
| `draw.c` | The screen: double buffering, the 5 × 7 font (with accents and Ñ), sprites drawn with characters or with the engine, collisions and the stars in three layers |
| `sound.c` | The music (first AY), the effects (second AY) and the digitised samples |
| `game.c` | The stages, the terrain, the enemies, the bosses, the title, the demo and the high score |

The source code and its comments are in Spanish.

- **Double buffering:** every frame is drawn in full on the hidden page
  (`zxv_gfx_draw_page`), which is shown at the next VBLANK
  (`zxv_set_page` and `zxv_wait_vblank`). A lost frame shows up in
  `ZXV_SV_FRAMES`.
- **Sprites without files:** the ship, the darts and the turrets are
  drawings made of characters in the code (`sprite_ascii`); the rocks, the
  mine, the explosions and the bosses are drawn with the engine's circles,
  rectangles and polygons on a canvas (`zxv_gfx_target`) and prepared as
  sprites. All of it at start-up, in a 48 KB store.
- **The terrain** is a tile map of 512 × 24 cells made when the stage
  starts (a random path with a minimum gap and a flat area for the boss).
  Only the bands at the top and bottom where there is wall are drawn, and
  collisions are checked with `zxv_map_cell`.
- **Positions** are in 1/16 of a pixel, and angles in 64 steps per turn
  with a sine table: no floating point at all.
- **The sound:** the music goes on the first AY (melody, bass and drums
  with the noise); the effects, on the second (shots and power-ups on
  channel A, explosions with noise and envelope on B). Big explosions also
  have a 0.6 s sample at 8 kHz through the volume of channel C, sent a
  little at a time, about 30 ms ahead, with `zxv_ay_event`.
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
counts for bullets. Over six seeds, the pilot loses 2 to 7 lives in the whole
game, and the last boss lasts about 40 seconds.

## Performance

Built with `make -C sdk -B build/estela.zxv EXTRA_CFLAGS=-DESTELA_STATS`,
the score line shows the lost frames, and the demo plays the three stages
in full, with their bosses. On the emulator (at 144 MHz) no frame is lost,
even with dozens of bullets on screen. It has not been measured on the
Pico 2.

