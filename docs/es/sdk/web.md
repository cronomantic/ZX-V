[English](../../en/sdk/web.md) · **Español**

# Publicar tu juego en la web

Con **`sdk/tools/mkweb.py`** conviertes tu juego en una página web que solo
tiene ese juego: se sube tal cual a [itch.io](https://itch.io) o a cualquier
servidor, y también funciona abierta desde el disco. Vale para los programas
en C (`.zxv`), las cintas (`.tap`, `.tzx`) y los listados en BASIC (`.bas`).

**Contenido**

- [Lo que ve el jugador](#lo-que-ve-el-jugador)
- [Lo que hace falta](#lo-que-hace-falta)
- [Hacer la página](#hacer-la-página)
- [Las opciones](#las-opciones)
- [Los controles táctiles](#los-controles-táctiles)
- [game.json](#gamejson)
- [Subirla a itch.io](#subirla-a-itchio)
- [Cómo funciona](#cómo-funciona)

## Lo que ve el jugador

- **Una portada** con el nombre del juego y «Haz clic para jugar» («Toca para
  jugar» en un móvil). Mientras tanto el juego se carga sin verse: al
  pulsar, empieza en su pantalla de título, sin pasar por el BASIC.
- **Solo la imagen del juego**, del tamaño de la ventana (o del marco de
  itch.io), y dos botones pequeños arriba a la derecha: el sonido y la
  pantalla completa.
- **Cuando el juego acaba o vuelve al BASIC** (con Esc, por ejemplo), empieza
  otra vez solo. Si se para enseguida dos veces seguidas (un error en el
  programa), se queda en la pantalla del BASIC, con su informe, y un botón
  para volver a empezar.
- **En un móvil o una tableta**, una cruceta y unos botones en la pantalla
  ([los controles táctiles](#los-controles-táctiles)). Con teclado o mando
  también se juega, como en el emulador.
- **Los récords se quedan en el navegador**: el juego tiene su propia
  tarjeta SD, que se guarda de una visita a otra y no se mezcla con la de
  otros juegos del mismo sitio.

## Lo que hace falta

- **Python 3**, sin nada más.
- **El kit**: la carpeta `play/` del paquete web de ZX-V (`zxv-web.zip`, en
  las [versiones publicadas](https://github.com/cronomantic/ZX-V/releases)).
  Lleva la página (`index.html`, `play.js`, `play.css`), el motor de la
  página (`engine.js`) y la máquina (`zxv-core.js`, el mismo emulador que la
  versión web, sin los ejemplos). Si compilas la versión web tú mismo, está
  en `emulator/build-web/web/play`.

## Hacer la página

```sh
python3 sdk/tools/mkweb.py estela-web sdk/build/estela.zxv --kit play --title Estela --zip estela.zip
```

Deja la página en la carpeta `estela-web` (ábrela con `index.html` para
probarla) y, con `--zip`, la misma página en `estela.zip`, lista para subir.

Después del programa van los ficheros que necesita: los gráficos, la música,
los niveles. Un juego en BASIC con sus ficheros:

```sh
python3 sdk/tools/mkweb.py cascade-web cascade.bas cascade.gfx cascade.pt3 cascade.afb --kit play --title Cascade
```

Todos van a la unidad `ROM:` de la máquina de la página, en su raíz, y la
página carga el programa de ahí. Un listado en BASIC encuentra sus ficheros
con `LOAD "cascade" GRAPHICS`, sin escribir `ROM:` (el BASIC los busca en la
carpeta de la que se cargó el programa). Lo que el juego graba (un récord,
una partida) va a la tarjeta, `SD:`, que se guarda en el navegador.

## Las opciones

| Opción | Efecto |
|---|---|
| `--kit CARPETA` | La carpeta `play/` del paquete web (obligatoria) |
| `--title TEXTO` | El nombre de la portada y de la pestaña (por defecto, el del fichero) |
| `--id NOMBRE` | El nombre de su tarjeta en el navegador: letras, números, `-` y `_` (por defecto, el del fichero). Cambiarlo es empezar con la tarjeta vacía, sin los récords de antes |
| `--lang auto\|es\|en` | El idioma de la máquina y de la página; `auto` (por defecto), el del navegador |
| `--border 0\|1\|2` | El borde: completo (por defecto), pequeño o ninguno. Sin borde, la imagen del juego sale más grande |
| `--fit proporcion\|entera\|llenar` | Cómo se escala la imagen: manteniendo la proporción (por defecto), en múltiplos enteros o llenando la ventana. Valen también `aspect`, `integer` y `fill` |
| `--touch PREAJUSTE\|JSON` | Los [controles táctiles](#los-controles-táctiles) |
| `--zip FICHERO.zip` | Además, la página en un `.zip`, con `index.html` en la raíz |
| `--config game.json` | Todo lo anterior en un fichero ([game.json](#gamejson)) |

## Los controles táctiles

En un móvil o una tableta, la página pone una cruceta a la izquierda, hasta
cuatro botones redondos a la derecha y, abajo, hasta cuatro botones
pequeños (para Esc, una pausa...). En vertical, la imagen va arriba y los
controles debajo. Con `--touch` eliges cómo:

| Preajuste | Cruceta | A | B | Abajo |
|---|---|---|---|---|
| `joystick` (por defecto) | El mando 1 | Fuego 1 | Fuego 2 | Esc |
| `arrows` | Los cursores | Espacio | Intro | Esc |
| `qaop` | Q, A, O y P | Espacio | M | Esc |
| `none` | Sin controles táctiles | | | |

O a tu medida, con un JSON (en un fichero o en la misma orden):

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

| Campo | Contenido |
|---|---|
| `preset` | El preajuste del que se parte (lo demás lo cambia) |
| `show` | `auto` (solo con pantalla táctil, por defecto), `always` o `never` |
| `dpad` | `joy`, `arrows`, `qaop`, un objeto con `up`, `down`, `left` y `right` (cada uno, un control) o `null` (sin cruceta) |
| `buttons` | Hasta cuatro botones redondos |
| `menu` | Hasta cuatro botones pequeños, abajo |

Cada control es `{"label": "A", "joy": "FIRE1"}`, un bit del mando 1 (`UP`,
`DOWN`, `LEFT`, `RIGHT`, `FIRE1`, `FIRE2`, `FIRE3`, `START` o `SELECT`), o
`{"label": "Z", "key": "KeyZ"}`, una tecla con el nombre que le da el
navegador (`KeyboardEvent.code`: `KeyA`…`KeyZ`, `Digit0`…`Digit9`, `Space`,
`Enter`, `Escape`, `ArrowUp`, `ShiftLeft`...).

## game.json

Lo mismo, en un fichero que se guarda con el juego:

```json
{
  "start": "build/minijuego.zxv",
  "files": ["minijuego.gfx", "minijuego.pt3"],
  "kit": "../zxv-web/play",
  "title": "Minijuego",
  "id": "minijuego",
  "lang": "auto",
  "border": 2,
  "fit": "proporcion",
  "touch": "arrows"
}
```

```sh
python3 sdk/tools/mkweb.py web --config game.json --zip minijuego.zip
```

Las rutas son relativas al propio `game.json`. Lo que se diga en la línea de
órdenes manda sobre lo del fichero.

## Subirla a itch.io

1. Haz la página con `--zip`.
2. En itch.io, crea un proyecto nuevo y elige **Kind of project: HTML**.
3. Sube el `.zip` y marca **This file will be played in the browser**.
4. En **Embed options**, un tamaño de 4:3 (por ejemplo, 960 × 720); marca
   **Mobile friendly** si tiene controles táctiles. El botón de pantalla
   completa de itch.io sobra: la página trae el suyo.

Algunos navegadores (Safari, sobre todo) no dejan guardar nada a una página
dentro del marco de otro sitio: ahí los récords duran lo que la visita.

## Cómo funciona

- La página es la máquina entera (la misma que el emulador de la web), con
  el juego en su disco ROM, en `game.js` (en base64). Por eso funciona sin
  servidor.
- Al cargar, la máquina arranca sin verse, más deprisa que de costumbre y
  sin sonido. Cuando el BASIC espera una orden (la variable del sistema
  `EDITOR`, en `0x2000_0724`, vale 1), la página teclea
  `LOAD "ROM:juego.zxv"` (y `RUN` si es un listado). Cuando `EDITOR` lleva
  unos frames a 0, el juego ya está en marcha y se enseña; si vuelve a 1, el
  juego ha vuelto al BASIC y la página reinicia la máquina.
- La tarjeta del juego es la carpeta `/tarjeta-ID` del navegador, con su
  propia base de datos de IndexedDB.
