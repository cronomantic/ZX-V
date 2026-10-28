**English** · [Español](README.es.md)

# Street Justice

A street brawler in C for ZX-V: Kai fights his way through the neighbourhood
against the gangs' thugs, in the style of the arcade beat 'em ups. It uses
the ROM's graphics engine (polygons, circles, canvases turned into sprites,
bitmap copies), double buffering and the two AY chips.

```sh
make -C sdk                       # build/street.zxv
make -C sdk run SD=my_card        # and in BASIC: LOAD "street"
```

It is also on the machine's ROM disk: `LOAD "ROM:street"`.

## The game

- **Three stages:** the street at night (bars, shops with their shutters
  down and graffiti, an alley), the market by day and the rooftops at
  sunset. You walk to the right; the screen stays still until each group of
  thugs is beaten, and then "GO" lets you move on.
- **Depth:** the fighters walk along a band of the floor, towards the back
  or the front, and are drawn in depth order. A blow lands if the other one
  is in front, within reach and at almost the same depth.
- **Kai's moves:** a punch combo (jab, cross and an uppercut that knocks
  down), the jump and the flying kick, and the spinning kick (both buttons
  at once), which hits both sides. Walking into an enemy grabs him: punching
  gives knee strikes and jumping throws him.
- **The thugs:** the punk (all-round), the runner (fast, with kicks) and the
  bruiser (slow, tough, charges). Only two attack at a time; the rest circle
  around waiting for their turn. At the end of each stage, the gang boss:
  Wrench, Chains and The Boss, each one tougher.
- **Breakables:** bins, crates and barrels, with food inside (an apple,
  bread or chicken) that restores health.
- **Three lives**, one more every 30,000 points, a bonus for each stage
  cleared and for the lives left at the end.
- **High score** in `SD:street.txt` (on the card, even when the game was
  loaded from `ROM:`, which cannot be written to). Without a card, it lasts
  for the session.
- **Demo:** if nothing is pressed on the title screen, after 12 seconds the
  autopilot plays a stage (the next one each time).
- **Texts in Spanish or English**, following the machine language
  (`zxv_lang_get`).

| Key | Gamepad | |
|---|---|---|
| Arrows or OPQA | D-pad | Walk |
| Space or M | Button 1 | Punch (knee strikes on a grabbed enemy) |
| N or Z | Button 2 | Jump (kick in the air; throw when grabbing) |
| Both at once | Both | Spinning kick |
| Enter | Start | Pause |
| Esc | | To the title; on the title, to BASIC |

## How it is made

| File | |
|---|---|
| `street.h` | What the parts share: sizes, fighters, poses, objects |
| `draw.c` | The screen: double buffering, the 5 × 7 font (with Spanish accents and Ñ), the canvases where the sprites are drawn, shadows and thick strokes |
| `art.c` | The fighters (a skeleton in poses) and the scenery of the three stages |
| `fight.c` | The fighters: the player, the thugs and the bosses, their moves, the enemy AI, breakables and food, and the demo's autopilot |
| `sound.c` | The music (first AY) and the effects (second AY) |
| `game.c` | The stages, the camera, the groups, the score bar, the title, the demo and the high score |

- **Double buffering:** each frame is drawn in full on the hidden page,
  which is shown at the next VBLANK. A lost frame shows in `ZXV_SV_FRAMES`.
- **Fighters without files:** each one is a skeleton (thighs, legs, torso,
  arms) with its lengths and widths, put in one of 22 poses (walking, the
  moves, the jump, falling, getting up...) with angles in 64 steps per turn.
  At start-up each pose is drawn with the engine's polygons and circles on a
  canvas, outlined, and kept as a sprite; the heads are separate, and each
  thug's colours are a table passed to the sprite when it is drawn
  (`zxv_spr_draw`). During play only sprites are
  copied.
- **The scale:** the fighters are about 50 pixels tall, and the scenery is
  made to their measure: on the street you see the ground floor, with doors
  and shop windows about 1.35 times a fighter and the signs above their
  heads, and the buildings carry on above the screen.
- **The scenery** is made at the start of each stage with the engine in
  bitmaps: the far background (which scrolls at a quarter of the speed), six
  64-pixel facades repeated in the stage's order, and the floor. Each frame
  is a few bitmap copies.
- **Positions** are in 1/16 of a pixel: no floating point.
- **The keyboard:** the map of keys held down (`ZXV_I_KEYMAP`) is read so
  you can walk and punch at the same time, and the key queue is emptied
  every frame so that what was typed does not reach BASIC on the way back.

## Difficulty

Built with `make -C sdk -B build/street.zxv EXTRA_CFLAGS=-DSTREET_BOT=1` (the
number is the seed) and run with `zxv-cli --test`, the demo's pilot plays the
three stages from the start, takes hits like a player and, at the end of
each stage, writes the lives it lost, how many against the boss, the hits
taken, how long it took and the lost frames. It never runs out of lives, so
that the whole game can be measured.

The difficulty was tuned with it: in a whole game, the pilot loses 1 to 3
lives, nearly always against the bosses, and each stage takes it about two
minutes.

## Performance

Built with `EXTRA_CFLAGS=-DSTREET_STATS`, the score bar shows the lost
frames. On the emulator (at 144 MHz) none is lost, even with five fighters on
screen. It has not been measured on the Pico 2.

