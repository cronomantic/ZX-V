[English](../../en/basic/reference.md) · **Español**

# Referencia del BASIC

Esta página describe todas las órdenes, funciones y operadores del BASIC de
ZX-V, además de los mensajes de error, el juego de caracteres y los límites
de la máquina. Es una referencia: para aprender el lenguaje paso a paso,
empieza por el [curso de BASIC](README.md), y para aprender a teclear y
cambiar programas, lee [el editor](editor.md).

## Cómo leer esta referencia

Cada entrada empieza por su **sintaxis**, en un recuadro:

```
PRINT [AT fila, columna;] [elemento] [; elemento]...
```

- Las palabras en MAYÚSCULAS son palabras clave: se teclean tal cual (en
  mayúsculas o en minúsculas, da igual).
- Las palabras en minúsculas son lo que pones tú: `n`, `x`, `fila` son
  números o expresiones numéricas; los nombres que acaban en `$` (`a$`,
  `nombre$`) son cadenas de texto; `línea` es un número de línea y `var`, el
  nombre de una variable.
- `[` `]` encierran una parte opcional; `...` quiere decir que lo anterior se
  puede repetir; `|` separa alternativas (`IN | OUT` es «IN u OUT»).
- Los signos como `,`, `;`, `(` y `#` se teclean tal como aparecen.

Después viene lo que hace, sus parámetros y sus rangos, un ejemplo corto que
puedes teclear y las entradas relacionadas (**Ver también**). Cuando un valor
se sale de su rango, la máquina da un informe de error, que aquí se escribe
con su código y su mensaje, por ejemplo `B Entero fuera de rango` (ver
[Mensajes de error](#mensajes-de-error)).

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
  matrices, operadores
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
  `SCREEN`, `SCROLL`, `REMAP`, `RASTER`
- [Sprites, mapas de tiles y gráficos rápidos](#sprites-mapas-de-tiles-y-gráficos-rápidos):
  `GRAB`, `SPRITE`, `TILE`, `MAP`, `FLOOR`, `BOX`, `DISC`, `POLY`...
- [Sonido y música](#sonido-y-música): `BEEP`, `PLAY`, `SOUND`
- [Teclado, mando y ratón](#teclado-mando-y-ratón): `INPUT`, `INKEY$`,
  `KEY`, `JOY`, `MOUSE`
- [Ficheros y la tarjeta SD](#ficheros-y-la-tarjeta-sd): `SAVE`, `LOAD`,
  `CAT`, `CD`, `COPY`, `ERASE`...
- [Ficheros de cinta](#ficheros-de-cinta): `.tap` y `.tzx`
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

Cuando algo falla, el informe de error dice dónde: `2 Variable no
encontrada, 20:1` es la orden 1 de la línea 20 (ver [Informes](#informes)).

### Números

Los números se escriben como `42`, `-7`, `3.25`, `.5` (con punto, no con
coma) o en notación científica: `1E6` es 1 000 000 y `2.5E-3` es 0,0025. Van
de unos ±2,9E-39 a ±1,7E38; un resultado mayor da `6 Número demasiado
grande`, y uno más pequeño se queda en 0. [`BIN`](#bin) escribe un número en
binario.

`PRINT` enseña hasta 8 cifras significativas, y pasa a la notación científica
con los números muy grandes o muy pequeños:

```
PRINT 1/3, 2^31, 1E10, 0.000001
```

enseña `0.33333333`, `2.1474836E+9`, `1E+10` y `1E-6`.

Las comparaciones (`=`, `<`, `>`...) se hacen con la misma precisión que en
el Spectrum, así que `0.1*3=0.3` es cierto.

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

Un trozo que empieza después de donde acaba es la cadena vacía; una posición
fuera de la cadena da `3 Subíndice incorrecto`. Los trozos valen con
variables de cadena, con cadenas escritas en el programa y con cualquier
expresión entre paréntesis: `(a$+b$)(2 TO)`. También se puede asignar a un
trozo: `LET a$(1)="J"` cambia el primer carácter (sin cambiar la longitud
de `a$`).

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
- La variable de control de [`FOR`](#for-next) es una sola letra.
- Un nombre no puede ser una palabra clave: `LET print=1` es un error. Una
  palabra clave dentro de un nombre más largo no molesta (`total`, `ink2`).

Una variable se crea la primera vez que recibe un valor (con `LET`, `INPUT`,
`READ` o `FOR`). Leer una variable que no existe da `2 Variable no
encontrada`. [`RUN`](#run) y [`CLEAR`](#clear) borran todas las variables.

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
variable de cadena `a$`.

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

enseña `hola|    |`.

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
`-2^2` es -4. Los paréntesis cambian el orden: `(1+2)*3` es 9.

Las funciones toman solo el valor que las sigue: `SQR 16+9` es `(SQR 16)+9`
= 13, y `SQR (16+9)` es 5. Si dudas, pon paréntesis.

Dividir entre cero da `6 Número demasiado grande`. Elevar un número negativo
a una potencia da `A Argumento no válido`.

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
cercano (2,5 pasa a 3). Si queda fuera del rango permitido, da `B Entero
fuera de rango` (o `K Color no válido` con los colores).

## Control del programa

### GO TO

```
GO TO línea
GO TO @etiqueta
```

Salta a una línea y sigue desde ahí. `línea` puede ser cualquier expresión
(`GO TO n*100`). Si esa línea no existe, va a la siguiente que haya; si no hay
ninguna, el programa termina. Con una [etiqueta](#etiqueta), salta a esa orden
exacta.

```
10 PRINT "sin parar"
20 GO TO 10
```

Pulsa BREAK (Esc) para pararlo.

**Ver también:** [GO SUB](#go-sub-return), [@etiqueta](#etiqueta),
[RUN](#run).

### GO SUB, RETURN

```
GO SUB línea
GO SUB @etiqueta
RETURN
```

`GO SUB` salta a una subrutina, como `GO TO`, pero recuerda de dónde venía;
`RETURN` vuelve a la orden que sigue al `GO SUB`. Una subrutina puede llamar a
otras (hasta 512 niveles). Un `RETURN` sin `GO SUB` da `7 RETURN sin GO SUB`.

```
10 GO SUB 100: GO SUB 100
20 STOP
100 PRINT "en la subrutina"
110 RETURN
```

**Ver también:** [DEF PROC](#def-proc-proc-end-proc), que suele quedar más
claro.

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
`ELSE`, si hay uno en la misma línea, o por la línea siguiente. Un número
tras `THEN` es un error (`IF a THEN 100`; se escribe `IF a THEN GO TO 100`).

```
10 LET n=-5
20 IF n<0 THEN PRINT "negativo" ELSE PRINT "cero o positivo"
30 IF n<0 THEN PRINT "a": PRINT "b" ELSE PRINT "c": PRINT "d"
```

La línea 30 escribe `a` y `b`. Todo lo que va de `THEN` a `ELSE` es la parte
de cierto, y todo lo que va de `ELSE` al final de la línea, la de falso. Con
varios `IF` en una línea, cada `ELSE` va con el `IF` más cercano.

**El IF de bloque.** Si `THEN` es lo último de la línea, el `IF` abre un
bloque de varias líneas, que acaba en `END IF`. Dentro puede haber partes
`ELSE IF condición THEN` (también al final de su línea), que se miran por
orden si no se cumplió nada antes, y una parte `ELSE` para cuando no se
cumple ninguna. Solo se ejecuta una parte; después, el programa sigue tras
`END IF`.

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

- `ELSE`, `ELSE IF` y `END IF` empiezan su línea. Detrás de `ELSE` pueden ir
  órdenes en la misma línea (`ELSE PRINT "Adulto"`), y detrás de `END IF`,
  otras separadas por `:`. Un `ELSE` que no empieza la línea es el del IF de
  una línea.
- Los bloques se pueden meter unos dentro de otros, sin límite, y llevar
  dentro IF de una línea, bucles o `GO SUB`. Se puede salir de un bloque con
  `GO TO`.
- Si a un bloque le falta el `END IF`, `Y Error de estructura` cuando se
  busca. Solo funciona en un programa: como orden directa, `IF a THEN` no
  hace nada.

**Ver también:** [SELECT CASE](#select-case), [WHILE](#while-wend),
[Expresiones](#expresiones-y-operadores).

### SELECT CASE

```
SELECT CASE valor
CASE valores
  líneas
CASE ELSE
  líneas
END SELECT
```

Elige una parte según `valor`, un número o una cadena: se ejecuta la del
primer `CASE` que le encaja (o la de `CASE ELSE` si no encaja ninguno), y el
programa sigue tras `END SELECT`. Si no encaja ninguno y no hay `CASE ELSE`,
no se ejecuta nada. Cada `CASE` lleva una lista de valores separados por
comas, que pueden ser:

| Valor | Encaja si |
|---|---|
| `5`, `"si"`, `a+1` | `valor` es igual |
| `1 TO 9`, `"a" TO "m"` | `valor` está entre los dos (ambos incluidos) |
| `< 0`, `>= 100`, `<> 7` | `valor` cumple la comparación |

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

Los `CASE` pueden ir en líneas propias o detrás de `:`, y las órdenes de cada
parte, en la misma línea o en las siguientes; todo puede ir incluso en una
sola línea (`SELECT CASE n: CASE 1: PRINT "uno": CASE ELSE: PRINT "otro":
END SELECT`). Los `SELECT CASE` se pueden meter unos dentro de otros. Si el
valor y los de un `CASE` no son del mismo tipo, `C Sintaxis incorrecta`; si
falta el `END SELECT`, `Y Error de estructura`.

**Ver también:** [IF](#if-then-else).

### FOR, NEXT

```
FOR var = primero TO último [STEP paso]
NEXT var
```

Repite las órdenes que hay entre `FOR` y `NEXT`, con la variable yendo de
`primero` a `último` y sumando `paso` cada vez (1 si no hay `STEP`). La
variable es una sola letra. Con un paso negativo cuenta hacia atrás. Si el
bucle no tiene que ejecutarse ninguna vez (`FOR i=1 TO 0`), el programa salta
tras el `NEXT` que le corresponde; si no lo hay, `I FOR sin NEXT`.

```
10 FOR i=1 TO 10 STEP 3: PRINT i;" ";: NEXT i
20 PRINT
30 FOR i=3 TO 1 STEP -1: PRINT i: NEXT i
```

Al acabar el bucle, la variable se queda con el primer valor que se pasa del
final (aquí, 0). Un `NEXT` con una variable que no es de un `FOR` da `1 NEXT
sin FOR`.

**Ver también:** [WHILE](#while-wend), [REPEAT](#repeat-until).

### WHILE, WEND

```
WHILE condición
WEND
```

Repite las órdenes que hay entre `WHILE` y `WEND` mientras la condición sea
cierta. La condición se mira al principio, así que si es falsa la primera
vez, el bucle se salta entero.

```
10 LET n=1
20 WHILE n<100: LET n=n*2: WEND
30 PRINT n
```

enseña 128. Los bucles se pueden anidar. Un `WEND` sin su `WHILE`, o un
`WHILE` sin su `WEND`, da `Y Error de estructura`.

**Ver también:** [REPEAT](#repeat-until), [FOR](#for-next).

### REPEAT, UNTIL

```
REPEAT
UNTIL condición
```

Repite las órdenes que hay entre `REPEAT` y `UNTIL` hasta que la condición
sea cierta. La condición se mira al final, así que el bucle se ejecuta al
menos una vez.

```
10 REPEAT
20 LET d=INT (RND*6)+1: PRINT d
30 UNTIL d=6
```

**Ver también:** [WHILE](#while-wend).

### DEF PROC, PROC, END PROC

```
DEF PROC nombre [(parámetro [, parámetro]...)]
END PROC
PROC nombre [(valor [, valor]...)]
```

Un **procedimiento** es un trozo de programa con nombre. `DEF PROC` lo empieza
y `END PROC` lo acaba; `PROC` lo ejecuta y luego vuelve. Los parámetros
reciben los valores que se dan en `PROC` y son locales: al llegar a
`END PROC` recuperan el valor que tenían antes. Un parámetro numérico es un
nombre de variable numérica; uno de cadena, una letra con `$`. Un
procedimiento puede llamarse a sí mismo.

Cuando el programa llega a un `DEF PROC` siguiendo su curso normal, se salta
el procedimiento entero, así que los procedimientos pueden ir en cualquier
sitio.

```
10 PROC saluda("Ana", 2)
20 PROC saluda("Luis", 1)
30 STOP
100 DEF PROC saluda(n$, veces)
110 FOR i=1 TO veces: PRINT "Hola, "; n$: NEXT i
120 END PROC
```

Los nombres siguen las reglas de las variables numéricas (letras y cifras,
sin distinguir mayúsculas ni espacios). Hasta 16 parámetros. Errores:
`Z PROC no encontrado` (no hay un `DEF PROC` con ese nombre), `Q Error en los
parámetros` (no coinciden el número o el tipo de los valores) e `Y Error de
estructura` (`END PROC` fuera de un procedimiento, o un `DEF PROC` sin su
`END PROC`).

Un procedimiento no devuelve un valor: para dar un resultado, guárdalo en una
variable.

**Ver también:** [LOCAL](#local), [GO SUB](#go-sub-return).

### LOCAL

```
LOCAL var [, var]...
```

Dentro de un procedimiento, hace locales esas variables: empiezan valiendo 0
(o `""`) y recuperan su valor anterior en `END PROC`. Fuera de un
procedimiento, `Y Error de estructura`.

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

Una orden formada por `@` y un nombre marca un sitio del programa. `GO TO`,
`GO SUB`, `RUN`, `RESTORE` y `ON ERROR GO TO` pueden saltar a ella por su
nombre en vez de por su número de línea, a la orden exacta (no hace falta que
sea la primera de su línea). Las etiquetas no cambian al renumerar. Una
etiqueta que no existe da `X Etiqueta no encontrada`.

```
10 GO SUB @hola: GO SUB @hola
20 STOP
30 @hola: PRINT "hola": RETURN
```

**Ver también:** [GO TO](#go-to), [RENUM](#renum).

### RUN

```
RUN [línea | @etiqueta]
```

Ejecuta el programa desde el principio, o desde esa línea o etiqueta. Antes de
empezar lo limpia todo como [`CLEAR`](#clear): borra las variables y la
pantalla, cierra los ficheros abiertos y quita el `ON ERROR`. Para empezar
sin perder las variables, usa `GO TO`.

**Ver también:** [CONTINUE](#continue), [STOP](#stop).

### STOP

```
STOP
```

Para el programa con el informe `9 Sentencia STOP`. [`CONTINUE`](#continue)
sigue por la orden siguiente. `ON ERROR` no atrapa `STOP`.

### CONTINUE

```
CONTINUE
```

Sigue después de una parada: tras `STOP` o BREAK, por la orden siguiente;
tras un error, repite la orden que falló (útil cuando ya has arreglado la
causa, por ejemplo dando valor a una variable que faltaba).

### TRACE

```
TRACE ON
TRACE #stream
TRACE OFF
```

Para seguir por dónde va un programa: cada vez que empieza a ejecutarse una
línea (también al volver a ella con un `GO TO`, un `NEXT` o un `RETURN`), se
escribe su número entre corchetes, como `[10][20][30]`. Con `TRACE ON` sale en
la pantalla del programa, mezclado con lo que escribe; con `TRACE #3`, en la
impresora (el fichero `printer.txt`), o en el fichero de un stream abierto con
`OPEN #`, para no estropear la pantalla. `TRACE OFF` lo apaga; también se puede
poner dentro del programa, para seguir solo una parte. `NEW` lo apaga. Con un
stream que no está abierto, `O Canal no válido`.

```
TRACE ON: RUN
```

### REM

```
REM cualquier texto
```

Un comentario: todo lo que va detrás de `REM` hasta el final de la línea se
ignora.

```
10 REM Juego del espacio, por Ana
```

### PAUSE

```
PAUSE n
```

Espera `n` frames (hay 50 por segundo), o hasta que se pulse una tecla, lo
que llegue antes. `PAUSE 0` espera una tecla sin límite. Solo cuentan las
teclas que se pulsan después de empezar el `PAUSE`. `n` va de 0 a 65535.

```
10 PRINT "Pulsa una tecla": PAUSE 0: PRINT "Gracias"
```

**Ver también:** [WAIT](#wait), [INKEY$](#inkey).

### WAIT

```
WAIT [n]
```

Espera al frame siguiente, y hasta que hayan pasado `n` frames (1 si no se
pone) desde el `WAIT` anterior. Así el bucle de un juego va a un ritmo fijo
aunque unas vueltas tarden más que otras: `WAIT 2` da 25 vueltas por segundo.
`n` va de 0 a 65535.

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
(`a=1` sin más es un error). El valor tiene que ser del tipo de la variable:
un número para las numéricas y una cadena para las de `$`.

```
10 LET a=5: LET a=a+1: LET b$="ZX"
20 LET b$(2)="-": PRINT a; b$
```

**Ver también:** [Variables](#variables), [DIM](#dim).

### DIM

```
DIM letra(tamaño [, tamaño]...)
DIM letra$(tamaño [, tamaño]...)
```

Crea una matriz (ver [Matrices](#matrices)), y sustituye a la que tuviera el
mismo nombre. Hasta 16 dimensiones; cada tamaño va de 1 a 65535. Las matrices
numéricas empiezan llenas de ceros y las de cadenas, de espacios. Un
subíndice fuera de rango da `3 Subíndice incorrecto`.

```
10 DIM s(10): LET s(3)=42: PRINT s(3), s(4)
```

### READ, DATA, RESTORE

```
DATA valor [, valor]...
READ var [, var]...
RESTORE [línea | @etiqueta]
```

Las líneas `DATA` guardan una lista de valores dentro del programa; `READ` los
va cogiendo uno a uno, en orden, y los guarda en variables. Los valores pueden
ser cualquier expresión del tipo adecuado. `RESTORE` hace que el siguiente
`READ` vuelva a empezar por el primer `DATA` (o por el primero que haya desde
esa línea o etiqueta). Leer después del último valor da `E No quedan DATA`.

```
10 FOR i=1 TO 3
20 READ n$, edad: PRINT n$; " tiene "; edad
30 NEXT i
40 DATA "Ana", 12, "Luis", 9, "Eva", 10+1
```

`DATA` no hace nada cuando el programa pasa por él.

### CLEAR

```
CLEAR [dirección]
```

Borra todas las variables y la pantalla, cierra los ficheros abiertos con
`OPEN #`, olvida las vueltas pendientes de `GO SUB` y de los bucles, quita
`ON ERROR` y `CLIP`, y hace que `READ` empiece por el primer `DATA`. Los
sprites y demás gráficos se conservan. [`RUN`](#run) hace primero un
`CLEAR`.

Con una dirección, además mueve un límite de la memoria (solo para programas
avanzados): por debajo de 65536 pone el «RAMTOP» de la [memoria del Spectrum
virtual](#la-memoria-del-spectrum-virtual) (como poco 23821; si no,
`M RAMTOP no válido`); de 65536 en adelante es una dirección real, y el
almacén de gráficos se coloca justo debajo, de modo que la memoria de encima
queda libre para lo que quieras.

### DEF FN, FN

```
DEF FN letra([parámetro [, parámetro]...]) = expresión
DEF FN letra$([parámetro [, parámetro]...]) = expresión de cadena
FN letra(valores)
```

Define una función propia, con un nombre de una letra y parámetros de una
letra (`x`, o `a$` para las cadenas). `FN` la usa. `DEF FN` puede ir en
cualquier sitio del programa y no hace nada cuando se llega a él. Usar una
función que no está definida da `P FN sin DEF FN`; dar un número o un tipo de
valores que no encajan, `Q Error en los parámetros`.

```
10 DEF FN h(x,y)=SQR (x*x+y*y)
20 DEF FN u$(a$)=a$+"!"
30 PRINT FN h(3,4); " "; FN u$("hola")
```

enseña `5 hola!`.

## Matemáticas

### SIN, COS, TAN

```
SIN x     COS x     TAN x
```

Seno, coseno y tangente de un ángulo `x` en **radianes** (una vuelta entera
es `2*PI`). Para usar grados, multiplica por `PI/180`.

```
PRINT SIN (PI/2); " "; COS (60*PI/180)
```

### ASN, ACS, ATN

```
ASN x     ACS x     ATN x
```

Arcoseno, arcocoseno y arcotangente: el ángulo, en radianes, cuyo seno,
coseno o tangente es `x`. `ASN` y `ACS` necesitan una `x` entre -1 y 1 (si
no, `A Argumento no válido`).

```
PRINT 4*ATN 1
```

### EXP, LN

```
EXP x     LN x
```

`EXP x` es e (2,7182818...) elevado a `x`. `LN x` es el logaritmo neperiano;
`x` tiene que ser mayor que 0 (si no, `A Argumento no válido`). Para el
logaritmo en base 10, `LN x/LN 10`.

### SQR

```
SQR x
```

Raíz cuadrada. `x` no puede ser negativo (`A Argumento no válido`).

```
PRINT SQR 2
```

### ABS, SGN, INT

```
ABS x     SGN x     INT x
```

- `ABS x`: `x` sin su signo (`ABS -3` es 3).
- `SGN x`: -1, 0 o 1 según `x` sea negativo, cero o positivo.
- `INT x`: el entero igual o inferior a `x`: `INT 2.7` es 2 e `INT -2.5` es
  -3. Para redondear al más cercano, `INT (x+0.5)`.

### PI

```
PI
```

El número π, 3.1415927.

### RND, RANDOMIZE

```
RND
RANDOMIZE [n]
```

`RND` da un número al azar desde 0 hasta 1 (sin llegar a 1). Para un entero
del 1 al 6, `INT (RND*6)+1`.

`RANDOMIZE n` (de 0 a 65535) empieza la secuencia en un punto fijo, así que
vuelven a salir los mismos números «al azar». `RANDOMIZE` o `RANDOMIZE 0` la
empiezan según el tiempo que lleva encendida la máquina.

```
10 RANDOMIZE
20 FOR i=1 TO 5: PRINT INT (RND*6)+1;" ";: NEXT i
```

> **Si conoces el Spectrum:** `RND` da exactamente la misma secuencia que en
> el 48K.

### BIN

```
BIN cifras
```

Escribe un número en binario: `BIN 1010` es 10. Viene bien para diseñar
gráficos con [`POKE USR`](#gráficos-definidos-por-el-usuario-udg).

## Funciones de texto

### LEN

```
LEN a$
```

El número de caracteres de una cadena. `LEN ""` es 0.

```
PRINT LEN "ZX-V"
```

### CHR$, CODE

```
CHR$ n     CODE a$
```

`CHR$ n` es el carácter de código `n` (de 0 a 255) y `CODE a$`, el código del
primer carácter de `a$` (0 si está vacía). Ver el [Juego de
caracteres](#juego-de-caracteres).

```
PRINT CHR$ 65; " "; CODE "a"
```

enseña `A 97`.

### STR$, VAL, VAL$

```
STR$ x     VAL a$     VAL$ a$
```

- `STR$ x` convierte un número en una cadena, tal como lo enseñaría `PRINT`:
  `STR$ 3.5` es `"3.5"`.
- `VAL a$` calcula la cadena como una expresión numérica: `VAL "2*3"` es 6.
  Si el texto no es una expresión válida, `C Sintaxis incorrecta`.
- `VAL$ a$` calcula la cadena como una expresión de cadena: `VAL$ """x"""` es
  `"x"`.

```
10 LET a$="12": LET b$="30"
20 PRINT a$+b$; " "; VAL a$+VAL b$
```

enseña `1230 42`.

## La pantalla y PRINT

La pantalla del programa tiene una **parte superior**, donde trabajan `PRINT`
y las órdenes de dibujo, y una **parte inferior** de dos líneas, abajo del
todo, que usan `INPUT` y los informes. En el modo normal, el 0, la pantalla
tiene 24 filas de 32 caracteres (256 × 192 píxeles), y `PRINT` puede usar
las filas 0 a 21; en el modo 1, 48 filas de 64 caracteres (ver
[MODE](#mode)).

Cuando lo que se escribe pasa de la última fila, la pantalla sube. Cada vez
que ha subido una pantalla entera, se para y pregunta `scroll?`: pulsa N,
Espacio o BREAK para parar el programa (`D BREAK - CONT repite`), o
cualquier otra tecla para seguir.

### PRINT

```
PRINT [elemento] [separador] [elemento]...
```

Escribe números y textos en la pantalla. Los elementos pueden ser:

| Elemento | Efecto |
|---|---|
| una expresión numérica o de cadena | escribe su valor |
| `AT fila, columna` | mueve la posición de escritura (la fila 0 es la de arriba) |
| `TAB columna` | avanza hasta esa columna, rellenando con espacios |
| `INK n`, `PAPER n`, `FLASH n`, `BRIGHT n`, `INVERSE n`, `OVER n` | cambia los colores solo para el resto de este `PRINT` (ver [Colores](#colores)) |
| `#stream` | escribe por ese stream (ver [Streams](#streams-y-canales)) |

Y entre ellos, los separadores:

| Separador | Efecto |
|---|---|
| `;` | nada: lo siguiente va justo detrás |
| `,` | pasa a la siguiente mitad de la línea (a la columna 0 o 16; en el modo 1, cada 16 columnas) |
| `'` | pasa a la línea siguiente |

Si el `PRINT` no acaba en un separador, al final pasa a la línea siguiente.
`PRINT` sin nada escribe una línea vacía.

```
10 PRINT "Nombre", "Puntos"
20 PRINT "Ana", 120
30 PRINT AT 10,5; "en la fila 10"; TAB 25; "tab"
40 PRINT "uno"'"dos"; " y tres"
50 PRINT INK 2; "rojo"; INK 0; " negro"
```

`AT` fuera de la pantalla da `5 Fuera de la pantalla`. `TAB` da la vuelta
según el ancho de la línea.

**Ver también:** [LPRINT](#lprint-llist), [INPUT](#input),
[Colores](#colores).

### CLS

```
CLS
```

Borra la pantalla con el color de `PAPER` actual y lleva la posición de
escritura a la esquina superior izquierda.

### SCREEN$, ATTR

```
SCREEN$ (fila, columna)
ATTR (fila, columna)
```

`SCREEN$` da el carácter escrito en esa posición (`""` si no lo reconoce, por
ejemplo un dibujo). `ATTR` da los colores de esa celda en un solo número:
`FLASH`×128 + `BRIGHT`×64 + `PAPER`×8 + `INK`. Las posiciones fuera de la
pantalla dan `B Entero fuera de rango`.

```
10 PRINT AT 0,0; INK 2; PAPER 6; "A"
20 PRINT SCREEN$ (0,0); " "; ATTR (0,0)
```

enseña `A 50` (6×8 + 2).

**Ver también:** [POINT](#point), [PIXEL](#pixel).

### LPRINT, LLIST

```
LPRINT [elementos]
LLIST [línea]
```

Como `PRINT` y `LIST`, pero a la **impresora**. ZX-V no tiene impresora de
papel: la impresora es el fichero de texto `printer.txt` de la raíz de la
tarjeta SD, y todo lo que se imprime se añade al final. Se ignoran los
colores y `AT` (salvo su columna). Las líneas tienen hasta 64 caracteres (32
en el modo 0).

```
LPRINT "Lista de la compra": LPRINT "- leche"
```

**Ver también:** [COPY](#copy), [Streams](#streams-y-canales).

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
sprites se usa como «transparente».

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

```
10 PAPER 1: INK 6: CLS
20 PRINT "amarillo sobre azul"
30 PRINT PAPER 2; INK 7; "blanco sobre rojo, solo esta vez"
40 PRINT "otra vez amarillo sobre azul"
```

### INK, PAPER

```
INK n     PAPER n
```

El color de la tinta y del papel, del 0 al 7 (ver la tabla), o:

- **8**, transparente: conserva el color que ya tiene cada celda;
- **9**, contraste: blanco o negro, el que mejor se vea sobre el otro color.

Otros valores dan `K Color no válido`. `PAPER` llega a toda la pantalla con el
siguiente `CLS`.

### BORDER

```
BORDER n
```

El color del borde que rodea la pantalla, del 0 al 7. La parte inferior de la
pantalla (las dos líneas de `INPUT` y de los informes) usa el color del borde
como papel.

```
BORDER 0: PAPER 0: INK 4: CLS
```

**Ver también:** [RASTER BORDER](#raster), para hacer franjas de colores.

### BRIGHT, FLASH

```
BRIGHT n     FLASH n
```

`BRIGHT 1` usa la versión brillante de la tinta y del papel; `BRIGHT 0`, la
normal. `FLASH 1` hace que la celda intercambie su tinta y su papel cada
tercio de segundo, más o menos. El 8 es transparente (conserva lo que tenía
la celda). Otros valores dan `K Color no válido`.

```
PRINT FLASH 1; "FIN DE LA PARTIDA"
```

### INVERSE, OVER

```
INVERSE n     OVER n
```

`INVERSE 1` escribe con la tinta y el papel intercambiados. `OVER 1` escribe
encima de lo que ya hay, cambiando cada píxel del carácter entre la tinta y
el papel (si escribes dos veces lo mismo con `OVER 1`, se borra). El 0 los
quita; otros valores dan `K Color no válido`.

```
10 PRINT AT 5,5; "X": PRINT AT 5,5; OVER 1; "O"
```

## Dibujo con PLOT y DRAW

Estas órdenes dibujan en la pantalla del programa con el color de la tinta.
Sus coordenadas son píxeles, con el **(0, 0) en la esquina inferior
izquierda**: `x` va del 0 al 255 hacia la derecha e `y` del 0 al 175 hacia
arriba (las dos filas de abajo son de la parte inferior de la pantalla). En
el modo 1, `x` llega a 511 e `y` a 367. Un punto fuera de la pantalla da
`B Entero fuera de rango`.

Al principio se pueden poner elementos de color, como en `PLOT INK 2; 10,10`.

> Las [órdenes de gráficos rápidos](#sprites-mapas-de-tiles-y-gráficos-rápidos)
> (`BOX`, `SPRITE`...) cuentan la `y` desde arriba, como `PRINT AT`.

### PLOT

```
PLOT [colores;] x, y
```

Dibuja un punto, que será el de partida del siguiente `DRAW`.

```
10 FOR x=0 TO 255 STEP 4: PLOT x, 88+40*SIN (x/20): NEXT x
```

### DRAW

```
DRAW [colores;] dx, dy [, ángulo]
```

Dibuja una línea desde el último punto hasta el que está `dx` píxeles a la
derecha y `dy` píxeles más arriba (los valores negativos van a la izquierda y
hacia abajo). Con `ángulo` (en radianes) dibuja un arco, que gira eso por el
camino: `PI` dibuja media circunferencia, y un ángulo positivo curva en el
sentido contrario a las agujas del reloj.

```
10 PLOT 50,50: DRAW 100,0: DRAW 0,100: DRAW -100,-100
20 PLOT 150,60: DRAW 60,0,PI
```

### CIRCLE

```
CIRCLE [colores;] x, y, radio
```

Dibuja una circunferencia con el centro en `x`, `y`.

```
10 FOR r=10 TO 80 STEP 10: CIRCLE INK r/10; 128,88,r: NEXT r
```

### POINT

```
POINT (x, y)
```

1 si el píxel de `x`, `y` está pintado con la tinta, 0 si es papel. Ve lo que
dibujan `PRINT`, `PLOT`, `DRAW` y `CIRCLE`, pero no las órdenes de gráficos
rápidos (para esas, [`PIXEL`](#pixel)).

```
10 PLOT 10,10: PRINT POINT (10,10); POINT (11,10)
```

## Modos de vídeo y efectos

### MODE

```
MODE n
MODE 0, ancho, alto
```

Elige el modo de la pantalla y la borra:

| Modo | Píxeles | Texto |
|---|---|---|
| 0 | 256 × 192 | 32 columnas × 24 filas (el del arranque) |
| 1 | 512 × 384 | 64 columnas × 48 filas |

`MODE` borra las dos páginas, enseña la 0 y quita el scroll, `REMAP`,
`RASTER` y `CLIP`. En el modo 0, la página puede ser además más grande que la
pantalla, que es entonces una ventana que se mueve por ella con
[`SCROLL`](#scroll): `MODE 0, 512, 384`, `MODE 0, 1024, 192` o
`MODE 0, 256, 768`. Así, `PRINT AT`, `PLOT` y las demás cubren toda la página
(`PRINT AT` llega a la columna 127 en una página de 1024 píxeles de ancho).
Otros tamaños dan `A Argumento no válido`.

```
10 MODE 1: CIRCLE 256,184,150: PRINT AT 24,26; "Modo 1"
```

### PAGE, SCREEN

```
PAGE n
SCREEN n
```

ZX-V tiene dos páginas de pantalla, la 0 y la 1. `PAGE` elige la que se ve;
`SCREEN`, aquella en la que dibujan `PRINT`, `CLS`, `PLOT`, los sprites y las
demás. Dibujar en la página oculta y luego enseñarla da animaciones suaves,
sin parpadeo (el doble buffer). Los valores que no son 0 ni 1 dan
`A Argumento no válido`.

```
10 LET s=1
20 SCREEN s: CLS: CIRCLE 128,88,RND*80
30 PAGE s: LET s=1-s: WAIT 10: GO TO 20
```

Los informes de error e `INPUT` salen siempre en la página que se ve. El
editor también usa la página 1: cuando un programa vuelve a usarla, se
recuperan el texto y los dibujos de `PLOT` y `DRAW`, pero se pierden los
gráficos rápidos (sprites, mapas...) que hubiera en ella.

### SCROLL

```
SCROLL x, y
SCROLL
```

Desplaza la imagen sin volver a dibujarla: el píxel (`x`, `y`) de la página
se ve en la esquina superior izquierda de la pantalla, y la página da la
vuelta por los bordes. `SCROLL` sin nada la deja como estaba. Así es como un
juego recorre una página grande de [`MODE 0`](#mode).

```
10 MODE 0,1024,192: CIRCLE 600,100,50
20 FOR x=0 TO 1023 STEP 2: SCROLL x,0: WAIT: NEXT x
```

### REMAP

```
REMAP color, nuevo [; color, nuevo]...
REMAP #tabla; color, nuevo [; color, nuevo]...
REMAP #tabla
REMAP
```

Hace que un color se vea como otro en toda la pantalla, al instante y sin
volver a dibujar nada: tras `REMAP 1,10`, todo lo azul se ve rojo brillante.
Los colores van del 0 al 15 (si no, `K Color no válido`). Sirve para
fundidos, destellos y ciclos de color.

Hay tres tablas de cambio, de la 0 a la 2. Sin `#`, `REMAP` cambia la tabla
0, que es la que se usa normalmente. `REMAP #t; ...` cambia la tabla `t` sin
enseñarla; `REMAP #t` hace que toda la pantalla use la tabla `t` (y así todos
sus cambios aparecen de golpe); [`RASTER REMAP`](#raster) usa una tabla solo
en algunas líneas. `REMAP` sin nada deja las tres tablas como al principio y
vuelve a la 0.

```
10 PRINT INK 1; "parpadeo"
20 REMAP 1,10: PAUSE 25: REMAP 1,1: PAUSE 25: GO TO 20
```

### RASTER

```
RASTER BORDER y1, y2, color
RASTER REMAP y1, y2, tabla
RASTER SCROLL y1, y2, x [, y]
RASTER
```

Cambia cosas línea a línea en la pantalla, sin gastar nada de tiempo del
procesador mientras el programa sigue:

- `RASTER BORDER`: las líneas de `y1` a `y2` tienen el borde de ese color
  (del 0 al 15).
- `RASTER REMAP`: esas líneas usan la tabla de cambio de color 0 a 2 (ver
  [REMAP](#remap)).
- `RASTER SCROLL`: esas líneas tienen su propio scroll horizontal `x` y, si se
  da `y`, enseñan las líneas de la página a partir de la `y` (para pantallas
  partidas con scroll independiente, y para el *parallax*).
- `RASTER` sin nada lo quita todo.

Las líneas son filas de píxeles contadas desde arriba de la pantalla (la 0).
Las del borde de arriba son negativas (hasta -48 en el modo 0 y -96 en el
modo 1) y las de abajo pasan del alto (hasta 239 en el modo 0 y 479 en el
modo 1). `MODE` lo quita.

```
10 FOR y=-48 TO 239 STEP 4: RASTER BORDER y,y+1,2: RASTER BORDER y+2,y+3,6: NEXT y
```

## Sprites, mapas de tiles y gráficos rápidos

Estas órdenes usan el motor gráfico de la ROM para dibujar juegos deprisa:
sprites con transparencia, mapas de tiles con scroll, un suelo en perspectiva
(el «modo 7») y figuras rellenas. Se diferencian de `PLOT` y `DRAW` en
algunas cosas:

- Las **coordenadas** son píxeles desde la esquina **superior izquierda** de
  la página, con la `y` hacia abajo, como `PRINT AT`. Pueden ser negativas o
  salirse de la página: lo que cae fuera simplemente no se dibuja.
- Los **colores** son los 16 colores por su número, del 0 al 15 (ver la
  [tabla de colores](#colores)); un número mayor que 15 da `K Color no
  válido`. En los sprites y los tiles, el color 8 es transparente.
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
También puedes crear ficheros `.gfx` a partir de imágenes PNG en el
ordenador, con la herramienta `png2gfx` del SDK (ver la [guía del
SDK](../sdk/README.md)).

Un sprite, juego de tiles, mapa o textura que no existe da `A Argumento no
válido`; los números fuera de rango, `B Entero fuera de rango`, y si los
gráficos no caben en la memoria, `4 Sin memoria`.

### GRAB

```
GRAB n, x, y, ancho, alto [, t]
GRAB n, ancho, a$
```

Define el sprite `n` (del 0 al 255).

- La primera forma copia un rectángulo de la página de dibujo, con su esquina
  superior izquierda en `x`, `y` (hasta 1024 × 1024 píxeles). Si se da `t`,
  ese color pasa a ser transparente.
- La segunda toma los píxeles de una cadena, un carácter por píxel, fila a
  fila y `ancho` píxeles por fila: las cifras hexadecimales `0`–`9` y `a`–`f`
  son los colores del 0 al 15, y cualquier otro carácter (`.`, un espacio...)
  es transparente.

```
10 GRAB 0,8,"..2222..22222222.222222...2222..."
20 FOR x=0 TO 240 STEP 16: SPRITE 0,x,80: NEXT x
```

### SPRITE

```
SPRITE n, x, y [, espejo [, c$ [, ancho, alto]]]
```

Dibuja el sprite `n` con su esquina superior izquierda en `x`, `y`. Sus
píxeles transparentes dejan ver el fondo.

- `espejo`: 0 normal, 1 de izquierda a derecha, 2 boca abajo, 3 las dos cosas.
- `c$` cambia los colores: el color `i` se dibuja con el color que dice el
  carácter `i` de la cadena (contando desde 0), una cifra hexadecimal. Un
  carácter que no es una cifra hexadecimal (o una cadena demasiado corta)
  deja ese color como está: `"0a"` dibuja el color 1 como el 10 y deja los
  demás. `""` no cambia nada.
- `ancho`, `alto` (de 1 a 4096): lo dibuja estirado o encogido a ese tamaño,
  para las cosas que se acercan o se alejan.

```
10 GRAB 0,4,"1111122112211111"
20 SPRITE 0,10,10: SPRITE 0,30,10,0,"0a": SPRITE 0,50,10,0,"",32,32
```

**Ver también:** [HIT](#hit), [GRAB](#grab).

### HIT

```
HIT(a, ax, ay, b, bx, by)
```

Comprueba un choque: 1 si algún píxel sólido (no transparente) del sprite
`a` dibujado en `ax`, `ay` caería sobre un píxel sólido del sprite `b`
dibujado en `bx`, `by`; si no, 0. No dibuja nada.

```
10 GRAB 0,4,"1111111111111111"
20 PRINT HIT(0,0,0,0,3,3); HIT(0,0,0,0,4,0)
```

enseña `10`.

### TILE

```
TILE lado, n, x, y [, ancho, alto [, t]]
TILE lado, n, a$
```

Define tiles para los mapas. Hay dos juegos de tiles, uno de 8 × 8 píxeles y
otro de 16 × 16 (`lado` es 8 o 16; si no, `A Argumento no válido`), cada uno
con tiles del 0 al 4095.

- La primera forma copia el tile `n` de la página de dibujo en `x`, `y`. Con
  `ancho` y `alto`, corta ese rectángulo en tiles, numerados desde `n` fila a
  fila. El color `t`, si se da, pasa a ser transparente.
- La segunda toma los píxeles de una cadena, como [`GRAB`](#grab) (lado × lado
  caracteres por tile; una cadena más larga define los tiles siguientes).

Un juego de tiles crece al definir un tile más alto; los nuevos empiezan
negros.

```
10 BOX 2,0,0,8,8: POLY 7,0,3,7,3: TILE 8,1,0,0
```

### MAP DIM, MAP POKE

```
MAP DIM m, ancho, alto [, lado [, repetir]]
MAP POKE m, x, y, valor
MAP POKE m, x, y, a$
```

`MAP DIM` crea el mapa de tiles `m` (del 0 al 15) con `ancho` × `alto`
celdas, todas con el tile 0. `lado` es el tamaño de los tiles, 8 (el de por
defecto) o 16. Con `repetir` = 1, el mapa se repite sin fin en las dos
direcciones.

`MAP POKE` pone la celda de la columna `x` y la fila `y` (contando desde 0).
El valor es el número del tile, más lo que haga falta de esto:

| Suma | Efecto |
|---|---|
| 4096 | tile al revés de izquierda a derecha |
| 8192 | tile boca abajo |
| 16384 | el color 8 del tile es transparente (deja ver lo de detrás) |
| 32768 | prioridad (ver el filtro de [MAP](#map)) |

Con una cadena, llena las celdas desde `x` hacia la derecha usando el código
de cada carácter como número de tile: `"  ##.."` usa los tiles 32, 35 y 46.
Así es fácil dibujar los niveles con texto.

**Ver también:** [MAP](#map), [CELL](#cell).

### MAP

```
MAP m [, mx, my [, x, y, ancho, alto [, filtro]]]
```

Dibuja el mapa `m` a partir de su píxel `mx`, `my` (0, 0 si no se ponen), en
toda la página o en el rectángulo `x`, `y`, `ancho`, `alto`. Cambiando `mx` y
`my`, el mapa se desplaza. `filtro`: 0 dibuja todas las celdas, 1 solo las que
no tienen prioridad y 2 solo las que la tienen (así se puede dibujar el
fondo, después los sprites y por último las partes del escenario que van
delante). Tienen que existir todos los tiles que usa el mapa (si no,
`A Argumento no válido`).

```
10 BOX 5,0,0,8,8: TILE 8,32,0,0
20 BOX 2,0,0,8,8: POLY 7,0,3,7,3: TILE 8,35,0,0
30 MAP DIM 0,40,1: MAP POKE 0,0,0,"####  ####   ######  #########   ####"
40 FOR x=0 TO 64: MAP 0,x,0,0,100,256,8: WAIT: NEXT x
```

### CELL

```
CELL(m, x, y)
```

El valor de la celda de la columna `x` y la fila `y` del mapa `m`, con los
bits de espejo, transparencia y prioridad. Fuera de un mapa que no se repite,
0. Sirve para saber qué hay en el escenario en un sitio: la celda que hay
bajo un sprite en el píxel `px`, `py` es `CELL(m, INT (px/8), INT (py/8))` con
tiles de 8 píxeles.

### TEXTURE

```
TEXTURE n, x, y, lado [, c]
```

Define la textura `n` (de la 0 a la 3) para [`FLOOR`](#floor): el cuadrado de
`lado` × `lado` píxeles (una potencia de 2 de 2 a 256: 2, 4, 8... 256; otro
tamaño da `A Invalid argument`) de la página de dibujo con su esquina
superior izquierda en `x`, `y`. La textura se repite sin fin; si se da el
color `c`, todo lo que queda fuera de ella es de ese color.

### FLOOR

```
FLOOR n, x, y, altura, ángulo, horizonte [, focal]
```

Dibuja un suelo en perspectiva con la textura `n`, visto desde el punto `x`,
`y` de la textura a esa `altura`, mirando en la dirección `ángulo` (en
radianes; 0 mira hacia donde crece `x`). El horizonte está en la línea
`horizonte` de la página, y el suelo ocupa desde la línea de debajo hasta el
final de la página. `focal` es la distancia focal (media página si no se
pone): con valores más pequeños se ve más ancho. Es el efecto «modo 7» de
los juegos de carreras y de vuelo.

```
10 FOR i=0 TO 7: BOX i,i*8,0,8,64: NEXT i: TEXTURE 0,0,0,64
20 FOR a=0 TO 2*PI STEP .05: FLOOR 0,32,32,20,a,80: WAIT: NEXT a
```

### POLY

```
POLY color, x1, y1 [, x2, y2]...
```

Con un punto, dibuja un píxel; con dos, una línea; con tres o más (hasta 64),
un polígono relleno.

```
POLY 4, 128,20, 200,150, 56,150
```

### BOX

```
BOX color, x, y, ancho, alto
```

Un rectángulo relleno con su esquina superior izquierda en `x`, `y`.

```
BOX 2, 10,10, 100,50
```

### DISC

```
DISC color, x, y, radio
```

Un círculo relleno con el centro en `x`, `y`.

```
DISC 6, 128,96, 40
```

### CLIP

```
CLIP x, y, ancho, alto
CLIP
```

A partir de ese momento, las órdenes de esta sección solo dibujan dentro de
ese rectángulo. `CLIP` sin nada quita el límite (también lo quitan `MODE`,
`RUN` y `CLEAR`).

```
10 CLIP 50,50,100,50: DISC 2,100,75,60: CLIP
```

### PIXEL

```
PIXEL(x, y)
```

El color (del 0 al 15) del píxel de `x`, `y` de la página de dibujo, contando
desde arriba a la izquierda, lo haya dibujado quien lo haya dibujado; -1
fuera de la página. No tiene en cuenta `REMAP`. Sirve para los choques con
cualquier cosa dibujada en la pantalla.

```
10 BOX 4,10,10,20,20: PRINT PIXEL(15,15); " "; PIXEL(5,5); " "; PIXEL(-1,0)
```

## Sonido y música

ZX-V tiene un *beeper* (un altavoz sencillo, el que usa `BEEP`) y dos chips
de sonido AY de tres canales cada uno (los que usan `PLAY` y `SOUND`).

### BEEP

```
BEEP duración, tono
```

Toca una nota con el beeper. `duración` va en segundos (de 0 a 10); `tono`,
en semitonos por encima del do central (0 es el do central, 12 el do de
arriba y -12 el de abajo), de -60 a 69, y puede llevar decimales. El programa
espera a que acabe la nota.

```
10 FOR n=0 TO 12: BEEP 0.1,n: NEXT n
```

### PLAY

```
PLAY a$ [, b$]...
```

Toca música escrita como texto, con hasta seis voces a la vez: las tres
primeras cadenas suenan en los tres canales del primer chip AY y las tres
siguientes, en los del segundo. El programa espera a que acaben todas;
BREAK las corta. Una cadena mal escrita da `A Argumento no válido`, y un
número fuera de rango, `B Entero fuera de rango`; se comprueban todas antes
de que empiece a sonar nada.

Dentro de cada cadena:

| Texto | Significado |
|---|---|
| `c d e f g a b` | una nota (do, re, mi, fa, sol, la, si); en mayúsculas, `C`...`B`, una octava más arriba |
| `#`, `$` | sostenido y bemol (delante de la nota): `#f`, `$b` |
| `&` | un silencio |
| `1` a `9` | la duración de las notas siguientes: 1 semicorchea, 2 semicorchea con puntillo, 3 corchea, 4 corchea con puntillo, 5 negra (la de partida), 6 negra con puntillo, 7 blanca, 8 blanca con puntillo, 9 redonda |
| `10` `11` `12` | tresillos de semicorcheas, corcheas y negras: valen para las tres notas siguientes |
| `n_m` | duraciones ligadas: la nota siguiente dura las dos |
| `O n` | octava, de 0 a 8 (por defecto 5, en la que `c` es el do central) |
| `T n` | tempo en negras por minuto, de 60 a 240 (por defecto 120); solo en la primera cadena |
| `V n` | volumen, de 0 a 15 (por defecto 15) |
| `U` | usar la envolvente de volumen |
| `W n` | forma de la envolvente, de 0 a 7 |
| `X n` | periodo de la envolvente, de 0 a 65535 |
| `M n` | mezclador: los bits 0–2 encienden el tono de los canales A, B y C, y los bits 3–5 su ruido |
| `N` | separador (no hace nada) |
| `(` `)` | repite una vez lo que hay dentro (hasta 4 niveles); un `)` sin su `(` repite la cadena entera sin fin (hasta BREAK) |
| `!...!` | un comentario |
| `H` | para todas las voces |
| `Y n`, `Z n` | MIDI en el 128K: se ignoran |

Los espacios se ignoran. La envolvente y el mezclador son de cada chip, así
que los comparten sus tres voces.

```
10 PLAY "T140 O4 5cdec 5cdec 5efg&"
20 PLAY "O4 3cegC", "O3 9c", "O5 V10 3(gfed)"
```

### SOUND

```
SOUND registro, valor [; registro, valor]...
```

Escribe directamente en los registros de los chips AY, para hacer efectos de
sonido. Los registros del 0 al 15 son los del primer chip y del 16 al 31, los
del segundo (con el mismo significado, 16 más arriba); los valores van de 0 a
255. Un registro mayor que 31 da `B Entero fuera de rango`.

| Registro | Uso |
|---|---|
| 0–1, 2–3, 4–5 | periodo del tono de los canales A, B y C (byte bajo, byte alto de 0 a 15); la frecuencia es 110 840 / periodo Hz |
| 6 | periodo del ruido, de 0 a 31 |
| 7 | mezclador: un bit a **0** enciende: bits 0–2 tono de A, B y C; bits 3–5 ruido de A, B y C |
| 8, 9, 10 | volumen de A, B y C, de 0 a 15; 16 usa la envolvente |
| 11–12 | periodo de la envolvente (bajo, alto) |
| 13 | forma de la envolvente (escribirlo la vuelve a empezar) |

```
10 SOUND 7,62; 8,15; 0,200; 1,0
20 PAUSE 25: SOUND 8,0
```

toca un tono en el canal A durante medio segundo.

**Ver también:** [`IN` y `OUT`](#in-out), para los puertos del 128K.

## Teclado, mando y ratón

### INPUT

```
INPUT [elementos de aviso] var [, var]...
INPUT ... LINE a$
```

Para el programa y deja que el usuario teclee los valores de unas variables,
en la parte inferior de la pantalla. Los elementos son como los de `PRINT`:
las cadenas escritas en el programa y las expresiones entre paréntesis se
enseñan como aviso, `AT`, `TAB` y los colores funcionan, y cada variable
espera un valor.

- Para un **número**, el usuario teclea un número o una expresión numérica
  (vale incluso `2*3`) y pulsa Enter. Si no es válido, un `?` marca el error
  para que lo corrija.
- Para una **cadena**, la entrada empieza con unas comillas y el cursor
  dentro, porque lo que se teclea se toma como una expresión de cadena.
- `LINE a$` toma el texto tal como se teclea, sin comillas.
- Teclear `STOP` (para un número), o pulsar BREAK, para el programa con
  `H STOP en INPUT`.

El cursor enseña una `L`, o una `C` si está activado el bloqueo de
mayúsculas.

```
10 INPUT "Tu nombre? "; LINE n$
20 INPUT "Tu edad? "; a
30 PRINT "Hola, "; n$; ". Pronto cumples "; a+1
```

Los textos del programa no pueden llevar tildes, eñes ni `¿`, y lo que se
teclea en el `INPUT` tampoco: las letras pierden la tilde, así que `Peña`
llega como `Pena` (ver [Juego de caracteres](#juego-de-caracteres)).

`INPUT #n` lee de un fichero (ver [Streams](#leer-y-escribir-ficheros-de-texto)).

### INKEY$

```
INKEY$
```

La tecla que está pulsada en ese momento, como una cadena de un carácter, o
`""` si no hay ninguna. No espera. Las letras salen en minúscula (en
mayúscula con Mayúsculas o con el bloqueo de mayúsculas), las cifras como
cifras, Enter como `CHR$ 13` y los cursores como `CHR$ 8` (izquierda),
`CHR$ 9` (derecha), `CHR$ 10` (abajo) y `CHR$ 11` (arriba). La tecla
Mayúsculas hace de CAPS SHIFT del Spectrum, y Ctrl, de SYMBOL SHIFT. También
cuenta un mando que se presenta como teclas con [`JOY`](#joy).

Lo que se pulsa mientras un programa lee así el teclado (con `INKEY$`, `IN` o
[`KEY`](#key)) no se queda guardado para después: al acabar el programa no
sale escrito en el editor, y un `INPUT` empieza vacío.

```
10 LET k$=INKEY$: IF k$="" THEN GO TO 10
20 PRINT "Has pulsado "; k$: IF k$<>"q" THEN GO TO 10
```

**Ver también:** [KEY](#key), mejor para los juegos, y [PAUSE](#pause).

### KEY

```
KEY n
```

1 si la tecla `n` está pulsada ahora mismo, 0 si no. Se pueden mirar varias
teclas pulsadas a la vez, así que es la forma de leer el teclado en los
juegos. `n` es:

| `n` | Tecla |
|---|---|
| de `CODE "a"` a `CODE "z"` (o en mayúsculas) | una letra |
| de `CODE "0"` a `CODE "9"` | una cifra |
| 8, 9, 10, 11 | los cursores izquierda, derecha, abajo, arriba |
| 12 | Retroceso |
| 13 | Enter |
| 32 | Espacio |
| 256 + código | cualquier otra tecla, por su código de teclado USB: 298 Retroceso, 299 Tab, de 314 a 325 F1 a F12, 480 Ctrl izquierdo, 481 Mayúsculas izquierda, 482 Alt izquierdo |

Otros valores dan `A Argumento no válido`.

```
10 LET x=128
20 LET x=x+KEY 9-KEY 8: PLOT x,88: WAIT: GO TO 20
```

### JOY

```
JOY n
JOY n, modo
```

La función `JOY n` da el estado del mando `n` (0 para el primero que se
conecta y 1 para el segundo) como un número formado por estos bits; 0 si no
está conectado:

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

Al arrancar, el mando 0 es Sinclair 1 y el mando 1, Sinclair 2. `KEY` no ve
estas teclas. Un mando que no sea el 0 o el 1, o un modo que no sea del 0 al
3, da `B Entero fuera de rango`, igual que en la función.

```
10 PRINT AT 0,0; JOY 0; "   ": GO TO 10
```

### MOUSE

```
MOUSE n
MOUSE x, y
```

La función `MOUSE n` lee el ratón:

| `n` | Valor |
|---|---|
| 0 | la x del puntero, en píxeles desde la izquierda de la pantalla |
| 1 | la y del puntero, en píxeles desde arriba |
| 2 | los botones: 1 izquierdo, 2 derecho, 4 central (sumados) |
| 3 | la rueda: crece al girarla hacia arriba y baja al girarla hacia abajo |

El puntero empieza en el centro de la pantalla y no sale de ella (0–255 y
0–191 en el modo 0). La orden `MOUSE x, y` coloca el puntero. ZX-V no dibuja
el puntero: lo dibuja tu programa, por ejemplo con un sprite.

```
10 GRAB 0,3,"ff.f.f..f"
20 CLS: SPRITE 0,MOUSE 0,MOUSE 1: IF MOUSE 2 THEN PRINT "clic"
30 WAIT: GO TO 20
```

## Ficheros y la tarjeta SD

Los programas y los datos se guardan en la tarjeta SD. Hay además una unidad
de solo lectura, `ROM:`, con los ejemplos que trae la máquina.

### Nombres de fichero y unidades

- Un nombre tiene hasta 63 caracteres, sin `\ : * ? " < > |`, sin letras con
  tilde ni eñes, y no acaba en espacio ni en punto. No se distinguen
  mayúsculas y minúsculas.
- Las carpetas se separan con `/`. Una ruta que empieza por `/` parte de la
  raíz de la unidad; si no, de la carpeta actual (ver [CD](#cd-dir)). `..`
  es la carpeta de arriba.
- `ROM:` y `SD:` al principio eligen la unidad, desde su raíz:
  `LOAD "ROM:hello"`, `CAT "SD:/juegos"`. Sin unidad, se usa la de la carpeta
  actual. Escribir en `ROM:` (`SAVE`, `ERASE`, `MKDIR`...) da `V Error de
  disco (no se puede)`.
- **Comodines** en `CAT`, `ERASE`, `MOVE` y `COPY`: `*` vale por cualquier
  grupo de caracteres (también ninguno) y `?` por uno, solo en la última parte
  de la ruta: `"*.tap"`, `"juegos/nivel?.dat"`. Si no encaja nada,
  `T Fichero no encontrado`.

Los errores de ficheros más corrientes son `F Nombre de fichero no válido`,
`T Fichero no encontrado`, `U No hay tarjeta SD` y `V Error de disco`, que
añade el motivo cuando se sabe: `(ya existe)`, `(la carpeta no está vacía)`,
`(es una carpeta)`, `(no es una carpeta)`, `(la tarjeta está llena)` o
`(no se puede)`.

### SAVE

```
SAVE nombre$ [LINE línea]
SAVE nombre$ CODE inicio, longitud
SAVE nombre$ SCREEN$
SAVE nombre$ DATA letra()
SAVE nombre$ DATA letra$()
SAVE nombre$ GRAPHICS
```

Graba en la tarjeta SD:

- El **programa** (sin más palabras): como un fichero `.tap`, el formato de
  cinta del Spectrum, así que se puede abrir en los emuladores del Spectrum.
  Con `LINE`, el programa arranca solo desde esa línea al cargarlo.
- `CODE`: `longitud` bytes de la memoria desde la dirección `inicio`.
- `SCREEN$`: la pantalla, como una pantalla del Spectrum (`CODE 16384,6912`).
- `DATA`: una matriz (de números o de caracteres), con su nombre.
- `GRAPHICS`: el almacén de gráficos (sprites, tiles, mapas, texturas) en un
  fichero `.gfx`.

Si el nombre no tiene extensión, se le añade `.tap` (`.gfx` con `GRAPHICS`).
Con otra extensión, `CODE` y `SCREEN$` se graban como bytes sin más, sin la
cabecera de cinta; `DATA` se graba siempre como `.tap`. Si el fichero ya
existe, se sustituye.

Con la extensión `.bas`, el programa se graba como **listado en texto** (ver
[Listados en texto](#listados-en-texto)): `SAVE "juego.bas"`.

```
10 PRINT "Me grabaron"
SAVE "hola" LINE 10
```

### LOAD

```
LOAD nombre$
LOAD nombre$ CODE [inicio [, longitud]]
LOAD nombre$ SCREEN$
LOAD nombre$ DATA letra()
LOAD nombre$ DATA letra$()
LOAD nombre$ GRAPHICS
```

Carga de la tarjeta SD (o de la unidad `ROM:`):

- Un **programa**: sustituye al que hay en la memoria (las variables no se
  conservan). Si se grabó con `LINE`, se ejecuta. Sin extensión, ZX-V prueba
  `nombre.tap`, después `nombre.zxv` (un [programa
  nativo](#programas-nativos), que se ejecuta), después `nombre.bas` (un
  [listado en texto](#listados-en-texto)) y por último el nombre tal cual. Los `.tap` y `.tzx` del Spectrum con programas en BASIC se cargan
  directamente.
- `CODE`: un bloque de bytes, en la dirección desde la que se grabó o en
  `inicio`, y como mucho `longitud` bytes. Un fichero que no es `.tap` se lee
  como bytes sin más, y entonces hace falta `inicio`.
- `SCREEN$`: una pantalla del Spectrum (un fichero `.scr` o un bloque de un
  `.tap`).
- `DATA`: una matriz, que sustituye a la que tenga ese nombre. El tipo tiene
  que coincidir.
- `GRAPHICS`: los objetos de un fichero `.gfx` sustituyen a los que tienen el
  mismo número; los demás se quedan, así que puedes cargar los sprites de un
  fichero y un nivel de otro. Un fichero que no es válido da `R Error al
  cargar`.

`LOAD ""` (con el nombre vacío) carga el bloque siguiente de la cinta puesta
(ver [Ficheros de cinta](#ficheros-de-cinta)).

```
LOAD "hola"
LOAD "ROM:graphics"
```

### VERIFY

```
VERIFY nombre$ [CODE ... | SCREEN$ | DATA ... | GRAPHICS]
```

Comprueba que un fichero es igual que lo que hay en la memoria, con las
mismas formas que `LOAD` y sin cambiar nada. Si hay diferencias, `R Error al
cargar`.

### MERGE

```
MERGE nombre$
```

Añade las líneas de un programa grabado al que hay en la memoria; las líneas
con el mismo número se sustituyen. Las variables se borran. También vale con
un [listado en texto](#listados-en-texto) (`.bas`).

### Listados en texto

Un programa también puede estar en un fichero de texto con la extensión
`.bas`, una línea del programa por línea, cada una con su número, como lo
saca `LIST`. Se puede escribir con cualquier editor de texto del ordenador,
copiarlo a la tarjeta y cargarlo con `LOAD "juego.bas"` (o `MERGE`);
`SAVE "juego.bas"` escribe el que hay en la memoria.

```
10 REM Escrito fuera de ZX-V
20 for i = 1 to 10: print i: next i
```

- Cada línea se lee como si la tecleases en el editor: las palabras clave en
  mayúsculas o no, y la misma comprobación de sintaxis. Una línea con un error
  para la carga con su informe y su número de línea
  (`C Nonsense in BASIC, 20:1`); las de antes se quedan en la memoria.
- Una línea con solo el número borra esa línea, y las líneas en blanco no
  cuentan. Una línea sin número da un error en la de antes: un `.bas` es un
  programa, no una lista de órdenes (no pongas `RUN`).
- El texto va en UTF-8. Las letras con tilde la pierden, como al teclearlas
  (`año` queda `ano`); `£` y `©` son las del Spectrum, y los gráficos de
  bloque se escriben con los caracteres de cuadrantes de Unicode
  (`▘ ▝ ▀ ▖ ▌ ▞ ▛ ▗ ▚ ▐ ▜ ▄ ▙ ▟ █`). Los caracteres UDG se graban como un byte
  suelto, que un editor de texto puede enseñar raro pero que se vuelve a
  cargar bien.
- Las líneas pueden acabar en LF (Linux, macOS) o en CR LF (Windows); una
  línea puede tener hasta 1024 caracteres.

### CAT

```
CAT [nombre$]
```

Lista una carpeta: primero el nombre de la carpeta, después cada fichero con
su tamaño en bytes, y cada carpeta con `/` y `<DIR>`. Sin nombre, la carpeta
actual. Con comodines o con el nombre de un fichero, solo lo que encaja.

```
CAT "ROM:"
CAT "*.tap"
```

### CD, DIR$

```
CD carpeta$
DIR$
```

`CD` cambia la carpeta actual: `CD "juegos"`, `CD ".."` (la de arriba),
`CD "/"` (la raíz de la unidad), `CD "ROM:"` (la unidad de los ejemplos) y
`CD "SD:"` (vuelta a la tarjeta). La función `DIR$` da la carpeta actual,
como `"/juegos"` o `"ROM:/"`.

```
CD "ROM:": PRINT DIR$: CAT
```

Un programa que carga más ficheros (como el juego de plataformas) los busca
en la carpeta actual, así que primero hay que ir a ella:
`CD "ROM:": LOAD "platformer"`.

### MKDIR, RMDIR

```
MKDIR carpeta$
RMDIR carpeta$
```

`MKDIR` crea una carpeta; `RMDIR` borra una carpeta vacía (si no lo está,
`V Error de disco (la carpeta no está vacía)`).

### ERASE

```
ERASE nombre$
```

Borra un fichero, o con comodines todos los que encajan (`ERASE "*.bak"`).
Las carpetas no se borran (para eso, `RMDIR`). No se puede deshacer.

### MOVE

```
MOVE de$ TO a$
```

Cambia el nombre de un fichero o de una carpeta, o lo lleva a otra carpeta:
si `a$` es una carpeta, va dentro con el mismo nombre. Con comodines, `a$`
tiene que ser una carpeta y se mueven todos los ficheros que encajan.

```
MOVE "juego.tap" TO "juego viejo.tap"
MOVE "*.tap" TO "juegos"
```

### COPY

```
COPY de$ TO a$
COPY
```

Con dos nombres, copia un fichero (si `a$` existe, lo sustituye; si es una
carpeta, la copia va dentro). Con comodines, `a$` tiene que ser una carpeta.
Copiar un fichero sobre sí mismo da `V Error de disco (ya existe)`. Para
copiar un ejemplo a la tarjeta: `COPY "ROM:hello.zxv" TO "hello.zxv"`.

`COPY` sin nada manda el texto de la pantalla a la impresora (el fichero
`printer.txt`, ver [LPRINT](#lprint-llist)).

### Programas nativos

Los programas escritos en C con el [SDK](../sdk/README.md) son ficheros
`.zxv`. `LOAD "nombre"` los ejecuta (la extensión no hace falta). Un programa
nativo usa la memoria del BASIC: **cuando acaba, el programa BASIC y sus
variables han desaparecido**, así que graba antes tu programa en BASIC.

```
LOAD "ROM:hello"
```

Cuando el programa acaba, el BASIC enseña un informe, como tras una orden:

- `0 OK, 0:1` si ha acabado bien.
- `W Error en el código nativo (3), 0:1` si ha acabado con un código de error
  (el 3 en este ejemplo), o `W` seguido del mensaje del propio programa, si
  da uno: `W Fichero dañado (5), 0:1`.
- `W Error en el código nativo, 0:1` si ha fallado (por ejemplo, al leer de
  una dirección prohibida).
- `L BREAK en el programa, 0:1` si lo has parado con Ctrl+Alt+Supr.

El informe es del programa nativo: tu programa BASIC ha desaparecido en todos
los casos.

## Ficheros de cinta

ZX-V lee los ficheros de cinta de los emuladores del Spectrum, `.tap` y
`.tzx` (sus bloques de velocidad normal: programas, `CODE`, pantallas y
matrices). El código máquina del Spectrum que contengan no se puede
ejecutar.

Un fichero de cinta que se ha cargado se queda **puesto**, como una cinta en
la casetera, así que funcionan los programas de varias partes:

- `LOAD ""` (y `LOAD "" SCREEN$`, `LOAD "" CODE`, `LOAD "" DATA a()`,
  `VERIFY ""`, `MERGE ""`) lee el siguiente bloque de ese tipo desde donde se
  quedó la cinta. Al llegar al final, se rebobina una vez. Sin cinta puesta, o
  sin ningún bloque de ese tipo, `R Error al cargar`.
- `LOAD "nombre"` usa el fichero `nombre` si existe en la tarjeta (y pasa a
  ser la cinta puesta); si no, busca en la cinta puesta un bloque con ese
  nombre, y si tampoco está, `T Fichero no encontrado`.
- La cinta sigue puesta después de `NEW` y de `CD`, hasta que se carga otro
  fichero de cinta o se reinicia la máquina.

Así, `LOAD "juego"` carga el cargador de `juego.tap`, que puede traer su
pantalla de presentación y el programa con `LOAD "" SCREEN$: LOAD ""`.

## Streams y canales

El texto entra y sale por unos **streams** (flujos) numerados del 0 al 15.
Los streams 0 a 3 están listos desde el principio:

| Stream | Va a |
|---|---|
| 0, 1 | la parte inferior de la pantalla (el canal `"k"`) |
| 2 | la parte superior de la pantalla (el canal `"s"`), la de `PRINT` |
| 3 | la impresora, el fichero `printer.txt` (el canal `"p"`), la de `LPRINT` |

`PRINT`, `LIST` e `INPUT` admiten un stream con `#`: `PRINT #1; "abajo"`
escribe en la parte inferior y `LIST #3` lista en la impresora. Los streams 4
a 15 se pueden conectar a ficheros de texto de la tarjeta. Un stream que no
está abierto da `O Canal no válido`.

### OPEN, CLOSE

```
OPEN #stream, nombre$ IN
OPEN #stream, nombre$ OUT
OPEN #stream, canal$
CLOSE #stream
```

`OPEN # ... OUT` crea un fichero de texto (y lo vacía si ya existe) para
escribir en él; `OPEN # ... IN` abre uno para leer. Los ficheros solo pueden
usar los streams 4 a 15, y puede haber hasta 8 abiertos a la vez (si no,
`O Canal no válido`). Abrir un stream que ya tiene un fichero lo cierra antes.

`OPEN #stream, "k"`, `"s"` o `"p"` conecta un stream a la pantalla inferior,
a la superior o a la impresora; otro canal da `J Dispositivo no válido`.

`CLOSE #stream` cierra el fichero (escribiendo lo que quedaba pendiente) y
deja libre el stream; los streams 0 a 3 vuelven a sus canales de siempre.
`RUN`, `CLEAR` y `NEW` cierran todos los ficheros.

### Leer y escribir ficheros de texto

```
PRINT #stream; elementos
LIST #stream
INPUT #stream; var [, var]... [LINE a$]
INKEY$ #stream
```

- `PRINT #n` escribe como `PRINT`, sin colores ni `AT`; `TAB` escribe
  espacios. En un fichero, **la coma empieza una línea nueva**, para que lo
  que escribe `PRINT #n; a, b$` se pueda leer después con
  `INPUT #n; a, b$`.
- `LIST #n` escribe el programa como texto.
- `INPUT #n` lee una línea por cada variable (las líneas pueden acabar en LF,
  CR o CR LF). Una variable de cadena se queda con la línea entera; un número
  se calcula como una expresión (si no es válida, `C Sintaxis incorrecta`). Al
  final del fichero, `8 Fin del fichero`.
- `INKEY$ #n` da el siguiente carácter del fichero, o `""` al final.

Escribir en un fichero abierto con `IN`, o leer de uno abierto con `OUT`, da
`J Dispositivo no válido`.

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

### FORMAT

`FORMAT` solo existe por compatibilidad con programas antiguos del Spectrum, y
da `O Canal no válido`.

## Memoria y sistema

### PEEK, POKE

```
PEEK dirección
POKE dirección, valor
```

`PEEK` lee un byte (de 0 a 255) de la memoria y `POKE` escribe uno (un valor
negativo, de -255 a -1, se escribe como 256 más ese valor).

- Las direcciones **por debajo de 65536** van a una **memoria de Spectrum
  virtual**, para que sigan funcionando los programas del Spectrum que hacen
  `PEEK` y `POKE` en la pantalla, en los colores o en las variables del
  sistema (ver la tabla de abajo).
- Las direcciones **de 65536 en adelante** leen y escriben la memoria real de
  ZX-V, byte a byte (por ejemplo, los [registros de la
  máquina](../machine.md)). `POKE` solo escribe en la RAM; las direcciones
  fuera de la memoria, y la zona protegida del sistema, dan `B Entero fuera de
  rango`.

```
PRINT PEEK 536872704
```

escribe el byte bajo del contador de frames.

### La memoria del Spectrum virtual

| Direcciones | Contenido |
|---|---|
| 0–16383 | la ROM del Spectrum: se lee como 0, salvo el juego de caracteres en 15616 |
| 16384–22527 | los píxeles de la pantalla, ordenados como en el Spectrum (`POKE` dibuja con los colores de cada celda) |
| 22528–23295 | los colores de cada celda (`POKE` los cambia) |
| 23552–23733 | variables del sistema (ver abajo) |
| 23296–65535 | memoria normal: lo que escribes con `POKE` lo lees con `PEEK` |
| 65368–65535 | los gráficos definidos por el usuario, de la A a la U |

En estas direcciones solo se ven los primeros 256 × 192 píxeles de la
pantalla.

Variables del sistema que funcionan: `LAST K` (23560, la última tecla),
`CHARS` (23606, dónde está el juego de caracteres; puedes apuntarla a una
fuente tuya en la memoria virtual), `BORDCR` (23624), `PROG` (23635),
`FLAGS2` (23658, el bit 3 es el bloqueo de mayúsculas), `DF SZ` (23659),
`SEED` (23670, la semilla de `RND`), `FRAMES` (23672, tres bytes que cuentan
frames), `UDG` (23675), `COORDS` (23677, el último punto de `PLOT`), `S POSN`
(23688), `SCR CT` (23692), `ATTR P`, `MASK P`, `ATTR T`, `MASK T` y `P FLAG`
(de 23693 a 23697) y `RAMTOP` (23730).

### Gráficos definidos por el usuario (UDG)

Los códigos de carácter del 144 al 164 son 21 caracteres, de la A a la U, que
puedes dibujar tú. `USR "a"` da la dirección del primer byte del carácter A (y
así hasta `USR "u"`); cada carácter son 8 bytes, uno por fila empezando por
arriba, y cada bit es un píxel (1 = tinta). Al principio se ven como las
letras de la A a la U.

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

Con una dirección de 65536 o más, llama al código máquina nativo RISC-V de esa
dirección y devuelve el valor que da (en el registro `a0`). Si el código falla,
`W Error en el código nativo`. El código máquina Z80 del Spectrum no se puede
ejecutar: una dirección por debajo de 65536 da `S Código Z80 no admitido`.
Para llamar a un código sin usar el resultado, `RANDOMIZE USR dirección`.

De `USR "a"` a `USR "u"` dan la dirección de un [gráfico definido por el
usuario](#gráficos-definidos-por-el-usuario-udg); otra letra da `A Argumento no
válido`.

### IN, OUT

```
IN puerto
OUT puerto, valor
```

Leen y escriben los puertos del Spectrum, tal como los imita ZX-V:

| Puerto | `IN` | `OUT` |
|---|---|---|
| 254 (y cualquier puerto par) | las semifilas del teclado que elige el byte alto, como en el Spectrum (bits a 0 = pulsada) | bits 0–2 color del borde, bit 4 beeper |
| 31 | joystick Kempston (el mando 0) | |
| 64479, 65503 | ratón Kempston: contadores de x e y | |
| 64223 | botones del ratón Kempston (0 = pulsado) | |
| 65533 | lee el registro del AY elegido | elige un registro del AY; 255 elige el primer chip y 254 el segundo |
| 49149 | | escribe en el registro del AY elegido |

Los demás puertos leen 255 y no hacen caso de lo que se escribe. Para los
programas hechos para ZX-V es mejor `KEY`, `JOY`, `MOUSE`, `BORDER` y `SOUND`.

### TIME

```
TIME
```

El número de frames desde que se encendió la máquina, 50 por segundo. Para
medir un tiempo, resta dos lecturas:

```
10 LET t=TIME: PAUSE 100: PRINT (TIME-t)/50; " segundos"
```

### CLOCK

```
CLOCK n
CLOCK año, mes, día, hora, minutos [, segundos]
```

La función `CLOCK n` lee la fecha y la hora: 0 el año, 1 el mes, 2 el día, 3
la hora, 4 los minutos, 5 los segundos y 6 el día de la semana (1 lunes ...
7 domingo). Da 0 si el reloj no está en hora. La orden pone el reloj en hora
(años de 2000 a 2099); una fecha que no existe da `A Argumento no válido`. Los
ficheros que se graban en la tarjeta llevan esa fecha.

```
PRINT CLOCK 2; "/"; CLOCK 1; "/"; CLOCK 0; " "; CLOCK 3; ":"; CLOCK 4
```

### SPEED

```
SPEED n
```

`SPEED 1` (el de partida) ejecuta el BASIC a toda velocidad, unas 200 veces
más deprisa que un Spectrum. `SPEED 0` hace que cada orden tarde más o menos
lo que tardaría en un Spectrum 48K, para los programas antiguos que usan
bucles como pausa o que serían injugables a toda velocidad. Otros valores dan
`A Argumento no válido`. Las órdenes nuevas (sprites, mapas...) no se frenan.

### LANGUAGE

```
LANGUAGE código$
```

Elige el idioma de la máquina con un código de dos letras: `"en"` inglés o
`"es"` español. Los informes de error y la barra y el menú del editor lo usan
desde ese momento, y la máquina lo recuerda (en el hardware de verdad, en
su reloj con pila; en el emulador, en sus ajustes). Se admite cualquier
otro código de dos letras, que saca los textos en inglés (un programa puede
leerlo para elegir los suyos: `PEEK 536872736` y `PEEK 536872737` son los
códigos de las dos letras). Algo que no son dos letras da `A Argumento no
válido`.

```
LANGUAGE "es"
```

### MONITOR

```
MONITOR
```

Abre el monitor de pruebas, una pantalla para comprobar el hardware (teclado,
mando, modos de vídeo, sonido). Para salir, reinicia la máquina
(Ctrl+Alt+Supr).

## Órdenes de edición

Estas órdenes se usan sobre todo como órdenes directas, mientras escribes un
programa. En [el editor](editor.md) está todo lo demás.

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
espacios más a la derecha por nivel (los espacios no se guardan).

**Ver también:** [LLIST](#lprint-llist), [EDIT](#edit).

### EDIT

```
EDIT [línea]
```

Pone el cursor del editor en esa línea (sin número, en la línea actual). No
hay que confundirla con la tecla EDIT (Tab), que abre el menú de opciones del
editor.

### AUTO

```
AUTO [primera [, paso]]
```

Números de línea automáticos: cada línea nueva del editor empieza con el
número siguiente. Sin valores, empieza en la última línea más 10 y va de diez
en diez. Se acaba al pulsar Enter en una línea que solo tiene el número, al
pulsar Esc o al pasar a otra línea. Solo como orden directa (en un programa,
`C Sintaxis incorrecta`).

```
AUTO 100, 5
```

### RENUM

```
RENUM [primera [, paso]]
```

Renumera el programa (desde 10 y de diez en diez si no se dice otra cosa), y
corrige los números de línea que van detrás de `GO TO`, `GO SUB`, `RESTORE`,
`RUN`, `LIST`, `LLIST` y `LINE`. Solo se corrigen los números escritos tal
cual: en `GO TO n*10` la expresión no cambia. Las etiquetas no hace falta
corregirlas. Si los números pasaran de 9999, `B Entero fuera de rango`. Solo
como orden directa. El menú de opciones del editor tiene lo mismo.

```
RENUM 100, 10
```

### DELETE

```
DELETE primera [, última]
```

Borra las líneas de `primera` a `última` (o solo la `primera`). Solo como
orden directa.

```
DELETE 100, 190
```

### FIND

```
FIND [texto]
```

Lleva el cursor del editor a la siguiente línea del programa donde aparece
`texto`, desde la línea que va detrás de la del cursor y dando la vuelta al
final, con el cursor sobre el texto. No distingue mayúsculas de minúsculas, y
busca en el listado tal como se ve (no en los números de línea). Sin texto,
busca lo mismo que la última vez, como la tecla F3. Si no está, la línea de
estado dice `No encontrado`. Solo como orden directa.

```
FIND "puntos"
```

**Ver también:** [REPLACE](#replace).

### REPLACE

```
REPLACE viejo, nuevo
```

Cambia el texto `viejo` por `nuevo` en todo el programa, sin distinguir
mayúsculas de minúsculas. Cada línea cambiada se comprueba como si se
tecleara, y las que quedarían con un error no se tocan. La línea de estado
dice cuántos cambios ha hecho y cuántas líneas se quedaron sin cambiar. Con
`viejo` vacío, `A Argumento no válido`. Solo como orden directa.

```
REPLACE "vidas", "energia"
```

**Ver también:** [FIND](#find).

### NEW

```
NEW
```

Borra el programa, las variables y el almacén de gráficos, y empieza de cero
(la cinta puesta se queda). No se puede deshacer: graba antes.

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
programa ha acabado bien. Pulsa una tecla para volver al editor.

Los informes salen en el idioma de la máquina (ver [LANGUAGE](#language)).

### ON ERROR

```
ON ERROR GO TO línea | @etiqueta
ON ERROR GO TO 0
```

A partir de ese momento, si un error para el programa, salta a esa línea o
etiqueta. Ahí, `ERR` y `ERL` dicen qué ha pasado. `ON ERROR GO TO 0` lo quita,
y también `RUN` y `CLEAR`.

- La trampa se quita sola al usarse, para que un error dentro del manejador no
  se repita sin fin: escribe otra vez `ON ERROR` para seguir atrapando
  errores.
- `CONTINUE` repite la orden que falló (tras un BREAK, sigue por la
  siguiente).
- Atrapa todos los errores, BREAK incluido, salvo `STOP` y los de las órdenes
  directas.

```
10 ON ERROR GO TO @fallo
20 PRINT 1/0
30 PRINT "sigo"
40 STOP
50 @fallo: PRINT "Error "; ERR; " en la linea "; ERL
60 ON ERROR GO TO @fallo: GO TO 30
```

### ERR, ERL

```
ERR
ERL
```

Tras un error atrapado con `ON ERROR`, `ERR` es el código del error como número
(las cifras son ellas mismas y las letras siguen contando desde el 10: la `A`
es el 10, la `B` el 11... la `Z` el 35; la tabla de abajo los da todos) y
`ERL`, la línea en la que pasó.

## Mensajes de error

Los informes en español y en inglés, con el número que da `ERR`:

| Código | `ERR` | Español | Inglés | Por qué suele salir |
|---|---|---|---|---|
| 0 | 0 | OK | OK | No es un error: el programa o la orden han acabado bien. |
| 1 | 1 | NEXT sin FOR | NEXT without FOR | `NEXT` con una variable que no es la de un `FOR`. |
| 2 | 2 | Variable no encontrada | Variable not found | Usar una variable antes de darle valor, o una matriz antes de su `DIM`. |
| 3 | 3 | Subíndice incorrecto | Subscript wrong | Un subíndice fuera del tamaño de la matriz, un número de subíndices que no es el suyo o un trozo fuera de la cadena. |
| 4 | 4 | Sin memoria | Out of memory | No hay memoria, hay demasiados `GO SUB` (512) o bucles y procedimientos (64) uno dentro de otro, o una expresión demasiado anidada. |
| 5 | 5 | Fuera de la pantalla | Out of screen | `PRINT AT` o `INPUT` fuera de la pantalla. |
| 6 | 6 | Número demasiado grande | Number too big | Un resultado mayor que 1,7E38; muchas veces, una división entre cero. |
| 7 | 7 | RETURN sin GO SUB | RETURN without GOSUB | `RETURN` sin un `GO SUB` al que volver. |
| 8 | 8 | Fin del fichero | End of file | `INPUT #` después de la última línea de un fichero. |
| 9 | 9 | Sentencia STOP | STOP statement | El programa ha llegado a un `STOP`. `CONTINUE` sigue. |
| A | 10 | Argumento no válido | Invalid argument | Un valor que una función no admite (`SQR -1`, `LN 0`), un sprite o un mapa que no existe, una cadena de `PLAY` mal escrita, una fecha imposible. |
| B | 11 | Entero fuera de rango | Integer out of range | Un número entero fuera de su rango: una coordenada fuera de la pantalla, `CHR$ 300`, `POKE` con 256... |
| C | 12 | Sintaxis incorrecta | Nonsense in BASIC | La línea no es BASIC válido (una errata, un paréntesis que falta), o un valor del tipo equivocado. |
| D | 13 | BREAK - CONT repite | BREAK - CONT repeats | Has contestado N, Espacio o BREAK a `scroll?`. `CONTINUE` repite. |
| E | 14 | No quedan DATA | Out of DATA | `READ` sin `DATA` por leer. |
| F | 15 | Nombre de fichero no válido | Invalid file name | Un nombre vacío, demasiado largo o con caracteres no permitidos. |
| G | 16 | No cabe la línea | No room for line | La línea es demasiado larga, o la memoria está llena. |
| H | 17 | STOP en INPUT | STOP in INPUT | Se ha tecleado `STOP`, o pulsado BREAK, en un `INPUT`. |
| I | 18 | FOR sin NEXT | FOR without NEXT | Un bucle `FOR` que no se ejecuta ninguna vez y no tiene un `NEXT` al que saltar. |
| J | 19 | Dispositivo no válido | Invalid I/O device | Escribir en un fichero abierto para leer (o al revés), o un canal desconocido en `OPEN #`. |
| K | 20 | Color no válido | Invalid colour | Un color fuera de rango (`INK 10`, `BORDER 8`, `BOX 16,...`). |
| L | 21 | BREAK en el programa | BREAK into program | Has pulsado BREAK (Esc). `CONTINUE` sigue por la orden siguiente. |
| M | 22 | RAMTOP no válido | RAMTOP no good | Una dirección de `CLEAR` demasiado baja o demasiado alta. |
| N | 23 | Sentencia perdida | Statement lost | Un salto de vuelta a una orden que ya no existe (por ejemplo, tras cambiar la línea). |
| O | 24 | Canal no válido | Invalid stream | Un stream sin abrir, un fichero en los streams 0–3, más de 8 ficheros abiertos, o `FORMAT`. |
| P | 25 | FN sin DEF FN | FN without DEF | `FN` de una función sin `DEF FN` en el programa. |
| Q | 26 | Error en los parámetros | Parameter error | El número o el tipo de los valores de `FN` o de `PROC` no encajan, o son demasiados. |
| R | 27 | Error al cargar | Tape loading error | Un fichero que no es válido o no es del tipo pedido, `LOAD ""` sin cinta puesta, o `VERIFY` ha encontrado diferencias. |
| S | 28 | Código Z80 no admitido | Z80 code not supported | `USR` con una dirección por debajo de 65536: código máquina del Spectrum. |
| T | 29 | Fichero no encontrado | File not found | No hay un fichero o una carpeta con ese nombre, o nada encaja con los comodines. |
| U | 30 | No hay tarjeta SD | No SD card | No hay tarjeta en la ranura. |
| V | 31 | Error de disco | Disk error | La tarjeta ha fallado o la operación no se puede hacer; el motivo va detrás, entre paréntesis. |
| W | 32 | Error en el código nativo | Native code error | Ha fallado el código nativo llamado con `USR`, o un programa nativo (`.zxv`) ha acabado con un error. Puede llevar el código del programa o su propio mensaje (ver [Programas nativos](#programas-nativos)). |
| X | 33 | Etiqueta no encontrada | Label not found | `GO TO @nombre` (o parecido) sin un `@nombre` en el programa. |
| Y | 34 | Error de estructura | Structure error | `WEND`, `UNTIL`, `END PROC` o `LOCAL` sin su principio, o un bloque (`WHILE`, `IF`, `SELECT CASE`...) sin su final. |
| Z | 35 | PROC no encontrado | PROC not found | `PROC` de un procedimiento sin `DEF PROC`. |

Los motivos de `V Error de disco`: `ya existe` / `already exists`, `la carpeta
no está vacía` / `folder not empty`, `es una carpeta` / `it is a folder`,
`no es una carpeta` / `not a folder`, `la tarjeta está llena` / `card full`,
`no se puede` / `not allowed`.

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

En el editor se pueden teclear los códigos del 32 al 127 (también `£` y `©`,
si tu teclado los tiene). Las letras con tilde, la `ñ`, `¿` y `¡` **no** son
caracteres del BASIC, así que en los textos de tus programas hay que pasarse
sin ellas. Si las tecleas, en el editor o en un `INPUT`, las letras con
tilde, diéresis o cedilla salen sin ella (`á` sale `a`, `Ñ` sale `N`, `ç` sale
`c`), `¿` y `¡` salen `?` y `!`, y los demás caracteres no hacen nada. La ROM
sí las usa en sus propios mensajes, como los informes de error en español,
pero tus programas no pueden. Los gráficos de bloques y los definidos por el
usuario se escriben con `CHR$`.

## Límites

| Qué | Límite |
|---|---|
| Números de línea | del 1 al 9999 |
| Longitud de una línea en el editor | 1024 caracteres |
| Números | de unos ±2,9E-39 a ±1,7E38, con 8 cifras a la vista |
| Nombres de variables numéricas | hasta 63 letras y cifras |
| Variables de cadena, matrices, variables de `FOR`, `DEF FN` | una letra (26 de cada) |
| Cadenas | tan largas como quepa en la memoria |
| Dimensiones de una matriz | hasta 16, cada una hasta 65535 |
| `GO SUB` uno dentro de otro | 512 |
| `WHILE`, `REPEAT` y `PROC` uno dentro de otro | 64 |
| IF de bloque y `SELECT CASE` uno dentro de otro | sin límite |
| Parámetros de un procedimiento | 16 |
| Paréntesis anidados en una expresión | unos 150 (unas 25 `FN` una dentro de otra) |
| Memoria para el programa y las variables | unos 96 KB (se ve en la barra del editor) |
| Ficheros abiertos | 8, en los streams 4 a 15 |
| Sprites | 256 (0–255), cada uno de hasta 1024 × 1024 píxeles |
| Tiles | 4096 de 8 × 8 y 4096 de 16 × 16 |
| Mapas de tiles | 16 (0–15), de hasta 32767 × 32767 celdas |
| Texturas | 4 (0–3) |
| Vértices de un polígono | 64 |
| `PLAY` | 8 cadenas (suenan 6), 4 niveles de paréntesis |
| Nombres de fichero | 63 caracteres; rutas de 255 |

## Índice de palabras clave

| Palabra clave | Tipo | Entrada |
|---|---|---|
| `@` | orden | [@etiqueta](#etiqueta) |
| `ABS` | función | [ABS, SGN, INT](#abs-sgn-int) |
| `ACS` | función | [ASN, ACS, ATN](#asn-acs-atn) |
| `AND` | operador | [Verdadero y falso](#verdadero-y-falso) |
| `ASN` | función | [ASN, ACS, ATN](#asn-acs-atn) |
| `AT` | en `PRINT`, `INPUT` | [PRINT](#print) |
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
| `CODE` | función; en `SAVE`, `LOAD` | [CHR$, CODE](#chr-code), [SAVE](#save) |
| `CONTINUE` | orden | [CONTINUE](#continue) |
| `COPY` | orden | [COPY](#copy) |
| `COS` | función | [SIN, COS, TAN](#sin-cos-tan) |
| `DATA` | orden; en `SAVE`, `LOAD` | [READ, DATA, RESTORE](#read-data-restore), [SAVE](#save) |
| `DEF FN` | orden | [DEF FN, FN](#def-fn-fn) |
| `DEF PROC` | orden | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `DELETE` | orden | [DELETE](#delete) |
| `DIM` | orden | [DIM](#dim) |
| `DIR$` | función | [CD, DIR$](#cd-dir) |
| `DISC` | orden | [DISC](#disc) |
| `DRAW` | orden | [DRAW](#draw) |
| `EDIT` | orden | [EDIT](#edit) |
| `ELSE` | en `IF` | [IF, THEN, ELSE](#if-then-else) |
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
| `GO TO` | orden | [GO TO](#go-to) |
| `GRAB` | orden | [GRAB](#grab) |
| `GRAPHICS` | en `SAVE`, `LOAD`, `VERIFY` | [SAVE](#save), [LOAD](#load) |
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
| `MAP` | orden | [MAP](#map), [MAP DIM, MAP POKE](#map-dim-map-poke) |
| `MERGE` | orden | [MERGE](#merge) |
| `MKDIR` | orden | [MKDIR, RMDIR](#mkdir-rmdir) |
| `MODE` | orden | [MODE](#mode) |
| `MONITOR` | orden | [MONITOR](#monitor) |
| `MOUSE` | función, orden | [MOUSE](#mouse) |
| `MOVE` | orden | [MOVE](#move) |
| `NEW` | orden | [NEW](#new) |
| `NEXT` | orden | [FOR, NEXT](#for-next) |
| `NOT` | operador | [Verdadero y falso](#verdadero-y-falso) |
| `ON ERROR` | orden | [ON ERROR](#on-error) |
| `OPEN #` | orden | [OPEN, CLOSE](#open-close) |
| `OR` | operador | [Verdadero y falso](#verdadero-y-falso) |
| `OUT` | orden; en `OPEN #` | [IN, OUT](#in-out), [OPEN, CLOSE](#open-close) |
| `OVER` | orden, elemento | [INVERSE, OVER](#inverse-over) |
| `PAGE` | orden | [PAGE, SCREEN](#page-screen) |
| `PAPER` | orden, elemento | [INK, PAPER](#ink-paper) |
| `PAUSE` | orden | [PAUSE](#pause) |
| `PEEK` | función | [PEEK, POKE](#peek-poke) |
| `PI` | función | [PI](#pi) |
| `PIXEL` | función | [PIXEL](#pixel) |
| `PLAY` | orden | [PLAY](#play) |
| `PLOT` | orden | [PLOT](#plot) |
| `POINT` | función | [POINT](#point) |
| `POKE` | orden; en `MAP` | [PEEK, POKE](#peek-poke), [MAP DIM, MAP POKE](#map-dim-map-poke) |
| `POLY` | orden | [POLY](#poly) |
| `PRINT` | orden | [PRINT](#print) |
| `PROC` | orden | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `RANDOMIZE` | orden | [RND, RANDOMIZE](#rnd-randomize) |
| `RASTER` | orden | [RASTER](#raster) |
| `READ` | orden | [READ, DATA, RESTORE](#read-data-restore) |
| `REM` | orden | [REM](#rem) |
| `REMAP` | orden | [REMAP](#remap) |
| `RENUM` | orden | [RENUM](#renum) |
| `REPEAT` | orden | [REPEAT, UNTIL](#repeat-until) |
| `REPLACE` | orden | [REPLACE](#replace) |
| `RESTORE` | orden | [READ, DATA, RESTORE](#read-data-restore) |
| `RETURN` | orden | [GO SUB, RETURN](#go-sub-return) |
| `RMDIR` | orden | [MKDIR, RMDIR](#mkdir-rmdir) |
| `RND` | función | [RND, RANDOMIZE](#rnd-randomize) |
| `RUN` | orden | [RUN](#run) |
| `SAVE` | orden | [SAVE](#save) |
| `SCREEN` | orden | [PAGE, SCREEN](#page-screen) |
| `SCREEN$` | función; en `SAVE`, `LOAD` | [SCREEN$, ATTR](#screen-attr), [SAVE](#save) |
| `SCROLL` | orden | [SCROLL](#scroll) |
| `SELECT CASE` | orden | [SELECT CASE](#select-case) |
| `SGN` | función | [ABS, SGN, INT](#abs-sgn-int) |
| `SIN` | función | [SIN, COS, TAN](#sin-cos-tan) |
| `SOUND` | orden | [SOUND](#sound) |
| `SPEED` | orden | [SPEED](#speed) |
| `SPRITE` | orden | [SPRITE](#sprite) |
| `SQR` | función | [SQR](#sqr) |
| `STEP` | en `FOR` | [FOR, NEXT](#for-next) |
| `STOP` | orden | [STOP](#stop) |
| `STR$` | función | [STR$, VAL, VAL$](#str-val-val) |
| `TAB` | en `PRINT`, `INPUT` | [PRINT](#print) |
| `TAN` | función | [SIN, COS, TAN](#sin-cos-tan) |
| `TEXTURE` | orden | [TEXTURE](#texture) |
| `THEN` | en `IF` | [IF, THEN, ELSE](#if-then-else) |
| `TILE` | orden | [TILE](#tile) |
| `TIME` | función | [TIME](#time) |
| `TO` | en `FOR`, trozos, `MOVE`, `COPY` | [FOR, NEXT](#for-next), [Cadenas](#cadenas) |
| `TRACE` | orden | [TRACE](#trace) |
| `UNTIL` | orden | [REPEAT, UNTIL](#repeat-until) |
| `USR` | función | [USR](#usr) |
| `VAL` | función | [STR$, VAL, VAL$](#str-val-val) |
| `VAL$` | función | [STR$, VAL, VAL$](#str-val-val) |
| `VERIFY` | orden | [VERIFY](#verify) |
| `WAIT` | orden | [WAIT](#wait) |
| `WEND` | orden | [WHILE, WEND](#while-wend) |
| `WHILE` | orden | [WHILE, WEND](#while-wend) |
