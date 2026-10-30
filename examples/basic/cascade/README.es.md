[English](README.md) · **Español**

# Cascade

Un juego de gemas que caen, en el BASIC de ZX-V. Caen parejas de gemas en un
pozo de seis casillas de ancho y doce de hondo. Junta tres o más del mismo
color, en fila, en columna o en diagonal, y desaparecen; las de arriba caen,
y si hacen líneas nuevas, la cadena vale más. Sirve para ver un mapa de
tiles que cambia mientras se juega, sprites para lo que se mueve, el doble
buffer y los gráficos, la música y los efectos cargados de ficheros que se
pueden editar, todo a 60 imágenes por segundo. Los textos siguen el idioma de
la máquina.

| Fichero | Contenido |
|---|---|
| `cascade.bas` | El programa, en texto con números de línea |
| `cascade.gfx` | Los gráficos: las gemas, el pozo, la pared y el título |
| `cascade.pt3` | La música, un módulo de Pro Tracker 3 |
| `cascade.afb` | Los efectos de sonido, un banco AYFX |

## Probarlo

`cascade.bas` es un listado en texto: no hace falta convertirlo. Copia los
cuatro ficheros en la misma carpeta de la tarjeta y escribe:

```
LOAD "cascade"
RUN
```

También está en el disco ROM de la máquina: `LOAD "ROM:cascade"` y `RUN`.

| Control | Qué hace |
|---|---|
| Cursores izquierda y derecha, u O y P | Mueven la pareja (sin soltar, sigue moviéndose) |
| Cursor arriba, Q o Espacio | Gira a la derecha |
| Z | Gira a la izquierda |
| Cursor abajo o A | Baja deprisa |
| Mando | La cruceta mueve y baja, arriba y el botón 1 giran a la derecha, el botón 2 a la izquierda; cualquier botón empieza la partida en el título |
| M | Quita y pone la música |
| Espacio o Enter | Empieza la partida en el título |
| Esc | Para el programa (BREAK) |

## El juego

- **La pareja** aparece por encima del pozo y cae despacio. Mientras cae,
  puedes moverla y girarla; cuando toca el fondo u otras gemas, aún tienes un
  tercio de segundo para deslizarla antes de que se quede. Si la pareja está
  tumbada y solo una gema tiene apoyo, la otra sigue cayendo por su columna.
- **Las líneas:** tres o más gemas del mismo color seguidas, en fila, en
  columna o en cualquiera de las dos diagonales, parpadean y desaparecen. Todo
  lo de encima cae, y si así se forman líneas nuevas, también desaparecen: una
  **cadena**. Cada gema vale 10 puntos, por el paso de la cadena (×2, ×3...)
  y por el nivel.
- **El prisma** (un rombo con el arco iris) sale de vez en cuando como una de
  las dos gemas de una pareja. Al posarse borra todas las gemas del color
  sobre el que cae, y a sí mismo (100 puntos más). En el suelo solo se borra
  él.
- **Las gemas** tienen formas distintas además de colores: un rubí redondo,
  una esmeralda hexagonal, un zafiro en rombo, un topacio triangular, una
  amatista cuadrada y una aguamarina en estrella.
- **Los niveles:** cada 30 gemas borradas sube el nivel y las parejas caen
  más deprisa. Los niveles 1 y 2 tienen cuatro colores, desde el 3 cinco y
  desde el 6 seis.
- **El final:** cuando una gema se queda por encima del borde del pozo, las
  gemas se vuelven piedra de abajo arriba.
- **El récord**, en `SD:cascade.txt` (en la tarjeta, aunque el juego se
  cargue de `ROM:`).
- **La demostración:** si no tocas nada en el título, a los 10 segundos
  juega un piloto automático: para cada pareja prueba todas las columnas y
  todos los giros, y se queda con el que hace las líneas más largas sin
  amontonar demasiado.

## Cambiar los gráficos, la música y los efectos

El programa no fabrica nada: todo lo que se ve y se oye está en los tres
ficheros, que carga al empezar con `LOAD "cascade" GRAPHICS`, `MUSIC` y
`SFX`. `LOAD` busca cada uno primero en la carpeta actual y, si no está, en
la del programa (`ROM:`, si se cargó de ahí). Así, para jugar al de la ROM con tus gráficos, pon tu
`cascade.gfx` en la carpeta actual de la tarjeta y escribe
`LOAD "ROM:cascade"`. Si falta un fichero o está estropeado, el programa dice
cuál y se para con el error de `LOAD` (`T Fichero no encontrado` o `R Error
al cargar`).

`cascade.gfx` se abre con `zxv-gfx` (en el PC) y con `gfxedit`
(`LOAD "ROM:gfxedit"`, en la máquina). Respeta los números, porque el
programa los usa:

| Objeto | Qué es |
|---|---|
| Sprites 1 a 7 | Las gemas de la pareja que cae y del título: 1 rubí, 2 esmeralda, 3 zafiro, 4 topacio, 5 amatista, 6 aguamarina, 7 el prisma (el color 8 es transparente) |
| Sprite 20 | El nombre del título, que se dibuja al doble de su tamaño |
| Tiles de 16 × 16, del 0 al 10 | El pozo: 0 vacío, del 1 al 7 las gemas como arriba, 8 el destello y 9 la chispa de una gema que desaparece, 10 la piedra del final |
| Tiles de 8 × 8, 1 y 2 | Los ladrillos de la pared |
| Mapa 1 | La pared: una columna de 24 ladrillos, que se dibuja a los dos lados del pozo |

`cascade.pt3` y `cascade.afb` se abren con `zxv-tracker`, el módulo en la
pestaña de la música y el banco en la de los efectos. Los efectos van
numerados:

| Efecto | Cuándo |
|---|---|
| 0 | La pareja se mueve |
| 1 | La pareja gira |
| 2 | Se posan gemas |
| 3, 4, 5, 6 | Desaparece una línea: el primer paso de una cadena, el segundo, el tercero, el cuarto y los siguientes |
| 7 | El prisma |
| 8 | Nivel nuevo |
| 9 | Fin de la partida |
| 10 | Récord nuevo |
| 11 | *¡Listo!* al empezar una partida |

La música suena en el primer chip AY y los efectos en el segundo, así que no
se estorban.

## Cómo funciona

- **El pozo** es una tabla `g` de 8 × 14: las columnas 2 a 7 y las filas 2 a
  13 son las casillas que se ven, la fila 1 es la de encima del pozo (donde
  aparecen las parejas) y las columnas 1 y 8 y la fila 14 son las paredes y el
  suelo (99), para que ninguna búsqueda tenga que mirar dónde se acaba el
  pozo. `n` cuenta las gemas de cada columna: como las columnas están siempre
  apretadas hacia abajo, una gema que baja por la columna `c` se para en la
  fila `13-n(c)`.
- **En la pantalla**, el pozo es el mapa 0, de 6 × 12 tiles de 16 × 16
  píxeles: `MAP POKE` cambia una casilla cuando una gema se posa, parpadea o
  desaparece, y `MAP 0` lo dibuja entero de una vez en cada imagen. Encima van
  la pareja que cae (dos sprites, píxel a píxel) y las gemas que caen después
  de borrarse una línea.
- **Si una gema cabe** lo dice una función, `DEF FN f(c,y)`: la fila más baja
  que toca la gema a la altura `y` (en píxeles) está por encima de lo alto de
  la columna `c`. Moverse y girar la prueban con las dos gemas; al girar
  contra una pared, la pareja se aparta, y si no hay sitio para tumbarse, las
  dos gemas se cambian.
- **Las líneas** se buscan desde la primera gema de cada tramo (la de antes
  en esa dirección es de otro color), en cuatro direcciones guardadas en las
  tablas `w` y `z`. Las gemas de una línea se marcan en `m`, y no se borran
  hasta haber mirado todo el pozo, así que una gema puede estar en dos líneas
  a la vez.
- **La gravedad** aprieta cada columna hacia abajo y apunta qué gemas cambian
  de fila; esas caen como sprites, cada vez más deprisa, y vuelven al mapa al
  llegar.
- **El idioma:** `PEEK 536872736` y `PEEK 536872737` son las dos letras del
  idioma de la máquina, y `DEF FN t$(a$,b$)` elige el texto con el truco de
  siempre del Spectrum: `a$ AND n` es `a$` si `n` no es 0, y `""` si lo es.
- **La prueba** (`emulator/tests/cascade_test.py`) cambia la línea 5220 para
  empezar con el pozo ya lleno (`p$`) y una primera pareja conocida (`q$`), y
  así comprueba una cadena de tres pasos.

## Velocidad

El juego va a 60 imágenes por segundo (medido en el emulador con `TIME`).
Cuando se posa una pareja, buscar las líneas puede retrasar una imagen, más
o menos una vez cada 250. Para que siga así:

- **Lo que se hace en cada imagen va primero** (líneas 100 a 2430), y lo que
  se hace una vez (la carga, el título, el final de la partida), después.
- **Poco trabajo por imagen:** la pareja que cae solo mueve dos sprites; el
  pozo solo se mira cuando algo se posa, y el piloto automático piensa una vez
  por pareja.
- **Las variables del bucle se crean primero**, nada más empezar.
