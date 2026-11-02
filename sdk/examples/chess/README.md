**English** · [Español](README.es.md)

# ZXV Chess

Complete chess for ZX-V, against the machine or for two players. Mode 0
(256 x 192): the board on the left (24 x 24 squares) and a text panel on the
right.

```sh
make -C sdk                       # build/chess.zxv
make -C sdk run SD=my_card        # and in BASIC: LOAD "chess"
```

It is also on the machine's ROM disk: `LOAD "ROM:chess"`.

## Controls

| Action | Keyboard | Joystick |
| --- | --- | --- |
| Move the cursor | cursor keys or WASD | d-pad |
| Pick a piece / move | Enter or Space | FIRE1 |
| Cancel the choice | Esc or Backspace | FIRE2 |
| Undo (also the machine's reply) | U | SELECT |
| New game (asks for N again) | N | |
| Menu | M | START |

In the menu, up and down choose and also reach the two selectors; Enter starts
and Esc (or the Quit option) goes back to BASIC. The second to last row, in
magenta, is the **level** selector: left and right (or Enter, when it has the
focus) change the level. The last one, in cyan, is the **board** selector:
left and right change the board (Enter moves to the next one). With the
keyboard the selector with the focus shows a `>`; when the mouse gives it the
focus by passing over it, it is highlighted without the `>`.
When a pawn is promoted a 2 x 2 grid appears with queen, rook, bishop and
knight: choose with the cursor keys and Enter.

## Mouse

The pointer appears as soon as the mouse moves and hides when you go back to
the keyboard or the joystick; you can switch whenever you like.

| Where | What it does |
| --- | --- |
| Menu: pass over | highlights the option |
| Menu: left button | chooses the option (start, resume or quit) |
| Menu: click on `<` / `>` (or the wheel) of the level or board selector | lowers / raises the level or changes the board |
| Game: move | the cursor follows the pointer |
| Game: left button on the board | picks a piece and moves (like Enter) |
| Game: right button | cancels the choice (like Esc) |
| Game: click on `U undo`, `N new` or `M menu` (text rows 18, 20 and 22 of the panel) | does the same as that key; the first letter is always red and the rest turns yellow under the pointer, like the marked option of the main menu |
| Promotion: click on a piece of the grid | promotes to it |

Levels: 1 beginner (looks one move ahead and slips up), 2 easy, 3 medium
(4 half-moves, up to 40,000 nodes) and 4 strong (up to 6 half-moves,
100,000 nodes; about 3 seconds per move at most in the emulator). The game
starts at level 1.

Boards (all with non-bright colours; dark - light squares): 1 blue - cyan (the
first one), 2 magenta - yellow, 3 magenta - cyan and 4 magenta - green. Each
has its own colours for the last move and the chosen piece, so that they
stand out from the squares; check is always bright red and the cursor a black
and white frame that shows on any colour.

When you play Black the board turns round. The texts are in Spanish, or in
English if the machine is in English.

## Graphics (editable as PNG)

The drawings are in `art/`. The `Makefile` turns them into `build/chess.gfx`
(with `png2gfx.py`), which goes inside the program: sprites 0 to 5 are the
white pieces, 6 to 11 the black ones and 12 the pointer. If there is another
`chess.gfx` in the folder the game is loaded from, it uses that one, so you
can change the pieces without compiling, also with `zxv-gfx` or `gfxedit`. One
that is not valid (or whose pieces are not 24 x 24) gives a report and goes
back to BASIC.

| File | Size | Contents |
| --- | --- | --- |
| `art/pieces.png` | 144 x 48 | 12 pieces of 24 x 24, from left to right: pawn, knight, bishop, rook, queen and king. **Top row: White; bottom row: Black** |
| `art/pointer.png` | 16 x 17 | the mouse arrow; the hot spot is the top left corner |

Any drawing program will do (with zoom, as they are small):

* **From Photoshop, GIMP, Aseprite...:** export the PNG as you like (indexed,
  with the background in a flat colour) and pass it through the importer:

      python3 art/import_png.py MY_EXPORT.png art/pieces.png

  It makes transparent only the background connected to the edge of each
  square (the same colour inside a piece stays as drawing) and snaps the
  colours to the exact ZX-V palette, saying what changed. For the pointer:
  `python3 art/import_png.py MY_POINTER.png art/pointer.png --celda 0`.
* **Colours:** only the 15 of ZX-V, each component `00`, `D7` or `FF` (black
  `000000`, white `D7D7D7`, bright white `FFFFFF`, red `D70000`, bright red
  `FF0000`...). Any other colour becomes the nearest one and `png2gfx.py` warns.
  **Transparent** is a pixel with alpha below 128.
* **Contrast:** the pieces are drawn on the squares of the chosen board, the
  turn king on the black panel and the mate king on a light square. A blue fill
  on a blue square (board 1) gets lost, and so does black on black.
* **Sizes:** keep the 24 x 24 squares of the sheet and the 12 pieces of
  `pieces.png`. The pointer, up to 32 x 32.
* **Save and rebuild:** `make build/chess.zxv`, which makes `build/chess.gfx`
  again when the PNG have changed. Or, without compiling, copy that
  `chess.gfx` to the card next to `chess.zxv`.
* **Back to the originals:** `git checkout -- art/` (or copy them from an SDK
  release).

The board colours (squares, last move, chosen piece) are not images: they are
in the `BOARDS` table at the top of `game.c` (name in Spanish and English,
dark, light, last move and selection colours; adding a row and raising
`N_BOARDS` gives you another board). Check is `C_CHECK`.

## Rules

All the FIDE rules except threefold repetition: castling, en passant,
promotion (to the piece you choose), check, mate, stalemate, the 50-move rule
and insufficient material (a lone king, or king and one minor piece).

## Building

It is in the SDK `Makefile` (the `EXAMPLES` list), which also makes
`build/chess.gfx`:

    make build/chess.zxv


## Files

* `chess.h`, `chess.c`: the rules and the computer player. They do not depend
  on ZX-V. 0x88 board, copy-make, alpha-beta with capture ordering, quiescence
  search, check extension and iterative deepening with a node cap.
* `game.c`: menu, game, drawing (double buffer), input and sound (second AY).
  The pieces and the pointer are read from `chess.gfx` with `zxv_gfxfile.h`
  and prepared at start-up with `zxv_spr_prepare`.
* `art/`: the editable PNG and `import_png.py` (prepares a PNG from an
  editor).
* `test/test_chess.c`: engine tests on the PC.
* `test/ui_test.c`, `test/mock/`, `test/shots_to_png.py`: run `game.c` on the
  PC with a simulated ZX-V API (screen, keyboard and mouse) and scripted key
  presses, and save screenshots.

## Testing on the PC

    make build/chess.zxv              # the harness reads build/chess.gfx
    cd examples/chess/test
    gcc -O2 -Wall -Wextra -I.. test_chess.c ../chess.c -o test_chess && ./test_chess
    gcc -O1 -g -Wall -Wextra -Imock -I.. ui_test.c ../chess.c -o ui_test
    mkdir -p shots && ./ui_test ia shots && python3 shots_to_png.py shots

`shots_to_png.py` needs Pillow. The harness scenarios: `ia`, `mate`, `promo`,
`negras`, `fin`, `raton`, `ratonpromo`, `ratonnegras`, `tablero` (the
selectors), `tableros` (the same game on the four boards) and `panel` (the 14
panel states and the main menu; it also checks the double-height title pixel
by pixel). `ZXV_LANG=en` runs the game in English. The harness carries a tiny
8 x 8 font just for the title letters; the machine uses the ROM's.

## The right panel

* The panel text starts at column 25 (one column of margin) except the title
  (`zxv`, column 26), `Turno`/`Turn` (26), the cursor square (27) and the level
  digit (31).
* The title has `zxv` in bright red and `CHESS` in bright magenta
  (`C_TITLE_ZXV` and `C_TITLE_CHESS` in `game.c`). What the mouse or the cursor
  marks is always bright yellow (`INK_HILITE`), and the first letter of the
  buttons bright red (`C_KEY`).
* The turn king is drawn on the panel background, with no square behind it
  (transparent); the winning king of a mate stays on a light square.
* **Double-height main menu title:** `Z X V` in red and, below, `C H E S S` in
  magenta, in the same font but twice as tall. The SDK has no scaled text, so
  `big_text()` writes the text with the ROM font on rows 22 and 23 of the
  *hidden* page (`SCRATCH_ROW`; never seen), reads its pixels back from video
  memory (4 bits per pixel) and redraws them with each line twice and the
  right colour; then it clears those rows to black. It only works in mode 0
  and with ASCII.
* A text that does not fit from its column moves left just enough (`ptxt()` in
  `game.c`). Today only `Regla de` (in Spanish, 8 characters) needs it.
