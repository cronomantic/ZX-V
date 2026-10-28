[English](../../../en/basic/tutorial/02-variables-and-maths.md) · **Español**

# 2. Variables y cuentas

**En este capítulo aprenderás** a guardar números en **variables** con
`LET`, a hacer todo tipo de cuentas, a ordenar lo que escribe `PRINT` y a
sacar números al azar para tirar dados.

## Variables: cajas con nombre

Una **variable** es una caja con nombre donde la máquina guarda un número. Se
le mete un valor con `LET`:

```
10 LET manzanas=5
20 LET peras=3
30 PRINT "Fruta: ";manzanas+peras
40 LET manzanas=manzanas-1
50 PRINT "Quedan ";manzanas;" manzanas"
```

Escribe `Fruta: 8` y `Quedan 4 manzanas`. La línea 40 parece rara si la lees
como una ecuación, pero en BASIC quiere decir "coge el valor de `manzanas`,
réstale 1 y guarda el resultado otra vez en `manzanas`". Así se hace que una
variable cambie: sumar puntos, perder vidas, mover cosas.

Las reglas de los nombres de las variables numéricas:

- Empiezan por una letra y pueden seguir con letras y cifras: `x`, `puntos`,
  `nivel2`.
- Da igual mayúsculas que minúsculas: `Puntos` y `PUNTOS` son la misma caja.
  Los espacios dentro del nombre tampoco cuentan.
- Sin tildes ni eñes: `numero`, no `número` (si tecleas `número`, el
  editor escribe `numero`).
- Un nombre no puede ser justo una palabra clave: `LET print=1` es un error.
- Hay que dar un valor a una variable antes de usarla. Si no, sale
  `2 Variable no encontrada`, como viste en el capítulo 1.

## Hacer cuentas

Estos son los signos de las cuentas:

| Signo | Significado | Ejemplo | Resultado |
|---|---|---|---|
| `+` | sumar | `7+3` | `10` |
| `-` | restar | `7-3` | `4` |
| `*` | multiplicar | `7*3` | `21` |
| `/` | dividir | `7/2` | `3.5` |
| `^` | potencia | `2^10` | `1024` |

Como en el colegio, primero va `^`, luego `*` y `/`, y al final `+` y `-`.
Así que `2+3*4` da 14. Con paréntesis cambias el orden: `(2+3)*4` da 20. Fíjate
en que los decimales se escriben con punto, no con coma: `3.5`.

La máquina también conoce muchas **funciones**, que toman un número y
devuelven otro:

| Función | Qué da | Ejemplo | Resultado |
|---|---|---|---|
| `SQR` | la raíz cuadrada | `SQR 16` | `4` |
| `INT` | el entero de abajo | `INT 7.9` | `7` |
| `ABS` | el número sin signo | `ABS -5` | `5` |
| `PI` | el número π | `PI` | `3.1415927` |

Este programa usa algunas para calcular el área de un círculo:

```
10 LET r=10
20 LET area=PI*r^2
30 PRINT "Radio ";r
40 PRINT "Area ";area
50 PRINT "Redondeada ";INT (area+0.5)
```

Escribe `Radio 10`, `Area 314.15927` y `Redondeada 314`. Sumar 0.5 antes
de `INT` redondea al entero más cercano. Fíjate en los paréntesis detrás de
`INT`: sin ellos, `INT area+0.5` querría decir `(INT area)+0.5`, porque una
función solo se lleva lo que tiene justo detrás.

Los números muy grandes o muy pequeños se escriben con una `E`:
`PRINT 2^40` da `1.0995116E+12`, que quiere decir 1,0995116 × 10¹².

## Ordenar lo que escribe `PRINT`

En un `PRINT` caben varias cosas, separadas por estos signos:

| Signo | Efecto |
|---|---|
| `;` | Escribe lo siguiente justo detrás, sin hueco |
| `,` | Salta a la siguiente mitad de la línea (a la columna 0 o a la 16) |
| `'` | Empieza una línea nueva |

```
10 PRINT "A";"B";"C"
20 PRINT "A","B"
30 PRINT "A"'"B"
40 PRINT "Total: ";10*3
```

La línea 10 escribe `ABC`; la 20, `A` y `B` en dos columnas; la 30, una en
cada línea, y la 40, `Total: 30`.

También puedes poner varias sentencias en la misma línea, separadas por `:`.
Se ejecutan de izquierda a derecha:

```
10 LET total=10: LET total=total*2: PRINT total
```

## Números al azar

`RND` da un número distinto cada vez, entre 0 y 1 (nunca el 1). Para tirar un
dado, se multiplica por 6, se queda la parte entera y se suma 1, y así sale
de 1 a 6:

```
10 RANDOMIZE
20 LET dado=INT (RND*6)+1
30 PRINT "Has sacado un ";dado
```

Ejecútalo varias veces. `RANDOMIZE` baraja los números al azar, para que cada
vez se empiece por un sitio distinto. Ponlo una vez al principio de los
programas que usen `RND`.

## Pruébalo

1. Escribe un programa que pase una temperatura de grados Celsius a
   Fahrenheit. La fórmula es F = C × 9 / 5 + 32. Con 25 °C tiene que salir 77.
2. Cambia el programa del dado para tirar dos dados y escribir los dos y la
   suma.

<details>
<summary>Pista</summary>

Para el primero, guarda los grados Celsius en una variable (`LET c=25`) y
calcula los otros con `LET f=c*9/5+32`. Para el segundo, usa dos variables,
por ejemplo `d1` y `d2`, cada una con su `INT (RND*6)+1`.

</details>

<details>
<summary>Una solución para el primero</summary>

```
10 LET c=25
20 LET f=c*9/5+32
30 PRINT c;" C son ";f;" F"
```

</details>

---

**Referencia:** [`LET`](../reference.md#let),
[Variables](../reference.md#variables),
[Expresiones y operadores](../reference.md#expresiones-y-operadores),
[Matemáticas](../reference.md#matemáticas),
[`RND`, `RANDOMIZE`](../reference.md#rnd-randomize),
[`PRINT`](../reference.md#print).

**Siguiente:** [3. Texto y preguntas](03-text-and-input.md) ·
**Anterior:** [1. Primeros pasos en BASIC](01-first-steps.md)
