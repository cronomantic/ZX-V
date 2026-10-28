[English](../../../en/basic/tutorial/06-arrays-and-data.md) · **Español**

# 6. Matrices y datos

**En este capítulo aprenderás** a guardar muchos valores con un solo nombre
usando **matrices** (`DIM`), y a guardar listas de valores dentro del
programa con `READ`, `DATA` y `RESTORE`.

## Matrices: una fila de cajas

Imagina que quieres guardar cinco notas. Cinco variables (`n1`, `n2`...)
servirían, pero no se pueden usar en un bucle. Una **matriz** es una fila de
cajas numeradas que comparten nombre. Se crea con `DIM`, diciendo cuántas
cajas tiene:

```
10 DIM s(5)
20 FOR i=1 TO 5
30 INPUT "Nota? ";s(i)
40 NEXT i
50 LET total=0
60 FOR i=1 TO 5: LET total=total+s(i): NEXT i
70 PRINT "Notas: ";
80 FOR i=1 TO 5: PRINT s(i);" ";: NEXT i
90 PRINT: PRINT "Media: ";total/5
```

`DIM s(5)` crea las cajas de `s(1)` a `s(5)`, todas a 0. El número entre
paréntesis es el **índice**, y puede ser una variable: `s(i)` es "la caja
número `i`". Eso es lo que hace tan útiles las matrices con `FOR`.

Algunas reglas:

- El nombre de una matriz es una sola letra: `s`, `a`, `b`.
- Las cajas se numeran desde el 1. Pedir una caja que no existe, como `s(6)`
  o `s(0)`, da `3 Subíndice incorrecto`.
- Una matriz puede tener más dimensiones: `DIM b(8,8)` es un tablero de
  8 × 8 cajas, y `b(2,3)` es la fila 2, columna 3.

Aquí una matriz cuenta cuántas veces sale cada cara de un dado en 600
tiradas:

```
10 RANDOMIZE
20 DIM c(6)
30 FOR i=1 TO 600
40 LET d=INT (RND*6)+1
50 LET c(d)=c(d)+1
60 NEXT i
70 FOR d=1 TO 6: PRINT d;": ";c(d): NEXT d
```

Cada cara sale unas 100 veces.

## Matrices de cadenas

Las matrices de cadenas necesitan un número más: lo larga que es cada
cadena. `DIM d$(7,9)` crea 7 cadenas de 9 caracteres exactos cada una. Un
texto más corto se rellena con espacios al final, y uno más largo se corta.

## `READ` y `DATA`: listas dentro del programa

Las líneas `DATA` guardan una lista de valores. `READ` coge el siguiente
valor de la lista y lo mete en una variable:

```
10 DIM d$(7,9)
20 FOR i=1 TO 7: READ d$(i): NEXT i
30 FOR i=1 TO 7: PRINT i;" ";d$(i);"|": NEXT i
40 DATA "lunes","martes","miercoles","jueves"
50 DATA "viernes","sabado","domingo"
```

El `|` del final enseña los espacios que rellenan cada día hasta 9
caracteres.

Las líneas `DATA` pueden ir en cualquier sitio del programa; la máquina se
las salta al ejecutarlo. Los valores se leen en orden, línea tras línea. Una
línea `DATA` puede mezclar números y cadenas, siempre que cada `READ` use el
tipo de variable que toca:

```
10 FOR i=1 TO 3
20 READ n$,e
30 PRINT n$;" tiene ";e
40 NEXT i
50 RESTORE
60 READ n$: PRINT "Otra vez el primero: ";n$
70 DATA "Ada",36,"Alan",41,"Grace",85
```

`RESTORE` vuelve al principio de la lista, así que el siguiente `READ` coge
otra vez el primer valor. `RESTORE 100` (o `RESTORE @etiqueta`) va a los
`DATA` de esa línea. Leer más valores de los que hay da `E No quedan DATA`.

## Pruébalo

1. Guarda los nombres de los meses en `DATA` y escribe un programa que pida
   un número del 1 al 12 y escriba el nombre de ese mes.
2. Llena una matriz con 10 números al azar del 1 al 100 y escribe el mayor.

<details>
<summary>Pista</summary>

Para el primero, lee los meses en `DIM m$(12,10)` con un bucle y luego
escribe `m$(n)`. Para el segundo, guarda el mayor hasta el momento en una
variable que empiece en 0 y compara cada número con ella:
`IF a(i)>mayor THEN LET mayor=a(i)`.

</details>

<details>
<summary>Una solución para el segundo</summary>

```
10 RANDOMIZE
20 DIM a(10): LET mayor=0
30 FOR i=1 TO 10
40 LET a(i)=INT (RND*100)+1: PRINT a(i);" ";
50 IF a(i)>mayor THEN LET mayor=a(i)
60 NEXT i
70 PRINT: PRINT "El mayor es ";mayor
```

</details>

---

**Referencia:** [`DIM`](../reference.md#dim),
[Matrices](../reference.md#matrices),
[`READ`, `DATA`, `RESTORE`](../reference.md#read-data-restore).

**Siguiente:** [7. Procedimientos](07-procedures.md) ·
**Anterior:** [5. Bucles](05-loops.md)
