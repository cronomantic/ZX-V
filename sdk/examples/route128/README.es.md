[English](README.md) · **Español**

# Route 128

Un juego de carreras en C para ZX-V, al estilo de los de recreativa de los
80: una carretera en pseudo-3D con curvas y cambios de rasante, y todo lo que
hay junto a ella, palmeras, cactus, pinos, carteles y los coches del tráfico,
hecho con sprites que se escalan según la distancia (`zxv_spr_draw_scaled`).
El nombre es un guiño a la Ruta 66 y al Spectrum 128.

```sh
make -C sdk                       # build/route128.zxv
make -C sdk run SD=mi_tarjeta     # y en el BASIC: LOAD "route128"
```

También está en el disco ROM de la máquina: `LOAD "ROM:route128"`.

## El juego

- **Contra el reloj:** tres etapas, la costa, el desierto y el bosque, cada
  una con su cielo, su paisaje y lo que hay en la cuneta. Cada control da
  tiempo extra; hay que llegar a la meta antes de que se acabe.
- **La carretera:** curvas suaves y cerradas, rectas largas, dunas, puertos
  de montaña y rasantes detrás de los que no se ve lo que viene. Las flechas
  avisan de las curvas cerradas.
- **La conducción:** dos marchas, LO y HI, que se cambian a mano. LO tira
  fuerte pero no pasa de 190 km/h; HI llega a 290, pero por debajo de 120
  apenas tira (para salir y después de un choque, LO). Pasar a LO yendo
  deprisa frena con el motor: para entrar en una curva cerrada y salir de
  ella tirando fuerte. Cada cambio da medio segundo de agarre, en el que la
  curva empuja la tercera parte: de HI a LO y otra vez a HI en mitad de una
  curva, para pasarla sin perder casi nada. La marcha parpadea en el
  marcador cuando toca cambiar. La curva empuja el coche hacia fuera, más cuanto más deprisa, así
  que en las cerradas hay que frenar o reducir; la cuneta frena y hace botar el coche; chocar con
  algo de la cuneta hace dar vueltas al coche, y alcanzar a otro coche, un
  golpe que frena.
- **Puntos** por la velocidad y, al llegar, mil por cada segundo que sobre.
  **Récord** en `SD:route128.txt` (en la tarjeta, aunque el juego se cargue
  desde `ROM:`).
- **Demostración:** sin tocar nada en el título, a los 12 segundos un piloto
  automático corre la primera etapa.
- **Textos en español o en inglés**, según el idioma de la máquina.

| Tecla | Mando | |
|---|---|---|
| Cursor arriba, Q o espacio | Arriba o botón 1 | Acelerar |
| Cursor abajo o A | Abajo o botón 2 | Frenar |
| Z | Botón 3 o Select | Cambiar de marcha (LO o HI) |
| Cursores izquierda y derecha, u O y P | Cruceta | Girar |
| Intro | Start | Pausa |
| S | | La música, sí o no |
| Esc | | Al título; en el título, al BASIC |

## Sus gráficos y su música, a tu gusto

Los gráficos están en [`route128.gfx`](route128.gfx), que se edita con
`zxv-gfx` en el PC o con `gfxedit` en la propia máquina, y las músicas en
módulos de Pro Tracker 3 que se editan con `zxv-tracker`:
[`route128-race.pt3`](route128-race.pt3) (la carrera),
[`-title`](route128-title.pt3) (el título), [`-goal`](route128-goal.pt3) (la
meta) y [`-over`](route128-over.pt3) (se acabó el tiempo). El programa los
lleva dentro, pero si en la carpeta desde la que se carga hay uno con ese
nombre usa ese: para cambiar un dibujo o una música no hace falta compilar
nada. Uno que no vale (o no cabe) da un informe y vuelve al BASIC.

Los sprites de `route128.gfx`, por su número:

| Número | |
|---|---|
| 0–9 | Junto a la carretera: palmera, arbusto, sombrilla, cactus, roca, pino, árbol, flechas de curva a la izquierda y a la derecha, y el cartel |
| 10–12 | Los arcos de la salida, los controles y la meta, en español |
| 13–15 | Los mismos, en inglés |
| 16–18 | Los rivales: cupé, furgoneta y camión (el 2 y el 3, la carrocería y sus brillos, se cambian por los colores de cada coche) |
| 20–24 | El coche, del más girado a la izquierda al más girado a la derecha |
| 30–38 | El paisaje de la costa, el desierto y el bosque: de cada etapa, las montañas, lo de cerca y las nubes; de 256 píxeles de ancho o más, y se repiten |

Los objetos se dibujan más pequeños cuanto más lejos están: su tamaño en el
fichero es el de cerca, y su ancho en el mundo (lo que ocupan en la
carretera) está en `art.c`.

## Cómo está hecho

| Fichero | |
|---|---|
| `route128.h` | Lo que comparten las partes: tamaños, los rellenos, los sprites, el circuito y el sonido |
| `video.c` | La pantalla: doble buffer, los tramos de color de la carretera, la fuente de 5 × 7 y los sprites |
| `art.c` | Los gráficos de `route128.gfx` (con `zxv_gfxfile.h`), y el paisaje de cada etapa |
| `road.c` | El circuito y su proyección: la carretera, el paisaje, los objetos y los coches |
| `sound.c` | La música (primer AY, con `zxv_music.h`) y el coche (segundo AY) |
| `game.c` | La conducción, el tráfico, los choques, el marcador, el título y la demostración |

- **La carretera** es una tira de segmentos, cada uno con su curva y su
  altura, que se construye por tramos (entrada, recta y salida, con curvas de
  aceleración suaves), como en *Lou's Pseudo 3d Page* (Louis Gorenfeld) y
  *Javascript Racer* (Jake Gordon). Cada
  frame se proyectan los extremos de los 160 segmentos que hay delante: la
  curva se acumula de uno a otro (cada segmento desvía un poco más los
  siguientes) y la altura sube o baja su línea en la pantalla.
- **Se pinta de cerca a lejos, línea a línea:** cada línea de la pantalla la
  pinta el segmento más cercano que la cubre (la hierba, el bordillo, el
  asfalto y las líneas de los carriles, con tramos de color escritos por
  palabras). Lo que hay detrás de una cresta no se pinta, y los objetos que
  hay detrás se recortan por ella: así se ven los cambios de rasante.
- **Sin grises:** la paleta de ZX-V no los tiene, así que el asfalto va
  tramado, a la mitad y a la cuarta parte de blanco, y la hierba y la arena,
  igual. Las bandas claras y oscuras cada tres segmentos dan la sensación de
  velocidad.
- **Los sprites escalados:** los objetos y los coches se dibujan de lejos a
  cerca con `zxv_spr_draw_scaled`, con el tamaño que da la distancia. Los
  coches del tráfico son tres dibujos (cupé, furgoneta y camión) que se
  recolorean al dibujarlos con la tabla de colores. El coche del jugador va a
  su tamaño, en cinco posturas según gire.
- **Los gráficos** son los de `route128.gfx`, más grandes de lo que se ven
  de lejos para que aguanten de cerca. Al empezar se preparan para el motor
  (`zxv_spr_prepare`).
- **La memoria de vídeo que no se ve:** el fichero de gráficos ocupa 110 KB,
  y con los sprites preparados no cabe en la RAM de usuario. Pero en el modo
  0 la pantalla solo usa 24 KB de cada página de vídeo de 96: el resto es
  memoria como la otra, y ahí van los sprites preparados, las músicas de la
  tarjeta y el paisaje de la etapa.
- **El paisaje:** nubes, montañas y el mar, las dunas o las lomas, en tres
  capas de 512 píxeles que se repiten y se desplazan a distinta velocidad
  con las curvas. Al llegar a una etapa se copian las suyas del fichero.
- **El sonido:** el motor son dos tonos del segundo AY, uno una octava más
  grave, que siguen a las vueltas de la marcha; el derrape y la cuneta, su
  ruido; y la música, en el primer AY, la biblioteca de música del SDK. La
  de la carrera dura minuto y medio antes de repetirse: una entrada, el
  tema, una parte en mi menor, un respiro con acordes, una subida y el tema
  otra vez, un tono más arriba.
- **Sin coma flotante:** las posiciones y la proyección van con enteros (la
  curva en 1/16, el lateral en 1/1024 de media carretera).
- **A 60 imágenes por segundo:** las velocidades, las aceleraciones y los
  tiempos van por segundo (`ZXV_FRAME_HZ`), y lo que sobra de lo que toca en
  cada frame pasa al siguiente: nada se mueve a tirones. La música y los
  efectos van a pasos de un cincuentavo de segundo, como se escriben.

## Rendimiento

Compilado con `make -C sdk -B build/route128.zxv EXTRA_CFLAGS=-DRUTA128_STATS`,
el marcador enseña los frames perdidos y lo que tardó el frame más lento del
último segundo, en microsegundos, y la demostración corre el circuito entero.
En el emulador (a 144 MHz) el peor frame tarda unos 11 ms de los 16,7 de un
frame a 60 por segundo (la costa y el bosque, con más objetos; el desierto,
menos de la mitad), y no se pierde ninguno en todo el circuito, con los
cambios de etapa, ni al pasar del título a la carrera y volver. En la Pico 2
no está medido.

