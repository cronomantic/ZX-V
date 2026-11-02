[English](README.md) · **Español**

# ZXV Chess

Ajedrez para ZX-V, completo, contra la máquina o entre dos jugadores. Modo 0 (256 x 192):
el tablero a la izquierda (casillas de 24 x 24) y un panel de texto a la derecha.

```sh
make -C sdk                       # build/chess.zxv
make -C sdk run SD=mi_tarjeta     # y en el BASIC: LOAD "chess"
```

También está en el disco ROM de la máquina: `LOAD "ROM:chess"`.

## Manejo

| Acción | Teclado | Mando |
| --- | --- | --- |
| Mover el cursor | cursores o WASD | cruceta |
| Elegir pieza / mover | Intro o Espacio | FIRE1 |
| Anular la elección | Esc o Retroceso | FIRE2 |
| Deshacer (también la respuesta de la máquina) | U | SELECT |
| Nueva partida (pide pulsar N otra vez) | N | |
| Menú | M | START |

En el menú: arriba/abajo eligen y llegan a los dos selectores, Intro empieza y
Esc (o la opción Salir) vuelve al BASIC. La penúltima fila, en magenta, es el
selector de **nivel**: izquierda/derecha (o Intro, cuando está enfocado) cambian
el nivel. La última, en cian, es el selector de **tablero**: izquierda/derecha
cambian el tablero (Intro pasa al siguiente). Con el teclado, el selector
enfocado muestra un `>`; si es el ratón el que lo enfoca al pasar por encima, se
resalta sin `>`.
Al coronar un peón aparece una rejilla de 2 x 2
con dama, torre, alfil y caballo: se elige con los cursores e Intro.

## Ratón

El puntero aparece en cuanto se mueve el ratón y desaparece si se vuelve al
teclado o al mando; se puede alternar cuando se quiera.

| Dónde | Qué hace |
| --- | --- |
| Menú: pasar por encima | resalta la opción |
| Menú: botón izquierdo | elige la opción (empezar, continuar o salir) |
| Menú: clic en `<` / `>` (o la rueda) del selector de nivel o del de tablero | baja / sube el nivel o cambia el tablero |
| Partida: mover | el cursor sigue al puntero |
| Partida: botón izquierdo sobre el tablero | elige pieza y mueve (como Intro) |
| Partida: botón derecho | anula la elección (como Esc) |
| Partida: clic en `U atras`, `N nueva` o `M menu` (filas de texto 18, 20 y 22 del panel) | hace lo mismo que esa tecla; la primera letra va siempre en rojo y el resto se pone amarillo al pasar el puntero, como la opción marcada del menú principal |
| Coronación: clic en la pieza de la rejilla | corona con ella |

Niveles: 1 principiante (mira una jugada y se despista), 2 fácil, 3 medio
(4 medias jugadas, hasta 40 000 nodos) y 4 fuerte (hasta 6 medias jugadas,
100 000 nodos; en el emulador, unos 3 segundos por jugada como mucho). Al
arrancar el juego el nivel es el 1.

Tableros (todos con colores sin brillo; casillas oscuras - claras): 1 azul - cian
(el de arranque), 2 magenta - amarillo, 3 magenta - cian y 4 magenta - verde.
Cada uno lleva sus propios colores para la última jugada y la pieza elegida, para
que no se confundan con las casillas; el jaque es siempre rojo brillante y el
cursor un marco negro y blanco que se ve sobre cualquier color.

Con negras el tablero se gira. El texto sale en español, o en inglés si la
máquina está en inglés.

## Gráficos (editables como PNG)

Los dibujos están en `art/`. El `Makefile` hace con ellos `build/chess.gfx` (con
`png2gfx.py`), que va dentro del programa: los sprites 0 a 5 son las piezas
blancas, del 6 al 11 las negras y el 12 el puntero. Si en la carpeta desde la
que se carga el juego hay otro `chess.gfx`, usa ese; así se pueden cambiar las
piezas sin compilar, también con `zxv-gfx` o `gfxedit`. Uno que no vale (o con
piezas que no son de 24 x 24) da un informe y vuelve al BASIC.

| Fichero | Tamaño | Contenido |
| --- | --- | --- |
| `art/pieces.png` | 144 x 48 | 12 piezas de 24 x 24, de izquierda a derecha: peón, caballo, alfil, torre, dama y rey. **Fila de arriba: blancas; fila de abajo: negras** |
| `art/pointer.png` | 16 x 17 | la flecha del ratón; el punto caliente es la esquina de arriba a la izquierda |

Para editarlos basta cualquier programa de dibujo (con zoom, que son pequeños):

* **Desde Photoshop, GIMP, Aseprite...:** exporta el PNG como quieras (indexado,
  con el fondo en un color liso) y pásalo por el importador:

      python3 art/import_png.py MI_EXPORTACION.png art/pieces.png

  Convierte en transparente solo el fondo conectado con el borde de cada casilla
  (el mismo color dentro de una pieza se respeta como dibujo) y ajusta los colores
  a la paleta exacta de ZX-V avisando de lo que cambie. Para el puntero:
  `python3 art/import_png.py MI_PUNTERO.png art/pointer.png --celda 0`.
* **Colores:** solo los 15 de ZX-V, con cada componente a `00`, `D7` o `FF`
  (negro `000000`, blanco `D7D7D7`, blanco brillante `FFFFFF`, rojo `D70000`,
  rojo brillante `FF0000`...). Otro color se aproxima al más cercano y
  `png2gfx.py` avisa. El **transparente** es un píxel con alfa menor que 128.
* **Contraste:** las piezas se dibujan sobre las casillas del tablero elegido, el
  rey del turno sobre el fondo negro del panel y el del mate sobre una casilla
  clara. Un relleno azul sobre una casilla azul (tablero 1) se pierde, y el negro
  sobre negro también.
* **Medidas:** hay que respetar el tamaño de cada casilla de la hoja (24 x 24) y
  que `pieces.png` tenga 12. El puntero, de hasta 32 x 32.
* **Guardar y recompilar:** `make build/chess.zxv`, que vuelve a hacer
  `build/chess.gfx` si han cambiado los PNG. O, sin compilar, copia ese
  `chess.gfx` en la tarjeta junto a `chess.zxv`.
* **Volver a los originales:** `git checkout -- art/` (o cópialos de una versión del SDK).

Los colores del tablero (casillas, última jugada, pieza elegida) no son imágenes:
están en la tabla `BOARDS` del principio de `game.c` (nombre en español e inglés,
color oscuro, claro, de última jugada y de selección; añadir una fila y subir
`N_BOARDS` basta para tener otro tablero). El jaque es `C_CHECK`.

## Reglas

Todas las de la FIDE salvo la triple repetición: enroques, captura al paso,
coronación (a la pieza que elijas), jaque, mate, rey ahogado, regla de las 50
jugadas y material insuficiente (rey solo, o rey y una pieza menor).

## Compilar

Está en el `Makefile` del SDK (lista `EXAMPLES`), que hace también
`build/chess.gfx`:

    make build/chess.zxv


## Ficheros

* `chess.h`, `chess.c`: las reglas y el jugador artificial. No dependen de ZX-V.
  Tablero 0x88, copia-y-juega, alfa-beta con ordenación de capturas, búsqueda
  de reposo, extensión en jaque y profundización iterativa con tope de nodos.
* `game.c`: menú, partida, dibujo (doble buffer), entrada y sonido (segundo AY).
  Las piezas y el puntero se leen de `chess.gfx` con `zxv_gfxfile.h` y se
  preparan al arrancar con `zxv_spr_prepare`.
* `art/`: los PNG editables e `import_png.py` (prepara un PNG de un editor).
* `test/test_chess.c`: pruebas del motor en el PC.
* `test/ui_test.c`, `test/mock/`, `test/shots_to_png.py`: ejecutan `game.c`
  en el PC con la API de ZX-V simulada (pantalla, teclado y ratón) y
  pulsaciones guionizadas, y guardan capturas.

## Probar en el PC

    make build/chess.zxv              # el arnés lee build/chess.gfx
    cd examples/chess/test
    gcc -O2 -Wall -Wextra -I.. test_chess.c ../chess.c -o test_chess && ./test_chess
    gcc -O1 -g -Wall -Wextra -Imock -I.. ui_test.c ../chess.c -o ui_test
    mkdir -p shots && ./ui_test ia shots && python3 shots_to_png.py shots

`shots_to_png.py` necesita Pillow. Escenarios del arnés: `ia`, `mate`, `promo`,
`negras`, `fin`, `raton`, `ratonpromo`, `ratonnegras`, `tablero` (los
selectores), `tableros` (la misma partida en los cuatro tableros) y `panel`
(los 14 estados del panel y el menú principal; comprueba también, píxel a píxel,
el título en doble altura). `ZXV_LANG=en` ejecuta el juego en inglés. El arnés
trae una mini-fuente de 8 x 8 solo para las letras del título; en la máquina se
usa la de la ROM.

## El panel derecho

* El texto del panel empieza en la columna 25 (margen de una columna) salvo el
  título (`zxv`, columna 26), `Turno` (26), la casilla del cursor (27) y el dígito del
  nivel (31).
* El título lleva `zxv` en rojo brillante y `CHESS` en magenta brillante
  (`C_TITLE_ZXV` y `C_TITLE_CHESS` en `game.c`). Lo marcado con el ratón o el cursor
  es siempre amarillo brillante (`INK_HILITE`) y la primera letra de los botones,
  rojo brillante (`C_KEY`).
* El rey del indicador de turno se dibuja sobre el fondo del panel, sin casilla detrás
  (transparente); el rey ganador del mate sigue sobre una casilla clara.
* **Título del menú principal en doble altura:** `Z X V` en rojo y, debajo, `C H E S S`
  en magenta, con la misma letra pero al doble de alto. El SDK no tiene texto escalado,
  así que `big_text()` escribe el texto con la fuente de la ROM en las filas 22 y 23 de
  la página *oculta* (`SCRATCH_ROW`; nunca se ve), lee sus píxeles de la memoria de
  vídeo (4 bits por píxel) y los redibuja con cada línea repetida dos veces y el color
  que toque; después deja esas filas en negro. Solo sirve en modo 0 y con ASCII.
* Un texto que no cabe desde su columna se corre a la izquierda lo justo (`ptxt()`
  en `game.c`). Hoy solo le pasa, por tener 8 caracteres, a `Regla de` (en español).
