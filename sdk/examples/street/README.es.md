[English](README.md) · **Español**

# Street Justice

Un juego de peleas por las calles, en C, para ZX-V: Kai avanza por el barrio
a puñetazo limpio contra los matones de las bandas, al estilo de los de las
recreativas. Usa el motor gráfico de la ROM (polígonos, círculos, lienzos que
se convierten en sprites, copias de bitmaps), el doble buffer y los dos AY.

```sh
make -C sdk                       # build/street.zxv
make -C sdk run SD=mi_tarjeta     # y en el BASIC: LOAD "street"
```

También está en el disco ROM de la máquina: `LOAD "ROM:street"`.

## El juego

- **Tres fases:** la calle de noche (bares, tiendas con el cierre echado y
  pintadas, un callejón), el mercado de día y las azoteas al atardecer. Se
  anda hacia la derecha; la pantalla se queda quieta hasta acabar con cada
  grupo de matones, y entonces «GO» deja seguir.
- **Profundidad:** los luchadores andan por una franja del suelo, hacia el
  fondo o hacia delante, y se dibujan por orden de profundidad. Un golpe da
  si el otro está delante, a su alcance y casi a la misma altura del suelo.
- **Los golpes de Kai:** una serie de puñetazos (directo, cruzado y
  gancho, que tira al suelo), el salto y la patada en el aire, y la patada
  giratoria (los dos botones a la vez), que da a los dos lados. Andando
  contra un enemigo se le agarra: pegar da rodillazos y saltar lo lanza.
- **Los matones:** el macarra (equilibrado), el corredor (rápido, con
  patadas) y el armario (lento, aguanta mucho y embiste). Solo dos atacan a
  la vez; los demás rondan esperando su turno. Al final de cada fase, el jefe
  de la banda: Llave, Cadenas y El Jefe, cada uno más duro.
- **Lo que se rompe:** cubos de basura, cajas y barriles, con comida dentro
  (una manzana, pan o pollo) que devuelve vida.
- **Tres vidas**, una más cada 30 000 puntos, bonificación al acabar cada
  fase y por las vidas que queden al final.
- **Récord** en `SD:street.txt` (en la tarjeta, aunque el juego se cargue
  desde `ROM:`, que no se puede escribir). Sin tarjeta, dura lo que la
  sesión.
- **Demostración:** sin tocar nada en el título, a los 12 segundos el piloto
  automático juega una fase (cada vez, la siguiente).
- **Textos en español o en inglés**, según el idioma de la máquina
  (`zxv_lang_get`).

| Tecla | Mando | |
|---|---|---|
| Cursores o OPQA | Cruceta | Andar |
| Espacio o M | Botón 1 | Pegar (contra un enemigo agarrado, rodillazos) |
| N o Z | Botón 2 | Saltar (en el aire, patada; agarrando, lanzar) |
| Los dos a la vez | Los dos | Patada giratoria |
| Intro | Start | Pausa |
| Esc | | Al título; en el título, al BASIC |

## Cómo está hecho

| Fichero | |
|---|---|
| `street.h` | Lo que comparten las partes: tamaños, los luchadores, las poses, los objetos |
| `draw.c` | La pantalla: doble buffer, la fuente de 5 × 7 (con los acentos y la Ñ), los lienzos en los que se dibujan los sprites, las sombras y los trazos gruesos |
| `art.c` | Los luchadores (un esqueleto en poses) y los decorados de las tres fases |
| `fight.c` | Los luchadores: el jugador, los matones y los jefes, sus golpes, la inteligencia de los enemigos, lo que se rompe y lo que se come, y el piloto de la demostración |
| `sound.c` | La música (primer AY) y los efectos (segundo AY) |
| `game.c` | Las fases, la cámara, los grupos, el marcador, el título, la demostración y el récord |

- **Doble buffer:** cada frame se dibuja entero en la página oculta, que se
  enseña en el siguiente VBLANK. Un frame perdido se nota en
  `ZXV_SV_FRAMES`.
- **Luchadores sin ficheros:** cada uno es un esqueleto (muslos, piernas,
  torso, brazos) con sus largos y anchos, puesto en una de 22 poses (andar,
  los golpes, el salto, caer, levantarse...) con ángulos de 64 pasos por
  vuelta. Al empezar se dibuja cada pose con polígonos y círculos del motor
  en un lienzo, con su contorno, y se guarda como sprite; las cabezas van
  aparte, y cada matón lleva sus colores en una tabla que se le pasa al
  sprite al dibujarlo (`zxv_spr_draw`). En la
  partida solo se copian sprites.
- **La escala:** los luchadores miden unos 50 píxeles, y los decorados están
  hechos a su medida: en la calle se ve la planta baja, con puertas y
  escaparates de unas 1,35 veces un luchador y los letreros por encima de
  las cabezas, y los edificios siguen por encima de la pantalla.
- **Los decorados** se hacen al empezar la fase con el motor en bitmaps: el
  fondo lejano (que se desplaza a un cuarto de velocidad), seis fachadas de
  64 píxeles que se repiten en el orden de la fase, y el suelo. Cada frame
  son unas pocas copias de bitmaps.
- **Las posiciones** van en 1/16 de píxel: nada de coma flotante.
- **El teclado:** se lee el mapa de teclas pulsadas (`ZXV_I_KEYMAP`) para
  andar y pegar a la vez, y se vacía la cola de teclas cada frame para que lo
  tecleado no le llegue al BASIC al volver.

## La dificultad

Compilado con `make -C sdk -B build/street.zxv EXTRA_CFLAGS=-DSTREET_BOT=1`
(el número es la semilla) y ejecutado con `zxv-cli --test`, el piloto de la
demostración juega las tres fases desde el principio, recibe los golpes como
un jugador y escribe, al terminar cada fase, las vidas que ha perdido,
cuántas con el jefe, los golpes recibidos, lo que ha tardado y los frames
perdidos. No se queda nunca sin vidas, para poder medir la partida entera.

Con él se ajustó la dificultad: en una partida, el piloto pierde de 1 a 3
vidas, casi siempre con los jefes, y cada fase le dura unos dos minutos.

## Rendimiento

Compilado con `EXTRA_CFLAGS=-DSTREET_STATS`, el marcador enseña los frames
perdidos. En el emulador (a 144 MHz) no se pierde ninguno, ni con cinco
luchadores en pantalla. En la Pico 2 no está medido.

