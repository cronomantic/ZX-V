[English](README.md) · **Español**

# Sentry

Un juego en el BASIC de ZX-V al estilo de *Missile Command*: los misiles
caen sobre seis ciudades y tú los derribas con el ratón, disparando desde tres
bases. Cada explosión derriba los misiles que toca, y cada misil derribado
estalla a su vez: en cadena. Para ver el ratón, los gráficos rápidos y las
estructuras nuevas del BASIC (el IF de bloque, `SELECT CASE` y los
procedimientos) en un juego que va a 60 imágenes por segundo.

| Fichero | Contenido |
|---|---|
| `sentry.bas` | El programa, en texto con números de línea |

## Probarlo

`sentry.bas` es un listado en texto: no hace falta convertirlo. Cópialo a
la tarjeta y escribe:

```
LOAD "sentry"
RUN
```

También viene en el disco ROM de la máquina: `LOAD "ROM:sentry"`.

| Control | Qué hace |
|---|---|
| Ratón | Mueve la mira |
| Botón del ratón | Dispara desde la base más cercana que tenga misiles |
| Cursores y espacio | Lo mismo, con el teclado |
| Mando | La cruceta mueve la mira y cualquier botón dispara |
| Esc | Para el programa (BREAK) |

No hace falta ratón: en el título, espacio o un botón del mando empiezan la
partida. Con los cursores o la cruceta, la mira empieza despacio, para
afinar, y acelera mientras se mantienen pulsados.

## El juego

- **Las oleadas:** cada una trae más misiles, más deprisa y más a la vez, y
  vale más puntos (el multiplicador sube cada dos). Desde la tercera, algunos
  misiles se dividen en tres a media altura.
- **Las bases:** cada una tiene diez misiles por oleada (las marcas blancas
  de debajo). Un misil enemigo que cae en una base la deja sin munición
  hasta la oleada siguiente, en la que se reconstruye.
- **Al acabar cada oleada**, puntos por los misiles que quedan y por las
  ciudades en pie; cada 10 000 puntos vuelve una ciudad destruida. La partida
  se acaba cuando no queda ninguna.
- **Récord** en `SD:sentry.txt` (en la tarjeta, aunque el juego se cargue
  desde `ROM:`).
- **Demostración:** sin tocar nada en el título, a los 10 segundos un piloto
  automático defiende las ciudades.
- **Textos en español o en inglés**, según el idioma de la máquina.

## Cómo está hecho

- **Los gráficos** se dibujan al empezar con `BOX`, `POLY` y `DISC` en la
  página oculta y se copian con `GRAB` como sprites: la ciudad y sus ruinas,
  la base, la mira, la marca del blanco y el fondo entero (las estrellas, la
  luna, las montañas y el suelo), que se dibuja de una vez cada frame. El
  nombre del título es un texto de `PRINT` copiado con `GRAB` y dibujado al
  triple de su tamaño con `SPRITE`.
- **Cada frame**, en la página oculta: el fondo, las ciudades y las bases, las
  estelas de los misiles (`POLY` con dos puntos es una línea), las
  explosiones (`DISC`, que crecen, se encogen y cambian de color), la mira y
  el marcador; luego `PAGE` y `WAIT 1`.
- **Los choques** son distancias: un misil está dentro de una explosión si
  `dx*dx+dy*dy` es menor que el radio al cuadrado. (`x^2` no sirve aquí: con
  `x` negativo da `A Argumento no válido`, como en el Spectrum.)
- **Las tablas** de los misiles, las explosiones y los blancos son matrices
  de dos dimensiones (los nombres de las matrices son de una letra); cada
  columna se explica en un `REM`.
- **El idioma:** `PEEK 536872736` y `PEEK 536872737` son las dos letras del
  idioma de la máquina, y `DEF FN t$(a$,b$)` elige el texto con el viejo
  truco del Spectrum: `a$ AND n` es `a$` si `n` no es 0, y `""` si lo es.
- **El sonido:** `SOUND` en el segundo chip (registros 16 a 31): un tono que
  baja para los disparos y ruido que se apaga para las explosiones. `PLAY`
  toca las fanfarrias entre oleada y oleada.

## La velocidad

Como el del Spectrum, el BASIC busca la línea de cada salto (`NEXT`,
`UNTIL`, `END PROC`, `PROC`) desde el principio del programa, y las
variables en el orden en que se crearon. Sentry se escribió cuando eso
pesaba (en ZX-V 0.3.0), y está ordenado para que vaya deprisa; ahora el
intérprete recuerda dónde encontró lo último que buscó y el orden
ya casi no importa, salvo con `SPEED 0`, que va al paso del 48K. Así está
hecho:

- **Lo que se hace en cada frame va al principio** (líneas 100 a 1999), y lo
  que se hace una vez (preparar los gráficos, el título, las pantallas entre
  oleadas), detrás. Con el bucle del juego al final del programa, iba a la
  mitad de velocidad.
- **Las matrices y las variables del bucle se crean primero**, nada más
  empezar.
- **Los bucles recorren solo los huecos usados** de cada tabla.

Así va a 60 imágenes por segundo hasta la octava oleada; en las siguientes,
con nueve misiles y varias explosiones a la vez, el juego se ralentiza un
poco, como les pasaba a las máquinas recreativas.
