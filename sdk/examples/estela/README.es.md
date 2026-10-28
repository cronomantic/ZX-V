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
| Esc | | Al título; en el título, al BASIC |

## Cómo está hecho

| Fichero | |
|---|---|
| `estela.h` | Lo que comparten las partes: tamaños, sprites, sonido, el generador aleatorio y la tabla del seno |
| `draw.c` | La pantalla: doble buffer, la fuente de 5 × 7 (con los acentos y la Ñ), sprites dibujados con caracteres o con el motor, colisiones y las estrellas en tres capas |
| `sound.c` | La música (primer AY), los efectos (segundo AY) y las muestras digitalizadas |
| `game.c` | Las fases, el terreno, los enemigos, los jefes, el título, la demostración y el récord |

- **Doble buffer:** cada frame se dibuja entero en la página oculta
  (`zxv_gfx_draw_page`), que se enseña en el siguiente VBLANK
  (`zxv_set_page` y `zxv_wait_vblank`). Un frame perdido se nota en
  `ZXV_SV_FRAMES`.
- **Sprites sin ficheros:** la nave, los dardos y las torretas son dibujos
  con caracteres en el código (`sprite_ascii`); las rocas, la mina, las
  explosiones y los jefes se dibujan con círculos, rectángulos y polígonos
  del motor en un lienzo (`zxv_gfx_target`) y se preparan como sprites. Todo
  al arrancar, en un depósito de 48 KB.
- **El terreno** es un mapa de tiles de 512 × 24 celdas hecho al empezar la
  fase (un camino aleatorio con un hueco mínimo y una zona llana para el
  jefe). Se dibujan solo las franjas de arriba y de abajo donde hay pared, y
  los choques se miran con `zxv_map_cell`.
- **Las posiciones** van en 1/16 de píxel, y los ángulos en 64 pasos por
  vuelta con una tabla del seno: nada de coma flotante.
- **El sonido:** la música va por el primer AY (melodía, bajo y batería con
  el ruido); los efectos, por el segundo (disparos y mejoras en el canal A,
  explosiones con ruido y envolvente en el B). Las explosiones grandes llevan
  además una muestra de 0,6 s a 8 kHz por el volumen del canal C, mandada
  poco a poco, unos 30 ms por delante, con `zxv_ay_event` (spec §5.4).
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
solo su centro (7 × 3 píxeles). Con seis semillas, el piloto pierde de 2 a
7 vidas en toda la partida, y el último jefe le dura unos 40 segundos.

## Rendimiento

Compilado con `make -C sdk -B build/estela.zxv EXTRA_CFLAGS=-DESTELA_STATS`,
el marcador enseña los frames perdidos y la demostración juega las tres fases
enteras, con sus jefes. En el emulador (a 144 MHz) no se pierde ninguno, ni
con decenas de balas en pantalla. En la Pico 2 no está medido.

