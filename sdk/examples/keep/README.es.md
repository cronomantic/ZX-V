[English](README.md) · **Español**

# The Keep

Una aventura de mazmorras en C para ZX-V, vista desde arriba: una torre de
cuatro plantas, cada una de 3 × 3 salas, con limos, murciélagos, esqueletos
y arqueros, llaves, puertas cerradas y un tesoro en cada planta que abre el
camino. Arriba espera el hechicero. Las salas son mapas de tiles de 16 × 16
(`zxv_map_draw`) con sprites encima, y todo, los dibujos, las plantas, la
fuente, la música y los efectos, está en ficheros que se editan.

```sh
make -C sdk                       # build/keep.zxv
make -C sdk run SD=mi_tarjeta     # y en el BASIC: LOAD "keep"
```

También está en el disco ROM de la máquina: `LOAD "ROM:keep"`.

## El juego

- **La torre:** las mazmorras, la armería, la biblioteca (a oscuras) y la
  cima. Se pasa de una sala a otra por sus puertas, y la pantalla se
  desliza; se sube por la escalera.
- **Los tesoros de cada planta:** las **botas de hierro** (los pinchos ya
  no hacen daño), la **espada grande** (pega más fuerte y corta las zarzas),
  el **farol** (sin él, en la biblioteca solo se ve alrededor del héroe) y la
  **llave grande** (abre la puerta del hechicero).
- **Los enemigos:** el limo va a su aire y al vencerlo se parte en dos; el
  murciélago vuela a trompicones hacia el héroe y pasa por encima del agua;
  el esqueleto lo persigue; el arquero dispara en cuanto lo tiene en línea.
  Un enemigo vencido no vuelve mientras se esté en la planta; a veces deja
  una moneda o un corazoncito.
- **El hechicero** flota por su sala y lanza anillos de fuego, bolas
  dirigidas al héroe y, cuando está malherido, llama a los murciélagos; a
  veces se desvanece y aparece en otro sitio.
- **El héroe** sube de nivel con lo que vence: en los niveles pares gana un
  corazón y en el 3 y el 6 pega más fuerte. Los corazones grandes también
  dan uno más. Las **llaves** abren las puertas cerradas (cada una, una
  vez) y las **pociones** llenan los corazones.
- **El sabio** de la entrada da un consejo, y lo que no deja pasar (una
  puerta cerrada, los pinchos, las zarzas) se explica la primera vez.
- **Récord** en `SD:keep.txt` (en la tarjeta, aunque el juego se cargue
  desde `ROM:`): la victoria más rápida y el que más oro ha juntado.
- **Demostración:** sin tocar nada en el título, a los 12 segundos un
  piloto juega solo.
- **Textos en español o en inglés**, según el idioma de la máquina; el
  nombre, *The Keep*, es el mismo en los dos.

| Tecla | Mando | |
|---|---|---|
| Cursores, u O, P, Q y A | Cruceta | Andar |
| Espacio o M | Botón 1 | La espada; en un mensaje, seguir |
| Z | Botón 2 | Beber una poción |
| Intro | Start | Pausa (con el tiempo de la partida) |
| S | | La música, sí o no |
| Esc | | Al título; en el título, al BASIC |

## Todo, a tu gusto

El programa lleva dentro estos ficheros, pero si en la carpeta desde la que
se carga hay uno con el mismo nombre, usa ese: para cambiar un dibujo, una
planta, una música o un efecto no hace falta compilar nada. Uno que no vale
(o no cabe) da un informe y vuelve al BASIC.

| Fichero | Lo que tiene | Se edita con |
|---|---|---|
| [`keep.gfx`](keep.gfx) | Los sprites, la fuente, los tiles y las cuatro plantas | `zxv-gfx` en el PC o `gfxedit` en la máquina |
| [`keep-title.pt3`](keep-title.pt3) | La música del título | `zxv-tracker` |
| [`keep-tower.pt3`](keep-tower.pt3) | La de las plantas | `zxv-tracker` |
| [`keep-boss.pt3`](keep-boss.pt3) | La del hechicero | `zxv-tracker` |
| [`keep-win.pt3`](keep-win.pt3) | La de la victoria | `zxv-tracker` |
| [`keep-over.pt3`](keep-over.pt3) | La del final de la partida | `zxv-tracker` |
| [`keep.afb`](keep.afb) | Los efectos, un banco AYFX | `zxv-tracker` o el AY Sound FX Editor |

### Los sprites de `keep.gfx`

| Número | |
|---|---|
| 0–2 | El héroe hacia abajo: andando (dos) y con la espada |
| 3–5 | Hacia arriba, igual |
| 6–8 | Hacia la derecha, igual (hacia la izquierda, en espejo) |
| 9–10 | La espada, horizontal (hacia la derecha) y vertical (hacia abajo); con la espada grande, su 7 sale en cian |
| 11–12 | El limo (los pequeños, el mismo más pequeño) |
| 13–14 | El murciélago |
| 15–16 | El esqueleto |
| 17–18 | El arquero |
| 19–20 | La flecha, horizontal y vertical |
| 21–22 | El sabio (el segundo, con los ojos cerrados) |
| 23–25 | El hechicero, de 24 × 32: quieto (dos) y lanzando un hechizo |
| 26–27 | La bola de fuego |
| 28–29 | La nube de cuando muere algo |
| 30–35 | Llave, poción, corazón grande, oro, moneda y corazoncito |
| 36–39 | Los tesoros: botas, espada grande, farol y llave grande |
| 40–49 | Los iconos del marcador, de 8 × 8: corazón lleno, medio y vacío, llave, poción, moneda, botas, espada, farol y llave grande |
| 50 | La torre del título |
| 51 | La fuente: glifos de 6 × 9 seguidos (la letra en las 7 filas de abajo y las marcas encima; el 15 es la tinta), en este orden: espacio, `0`–`9`, `A`–`Z`, `! ? . , : - / ' + ( ) *` (el signo de multiplicar), `¡`, `¿` y las marcas: acento, tilde y diéresis |

### Las plantas: los mapas 0–3

Cada planta es un mapa de **48 × 33 celdas** de tiles de 16 × 16: 3 × 3
salas de 16 × 11. Las salas se dibujan con su pared alrededor y se pasa de
una a otra por un hueco en la pared. Un mapa de otro tamaño da un informe.

| Tile | | |
|---|---|---|
| 0–3 | Los suelos: piedra, madera, alfombra y mármol | Se pisa |
| 4, 5 | La pared, por arriba y por delante | |
| 6–9 | Columna, barril, estantería y antorcha en la pared | |
| 10 | Agua | Solo la cruzan los murciélagos |
| 11 | Pinchos | Se pisa; sin las botas, hace daño y no deja pasar |
| 12 | Zarzas | La espada grande las corta |
| 13 | Puerta cerrada | Se abre con una llave (las dos de un paso, a la vez) |
| 14 | La puerta del hechicero | Se abre con la llave grande |
| 15 | Escalera | Sube a la planta siguiente |
| 16, 17 | Alfombra y suelo agrietado | Se pisa |
| 18 | Altar | |

Los demás tiles no se pisan. Las **marcas** dicen lo que sale en esa celda;
al empezar la planta se cambian por su suelo (el más común de los 0–3 del
mapa):

| Tile | Marca |
|---|---|
| 32 | Donde empieza el héroe |
| 33 | El sabio |
| 34 | Un limo |
| 35 | Un murciélago |
| 36 | Un esqueleto |
| 37 | Un arquero |
| 38 | El hechicero |
| 39 | Una llave |
| 40 | Una poción |
| 41 | Un corazón grande (un corazón más) |
| 42 | Oro |
| 43 | El tesoro de la planta: las botas en la 1, la espada grande en la 2, el farol en la 3 y la llave grande en la 4 |

La planta 3 (el mapa 2) está a oscuras hasta tener el farol. Vencer al
hechicero acaba la partida.

### Los efectos de `keep.afb`

Por su número: 0 la espada, 1 un golpe, 2 un enemigo vencido, 3 el héroe
herido, 4 una moneda, 5 un objeto, 6 una puerta, 7 la escalera, 8 una
flecha, 9 un hechizo, 10 un nivel nuevo, 11 una poción, 12 un mensaje, 13 un
golpe al hechicero, 14 las zarzas y 15 los pinchos. Suenan en el segundo AY
y la música, en el primero.

## Cómo está hecho

| Fichero | |
|---|---|
| `keep.h` | Lo que comparten las partes: tamaños, tiles, marcas, actores y sonido |
| `video.c` | La pantalla: doble buffer, el texto con la fuente de `keep.gfx`, los sprites y los recuadros |
| `art.c` | Los gráficos de `keep.gfx` (con `zxv_gfxfile.h`), preparados para el motor |
| `world.c` | La planta: su mapa, lo que se pisa, las puertas y las marcas |
| `actors.c` | El héroe, los enemigos, el hechicero, las flechas, el fuego y lo que cae |
| `sound.c` | La música y los efectos, con `zxv_music.h` |
| `game.c` | La partida, el marcador, los mensajes, el título, el récord y el piloto |

- **Las salas** son el mapa de la planta dibujado con `zxv_map_draw` desde
  la esquina de la sala; al pasar a otra, el mismo mapa se dibuja desde un
  punto que se desliza de una a otra. Las puertas que se abren y las zarzas
  que se cortan cambian las celdas del mapa, que está en la RAM.
- **Los choques con el mapa** son los de una caja en los pies de cada uno
  contra las celdas que no se pisan; el héroe, en una esquina, se desliza
  hacia el hueco, para entrar por las puertas sin ir al píxel.
- **Lo de pie se dibuja de arriba abajo:** el que está más abajo queda
  delante. Lo que vuela, encima.
- **El marcador** son dos líneas arriba, cada cosa en su sitio: en la
  primera, los corazones, las llaves, las pociones, el oro, los tesoros y
  el nivel; en la segunda, la planta (o el letrero de la demostración), la
  vida del hechicero o el nivel nuevo, y cómo salir de la demostración.
  Tapa la mitad de fuera de la pared de arriba de la sala, que solo es
  pared. Los mensajes, la pausa y el final van en un recuadro en la sala, y
  nunca dos a la vez.
- **La oscuridad** de la biblioteca son cuatro rectángulos negros alrededor
  del héroe.
- **El texto** usa la fuente del fichero: cada glifo se prepara como un
  sprite y se dibuja recoloreado (el 15, del color que se pida); las letras
  con tilde son la letra y su marca encima. El título, con los mismos
  glifos escalados.
- **El piloto de la demostración** busca el camino más corto (por anchura,
  en las celdas de la planta) a lo que quede por coger y luego a la escalera
  o al hechicero, contando con lo que lleva: las puertas con llave, los
  pinchos con las botas y las zarzas con la espada grande. Pelea con lo que
  se le acerca y bebe una poción cuando le queda un corazón.
- **A 60 imágenes por segundo:** las posiciones van en 1/16 de píxel y todo
  se mueve en enteros por frame; la música y los efectos van a pasos de un
  cincuentavo de segundo, como se escriben.

## Rendimiento

Compilado con `make -C sdk -B build/keep.zxv EXTRA_CFLAGS=-DKEEP_STATS`, el
marcador enseña los frames perdidos y lo que tardó el frame más lento del
último segundo, en microsegundos, y la demostración juega la partida entera
sin perder vida, hasta vencer al hechicero, y lo apunta en
`SD:keepstat.txt`. En el emulador (a 144 MHz) la partida dura unos tres
minutos y medio, no se pierde ningún frame y el peor tarda 2,5 ms de los
16,7 de un frame a 60 por segundo. Con `-DKEEP_BOT`, el piloto juega como
cualquiera y también la gana. En la Pico 2 no está medido.

