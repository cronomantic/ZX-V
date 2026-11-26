[English](../../en/basic/reference.md) · **Español**

# Referencia del BASIC

Esta página describe todas las órdenes, funciones y operadores del BASIC de
ZX-V, además de los mensajes de error, el juego de caracteres y los límites
de la máquina. Es una referencia: para aprender el lenguaje paso a paso,
empieza por el [curso de BASIC](README.md), y para aprender a teclear y
cambiar programas, lee [el editor](editor.md).

## Cómo leer esta referencia

Cada orden y cada función tiene una entrada con las mismas partes, siempre
en este orden:

1. **Sintaxis**, en un recuadro:

   ```
   PRINT [AT fila, columna;] [elemento] [; elemento]...
   ```

2. **Qué hace**, en una o dos frases, con lo que puede sorprender.
3. **Parámetros**: una tabla con cada valor que pones tú, los valores que
   admite (rangos, unidades, valores especiales, el que vale si no lo
   pones) y qué es.
4. **Devuelve** (solo las funciones): el valor que dan y qué significa.
5. **Errores**: una tabla con los informes que puede dar y cuándo. La fila
   `(comunes)` remite a los [errores que puede dar cualquier
   orden](#errores-en-cualquier-orden), que no se repiten en cada entrada.
6. **Ejemplo**: unas líneas que puedes teclear tal cual.
7. **Ver también**: las entradas relacionadas.

Las entradas que explican una idea y no son una orden (Números, Cadenas,
Variables...) no tienen esas partes: son una explicación.

En la sintaxis:

- Las palabras en MAYÚSCULAS son palabras clave: se teclean tal cual (en
  mayúsculas o en minúsculas, da igual).
- Las palabras en minúsculas son lo que pones tú: `n`, `x`, `fila` son
  números o expresiones numéricas; los nombres que acaban en `$` (`a$`,
  `nombre$`) son cadenas de texto; `línea` es un número de línea y `var`, el
  nombre de una variable.
- `[` `]` encierran una parte opcional; `...` quiere decir que lo anterior se
  puede repetir; `|` separa alternativas (`IN | OUT` es «IN u OUT»).
- Los signos como `,`, `;`, `(` y `#` se teclean tal como aparecen.

Cuando algo va mal, la máquina da un **informe de error**, que aquí se
escribe con su código y su mensaje, por ejemplo `B Entero fuera de rango`
(ver [Mensajes de error](#mensajes-de-error)).

Una **orden** (o instrucción) hace algo: `PRINT`, `GO TO`, `BEEP`. Una
**función** devuelve un valor y se usa dentro de una expresión: `SQR 16`,
`LEN a$`, `INKEY$`. Casi todas las órdenes se pueden teclear en una línea de
programa (con número de línea, para ejecutarla después) o como **orden
directa** (sin número, que se ejecuta en cuanto pulsas Enter).

> **Si conoces el Spectrum:** el BASIC de ZX-V ejecuta los programas en
> Sinclair BASIC del 48K, más el `PLAY` del 128K, y añade muchas extensiones
> (bucles estructurados, procedimientos, etiquetas, captura de errores,
> sprites, ficheros...). Las palabras clave se teclean letra a letra, como en
> el 128K, no con una sola tecla.

## Contenido

- [El lenguaje](#el-lenguaje): líneas, números, cadenas, variables,
  matrices, operadores, argumentos enteros y los errores comunes
- [Control del programa](#control-del-programa): `GO TO`, `GO SUB`, `IF`,
  `SELECT CASE`, `FOR`, `WHILE`, `REPEAT`, procedimientos, etiquetas, `RUN`,
  `STOP`, `CONTINUE`, `TRACE`, `PAUSE`, `WAIT`
- [Variables y datos](#variables-y-datos): `LET`, `DIM`, `READ`, `DATA`,
  `CLEAR`, `DEF FN`
- [Matemáticas](#matemáticas): `SIN`, `SQR`, `INT`, `RND`...
- [Funciones de texto](#funciones-de-texto): `LEN`, `CHR$`, `CODE`,
  `STR$`, `VAL`
- [La pantalla y PRINT](#la-pantalla-y-print): `PRINT`, `CLS`, `SCREEN$`,
  `ATTR`, `LPRINT`
- [Colores](#colores): `INK`, `PAPER`, `BORDER`, `BRIGHT`, `FLASH`,
  `INVERSE`, `OVER`
- [Dibujo con PLOT y DRAW](#dibujo-con-plot-y-draw): `PLOT`, `DRAW`,
  `CIRCLE`, `POINT`
- [Modos de vídeo y efectos](#modos-de-vídeo-y-efectos): `MODE`, `PAGE`,
  `SCREEN`, `SCROLL`, `REMAP`, `PALETTE`, `RASTER`
- [Sprites, mapas de tiles y gráficos rápidos](#sprites-mapas-de-tiles-y-gráficos-rápidos):
  `GRAB`, `SPRITE`, `TILE`, `MAP`, `FLOOR`, `ROTATE`, `BOX`, `DISC`, `POLY`,
  `PATTERN`...
- [Sonido y música](#sonido-y-música): `BEEP`, `PLAY`, `SOUND`, `MUSIC`,
  `SFX`, `SAMPLE`
- [Teclado, mando y ratón](#teclado-mando-y-ratón): `INPUT`, `INKEY$`,
  `KEY`, `JOY`, `MOUSE`
- [Ficheros y la tarjeta SD](#ficheros-y-la-tarjeta-sd): `SAVE`, `LOAD`,
  `CAT`, `CD`, `COPY`, `ERASE`...
- [Ficheros de cinta](#ficheros-de-cinta): `.tap` y `.tzx`, los del Spectrum
- [Streams y canales](#streams-y-canales): `OPEN #`, `CLOSE #`, ficheros de
  texto
- [Memoria y sistema](#memoria-y-sistema): `PEEK`, `POKE`, `USR`, `IN`,
  `OUT`, `TIME`, `CLOCK`, `SPEED`, `LANGUAGE`
- [Órdenes de edición](#órdenes-de-edición): `LIST`, `EDIT`, `AUTO`,
  `RENUM`, `DELETE`, `FIND`, `REPLACE`, `NEW`
- [Errores](#errores): informes, `ON ERROR`, `ERR`, `ERL`
- [Mensajes de error](#mensajes-de-error)
- [Juego de caracteres](#juego-de-caracteres)
- [Límites](#límites)
- [Índice de palabras clave](#índice-de-palabras-clave)

## El lenguaje

### Líneas y órdenes

Un programa es una lista de **líneas**. Cada una empieza por un **número de
línea** del 1 al 9999, y se guardan ordenadas por su número, las teclees en
el orden que las teclees. En una línea caben varias órdenes, separadas por dos
puntos (`:`):

```
10 PRINT "Hola": PRINT "otra vez"
20 GO TO 10
```

Los números suelen ir de diez en diez (10, 20, 30...) para que quede sitio
para meter líneas en medio; [`RENUM`](#renum) los vuelve a ordenar. Una línea
tecleada sin número es una orden directa y se ejecuta en el momento.

Al pulsar Enter, el editor comprueba la línea. Si no es BASIC válido, no la
guarda ni la ejecuta: la línea de estado dice qué pasa (casi siempre
`C Sintaxis incorrecta`) y el cursor se queda donde está el fallo. Así, la
mayoría de las erratas se ven al teclear, no al ejecutar.

Cuando algo falla al ejecutar, el informe de error dice dónde: `2 Variable no
encontrada, 20:1` es la orden 1 de la línea 20 (ver [Informes](#informes)).

### Números

Los números se escriben como `42`, `3.25`, `.5` (con punto, no con coma) o
en notación científica: `1E6` es 1 000 000 y `2.5E-3` es 0,0025 (`-7` es el
número 7 con el signo menos delante). Van de unos ±2,9E-39 a ±1,7E38; un
resultado mayor da `6 Número demasiado grande`, y uno más pequeño se queda
en 0. [`BIN`](#bin) escribe un número en binario.

`PRINT` enseña hasta 8 cifras significativas, redondeadas y sin ceros al
final. Pasa a la notación científica cuando hay 9 cifras o más antes del
punto, o más de 4 ceros detrás de él; los números entre 0 y 1 con algún cero
tras el punto se escriben sin el 0 de delante:

```
PRINT 1/3, 2^31, 1E10, 0.000001, 0.05
```

enseña `0.33333333`, `2.1474836E+9`, `1E+10`, `1E-6` y `.05`.

Por dentro, los números tienen más precisión que en el Spectrum, pero las
comparaciones (`=`, `<`, `>`...) se hacen con la misma que en el Spectrum,
así que `0.1*3=0.3` es cierto.

### Cadenas

Una cadena es un trozo de texto entre comillas dobles: `"Hola"`. Para meter
unas comillas dentro de una cadena, se escriben dos veces: `PRINT "di
""hola"""` enseña `di "hola"`. La cadena vacía es `""`. Una cadena puede ser
tan larga como quepa en la memoria.

`+` une cadenas: `"ZX" + "-V"` es `"ZX-V"`.

Un **trozo** es una parte de una cadena. Las posiciones de los caracteres
empiezan en 1:

| Expresión | Resultado | Qué es |
|---|---|---|
| `"hola"(2 TO 3)` | `"ol"` | los caracteres 2 a 3 |
| `"hola"(3)` | `"l"` | el carácter 3 |
| `"hola"(TO 2)` | `"ho"` | del principio al 2 |
| `"hola"(3 TO)` | `"la"` | del 3 al final |

Un trozo que empieza después de donde acaba es la cadena vacía (`"hola"(3
TO 2)` es `""`, sin error); si no, una posición fuera de la cadena (también
la 0) da `3 Subíndice incorrecto`, y una mayor que 65535 o negativa, `B
Entero fuera de rango`. Los trozos valen con variables de cadena, con cadenas
escritas en el programa y con cualquier expresión entre paréntesis:
`(a$+b$)(2 TO)`. También se puede asignar a un trozo: `LET a$(1)="J"`
cambia el primer carácter (sin cambiar la longitud de `a$`).

Las cadenas se comparan carácter a carácter según sus códigos (ver [Juego
de caracteres](#juego-de-caracteres)), así que `"B"<"a"` es cierto, y
`"ab"<"abc"` también.

### Variables

Una variable guarda un valor con un nombre.

- Las **variables numéricas** tienen nombres de letras y cifras que empiezan
  por una letra: `x`, `puntos`, `nivel2`. Los nombres pueden ser largos (hasta
  63 caracteres), no se distinguen mayúsculas y minúsculas (`Puntos` y
  `PUNTOS` son la misma) y se ignoran los espacios de dentro (`mis puntos` es
  `mispuntos`). No admiten letras con tilde ni la ñ (si las tecleas, se
  escriben sin la tilde: `número` queda `numero`).
- Las **variables de cadena** son una sola letra seguida de `$`: `a$`, `n$`.
  Hay 26.
- La variable de control de [`FOR`](#for-next) es una sola letra, y es una
  variable numérica normal: `FOR i` usa (y cambia) la `i` que hubiera.
- Un nombre no puede ser una palabra clave del Spectrum ni una función de
  ZX-V: `LET print=1` y `LET time=1` son errores. Las órdenes propias de
  ZX-V (`MODE`, `WHILE`, `SPRITE`...) solo se reconocen al principio de una
  orden, así que sí valen como nombres (`LET mode=1`), para no romper
  programas antiguos. Una palabra clave dentro de un nombre más largo no
  molesta (`total`, `ink2`).

Una variable se crea la primera vez que recibe un valor (con `LET`, `INPUT`,
`READ`, `FOR`, o como parámetro de un procedimiento). Leer una variable que
no existe da `2 Variable no encontrada`. [`RUN`](#run) y [`CLEAR`](#clear)
borran todas las variables.

```
10 LET mis puntos=10
20 LET a$="Ana"
30 PRINT a$; " tiene "; MisPuntos; " puntos"
```

### Matrices

Una matriz es una lista numerada de valores (o una tabla, si tiene varias
dimensiones). Su nombre es una sola letra y se crea con [`DIM`](#dim):

```
10 DIM t(5): DIM g(3,3)
20 LET t(1)=100: LET g(2,3)=7
30 PRINT t(1); " "; g(2,3); " "; t(5)
```

Los subíndices empiezan en 1. Las matrices numéricas empiezan llenas de
ceros. Una matriz numérica y una variable numérica pueden compartir letra
(`a` y `a(1)` son distintas). Una matriz de cadenas `a$` sustituye a la
variable de cadena `a$`. Un subíndice fuera de su dimensión, o un número de
subíndices que no es el de la matriz, da `3 Subíndice incorrecto`; usar una
matriz antes de su `DIM`, `2 Variable no encontrada`.

Las **matrices de cadenas** son matrices de caracteres: la última dimensión
es la longitud fija de cada cadena. `DIM n$(3,10)` crea 3 cadenas de 10
caracteres, llenas de espacios. `n$(2)` es la segunda cadena y `n$(2,1)`, su
primer carácter. Si se asigna un texto más corto, el resto se rellena con
espacios; si es más largo, se corta:

```
10 DIM n$(2,4)
20 LET n$(1)="hola mundo"
30 PRINT n$(1); "|"; n$(2); "|"
```

enseña `hola|    |`. `n$` sin subíndices es la matriz entera como una sola
cadena (aquí, de 8 caracteres), y `LET n$="..."` la rellena de principio a
fin de la misma manera, sin cambiar su tamaño.

### Expresiones y operadores

Una expresión combina valores con operadores. Cuando hay varios, primero se
calculan los de más prioridad:

| Prioridad | Operadores | Qué hacen |
|---|---|---|
| la más alta | funciones | `SIN x`, `LEN a$`... toman solo el valor que las sigue |
| 10 | `^` | potencia |
| 9 | `-` (delante) | cambio de signo |
| 8 | `*` `/` | multiplicación, división |
| 6 | `+` `-` | suma (y unión de cadenas), resta |
| 5 | `=` `<` `>` `<=` `>=` `<>` | comparaciones |
| 4 | `NOT` | no lógico |
| 3 | `AND` | y lógico |
| la más baja | `OR` | o lógico |

Los operadores de la misma prioridad se calculan de izquierda a derecha, así
que `2^3^2` es `(2^3)^2` = 64. El cambio de signo va después de la potencia:
`-2^2` es -4. Los paréntesis cambian el orden: `(1+2)*3` es 9. Un `+`
delante de un número se admite y no hace nada.

Las funciones toman solo el valor que las sigue: `SQR 16+9` es `(SQR 16)+9`
= 13, y `SQR (16+9)` es 5. Si dudas, pon paréntesis.

Los dos lados de un operador tienen que ser del mismo tipo (dos números o
dos cadenas); si no, `C Sintaxis incorrecta`. Con cadenas solo valen `+`,
las comparaciones y `AND` (con un número a la derecha).

Dividir entre cero da `6 Número demasiado grande`, y elevar 0 a un número
negativo, también. Elevar un número negativo a una potencia da `A Argumento
no válido`, aunque la potencia sea entera (`(-2)^2`); `0^0` es 1.

### Verdadero y falso

Las comparaciones dan 1 si son ciertas y 0 si son falsas: `PRINT 3>2` enseña
1. `IF`, `WHILE` y `UNTIL` admiten cualquier número: el 0 es falso y todo lo
demás, cierto.

| Expresión | Valor |
|---|---|
| `NOT x` | 1 si `x` es 0; si no, 0 |
| `x AND y` | `x` si `y` no es 0; si no, 0 |
| `x OR y` | 1 si `y` no es 0; si no, `x` |
| `a$ AND y` | `a$` si `y` no es 0; si no, `""` |

Así, `IF a>0 AND a<10 THEN ...` funciona como esperas, y un truco como
`PRINT ("vale" AND ok); ("no vale" AND NOT ok)` escribe una cosa o la otra.

### Argumentos enteros

Muchas órdenes necesitan números enteros (un número de línea, un color, una
coordenada). Si les das un número con decimales, se redondea al entero más
cercano, y los medios hacia arriba: 2,5 pasa a 3 y -2,5, a -2. Un valor entre
-0,5 y 0 cuenta como 0.

Casi todas leen primero el número con un rango amplio, el de su tipo: de 0 a
255 los colores, las filas y las columnas; de 0 a 65535 los números de línea
y los tiempos. Un valor negativo o mayor que ese máximo da siempre
`B Entero fuera de rango`. Después, cada orden comprueba su propio rango, y
ahí el informe puede ser otro. Por ejemplo, con `INK` (que admite del 0 al
9):

| Orden | Informe | Por qué |
|---|---|---|
| `INK 10` | `K Color no válido` | Entre 0 y 255, pero no es un color. |
| `INK 300` | `B Entero fuera de rango` | Mayor que 255. |
| `INK -1` | `B Entero fuera de rango` | Negativo. |

Cada entrada dice los dos rangos y qué informe da cada uno.

### Errores en cualquier orden

Estos informes los puede dar casi cualquier orden o función, porque salen al
calcular sus valores o mientras corre el programa. Las tablas de errores de
cada entrada los resumen en la fila `(comunes)`.

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | La línea no es BASIC válido, o mezcla números y cadenas donde no se puede (`LET a="x"`, `"a"+1`). Casi siempre lo dice el editor al teclear la línea, que no la guarda; en las entradas se dice cuándo sale al ejecutar. |
| `2 Variable no encontrada` | Se usa una variable que no tiene valor, o una matriz antes de su `DIM`. |
| `3 Subíndice incorrecto` | Un subíndice o un trozo fuera de la matriz o de la cadena. |
| `4 Sin memoria` | No queda memoria para un resultado, o la expresión tiene demasiados paréntesis o `FN` unos dentro de otros. |
| `6 Número demasiado grande` | Un resultado mayor que 1,7E38, una división entre cero o 0 elevado a un número negativo. |
| `A Argumento no válido` | Una función con un valor que no admite: `SQR -1`, `LN 0`, `ASN 2`, la potencia de un número negativo. |
| `B Entero fuera de rango` | Un argumento entero negativo o mayor que el máximo de su tipo (ver [Argumentos enteros](#argumentos-enteros)). |
| `L BREAK en el programa` | Has pulsado BREAK (Esc) mientras corría el programa, o durante una espera (`PAUSE`, `WAIT`, `BEEP`...). `CONTINUE` sigue por la orden siguiente. |

## Control del programa

### GO TO

```
GO TO línea
GO TO @etiqueta
```

Salta a una línea y sigue desde ahí. Si esa línea no existe, va a la
siguiente que haya; si no hay ninguna, el programa termina con `0 OK`. Con
una [etiqueta](#etiqueta), salta a esa orden exacta, aunque no sea la
primera de su línea. También se puede teclear `GOTO`, junto.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `línea` | 0–61439; cualquier expresión (`GO TO n*100`) | El número de la línea. Las líneas llegan a 9999: un número mayor termina el programa. |
| `etiqueta` | un nombre | Una etiqueta `@nombre` del programa. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `línea` es negativa o mayor que 61439. |
| `X Etiqueta no encontrada` | No hay ninguna `@etiqueta` con ese nombre. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PRINT "sin parar"
20 GO TO 10
```

Pulsa BREAK (Esc) para pararlo.

**Ver también:** [GO SUB](#go-sub-return), [@etiqueta](#etiqueta),
[RUN](#run), [ON ERROR](#on-error).

### GO SUB, RETURN

```
GO SUB línea
GO SUB @etiqueta
RETURN
```

`GO SUB` salta a una subrutina, como `GO TO`, pero recuerda de dónde venía;
`RETURN` vuelve a la orden que sigue al `GO SUB`. Una subrutina puede llamar a
otras, hasta 512 `GO SUB` pendientes. También se puede teclear `GOSUB`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `línea` | 0–61439 | Como en [`GO TO`](#go-to). |
| `etiqueta` | un nombre | Una etiqueta `@nombre` del programa. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `línea` es negativa o mayor que 61439. |
| `X Etiqueta no encontrada` | No hay ninguna `@etiqueta` con ese nombre. |
| `4 Sin memoria` | Ya hay 512 `GO SUB` sin su `RETURN`. |
| `7 RETURN sin GO SUB` | `RETURN` sin un `GO SUB` pendiente al que volver. |
| `N Sentencia perdida` | La línea a la que vuelve `RETURN` ya no tiene esa orden (se cambió el programa con el `GO SUB` pendiente). |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 GO SUB 100: GO SUB 100
20 STOP
100 PRINT "en la subrutina"
110 RETURN
```

**Ver también:** [DEF PROC](#def-proc-proc-end-proc), que suele quedar más
claro; [GO TO](#go-to), [@etiqueta](#etiqueta).

### IF, THEN, ELSE

```
IF condición THEN órdenes
IF condición THEN órdenes ELSE órdenes

IF condición THEN
  líneas
ELSE IF condición THEN
  líneas
ELSE
  líneas
END IF
```

Si la condición es cierta (no es 0), se ejecutan las órdenes que siguen a
`THEN`. Si es falsa, se saltan: el programa sigue por las órdenes que hay tras
`ELSE`, si hay uno en la misma línea, o por la línea siguiente.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `condición` | un número | Cierta si no es 0 (ver [Verdadero y falso](#verdadero-y-falso)). Una cadena es un error. |
| `órdenes` | órdenes separadas por `:` | Lo que se ejecuta. Tras `THEN` tiene que ir una orden: un número solo (`IF a THEN 100`) es un error; se escribe `IF a THEN GO TO 100`. |

Todo lo que va de `THEN` a `ELSE` es la parte de cierto, y todo lo que va de
`ELSE` al final de la línea, la de falso. Con varios `IF` en una línea, cada
`ELSE` va con el `IF` más cercano.

**El IF de bloque.** Si `THEN` es lo último de la línea, el `IF` abre un
bloque de varias líneas, que acaba en `END IF`. Dentro puede haber partes
`ELSE IF condición THEN` (también al final de su línea), que se miran por
orden si no se cumplió nada antes, y una parte `ELSE` para cuando no se
cumple ninguna. Solo se ejecuta una parte; después, el programa sigue tras
`END IF`.

- `ELSE`, `ELSE IF` y `END IF` empiezan su línea. Detrás de `ELSE` pueden ir
  órdenes en la misma línea (`ELSE PRINT "Adulto"`), y detrás de `END IF`,
  otras separadas por `:`. Un `ELSE` que no empieza la línea es el del IF de
  una línea.
- Los bloques se pueden meter unos dentro de otros, sin límite, y llevar
  dentro IF de una línea, bucles o `GO SUB`. Se puede salir de un bloque con
  `GO TO`.
- Solo funciona en un programa: como orden directa, `IF a THEN` no hace
  nada.

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | Al teclear la línea: la condición es una cadena, falta `THEN`, tras `THEN` va un número, o un `ELSE IF ... THEN` no acaba la línea. |
| `Y Error de estructura` | A un bloque le falta el `END IF`. Sale cuando hay que buscarlo: al saltar una parte que no se cumple, o al acabar la que se cumplía. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 LET n=-5
20 IF n<0 THEN PRINT "negativo" ELSE PRINT "cero o positivo"
30 IF n<0 THEN PRINT "a": PRINT "b" ELSE PRINT "c": PRINT "d"
```

La línea 30 escribe `a` y `b`. Con un bloque:

```
10 INPUT "Edad? ";e
20 IF e<13 THEN
30 PRINT "Nino"
40 ELSE IF e<18 THEN
50 PRINT "Adolescente"
60 ELSE
70 PRINT "Adulto"
80 IF e>=65 THEN PRINT "Jubilado"
90 END IF
```

**Ver también:** [SELECT CASE](#select-case), [WHILE](#while-wend),
[Expresiones](#expresiones-y-operadores),
[Verdadero y falso](#verdadero-y-falso).

### SELECT CASE

```
SELECT CASE valor
CASE valores
  líneas
CASE ELSE
  líneas
END SELECT
```

Elige una parte según `valor`: se ejecuta la del primer `CASE` que le encaja
(o la de `CASE ELSE` si no encaja ninguno), y el programa sigue tras
`END SELECT`. Si no encaja ninguno y no hay `CASE ELSE`, no se ejecuta nada.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `valor` | un número o una cadena | Lo que se compara con cada `CASE`. |
| `valores` | una lista separada por comas (tabla) | Encaja si encaja cualquiera de la lista. Todos tienen que ser del tipo de `valor`. |

| Valor | Encaja si |
|---|---|
| `5`, `"si"`, `a+1` | `valor` es igual |
| `1 TO 9`, `"a" TO "m"` | `valor` está entre los dos (ambos incluidos) |
| `< 0`, `>= 100`, `<> 7`, `= 3` | `valor` cumple la comparación |

- Los `CASE` se miran en orden, y `CASE ELSE` se toma en cuanto se llega a
  él: ponlo el último, porque los `CASE` que vayan detrás no se miran nunca.
- Los números se comparan con toda su precisión, no con la de `=`: `SELECT
  CASE 0.1*3` no encaja con `CASE 0.3`. Para valores con decimales, usa
  rangos (`CASE 0.29 TO 0.31`).
- Los `CASE` pueden ir en líneas propias o detrás de `:`, y las órdenes de
  cada parte, en la misma línea o en las siguientes; todo puede ir incluso en
  una sola línea, también como orden directa. Los `SELECT CASE` se pueden
  meter unos dentro de otros.

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | Un valor de un `CASE` no es del tipo de `valor` (sale al compararlo), o detrás de los valores de un `CASE` hay algo que no es `:` ni el final de la línea. |
| `Y Error de estructura` | Falta el `END SELECT`. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 INPUT "Una tecla? ";t$
20 SELECT CASE t$
30 CASE "q", "Q": PRINT "Arriba"
40 CASE "a", "A": PRINT "Abajo"
50 CASE "0" TO "9"
60 PRINT "Un numero"
70 CASE ELSE: PRINT "Otra"
80 END SELECT
```

En una sola línea: `SELECT CASE n: CASE 1: PRINT "uno": CASE ELSE: PRINT
"otro": END SELECT`.

**Ver también:** [IF](#if-then-else).

### FOR, NEXT

```
FOR var = primero TO último [STEP paso]
NEXT var
```

Repite las órdenes que hay entre `FOR` y `NEXT`, con la variable yendo de
`primero` a `último` y sumando `paso` cada vez. Con un paso negativo cuenta
hacia atrás; con `STEP 0`, no acaba nunca. Si el bucle no tiene que
ejecutarse ninguna vez (`FOR i=1 TO 0`), el programa salta tras el primer
`NEXT` de esa variable que haya más adelante.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `var` | una letra | La variable de control, numérica. Si ya había una variable con ese nombre, la sustituye. |
| `primero`, `último` | números | El primer valor y el último. |
| `paso` | un número; 1 si no se pone | Lo que se suma en cada vuelta. |

Al acabar el bucle, la variable se queda con el primer valor que se pasa del
final (en el ejemplo, 0). `NEXT` compara con la precisión de `=`, así que
`FOR x=0 TO 1 STEP 0.1` llega al 1.

**Errores**

| Informe | Cuándo |
|---|---|
| `I FOR sin NEXT` | El bucle no se ejecuta ninguna vez y no hay un `NEXT` de esa variable más adelante. |
| `1 NEXT sin FOR` | La variable de `NEXT` existe, pero no es la de un `FOR`. |
| `2 Variable no encontrada` | No hay ninguna variable con el nombre de `NEXT`. |
| `6 Número demasiado grande` | Al sumar el paso, la variable pasa de 1,7E38. |
| `C Sintaxis incorrecta` | Al teclear la línea: `var` no es una sola letra, o es de cadena. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 FOR i=1 TO 10 STEP 3: PRINT i;" ";: NEXT i
20 PRINT
30 FOR i=3 TO 1 STEP -1: PRINT i: NEXT i
```

**Ver también:** [WHILE](#while-wend), [REPEAT](#repeat-until).

### WHILE, WEND

```
WHILE condición
WEND
```

Repite las órdenes que hay entre `WHILE` y `WEND` mientras la condición sea
cierta. La condición se mira al principio, así que si es falsa la primera
vez, el bucle se salta entero. Los bucles se pueden anidar: `WHILE`,
`REPEAT` y los procedimientos, hasta 64 unos dentro de otros.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `condición` | un número | Cierta si no es 0. |

**Errores**

| Informe | Cuándo |
|---|---|
| `Y Error de estructura` | `WEND` sin su `WHILE`, o la condición es falsa y no hay un `WEND` más adelante. |
| `4 Sin memoria` | Más de 64 bucles y procedimientos unos dentro de otros. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 LET n=1
20 WHILE n<100: LET n=n*2: WEND
30 PRINT n
```

enseña 128.

**Ver también:** [REPEAT](#repeat-until), [FOR](#for-next).

### REPEAT, UNTIL

```
REPEAT
UNTIL condición
```

Repite las órdenes que hay entre `REPEAT` y `UNTIL` hasta que la condición
sea cierta. La condición se mira al final, así que el bucle se ejecuta al
menos una vez.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `condición` | un número | El bucle acaba cuando no es 0. |

**Errores**

| Informe | Cuándo |
|---|---|
| `Y Error de estructura` | `UNTIL` sin su `REPEAT`. |
| `4 Sin memoria` | Más de 64 bucles y procedimientos unos dentro de otros. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 REPEAT
20 LET d=INT (RND*6)+1: PRINT d
30 UNTIL d=6
```

**Ver también:** [WHILE](#while-wend), [FOR](#for-next).

### DEF PROC, PROC, END PROC

```
DEF PROC nombre [(parámetro [, parámetro]...)]
END PROC
PROC nombre [(valor [, valor]...)]
```

Un **procedimiento** es un trozo de programa con nombre. `DEF PROC` lo empieza
y `END PROC` lo acaba; `PROC` lo ejecuta y luego vuelve a la orden siguiente.
Los parámetros reciben los valores que se dan en `PROC` y son locales: al
llegar a `END PROC` recuperan el valor que tenían antes (o dejan de existir,
si no existían). Un procedimiento puede llamarse a sí mismo.

Cuando el programa llega a un `DEF PROC` siguiendo su curso normal, se salta
el procedimiento entero, así que los procedimientos pueden ir en cualquier
sitio. Un procedimiento no devuelve un valor: para dar un resultado, guárdalo
en una variable.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre` | como el de una variable numérica | El nombre del procedimiento: letras y cifras, sin distinguir mayúsculas ni espacios. |
| `parámetro` | hasta 16 | Un nombre de variable numérica, o una letra con `$` para una cadena. |
| `valor` | hasta 16, uno por parámetro | Un número o una cadena, del tipo de su parámetro. |

`END PROC` también cierra los `WHILE` y `REPEAT` que hayan quedado abiertos
dentro del procedimiento.

**Errores**

| Informe | Cuándo |
|---|---|
| `Z PROC no encontrado` | No hay un `DEF PROC` con ese nombre. |
| `Q Error en los parámetros` | El número o el tipo de los valores de `PROC` no coinciden con los parámetros, o hay más de 16. |
| `Y Error de estructura` | `END PROC` fuera de un procedimiento, o el programa llega a un `DEF PROC` que no tiene su `END PROC` antes del siguiente `DEF PROC`. |
| `4 Sin memoria` | Más de 64 bucles y procedimientos unos dentro de otros, o no caben más valores guardados de parámetros y variables locales (8 KB entre todos). |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PROC saluda("Ana", 2)
20 PROC saluda("Luis", 1)
30 STOP
100 DEF PROC saluda(n$, veces)
110 FOR i=1 TO veces: PRINT "Hola, "; n$: NEXT i
120 END PROC
```

**Ver también:** [LOCAL](#local), [GO SUB](#go-sub-return).

### LOCAL

```
LOCAL var [, var]...
```

Dentro de un procedimiento, hace locales esas variables: empiezan valiendo 0
(o `""`) y recuperan su valor anterior en `END PROC`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `var` | hasta 16 nombres | Variables numéricas (cualquier nombre) o de cadena (`a$`). Las matrices no. |

**Errores**

| Informe | Cuándo |
|---|---|
| `Y Error de estructura` | `LOCAL` fuera de un procedimiento. |
| `Q Error en los parámetros` | Más de 16 nombres en un `LOCAL`. |
| `4 Sin memoria` | No caben más valores guardados de parámetros y variables locales (8 KB entre todos). |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 LET i=99: PROC cuenta: PRINT "i sigue valiendo "; i
20 STOP
100 DEF PROC cuenta
110 LOCAL i
120 FOR i=1 TO 3: PRINT i: NEXT i
130 END PROC
```

**Ver también:** [DEF PROC](#def-proc-proc-end-proc).

### @etiqueta

```
@nombre
```

Una orden formada por `@` y un nombre marca un sitio del programa, y no hace
nada más. `GO TO`, `GO SUB`, `RUN`, `RESTORE` y `ON ERROR GO TO` pueden
saltar a ella por su nombre en vez de por su número de línea. `GO TO`,
`GO SUB`, `RUN` y `ON ERROR GO TO` van a la orden exacta (no hace falta que
sea la primera de su línea); `RESTORE`, al principio de su línea. Las
etiquetas no cambian al renumerar. Si dos etiquetas tienen el mismo nombre,
vale la primera.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre` | como el de una variable numérica | Letras y cifras, sin distinguir mayúsculas ni espacios. |

**Errores**

| Informe | Cuándo |
|---|---|
| `X Etiqueta no encontrada` | (En la orden que salta.) No hay ninguna etiqueta con ese nombre. |
| `C Sintaxis incorrecta` | Al teclear la línea: tras `@` no hay un nombre válido. |

**Ejemplo**

```
10 GO SUB @hola: GO SUB @hola
20 STOP
30 @hola: PRINT "hola": RETURN
```

**Ver también:** [GO TO](#go-to), [RESTORE](#read-data-restore),
[RENUM](#renum).

### RUN

```
RUN [línea | @etiqueta]
```

Ejecuta el programa desde el principio, o desde esa línea o etiqueta. Antes de
empezar para la música y los efectos de sonido y lo limpia todo como
[`CLEAR`](#clear): borra las variables y la pantalla, cierra los ficheros
abiertos, quita el `ON ERROR`, `CLIP` y `PATTERN`, y hace que `READ` empiece
por el primer `DATA`. No cambia el modo de vídeo, los colores, `TRACE` ni
los sprites guardados. Para empezar sin perder las variables, usa `GO TO`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `línea` | 0–65535; 0 si no se pone | Dónde empieza. Si no existe, en la siguiente que haya; si no hay ninguna, no se ejecuta nada (`0 OK`). |
| `etiqueta` | un nombre | Una etiqueta `@nombre` del programa. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `línea` es negativa o mayor que 65535. |
| `X Etiqueta no encontrada` | No hay ninguna `@etiqueta` con ese nombre. |
| (comunes) | Y los del programa que ejecuta. Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
RUN 100
```

**Ver también:** [CONTINUE](#continue), [STOP](#stop), [CLEAR](#clear),
[GO TO](#go-to).

### STOP

```
STOP
```

Para el programa con el informe `9 Sentencia STOP`. [`CONTINUE`](#continue)
sigue por la orden siguiente. `ON ERROR` no atrapa `STOP`.

**Parámetros:** ninguno.

**Errores**

| Informe | Cuándo |
|---|---|
| `9 Sentencia STOP` | Siempre: es la manera en que para. |

**Ejemplo**

```
10 PRINT "uno": STOP: PRINT "dos"
```

Tras `9 Sentencia STOP, 10:2`, `CONTINUE` escribe `dos`.

**Ver también:** [CONTINUE](#continue), [ON ERROR](#on-error).

### CONTINUE

```
CONTINUE
```

Sigue después de una parada: tras `STOP` o BREAK (`L`), por la orden
siguiente; tras un error (también `D BREAK - CONT repite`), repite la orden
que falló, lo que sirve cuando ya has arreglado la causa (por ejemplo, dando
valor a una variable que faltaba).

Solo sirve para los programas: tras una parada en una orden directa, esa
orden ya no está (la línea de edición es ahora el propio `CONTINUE`), así que
da `N Sentencia perdida`. (En el Spectrum, el `CONTINUE` se repetía sin fin.)

**Parámetros:** ninguno.

**Errores**

| Informe | Cuándo |
|---|---|
| `N Sentencia perdida` | La parada fue en una orden directa, o la línea en la que tiene que seguir ya no tiene esa orden (se ha cambiado). |
| (comunes) | Y los de la orden que repite. Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PRINT x
```

`RUN` da `2 Variable no encontrada, 10:1`; después, `LET x=5` y `CONTINUE`
escriben 5.

**Ver también:** [STOP](#stop), [ON ERROR](#on-error), [Informes](#informes).

### TRACE

```
TRACE ON
TRACE #stream
TRACE OFF
```

Para seguir por dónde va un programa: cada vez que empieza a ejecutarse una
línea (también al volver a ella con un `GO TO`, un `NEXT` o un `RETURN`), se
escribe su número entre corchetes, como `[10][20][30]`. `TRACE OFF` lo
apaga; también se puede poner dentro del programa, para seguir solo una
parte. `NEW` lo apaga, `RUN` no. Si el stream se cierra, se apaga solo.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ON` | — | En la pantalla del programa (el stream 2), mezclado con lo que escribe. |
| `stream` | 0–15, abierto | Por ese stream: `TRACE #3` va a la impresora (el fichero `printer.txt`), y un stream abierto con `OPEN #` a un fichero, a ese fichero, para no estropear la pantalla. |

**Errores**

| Informe | Cuándo |
|---|---|
| `O Canal no válido` | El stream no está abierto. |
| `B Entero fuera de rango` | `stream` es negativo o mayor que 15. |
| `C Sintaxis incorrecta` | Al teclear la línea: detrás de `TRACE` no hay `ON`, `OFF` ni `#`. |

**Ejemplo**

```
TRACE ON: RUN
```

**Ver también:** el [depurador del BASIC](../emulator.md#depuración), para
parar un programa en una línea y mirar sus variables desde VS Code;
[OPEN #](#open-close).

### REM

```
REM cualquier texto
```

Un comentario: todo lo que va detrás de `REM` hasta el final de la línea se
ignora, también los `:` y las órdenes que haya.

**Parámetros:** ninguno. **Errores:** ninguno.

**Ejemplo**

```
10 REM Juego del espacio, por Ana
```

**Ver también:** [Líneas y órdenes](#líneas-y-órdenes).

### PAUSE

```
PAUSE n
```

Espera `n` cincuentavos de segundo (`PAUSE 50` es un segundo, como en el
Spectrum), o hasta que se pulse una tecla, lo que llegue antes. Solo cuentan
las teclas que se pulsan después de empezar el `PAUSE`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–65535 | Cincuentavos de segundo. 0 espera una tecla sin límite. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `n` es negativo o mayor que 65535. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PRINT "Pulsa una tecla": PAUSE 0: PRINT "Gracias"
```

**Ver también:** [WAIT](#wait), [INKEY$](#inkey).

### WAIT

```
WAIT [n]
```

Espera al frame siguiente, y hasta que hayan pasado `n` frames desde el
`WAIT` anterior. Hay 60 frames por segundo. Así el bucle de un juego va a un
ritmo fijo aunque unas vueltas tarden más que otras: `WAIT 2` da 30 vueltas
por segundo. No mira el teclado.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–65535; 1 si no se pone | Frames desde el `WAIT` anterior. Con 0 (o si ya han pasado), solo espera al frame siguiente. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `n` es negativo o mayor que 65535. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 FOR x=0 TO 200: BOX 0,x,80,8,8: WAIT: NEXT x
```

**Ver también:** [PAUSE](#pause), [TIME](#time).

## Variables y datos

### LET

```
LET var = valor
```

Da un valor a una variable, y la crea si hace falta. El `LET` es obligatorio
(`a=1` sin más es un error). `var` también puede ser un elemento de una
matriz (`t(3)`) o un trozo de una cadena (`a$(2 TO 4)`); al asignar a un
trozo, el texto se corta o se rellena con espacios para que la cadena no
cambie de longitud.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `var` | una variable, un elemento de matriz o un trozo de cadena | Dónde se guarda. Ver [Variables](#variables). |
| `valor` | un número o una cadena | Del tipo de `var`: un número para las numéricas y una cadena para las de `$`. |

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | Al teclear la línea: falta `LET` o `=`, el nombre es una palabra clave o tiene más de 63 caracteres, o el valor no es del tipo de la variable. |
| `2 Variable no encontrada` | Se asigna a un elemento de una matriz sin `DIM`, o a un trozo de una cadena que no existe. |
| `3 Subíndice incorrecto` | El subíndice o el trozo se salen de la matriz o de la cadena. |
| `4 Sin memoria` | No cabe la variable. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 LET a=5: LET a=a+1: LET b$="ZX"
20 LET b$(2)="-": PRINT a; b$
```

enseña `6Z-`.

**Ver también:** [Variables](#variables), [DIM](#dim), [Cadenas](#cadenas).

### DIM

```
DIM letra(tamaño [, tamaño]...)
DIM letra$(tamaño [, tamaño]...)
```

Crea una matriz (ver [Matrices](#matrices)), y sustituye a la que tuviera el
mismo nombre (`DIM a$` sustituye también a la variable de cadena `a$`). Las
matrices numéricas empiezan llenas de ceros y las de cadenas, de espacios.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `letra` | una letra | El nombre de la matriz. |
| `tamaño` | 1–65535; hasta 16 dimensiones | Cuántos elementos tiene cada dimensión. En las de cadenas, el último es la longitud de cada cadena. En total, hasta 16 777 215 elementos, si caben en la memoria. |

**Errores**

| Informe | Cuándo |
|---|---|
| `3 Subíndice incorrecto` | Un tamaño es 0, o hay más de 16 dimensiones (esto, al teclear la línea). |
| `B Entero fuera de rango` | Un tamaño es negativo o mayor que 65535. |
| `4 Sin memoria` | La matriz tiene más de 16 777 215 elementos o no cabe en la memoria. |
| `C Sintaxis incorrecta` | Al teclear la línea: el nombre tiene más de una letra. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 DIM s(10): LET s(3)=42: PRINT s(3), s(4)
```

**Ver también:** [Matrices](#matrices), [LET](#let), [CLEAR](#clear).

### READ, DATA, RESTORE

```
DATA valor [, valor]...
READ var [, var]...
RESTORE [línea | @etiqueta]
```

Las líneas `DATA` guardan una lista de valores dentro del programa; `READ` los
va cogiendo uno a uno, en orden, y los guarda en variables. `RESTORE` hace
que el siguiente `READ` vuelva a empezar por el primer `DATA` del programa (o
por el primero que haya desde esa línea, o desde el principio de la línea de
esa etiqueta). `DATA` no hace nada cuando el programa pasa por él.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `valor` | cualquier expresión | Se calcula al leerlo, no antes: puede usar variables (`DATA x*2`). Tiene que ser del tipo de la variable que lo lee. |
| `var` | variables, elementos de matriz o trozos de cadena | Dónde se guarda cada valor, como en `LET`. |
| `línea` | 0–65535; 0 si no se pone | Desde dónde se buscará el siguiente `DATA`. |
| `etiqueta` | un nombre | Una etiqueta `@nombre`; se busca desde el principio de su línea. |

**Errores**

| Informe | Cuándo |
|---|---|
| `E No quedan DATA` | `READ` después del último valor. |
| `C Sintaxis incorrecta` | El valor leído no es del tipo de la variable, o detrás de él en el `DATA` hay algo que no es `,`, `:` ni el final de la línea. |
| `B Entero fuera de rango` | La `línea` de `RESTORE` es negativa o mayor que 65535. |
| `X Etiqueta no encontrada` | No hay ninguna `@etiqueta` con ese nombre. |
| (comunes) | Y los del cálculo del valor (los de un `DATA` salen en la línea del `READ`). Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 FOR i=1 TO 3
20 READ n$, edad: PRINT n$; " tiene "; edad
30 NEXT i
40 DATA "Ana", 12, "Luis", 9, "Eva", 10+1
```

**Ver también:** [LET](#let), [@etiqueta](#etiqueta).

### CLEAR

```
CLEAR [dirección]
```

Borra todas las variables y la pantalla, cierra los ficheros abiertos con
`OPEN #`, olvida las vueltas pendientes de `GO SUB`, de los bucles y de los
procedimientos, quita `ON ERROR`, `CLIP` y la trama de `PATTERN`, lleva el
punto de partida de `DRAW` a (0, 0) y hace que `READ` empiece por el primer
`DATA`. Los sprites y demás gráficos guardados, los colores y el modo de
vídeo se conservan, y la música sigue sonando. [`RUN`](#run) hace primero un
`CLEAR`.

Con una dirección, además mueve un límite de la memoria (solo para programas
avanzados).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `dirección` | 0; 23821–65535; o una dirección real de 65536 en adelante | Con 0 (o sin ella), nada más. Por debajo de 65536 pone el «RAMTOP» de la [memoria del Spectrum virtual](#la-memoria-del-spectrum-virtual) (al encender, 65367). De 65536 en adelante es una dirección real de la memoria del BASIC (se redondea a múltiplo de 4), y el almacén de gráficos se coloca justo debajo, de modo que la memoria de encima queda libre para lo que quieras. |

**Errores**

| Informe | Cuándo |
|---|---|
| `M RAMTOP no válido` | `dirección` está entre 1 y 23820; o es una dirección real por encima de la memoria del BASIC, o tan baja que no dejaría sitio para las variables (1 KB más) y el almacén de gráficos. |
| `B Entero fuera de rango` | `dirección` es negativa o mayor que 4 294 967 295. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 LET a=1: CLEAR: PRINT a
```

da `2 Variable no encontrada, 10:3`.

**Ver también:** [RUN](#run), [NEW](#new), [La memoria del Spectrum
virtual](#la-memoria-del-spectrum-virtual).

### DEF FN, FN

```
DEF FN letra([parámetro [, parámetro]...]) = expresión
DEF FN letra$([parámetro [, parámetro]...]) = expresión de cadena
FN letra(valores)
FN letra$(valores)
```

`DEF FN` define una función propia, y `FN` la usa dentro de una expresión.
`DEF FN` puede ir en cualquier sitio del programa y no hace nada cuando se
llega a él; `FN` busca la definición en el programa cada vez (si hay dos con
el mismo nombre, vale la primera). Los paréntesis van siempre, aunque no
haya parámetros: `DEF FN r()=INT (RND*6)+1` y `FN r()`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `letra` | una letra, con `$` si devuelve una cadena | El nombre de la función. |
| `parámetro` | una letra, o una letra con `$` | Dentro de la expresión, toma el valor que se le da en `FN`. Fuera de ella, las variables con ese nombre no cambian. |
| `expresión` | número o cadena, según el nombre | Lo que devuelve. Puede usar variables del programa y otras `FN`. |
| `valores` | hasta 32, uno por parámetro | Números o cadenas, del tipo de su parámetro. |

**Devuelve** (`FN`): el valor de la expresión con esos parámetros.

**Errores**

| Informe | Cuándo |
|---|---|
| `P FN sin DEF FN` | No hay un `DEF FN` con ese nombre (y ese tipo) en el programa. |
| `Q Error en los parámetros` | El número o el tipo de los valores no coinciden con los parámetros, o hay más de 32 (contando los de las `FN` que se están calculando unas dentro de otras). |
| `C Sintaxis incorrecta` | El tipo de la expresión no es el del nombre (al teclear el `DEF FN`). |
| `4 Sin memoria` | Demasiadas `FN` unas dentro de otras (una que se llama a sí misma sin fin). |
| (comunes) | Y los del cálculo de la expresión. Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 DEF FN h(x,y)=SQR (x*x+y*y)
20 DEF FN u$(a$)=a$+"!"
30 PRINT FN h(3,4); " "; FN u$("hola")
```

enseña `5 hola!`.

**Ver también:** [DEF PROC](#def-proc-proc-end-proc),
[Expresiones](#expresiones-y-operadores).

## Matemáticas

Las funciones de esta sección toman un número y devuelven un número. Como
todas las funciones, toman solo el valor que las sigue: `SIN x*2` es
`(SIN x)*2` (ver [Expresiones](#expresiones-y-operadores)). Un argumento que
no es un número da `C Sintaxis incorrecta` al teclear la línea.

### SIN, COS, TAN

```
SIN x     COS x     TAN x
```

Seno, coseno y tangente de un ángulo en **radianes** (una vuelta entera es
`2*PI`). Para usar grados, multiplica por `PI/180`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x` | cualquier número | El ángulo, en radianes. |

**Devuelve:** el seno o el coseno (de -1 a 1), o la tangente (cualquier
número: `TAN (PI/2)` da un número muy grande, no un error).

**Errores**

| Informe | Cuándo |
|---|---|
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
PRINT SIN (PI/2); " "; COS (60*PI/180)
```

enseña `1 0.5`.

**Ver también:** [ASN, ACS, ATN](#asn-acs-atn), [PI](#pi).

### ASN, ACS, ATN

```
ASN x     ACS x     ATN x
```

Arcoseno, arcocoseno y arcotangente: el ángulo cuyo seno, coseno o tangente
es `x`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x` | de -1 a 1 en `ASN` y `ACS`; cualquier número en `ATN` | El seno, el coseno o la tangente. |

**Devuelve:** el ángulo en radianes: de `-PI/2` a `PI/2` (`ASN` y `ATN`) o de
0 a `PI` (`ACS`).

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | En `ASN` o `ACS`, `x` es menor que -1 o mayor que 1. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
PRINT 4*ATN 1
```

enseña `3.1415927`.

**Ver también:** [SIN, COS, TAN](#sin-cos-tan).

### EXP, LN

```
EXP x     LN x
```

`EXP x` es e (2,7182818...) elevado a `x`. `LN x` es el logaritmo neperiano.
Para el logaritmo en base 10, `LN x/LN 10`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x` | en `EXP`, hasta unos 88,7; en `LN`, mayor que 0 | El exponente o el número. |

**Devuelve:** e elevado a `x`, o su logaritmo.

**Errores**

| Informe | Cuándo |
|---|---|
| `6 Número demasiado grande` | En `EXP`, `x` pasa de unos 88,7 (el resultado pasaría de 1,7E38). |
| `A Argumento no válido` | En `LN`, `x` es 0 o negativo. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
PRINT EXP 1; " "; LN 100/LN 10
```

enseña `2.7182818 2`.

**Ver también:** [SQR](#sqr), [Expresiones](#expresiones-y-operadores) (la
potencia `^`).

### SQR

```
SQR x
```

Raíz cuadrada.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x` | 0 o mayor | El número. |

**Devuelve:** la raíz cuadrada de `x`.

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `x` es negativo. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
PRINT SQR 2
```

**Ver también:** [EXP, LN](#exp-ln), [ABS, SGN, INT](#abs-sgn-int).

### ABS, SGN, INT

```
ABS x     SGN x     INT x
```

- `ABS x`: `x` sin su signo (`ABS -3` es 3).
- `SGN x`: -1, 0 o 1 según `x` sea negativo, cero o positivo.
- `INT x`: el entero igual o inferior a `x`: `INT 2.7` es 2 e `INT -2.5` es
  -3. Para redondear al más cercano, `INT (x+0.5)`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x` | cualquier número | El número. |

**Devuelve:** lo dicho arriba.

**Errores**

| Informe | Cuándo |
|---|---|
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
PRINT ABS -3; " "; SGN -3; " "; INT -2.5
```

enseña `3 -1 -3`.

**Ver también:** [Argumentos enteros](#argumentos-enteros).

### PI

```
PI
```

El número π.

**Parámetros:** ninguno.

**Devuelve:** 3.14159265358979..., que `PRINT` enseña como `3.1415927`.

**Errores:** ninguno.

**Ejemplo**

```
PRINT "Area: "; PI*10^2
```

**Ver también:** [SIN, COS, TAN](#sin-cos-tan).

### RND, RANDOMIZE

```
RND
RANDOMIZE [n]
```

`RND` da un número al azar. Para un entero del 1 al 6, `INT (RND*6)+1`.

`RANDOMIZE n` empieza la secuencia en un punto fijo, así que vuelven a salir
los mismos números «al azar». `RANDOMIZE` o `RANDOMIZE 0` la empiezan según
el tiempo que lleva encendida la máquina. Al encender, la secuencia empieza
siempre en el mismo punto; ni `RUN` ni `NEW` la cambian.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–65535; 0 si no se pone | El punto de partida; 0, según el reloj. |

**Devuelve** (`RND`): un número desde 0 hasta 1, sin llegar a 1. Hay 65536
distintos, a saltos de 1/65536, y la secuencia se repite cada 65536.

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `n` es negativo o mayor que 65535. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 RANDOMIZE
20 FOR i=1 TO 5: PRINT INT (RND*6)+1;" ";: NEXT i
```

> **Si conoces el Spectrum:** `RND` da exactamente la misma secuencia que en
> el 48K (`RANDOMIZE 1: PRINT RND` da `0.0022735596` en los dos).

**Ver también:** [INT](#abs-sgn-int), [TIME](#time).

### BIN

```
BIN cifras
```

Escribe un número en binario, con ceros y unos: `BIN 1010` es 10. Viene bien
para diseñar gráficos con
[`POKE USR`](#gráficos-definidos-por-el-usuario-udg). Es una forma de
escribir un número, no una función: detrás tiene que ir el número, no una
expresión.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `cifras` | ceros y unos, seguidos | El número en binario. Lo primero que no es un 0 ni un 1 (también un espacio) acaba el número. Sin cifras, `BIN` vale 0. |

Ojo con los espacios: `BIN 1010 1010` es el 10 seguido de otro número, el
1010, y en un `PRINT` sale `101010`.

**Devuelve:** el número.

**Errores:** ninguno al ejecutar. Lo que sigue al número tiene que encajar
en la línea: `LET a=BIN 2` da `C Sintaxis incorrecta` al teclearla.

**Ejemplo**

```
PRINT BIN 11111111; " "; BIN 10000000
```

enseña `255 128`.

**Ver también:** [Números](#números),
[UDG](#gráficos-definidos-por-el-usuario-udg).

## Funciones de texto

### LEN

```
LEN a$
```

El número de caracteres de una cadena.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `a$` | una cadena | El texto. |

**Devuelve:** su longitud; `LEN ""` es 0.

**Errores**

| Informe | Cuándo |
|---|---|
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
PRINT LEN "ZX-V"
```

**Ver también:** [Cadenas](#cadenas), [CHR$, CODE](#chr-code).

### CHR$, CODE

```
CHR$ n     CODE a$
```

`CHR$ n` es el carácter de código `n`, y `CODE a$`, el código del primer
carácter de `a$`. Ver el [Juego de caracteres](#juego-de-caracteres).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–255 | El código. |
| `a$` | una cadena | El texto; solo cuenta su primer carácter. |

**Devuelve:** `CHR$`, una cadena de un carácter; `CODE`, el código (0–255),
o 0 si `a$` está vacía.

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | En `CHR$`, `n` es negativo o mayor que 255. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
PRINT CHR$ 65; " "; CODE "a"
```

enseña `A 97`.

**Ver también:** [Juego de caracteres](#juego-de-caracteres),
[STR$, VAL](#str-val-val).

### STR$, VAL, VAL$

```
STR$ x     VAL a$     VAL$ a$
```

- `STR$ x` convierte un número en una cadena, tal como lo enseñaría `PRINT`:
  `STR$ 3.5` es `"3.5"`.
- `VAL a$` calcula la cadena como una expresión numérica: `VAL "2*3"` es 6.
- `VAL$ a$` calcula la cadena como una expresión de cadena: `VAL$ """x"""` es
  `"x"`.

`VAL` y `VAL$` entienden números, variables, comillas, paréntesis y los
signos `+ - * / ^ = < >`, pero no las palabras clave: dentro de una cadena,
`SIN`, `PI`, `AND` o `<=` son letras y signos sueltos, no la función o el
operador. Así, `VAL "PI"` busca una variable llamada `pi`, y `VAL "1<=2"` es
un error. Los espacios entre los elementos se ignoran: `VAL " 12 "` es 12,
pero `VAL "1 2"` es un error.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x` | un número | Lo que se convierte. |
| `a$` | una cadena | La expresión que se calcula. |

**Devuelve:** `STR$`, una cadena; `VAL`, un número; `VAL$`, una cadena.

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | La cadena de `VAL` o `VAL$` no es una expresión válida (también si está vacía, o lleva un operador escrito con letras o con dos signos, como `AND` o `<=`), o no es del tipo pedido (`VAL "a$"`). |
| `2 Variable no encontrada` | La expresión usa una variable que no existe (también una palabra clave como `VAL "PI"`). |
| (comunes) | Y los del cálculo de la expresión. Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 LET a$="12": LET b$="30"
20 PRINT a$+b$; " "; VAL a$+VAL b$
```

enseña `1230 42`.

**Ver también:** [Números](#números), [LEN](#len), [INPUT](#input).

## La pantalla y PRINT

La pantalla del programa tiene una **parte superior**, donde trabajan `PRINT`
y las órdenes de dibujo, y una **parte inferior** de dos líneas, abajo del
todo, que usan `INPUT` y los informes (y crece si hace falta). En el modo
normal, el 0, la pantalla tiene 24 filas de 32 caracteres (256 × 192
píxeles), y `PRINT` puede usar las filas 0 a 21; en el modo 1, 48 filas de
64 caracteres, y `PRINT` usa las filas 0 a 45. En una página mayor que la
pantalla (ver [MODE](#mode)), las filas y las columnas son las de la página.

`PRINT` y las órdenes de dibujo trabajan en la página de dibujo (ver
[PAGE, SCREEN](#page-screen)), que normalmente es la que se ve.

Cuando lo que se escribe pasa de la última fila, la pantalla sube. Cada vez
que ha subido una pantalla entera, se para y pregunta `scroll?`: pulsa N,
Espacio o BREAK para parar el programa (`D BREAK - CONT repite`), o
cualquier otra tecla para seguir.

### PRINT

```
PRINT [elemento] [separador] [elemento]...
```

Escribe números y textos en la pantalla, o en otro sitio con `#stream`.

**Parámetros**

Los elementos pueden ser:

| Elemento | Valores | Efecto |
|---|---|---|
| una expresión numérica o de cadena | — | Escribe su valor. Los números, como se explica en [Números](#números). Los caracteres de control de una cadena (`CHR$ 22` es `AT`...) funcionan como en el Spectrum (ver [Juego de caracteres](#juego-de-caracteres)). |
| `AT fila, columna` | `fila`: 0–21 (modo 1: 0–45); `columna`: 0–31 (modo 1: 0–63) | Mueve la posición de escritura. La fila 0 es la de arriba. |
| `TAB columna` | 0–65535 | Avanza hasta esa columna (contada dando la vuelta a la línea: `TAB 33` es `TAB 1` en el modo 0), rellenando con espacios. Si ya se ha pasado, va a la línea siguiente. |
| `INK n`, `PAPER n`, `FLASH n`, `BRIGHT n`, `INVERSE n`, `OVER n` | los de cada orden | Cambian los colores solo para el resto de este `PRINT` (ver [Colores](#colores)). |
| `#stream` | 0–15, abierto | Lo que sigue va por ese stream: 0 y 1, la parte inferior de la pantalla; 2, la superior; 3, la impresora; o un fichero abierto con `OPEN #` (ver [Streams](#streams-y-canales)). |

Y entre ellos, los separadores:

| Separador | Efecto |
|---|---|
| `;` | nada: lo siguiente va justo detrás |
| `,` | pasa a la siguiente columna múltiplo de 16 (en el modo 0, a la mitad o a la línea siguiente) |
| `'` | pasa a la línea siguiente |

Si el `PRINT` no acaba en un separador, al final pasa a la línea siguiente.
`PRINT` sin nada escribe una línea vacía.

**Errores**

| Informe | Cuándo |
|---|---|
| `5 Fuera de la pantalla` | `AT` con una fila o una columna que no caben (ver la tabla), o la parte inferior de la pantalla ya no puede crecer más. |
| `B Entero fuera de rango` | La fila o la columna de `AT` son negativas o mayores que 255, la columna de `TAB` es negativa o mayor que 65535, o el stream, negativo o mayor que 15. |
| `K Color no válido` | Un color fuera de su rango (ver [INK, PAPER](#ink-paper) y las demás). |
| `O Canal no válido` | El stream no está abierto. |
| `D BREAK - CONT repite` | Se ha contestado N, Espacio o BREAK a `scroll?`. |
| `J Dispositivo no válido` | El stream es un fichero abierto para leer (`IN`). |
| `U`, `V`... | Los de escribir en un fichero (ver [Leer y escribir ficheros de texto](#leer-y-escribir-ficheros-de-texto)). |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PRINT "Nombre", "Puntos"
20 PRINT "Ana", 120
30 PRINT AT 10,5; "en la fila 10"; TAB 25; "tab"
40 PRINT "uno"'"dos"; " y tres"
50 PRINT INK 2; "rojo"; INK 0; " negro"
```

**Ver también:** [LPRINT](#lprint-llist), [INPUT](#input),
[Colores](#colores), [Streams](#streams-y-canales).

### CLS

```
CLS
```

Borra la página de dibujo con el color de `PAPER` (y `BRIGHT` y `FLASH`)
permanente, también lo dibujado con las órdenes de gráficos rápidos y los
sprites. La parte inferior se borra con el color del borde y vuelve a tener
dos líneas, y la posición de escritura pasa a la esquina superior izquierda.
No cambia el punto de partida de `DRAW`.

**Parámetros:** ninguno.

**Errores:** ninguno.

**Ejemplo**

```
10 PAPER 1: CLS: PRINT "Fondo azul"
```

**Ver también:** [INK, PAPER](#ink-paper), [PAGE, SCREEN](#page-screen),
[CLEAR](#clear).

### SCREEN$, ATTR

```
SCREEN$ (fila, columna)
ATTR (fila, columna)
```

`SCREEN$` da el carácter escrito en esa posición de la pantalla. `ATTR` da
los colores de esa celda en un solo número. Las dos miran la página de
dibujo, también la parte inferior.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `fila` | 0–23 (modo 1: 0–47) | La fila de caracteres, contando desde arriba e incluyendo la parte inferior. |
| `columna` | 0–31 (modo 1: 0–63) | La columna. |

**Devuelve:** `SCREEN$`, el carácter (del 32 al 127, con la fuente actual,
normal o en inverso), o `""` si no reconoce lo que hay (un dibujo, un UDG o
un gráfico de bloque). `ATTR`, los últimos colores escritos en la celda:
`FLASH`×128 + `BRIGHT`×64 + `PAPER`×8 + `INK`, de 0 a 255.

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | La fila o la columna se salen de la pantalla (o son negativas). |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PRINT AT 0,0; INK 2; PAPER 6; "A"
20 PRINT SCREEN$ (0,0); " "; ATTR (0,0)
```

enseña `A 50` (6×8 + 2).

**Ver también:** [POINT](#point), [PIXEL](#pixel), [Colores](#colores).

### LPRINT, LLIST

```
LPRINT [elementos]
LLIST [#stream [, | ;]] [línea]
```

Como `PRINT` y `LIST`, pero a la **impresora**. ZX-V no tiene impresora de
papel: la impresora es el fichero de texto `printer.txt` de la raíz de la
tarjeta SD, y todo lo que se imprime se añade al final, línea a línea (lo
que queda de la última se escribe al pararse el programa). Se ignoran los
colores y `AT` (salvo su columna). Las líneas tienen hasta 64 caracteres (32
en el modo 0). Los caracteres que no son ASCII se escriben como `?` (la
libra, como `#`; el ©, como `c`), y las palabras clave, como palabras. Si
no hay tarjeta, lo impreso se pierde sin dar error.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `elementos` | los de [`PRINT`](#print) | Lo que se imprime. |
| `stream` | 0–15, abierto; 3 si no se pone | En `LLIST`, por dónde sale el listado. |
| `línea` | 0–65535; 0 si no se pone | En `LLIST`, la primera línea que se lista. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | Un valor de `AT`, `TAB`, el stream o la línea fuera de su rango (como en `PRINT`). |
| `K Color no válido` | Un color fuera de su rango (aunque no se use). |
| `O Canal no válido` | En `LLIST #`, el stream no está abierto. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
LPRINT "Lista de la compra": LPRINT "- leche"
```

**Ver también:** [PRINT](#print), [LIST](#list), [COPY](#copy),
[Streams](#streams-y-canales).

## Colores

Cada celda de carácter de la pantalla tiene una tinta (`INK`, el color del
carácter) y un papel (`PAPER`, el fondo). Los colores se numeran del 0 al 7, y
`BRIGHT 1` los hace más vivos:

| Número | Color | Con `BRIGHT 1` |
|---|---|---|
| 0 | negro | negro |
| 1 | azul | azul brillante |
| 2 | rojo | rojo brillante |
| 3 | magenta | magenta brillante |
| 4 | verde | verde brillante |
| 5 | cian | cian brillante |
| 6 | amarillo | amarillo brillante |
| 7 | blanco (gris claro) | blanco brillante |

Las [órdenes de sprites y gráficos
rápidos](#sprites-mapas-de-tiles-y-gráficos-rápidos), [`RASTER
BORDER`](#raster) y [`REMAP`](#remap) usan directamente los 16 colores, del
0 al 15: del 0 al 7 son los de la tabla y del 8 al 15, sus versiones
brillantes (el 10 es el rojo brillante). El 8 también es negro, y en los
sprites se usa como «transparente». Con [`PALETTE`](#palette), cada uno de
los 16 puede ser el color que quieras.

En ZX-V cada píxel tiene su propio color: dibujar un píxel solo cambia el
color de ese píxel, nunca el de sus vecinos.

> **Si conoces el Spectrum:** no hay *clash* de colores. `PLOT`, `DRAW` y
> `CIRCLE` pintan solo con la tinta, píxel a píxel; `ATTR` sigue dando los
> últimos colores escritos en cada celda.

`INK`, `PAPER`, `FLASH`, `BRIGHT`, `INVERSE` y `OVER` se usan de dos maneras:

- Como órdenes, cambian los colores de todo lo que se escribe y se dibuja a
  partir de ese momento (colores permanentes).
- Como elementos dentro de `PRINT`, `INPUT`, `PLOT`, `DRAW` y `CIRCLE`,
  seguidos de `;`, valen solo para esa orden (colores temporales).

Todas leen su valor como un número de 0 a 255: un valor negativo o mayor que
255 da `B Entero fuera de rango`, y uno de 0 a 255 que no es de los suyos,
`K Color no válido` (ver [Argumentos enteros](#argumentos-enteros)).

```
10 PAPER 1: INK 6: CLS
20 PRINT "amarillo sobre azul"
30 PRINT PAPER 2; INK 7; "blanco sobre rojo, solo esta vez"
40 PRINT "otra vez amarillo sobre azul"
```

### INK, PAPER

```
INK n
PAPER n
```

Eligen el color de la tinta y el del papel para lo que se escriba y se dibuje
después. Dentro de un `PRINT` (`PRINT INK 2; "Hola"`) solo valen para ese
`PRINT`. `PAPER` llega a toda la pantalla con el siguiente `CLS`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–7, 8 o 9 | El color: del 0 al 7, los de la [tabla de colores](#colores); 8, transparente (cada celda conserva el que tiene); 9, contraste (blanco o negro, el que mejor se vea sobre el otro color). |

**Errores**

| Informe | Cuándo |
|---|---|
| `K Color no válido` | `n` está entre 10 y 255. |
| `B Entero fuera de rango` | `n` es negativo o mayor que 255. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PAPER 1: INK 6: CLS
20 PRINT "Amarillo sobre azul"
```

**Ver también:** [BORDER](#border), [BRIGHT, FLASH](#bright-flash),
[ATTR](#screen-attr).

### BORDER

```
BORDER n
```

El color del borde que rodea la pantalla. La parte inferior de la pantalla
(las líneas de `INPUT` y de los informes) usa el color del borde como papel,
con tinta blanca sobre los colores oscuros (0 a 3) y negra sobre los claros
(4 a 7).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–7 | El color, de la [tabla de colores](#colores) (sin brillo). |

**Errores**

| Informe | Cuándo |
|---|---|
| `K Color no válido` | `n` está entre 8 y 255. |
| `B Entero fuera de rango` | `n` es negativo o mayor que 255. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
BORDER 0: PAPER 0: INK 4: CLS
```

**Ver también:** [RASTER BORDER](#raster), para hacer franjas de colores;
[INK, PAPER](#ink-paper).

### BRIGHT, FLASH

```
BRIGHT n
FLASH n
```

`BRIGHT 1` usa la versión brillante de la tinta y del papel; `BRIGHT 0`, la
normal. `FLASH 1` hace que la celda intercambie su tinta y su papel cada
16 cincuentavos de segundo (un tercio de segundo, más o menos); `FLASH 0`
lo quita.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0, 1 u 8 | 0 lo quita, 1 lo pone y 8 es transparente (cada celda conserva lo que tenía). |

**Errores**

| Informe | Cuándo |
|---|---|
| `K Color no válido` | `n` está entre 2 y 7, o entre 9 y 255. |
| `B Entero fuera de rango` | `n` es negativo o mayor que 255. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
PRINT FLASH 1; "FIN DE LA PARTIDA"
```

**Ver también:** [INK, PAPER](#ink-paper), [ATTR](#screen-attr).

### INVERSE, OVER

```
INVERSE n
OVER n
```

`INVERSE 1` escribe con la tinta y el papel intercambiados. `OVER 1` escribe
encima de lo que ya hay, cambiando cada píxel del carácter entre la tinta y
el papel (si escribes dos veces lo mismo con `OVER 1`, se borra). Con `PLOT`,
`DRAW` y `CIRCLE`, `OVER 1` cambia cada punto (si estaba pintado, lo borra)
e `INVERSE 1` pinta con el papel, es decir, borra.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0 o 1 | 0 lo quita y 1 lo pone. Aquí no hay 8. |

**Errores**

| Informe | Cuándo |
|---|---|
| `K Color no válido` | `n` está entre 2 y 255. |
| `B Entero fuera de rango` | `n` es negativo o mayor que 255. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PRINT AT 5,5; "X": PRINT AT 5,5; OVER 1; "O"
```

**Ver también:** [PLOT](#plot), [INK, PAPER](#ink-paper).

## Dibujo con PLOT y DRAW

Estas órdenes dibujan en la página de dibujo con el color de la tinta. Sus
coordenadas son píxeles, con el **(0, 0) en la esquina inferior izquierda**:
`x` va del 0 al 255 hacia la derecha e `y` del 0 al 175 hacia arriba (las dos
filas de abajo son de la parte inferior de la pantalla). En el modo 1, `x`
llega a 511 e `y` a 367; en una página mayor que la pantalla (ver
[MODE](#mode)), `x` llega al ancho de la página menos 1 e `y`, a su alto
menos 17.

Un punto que se sale de la pantalla da `B Entero fuera de rango`. Una línea
o una circunferencia que se salen dan el informe al llegar al borde: lo
dibujado hasta ahí se queda. Ojo: `PLOT` y `POINT` no miran el signo de las
coordenadas: `PLOT -10,20` dibuja en (10, 20).

Al principio se pueden poner elementos de color, seguidos de `;`, como en
`PLOT INK 2; 10,10`, que valen solo para esa orden. Sin ellos, cada punto
cambia solo la tinta de su celda (la que da `ATTR`): el papel, el brillo y el
parpadeo se conservan.

`PLOT`, `DRAW` y `CIRCLE` dejan el último punto dibujado como punto de
partida del siguiente `DRAW`. `CLEAR` y `RUN` lo llevan a (0, 0).

> Las [órdenes de gráficos rápidos](#sprites-mapas-de-tiles-y-gráficos-rápidos)
> (`BOX`, `SPRITE`...) cuentan la `y` desde arriba, como `PRINT AT`.

### PLOT

```
PLOT [colores;] x, y
```

Dibuja un punto, que será el de partida del siguiente `DRAW`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `colores` | `INK`, `PAPER`, `FLASH`, `BRIGHT`, `INVERSE`, `OVER` con su valor | Colores solo para este punto (ver [Colores](#colores)). |
| `x` | 0–255 (modo 1: 0–511) | La columna de píxeles, desde la izquierda. El signo se ignora. |
| `y` | 0–175 (modo 1: 0–367) | La fila de píxeles, desde abajo. El signo se ignora. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | El punto se sale de la pantalla (sin contar el signo), o un color es negativo o mayor que 255. |
| `K Color no válido` | Un color fuera de su rango. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 FOR x=0 TO 255 STEP 4: PLOT x, 88+40*SIN (x/20): NEXT x
```

**Ver también:** [DRAW](#draw), [POINT](#point), [BOX](#box),
[INVERSE, OVER](#inverse-over).

### DRAW

```
DRAW [colores;] dx, dy [, ángulo]
```

Dibuja una línea desde el último punto hasta el que está `dx` píxeles a la
derecha y `dy` píxeles más arriba (los valores negativos van a la izquierda y
hacia abajo). Con `ángulo` dibuja un arco, que gira eso por el camino: `PI`
dibuja media circunferencia, y un ángulo positivo curva en el sentido
contrario a las agujas del reloj. El punto de llegada es el de partida del
siguiente `DRAW`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `colores` | como en [`PLOT`](#plot) | Colores solo para esta línea. |
| `dx`, `dy` | de -255 a 255 (modo 1: de -511 a 511) | Cuánto se mueve. El punto de llegada tiene que caer en la pantalla. |
| `ángulo` | cualquier número, en radianes; 0 si no se pone | Cuánto gira el arco. 0, o una vuelta entera (`2*PI`), dibujan una recta. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `dx` o `dy` pasan del máximo, la línea se sale de la pantalla (lo dibujado hasta el borde se queda), o un color es negativo o mayor que 255. |
| `K Color no válido` | Un color fuera de su rango. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PLOT 50,50: DRAW 100,0: DRAW 0,100: DRAW -100,-100
20 PLOT 150,60: DRAW 60,0,PI
```

**Ver también:** [PLOT](#plot), [CIRCLE](#circle), [POLY](#poly).

### CIRCLE

```
CIRCLE [colores;] x, y, radio
```

Dibuja una circunferencia con el centro en `x`, `y`. Después, el siguiente
`DRAW` sale de un punto de su borde derecho.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `colores` | como en [`PLOT`](#plot) | Colores solo para esta circunferencia. |
| `x`, `y` | los de la pantalla | El centro. |
| `radio` | un número; el signo se ignora | En píxeles. Menor que 1, dibuja solo el punto del centro. Toda la circunferencia tiene que caber en la pantalla. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | La circunferencia se sale de la pantalla (lo dibujado hasta el borde se queda), o un color es negativo o mayor que 255. |
| `K Color no válido` | Un color fuera de su rango. |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 FOR r=10 TO 80 STEP 10: CIRCLE INK r/10; 128,88,r: NEXT r
```

**Ver también:** [DRAW](#draw), [DISC](#disc).

### POINT

```
POINT (x, y)
```

Una función: dice si un píxel de la página de dibujo está pintado con la
tinta. Ve lo que dibujan `PRINT`, `PLOT`, `DRAW` y `CIRCLE`, pero no las
órdenes de gráficos rápidos (para esas, [`PIXEL`](#pixel)).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x` | 0–255 (modo 1: 0–511) | La columna de píxeles, desde la izquierda. El signo se ignora. |
| `y` | 0–175 (modo 1: 0–367) | La fila de píxeles, desde abajo. El signo se ignora. |

**Devuelve:** 1 si el píxel es de tinta, 0 si es de papel.

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | El punto se sale de la pantalla (sin contar el signo). |
| (comunes) | Ver [Errores en cualquier orden](#errores-en-cualquier-orden). |

**Ejemplo**

```
10 PLOT 10,10: PRINT POINT (10,10); POINT (11,10)
```

enseña `10`.

**Ver también:** [PIXEL](#pixel), [HIT](#hit), [SCREEN$, ATTR](#screen-attr).

## Modos de vídeo y efectos

Estas órdenes cambian cómo se ve la pantalla entera: su tamaño, la página que
se enseña, el scroll y los colores, todo sin volver a dibujar nada.

**Errores comunes.** Los números se redondean al entero más cercano. Uno
negativo (salvo las líneas de `RASTER`, que pueden serlo), o mayor que el
máximo que admite la orden (255 en casi todas; 65535 en los tamaños, los
scroll y los valores de `RASTER`), da
`B Entero fuera de rango` antes de nada, aunque luego la orden compruebe un
rango más estrecho con otro informe: `PAGE 2` da `A`, pero `PAGE 300` da
`B`; `REMAP 20,1` da `K`, pero `REMAP 300,1` da `B`.

### MODE

```
MODE n
MODE n, ancho, alto
```

Elige el modo de la pantalla y la borra:

| Modo | Píxeles | Texto |
|---|---|---|
| 0 | 256 × 192 | 32 columnas × 24 filas (el del arranque) |
| 1 | 512 × 384 | 64 columnas × 48 filas |

`MODE` borra las dos páginas, enseña la 0 y dibuja en ella (como `PAGE 0`
y `SCREEN 0`), y quita el scroll, `REMAP`, `PALETTE`, `RASTER` y `CLIP`. Los
sprites, los tiles y lo demás del almacén de gráficos se conservan, y la
trama de `PATTERN` también.

En el modo 0, la página puede ser además más grande que la pantalla, que es
entonces una ventana que se mueve por ella con [`SCROLL`](#scroll):
`MODE 0, 512, 384`, `MODE 0, 1024, 192` o `MODE 0, 256, 768`. Así,
`PRINT AT`, `PLOT` y las demás cubren toda la página (`PRINT AT` llega a la
columna 127 en una página de 1024 píxeles de ancho).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0 o 1 | El modo. |
| `ancho`, `alto` | en el modo 0: 256, 192 (el normal), 512, 384, 1024, 192 o 256, 768; en el modo 1: 512, 384 (el normal) | El tamaño de la página, en píxeles. Opcionales; `0, 0` es lo mismo que no ponerlos. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `n` está entre 2 y 255; o `ancho`, `alto` no es uno de los tamaños de su modo (`MODE 1, 256, 192`, `MODE 0, 512, 192`...). |
| `B Entero fuera de rango` | `n` es negativo o mayor que 255; `ancho` o `alto`, negativo o mayor que 65535. |

**Ejemplo**

```
10 MODE 1: CIRCLE 256,184,150: PRINT AT 24,26; "Modo 1"
```

**Ver también:** [PAGE, SCREEN](#page-screen), [SCROLL](#scroll),
[CLS](#cls), [RASTER](#raster).

### PAGE, SCREEN

```
PAGE n
SCREEN n
```

ZX-V tiene dos páginas de pantalla, la 0 y la 1. `PAGE` elige la que se ve;
`SCREEN`, aquella en la que dibujan `PRINT`, `CLS`, `PLOT`, los sprites y las
demás. Dibujar en la página oculta y luego enseñarla da animaciones suaves,
sin parpadeo (el doble buffer).

Los informes de error e `INPUT` salen siempre en la página que se ve. El
editor también usa la página 1: cuando un programa vuelve a usarla, se
recuperan el texto y los dibujos de `PLOT` y `DRAW`, pero se pierden los
gráficos rápidos (sprites, mapas...) que hubiera en ella.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0 o 1 | La página. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `n` está entre 2 y 255. |
| `B Entero fuera de rango` | `n` es negativo o mayor que 255. |

**Ejemplo**

```
10 LET s=1
20 SCREEN s: CLS: CIRCLE 128,88,RND*80
30 PAGE s: LET s=1-s: WAIT 10: GO TO 20
```

**Ver también:** [MODE](#mode), [WAIT](#wait), [SCREEN$, ATTR](#screen-attr).

### SCROLL

```
SCROLL x, y
SCROLL
```

Desplaza la imagen sin volver a dibujarla: el píxel (`x`, `y`) de la página
se ve en la esquina superior izquierda de la pantalla, y la página da la
vuelta por los bordes. `SCROLL` sin nada la deja como estaba (0, 0). Así es
como un juego recorre una página grande de [`MODE 0`](#mode). `MODE` también
quita el scroll.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x`, `y` | 0 a 65535 | El píxel de la página que va a la esquina. Se toma el resto de dividir por el ancho o el alto de la página: en una página de 256 píxeles, `SCROLL 300,0` es lo mismo que `SCROLL 44,0`. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `x` o `y` es negativo o mayor que 65535. |

**Ejemplo**

```
10 MODE 0,1024,192: CIRCLE 600,100,50
20 FOR x=0 TO 1023 STEP 2: SCROLL x,0: WAIT: NEXT x
```

**Ver también:** [MODE](#mode), [RASTER](#raster) (scroll por líneas).

### REMAP

```
REMAP color, nuevo [; color, nuevo]...
REMAP #tabla; color, nuevo [; color, nuevo]...
REMAP #tabla
REMAP
```

Hace que un color se vea como otro en toda la pantalla, al instante y sin
volver a dibujar nada: tras `REMAP 1,10`, todo lo azul se ve rojo brillante.
Sirve para fundidos, destellos y ciclos de color.

Hay tres tablas de cambio, de la 0 a la 2. Sin `#`, `REMAP` cambia la tabla
0, que es la que se usa normalmente. `REMAP #t; ...` cambia la tabla `t` sin
enseñarla; `REMAP #t` hace que toda la pantalla use la tabla `t` (y así todos
sus cambios aparecen de golpe); [`RASTER REMAP`](#raster) usa una tabla solo
en algunas líneas. `REMAP` sin nada deja las tres tablas como al principio y
vuelve a la 0. [`MODE`](#mode) también.

`REMAP` cambia un color por otro de los 16; qué color es cada uno lo dice
[`PALETTE`](#palette).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `tabla` | 0–2 | La tabla de cambio. Sin `#`, la 0. |
| `color` | 0–15 | El color que cambia (el que está en la memoria de vídeo). |
| `nuevo` | 0–15 | El color con el que se ve. `REMAP c, c` lo deja como era. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `tabla` está entre 3 y 255. |
| `K Color no válido` | `color` o `nuevo` está entre 16 y 255. |
| `B Entero fuera de rango` | `tabla`, `color` o `nuevo` es negativo o mayor que 255. |

**Ejemplo**

```
10 PRINT INK 1; "parpadeo"
20 REMAP 1,10: PAUSE 25: REMAP 1,1: PAUSE 25: GO TO 20
```

**Ver también:** [PALETTE](#palette), [RASTER](#raster), [PIXEL](#pixel)
(que no tiene en cuenta `REMAP`).

### PALETTE

```
PALETTE color, r, g, b [; color, r, g, b]...
PALETTE #tabla; color, r, g, b [; color, r, g, b]...
PALETTE #tabla
PALETTE
```

Cambia cómo se ve uno de los 16 colores: `r`, `g` y `b` son su rojo, su
verde y su azul. Todo lo que está dibujado con ese color cambia al instante.
Tras `PALETTE 1, 255, 128, 0`, el azul se ve naranja.

Los colores de siempre tienen 215 de cada componente (255 los brillantes):
el rojo es `PALETTE 2, 215, 0, 0`. La pantalla tiene 65.536 colores
posibles, y se toma el más cercano.

Cada tabla de [`REMAP`](#remap) tiene su paleta. Sin `#`, `PALETTE` cambia
la de la tabla 0, la que se usa normalmente; `PALETTE #t; ...`, la de la
tabla `t`. Así, con [`RASTER REMAP`](#raster), cada franja de la pantalla
puede tener sus 16 colores. `PALETTE #t` sin más devuelve sus colores de
siempre a la tabla `t`, y `PALETTE` sin nada, a las tres (sin cambiar la
tabla que se usa). [`MODE`](#mode) también los devuelve.

El color 8 sigue siendo transparente en los sprites y en las celdas de mapa
que lo piden (no se dibuja), aunque tenga otro color en la paleta.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `tabla` | 0–2 | La tabla cuya paleta cambia. Sin `#`, la 0. |
| `color` | 0–15 | El color que cambia. |
| `r`, `g`, `b` | 0–255 | Su rojo, su verde y su azul. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `tabla` está entre 3 y 255. |
| `K Color no válido` | `color` está entre 16 y 255. |
| `B Entero fuera de rango` | `tabla`, `color`, `r`, `g` o `b` es negativo o mayor que 255. |

**Ejemplo**

```
10 FOR i=0 TO 15: BOX i, i*16, 0, 16, 176: NEXT i
20 FOR v=0 TO 255 STEP 5: PALETTE 7, v, v, 255-v: WAIT: NEXT v
```

**Ver también:** [REMAP](#remap), [RASTER](#raster),
[tabla de colores](#colores).

### RASTER

```
RASTER BORDER y1, y2, color
RASTER REMAP y1, y2, tabla
RASTER SCROLL y1, y2, x [, y]
RASTER
```

Cambia cosas línea a línea en la pantalla, sin gastar nada de tiempo del
procesador mientras el programa sigue:

- `RASTER BORDER`: las líneas de `y1` a `y2` tienen el borde de ese color.
- `RASTER REMAP`: esas líneas usan esa tabla de cambio de color (ver
  [REMAP](#remap) y [PALETTE](#palette)).
- `RASTER SCROLL`: esas líneas tienen su propio scroll horizontal `x` y, si se
  da `y`, enseñan las líneas de la página a partir de la `y` (para pantallas
  partidas con scroll independiente, y para el *parallax*). Sin `y`, enseñan
  las mismas líneas que sin `RASTER`.
- `RASTER` sin nada lo quita todo. `MODE` también.

Cada `RASTER` se suma a los anteriores: las líneas que ninguno ha tocado se
ven como sin `RASTER`. Da igual dar `y1` y `y2` en un orden o en el otro.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `y1`, `y2` | −24 a 215 en el modo 0; −48 a 431 en el modo 1 | Las líneas, filas de píxeles de la pantalla contadas desde arriba (la 0). Las del borde de arriba son negativas y las de abajo pasan del alto. No dependen del tamaño de la página. |
| `color` | 0–15 | El color del borde. |
| `tabla` | 0–2 | La tabla de cambio de color. |
| `x` | 0 a 65535 | El scroll horizontal de esas líneas: el píxel de la página que se ve a la izquierda (se toma el resto de dividir por el ancho de la página). |
| `y` | 0 a 65535 | La línea de la página que se ve en `y1`; las siguientes, detrás (se da la vuelta por el alto de la página). |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `y1` o `y2` fuera de las líneas del modo; `color`, `tabla`, `x` o `y` negativo o mayor que 65535. |
| `K Color no válido` | `color` está entre 16 y 65535. |
| `A Argumento no válido` | `tabla` está entre 3 y 65535. |
| `C Sintaxis incorrecta` | Detrás de `RASTER` no va `BORDER`, `REMAP` ni `SCROLL`. |

**Ejemplo**

```
10 FOR y=-24 TO 215 STEP 4: RASTER BORDER y,y+1,2: RASTER BORDER y+2,y+3,6: NEXT y
```

**Ver también:** [BORDER](#border), [REMAP](#remap), [SCROLL](#scroll),
[MODE](#mode).

## Sprites, mapas de tiles y gráficos rápidos

Estas órdenes usan el motor gráfico de la ROM para dibujar juegos deprisa:
sprites con transparencia, mapas de tiles con scroll, suelos en perspectiva
y fondos que giran (el «modo 7») y figuras rellenas, lisas o con trama. Se
diferencian de `PLOT` y `DRAW` en algunas cosas:

- Las **coordenadas** son píxeles desde la esquina **superior izquierda** de
  la página, con la `y` hacia abajo, como `PRINT AT`. Van de −32768 a 32767,
  y pueden ser negativas o salirse de la página: lo que cae fuera
  simplemente no se dibuja.
- Los **colores** son los 16 colores por su número, del 0 al 15 (ver la
  [tabla de colores](#colores)). En los sprites, el color 8 es
  transparente; en los mapas, solo en las celdas que lo piden (ver
  [MAP POKE](#map-dim-map-poke)).
- Dibujan en la página de dibujo que elige [`SCREEN`](#page-screen). Para
  animar sin parpadeo, dibuja en la página oculta, enséñala con `PAGE` y
  espera con [`WAIT`](#wait).
- `POINT`, `ATTR` y `SCREEN$` no ven lo que dibujan; [`PIXEL`](#pixel) sí.
  Una celda con `FLASH` deja de parpadear cuando una de estas órdenes dibuja
  encima.

Los sprites, los tiles, los mapas y las texturas se guardan por número en un
**almacén de gráficos** en la memoria. Se conservan con `RUN`, `CLEAR` y
`LOAD` (así un cargador puede preparar los gráficos y después cargar el
juego), y `NEW` los borra. Definir otra vez un número lo sustituye.
`SAVE "nombre" GRAPHICS` graba el almacén entero en un fichero `.gfx`, y
`LOAD "nombre" GRAPHICS` lo carga (ver [SAVE](#save) y [LOAD](#load)).
También puedes dibujarlos en el PC con el editor de gráficos `zxv-gfx`,
que viene con el emulador, o en la propia máquina con `LOAD "ROM:gfxedit"`,
o crear ficheros `.gfx` a partir de imágenes PNG con la herramienta
`png2gfx` del SDK (ver la [guía de gráficos del
SDK](../sdk/graphics.md#el-editor-de-gráficos-para-el-pc-zxv-gfx)).

| Almacén | Números |
|---|---|
| Sprites | 0–255 |
| Tiles | 0–4095 en cada uno de los dos juegos (de 8 × 8 y de 16 × 16) |
| Mapas | 0–15 |
| Texturas | 0–3 |

**Errores comunes.** Además de los propios de cada orden, todas las de esta
sección pueden dar:

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | El sprite, el mapa o la textura que se usa no existe, o al mapa le faltan tiles (tienen que existir todos los que usan sus celdas). |
| `B Entero fuera de rango` | Un número está fuera de su rango: un número de sprite, mapa, textura o tile; una coordenada fuera de −32768 a 32767; un color negativo o mayor que 255. |
| `K Color no válido` | Un color está entre 16 y 255. |
| `4 Sin memoria` | El sprite, el juego de tiles, el mapa o la textura no cabe en la memoria. |

### GRAB

```
GRAB n, x, y, ancho, alto [, t]
GRAB n, ancho, a$
```

Define el sprite `n`.

- La primera forma copia un rectángulo de la página de dibujo, con su esquina
  superior izquierda en `x`, `y`. Si se da `t`, ese color pasa a ser
  transparente. Lo que cae fuera de la página también es transparente, y el
  color 8 que hubiera en ella se guarda como 0 (los dos son negros).
- La segunda toma los píxeles de una cadena, un carácter por píxel, fila a
  fila y `ancho` píxeles por fila: las cifras hexadecimales `0`–`9` y `a`–`f`
  (o `A`–`F`) son los colores del 0 al 15, y cualquier otro carácter (`.`,
  un espacio...) es transparente, igual que el `8`. Si a la última fila le
  faltan caracteres, lo que falta es transparente.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–255 | El sprite. |
| `x`, `y` | −32768 a 32767 | La esquina superior izquierda del rectángulo en la página. |
| `ancho`, `alto` | 1–1024 | El tamaño del sprite, en píxeles. Con una cadena, el alto es el número de filas que salen de ella (hasta 1024). |
| `t` | 0–15 | Opcional: el color que pasa a ser transparente. |
| `a$` | de 1 carácter en adelante | Los píxeles. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | Con una cadena: `ancho` es menor que 1 o mayor que 1024, la cadena está vacía o salen más de 1024 filas. |
| `B Entero fuera de rango` | `n` fuera de 0–255; con un rectángulo, `ancho` o `alto` fuera de 1–1024; con una cadena, `ancho` fuera de −32768 a 32767. |
| (comunes) | Ver [Errores comunes](#sprites-mapas-de-tiles-y-gráficos-rápidos). |

**Ejemplo**

```
10 GRAB 0,8,"..2222..22222222.222222...2222..."
20 FOR x=0 TO 240 STEP 16: SPRITE 0,x,80: NEXT x
```

**Ver también:** [SPRITE](#sprite), [HIT](#hit), [TILE](#tile).

### SPRITE

```
SPRITE n, x, y [, espejo [, c$ [, ancho, alto]]]
```

Dibuja el sprite `n` con su esquina superior izquierda en `x`, `y`. Sus
píxeles transparentes dejan ver el fondo.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–255 | El sprite, definido con [GRAB](#grab). |
| `x`, `y` | −32768 a 32767 | Dónde va su esquina superior izquierda. |
| `espejo` | 0–3 | 0 normal (por defecto), 1 de izquierda a derecha, 2 boca abajo, 3 las dos cosas. |
| `c$` | cadena | Cambia los colores: el color `i` se dibuja con el color que dice el carácter `i` de la cadena (contando desde 0), una cifra hexadecimal. Un carácter que no es una cifra hexadecimal (o una cadena demasiado corta) deja ese color como está: `"0a"` dibuja el color 1 como el 10 y deja los demás. `""` no cambia nada. |
| `ancho`, `alto` | 1–4096 | Lo dibuja estirado o encogido a ese tamaño, para las cosas que se acercan o se alejan. Sin ellos, a su tamaño. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `n` fuera de 0–255, `espejo` fuera de 0–3, o `ancho` o `alto` fuera de 1–4096. |
| `C Sintaxis incorrecta` | `c$` no es una cadena. |
| (comunes) | Ver [Errores comunes](#sprites-mapas-de-tiles-y-gráficos-rápidos); sobre todo, `A` si el sprite no existe. |

**Ejemplo**

```
10 GRAB 0,4,"1111122112211111"
20 SPRITE 0,10,10: SPRITE 0,30,10,0,"0a": SPRITE 0,50,10,0,"",32,32
```

**Ver también:** [GRAB](#grab), [HIT](#hit), [CLIP](#clip).

### HIT

```
HIT(a, ax, ay, b, bx, by)
```

Una función: comprueba si chocan dos sprites, sin dibujar nada. Mira si
algún píxel sólido (no transparente) del sprite `a` dibujado en `ax`, `ay`
caería sobre un píxel sólido del sprite `b` dibujado en `bx`, `by`. Compara
los sprites como son, sin espejo ni cambio de tamaño.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `a`, `b` | 0–255 | Los dos sprites (pueden ser el mismo). |
| `ax`, `ay`, `bx`, `by` | −32768 a 32767 | Dónde estaría la esquina superior izquierda de cada uno. |

**Devuelve:** 1 si se tocan, 0 si no.

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `a` o `b` no está definido. |
| `B Entero fuera de rango` | Un sprite fuera de 0–255 o una coordenada fuera de −32768 a 32767. |

**Ejemplo**

```
10 GRAB 0,4,"1111111111111111"
20 PRINT HIT(0,0,0,0,3,3); HIT(0,0,0,0,4,0)
```

enseña `10`.

**Ver también:** [SPRITE](#sprite), [PIXEL](#pixel), [CELL](#cell).

### TILE

```
TILE lado, n, x, y [, ancho, alto [, t]]
TILE lado, n, a$
```

Define tiles para los mapas. Hay dos juegos de tiles, uno de 8 × 8 píxeles y
otro de 16 × 16, cada uno con tiles del 0 al 4095.

- La primera forma copia el tile `n` de la página de dibujo en `x`, `y`. Con
  `ancho` y `alto`, corta ese rectángulo en tiles (los que caben enteros),
  numerados desde `n` fila a fila. El color `t`, si se da, y lo que cae
  fuera de la página pasan a ser el 8, que es transparente en las celdas
  que lo piden (ver [MAP POKE](#map-dim-map-poke)); el 8 que hubiera en la
  página se guarda como 0.
- La segunda toma los píxeles de una cadena, como [`GRAB`](#grab) (lado × lado
  caracteres por tile; una cadena más larga define los tiles siguientes, y
  lo que le falte al último es el 8).

Un juego de tiles crece al definir un tile más alto; los nuevos empiezan
negros.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `lado` | 8 o 16 | El juego de tiles. |
| `n` | 0–4095 | El primer tile que se define. |
| `x`, `y` | −32768 a 32767 | La esquina superior izquierda en la página. |
| `ancho`, `alto` | 1–32767 | Opcionales: el rectángulo que se corta en tiles. Sin ellos, un tile (`lado` × `lado`). |
| `t` | 0–15 | Opcional: el color que pasa a ser transparente. |
| `a$` | cadena | Los píxeles. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `lado` está entre 0 y 255 pero no es 8 ni 16; no sale ningún tile (el rectángulo es más pequeño que un tile, o la cadena está vacía). |
| `B Entero fuera de rango` | `lado` es negativo o mayor que 255; `n` fuera de 0–4095; `ancho` o `alto` fuera de 1–32767; los tiles pasarían del 4095. |
| (comunes) | Ver [Errores comunes](#sprites-mapas-de-tiles-y-gráficos-rápidos). |

**Ejemplo**

```
10 BOX 2,0,0,8,8: POLY 7,0,3,7,3: TILE 8,1,0,0
```

**Ver también:** [MAP DIM, MAP POKE](#map-dim-map-poke), [MAP](#map),
[GRAB](#grab).

### MAP DIM, MAP POKE

```
MAP DIM m, ancho, alto [, lado [, repetir]]
MAP POKE m, x, y, valor
MAP POKE m, x, y, a$
```

`MAP DIM` crea el mapa de tiles `m` con `ancho` × `alto` celdas, todas con
el tile 0. Con `repetir` = 1, el mapa se repite sin fin en las dos
direcciones. Ocupa 2 bytes por celda.

`MAP POKE` pone la celda de la columna `x` y la fila `y` (contando desde 0).
El valor es el número del tile, más lo que haga falta de esto:

| Suma | Efecto |
|---|---|
| 4096 | tile al revés de izquierda a derecha |
| 8192 | tile boca abajo |
| 16384 | el color 8 del tile es transparente (deja ver lo de detrás); sin esto, el 8 se dibuja como un color más (negro, con la paleta de siempre) |
| 32768 | prioridad (ver el filtro de [MAP](#map)) |

Con una cadena, llena las celdas desde `x` hacia la derecha usando el código
de cada carácter como número de tile: `"  ##.."` usa los tiles 32, 35 y 46.
Así es fácil dibujar los niveles con texto. Lo que pasa del borde derecho
del mapa no se pone.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `m` | 0–15 | El mapa. |
| `ancho`, `alto` | 1–32767 | El tamaño del mapa, en celdas. |
| `lado` | 8 o 16 | El juego de tiles que usa. Por defecto, 8. |
| `repetir` | 0 o 1 | 1: se repite sin fin. Por defecto, 0. |
| `x`, `y` | 0 a `ancho` − 1, 0 a `alto` − 1 | La celda. |
| `valor` | 0–65535 | El tile (0–4095) más las sumas de la tabla. |
| `a$` | cadena | Un tile por carácter, por su código (0–255). |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `MAP DIM`: `lado` está entre 0 y 255 pero no es 8 ni 16. `MAP POKE`: el mapa no existe. |
| `B Entero fuera de rango` | `m` fuera de 0–15; `ancho` o `alto` fuera de 1–32767; `lado` negativo o mayor que 255; `repetir` que no es 0 ni 1; `x` o `y` fuera del mapa; `valor` fuera de 0–65535. |
| `4 Sin memoria` | El mapa no cabe. |

**Ejemplo**

```
10 BOX 5,0,0,8,8: TILE 8,35,0,0
20 MAP DIM 0,32,24: MAP POKE 0,0,12,"################################"
30 MAP POKE 0,5,11,35+8192: MAP 0
```

**Ver también:** [MAP](#map), [CELL](#cell), [TILE](#tile).

### MAP

```
MAP m [, mx, my [, x, y, ancho, alto [, filtro]]]
```

Dibuja el mapa `m` a partir de su píxel `mx`, `my`, en toda la página o en
el rectángulo `x`, `y`, `ancho`, `alto`. Cambiando `mx` y `my`, el mapa se
desplaza. Lo de fuera de un mapa que no se repite no se dibuja (se queda
lo que hubiera). `MAP` también se usa detrás de [`FLOOR`](#floor) y
[`ROTATE`](#rotate).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `m` | 0–15 | El mapa. |
| `mx`, `my` | −32768 a 32767 | El píxel del mapa que va en la esquina superior izquierda. Por defecto, 0, 0. |
| `x`, `y` | −32768 a 32767 | La esquina superior izquierda del rectángulo en la página. Por defecto, 0, 0. |
| `ancho`, `alto` | 0–32767 | El tamaño del rectángulo. Por defecto, toda la página. |
| `filtro` | 0–2 | 0 (por defecto) dibuja todas las celdas, 1 solo las que no tienen prioridad y 2 solo las que la tienen: así se puede dibujar el fondo, después los sprites y por último las partes del escenario que van delante. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | El mapa no existe, o falta alguno de los tiles que usan sus celdas. |
| `B Entero fuera de rango` | `m` fuera de 0–15, `ancho` o `alto` fuera de 0–32767, `filtro` fuera de 0–2, o una coordenada fuera de −32768 a 32767. |

**Ejemplo**

```
10 BOX 5,0,0,8,8: TILE 8,32,0,0
20 BOX 2,0,0,8,8: POLY 7,0,3,7,3: TILE 8,35,0,0
30 MAP DIM 0,40,1: MAP POKE 0,0,0,"####  ####   ######  #########   ####"
40 FOR x=0 TO 64: MAP 0,x,0,0,100,256,8: WAIT: NEXT x
```

**Ver también:** [MAP DIM, MAP POKE](#map-dim-map-poke), [TILE](#tile),
[CELL](#cell), [CLIP](#clip).

### CELL

```
CELL(m, x, y)
```

Una función: el valor de la celda de la columna `x` y la fila `y` del mapa
`m`, con los bits de espejo, transparencia y prioridad. Sirve para saber
qué hay en el escenario en un sitio: la celda que hay bajo un sprite en el
píxel `px`, `py` es `CELL(m, INT (px/8), INT (py/8))` con tiles de 8
píxeles. No hace falta que existan los tiles.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `m` | 0–15 | El mapa. |
| `x`, `y` | −32768 a 32767 | La columna y la fila. En un mapa que se repite, dan la vuelta. |

**Devuelve:** el valor de la celda (0–65535: el tile más las sumas de
[MAP POKE](#map-dim-map-poke)); 0 fuera de un mapa que no se repite.

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | El mapa no existe. |
| `B Entero fuera de rango` | `m` fuera de 0–15, o `x` o `y` fuera de −32768 a 32767. |

**Ejemplo**

```
10 MAP DIM 0,10,10: MAP POKE 0,3,4,7+8192
20 PRINT CELL(0,3,4); " "; CELL(0,20,20)
```

enseña `8199 0`.

**Ver también:** [MAP DIM, MAP POKE](#map-dim-map-poke), [HIT](#hit),
[PIXEL](#pixel).

### TEXTURE

```
TEXTURE n, x, y, lado [, c [, t]]
```

Define la textura `n` para [`FLOOR`](#floor) y [`ROTATE`](#rotate): el
cuadrado de `lado` × `lado` píxeles de la página de dibujo con su esquina
superior izquierda en `x`, `y`. Lo que cae fuera de la página es negro (el
0). La textura se repite sin fin; si se da el color `c`, todo lo que queda
fuera de ella es de ese color (con `c` = −1 se repite, como sin `c`).

Si se da `t`, ese color pasa a ser **transparente**: donde la textura lo
tiene, se ve lo que había debajo. Con `c` igual que `t`, lo de fuera
tampoco se dibuja. Con `t`, el 8 que hubiera en la página se guarda como 0.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–3 | La textura. |
| `x`, `y` | −32768 a 32767 | La esquina superior izquierda del cuadrado en la página. |
| `lado` | 2, 4, 8, 16, 32, 64, 128 o 256 | El lado, una potencia de 2. Ocupa `lado` × `lado` bytes (64 KB con 256). |
| `c` | −1 o 0–15 | Opcional: el color de fuera; −1, se repite. |
| `t` | 0–15 | Opcional: el color transparente. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `lado` está entre 0 y 256 pero no es una potencia de 2 de 2 en adelante (0, 1, 3, 100...). |
| `B Entero fuera de rango` | `n` fuera de 0–3; `lado` negativo o mayor que 256 (`TEXTURE 0,0,0,512` da `B`); `c` menor que −1 o mayor que 255. |
| `K Color no válido` | `c` está entre 16 y 255. |
| (comunes) | Ver [Errores comunes](#sprites-mapas-de-tiles-y-gráficos-rápidos) (para `t` y la memoria). |

**Ejemplo**

```
10 PAPER 0: CLS: DISC 6,16,16,15: DISC 2,16,16,8: TEXTURE 1,0,0,32,0,0
```

define una textura con un sol sobre negro, y el negro (y lo de fuera) no se
dibuja.

**Ver también:** [FLOOR](#floor), [ROTATE](#rotate).

### FLOOR

```
FLOOR n, x, y, altura, ángulo, horizonte [, focal]
FLOOR MAP m, x, y, altura, ángulo, horizonte [, focal]
```

Dibuja un suelo en perspectiva con la textura `n` o, con `MAP`, con el mapa
de tiles `m` (como hacía la SNES: un circuito entero cabe en un mapa),
visto desde el punto `x`, `y` de la textura o del mapa a esa `altura`,
mirando en la dirección `ángulo`. El suelo ocupa desde la línea de debajo
del horizonte hasta el final de la página (o lo que deje [CLIP](#clip)). Es
el efecto «modo 7» de los juegos de carreras y de vuelo.

Con un mapa, cada celda lleva sus espejos y su transparencia (ver [MAP
POKE](#map-dim-map-poke)); un mapa que se repite se repite también en el
suelo, y lo de fuera de uno que no se repite no se dibuja. Dibujar con un
mapa cuesta más que con una textura: un suelo de media pantalla se lleva
un tercio de un frame en el modo 0. `MAP` va justo detrás de
`FLOOR`; en otro sitio es un nombre de variable.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–3 (textura) o 0–15 (con `MAP`) | La textura o el mapa. |
| `x`, `y` | mayores que −32768 y menores que 32768; pueden llevar decimales | El punto de la textura o del mapa, en píxeles, sobre el que está el que mira. |
| `altura` | mayor que 0 y menor que 32768 | La altura a la que mira. |
| `ángulo` | cualquiera, en radianes | La dirección: 0 mira hacia donde crece `x`. |
| `horizonte` | −32768 a 32767 | La línea de la página en la que está el horizonte. |
| `focal` | mayor que 0 y menor que 32768 | La distancia focal: con valores más pequeños se ve más ancho. Por defecto, la mitad del ancho de la página. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | La textura o el mapa no existe, o al mapa le falta algún tile. |
| `B Entero fuera de rango` | `n` fuera de su rango; `altura` o `focal` es 0 o negativa; `x`, `y`, `altura` o `focal` llega a 32768 (o a −32768); `horizonte` fuera de −32768 a 32767. |

**Ejemplo**

```
10 FOR i=0 TO 7: BOX i,i*8,0,8,64: NEXT i: TEXTURE 0,0,0,64
20 FOR a=0 TO 2*PI STEP .05: FLOOR 0,32,32,20,a,80: WAIT: NEXT a
```

Un circuito con un mapa de 32 × 32 celdas: una pista negra en cuadrado,
con bordillos rojos y blancos (una trama) y hierba alrededor:

```
10 BOX 4,0,0,8,8: POLY 12,2,2: POLY 12,6,5: TILE 8,46,0,0
20 BOX 0,0,0,8,8: TILE 8,35,0,0
30 PATTERN 1,15: BOX 2,0,0,8,8: PATTERN: TILE 8,61,0,0
40 MAP DIM 0,32,32,8,1
50 FOR r=0 TO 31: FOR c=0 TO 31: LET a=ABS (c-15.5): LET b=ABS (r-15.5)
60 LET m=a: IF b>a THEN LET m=b
70 LET t=46: IF m>=8 AND m<12 THEN LET t=35
80 IF (m>=7 AND m<8) OR (m>=12 AND m<13) THEN LET t=61
90 MAP POKE 0,c,r,t: NEXT c: NEXT r
100 BOX 5,0,0,256,96: FLOOR MAP 0,204,150,10,1.5*PI,95
```

**Ver también:** [TEXTURE](#texture), [ROTATE](#rotate), [MAP](#map),
[CLIP](#clip).

### ROTATE

```
ROTATE n, x, y [, ángulo [, zoom [, cx, cy]]]
ROTATE MAP m, x, y [, ángulo [, zoom [, cx, cy]]]
```

Dibuja la textura `n` o, con `MAP`, el mapa `m`, **girada y ampliada** en
toda la página (o en el rectángulo de [CLIP](#clip)), como los fondos y los
jefes que giran de la SNES. El punto `x`, `y` de la textura o del mapa se ve
en el punto `cx`, `cy` de la página, girado `ángulo` radianes en el sentido
del reloj y ampliado `zoom` veces.

La textura se repite si no tiene color de fuera. Con una textura con color
transparente (ver [TEXTURE](#texture)) o un mapa que no se repite, lo que
no se dibuja deja ver lo de debajo. Sin girar ni ampliar, `ROTATE` es un
scroll de la textura o del mapa: `ROTATE 0, x, y` pone su punto `x`, `y` en
el centro.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–3 (textura) o 0–15 (con `MAP`) | La textura o el mapa. |
| `x`, `y` | mayores que −32768 y menores que 32768; pueden llevar decimales | El punto de la textura o del mapa que va en `cx`, `cy`. |
| `ángulo` | cualquiera, en radianes | El giro, en el sentido del reloj. Por defecto, 0. |
| `zoom` | mayor que 0 y menor que 32768 | Cuántas veces se amplía: 1 (por defecto), un píxel por píxel; 2, el doble de grande; 0,5, la mitad. |
| `cx`, `cy` | −32768 a 32767 | Dónde se ve el punto `x`, `y` en la página. Por defecto, su centro. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | La textura o el mapa no existe, o al mapa le falta algún tile. |
| `B Entero fuera de rango` | `n` fuera de su rango; `zoom` es 0 o negativo (o tan pequeño que no llega a 1/65536); `x`, `y` o `zoom` llega a 32768 (o a −32768); `cx` o `cy` fuera de −32768 a 32767. |

**Ejemplo**

```
10 PAPER 0: CLS: DISC 6,16,16,15: DISC 2,16,16,8: BOX 1,12,4,8,24: TEXTURE 1,0,0,32,0,0
20 LET s=1: FOR a=0 TO 2*PI STEP .05
30 SCREEN s: BOX 5,0,0,256,192: ROTATE 1,16,16,a,2
40 PAGE s: LET s=1-s: WAIT: NEXT a
```

gira un sol sobre el cielo. Como cada vuelta borra la página y la vuelve a
dibujar, lo hace en la página oculta y luego la enseña (ver
[PAGE, SCREEN](#page-screen)): en la página que se ve, la máquina podría
mandarla a la pantalla a medio dibujar y el sol parpadearía.

**Ver también:** [TEXTURE](#texture), [FLOOR](#floor), [MAP](#map),
[CLIP](#clip).

### PATTERN

```
PATTERN n [, papel]
PATTERN b1, b2, b3, b4, b5, b6, b7, b8 [, papel]
PATTERN
```

Elige una **trama** para [POLY](#poly), [BOX](#box) y [DISC](#disc): en vez
de un color liso, dibujan su color donde la trama tiene un 1 y el `papel`
donde tiene un 0. Sin `papel`, donde la trama tiene un 0 no se dibuja nada
y se ve lo de debajo: así salen sombras y cristales «a medias».

- `PATTERN n` usa una de las tramas que ya trae (del 1 al 11):

  | n | Trama | n | Trama |
  |---|---|---|---|
  | 1 | Ajedrez (la mitad) | 7 | Diagonal que sube |
  | 2 | Un cuarto | 8 | Diagonal que baja |
  | 3 | Un octavo | 9 | Cuadrícula |
  | 4 | Tres cuartos | 10 | Ladrillos |
  | 5 | Rayas horizontales | 11 | Rombos |
  | 6 | Rayas verticales | | |

- `PATTERN b1, ..., b8` define una de 8 × 8 píxeles: un byte por fila, de
  arriba abajo, con el bit más alto para el píxel de la izquierda, como los
  [UDG](#gráficos-definidos-por-el-usuario-udg): `PATTERN BIN 11110000, ...`.
- `PATTERN` solo, o `PATTERN 0`, vuelve al color liso. `RUN` y `CLEAR`
  también (`MODE`, no).

La trama va pegada a la página, no a la figura: dos figuras que se tocan
casan. Dibujar con trama es igual de rápido.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–11 | Una de las tramas de la tabla; 0, ninguna. |
| `b1` ... `b8` | 0–255 | Las ocho filas de una trama propia. |
| `papel` | 0–15 | Opcional: el color de los ceros de la trama. Sin él, no se dibujan. |

Con 1, 2, 8 o 9 números: `PATTERN n`, `PATTERN n, papel`, las ocho filas, o
las ocho filas y el papel.

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | Hay de 3 a 7 números, o más de 9. Se ve ya al escribir la línea. |
| `B Entero fuera de rango` | `n` está entre 12 y 255; cualquier número es negativo o mayor que 255. |
| `K Color no válido` | `papel` está entre 16 y 255. |

**Ejemplo**

```
10 PATTERN 10,2: BOX 15,32,96,192,96
20 PATTERN 1: DISC 0,128,96,40: PATTERN
```

dibuja una pared de ladrillos rojos con argamasa blanca y encima una
sombra redonda que deja ver la pared a medias.

**Ver también:** [POLY](#poly), [BOX](#box), [DISC](#disc), [BIN](#bin).

### POLY

```
POLY color, x1, y1 [, x2, y2]...
```

Con un punto, dibuja un píxel; con dos, una línea; con tres o más, un
polígono relleno. Usa la trama de [PATTERN](#pattern), si la hay.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `color` | 0–15 | El color. |
| `x1`, `y1`... | −32768 a 32767 | Los puntos, de 1 a 64. |

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | No hay ningún punto (`POLY 4`). Se ve ya al escribir la línea. |
| `A Argumento no válido` | Hay más de 64 puntos. Se ve ya al escribir la línea. |
| `B Entero fuera de rango` | Una coordenada fuera de −32768 a 32767; `color` negativo o mayor que 255. |
| `K Color no válido` | `color` está entre 16 y 255. |

**Ejemplo**

```
10 POLY 4, 128,20, 200,150, 56,150
```

**Ver también:** [BOX](#box), [DISC](#disc), [PATTERN](#pattern),
[DRAW](#draw).

### BOX

```
BOX color, x, y, ancho, alto
```

Un rectángulo relleno con su esquina superior izquierda en `x`, `y`. Usa la
trama de [PATTERN](#pattern), si la hay. Con `ancho` o `alto` 0 no dibuja
nada.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `color` | 0–15 | El color. |
| `x`, `y` | −32768 a 32767 | La esquina superior izquierda. |
| `ancho`, `alto` | 0–32767 | El tamaño, en píxeles. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `ancho` o `alto` negativo o mayor que 32767; una coordenada fuera de −32768 a 32767; `color` negativo o mayor que 255. |
| `K Color no válido` | `color` está entre 16 y 255. |

**Ejemplo**

```
10 BOX 2, 10,10, 100,50
```

**Ver también:** [DISC](#disc), [POLY](#poly), [PATTERN](#pattern),
[CLIP](#clip).

### DISC

```
DISC color, x, y, radio
```

Un círculo relleno con el centro en `x`, `y`. Usa la trama de
[PATTERN](#pattern), si la hay. Con `radio` 0, un píxel.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `color` | 0–15 | El color. |
| `x`, `y` | −32768 a 32767 | El centro. |
| `radio` | 0–32767 | El radio, en píxeles. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `radio` negativo o mayor que 32767; una coordenada fuera de −32768 a 32767; `color` negativo o mayor que 255. |
| `K Color no válido` | `color` está entre 16 y 255. |

**Ejemplo**

```
10 DISC 6, 128,96, 40
```

**Ver también:** [BOX](#box), [POLY](#poly), [CIRCLE](#circle),
[PATTERN](#pattern).

### CLIP

```
CLIP x, y, ancho, alto
CLIP
```

A partir de ese momento, las órdenes de esta sección solo dibujan dentro de
ese rectángulo. `CLIP` sin nada quita el límite (también lo quitan `MODE`,
`RUN` y `CLEAR`).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x`, `y` | −32768 a 32767 | La esquina superior izquierda del rectángulo. |
| `ancho`, `alto` | 0–32767 | Su tamaño, en píxeles. Con 0, no se dibuja nada. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `ancho` o `alto` negativo o mayor que 32767; `x` o `y` fuera de −32768 a 32767. |

**Ejemplo**

```
10 CLIP 50,50,100,50: DISC 2,100,75,60: CLIP
```

**Ver también:** [MAP](#map), [FLOOR](#floor), [ROTATE](#rotate),
[SPRITE](#sprite).

### PIXEL

```
PIXEL(x, y)
```

Una función: el color de un píxel de la página de dibujo, lo haya dibujado
quien lo haya dibujado. Sirve para los choques con cualquier cosa de la
pantalla.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x`, `y` | −32768 a 32767 | El píxel, contando desde arriba a la izquierda de la página. |

**Devuelve:** el color (0–15) tal como está en la memoria de vídeo, sin
aplicar `REMAP`; −1 si el píxel cae fuera de la página.

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `x` o `y` fuera de −32768 a 32767. |

**Ejemplo**

```
10 BOX 4,10,10,20,20
20 PRINT PIXEL(15,15); " "; PIXEL(5,5); " "; PIXEL(-1,0)
```

**Ver también:** [HIT](#hit), [POINT](#point), [CELL](#cell).

## Sonido y música

ZX-V tiene un *beeper* (un altavoz sencillo, el que usa `BEEP`), dos chips
de sonido AY de tres canales cada uno (los que usan `PLAY`, `SOUND`, `MUSIC`
y `SFX`) y doce canales de muestras, que tocan sonidos grabados (los de
`SAMPLE` y los de la música de muestras).

**Errores comunes.** `LOAD nombre$ MUSIC`, `LOAD nombre$ SFX` y
`LOAD nombre$ SAMPLE n` pueden dar, además de los suyos:

| Informe | Cuándo |
|---|---|
| `F Nombre de fichero no válido` | El nombre está vacío, es demasiado largo o lleva caracteres que no valen. |
| `T Fichero no encontrado` | No existe el fichero (sin extensión, ninguno de los que se buscan). |
| `U No hay tarjeta SD` | El fichero va a `SD:` y no hay tarjeta. |
| `V Error de disco` | La tarjeta, la unidad `EXT:` o el fichero fallaron al leer. |
| `O Canal no válido` | Ya hay 8 ficheros abiertos en esa unidad. |
| `R Error al cargar` | El fichero no es de un formato que se entienda, o está roto. |
| `4 Sin memoria` | No cabe en la memoria del BASIC o en la de sonido. |
| `C Sintaxis incorrecta` | Con `SAVE`, `VERIFY` o `MERGE` en vez de `LOAD`. |

### BEEP

```
BEEP duración, tono
```

Toca una nota con el beeper, una onda cuadrada. El programa espera a que
acabe la nota; BREAK la corta.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `duración` | 0 a 10 | En segundos; puede llevar decimales. |
| `tono` | −60 a 69,99... | En semitonos por encima del do central: 0 es el do central, 12 el do de arriba y −12 el de abajo. Puede llevar decimales. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `duración` es negativa o mayor que 10, o `tono` es menor que −60 o llega a 70. |
| `L BREAK en el programa` | Se pulsó BREAK. |

**Ejemplo**

```
10 FOR n=0 TO 12: BEEP 0.1,n: NEXT n
```

**Ver también:** [PLAY](#play), [SOUND](#sound).

### PLAY

```
PLAY a$ [, b$]...
```

Toca música escrita como texto, con hasta seis voces a la vez: las tres
primeras cadenas suenan en los canales A, B y C del primer chip AY y las
tres siguientes, en los del segundo. Se pueden dar hasta ocho cadenas, como
en el 128K (allí la séptima y la octava eran para MIDI): la séptima y la
octava se comprueban, pero no suenan.

El programa espera a que acaben todas; BREAK las corta. Se comprueban todas
antes de que empiece a sonar nada. `PLAY` para la música de
[`MUSIC`](#music), que usa los chips a su manera; los efectos de `SFX`
siguen.

Dentro de cada cadena (las órdenes, en mayúsculas o minúsculas):

| Texto | Significado |
|---|---|
| `c d e f g a b` | una nota (do, re, mi, fa, sol, la, si); en mayúsculas, `C`...`B`, una octava más arriba |
| `#`, `$` | sostenido y bemol (delante de la nota): `#f`, `$b` |
| `&` | un silencio |
| `1` a `9` | la duración de las notas siguientes: 1 semicorchea, 2 semicorchea con puntillo, 3 corchea, 4 corchea con puntillo, 5 negra (la de partida), 6 negra con puntillo, 7 blanca, 8 blanca con puntillo, 9 redonda |
| `10` `11` `12` | tresillos de semicorcheas, corcheas y negras: valen para las tres notas siguientes |
| `n_m` | duraciones ligadas: la nota siguiente dura las dos (se pueden ligar más: `5_5_3`) |
| `O n` | octava, de 0 a 8 (por defecto 5, en la que `c` es el do central) |
| `T n` | tempo en negras por minuto, de 60 a 240 (por defecto 120); solo cuenta en la primera cadena |
| `V n` | volumen, de 0 a 15 (por defecto 15); deja de usar la envolvente |
| `U` | usar la envolvente de volumen |
| `W n` | forma de la envolvente, de 0 a 7 |
| `X n` | periodo de la envolvente, de 0 a 65535 |
| `M n` | mezclador, de 0 a 63: los bits 0–2 encienden el tono de los canales A, B y C, y los bits 3–5 su ruido |
| `N` | separador (no hace nada) |
| `(` `)` | repite una vez lo que hay dentro (hasta 4 niveles); un `)` sin su `(` repite la cadena entera sin fin (hasta BREAK) |
| `!...!` | un comentario |
| `H` | para todas las voces y acaba el `PLAY` |
| `Y n`, `Z n` | MIDI en el 128K: se comprueban (`Y` hasta 16, `Z` hasta 255) y se ignoran |

Los espacios se ignoran. La envolvente y el mezclador son de cada chip, así
que los comparten sus tres voces. Cada `PLAY` empieza con el tempo a 120 y
el mezclador con el tono de sus voces encendido y el ruido apagado.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `a$`, `b$`... | de 1 a 8 cadenas | Las voces: 1.ª–3.ª en el primer AY, 4.ª–6.ª en el segundo, 7.ª–8.ª no suenan. |

**Errores**

| Informe | Cuándo |
|---|---|
| `Q Error en los parámetros` | Hay más de 8 cadenas. Se ve ya al escribir la línea. |
| `C Sintaxis incorrecta` | Un argumento no es una cadena. |
| `A Argumento no válido` | Una cadena lleva un carácter que no es de la tabla; una orden (`O`, `T`, `V`, `W`, `X`, `M`, `Y`, `Z`, o un `_`) sin su número; más de 4 `(` anidados; o un `(` sin su `)`. |
| `B Entero fuera de rango` | Un número pasa del máximo de su orden (`O` 8, `T` 240, `V` 15, `W` 7, `M` 63, `Y` 16, `Z` 255), o de 65535; `T` menor que 60; una duración 0 o mayor que 12. |
| `4 Sin memoria` | Las cadenas no caben en la memoria. |
| `L BREAK en el programa` | Se pulsó BREAK. |

**Ejemplo**

```
10 PLAY "T140 O4 5cdec 5cdec 5efg&"
20 PLAY "O4 3cegC", "O3 9c", "O5 V10 3(gfed)"
```

**Ver también:** [BEEP](#beep), [SOUND](#sound), [MUSIC](#music).

### SOUND

```
SOUND registro, valor [; registro, valor]...
```

Escribe directamente en los registros de los chips AY y de los canales de
muestras, para hacer efectos de sonido. Los registros del 0 al 15 son los
del primer chip y del 16 al 31, los del segundo (con el mismo significado,
16 más arriba). Lo que escriba `SOUND` en el chip de la música dura hasta
que la música vuelve a escribir ese registro.

| Registro | Uso |
|---|---|
| 0–1, 2–3, 4–5 | periodo del tono de los canales A, B y C (byte bajo, byte alto de 0 a 15); la frecuencia es 110 840 / periodo Hz |
| 6 | periodo del ruido, de 0 a 31 |
| 7 | mezclador: un bit a **0** enciende: bits 0–2 tono de A, B y C; bits 3–5 ruido de A, B y C |
| 8, 9, 10 | volumen de A, B y C, de 0 a 15; 16 usa la envolvente |
| 11–12 | periodo de la envolvente (bajo, alto) |
| 13 | forma de la envolvente (escribirlo la vuelve a empezar) |

Del 64 al 159 son los de los doce canales de muestras, ocho por canal: el
canal `c` (0–11) empieza en el 64 + 8 × `c`. Para tocar sonidos grabados es
más fácil [`SAMPLE`](#sample); los detalles están en los canales de
muestras de la [referencia del SDK](../sdk/reference.md).

| Registro | Uso |
|---|---|
| 64 + 8 × `c` | la muestra: una entrada de la tabla de muestras, 0–255 (las de `SAMPLE` son de la 224 a la 255) |
| + 1 | el volumen, 0–64 |
| + 2 | el panorama: 0 izquierda, 128 centro, 255 derecha |
| + 3 | el control: 1 la arranca, 2 la para (4, sin interpolar; 8, modo directo) |
| + 4, + 5, + 6 | el paso, de byte bajo a alto (8.16: 65 536 es 48 kHz) |
| + 7 | desde dónde empieza, de 256 en 256 muestras |

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `registro` | 0–31 y 64–159 | El registro. |
| `valor` | 0–255 | Lo que se escribe. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `registro` está entre 32 y 63 o pasa de 159; o `registro` o `valor` es negativo o mayor que 255. |

**Ejemplo**

```
10 SOUND 7,62; 8,15; 0,200; 1,0
20 PAUSE 25: SOUND 8,0
```

toca un tono en el canal A durante medio segundo.

**Ver también:** [PLAY](#play), [SAMPLE](#sample), [`IN` y `OUT`](#in-out),
para los puertos del 128K.

### MUSIC

```
LOAD nombre$ MUSIC
MUSIC PLAY
MUSIC STOP
MUSIC PAUSE
MUSIC CONTINUE
MUSIC                          (función)
```

Toca música hecha con un tracker, de fondo: el programa sigue mientras
suena, como en los juegos.

`LOAD nombre$ MUSIC` carga un módulo. Sin extensión, el primero que haya de
`nombre.zxm`, `nombre.pt3`, `nombre.mod`, `nombre.s3m`, `nombre.xm`,
`nombre.vgm` y `nombre.vgz`; si no hay ninguno, da el error de no
encontrar `nombre.pt3`. Puede ser:

- del formato propio de ZX-V (`.zxm`, con los dos chips AY y los canales de
  muestras: el que guarda
  [`zxv-tracker`](../sdk/README.md#el-tracker-para-el-pc-zxv-tracker) o
  [`zxv-music`](../sdk/README.md#el-formato-propio-zxm-y-zxv-music));
- un MOD, un S3M (Scream Tracker 3) o un XM (FastTracker 2), que convierte
  al cargarlos y suenan en los canales de muestras (hasta 12: de los que
  tengan más, los 12 que más notas tocan);
- de Pro Tracker 3 o Vortex Tracker II (`.pt3`, que también guarda
  `zxv-tracker`), un TurboSound (dos módulos, uno en cada chip AY: seis
  voces), de Pro Tracker 2, Sound Tracker, ASC Sound Master o un volcado
  PSG;
- un VGM (`.vgm` o comprimido, `.vgz`: los registros de los AY, como los
  graban los emuladores; ver [VGM](../sdk/README.md#vgm-los-ay)).

Las muestras de un MOD, un S3M, un XM o un `.zxm` van del fichero a la
memoria de sonido (96 KB) al cargarlo, y en la memoria del BASIC solo se
queda la canción (los patrones que se repiten, una vez): el fichero puede
ser más grande que ella (caben, por ejemplo, un MOD de 300 KB o un XM de
1,6 MB). Con más de 96 KB de muestras, las más grandes van en ADPCM, y si
ni así caben, además, a la mitad (con menos agudos): suenan mejor los que
tienen hasta unos 96 KB de muestras, y casi igual hasta unos 170 KB.

- `MUSIC PLAY` lo toca desde el principio, y al acabar vuelve a la
  posición de vuelta del módulo, sin fin.
- `MUSIC STOP` lo para; `MUSIC PAUSE` lo deja en silencio y `MUSIC
  CONTINUE` sigue donde estaba.
- La función `MUSIC` vale 1 mientras suena y 0 si no (también en pausa).

Un `.zxm` con pistas de muestras las toca en los canales de muestras desde
el 0 (uno por pista; los demás quedan libres), y al empezar sube sus
muestras al principio de la memoria de sonido y de su tabla.

Un módulo de los AY suena en el primer chip (un TurboSound o un VGM de dos
AY, en los dos); un MOD, un S3M o un XM, en los canales de muestras; y un
`.zxm`, en los chips y los canales de muestras que usan sus pistas. Lo toca
el mismo código que el tracker y que los programas en C. Se para con
`MUSIC STOP`, BREAK, `RUN`, `NEW`, [`PLAY`](#play) (que usa los chips a su
manera), al cargar otro módulo y al cargar un programa nativo; si el
programa acaba o da un error, sigue sonando. Lo que escriba `SOUND` en el
chip de la música dura hasta que la música vuelve a escribir ese registro.

En la memoria del BASIC, un módulo de los AY o un VGM ocupa lo que pesa su
fichero (de 2 a 15 KB un PT3 normal; un `.vgz`, además, 32 KB para
descomprimirlo mientras suena), y un MOD, un S3M, un XM o un `.zxm`, solo
su canción: sus muestras van a la memoria de sonido. El módulo va con los
gráficos: se conserva con `RUN` y `CLEAR` y `NEW` lo borra.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre$` | nombre de fichero | El módulo, con extensión o sin ella. |

**Devuelve** (la función `MUSIC`): 1 mientras suena la música; 0 si está
parada, en pausa o no hay ninguna.

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `MUSIC PLAY` sin un módulo cargado. |
| `C Sintaxis incorrecta` | `MUSIC` como orden sin `PLAY`, `STOP`, `PAUSE` ni `CONTINUE`; `SAVE`, `VERIFY` o `MERGE` con `MUSIC`. |
| `R Error al cargar` | El fichero no es de ninguno de esos formatos, o está roto. |
| `4 Sin memoria` | La canción no cabe en la memoria del BASIC; las muestras de un `.zxm` no caben en la memoria de sonido que dejan las de `SAMPLE`; o la canción tiene más de 224 muestras y hay muestras de `SAMPLE` cargadas. |
| (comunes) | Ver [Errores comunes](#sonido-y-música). |

**Ejemplo**

```
10 LOAD "tema" MUSIC: MUSIC PLAY
20 PRINT "Suena..."
30 IF INKEY$="" THEN GO TO 30
40 MUSIC STOP
```

**Ver también:** [SFX](#sfx), [SAMPLE](#sample), [PLAY](#play),
[LOAD](#load).

### SFX

```
LOAD nombre$ SFX
SFX n [, canal]
SFX STOP
SFX                            (función)
```

Efectos de sonido como los de los juegos, en los chips AY: un banco de
efectos AYFX (`.afb`, el que guarda la pestaña de efectos de `zxv-tracker`
o el AY Sound FX Editor; sin extensión, `nombre.afb`) o un efecto suelto
(`.afx`, con su extensión), que se toma como un banco de un efecto.

- `SFX n` toca el efecto `n` del banco (el primero es el 0) sin esperar,
  en un canal libre del chip que no usa la música (si no hay ninguno, en el
  que lleva más tiempo sonando). Con `canal`, en ese; un efecto en un canal
  de la música lo tapa mientras suena.
- `SFX STOP` para todos los efectos.
- La función `SFX` vale 1 mientras suena algún efecto.

Suenan a la vez que la música, `BEEP` y lo demás; `PLAY` no los para.
BREAK, `RUN` y `NEW` los paran; el banco se conserva como el módulo de
`MUSIC` (con `RUN` y `CLEAR`; `NEW` lo borra). Cargar otro banco lo
sustituye.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre$` | nombre de fichero | El banco (`.afb`) o el efecto (`.afx`). |
| `n` | 0 al número de efectos menos 1 (como mucho, 255) | El efecto. |
| `canal` | 0–5 | Opcional: 0 a 2, los canales A, B y C del primer chip; 3 a 5, los del segundo. |

**Devuelve** (la función `SFX`): 1 si suena algún efecto; 0 si no.

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `SFX n` sin un banco cargado. |
| `B Entero fuera de rango` | El efecto `n` no está en el banco; `n` negativo o mayor que 255; `canal` negativo o mayor que 5. |
| `C Sintaxis incorrecta` | `SFX PLAY`, `SFX PAUSE` o `SFX CONTINUE`; `SAVE`, `VERIFY` o `MERGE` con `SFX`. |
| `R Error al cargar` | El fichero no es un banco de efectos. |
| (comunes) | Ver [Errores comunes](#sonido-y-música). |

**Ejemplo**

```
10 LOAD "efectos" SFX
20 IF INKEY$=" " THEN SFX 0
30 GO TO 20
```

**Ver también:** [MUSIC](#music), [SAMPLE](#sample), [LOAD](#load).

### SAMPLE

```
LOAD nombre$ SAMPLE n
SAMPLE n [, tono [, volumen [, panorama]]] [; canal]
SAMPLE STOP [canal]
SAMPLE                         (función)
```

Sonidos grabados (golpes, voces, instrumentos) en los doce canales de
muestras, sin esperar, a la vez que la música y lo demás.

- `LOAD nombre$ SAMPLE n` carga un WAV (sin extensión, `nombre.wav`) en la
  muestra `n`: PCM de 8 o 16 bits, mono o estéreo (se mezcla), a cualquier
  frecuencia hasta 1 MHz. Va a la memoria de sonido (96 KB), en 8 bits; en
  la del BASIC no ocupa nada. Si el WAV trae un bucle (el que guardan
  Audacity y los editores de sonido), la muestra se repite en él hasta que
  se para. Cargar una muestra para las que estén sonando.
- `SAMPLE n` la toca a su altura, a todo volumen y en el centro.
- Sin `; canal`, suena en un canal que no usa la música (si no queda
  ninguno, en el que lleva más tiempo con una muestra). Con `; canal`, en
  ese: si es de la música, la tapa mientras suena, y la música sigue en él
  en su nota siguiente.
- `SAMPLE STOP` para todas; con `canal`, la de ese canal.
- La función `SAMPLE` vale 1 mientras suena alguna.

**Con la música:** las muestras de la canción van a la memoria de sonido
desde abajo y las de `SAMPLE`, desde arriba. Carga primero las de `SAMPLE`:
una canción que se carga después se ajusta al sitio que queda (con ADPCM, o
con menos agudos si hace falta), y una muestra que se carga después de una
canción solo usa lo que esta deja libre. Un `.zxm` ya está convertido y no
se ajusta: si no cabe, `4 Sin memoria`; hazlo con
[`zxv-music --free`](../sdk/README.md#el-formato-propio-zxm-y-zxv-music)
para que deje sitio. BREAK, `RUN` y `NEW` paran las muestras; las cargadas
se conservan como el banco de `SFX`, y `NEW` las borra.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre$` | nombre de fichero | El WAV. |
| `n` | 0–31 | La muestra (en la tabla de muestras, las entradas 224 a 255). |
| `tono` | en semitonos; puede llevar decimales | Sobre la altura a la que se grabó: 0 (por defecto) igual, 12 una octava más aguda, −12 una más grave. |
| `volumen` | 0–64 | Por defecto, 64. |
| `panorama` | 0–255 | 0 izquierda, 128 (por defecto) el centro, 255 derecha. |
| `canal` | 0–11 | Opcional: el canal de muestras. |

**Devuelve** (la función `SAMPLE`): 1 si suena alguna muestra de `SAMPLE`;
0 si no.

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `SAMPLE n` con la muestra `n` sin cargar. |
| `B Entero fuera de rango` | `n` negativo o mayor que 31 (también en `LOAD`); `volumen` fuera de 0–64; `panorama` fuera de 0–255; `canal` fuera de 0–11; un `tono` tan grave o tan agudo que la muestra no puede sonar (el paso tiene que quedar entre 1/65536 y 256 veces 48 kHz: para una muestra de 22 050 Hz, entre unos −178 y +109 semitonos). |
| `R Error al cargar` | El fichero no es un WAV PCM de 8 o 16 bits y uno o dos canales, o no trae sonido. |
| `4 Sin memoria` | La muestra no cabe en la memoria de sonido que queda; o la canción cargada tiene más de 224 muestras. |
| `C Sintaxis incorrecta` | `SAVE`, `VERIFY` o `MERGE` con `SAMPLE`. |
| (comunes) | Ver [Errores comunes](#sonido-y-música). |

**Ejemplo**

```
10 LOAD "golpe" SAMPLE 0: LOAD "tema" MUSIC: MUSIC PLAY
20 IF INKEY$=" " THEN SAMPLE 0, RND*4-2
30 GO TO 20
```

**Ver también:** [MUSIC](#music), [SFX](#sfx), [SOUND](#sound),
[LOAD](#load).

## Teclado, mando y ratón

### INPUT

```
INPUT [elementos] variable [, variable]...
INPUT [elementos] LINE a$
INPUT #stream; variable [, variable]...
```

Para el programa y deja que el usuario teclee los valores de unas variables
en la parte inferior de la pantalla. Los elementos son como los de `PRINT`:
las cadenas escritas en el programa y las expresiones entre paréntesis se
enseñan como aviso, `AT`, `TAB` y los colores funcionan, y cada variable
espera un valor.

- Para un **número**, el usuario teclea un número o una expresión numérica
  (vale incluso `2*3`) y pulsa Enter. Si tiene un error de sintaxis, un `?`
  marca dónde, para que lo corrija. Si es correcta pero falla al calcularla
  (una variable que no existe, `1/0`), el programa se para con ese informe.
- Para una **cadena**, la entrada empieza con unas comillas y el cursor
  dentro, porque lo que se teclea se toma como una expresión de cadena.
- `LINE a$` toma el texto tal como se teclea, sin comillas.
- Pulsar Esc (BREAK), o teclear `STOP` como valor, para el programa con
  `H STOP en INPUT`.

Mientras se teclea funcionan los cursores izquierda y derecha, Inicio, Fin,
Retroceso y Supr. El cursor enseña una `L`, o una `C` si está activado el
bloqueo de mayúsculas. Lo que se pulsó antes para `INKEY$`, `IN` o `KEY` no
aparece: la entrada empieza vacía.

Los textos del programa no pueden llevar tildes, eñes ni `¿`, y lo que se
teclea en el `INPUT` tampoco: las letras pierden la tilde, así que `Peña`
llega como `Pena` (ver [Juego de caracteres](#juego-de-caracteres)).

Con `#stream` lee de un fichero, una línea por variable (ver
[Leer y escribir ficheros de texto](#leer-y-escribir-ficheros-de-texto)).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| elementos | `"texto"`, `(expresión)`, `AT fila, col`, `TAB n`, `INK`...`OVER`, `#stream`, `;`, `,`, `'` | Lo que se escribe antes de pedir el valor, como en [PRINT](#print). Un texto de aviso tiene que ser una cadena escrita tal cual o una expresión entre paréntesis. |
| `variable` | una variable numérica o de cadena, o un elemento de una matriz | Dónde se guarda lo tecleado. |
| `LINE a$` | una variable de cadena | Toma el texto tal cual, sin comillas. |
| `stream` | 0–15 | El stream del que se lee (ver [Streams y canales](#streams-y-canales)). |

**Errores**

| Informe | Cuándo |
|---|---|
| `H STOP en INPUT` | Se pulsa Esc, o se teclea `STOP` como valor. |
| `C Sintaxis incorrecta` | Un aviso que no es una cadena escrita tal cual ni va entre paréntesis; `LINE` con una variable numérica. |
| `5 Fuera de la pantalla` | `AT` fuera de la pantalla inferior. |
| `B Entero fuera de rango` | Un valor de `AT`, `TAB`, un color o un stream fuera de su rango (más de 255, o del stream, más de 15). |
| `K Color no válido` | Un color que no vale (como en [INK, PAPER](#ink-paper)). |
| `O Canal no válido` | `#stream` de un stream que no está abierto. |
| (otros) | Cualquier error al calcular el valor tecleado (`2 Variable no encontrada`, `6 Número demasiado grande`...) para el programa. Los de un fichero: ver [Leer y escribir ficheros de texto](#leer-y-escribir-ficheros-de-texto). |

**Ejemplo**

```
10 INPUT "Tu nombre? "; LINE n$
20 INPUT "Tu edad? "; a
30 PRINT "Hola, "; n$; ". Pronto cumples "; a+1
```

**Ver también:** [PRINT](#print), [INKEY$](#inkey), [KEY](#key),
[Leer y escribir ficheros de texto](#leer-y-escribir-ficheros-de-texto).

### INKEY$

```
INKEY$
INKEY$ #stream
```

Una función: la tecla que está pulsada en ese momento, como una cadena de un
carácter, o `""` si no hay ninguna (o hay más de dos a la vez). No espera.
Lee el teclado como el del Spectrum:

- Las letras salen en minúscula (en mayúscula con Mayúsculas o con el bloqueo
  de mayúsculas), las cifras como cifras y Enter como `CHR$ 13`.
- Los cursores dan `CHR$ 8` (izquierda), `CHR$ 9` (derecha), `CHR$ 10`
  (abajo) y `CHR$ 11` (arriba), y Retroceso, `CHR$ 12`.
- La tecla Mayúsculas hace de CAPS SHIFT y Ctrl, de SYMBOL SHIFT: los signos
  de puntuación salen como los da el teclado (`,`, `"`...), y Ctrl con una
  letra da lo mismo que SYMBOL SHIFT con ella en el Spectrum, que a veces es
  una palabra clave (Ctrl+A da `CHR$ 226`, `STOP`).
- También cuenta un mando que se presenta como teclas con [`JOY`](#joy).

Lo que se pulsa mientras un programa lee así el teclado (con `INKEY$`, `IN` o
[`KEY`](#key)) no se queda guardado para después: al acabar el programa no
sale escrito en el editor, y un `INPUT` empieza vacío.

`INKEY$ #stream` lee el siguiente carácter de un fichero abierto para leer
(ver [Leer y escribir ficheros de texto](#leer-y-escribir-ficheros-de-texto)).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `stream` | 0–15 | Opcional: el stream de un fichero del que se lee. |

**Devuelve:** una cadena de un carácter, o `""` si no hay ninguna tecla (o
dos que no se combinan, o tres o más). Con `#stream`: el siguiente carácter
del fichero, o `""` al final del fichero o si el stream no va a un fichero
(la pantalla o la impresora).

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `stream` es negativo o mayor que 15. |
| `O Canal no válido` | El stream no está abierto. |
| `J Dispositivo no válido` | El fichero del stream está abierto para escribir (`OUT`). |
| (ficheros) | Si falla la lectura, los de [Ficheros y la tarjeta SD](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
10 LET k$=INKEY$: IF k$="" THEN GO TO 10
20 PRINT "Has pulsado "; k$: IF k$<>"q" THEN GO TO 10
```

**Ver también:** [KEY](#key), mejor para los juegos, [PAUSE](#pause),
[IN, OUT](#in-out).

### KEY

```
KEY n
```

Una función: 1 si la tecla `n` está pulsada ahora mismo, 0 si no. Se pueden
mirar varias teclas pulsadas a la vez, así que es la forma de leer el
teclado en los juegos. Mira las teclas del PC, no la matriz del Spectrum:
no ve un mando que se presenta como teclas con [`JOY`](#joy).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–511 (ver la tabla) | La tecla. |

| `n` | Tecla |
|---|---|
| de `CODE "a"` a `CODE "z"`, o de `CODE "A"` a `CODE "Z"` | una letra (las dos dan la misma tecla) |
| de `CODE "0"` a `CODE "9"` | una cifra |
| 8, 9, 10, 11 | los cursores izquierda, derecha, abajo, arriba |
| 12 | Retroceso |
| 13 | Enter |
| 32 | Espacio |
| de 256 a 511 | 256 + el código de teclado USB de cualquier tecla: 297 Esc, 298 Retroceso, 299 Tab, de 314 a 325 F1 a F12, 480 Ctrl izquierdo, 481 Mayúsculas izquierda, 482 Alt izquierdo, 484 Ctrl derecho, 485 Mayúsculas derecha, 486 Alt derecho (AltGr) |

**Devuelve:** 1 si la tecla está pulsada, 0 si no (también 0 con un código
de 256 a 511 que no es de ninguna tecla del teclado).

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `n` está entre 0 y 255 y no es de la tabla: de 0 a 7, de 14 a 31, de 33 a 47, de 58 a 64, de 91 a 96 y de 123 a 255. |
| `B Entero fuera de rango` | `n` es negativo o mayor que 511. |

**Ejemplo**

```
10 LET x=128
20 LET x=x+KEY 9-KEY 8: PLOT x,88: WAIT: GO TO 20
```

**Ver también:** [INKEY$](#inkey), [JOY](#joy), [MOUSE](#mouse).

### JOY

```
JOY n
JOY n, modo
```

La función `JOY n` da el estado del mando `n` (0 para el primero que se
conecta, 1 para el segundo, y así hasta el 3) como un número formado por estos
bits:

| Bit | Valor | Botón |
|---|---|---|
| 0 | 1 | derecha |
| 1 | 2 | izquierda |
| 2 | 4 | abajo |
| 3 | 8 | arriba |
| 4 | 16 | fuego 1 |
| 5 | 32 | fuego 2 |
| 6 | 64 | fuego 3 |
| 7 | 128 | start |
| 8 | 256 | select |
| 9 | 512 | fuego 4 |
| 10 | 1024 | L (gatillo de arriba izquierdo) |
| 11 | 2048 | R (gatillo de arriba derecho) |

Para mirar un botón, divide y quédate con el resto: por ejemplo,
`INT (JOY 0/16)-2*INT (JOY 0/32)` vale 1 mientras se pulsa el fuego 1.

La orden `JOY n, modo` hace que el mando `n` funcione además como teclas, para
los programas que leen el teclado con `INKEY$` o con `IN` (cualquier botón de
fuego cuenta como fuego):

| Modo | Izquierda | Derecha | Abajo | Arriba | Fuego |
|---|---|---|---|---|---|
| 0 | (no hace de teclas) | | | | |
| 1 Sinclair 1 | 6 | 7 | 8 | 9 | 0 |
| 2 Sinclair 2 | 1 | 2 | 3 | 4 | 5 |
| 3 Cursor | 5 | 8 | 6 | 7 | 0 |

Al encender la máquina, el mando 0 es Sinclair 1, el mando 1, Sinclair 2, y
los mandos 2 y 3 no hacen de teclas (`NEW` y `RUN` no lo cambian). `KEY` no ve estas teclas.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–3 | El mando (hasta la 0.11, 0 o 1). |
| `modo` | 0–3 | Solo la orden: cómo hace de teclas (tabla de arriba). |

**Devuelve** (la función): los botones pulsados, de 0 a 4095 (la suma de los
valores de la tabla); 0 si el mando no está conectado.

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `n` o `modo` no son de 0 a 3 (también en la función). |

**Ejemplo**

```
10 PRINT AT 0,0; JOY 0; "   ": GO TO 10
```

**Ver también:** [KEY](#key), [INKEY$](#inkey), [IN, OUT](#in-out).

### MOUSE

```
MOUSE n
MOUSE x, y
```

La función `MOUSE n` lee el ratón; la orden `MOUSE x, y` coloca el puntero.
El puntero empieza en el centro de la pantalla al encender la máquina y no
sale de ella: de 0 a 255 y de 0 a 191 en el modo 0, de 0 a 511 y de 0 a 383
en el modo 1. Un valor de la orden que cae fuera de la pantalla se lleva al
borde. ZX-V no dibuja el puntero: lo dibuja tu programa, por ejemplo con un
sprite.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–3 | Lo que se lee (tabla de abajo). |
| `x`, `y` | 0–65535 | Solo la orden: dónde se pone el puntero, en píxeles desde arriba a la izquierda. |

| `n` | Valor |
|---|---|
| 0 | la x del puntero, en píxeles desde la izquierda de la pantalla |
| 1 | la y del puntero, en píxeles desde arriba |
| 2 | los botones: 1 izquierdo, 2 derecho, 4 central (sumados) |
| 3 | la rueda: empieza en 0, crece al girarla hacia arriba y baja al girarla hacia abajo |

**Devuelve** (la función): el valor de la tabla.

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `n` es negativo o mayor que 3; `x` o `y` son negativos o mayores que 65535 (aunque luego el puntero se recorte a la pantalla). |

**Ejemplo**

```
10 GRAB 0,3,"ff.f.f..f"
20 CLS: SPRITE 0,MOUSE 0,MOUSE 1: IF MOUSE 2 THEN PRINT "clic"
30 WAIT: GO TO 20
```

**Ver también:** [SPRITE](#sprite), [KEY](#key), [IN, OUT](#in-out) (el
ratón Kempston).

## Ficheros y la tarjeta SD

Los programas y los datos se guardan en la tarjeta SD. Hay además una unidad
de solo lectura, `ROM:`, con los ejemplos que trae la máquina, y la unidad
externa, `EXT:`: una carpeta que sirve otro equipo (por la tira J5 de la
placa, o en el emulador con `--ext`), en la que se lee y se escribe como en
la tarjeta (ver [La unidad EXT:](../hardware/firmware.md#la-unidad-ext)).

Los errores de la tarjeta (o de `ROM:` o `EXT:`) los da el firmware, y el
BASIC los convierte en informes: un fichero que no existe da `T`, un nombre
que no vale da `F`, la falta de tarjeta da `U`, demasiados ficheros abiertos
dan `O`, y lo demás da `V Error de disco`, que añade el motivo cuando lo sabe.

**Errores comunes.** Además de los propios de cada orden, cualquier orden de
ficheros puede dar:

| Informe | Cuándo |
|---|---|
| `F Nombre de fichero no válido` | El nombre está vacío, pasa de 255 caracteres o lleva un carácter de control; una parte de la ruta pasa de 63 caracteres, lleva `\ : * ? " < > \|` o acaba en espacio o en punto; o hay comodines donde no valen. |
| `T Fichero no encontrado` | No existe una carpeta de la ruta, o el fichero donde tiene que existir. |
| `U No hay tarjeta SD` | La ruta va a la tarjeta y no hay ninguna en la ranura (o se ha sacado). |
| `O Canal no válido` | Ya hay 8 ficheros abiertos en esa unidad (con `OPEN #`) y hace falta abrir otro. |
| `V Error de disco` | La tarjeta ha fallado al leer o escribir, o no se entiende su formato (sin motivo detrás). |
| `V Error de disco (no es una carpeta)` | Una parte de la ruta que tendría que ser una carpeta es un fichero. |
| `V Error de disco (EXT: sin servidor)` | La ruta va a `EXT:` y no hay servidor, o ha dejado de contestar. |

Las órdenes que escriben dan además `V Error de disco (no se puede)` en
`ROM:`, y `V Error de disco (la tarjeta está llena)` si no queda sitio.

### Nombres de fichero y unidades

- Un nombre tiene hasta 63 caracteres, sin `\ : * ? " < > |`, sin letras con
  tilde ni eñes, y no acaba en espacio ni en punto. No se distinguen
  mayúsculas y minúsculas. Una ruta entera tiene hasta 255 caracteres.
- Las carpetas se separan con `/`. Una ruta que empieza por `/` parte de la
  raíz de la unidad; si no, de la carpeta actual (ver [CD](#cd-dir)). `..`
  es la carpeta de arriba (en la raíz, se queda en ella) y `.`, la misma.
- `ROM:`, `SD:` y `EXT:` al principio (en mayúsculas o no) eligen la unidad,
  desde su raíz: `LOAD "ROM:hello"`, `CAT "SD:/juegos"`, `SAVE "EXT:copia"`.
  Sin unidad, se usa la de la carpeta actual. Escribir en `ROM:` (`SAVE`,
  `ERASE`, `MKDIR`...) da `V Error de disco (no se puede)`. Sin un servidor
  conectado, todo lo de `EXT:` da `V Error de disco (EXT: sin servidor)`.
- **Comodines** en `CAT`, `ERASE`, `MOVE` y `COPY`: `*` vale por cualquier
  grupo de caracteres (también ninguno) y `?` por uno, sin distinguir
  mayúsculas, solo en la última parte de la ruta: `"*.zvb"`,
  `"juegos/nivel?.dat"`. Un comodín en una carpeta de la ruta da
  `F Nombre de fichero no válido`. Si no encaja nada, `ERASE`, `MOVE` y
  `COPY` dan `T Fichero no encontrado`; `CAT` no lista nada.
- Un fichero que está abierto (con `OPEN #`) no se puede abrir otra vez para
  escribir, ni borrar, ni cambiar de nombre: `V Error de disco (no se
  puede)`.

### SAVE

```
SAVE nombre$ [LINE línea [SCREEN$] [PROTECT]]
SAVE nombre$ CODE inicio, longitud
SAVE nombre$ SCREEN$
SAVE nombre$ DATA letra()
SAVE nombre$ DATA letra$()
SAVE nombre$ GRAPHICS
```

Graba en la tarjeta SD (o en `EXT:`). Si el fichero ya existe, se sustituye.

- El **programa** (sin más palabras) y sus variables, en un fichero `.zvb`,
  el formato de los programas de ZX-V: los dos tal como están en la memoria,
  sin más límite que la memoria. Con `LINE`, el programa arranca solo desde
  esa línea al cargarlo, con sus variables. Con `LINE` y `SCREEN$`, lleva
  además la pantalla que se ve como **pantalla de carga**: `LOAD` la pone
  antes que nada, y el programa empieza con ella. Con `PROTECT` al final, se
  graba **protegido** (ver [Programas protegidos](#programas-protegidos)).
- `CODE`: `longitud` bytes de la memoria desde la dirección `inicio`, en un
  fichero `.bin` (los bytes sin más). Las direcciones son las de
  [`PEEK`](#peek-poke): por debajo de 65536, el Spectrum virtual.
- `SCREEN$`: la pantalla del programa, tal como se ve (su modo, su borde y el
  color de cada píxel; si no son los colores de siempre, también los colores
  con que se ve, ver [`PALETTE`](#palette)), en un fichero `.gfx` con una
  pantalla (que se edita en `zxv-gfx`). Con la extensión `.scr`, la del
  Spectrum (`CODE 16384,6912`: la sombra de tinta y papel de cada celda, sin
  lo que se dibuja con los colores de ZX-V), en el formato de sus
  herramientas.
- `DATA`: una matriz (de números o de caracteres), en un fichero `.zvd`, el
  formato de los datos de ZX-V: como está en la memoria, sin más límite que
  la memoria.
- `GRAPHICS`: el almacén de gráficos (sprites, tiles, mapas, texturas) en un
  fichero `.gfx`.

Si el nombre no tiene extensión, se le añade la de cada cosa: `.zvb`,
`.bin`, `.zvd` o `.gfx` (`GRAPHICS` y `SCREEN$`). Con otra extensión, `CODE`
se graba igual, como bytes sin más, y un programa, una matriz o una pantalla
en su formato.

Con la extensión `.tap`, se graba como **cinta del Spectrum**, para llevarlo a
un emulador del Spectrum: el programa y sus variables, el bloque `CODE` (con
su dirección, para que `LOAD` lo devuelva a su sitio), la pantalla (la del
Spectrum, como con `.scr`) o la matriz, en el formato del Spectrum. La
pantalla de carga y la protección solo van en un `.zvb`. Las palabras clave
propias de ZX-V salen allí como caracteres raros, y un bloque no pasa de
65535 bytes, ni una cadena de 65535 caracteres. Un `.tzx` no se graba.

Con la extensión `.bas`, el programa se graba como **listado en texto** (ver
[Listados en texto](#listados-en-texto)): `SAVE "juego.bas"`. Un listado no
lleva las variables ni la línea de arranque.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre$` | ruta (ver [Nombres de fichero](#nombres-de-fichero-y-unidades)) | El fichero. No puede ser `""`. |
| `línea` | 0–9999 (hasta 65535 en un `.tap`) | La línea desde la que arranca el programa al cargarlo. |
| `inicio` | 0–4294967295 | La primera dirección de `CODE`. |
| `longitud` | 0–4294967295 (hasta 65535 en un `.tap`) | Cuántos bytes. |
| `letra` | una letra | La matriz de `DATA`: `a()` de números, `a$()` de caracteres. |

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | `CODE` sin `longitud`; `SAVE ... MUSIC`, `SFX` o `SAMPLE`; extensión `.tzx`; `.zvb` con otra cosa que un programa, o `.zvd` con otra cosa que una matriz; `LINE` (o `CODE`, `DATA`) en un `.bas`; `SCREEN$` de carga o `PROTECT` en un `.tap` o un `.bas`; `PROTECT` sin `LINE`; un programa protegido que se graba sin `LINE`. |
| `B Entero fuera de rango` | `línea` mayor que 9999 en un `.zvb` (o que 65535); una dirección de `CODE` que no se puede leer (ver [PEEK, POKE](#peek-poke)): entonces no se graba nada; en un `.tap`, un bloque (`CODE`, matriz, programa con variables) de más de 65535 bytes, o una cadena de más de 65535 caracteres. |
| `2 Variable no encontrada` | `DATA` de una matriz que no existe. |
| `6 Número demasiado grande` | En un `.tap`, un número que no cabe en el formato del Spectrum. |
| `V Error de disco (no se puede)` | En `ROM:`, o el fichero está abierto con `OPEN #`. |
| `V Error de disco (es una carpeta)` | `nombre$` es una carpeta. |
| `V Error de disco (la tarjeta está llena)` | No queda sitio. |
| (comunes) | Ver [Errores comunes](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
10 PRINT "Me grabaron"
SAVE "hola" LINE 10
```

**Ver también:** [LOAD](#load), [VERIFY](#verify),
[Programas protegidos](#programas-protegidos),
[Listados en texto](#listados-en-texto),
[Ficheros de cinta](#ficheros-de-cinta).

### LOAD

```
LOAD nombre$
LOAD nombre$ CODE [inicio [, longitud]]
LOAD nombre$ SCREEN$
LOAD nombre$ DATA letra()
LOAD nombre$ DATA letra$()
LOAD nombre$ GRAPHICS
LOAD nombre$ MUSIC
LOAD nombre$ SFX
LOAD nombre$ SAMPLE n
```

Carga de la tarjeta SD (o de `ROM:` o `EXT:`):

- Un **programa**: sustituye al que hay en la memoria, y sus variables a las
  que había. Si se grabó con `LINE`, se ejecuta desde esa línea sin borrarlas,
  como con `GO TO`. Sin extensión, ZX-V prueba `nombre.zvb`, después
  `nombre.tap`, después `nombre.zxv` (un [programa
  nativo](#programas-nativos), que se ejecuta), después `nombre.bas` (un
  [listado en texto](#listados-en-texto)) y por último el nombre tal cual. Los
  `.tap` y `.tzx` del Spectrum con programas en BASIC se cargan directamente,
  con sus variables (ver [Ficheros de cinta](#ficheros-de-cinta)); los `.tap`
  de ZX-V de antes de la 0.7 no llevan variables. El programa de antes se
  borra antes de leer el nuevo (así cabe uno tan grande como la memoria): si
  el fichero resulta no valer, la memoria se queda sin programa ni
  variables, pero la opción **Deshacer** del editor trae de vuelta las
  líneas.
- `CODE`: un fichero de bytes (sin extensión, `nombre.bin` y luego
  `nombre.tap`), en `inicio` y como mucho `longitud` bytes (sin `longitud`,
  el fichero entero). Un fichero que no es `.tap` no sabe dónde va, así que
  hace falta `inicio`: `LOAD "datos" CODE 40000`; el bloque `CODE` de un
  `.tap` vuelve sin ella a la dirección desde la que se grabó.
- `SCREEN$`: una pantalla (sin extensión, `nombre.gfx` y luego un bloque de
  `nombre.tap`). La de un `.gfx` se pone tal cual, con su modo (si no es el
  de ahora, cambia como con `MODE`) y su borde; si trae sus colores, se
  ponen en la paleta de la tabla 0, sin cambios de `REMAP`, y esa tabla pasa
  a ser la que se usa (como con `REMAP #0`). `SCREEN$()` no reconoce el texto
  que lleve. Un `.scr` o un `.tap` es una pantalla del Spectrum, que se ve
  como era.
- `DATA`: una matriz (sin extensión, `nombre.zvd` y luego `nombre.tap`), que
  sustituye a la que tenga ese nombre, como `DIM`. El tipo tiene que
  coincidir.
- `GRAPHICS`: los objetos de un fichero `.gfx` (sin extensión, se le añade)
  sustituyen a los que tienen el mismo número; los demás se quedan, así que
  puedes cargar los sprites de un fichero y un nivel de otro.
- `MUSIC`, `SFX` y `SAMPLE`: un módulo de música, un banco de efectos o
  un sonido grabado (ver [`MUSIC`](#music), [`SFX`](#sfx) y
  [`SAMPLE`](#sample)). Sin extensión, `MUSIC` prueba `.zxm`, `.pt3`,
  `.mod`, `.s3m`, `.xm`, `.vgm` y `.vgz` (si no hay ninguno, `.pt3`); `SFX`
  añade `.afb`, y `SAMPLE`, `.wav`.

`LOAD ""` (con el nombre vacío) carga el bloque siguiente de la cinta puesta
(ver [Ficheros de cinta](#ficheros-de-cinta)); con `GRAPHICS`, `MUSIC`,
`SFX` o `SAMPLE` el nombre vacío no vale.

**La carpeta del programa.** ZX-V se acuerda de la carpeta de la que se
cargó el programa. Un nombre sin unidad ni `/` delante se busca primero en
la carpeta actual y, si ahí no está, en la del programa. Así, un juego que
se carga con `LOAD "ROM:juego"` encuentra sus gráficos y su música con
`LOAD "juego" GRAPHICS`, sin escribir `ROM:`; y uno con el mismo nombre en
la carpeta actual de la tarjeta va antes, para cambiarlos sin tocar el
juego. Vale también para `VERIFY`, `MERGE` y `OPEN #` para leer (no para
grabar: `SAVE` va siempre a la carpeta actual). `NEW` la olvida.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre$` | ruta, o `""` (la cinta puesta) | El fichero. |
| `inicio` | 0–4294967295 | Dónde se carga `CODE`. Obligatoria si el fichero no es una cinta. |
| `longitud` | 0–4294967295 | Como mucho, cuántos bytes. Por defecto, todos. |
| `letra` | una letra | La matriz de `DATA`. |
| `n` | 0–31 | El número del sonido de `SAMPLE`. |

**Errores**

| Informe | Cuándo |
|---|---|
| `R Error al cargar` | El fichero no es válido o no es del tipo pedido (un `.zvb` con `CODE`, un `.zxv` o un `.bas` que no se cargan como programa, una matriz de otro tipo...); `LOAD ""` sin cinta puesta, o sin un bloque de ese tipo en ella; un bloque de cinta dañado; un `.zxv` que no es válido o es para una ROM más nueva. Si es un programa, la memoria se queda sin programa (ver arriba). |
| `T Fichero no encontrado` | No está el fichero con ninguna de sus extensiones; con una cinta puesta, tampoco hay en ella un bloque con ese nombre. |
| `C Sintaxis incorrecta` | `CODE` sin `inicio` de un fichero que no es `.tap`. |
| `B Entero fuera de rango` | `n` de `SAMPLE` mayor que 31; una dirección de `CODE` en la que no se puede escribir (ver [PEEK, POKE](#peek-poke)). |
| `4 Sin memoria` | El programa, la matriz, los gráficos o la música no caben; un `.zxv` que no cabe en la memoria del usuario. |
| `V Error de disco (es una carpeta)` | `nombre$` es una carpeta. |
| (comunes) | Ver [Errores comunes](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
LOAD "hola"
LOAD "ROM:graphics"
```

**Ver también:** [SAVE](#save), [MERGE](#merge), [VERIFY](#verify),
[Ficheros de cinta](#ficheros-de-cinta),
[Programas nativos](#programas-nativos).

### VERIFY

```
VERIFY nombre$
VERIFY nombre$ CODE [inicio [, longitud]]
VERIFY nombre$ SCREEN$
VERIFY nombre$ DATA letra()
VERIFY nombre$ GRAPHICS
```

Comprueba que un fichero es igual que lo que hay en la memoria, con las
mismas formas y las mismas extensiones que `LOAD`, y sin cambiar nada. Un
programa se compara con sus variables: si el programa ha funcionado desde que
se grabó, sus variables ya no son las mismas. Un programa sin extensión se
busca como `.zvb` y `.tap`, no como `.zxv` ni `.bas`.

**Parámetros:** los de [LOAD](#load).

**Errores**

| Informe | Cuándo |
|---|---|
| `R Error al cargar` | Hay diferencias; o el fichero no es válido o no es del tipo pedido (un `.zxv` o un `.bas` no se comprueban). |
| `C Sintaxis incorrecta` | `VERIFY ... MUSIC`, `SFX` o `SAMPLE`; `CODE` sin `inicio` de un fichero que no es `.tap`. |
| `B Entero fuera de rango` | Una dirección de `CODE` que no se puede leer. |
| (los de LOAD) | `T`, `4`... como en [LOAD](#load), y los [comunes](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
SAVE "prueba" CODE 40000,100: VERIFY "prueba" CODE 40000
```

**Ver también:** [SAVE](#save), [LOAD](#load).

### MERGE

```
MERGE nombre$
```

Añade las líneas de un programa grabado al que hay en la memoria; las líneas
con el mismo número se sustituyen. Con las variables pasa lo mismo, como en
el Spectrum: las del fichero sustituyen a las que tienen el mismo nombre y las
demás se quedan. Vale con un `.zvb`, un programa de una cinta (`.tap`,
`.tzx`) y un [listado en texto](#listados-en-texto) (`.bas`, que no lleva
variables); sin extensión se prueba en ese orden. No arranca el programa
aunque se grabara con `LINE`, y se puede deshacer con la opción
**Deshacer** del editor.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre$` | ruta, o `""` (el siguiente programa de la cinta puesta) | El fichero. |

**Errores**

| Informe | Cuándo |
|---|---|
| `R Error al cargar` | El fichero no es válido o no es un programa; el programa está [protegido](#programas-protegidos). |
| `C Sintaxis incorrecta` | `MERGE ... DATA`, `GRAPHICS`, `SCREEN$` (de un `.gfx`), `MUSIC`, `SFX` o `SAMPLE`, o un `.zvd`. |
| `4 Sin memoria` | Las líneas no caben. |
| `G No cabe la línea` | En un `.bas`, una línea de más de 1024 caracteres, o que no cabe en la memoria. |
| (los de LOAD) | `T`... como en [LOAD](#load), y los [comunes](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
MERGE "rutinas"
```

**Ver también:** [LOAD](#load), [Listados en texto](#listados-en-texto).

### Programas protegidos

```
SAVE nombre$ LINE línea [SCREEN$] PROTECT
```

Si no quieres que quien juegue a tu programa vea cómo está hecho, grábalo
con `PROTECT`. El programa que tienes en la memoria no cambia: puedes seguir
con él y grabarlo también sin proteger, para ti.

- Un programa protegido arranca solo al cargarlo, por eso necesita `LINE`.
  Solo se graba en un `.zvb` (en un `.tap` o un `.bas`, `C Sintaxis
  incorrecta`).
- Mientras funciona, es como cualquier otro: `ON ERROR` atrapa los errores y
  BREAK.
- Cuando se para, por lo que sea (un error, BREAK, `STOP` o porque acaba),
  sale su informe y **se borra**, con sus variables, como con `NEW`. No se
  puede listar ni deshacer el borrado, y el depurador no se conecta a él.
- `MERGE` no lo admite (`R Error al cargar`), y si él mismo graba el
  programa, se graba protegido (y necesita `LINE`).
- Si carga otro programa, ese ya no es protegido (salvo que se grabara así).

Es una protección como las del Spectrum: frena a los curiosos, pero el
formato de los ficheros es abierto, y quien sepa puede leerlo en un PC. Las
herramientas de ZX-V tampoco enseñan su listado.

### Listados en texto

Un programa también puede estar en un fichero de texto con la extensión
`.bas`, una línea del programa por línea, cada una con su número, como lo
saca `LIST`. Se puede escribir con cualquier editor de texto del ordenador,
copiarlo a la tarjeta y cargarlo con `LOAD "juego.bas"` (o `MERGE`);
`SAVE "juego.bas"` escribe el que hay en la memoria, con la sangría de
`LIST`.

```
10 REM Escrito fuera de ZX-V
20 for i = 1 to 10: print i: next i
```

- Cada línea se lee como si la tecleases en el editor: las palabras clave en
  mayúsculas o no, y la misma comprobación de sintaxis. Una línea con un error
  para la carga con su informe y su número de línea
  (`C Sintaxis incorrecta, 20:1`); las de antes se quedan en la memoria.
- El número va al principio (puede llevar espacios delante) y es de 1 a 9999;
  si no, `C Sintaxis incorrecta` (con el número de la última línea buena).
  Una línea con solo el número borra esa línea, y las líneas en blanco no
  cuentan. Un `.bas` es un programa, no una lista de órdenes (no pongas
  `RUN`).
- `LOAD` de un `.bas` borra antes el programa y las variables; `MERGE` lo
  mezcla con el que hay y deja las variables. Los dos se pueden deshacer
  (las líneas del programa, no las variables).
- El texto va en UTF-8. Las letras con tilde la pierden, como al teclearlas
  (`año` queda `ano`); `£` y `©` son las del Spectrum, y los gráficos de
  bloque se escriben con los caracteres de cuadrantes de Unicode
  (`▘ ▝ ▀ ▖ ▌ ▞ ▛ ▗ ▚ ▐ ▜ ▄ ▙ ▟ █`). Los caracteres UDG se graban como un byte
  suelto, que un editor de texto puede enseñar raro pero que se vuelve a
  cargar bien.
- Las líneas pueden acabar en LF (Linux, macOS) o en CR LF (Windows); una
  línea puede tener hasta 1024 caracteres (si no, `G No cabe la línea`).
- Un listado no lleva variables, ni línea de arranque, ni protección.

### CAT

```
CAT [nombre$]
```

Lista una carpeta en la pantalla del programa (el stream 2): primero su
nombre, después cada fichero con su tamaño en bytes, y cada carpeta con `/`
y `<DIR>`. Sin nombre, la carpeta actual. Con comodines o con el nombre de
un fichero, solo lo que encaja (si no encaja nada, solo sale el nombre).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre$` | ruta, con comodines en la última parte | Opcional: la carpeta, el fichero o el patrón. |

**Errores**

| Informe | Cuándo |
|---|---|
| `T Fichero no encontrado` | La carpeta no existe. |
| (comunes) | Ver [Errores comunes](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
CAT "ROM:"
CAT "*.zvb"
```

**Ver también:** [CD, DIR$](#cd-dir), [Nombres de fichero y
unidades](#nombres-de-fichero-y-unidades).

### CD, DIR$

```
CD carpeta$
DIR$
```

`CD` cambia la carpeta actual: `CD "juegos"`, `CD ".."` (la de arriba),
`CD "/"` (la raíz de la unidad), `CD "ROM:"` (la raíz de la unidad de los
ejemplos), `CD "EXT:"` (la de la unidad externa) y `CD "SD:"` (la raíz de la
tarjeta). La función `DIR$` da la carpeta actual, como `"/juegos"` (en la
tarjeta), `"ROM:/"` o `"EXT:/"`.

Un programa que carga más ficheros (como el juego de plataformas) los busca
en la carpeta actual y, si no están, en la suya (ver [LOAD](#load)): con
`LOAD "ROM:platformer"` encuentra sus gráficos sin cambiar de carpeta.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `carpeta$` | ruta de una carpeta | La nueva carpeta actual. |

**Devuelve** (`DIR$`): la ruta de la carpeta actual, desde la raíz de su
unidad, con `ROM:` o `EXT:` delante si no es la tarjeta.

**Errores**

| Informe | Cuándo |
|---|---|
| `T Fichero no encontrado` | La carpeta no existe. |
| `V Error de disco (no es una carpeta)` | `carpeta$` es un fichero. |
| (comunes) | Ver [Errores comunes](#ficheros-y-la-tarjeta-sd). `DIR$` no da errores. |

**Ejemplo**

```
CD "ROM:": PRINT DIR$: CAT
```

**Ver también:** [CAT](#cat), [MKDIR, RMDIR](#mkdir-rmdir).

### MKDIR, RMDIR

```
MKDIR carpeta$
RMDIR carpeta$
```

`MKDIR` crea una carpeta; `RMDIR` borra una carpeta vacía.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `carpeta$` | ruta | La carpeta. Las de antes en la ruta tienen que existir. |

**Errores**

| Informe | Cuándo |
|---|---|
| `V Error de disco (ya existe)` | `MKDIR` de algo que ya existe (carpeta o fichero). |
| `V Error de disco (la carpeta no está vacía)` | `RMDIR` de una carpeta con algo dentro. |
| `V Error de disco (no es una carpeta)` | `RMDIR` de un fichero. |
| `T Fichero no encontrado` | `RMDIR` de algo que no existe. |
| `V Error de disco (no se puede)` | En `ROM:`; `RMDIR` de la raíz. |
| `V Error de disco (la tarjeta está llena)` | `MKDIR` sin sitio. |
| (comunes) | Ver [Errores comunes](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
MKDIR "juegos": CD "juegos"
```

**Ver también:** [CD, DIR$](#cd-dir), [ERASE](#erase).

### ERASE

```
ERASE nombre$
```

Borra un fichero, o con comodines todos los ficheros que encajan
(`ERASE "*.bak"`); las carpetas no se borran con comodines (para eso,
`RMDIR`). Sin comodines, el nombre de una carpeta vacía también la borra.
No se puede deshacer.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `nombre$` | ruta, con comodines en la última parte | Lo que se borra. |

**Errores**

| Informe | Cuándo |
|---|---|
| `T Fichero no encontrado` | No existe, o no encaja ningún fichero con los comodines. |
| `V Error de disco (no se puede)` | En `ROM:`; el fichero está abierto con `OPEN #`; la raíz. |
| `V Error de disco (la carpeta no está vacía)` | Es una carpeta con algo dentro. |
| (comunes) | Ver [Errores comunes](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
ERASE "records.dat"
```

**Ver también:** [MKDIR, RMDIR](#mkdir-rmdir), [MOVE](#move).

### MOVE

```
MOVE de$ TO a$
```

Cambia el nombre de un fichero o de una carpeta, o lo lleva a otra carpeta
de la misma unidad: si `a$` es una carpeta, va dentro con el mismo nombre.
Con comodines, `a$` tiene que ser una carpeta y se mueven todos los ficheros
que encajan (las carpetas no). A diferencia de `COPY`, no sustituye un
fichero que ya existe. Para pasar algo a otra unidad, cópialo con `COPY` y
bórralo con `ERASE`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `de$` | ruta, con comodines en la última parte | Lo que se mueve. |
| `a$` | ruta | El nombre nuevo, o la carpeta a la que va. |

**Errores**

| Informe | Cuándo |
|---|---|
| `T Fichero no encontrado` | `de$` no existe, o no encaja ningún fichero con los comodines. |
| `V Error de disco (ya existe)` | Ya hay otra cosa con el nombre nuevo; con comodines, `a$` es la misma carpeta. |
| `V Error de disco (no es una carpeta)` | Con comodines, `a$` no es una carpeta. |
| `V Error de disco (no se puede)` | De una unidad a otra; en `ROM:`; un fichero abierto con `OPEN #`; la raíz. |
| `V Error de disco` | Una carpeta dentro de sí misma. |
| (comunes) | Ver [Errores comunes](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
MOVE "juego.zvb" TO "juego viejo.zvb"
MOVE "*.zvb" TO "juegos"
```

**Ver también:** [COPY](#copy), [ERASE](#erase).

### COPY

```
COPY de$ TO a$
COPY
```

Con dos nombres, copia un fichero, también de una unidad a otra (si `a$`
existe, lo sustituye; si es una carpeta, la copia va dentro con el mismo
nombre). Con comodines, `a$` tiene que ser una carpeta y se copian todos los
ficheros que encajan; si uno falla, se para ahí. Para copiar un ejemplo a la
tarjeta: `COPY "ROM:hello.zxv" TO "hello.zxv"`.

`COPY` sin nada manda el texto de la pantalla del programa a la impresora (el
fichero `printer.txt` de la tarjeta, ver [LPRINT](#lprint-llist)): cada
celda que no es un carácter conocido sale como `?`. Así no da errores.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `de$` | ruta, con comodines en la última parte | Lo que se copia. |
| `a$` | ruta | La copia, o la carpeta en la que va. |

**Errores**

| Informe | Cuándo |
|---|---|
| `T Fichero no encontrado` | `de$` no existe, o no encaja ningún fichero con los comodines. |
| `V Error de disco (es una carpeta)` | `de$` es una carpeta (sin comodines). |
| `V Error de disco (ya existe)` | Copiar un fichero sobre sí mismo. |
| `V Error de disco (no es una carpeta)` | Con comodines, `a$` no es una carpeta. |
| `V Error de disco (no se puede)` | `a$` está en `ROM:`, o está abierto con `OPEN #`. |
| `V Error de disco (la tarjeta está llena)` | No queda sitio (la copia queda a medias). |
| (comunes) | Ver [Errores comunes](#ficheros-y-la-tarjeta-sd). |

**Ejemplo**

```
COPY "ROM:hello.zxv" TO "hello.zxv"
```

**Ver también:** [MOVE](#move), [LPRINT, LLIST](#lprint-llist).

### Programas nativos

Los programas escritos en C con el [SDK](../sdk/README.md) son ficheros
`.zxv`. `LOAD "nombre"` los ejecuta (la extensión no hace falta). Un programa
nativo usa la memoria del BASIC: **cuando acaba, el programa BASIC y sus
variables han desaparecido**, así que graba antes tu programa en BASIC. Antes
de cargarlo, la máquina lo comprueba entero (un fichero dañado o copiado a
medias no se carga); luego cierra los ficheros abiertos con `OPEN #`.

```
LOAD "ROM:hello"
```

Cuando el programa acaba, el BASIC enseña un informe, como tras una orden
(`ON ERROR` no lo atrapa):

- `0 OK, 0:1` si ha acabado bien.
- `W Error en el código nativo (3), 0:1` si ha acabado con un código de error
  (el 3 en este ejemplo), o `W` seguido del mensaje del propio programa, si
  da uno: `W Fichero dañado (5), 0:1`.
- `W Error en el código nativo, 0:1` si ha fallado (por ejemplo, al leer de
  una dirección prohibida).
- Ninguno si lo has parado con Ctrl+Alt+Supr: la máquina vuelve a arrancar,
  como al encenderla.

El informe es del programa nativo: tu programa BASIC ha desaparecido en todos
los casos. Antes de arrancarlo pueden salir estos, y entonces el programa
BASIC sigue en la memoria:

| Informe | Cuándo |
|---|---|
| `R Error al cargar` | No es un `.zxv` válido, es para una ROM más nueva, o se ha pedido con `VERIFY` o `MERGE`. |
| `4 Sin memoria` | No cabe en la memoria del usuario (compilado para otra memoria, o demasiado grande). |
| (comunes) | Ver [Errores comunes](#ficheros-y-la-tarjeta-sd). |

Si la tarjeta falla, o el fichero cambia, cuando ya se está cargando, sale
su error (por ejemplo, `V Error de disco`) y el programa BASIC también se ha
perdido.

## Ficheros de cinta

ZX-V lee los ficheros de cinta de los emuladores del Spectrum, `.tap` y
`.tzx` (sus bloques de velocidad normal: programas, `CODE`, pantallas y
matrices; los demás bloques de un `.tzx` se saltan), para usar el código
BASIC hecho para el Spectrum. El código máquina del Spectrum que contengan no
se puede ejecutar. Los programas de ZX-V se graban en su propio formato,
`.zvb` (ver [SAVE](#save)); un `.tap` solo se graba si se pide, para llevar
un programa a un emulador del Spectrum.

Un fichero de cinta que se ha cargado se queda **puesto**, como una cinta en
la casetera, así que funcionan los programas de varias partes:

- `LOAD ""` (y `LOAD "" SCREEN$`, `LOAD "" CODE`, `LOAD "" DATA a()`,
  `VERIFY ""`, `MERGE ""`) lee el siguiente bloque de ese tipo desde donde se
  quedó la cinta. Al llegar al final, se rebobina una vez. Sin cinta puesta, o
  sin ningún bloque de ese tipo, `R Error al cargar`.
- `LOAD "nombre"` usa el fichero `nombre` (con las extensiones de siempre) si
  existe en la tarjeta, y si es una cinta pasa a ser la cinta puesta; si no
  existe, busca en la cinta puesta un bloque con ese nombre (hasta 10
  caracteres, sin distinguir mayúsculas), y si tampoco está,
  `T Fichero no encontrado`.
- Un bloque dañado (con la suma de comprobación mal, o cortado) da
  `R Error al cargar`.
- La cinta sigue puesta después de `NEW` y de `CD`, hasta que se carga otro
  fichero de cinta o se reinicia la máquina.

Así, `LOAD "juego"` carga el cargador de `juego.tap`, que puede traer su
pantalla de presentación y el programa con `LOAD "" SCREEN$: LOAD ""`.

Al grabar un `.tap`, el nombre del bloque es el del fichero sin la carpeta
ni la extensión, en sus 10 primeros caracteres: `SAVE "juegos/nave.tap"` se
llama `nave`, como en el Spectrum.

## Streams y canales

El texto entra y sale por unos **streams** (flujos) numerados del 0 al 15.
Los streams 0 a 3 están listos desde el principio:

| Stream | Va a |
|---|---|
| 0, 1 | la parte inferior de la pantalla (el canal `"k"`) |
| 2 | la parte superior de la pantalla (el canal `"s"`), la de `PRINT` |
| 3 | la impresora, el fichero `printer.txt` de la tarjeta (el canal `"p"`), la de `LPRINT` |

`PRINT`, `LIST` e `INPUT` admiten un stream con `#`: `PRINT #1; "abajo"`
escribe en la parte inferior y `LIST #3` lista en la impresora. Los streams 4
a 15 se pueden conectar a ficheros de texto. Un stream que no está abierto da
`O Canal no válido`.

**Errores comunes.** Además de los propios de cada orden, las de esta sección
pueden dar:

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | El número del stream es negativo o mayor que 15. |
| `O Canal no válido` | El stream no está abierto (salvo en `OPEN #` y `CLOSE #`). |
| (ficheros) | Al abrir, leer o escribir un fichero, los [errores comunes de los ficheros](#ficheros-y-la-tarjeta-sd). |

### OPEN, CLOSE

```
OPEN #stream, nombre$ IN
OPEN #stream, nombre$ OUT
OPEN #stream, canal$
CLOSE #stream
```

`OPEN # ... OUT` crea un fichero de texto (y lo vacía si ya existe) para
escribir en él; `OPEN # ... IN` abre uno para leer (también en la [carpeta
del programa](#load)). Los ficheros solo pueden usar los streams 4 a 15, y
cada unidad tiene como mucho 8 ficheros abiertos a la vez. Abrir un stream
que ya tiene un fichero lo cierra antes.

`OPEN #stream, "k"`, `"s"` o `"p"` (en mayúsculas o no) conecta un stream,
del 0 al 15, a la pantalla inferior, a la superior o a la impresora.

`CLOSE #stream` cierra el fichero (escribiendo lo que quedaba pendiente) y
deja libre el stream; los streams 0 a 3 vuelven a sus canales de siempre.
Cerrar un stream que no está abierto no hace nada. `RUN`, `CLEAR` y `NEW`
cierran todos los ficheros; `NEW` devuelve además los streams 0 a 3 a sus
canales.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `stream` | 0–15 (4–15 para un fichero) | El stream. |
| `nombre$` | ruta | El fichero de texto. |
| `canal$` | `"k"`, `"s"` o `"p"` | El canal de la pantalla o de la impresora. |

**Errores**

| Informe | Cuándo |
|---|---|
| `O Canal no válido` | Un fichero en los streams 0 a 3; ya hay 8 ficheros abiertos en esa unidad. |
| `J Dispositivo no válido` | `canal$` no es `"k"`, `"s"` ni `"p"` (sin `IN` ni `OUT`). |
| `T Fichero no encontrado` | `IN` de un fichero que no existe. |
| `V Error de disco (no se puede)` | `OUT` en `ROM:`; `OUT` de un fichero que ya está abierto. |
| `V Error de disco (es una carpeta)` | `nombre$` es una carpeta. |
| `V Error de disco (la tarjeta está llena)` | `OUT`, o `CLOSE` al escribir lo pendiente, sin sitio. |
| (comunes) | Ver [Errores comunes](#streams-y-canales). |

**Ejemplo**

```
10 OPEN #4, "notas.txt" OUT: PRINT #4; "Hola": CLOSE #4
```

**Ver también:** [Leer y escribir ficheros de texto](#leer-y-escribir-ficheros-de-texto),
[PRINT](#print), [LPRINT, LLIST](#lprint-llist).

### Leer y escribir ficheros de texto

```
PRINT #stream; elementos
LIST #stream [, línea]
INPUT #stream; var [, var]... [LINE a$]
INKEY$ #stream
```

- `PRINT #n` escribe como `PRINT`, sin colores ni `AT`; `TAB` escribe
  espacios y las palabras clave salen como texto. En un fichero, **la coma
  empieza una línea nueva**, para que lo que escribe `PRINT #n; a, b$` se
  pueda leer después con `INPUT #n; a, b$`. Lo escrito llega al fichero en
  cada salto de línea, al cerrarlo y al acabar cada orden.
- `LIST #n` escribe el programa como texto.
- `INPUT #n` lee una línea por cada variable (las líneas pueden acabar en LF,
  CR o CR LF; de una línea de más de 1024 caracteres, el resto se pierde).
  Una variable de cadena se queda con la línea entera; un número se calcula
  como una expresión. En `INPUT #n` no se escribe nada en el fichero: los
  avisos no valen.
- `INKEY$ #n` da el siguiente carácter del fichero, o `""` al final.

**Errores**

| Informe | Cuándo |
|---|---|
| `8 Fin del fichero` | `INPUT #` después de la última línea. |
| `C Sintaxis incorrecta` | `INPUT #` de un número, y la línea no es una expresión numérica válida. |
| `J Dispositivo no válido` | Escribir (`PRINT #`, `LIST #`, un aviso de `INPUT #`) en un fichero abierto con `IN`, o leer (`INPUT #`, `INKEY$ #`) de uno abierto con `OUT`. |
| `V Error de disco (la tarjeta está llena)` | Al escribir, sin sitio. |
| (comunes) | Ver [Errores comunes](#streams-y-canales). |

**Ejemplo**

```
10 OPEN #4, "records.txt" OUT: PRINT #4; "Ana", 120: CLOSE #4
20 OPEN #4, "records.txt" IN: INPUT #4; n$, p: CLOSE #4
30 PRINT n$; " hizo "; p; " puntos"
```

Para leer un fichero cuya longitud no conoces, atrapa el final del fichero
con [`ON ERROR`](#on-error):

```
10 ON ERROR GO TO 50: OPEN #5, "records.txt" IN
20 INPUT #5; LINE l$: PRINT l$: GO TO 20
50 CLOSE #5: IF ERR<>8 THEN PRINT "Error "; ERR
```

**Ver también:** [OPEN, CLOSE](#open-close), [INPUT](#input),
[INKEY$](#inkey), [LIST](#list).

### FORMAT

```
FORMAT ...
```

`FORMAT` solo existe por compatibilidad con programas antiguos del Spectrum:
lo que vaya detrás no se mira, y al ejecutarse da siempre
`O Canal no válido`.

**Ver también:** [OPEN, CLOSE](#open-close).

## Memoria y sistema

### PEEK, POKE

```
PEEK dirección
POKE dirección, valor
```

`PEEK` (una función) lee un byte de la memoria y `POKE` escribe uno.

- Las direcciones **por debajo de 65536** van a una **memoria de Spectrum
  virtual**, para que sigan funcionando los programas del Spectrum que hacen
  `PEEK` y `POKE` en la pantalla, en los colores o en las variables del
  sistema (ver [La memoria del Spectrum
  virtual](#la-memoria-del-spectrum-virtual)). Ahí todo se puede leer, y
  `POKE` en la ROM (por debajo de 16384) no hace nada.
- Las direcciones **de 65536 en adelante** leen y escriben la memoria real de
  ZX-V, byte a byte (por ejemplo, los [registros de la
  máquina](../machine.md)). `PEEK` lee la flash hasta el final de la ROM (de
  268435456 a 269221887) y la RAM (de 536870912 a 537333759); `POKE` solo
  escribe en esa RAM. Ojo: ahí también están el BASIC y tu programa, y un
  `POKE` equivocado puede colgar la máquina.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `dirección` | 0–65535 (el Spectrum virtual); 268435456–269221887 (solo `PEEK`) y 536870912–537333759 (la RAM) | El byte. |
| `valor` | −255–255 | Lo que escribe `POKE`. Un valor negativo se escribe como 256 más ese valor (−1 es 255). |

**Devuelve** (`PEEK`): el byte, de 0 a 255.

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `dirección` es negativa, mayor que 4294967295, o de 65536 en adelante fuera de las zonas de la tabla; `valor` fuera de −255 a 255. |

**Ejemplo**

```
PRINT PEEK 536872704
```

escribe el byte bajo del contador de frames de ZX-V (60 por segundo; el del
Spectrum, `FRAMES`, está en la 23672).

**Ver también:** [La memoria del Spectrum
virtual](#la-memoria-del-spectrum-virtual), [USR](#usr), [IN, OUT](#in-out).

### La memoria del Spectrum virtual

| Direcciones | Contenido |
|---|---|
| 0–16383 | la ROM del Spectrum: se lee como 0, salvo el juego de caracteres en 15616–16383; `POKE` no hace nada |
| 16384–22527 | los píxeles de la pantalla, ordenados como en el Spectrum (`POKE` dibuja con los colores de cada celda) |
| 22528–23295 | los colores de cada celda (`POKE` los cambia) |
| 23296–65535 | memoria normal: lo que escribes con `POKE` lo lees con `PEEK`, salvo en las variables del sistema (ver abajo) |
| 23552–23733 | variables del sistema (ver abajo) |
| 65368–65535 | los gráficos definidos por el usuario, de la A a la U |

En estas direcciones solo se ven los primeros 256 × 192 píxeles de la
pantalla y las primeras 32 × 24 celdas.

Variables del sistema que funcionan: `LAST K` (23560, la última tecla),
`CHARS` (23606, dónde está el juego de caracteres menos 256; puedes
apuntarla a una fuente tuya en la memoria virtual), `BORDCR` (23624),
`PROG` (23635, siempre 23755), `FLAGS2` (23658, el bit 3 es el bloqueo de
mayúsculas), `DF SZ` (23659, se puede poner de 0 a 2), `SEED` (23670, la
semilla de `RND`), `FRAMES` (23672, tres bytes que cuentan cincuentavos de
segundo, como los frames del Spectrum), `UDG` (23675), `COORDS` (23677 y
23678, el último punto de `PLOT`), `S POSN` (23688, solo leer), `SCR CT`
(23692), `ATTR P`, `MASK P`, `ATTR T`, `MASK T` y `P FLAG` (de 23693 a
23697) y `RAMTOP` (23730).

### Gráficos definidos por el usuario (UDG)

Los códigos de carácter del 144 al 164 son 21 caracteres, de la A a la U, que
puedes dibujar tú. `USR "a"` da la dirección del primer byte del carácter A (y
así hasta `USR "u"`); cada carácter son 8 bytes, uno por fila empezando por
arriba, y cada bit es un píxel (1 = tinta). Al principio, y tras `NEW`, se ven
como las letras de la A a la U. La variable del sistema `UDG` (23675) dice
dónde empiezan; puedes moverla a otro sitio de la memoria virtual.

```
10 FOR i=0 TO 7: READ b: POKE USR "a"+i, b: NEXT i
20 PRINT CHR$ 144; CHR$ 144; CHR$ 144
30 DATA BIN 00111100, BIN 01111110, BIN 11011011, BIN 11111111
40 DATA BIN 11111111, BIN 10111101, BIN 11000011, BIN 01111110
```

En el editor no se pueden teclear estos caracteres: usa `CHR$`.

### USR

```
USR dirección
USR "letra"
```

Una función. Con una dirección de 65536 o más, llama al código máquina
nativo RISC-V de esa dirección y devuelve el valor que da (en el registro
`a0`). El código máquina Z80 del Spectrum no se puede ejecutar: una
dirección por debajo de 65536 da `S Código Z80 no admitido`. Para llamar a
un código sin usar el resultado, `RANDOMIZE USR dirección` (que pone en la
semilla de `RND` lo que devuelva; si pasa de 65535, `B Entero fuera de
rango`).

Si el código nativo falla (una instrucción ilegal, una dirección prohibida),
el programa se para con `W Error en el código nativo`; ese informe no lo
atrapa `ON ERROR`, pero el programa BASIC se queda en la memoria.

De `USR "a"` a `USR "u"` dan la dirección de un [gráfico definido por el
usuario](#gráficos-definidos-por-el-usuario-udg).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `dirección` | 65536–4294967295 | Dónde empieza el código (en la RAM, normalmente cargado con `LOAD ... CODE`). |
| `"letra"` | de `"a"` a `"u"` (o en mayúsculas) | El gráfico definido por el usuario. |

**Devuelve:** lo que deja el código en `a0`, de 0 a 4294967295; o la
dirección del gráfico.

**Errores**

| Informe | Cuándo |
|---|---|
| `S Código Z80 no admitido` | `dirección` es menor que 65536. |
| `B Entero fuera de rango` | `dirección` es negativa o mayor que 4294967295. |
| `A Argumento no válido` | Una cadena que no es una sola letra de la `a` a la `u`. |
| `W Error en el código nativo` | El código ha fallado. |

**Ejemplo**

```
PRINT USR "a"
```

**Ver también:** [PEEK, POKE](#peek-poke),
[Gráficos definidos por el usuario](#gráficos-definidos-por-el-usuario-udg),
[Programas nativos](#programas-nativos).

### IN, OUT

```
IN puerto
OUT puerto, valor
```

Leen (`IN`, una función) y escriben (`OUT`) los puertos del Spectrum, tal
como los imita ZX-V:

| Puerto | `IN` | `OUT` |
|---|---|---|
| 254 (y cualquier puerto par) | las semifilas del teclado que elige el byte alto, como en el Spectrum (bits a 0 = pulsada; también los mandos que hacen de teclas) | bits 0–2 color del borde, bit 4 beeper |
| 31 (y cualquiera impar acabado en 31) | joystick Kempston (el mando 0): bits 0–4 derecha, izquierda, abajo, arriba, fuego | |
| 64479, 65503 | ratón Kempston: contadores de x e y (la y crece hacia arriba) | |
| 64223 | botones del ratón Kempston: bit 0 derecho, 1 izquierdo, 2 central (0 = pulsado) | |
| 65533 | lee el registro del AY elegido | elige un registro del AY (0–15); 255 elige el primer chip y 254 el segundo |
| 49149 | | escribe en el registro del AY elegido |

Los demás puertos leen 255 y no hacen caso de lo que se escribe. Para los
programas hechos para ZX-V es mejor `KEY`, `JOY`, `MOUSE`, `BORDER` y `SOUND`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `puerto` | 0–65535 | El puerto. |
| `valor` | −255–255 | Lo que se escribe (un negativo, como 256 más él). |

**Devuelve** (`IN`): el byte leído, de 0 a 255.

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `puerto` es negativo o mayor que 65535; `valor` fuera de −255 a 255. |

**Ejemplo**

```
10 PRINT AT 0,0; IN 65278; "   ": GO TO 10
```

**Ver también:** [KEY](#key), [JOY](#joy), [MOUSE](#mouse),
[BORDER](#border), [SOUND](#sound).

### TIME

```
TIME
```

Una función: el número de frames desde que se encendió la máquina, 60 por
segundo. Para medir un tiempo, resta dos lecturas.

**Parámetros:** ninguno.

**Devuelve:** de 0 a 4294967295 (vuelve a 0 al cabo de unos 828 días).

**Errores:** ninguno.

**Ejemplo**

```
10 LET t=TIME: PAUSE 100: PRINT (TIME-t)/60; " segundos"
```

**Ver también:** [PAUSE](#pause), [WAIT](#wait), [CLOCK](#clock).

### CLOCK

```
CLOCK n
CLOCK año, mes, día, hora, minutos [, segundos]
```

La función `CLOCK n` lee la fecha y la hora del reloj de la máquina. La
orden lo pone en hora. Los ficheros que se graban en la tarjeta llevan esa
fecha. Sin programa, también lo pone en hora el menú del editor: [Fecha y
hora](editor.md#la-fecha-y-la-hora).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–6 | Solo la función: 0 el año, 1 el mes, 2 el día, 3 la hora, 4 los minutos, 5 los segundos, 6 el día de la semana. |
| `año` | 2000–2099 | |
| `mes` | 1–12 | |
| `día` | 1–31 (los que tenga el mes) | |
| `hora` | 0–23 | |
| `minutos` | 0–59 | |
| `segundos` | 0–59 | Opcional; por defecto, 0. |

**Devuelve** (la función): el valor pedido (el día de la semana, de 1 lunes
a 7 domingo); 0 si el reloj no está en hora.

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | Una fecha o una hora que no existe (un año fuera de 2000–2099, el 30 de febrero, la hora 24...), o la máquina no tiene reloj. |
| `B Entero fuera de rango` | `n` es negativo o mayor que 6; un valor de la orden negativo o mayor que 65535. |

**Ejemplo**

```
CLOCK 2026,11,24,18,30
PRINT CLOCK 2; "/"; CLOCK 1; "/"; CLOCK 0; " "; CLOCK 3; ":"; CLOCK 4
```

**Ver también:** [TIME](#time).

### SPEED

```
SPEED n
```

`SPEED 1` (el de partida) ejecuta el BASIC a toda velocidad, unas 200 veces
más deprisa que un Spectrum. `SPEED 0` hace que cada orden tarde más o menos
lo que tardaría en un Spectrum 48K, para los programas antiguos que usan
bucles como pausa o que serían injugables a toda velocidad. Las órdenes
nuevas (sprites, mapas...) no se frenan.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0 o 1 | 0, a la velocidad del Spectrum; 1, a toda velocidad. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `n` está entre 2 y 255. |
| `B Entero fuera de rango` | `n` es negativo o mayor que 255. |

**Ejemplo**

```
10 SPEED 0: FOR i=1 TO 100: PRINT i: NEXT i: SPEED 1
```

**Ver también:** [PAUSE](#pause), [WAIT](#wait).

### LANGUAGE

```
LANGUAGE código$
```

Elige el idioma de la máquina con un código de dos letras: `"en"` inglés o
`"es"` español (en mayúsculas o no). Los informes de error y la barra y el
menú del editor lo usan desde ese momento, y la máquina lo recuerda (en el
hardware de verdad, en su reloj con pila; en el emulador, en sus ajustes).
Se admite cualquier otro código de dos letras, que saca los textos en inglés
(un programa puede leerlo para elegir los suyos: `PEEK 536872736` y
`PEEK 536872737` son los códigos de las dos letras, en minúsculas).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `código$` | dos letras de la `a` a la `z` | El idioma. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `código$` no son dos letras. |
| `C Sintaxis incorrecta` | `código$` no es una cadena. |

**Ejemplo**

```
LANGUAGE "es"
```

**Ver también:** [Mensajes de error](#mensajes-de-error).

### MONITOR

```
MONITOR
```

Abre el monitor de pruebas, una pantalla para comprobar el hardware. Tiene
una consola con las órdenes `CAT` (o `DIR`), `CD`, `TYPE` (enseña un fichero
de texto) y `LOAD` (ejecuta un programa nativo `.zxv`), y estas teclas: F1
cambia el modo de vídeo, F2 pone franjas en el borde, F3 desplaza la
pantalla, F4 toca un acorde en el AY, F6 un barrido en el beeper y F5 prueba
una excepción. No vuelve al BASIC: para salir, reinicia la máquina
(Ctrl+Alt+Supr). El programa BASIC se pierde.

**Parámetros:** ninguno. **Errores:** ninguno.

**Ejemplo**

```
MONITOR
```

**Ver también:** [Programas nativos](#programas-nativos).

## Órdenes de edición

Estas órdenes se usan sobre todo como órdenes directas, mientras escribes un
programa. En [el editor](editor.md) está todo lo demás. Las que cambian el
programa (`RENUM`, `DELETE`, `REPLACE`, `NEW`) se pueden deshacer con la
opción **Deshacer** del menú EDIT (la tecla Tab), como un cambio hecho a mano
(ver [Deshacer](editor.md#deshacer)).

### LIST

```
LIST [línea]
LIST #stream [, línea]
```

Como orden directa, `LIST` te lleva al editor con el cursor en esa línea (o
en la siguiente que haya), lista para cambiarla. Dentro de un programa, o con
un stream, escribe el listado desde esa línea: `LIST #2` en la pantalla del
programa, `LIST #3` en la impresora, `LIST #4` en un fichero. El listado sale
sangrado: lo que va dentro de un bucle, un procedimiento o un bloque, dos
espacios más a la derecha por nivel (los espacios no se guardan). BREAK (Esc)
lo para.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `línea` | 0–65535 | Desde dónde. Por defecto, desde el principio. |
| `stream` | 0–15 | Dónde se escribe. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `línea` mayor que 65535, o `stream` mayor que 15 (o negativos). |
| `O Canal no válido` | El stream no está abierto. |
| `J Dispositivo no válido` | El stream es un fichero abierto con `IN`. |
| `L BREAK en el programa` | Se pulsa Esc mientras lista. |

**Ejemplo**

```
LIST #2, 100
```

**Ver también:** [LLIST](#lprint-llist), [EDIT](#edit).

### EDIT

```
EDIT [línea]
```

Pone el cursor del editor en esa línea (o en la siguiente que haya); sin
número, en la línea actual. Dentro de un programa, lo para y abre el editor
en esa línea, sin informe. No hay que confundirla con la tecla EDIT (Tab),
que abre el menú de opciones del editor.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `línea` | 0–9999 | La línea. |

**Errores**

| Informe | Cuándo |
|---|---|
| `B Entero fuera de rango` | `línea` es negativa o mayor que 9999. |

**Ejemplo**

```
EDIT 100
```

**Ver también:** [LIST](#list), [FIND](#find).

### AUTO

```
AUTO [primera [, paso]]
```

Números de línea automáticos: cada línea nueva del editor empieza con el
número siguiente. Sin valores (o con `primera` 0), empieza en la última línea
del programa más `paso`; y cada línea, `paso` más que la anterior (10 si
no se da). Se acaba al pulsar Enter en
una línea que solo tiene el número, al pulsar Esc o al pasar a otra línea.
Solo como orden directa.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `primera` | 0–9999 | El número de la primera línea. Por defecto (o con 0), la última más `paso`. |
| `paso` | 1–9999 | De cuánto en cuánto. Por defecto, 10. |

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | Dentro de un programa. |
| `B Entero fuera de rango` | Un valor mayor que 9999 o negativo; `paso` 0; la primera línea pasaría de 9999. |

**Ejemplo**

```
AUTO 100, 5
```

**Ver también:** [RENUM](#renum).

### RENUM

```
RENUM [primera [, paso]]
```

Renumera todo el programa, y corrige los números de línea que van detrás de
`GO TO` (también el de `ON ERROR GO TO`), `GO SUB`, `RESTORE`, `RUN`, `LIST`,
`LLIST` y `LINE`. Solo se corrigen los números escritos tal cual al final de
una orden: en `GO TO n*10` la expresión no cambia. Un número que apunta a una
línea que no existe pasa a apuntar a la siguiente que haya (si no hay
ninguna, se queda como está). Las etiquetas no hace falta corregirlas. Solo
como orden directa. El menú de opciones del editor tiene lo mismo, y se
puede deshacer.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `primera` | 1–9999 | El número nuevo de la primera línea. Por defecto, 10. |
| `paso` | 1–9999 | De cuánto en cuánto. Por defecto, 10. |

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | Dentro de un programa. |
| `B Entero fuera de rango` | Un valor 0, negativo o mayor que 9999; los números pasarían de 9999. |
| `4 Sin memoria` | No hay memoria para rehacer el programa. |

**Ejemplo**

```
RENUM 100, 10
```

**Ver también:** [AUTO](#auto), [DELETE](#delete).

### DELETE

```
DELETE primera [, última]
```

Borra las líneas de `primera` a `última` (o solo la `primera`). Si no hay
ninguna en ese tramo, no hace nada. Solo como orden directa; se puede
deshacer.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `primera` | 0–9999 | La primera línea que se borra. |
| `última` | 0–9999 | La última. Por defecto, la misma que `primera`. |

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | Dentro de un programa. |
| `B Entero fuera de rango` | Un valor negativo o mayor que 9999. |

**Ejemplo**

```
DELETE 100, 190
```

**Ver también:** [RENUM](#renum), [NEW](#new).

### FIND

```
FIND [texto$]
```

Lleva el cursor del editor a la siguiente línea del programa donde aparece
`texto$`, desde la línea que va detrás de la del cursor y dando la vuelta al
final, con el cursor sobre el texto. No distingue mayúsculas de minúsculas, y
busca en el listado tal como se ve (palabras clave, textos entre comillas y
`REM` incluidos, pero no en los números de línea). Sin texto, busca lo mismo
que la última vez, como la tecla F3. Si no está, la línea de estado dice
`No encontrado`. Solo como orden directa.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `texto$` | una cadena | Opcional: lo que se busca. |

**Errores**

| Informe | Cuándo |
|---|---|
| `C Sintaxis incorrecta` | Dentro de un programa; `texto$` no es una cadena. |

**Ejemplo**

```
FIND "puntos"
```

**Ver también:** [REPLACE](#replace).

### REPLACE

```
REPLACE viejo$, nuevo$
```

Cambia el texto `viejo$` por `nuevo$` en todo el programa, sin distinguir
mayúsculas de minúsculas. Cada línea cambiada se comprueba como si se
tecleara, y las que quedarían con un error (o pasarían de 1024 caracteres)
no se tocan. La línea de estado dice cuántos cambios ha hecho y cuántas
líneas se quedaron sin cambiar. Solo como orden directa; se puede deshacer.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `viejo$` | una cadena no vacía | Lo que se busca. |
| `nuevo$` | una cadena (puede ser vacía) | Por lo que se cambia. |

**Errores**

| Informe | Cuándo |
|---|---|
| `A Argumento no válido` | `viejo$` está vacía. |
| `C Sintaxis incorrecta` | Dentro de un programa; un valor que no es una cadena. |

**Ejemplo**

```
REPLACE "vidas", "energia"
```

**Ver también:** [FIND](#find).

### NEW

```
NEW
```

Borra el programa, las variables y el almacén de gráficos (con la música,
los efectos y los sonidos cargados, que se callan), cierra los ficheros y
empieza de cero: los colores, el borde, el modo de vídeo, los UDG y los
streams vuelven a los de siempre, y se olvida la carpeta del programa. La
cinta puesta se queda.

Se puede deshacer: la opción **Deshacer** del menú EDIT (la tecla Tab) trae
de vuelta las líneas del programa (no sus variables, ni los gráficos). Para
eso el editor guarda una copia del programa en la memoria libre: si el
programa es tan grande que no cabe la copia, o si después algo usa esa
memoria, ya no se puede deshacer. Así que, mejor, graba antes.

**Parámetros:** ninguno. **Errores:** ninguno.

**Ejemplo**

```
NEW
```

**Ver también:** [CLEAR](#clear), [DELETE](#delete),
[Deshacer](editor.md#deshacer).

## Errores

### Informes

Cuando un programa se para, sale un informe abajo, en la pantalla del
programa:

```
2 Variable no encontrada, 20:1
```

Tiene un código (una cifra o una letra), el mensaje y dónde ha pasado: el
número de línea y la orden dentro de la línea (`20:1` es la primera orden de
la línea 20; la línea 0 es una orden directa). `0 OK` quiere decir que el
programa ha acabado bien. Pulsa una tecla para volver al editor (esa tecla
cuenta también para el editor).

`V Error de disco` añade el motivo entre paréntesis si se sabe, y `W Error
en el código nativo`, el número o el mensaje del programa nativo (ver
[Programas nativos](#programas-nativos)). Los informes salen en el idioma de
la máquina (ver [LANGUAGE](#language)). Un BREAK para también la música y
los efectos.

### ON ERROR

```
ON ERROR GO TO línea
ON ERROR GO TO @etiqueta
ON ERROR GO TO 0
```

A partir de ese momento, si un error para el programa, salta a esa línea o
etiqueta. Ahí, `ERR` y `ERL` dicen qué ha pasado. `ON ERROR GO TO 0` lo
quita, y también `RUN`, `CLEAR` y `NEW`.

- La trampa se quita sola al usarse, para que un error dentro del manejador no
  se repita sin fin: escribe otra vez `ON ERROR` para seguir atrapando
  errores.
- `CONTINUE` repite la orden que falló (tras un BREAK, sigue por la
  siguiente).
- Atrapa todos los errores de un programa, BREAK y `H STOP en INPUT`
  incluidos, salvo `9 Sentencia STOP`, los de las órdenes directas, el `W`
  de un código nativo que falla con `USR` y el informe de un programa nativo.
- Al saltar, se olvidan los `FN` en curso y, si el programa estaba
  escribiendo en la pantalla inferior, vuelve a la superior.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `línea` | 0–65535 | Adónde se salta (si no existe, a la siguiente que haya). 0 quita la trampa. |
| `@etiqueta` | una etiqueta del programa | Adónde se salta. |

**Errores**

| Informe | Cuándo |
|---|---|
| `X Etiqueta no encontrada` | No hay esa `@etiqueta` en el programa. |
| `B Entero fuera de rango` | `línea` negativa o mayor que 65535. |
| `C Sintaxis incorrecta` | Falta `GO TO`. |

**Ejemplo**

```
10 ON ERROR GO TO @fallo
20 PRINT 1/0
30 PRINT "sigo"
40 STOP
50 @fallo: PRINT "Error "; ERR; " en la linea "; ERL
60 ON ERROR GO TO @fallo: GO TO 30
```

**Ver también:** [ERR, ERL](#err-erl), [CONTINUE](#continue),
[Mensajes de error](#mensajes-de-error).

### ERR, ERL

```
ERR
ERL
```

Dos funciones. Tras un error atrapado con `ON ERROR`, `ERR` es el código del
error como número (las cifras son ellas mismas y las letras siguen contando
desde el 10: la `A` es el 10, la `B` el 11... la `Z` el 35; la [tabla de
abajo](#mensajes-de-error) los da todos) y `ERL`, la línea en la que pasó.

**Parámetros:** ninguno.

**Devuelve:** `ERR`, de 1 a 35; `ERL`, de 1 a 9999. Los dos se quedan con el
último error atrapado (aunque se vuelva a ejecutar el programa), y son 0 si
todavía no se ha atrapado ninguno.

**Errores:** ninguno.

**Ejemplo**

```
10 ON ERROR GO TO 30
20 PRINT SQR -1
30 PRINT ERR; " "; ERL
```

**Ver también:** [ON ERROR](#on-error).

## Mensajes de error

Los informes en español y en inglés, con el número que da `ERR`:

| Código | `ERR` | Español | Inglés | Cuándo sale |
|---|---|---|---|---|
| 0 | 0 | OK | OK | No es un error: el programa o la orden han acabado bien. |
| 1 | 1 | NEXT sin FOR | NEXT without FOR | `NEXT v`, y `v` es una variable normal, no la de un `FOR` (si no existe, `2`). |
| 2 | 2 | Variable no encontrada | Variable not found | Usar una variable antes de darle valor, o una matriz antes de su `DIM`; `NEXT` de una variable que no existe; `SAVE ... DATA` de una matriz que no existe. |
| 3 | 3 | Subíndice incorrecto | Subscript wrong | Un subíndice fuera del tamaño de la matriz, un número de subíndices que no es el suyo o un trozo fuera de la cadena; un `DIM` con un tamaño 0 o con más de 16 dimensiones. |
| 4 | 4 | Sin memoria | Out of memory | No hay memoria (para una variable, una línea, un fichero que se carga...); demasiados `GO SUB` (512) o bucles y procedimientos (64) uno dentro de otro; demasiado que guardar para `PROC` y `LOCAL` (8 KB); una expresión demasiado anidada; un `DIM` de más de 16777215 elementos; un `.zxv` que no cabe. |
| 5 | 5 | Fuera de la pantalla | Out of screen | `AT` fuera de la pantalla (en `PRINT` o en `INPUT`), o la pantalla inferior ya no puede crecer. |
| 6 | 6 | Número demasiado grande | Number too big | Un resultado mayor que 1,7E38; muchas veces, una división entre cero. También un número que no cabe en el formato del Spectrum al grabar un `.tap`. |
| 7 | 7 | RETURN sin GO SUB | RETURN without GOSUB | `RETURN` sin un `GO SUB` al que volver. |
| 8 | 8 | Fin del fichero | End of file | `INPUT #` después de la última línea de un fichero. |
| 9 | 9 | Sentencia STOP | STOP statement | El programa ha llegado a un `STOP`. `CONTINUE` sigue. |
| A | 10 | Argumento no válido | Invalid argument | Un valor que una función o una orden no admite: `SQR -1`, `LN 0`, `ASN 2`, un número negativo elevado a uno no entero; `KEY` de un código que no es una tecla; `USR` de una cadena que no es de `"a"` a `"u"`; un sprite, un mapa o una textura que no existe; una cadena de `PLAY` mal escrita; una fecha imposible (o sin reloj) en `CLOCK`; `SPEED` que no es 0 ni 1; `LANGUAGE` sin dos letras; `REPLACE` con el texto vacío; más de 64 vértices en `POLY`; un modo de `MODE` que no existe. |
| B | 11 | Entero fuera de rango | Integer out of range | Un número entero fuera de su rango: una coordenada fuera de la pantalla, `CHR$ 300`, `POKE` con 256 o en una dirección que no es de la RAM, un stream mayor que 15, `JOY 4`, `MOUSE 4`, `CLOCK 7`, `KEY 512`, una línea mayor que 9999 en `RENUM`, `AUTO` o `SAVE`; un bloque de más de 65535 bytes en un `.tap`. Un valor negativo o mayor que 255 donde se espera un byte da `B` aunque la orden luego compruebe otro rango con otro informe. |
| C | 12 | Sintaxis incorrecta | Nonsense in BASIC | La línea no es BASIC válido (una errata, un paréntesis que falta); un valor del tipo equivocado; un número de valores que no vale (`PATTERN` lleva 1, 2, 8 o 9; `POLY`, al menos un punto); `RENUM`, `DELETE`, `AUTO`, `FIND` o `REPLACE` dentro de un programa; una forma de `SAVE`, `LOAD`, `VERIFY` o `MERGE` que no se admite (un `.tzx` en `SAVE`, `PROTECT` sin `LINE`, `MERGE ... GRAPHICS`...); en `INPUT #`, una línea que no es un número. |
| D | 13 | BREAK - CONT repite | BREAK - CONT repeats | Has contestado N, Espacio o Esc a `scroll?`. `CONTINUE` repite. |
| E | 14 | No quedan DATA | Out of DATA | `READ` sin `DATA` por leer. |
| F | 15 | Nombre de fichero no válido | Invalid file name | Un nombre vacío, demasiado largo o con caracteres no permitidos; comodines en la carpeta de una ruta. |
| G | 16 | No cabe la línea | No room for line | La línea es demasiado larga (más de 2048 bytes una vez convertida), o la memoria está llena al guardarla; en un `.bas`, una línea de más de 1024 caracteres o que no cabe. |
| H | 17 | STOP en INPUT | STOP in INPUT | Se ha tecleado `STOP`, o pulsado Esc, en un `INPUT`. |
| I | 18 | FOR sin NEXT | FOR without NEXT | Un bucle `FOR` que no se ejecuta ninguna vez y no tiene un `NEXT` al que saltar. |
| J | 19 | Dispositivo no válido | Invalid I/O device | Escribir en un fichero abierto para leer, o leer de uno abierto para escribir; un canal desconocido en `OPEN #`. |
| K | 20 | Color no válido | Invalid colour | Un color fuera de rango (`INK 10`, `BORDER 8`, `BOX 16,...`). |
| L | 21 | BREAK en el programa | BREAK into program | Has pulsado BREAK (Esc). `CONTINUE` sigue por la orden siguiente. |
| M | 22 | RAMTOP no válido | RAMTOP no good | Una dirección de `CLEAR` demasiado baja o demasiado alta. |
| N | 23 | Sentencia perdida | Statement lost | Un salto de vuelta a una orden que ya no existe (con `CONTINUE`, `RETURN` o `NEXT`, tras cambiar la línea); `CONTINUE` tras una parada en una orden directa. |
| O | 24 | Canal no válido | Invalid stream | Un stream sin abrir; un fichero en los streams 0–3; más de 8 ficheros abiertos en una unidad; `FORMAT`. |
| P | 25 | FN sin DEF FN | FN without DEF | `FN` de una función sin `DEF FN` en el programa. |
| Q | 26 | Error en los parámetros | Parameter error | El número o el tipo de los valores de `FN` o de `PROC` no encajan con su `DEF`; más de 32 valores de `FN` en curso; más de 16 parámetros en `DEF PROC`, `PROC` o `LOCAL`; más de 8 cadenas en `PLAY`. |
| R | 27 | Error al cargar | Loading error | Un fichero que no es válido o no es del tipo pedido; `LOAD ""` sin cinta puesta o sin un bloque de ese tipo; `VERIFY` ha encontrado diferencias; `MERGE` de un programa protegido; un `.zxv` para una ROM más nueva. |
| S | 28 | Código Z80 no admitido | Z80 code not supported | `USR` con una dirección por debajo de 65536: código máquina del Spectrum. |
| T | 29 | Fichero no encontrado | File not found | No hay un fichero o una carpeta con ese nombre; nada encaja con los comodines de `ERASE`, `MOVE` o `COPY`; ni el fichero ni un bloque con ese nombre en la cinta puesta. |
| U | 30 | No hay tarjeta SD | No SD card | La ruta va a la tarjeta y no hay tarjeta en la ranura. |
| V | 31 | Error de disco | Disk error | La tarjeta ha fallado o la operación no se puede hacer; el motivo va detrás, entre paréntesis, si se sabe (ver abajo). |
| W | 32 | Error en el código nativo | Native code error | Ha fallado el código nativo llamado con `USR`, o un programa nativo (`.zxv`) ha acabado con un error. Puede llevar el código del programa o su propio mensaje (ver [Programas nativos](#programas-nativos)). |
| X | 33 | Etiqueta no encontrada | Label not found | `GO TO @nombre` (o `GO SUB`, `RUN`, `RESTORE`, `ON ERROR GO TO`) sin un `@nombre` en el programa. |
| Y | 34 | Error de estructura | Structure error | `WEND`, `UNTIL` o `END PROC` sin su principio; `LOCAL` fuera de un procedimiento; un bloque (`WHILE`, `IF`, `SELECT CASE`, `DEF PROC`...) sin su final. |
| Z | 35 | PROC no encontrado | PROC not found | `PROC` de un procedimiento sin `DEF PROC`. |

Los motivos de `V Error de disco`: `ya existe` / `already exists`, `la carpeta
no está vacía` / `folder not empty`, `es una carpeta` / `it is a folder`,
`no es una carpeta` / `not a folder`, `la tarjeta está llena` / `card full`,
`no se puede` / `not allowed`, `EXT: sin servidor` / `no server for EXT:`.
Sin motivo, la tarjeta ha fallado al leer o escribir.

## Juego de caracteres

Cada carácter tiene un código del 0 al 255 (`CODE` y `CHR$` pasan de uno a
otro):

| Códigos | Caracteres |
|---|---|
| 0–31 | códigos de control (ver abajo); los demás se escriben como `?` |
| 32–126 | letras, cifras y signos, como en ASCII, salvo el 96, que es `£` |
| 127 | `©` |
| 128–143 | gráficos de bloques: las 16 combinaciones de las cuatro cuartas partes de una celda |
| 144–164 | los [gráficos definidos por el usuario](#gráficos-definidos-por-el-usuario-udg), de la A a la U |
| 165–255 | las palabras clave: `PRINT CHR$ 245` escribe `PRINT` |

Los códigos de control que hacen algo al escribirlos:

| Código | Efecto |
|---|---|
| 6 | como una coma en `PRINT` |
| 8 | una columna hacia atrás |
| 13 | línea nueva |
| 16 a 21 | `INK`, `PAPER`, `FLASH`, `BRIGHT`, `INVERSE`, `OVER`: el carácter siguiente es el valor |
| 22 | `AT`: los dos caracteres siguientes son la fila y la columna |
| 23 | `TAB`: los dos caracteres siguientes son la columna (byte bajo y alto) |

Por ejemplo, `PRINT CHR$ 22; CHR$ 5; CHR$ 10; "X"` es lo mismo que
`PRINT AT 5,10; "X"`.

**Gráficos de bloques.** Los códigos del 128 al 143 dividen la celda del
carácter en cuatro cuartos. El código de un dibujo es 128 más los valores de
los cuartos que van en tinta: 1 arriba a la derecha, 2 arriba a la izquierda,
4 abajo a la derecha y 8 abajo a la izquierda. Por ejemplo, 128 + 2 + 8 = 138
es la mitad izquierda.

| Código | Carácter | Código | Carácter | Código | Carácter | Código | Carácter |
|---|---|---|---|---|---|---|---|
| 128 | (vacío) | 132 | ▗ | 136 | ▖ | 140 | ▄ |
| 129 | ▝ | 133 | ▐ | 137 | ▞ | 141 | ▟ |
| 130 | ▘ | 134 | ▚ | 138 | ▌ | 142 | ▙ |
| 131 | ▀ | 135 | ▜ | 139 | ▛ | 143 | █ |

Los cuartos van en el color de la tinta y el resto de la celda en el del
papel, como en cualquier otro carácter. Los códigos son los mismos que en el
Spectrum.

En el editor se pueden teclear los códigos del 32 al 127 (`£` y `©` con sus
teclas, si tu teclado las tiene; la tecla `` ` `` no escribe nada). Las
letras con tilde, la `ñ`, `¿` y `¡` **no** son caracteres del BASIC, así que
en los textos de tus programas hay que pasarse sin ellas. Si las tecleas, en
el editor o en un `INPUT`, las letras con tilde, diéresis o cedilla salen sin
ella (`á` sale `a`, `Ñ` sale `N`, `ç` sale `c`), `¿` y `¡` salen `?` y `!`, y
los demás caracteres no hacen nada. La ROM sí las usa en sus propios
mensajes, como los informes de error en español, pero tus programas no
pueden. Los gráficos de bloques y los definidos por el usuario se escriben
con `CHR$`.

## Límites

| Qué | Límite |
|---|---|
| Números de línea | del 1 al 9999 |
| Longitud de una línea en el editor | 1024 caracteres (2048 bytes una vez convertida) |
| Números | de unos ±2,9E-39 a ±1,7E38, con 8 cifras a la vista |
| Nombres de variables numéricas | hasta 63 letras y cifras |
| Variables de cadena, matrices, variables de `FOR`, `DEF FN` | una letra (26 de cada) |
| Cadenas | tan largas como quepa en la memoria (65535 caracteres en un `.tap`) |
| Dimensiones de una matriz | hasta 16, cada una hasta 65535, y hasta 16777215 elementos |
| `GO SUB` uno dentro de otro | 512 |
| `WHILE`, `REPEAT` y `PROC` uno dentro de otro | 64 |
| IF de bloque y `SELECT CASE` uno dentro de otro | sin límite |
| Parámetros de un procedimiento, y variables de un `LOCAL` | 16 |
| Lo que guardan `PROC` y `LOCAL` (los valores de antes de sus variables) | 8 KB |
| Valores de `FN` en curso (sumando las que van una dentro de otra) | 32 |
| Paréntesis anidados en una expresión | unos 170 (20 `FN` una dentro de otra) |
| Memoria para el programa y las variables | unos 94 KB (se ve en la barra del editor) |
| Ficheros abiertos | 8 por unidad (la tarjeta, `ROM:` y `EXT:`), en los streams 4 a 15 |
| Sprites | 256 (0–255), cada uno de hasta 1024 × 1024 píxeles |
| Tiles | 4096 de 8 × 8 y 4096 de 16 × 16 |
| Mapas de tiles | 16 (0–15), de hasta 32767 × 32767 celdas |
| Texturas | 4 (0–3) |
| Vértices de un polígono | 64 |
| `PLAY` | 8 cadenas (suenan 6), 4 niveles de paréntesis |
| Sonidos de `SAMPLE` | 32 (0–31) |
| Nombres de fichero | 63 caracteres; rutas de 255 |

## Índice de palabras clave

| Palabra clave | Tipo | Entrada |
|---|---|---|
| `@` | orden | [@etiqueta](#etiqueta) |
| `<=`, `>=`, `<>` | operador | [Expresiones y operadores](#expresiones-y-operadores) |
| `ABS` | función | [ABS, SGN, INT](#abs-sgn-int) |
| `ACS` | función | [ASN, ACS, ATN](#asn-acs-atn) |
| `AND` | operador | [Verdadero y falso](#verdadero-y-falso) |
| `ASN` | función | [ASN, ACS, ATN](#asn-acs-atn) |
| `AT` | en `PRINT`, `INPUT` | [PRINT](#print), [INPUT](#input) |
| `ATN` | función | [ASN, ACS, ATN](#asn-acs-atn) |
| `ATTR` | función | [SCREEN$, ATTR](#screen-attr) |
| `AUTO` | orden | [AUTO](#auto) |
| `BEEP` | orden | [BEEP](#beep) |
| `BIN` | número | [BIN](#bin) |
| `BORDER` | orden | [BORDER](#border) |
| `BOX` | orden | [BOX](#box) |
| `BRIGHT` | orden, elemento | [BRIGHT, FLASH](#bright-flash) |
| `CAT` | orden | [CAT](#cat) |
| `CASE` | en `SELECT CASE` | [SELECT CASE](#select-case) |
| `CD` | orden | [CD, DIR$](#cd-dir) |
| `CELL` | función | [CELL](#cell) |
| `CHR$` | función | [CHR$, CODE](#chr-code) |
| `CIRCLE` | orden | [CIRCLE](#circle) |
| `CLEAR` | orden | [CLEAR](#clear) |
| `CLIP` | orden | [CLIP](#clip) |
| `CLOCK` | función, orden | [CLOCK](#clock) |
| `CLOSE #` | orden | [OPEN, CLOSE](#open-close) |
| `CLS` | orden | [CLS](#cls) |
| `CODE` | función; en `SAVE`, `LOAD`, `VERIFY` | [CHR$, CODE](#chr-code), [SAVE](#save), [LOAD](#load) |
| `CONTINUE` | orden | [CONTINUE](#continue) |
| `COPY` | orden | [COPY](#copy) |
| `COS` | función | [SIN, COS, TAN](#sin-cos-tan) |
| `DATA` | orden; en `SAVE`, `LOAD`, `VERIFY` | [READ, DATA, RESTORE](#read-data-restore), [SAVE](#save), [LOAD](#load) |
| `DEF FN` | orden | [DEF FN, FN](#def-fn-fn) |
| `DEF PROC` | orden | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `DELETE` | orden | [DELETE](#delete) |
| `DIM` | orden | [DIM](#dim) |
| `DIR$` | función | [CD, DIR$](#cd-dir) |
| `DISC` | orden | [DISC](#disc) |
| `DRAW` | orden | [DRAW](#draw) |
| `EDIT` | orden | [EDIT](#edit) |
| `ELSE` | en `IF`, `CASE ELSE` | [IF, THEN, ELSE](#if-then-else), [SELECT CASE](#select-case) |
| `END IF` | orden | [IF, THEN, ELSE](#if-then-else) |
| `END PROC` | orden | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `END SELECT` | orden | [SELECT CASE](#select-case) |
| `ERASE` | orden | [ERASE](#erase) |
| `ERL` | función | [ERR, ERL](#err-erl) |
| `ERR` | función | [ERR, ERL](#err-erl) |
| `EXP` | función | [EXP, LN](#exp-ln) |
| `FIND` | orden | [FIND](#find) |
| `FLASH` | orden, elemento | [BRIGHT, FLASH](#bright-flash) |
| `FLOOR` | orden | [FLOOR](#floor) |
| `FN` | función | [DEF FN, FN](#def-fn-fn) |
| `FOR` | orden | [FOR, NEXT](#for-next) |
| `FORMAT` | orden | [FORMAT](#format) |
| `GO SUB` | orden | [GO SUB, RETURN](#go-sub-return) |
| `GO TO` | orden; en `ON ERROR` | [GO TO](#go-to), [ON ERROR](#on-error) |
| `GRAB` | orden | [GRAB](#grab) |
| `GRAPHICS` | en `SAVE`, `LOAD`, `VERIFY` | [SAVE](#save), [LOAD](#load), [VERIFY](#verify) |
| `HIT` | función | [HIT](#hit) |
| `IF` | orden | [IF, THEN, ELSE](#if-then-else) |
| `IN` | función; en `OPEN #` | [IN, OUT](#in-out), [OPEN, CLOSE](#open-close) |
| `INK` | orden, elemento | [INK, PAPER](#ink-paper) |
| `INKEY$` | función | [INKEY$](#inkey) |
| `INPUT` | orden | [INPUT](#input) |
| `INT` | función | [ABS, SGN, INT](#abs-sgn-int) |
| `INVERSE` | orden, elemento | [INVERSE, OVER](#inverse-over) |
| `JOY` | función, orden | [JOY](#joy) |
| `KEY` | función | [KEY](#key) |
| `LANGUAGE` | orden | [LANGUAGE](#language) |
| `LEN` | función | [LEN](#len) |
| `LET` | orden | [LET](#let) |
| `LINE` | en `INPUT`, `SAVE` | [INPUT](#input), [SAVE](#save) |
| `LIST` | orden | [LIST](#list) |
| `LLIST` | orden | [LPRINT, LLIST](#lprint-llist) |
| `LN` | función | [EXP, LN](#exp-ln) |
| `LOAD` | orden | [LOAD](#load) |
| `LOCAL` | orden | [LOCAL](#local) |
| `LPRINT` | orden | [LPRINT, LLIST](#lprint-llist) |
| `MAP` | orden; en `FLOOR` y `ROTATE` | [MAP](#map), [MAP DIM, MAP POKE](#map-dim-map-poke), [FLOOR](#floor), [ROTATE](#rotate) |
| `MERGE` | orden | [MERGE](#merge) |
| `MKDIR` | orden | [MKDIR, RMDIR](#mkdir-rmdir) |
| `MODE` | orden | [MODE](#mode) |
| `MONITOR` | orden | [MONITOR](#monitor) |
| `MOUSE` | función, orden | [MOUSE](#mouse) |
| `MOVE` | orden | [MOVE](#move) |
| `MUSIC` | función, orden; en `LOAD` | [MUSIC](#music), [LOAD](#load) |
| `NEW` | orden | [NEW](#new) |
| `NEXT` | orden | [FOR, NEXT](#for-next) |
| `NOT` | operador | [Verdadero y falso](#verdadero-y-falso) |
| `ON ERROR` | orden | [ON ERROR](#on-error) |
| `OPEN #` | orden | [OPEN, CLOSE](#open-close) |
| `OR` | operador | [Verdadero y falso](#verdadero-y-falso) |
| `OUT` | orden; en `OPEN #` | [IN, OUT](#in-out), [OPEN, CLOSE](#open-close) |
| `OVER` | orden, elemento | [INVERSE, OVER](#inverse-over) |
| `PAGE` | orden | [PAGE, SCREEN](#page-screen) |
| `PALETTE` | orden | [PALETTE](#palette) |
| `PAPER` | orden, elemento | [INK, PAPER](#ink-paper) |
| `PATTERN` | orden | [PATTERN](#pattern) |
| `PAUSE` | orden | [PAUSE](#pause) |
| `PEEK` | función | [PEEK, POKE](#peek-poke) |
| `PI` | función | [PI](#pi) |
| `PIXEL` | función | [PIXEL](#pixel) |
| `PLAY` | orden; en `MUSIC` | [PLAY](#play), [MUSIC](#music) |
| `PLOT` | orden | [PLOT](#plot) |
| `POINT` | función | [POINT](#point) |
| `POKE` | orden; en `MAP` | [PEEK, POKE](#peek-poke), [MAP DIM, MAP POKE](#map-dim-map-poke) |
| `POLY` | orden | [POLY](#poly) |
| `PRINT` | orden | [PRINT](#print) |
| `PROC` | orden | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `PROTECT` | en `SAVE` | [Programas protegidos](#programas-protegidos) |
| `RANDOMIZE` | orden | [RND, RANDOMIZE](#rnd-randomize) |
| `RASTER` | orden | [RASTER](#raster) |
| `READ` | orden | [READ, DATA, RESTORE](#read-data-restore) |
| `REM` | orden | [REM](#rem) |
| `REMAP` | orden; en `RASTER` | [REMAP](#remap), [RASTER](#raster) |
| `RENUM` | orden | [RENUM](#renum) |
| `REPEAT` | orden | [REPEAT, UNTIL](#repeat-until) |
| `REPLACE` | orden | [REPLACE](#replace) |
| `RESTORE` | orden | [READ, DATA, RESTORE](#read-data-restore) |
| `RETURN` | orden | [GO SUB, RETURN](#go-sub-return) |
| `RMDIR` | orden | [MKDIR, RMDIR](#mkdir-rmdir) |
| `RND` | función | [RND, RANDOMIZE](#rnd-randomize) |
| `ROTATE` | orden | [ROTATE](#rotate) |
| `RUN` | orden | [RUN](#run) |
| `SAMPLE` | función, orden; en `LOAD` | [SAMPLE](#sample), [LOAD](#load) |
| `SAVE` | orden | [SAVE](#save) |
| `SCREEN` | orden | [PAGE, SCREEN](#page-screen) |
| `SCREEN$` | función; en `SAVE`, `LOAD`, `VERIFY` | [SCREEN$, ATTR](#screen-attr), [SAVE](#save), [LOAD](#load) |
| `SCROLL` | orden; en `RASTER` | [SCROLL](#scroll), [RASTER](#raster) |
| `SELECT CASE` | orden | [SELECT CASE](#select-case) |
| `SFX` | función, orden; en `LOAD` | [SFX](#sfx), [LOAD](#load) |
| `SGN` | función | [ABS, SGN, INT](#abs-sgn-int) |
| `SIN` | función | [SIN, COS, TAN](#sin-cos-tan) |
| `SOUND` | orden | [SOUND](#sound) |
| `SPEED` | orden | [SPEED](#speed) |
| `SPRITE` | orden | [SPRITE](#sprite) |
| `SQR` | función | [SQR](#sqr) |
| `STEP` | en `FOR` | [FOR, NEXT](#for-next) |
| `STOP` | orden | [STOP](#stop) |
| `STR$` | función | [STR$, VAL, VAL$](#str-val-val) |
| `TAB` | en `PRINT`, `INPUT` | [PRINT](#print), [INPUT](#input) |
| `TAN` | función | [SIN, COS, TAN](#sin-cos-tan) |
| `TEXTURE` | orden | [TEXTURE](#texture) |
| `THEN` | en `IF` | [IF, THEN, ELSE](#if-then-else) |
| `TILE` | orden | [TILE](#tile) |
| `TIME` | función | [TIME](#time) |
| `TO` | en `FOR`, trozos, `CASE`, `MOVE`, `COPY` | [FOR, NEXT](#for-next), [Cadenas](#cadenas), [SELECT CASE](#select-case), [MOVE](#move), [COPY](#copy) |
| `TRACE` | orden | [TRACE](#trace) |
| `UNTIL` | orden | [REPEAT, UNTIL](#repeat-until) |
| `USR` | función | [USR](#usr) |
| `VAL` | función | [STR$, VAL, VAL$](#str-val-val) |
| `VAL$` | función | [STR$, VAL, VAL$](#str-val-val) |
| `VERIFY` | orden | [VERIFY](#verify) |
| `WAIT` | orden | [WAIT](#wait) |
| `WEND` | orden | [WHILE, WEND](#while-wend) |
| `WHILE` | orden | [WHILE, WEND](#while-wend) |
