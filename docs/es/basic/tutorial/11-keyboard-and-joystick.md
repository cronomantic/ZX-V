[English](../../../en/basic/tutorial/11-keyboard-and-joystick.md) · **Español**

# 11. Teclado y mando

**En este capítulo aprenderás** a leer el teclado sin parar el programa, con
`INKEY$` y `KEY`, a leer un mando (joystick) con `JOY`, y a llevar una
velocidad fija con `WAIT` y `TIME`. Son los controles de cualquier juego.

`INPUT` va bien para hacer preguntas, pero un juego no puede pararse a
esperar un Enter: tiene que mirar el teclado y seguir.

## `INKEY$`: ¿qué tecla está pulsada?

`INKEY$` da la tecla que se está pulsando en ese momento, como una cadena, o
`""` (una cadena vacía) si no hay ninguna. Nunca espera.

```
10 PRINT "Pulsa teclas (Q para salir)"
20 REPEAT
30 REPEAT: UNTIL INKEY$=""
40 REPEAT: LET k$=INKEY$: UNTIL k$<>""
50 PRINT "Has pulsado ";k$
60 UNTIL k$="q"
70 PAUSE 0
```

La línea 30 espera a que no haya ninguna tecla pulsada, y la 40 a que haya
una. ¿Por qué las dos? Porque el programa es muy rápido: mientras tienes el
dedo en una tecla, `INKEY$` da esa tecla una y otra vez. Sin la línea 30, una
sola pulsación llenaría la pantalla.

Un bucle se puede escribir en una sola línea, como en las líneas 30 y 40:
`REPEAT: UNTIL INKEY$=""` no repite nada, solo espera.

### Por qué los programas acaban con `PAUSE 0`

La máquina se acuerda de las teclas que pulsas mientras funciona un
programa, para escribirlas luego en el editor; `INKEY$` y `KEY` miran el
teclado, pero no las sacan de esa memoria. Sin la línea 70, las teclas que
has pulsado llegarían al editor en cuanto acabara el programa, como si
acabaras de teclearlas. `PAUSE` se olvida de las teclas pulsadas antes y
espera una nueva, así que acabar con `PAUSE 0` ("pulsa una tecla para
terminar") lo arregla. Todos los programas de este capítulo que leen el
teclado acaban así.

## `KEY`: varias teclas a la vez

En un juego puedes tener pulsadas "derecha" y "disparo" a la vez. `KEY n` da
1 si la tecla `n` está pulsada ahora mismo, y 0 si no. Puedes preguntar por
todas las teclas que quieras:

| `n` | Tecla |
|---|---|
| `CODE "a"` ... `CODE "z"`, `CODE "0"` ... `CODE "9"` | Letras y cifras |
| 8, 9, 10, 11 | Los cursores: izquierda, derecha, abajo, arriba |
| 13 | Enter |
| 32 | Espacio |

Este programa mueve una `@` por la pantalla. Usa **O**, **P**, **Q** y
**A**, o los cursores, y **Espacio** para parar:

```
10 BORDER 0: PAPER 0: INK 7: CLS
20 PRINT AT 21,0;"Mueve con OPQA o los cursores"
30 LET x=15: LET y=10
40 REPEAT
50 PRINT AT y,x;" "
60 IF (KEY CODE "o" OR KEY 8) AND x>0 THEN LET x=x-1
70 IF (KEY CODE "p" OR KEY 9) AND x<31 THEN LET x=x+1
80 IF (KEY CODE "q" OR KEY 11) AND y>0 THEN LET y=y-1
90 IF (KEY CODE "a" OR KEY 10) AND y<20 THEN LET y=y+1
100 PRINT AT y,x; INK 6;"@"
110 WAIT 3
120 UNTIL KEY 32
130 PAUSE 0
```

En cada vuelta del bucle, borra la `@` (línea 50), cambia `x` e `y` según las
teclas y la vuelve a escribir en su sitio nuevo. Si pulsas dos teclas a la
vez, se mueve en diagonal. Las condiciones `x>0`, `x<31`... la paran en los
bordes.

## Una velocidad fija: `WAIT` y `TIME`

La pantalla se redibuja 50 veces por segundo; cada vez es un **frame**.
`WAIT n` espera a que hayan pasado `n` frames desde el último `WAIT`. En el
programa de arriba, `WAIT 3` hace que cada vuelta del bucle dure justo tres
frames, así que la `@` va a la misma velocidad tenga el bucle mucho o poco
que hacer. Eso es lo que quieres en un juego.

`TIME` da el número de frames desde que se encendió la máquina. Sirve para
medir el tiempo. Este juego mide tus reflejos:

```
10 RANDOMIZE
20 BORDER 2: PAPER 0: INK 7: CLS
30 PRINT "Espera al verde y pulsa ESPACIO"
40 PAUSE 50+INT (RND*150)
50 BORDER 4: LET t=TIME
60 REPEAT: UNTIL KEY 32
70 LET t=TIME-t
80 PRINT "Tu tiempo: ";t*20;" milisegundos"
90 PAUSE 0
```

Cada frame dura 20 milisegundos; de ahí el `t*20`. Sin el `PAUSE 0` de la
línea 90, el espacio que has pulsado te devolvería al editor antes de que
pudieras leer tu tiempo.

## El mando: `JOY`

`JOY 0` lee el primer mando (joystick o gamepad) y `JOY 1` el segundo. Da 0
si no se pulsa nada (o no hay nada conectado), y si no, un número que suma
estos valores:

| Valor | Dirección o botón |
|---|---|
| 1 | derecha |
| 2 | izquierda |
| 4 | abajo |
| 8 | arriba |
| 16 | disparo |

Por ejemplo, arriba y disparo a la vez dan 8 + 16 = 24. Para saber si uno de
esos valores está dentro del número, viene bien esta función: `FN b(j,n)`
vale 1 cuando el valor 2^n está incluido en `j` (n = 0 para derecha, 1
izquierda, 2 abajo, 3 arriba, 4 disparo):

```
10 DEF FN b(j,n)=INT (j/2^n)-2*INT (j/2^(n+1))
20 CLS
30 REPEAT
40 LET j=JOY 0
50 PRINT AT 0,0;"Derecha ";FN b(j,0);" Izquierda ";FN b(j,1);" Disparo ";FN b(j,4)
60 WAIT 5
70 UNTIL INKEY$<>""
80 PAUSE 0
```

Para manejar un juego con el teclado y con el mando, júntalos con `OR`:
`IF KEY 9 OR FN b(j,0) THEN ...`. En el emulador, un mando conectado al PC
funciona como el mando 0.

**Si conoces el Spectrum:** el mando también se ve como las teclas de los
números, como hacía el interfaz de Sinclair (6 izquierda, 7 derecha, 8 abajo,
9 arriba, 0 disparo), así que los programas antiguos que leen el teclado con
`INKEY$` o `IN` se pueden jugar con él. `KEY` solo ve el teclado de verdad.
El ratón se lee con `MOUSE`; mira la referencia.

## Pruébalo

1. En el programa de la `@`, deja un rastro: en vez de borrar la `@`, escribe
   un `.` donde estaba.
2. Convierte el juego de los reflejos en un concurso: juega cinco rondas y
   escribe el mejor tiempo.

<details>
<summary>Pista</summary>

Para el primero, cambia el `" "` de la línea 50. Para el segundo, mete las
líneas 20 a 80 dentro de un bucle `FOR`, guarda el mejor tiempo en una
variable que empiece muy grande (`LET mejor=1000`) y actualízala con
`IF t<mejor THEN LET mejor=t`.

</details>

---

**Referencia:** [`INKEY$`](../reference.md#inkey), [`KEY`](../reference.md#key),
[`JOY`](../reference.md#joy), [`WAIT`](../reference.md#wait),
[`TIME`](../reference.md#time), [`PAUSE`](../reference.md#pause),
[`MOUSE`](../reference.md#mouse).

**Siguiente:** [12. Sprites y mapas de tiles](12-sprites-and-tile-maps.md) ·
**Anterior:** [10. Sonido y música](10-sound-and-music.md)
