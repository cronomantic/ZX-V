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
| S | | Music on or off |
| Esc | | To the title; on the title, to BASIC |

## Its graphics and music, your way

The graphics are in [`route128.gfx`](route128.gfx), edited with `zxv-gfx`
on the PC or with `gfxedit` on the machine itself, and the music in Pro
Tracker 3 modules edited with `zxv-tracker`:
[`route128-race.pt3`](route128-race.pt3) (the race),
[`-title`](route128-title.pt3) (the title), [`-goal`](route128-goal.pt3)
(the finish) and [`-over`](route128-over.pt3) (time up). The program
carries them inside, but if there is one with that name in the folder it
is loaded from, it uses that one: changing a drawing or a tune needs no
compiling. One that is not valid (or does not fit) gives a report and goes
back to BASIC.

The sprites of `route128.gfx`, by number:

| Number | |
|---|---|
| 0–9 | By the road: palm, bush, sunshade, cactus, rock, pine, tree, left and right bend arrows, and the billboard |
| 10–12 | The start, checkpoint and finish arches, in Spanish |
| 13–15 | The same, in English |
| 16–18 | The rivals: coupé, van and lorry (colours 2 and 3, the body and its highlights, become each car's colours) |
| 20–24 | The car, from the hardest turn to the left to the hardest to the right |
| 30–38 | The scenery of the coast, the desert and the forest: for each stage, the mountains, the near layer and the clouds; 256 pixels wide or more, and they repeat |

Objects are drawn smaller the further away they are: their size in the
file is the close-up one, and their width in the world (how much of the
road they take) is in `art.c`.

## How it works

| File | |
|---|---|
| `route128.h` | What the parts share: sizes, fills, sprites, the track and the sound |
| `video.c` | The screen: double buffer, the colour spans of the road, the 5 × 7 font and the sprites |
| `art.c` | The graphics of `route128.gfx` (with `zxv_gfxfile.h`), and each stage's scenery |
| `road.c` | The track and its projection: the road, the scenery, the objects and the cars |
| `sound.c` | The music (first AY, with `zxv_music.h`) and the car (second AY) |
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
- **The graphics** are those of `route128.gfx`, larger than they are seen
  from afar so that they hold up close by. At start-up they are prepared for
  the engine (`zxv_spr_prepare`).
- **The video memory nobody sees:** the graphics file takes 110 KB, and with
  the prepared sprites it does not fit in user RAM. But in mode 0 the screen
  only uses 24 KB of each 96 KB video page: the rest is memory like any
  other, and the prepared sprites, the music from the card and the stage's
  scenery go there.
- **The scenery:** clouds, mountains and the sea, the dunes or the hills, in
  three 512-pixel layers that repeat and move at different speeds with the
  bends. On reaching a stage its layers are copied from the file.
- **The sound:** the engine is two tones of the second AY, one an octave
  lower, that follow the gear's revs; skidding and the verge, its noise; and
  the music, on the first AY, the SDK's music library. The race tune lasts a
  minute and a half before it repeats: an intro, the theme, a part in E
  minor, a breather with chords, a build-up and the theme again, a tone
  higher.
- **No floating point:** positions and projection use integers (the curve in
  1/16, the lateral position in 1/1024 of half the road).
- **At 60 frames per second:** speeds, accelerations and times are written
  per second (`ZXV_FRAME_HZ`), and what is left over from each frame's share
  carries on to the next, so nothing moves in jerks. The music and the sound
  effects go in steps of a fiftieth of a second, as they are written.

## Performance

Built with `make -C sdk -B build/route128.zxv EXTRA_CFLAGS=-DRUTA128_STATS`,
the HUD shows the frames lost and how long the slowest frame of the last
second took, in microseconds, and the demo drives the whole track. In the
emulator (at 144 MHz) the worst frame takes about 11 ms of the 16.7 of a frame
at 60 per second (the coast and the forest, with more objects; the desert,
less than half that), and none are lost on the whole track, the changes of
stage included, nor going from the title to the race and back. It has not
been measured on the Pico 2.

