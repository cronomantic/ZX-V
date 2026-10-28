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
| Esc | | Al título; en el título, al BASIC |

## Cómo está hecho

| Fichero | |
|---|---|
| `route128.h` | Lo que comparten las partes: tamaños, los rellenos, los sprites, el circuito y el sonido |
| `video.c` | La pantalla: doble buffer, los tramos de color de la carretera, la fuente de 5 × 7 y los sprites |
| `art.c` | Los gráficos, dibujados con el motor al empezar, y el paisaje de cada etapa |
| `road.c` | El circuito y su proyección: la carretera, el paisaje, los objetos y los coches |
| `sound.c` | La música (primer AY) y el coche (segundo AY) |
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
- **Los gráficos** se dibujan al arrancar con polígonos, rectángulos y
  círculos del motor en un lienzo (`zxv_gfx_target`), más grandes de lo que
  se ven de lejos para que aguanten de cerca.
- **El paisaje:** nubes, montañas y el mar, las dunas o las lomas, en tres
  capas de 512 píxeles que se repiten y se desplazan a distinta velocidad
  con las curvas. Se dibujan de nuevo al cambiar de etapa.
- **El sonido:** el motor son dos tonos del segundo AY, uno una octava más
  grave, que siguen a las vueltas de la marcha; el derrape y la cuneta, su
  ruido; y la música, el primer AY.
- **Sin coma flotante:** las posiciones y la proyección van con enteros (la
  curva en 1/16, el lateral en 1/1024 de media carretera).

## Rendimiento

Compilado con `make -C sdk -B build/route128.zxv EXTRA_CFLAGS=-DRUTA128_STATS`,
el marcador enseña los frames perdidos y lo que tardó el frame más lento del
último segundo, en microsegundos, y la demostración corre el circuito entero.
En el emulador (a 144 MHz) no se pierde ninguno: el peor frame tarda unos
10 ms de los 20 de un frame a 50 por segundo (la costa y el bosque, con más
objetos; el desierto, la mitad). En la Pico 2 no está medido.

