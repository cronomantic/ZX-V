[English](README.md) · **Español**

# The Mine

Un juego de cavar, de piedras y diamantes, en C, para ZX-V. El minero
cava la tierra juntando los diamantes que pide cada cueva antes de que se
acabe el tiempo; entonces se abre la salida. Las piedras y los diamantes
caen y ruedan, y lo que le cae encima al minero, lo mata. Usa los mapas de
tiles y los sprites de la ROM, el doble buffer y los dos AY.

```sh
make -C sdk                       # build/mine.zxv
make -C sdk run SD=mi_tarjeta     # y en el BASIC: LOAD "mine"
```

También está en el disco ROM de la máquina: `LOAD "ROM:mine"`.

## El juego

- **Doce cuevas**, diez normales y dos de premio, más grandes que la
  pantalla (40 × 22 celdas de 16 × 16 píxeles, 640 × 352 píxeles): la
  entrada, las vetas, las ascuas, la cascada, la lluvia (premio), los
  murciélagos, el musgo, el tamiz, el laberinto, el sótano (premio), la
  avalancha y el corazón de la mina. Las primeras son fáciles: mucha
  tierra, pocas piedras y más diamantes de los que hacen falta.
- **Las reglas:** las piedras y los diamantes caen al hueco y ruedan desde
  lo redondo (piedras, diamantes y ladrillos) si al lado y debajo hay
  hueco. Una piedra apoyada en la cabeza del minero no le hace nada; una que
  ya está cayendo, o una de debajo de la que se aparta quedándose abajo, lo
  mata. Las piedras se empujan de lado (insistiendo un momento). El acero
  no se rompe; todo lo demás, menos la salida, salta con las explosiones.
- **Dos bichos** que siguen las paredes: las ascuas van con la pared a su
  izquierda y estallan en hueco; los murciélagos, con la pared a su
  derecha, y estallan en nueve diamantes. Los dos estallan si tocan al
  minero o el musgo, y si les cae algo encima.
- **El musgo** crece despacio al principio y luego deprisa. Si queda
  encerrado y ya no puede crecer, se vuelve diamantes; si se hace demasiado
  grande, piedras.
- **El tamiz** se pone en marcha con lo primero que le cae encima: durante
  un rato, una piedra que lo atraviesa sale por debajo hecha diamante, y un
  diamante, hecho piedra. Después es una pared más.
- **Tiempo, puntos y vidas:** cada cueva tiene un tiempo (los diez últimos
  segundos suenan); los diamantes necesarios valen unos puntos y los de más,
  más; en la salida se suman los segundos que quedan. Tres vidas, una más
  cada 500 puntos. En las cuevas de premio no se pierden vidas.
- **En el título,** izquierda y derecha eligen la cueva de salida entre las
  cuatro primeras.
- **Récord** en `SD:mine.txt` (en la tarjeta, aunque el juego se cargue
  desde `ROM:`, que no se puede escribir). Salir de la partida con Esc
  también cuenta. Sin tarjeta, dura lo que la sesión.
- **Demostración:** sin tocar nada en el título, a los 12 segundos un
  piloto automático juega una de las tres primeras cuevas (cada vez, la
  siguiente), con la música del título.
- **Textos en español o en inglés**, según el idioma de la máquina
  (`zxv_lang_get`).

| Tecla | Mando | |
|---|---|---|
| Cursores o OPQA | Cruceta | Moverse (y cavar, coger, empujar) |
| Espacio o M + dirección | Botón 1 o 2 + dirección | Cavar o coger sin moverse |
| Intro | Start | Pausa |
| Esc | | Al título; en el título, al BASIC |

## Cómo está hecho

| Fichero | |
|---|---|
| `mine.h` | Lo que comparten las partes: tamaños, tiles, sprites, sonido |
| `cave.h`, `cave.c` | Las reglas de la cueva y el piloto automático de la demostración, sin nada de la máquina (se compila también en el anfitrión) |
| `caves.c` | Las doce cuevas, dibujadas con caracteres |
| `draw.c` | La pantalla: doble buffer, la fuente de 5 × 7 (con los acentos y la Ñ) y los sprites |
| `art.c` | Los tiles de la cueva, dibujados con el motor al empezar, y el minero, dibujado con caracteres |
| `sound.c` | La música (primer AY) y los efectos (segundo AY) |
| `game.c` | La partida, el desplazamiento, el título, la demostración y el récord |

- **Pasos y píxeles:** la cueva avanza a pasos de 100 o 120 ms (según la
  cueva): se recorre de arriba abajo y cada cosa hace lo suyo una vez. Pero
  lo que se mueve en un paso se dibuja entre su celda de antes y la nueva,
  frame a frame, así el minero, las piedras que caen y los bichos se
  deslizan en vez de saltar 16 píxeles. A 60 imágenes por segundo, un paso
  de 120 ms son 7,2 frames: el juego cuenta lo que le queda a cada paso y
  dibuja en proporción, así que lo que se mueve va a velocidad fija aunque
  unos pasos duren 7 frames y otros 8.
- **El desplazamiento:** la ventana de 256 × 176 sigue al minero píxel a
  píxel cuando se aleja del centro, sin salirse de la cueva. Cada frame se
  dibujan en la página oculta las celdas que se ven de un mapa de tiles
  (`zxv_map_draw`, tiles opacos de 16 × 16), encima los sprites de lo que se
  mueve y arriba el marcador; la página se enseña en el siguiente VBLANK
  (`zxv_set_page` y `zxv_wait_vblank`).
- **Gráficos sin ficheros:** la tierra, el ladrillo, el acero, las piedras,
  los diamantes (en cuatro brillos), la salida, el musgo, el tamiz, las
  explosiones y los bichos se dibujan con los rectángulos, círculos, líneas
  y polígonos del motor sobre un bitmap (`zxv_gfx_target`), que es también
  el almacén de los tiles. La tierra y el ladrillo se repintan con los
  colores de cada cueva. Las piedras, los diamantes y los bichos tienen el
  fondo transparente (el color 8, que el mapa opaco pinta negro), así que
  los mismos dibujos se preparan como sprites (`zxv_spr_prepare`). El
  minero (quieto, parpadeando, andando, empujando, trepando y contento)
  está dibujado con caracteres.
- **Las cuevas** son líneas de caracteres: paredes, acero, piedras,
  diamantes, bichos, musgo, el tamiz, la entrada y la salida, y `.` para la
  tierra que se rellena al azar con piedras, diamantes y huecos en las
  proporciones de cada cueva, siempre igual (cada cueva tiene su semilla).
  Cada cueva se destapa desde la entrada mientras se ve su nombre.
- **El piloto automático** busca hacia atrás, desde sus objetivos (la
  salida abierta, los diamantes, una piedra que empujar, tierra que cavar),
  lo que cuesta llegar desde cada celda, lejos de los bichos y de lo que va
  a caer, y con cavar bajo una piedra más caro (al irse el minero, cae y
  tapa el paso). Da el mejor paso solo si una simulación de los pasos
  siguientes (con una copia de la cueva) dice que el minero sigue vivo,
  quieto o apartándose.
- **El sonido:** la música del título va en el primer AY (melodía, bajo y
  batería con el ruido); los efectos, en el segundo: el canal A los avisos
  (diamante, salida abierta, el tiempo que se acaba), el B el ruido de
  cavar, empujar, las piedras que caen y las explosiones (con la
  envolvente), y el C los tintineos de los diamantes que caen, el tamiz y la
  cuenta de la bonificación.
- **El teclado:** cada frame se lee el mapa de teclas pulsadas
  (`ZXV_I_KEYMAP`); manda la última dirección pulsada, y una pulsación corta
  entre dos pasos no se pierde. La cola de teclas se vacía cada frame para
  que lo tecleado no le llegue al BASIC al volver.

## Las cuevas se pueden hacer

Compilado con `make -C sdk -B build/mine.zxv EXTRA_CFLAGS=-DMINA_BOT` y
ejecutado con `zxv-cli --test`, el piloto de la demostración juega las doce
cuevas seguidas, con todo su dibujo, y al final de cada una escribe si la
ha terminado, en cuántos segundos, cuántos diamantes ha cogido, las vidas
que ha perdido y los frames perdidos. Las termina todas sin perder ninguna
vida, en 5 a 30 segundos cada una (los límites son de 20 a 170). Como
`cave.c` no depende de la máquina, el mismo piloto se ha probado también en
el anfitrión, más deprisa, y con un «jugador indeciso» que solo da un paso
de cada dos a seis cuando quedarse quieto no es peligroso: sigue terminando
todas las cuevas a tiempo, menos, a algunas de esas velocidades, el musgo y
el corazón, donde acaba titubeando al lado de una ascua hasta que se acaba
el tiempo.

## Rendimiento

Compilado con `make -C sdk -B build/mine.zxv EXTRA_CFLAGS=-DMINA_STATS`, el
marcador enseña los frames perdidos. En el emulador (a 144 MHz) no se
pierde ninguno en las doce cuevas, con el piloto simulando los pasos
siguientes: el frame más lento tarda unos 4,4 ms de los 16,7 que dura un
frame a 60 por segundo. No se ha medido en la Pico 2.

