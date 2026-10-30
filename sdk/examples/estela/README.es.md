[English](README.md) · **Español**

# Estela

Un matamarcianos de tres fases, en C, para ZX-V: el juego nativo de ejemplo
del SDK. Usa el motor gráfico de la ROM (sprites, mapas de tiles, círculos,
polígonos), los dos AY y las escrituras del AY con su hora para una
explosión digitalizada.

```sh
make -C sdk                       # build/estela.zxv
make -C sdk run SD=mi_tarjeta     # y en el BASIC: LOAD "estela"
```

También está en el disco ROM de la máquina: `LOAD "ROM:estela"`.

## El juego

- **Tres fases:** el cinturón de asteroides (con un planeta al fondo), el
  cañón (paredes de roca arriba y abajo, con torretas) y la fortaleza
  (paredes de metal). Al final de cada una, tras el aviso «¡PELIGRO!», espera
  un jefe: la roca madre (anillos de balas y piedras), el guardián (ráfagas
  apuntadas y dardos) y el núcleo, que solo se puede dañar cuando se abre.
- **Enemigos:** dardos que ondulan, ruedas que disparan, rocas que se parten
  en dos, torretas en el suelo y en el techo, minas que persiguen a la nave y
  portadores que sueltan mejoras: el arma (doble, triple) y el escudo, que
  aguanta un golpe.
- **Tres vidas**, una más cada 20 000 puntos y una bonificación por las que
  queden al terminar. Al perder una vida, el arma se conserva.
- **Récord** en `SD:estela.txt` (en la tarjeta, aunque el juego se cargue
  desde `ROM:`, que no se puede escribir). Sin tarjeta, dura lo que la
  sesión.
- **Demostración:** sin tocar nada en el título, a los 12 segundos un piloto
  automático juega el final de una fase y su jefe (cada vez, la siguiente).
- **Textos en español o en inglés**, según el idioma de la máquina
  (`zxv_lang_get`).

| Tecla | Mando | |
|---|---|---|
| Cursores o OPQA | Cruceta | Mover |
| Espacio o M | Botón 1 o 2 | Disparar (manteniéndolo, en ráfaga) |
| Intro | Start | Pausa |
| S | | La música, sí o no |
| Esc | | Al título; en el título, al BASIC |

## Sus gráficos y su música, a tu gusto

Los gráficos están en [`estela.gfx`](estela.gfx), que se edita con
`zxv-gfx` en el PC o con `gfxedit` en la propia máquina, y las músicas en
módulos de Pro Tracker 3 que se editan con `zxv-tracker`:
[`estela-title.pt3`](estela-title.pt3) (el título),
[`-stage1`](estela-stage1.pt3), [`-stage2`](estela-stage2.pt3) y
[`-stage3`](estela-stage3.pt3) (una para cada fase),
[`-boss`](estela-boss.pt3) (los jefes), [`-over`](estela-over.pt3) (el final
de la partida) y [`-clear`](estela-clear.pt3) (fase superada). El programa
los lleva dentro, pero si en la carpeta desde la que se carga hay uno con
ese nombre usa ese: para cambiar un dibujo o una música no hace falta
compilar nada. Uno que no vale (o no cabe) da un informe y vuelve al BASIC.

| Sprite | |
|---|---|
| 0 y 1 | La nave y su escudo |
| 2, 3 y 4 | El dardo y la rueda (en dos posiciones) |
| 5 y 6 | La roca grande y la pequeña |
| 7, 8 y 9 | La torreta, la mina y el portador de mejoras |
| 10 | Las balas enemigas |
| 11 y 12 | Las mejoras: el arma y el escudo |
| 13–16 | La explosión, en cuatro pasos |
| 17–19 | Los jefes de las tres fases |

Y los tiles de 8 × 8 del terreno: vacío, roca, el borde de arriba y el de
abajo de la roca, metal, sus dos bordes y una luz.

## Cómo está hecho

| Fichero | |
|---|---|
| `estela.h` | Lo que comparten las partes: tamaños, sprites, sonido, el generador aleatorio y la tabla del seno |
| `draw.c` | La pantalla: doble buffer, la fuente de 5 × 7 (con los acentos y la Ñ), los sprites de `estela.gfx` (con `zxv_gfxfile.h`), colisiones y las estrellas en tres capas |
| `sound.c` | La música (primer AY, con `zxv_music.h`), los efectos (segundo AY) y las muestras digitalizadas |
| `game.c` | Las fases, el terreno, los enemigos, los jefes, el título, la demostración y el récord |

- **Doble buffer:** cada frame se dibuja entero en la página oculta
  (`zxv_gfx_draw_page`), que se enseña en el siguiente VBLANK
  (`zxv_set_page` y `zxv_wait_vblank`), 60 veces por segundo. Un frame
  perdido se nota en `ZXV_SV_FRAMES`.
- **El tiempo:** el juego se ajustó cuando la máquina iba a 50 imágenes por
  segundo, y a 60 (`ZXV_FRAME_HZ`) todo dura lo mismo en segundos: las
  velocidades siguen escritas en 1/16 de píxel cada 1/50 de segundo y `V()`
  las pasa a cada frame; los tiempos van en milisegundos con `MS()`. El
  terreno avanza un píxel por frame, así que las fases se miden en
  cincuentavos de segundo (sus píxeles a 50 Hz) y `PX()` los pasa a los
  píxeles de ahora: a 60, las fases son un 20 % más largas, y las oleadas,
  las torretas y el jefe salen a los mismos segundos.
- **Los sprites** son los de `estela.gfx`, preparados para el motor
  (`zxv_spr_prepare`) al arrancar, en un depósito de 48 KB.
- **El terreno** es un mapa de tiles de 512 × 24 celdas hecho al empezar la
  fase (un camino aleatorio con un hueco mínimo y una zona llana para el
  jefe). Se dibujan solo las franjas de arriba y de abajo donde hay pared, y
  los choques se miran con `zxv_map_cell`.
- **Las posiciones** van en 1/96 de píxel (así `V()` es exacta a 50 y a
  60 Hz), y los ángulos en 64 pasos por vuelta con una tabla del seno: nada
  de coma flotante.
- **El sonido:** la música va por el primer AY, con la biblioteca de
  música del SDK; los efectos, por el segundo (disparos y mejoras en el
  canal A, explosiones con ruido y envolvente en el B), a 50 pasos por
  segundo, como cuando se hicieron. Las explosiones grandes llevan además
  una muestra de 0,6 s a 8 kHz por el volumen del canal C, mandada poco a
  poco, por delante, con `zxv_ay_event` (spec §5.4). Va por la misma cola
  que la música, que tiene que ir en orden de hora: la muestra empieza
  detrás de lo que ya tiene la música y, mientras suena, la música va hasta
  cada muestra y no más allá (`zxv_music_update_until`).
- **El teclado:** se lee el mapa de teclas pulsadas (`ZXV_I_KEYMAP`) para
  mover y disparar a la vez, y se vacía la cola de teclas cada frame para que
  lo tecleado no le llegue al BASIC al volver.

## La dificultad

Compilado con `make -C sdk -B build/estela.zxv EXTRA_CFLAGS=-DESTELA_BOT=1`
(el número es la semilla) y ejecutado con `zxv-cli --test`, el piloto de la
demostración juega las tres fases desde el principio, recibe los golpes como
un jugador y escribe, al terminar cada fase, las vidas que ha perdido, de
qué (paredes, balas, enemigos o el jefe), cuántas con el jefe y lo que ha
tardado. No se queda nunca sin naves, para poder medir la partida entera.

Con él se ajustó la dificultad. Antes, el piloto perdía de 15 a 27 vidas en
una partida, de 9 a 15 en la fase 3, y el último jefe le duraba casi cuatro
minutos: cada vida perdida bajaba el arma, y con el arma al mínimo todo se
alargaba. Ahora el arma se conserva, los jefes tienen menos vida y disparan
menos, las ruedas, las torretas y los dardos van más despacio, la nave es
invulnerable un poco más tras un golpe y lo que cuenta para las balas es
solo su centro (7 × 3 píxeles). Con seis semillas, el piloto pierde de 4 a
7 vidas en toda la partida (de 2 a 7 cuando la máquina iba a 50 imágenes por
segundo), y el último jefe le dura unos 40 segundos; cada fase y cada jefe
duran lo mismo que entonces. A 60 imágenes por segundo, el piloto mira
también la pared que la nave aún tiene encima o debajo: con pasos más
cortos, se iba al hueco siguiente antes de tiempo y chocaba con el borde.

## Rendimiento

Compilado con `make -C sdk -B build/estela.zxv EXTRA_CFLAGS=-DESTELA_STATS`,
el marcador enseña los frames perdidos y la demostración juega las tres fases
enteras, con sus jefes. En el emulador (a 144 MHz y 60 imágenes por segundo)
no se pierde ninguno jugando, ni con decenas de balas en pantalla; solo uno
al arrancar, con lo que se prepara antes del primer frame. En la Pico 2 no está medido.

