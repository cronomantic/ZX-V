[English](../en/README.md) · **Español**

# Documentación de ZX-V

ZX-V es un ordenador de 32 bits para aficionados, inspirado en el ZX Spectrum:
lo enciendes y estás en el BASIC. Estas páginas explican cómo usarlo, cómo
programarlo en BASIC o en C y cómo construir la máquina real.

## Para empezar

- **[Primeros pasos](getting-started.md)**: consigue el emulador, escribe tu
  primer programa y carga los ejemplos que trae.
- **[El emulador](emulator.md)**: las versiones de escritorio y web: menús,
  teclas, la carpeta que hace de tarjeta SD, instantáneas y opciones.
- **[La máquina](machine.md)**: cómo es ZX-V por dentro: la pantalla y sus
  colores, el sonido, la entrada, la memoria y los ficheros que usa.

## Programar en BASIC

- **[Aprende BASIC](basic/README.md)**: un curso desde cero, sin haber
  programado nunca ni conocer el Spectrum. Acaba con un juego completo.
- **[El editor](basic/editor.md)**: escribir y corregir programas, las teclas
  y el menú de opciones.
- **[Referencia del BASIC](basic/reference.md)**: cada orden, cada función y
  cada mensaje de error.

## Programar en C

- **[Programas nativos (SDK)](sdk/README.md)**: las herramientas, tu primer
  programa, gráficos, sonido, entrada y ficheros.
- **[Gráficos](sdk/graphics.md)**: el motor gráfico a fondo: la memoria de
  vídeo, sprites, mapas de tiles, scroll, el modo 7 y convertir imágenes.
- **[Referencia de la API](sdk/reference.md)**: cada función del SDK.

## Compilar BASIC

- **[ZX Basic](zxbasic.md)**: el compilador de BASIC de Boriel, con un target
  para ZX-V: programas en BASIC que van como los nativos.

## La máquina real

- **[Construir el hardware](hardware/README.md)**: qué hace falta y cómo se
  conecta todo.
- **[La placa](hardware/board.md)**: pedir la placa montada y las piezas que
  sueldas tú.
- **[El firmware](hardware/firmware.md)**: poner el software en las dos Pico.
- **[En una protoboard](hardware/protoboard.md)**: montarla sin la placa.

## Ejemplos

- [Un juego de plataformas en BASIC](../../examples/basic/platformer/README.es.md):
  sprites, un mapa de tiles, scroll y música.
- [Rock, Paper, Scissors en BASIC](../../examples/basic/rps/README.es.md):
  el IF de bloque, `SELECT CASE` y procedimientos, en un listado que se carga
  en texto.
- [Sentry, defensa contra misiles en BASIC](../../examples/basic/sentry/README.es.md):
  el ratón, los gráficos rápidos y explosiones en cadena, a 50 imágenes por
  segundo.
- [Estela, un matamarcianos en C](../../sdk/examples/estela/README.es.md): un juego nativo
  completo.
- [Route 128, un juego de carreras en C](../../sdk/examples/route128/README.es.md): una
  carretera en pseudo-3D con curvas y rasantes, y sprites escalados.
- [Muncher en BASIC](../../examples/basic/muncher/README.es.md): un
  laberinto con cuatro perseguidores, cada uno con su manera de buscarte, con
  un mapa de tiles y sprites a 50 imágenes por segundo.
- [The Mine, un juego de cavar en C](../../sdk/examples/mine/README.es.md):
  piedras y diamantes que caen y ruedan, en cuevas más grandes que la
  pantalla.
- [Street Justice, un juego de peleas en C](../../sdk/examples/street/README.es.md):
  por las calles contra los matones de las bandas, con luchadores dibujados
  por el motor a partir de un esqueleto.
- [Stardust, un juego en ZX Basic](../../examples/zxbasic/stardust/README.es.md):
  doble buffer, sprites y choques con `zxv.bas`, a 50 imágenes por segundo.

---

Esta documentación tiene licencia [CC BY 4.0](../LICENSE); sus ejemplos de código son
de dominio público (CC0): puedes copiarlos en tus programas sin más.
