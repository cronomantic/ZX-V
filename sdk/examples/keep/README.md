**English** · [Español](README.es.md)

# The Keep

A top-down dungeon adventure in C for ZX-V: a tower of four floors, each of
3 × 3 rooms, with slimes, bats, skeletons and archers, keys, locked doors
and a treasure on each floor that opens the way. The sorcerer waits at the
top. The rooms are tile maps of 16 × 16 tiles (`zxv_map_draw`) with sprites
on top, and everything (the drawings, the floors, the font, the music and
the sound effects) lives in files you can edit.

```sh
make -C sdk                       # build/keep.zxv
make -C sdk run SD=my_card        # and in BASIC: LOAD "keep"
```

It is also on the machine's ROM disk: `LOAD "ROM:keep"`.

## The game

- **The tower:** the dungeons, the armoury, the library (in the dark) and
  the summit. You go from room to room through their doorways, and the
  screen slides; the stairs take you up.
- **Each floor's treasure:** the **iron boots** (spikes no longer hurt),
  the **great sword** (hits harder and cuts brambles), the **lantern**
  (without it you only see around the hero in the library) and the **big
  key** (it opens the sorcerer's door).
- **The enemies:** the slime wanders and splits in two when beaten; the bat
  flutters towards the hero and flies over water; the skeleton chases him;
  the archer shoots as soon as the hero is in line. A beaten enemy stays
  beaten while you are on the floor; sometimes it drops a coin or a small
  heart.
- **The sorcerer** floats around his room casting rings of fire, fireballs
  aimed at the hero and, when badly hurt, a pair of bats; now and then he
  fades out and appears somewhere else.
- **The hero** levels up with what he beats: one more heart at even levels
  and harder hits at levels 3 and 6. Big hearts give one more heart too.
  **Keys** open locked doors (each one, once) and **potions** fill up the
  hearts.
- **The sage** at the entrance gives advice, and whatever blocks the way (a
  locked door, the spikes, the brambles) is explained the first time.
- **Records** in `SD:keep.txt` (on the card, even when the game is loaded
  from `ROM:`): the fastest win and the most gold.
- **Demo:** leave the title screen alone for 12 seconds and a pilot plays by
  itself.
- **Texts in English or Spanish**, following the machine's language; the
  name, *The Keep*, is the same in both.

| Key | Gamepad | |
|---|---|---|
| Arrows, or O, P, Q and A | D-pad | Walk |
| Space or M | Button 1 | The sword; in a message, go on |
| Z | Button 2 | Drink a potion |
| Enter | Start | Pause (with the game time) |
| S | | Music on or off |
| Esc | | Back to the title; on the title, back to BASIC |

## Make it yours

The program carries these files inside, but if the folder it is loaded from
has one with the same name, it uses that one instead: changing a drawing, a
floor, a tune or a sound effect needs no compiling. One that is not valid
(or does not fit) gives a report and goes back to BASIC.

| File | What it holds | Edit it with |
|---|---|---|
| [`keep.gfx`](keep.gfx) | The sprites, the font, the tiles and the four floors | `zxv-gfx` on the PC or `gfxedit` on the machine |
| [`keep-title.pt3`](keep-title.pt3) | The title music | `zxv-tracker` |
| [`keep-tower.pt3`](keep-tower.pt3) | The music of the floors | `zxv-tracker` |
| [`keep-boss.pt3`](keep-boss.pt3) | The sorcerer's | `zxv-tracker` |
| [`keep-win.pt3`](keep-win.pt3) | The victory tune | `zxv-tracker` |
| [`keep-over.pt3`](keep-over.pt3) | The game over tune | `zxv-tracker` |
| [`keep.afb`](keep.afb) | The sound effects, an AYFX bank | `zxv-tracker` or the AY Sound FX Editor |

### The sprites in `keep.gfx`

| Number | |
|---|---|
| 0–2 | The hero facing down: walking (two) and with the sword |
| 3–5 | Facing up, the same |
| 6–8 | Facing right, the same (facing left is mirrored) |
| 9–10 | The sword, horizontal (pointing right) and vertical (pointing down); with the great sword its colour 7 turns cyan |
| 11–12 | The slime (the small ones are the same, drawn smaller) |
| 13–14 | The bat |
| 15–16 | The skeleton |
| 17–18 | The archer |
| 19–20 | The arrow, horizontal and vertical |
| 21–22 | The sage (the second one with his eyes closed) |
| 23–25 | The sorcerer, 24 × 32: idle (two) and casting a spell |
| 26–27 | The fireball |
| 28–29 | The cloud when something dies |
| 30–35 | Key, potion, big heart, gold, coin and small heart |
| 36–39 | The treasures: boots, great sword, lantern and big key |
| 40–49 | The status bar icons, 8 × 8: full, half and empty heart, key, potion, coin, boots, sword, lantern and big key |
| 50 | The tower on the title screen |
| 51 | The font: 6 × 9 glyphs side by side (the letter in the bottom 7 rows and the marks above it; colour 15 is the ink), in this order: space, `0`–`9`, `A`–`Z`, `! ? . , : - / ' + ( ) *` (the times sign), `¡`, `¿` and the marks: acute accent, tilde and diaeresis |

### The floors: maps 0–3

Each floor is a map of **48 × 33 cells** of 16 × 16 tiles: 3 × 3 rooms of
16 × 11. Each room is drawn with its wall around it, and you go from one to
the next through a gap in the wall. A map of any other size gives a report.

| Tile | | |
|---|---|---|
| 0–3 | The floors: stone, wood, carpet and marble | Walkable |
| 4, 5 | The wall, from above and from the front | |
| 6–9 | Pillar, barrel, bookcase and wall torch | |
| 10 | Water | Only bats cross it |
| 11 | Spikes | Walkable; without the boots they hurt and block the way |
| 12 | Brambles | The great sword cuts them |
| 13 | Locked door | A key opens it (both cells of a doorway at once) |
| 14 | The sorcerer's door | The big key opens it |
| 15 | Stairs | Up to the next floor |
| 16, 17 | Rug and cracked floor | Walkable |
| 18 | Altar | |

Any other tile is not walkable. The **markers** say what appears on that
cell; when the floor starts they become its floor (the most common of tiles
0–3 in the map):

| Tile | Marker |
|---|---|
| 32 | Where the hero starts |
| 33 | The sage |
| 34 | A slime |
| 35 | A bat |
| 36 | A skeleton |
| 37 | An archer |
| 38 | The sorcerer |
| 39 | A key |
| 40 | A potion |
| 41 | A big heart (one more heart) |
| 42 | Gold |
| 43 | The floor's treasure: the boots on floor 1, the great sword on 2, the lantern on 3 and the big key on 4 |

Floor 3 (map 2) is dark until you have the lantern. Beating the sorcerer
ends the game.

### The sound effects in `keep.afb`

By number: 0 the sword, 1 a hit, 2 an enemy beaten, 3 the hero hurt, 4 a
coin, 5 an item, 6 a door, 7 the stairs, 8 an arrow, 9 a spell, 10 a new
level, 11 a potion, 12 a message, 13 a hit on the sorcerer, 14 the brambles
and 15 the spikes. They play on the second AY, and the music on the first.

## How it is made

| File | |
|---|---|
| `keep.h` | What the parts share: sizes, tiles, markers, actors and sound |
| `video.c` | The screen: double buffer, text with the font in `keep.gfx`, sprites and boxes |
| `art.c` | The graphics in `keep.gfx` (with `zxv_gfxfile.h`), prepared for the engine |
| `world.c` | The floor: its map, what can be walked on, the doors and the markers |
| `actors.c` | The hero, the enemies, the sorcerer, arrows, fire and drops |
| `sound.c` | Music and sound effects, with `zxv_music.h` |
| `game.c` | The game, the status bar, the messages, the title, the records and the pilot |

- **The rooms** are the floor's map drawn with `zxv_map_draw` from the
  room's corner; going to another room draws the same map from a point that
  slides from one to the other. Doors that open and brambles that get cut
  change the cells of the map, which is in RAM.
- **Collisions with the map** use a box at each one's feet against the cells
  that cannot be walked on; at a corner the hero slides towards the gap, so
  doorways need no pixel-perfect aim.
- **Standing things are drawn top to bottom:** whoever is lower on the
  screen is in front. Flying things go on top.
- **The status bar** is two lines at the top, everything in its own place:
  on the first, the hearts, keys, potions, gold, treasures and level; on
  the second, the floor (or the demo label), the sorcerer's health or the
  new level, and how to leave the demo. It covers the outer half of the
  room's top wall, which is only wall. Messages, the pause and game over go
  in a box in the room, never two at once.
- **The darkness** in the library is four black rectangles around the hero.
- **Text** uses the font in the file: each glyph is prepared as a sprite and
  drawn recoloured (colour 15 in whatever colour is asked for); accented
  letters are the letter with its mark above. The title uses the same glyphs
  scaled up.
- **The demo pilot** finds the shortest path (breadth-first, over the
  floor's cells) to whatever is left to pick up and then to the stairs or
  the sorcerer, counting on what it carries: doors with keys, spikes with
  the boots and brambles with the great sword. It fights whatever comes
  close and drinks a potion when it is down to one heart.
- **At 60 frames per second:** positions are in 1/16 of a pixel and
  everything moves in whole steps per frame; music and effects run in steps
  of a fiftieth of a second, as they are written.

## Performance

Built with `make -C sdk -B build/keep.zxv EXTRA_CFLAGS=-DKEEP_STATS`, the
status bar shows the lost frames and how long the slowest frame of the last
second took, in microseconds, and the demo plays the whole game without
losing health, until the sorcerer is beaten, and writes it down in
`SD:keepstat.txt`. In the emulator (at 144 MHz) the game takes about three
and a half minutes, no frame is lost and the slowest takes 2.5 ms of the
16.7 of a frame at 60 per second. With `-DKEEP_BOT` the pilot plays like
anybody else, and wins too. Not measured on the Pico 2.

