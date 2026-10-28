**English** · [Español](README.es.md)

# Route 128

A racing game in C for ZX-V, in the style of the 1980s arcade ones: a
pseudo-3D road with bends and blind crests, and everything beside it, palm
trees, cacti, pines, signs and the traffic, made of sprites scaled by distance
(`zxv_spr_draw_scaled`). The name nods to Route 66 and to the Spectrum 128.

```sh
make -C sdk                       # build/route128.zxv
make -C sdk run SD=my_card        # and in BASIC: LOAD "route128"
```

It is also on the machine's ROM disk: `LOAD "ROM:route128"`.

## The game

- **Against the clock:** three stages, the coast, the desert and the forest,
  each with its own sky, scenery and roadside. Each checkpoint gives extra
  time; you have to reach the goal before it runs out.
- **The road:** gentle and sharp bends, long straights, dunes, mountain
  passes and crests that hide what is coming. Arrows warn of the sharp bends.
- **Driving:** two gears, LO and HI, that you change by hand. LO pulls hard
  but tops out at 190 km/h; HI reaches 290, but below 120 it barely pulls (to
  start off and after a crash, LO). Shifting down to LO at speed brakes with
  the engine: to go into a sharp bend and come out of it pulling hard. Each
  shift gives half a second of grip, when a bend pushes a third as much: HI
  to LO and back to HI in the middle of a bend, to take it losing hardly any
  speed. The gear flashes on the dashboard when it is time to shift. A bend pushes the
  car outwards, more the faster you go, so you have to brake or shift down in
  the sharp ones; the verge slows you down and makes the car bounce; hitting something
  by the road sends the car spinning, and catching another car, a bump that
  slows you down.
- **Points** for speed and, at the goal, a thousand for each second left.
  **High score** in `SD:route128.txt` (on the card, even when the game is
  loaded from `ROM:`).
- **Demo:** if you touch nothing on the title screen, after 12 seconds an
  autopilot drives the first stage.
- **Texts in Spanish or English**, following the machine's language.

| Key | Joystick | |
|---|---|---|
| Cursor up, Q or space | Up or button 1 | Accelerate |
| Cursor down or A | Down or button 2 | Brake |
| Z | Button 3 or Select | Change gear (LO or HI) |
| Cursor left and right, or O and P | D-pad | Steer |
| Enter | Start | Pause |
| Esc | | To the title; on the title, to BASIC |

## How it works

| File | |
|---|---|
| `route128.h` | What the parts share: sizes, fills, sprites, the track and the sound |
| `video.c` | The screen: double buffer, the colour spans of the road, the 5 × 7 font and the sprites |
| `art.c` | The graphics, drawn with the engine at start-up, and each stage's scenery |
| `road.c` | The track and its projection: the road, the scenery, the objects and the cars |
| `sound.c` | The music (first AY) and the car (second AY) |
| `game.c` | Driving, traffic, crashes, the HUD, the title and the demo |

- **The road** is a strip of segments, each with its curve and height, built
  in sections (enter, hold and leave, with smooth easing), as in *Lou's
  Pseudo 3d Page* (Louis Gorenfeld) and *Javascript Racer* (Jake Gordon).
  Every frame the ends of the 160 segments
  ahead are projected: the curve accumulates from one to the next (each
  segment bends the following ones a little more) and the height moves its
  line up or down the screen.
- **It is painted near to far, line by line:** each screen line is painted by
  the nearest segment that covers it (grass, kerb, tarmac and lane lines, with
  colour spans written a word at a time). What lies beyond a crest is not
  painted, and the objects behind it are clipped by it: that is how the blind
  crests look.
- **No greys:** ZX-V's palette has none, so the tarmac is dithered, at a half
  and a quarter of white, and so are the grass and the sand. The light and
  dark bands every three segments give the sense of speed.
- **Scaled sprites:** objects and cars are drawn far to near with
  `zxv_spr_draw_scaled`, at the size the distance gives. The traffic is three
  drawings (coupé, van and lorry) recoloured with the colour table as they
  are drawn. The player's car is drawn at its own size, in five poses
  depending on how it turns.
- **The graphics** are drawn at start-up with the engine's polygons,
  rectangles and circles on a canvas (`zxv_gfx_target`), larger than they are
  seen from afar so that they hold up close by.
- **The scenery:** clouds, mountains and the sea, the dunes or the hills, in
  three 512-pixel layers that repeat and move at different speeds with the
  bends. They are redrawn when the stage changes.
- **The sound:** the engine is two tones of the second AY, one an octave
  lower, that follow the gear's revs; skidding and the verge, its noise; and
  the music, the first AY.
- **No floating point:** positions and projection use integers (the curve in
  1/16, the lateral position in 1/1024 of half the road).

## Performance

Built with `make -C sdk -B build/route128.zxv EXTRA_CFLAGS=-DRUTA128_STATS`,
the HUD shows the frames lost and how long the slowest frame of the last
second took, in microseconds, and the demo drives the whole track. In the
emulator (at 144 MHz) none are lost: the worst frame takes about 10 ms of the
20 of a frame at 50 per second (the coast and the forest, with more objects;
the desert, half that). It has not been measured on the Pico 2.

