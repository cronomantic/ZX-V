**English** · [Español](../../es/sdk/web.md)

# Publishing your game on the web

With **`sdk/tools/mkweb.py`** you turn your game into a web page that has
only that game: it can be uploaded as it is to [itch.io](https://itch.io) or
to any server, and it also works opened from the disk. It works for C
programs (`.zxv`), tapes (`.tap`, `.tzx`) and BASIC listings (`.bas`).

**Contents**

- [What the player sees](#what-the-player-sees)
- [What you need](#what-you-need)
- [Making the page](#making-the-page)
- [The options](#the-options)
- [The touch controls](#the-touch-controls)
- [game.json](#gamejson)
- [Uploading it to itch.io](#uploading-it-to-itchio)
- [How it works](#how-it-works)

## What the player sees

- **A cover** with the game's name and "Click to play" ("Tap to play" on a
  phone). Meanwhile the game loads out of sight: when the player clicks, it
  starts on its title screen, without going through the BASIC.
- **Only the game's picture**, as big as the window (or itch.io's frame),
  and two small buttons at the top right: the sound and full screen.
- **When the game ends or goes back to the BASIC** (with Esc, for example),
  it starts again by itself. If it stops straight away twice in a row (an
  error in the program), the page stays on the BASIC screen, with its
  report, and a button to start again.
- **On a phone or a tablet**, a d-pad and some buttons on the screen
  ([the touch controls](#the-touch-controls)). The keyboard and gamepads
  work too, as in the emulator.
- **High scores stay in the browser**: the game has its own SD card, kept
  from one visit to the next and not mixed with other games from the same
  site.

## What you need

- **Python 3**, nothing else.
- **The kit**: the `play/` folder of the ZX-V web package (`zxv-web.zip`, in
  the [releases](https://github.com/cronomantic/ZX-V/releases)). It has the
  page (`index.html`, `play.js`, `play.css`), the page's engine
  (`engine.js`) and the machine (`zxv-core.js`, the same emulator as the web
  version, without the examples). If you build the web version yourself, it
  is in `emulator/build-web/web/play`.

## Making the page

```sh
python3 sdk/tools/mkweb.py estela-web sdk/build/estela.zxv --kit play --title Estela --zip estela.zip
```

This leaves the page in the `estela-web` folder (open its `index.html` to
try it) and, with `--zip`, the same page in `estela.zip`, ready to upload.

After the program come the files it needs: graphics, music, levels. A BASIC
game with its files:

```sh
python3 sdk/tools/mkweb.py cascade-web cascade.bas cascade.gfx cascade.pt3 cascade.afb --kit play --title Cascade
```

They all go to the root of the `ROM:` drive of the page's machine, and the
page loads the program from there. A BASIC listing finds its files with
`LOAD "cascade" GRAPHICS`, without writing `ROM:` (the BASIC looks for them
in the folder the program was loaded from). What the game saves (a high
score, a saved game) goes to the card, `SD:`, which is kept in the browser.

## The options

| Option | Effect |
|---|---|
| `--kit FOLDER` | The `play/` folder of the web package (required) |
| `--title TEXT` | The name on the cover and in the tab (by default, the file's) |
| `--id NAME` | The name of its card in the browser: letters, digits, `-` and `_` (by default, the file's). Changing it starts with an empty card, without the old high scores |
| `--lang auto\|es\|en` | The machine's and the page's language; `auto` (the default), the browser's |
| `--border 0\|1\|2` | The border: full (the default), small or none. With no border, the game's picture is bigger |
| `--fit aspect\|integer\|fill` | How the picture is scaled: keeping the aspect ratio (the default), in whole multiples or filling the window. `proporcion`, `entera` and `llenar` work too |
| `--touch PRESET\|JSON` | The [touch controls](#the-touch-controls) |
| `--zip FILE.zip` | Also, the page in a `.zip`, with `index.html` at its root |
| `--config game.json` | All of the above in a file ([game.json](#gamejson)) |

## The touch controls

On a phone or a tablet, the page shows a d-pad on the left, up to four round
buttons on the right and, at the bottom, up to four small buttons (for Esc,
a pause...). In portrait, the picture goes on top and the controls below.
With `--touch` you choose how:

| Preset | D-pad | A | B | Bottom |
|---|---|---|---|---|
| `joystick` (the default) | Joystick 1 | Fire 1 | Fire 2 | Esc |
| `arrows` | The cursor keys | Space | Enter | Esc |
| `qaop` | Q, A, O and P | Space | M | Esc |
| `none` | No touch controls | | | |

Or your own, with JSON (in a file or on the command line):

```json
{
  "preset": "joystick",
  "buttons": [
    { "label": "A", "joy": "FIRE1" },
    { "label": "B", "key": "KeyZ" }
  ],
  "menu": [
    { "label": "ESC", "key": "Escape" },
    { "label": "S", "key": "KeyS" }
  ]
}
```

| Field | Contents |
|---|---|
| `preset` | The preset to start from (the rest changes it) |
| `show` | `auto` (only with a touch screen, the default), `always` or `never` |
| `dpad` | `joy`, `arrows`, `qaop`, an object with `up`, `down`, `left` and `right` (one control each) or `null` (no d-pad) |
| `buttons` | Up to four round buttons |
| `menu` | Up to four small buttons, at the bottom |

Each control is `{"label": "A", "joy": "FIRE1"}`, a bit of joystick 1 (`UP`,
`DOWN`, `LEFT`, `RIGHT`, `FIRE1`, `FIRE2`, `FIRE3`, `START` or `SELECT`), or
`{"label": "Z", "key": "KeyZ"}`, a key with the browser's name for it
(`KeyboardEvent.code`: `KeyA`…`KeyZ`, `Digit0`…`Digit9`, `Space`, `Enter`,
`Escape`, `ArrowUp`, `ShiftLeft`...).

## game.json

The same, in a file you keep with the game:

```json
{
  "start": "build/minigame.zxv",
  "files": ["minigame.gfx", "minigame.pt3"],
  "kit": "../zxv-web/play",
  "title": "Minigame",
  "id": "minigame",
  "lang": "auto",
  "border": 2,
  "fit": "proporcion",
  "touch": "arrows"
}
```

```sh
python3 sdk/tools/mkweb.py web --config game.json --zip minigame.zip
```

Paths are relative to the `game.json` itself. What you give on the command
line wins over the file.

## Uploading it to itch.io

1. Make the page with `--zip`.
2. On itch.io, create a new project and choose **Kind of project: HTML**.
3. Upload the `.zip` and tick **This file will be played in the browser**.
4. In **Embed options**, a 4:3 size (for example, 960 × 720); tick **Mobile
   friendly** if it has touch controls. itch.io's full-screen button is not
   needed: the page has its own.

Some browsers (Safari, above all) do not let a page inside another site's
frame store anything: there, high scores last as long as the visit.

## How it works

- The page is the whole machine (the same as the web emulator), with the
  game on its ROM disk, in `game.js` (in base64). That is why it works
  without a server.
- On load, the machine starts out of sight, faster than usual and without
  sound. When the BASIC waits for a command (the `EDITOR` system variable,
  at `0x2000_0724`, is 1), the page types `LOAD "ROM:game.zxv"` (and `RUN`
  for a listing). When `EDITOR` has been 0 for a few frames, the game is
  running and is shown; if it goes back to 1, the game has returned to the
  BASIC and the page resets the machine.
- The game's card is the browser's `/tarjeta-ID` folder, with its own
  IndexedDB database.
