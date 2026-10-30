[English](../en/emulator.md) · **Español**

# El emulador

El emulador es ZX-V hecho software. Ejecuta la misma ROM (el BASIC) que la
máquina real, así que un programa que funciona en el emulador funciona en el
hardware, y al revés. Viene en varias formas:

| Programa | Qué es |
|---|---|
| **`zxv`** | La aplicación de escritorio, con menús, depurador y ajustes que se recuerdan. La que casi todo el mundo quiere |
| **`zxv-sdl`** | Una ventana sencilla sin menús, que se maneja con atajos de teclado |
| **`zxv-cli`** | Sin ventana: va tan rápido como puede, para guiones, pruebas y capturas |
| **La versión web** | La misma máquina en el navegador |
| **`sdimg`** | Una pequeña herramienta para crear y llenar imágenes de tarjeta SD |
| **`zxv-gfx`** | El editor de gráficos: sprites, tiles, mapas y texturas en ficheros `.gfx` (ver la [guía de gráficos del SDK](sdk/graphics.md#el-editor-de-gráficos-para-el-pc-zxv-gfx)) |
| **`zxv-tracker`** | El tracker: música para los dos AY en módulos PT3 y efectos AYFX, los que toca la biblioteca de música del SDK (ver la [guía del SDK](sdk/README.md#el-tracker-para-el-pc-zxv-tracker)) |

Los paquetes de Windows y Linux traen `zxv`, `zxv-sdl`, `zxv-cli`, `sdimg`, `zxv-gfx` y `zxv-tracker`,
además de los ejemplos (la carpeta `ejemplos`) y la unidad `ROM:`
(`romdisk.bin`). Cómo descargarlos se explica en
[Primeros pasos](getting-started.md#descarga).

## La aplicación de escritorio (`zxv`)

`zxv` abre una ventana con la pantalla de la máquina, menús y una barra de
estado. La imagen se escala sin deformarse al cambiar el tamaño de la ventana.
Lo que eliges (tamaño, tarjeta SD, ficheros recientes, opciones) se recuerda
para la próxima vez.

### Los menús

**Archivo**

| Opción | Qué hace |
|---|---|
| Abrir... (Alt+O) | Abre un programa, un listado, una instantánea o una imagen de tarjeta ([ver abajo](#abrir-ficheros)) |
| Volver a abrir (Alt+L) | Abre otra vez el último fichero (útil después de recompilar tu programa) |
| Recientes | Los ficheros abiertos últimamente |
| Grabar el programa... (Alt+G) | Graba el programa BASIC en la tarjeta: eliges el nombre y el emulador teclea el `SAVE` por ti (el BASIC tiene que estar en su editor, no ejecutando un programa) |
| Guardar una instantánea... (Alt+I) | Guarda todo el estado de la máquina en un fichero `.zxvs` |
| Cargar una instantánea... | Sigue desde una instantánea |
| Volver a la instantánea (Alt+U) | Vuelve a la última instantánea guardada o cargada |
| Tarjeta SD | Elige la tarjeta: una carpeta, una imagen de tarjeta, la de siempre (Documentos/ZX-V) o ninguna; y abre la carpeta de la tarjeta en el PC |
| Guardar la imagen (Alt+S) | Guarda la pantalla en PNG, en tu carpeta *Imágenes/ZX-V* |
| Grabar el sonido... | Graba todo lo que suena en un fichero `.wav`, hasta que vuelves a elegir la opción para parar |
| Salir (Alt+Q) | Cierra el emulador |

**Máquina**

| Opción | Qué hace |
|---|---|
| Parada (Alt+P) | Para la máquina o la deja seguir |
| Reiniciar (Alt+R) | Un reinicio completo, como apagar y encender: el programa que haya en memoria se pierde |
| Ctrl+Alt+Supr (vuelve al BASIC) | El reset de emergencia: para lo que esté en marcha y vuelve al BASIC, conservando tu programa BASIC |
| Pegar texto (Alt+V) | Teclea en la máquina el texto del portapapeles, como si lo escribieras tú |

**Ver**: el tamaño de la ventana (x1 es 640 × 480, hasta x4), el borde, el
escalado, la pantalla completa (Alt+Intro, sin menús) y la barra de estado. El borde es
lo que rodea el área de 512 × 384 en la que se dibuja: **Completo** enseña lo
que la máquina de verdad manda a la pantalla (640 × 480), **Pequeño**
(576 × 432) lo recorta y **Sin borde** deja solo el área de dibujo. Se recuerda para la próxima vez. **Escalado** dice cómo llena la
imagen la ventana cuando la agrandas: **Mantener la proporción** (lo normal:
tan grande como quepa sin deformarse), **Escala entera** (solo a x1, x2,
x3..., con todos los píxeles iguales) o **Llenar la ventana**, aunque la
imagen se deforme. Cuando no es una escala entera, la imagen se amplía con
los bordes de los píxeles suavizados, sin filas más gruesas que otras.

**Opciones**

| Opción | Qué hace |
|---|---|
| Idioma | Español, inglés o automático (el de tu sistema). Cambia los menús y la máquina ([ver abajo](#el-idioma)) |
| Sin sonido | Quita el sonido |
| CPU sin límite de velocidad | La máquina va todo lo deprisa que permita tu PC ([ver abajo](#la-velocidad)) |
| Modo estricto | Avisa de lo que haga un programa que la máquina real no permitiría (para programadores) |
| Ocultar el puntero del PC sobre la imagen | Para los programas que dibujan su propio puntero del ratón |
| Programas de fuera de la tarjeta | Qué hacer al abrir un programa que no está en la tarjeta SD: preguntar, copiarlo a la tarjeta o usar su carpeta como tarjeta |

**Depurar**: la ventana del depurador (Alt+D), cargar los símbolos de un
programa, una instrucción (Alt+N), hasta el final del frame (Alt+F), el
servidor GDB y los avisos del modo estricto. Ver [Depuración](#depuración).

**Ayuda**: el teclado y los atajos, esta documentación (la que trae el
paquete, que se lee sin conexión, o si no la de GitHub) y la versión.

### El teclado

Se teclea como en cualquier PC, con tu propia distribución de teclado: lo que
pone en la tecla es lo que sale. El BASIC y su editor usan directamente las
teclas del PC:

| Tecla | En ZX-V |
|---|---|
| Intro | Guarda la línea o ejecuta una orden |
| Esc | **BREAK**: para el programa en marcha. En el editor, deshace los cambios de la línea |
| Tab | **EDIT**: el menú de opciones del editor |
| Flechas, Inicio, Fin, RePág, AvPág, Ctrl+←/→ | Se mueven por el listado |
| Retroceso, Supr | Borran a la izquierda o bajo el cursor |
| Ctrl+Alt+Supr | Reset de emergencia: vuelve al BASIC conservando el programa |

Windows se queda Ctrl+Alt+Supr para sí, y algunos escritorios de Linux
también: si te pasa, usa **Máquina > Ctrl+Alt+Supr (vuelve al BASIC)**.

El editor se explica en [El editor](basic/editor.md).

Como Ctrl es de la máquina, todos los atajos del emulador van con **Alt**. La
tecla Alt sola no abre la barra de menús.

| Atajo | Qué hace |
|---|---|
| Alt+O, Alt+L | Abrir; volver a abrir el último fichero |
| Alt+G | Grabar el programa BASIC en la tarjeta |
| Alt+I, Alt+U | Guardar una instantánea; volver a ella |
| Alt+V | Pegar texto |
| Alt+R | Reinicio completo |
| Alt+P | Parar o seguir |
| Alt+S | Guardar la imagen |
| Alt+Intro | Pantalla completa |
| Alt+D | El depurador |
| Alt+N, Alt+F | Una instrucción; hasta el final del frame |
| Alt+Q | Salir |

**Las teclas del Spectrum.** Algunos programas, sobre todo los que vienen del
ZX Spectrum, leen el teclado como la rejilla de teclas del Spectrum (con `IN` o
`INKEY$`). Para ellos, el teclado del PC también pulsa las teclas del Spectrum:
las letras y las cifras son ellas mismas, Mayús es CAPS SHIFT, Ctrl es
SYMBOL SHIFT, Retroceso es DELETE (CAPS SHIFT + 0), Esc es BREAK
(CAPS SHIFT + SPACE) y las flechas son CAPS SHIFT + 5, 6, 7 y 8. Los signos
siguen al carácter que da tu tecla: la que escribe `"` pulsa SYMBOL SHIFT + P,
sea cual sea tu distribución. Normalmente no tienes que pensar en esto.

### El ratón y los mandos

**El ratón** no hay que capturarlo: mientras el puntero del PC está sobre la
imagen, el de la máquina va al mismo sitio, y los botones y la rueda son de la
máquina. Al salir de la imagen vuelve a ser el puntero de tu PC. Si un programa
dibuja su propio puntero, **Opciones > Ocultar el puntero del PC sobre la
imagen** quita el que sobra.

**Los mandos** funcionan en cuanto los conectas; el primero es el jugador 1 y
el segundo, el jugador 2. La cruceta o el stick izquierdo son las direcciones,
y los botones son:

| Mando (nombres de Xbox) | ZX-V |
|---|---|
| A | Fuego 1 |
| B | Fuego 2 |
| X | Fuego 3 |
| Start (Menú) | Start |
| Back (Vista) | Select |

En un mando de PlayStation son ✕, ○ y □. El teclado del PC no se convierte en
un mando; la mayoría de los juegos, como *Estela*, admiten las dos cosas.

### La tarjeta SD

La máquina guarda tus ficheros en su tarjeta SD. En `zxv` se elige en
**Archivo > Tarjeta SD**:

- **La de siempre (Documentos/ZX-V):** la de partida. La primera vez, `zxv`
  crea esta carpeta y copia en ella los ejemplos.
- **Una carpeta...:** cualquier carpeta de tu PC hace de tarjeta. Todo lo que
  grabe la máquina va ahí, y lo que copies ahí con el explorador de archivos
  está en la tarjeta. **Abrir la carpeta** la abre en el explorador.
- **Una imagen de tarjeta...:** un fichero `.img` con una tarjeta entera,
  formateada en FAT, FAT32 o exFAT. Puede ser una copia de una tarjeta de
  ZX-V de verdad, hecha con `dd` en Linux o con un programa como Win32 Disk
  Imager en Windows, y una imagen también se puede grabar en una tarjeta.
- **Sin tarjeta:** como si se hubiera sacado la tarjeta. El BASIC dice entonces
  `U No hay tarjeta SD`.

Una carpeta se comporta como una tarjeta de verdad: no se distinguen las
mayúsculas de las minúsculas (`JUEGO.TAP` y `juego.tap` son el mismo fichero) y
los programas no ven nada fuera de la carpeta. Los nombres solo pueden llevar
letras sin tildes, cifras y algunos signos, hasta 63 caracteres; los ficheros
con otros nombres no aparecen.

**`sdimg`** crea y llena imágenes de tarjeta desde la línea de órdenes:

```sh
sdimg new tarjeta.img 64 fat32            # una imagen nueva de 64 MB (fat, fat32 o exfat)
sdimg put tarjeta.img juego.tap /juego.tap   # copia un fichero a la imagen
sdimg get tarjeta.img /juego.tap juego.tap   # copia un fichero de la imagen
sdimg mkdir tarjeta.img /juegos           # crea una carpeta
sdimg ls tarjeta.img /                    # lista una carpeta, con las fechas
```

(Por ahora `sdimg` solo habla español.)

**La unidad `ROM:`**, con los ejemplos de serie, es el fichero `romdisk.bin` que
está junto al emulador. Si falta, `ROM:` está vacía.

### Abrir ficheros

**Archivo > Abrir...** (Alt+O), arrastrar un fichero a la ventana o darlo en la
línea de órdenes (`zxv juego.tap`) hacen lo mismo:

| Fichero | Qué pasa |
|---|---|
| `.zxv`, `.tap`, `.tzx` | La máquina se reinicia y el programa se carga con `LOAD`, igual que lo harías en la máquina. Si está en una subcarpeta de la tarjeta, el emulador entra antes en ella con `CD`, por si el programa carga más ficheros de ahí |
| `.bas` | Un listado BASIC en texto: como un programa, se carga con `LOAD` (el BASIC lo convierte en programa y comprueba cada línea) |
| `.txt` | Un texto, que se teclea solo en el editor, línea a línea: un listado, u órdenes como `RUN` |
| `.zxvs` | Una instantánea: la máquina sigue desde donde se guardó |
| `.img`, o una carpeta | Pasa a ser la tarjeta SD |

**Programas de fuera de la tarjeta.** Para cargar un programa, tiene que estar
en la tarjeta; si está en otro sitio, el emulador te pide que elijas:

- **Copiarlo a la tarjeta**, a su carpeta principal, junto con los ficheros de
  al lado que se llaman igual (como los gráficos `.gfx` de un juego).
- **Usar su carpeta** como tarjeta hasta que elijas otra. Lo que grabe el
  programa irá a esa carpeta.

**Opciones > Programas de fuera de la tarjeta** puede dejar hecha la elección
para siempre.

Un listado `.bas` es la forma más cómoda de escribir programas en tu editor de
textos favorito: un fichero de texto con líneas numeradas y las palabras clave
escritas con letras (`PRINT`, `GO TO`), en mayúsculas o no. También se carga
en la máquina de verdad, desde la tarjeta
([Listados en texto](basic/reference.md#listados-en-texto)). Cuando esté
cargado, escribe `RUN`.

### Instantáneas

Una **instantánea** (`.zxvs`) guarda el estado completo de la máquina: la
memoria, la pantalla, los chips de sonido, el reloj, el idioma y la carpeta
actual. Al cargarla, la máquina sigue exactamente desde ese momento, como si
nunca se hubiera parado. Sirve para guardar una partida a medias, o para volver
a un punto conocido mientras experimentas:

- **Alt+I** guarda una instantánea: pide el nombre del fichero y propone el
  último. **Alt+U** vuelve a la última instantánea guardada o cargada.
- **Archivo > Cargar una instantánea...**, Abrir o arrastrar el fichero a la
  ventana cargan cualquier instantánea.

Una instantánea **no** guarda la tarjeta SD: al cargarla, la tarjeta es la que
esté puesta, y los ficheros que tenía abiertos el programa se cierran (como con
Ctrl+Alt+Supr). Tampoco guarda los ajustes del emulador.

### Capturas y sonido

- **Alt+S** (Archivo > Guardar la imagen) guarda la pantalla tal como se ve,
  con el borde de **Ver > Borde**, en `zxv-FECHA-HORA.png` dentro de
  *Imágenes/ZX-V*.
- **Archivo > Grabar el sonido...** graba todo lo que suena en un fichero WAV
  (48 kHz, estéreo, 16 bits) hasta que lo paras.

No se graba vídeo; para eso, usa un programa de grabación de pantalla en tu PC.

### La velocidad

El emulador lleva la máquina a su velocidad real: 60 imágenes por segundo, con
un procesador de 144 MHz. **Opciones > CPU sin límite de velocidad** deja que
el procesador vaya todo lo rápido que pueda tu PC (hasta veinte veces más
trabajo en cada imagen), mientras la pantalla y el sonido siguen a sus 60
imágenes por segundo. Los programas que se sincronizan con la pantalla (casi
todos los juegos) se ven igual; los cálculos largos en BASIC terminan mucho
antes.

El emulador cuenta una instrucción por ciclo de reloj, así que un programa
nativo que cabe justo en una imagen en el emulador puede ir algo más lento en
el hardware. Cómo medirlo, en la [guía del SDK](sdk/README.md).

### El idioma

**Opciones > Idioma** elige español o inglés (o *Automático*, el de tu
sistema, o el inglés si no es ninguno de los dos). Cambia:

- los menús y las ventanas del emulador, la próxima vez que abras `zxv`;
- el idioma de la máquina (los mensajes y el editor del BASIC), al reiniciarla
  (Alt+R).

Un programa también puede cambiar el idioma de la máquina con `LANGUAGE "es"` o
`LANGUAGE "en"`; `zxv` recuerda esa elección, como hace la máquina real.

Se pueden añadir traducciones de los menús sin recompilar: un fichero
`zxv_XX.qm` en una carpeta llamada `idiomas`, junto al programa.

## La ventana sencilla (`zxv-sdl`)

`zxv-sdl` enseña la máquina en una ventana sin menús. Se teclea y se usa el
ratón como en `zxv`, y el emulador se maneja con estas teclas:

| Tecla | Qué hace |
|---|---|
| Alt+Intro | Pantalla completa |
| Alt+S | Guarda la imagen (`zxv-NNNN.bmp`, en la carpeta actual) |
| Alt+R | Reinicio completo |
| Alt+I, Alt+U | Guarda una instantánea (en `zxv.zxvs`, o en la última que se abrió); vuelve a ella |
| Alt+P, Alt+N, Alt+F | Parar o seguir; una instrucción; hasta el final del frame |
| Alt+D | Panel de depuración sobre la imagen: CPU, VRAM, sonido, tabla por línea, ninguno |
| Alt+B | El borde: completo (640 × 480), pequeño, ninguno (con el panel de depuración se ve siempre entero) |
| Ctrl+Alt+Supr | Reset de emergencia: vuelve al BASIC |
| Alt+Q | Salir |

`zxv-sdl` **no tiene tarjeta SD** salvo que le des una con `--sd CARPETA` o
`--sd-image FICHERO`. Si abres un programa de fuera, su carpeta pasa a ser la
tarjeta. Se pueden soltar ficheros en la ventana, como en `zxv`. Si no se le
dice otra cosa, la máquina y los mensajes van en el idioma del sistema.

## Opciones de la línea de órdenes

`zxv`, `zxv-sdl` y `zxv-cli` admiten las mismas opciones (`zxv-cli --help` las
lista todas). Estas son las que más vas a usar:

| Opción | Qué hace |
|---|---|
| `FICHERO` | Abre un fichero al arrancar, como Archivo > Abrir (`zxv`, `zxv-sdl`) |
| `--sd CARPETA` | Usa CARPETA como tarjeta SD |
| `--sd-image FICHERO` | Usa una imagen de tarjeta como tarjeta SD |
| `--romdisk FICHERO` | La unidad `ROM:` (si no, `romdisk.bin` junto al programa) |
| `--lang XX` | El idioma de la máquina, `es` o `en`, solo para esa vez. En `zxv-sdl` y `zxv-cli` es también el de los mensajes del emulador |
| `--scale N` | Tamaño de la ventana, de 1 a 4 (`zxv`, `zxv-sdl`) |
| `--border TAMAÑO` | El borde: `completo` (`full`, 640 × 480), `pequeño` (`small`) o `ninguno` (`none`) (`zxv`, `zxv-sdl`) |
| `--no-sound` | Sin sonido (`zxv`, `zxv-sdl`) |
| `--turbo` | CPU sin límite de velocidad |
| `--cpu-hz N` | La velocidad del procesador emulado, en Hz (144000000 si no se dice) |
| `--clock FECHA` | El reloj: `host` (la hora de tu PC, la de partida), `"2026-01-31 12:00:00"` (esa hora al arrancar, y luego avanza) u `off` (sin hora hasta que un programa la ponga) |
| `--type TEXTO` | Teclea TEXTO al arrancar. `\n` o `{ENTER}` es Intro; también `{ESC}`, `{TAB}`, `{BS}`, `{DEL}`, `{UP}`, `{DOWN}`, `{LEFT}`, `{RIGHT}`, `{HOME}`, `{END}`, `{PGUP}`, `{PGDN}`, `{F1}`...`{F12}`, `{CTRL+END}`, `{SHIFT+UP}`; `{WAIT n}` espera n imágenes, `{RESET}` pulsa Ctrl+Alt+Supr y `{JOY bits}` deja el mando 1 con esos botones (los de [`JOY`](basic/reference.md#joy)) hasta el siguiente `{JOY}`: `{JOY 1}{WAIT 60}{JOY 0}` lo lleva a la derecha un segundo |
| `--paste FICHERO` | Teclea el contenido de un fichero de texto (un listado BASIC, por ejemplo) después de `--type`, mucho más deprisa que tecla a tecla |
| `--load-snapshot F.zxvs` | Empieza desde una instantánea |
| `--save-snapshot F.zxvs` | Guarda una instantánea al terminar (`zxv-cli`) |
| `--frames N` | Termina tras N imágenes (60 por segundo) |
| `--screenshot F.bmp` | Guarda la pantalla al terminar |
| `--wav F.wav` | Graba el sonido en un fichero WAV |
| `--strict` | Modo estricto: avisa de lo que la máquina real no permitiría |
| `--version` | Escribe la versión de ZX-V (`ZX-V 0.6.0`) y termina. En Windows, `zxv` no tiene consola: su versión está en **Ayuda > Acerca de ZX-V** |

Las demás (`--rom`, `--elf`, `--load`, `--gdb`, `--debug`, `--profile`,
`--panel`, `--test`, `--fail-on-warning`) son para programadores y se explican
en la [guía del SDK](sdk/README.md).

### `zxv-cli`: la máquina sin ventana

`zxv-cli` no tiene ventana, ni salida de sonido, ni teclado: va tan rápido como
puede, teclea lo que le digas y guarda lo que le pidas. Sirve para guiones,
pruebas automáticas y hacer capturas para la documentación. Como `zxv-sdl`, no
tiene tarjeta salvo que le des una, y su máquina va en inglés salvo que uses
`--lang`.

Ejecutar un listado BASIC y hacer una captura a los seis segundos. `--paste`
teclea el fichero después de lo que se dé con `--type`, así que la última línea
de `programa.bas` debe ser `RUN`:

```sh
zxv-cli --lang es --paste programa.bas --frames 300 --screenshot resultado.bmp
```

Cargar un juego de la tarjeta, esperar y grabar diez segundos de su música:

```sh
zxv-cli --sd mitarjeta --type 'LOAD "juego"\n' --frames 500 --wav musica.wav
```

## La versión web

La versión web es la misma máquina funcionando en el navegador, con la misma
ROM, el mismo BASIC y la misma unidad `ROM:`. Descomprime `zxv-web.zip` y abre
`index.html`: funciona directamente desde el disco o desde cualquier servidor
web. Son cuatro ficheros, 1 MB en total (con los ejemplos y el disco ROM).

Haz clic en la imagen para que reciba el teclado. Los navegadores no dejan
sonar nada hasta que se pulsa una tecla o se hace clic, así que la máquina
arranca en silencio.

**Los menús** son como los de `zxv`, y los atajos con Alt también (con la
tecla Alt de la izquierda):

| Menú | Opciones |
|---|---|
| Archivo | Abrir... (Alt+O), La tarjeta SD..., Grabar el programa... (Alt+G), Guardar una instantánea (Alt+I), Volver a la instantánea (Alt+U), Descargar la instantánea, Guardar la imagen (Alt+S) |
| Máquina | Reiniciar (Alt+R), Ctrl+Alt+Supr (vuelve al BASIC), Parar/Seguir (Alt+P), Una instrucción (Alt+N), Panel de depuración (Alt+D) |
| Ver | Sonido sí o no, Cambiar el borde (Alt+B), Pantalla completa (Alt+Intro), Opciones..., Abrir en una pestaña aparte (solo cuando la página está dentro de otra) |
| Ayuda | El teclado y los atajos |

Ctrl+Alt+Supr está en el menú Máquina porque el sistema operativo se queda esas
teclas.

**La tarjeta SD** se guarda en el navegador, así que sigue ahí la próxima vez
que abras la página. La primera vez trae los ejemplos. **Archivo > La tarjeta
SD...** la enseña: puedes recorrer las carpetas, **subir ficheros** desde tu
PC, **descargar** ficheros a tu PC, crear carpetas, borrar ficheros y
abrirlos. Los ficheros que arrastras a la página se copian a la tarjeta (a la
carpeta que se ve en la ventana de la tarjeta) y se abre el primero. Lo que
graban los programas se guarda en el almacenamiento del navegador cada tres
segundos y al salir de la página.

La tarjeta vive solo en ese navegador: otro navegador, u otro ordenador, tiene
la suya. Si borras los datos del sitio en el navegador, la tarjeta se borra.
Descarga los ficheros que quieras conservar.

**Instantáneas**: Alt+I guarda una instantánea dentro de la página y Alt+U
vuelve a ella; **Archivo > Descargar la instantánea** la guarda en un fichero
`.zxvs`, que luego puedes abrir en la versión web o en `zxv`.

**Opciones** (Ver > Opciones...): el idioma (de la página y de la máquina; si
no se elige, el del navegador), el borde y el escalado (los mismos que en
`zxv`: la imagen ocupa todo el sitio de la página, manteniendo la proporción,
en múltiplos enteros o llenándolo), el modo estricto, la CPU sin límite de
velocidad y ocultar el puntero sobre la imagen. El idioma de la máquina cambia
al reiniciarla. El borde y el escalado se recuerdan, y Guardar la imagen
(Alt+S) guarda lo que se ve, con ese borde.

**Lo que no tiene:** las imágenes de tarjeta (`.img`), el servidor GDB y las
ventanas del depurador. El panel de depuración (Alt+D) sí está.

### Ponerla en tu web

El contenido de `zxv-web.zip` se puede subir tal cual a cualquier servidor web
o a sitios como **itch.io**: crea un proyecto de tipo *HTML*, sube el zip y
márcalo como *This file will be played in the browser*. La página se adapta al
tamaño que se le dé (960 × 720, por ejemplo), y Ver > Pantalla completa
funciona si activas el botón de pantalla completa de itch.io.

En sitios como itch.io la página va dentro de un marco de otro dominio, y
algunos navegadores (Safari, y puede que Firefox) no dejan guardar nada a esos
marcos. Entonces la tarjeta dura lo que la página abierta, y la página lo
avisa. **Ver > Abrir en una pestaña aparte** abre el emulador solo, donde la
tarjeta se guarda normalmente. Las descargas desde dentro de un marco dependen
de lo que permita el sitio.

## Depuración

El emulador es también un entorno de desarrollo completo para programas
nativos:

- en `zxv`, la ventana del **depurador** (Alt+D): desensamblado, puntos de
  ruptura, vigilancias, registros, memoria, pila, los chips de vídeo y de
  sonido, y los símbolos de tu programa;
- en `zxv-sdl` y en la versión web, un **panel de depuración** dibujado sobre
  la imagen;
- en `zxv-cli`, un depurador de consola (`--debug`);
- un **servidor GDB** (`--gdb PUERTO`, o Depurar > Servidor GDB... en `zxv`)
  para depurar programas en C con GDB, línea a línea;
- el **modo estricto**, que avisa de lo que fallaría en la máquina real.

Todo esto se explica en la [guía del SDK](sdk/README.md).

