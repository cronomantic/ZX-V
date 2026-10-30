[English](../../../en/basic/tutorial/10-sound-and-music.md) · **Español**

# 10. Sonido y música

**En este capítulo aprenderás** a hacer pitidos sencillos con `BEEP`, a
tocar melodías de hasta seis voces con `PLAY` y a crear efectos de sonido
para juegos con `SOUND`.

ZX-V tiene dos clases de sonido: el **beeper**, un altavoz sencillo como el
del primer Spectrum, y dos **chips de sonido** (llamados AY), como el del
Spectrum 128, que pueden tocar tres notas a la vez cada uno.

## `BEEP`

`BEEP duración,tono` toca una nota. La duración va en segundos, y el tono es
el número de semitonos por encima del do central (0 es el do central, 12 el
do una octava más arriba y -12 una más abajo):

```
10 BEEP 1,0
20 BEEP 0.5,12
30 BEEP 0.5,-12
```

El programa espera mientras suena la nota. Con `READ` y `DATA` puedes tocar
una escala:

```
10 FOR i=1 TO 8
20 READ t
30 BEEP 0.25,t
40 NEXT i
50 DATA 0,2,4,5,7,9,11,12
```

## `PLAY`: música en una cadena

`PLAY` usa los chips de sonido y toma la música escrita en una cadena. Las
cadenas más sencillas son solo los nombres de las notas, en el sistema
inglés:

```
10 PLAY "cdefgabC"
```

Es una escala: `c` es do, `d` es re, `e` mi, `f` fa, `g` sol, `a` la y `b`
si; una letra mayúscula es una octava más alta. Estos son los signos más
útiles:

| En la cadena | Significado |
|---|---|
| `c d e f g a b` | Notas; en mayúsculas, una octava más arriba |
| `#c`, `$c` | Sostenido y bemol (el signo va delante de la nota) |
| `&` | Un silencio |
| `1` a `9` | La duración de las notas siguientes: `3` corchea, `5` negra (la de partida), `7` blanca, `9` redonda |
| `O n` | Octava, de 0 a 8 (empieza en la 5, donde `c` es el do central) |
| `T n` | Tempo, en negras por minuto (de 60 a 240; empieza en 120) |
| `V n` | Volumen, de 0 a 15 |

Una duración vale para las notas siguientes hasta que la cambies. Los
espacios no cuentan, así que úsalos para que la melodía se lea mejor:

```
10 LET m$="T100 O4 5ccgg aa7g 5ffee dd7c"
20 PLAY m$
```

### Varias voces

Si le das a `PLAY` varias cadenas, separadas por comas, suenan a la vez: una
melodía y su acompañamiento.

```
10 LET a$="T120 O5 3cegCegCe 7C"
20 LET b$="O3 7c&e&"
30 PLAY a$,b$
```

Las tres primeras cadenas usan las tres voces del primer chip; la cuarta,
quinta y sexta, las del segundo. Seis voces a la vez:

```
10 PLAY "cdefg","efgab","gabCD","O3c","O3e","O3g"
```

El tempo es el de la primera cadena. `PLAY` espera a que acaben todas las
cadenas; **Esc** lo corta.

**Si conoces el Spectrum:** `PLAY` es el del Spectrum 128, con las mismas
cadenas, ampliado al segundo chip. La [referencia](../reference.md#play)
tiene el resto de los signos: envolventes, repeticiones, tresillos y más.

## Efectos de sonido con `SOUND`

Los juegos necesitan sonidos que no son notas: explosiones, saltos, disparos.
`SOUND` habla directamente con el chip de sonido. Escribe un valor en uno de
sus **registros**, sus mandos de control: `SOUND registro,valor`. Puedes
escribir varios a la vez, separados por `;`. Para empezar bastan estos:

| Registro | Qué controla |
|---|---|
| 0 y 1 | El tono de la voz A: es un **periodo**, así que los números más grandes son notas más graves. El registro 0 va de 0 a 255, y el 1 suma pasos de 256 |
| 6 | El tono del ruido (de 0 a 31) |
| 7 | El **mezclador**: qué voces tocan tono y cuáles ruido. 62 quiere decir "la voz A toca un tono"; 55, "la voz A toca ruido" |
| 8 | El volumen de la voz A (de 0 a 15), o 16 para que lo lleve la **envolvente** |
| 11 y 12 | Lo que tarda la envolvente |
| 13 | La forma de la envolvente: 0 se apaga una vez |

Un zumbido que sube:

```
10 REM Un zumbido que sube, en el canal A
20 SOUND 7,62;8,15
30 FOR p=200 TO 20 STEP -6
40 SOUND 0,p;1,0
50 WAIT 1
60 NEXT p
70 SOUND 8,0
```

La línea 20 enciende el tono de la voz A a todo volumen, el bucle baja el
periodo (así que la nota sube) una vez cada sesentavo de segundo y la línea
70 baja el volumen a 0. `WAIT 1` espera al siguiente frame de la pantalla:
lo volverás a ver en los próximos capítulos.

Una explosión: ruido que se apaga solo, gracias a la envolvente:

```
10 REM Una explosion: ruido en el canal A, con la envolvente
20 SOUND 6,31;7,55;8,16;11,0;12,40;13,0
30 PAUSE 100
```

Al contrario que `BEEP` y `PLAY`, `SOUND` no espera: el programa sigue
mientras toca el chip. Eso es justo lo que necesita un juego. El sonido sigue
hasta que lo cambies, así que acuérdate de callarlo con `SOUND 8,0`. Los
registros del 16 al 31 son los mismos del segundo chip.

## Música de verdad: `MUSIC` y `SFX`

Para la música de un juego se usa un *tracker*, un programa para componer
como los que usaban los músicos del Spectrum. ZX-V trae uno para el PC,
[`zxv-tracker`](../../sdk/README.md#el-tracker-para-el-pc-zxv-tracker),
que guarda la canción en un fichero `.pt3` y los efectos de sonido en un
`.afb`. El BASIC los toca **de fondo**, mientras el programa sigue:

```
10 LOAD "tema" MUSIC: MUSIC PLAY
20 LOAD "efectos" SFX
30 IF INKEY$=" " THEN SFX 0
40 GO TO 30
```

`MUSIC STOP` para la música y `SFX n` toca el efecto número `n` sin
callarla. Todo lo demás está en la referencia: [`MUSIC`](../reference.md#music)
y [`SFX`](../reference.md#sfx).

## Pruébalo

1. Toca con `PLAY` el principio de una canción que conozcas.
2. Haz un sonido de "moneda" para un juego: dos notas agudas y rápidas, una
   detrás de otra, con `SOUND`.

<details>
<summary>Pista</summary>

Para la moneda, enciende la voz A (`SOUND 7,62;8,15`), pon un periodo como
60, espera unos frames con `WAIT 4`, pon un periodo más pequeño (una nota
más aguda) como 40, espera otra vez y baja el volumen con `SOUND 8,0`.

</details>

<details>
<summary>Una solución para el segundo</summary>

```
10 SOUND 7,62;8,15
20 SOUND 0,60;1,0: WAIT 4
30 SOUND 0,40;1,0: WAIT 8
40 SOUND 8,0
```

</details>

---

**Referencia:** [`BEEP`](../reference.md#beep), [`PLAY`](../reference.md#play),
[`SOUND`](../reference.md#sound), [`WAIT`](../reference.md#wait),
[`MUSIC`](../reference.md#music), [`SFX`](../reference.md#sfx).

**Siguiente:** [11. Teclado y mando](11-keyboard-and-joystick.md) ·
**Anterior:** [9. Dibujar](09-drawing.md)
