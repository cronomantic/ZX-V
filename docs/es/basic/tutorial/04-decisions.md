[English](../../../en/basic/tutorial/04-decisions.md) · **Español**

# 4. Decisiones

**En este capítulo aprenderás** a hacer que el programa elija qué hacer con
`IF`, `THEN`, `ELSE` y `SELECT CASE`, a comparar números y cadenas, y a saltar a otro sitio
con `GO TO` y con etiquetas. Al final escribirás un juego de adivinar
números.

## `IF` ... `THEN`

`IF` ("si") comprueba una **condición**. Si se cumple, la máquina ejecuta lo
que va detrás de `THEN` ("entonces"); si no, sigue con la línea siguiente.

```
10 INPUT "Que edad tienes? ";e
20 IF e>=18 THEN PRINT "Ya puedes votar."
30 IF e<18 THEN PRINT "Podras votar dentro de ";18-e
```

Con 12, escribe `Podras votar dentro de 6`.

Estos son los signos para comparar:

| Signo | Significa |
|---|---|
| `=` | igual a |
| `<>` | distinto de |
| `<` | menor que |
| `>` | mayor que |
| `<=` | menor o igual que |
| `>=` | mayor o igual que |

También sirven con cadenas: `"lima"<"pera"` es cierto, porque compara por
orden alfabético (las mayúsculas van antes que las minúsculas).

## `ELSE`: una cosa o la otra

`ELSE` ("si no") dice qué hacer cuando la condición no se cumple:

```
10 INPUT "Contrasena? ";c$
20 IF c$="zxv" THEN PRINT "Bienvenido!" ELSE PRINT "Contrasena incorrecta."
```

## Varias cosas a la vez

Todo lo que hay detrás de `THEN` hasta el final de la línea depende de la
condición, aunque sean varias sentencias separadas por `:`:

```
10 INPUT "Un numero del 1 al 10? ";n
20 IF n<1 OR n>10 THEN PRINT "No es del 1 al 10.": STOP
30 IF n=7 THEN PRINT "El siete, que suerte!": PRINT "Muy bien."
40 PRINT "Has elegido el ";n
```

`STOP` acaba el programa ahí, con el mensaje `9 Sentencia STOP`.

## Bloques: un `IF` de varias líneas

Cuando lo que depende de la condición no cabe a gusto en una línea, deja
`THEN` al final y escribe las órdenes en las líneas siguientes, hasta
`END IF`. `ELSE` y `ELSE IF` (otra condición, que se mira si la anterior no
se cumplió) separan las partes:

```
10 INPUT "Nota? ";n
20 IF n>=9 THEN
30 PRINT "Sobresaliente"
40 PRINT "Enhorabuena!"
50 ELSE IF n>=5 THEN
60 PRINT "Aprobado"
70 ELSE
80 PRINT "Suspenso"
90 END IF
100 PRINT "Fin"
```

Solo se ejecuta una de las partes, y después el programa sigue tras `END IF`.
No hace falta sangrar las líneas: el editor lo hace solo al guardarlas, para
que se vea qué va dentro de qué.

## Elegir entre muchas: `SELECT CASE`

Para comparar un mismo valor con muchos, `SELECT CASE` es más claro que una
cadena de `ELSE IF`. Cada `CASE` lleva los valores con los que encaja: uno,
varios separados por comas, un rango con `TO` o una comparación como `< 0`.
`CASE ELSE` recoge lo demás:

```
10 INPUT "Mes (1-12)? ";m
20 SELECT CASE m
30 CASE 12, 1, 2: PRINT "Invierno"
40 CASE 3 TO 5: PRINT "Primavera"
50 CASE 6 TO 8: PRINT "Verano"
60 CASE 9 TO 11: PRINT "Otono"
70 CASE ELSE: PRINT "Ese mes no existe"
80 END SELECT
```

También vale con cadenas: `SELECT CASE r$` con `CASE "s", "S"`.

## `AND`, `OR`, `NOT`

Para juntar condiciones:

- `a AND b` ("y") se cumple cuando se cumplen las dos.
- `a OR b` ("o") se cumple cuando se cumple al menos una.
- `NOT a` ("no") se cumple cuando `a` no se cumple.

```
10 LET t=20
20 IF t>15 AND t<25 THEN PRINT "Hace buen tiempo"
30 IF NOT t>30 THEN PRINT "No hace demasiado calor"
```

Por dentro, una condición que se cumple vale 1 y una que no, 0: `PRINT 3>2`
escribe `1`. Más adelante le sacarás partido.

## Saltar: `GO TO` y etiquetas

Ya viste `GO TO` en el capítulo 1: manda el programa a otra línea. Junto con
`IF`, permite repetir algo hasta que salga bien.

En vez de un número de línea, puedes saltar a una **etiqueta**: un nombre
que empieza por `@`, puesto al principio de una sentencia. Las etiquetas
hacen que el programa se lea mejor, y siguen funcionando aunque renumeres las
líneas.

Aquí tienes un juego completo. La máquina piensa un número del 1 al 100 y
tienes que adivinarlo:

```
10 RANDOMIZE
20 LET secreto=INT (RND*100)+1
30 LET intentos=0
40 @pregunta: INPUT "Adivina mi numero (1-100): ";n
50 LET intentos=intentos+1
60 IF n<secreto THEN PRINT n;" es poco": GO TO @pregunta
70 IF n>secreto THEN PRINT n;" es mucho": GO TO @pregunta
80 PRINT "Si! Era el ";secreto
90 PRINT "Lo has adivinado en ";intentos;" intentos"
```

La línea con `@pregunta` marca el sitio al que volver. Cada intento fallido
manda el programa allí; cuando aciertas, ni la línea 60 ni la 70 saltan y el
programa llega a la 80.

## Pruébalo

1. Escribe un programa que pida un número y diga si es par o impar.
2. Pon un límite al juego de adivinar: si no lo has adivinado en 7 intentos,
   el programa te dice el número y se para.

<details>
<summary>Pista</summary>

Un número `n` es par cuando `n/2` no tiene decimales, es decir, cuando
`INT (n/2)=n/2`. Para el segundo, mira `intentos` antes de volver a
`@pregunta`: te vale una línea nueva
`55 IF intentos=7 AND n<>secreto THEN ...`.

</details>

<details>
<summary>Una solución para el primero</summary>

```
10 INPUT "Un numero? ";n
20 IF INT (n/2)=n/2 THEN PRINT n;" es par" ELSE PRINT n;" es impar"
```

</details>

---

**Referencia:** [`IF`, `THEN`, `ELSE`](../reference.md#if-then-else),
[`SELECT CASE`](../reference.md#select-case),
[Verdadero y falso](../reference.md#verdadero-y-falso),
[`GO TO`](../reference.md#go-to), [`@etiqueta`](../reference.md#etiqueta),
[`STOP`](../reference.md#stop).

**Siguiente:** [5. Bucles](05-loops.md) ·
**Anterior:** [3. Texto y preguntas](03-text-and-input.md)
