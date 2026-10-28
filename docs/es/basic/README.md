[English](../../en/basic/README.md) · **Español**

# El BASIC de ZX-V

Cuando enciendes ZX-V, está lista para que escribas en **BASIC**, un lenguaje
de programación pensado para aprender. Escribes una línea, pulsas Intro y la
máquina la hace. Unas líneas más y tienes un programa: un dibujo, una
melodía, un juego.

El BASIC de ZX-V es el del ZX Spectrum, el ordenador doméstico de los años
ochenta, con muchas cosas más. Funcionan los programas escritos para el
Spectrum 48K y la música con `PLAY` del Spectrum 128, y añade lo que le
faltaba:

- **Programación estructurada**: bucles `WHILE` y `REPEAT`, `IF` ... `ELSE`,
  procedimientos con parámetros y variables locales, etiquetas en lugar de
  números de línea y captura de errores con `ON ERROR`.
- **Gráficos para juegos**: sprites, mapas de tiles con scroll, figuras
  rellenas, dos páginas de pantalla para animar sin parpadeo y un modo de
  alta resolución de 512 × 384 píxeles. Cada píxel tiene su propio color.
- **Sonido**: el beeper y dos chips de sonido con seis voces para música y
  efectos.
- **Ficheros**: programas y datos en una tarjeta SD, en carpetas, y una
  unidad `ROM:` con ejemplos.
- **Un editor a pantalla completa**, donde las palabras clave se escriben
  letra a letra y te mueves por el listado con las flechas.

## El curso

El curso enseña BASIC desde cero: no hace falta haber programado antes ni
haber visto un Spectrum. Cada capítulo se apoya en los anteriores, con
programas pequeños para escribir y probar, y ejercicios al final.

| Capítulo | Qué aprendes |
|---|---|
| [1. Primeros pasos](tutorial/01-first-steps.md) | Escribir una línea, `PRINT`, `RUN`, el editor en dos minutos, los mensajes de error |
| [2. Variables y cuentas](tutorial/02-variables-and-maths.md) | `LET`, operaciones y funciones, ordenar lo que sale con `PRINT`, números al azar |
| [3. Texto y preguntas](tutorial/03-text-and-input.md) | Cadenas de texto, unirlas y cortarlas, preguntar con `INPUT` |
| [4. Decisiones](tutorial/04-decisions.md) | `IF`, `THEN`, `ELSE`, comparaciones, `GO TO` y etiquetas, un juego de adivinar |
| [5. Bucles](tutorial/05-loops.md) | `FOR`, `WHILE`, `REPEAT`, bucles dentro de bucles, `PAUSE` |
| [6. Matrices y datos](tutorial/06-arrays-and-data.md) | `DIM`, listas de valores con `READ` y `DATA` |
| [7. Procedimientos](tutorial/07-procedures.md) | `DEF PROC`, parámetros, `LOCAL`, recursión, `DEF FN`, `GO SUB` |
| [8. Los colores y la pantalla](tutorial/08-colours-and-the-screen.md) | `INK`, `PAPER`, `BORDER`, `PRINT AT`, `MODE 1` |
| [9. Dibujar](tutorial/09-drawing.md) | `PLOT`, `DRAW`, `CIRCLE` y figuras rellenas con `BOX`, `DISC`, `POLY` |
| [10. Sonido y música](tutorial/10-sound-and-music.md) | `BEEP`, música con `PLAY`, efectos con `SOUND` |
| [11. Teclado y mando](tutorial/11-keyboard-and-joystick.md) | `INKEY$`, `KEY`, `JOY`, velocidad constante con `WAIT` y `TIME` |
| [12. Sprites y mapas de tiles](tutorial/12-sprites-and-tile-maps.md) | `GRAB`, `SPRITE`, animar sin parpadeo, `HIT`, tiles, mapas y scroll |
| [13. Ficheros y la tarjeta SD](tutorial/13-files-and-the-sd-card.md) | `SAVE`, `LOAD`, carpetas, la unidad `ROM:`, ficheros de texto, `ON ERROR` |
| [14. Un juego completo](tutorial/14-a-complete-game.md) | *Atrapa estrellas*, paso a paso |

## Más sobre el BASIC

- [La referencia del BASIC](reference.md): cada orden y cada función, con su
  sintaxis, sus detalles y ejemplos, y los mensajes de error.
- [El editor](editor.md): todas las teclas, el menú EDIT, renumerar y las
  demás órdenes para editar.
- [Primeros pasos](../getting-started.md): cómo arrancar ZX-V, el emulador o
  la máquina real, por primera vez.
