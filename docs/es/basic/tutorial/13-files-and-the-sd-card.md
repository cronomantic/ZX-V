[English](../../../en/basic/tutorial/13-files-and-the-sd-card.md) · **Español**

# 13. Ficheros y la tarjeta SD

**En este capítulo aprenderás** a grabar y cargar tus programas, a
ordenarlos en carpetas, a usar los ejemplos de serie de la unidad `ROM:`, y a
escribir y leer tus propios ficheros de texto, por ejemplo para guardar el
récord de un juego. También aprenderás a atrapar los errores con
`ON ERROR`.

## La tarjeta SD

ZX-V guarda sus ficheros en una **tarjeta SD**, igual que el Spectrum los
guardaba en cintas de casete. En el emulador, la tarjeta SD es una carpeta
de tu PC: la carpeta **ZX-V** dentro de *Documentos* (la versión web la
guarda en el navegador). La [guía del emulador](../../emulator.md) lo cuenta
con detalle.

## Grabar y cargar programas

Teclea un programa corto y grábalo con `SAVE` y un nombre:

```
10 PRINT "Me han grabado y cargado!"
SAVE "hola"
```

El programa está ahora en el fichero `hola.tap` de la tarjeta: ZX-V añade
solo la terminación `.tap`. `CAT` lista los ficheros de la tarjeta:

```
NEW
CAT
```

`NEW` borra el programa de la memoria, pero el fichero sigue en la tarjeta.
Cárgalo otra vez y ejecútalo:

```
LOAD "hola"
RUN
```

Si añades `LINE` y un número de línea al grabar, el programa arrancará solo,
desde esa línea, cada vez que se cargue:

```
SAVE "hola" LINE 10
```

**Si conoces el Spectrum:** los ficheros `.tap` son los mismos ficheros de
cinta del Spectrum, así que puedes abrir tus programas de ZX-V en un
emulador del Spectrum, y cargar programas en BASIC de los `.tap` y `.tzx`
del Spectrum.

## Carpetas

Cuando tengas más programas, puedes meterlos en **carpetas**:

| Orden | Qué hace |
|---|---|
| `MKDIR "juegos"` | Crea la carpeta `juegos` |
| `CD "juegos"` | Entra en ella: desde ese momento, `SAVE`, `LOAD` y `CAT` trabajan ahí |
| `CD ".."` | Vuelve a la carpeta de arriba |
| `CD "/"` | Vuelve a la carpeta principal de la tarjeta |
| `PRINT DIR$` | Enseña la carpeta actual |
| `CAT "*.tap"` | Lista solo los ficheros cuyo nombre acaba en `.tap` (`*` quiere decir "cualquier cosa") |
| `ERASE "viejo.tap"` | Borra un fichero. Cuidado: no se puede deshacer |

También puedes poner la carpeta en el nombre: `LOAD "juegos/hola"`.

## La unidad `ROM:`: los ejemplos de serie

Además de la tarjeta, ZX-V tiene una segunda unidad en la que no se puede
escribir, la **unidad `ROM:`**, con programas de ejemplo que vienen con la
máquina:

```
CAT "ROM:"
```

Para cargar uno, pon `ROM:` delante de su nombre: `LOAD "ROM:graficos"`. Un
programa que carga más ficheros, como el juego de plataformas, los busca en
la carpeta actual, así que es mejor entrar antes en la unidad: `CD "ROM:"` y
luego `LOAD "platformer"`. `CD "SD:"` te devuelve a la tarjeta. Si intentas
grabar en la unidad `ROM:`, sale `V Error de disco (no se puede)`.

## Ficheros de texto

Un programa también puede escribir sus propios ficheros: una lista, un
diario, los récords. Los ficheros se abren en un **stream**, un canal
numerado del 4 al 15.

- `OPEN #4,"notas.txt" OUT` crea el fichero (o lo vacía, si ya existe) para
  escribir en él.
- `PRINT #4;...` escribe en el fichero igual que `PRINT` escribe en la
  pantalla. Una coma empieza una línea nueva en el fichero.
- `OPEN #4,"notas.txt" IN` lo abre para leerlo.
- `INPUT #4;...` lee del fichero, una línea para cada variable. Con
  `LINE a$`, la línea entera va a `a$`.
- `CLOSE #4` lo cierra. Cierra siempre los ficheros que abras.

```
10 OPEN #4,"notas.txt" OUT
20 PRINT #4;"Lista de la compra"
30 PRINT #4;"pan","leche"
40 CLOSE #4
50 OPEN #4,"notas.txt" IN
60 INPUT #4;LINE a$,LINE b$,LINE c$
70 CLOSE #4
80 PRINT a$: PRINT b$: PRINT c$
```

El fichero `notas.txt` es un fichero de texto normal: puedes abrirlo en el PC
con cualquier editor de textos.

## Cuando algo sale mal: `ON ERROR`

Leer un fichero que no existe para el programa con
`T Fichero no encontrado`. Un buen programa no debería pararse por eso.
`ON ERROR GO TO línea` le dice a la máquina adónde saltar si hay un error, en
vez de pararse:

```
10 ON ERROR GO TO 100
20 LOAD "nada"
30 STOP
100 PRINT "Error ";ERR;" en la linea ";ERL
```

`ERR` es el código del error (el 29 es `T Fichero no encontrado`; la
[referencia](../reference.md#mensajes-de-error) tiene la lista entera) y
`ERL` es la línea en la que ha pasado. `ON ERROR GO TO 0` deja de atrapar
errores. Solo atrapa uno: después de un error, vuelve a poner `ON ERROR` si
quieres seguir atrapándolos.

## Guardar el récord

Aquí está todo junto: un programa que se acuerda del récord de una vez para
otra, en el fichero `record.txt`:

```
10 LET record=0
20 ON ERROR GO TO 40
30 OPEN #4,"record.txt" IN: INPUT #4;record
40 ON ERROR GO TO 0: CLOSE #4
50 PRINT "El record es ";record
60 INPUT "Tus puntos? ";p
70 IF p>record THEN PRINT "Nuevo record!": OPEN #4,"record.txt" OUT: PRINT #4;p: CLOSE #4
```

La primera vez, `record.txt` no existe: la línea 30 falla, el programa salta
a la 40 y `record` se queda en 0. Desde entonces, lee los puntos que se
guardaron la última vez. Ejecútalo dos veces y verás.

## Pruébalo

1. Crea una carpeta `pruebas`, graba un programa dentro y cárgalo desde la
   carpeta principal.
2. Escribe un "libro de visitas": el programa pide un nombre y lo añade a un
   fichero; luego escribe todos los nombres del fichero.

<details>
<summary>Pista</summary>

`OUT` vacía el fichero, así que para añadir un nombre, lee primero todos los
nombres (en una matriz, por ejemplo) y luego escríbelos otra vez junto con el
nuevo. Para leer hasta el final del fichero, usa `ON ERROR`: cuando no quedan
líneas, `INPUT #4` da el error 8, `Fin del fichero`.

</details>

---

**Referencia:** [Ficheros y la tarjeta SD](../reference.md#ficheros-y-la-tarjeta-sd),
[`SAVE`](../reference.md#save), [`LOAD`](../reference.md#load),
[`CAT`](../reference.md#cat), [`CD`, `DIR$`](../reference.md#cd-dir),
[`OPEN`, `CLOSE`](../reference.md#open-close),
[`ON ERROR`](../reference.md#on-error), [`ERR`, `ERL`](../reference.md#err-erl).

**Siguiente:** [14. Un juego completo](14-a-complete-game.md) ·
**Anterior:** [12. Sprites y mapas de tiles](12-sprites-and-tile-maps.md)
