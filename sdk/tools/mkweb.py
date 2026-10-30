#!/usr/bin/env python3
"""
Prepara un juego de ZX-V para la web (D85): una página que solo tiene ese
juego, que empieza solo y vuelve a empezar al acabar, sin que se vea el BASIC.
Para subirla a itch.io (un .zip con index.html) o a cualquier servidor; también
funciona abierta desde el disco.

Uso: mkweb.py SALIDA JUEGO [FICHERO...] --kit KIT [opciones]
     mkweb.py SALIDA --config game.json [--kit KIT] [opciones]

  SALIDA        la carpeta de la página (se crea; lo que hubiera se cambia)
  JUEGO         el programa que arranca: .zxv, .tap, .tzx o un listado .bas
  FICHERO...    los que necesita el juego (.gfx, .pt3, .afb...): van con él
                en la unidad ROM:, donde el BASIC los encuentra solo (D84)
  --kit KIT     la carpeta play/ de la versión web (zxv-web.zip, o
                emulator/build-web/web/play): index.html, play.js, play.css,
                engine.js y zxv-core.js

Opciones:
  --title TEXTO       el nombre que sale en la portada y en la pestaña
  --id NOMBRE         el nombre de su tarjeta en el navegador (letras, números,
                      - y _; por defecto, el del juego). Cambiarlo es
                      empezar con la tarjeta vacía (sin los récords de antes)
  --lang auto|es|en   el idioma de la máquina y de la página (auto: el del
                      navegador)
  --border 0|1|2      el borde: completo, pequeño o ninguno
  --fit proporcion|entera|llenar   cómo se escala la imagen (también
                      aspect, integer y fill)
  --touch PRESET|JSON los controles táctiles: joystick (la cruceta es el
                      mando 1; A y B, sus dos botones), arrows (los cursores;
                      A es espacio y B, Intro), qaop (Q, A, O y P; A es
                      espacio y B, M) o none; o un fichero o texto JSON
  --zip FICHERO.zip   además, la página en un .zip (index.html en la raíz)

game.json lleva lo mismo: "start", "files", "title", "id", "lang", "border",
"fit", "touch" y "kit" (las rutas, relativas a él). Las opciones de la línea
de órdenes mandan sobre las suyas.
"""
import base64
import json
import os
import re
import shutil
import struct
import sys
import zipfile


def system_spanish():
    """¿Es español el idioma del sistema? El de LC_ALL, LC_MESSAGES o LANG
    o, sin ellas, en Windows el del usuario."""
    for var in ("LC_ALL", "LC_MESSAGES", "LANG"):
        v = os.environ.get(var)
        if v:
            return v[:2].lower() == "es"
    if sys.platform == "win32":
        try:
            import ctypes
            buf = ctypes.create_unicode_buffer(85)
            if ctypes.windll.kernel32.GetUserDefaultLocaleName(buf, len(buf)):
                return buf.value[:2].lower() == "es"
        except (AttributeError, OSError):
            pass
    return False


SPANISH = system_spanish()


def tr(en, es):
    """El texto en el idioma del sistema: el español, o el inglés para cualquier otro."""
    return es if SPANISH else en


# La ayuda en inglés (la española es la de arriba).
HELP_EN = """
Prepares a ZX-V game for the web (D85): a page with only that game, which
starts by itself and starts again when it ends, without showing the BASIC. To
upload to itch.io (a .zip with index.html) or to any server; it also works
opened from the disk.

Usage: mkweb.py OUTPUT GAME [FILE...] --kit KIT [options]
       mkweb.py OUTPUT --config game.json [--kit KIT] [options]

  OUTPUT        the page's folder (created; whatever was there is replaced)
  GAME          the program that starts: .zxv, .tap, .tzx or a .bas listing
  FILE...       the files the game needs (.gfx, .pt3, .afb...): they go with
                it on the ROM: drive, where the BASIC finds them by itself (D84)
  --kit KIT     the play/ folder of the web version (zxv-web.zip, or
                emulator/build-web/web/play): index.html, play.js, play.css,
                engine.js and zxv-core.js

Options:
  --title TEXT        the name on the cover and in the browser tab
  --id NAME           the name of its card in the browser (letters, digits,
                      - and _; by default, the game's). Changing it starts
                      with an empty card (without the old high scores)
  --lang auto|es|en   the machine's and the page's language (auto: the
                      browser's)
  --border 0|1|2      the border: full, small or none
  --fit aspect|integer|fill   how the image is scaled (also proporcion,
                      entera and llenar)
  --touch PRESET|JSON the touch controls: joystick (the d-pad is joystick 1;
                      A and B, its two buttons), arrows (the cursor keys; A is
                      space and B, Enter), qaop (Q, A, O and P; A is space and
                      B, M) or none; or a JSON file or text
  --zip FILE.zip      also, the page in a .zip (index.html at its root)

game.json takes the same: "start", "files", "title", "id", "lang", "border",
"fit", "touch" and "kit" (paths relative to it). Command-line options win
over its own.
"""

PROGRAMS = (".zxv", ".tap", ".tzx", ".bas", ".txt")
KIT = ("index.html", "play.js", "play.css", "engine.js", "zxv-core.js")
KIT_EXTRA = ("LICENSE.txt", "THIRD_PARTY_NOTICES.txt", "licenses")
FITS = {"proporcion": "proporcion", "entera": "entera", "llenar": "llenar",
        "aspect": "proporcion", "integer": "entera", "fill": "llenar"}
TOUCH = {
    "joystick": {"dpad": "joy", "buttons": [{"label": "A", "joy": "FIRE1"}, {"label": "B", "joy": "FIRE2"}],
                 "menu": [{"label": "ESC", "key": "Escape"}]},
    "arrows": {"dpad": "arrows", "buttons": [{"label": "A", "key": "Space"}, {"label": "B", "key": "Enter"}],
               "menu": [{"label": "ESC", "key": "Escape"}]},
    "qaop": {"dpad": "qaop", "buttons": [{"label": "A", "key": "Space"}, {"label": "B", "key": "KeyM"}],
             "menu": [{"label": "ESC", "key": "Escape"}]},
    "none": {"show": "never"},
}
JOY_BITS = ("UP", "DOWN", "LEFT", "RIGHT", "FIRE1", "FIRE2", "FIRE3", "START", "SELECT")

# El disco ROM (pico/common/romdisk.h, como rom/tools/mkromdisk.py), con
# todos los ficheros en la raíz.
ROMDISK_MAGIC = b"ZXVROMD1"
ROMDISK_HEADER = 16
ROMDISK_ENTRY = 80
ROMDISK_MAX = 0x10400000 - 0x100C0000      # lo que cabe en la flash (spec §3.1)


class Error(Exception):
    pass


def valid_name(name):
    """Un nombre que vale en ZX-V (spec §7.7)."""
    return (0 < len(name) <= 63 and all(0x20 <= ord(c) <= 0x7E and c not in '\\:*?"<>|' for c in name)
            and name[-1] not in " .")


def romdisk(files):
    """La imagen del disco ROM con esos ficheros [(nombre, bytes)] en la raíz."""
    files = sorted(files, key=lambda f: (f[0].lower(), f[0]))
    table_end = ROMDISK_HEADER + ROMDISK_ENTRY * (1 + len(files))
    entries = [struct.pack("<64sIIII", b"", 1, 0, 1 if files else 0, len(files))]
    data = bytearray()
    for name, body in files:
        while len(data) % 4:
            data.append(0)
        entries.append(struct.pack("<64sIIII", name.encode(), 0, len(body), table_end + len(data), 0))
        data += body
    total = table_end + len(data)
    if total > ROMDISK_MAX:
        raise Error(tr(f"the game takes {total} bytes; the ROM: drive holds {ROMDISK_MAX}",
                       f"el juego ocupa {total} bytes; en la unidad ROM: caben {ROMDISK_MAX}"))
    head = ROMDISK_MAGIC + struct.pack("<II", total, 1 + len(files))
    return head + b"".join(entries) + bytes(data)


def touch_config(value):
    """Los controles táctiles: un preajuste, un fichero JSON o JSON."""
    if isinstance(value, dict):
        t = value
    elif value in TOUCH:
        return TOUCH[value]
    elif os.path.isfile(value):
        with open(value, encoding="utf-8") as f:
            t = json.load(f)
    else:
        try:
            t = json.loads(value)
        except ValueError:
            raise Error(tr(f"--touch: {value}: not a preset (joystick, arrows, qaop, none), a file or JSON",
                           f"--touch: {value}: no es un preajuste (joystick, arrows, qaop, none), un fichero ni JSON"))
    if not isinstance(t, dict):
        raise Error(tr("--touch: the JSON must be an object", "--touch: el JSON tiene que ser un objeto"))
    if "preset" in t:
        base = dict(touch_config(t.pop("preset")))
        base.update(t)
        t = base
    for group in ("buttons", "menu"):
        for b in t.get(group, []):
            if not isinstance(b, dict) or not (b.get("joy") in JOY_BITS or isinstance(b.get("key"), str)):
                raise Error(tr(f"--touch: {json.dumps(b)}: each control needs \"joy\" ({', '.join(JOY_BITS)}) or \"key\"",
                               f"--touch: {json.dumps(b)}: cada control necesita \"joy\" ({', '.join(JOY_BITS)}) o \"key\""))
    if t.get("dpad") not in (None, "joy", "arrows", "qaop") and not isinstance(t.get("dpad"), dict):
        raise Error(tr("--touch: \"dpad\" is joy, arrows, qaop or an object with up, down, left and right",
                       "--touch: \"dpad\" es joy, arrows, qaop o un objeto con up, down, left y right"))
    return t


def parse(argv):
    """Las opciones: un diccionario con los campos de game.json."""
    cfg, files, config_path = {}, [], None
    names = {"--title": "title", "--id": "id", "--lang": "lang", "--border": "border", "--fit": "fit",
             "--touch": "touch", "--kit": "kit", "--zip": "zip", "--config": None}
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in names:
            if i + 1 >= len(argv):
                raise Error(tr(f"{a} needs a value", f"{a} necesita un valor"))
            if a == "--config":
                config_path = argv[i + 1]
            else:
                cfg[names[a]] = argv[i + 1]
            i += 2
        elif a.startswith("--"):
            raise Error(tr(f"unknown option {a}", f"opción desconocida {a}"))
        else:
            files.append(a)
            i += 1
    if not files:
        raise Error(tr("missing the output folder", "falta la carpeta de salida"))
    out, files = files[0], files[1:]
    base = {}
    if config_path:
        with open(config_path, encoding="utf-8") as f:
            base = json.load(f)
        here = os.path.dirname(os.path.abspath(config_path))
        rel = lambda p: p if os.path.isabs(p) else os.path.join(here, p)
        if "start" in base:
            base["start"] = rel(base["start"])
        base["files"] = [rel(p) for p in base.get("files", [])]
        if "kit" in base:
            base["kit"] = rel(base["kit"])
        if isinstance(base.get("touch"), str) and base["touch"] not in TOUCH:
            base["touch"] = rel(base["touch"])
    if files:
        base["start"] = files[0]
        base["files"] = files[1:]
    base.update(cfg)
    base["out"] = out
    return base


def build(cfg):
    start = cfg.get("start")
    if not start:
        raise Error(tr("missing the game (the program that starts)", "falta el juego (el programa que arranca)"))
    kit = cfg.get("kit")
    if not kit:
        raise Error(tr("missing --kit (the play/ folder of the web version)",
                       "falta --kit (la carpeta play/ de la versión web)"))
    for f in KIT:
        if not os.path.isfile(os.path.join(kit, f)):
            raise Error(tr(f"{kit}: {f} is missing (is it the play/ folder of the web version?)",
                           f"{kit}: falta {f} (¿es la carpeta play/ de la versión web?)"))
    if os.path.splitext(start)[1].lower() not in PROGRAMS:
        raise Error(tr(f"{start}: the game is a .zxv, .tap, .tzx or .bas", f"{start}: el juego es un .zxv, .tap, .tzx o .bas"))

    # Los ficheros del juego, en la raíz de ROM:.
    entries, seen = [], {}
    for path in [start] + list(cfg.get("files", [])):
        name = os.path.basename(path)
        if not os.path.isfile(path):
            raise Error(tr(f"{path}: not found", f"{path}: no está"))
        if not valid_name(name):
            raise Error(tr(f"{name}: the name is not valid in ZX-V (up to 63 printable ASCII characters, without \\ : * ? \" < > |)",
                           f"{name}: el nombre no vale en ZX-V (hasta 63 caracteres ASCII imprimibles, sin \\ : * ? \" < > |)"))
        if name.lower() in seen:
            raise Error(tr(f"{name}: two files with the same name", f"{name}: dos ficheros con el mismo nombre"))
        seen[name.lower()] = path
        with open(path, "rb") as f:
            entries.append((name, f.read()))
    image = romdisk(entries)

    stem = os.path.splitext(os.path.basename(start))[0]
    gid = cfg.get("id") or re.sub(r"[^a-z0-9_-]+", "-", stem.lower()).strip("-") or "juego"
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,40}", gid):
        raise Error(tr(f"--id {gid}: letters, digits, - and _ (up to 40)", f"--id {gid}: letras, números, - y _ (hasta 40)"))
    lang = cfg.get("lang", "auto")
    if lang not in ("auto", "es", "en"):
        raise Error(tr("--lang is auto, es or en", "--lang es auto, es o en"))
    try:
        border = int(cfg.get("border", 0))
    except (TypeError, ValueError):
        border = -1
    if border not in (0, 1, 2):
        raise Error(tr("--border is 0, 1 or 2", "--border es 0, 1 o 2"))
    fit = FITS.get(cfg.get("fit", "proporcion"))
    if not fit:
        raise Error(tr("--fit is aspect, integer or fill", "--fit es proporcion, entera o llenar"))
    game = {
        "id": gid,
        "title": cfg.get("title") or stem,
        "start": os.path.basename(start),
        "lang": lang,
        "border": border,
        "fit": fit,
        "touch": touch_config(cfg.get("touch", "joystick")),
        "romdisk": base64.b64encode(image).decode("ascii"),
    }

    out = cfg["out"]
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    for f in KIT + KIT_EXTRA:
        src = os.path.join(kit, f)
        if os.path.isdir(src):
            shutil.copytree(src, os.path.join(out, f))
        elif os.path.isfile(src):
            shutil.copyfile(src, os.path.join(out, f))
    with open(os.path.join(out, "game.js"), "w", encoding="utf-8", newline="\n") as f:
        f.write("/* El juego de esta página (sdk/tools/mkweb.py). */\n")
        f.write("window.ZXV_GAME = " + json.dumps(game, ensure_ascii=False, indent=1) + ";\n")
    if cfg.get("zip"):
        with zipfile.ZipFile(cfg["zip"], "w", zipfile.ZIP_DEFLATED) as z:
            for dirpath, _, names in os.walk(out):
                for n in sorted(names):
                    full = os.path.join(dirpath, n)
                    z.write(full, os.path.relpath(full, out).replace(os.sep, "/"))
    return game, len(image)


def main(argv):
    if not argv or argv[0] in ("-h", "--help"):
        print((__doc__ if SPANISH else HELP_EN).strip())
        return 0 if argv else 2
    try:
        game, size = build(parse(argv))
    except (Error, OSError, ValueError) as e:
        print("mkweb: " + str(e), file=sys.stderr)
        return 1
    print(tr(f"{game['title']}: {game['start']} and {size} bytes on ROM:, card \"{game['id']}\"",
             f"{game['title']}: {game['start']} y {size} bytes en ROM:, tarjeta \"{game['id']}\""))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
