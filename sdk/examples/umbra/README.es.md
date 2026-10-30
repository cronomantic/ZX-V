[English](README.md) · **Español**

# Umbra

Un laberinto en 3D en C para ZX-V, a oscuras: la única luz es la de tu
lámpara, que se va apagando. Reúne las runas de cada nivel para abrir la
puerta de salida antes de que se acabe el aceite, y huye de las sombras que
vagan por los pasillos. Las paredes se dibujan por *raycasting*, el suelo y
el techo con el modo 7 del motor gráfico, y la niebla de la lámpara, con
tablas de colores.

```sh
make -C sdk                       # build/umbra.zxv
make -C sdk run SD=mi_tarjeta     # y en el BASIC: LOAD "umbra"
```

También está en el disco ROM de la máquina: `LOAD "ROM:umbra"`.

## El juego

- **Cinco laberintos**, cada uno más grande y con más sombras: las criptas
  de piedra, los sótanos de ladrillo, las cuevas del musgo, la gruta de
  cristal y el templo. En cada uno hay unas runas escondidas; con todas, la
  puerta de salida se abre y la salida lleva al siguiente.
- **La lámpara:** una llena dura 80 segundos. Cuanto menos aceite le queda,
  menos lejos se ve: los colores se apagan, la niebla llega antes y al final
  solo se ve lo que está al lado. Los frascos de aceite la rellenan. Cuando
  le queda poco, se oye el latido.
- **Las sombras** vagan por el laberinto y te huelen a ocho pasos: entonces
  te persiguen, y se oye su susurro, más fuerte cuanto más cerca. Van más
  deprisa cuanta menos luz tienes y en los niveles de más adelante. Si te
  tocan se beben parte del aceite (el borde se pone rojo) y desaparecen,
  para volver a salir lejos. Si la lámpara se apaga del todo, enciendes otra
  de las de repuesto; sin ninguna, se acaba la partida. Sus ojos rojos se ven
  aunque estén a oscuras.
- **El mapa:** lo que ya has visto queda apuntado. Abajo, a la izquierda,
  lo que tienes alrededor; con M, el laberinto entero (el juego se para
  mientras lo miras).
- **Puntos** por cada runa (más en los niveles de más adelante), por los
  frascos y por cada nivel superado, más el aceite que te quede. **Récord**
  en `SD:umbra.txt` (en la tarjeta, aunque el juego se cargue desde `ROM:`).
- **Demostración:** sin tocar nada en el título, a los 12 segundos un piloto
  automático juega: va a por la runa más cercana, a por aceite cuando le
  falta y a la puerta cuando está abierta.
- **Textos en español o en inglés**, según el idioma de la máquina.

| Tecla | Mando | |
|---|---|---|
| Cursor arriba o Q | Arriba | Andar |
| Cursor abajo o A | Abajo | Atrás |
| Cursores izquierda y derecha, u O y P | Izquierda y derecha | Girar |
| Z y X | | De lado, a la izquierda y a la derecha |
| M | Botón 2 | El mapa |
| Espacio | Botón 1 | Empezar |
| Intro | Start | Pausa |
| S | | La música, sí o no |
| Esc | | Al título; en el título, al BASIC |

## Sus niveles, sus gráficos y su sonido, a tu gusto

Todo lo que se ve y se oye está en ficheros que se editan con las
herramientas de ZX-V, y el programa los lleva dentro. Si en la carpeta desde
la que se carga hay uno con el mismo nombre, usa ese: para cambiar un
laberinto, un dibujo o una música no hace falta compilar nada. Uno que no
vale (o no cabe) da un informe y vuelve al BASIC.

| Fichero | Qué tiene | Se edita con |
|---|---|---|
| [`umbra.gfx`](umbra.gfx) | Los niveles, las texturas y los sprites | `zxv-gfx` en el PC o `gfxedit` en la máquina |
| [`umbra-title.pt3`](umbra-title.pt3) | La música del título | `zxv-tracker` |
| [`umbra-maze.pt3`](umbra-maze.pt3) | La del laberinto | `zxv-tracker` |
| [`umbra-clear.pt3`](umbra-clear.pt3) | Nivel superado | `zxv-tracker` |
| [`umbra-over.pt3`](umbra-over.pt3) | Fin de la partida | `zxv-tracker` |
| [`umbra.afb`](umbra.afb) | Los efectos, un banco AYFX | `zxv-tracker` |

### Los niveles

Cada nivel es un **mapa** de `umbra.gfx`: el 0 es el primero, el 1 el
segundo, y así los que haya seguidos (hasta 16, de hasta 32 × 32 celdas).
Cada celda del laberinto es una celda del mapa, y el número de su tile dice
lo que hay en ella. Los tiles de 8 × 8 del fichero tienen un dibujo de cada
cosa, para ver el laberinto en el editor y pintarlo con ellos:

| Tile | En la celda |
|---|---|
| 0 | Suelo |
| 1 | Pared de piedra |
| 2 | Pared de ladrillo |
| 3 | Pared de musgo |
| 4 | Pared de cristal |
| 5 | Pared del templo |
| 6 | Pared con una runa tallada |
| 7 | La puerta de salida (en una pared, a la que se llega por un lado) |
| 8 | Una runa |
| 9 | Un frasco de aceite |
| 10 | Una sombra, donde empieza |
| 11–14 | La entrada: donde empieza el jugador, mirando al este, al sur, al oeste o al norte |

Lo que está fuera del mapa es pared, y lo que no es ninguno de estos
números, también. Cada nivel necesita una entrada y una puerta; sin runas,
la puerta está abierta desde el principio. El suelo y el techo toman los
colores de la pared que más hay en el nivel. Caben 24 runas y frascos y 8
sombras por nivel.

### Las texturas y los sprites

| Textura | |
|---|---|
| 0 | Paredes, 128 × 128: cuatro de 64 × 64, la piedra, el ladrillo, el musgo y el cristal |
| 1 | Paredes, 128 × 128: el templo, la puerta cerrada, la puerta abierta y la pared con la runa |
| 2 | El suelo, 128 × 128: cada 64 × 64 es una celda |
| 3 | El techo, 64 × 64 |

El suelo y el techo se dibujan con los colores 1 (oscuro), 7 (medio) y 15
(claro), que cada nivel cambia por los suyos; el 0 se queda negro.

| Sprite | |
|---|---|
| 0 | La runa |
| 1 | El frasco de aceite |
| 2–4 | La sombra, que se anima con los tres; el 10 (rojo brillante) son sus ojos, que se ven a oscuras |
| 5 | La lámpara del marcador (también las de repuesto, más pequeñas) |
| 6 | La runa pequeña del marcador |

En el laberinto una celda mide lo que 64 píxeles de una sombra o 96 de un
objeto.

### Los efectos

Por su número en `umbra.afb`: 0 coger una runa, 1 un frasco, 2 una sombra te
toca, 3 la puerta se abre, 4 la lámpara se apaga, 5 un paso, 6 un pitido, 7
el latido y 8–10 el susurro de una sombra, de lejos a cerca. Suenan en el
segundo AY; la música, en el primero.

## Cómo está hecho

| Fichero | |
|---|---|
| `umbra.h` | Lo que comparten las partes: tamaños, las celdas, los objetos y las sombras, el sonido |
| `video.c` | La pantalla: doble buffer, la fuente de 5 × 7 y los sprites |
| `art.c` | Los gráficos y los niveles de `umbra.gfx` (con `zxv_gfxfile.h`) |
| `view.c` | La vista en 3D: las paredes, el suelo y el techo, la niebla y los objetos |
| `maze.c` | Los laberintos (los mapas), los caminos y las sombras |
| `sound.c` | La música y los efectos, con `zxv_music.h` |
| `game.c` | El jugador, la lámpara, el marcador, el mapa, el título y la demostración |

- **Las paredes, por raycasting:** la vista son las 144 líneas de arriba, y
  cada columna de 2 píxeles (128 en total) lanza un rayo que avanza por la
  cuadrícula del laberinto, de un borde de celda al siguiente, hasta dar con
  una pared. Su distancia da la altura de la columna, y el sitio donde da,
  la columna de la textura. Se escribe directamente en la página de vídeo,
  un byte por línea: los dos píxeles del byte, del mismo color.
- **El suelo y el techo, con el modo 7:** la misma cámara que los rayos
  (`zxv_m7_camera`, con la misma tabla de senos que el motor) da las líneas
  del suelo; el techo usa las del otro lado del horizonte. Solo se dibujan
  las líneas que alguna pared deja ver.
- **La niebla:** cada nivel de niebla es una tabla de colores: sin brillo,
  luego la mitad de los píxeles en negro (un tablero), luego tres de cada
  cuatro, y al final negro. Las paredes la llevan columna a columna; el
  suelo y el techo, línea a línea, con una tabla de 256 para cada byte (sus
  dos píxeles), que además les pone los colores del nivel; los sprites, con
  la tabla de colores de `zxv_spr_draw_scaled`. El alcance lo da el aceite
  que queda.
- **Los objetos y las sombras** son sprites escalados, de lejos a cerca, y
  se cortan por las paredes que tienen delante: con la distancia de cada
  columna, se dibujan solo en los tramos de columnas en los que la pared
  está más lejos (`zxv_gfx_clip`).
- **Los caminos:** un recorrido en anchura desde el jugador da a cada celda
  los pasos que hay hasta él. Las sombras lo usan para perseguirte y la
  demostración, el suyo desde lo que busca.
- **La memoria de vídeo que no se ve:** en el modo 0 la pantalla solo usa
  24 KB de cada página de vídeo de 96. Ahí van las texturas, ya listas para
  el motor (la página 0), y los sprites preparados y lo que se cargue de la
  tarjeta (la página 1).
- **Sin coma flotante:** las posiciones van en coma fija 16.16, en celdas;
  los ángulos, en 1/1024 de vuelta.
- **A 60 imágenes por segundo:** las velocidades van por segundo
  (`ZXV_FRAME_HZ`), y lo que sobra de lo que toca en cada frame pasa al
  siguiente.

## Rendimiento

Compilado con `make -C sdk -B build/umbra.zxv EXTRA_CFLAGS=-DUMBRA_STATS`,
el marcador enseña los frames perdidos y lo que tardó el frame más lento del
último segundo, en microsegundos, y la demostración juega todos los niveles
sin gastar aceite. Con `EXTRA_CFLAGS=-DUMBRA_SCORE=9876543` la partida
empieza con esos puntos, para ver el marcador con los números más grandes.
En el emulador (a 144 MHz) un frame del laberinto tarda menos de 7 ms de los
16,7 de un frame a 60 por segundo, y el más lento, el que empieza un nivel
(prepara las tablas de colores del suelo y del techo), unos 12,5; no se
pierde ninguno. En la
Pico 2 no está medido.

