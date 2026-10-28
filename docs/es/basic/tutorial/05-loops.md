[English](../../../en/basic/tutorial/05-loops.md) · **Español**

# 5. Bucles

**En este capítulo aprenderás** a repetir cosas con `FOR` ... `NEXT`,
`WHILE` ... `WEND` y `REPEAT` ... `UNTIL`, a meter un bucle dentro de otro y
a esperar con `PAUSE`.

## Contar con `FOR` ... `NEXT`

Un **bucle** repite un grupo de líneas. `FOR` es el bucle que cuenta:

```
10 FOR i=1 TO 5
20 PRINT "Vuelta ";i
30 NEXT i
40 PRINT "Terminado!"
```

`FOR i=1 TO 5` da a `i` el valor 1 y ejecuta las líneas siguientes. Al llegar
a `NEXT i`, suma 1 a `i` y vuelve, hasta que `i` pasa de 5. Así que escribe
de `Vuelta 1` a `Vuelta 5` y luego `Terminado!`.

La variable que cuenta (aquí `i`) tiene que tener un **nombre de una sola
letra**: `FOR vuelta=1 TO 5` es un error.

`STEP` cambia lo que se suma cada vez, y puede ser negativo o tener
decimales:

```
10 FOR i=10 TO 0 STEP -2
20 PRINT i;" ";
30 NEXT i
40 PRINT
50 FOR x=0 TO 1 STEP 0.25: PRINT x;" ";: NEXT x
```

Escribe `10 8 6 4 2 0` y, en la línea siguiente, `0 0.25 0.5 0.75 1`. Un
`PRINT` sin nada detrás solo termina la línea.

Cualquiera de los números puede ser una variable o una cuenta. Aquí tienes
una tabla de multiplicar:

```
10 INPUT "Que tabla? ";t
20 FOR i=1 TO 10
30 PRINT i;" x ";t;" = ";i*t
40 NEXT i
```

## Bucles dentro de bucles

Puedes meter un bucle dentro de otro. Cada uno necesita su letra:

```
10 FOR f=1 TO 5
20 FOR c=1 TO f
30 PRINT "*";
40 NEXT c
50 PRINT
60 NEXT f
```

El bucle de fuera cuenta las filas (`f`); en cada fila, el de dentro escribe
`f` estrellas. Sale un triángulo:

```
*
**
***
****
*****
```

## `WHILE` ... `WEND`: repetir mientras se cumpla algo

A veces no sabes cuántas veces hay que repetir. `WHILE` ("mientras") repite
mientras se cumpla una condición, y la comprueba **antes** de cada vuelta:

```
10 LET dinero=100: LET t=0
20 WHILE dinero<200
30 LET dinero=dinero*1.1: LET t=t+1
40 WEND
50 PRINT "Se ha doblado en ";t;" vueltas"
```

Si cada vuelta (un año en el banco, por ejemplo) el dinero crece un 10 %, se
dobla en 8. Si la condición no se cumple desde el principio, las líneas de
dentro no se ejecutan nunca.

## `REPEAT` ... `UNTIL`: repetir hasta que pase algo

`REPEAT` ("repite") comprueba su condición **después** de cada vuelta, así
que las líneas de dentro se ejecutan al menos una vez. Repite hasta
(`UNTIL`) que la condición se cumple:

```
10 RANDOMIZE
20 LET tiradas=0
30 REPEAT
40 LET d=INT (RND*6)+1: LET tiradas=tiradas+1
50 PRINT d;" ";
60 UNTIL d=6
70 PRINT: PRINT "Un seis en ";tiradas;" tiradas"
```

Con `REPEAT`, el juego de adivinar del capítulo 4 ya no necesita `GO TO`:

```
10 RANDOMIZE
20 LET secreto=INT (RND*100)+1: LET intentos=0
30 REPEAT
40 INPUT "Adivina mi numero (1-100): ";n
50 LET intentos=intentos+1
60 IF n<secreto THEN PRINT n;" es poco"
70 IF n>secreto THEN PRINT n;" es mucho"
80 UNTIL n=secreto
90 PRINT "Si! ";intentos;" intentos"
```

¿Qué bucle usar? `FOR` cuando sabes cuántas veces; `WHILE` cuando puede que
no haga falta ninguna; `REPEAT` cuando hay que hacerlo al menos una vez.

## Esperar: `PAUSE`

`PAUSE n` espera `n` cincuentavos de segundo, así que `PAUSE 50` es un
segundo. `PAUSE 0` espera hasta que se pulse una tecla. Una tecla también
corta cualquier pausa.

```
10 FOR n=10 TO 1 STEP -1
20 PRINT n
30 PAUSE 50
40 NEXT n
50 PRINT "Despegue!"
```

**Si conoces el Spectrum:** `WHILE`, `WEND`, `REPEAT` y `UNTIL` son
extensiones de ZX-V; `FOR`, `NEXT` y `PAUSE` funcionan como en el 48K.

## Pruébalo

1. Escribe los números del 1 al 20 con sus cuadrados (el número por sí
   mismo).
2. Escribe un programa que pida una palabra y la escriba al revés: con
   `HOLA` escribe `ALOH`.

<details>
<summary>Pista</summary>

Para el segundo, `p$(i)` es la letra número `i`. Recorre las letras desde
`LEN p$` hasta 1 con `STEP -1` y escribe cada una seguida de `;`.

</details>

<details>
<summary>Una solución para el segundo</summary>

```
10 INPUT "Una palabra? ";p$
20 FOR i=LEN p$ TO 1 STEP -1
30 PRINT p$(i);
40 NEXT i
```

</details>

---

**Referencia:** [`FOR`, `NEXT`](../reference.md#for-next),
[`WHILE`, `WEND`](../reference.md#while-wend),
[`REPEAT`, `UNTIL`](../reference.md#repeat-until),
[`PAUSE`](../reference.md#pause).

**Siguiente:** [6. Matrices y datos](06-arrays-and-data.md) ·
**Anterior:** [4. Decisiones](04-decisions.md)
