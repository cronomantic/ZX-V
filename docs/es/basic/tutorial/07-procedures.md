[English](../../../en/basic/tutorial/07-procedures.md) · **Español**

# 7. Procedimientos

**En este capítulo aprenderás** a dividir un programa en trozos con nombre
usando `DEF PROC` y `PROC`, a darles valores con los que trabajar, a que sus
variables sean solo suyas con `LOCAL`, y a crear tus propias funciones con
`DEF FN`. También conocerás `GO SUB`, la forma clásica de hacerlo.

## Para qué dividir un programa

Cuando un programa crece, es fácil perderse entre números de línea. La
solución es poner nombre a los trozos: "dibuja el tablero", "mueve al
jugador", "enseña los puntos". Cada trozo es un **procedimiento**. El
programa principal se queda en una lista corta de nombres, y cada
procedimiento se puede escribir y probar por separado.

**Si conoces el Spectrum:** los procedimientos, `LOCAL` y las etiquetas son
extensiones de ZX-V. `GO SUB` y `DEF FN` funcionan como en el 48K.

## `DEF PROC` y `PROC`

```
10 PROC titulo("ZX-V")
20 PROC titulo("BASIC")
30 PRINT "Hecho"
40 DEF PROC titulo(t$)
50 PRINT "*** ";t$;" ***"
60 END PROC
```

- `DEF PROC titulo(t$)` empieza la definición de un procedimiento que se
  llama `titulo`, y `END PROC` la termina.
- `PROC titulo("ZX-V")` lo **llama**: el programa salta al procedimiento,
  con `t$` valiendo `"ZX-V"`, lo ejecuta y, en `END PROC`, vuelve a la
  sentencia que sigue a la llamada.
- Cuando el programa llega por sí solo a un `DEF PROC` (la línea 40, después
  de la 30), se salta el procedimiento entero. Por eso puedes poner los
  procedimientos detrás del programa principal sin que pase nada raro.

Los valores entre paréntesis son los **parámetros**. Un procedimiento puede
tener varios, números o cadenas, separados por comas: `DEF PROC caja(n,c$)`.
Un parámetro de cadena es una sola letra seguida de `$`. Un procedimiento
también puede no tener parámetros: `DEF PROC cabecera` y `PROC cabecera`.

Los nombres de los procedimientos siguen las mismas reglas que las variables
numéricas, así que un nombre que sea justo una palabra clave no vale: llámalo
`guiones`, no `line`. Llamar a un procedimiento que no existe da
`Z PROC no encontrado`.

## `LOCAL`: variables propias

Los parámetros solo existen dentro del procedimiento: al terminar, una
variable de fuera con el mismo nombre recupera su valor. Para tener más
variables así, ponlas detrás de `LOCAL`:

```
10 LET i=99
20 PROC guiones(10)
30 PRINT "i sigue valiendo ";i
40 DEF PROC guiones(n)
50 LOCAL i
60 FOR i=1 TO n: PRINT "-";: NEXT i
70 PRINT
80 END PROC
```

El procedimiento usa `i` para su bucle, pero la `i` del programa principal
sigue valiendo 99. Sin `LOCAL i`, valdría 11.

## Un programa hecho de procedimientos

Esta es la forma de un programa bien organizado: arriba la parte principal,
que se lee casi como una descripción, y debajo los procedimientos.

```
10 REM Programa principal
20 PROC cabecera
30 FOR i=1 TO 3: PROC estrellas(i*3): NEXT i
40 PROC cabecera
50 REM Procedimientos
60 DEF PROC cabecera
70 PRINT "=========="
80 END PROC
90 DEF PROC estrellas(n)
100 LOCAL i
110 FOR i=1 TO n: PRINT "*";: NEXT i
120 PRINT
130 END PROC
```

## Un procedimiento que se llama a sí mismo

Un procedimiento puede llamar a otros, y también a sí mismo. Eso se llama
**recursividad**, y resuelve algunos problemas de una forma
sorprendentemente corta. El ejemplo clásico son las torres de Hanói: pasar
una pila de discos del palo 1 al 3, de uno en uno, sin poner nunca un disco
grande encima de uno más pequeño.

```
10 PROC hanoi(3,1,3,2)
20 DEF PROC hanoi(n,o,d,a)
30 IF n=0 THEN GO TO 70
40 PROC hanoi(n-1,o,a,d)
50 PRINT "Pasa un disco del ";o;" al ";d
60 PROC hanoi(n-1,a,d,o)
70 END PROC
```

Para pasar `n` discos del palo `o` (origen) al `d` (destino), usando `a`
como auxiliar: se apartan `n-1` discos al `a`, se pasa el más grande al `d`,
y se ponen encima los `n-1` discos que estaban en el `a`. Escribe los 7
movimientos de 3 discos. Prueba con `PROC hanoi(4,1,3,2)`.

## Tus propias funciones: `DEF FN`

Un procedimiento hace algo; una **función** devuelve un valor. `DEF FN`
define una función en una línea. Su nombre es una sola letra (con `$` si
devuelve una cadena), y se usa con `FN`:

```
10 DEF FN a(r)=PI*r*r
20 DEF FN m$(a$,b$)=a$+" y "+b$
30 PRINT FN a(10)
40 PRINT FN m$("sal","pimienta")
```

## La forma clásica: `GO SUB`

Antes de que existieran los procedimientos, los programas en BASIC usaban
**subrutinas**. `GO SUB` salta a una línea y `RETURN` vuelve justo detrás
del `GO SUB`:

```
10 GO SUB 100
20 PRINT "De vuelta en el programa principal"
30 STOP
100 PRINT "En la subrutina"
110 RETURN
```

Aquí hace falta el `STOP` de la línea 30: sin él, el programa seguiría y se
metería en la subrutina. Una subrutina no tiene nombre ni parámetros, y
todas sus variables son compartidas, así que para programas nuevos los
procedimientos son más cómodos. Encontrarás `GO SUB` en muchos programas
antiguos del Spectrum.

## Pruébalo

1. Escribe un procedimiento `cuadrado(n)` que dibuje un cuadrado de `n` × `n`
   estrellas, y llámalo con 3 y con 5.
2. Escribe una función `FN c(f)` que pase grados Fahrenheit a Celsius
   (C = (F − 32) × 5 / 9) y escribe el valor para 212.

<details>
<summary>Pista</summary>

Para el primero, usa dos bucles `FOR` dentro del procedimiento (como el
triángulo del capítulo 5) y declara sus variables con `LOCAL`. Para el
segundo: `DEF FN c(f)=(f-32)*5/9`.

</details>

<details>
<summary>Una solución para el primero</summary>

```
10 PROC cuadrado(3): PRINT
20 PROC cuadrado(5)
30 DEF PROC cuadrado(n)
40 LOCAL f,c
50 FOR f=1 TO n
60 FOR c=1 TO n: PRINT "*";: NEXT c
70 PRINT
80 NEXT f
90 END PROC
```

</details>

---

**Referencia:** [`DEF PROC`, `PROC`, `END PROC`](../reference.md#def-proc-proc-end-proc),
[`LOCAL`](../reference.md#local), [`DEF FN`, `FN`](../reference.md#def-fn-fn),
[`GO SUB`, `RETURN`](../reference.md#go-sub-return).

**Siguiente:** [8. Los colores y la pantalla](08-colours-and-the-screen.md) ·
**Anterior:** [6. Matrices y datos](06-arrays-and-data.md)
