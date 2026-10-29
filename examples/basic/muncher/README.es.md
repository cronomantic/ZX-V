[English](README.md) · **Español**

# Muncher

Un juego en el BASIC de ZX-V al estilo de *Pac-Man*: cómete todos los puntos
del laberinto mientras cuatro perseguidores te buscan, cada uno a su manera.
Una píldora da la vuelta a la tortilla durante unos segundos. Para ver los
mapas de tiles, los sprites sobre un fondo quieto, el doble buffer y las
estructuras nuevas del BASIC (el IF de bloque, `REPEAT` y los procedimientos)
en un juego que va a 60 imágenes por segundo. Los textos siguen el idioma de
la máquina.

| Fichero | Contenido |
|---|---|
| `muncher.bas` | El programa, en texto con números de línea |

## Probarlo

`muncher.bas` es un listado en texto: no hace falta convertirlo. Cópialo a
la tarjeta y escribe:

```
LOAD "muncher"
RUN
```

También viene en el disco ROM de la máquina: `LOAD "ROM:muncher"`.

| Control | Qué hace |
|---|---|
| Cursores o O, P, Q, A | Izquierda, derecha, arriba y abajo |
| Mando | La cruceta mueve; cualquier botón empieza la partida en el título |
| Espacio o Enter | Empieza la partida en el título |
| Esc | Para el programa (BREAK) |

La última dirección que pulsas se recuerda: púlsala antes de llegar a un
cruce y girarás en cuanto el pasillo lo permita.

## El juego

- **Los perseguidores** salen de su casa, en el centro, uno detrás de otro, y
  cada uno tiene su carácter:
  - **Rufo** (rojo) va derecho a por ti.
  - **Lila** (magenta) apunta cuatro casillas por delante de ti, para
    cortarte el paso.
  - **Niebla** (cian) vigila su rincón (abajo a la izquierda): solo te
    persigue cuando entras en su parte del laberinto.
  - **Chispa** (verde) te persigue, pero en muchos cruces tira por cualquier
    lado.
- **Retirada y persecución:** de vez en cuando (unos 6, 17, 6, 17, 4, 17 y 4
  segundos) dejan de perseguirte y cada uno se va un rato a su rincón; en cada
  cambio dan la vuelta.
- **Las píldoras** (los cuatro puntos gordos, 50 puntos): los perseguidores se
  vuelven azules, dan la vuelta y huyen; puedes comértelos por 200, 400, 800 y
  1600 puntos. Parpadean en blanco justo antes de recuperarse. Uno comido
  vuelve a casa hecho un par de ojos y sale otra vez.
- **El túnel** de los lados te lleva al otro lado; los perseguidores van más
  despacio por él.
- **La fruta:** dos veces por nivel (a los 70 y a los 170 puntos comidos) sale
  una fruta bajo la casa durante ocho segundos: una cereza (100 puntos), un
  limón (300), uvas (500), una seta (1000)... Arriba se ve la del nivel.
- **Niveles:** tras los 224 puntos, las paredes parpadean y empieza el
  siguiente, con los perseguidores más rápidos y el susto más corto. Tres
  vidas, y una más a los 10.000 puntos.
- **Récord** en `SD:muncher.txt` (en la tarjeta, aunque el juego se cargue
  desde `ROM:`).
- **Demostración:** si no tocas nada en el título, a los 10 segundos juega un
  piloto automático: en cada cruce mira por cada pasillo, cuenta los puntos
  que ve y se aparta de los perseguidores.

## Cómo funciona

- **El laberinto** está escrito en `DATA` como texto, una fila en cada cadena
  (`#` pared, `.` punto, `o` píldora, `=` la puerta de la casa), y solo la
  mitad izquierda: la derecha es su reflejo. De ahí salen una tabla de por
  dónde se puede pasar (`m`) y el mapa de tiles.
- **Las paredes** se dibujan una vez al empezar en la página oculta: cada
  casilla de pared lleva una línea en los lados que dan a un pasillo, metida
  dos píxeles, con las esquinas redondeadas. `TILE 8,100,0,8,256,184` corta el
  dibujo entero en tiles (uno por casilla), y el mapa los usa para las paredes
  y otros tres tiles para el pasillo vacío, el punto y la píldora. Comerse un
  punto es un `MAP POKE` con el tile vacío; `CELL` dice qué hay en una
  casilla.
- **En cada imagen**, en la página oculta: `MAP 0` dibuja todo el laberinto de
  una vez, después los cinco sprites y el marcador con `PRINT`; luego `PAGE` y
  `WAIT 1`. No hay que borrar nada, porque el mapa lo tapa todo.
- **Los sprites** son de 10 por 10 píxeles y están escritos en `DATA` como
  cadenas de cifras hexadecimales (`GRAB n,10,a$`). El jugador mirando arriba
  es el que mira a la derecha girado un cuarto de vuelta (`PROC gira`); la
  izquierda y abajo son reflejos (el cuarto argumento de `SPRITE`). Los
  perseguidores son un mismo cuerpo rojo con los ojos de cada dirección
  encima, y la cadena de colores de `SPRITE` lo pinta del color de cada uno.
  El jugador que desaparece es un `DISC` con una boca recortada con `POLY`,
  cada vez más abierta.
- **El movimiento** va de píxel en píxel; en el centro de cada casilla, el
  perseguidor elige la casilla vecina más cercana a su blanco (sin dar nunca
  la vuelta), como en la recreativa, y el jugador gira si puede. El jugador
  anda siempre un píxel por imagen (60 por segundo), sin paradas ni saltos;
  las velocidades de los perseguidores son fracciones de eso, que se van
  sumando (`f(i)`), en la misma proporción que cuando el juego iba a 50
  imágenes por segundo. Los tiempos del juego (el susto, las retiradas,
  cuándo sale cada uno, la fruta) se cuentan en imágenes, tantas como
  entonces: en el laberinto el juego es el mismo, solo que una quinta parte
  más rápido.
- **El sonido:** `SOUND` en el segundo chip: los puntos y los efectos en el
  canal A, y en el canal B una sirena que cambia con el susto y cuando hay ojos
  volviendo a casa. `PLAY` toca la música del principio de cada nivel en el
  primer chip.
- **El idioma:** `PEEK 536872736` y `PEEK 536872737` son las dos letras del
  idioma de la máquina, y `DEF FN t$(a$,b$)` elige el texto con el truco de
  siempre del Spectrum: `a$ AND n` es `a$` si `n` no es 0, y `""` si lo es.

## Velocidad

El juego va a 60 imágenes por segundo (medido en el emulador con `TIME`), y
la demostración, cuyo piloto piensa más, también. Para que siga así:

- **Lo que se hace en cada imagen va primero** (líneas 100 a 2640), y lo que
  se hace una vez (los gráficos, el título, las pantallas entre niveles),
  después.
- **Poco trabajo en cada imagen:** cada uno avanza un píxel y solo piensa en
  el centro de una casilla, una vez cada ocho píxeles; el laberinto, los
  sprites y los cambios de color son órdenes gráficas que la ROM hace de una
  vez.
- **Cuentas con enteros:** las direcciones son tablas (`u`, `v`, `o`), y las
  distancias, `dx*dx+dy*dy` en casillas, sin `SQR`.
- **Las variables del bucle se crean las primeras**, nada más empezar.
