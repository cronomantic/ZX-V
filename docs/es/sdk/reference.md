[English](../../en/sdk/reference.md) · **Español**

# Referencia del SDK

Todo lo que trae el SDK, agrupado por temas: las funciones del sistema (la
API), el motor gráfico, los registros y las constantes de la máquina, el
mapa de memoria y los formatos de fichero. Para aprender a usarlo, lee la
[guía del SDK](README.md) y [Gráficos](graphics.md). Las bibliotecas
opcionales de `sdk/lib` (cuentas en coma fija, teclado y mandos a la vez,
guardar datos, fuentes de letra, 3D...) no están en la ROM y tienen su
propia referencia: [Las bibliotecas del SDK](libraries.md).

**Contenido**

- [Los ficheros del SDK](#los-ficheros-del-sdk)
- [Convenciones](#convenciones)
- [Sistema y tiempo](#sistema-y-tiempo)
- [Pantalla y texto](#pantalla-y-texto)
- [Teclado, mando y ratón](#teclado-mando-y-ratón)
- [Sonido](#sonido)
- [Música y efectos](#música-y-efectos)
- [Ficheros y carpetas](#ficheros-y-carpetas)
- [Bancos](#bancos)
- [Reloj e idioma](#reloj-e-idioma)
- [Terminar un programa y ejecutar otros](#terminar-un-programa-y-ejecutar-otros)
- [Motor gráfico](#motor-gráfico)
- [Ficheros de gráficos `.gfx`](#ficheros-de-gráficos-gfx)
- [Registros de vídeo](#registros-de-vídeo)
- [Registros de entrada](#registros-de-entrada)
- [Registros de sonido](#registros-de-sonido)
- [Variables del sistema](#variables-del-sistema)
- [Códigos de error](#códigos-de-error)
- [Números de las funciones de la API](#números-de-las-funciones-de-la-api)
- [Mapa de memoria](#mapa-de-memoria)
- [El formato `.zxv`](#el-formato-zxv)
- [Herramientas](#herramientas)
- [La biblioteca estándar de C (picolibc)](#la-biblioteca-estándar-de-c-picolibc)
- [Definiciones internas](#definiciones-internas)

## Los ficheros del SDK

Todo está en la carpeta `sdk` del repositorio.

**Las cabeceras y el código que va en tu programa**

| Fichero | Contenido |
|---|---|
| `include/zxv.h` | Direcciones, registros y constantes de la máquina, solo con `#define`, así que sirve también en ensamblador (ficheros `.S`) |
| `include/zxv_api.h` | La API como funciones `static inline` de C, que llaman a la ROM por la [tabla de la API](#números-de-las-funciones-de-la-api); los canales de muestras, el puntero del ratón (`zxv_mouse_t`) y los tipos `zxv_dirent_t` y `zxv_clock_t`. Incluye `zxv.h`, `<stdint.h>` y `<stddef.h>` (así que `NULL` y `size_t` están disponibles) |
| `include/zxv_gfx.h` | El [motor gráfico](#motor-gráfico): tipos, constantes y funciones. Incluye `zxv_api.h` (define `ZXV_GFX_NO_API` antes para tener solo los tipos y las constantes) |
| `include/zxv_gfxfile.h`, `gfxfile.c` | Lee los [ficheros de gráficos `.gfx`](#ficheros-de-gráficos-gfx) (de `zxv-gfx`, el editor de la ROM y `png2gfx.py`): sprites, tiles, mapas y texturas, en los formatos de `zxv_gfx.h`, y pantallas |
| `include/zxv_music.h`, `music/` | [Música y efectos](#música-y-efectos): el formato propio `.zxm`, módulos PT3, TurboSound, PT2, STC, ASC, PSG y VGM, y efectos AYFX |
| `include/zxv_bank.h`, `zxvbank.c` | Los [bancos](#bancos) de datos: `zxv_bank_load` y `zxv_bank_size` |
| `zxvbankcode.c` | Los bancos de código: lo que hacen los puentes y `zxv_bank_preload`. Lo enlaza `tools/zxvbanks.py`, no hace falta ponerlo a mano |
| `include/zxv_version.h` | La versión de ZX-V con la que se publicó este SDK (`ZXV_VERSION`, por ejemplo `"0.12.0"`, y `ZXV_VERSION_MAJOR`, `_MINOR` y `_PATCH`); ver [`zxv_api_version`](#zxv_api_version) |
| `include/sys/dirent.h` | `struct dirent` y `DIR` para el `<dirent.h>` de picolibc |
| `lib/` y sus cabeceras | Las [bibliotecas opcionales](libraries.md): `zxv_math.h` (`lib/math.c`), `zxv_m7.h` (`lib/m7.c`), `zxv_mapmove.h` (`lib/mapmove.c`), `zxv_fade.h` (`lib/fade.c`), `zxv_input.h` (`lib/input.c`), `zxv_save.h` (`lib/save.c`), `zxv_inflate.h` (`lib/inflate.c`), `zxv_font.h` (`lib/font.c` y `lib/font_rom.c`) y `zxv_3d.h` (`lib/3d.c`) |
| `sound/` | El sonido de la máquina, el mismo código que suena en la Pico ULA, en el emulador y en `zxv-tracker`: los dos AY (`aysynth`), los doce canales de muestras (`smpsynth`, con el codificador de ADPCM `smp_adpcm_encode`) y la mezcla (`sndsynth`, con los registros de `sndregs.h`) |
| `crt0.S` | Código de arranque: borra las variables de la VRAM oculta (`ZXV_VRAM0_DATA`, `ZXV_VRAM1_DATA`), pone el registro `tp` (las variables de hilo, que usa el `errno` de picolibc) y salta a `main` |
| `zxv.ld` | Script de enlazado: todo seguido al principio de la RAM de usuario, las secciones de la VRAM oculta y el montón para picolibc |
| `zxvlib.c` | `memcpy`, `memmove`, `memset`, `memcmp`, para los programas sin la biblioteca de C |
| `zxvc.c` | La unión entre picolibc y la API (ficheros, carpetas, consola, montón, hora, salida): ver [La biblioteca estándar de C](#la-biblioteca-estándar-de-c-picolibc) |

**Las herramientas** (ver también [Herramientas](#herramientas))

| Fichero | Contenido |
|---|---|
| `tools/elf2zxv.py` | Convierte el `.elf` enlazado en un `.zxv`, con la pantalla de carga y los bancos de datos (`--bank`, `--pack`) |
| `tools/zxvbanks.py` | Compila un programa con bancos de código: compila cada banco aparte, genera los puentes, enlaza y llama a `elf2zxv.py` |
| `tools/zxvbanks-gdb.py` | Para GDB: carga y quita los símbolos de cada banco de código según se cargan en la máquina |
| `tools/png2gfx.py` | Convierte imágenes PNG en un fichero de gráficos `.gfx` |
| `tools/obj2c.py` | Convierte un modelo `.obj` (Blender y casi todos los programas de 3D) en C para `zxv_3d.h` |
| `tools/fontconv.py` | Convierte fuentes de mapa de bits (`.ch8` del Spectrum, `.psf`, `.bdf`, `.hex`, volcados...) en C para `zxv_font.h` |
| `tools/mkweb.py` | La página web de un juego solo (ver [Publicar tu juego en la web](web.md)) |
| `tools/gfx/` | `zxv-gfx`, el editor de gráficos para el PC |
| `tools/tracker/` | `zxv-tracker`, el tracker para el PC: música `.zxm` y PT3, y efectos AYFX |
| `tools/music/` | `zxv-music`, el conversor de PT3, TurboSound, MOD, S3M y XM al formato `.zxm` |
| `tools/link/` | `zxv-link` y `zxv-link-gui` (con ventana), la carga remota: mandan programas a la placa por la tira J5 y los ejecutan, y sirven una carpeta del PC como la unidad `EXT:` |

**Lo demás**

| Fichero | Contenido |
|---|---|
| `examples/` | Los ejemplos, uno por carpeta (`hello`, `graphics`, `samples`, `fases`...) |
| `Makefile` | Compila los ejemplos (`make`, `make run SD=carpeta`, `make debug SD=carpeta`) |
| `README.md`, `LICENSE` | Una presentación corta y la licencia (MIT): tus programas pueden usar el SDK como quieras |

## Convenciones

- Todas las funciones siguen la convención de llamada estándar de RISC-V
  `ilp32`. Las de `zxv_api.h` son `inline`: llaman a la ROM a través de la
  [tabla de la API](#números-de-las-funciones-de-la-api), o hacen su
  trabajo ellas mismas con los registros (el ratón, los canales de
  muestras).
- Las funciones que pueden fallar devuelven un `int`: **0 o más** si van
  bien y un [código de error](#códigos-de-error) **negativo** si fallan.
- «La página de dibujo» es la página de vídeo elegida con
  `zxv_gfx_draw_page` (la 0 al empezar un programa); «la página visible»
  es la elegida con `zxv_set_page`.
- Los registros son memoria: se leen y se escriben con punteros
  `volatile`, por ejemplo `*(volatile uint8_t *)(uintptr_t)ZXV_V_BORDER = 2;`.
  Los registros de vídeo se aplican en el siguiente VBLANK.
- Cada entrada dice **desde** qué versión de la especificación de la
  máquina existe y, si es una función de la API, su número. Un programa
  que use algo de la especificación 0.2 debe comprobar la versión de la
  ROM antes: ver [`zxv_api_version`](#zxv_api_version). Una función que la
  ROM no tiene no hace nada y devuelve −1. Lo que llegó en una ROM
  posterior a la primera versión publicada (la 0.4.0) lo dice también:
  «ROM 0.6.0 o posterior».
- Lo que no es de la ROM (los bancos, la música, las bibliotecas de
  `sdk/lib`) se compila con tu programa: no lleva «Desde», pero dice qué
  ROM necesita si hace falta una concreta.

## Sistema y tiempo

### `zxv_wait_vblank`

```c
void zxv_wait_vblank(void);
```

Espera al siguiente VBLANK, el principio de una imagen: hay 60 por segundo
(`ZXV_FRAME_HZ`). Es el reloj de un juego: haz el trabajo de una imagen y
espera a la siguiente.

Activa las interrupciones si estaban desactivadas (las necesita para
enterarse del VBLANK). **No la llames desde un gancho de frame**: el
gancho va con las interrupciones quitadas, y esperaría para siempre.

**Ejemplo**

```c
for (;;) {
    mover_todo();
    dibujar_todo();
    zxv_wait_vblank();              /* 60 vueltas por segundo */
}
```

**Desde:** especificación 0.1 · función n.º 1 de la API.

**Ver también:** [`zxv_set_frame_hook`](#zxv_set_frame_hook),
[`zxv_time_us`](#zxv_time_us), la variable del sistema `ZXV_SV_FRAMES`
([Variables del sistema](#variables-del-sistema)).

### `zxv_set_frame_hook`

```c
void zxv_set_frame_hook(void (*fn)(void));
```

Hace que el sistema llame a `fn` en cada VBLANK, desde la interrupción
(con las interrupciones quitadas), aunque el programa esté ocupado en otra
cosa. Sirve para lo que tiene que pasar en cada imagen sin falta: la
música, un contador.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `fn` | una función `void f(void)`, o `0` | La función que se llama en cada VBLANK; `0` quita la que hubiera. Solo hay un gancho: poner otro quita el anterior. |

**Reglas del gancho:**

- Que sea breve: mientras va, no hay más interrupciones.
- No puede llamar a `zxv_wait_vblank` ni a nada que la use (como
  `zxv_smp_upload`), ni cargar un banco de código: el programa pararía
  con un error.
- Las variables que comparta con el resto del programa deben ser
  `volatile`.
- Quítalo antes de que termine tu programa. Al volver al BASIC la ROM lo
  quita, pero no al volver al monitor del sistema.

**Ejemplo**

```c
static volatile uint32_t imagenes;

static void cada_imagen(void)
{
    imagenes++;
}

zxv_set_frame_hook(cada_imagen);
/* ... el juego ... */
zxv_set_frame_hook(0);              /* antes de terminar */
```

**Desde:** especificación 0.1 · función n.º 2.

**Ver también:** [`zxv_wait_vblank`](#zxv_wait_vblank),
[`zxv_music_update`](#zxv_music_update), la variable del sistema
`ZXV_SV_FRAME_HOOK`.

### `zxv_time_us`

```c
uint32_t zxv_time_us(void);
```

Un contador de microsegundos, que no se para ni se pone en hora. Sirve para
medir tiempos y para dar la hora a la [cola con
hora](#zxv_ay_event) del sonido.

**Devuelve:** los microsegundos del contador. Da la vuelta cada 2³² µs
(unos 71 minutos): compara tiempos restándolos como números sin signo, y
la resta sale bien aunque haya dado la vuelta en medio.

**Errores:** ninguno.

**Ejemplo**

```c
uint32_t t0 = zxv_time_us();
hacer_algo();
uint32_t tardado = zxv_time_us() - t0;     /* en µs, aunque dé la vuelta */
```

**Desde:** especificación 0.1 · función n.º 3.

**Ver también:** [`zxv_wait_vblank`](#zxv_wait_vblank),
[`zxv_clock_get`](#zxv_clock_get-zxv_clock_set) (la fecha y la hora).

### `zxv_api_version`

```c
uint32_t zxv_api_version(void);
```

La versión de la especificación de la máquina que cumple la ROM en la que
va el programa. Es la que dice qué funciones y qué variables del sistema
hay.

**Devuelve:** `mayor << 16 | menor`. La ROM actual da **3** (la versión
0.3); las anteriores, 2 (la 0.2) o 1 (la 0.1). Es lo mismo que la variable del sistema
`ZXV_SV_VERSION`. `ZXV_SPEC_VERSION` (3) es la versión de las cabeceras con
las que compilas, no la de la máquina.

**Errores:** ninguno.

**Comprobar la versión.** Un programa compilado con este SDK puede acabar
en una máquina con una ROM más antigua. Antes de usar lo que llegó con la
especificación 0.2 (la trama de las figuras, `zxv_m7_draw_map`,
`zxv_m7_affine`, los bancos, los mensajes de `zxv_report_error` de más de
47 bytes...), mira que la ROM lo tiene. Con una ROM 0.1, una función de la
0.2 no hace nada (devuelve −1), y los bancos dan `ZXV_E_ACCESS`. Lo mismo con
lo que llegó con la 0.3 (`JOY2`, `JOY3`, el fuego 4, L y R): con una ROM
anterior se lee a 0. La especificación 0.3 es la de ZX-V 0.12.0 y posteriores;
la 0.2, la de la 0.11.0; la 0.1, la de las anteriores.

**La versión de ZX-V no es esto.** `ZXV_VERSION` (de `zxv_version.h`, por
ejemplo `"0.12.0"`) es la versión de los programas de ZX-V (la ROM, el
emulador y el firmware, que se publican juntos) con la que salió este SDK.
Es un texto fijo que se compila dentro de tu programa: no dice qué ROM
tiene la máquina, y muchas versiones de ZX-V cumplen la misma
especificación. Para saber qué puede hacer la máquina, usa
`zxv_api_version`.

**Ejemplo**

```c
static const uint8_t trama[8] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };

if (zxv_api_version() >= 2)         /* especificación 0.2 o posterior */
    zxv_gfx_pattern(trama, 0);      /* figuras con trama */
```

**Desde:** especificación 0.1 · función n.º 0.

**Ver también:** [Constantes](#constantes),
[Números de las funciones de la API](#números-de-las-funciones-de-la-api).

### Constantes

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_FRAME_HZ` | 60 | Imágenes por segundo (los VBLANK) |
| `ZXV_CPU_HZ_REF` | 144000000 | Velocidad de referencia del procesador (Hz); la de verdad, en la variable del sistema `ZXV_SV_CPU_HZ` |
| `ZXV_SPEC_VERSION` | 3 | Versión de la especificación de estas cabeceras (0.3) |
| `ZXV_VERSION` | `"0.12.0"` | Versión de ZX-V con la que salió el SDK, en `zxv_version.h` (con `ZXV_VERSION_MAJOR`, `_MINOR`, `_PATCH`) |

## Pantalla y texto

### `zxv_set_mode`

```c
void zxv_set_mode(int mode);
```

Cambia el modo de vídeo desde el siguiente VBLANK y borra la página de
dibujo con el papel del texto (ver
[`zxv_text_colors`](#zxv_text_colors-zxv_text_at)). El cursor de texto
vuelve arriba a la izquierda.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `mode` | 0 o 1 | 0: 256 × 192 píxeles; 1: 512 × 384. Solo cuenta el bit 0 (2 es el modo 0, 3 el 1). |

El modo 1 usa la página de vídeo entera (96 KB): lo que el programa tenga
en la VRAM oculta (`ZXV_VRAM0_DATA`, `ZXV_VRAM1_DATA`, ver [Mapa de
memoria](#mapa-de-memoria)) se pierde. En el modo 0 con el campo de juego
virtual (`ZXV_V_VSIZE`), se borra la página virtual entera.

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_MODE0_W`, `ZXV_MODE0_H` | 256, 192 | La pantalla del modo 0 |
| `ZXV_MODE1_W`, `ZXV_MODE1_H` | 512, 384 | La del modo 1 |
| `ZXV_SCREEN_PIXELS(modo)` | 24.576 o 98.304 | Los bytes de los píxeles de una pantalla (4 bits por píxel) |
| `ZXV_SCREEN_SIZE(modo)` | 24.580 o 98.308 | Lo que ocupa una pantalla guardada sin paleta: la cabecera (`ZXV_SCREEN_HEAD`, 4) y los píxeles; con paleta, `ZXV_SCREEN_SIZE_F` (ver [El formato `.zxv`](#el-formato-zxv)) |

**Ejemplo**

```c
zxv_text_colors(7, 1);              /* el papel azul... */
zxv_set_mode(1);                    /* ...llena la pantalla de 512 x 384 */
```

**Desde:** especificación 0.1 · función n.º 4.

**Ver también:** [`zxv_set_page`](#zxv_set_page), [`zxv_cls`](#zxv_cls),
el registro `ZXV_V_MODE` ([Registros de vídeo](#registros-de-vídeo)).

### `zxv_set_page`

```c
void zxv_set_page(int page);
```

Elige la página de vídeo que se ve, desde el siguiente VBLANK (escribe
`ZXV_V_PAGE`). No cambia la página en la que se dibuja: para la doble
memoria, se dibuja en una y se enseña la otra.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `page` | 0 o 1 | La página. Solo cuenta el bit 0. |

**Ejemplo**

```c
int dibujo = 1;
for (;;) {
    zxv_gfx_draw_page(dibujo);      /* se dibuja en la que no se ve */
    dibujar_todo();
    zxv_set_page(dibujo);           /* y se enseña en el VBLANK */
    zxv_wait_vblank();
    dibujo ^= 1;
}
```

**Desde:** especificación 0.1 · función n.º 5.

**Ver también:** [`zxv_set_mode`](#zxv_set_mode),
`zxv_gfx_draw_page` ([Motor gráfico](#motor-gráfico)).

### `zxv_cls`

```c
void zxv_cls(int colour);
```

Llena la página de dibujo de un color y lleva el cursor de texto arriba a
la izquierda. No cambia los colores del texto.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `colour` | 0–15 | El color ([la paleta](#la-paleta)). Solo cuentan los 4 bits de abajo. |

En el modo 0 con el campo de juego virtual (`ZXV_V_VSIZE`) llena la página
virtual entera.

**Ejemplo**

```c
zxv_cls(1);                         /* todo azul */
```

**Desde:** especificación 0.1 · función n.º 6.

**Ver también:** [`zxv_set_mode`](#zxv_set_mode), `zxv_gfx_rect`
([Motor gráfico](#motor-gráfico)).

### `zxv_plot`

```c
void zxv_plot(int x, int y, int colour);
```

Pinta un píxel de la página de dibujo. Es sencilla y lenta: para dibujar
mucho, el [motor gráfico](#motor-gráfico).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x`, `y` | dentro de la página | El píxel, contando desde arriba a la izquierda. Un punto fuera de la página no se pinta (no es un error). |
| `colour` | 0–15 | El color. Solo cuentan los 4 bits de abajo. |

**Ejemplo**

```c
for (int x = 0; x < ZXV_MODE0_W; x++)
    zxv_plot(x, 96, 2);             /* una línea roja */
```

**Desde:** especificación 0.1 · función n.º 7.

**Ver también:** `zxv_gfx_line` ([Motor gráfico](#motor-gráfico)).

### `zxv_print_char`, `zxv_print_str`

```c
void zxv_print_char(int c);
void zxv_print_str(const char *s);
```

Escriben un carácter o una cadena en el cursor de texto de la página de
dibujo, en celdas de 8 × 8 píxeles, con los colores de `zxv_text_colors`,
y llevan el cursor detrás. Al llegar al final de la línea siguen en la
siguiente, y cuando el texto pasa de la última fila, la pantalla sube una
fila.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `c` | un código Unicode | El carácter (`'A'`, `0xF1` para la `ñ`...). |
| `s` | cadena en UTF-8 terminada en cero | El texto. UTF-8 es la codificación de los ficheros de C, así que puedes escribir `"¡Hola!"` en tu programa. |

Los caracteres que se dibujan:

| Carácter | Qué sale |
|---|---|
| ASCII del 32 al 126 | El carácter |
| `©` (0xA9) | `©` (también con el 127) |
| `á é í ó ú ü ñ`, `Á É Í Ó Ú Ü Ñ`, `¿`, `¡` | La letra, de la fuente de la ROM |
| `'\n'` o `'\r'` | Pasa al principio de la línea siguiente. Cada uno es un salto: `"\r\n"` deja una línea en blanco |
| `'\b'` | Borra el carácter anterior y retrocede (en la columna 0, nada) |
| Cualquier otro (`'\t'`, otros controles, otras letras) | `?` |

Una secuencia de UTF-8 mal formada sale como `?`.

Las filas y las columnas dependen del modo: 32 × 24 celdas en el modo 0 y
64 × 48 en el modo 1 (en el modo 0 con el campo de juego virtual, las de la
página virtual).

**Ejemplo**

```c
zxv_print_str("Vidas: ");
zxv_print_char('0' + vidas);
zxv_print_char('\n');
```

**Desde:** especificación 0.1 · funciones n.º 8 y 9.

**Ver también:** [`zxv_text_colors`, `zxv_text_at`](#zxv_text_colors-zxv_text_at),
las fuentes de [`zxv_font.h`](libraries.md#fuentes-de-letra-zxv_fonth).

### `zxv_text_colors`, `zxv_text_at`

```c
void zxv_text_colors(int ink, int paper);
void zxv_text_at(int col, int row);
```

`zxv_text_colors` elige los colores del texto que escriban desde entonces
`zxv_print_char` y `zxv_print_str`: cada celda se llena con el papel y el
carácter se dibuja con la tinta. `zxv_text_at` lleva el cursor de texto a
otra celda.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ink` | 0–15 | La tinta. Solo cuentan los 4 bits de abajo. |
| `paper` | 0–15 | El papel. También es el color con el que `zxv_set_mode` borra la pantalla. |
| `col` | 0–31 en el modo 0, 0–63 en el 1 | La columna, desde 0 a la izquierda. |
| `row` | 0–23 en el modo 0, 0–47 en el 1 | La fila, desde 0 arriba. |

Una posición fuera de la pantalla no es un error: lo que cae fuera no se
dibuja, y si el cursor está por debajo de la última fila, el carácter
siguiente hace subir la pantalla y sigue en la última.

Cada programa empieza con negro sobre blanco (0 y 7) y el cursor arriba a
la izquierda, deje lo que deje el anterior: la ROM los pone así al cargar
cada `.zxv`. `zxv_cls` y `zxv_set_mode` también llevan el cursor arriba a
la izquierda.

**Ejemplo**

```c
zxv_text_colors(14, 1);             /* amarillo brillante sobre azul */
zxv_text_at(8, 10);
zxv_print_str("¡Hola, señor Núñez!");
```

**Desde:** especificación 0.1 · funciones n.º 53 y 54.

**Ver también:** [`zxv_print_char`, `zxv_print_str`](#zxv_print_char-zxv_print_str),
[La paleta](#la-paleta).

### La paleta

| Número | Color | RGB | Número | Color | RGB |
|---|---|---|---|---|---|
| 0 | Negro | `000000` | 8 | Negro (transparente en el motor) | `000000` |
| 1 | Azul | `0000D7` | 9 | Azul brillante | `0000FF` |
| 2 | Rojo | `D70000` | 10 | Rojo brillante | `FF0000` |
| 3 | Magenta | `D700D7` | 11 | Magenta brillante | `FF00FF` |
| 4 | Verde | `00D700` | 12 | Verde brillante | `00FF00` |
| 5 | Cian | `00D7D7` | 13 | Cian brillante | `00FFFF` |
| 6 | Amarillo | `D7D700` | 14 | Amarillo brillante | `FFFF00` |
| 7 | Blanco | `D7D7D7` | 15 | Blanco brillante | `FFFFFF` |

Los bits del número son brillo, verde, rojo y azul (del bit 3 al 0).

Son los colores de las paletas al empezar cada programa. Cada tabla de
remapeo tiene su paleta, de 16 colores RGB565 (u16: rojo en los bits 15–11,
verde en los 10–5 y azul en los 4–0) en `ZXV_PALETTE(t)`, que se puede
cambiar: ver [Registros de vídeo](#registros-de-vídeo). `ZXV_RGB(r, g, b)`
da el RGB565 de un color de 0 a 255 por componente (215, `D7`, es el nivel
normal: `ZXV_RGB(215, 0, 0)` es el rojo) y `ZXV_STD_RGB565(i)`, el color de
siempre del índice `i`. Al cargar cada `.zxv`, la ROM vuelve a poner las
cuatro paletas y las tablas de remapeo como al principio.

## Teclado, mando y ratón

### `zxv_key_get`

```c
int zxv_key_get(void);
```

Saca la siguiente tecla de la cola de teclas. La cola guarda las pulsaciones
en orden (64 como mucho; si se llena, las nuevas se pierden), así que no se
pierde ninguna aunque el programa tarde en mirar. Para saber qué teclas
**están pulsadas** ahora (un juego), mira los [registros de
entrada](#registros-de-entrada) `ZXV_I_KEYMAP` o usa
[`zxv_input.h`](libraries.md#teclado-y-mandos-a-la-vez-zxv_inputh).

**Devuelve:** el código de la tecla (tabla), o −1 si la cola está vacía.

| Código | Tecla |
|---|---|
| El código Unicode del carácter | Una tecla que escribe un carácter, con la distribución del teclado (`'a'`, `'A'`, `'ñ'` = 0xF1, `'@'`...) |
| 0x0D, 0x08, 0x09, 0x1B | Intro, Retroceso, Tab, Esc |
| `ZXV_KEY_SPECIAL` (0xF000) \| código HID | Las teclas que no escriben (abajo) |

| Código | Tecla | Código | Tecla |
|---|---|---|---|
| 0xF03A–0xF045 | F1–F12 | 0xF04C | Supr |
| 0xF046 | Impr Pant | 0xF04D | Fin |
| 0xF047 | Bloq Despl | 0xF04E | Av Pág |
| 0xF048 | Pausa | 0xF04F | → |
| 0xF049 | Insert | 0xF050 | ← |
| 0xF04A | Inicio | 0xF051 | ↓ |
| 0xF04B | Re Pág | 0xF052 | ↑ |
| 0xF068–0xF073 | F13–F24 | | |

Las teclas mantenidas se repiten (a los 500 ms, y luego 20 veces por
segundo): las que escriben, Intro, Retroceso, Tab, Supr, Fin, Av Pág y los
cursores. Las letras con tilde no se pueden teclear: el acento y la letra
dan la letra sola. Ctrl+Alt+Supr no llega nunca (reinicia la máquina), ni
Ctrl+Alt+F12 (la página de diagnóstico de la placa).

**Errores:** ninguno.

**Ejemplo**

```c
int k;
while ((k = zxv_key_get()) >= 0) {
    if (k == 0x1B)
        salir = 1;                          /* Esc */
    else if (k == (int)(ZXV_KEY_SPECIAL | 0x52))
        subir();                            /* cursor arriba */
}
```

**Desde:** especificación 0.1 · función n.º 10.

**Ver también:** [Registros de entrada](#registros-de-entrada),
[`zxv_joy_read`](#zxv_joy_read).

### `zxv_joy_read`

```c
uint32_t zxv_joy_read(int n);
```

El estado de un mando USB: qué direcciones y botones están pulsados ahora
mismo. Es lo mismo que leer los registros `ZXV_I_JOY0` a `ZXV_I_JOY3`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0–3 | El mando: el primero que se conecta es el 0, el segundo, el 1, y así hasta el 3; cualquier otro valor da 0. El 2 y el 3, desde la especificación 0.3 (antes daban siempre 0); hasta ZX-V 0.11, cualquier valor distinto de 0 leía el 1. |

**Devuelve:** los bits de la tabla, a 1 los pulsados; `ZXV_JOY_CONNECTED`
dice que hay un mando. Un mando desconectado da 0.

| Constante | Bit | Constante | Bit |
|---|---|---|---|
| `ZXV_JOY_RIGHT` | 0 | `ZXV_JOY_START` | 7 |
| `ZXV_JOY_LEFT` | 1 | `ZXV_JOY_SELECT` | 8 |
| `ZXV_JOY_DOWN` | 2 | `ZXV_JOY_FIRE4` | 9 |
| `ZXV_JOY_UP` | 3 | `ZXV_JOY_L` | 10 |
| `ZXV_JOY_FIRE1` | 4 | `ZXV_JOY_R` | 11 |
| `ZXV_JOY_FIRE2` | 5 | `ZXV_JOY_CONNECTED` | 15 |
| `ZXV_JOY_FIRE3` | 6 | | |

`ZXV_JOY_FIRE4`, `ZXV_JOY_L` y `ZXV_JOY_R`, desde la especificación 0.3 (antes, a 0).

La dirección sale de la cruceta o del stick izquierdo (pasado un cuarto de
su recorrido). En un mando de PlayStation, ✕, ○, □ y △ son los fuegos 1 a
4, L1 es L y R1 es R; Share/Create es Select y Options, Start. En uno de
Xbox (360, One o Series), A, B, X e Y son los fuegos, LB y RB son L y R.
En un mando genérico, los botones 1–4 son los fuegos, el 5 es L, el 6 es
R, el 9 es Select y el 10, Start.

**Errores:** ninguno.

**Ejemplo**

```c
uint32_t j = zxv_joy_read(0);
if (j & ZXV_JOY_LEFT)
    x--;
if (j & ZXV_JOY_FIRE1)
    disparar();
```

**Desde:** especificación 0.1 · función n.º 11.

**Ver también:** [`zxv_key_get`](#zxv_key_get),
[`zxv_input.h`](libraries.md#teclado-y-mandos-a-la-vez-zxv_inputh).

### El ratón: `zxv_mouse_t`, `zxv_mouse_init`, `zxv_mouse_update`

```c
typedef struct {
    int x, y;                   /* el puntero, en píxeles */
    int wheel;                  /* la rueda, acumulada (positiva hacia arriba) */
    uint8_t buttons;            /* bits 0-2: izquierdo, derecho y central */
    uint16_t rx, ry;            /* interno: los contadores en la última llamada */
    uint8_t rw;
} zxv_mouse_t;

void zxv_mouse_init(zxv_mouse_t *p, int x, int y);
void zxv_mouse_update(zxv_mouse_t *p, int w, int h);
```

Un puntero en píxeles que sigue al ratón USB, como `MOUSE` en el BASIC.
`zxv_mouse_init` pone el puntero en (x, y) y se queda con los contadores
del ratón. `zxv_mouse_update`, una vez por imagen, suma el movimiento desde
la llamada anterior (una unidad del ratón es un píxel, sin aceleración),
lee los botones y la rueda, y mantiene el puntero dentro de la zona.
Leen directamente los [registros de entrada](#registros-de-entrada)
`ZXV_I_MOUSE_...`; sin ratón, el puntero no se mueve.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `p` | | El puntero, una estructura del programa. |
| `x`, `y` | | Dónde empieza el puntero. |
| `w`, `h` | 1 o más | El tamaño de la zona: el puntero queda en [0, w) × [0, h). Normalmente el de la pantalla. |

**Ejemplo**

```c
zxv_mouse_t m;
zxv_mouse_init(&m, ZXV_MODE0_W / 2, ZXV_MODE0_H / 2);
for (;;) {
    zxv_mouse_update(&m, ZXV_MODE0_W, ZXV_MODE0_H);
    if (m.buttons & 1)
        zxv_plot(m.x, m.y, 2);              /* pinta con el botón izquierdo */
    zxv_wait_vblank();
}
```

**Desde:** especificación 0.1 (están en `zxv_api.h`; no son funciones de la
API).

**Ver también:** [Registros de entrada](#registros-de-entrada).

## Sonido

El sonido son dos chips AY, el beeper y doce [canales de
muestras](#canales-de-muestras), que se mezclan en la salida. Al AY y a los
canales de muestras se escribe por dos colas: una que suena en cuanto
puede (`zxv_ay_write`) y otra en la que cada escritura lleva su hora
(`zxv_ay_event`). Los volúmenes y el estéreo están en los [registros de
sonido](#registros-de-sonido).

### `zxv_ay_write`

```c
void zxv_ay_write(int reg, int val);
```

Escribe en un registro de un AY (o de un canal de muestras). La escritura
pasa por una cola de 256 entradas y suena unos milisegundos después, en
orden con las demás; si la cola está llena, la función espera. Escribir el
registro 13 reinicia la envolvente, como en el chip de verdad.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `reg` | 0–15, el primer AY; `ZXV_AY2` + 0–15 (16–31), el segundo | El registro (tabla). También valen los números de los canales de muestras (`ZXV_SMP_REG(ch, reg)`, 64–159) y `ZXV_SMP_KEYON_HI` y `ZXV_SMP_KEYON` (36, 37). Los demás números se ignoran. |
| `val` | 0–255 | El valor. Los bits que el registro no tiene se quitan (el 1 solo guarda 4, por ejemplo). |

| Registro | Contenido |
|---|---|
| 0, 1 | Periodo del tono del canal A: 8 bits bajos, 4 bits altos |
| 2, 3 | Periodo del tono del canal B |
| 4, 5 | Periodo del tono del canal C |
| 6 | Periodo del ruido (5 bits) |
| 7 | Mezclador: los bits 0–2 a 1 apagan el tono de A, B, C; los bits 3–5 a 1 apagan el ruido de A, B, C |
| 8, 9, 10 | Volumen de A, B, C (0–15); el bit 4 = seguir la envolvente |
| 11, 12 | Periodo de la envolvente: byte bajo y alto |
| 13 | Forma de la envolvente (0–15) |

La frecuencia del tono es 1.773.447,5 / (16 × periodo) Hz; la envolvente
se repite cada 256 × periodo / 1.773.447,5 segundos. Los dos chips
empiezan en silencio, y la ROM los calla cuando un programa vuelve al
BASIC.

**Ejemplo**

```c
zxv_ay_write(0, 252);               /* La 440 Hz en el canal A: periodo 252 */
zxv_ay_write(1, 0);
zxv_ay_write(7, 0x3E);              /* solo el tono de A */
zxv_ay_write(8, 15);                /* a todo volumen */
```

**Desde:** especificación 0.1 · función n.º 12.

**Ver también:** [`zxv_ay_event`](#zxv_ay_event),
[Constantes](#constantes-1), [Música y efectos](#música-y-efectos).

### `zxv_beeper_event`

```c
void zxv_beeper_event(int level);
```

Pone el beeper a un nivel exactamente a la hora actual más
`ZXV_BEEP_LATENCY_US` (2000 µs). Los cambios pasan por la cola con hora (256
entradas, compartida con `zxv_ay_event`); si está llena, la función espera.
Así el beeper suena con la precisión del microsegundo, aunque el programa
no llame a tiempo exacto.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `level` | 0, u otro | 0, abajo; cualquier otro valor, arriba (1). |

Para sonidos sencillos puedes escribir en el registro `ZXV_S_BEEPER`; las
dos formas se suman. Con la música de `zxv_music.h` sonando, usa
[`zxv_beeper_event_at`](#zxv_beeper_event_at): esta pone la hora actual, y
la música ya ha puesto las suyas por delante.

**Ejemplo**

```c
for (int i = 0; i < 200; i++) {     /* un pitido de 1 kHz */
    zxv_beeper_event(i & 1);
    uint32_t t = zxv_time_us();
    while (zxv_time_us() - t < 500)
        ;
}
```

**Desde:** especificación 0.1 · función n.º 13.

**Ver también:** [`zxv_beeper_event_at`](#zxv_beeper_event_at),
[Registros de sonido](#registros-de-sonido).

### `zxv_ay_event`

```c
void zxv_ay_event(uint32_t t_us, int reg, int val);
```

Escribe en un registro de un AY (o de un canal de muestras) a una hora
exacta: `t_us` más 2 ms (`ZXV_BEEP_LATENCY_US`). Va por la cola con hora,
la misma del beeper. Está pensada para el sonido digitalizado y para la
música: ver el ejemplo [`digital`](../../../sdk/examples/digital/digital.c).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `t_us` | en el reloj de `zxv_time_us` | La hora. Las horas deben ir de menor a mayor: un cambio espera a los que tiene delante. Si ya ha pasado, suena enseguida. |
| `reg` | como en [`zxv_ay_write`](#zxv_ay_write) | El registro. |
| `val` | 0–255 | El valor. |

Si la cola está llena, la función espera. `zxv_smp_write` tiene su versión
con hora: `zxv_ay_event(t, ZXV_SMP_REG(ch, reg), val)`.

**Ejemplo**

```c
uint32_t t = zxv_time_us() + 5000;  /* dentro de 5 ms */
for (int v = 15; v >= 0; v--) {     /* el canal A se apaga en 160 ms */
    zxv_ay_event(t, 8, v);
    t += 10000;
}
```

**Desde:** especificación 0.1 · función n.º 51.

**Ver también:** [`zxv_ay_write`](#zxv_ay_write),
[`zxv_beeper_event_at`](#zxv_beeper_event_at),
[`zxv_music_update_until`](#zxv_music_update_until).

### `zxv_beeper_event_at`

```c
void zxv_beeper_event_at(uint32_t t_us, int level);
```

Como [`zxv_beeper_event`](#zxv_beeper_event), pero a la hora `t_us` (más
2 ms), con las mismas reglas que `zxv_ay_event`: las horas de menor a
mayor, y si la cola está llena, espera.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `t_us` | en el reloj de `zxv_time_us` | La hora. |
| `level` | 0, u otro | 0, abajo; cualquier otro, arriba. |

Con la música de `zxv_music.h` sonando, que va `ZXV_MUSIC_AHEAD_US` (35 ms)
por delante, el beeper tiene que ir igual de adelantado para no
desordenar la cola.

**Ejemplo**

```c
zxv_beeper_event_at(zxv_time_us() + ZXV_MUSIC_AHEAD_US, 1);
```

**Desde:** especificación 0.1, ROM 0.6.0 o posterior · función n.º 55.

**Ver también:** [`zxv_beeper_event`](#zxv_beeper_event),
[`zxv_ay_event`](#zxv_ay_event).

### Constantes

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_AY2` | 16 | Se suma al número de registro para llegar al segundo AY |
| `ZXV_AY_FIFO_LEN` | 256 | Entradas de la cola del AY (`zxv_ay_write`) |
| `ZXV_BEEP_QUEUE_LEN` | 256 | Entradas de la cola con hora (`zxv_ay_event`, el beeper) |
| `ZXV_BEEP_LATENCY_US` | 2000 | Retraso fijo de la cola con hora, en µs |
| `ZXV_AY_STEREO_MONO`, `_ABC`, `_ACB` | 0, 1, 2 | Valores de `ZXV_S_AY_STEREO` (ABC al encender) |
| `ZXV_EV_AY`, `ZXV_EV_AY_WRITE(reg, val)` | 0x80000000, | El dato de una escritura en el AY en la cola con hora (con el bit 31); solo hace falta si llenas la cola tú mismo |

### Canales de muestras

Doce canales que reproducen muestras de la **memoria de sonido**, que está
en la Pico ULA, como el Paula de un Amiga: música de ficheros MOD, efectos
y voces grabados, o un byte que se escribe y suena al momento (el DAC).
Cada canal tiene su nota (su paso), su volumen y su panorama, y se mezcla
con los dos AY. En el emulador suenan igual, con el mismo código.

**La memoria de sonido** tiene dos partes, y el programa la llena con
[`zxv_smp_upload`](#zxv_smp_upload):

| Desde | Hasta | Qué |
|---|---|---|
| 0 | `ZXV_SMP_MEM` (0x18000, 96 KB) | Las muestras. Una muestra tiene que estar entera aquí |
| `ZXV_SMP_TABLE` (0x18000) | `ZXV_SMP_SOUND_MEM` (0x18C00) | La **tabla de muestras**: 256 entradas (`ZXV_SMP_SLOTS`) de 12 bytes (`ZXV_SMP_SLOT_SIZE`), que se rellenan con [`zxv_smp_define`](#zxv_smp_define) |

Una entrada de la tabla (todo con el byte bajo primero):

| Bytes | Campo |
|---|---|
| 0–2 | Dónde empieza la muestra en la memoria de sonido (24 bits) |
| 3 | El formato: `ZXV_SMP_PCM8` (0) o `ZXV_SMP_ADPCM` (1) |
| 4–7 | Cuántas muestras tiene |
| 8–11 | Desde qué muestra vuelve a empezar el bucle; si es la longitud o más, no hay bucle |

Una entrada que no vale (otro formato, longitud 0, una muestra que no cabe
en los 96 KB) no da error: la muestra no suena.

**Los formatos:**

- **`ZXV_SMP_PCM8`**: un byte con signo por muestra (−128 a 127). Admite
  cualquier paso.
- **`ZXV_SMP_ADPCM`**: el IMA, en bloques independientes de 64 muestras
  (`ZXV_SMP_ADPCM_BLOCK`) en 36 bytes (`ZXV_SMP_ADPCM_BYTES`): la primera
  muestra en 16 bits con signo (bytes 0–1), el índice del paso (byte 2,
  0–88), un byte a 0 y las otras 63 en 4 bits, desde el byte 4, la de
  número par en los 4 bits de abajo. Suena como 16 bits y ocupa algo más
  de la mitad que 8. Cada muestra sale de la anterior, así que una nota
  aguda descodifica también las que se salta: un canal en ADPCM **no pasa
  del paso 2,0** (`ZXV_SMP_ADPCM_MAX_STEP`, una octava más aguda que como
  se grabó); un paso mayor cuenta como 2,0. Una muestra de `n` muestras
  ocupa `(n + 63) / 64 × 36` bytes. `smp_adpcm_encode`, de
  `sdk/sound/smpsynth.c`, codifica; el ejemplo
  [`samples`](../../../sdk/examples/samples/samples.c) trae su propio
  codificador.

**El sonido de un canal:**

- **La interpolación**, que viene puesta, suaviza las notas graves; la
  última muestra de un bucle va hacia la primera del bucle, así que una
  onda corta en bucle suena redonda. `ZXV_SMP_NOINTERP` la quita (el sonido
  crudo de un Amiga).
- **El modo directo** (`ZXV_SMP_DIRECT`) toca el valor de `ZXV_SMP_OFFSET`
  según se escribe, sin muestra: un DAC de 8 bits con signo, como el
  SpecDrum. Con `zxv_ay_event`, cada valor suena a su hora.
- **El volumen de todos** es el registro `ZXV_S_SMP_VOLUME` (0–255, 255 al
  encender; la ROM no lo vuelve a poner entre programas): un canal a tope
  suena como un canal del AY, y doce a tope saturan.

**Los registros de un canal.** Se escriben por las colas del AY con
`zxv_smp_write(ch, reg, val)` (o `ZXV_SMP_REG(ch, reg)` con `zxv_ay_write`
y `zxv_ay_event`), y se leen en `ZXV_S_SMP_REGS` + 8 × canal + registro
([Registros de sonido](#registros-de-sonido)).

| Registro | Número | Qué es |
|---|---|---|
| `ZXV_SMP_SLOT` | 0 | La muestra: una entrada de la tabla (0–255). Cuenta al dispararla |
| `ZXV_SMP_VOL` | 1 | Volumen, 0–64 (más cuenta como 64). Cambia al momento |
| `ZXV_SMP_PAN` | 2 | Panorama: 0 izquierda, 128 centro, 255 derecha. Cambia al momento |
| `ZXV_SMP_CTRL` | 3 | Al escribirlo: `ZXV_SMP_KEY` (0x01) dispara la muestra desde `ZXV_SMP_OFFSET` y `ZXV_SMP_STOP` (0x02) la para. `ZXV_SMP_NOINTERP` (0x04) y `ZXV_SMP_DIRECT` (0x08) se quedan hasta la siguiente escritura de `CTRL`: cada escritura los pone o los quita |
| `ZXV_SMP_STEP` | 4–6 | Tres bytes, del bajo al alto (8.16): cuánto avanza por cada muestra de la salida, que va a 48 kHz. 0x10000 (1,0) es la muestra a 48 kHz. Cambia al momento, también con la muestra sonando |
| `ZXV_SMP_OFFSET` | 7 | Desde dónde empieza al dispararla, de 256 en 256 muestras (el `9xx` de un MOD). En modo directo, lo que suena (con signo) |

#### `zxv_smp_upload`

```c
int zxv_smp_upload(uint32_t dst, const void *src, uint32_t len);
```

Copia bytes de la RAM a la memoria de sonido, y espera a que estén. La
copia va por el enlace con la ULA, un trozo en cada imagen, así que una
grande tarda varias imágenes. Sirve también para escribir la tabla de
muestras directamente (desde `ZXV_SMP_TABLE`).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `dst` | 0 a `ZXV_SMP_SOUND_MEM` | Dónde, en la memoria de sonido. |
| `src` | en la RAM (0x2000_0000–0x2006_FFFF) | De dónde. Los datos de tu programa (también los `const`) están en la RAM; la ROM no vale (el emulador la acepta, la placa no). |
| `len` | múltiplo de 4; 0 vale | Cuántos bytes. `dst + len` no puede pasar de `ZXV_SMP_SOUND_MEM`. |

**Devuelve:** `ZXV_OK`, o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_RANGE` | `len` no es múltiplo de 4; no cabe (`dst + len` pasa de `ZXV_SMP_SOUND_MEM`); `src` (entero) no está en la RAM; o la ULA no contesta en un segundo (un firmware de la ULA anterior a los canales de muestras). No se copia nada, o no entero. |

Usa `zxv_wait_vblank`: no la llames desde un gancho de frame.

**Ejemplo**

```c
static const int8_t onda[32] = { 0, 24, 48, 70, 90, 106, 117, 125,
                                 127, 125, 117, 106, 90, 70, 48, 24,
                                 0, -24, -48, -70, -90, -106, -117, -125,
                                 -127, -125, -117, -106, -90, -70, -48, -24 };

if (zxv_smp_upload(0, onda, sizeof onda) < 0)
    zxv_report_error(0, "Sin canales de muestras");
```

**Desde:** especificación 0.1, ROM 0.10.0 o posterior (está en `zxv_api.h`; usa los registros
`ZXV_S_SMP_SRC`, `_DST`, `_LEN` y `_UPLOAD`).

**Ver también:** [`zxv_smp_define`](#zxv_smp_define),
[`zxv_music_samples`](#zxv_music_samples).

#### `zxv_smp_define`

```c
int zxv_smp_define(int slot, uint32_t addr, int fmt, uint32_t len, uint32_t loop);
```

Rellena una entrada de la tabla de muestras: dónde está una muestra que ya
está en la memoria de sonido, su formato y su bucle. Es una
`zxv_smp_upload` de 12 bytes. Un canal que ya la está tocando sigue con la
de antes: la entrada se lee al disparar.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `slot` | 0–255 | La entrada de la tabla. |
| `addr` | 0 a 0x17FFF | Dónde empieza la muestra en la memoria de sonido. |
| `fmt` | `ZXV_SMP_PCM8` o `ZXV_SMP_ADPCM` | El formato. |
| `len` | 1 o más | Cuántas **muestras** tiene (no bytes). Tiene que caber entera en los 96 KB. |
| `loop` | 0 a `len` − 1, o `len` o más | La muestra desde la que vuelve a empezar al llegar al final; `len` o más: sin bucle, se para al final. |

**Devuelve:** `ZXV_OK`, o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_RANGE` | `slot` no está entre 0 y 255; o la ULA no contesta (como en [`zxv_smp_upload`](#zxv_smp_upload)). |

Los valores de la entrada no se comprueban aquí: una que no vale no suena.

**Ejemplo**

```c
zxv_smp_define(0, 0, ZXV_SMP_PCM8, 32, 0);          /* la onda, en bucle entero */
zxv_smp_define(1, 1024, ZXV_SMP_ADPCM, 8000, 8000); /* un golpe, sin bucle */
```

**Desde:** especificación 0.1, ROM 0.10.0 o posterior (está en `zxv_api.h`).

**Ver también:** [`zxv_smp_upload`](#zxv_smp_upload),
[`zxv_smp_play`](#zxv_smp_play).

#### `zxv_smp_play`

```c
void zxv_smp_play(int ch, int slot, uint32_t step, int vol, int pan);
```

Toca una muestra en un canal, ya, desde el principio. Es
[`zxv_smp_prepare`](#zxv_smp_prepare-zxv_smp_start) y una escritura de
`ZXV_SMP_KEY` en `ZXV_SMP_CTRL`: por la cola del AY, en orden con lo
demás. Si el canal tocaba otra cosa, la corta.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ch` | 0–11 | El canal. Otro valor escribe donde no debe: no lo uses. |
| `slot` | 0–255 | La entrada de la tabla. |
| `step` | 0 a 0xFFFFFF (8.16) | El paso: [`zxv_smp_step(hz)`](#zxv_smp_step) para que una muestra grabada a `hz` suene a su altura; el doble, una octava más aguda. En ADPCM, hasta 0x20000. |
| `vol` | 0–64 | El volumen (más cuenta como 64). |
| `pan` | 0–255 | El panorama: 0 izquierda, 128 centro, 255 derecha. |

Como escribe `ZXV_SMP_CTRL` con solo `ZXV_SMP_KEY`, quita
`ZXV_SMP_NOINTERP` y `ZXV_SMP_DIRECT` del canal. Para tocar sin interpolar,
escribe después `zxv_smp_write(ch, ZXV_SMP_CTRL, ZXV_SMP_NOINTERP)`.

**Ejemplo**

```c
zxv_smp_play(0, 0, zxv_smp_step(440 * 32), 64, 128);   /* la onda de 32: 440 Hz */
```

**Desde:** especificación 0.1, ROM 0.10.0 o posterior (está en `zxv_api.h`).

**Ver también:** [`zxv_smp_stop`](#zxv_smp_stop),
[`zxv_smp_prepare`, `zxv_smp_start`](#zxv_smp_prepare-zxv_smp_start),
[`zxv_music_smp_fx`](#zxv_music_smp_fx) (con la música sonando).

#### `zxv_smp_prepare`, `zxv_smp_start`

```c
void zxv_smp_prepare(int ch, int slot, uint32_t step, int vol, int pan);
void zxv_smp_start(uint32_t mask);
```

`zxv_smp_prepare` deja todo listo en un canal igual que `zxv_smp_play`
(muestra, paso, volumen, panorama y desde el principio), pero sin
arrancarlo. `zxv_smp_start` arranca **a la vez**, en la misma muestra de la
salida, los canales que se le digan, preparados o no (desde
`ZXV_SMP_OFFSET`). Úsalos para un acorde o una fila de música: con
`zxv_smp_play` en cada canal, si son muchos, unos pueden arrancar un poco
antes que otros, y con la misma muestra en dos canales se nota.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ch`, `slot`, `step`, `vol`, `pan` | | Como en [`zxv_smp_play`](#zxv_smp_play). |
| `mask` | 0 a 0xFFF | Los canales: el bit n, el canal n. |

`zxv_smp_start` escribe `ZXV_SMP_KEYON_HI` (los canales 8–11, si hay) y
`ZXV_SMP_KEYON` seguidos, como tienen que ir. A diferencia de
`zxv_smp_play`, no toca `ZXV_SMP_CTRL`: `ZXV_SMP_NOINTERP` y
`ZXV_SMP_DIRECT` se quedan como estén.

**Ejemplo**

```c
/* Un acorde: La, Do# y Mi, que empiezan juntos. */
zxv_smp_prepare(1, 0, zxv_smp_step(440 * 32), 40, 64);
zxv_smp_prepare(2, 0, zxv_smp_step(554 * 32), 40, 128);
zxv_smp_prepare(3, 0, zxv_smp_step(659 * 32), 40, 192);
zxv_smp_start(1 << 1 | 1 << 2 | 1 << 3);
```

**Desde:** especificación 0.1, ROM 0.10.0 o posterior (están en `zxv_api.h`).

**Ver también:** [`zxv_smp_play`](#zxv_smp_play).

#### `zxv_smp_stop`

```c
void zxv_smp_stop(int ch);
```

Para el canal: escribe `ZXV_SMP_STOP` en su `ZXV_SMP_CTRL`, por la cola del
AY. También le quita `ZXV_SMP_NOINTERP` y `ZXV_SMP_DIRECT`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ch` | 0–11 | El canal. |

Al volver al BASIC (o al monitor), la ROM para todos los canales de
muestras y calla los AY, también los de una muestra en bucle.

**Ejemplo**

```c
for (int ch = 0; ch < ZXV_SMP_CHANNELS; ch++)
    zxv_smp_stop(ch);
```

**Desde:** especificación 0.1, ROM 0.10.0 o posterior (está en `zxv_api.h`).

**Ver también:** [`zxv_smp_play`](#zxv_smp_play).

#### `zxv_smp_write`

```c
void zxv_smp_write(int ch, int reg, int val);
```

Escribe un [registro de un canal](#canales-de-muestras), por la cola del
AY, en orden con las escrituras de los AY. Si la cola está llena, espera.
Para hacerlo a una hora exacta: `zxv_ay_event(t, ZXV_SMP_REG(ch, reg), val)`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ch` | 0–11 | El canal. |
| `reg` | 0–7 | El registro: `ZXV_SMP_SLOT`, `_VOL`, `_PAN`, `_CTRL`, `ZXV_SMP_STEP` + 0–2, `ZXV_SMP_OFFSET`. |
| `val` | 0–255 | El valor. |

**Ejemplo**

```c
for (int p = 0; p < 256; p += 4) {          /* la muestra va de izquierda a derecha */
    zxv_smp_write(0, ZXV_SMP_PAN, p);
    zxv_wait_vblank();
}
```

**Desde:** especificación 0.1, ROM 0.10.0 o posterior (está en `zxv_api.h`).

**Ver también:** [`zxv_ay_event`](#zxv_ay_event).

#### `zxv_smp_step`

```c
uint32_t zxv_smp_step(uint32_t sample_hz);
```

El paso para que una muestra grabada a `sample_hz` suene a su altura: la
cuenta, en enteros, es `sample_hz × 65536 / 48000`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `sample_hz` | hasta 16.777.215 | Las muestras por segundo con que se grabó (o, para una onda de `n` muestras, `n ×` la frecuencia de la nota). |

**Devuelve:** el paso (8.16). 48000 da 0x10000; 24000, 0x8000.

**Errores:** ninguno.

**Ejemplo**

```c
uint32_t paso = zxv_smp_step(22050);        /* una muestra grabada a 22.050 Hz */
zxv_smp_play(2, 1, paso * 2, 64, 128);      /* una octava más aguda */
```

**Desde:** especificación 0.1, ROM 0.10.0 o posterior (está en `zxv_api.h`).

**Ver también:** [`zxv_smp_play`](#zxv_smp_play).

#### Constantes de los canales de muestras

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_SMP_CHANNELS` | 12 | Canales de muestras |
| `ZXV_SMP_MEM` | 0x18000 (96 KB) | Bytes para las muestras en la memoria de sonido |
| `ZXV_SMP_TABLE` | 0x18000 | Dónde empieza la tabla de muestras, detrás de las muestras |
| `ZXV_SMP_SLOTS` | 256 | Entradas de la tabla |
| `ZXV_SMP_SLOT_SIZE` | 12 | Bytes por entrada |
| `ZXV_SMP_SOUND_MEM` | 0x18C00 (101.376) | Toda la memoria de sonido, las muestras y la tabla: el límite de `zxv_smp_upload` |
| `ZXV_SMP_PCM8`, `ZXV_SMP_ADPCM` | 0, 1 | Los formatos |
| `ZXV_SMP_ADPCM_BLOCK` | 64 | Muestras por bloque de ADPCM |
| `ZXV_SMP_ADPCM_BYTES` | 36 | Bytes por bloque de ADPCM |
| `ZXV_SMP_ADPCM_MAX_STEP` | 0x20000 | El paso más alto en ADPCM (2,0) |
| `ZXV_SMP_REG(ch, reg)` | 64 + 8 × `ch` + `reg` | El número de un registro de un canal para las colas del AY |
| `ZXV_SMP_SLOT` ... `ZXV_SMP_OFFSET` | 0–7 | Los registros de un canal (tabla de arriba) |
| `ZXV_SMP_KEY`, `_STOP`, `_NOINTERP`, `_DIRECT` | 0x01, 0x02, 0x04, 0x08 | Los bits de `ZXV_SMP_CTRL` |
| `ZXV_SMP_KEYON` | 37 | Número para las colas: arranca a la vez los canales 0–7 de su valor (bits 0–7) y los que guardó `ZXV_SMP_KEYON_HI` |
| `ZXV_SMP_KEYON_HI` | 36 | Número para las colas: guarda los canales 8–11 (bits 0–3) para el `ZXV_SMP_KEYON` siguiente, que tiene que ir justo detrás |

## Música y efectos

Declarado en `zxv_music.h`. Se compila con los ficheros de `sdk/music`: no
está en la ROM (el BASIC lleva su propia copia), y funciona con cualquier
ROM. Las explicaciones y un ejemplo completo están en la
[guía](README.md#música-y-efectos-de-los-trackers).

Lo esencial:

- **Suena a su tempo:** 50 pasos por segundo, como en el Spectrum (un
  `.zxm`, a su tempo; un VGM, a sus esperas), aunque la máquina vaya a 60
  imágenes por segundo. Cada paso va a los AY a su hora con
  [`zxv_ay_event`](#zxv_ay_event), y solo los registros que cambian.
- **Hay que darle cuerda:** [`zxv_music_update`](#zxv_music_update)
  prepara lo de los próximos 35 ms (`ZXV_MUSIC_AHEAD_US`). Llámala en cada
  imagen, o desde el gancho de frame (y entonces solo desde él). Sin ella
  no suena nada, ni los efectos, ni el silencio al parar.
- **La biblioteca no copia nada:** el módulo, el banco de efectos y la
  memoria de un `.vgz` tienen que seguir en la memoria mientras suenan.
- **La cola con hora va en orden:** si el programa también la usa, sus
  horas tienen que ir 35 ms por delante, como las de la música
  ([`zxv_beeper_event_at`](#zxv_beeper_event_at)), o mezclarse con
  [`zxv_music_update_until`](#zxv_music_update_until).
- **Los canales:** un módulo de un AY suena en el primero (o en el
  segundo, con `ZXV_MUSIC_AY2`); TurboSound, un `.zxm` o un VGM de dos AY,
  en los dos. Los efectos AYFX van en los canales A, B y C de cualquiera
  de los dos, encima de la música. Un `.zxm` toca sus pistas de muestras
  en los canales de muestras desde el 0.

### Módulos

```c
int zxv_music_info(const void *data, uint32_t size, zxv_music_info_t *info);
int zxv_music_play(const void *data, uint32_t size, int flags);
void zxv_music_stop(void);
void zxv_music_samples(uint32_t base, int slot);
void zxv_music_work(void *buf);
void zxv_music_smp_fx(int ch, int slot, uint32_t step, int vol, int pan, uint32_t dur_us);
void zxv_music_smp_fx_stop(int ch);
int zxv_music_smp_tracks(void);
void zxv_music_pause(int on);
int zxv_music_playing(void);
uint32_t zxv_music_steps(void);
void zxv_music_moved(const void *song, const void *bank);
void zxv_music_update(void);
void zxv_music_update_until(uint32_t t_us);
```

Ninguna función de la música da errores: una que no puede hacer lo que se
le pide no hace nada, o lo dice en lo que devuelve.

#### `zxv_music_play`

```c
int zxv_music_play(const void *data, uint32_t size, int flags);
```

Empieza a tocar un módulo, desde el principio, en lugar del que sonara. Lo
reconoce por lo que tiene dentro, no por el nombre del fichero.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `data` | | El módulo, entero en la memoria, y que siga ahí mientras suena. |
| `size` | bytes | Lo que ocupa. |
| `flags` | 0, `ZXV_MUSIC_LOOP`, `ZXV_MUSIC_AY2`, o los dos | `ZXV_MUSIC_LOOP`: al acabar, sigue por su posición de vuelta (sin él, se para). `ZXV_MUSIC_AY2`: un módulo de un AY suena en el segundo (en uno de dos, no cuenta). |

**Devuelve:** el formato (`ZXV_MUSIC_PT3`...), o `ZXV_MUSIC_NONE` si no lo
reconoce; entonces no toca nada (y lo que sonaba ya se ha parado).

- **Un `.zxm`** sube sus muestras a la memoria de sonido antes de empezar
  (espera a que estén; no la llames desde un gancho de frame), donde diga
  [`zxv_music_samples`](#zxv_music_samples).
- **Un `.vgz`** se descomprime mientras suena, con la memoria que le des
  con [`zxv_music_work`](#zxv_music_work): sin ella, da `ZXV_MUSIC_NONE`.
  De un VGM solo suenan los AY; lo de otros chips se salta.
- Las muestras de la canción anterior se callan enseguida.

**Ejemplo**

```c
extern const uint8_t cancion[];
extern const uint32_t cancion_len;

if (zxv_music_play(cancion, cancion_len, ZXV_MUSIC_LOOP) == ZXV_MUSIC_NONE)
    zxv_report_error(0, "La canción no vale");
```

**Ver también:** [`zxv_music_update`](#zxv_music_update),
[`zxv_music_stop`](#zxv_music_stop), [`zxv_music_info`](#zxv_music_info).

#### `zxv_music_info`

```c
int zxv_music_info(const void *data, uint32_t size, zxv_music_info_t *info);

typedef struct {
    int format;                 /* ZXV_MUSIC_... */
    int chips;                  /* los AY que usa (abajo) */
    char title[33];
    char author[33];
} zxv_music_info_t;
```

Dice qué es un módulo, sin tocarlo: su formato, los AY que usa, su título y
su autor.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `data`, `size` | | El módulo, como en `zxv_music_play`. |
| `info` | | Dónde deja lo que encuentra. |

**Devuelve:** el formato, como `info->format`; `ZXV_MUSIC_NONE` si no es
ninguno conocido.

- `chips`: 1, un AY; 3, los dos; 0, ninguno (un `.zxm` solo de muestras, o
  un módulo que no se reconoce).
- `title` y `author`: hasta 32 caracteres ASCII (lo demás, como espacios),
  sin los espacios del final; vacíos si el formato no los tiene (PSG, el
  autor de PT2 y STC, un ASC sin ellos) o, en un VGM, si no lleva
  etiqueta GD3 (un `.vgz`, sin descomprimir, no la enseña). Un `.vgz` se
  reconoce aunque no haya `zxv_music_work`.

**Ejemplo**

```c
zxv_music_info_t mi;
if (zxv_music_info(cancion, cancion_len, &mi) != ZXV_MUSIC_NONE) {
    zxv_print_str(mi.title);
    zxv_print_str(mi.chips == 3 ? " (dos AY)\n" : "\n");
}
```

**Ver también:** [`zxv_music_play`](#zxv_music_play).

#### `zxv_music_stop`

```c
void zxv_music_stop(void);
```

Para la canción. Los AY y los canales de muestras que usaba se callan en la
siguiente [`zxv_music_update`](#zxv_music_update): llámala después. Los
efectos AYFX siguen sonando.

**Ejemplo**

```c
zxv_music_stop();
zxv_music_update();                 /* el silencio */
```

**Ver también:** [`zxv_music_pause`](#zxv_music_pause).

#### `zxv_music_samples`

```c
void zxv_music_samples(uint32_t base, int slot);
```

Dónde pone `zxv_music_play` las muestras de un `.zxm`: en la memoria de
sonido desde `base`, y en la tabla de muestras desde la entrada `slot`. Por
defecto, 0 y 0; las muestras del programa, detrás. Cuenta para el
siguiente `zxv_music_play`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `base` | 0 a 0x17FFF | El primer byte de la memoria de sonido. Lo que no cabe en los 96 KB no se sube; con `base` de 96 KB o más, nada. |
| `slot` | 0–255 | La primera entrada de la tabla. Las que pasarían de la 255 no se ponen. |

**Ejemplo**

```c
zxv_music_samples(64 * 1024, 200);  /* la canción, detrás de los efectos */
zxv_music_play(cancion, cancion_len, ZXV_MUSIC_LOOP);
```

**Ver también:** [Canales de muestras](#canales-de-muestras).

#### `zxv_music_work`

```c
void zxv_music_work(void *buf);
```

Da a la música 32 KB de memoria para descomprimir un `.vgz` mientras suena.
Sin ellos, un `.vgz` no suena. Cuenta para el siguiente `zxv_music_play`, y
la memoria tiene que seguir siendo de la música mientras suena.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `buf` | 32.768 bytes | La memoria. |

**Ejemplo**

```c
ZXV_VRAM1_DATA static uint8_t ventana[32 * 1024];   /* en la VRAM oculta */

zxv_music_work(ventana);
zxv_music_play(tema_vgz, tema_vgz_len, ZXV_MUSIC_LOOP);
```

**Ver también:** [`zxv_music_play`](#zxv_music_play).

#### `zxv_music_smp_fx`

```c
void zxv_music_smp_fx(int ch, int slot, uint32_t step, int vol, int pan, uint32_t dur_us);
```

Toca una muestra como efecto, a la vez que la canción, en un canal de
muestras. Con la música en marcha va a su paso, con su mismo adelanto (así
no desordena la cola con hora); sin música, enseguida. Si el canal es de la
canción, la tapa mientras suena, y la canción sigue en él en su siguiente
nota. Es lo que usa `SAMPLE` en el BASIC.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `ch` | 0–11 | El canal. Otro valor no hace nada. Los efectos, mejor en los que no usa la canción ([`zxv_music_smp_tracks`](#zxv_music_smp_tracks)). |
| `slot`, `step`, `vol`, `pan` | | Como en [`zxv_smp_play`](#zxv_smp_play). |
| `dur_us` | µs, o 0 | Cuánto tiempo es del efecto. 0: hasta `zxv_music_smp_fx_stop`, para una muestra en bucle. |

Toca la muestra interpolada (pone `ZXV_SMP_CTRL` a 0).

**Ejemplo**

```c
int ch = zxv_music_smp_tracks();            /* el primero libre */
zxv_music_smp_fx(ch, 10, zxv_smp_step(22050), 64, 128, 400000);
```

**Ver también:** [`zxv_music_smp_fx_stop`](#zxv_music_smp_fx_stop),
[`zxv_smp_play`](#zxv_smp_play).

#### `zxv_music_smp_fx_stop`

```c
void zxv_music_smp_fx_stop(int ch);
```

Para el efecto del canal `ch` (0–11; otro valor no hace nada), con el
adelanto de la música si suena. Si el canal es de la canción, la canción
sigue en él en su siguiente nota.

**Ejemplo**

```c
zxv_music_smp_fx(11, 12, zxv_smp_step(16000), 48, 128, 0);  /* un motor, en bucle */
/* ... */
zxv_music_smp_fx_stop(11);
```

**Ver también:** [`zxv_music_smp_fx`](#zxv_music_smp_fx).

#### `zxv_music_smp_tracks`

```c
int zxv_music_smp_tracks(void);
```

**Devuelve:** cuántos canales de muestras usa la canción que suena: los
primeros, desde el 0. 0 si no suena un `.zxm`, si está en pausa o si no
tiene pistas de muestras. Los efectos, en los de arriba.

**Ejemplo:** ver [`zxv_music_smp_fx`](#zxv_music_smp_fx).

#### `zxv_music_pause`

```c
void zxv_music_pause(int on);
```

`zxv_music_pause(1)` para la canción donde está, y `zxv_music_pause(0)` la
sigue desde ahí. Como al pararla, los AY se callan en la siguiente
`zxv_music_update`. Los efectos siguen.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `on` | 0, u otro | Otro valor que no sea 0: en pausa. |

**Ejemplo**

```c
if (tecla == 'p')
    zxv_music_pause(en_pausa ^= 1);
```

**Ver también:** [`zxv_music_stop`](#zxv_music_stop).

#### `zxv_music_playing`

```c
int zxv_music_playing(void);
```

**Devuelve:** 1 mientras hay una canción (también en pausa); 0 si no hay o
si ha llegado al final sin `ZXV_MUSIC_LOOP`.

**Ejemplo**

```c
zxv_music_play(fanfarria, fanfarria_len, 0);
while (zxv_music_playing()) {       /* hasta que acabe */
    zxv_music_update();
    zxv_wait_vblank();
}
```

#### `zxv_music_steps`

```c
uint32_t zxv_music_steps(void);
```

**Devuelve:** los pasos de la canción que han sonado desde
`zxv_music_play`: de 1/50 s (en un `.zxm`, los ticks de su tempo; en un
VGM, los de sus esperas). No cuentan los de la pausa. Sirve para
sincronizar algo con la música.

**Ejemplo**

```c
if (zxv_music_steps() >= 50 * 30)   /* a los 30 s de un PT3 */
    empezar_jefe();
```

#### `zxv_music_moved`

```c
void zxv_music_moved(const void *song, const void *bank);
```

Para un programa que mueve sus datos mientras suenan (como el BASIC, que
compacta su memoria): el módulo o el banco de efectos están ahora, enteros,
en otra dirección, y la música sigue por donde iba.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `song` | la nueva dirección, o `NULL` | Dónde está ahora el módulo; `NULL` si no se ha movido. |
| `bank` | la nueva dirección, o `NULL` | Dónde está ahora el banco de efectos; `NULL` si no se ha movido. |

**Ejemplo**

```c
memmove(nuevo, viejo, cancion_len);         /* se mueve el módulo... */
zxv_music_moved(nuevo, NULL);               /* ...y se le dice */
```

#### `zxv_music_update`

```c
void zxv_music_update(void);
```

Prepara los pasos de la música y de los efectos de los próximos
`ZXV_MUSIC_AHEAD_US` (35 ms) y los manda a su hora. Llámala al menos cada
35 ms: en cada imagen, o desde el gancho de frame (y entonces solo desde
él), que la hace sonar aunque el programa esté ocupado. Si pasa mucho
tiempo sin llamarla, la música no corre para recuperarlo: sigue desde ese
momento.

**Ejemplo**

```c
static void gancho(void)
{
    zxv_music_update();
}

zxv_set_frame_hook(gancho);         /* la música suena sola */
```

**Ver también:** [`zxv_music_update_until`](#zxv_music_update_until),
[`zxv_set_frame_hook`](#zxv_set_frame_hook).

#### `zxv_music_update_until`

```c
void zxv_music_update_until(uint32_t t_us);
```

Como `zxv_music_update`, pero solo los pasos de antes de `t_us` (nunca más
allá de 35 ms por delante). Para mezclar la música con tus `zxv_ay_event`,
que van en orden de hora: antes de cada uno, `zxv_music_update_until(su
hora)`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `t_us` | en el reloj de `zxv_time_us` | Hasta dónde. |

**Ejemplo**

```c
uint32_t t = zxv_time_us() + ZXV_MUSIC_AHEAD_US;
zxv_music_update_until(t);          /* la música, hasta ahí... */
zxv_ay_event(t, 10, 15);            /* ...y lo tuyo, en orden */
```

**Ver también:** [`zxv_ay_event`](#zxv_ay_event).

### Efectos AYFX

```c
int zxv_sfx_bank(const void *data, uint32_t size);
int zxv_sfx_play(int n, int channel);
int zxv_sfx_play_data(const void *data, uint32_t size, int channel);
void zxv_sfx_stop(int channel);
void zxv_sfx_channels(int mask);
int zxv_sfx_playing(int channel);
```

Los efectos de AYFX Editor (un banco `.afb` o un efecto suelto `.afx`),
50 pasos por segundo. Los canales son del 0 al 2 (A, B y C del primer AY)
y del 3 al 5 (los del segundo). Un efecto en un canal de la música lo tapa
mientras suena. Suenan con [`zxv_music_update`](#zxv_music_update), haya
música o no.

#### `zxv_sfx_bank`

```c
int zxv_sfx_bank(const void *data, uint32_t size);
```

Elige el banco de efectos (`.afb`) de `zxv_sfx_play`. Tiene que seguir en
la memoria mientras se use.

**Devuelve:** cuántos efectos tiene (0 si no es un banco).

**Ejemplo**

```c
int n = zxv_sfx_bank(efectos, efectos_len);
```

**Ver también:** [`zxv_sfx_play`](#zxv_sfx_play).

#### `zxv_sfx_play`

```c
int zxv_sfx_play(int n, int channel);
```

Toca un efecto del banco en un canal, desde el principio. Si ese canal ya
tenía un efecto, lo cambia por este.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `n` | 0 a los del banco − 1 | El efecto. |
| `channel` | 0–5, o `ZXV_SFX_AUTO` | El canal. `ZXV_SFX_AUTO` (o cualquier negativo): uno libre de los de [`zxv_sfx_channels`](#zxv_sfx_channels) o, si no hay, el que lleva más tiempo sonando. |

**Devuelve:** el canal, o −1 si el efecto no existe, si `channel` pasa de
5 o si no hay ningún canal elegido para `ZXV_SFX_AUTO`.

**Ejemplo**

```c
if (disparo)
    zxv_sfx_play(0, ZXV_SFX_AUTO);
```

**Ver también:** [`zxv_sfx_play_data`](#zxv_sfx_play_data),
[`zxv_sfx_stop`](#zxv_sfx_stop).

#### `zxv_sfx_play_data`

```c
int zxv_sfx_play_data(const void *data, uint32_t size, int channel);
```

Como `zxv_sfx_play`, con un efecto suelto (`.afx`) en vez de uno del
banco. Tiene que seguir en la memoria mientras suena.

**Devuelve:** el canal, o −1 (`size` 0, `channel` que pasa de 5 o sin
canales).

**Ejemplo**

```c
zxv_sfx_play_data(explosion, explosion_len, 2);
```

#### `zxv_sfx_stop`

```c
void zxv_sfx_stop(int channel);
```

Para el efecto de un canal (0–5), o todos con `ZXV_SFX_AUTO` (o cualquier
negativo).

**Ejemplo**

```c
zxv_sfx_stop(ZXV_SFX_AUTO);         /* silencio de efectos */
```

#### `zxv_sfx_channels`

```c
void zxv_sfx_channels(int mask);
```

Elige los canales de `ZXV_SFX_AUTO`: el bit *n*, el canal *n* (solo cuentan
los bits 0–5). Por defecto son los del AY que no usa la música (los seis si
no hay música o si usa los dos); después de llamarla ya no se vuelve a eso:
`zxv_sfx_channels(0x3F)` da los seis.

**Ejemplo**

```c
zxv_sfx_channels(1 << 2 | 1 << 5); /* los efectos, en los canales C de los dos AY */
```

#### `zxv_sfx_playing`

```c
int zxv_sfx_playing(int channel);
```

**Devuelve:** 1 si suena un efecto en el canal (0–5), o en alguno con
`ZXV_SFX_AUTO` (o cualquier negativo); 0 si no.

**Ejemplo**

```c
while (zxv_sfx_playing(ZXV_SFX_AUTO)) {     /* que acabe la explosión */
    zxv_music_update();
    zxv_wait_vblank();
}
```

### Constantes

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_MUSIC_NONE` | 0 | No es un módulo conocido |
| `ZXV_MUSIC_PT3`, `_TS`, `_PSG` | 1, 2, 3 | Pro Tracker 3 (y Vortex Tracker II), TurboSound (dos PT3, uno por AY), volcado de los registros del AY |
| `ZXV_MUSIC_PT2`, `_STC`, `_ASC` | 4, 5, 6 | Pro Tracker 2, Sound Tracker 1.x, ASC Sound Master |
| `ZXV_MUSIC_ZXM`, `_VGM` | 7, 8 | El formato propio (`.zxm`: los dos AY y los canales de muestras); un VGM (`.vgm` o `.vgz`) con AY |
| `ZXV_MUSIC_LOOP`, `ZXV_MUSIC_AY2` | 1, 2 | Opciones de `zxv_music_play` |
| `ZXV_MUSIC_AHEAD_US` | 35000 | Lo que la música va por delante, en µs |
| `ZXV_SFX_AUTO` | −1 | Un canal libre (o todos) |

## Ficheros y carpetas

Hay tres unidades: `SD:` (la tarjeta SD), `ROM:` (la unidad de solo
lectura de serie, en la flash) y `EXT:` (la unidad externa: una carpeta que
sirve otro equipo, ver [La unidad
EXT:](../hardware/firmware.md#la-unidad-ext)). Las funciones son las mismas
para las tres.

- Puede haber hasta **8 ficheros y 4 carpetas abiertos a la vez en cada
  unidad**. El fichero de un programa con [bancos](#bancos) está abierto
  mientras va, y cuenta.
- Los números de los ficheros dicen de qué unidad son: 0–7 en `SD:`, 8–15
  en `ROM:` y 16–23 en `EXT:`; los de las carpetas, 0–3, 4–7 y 8–11.
- Cada unidad tiene su carpeta actual, y una de ellas es la unidad actual
  (`SD:` al encender).
- **Los buffers** tienen que estar enteros en la RAM (de 0x2000_0000 a
  0x2006_FFFF: el área de sistema, la VRAM y la RAM de usuario) o, si la
  función solo los lee (lo que se escribe en un fichero, los nombres), en
  la ROM. Si no, la función devuelve `ZXV_E_FAULT` y no hace nada.
- **En una tarjeta SD**, cada `zxv_file_write` llega a la tarjeta antes de
  volver (si se saca después, lo escrito está). Un fichero abierto no se
  puede borrar, renombrar ni volver a abrir para escribir. Si se saca la
  tarjeta, lo que estaba abierto en ella solo da `ZXV_E_NOCARD` hasta que
  se cierra.
- **En `EXT:`** se lee y se escribe como en la tarjeta, pero solo mientras
  hay un servidor: si no, todas las llamadas dan `ZXV_E_OFFLINE`. Si el
  servidor se va, la primera espera 1,5 segundos y las siguientes fallan
  enseguida hasta que vuelve. El servidor escribe en una copia y cambia el
  fichero al cerrarlo, y puede negarse a sustituir o borrar un fichero que
  no escribió la máquina (`ZXV_E_ACCESS`).

**Errores comunes.** Además de los propios de cada función, cualquier
función de ficheros puede dar:

| Error | Cuándo |
|---|---|
| `ZXV_E_NOCARD` | La ruta va a `SD:` y no hay tarjeta (o no responde); o el fichero o la carpeta se abrieron antes de sacar la tarjeta. |
| `ZXV_E_IO` | La tarjeta falló al leer o escribir, o no tiene un formato FAT que se entienda. Lo que estaba abierto en ella deja de valer, como al sacarla. |
| `ZXV_E_OFFLINE` | La ruta va a `EXT:` y no hay servidor, o ha dejado de contestar. |
| `ZXV_E_FAULT` | Un buffer o un nombre no está entero en la memoria permitida (arriba). No se toca nada. |

### Rutas

- **La unidad**: una ruta puede empezar por `SD:`, `ROM:` o `EXT:`, en
  mayúsculas o minúsculas. Detrás de la unidad, la ruta va siempre desde su
  raíz: `ROM:graphics` es lo mismo que `ROM:/graphics`.
- **Sin unidad**, la ruta es de la unidad actual: relativa a su carpeta
  actual, o desde su raíz si empieza por `/`.
- `.` y `..` funcionan, y `..` en la raíz se queda en la raíz. Las
  mayúsculas dan igual (`Nivel1.DAT` es `nivel1.dat`).
- **Los nombres** tienen de 1 a 63 caracteres (`ZXV_NAME_MAX`) ASCII
  imprimibles, sin `\ : * ? " < > |`, y sin acabar en espacio ni en punto.
  **Las rutas**, hasta 255 caracteres (`ZXV_PATH_MAX`) y 64 carpetas de
  hondo. Si no, `ZXV_E_NAME`.
- En la tarjeta, los ficheros ocultos o de sistema, y los que tienen un
  nombre que no sigue estas reglas (con tildes, o de más de 63
  caracteres), no salen en los listados.

### `zxv_file_open`

```c
int zxv_file_open(const char *name, uint32_t flags);
```

Abre un fichero para leerlo, escribirlo o las dos cosas. La posición empieza
al principio (al final con `ZXV_O_APPEND`).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | ruta de hasta 255 caracteres; nombres de hasta 63 | El fichero. Sin unidad, en la actual; ver [Rutas](#rutas). |
| `flags` | una combinación de `ZXV_O_*` (tabla) | Qué se va a hacer con él. Tiene que llevar `ZXV_O_READ`, `ZXV_O_WRITE` o los dos. |

| Indicador | Valor | Efecto |
|---|---|---|
| `ZXV_O_READ` | 0x01 | Leer. |
| `ZXV_O_WRITE` | 0x02 | Escribir. |
| `ZXV_O_CREATE` | 0x04 | Crearlo si no existe. Solo con `ZXV_O_WRITE`. |
| `ZXV_O_TRUNC` | 0x08 | Vaciarlo al abrir. Solo con `ZXV_O_WRITE`. |
| `ZXV_O_APPEND` | 0x10 | Empezar al final. |

**Devuelve:** el número del fichero (0 o más), para las demás funciones de
ficheros; o un error (negativo).

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOENT` | No existe y no se pidió `ZXV_O_CREATE` con `ZXV_O_WRITE`; no existe una carpeta de la ruta; o la ruta va a `ROM:` y la máquina no tiene disco ROM. |
| `ZXV_E_ACCESS` | `flags` no lleva `ZXV_O_READ` ni `ZXV_O_WRITE`, o lleva bits que no son `ZXV_O_*`; en `ROM:`, cualquier cosa que no sea solo `ZXV_O_READ`; en `SD:`, abrir para escribir un fichero que ya está abierto, o uno de solo lectura; en `EXT:`, el servidor no deja sustituirlo. |
| `ZXV_E_INVAL` | En `EXT:`, `flags` no lleva `ZXV_O_READ` ni `ZXV_O_WRITE` (lo dice el servidor). |
| `ZXV_E_ISDIR` | `name` es una carpeta (o la raíz). |
| `ZXV_E_NAME` | Nombre o ruta no válidos (ver [Rutas](#rutas)). |
| `ZXV_E_NOTDIR` | Una parte de la ruta no es una carpeta. |
| `ZXV_E_MFILE` | Ya hay 8 ficheros abiertos en esa unidad. |
| `ZXV_E_NOSPC` | No se pudo crear: la tarjeta está llena. |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). |

**Ejemplo**

```c
int f = zxv_file_open("records.dat", ZXV_O_WRITE | ZXV_O_CREATE | ZXV_O_TRUNC);
if (f < 0)
    zxv_report_error(f, "No puedo guardar los records");
zxv_file_write(f, &records, sizeof records);
zxv_file_close(f);          /* en EXT:, el fichero cambia al cerrarlo */
```

**Desde:** especificación 0.1 · función n.º 14 de la API.

**Ver también:** [`zxv_file_close`](#zxv_file_close),
[`zxv_file_load`](#zxv_file_load),
[`zxv_save.h`](libraries.md#guardar-datos-zxv_saveh) (guardar sin
perder lo de antes si se va la luz).

### `zxv_file_read`, `zxv_file_write`

```c
int zxv_file_read(int f, void *buf, uint32_t n);
int zxv_file_write(int f, const void *buf, uint32_t n);
```

Leen o escriben hasta `n` bytes en la posición actual del fichero, y la
llevan detrás de lo leído o lo escrito. Si lees código para ejecutarlo,
ejecuta después una instrucción `fence.i` (o usa
[`zxv_file_load`](#zxv_file_load), que la ejecuta).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un número de `zxv_file_open` | El fichero. |
| `buf` | `n` bytes en la RAM (al escribir, también en la ROM) | Dónde se lee, o lo que se escribe. Al leer, `buf` tiene que tener sitio para `n` bytes aunque el fichero tenga menos. |
| `n` | bytes | Cuántos. |

**Devuelve:** cuántos bytes se han leído o escrito, o un error.
`zxv_file_read` devuelve menos de `n` cerca del final y 0 al final. Si la
posición está más allá del final, `zxv_file_write` rellena el hueco con
ceros (en la tarjeta).

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_BADF` | `f` no es un fichero abierto. |
| `ZXV_E_ACCESS` | Leer de un fichero abierto sin `ZXV_O_READ`; escribir en uno abierto sin `ZXV_O_WRITE`, o de `ROM:`. |
| `ZXV_E_NOSPC` | Al escribir, la tarjeta se ha llenado: puede que se haya escrito una parte, y la posición avanza lo escrito. |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). `ZXV_E_FAULT` si `buf` no tiene `n` bytes en la memoria permitida. |

En `EXT:`, si el servidor se va a mitad, devuelven lo que llegaron a leer o
escribir (o `ZXV_E_OFFLINE` si no llegó nada).

**Ejemplo**

```c
uint8_t cab[16];
int f = zxv_file_open("mapa.bin", ZXV_O_READ);
if (f >= 0) {
    int n = zxv_file_read(f, cab, sizeof cab);
    if (n == (int)sizeof cab)
        leer_resto(f, cab);
    zxv_file_close(f);
}
```

**Desde:** especificación 0.1 · funciones n.º 15 y 16.

**Ver también:** [`zxv_file_seek`](#zxv_file_seek),
[`zxv_file_load`](#zxv_file_load).

### `zxv_file_seek`

```c
int zxv_file_seek(int f, int32_t off, int whence);
```

Mueve la posición de un fichero. Puede ir más allá del final: leer allí da
0 bytes, y escribir alarga el fichero.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un número de `zxv_file_open` | El fichero. |
| `off` | bytes, con signo | Cuánto, desde `whence`. |
| `whence` | `ZXV_SEEK_SET` (0), `ZXV_SEEK_CUR` (1), `ZXV_SEEK_END` (2) | Desde el principio, desde la posición actual o desde el final. |

**Devuelve:** la posición nueva, desde el principio; o un error.
`zxv_file_seek(f, 0, ZXV_SEEK_END)` da el tamaño del fichero.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_BADF` | `f` no es un fichero abierto. |
| `ZXV_E_ACCESS` | `whence` no es 0, 1 ni 2, o la posición quedaría antes del principio. En `ROM:`, también si pasaría de 2 GB. |
| `ZXV_E_INVAL` | En `EXT:`, lo mismo (lo dice el servidor). |
| `ZXV_E_IO` | En `SD:`, la posición pasaría de 2 GB (0x7FFFFFFF). |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). |

**Ejemplo**

```c
int tam = zxv_file_seek(f, 0, ZXV_SEEK_END);        /* el tamaño... */
zxv_file_seek(f, 0, ZXV_SEEK_SET);                  /* ...y al principio */
```

**Desde:** especificación 0.1 · función n.º 21.

**Ver también:** [`zxv_file_read`, `zxv_file_write`](#zxv_file_read-zxv_file_write).

### `zxv_file_close`

```c
int zxv_file_close(int f);
```

Cierra un fichero y deja libre su número. Cierra siempre lo que abras: solo
hay 8 por unidad, y en `EXT:` el fichero que escribes no cambia hasta que
lo cierras (si no lo cierras, se queda el de antes).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un número de `zxv_file_open` | El fichero. |

**Devuelve:** 0, o un error. Un fichero de una tarjeta que se ha sacado, o
de `EXT:` sin servidor, se cierra igual y da 0.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_BADF` | `f` no es un fichero abierto. |
| `ZXV_E_IO`, `ZXV_E_NOCARD` | La tarjeta falló al cerrarlo, o se sacó en ese momento (el número queda libre igual). |

**Ejemplo:** ver [`zxv_file_open`](#zxv_file_open).

**Desde:** especificación 0.1 · función n.º 17.

**Ver también:** [`zxv_file_open`](#zxv_file_open).

### `zxv_file_load`

```c
int zxv_file_load(const char *name, void *addr, uint32_t max);
```

Lee un fichero entero en la memoria. Si cabe, es lo mismo que abrirlo,
leerlo y cerrarlo, en una llamada. Después ejecuta `fence.i`, así que lo
cargado puede ser código.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | ruta | El fichero. |
| `addr` | en la RAM | Dónde se carga. |
| `max` | bytes | El sitio que hay en `addr`. Pásale el tamaño del buffer. |

**Devuelve:** el tamaño del fichero en bytes, o un error (negativo).

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_RANGE` | El fichero ocupa más de `max` bytes. No se carga nada. |
| `ZXV_E_NOENT`, `ZXV_E_ISDIR`, `ZXV_E_NAME`, `ZXV_E_NOTDIR`, `ZXV_E_MFILE` | Como en [`zxv_file_open`](#zxv_file_open). |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). `ZXV_E_FAULT` si `addr` no tiene sitio en la RAM para el fichero entero. |

**Ejemplo**

```c
static uint8_t nivel[8192];
int tam = zxv_file_load("nivel1.dat", nivel, sizeof nivel);
if (tam < 0)
    zxv_report_error(tam, "No puedo cargar nivel1.dat");
```

**Desde:** especificación 0.1 · función n.º 18. (El argumento `max` llegó
después: un programa compilado sin él hay que volver a compilarlo.)

**Ver también:** [`zxv_bank_load`](#zxv_bank_load) (datos dentro del
propio `.zxv`).

### `zxv_file_stat`

```c
int zxv_file_stat(const char *name, zxv_dirent_t *e);

typedef struct {
    char name[64];              /* terminado en cero */
    uint32_t size;              /* en bytes (0 en una carpeta) */
    uint32_t attr;              /* ZXV_ATTR_DIR (bit 0): es una carpeta */
} zxv_dirent_t;
```

Dice si existe un fichero o una carpeta, y su nombre, su tamaño y si es una
carpeta, sin abrirlo.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | ruta | El fichero o la carpeta. |
| `e` | 72 bytes en la RAM | Dónde deja lo que encuentra: `name`, el nombre como está escrito en la unidad (la raíz, `/`). |

**Devuelve:** 0, o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOENT` | No existe (o no existe una carpeta de la ruta). |
| `ZXV_E_NAME`, `ZXV_E_NOTDIR` | Como en [`zxv_file_open`](#zxv_file_open). |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). |

Los mismos 72 bytes, descritos para el ensamblador: `ZXV_DIRENT_SIZE` (72),
`ZXV_DE_NAME` (0), `ZXV_DE_SIZE` (64), `ZXV_DE_ATTR` (68).

**Ejemplo**

```c
zxv_dirent_t e;
if (zxv_file_stat("partida.sav", &e) == 0 && !(e.attr & ZXV_ATTR_DIR))
    hay_partida = e.size > 0;
```

**Desde:** especificación 0.1 · función n.º 22.

**Ver también:** [`zxv_dir_read`](#zxv_dir_read).

### `zxv_file_remove`, `zxv_file_rename`

```c
int zxv_file_remove(const char *name);
int zxv_file_rename(const char *from, const char *to);
```

`zxv_file_remove` borra un fichero o una carpeta **vacía**.
`zxv_file_rename` cambia el nombre de un fichero o de una carpeta, o lo
mueve a otra carpeta de la misma unidad (de una unidad a otra no se puede:
hay que copiarlo).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | ruta | Lo que se borra. |
| `from` | ruta | Lo que se renombra o se mueve. |
| `to` | ruta | El nombre nuevo, con su carpeta. No puede existir. Si solo cambian las mayúsculas, no se hace nada (da 0). |

**Devuelve:** 0, o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOENT` | `name` o `from` no existen, o no existe una carpeta de la ruta de `to`. |
| `ZXV_E_EXIST` | `to` ya existe. |
| `ZXV_E_NOTEMPTY` | La carpeta que se borra no está vacía. |
| `ZXV_E_ACCESS` | Es la raíz; es un fichero abierto; es de `ROM:`; `from` y `to` son de unidades distintas; en `EXT:`, el servidor no deja (un fichero que no escribió la máquina). |
| `ZXV_E_IO` | En `SD:`, mover una carpeta dentro de sí misma. |
| `ZXV_E_NAME`, `ZXV_E_NOTDIR` | Como en [`zxv_file_open`](#zxv_file_open). |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). |

**Ejemplo**

```c
zxv_file_remove("partida.bak");                 /* si no existe, ZXV_E_NOENT */
zxv_file_rename("partida.sav", "partida.bak");
```

**Desde:** especificación 0.1 · funciones n.º 23 y 24.

**Ver también:** [`zxv_dir_make`](#zxv_dir_make).

### Carpetas

```c
int zxv_dir_make(const char *name);             /* crea una carpeta */
int zxv_dir_change(const char *name);           /* cambia la carpeta actual */
int zxv_dir_current(char *buf, uint32_t len);   /* la carpeta actual */
int zxv_dir_open(const char *name);             /* abre una carpeta para listarla */
int zxv_dir_read(int d, zxv_dirent_t *e);       /* la siguiente entrada */
int zxv_dir_close(int d);                       /* la cierra */
```

#### `zxv_dir_make`

```c
int zxv_dir_make(const char *name);
```

Crea una carpeta. La carpeta en la que va tiene que existir.

**Devuelve:** 0, o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_EXIST` | Ya hay un fichero o una carpeta con ese nombre. |
| `ZXV_E_NOENT` | No existe una carpeta de la ruta. |
| `ZXV_E_ACCESS` | En `ROM:`. |
| `ZXV_E_NOSPC` | La tarjeta está llena. |
| `ZXV_E_NAME`, `ZXV_E_NOTDIR` | Como en [`zxv_file_open`](#zxv_file_open). |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). |

**Ejemplo**

```c
int r = zxv_dir_make("SD:/partidas");
if (r < 0 && r != ZXV_E_EXIST)
    zxv_report_error(r, "No puedo crear la carpeta");
```

**Desde:** especificación 0.1 · función n.º 25.

#### `zxv_dir_change`

```c
int zxv_dir_change(const char *name);
```

Cambia la carpeta actual. Con una unidad delante, también la unidad actual:
`zxv_dir_change("ROM:")` entra en la raíz del disco ROM,
`zxv_dir_change("EXT:")` en la de la unidad externa y
`zxv_dir_change("SD:")` vuelve a la raíz de la tarjeta.

**Devuelve:** 0, o un error. Con un error no cambia nada.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOENT` | No existe (o no existe una carpeta de la ruta, o no hay disco ROM). |
| `ZXV_E_NOTDIR` | Es un fichero, o una parte de la ruta no es una carpeta. |
| `ZXV_E_NAME` | Como en [`zxv_file_open`](#zxv_file_open). |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). |

**Ejemplo**

```c
if (zxv_dir_change("ROM:/graphics") == 0)
    zxv_file_load("sprites.gfx", grafs, sizeof grafs);
```

**Desde:** especificación 0.1 · función n.º 26.

#### `zxv_dir_current`

```c
int zxv_dir_current(char *buf, uint32_t len);
```

Copia en `buf` la carpeta actual, terminada en cero: `/...` en la tarjeta,
`ROM:/...` en la unidad ROM y `EXT:/...` en la externa.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `buf` | `len` bytes en la RAM | Dónde. |
| `len` | bytes | El sitio en `buf`, con el cero. Con `ZXV_PATH_MAX + 1` (256) cabe cualquiera. |

**Devuelve:** la longitud (sin el cero), o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_RANGE` | No cabe en `len` bytes (o la ruta, con la unidad delante, pasaría de 255). |
| `ZXV_E_FAULT` | `buf` no tiene `len` bytes en la RAM. |

**Ejemplo**

```c
char cwd[ZXV_PATH_MAX + 1];
if (zxv_dir_current(cwd, sizeof cwd) >= 0)
    zxv_print_str(cwd);
```

**Desde:** especificación 0.1 · función n.º 27.

#### `zxv_dir_open`

```c
int zxv_dir_open(const char *name);
```

Abre una carpeta para listarla con `zxv_dir_read`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | ruta, `""` o `NULL` | La carpeta; `""` o `NULL`, la actual. |

**Devuelve:** el número de la carpeta abierta (0 o más), o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOENT` | No existe (o no existe una carpeta de la ruta, o no hay disco ROM). |
| `ZXV_E_NOTDIR` | Es un fichero, o una parte de la ruta no es una carpeta. |
| `ZXV_E_MFILE` | Ya hay 4 carpetas abiertas en esa unidad. |
| `ZXV_E_NAME` | Como en [`zxv_file_open`](#zxv_file_open). |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). |

**Ejemplo:** ver [`zxv_dir_read`](#zxv_dir_read).

**Desde:** especificación 0.1 · función n.º 28.

#### `zxv_dir_read`

```c
int zxv_dir_read(int d, zxv_dirent_t *e);
```

Da la siguiente entrada de una carpeta abierta (un fichero o una carpeta),
en orden alfabético sin distinguir mayúsculas. No da `.` ni `..`. En la
tarjeta, las entradas ocultas o con un nombre que no vale no salen
([Rutas](#rutas)).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `d` | un número de `zxv_dir_open` | La carpeta. |
| `e` | 72 bytes en la RAM | Dónde deja la entrada, como en [`zxv_file_stat`](#zxv_file_stat). |

**Devuelve:** 1 si deja una entrada en `*e`, 0 al final, o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_BADF` | `d` no es una carpeta abierta. |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). |

**Ejemplo**

```c
zxv_dirent_t e;
int d = zxv_dir_open("");
if (d >= 0) {
    while (zxv_dir_read(d, &e) == 1) {
        zxv_print_str(e.name);
        zxv_print_str(e.attr & ZXV_ATTR_DIR ? "/\n" : "\n");
    }
    zxv_dir_close(d);
}
```

**Desde:** especificación 0.1 · función n.º 29.

#### `zxv_dir_close`

```c
int zxv_dir_close(int d);
```

Cierra una carpeta abierta y deja libre su número.

**Devuelve:** 0, o un error. Una carpeta de una tarjeta que se ha sacado, o
de `EXT:` sin servidor, se cierra igual y da 0.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_BADF` | `d` no es una carpeta abierta. |

**Ejemplo:** ver [`zxv_dir_read`](#zxv_dir_read).

**Desde:** especificación 0.1 · función n.º 30.

### Constantes

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_O_READ`, `_WRITE`, `_CREATE`, `_TRUNC`, `_APPEND` | 0x01, 0x02, 0x04, 0x08, 0x10 | Indicadores de `zxv_file_open` |
| `ZXV_SEEK_SET`, `_CUR`, `_END` | 0, 1, 2 | `whence` de `zxv_file_seek` |
| `ZXV_NAME_MAX` | 63 | Caracteres por nombre |
| `ZXV_PATH_MAX` | 255 | Caracteres por ruta |
| `ZXV_MAX_FILES` | 8 | Ficheros abiertos a la vez (por unidad) |
| `ZXV_MAX_DIRS` | 4 | Carpetas abiertas a la vez (por unidad) |
| `ZXV_ATTR_DIR` | 0x01 | Atributo: carpeta |
| `ZXV_DIRENT_SIZE`, `ZXV_DE_NAME`, `_SIZE`, `_ATTR` | 72, 0, 64, 68 | `zxv_dirent_t` para el ensamblador |

## Bancos

`zxv_bank.h` y `zxvbank.c`: datos que van al final del propio `.zxv` y se
cargan cuando hacen falta; y, con `tools/zxvbanks.py`, también código. Las
explicaciones están en la guía: [Bancos: más datos de los que caben a la
vez](README.md#bancos-más-datos-de-los-que-caben-a-la-vez) y [Bancos de
código](README.md#bancos-de-código-más-código-del-que-cabe-a-la-vez).

- Los bancos de datos se ponen al convertir el ELF:
  `elf2zxv.py juego.elf juego.zxv --bank fase1=fase1.gfx --pack` (con
  `--pack`, cada uno va comprimido si así ocupa menos).
- Se leen del propio fichero del programa, que la ROM deja abierto mientras
  va (`ZXV_SV_PROG_FILE`): da igual la carpeta actual o la unidad. Ese
  fichero ocupa uno de los 8 de su unidad.
- Usan unos 2 KB de pila.
- **Necesitan una ROM con la especificación 0.2** o posterior; con una
  anterior, todo da `ZXV_E_ACCESS` (el programa carga y va igual).

### `zxv_bank_load`

```c
int zxv_bank_load(const char *name, void *dst, uint32_t max);
```

Carga un banco de datos en la memoria: lo descomprime directamente en
`dst` si va comprimido, y comprueba su CRC.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | hasta 15 caracteres | El nombre del banco, como en `--bank`. Aquí sí cuentan las mayúsculas. |
| `dst` | en la RAM | Dónde se carga. |
| `max` | bytes | El sitio en `dst`. [`zxv_bank_size`](#zxv_bank_size) dice cuánto hace falta. |

**Devuelve:** lo que ocupa el banco (descomprimido), o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOENT` | No hay un banco de datos con ese nombre (uno de código no vale: lo cargan sus puentes), o el programa no tiene bancos. |
| `ZXV_E_RANGE` | El banco ocupa más de `max`. No se toca `dst`. |
| `ZXV_E_CHANGED` | El fichero ya no es el del programa en marcha (otra versión copiada en la tarjeta, o recompilado en el PC mientras va desde `EXT:`): hay que volver a cargar el programa. |
| `ZXV_E_FORMAT` | El banco está dañado: su CRC no coincide, no se descomprime bien o el fichero se acaba antes. |
| `ZXV_E_ACCESS` | La ROM es anterior a la especificación 0.2. |
| `ZXV_E_NOCARD`, `ZXV_E_OFFLINE`, `ZXV_E_IO` | Los de leer el fichero: sin tarjeta, sin servidor de `EXT:`, un error de lectura. |
| `ZXV_E_FAULT` | `dst` no tiene sitio en la RAM para el banco (en uno comprimido, el programa para con una excepción). |

**Con `ZXV_E_FORMAT` o un error de lectura, lo que haya en `dst` ya no
vale:** se escribe antes de comprobar el CRC. Con los demás errores, `dst`
no se ha tocado.

**Ejemplo**

```c
#include "zxv_bank.h"

ZXV_VRAM1_DATA static uint8_t fase[64 * 1024];

int n = zxv_bank_load("fase2", fase, sizeof fase);
if (n < 0)
    zxv_report_error(n, "No se puede cargar la fase 2");
```

**Necesita:** una ROM con la especificación 0.2. Enlaza `zxvbank.c`.

**Ver también:** [`zxv_bank_size`](#zxv_bank_size),
[`zxv_file_load`](#zxv_file_load), [El formato `.zxv`](#el-formato-zxv).

### `zxv_bank_size`

```c
int zxv_bank_size(const char *name);
```

Lo que ocupa un banco descomprimido, para reservarle sitio antes de
cargarlo. Vale para los de datos y para los de código.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | hasta 15 caracteres | El nombre del banco. |

**Devuelve:** los bytes, o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOENT` | No hay un banco con ese nombre, o el programa no tiene bancos. |
| `ZXV_E_CHANGED`, `ZXV_E_ACCESS`, `ZXV_E_NOCARD`, `ZXV_E_OFFLINE`, `ZXV_E_IO` | Como en [`zxv_bank_load`](#zxv_bank_load). |
| `ZXV_E_FORMAT` | La entrada del banco en el índice no tiene sentido. |

**Ejemplo**

```c
int tam = zxv_bank_size("musica");
if (tam > 0 && (uint32_t)tam <= sizeof buffer)
    zxv_bank_load("musica", buffer, sizeof buffer);
```

**Necesita:** una ROM con la especificación 0.2. Enlaza `zxvbank.c`.

### `zxv_bank_preload`

```c
int zxv_bank_preload(const char *name);
```

Carga ya un banco de código, para elegir cuándo se nota (el cartel de una
fase). Sin ella, se carga solo la primera vez que se llama a una de sus
funciones, y entonces un error para el programa; con ella, el error lo
tratas tú. Solo existe en los programas que se compilan con
`tools/zxvbanks.py` (lo enlaza con `zxvbankcode.c`).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | hasta 15 caracteres | El nombre del banco, como en `--code`. |

**Devuelve:** 0 (también si ya estaba cargado), o un error. Si falla al
leerlo, no queda ningún banco en su zona (se llena de `ebreak`); con los
demás errores, la zona sigue como estaba.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOENT` | El programa no tiene un banco de código con ese nombre. |
| `ZXV_E_ACCESS` | En su zona hay otro banco en uso (se está ejecutando una función suya); el gancho de frame apunta a su zona; o la ROM es anterior a la especificación 0.2. |
| `ZXV_E_FORMAT` | El banco está dañado, o no es el que se enlazó (otra zona, otra dirección, más grande que su zona). |
| `ZXV_E_CHANGED`, `ZXV_E_NOCARD`, `ZXV_E_OFFLINE`, `ZXV_E_IO` | Como en [`zxv_bank_load`](#zxv_bank_load). |

**Ejemplo**

```c
cartel("FASE 2");
int r = zxv_bank_preload("valle");
if (r < 0)
    zxv_report_error(r, "No se puede cargar el valle");
valle_empezar();                    /* ya está: no se nota */
```

**Necesita:** una ROM con la especificación 0.2, y compilar con
`tools/zxvbanks.py`.

**Ver también:** [Los errores de los bancos de
código](#los-errores-de-los-bancos-de-código).

### Los errores de los bancos de código

Un banco de código se carga en su **zona** desde su puente, la primera vez
que se llama a una de sus funciones. Un puente no puede devolver un error,
así que si algo sale mal el programa para con el informe `W`, el mensaje y
el código del error (en el idioma de la máquina):

| Mensaje | Error | Cuándo |
|---|---|---|
| «Banco a en uso: no se puede cargar b en su zona» | `ZXV_E_ACCESS` | Una función de `a` llama (directamente o no) a una de `b`, en la misma zona |
| «Banco b: el gancho del frame apunta a su zona» | `ZXV_E_ACCESS` | El gancho de frame está en la zona de `b` |
| «Banco b: no se carga con las interrupciones quitadas» | `ZXV_E_ACCESS` | Se llama a `b` con las interrupciones quitadas (desde el gancho de frame, por ejemplo), y no está cargado |
| «Demasiadas llamadas a bancos, una dentro de otra» | `ZXV_E_RANGE` | Más de 32 llamadas a bancos anidadas |
| «Banco b: el programa ha cambiado: vuelve a cargarlo» | `ZXV_E_CHANGED` | Como en `zxv_bank_load` |
| «Banco b: está dañado» | `ZXV_E_FORMAT` | Como en `zxv_bank_preload` |
| «Banco b: no está en el fichero» | `ZXV_E_NOENT` | El `.zxv` no lleva ese banco |
| «Banco b: la ROM no tiene bancos» | `ZXV_E_ACCESS` | La ROM es anterior a la especificación 0.2 |
| «Banco b: no hay tarjeta», «Banco b: EXT: sin servidor», «Banco b: no se puede leer» | `ZXV_E_NOCARD`, `ZXV_E_OFFLINE`, otro | Los de leer el fichero |

Por ejemplo: `W Banco valle: está dañado (-14), 0:1`.

Un banco ocupa su zona desde el principio. Sus variables vuelven a empezar
en cada carga, y la zona se rellena con `ebreak` (`c.ebreak`, 0x9002) antes
de cargarlo, para que una llamada a lo que ya no está pare enseguida.

### Constantes de los bancos

Para leer los bancos de un `.zxv` por tu cuenta (en un programa del PC, por
ejemplo). El formato y la entrada de cada banco (`ZXV_BANK_E_*`), en [El
formato `.zxv`](#el-formato-zxv).

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_BANK_MAGIC` | `"ZXVBANK1"` | La firma del cierre, al final del fichero |
| `ZXV_BANK_TAIL` | 16 | Bytes del cierre: la firma (8), dónde empieza el índice y lo que ocupa |
| `ZXV_BANK_T_INDEX`, `ZXV_BANK_T_INDEX_LEN` | 8, 12 | En el cierre: dónde empieza el índice y lo que ocupa (u32) |
| `ZXV_BANK_HEAD` | 16 | Bytes de la cabecera del índice |
| `ZXV_BANK_I_COUNT` | 0 | En la cabecera: cuántos bancos (u32) |
| `ZXV_BANK_I_PROG_CRC` | 4 | El CRC de la imagen del programa, su huella (u32); tiene que ser el de `ZXV_SV_PROG_CRC` |
| `ZXV_BANK_I_IMAGE_SIZE` | 8 | El tamaño de la imagen (u32) |
| `ZXV_BANK_I_CRC` | 12 | El CRC-32 de las entradas (u32) |
| `ZXV_BANK_ENTRY` | 48 | Bytes por entrada, detrás de la cabecera |
| `ZXV_BANK_NAME_MAX` | 15 | Caracteres por nombre |
| `ZXV_BANK_DATA`, `ZXV_BANK_CODE` | 0, 1 | Tipos de banco (`ZXV_BANK_E_KIND`) |
| `ZXV_BANK_STORED`, `ZXV_BANK_DEFLATE` | 0, 1 | Cómo va guardado (`ZXV_BANK_E_METHOD`): tal cual, o con deflate sin envoltorio (RFC 1951) |

## Reloj e idioma

### `zxv_clock_get`, `zxv_clock_set`

```c
typedef struct {
    uint16_t year;                      /* 2000-2099 */
    uint8_t month, day, hour, min, sec;
    uint8_t wday;                       /* 1 lunes ... 7 domingo (lo pone clock_get) */
} zxv_clock_t;

int zxv_clock_get(zxv_clock_t *c);
int zxv_clock_set(const zxv_clock_t *c);
```

Leen y ponen la fecha y la hora del reloj de la máquina: la local, sin zonas
horarias. Con un módulo de reloj con pila, la hora se guarda apagada; sin
él, dura hasta que se apaga la máquina, y al encenderla no está en hora.
Los ficheros que se crean en la tarjeta llevan esta fecha.

Las dos son lentas (preguntan al reloj): para medir tiempos, usa
`zxv_time_us` o `ZXV_SV_FRAMES`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `c` | 8 bytes en la RAM (al poner, también en la ROM) | La fecha y la hora. |
| `year` | 2000–2099 | El año. |
| `month`, `day` | 1–12; 1–28, 29, 30 o 31 | El mes y el día, que tiene que existir (el 29 de febrero, solo en los bisiestos). |
| `hour`, `min`, `sec` | 0–23, 0–59, 0–59 | La hora. |
| `wday` | 1 lunes ... 7 domingo | El día de la semana: lo pone `zxv_clock_get`; `zxv_clock_set` no lo mira. |

**Devuelve:** 0, o un error. `zxv_clock_get` no toca `*c` si da un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_NOCLOCK` | (`zxv_clock_get`) El reloj no está en hora: no se ha puesto desde que se encendió, o el módulo perdió la hora (sin pila). Con un firmware de antes del reloj, las dos. |
| `ZXV_E_INVAL` | (`zxv_clock_set`) La fecha no existe o está fuera de 2000–2099, o la hora no vale. |
| `ZXV_E_FAULT` | `c` no está en la memoria permitida. |

Para el ensamblador: `ZXV_CLOCK_SIZE` (8) y los desplazamientos
`ZXV_CK_YEAR` (0), `ZXV_CK_MONTH` (2), `ZXV_CK_DAY` (3), `ZXV_CK_HOUR` (4),
`ZXV_CK_MIN` (5), `ZXV_CK_SEC` (6), `ZXV_CK_WDAY` (7).

**Ejemplo**

```c
zxv_clock_t c;
if (zxv_clock_get(&c) == 0 && c.month == 12 && c.day == 25)
    poner_gorro_de_navidad();
```

**Desde:** especificación 0.1 · funciones n.º 48 y 49.

**Ver también:** [`zxv_time_us`](#zxv_time_us).

### `zxv_lang_get`, `zxv_lang_set`

```c
uint32_t zxv_lang_get(void);
int zxv_lang_set(uint32_t lang);
```

El idioma de la máquina: un código ISO 639-1 de dos letras minúsculas, la
primera en el byte bajo, que se forma con `ZXV_LANG(a, b)`. `ZXV_LANG_EN` es
`ZXV_LANG('e', 'n')` y `ZXV_LANG_ES` es `ZXV_LANG('e', 's')`.

La ROM tiene sus textos en inglés y en español (con cualquier otro código,
los da en inglés), pero cualquier código vale, así que los programas
pueden usarlo para elegir los suyos. `zxv_lang_get` lee la variable del
sistema `ZXV_SV_LANG`. `zxv_lang_set` lo cambia; con un módulo de reloj, la
máquina lo recuerda apagada (sin él, hasta que se apaga).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `lang` | `ZXV_LANG(a, b)`, `a` y `b` de `'a'` a `'z'` | El idioma. |

**Devuelve:** `zxv_lang_get`, el código (en inglés, `ZXV_LANG_EN`, si
nunca se ha elegido); `zxv_lang_set`, 0 o un error.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_INVAL` | (`zxv_lang_set`) El código no son dos letras minúsculas (o tiene algo en los bits 16–31). No cambia nada. |

**Ejemplo**

```c
const char *hola = zxv_lang_get() == ZXV_LANG_ES ? "Hola" : "Hello";
```

**Desde:** especificación 0.1. `zxv_lang_get` está en `zxv_api.h` (lee la
variable del sistema); `zxv_lang_set` es la función n.º 50.

## Terminar un programa y ejecutar otros

Al terminar un programa (de cualquier forma), la memoria vuelve a ser del
BASIC, así que el programa BASIC que hubiera en ella se pierde. La ROM quita
el gancho de frame, vuelve a la página 0 sin scroll ni campo virtual y
calla los dos AY. El siguiente `.zxv` empieza con los colores, las paletas
y el texto de siempre.

### `zxv_exit_to_basic`

```c
void zxv_exit_to_basic(int code);
```

Termina el programa desde cualquier sitio, por muy dentro que esté, y
vuelve al BASIC: lo mismo que devolver `code` desde `main`. No vuelve.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `code` | cualquier `int` | **El 0 significa que todo ha ido bien**: el BASIC enseña `0 OK, 0:1`. Cualquier otro es un error, y el BASIC enseña `W Error en el código nativo (code), 0:1` (en inglés, `W Native code error (code), 0:1`), con su signo si es negativo. |

El valor que devuelve `main` funciona igual. El código nativo al que se
llama desde el BASIC con `USR` es distinto: lo que devuelve es sin más el
valor de `USR`.

**Ejemplo**

```c
if (!hay_tarjeta())
    zxv_exit_to_basic(2);           /* W Error en el código nativo (2), 0:1 */
```

**Desde:** especificación 0.1 · función n.º 19.

**Ver también:** [`zxv_report_error`](#zxv_report_error).

### `zxv_exec`

```c
int zxv_exec(const char *name, int *result);
```

Carga y ejecuta un `.zxv`, y vuelve cuando acaba. Es lo que hacen el BASIC
(`LOAD`) y el monitor del sistema.

**Un programa nativo no puede usarla**: llamada desde un `.zxv` (lo haya
lanzado el BASIC o el monitor), devuelve `ZXV_E_ACCESS` y no hace nada. Todos los programas se cargan en la misma
dirección, al principio de la RAM de usuario, así que el nuevo pisaría al
que lo llama.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `name` | ruta | El programa. Si no tiene extensión y no existe, se prueba con `.zxv` detrás. |
| `result` | | Dónde deja lo que devolvió el programa. |

**Devuelve:** 0 si el programa se cargó y terminó volviendo de `main` (su
valor, en `*result`); o un error, y no se ejecuta nada. El fichero se
comprueba entero (la cabecera, que quepa y el CRC) antes de tocar la
memoria.

**Errores**

| Error | Cuándo |
|---|---|
| `ZXV_E_ACCESS` | La llama un programa nativo. |
| `ZXV_E_FORMAT` | No es un programa válido: la cabecera, la pantalla de carga o el CRC no cuadran, o el fichero se acaba antes. |
| `ZXV_E_RANGE` | No cabe en la RAM de usuario (o su pila no vale). |
| `ZXV_E_CHANGED` | El fichero ha cambiado entre la comprobación y la carga. |
| `ZXV_E_NOENT`, `ZXV_E_ISDIR`, `ZXV_E_NAME`, `ZXV_E_NOTDIR`, `ZXV_E_MFILE` | Como en [`zxv_file_open`](#zxv_file_open). |
| (comunes) | Ver [Errores comunes](#ficheros-y-carpetas). |

**Desde:** especificación 0.1 · función n.º 31.

**Ver también:** [El formato `.zxv`](#el-formato-zxv).

### `zxv_report_error`

```c
void zxv_report_error(int code, const char *msg);
```

Termina el programa y vuelve al BASIC con el informe `W` y tu propio
mensaje en vez del del BASIC. No vuelve. El mensaje se copia antes de que
el BASIC recupere la memoria, así que puede estar en una matriz local.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `code` | cualquier `int` | Si no es 0, va detrás del mensaje entre paréntesis, con su signo. Un error `ZXV_E_*` sale tal cual: `(-1)`. |
| `msg` | UTF-8, hasta 79 bytes | El mensaje. Lo que pase de 79 bytes no sale (con una ROM anterior a la especificación 0.2, de 47). Vacío (`""`), o `NULL`, deja el texto de siempre del BASIC. |

Por ejemplo, `zxv_report_error(5, "Fichero dañado")` da
`W Fichero dañado (5), 0:1`; `zxv_report_error(0, "Faltan los niveles")`
da `W Faltan los niveles, 0:1`; y `zxv_report_error(5, "")`,
`W Error en el código nativo (5), 0:1`.

La fuente de la pantalla del BASIC tiene las letras del español, así que el
mensaje puede llevar `ñ`, `¿` o `á`; los demás caracteres que no son ASCII
salen como `?`. Cuenta los bytes, no las letras: en UTF-8, cada letra con
tilde ocupa 2.

**Ejemplo**

```c
int tam = zxv_file_load("nivel1.dat", nivel, sizeof nivel);
if (tam < 0)
    zxv_report_error(tam, "No puedo cargar nivel1.dat");
/* si no existe: W No puedo cargar nivel1.dat (-1), 0:1 */
```

**Desde:** especificación 0.1 · función n.º 20 (hasta 79 bytes desde la
especificación 0.2; antes, 47).

**Ver también:** [`zxv_exit_to_basic`](#zxv_exit_to_basic),
[Códigos de error](#códigos-de-error).

## Motor gráfico

Declarado en `zxv_gfx.h`. Ver [Gráficos](graphics.md) para las
explicaciones y los ejemplos largos. Todas las funciones dibujan en el
**destino** actual (la página de dibujo o un bitmap en memoria) y recortan
contra él y contra el rectángulo de recorte. El resultado está definido
píxel a píxel por el código de la ROM: el emulador y la máquina dibujan
exactamente los mismos píxeles.

**Comportamiento común.** Ninguna función del motor devuelve un error. Lo
que se sale de lo admitido se trata siempre igual:

| Caso | Qué hace la ROM |
|---|---|
| Coordenadas fuera del destino o del recorte | Recorta: se dibuja solo lo que cae dentro. Las coordenadas llevan signo, así que una figura o un sprite pueden estar a medias fuera. Nunca se escribe fuera del destino. |
| Un color de más de 15 o negativo | Se usan sus 4 bits bajos (`c & 15`): 16 es el 0 y −1, el 15. Salvo `paper` en `zxv_gfx_pattern` y `out` en `zxv_m7_draw_map`, donde vale como `ZXV_CLEAR`. |
| Un tamaño de 0 o negativo, un puntero `NULL` o un sprite que no es de `zxv_spr_prepare` | No dibuja nada (lo que hace cada función, en su entrada). |
| Un bitmap, unos tiles o un sprite preparado sin alinear a 4 bytes | No se comprueba: el motor lee y escribe palabras de 32 bits, y un acceso desalineado detiene el programa. |
| Un bitmap con `pix`, `stride`, `w` o `h` que no corresponden a su memoria | No se comprueba: se escribe donde digan. |

Al empezar un programa (y cada vez que `zxv_exec` arranca otro), el
destino es la página de dibujo 0, sin rectángulo de recorte y sin trama.
`zxv_cls`, `zxv_plot` y el texto no usan el motor: dibujan siempre en la
página de dibujo, aunque el destino sea un bitmap, y no hacen caso del
recorte ni de la trama.

### Tipos

```c
typedef struct {            /* un bitmap */
    uint8_t *pix;           /* primer píxel, alineado a 4 */
    uint16_t stride;        /* bytes por línea, múltiplo de 4 */
    uint16_t w, h;          /* en píxeles */
    uint16_t reserved;
} zxv_bitmap;

typedef struct {            /* un mapa de tiles */
    const uint16_t *cells;  /* celdas, fila a fila */
    uint16_t w, h;          /* en celdas */
    const uint8_t *tiles;   /* gráficos de los tiles, seguidos (32 o 128 bytes) */
    uint8_t size;           /* lado del tile: 8 o 16 */
    uint8_t flags;          /* ZXV_MAP_WRAP */
} zxv_tilemap;

typedef struct {            /* una textura del modo 7 */
    const uint8_t *texels;  /* 1 byte por texel (0-15), fila a fila */
    uint8_t log2size;       /* lado = 2^log2size (de 1 a 10) */
    uint8_t wrap;           /* distinto de 0: se repite; 0: fuera es del color 'out' */
    uint8_t out;
    uint8_t flags;          /* ZXV_M7_TRANSPARENT; antes 'reserved', a 0 */
} zxv_m7tex;

typedef struct { int32_t u, v, du, dv; } zxv_m7line;   /* coma fija 16.16 */

typedef struct {            /* un plano girado y ampliado, coma fija 16.16 */
    int32_t x, y;           /* el punto del plano que va en el centro */
    int32_t angle;          /* 0x10000 = una vuelta, en el sentido del reloj */
    int32_t zoom_x, zoom_y; /* aumento en cada eje del plano: 0x10000 = 1 */
    int16_t cx, cy;         /* el centro, en el destino (enteros) */
} zxv_m7aff;

typedef struct {            /* una cámara del modo 7, coma fija 16.16 */
    int32_t x, y;           /* posición sobre el plano */
    int32_t height;         /* altura sobre el suelo */
    int32_t angle;          /* 0x10000 = una vuelta; 0 mira hacia u creciente */
    int32_t focal;          /* distancia focal, en píxeles (también 16.16) */
    int16_t horizon;        /* línea del destino del horizonte (entera) */
    int16_t reserved;
} zxv_m7cam;
```

Los píxeles son de 4 bits, dos por byte, el par en los 4 bits bajos, como
en la VRAM. Un bitmap de `w` píxeles de ancho necesita al menos `w / 2`
bytes por línea, redondeado a un múltiplo de 4: un bitmap de 64 × 32 es de
32 bytes por línea y 1.024 bytes. Los bitmaps, los tiles y los sprites
preparados tienen que estar alineados a 4 bytes (una matriz de `uint32_t`
lo está siempre).

Un tile de 8 × 8 son 32 bytes (4 por línea) y uno de 16 × 16, 128 (8 por
línea), seguidos en `tiles`: el tile *n* empieza en `tiles + n·32` o
`tiles + n·128`. Una celda del mapa es un número de 16 bits con el número
de tile y sus atributos (las constantes `ZXV_CELL_*`, abajo).

### Constantes del motor gráfico

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_TRANSPARENT` | 8 | El color transparente (en `zxv_gfx_blit` con `ZXV_BLIT_TRANSPARENT`, en los sprites, en las celdas con `ZXV_CELL_TRANSP` y en las texturas con `ZXV_M7_TRANSPARENT`) |
| `ZXV_CLEAR` | −1 | Un papel (`zxv_gfx_pattern`) o un color de fuera (`zxv_m7_draw_map`) que no se pinta |
| `ZXV_FLIP_H`, `ZXV_FLIP_V` | 0x01, 0x02 | Espejo horizontal, vertical (`zxv_gfx_blit` y los sprites) |
| `ZXV_BLIT_TRANSPARENT` | 0x04 | `zxv_gfx_blit`: no copiar el color 8 |
| `ZXV_MAP_WRAP` | 0x01 | `zxv_tilemap.flags`: el mapa se repite en las dos direcciones |
| `ZXV_CELL_TILE` | 0x0FFF | Celda: número de tile (0–4095) |
| `ZXV_CELL_FLIP_H`, `ZXV_CELL_FLIP_V` | 0x1000, 0x2000 | Celda: con espejo horizontal, vertical |
| `ZXV_CELL_TRANSP` | 0x4000 | Celda: el color 8 del tile deja ver lo de debajo (sin él, el tile es opaco y se dibuja más deprisa) |
| `ZXV_CELL_PRIO` | 0x8000 | Celda: prioridad (para el filtro de `zxv_map_draw`) |
| `ZXV_MAP_ALL`, `ZXV_MAP_BACK`, `ZXV_MAP_FRONT` | 0, 1, 2 | Filtro de `zxv_map_draw`: todas las celdas, solo las que no tienen prioridad, solo las que la tienen |
| `ZXV_M7_TRANSPARENT` | 0x01 | `zxv_m7tex.flags`: el color 8 de la textura (y `out` si es el 8) no se dibuja. Desde la especificación 0.2 |

Los ángulos (`zxv_m7cam.angle`, `zxv_m7aff.angle`) van en vueltas de
0x10000: 0x4000 es un cuarto de vuelta. La biblioteca de cuentas,
[`zxv_math.h`](libraries.md#cuentas-zxv_mathh), lo llama `ZXV_TURN`
(0x10000) y da `ZXV_DEG(grados)`, con el mismo seno que la ROM.

### Destino y recorte

Dónde se dibuja: la página de dibujo (por defecto) o un bitmap en memoria,
y el rectángulo de recorte dentro de él.

### `zxv_gfx_target`

```c
void zxv_gfx_target(const zxv_bitmap *b);
```

Hace destino al bitmap `b`: desde ahora, todo el motor dibuja en él. Con
`NULL` (o `0`), el destino vuelve a ser la página de dibujo. Siempre quita
el rectángulo de recorte. La estructura se copia, así que puede ser una
variable local; los píxeles no, claro: tienen que seguir ahí mientras se
dibuja en ellos.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `b` | un bitmap, o `NULL` | El nuevo destino. `NULL`: la página de dibujo, con el tamaño que tenga en cada momento (ver [`zxv_gfx_draw_page`](#zxv_gfx_draw_page)). |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `b->pix` es `NULL`, o `w` o `h` valen 0 | Lo acepta, pero nada se dibuja hasta que cambie el destino. |
| `pix` sin alinear a 4, `stride` que no es múltiplo de 4 o menor que `w / 2` | No se comprueba (ver [Comportamiento común](#motor-gráfico)). |

**Ejemplo**

```c
static uint32_t buf_pix[64 * 32 / 8];           /* 64 x 32 píxeles, alineado a 4 */
const zxv_bitmap buf = { (uint8_t *)buf_pix, 32, 64, 32, 0 };
zxv_gfx_target(&buf);                           /* dibujar en memoria */
zxv_gfx_rect(0, 0, 64, 32, 1);
zxv_gfx_circle(32, 16, 12, 6, 1);
zxv_gfx_target(0);                              /* otra vez la página de dibujo */
zxv_gfx_blit(&buf, 0, 0, 64, 32, 96, 80, 0);    /* y se copia en ella */
```

**Desde:** especificación 0.1 · función n.º 32 de la API.

**Ver también:** [`zxv_gfx_draw_page`](#zxv_gfx_draw_page),
[`zxv_gfx_clip`](#zxv_gfx_clip), [`zxv_gfx_blit`](#zxv_gfx_blit).

### `zxv_gfx_draw_page`

```c
void zxv_gfx_draw_page(int page);
```

Elige la página de dibujo: la de la VRAM en la que dibujan el motor (cuando
el destino es la página), `zxv_cls`, `zxv_plot` y el texto. No cambia la
página que se ve (eso es [`zxv_set_page`](#zxv_set_page)), así que sirve
para el doble buffer: dibujar en la que no se ve y luego enseñarla. No
toca el destino: si es un bitmap, el motor sigue dibujando en él. Tampoco
quita el recorte.

El tamaño de la página sigue al modo de vídeo y a `ZXV_V_VSIZE` en cada
llamada al motor: 256 × 192 en el modo 0 (o el de `ZXV_V_VSIZE`, ver
[Registros de vídeo](#registros-de-vídeo)) y 512 × 384 en el modo 1.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `page` | 0 o 1 | La página: 0 en `ZXV_VRAM0_BASE` y 1 en `ZXV_VRAM1_BASE`. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| Otro valor | Cuenta su bit bajo (`page & 1`): 2 es la 0 y −1, la 1. |

**Ejemplo**

```c
int visible = 0;
for (int frame = 0; zxv_key_get() < 0; frame++) {
    int hidden = 1 - visible;
    zxv_gfx_draw_page(hidden);                  /* se dibuja en la que no se ve */
    zxv_cls(0);
    zxv_gfx_circle(frame % 256, 96, 20, 6, 1);
    zxv_set_page(hidden);                       /* y se enseña */
    zxv_wait_vblank();
    visible = hidden;
}
zxv_set_page(0);
zxv_gfx_draw_page(0);
```

**Desde:** especificación 0.1 · función n.º 33 de la API.

**Ver también:** [`zxv_set_page`](#zxv_set_page),
[`zxv_gfx_target`](#zxv_gfx_target),
[Gráficos: las páginas y el doble buffer](graphics.md#las-páginas-y-el-doble-buffer).

### `zxv_gfx_clip`

```c
void zxv_gfx_clip(int x0, int y0, int x1, int y1);
```

Limita el dibujo del motor al rectángulo de (x0, y0) a (x1, y1), sin incluir
x1 ni y1, en coordenadas del destino. Lo que cae fuera no se toca. El
rectángulo se guarda tal cual y en cada dibujo se cruza con el destino, así
que puede salirse de él. Dura hasta la siguiente `zxv_gfx_clip` o
`zxv_gfx_target` (que lo quita); `zxv_gfx_draw_page` no lo cambia.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x0`, `y0` | cualquier entero | La esquina de arriba a la izquierda, incluida. |
| `x1`, `y1` | cualquier entero | La esquina de abajo a la derecha, **excluida**: el rectángulo mide x1 − x0 × y1 − y0. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `x1 <= x0` o `y1 <= y0`, o el rectángulo no toca el destino | Lo acepta: no se dibuja nada hasta que cambie el recorte. |

Para quitar el recorte sin cambiar de destino, vuelve a dar el mismo:
`zxv_gfx_target(0)` (o el bitmap que fuera).

**Ejemplo**

```c
zxv_gfx_clip(0, 0, 256, 160);           /* el marcador, abajo, no se toca */
zxv_gfx_circle(128, 150, 40, 2, 1);     /* sale cortado por la línea 160 */
zxv_gfx_target(0);                      /* sin recorte otra vez */
```

**Desde:** especificación 0.1 · función n.º 34 de la API.

**Ver también:** [`zxv_gfx_target`](#zxv_gfx_target).

### Figuras

Líneas, rectángulos, triángulos, polígonos y círculos, de un color liso o
con la trama de [`zxv_gfx_pattern`](#zxv_gfx_pattern). Los triángulos y
los polígonos siguen una **regla de arriba a la izquierda**: un píxel se
pinta si su centro cae dentro, con los lados de arriba y de la izquierda
incluidos; así, dos que comparten un lado ni se solapan ni dejan huecos.

### `zxv_gfx_line`

```c
void zxv_gfx_line(int x0, int y0, int x1, int y1, int c);
```

Una línea de (x0, y0) a (x1, y1), con los dos extremos. En cada paso por el
eje más largo, el otro avanza al píxel más cercano a la recta (los empates,
hacia el final). Con la trama puesta, cada píxel toma el color que le toca
en la trama según dónde cae.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x0`, `y0` | cualquier entero | Un extremo. |
| `x1`, `y1` | cualquier entero | El otro. Si es el mismo punto, un píxel. |
| `c` | 0–15 | El color. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| Extremos fuera del destino, aunque sea muy lejos | Recorta: dibuja exactamente los píxeles que dibujaría sin recortar y caen dentro. |
| `c` fuera de 0–15 | (común) [Sus 4 bits bajos](#motor-gráfico). |

**Ejemplo**

```c
for (int i = 0; i < 16; i++)
    zxv_gfx_line(128, 191, i * 17, 0, i);       /* un abanico con los 16 colores */
```

**Desde:** especificación 0.1 · función n.º 43 de la API.

**Ver también:** [`zxv_plot`](#zxv_plot), [`zxv_gfx_poly`](#zxv_gfx_poly),
[`zxv_gfx_pattern`](#zxv_gfx_pattern).

### `zxv_gfx_rect`

```c
void zxv_gfx_rect(int x, int y, int w, int h, int c);
```

Un rectángulo relleno de `w` × `h` píxeles con la esquina de arriba a la
izquierda en (x, y). Es la forma más rápida de rellenar: de 8 en 8 píxeles,
también con trama.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x`, `y` | cualquier entero | La esquina de arriba a la izquierda. |
| `w`, `h` | 1 o más | El ancho y el alto, en píxeles. |
| `c` | 0–15 | El color. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `w` o `h` valen 0 o menos | Nada. |
| `c` fuera de 0–15 | (común) [Sus 4 bits bajos](#motor-gráfico). |

**Ejemplo**

```c
zxv_gfx_rect(0, 160, 256, 32, 1);       /* una franja azul abajo, para el marcador */
```

**Desde:** especificación 0.1 · función n.º 44 de la API.

**Ver también:** [`zxv_cls`](#zxv_cls), [`zxv_gfx_poly`](#zxv_gfx_poly).

### `zxv_gfx_tri`

```c
void zxv_gfx_tri(const int16_t *p, int c);
```

Un triángulo relleno. Es lo mismo que `zxv_gfx_poly(p, 3, c)`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `p` | 6 números de −32768 a 32767 | Los tres vértices, como pares x, y: `{ x0, y0, x1, y1, x2, y2 }`. Da igual el sentido. |
| `c` | 0–15 | El color. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `p` es `NULL` | Nada. |
| Los tres vértices en línea | Nada (no cubre el centro de ningún píxel). |
| `c` fuera de 0–15 | (común) [Sus 4 bits bajos](#motor-gráfico). |

**Ejemplo**

```c
static const int16_t t[] = { 128, 20,  200, 170,  56, 170 };
zxv_gfx_tri(t, 4);                      /* un triángulo verde */
```

**Desde:** especificación 0.1 · función n.º 45 de la API.

**Ver también:** [`zxv_gfx_poly`](#zxv_gfx_poly).

### `zxv_gfx_poly`

```c
void zxv_gfx_poly(const int16_t *p, int n, int c);
```

Un polígono relleno de `n` vértices, con la regla par-impar: un píxel se
pinta si una línea desde él cruza un número impar de lados. Así, una
estrella de cinco puntas dibujada de un trazo tiene un hueco en el centro,
y un polígono puede tener agujeros si se recorre su borde de ida y vuelta.
Los vértices se unen en orden y el último con el primero.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `p` | `2·n` números de −32768 a 32767 | Los vértices, como pares x, y. |
| `n` | 3–64 | Cuántos vértices. |
| `c` | 0–15 | El color. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `p` es `NULL`, o `n` menor que 3 o mayor que 64 | Nada. |
| `c` fuera de 0–15 | (común) [Sus 4 bits bajos](#motor-gráfico). |

**Ejemplo**

```c
static const int16_t star[] = { 128, 20,  168, 160,  56, 74,  200, 74,  88, 160 };
zxv_gfx_poly(star, 5, 6);               /* una estrella amarilla, hueca en el centro */
```

**Desde:** especificación 0.1 · función n.º 46 de la API.

**Ver también:** [`zxv_gfx_tri`](#zxv_gfx_tri),
[`zxv_gfx_pattern`](#zxv_gfx_pattern).

### `zxv_gfx_circle`

```c
void zxv_gfx_circle(int x, int y, int r, int c, int fill);
```

Un círculo de radio `r` con el centro en (x, y): solo el contorno, o un
disco relleno. Mide 2·r + 1 píxeles de lado a lado.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `x`, `y` | cualquier entero | El centro. |
| `r` | 0 o más | El radio, en píxeles. Con 0, un píxel. |
| `c` | 0–15 | El color. |
| `fill` | 0 u otro | 0: solo el contorno; otro valor: un disco. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `r` negativo | Nada. |
| `c` fuera de 0–15 | (común) [Sus 4 bits bajos](#motor-gráfico). |

**Ejemplo**

```c
zxv_gfx_circle(128, 96, 50, 2, 1);      /* un disco rojo... */
zxv_gfx_circle(128, 96, 50, 7, 0);      /* ...con el borde blanco */
```

**Desde:** especificación 0.1 · función n.º 47 de la API.

**Ver también:** [`zxv_gfx_pattern`](#zxv_gfx_pattern).

### `zxv_gfx_pattern`

```c
void zxv_gfx_pattern(const uint8_t *pat, int paper);
```

Pone una **trama** de 8 × 8 para las figuras: `zxv_gfx_line`,
`zxv_gfx_rect`, `zxv_gfx_tri`, `zxv_gfx_poly` y `zxv_gfx_circle`. Donde la
trama tiene un 1 se pinta el color de la figura y donde tiene un 0, el
color `paper`, o nada con `ZXV_CLEAR`. La trama va anclada al destino: el
píxel (x, y) usa el bit 7 − (x mod 8) de `pat[y mod 8]`, así que dos
figuras que se tocan casan. Rellenar con trama cuesta lo mismo que con
color liso. En las líneas y los contornos se ve según dónde cae cada píxel:
una diagonal a 45° sobre un ajedrez queda entera de un color.

La trama se copia: `pat` puede ser una variable local. Dura hasta la
siguiente llamada; cada programa empieza sin trama. No afecta a las copias,
los sprites, los mapas ni el modo 7, ni a `zxv_cls`, `zxv_plot` o el texto.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `pat` | 8 bytes, o `NULL` | Una fila por byte, de arriba abajo; el bit 7 es el píxel de la izquierda. `NULL` (o `0`): vuelve al color liso. |
| `paper` | 0–15, o `ZXV_CLEAR` | El color de los ceros de la trama, o `ZXV_CLEAR` para no pintarlos (lo de debajo se ve). Con `pat` a `NULL` no se usa. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `paper` negativo o mayor que 15 | Como `ZXV_CLEAR`: los ceros no se pintan. |
| Con una ROM de la especificación 0.1 | La función no existe: no hace nada y las figuras salen lisas. |

**Ejemplo**

```c
static const uint8_t check[8] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
zxv_gfx_pattern(check, 0);              /* ajedrez del color de la figura y negro */
zxv_gfx_rect(16, 16, 96, 64, 2);
zxv_gfx_pattern(check, ZXV_CLEAR);      /* aquí, lo de debajo se ve entre los puntos */
zxv_gfx_circle(160, 100, 40, 6, 1);
zxv_gfx_pattern(0, 0);                  /* color liso otra vez */
```

**Desde:** especificación 0.2 · función n.º 56 de la API.

**Ver también:** [`PATTERN`](../basic/reference.md#pattern) en el BASIC,
[Gráficos: figuras con trama](graphics.md#figuras-con-trama).

### Copias

Copiar un rectángulo de un bitmap al destino, tal cual.

### `zxv_gfx_blit`

```c
void zxv_gfx_blit(const zxv_bitmap *src, int sx, int sy, int w, int h, int x, int y, int flags);
```

Copia el rectángulo (sx, sy) de `w` × `h` píxeles de `src` en (x, y) del
destino, con espejo si se pide, y sin el color 8 con
`ZXV_BLIT_TRANSPARENT`. La parte del rectángulo que cae fuera de `src` no
se copia (no se lee fuera del bitmap). Con espejo, el rectángulo se da la
vuelta entero y luego se coloca en (x, y). Para muchas copias del mismo
dibujo con el 8 transparente, un sprite preparado es más rápido.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `src` | un bitmap | De dónde se copia. Puede ser una página de la VRAM descrita como bitmap. |
| `sx`, `sy` | cualquier entero | La esquina del rectángulo en `src`. |
| `w`, `h` | 1 o más | El tamaño del rectángulo. La parte que cae dentro de `src` puede tener hasta 1024 píxeles de ancho. |
| `x`, `y` | cualquier entero | Dónde va la esquina en el destino. |
| `flags` | `ZXV_FLIP_H`, `ZXV_FLIP_V`, `ZXV_BLIT_TRANSPARENT`, combinados con `\|`, o 0 | Espejos y transparencia. Los demás bits no cuentan. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `src` es `NULL`, o `w` o `h` valen 0 o menos | Nada. |
| El rectángulo no toca `src` | Nada. |
| La parte dentro de `src` mide más de 1024 píxeles de ancho | Nada. |
| `src` es el destino y las dos zonas se solapan | No está definido. |

**Ejemplo**

```c
static uint32_t strip_pix[64 * 16 / 8];         /* 4 fotogramas de 16 x 16 */
const zxv_bitmap strip = { (uint8_t *)strip_pix, 32, 64, 16, 0 };
int f = 2, x = 100, y = 80;
/* El fotograma f, mirando a la izquierda, sin su color 8. */
zxv_gfx_blit(&strip, f * 16, 0, 16, 16, x, y, ZXV_FLIP_H | ZXV_BLIT_TRANSPARENT);
```

**Desde:** especificación 0.1 · función n.º 35 de la API.

**Ver también:** [`zxv_gfx_target`](#zxv_gfx_target),
[`zxv_spr_prepare`](#zxv_spr_prepare),
[`zxv_gfxf_sprite`](#zxv_gfxf_sprite).

### Sprites

Un **sprite preparado** es un dibujo con su máscara ya calculada, para
dibujarlo con el 8 transparente de 8 en 8 píxeles. Se hace una vez con
`zxv_spr_prepare` y se dibuja todas las que haga falta. Su formato interno
es de la ROM: un sprite se usa solo con estas funciones.

### `zxv_spr_prepare`

```c
uint32_t zxv_spr_prepare(const zxv_bitmap *src, int sx, int sy, int w, int h, void *buf);
```

Convierte el rectángulo (sx, sy) de `w` × `h` píxeles de `src` en un sprite
preparado, en `buf`. Los píxeles del color 8, y los del rectángulo que caen
fuera de `src`, quedan transparentes. Con `buf` a `NULL` no escribe nada y
solo dice cuánto ocupa: así se pregunta el tamaño antes.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `src` | un bitmap | El dibujo. Hace falta también para preguntar el tamaño. |
| `sx`, `sy` | cualquier entero | La esquina del rectángulo en `src`. |
| `w`, `h` | 1–1024 | El tamaño del sprite. |
| `buf` | memoria alineada a 4, o `NULL` | Dónde se escribe, con sitio para lo que devuelve (no se comprueba). |

**Devuelve:** el tamaño del sprite en bytes, 8 + h × ⌈w / 8⌉ × 8 (264
para uno de 16 × 16), lo haya escrito o no; 0 si no se puede preparar.

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `src` es `NULL`, o `w` o `h` fuera de 1–1024 | Devuelve 0 y no escribe nada. |
| `buf` más pequeño que el tamaño | No se comprueba: escribe detrás. |

**Ejemplo**

```c
static uint32_t sheet_pix[32 * 16 / 8];         /* dos dibujos de 16 x 16 */
const zxv_bitmap sheet = { (uint8_t *)sheet_pix, 16, 32, 16, 0 };
static uint32_t ship[264 / 4];                  /* 8 + 16 · 2 · 8 bytes */
uint32_t n = zxv_spr_prepare(&sheet, 0, 0, 16, 16, NULL);    /* 264 */
if (n && n <= sizeof ship)
    zxv_spr_prepare(&sheet, 0, 0, 16, 16, ship);
```

**Desde:** especificación 0.1 · función n.º 36 de la API.

**Ver también:** [`zxv_spr_draw`](#zxv_spr_draw),
[`zxv_gfxf_sprite`](#zxv_gfxf_sprite),
[Gráficos: sprites](graphics.md#sprites).

### `zxv_spr_draw`

```c
void zxv_spr_draw(const void *spr, int x, int y, int flags, const uint8_t *colours);
```

Dibuja un sprite preparado con la esquina de arriba a la izquierda en
(x, y). Los píxeles transparentes dejan ver lo de debajo. Con `colours`, se
**recolorea**: cada píxel de color *c* se dibuja con `colours[c]`, sin
cambiar los transparentes (un 8 de la tabla se dibuja como el color 8, no
como transparente). Sin espejo ni recoloreo y entero dentro en horizontal
va por un camino más rápido.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `spr` | un sprite de `zxv_spr_prepare` | El sprite. |
| `x`, `y` | cualquier entero | Dónde va su esquina. |
| `flags` | `ZXV_FLIP_H`, `ZXV_FLIP_V`, combinados, o 0 | Espejos. Los demás bits no cuentan. |
| `colours` | una tabla de 16 bytes (0–15), o `NULL` | El recoloreo. `NULL`: tal cual. De cada byte se usan sus 4 bits bajos. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `spr` es `NULL` o no es un sprite preparado (no lleva su marca) | Nada. |

**Ejemplo**

```c
/* ship, de zxv_spr_prepare */
uint8_t red[16];
for (int i = 0; i < 16; i++)
    red[i] = (uint8_t)i;
red[5] = 2;                                     /* el cian (5) se ve rojo (2) */
zxv_spr_draw(ship, 120, 90, 0, NULL);           /* tal cual */
zxv_spr_draw(ship, 150, 90, ZXV_FLIP_H, red);   /* al revés y del otro color */
```

**Desde:** especificación 0.1 · función n.º 37 de la API.

**Ver también:** [`zxv_spr_draw_scaled`](#zxv_spr_draw_scaled),
[`zxv_spr_collide`](#zxv_spr_collide).

### `zxv_spr_draw_scaled`

```c
void zxv_spr_draw_scaled(const void *spr, int x, int y, int w, int h, int flags, const uint8_t *colours);
```

Dibuja un sprite preparado estirado o encogido a `w` × `h` píxeles, con la
esquina en (x, y), para lo que se acerca o se aleja. Cada píxel del destino
toma el del sprite que cae en su centro (el vecino más cercano): el píxel
*i* de los `w` es el ⌊(2·i + 1)·ancho / (2·w)⌋ del sprite, y lo mismo en
vertical. Con espejo, el píxel *i* es el que sería el `w − 1 − i`. A su
tamaño hace lo mismo que `zxv_spr_draw` (y la llama). Usa unos 2,5 KB de la
pila del que llama y cuesta unas tres veces lo que un sprite de su tamaño
final.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `spr` | un sprite de `zxv_spr_prepare` | El sprite. |
| `x`, `y` | cualquier entero | Dónde va la esquina. |
| `w`, `h` | 1–4096 | El tamaño con el que se dibuja. |
| `flags` | `ZXV_FLIP_H`, `ZXV_FLIP_V`, o 0 | Espejos, como en `zxv_spr_draw`. |
| `colours` | una tabla de 16 bytes, o `NULL` | El recoloreo, como en `zxv_spr_draw`. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `w` o `h` fuera de 1–4096 | Nada. |
| `spr` es `NULL` o no es un sprite preparado | Nada. |

**Ejemplo**

```c
for (int s = 4; s <= 64; s += 12)               /* la nave, cada vez más cerca */
    zxv_spr_draw_scaled(ship, 128 - s / 2, 96 - s / 2, s, s, 0, NULL);
```

**Desde:** especificación 0.1 · función n.º 52 de la API.

**Ver también:** [`zxv_spr_draw`](#zxv_spr_draw),
[`zxv_m7_camera`](#zxv_m7_camera) (el ejemplo `graphics` pone sprites
escalados sobre un suelo de modo 7).

### `zxv_spr_collide`

```c
int zxv_spr_collide(const void *a, int ax, int ay, const void *b, int bx, int by);
```

Dice si dos sprites preparados se tocan: si algún píxel opaco de `a`
dibujado en (ax, ay) cae en el mismo sitio que un píxel opaco de `b`
dibujado en (bx, by). Compara a su tamaño y sin espejos, de 8 en 8
píxeles; no mira la pantalla.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `a`, `b` | sprites de `zxv_spr_prepare` | Los dos sprites (pueden ser el mismo). |
| `ax`, `ay` | cualquier entero | Dónde está la esquina de `a`. |
| `bx`, `by` | cualquier entero | Dónde está la esquina de `b`. |

**Devuelve:** 1 si se tocan; 0 si no.

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `a` o `b` es `NULL` o no es un sprite preparado | Devuelve 0. |

**Ejemplo**

```c
if (zxv_spr_collide(ship, sx, sy, bullet, bx, by))
    lives--;
```

**Desde:** especificación 0.1 · función n.º 38 de la API.

**Ver también:** [`zxv_map_cell`](#zxv_map_cell) (choques con el
escenario), [`HIT`](../basic/reference.md#hit) en el BASIC.

### Mapas de tiles

Un mapa es una matriz de celdas; cada celda dice qué tile va en ella y con
qué atributos (espejos, transparencia y prioridad). Ver
[Gráficos: mapas de tiles](graphics.md#mapas-de-tiles).

### `zxv_map_draw`

```c
void zxv_map_draw(const zxv_tilemap *m, int mx, int my, int x, int y, int w, int h, int filter);
```

Dibuja, en el rectángulo (x, y) de `w` × `h` píxeles del destino, la parte
del mapa que empieza en el píxel (mx, my) del mapa: el píxel (X, Y) del
destino enseña el (mx + X − x, my + Y − y) del mapa. Cada celda va con sus
espejos, y con `ZXV_CELL_TRANSP` su color 8 deja ver lo de debajo. Con el
filtro, se dibujan solo las celdas con prioridad o solo las que no la
tienen: un frame típico es el fondo (`ZXV_MAP_BACK`), los sprites y la
parte delantera (`ZXV_MAP_FRONT`). Las celdas que no se dibujan (por el
filtro o por caer fuera de un mapa que no se repite) dejan lo que había.
Con tiles opacos sin espejo y alineados a 8 píxeles va por un camino más
rápido.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `m` | un mapa | El mapa, con `size` 8 o 16 y `w` y `h` de 1 o más. |
| `mx`, `my` | cualquier entero | El píxel del mapa que va en la esquina del rectángulo. Si el mapa se repite (`ZXV_MAP_WRAP`), da la vuelta en las dos direcciones, también con valores negativos. |
| `x`, `y` | cualquier entero | La esquina del rectángulo en el destino. |
| `w`, `h` | 1 o más | El tamaño del rectángulo. |
| `filter` | `ZXV_MAP_ALL`, `ZXV_MAP_BACK` o `ZXV_MAP_FRONT` | Qué celdas se dibujan. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `m` es `NULL`, su `w` o su `h` valen 0, o `size` no es 8 ni 16 | Nada. |
| `w` o `h` valen 0 o menos | Nada. |
| La parte visible cruza más de 136 columnas de tiles (más de unos 1.080 píxeles de ancho con tiles de 8) | Nada. En las páginas de la máquina (hasta 1.024 de ancho) no pasa. |
| `filter` con otro valor | Como `ZXV_MAP_ALL`. |
| Una celda con un número de tile que no está en `tiles` | No se comprueba: lee lo que haya detrás. |

**Ejemplo**

```c
static uint32_t tiles[2 * 8];                   /* dos tiles de 8 x 8 (32 bytes cada uno) */
static uint16_t cells[32 * 24];
for (int i = 0; i < 8; i++) {
    tiles[i] = 0x11111111u;                     /* el 0: azul liso */
    tiles[8 + i] = i & 1 ? 0x66666666u : 0x44444444u;   /* el 1: rayas */
}
for (int i = 0; i < 32 * 24; i++)
    cells[i] = i / 32 == 23 ? 1 : 0;            /* la última fila, el suelo */
const zxv_tilemap map = { cells, 32, 24, (const uint8_t *)tiles, 8, ZXV_MAP_WRAP };
zxv_map_draw(&map, scroll_x, 0, 0, 0, 256, 192, ZXV_MAP_ALL);
```

**Desde:** especificación 0.1 · función n.º 39 de la API.

**Ver también:** [`zxv_map_cell`](#zxv_map_cell),
[`zxv_gfxf_map`](#zxv_gfxf_map), [`zxv_m7_draw_map`](#zxv_m7_draw_map),
la biblioteca [`zxv_mapmove.h`](libraries.md#mover-por-un-mapa-de-tiles-zxv_mapmoveh).

### `zxv_map_cell`

```c
uint16_t zxv_map_cell(const zxv_tilemap *m, int px, int py);
```

La celda del mapa en la que cae el píxel (px, py) del mapa: para los
choques con el escenario. Cuenta con la repetición del mapa
(`ZXV_MAP_WRAP`), también hacia los negativos.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `m` | un mapa | El mapa. |
| `px`, `py` | cualquier entero | El píxel, en coordenadas del mapa (las de `mx` y `my` en `zxv_map_draw`). |

**Devuelve:** la celda, con su número de tile (`& ZXV_CELL_TILE`) y sus
atributos; 0 fuera de un mapa que no se repite. Como 0 es también el tile 0
sin atributos, conviene que el tile 0 sea el vacío.

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `m` es `NULL`, su `w` o su `h` valen 0, o `size` no es 8 ni 16 | Devuelve 0. |

**Ejemplo**

```c
/* ¿Pisa el jugador (16 x 16, en px, py del mapa) un tile del suelo (el 1)? */
uint16_t c = zxv_map_cell(&map, px + 8, py + 16);
if ((c & ZXV_CELL_TILE) == 1)
    vy = 0;
```

**Desde:** especificación 0.1 · función n.º 40 de la API.

**Ver también:** [`zxv_map_draw`](#zxv_map_draw),
[`zxv_spr_collide`](#zxv_spr_collide).

### Modo 7

El **modo 7** dibuja un plano, una textura o un mapa de tiles, con una
transformación propia en cada línea del destino: girado, ampliado o en
perspectiva, como en la SNES. Cada línea (`zxv_m7line`) lleva la posición
en el plano (u, v) del píxel x = 0 del destino y el paso (du, dv) por
píxel, en coma fija 16.16 (65536 = un texel o un píxel del mapa): el píxel
x de la línea enseña el punto (u + x·du, v + x·dv), con la parte entera de
cada coordenada. `zxv_m7_camera` y `zxv_m7_affine` calculan esas líneas; se
pueden escribir también a mano. Ver [Gráficos: modo 7](graphics.md#modo-7).

Con una textura cuesta unos 15 ciclos por píxel (la pantalla entera del
modo 0, un tercio del frame); con un mapa, unos 36 según la especificación
(se han medido unos 33 en la placa). En el modo 1 a pantalla completa no
cabe en un frame: dibuja solo el suelo.

### `zxv_m7_draw`

```c
void zxv_m7_draw(const zxv_m7tex *t, const zxv_m7line *l, int y, int n);
```

Dibuja `n` líneas de una textura desde la línea `y` del destino: la línea
`y + i` usa `l[i]`. Cada línea cubre todo el ancho del recorte. Las
coordenadas se toman **sin signo**: lo negativo cae fuera de la textura.
Si la textura se repite, todo cae dentro (también lo negativo, que da la
vuelta); si no, lo de fuera es del color `out`. Con `ZXV_M7_TRANSPARENT` en
`flags`, los texels del color 8 no se dibujan (y lo de fuera tampoco si
`out` es el 8).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `t` | una textura | Sus `texels` (un byte por texel, 0–15, fila a fila), su `log2size` (1–10: de 2 × 2 a 1024 × 1024), `wrap`, `out` (0–15) y `flags`. En la RAM de usuario caben hasta 256 × 256 (64 KB). |
| `l` | `n` líneas | La transformación de cada línea. |
| `y` | cualquier entero | La primera línea del destino. Las que caen fuera del recorte no se dibujan. |
| `n` | 1 o más | Cuántas líneas. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `t` o `l` son `NULL`, o `n` vale 0 o menos | Nada. |
| `log2size` fuera de 1–10 | Nada. |
| Un texel o `out` de más de 15 | Sus 4 bits bajos. |
| `flags` con `ZXV_M7_TRANSPARENT` en una ROM de la especificación 0.1 | El campo era reservado y había que dejarlo a 0: no lo uses sin comprobar antes [`zxv_api_version`](#zxv_api_version) ≥ 2. |

**Ejemplo**

```c
static uint8_t texels[64 * 64];                 /* un tablero de 64 x 64 */
static zxv_m7line rows[192];
for (int v = 0; v < 64; v++)
    for (int u = 0; u < 64; u++)
        texels[v * 64 + u] = ((u / 8) ^ (v / 8)) & 1 ? 12 : 4;
const zxv_m7tex tex = { texels, 6, 1, 0, 0 };   /* 2^6 = 64, se repite */
for (int i = 0; i < 192; i++)                   /* cada línea, algo más lejos */
    rows[i] = (zxv_m7line){ 0, i << 16, 0x4000 + i * 256, 0 };
zxv_m7_draw(&tex, rows, 0, 192);
```

**Desde:** especificación 0.1 · función n.º 41 de la API
(`ZXV_M7_TRANSPARENT`, desde la especificación 0.2).

**Ver también:** [`zxv_m7_camera`](#zxv_m7_camera),
[`zxv_m7_affine`](#zxv_m7_affine), [`zxv_m7_draw_map`](#zxv_m7_draw_map),
[`zxv_gfxf_texture`](#zxv_gfxf_texture), [`FLOOR`](../basic/reference.md#floor)
en el BASIC.

### `zxv_m7_draw_map`

```c
void zxv_m7_draw_map(const zxv_tilemap *m, const zxv_m7line *l, int y, int n, int out);
```

Lo mismo que `zxv_m7_draw`, con un **mapa de tiles** como plano, como en
la SNES: el punto (u, v) del plano es el píxel del mapa, con los espejos de
cada celda y su 8 transparente (`ZXV_CELL_TRANSP`), tal como lo dibujaría
`zxv_map_draw`. Aquí u y v llevan **signo**: por debajo de 0 es fuera del
mapa, o el otro lado si el mapa se repite (`ZXV_MAP_WRAP`, en las dos
direcciones, con cualquier tamaño). Lo de fuera de un mapa que no se
repite es del color `out`, o no se dibuja. La prioridad de las celdas no
cuenta. Un circuito de 1024 × 1024 píxeles es un mapa de 128 × 128 celdas
(32 KB) más sus tiles, en vez del megabyte que sería como textura.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `m` | un mapa | El plano, con `cells` y `tiles`, `size` 8 o 16 y `w` y `h` de 1 o más. |
| `l` | `n` líneas | La transformación de cada línea. |
| `y` | cualquier entero | La primera línea del destino. |
| `n` | 1 o más | Cuántas líneas. |
| `out` | 0–15, o `ZXV_CLEAR` | El color de lo de fuera de un mapa que no se repite; `ZXV_CLEAR`, no se dibuja. |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `m` o `l` son `NULL`, `cells` o `tiles` son `NULL`, `w` o `h` valen 0, `size` no es 8 ni 16, o `n` vale 0 o menos | Nada. |
| `out` negativo o mayor que 15 | Como `ZXV_CLEAR`: lo de fuera no se dibuja. |
| Con una ROM de la especificación 0.1 | La función no existe: no hace nada. |

**Ejemplo**

```c
/* map, de zxv_map_draw; rows, de zxv_m7_camera */
zxv_m7_draw_map(&map, rows + 72, 72, 120, ZXV_CLEAR);
```

**Desde:** especificación 0.2 · función n.º 57 de la API.

**Ver también:** [`zxv_m7_draw`](#zxv_m7_draw),
[`zxv_map_draw`](#zxv_map_draw).

### `zxv_m7_camera`

```c
void zxv_m7_camera(zxv_m7line *l, int n, const zxv_m7cam *c);
```

Rellena `l[0]` a `l[n − 1]` con las líneas de un **suelo en perspectiva**
visto desde una cámara, para las líneas 0 a n − 1 del destino. La línea
*i* por debajo del horizonte está a la distancia
altura × focal / (i − horizonte), mirando en la dirección `angle` (0 hacia
u creciente, 0x4000 hacia v creciente), con el centro del destino en la
posición de la cámara. Las líneas del horizonte para arriba quedan a cero.
Se dibuja con `zxv_m7_draw(t, l + y0, y0, n)` (o `zxv_m7_draw_map`) desde
la primera línea del suelo, `y0`. El centro es la mitad del ancho del
destino actual (el destino entero, no el recorte).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `l` | `n` líneas | Donde se escriben. |
| `n` | 1 o más | Cuántas líneas, desde la 0 del destino. |
| `c` | una cámara | `x`, `y` (posición sobre el plano), `height` y `focal` (mayor que 0), en coma fija 16.16; `angle` en vueltas de 0x10000; `horizon`, la línea del destino del horizonte (entera, puede ser negativa). |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `l` o `c` son `NULL`, `n` vale 0 o menos, o `focal` vale 0 o menos | No escribe nada. |
| El destino no tiene nada visible (un recorte vacío) | Toma 256 como ancho. |

**Ejemplo**

```c
/* tex, la textura de zxv_m7_draw */
static zxv_m7line rows[192];
zxv_m7cam cam = {
    .x = 32 << 16, .y = 32 << 16,               /* coma fija 16.16 */
    .height = 16 << 16,
    .angle = 0x2000,                            /* un octavo de vuelta */
    .focal = 128 << 16,
    .horizon = 64,                              /* línea del horizonte */
};
zxv_gfx_rect(0, 0, 256, 72, 9);                 /* el cielo */
zxv_m7_camera(rows, 192, &cam);                 /* las líneas 0..64 quedan a cero */
zxv_m7_draw(&tex, rows + 72, 72, 120);          /* el suelo: líneas 72..191 */
```

**Desde:** especificación 0.1 · función n.º 42 de la API.

**Ver también:** [`zxv_m7_draw`](#zxv_m7_draw),
[`zxv_m7_affine`](#zxv_m7_affine), la biblioteca
[`zxv_m7.h`](libraries.md#objetos-sobre-el-suelo-del-modo-7-zxv_m7h)
(objetos sobre el suelo).

### `zxv_m7_affine`

```c
void zxv_m7_affine(zxv_m7line *l, int n, const zxv_m7aff *a);
```

Rellena `l[0]` a `l[n − 1]` para un plano **girado y ampliado**, como la
matriz de la SNES, para las líneas 0 a n − 1 del destino: el punto (x, y)
del plano se ve en (cx, cy) del destino, girado `angle` en el sentido del
reloj y ampliado `zoom_x` y `zoom_y` en los ejes del plano. Es decir, el
píxel (X, Y) enseña (x, y) + Z⁻¹·R(−angle)·(X − cx, Y − cy). Usa el mismo
seno que `zxv_m7_camera`. Sirve para fondos que giran, jefes enormes o un
mapa que se acerca.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `l` | `n` líneas | Donde se escriben. |
| `n` | 1 o más | Cuántas líneas, desde la 0 del destino. |
| `a` | un plano | `x`, `y` (el punto del centro, 16.16); `angle` (vueltas de 0x10000, en el sentido del reloj); `zoom_x`, `zoom_y` (mayores que 0; 0x10000 es su tamaño y 0x20000, el doble); `cx`, `cy` (el centro en el destino, enteros). |

**Fuera de rango**

| Caso | Qué hace la ROM |
|---|---|
| `l` o `a` son `NULL`, `n` vale 0 o menos, o `zoom_x` o `zoom_y` valen 0 o menos | No escribe nada. |
| Con una ROM de la especificación 0.1 | La función no existe: no escribe nada. |

**Ejemplo**

```c
/* tex, la textura de 64 x 64 de zxv_m7_draw; rows, 192 líneas */
zxv_m7aff a = { .x = 32 << 16, .y = 32 << 16,  /* el centro de la textura */
                .angle = frame * 256, .zoom_x = 0x20000, .zoom_y = 0x20000,
                .cx = 128, .cy = 96 };
zxv_m7_affine(rows, 192, &a);
zxv_m7_draw(&tex, rows, 0, 192);
```

**Desde:** especificación 0.2 · función n.º 58 de la API.

**Ver también:** [`zxv_m7_camera`](#zxv_m7_camera),
[`zxv_m7_draw`](#zxv_m7_draw).

## Ficheros de gráficos `.gfx`

Declarado en `zxv_gfxfile.h`. Leen los ficheros `.gfx` (los de `png2gfx.py`,
`zxv-gfx`, el editor de la máquina y `LOAD "x" GRAPHICS` del BASIC) ya en
memoria. No están en la ROM: se compilan con `gfxfile.c`, que va dentro del
programa (añádelo a sus fuentes), así que funcionan con cualquier ROM. Las
explicaciones y un ejemplo completo están en
[Gráficos](graphics.md#leer-un-fichero-gfx-en-c).

```c
typedef struct {
    const uint8_t *data;
    uint32_t size;
} zxv_gfxfile;
```

El fichero puede estar donde sea: leído con `zxv_file_load` o dentro del
programa. Los objetos del fichero no van alineados, así que cada función
copia el suyo en `buf`, que tiene que estar alineado a 4, y **devuelve los
bytes que necesita `buf`**, lo haya copiado o no. Si `buf` es `NULL` o
`room` es menor, no copia nada: así se pregunta el tamaño. Si el objeto no
está, o `zxv_gfxf_open` no aceptó el fichero, devuelve 0 (o `NULL`).
Ninguna llama a la ROM.

### `zxv_gfxf_open`

```c
int zxv_gfxf_open(zxv_gfxfile *f, const void *data, uint32_t size);
```

Comprueba que `data` es un fichero `.gfx` entero y bien formado y deja `f`
listo para las demás funciones. Mira la firma, la versión y todos los
objetos: que quepan en `size` y que cada uno tenga la longitud que dicen
sus medidas. Los objetos de clases que no conoce se saltan. El fichero no
se copia: tiene que seguir en `data` mientras se use `f`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un `zxv_gfxfile` | Lo que se rellena. |
| `data` | el fichero en memoria | Desde su primer byte (`ZXVG`). |
| `size` | bytes | Lo que ocupa. |

**Devuelve:** 1 si lo acepta; 0 si no, y entonces `f` queda vacío (todas
las demás funciones dan 0 o `NULL` con él).

**Cuándo da 0**

| Caso | Por qué |
|---|---|
| `size` menor que 8, sin `ZXVG` al principio o con una versión que no es la 1 | No es un `.gfx` que entienda. |
| Un objeto se sale de `size` | El fichero está cortado (no se leyó entero: mira el `max` de `zxv_file_load`). |
| Un sprite de 0 o más de 1024 de lado, unos tiles que no son de 8 ni de 16 o sin ninguno, un mapa vacío o con tiles que no son de 8 ni de 16, una textura con `log2size` fuera de 1–10, una pantalla de modo, borde o marcas que no valen, o cualquiera con una longitud que no casa con sus medidas | El objeto no vale. |

**Ejemplo**

```c
static uint8_t data[16 * 1024];                 /* el fichero .gfx entero */
zxv_gfxfile f;
int n = zxv_file_load("art.gfx", data, sizeof data);
if (n < 0 || !zxv_gfxf_open(&f, data, (uint32_t)n))
    zxv_report_error(n < 0 ? n : 0, "No puedo leer art.gfx");
```

**Ver también:** [`zxv_file_load`](#zxv_file_load),
[El formato `.gfx`](#el-formato-gfx).

### `zxv_gfxf_find`

```c
const uint8_t *zxv_gfxf_find(const zxv_gfxfile *f, int kind, int num, uint32_t *len);
```

Los datos de un objeto tal como están en el fichero, sin alinear ni copiar,
para leer sus medidas o un objeto de una clase que no tiene función.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un fichero abierto | El fichero. |
| `kind` | `ZXV_GFXF_SPRITE` ... `ZXV_GFXF_SCREEN` (tabla de abajo) | La clase de objeto. |
| `num` | 0–65535 | Su número (en los tiles, el lado: 8 o 16). |
| `len` | un puntero, o `NULL` | Si no es `NULL`, recibe la longitud de los datos. |

**Devuelve:** un puntero a los datos del objeto (sin la cabecera de 8
bytes), o `NULL` si no está.

**Ejemplo**

```c
uint32_t len;
const uint8_t *d = zxv_gfxf_find(&f, ZXV_GFXF_SPRITE, 3, &len);
int w = d ? d[0] | d[1] << 8 : 0;               /* el ancho del sprite 3 */
```

**Ver también:** [El formato `.gfx`](#el-formato-gfx).

### `zxv_gfxf_sprite`

```c
uint32_t zxv_gfxf_sprite(const zxv_gfxfile *f, int num, zxv_bitmap *b, void *buf, uint32_t room);
```

El sprite `num` como bitmap: `b` recibe sus medidas y apunta a `buf`, donde
se copian sus píxeles con líneas de un múltiplo de 4 bytes (el relleno de
la derecha, transparente: el color 8). Listo para `zxv_spr_prepare`,
`zxv_gfx_blit` o `zxv_gfx_target`. `b` se rellena aunque no se copie nada
(con `pix` = `buf`, que puede ser `NULL`).

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un fichero abierto | El fichero. |
| `num` | 0–255 (los de `png2gfx.py` y el BASIC) | El número del sprite. |
| `b` | un bitmap | Lo que se rellena. |
| `buf` | memoria alineada a 4, o `NULL` | Dónde se copian los píxeles. |
| `room` | bytes | El sitio que hay en `buf`. |

**Devuelve:** los bytes que necesita `buf`: ⌈⌈ancho / 2⌉ / 4⌉ × 4 × alto;
0 si el sprite no está (y entonces `b` no se toca).

**Ejemplo**

```c
static uint32_t pix[4096];                      /* hasta 16 KB de píxeles */
static uint32_t face[1024];                     /* el sprite preparado */
zxv_bitmap b;
uint32_t need = zxv_gfxf_sprite(&f, 0, &b, pix, sizeof pix);
if (!need || need > sizeof pix || zxv_spr_prepare(&b, 0, 0, b.w, b.h, NULL) > sizeof face)
    zxv_report_error(0, "El sprite 0 no cabe");
zxv_spr_prepare(&b, 0, 0, b.w, b.h, face);
```

**Ver también:** [`zxv_spr_prepare`](#zxv_spr_prepare),
[`zxv_gfx_blit`](#zxv_gfx_blit).

### `zxv_gfxf_tiles`

```c
uint32_t zxv_gfxf_tiles(const zxv_gfxfile *f, int size, void *buf, uint32_t room, int *count);
```

Los tiles de lado `size`, seguidos, como los quiere `zxv_tilemap.tiles`.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un fichero abierto | El fichero. |
| `size` | 8 o 16 | El lado de los tiles. |
| `buf` | memoria alineada a 4, o `NULL` | Dónde se copian. |
| `room` | bytes | El sitio que hay en `buf`. |
| `count` | un puntero, o `NULL` | Si no es `NULL`, recibe cuántos tiles hay (aunque no se copien). |

**Devuelve:** los bytes que necesita `buf`: 32 por tile de 8 × 8 y 128 por
tile de 16 × 16; 0 si el fichero no tiene tiles de ese lado (y entonces
`count` no se toca).

**Ejemplo**

```c
static uint32_t tiles[64 * 32 / 4];             /* hasta 64 tiles de 8 x 8 */
int count;
uint32_t t = zxv_gfxf_tiles(&f, 8, tiles, sizeof tiles, &count);
if (!t || t > sizeof tiles)
    zxv_report_error(0, "Los tiles no caben");
```

**Ver también:** [`zxv_gfxf_map`](#zxv_gfxf_map).

### `zxv_gfxf_map`

```c
uint32_t zxv_gfxf_map(const zxv_gfxfile *f, int num, zxv_tilemap *m, const void *tiles,
                      void *buf, uint32_t room);
```

El mapa `num`: sus celdas se copian en `buf`, y `m` queda con ellas, con
su tamaño, el lado de sus tiles, su repetición (`ZXV_MAP_WRAP`) y `tiles`,
los gráficos de sus tiles (los de `zxv_gfxf_tiles` de su lado). `m` se
rellena aunque no se copie nada.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un fichero abierto | El fichero. |
| `num` | 0–15 (los de `png2gfx.py` y el BASIC) | El número del mapa. |
| `m` | un mapa | Lo que se rellena. |
| `tiles` | los tiles, alineados a 4 | Los de su lado (el `size` de `m`); no se comprueba que sean de ese lado ni que estén todos. |
| `buf` | memoria alineada a 4, o `NULL` | Dónde se copian las celdas. |
| `room` | bytes | El sitio que hay en `buf`. |

**Devuelve:** los bytes que necesita `buf`: 2 por celda; 0 si el mapa no
está (y entonces `m` no se toca).

**Ejemplo**

```c
static uint16_t cells[1024];
zxv_tilemap level;
uint32_t c = zxv_gfxf_map(&f, 0, &level, tiles, cells, sizeof cells);
if (!c || c > sizeof cells)
    zxv_report_error(0, "El mapa no cabe");
zxv_map_draw(&level, 0, 0, 0, 0, 256, 192, ZXV_MAP_ALL);
```

**Ver también:** [`zxv_gfxf_tiles`](#zxv_gfxf_tiles),
[`zxv_map_draw`](#zxv_map_draw).

### `zxv_gfxf_texture`

```c
uint32_t zxv_gfxf_texture(const zxv_gfxfile *f, int num, zxv_m7tex *t, void *buf, uint32_t room);
```

La textura `num` del modo 7: sus texels se copian en `buf`, y `t` queda
con ellos, su `log2size`, `wrap`, `out` y `flags` (`ZXV_M7_TRANSPARENT`).
`t` se rellena aunque no se copie nada.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un fichero abierto | El fichero. |
| `num` | 0–3 (los de `png2gfx.py` y el BASIC) | El número de la textura. |
| `t` | una textura | Lo que se rellena. |
| `buf` | memoria, o `NULL` | Dónde se copian los texels. |
| `room` | bytes | El sitio que hay en `buf`. |

**Devuelve:** los bytes que necesita `buf`: lado × lado (un byte por
texel); 0 si la textura no está (y entonces `t` no se toca).

**Ejemplo**

```c
static uint8_t texels[128 * 128];
zxv_m7tex floor_tex;
uint32_t n = zxv_gfxf_texture(&f, 0, &floor_tex, texels, sizeof texels);
if (!n || n > sizeof texels)
    zxv_report_error(0, "La textura no cabe");
```

**Ver también:** [`zxv_m7_draw`](#zxv_m7_draw).

### `zxv_gfxf_screen`

```c
uint32_t zxv_gfxf_screen(const zxv_gfxfile *f, int num, int *mode, int *border, void *buf, uint32_t room);
```

La pantalla `num` (las de `SAVE "x" SCREEN$`, `png2gfx.py --screen` y las
pantallas de carga): sus píxeles, como los de una página de la VRAM, se
copian en `buf`, y su modo y el color de su borde van a `*mode` y
`*border`. `buf` puede ser la página que no se ve: luego se cambia de
página (y se ponen el modo y el borde) en el VBLANK.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un fichero abierto | El fichero. |
| `num` | 0–255 | El número de la pantalla. |
| `mode` | un puntero, o `NULL` | Recibe su modo, 0 o 1. |
| `border` | un puntero, o `NULL` | Recibe el color de su borde, 0–15. |
| `buf` | memoria alineada a 4, o `NULL` | Dónde se copian los píxeles. |
| `room` | bytes | El sitio que hay en `buf`. |

**Devuelve:** los bytes que necesita `buf`: 24.576 en el modo 0 y 98.304
en el modo 1; 0 si la pantalla no está (y entonces `*mode` y `*border` no
se tocan).

**Ejemplo**

```c
int mode, border;
uint16_t pal[16];
if (!zxv_gfxf_screen(&f, 0, &mode, &border, NULL, 0))   /* primero, su modo */
    zxv_report_error(0, "No hay pantalla 0");
zxv_set_mode(mode);
zxv_gfxf_screen(&f, 0, NULL, NULL, (void *)(uintptr_t)ZXV_VRAM1_BASE, ZXV_VRAM_PAGE_SIZE);
zxv_gfxf_screen_palette(&f, 0, pal);
zxv_wait_vblank();
for (int i = 0; i < 16; i++)
    ((volatile uint16_t *)(uintptr_t)ZXV_PALETTE(0))[i] = pal[i];
*(volatile uint8_t *)(uintptr_t)ZXV_V_BORDER = (uint8_t)border;
zxv_set_page(1);
```

**Ver también:** [`zxv_gfxf_screen_palette`](#zxv_gfxf_screen_palette),
[la pantalla de carga](#la-pantalla-de-carga).

### `zxv_gfxf_screen_palette`

```c
int zxv_gfxf_screen_palette(const zxv_gfxfile *f, int num, uint16_t pal[16]);
```

Los 16 colores RGB565 con los que se veía la pantalla `num`, para ponerlos
en una paleta (`ZXV_PALETTE(t)`). Una pantalla lleva paleta si se hizo con
colores que no son los de siempre.

**Parámetros**

| Parámetro | Valores | Qué es |
|---|---|---|
| `f` | un fichero abierto | El fichero. |
| `num` | 0–255 | El número de la pantalla. |
| `pal` | 16 `uint16_t` | Lo que se rellena, siempre. |

**Devuelve:** 1 si la pantalla lleva paleta (y `pal` queda con ella); 0
si no la lleva o no está, y entonces `pal` queda con los colores de
siempre (`ZXV_STD_RGB565`).

**Ejemplo:** el de [`zxv_gfxf_screen`](#zxv_gfxf_screen).

**Ver también:** [La paleta](#la-paleta), [Registros de
vídeo](#registros-de-vídeo).

### Constantes de los ficheros `.gfx`

| Constante | Valor | Clase de objeto (`kind`) |
|---|---|---|
| `ZXV_GFXF_SPRITE` | 1 | Sprite |
| `ZXV_GFXF_TILES` | 2 | Tiles (su número es el lado: 8 o 16) |
| `ZXV_GFXF_MAP` | 3 | Mapa |
| `ZXV_GFXF_TEXTURE` | 4 | Textura |
| `ZXV_GFXF_SCREEN` | 5 | Pantalla |

### El formato `.gfx`

Las 4 letras `ZXVG`, un byte de versión (1), un byte a 0 y el número de
objetos (16 bits). Cada objeto tiene una cabecera de 8 bytes (clase, 0,
número en 16 bits y longitud de sus datos en 32 bits) seguida de sus datos.
Todos los números van con el byte bajo primero.

| Clase | Objeto | Datos |
|---|---|---|
| 1 | Sprite | Ancho y alto (16 bits cada uno, 1–1024) y sus píxeles: ⌈ancho / 2⌉ bytes por línea |
| 2 | Tiles de S × S (S es el número del objeto: 8 o 16) | Cuántos (16 bits, 1 o más), 0 (16 bits) y los tiles (32 o 128 bytes cada uno) |
| 3 | Mapa | Ancho y alto (16 bits cada uno), lado del tile (8 o 16) y repetición (bytes), 0 (16 bits) y las celdas (16 bits cada una) |
| 4 | Textura | `log2size` (1–10), repetición, color de fuera y marcas (bit 0: el 8 es transparente), un byte cada uno, y los texels (un byte cada uno) |
| 5 | Pantalla | Modo (0 o 1), color del borde, marcas (bit 0: lleva paleta) y 0, un byte cada uno; si lleva paleta, sus 16 colores RGB565 (32 bytes); y los píxeles de la página, como en la VRAM (24.576 bytes en el modo 0, 98.304 en el modo 1) |

## Registros de vídeo

En `ZXV_VIDEO_REGS` (`0x2000_0400`). Escríbelos cuando quieras: el sistema
los toma justo después de cada VBLANK y se ven en el frame siguiente.

| Registro | Desplazamiento | Tamaño | Contenido |
|---|---|---|---|
| `ZXV_V_MODE` | 0x00 | 8 bits | Modo de vídeo, 0 o 1 (usa `zxv_set_mode`, que además borra la página de dibujo) |
| `ZXV_V_PAGE` | 0x01 | 8 bits | Página visible, 0 o 1 (usa `zxv_set_page`) |
| `ZXV_V_BORDER` | 0x02 | 8 bits | Color del borde, 0–15 (sin remapeo; con la paleta de la tabla de `ZXV_V_REMAP`) |
| `ZXV_V_FLAGS` | 0x03 | 8 bits | `ZXV_VFLAG_LINETAB` (0x01): usar la tabla por línea; `ZXV_VFLAG_BLANK` (0x02): enseñar solo el borde |
| `ZXV_V_SCROLL_X` | 0x04 | 16 bits | Scroll horizontal de la imagen, circular |
| `ZXV_V_SCROLL_Y` | 0x06 | 16 bits | Scroll vertical, circular |
| `ZXV_V_REMAP` | 0x08 | 8 bits | Tabla de remapeo de colores en uso, 0–3 |
| `ZXV_V_VSIZE` | 0x09 | 8 bits | Tamaño de la página en el modo 0 (ver abajo). Cada programa empieza con 0 |
| `ZXV_V_OUT_INFO` | 0x0C | 32 bits | Solo lectura: la salida de vídeo (ver abajo) |

Con scroll, el píxel (x, y) de la pantalla enseña el píxel
((x + SCROLL_X) mod ancho, (y + SCROLL_Y) mod alto) de la página.

**La salida** (`ZXV_V_OUT_INFO`). La elige el usuario en la máquina; los
programas solo la leen:

| Bits | Constantes | Contenido |
|---|---|---|
| 0–3 | `ZXV_OUT_DVI` (1), `ZXV_OUT_HDMI` (2) | El tipo de salida: DVI, sin sonido por el cable, o HDMI, con sonido. Los demás valores, reservados |
| 8–15 | | La frecuencia de refresco en Hz: siempre 60 |
| 16–17 | `ZXV_OUT_SCREEN_MASK` (`3 << 16`), `ZXV_OUT_SCREEN_SHIFT` (16) | El monitor, por la línea de detección del HDMI: `ZXV_OUT_SCREEN_UNKNOWN` (`0 << 16`), no se sabe (en una protoboard, o hasta que se conecta uno); `ZXV_OUT_SCREEN_NONE` (`1 << 16`), estaba conectado y se ha quitado; `ZXV_OUT_SCREEN_ON` (`2 << 16`), conectado. El emulador dice siempre `ZXV_OUT_SCREEN_ON` |

```c
uint32_t o = *(volatile uint32_t *)(uintptr_t)ZXV_V_OUT_INFO;
int hdmi = (o & 15) == ZXV_OUT_HDMI;
int screen_on = (o & ZXV_OUT_SCREEN_MASK) == ZXV_OUT_SCREEN_ON;
```

**Tamaños de página** (`ZXV_V_VSIZE`, solo en el modo 0; en el modo 1 se
ignora). Al cargar un `.zxv`, la ROM lo pone a 0:

| Constante | Valor | Página | Bytes por línea |
|---|---|---|---|
| `ZXV_VSIZE_SCREEN` | 0 | 256 × 192 | 128 |
| `ZXV_VSIZE_2X2` | 1 | 512 × 384 | 256 |
| `ZXV_VSIZE_WIDE` | 2 | 1024 × 192 | 512 |
| `ZXV_VSIZE_TALL` | 3 | 256 × 768 | 128 |

Con `ZXV_VSIZE_SCREEN` en el modo 0, cada página enseña solo sus primeros
24 KB; los 72 KB de detrás (`ZXV_VRAM_HIDDEN`, `ZXV_VRAM_HIDDEN_SIZE`) son
memoria para el programa (`ZXV_VRAM0_DATA`, `ZXV_VRAM1_DATA`; ver
[Cómo coloca el SDK un programa](#cómo-coloca-el-sdk-un-programa)), que se
pierde si se pasa al modo 1 o a otro tamaño de página.

**Tablas de remapeo:** cuatro tablas de 16 bytes desde `ZXV_REMAP_TABLES`
(la tabla *t* en `ZXV_REMAP_TABLES + 16·t`). La entrada *v* es el color que
se ve para el valor de píxel *v*. La ROM las pone a la identidad al
empezar cada programa.

**Paletas:** cada tabla tiene la suya, 16 colores RGB565 (u16) desde
`ZXV_PALETTE(t)` (`ZXV_PALETTES + 32·t`, en `0x2000_0480`). Un píxel de
valor *v* en una línea que usa la tabla *t* se ve con el color
`paleta[t][tabla[t][v]]`. Empiezan con los colores de siempre
(`ZXV_STD_RGB565(i)`), y la ROM los vuelve a poner al empezar cada
programa. Como las tablas, se leen justo después de cada VBLANK. Para hacer
colores, `ZXV_RGB(r, g, b)` (ver [La paleta](#la-paleta)).

**Tabla por línea:** en `ZXV_LINE_TABLE`, una entrada de
`ZXV_LINE_ENTRY_SIZE` (8) bytes por línea del cuadro entero, borde
incluido, desde la primera línea del borde de arriba: 240 líneas en el
modo 0 (la imagen empieza en la línea 24) y 480 en el modo 1 (la imagen,
desde la línea 48): `ZXV_FRAME_LINES(modo)` y `ZXV_BORDER_LINES(modo)`. El
cuadro es de `ZXV_FRAME_W` × `ZXV_FRAME_H` (640 × 480), con un borde de
`ZXV_BORDER_X` (64) píxeles a cada lado y `ZXV_BORDER_Y` (48) líneas arriba
y abajo (la mitad, en píxeles del modo 0). Solo se usa con
`ZXV_VFLAG_LINETAB`.

| Desplazamiento | Tamaño | Campo | Valor «como siempre» |
|---|---|---|---|
| `ZXV_LT_BORDER` (0) | 8 bits | Color del borde | `0xFF` (el de `ZXV_V_BORDER`) |
| `ZXV_LT_REMAP` (1) | 8 bits | Tabla de remapeo | `0xFF` (la de `ZXV_V_REMAP`) |
| `ZXV_LT_SCROLL_X` (2) | 16 bits | Scroll horizontal (de 0 al ancho de la página − 1) | `0xFFFF` (el de `ZXV_V_SCROLL_X`) |
| `ZXV_LT_SRC_LINE` (4) | 16 bits | Línea de la página que se enseña (de 0 al alto de la página − 1) | `0xFFFF` (= (y + SCROLL_Y) mod alto) |
| 6 | 16 bits | Reservado, escribe 0 | |

En las líneas del borde solo cuenta el campo del borde.

**Cuándo es seguro cambiar las cosas:** justo después de cada VBLANK el
sistema lee la página visible, los registros y las tablas. Mientras el bit
`ZXV_SYSF_LINK_BUSY` de `ZXV_SV_SYSFLAGS` valga 1 (hasta 1,5 ms en el modo
0 y 6 ms en el modo 1 o con una página grande), no escribas en la página
visible ni en la tabla por línea. En la página que no se ve se puede
escribir siempre.

**Memoria de vídeo:** `ZXV_VRAM0_BASE` (`0x2000_4000`) y `ZXV_VRAM1_BASE`
(`0x2001_C000`), de `ZXV_VRAM_PAGE_SIZE` (96 KB) cada una. El modo 0 usa
los primeros 24.576 bytes de cada página con `ZXV_VSIZE_SCREEN`, y el modo
1, los 98.304. Los tamaños de la imagen: `ZXV_MODE0_W` × `ZXV_MODE0_H`
(256 × 192) y `ZXV_MODE1_W` × `ZXV_MODE1_H` (512 × 384).

## Registros de entrada

En `ZXV_INPUT_REGS` (`0x2000_0500`). Los escribe el sistema, salvo
`ZXV_I_KEY_TAIL`.

| Registro | Desplazamiento | Tamaño | Contenido |
|---|---|---|---|
| `ZXV_I_KEYROWS` | 0x00 | 8 × 8 bits | La matriz del teclado del Spectrum por semifilas: bits 0–4, 0 = pulsada; bits 5–7 a 1 |
| `ZXV_I_KEY_HEAD` | 0x08 | 8 bits | Cuántas teclas ha escrito el sistema en la cola (un contador que da la vuelta) |
| `ZXV_I_KEY_TAIL` | 0x09 | 8 bits | Cuántas ha leído el programa (lo avanza `zxv_key_get`). Si es igual que `ZXV_I_KEY_HEAD`, la cola está vacía; si no, la siguiente tecla está en `ZXV_I_KEY_FIFO[TAIL mod 64]` |
| `ZXV_I_KEYMAP` | 0x10 | 32 bytes | 256 bits, uno por código HID de tecla; 1 = pulsada |
| `ZXV_I_KEY_MODS` | 0x30 | 8 bits | Modificadores: bits 0–3 Ctrl, Mayús, Alt y GUI izquierdas; bits 4–7, los mismos a la derecha |
| `ZXV_I_JOY0` a `ZXV_I_JOY3` | 0x40, 0x42, 0x44, 0x46 | 16 bits cada uno | Los mandos de los jugadores 1 a 4 (los bits `ZXV_JOY_*`, como `zxv_joy_read`; `JOY2` y `JOY3` desde la especificación 0.3, antes a 0) |
| `ZXV_I_MOUSE_X`, `ZXV_I_MOUSE_Y` | 0x48, 0x4A | 16 bits con signo cada uno | Contadores del movimiento del ratón, circulares (x a la derecha, y hacia abajo) |
| `ZXV_I_MOUSE_BTN` | 0x4C | 8 bits | Bits 0–2: botón izquierdo, derecho y central |
| `ZXV_I_MOUSE_WHEEL` | 0x4D | 8 bits con signo | Contador de la rueda (positivo hacia arriba) |
| `ZXV_I_KEY_FIFO` | 0x80 | 64 × 16 bits | La cola de teclas (`ZXV_KEY_FIFO_LEN` = 64), lo que lee `zxv_key_get` |

**El ratón.** El sistema suma a los contadores lo que mueve el ratón, sin
aceleración ni límites: no son una posición. Guarda la última lectura y
mira cuánto han avanzado, con cuentas de 16 u 8 bits para que dé igual que
den la vuelta. `zxv_mouse_update` lo hace por ti.

**Códigos HID** de las teclas más usadas: A–Z 0x04–0x1D; 1–9 0x1E–0x26, 0
0x27; Intro 0x28, Esc 0x29, Retroceso 0x2A, Tab 0x2B, Espacio 0x2C; F1–F12
0x3A–0x45; Insert 0x49, Inicio 0x4A, RePág 0x4B, Supr 0x4C, Fin 0x4D, AvPág
0x4E; derecha 0x4F, izquierda 0x50, abajo 0x51, arriba 0x52; Ctrl, Mayús y
Alt izquierdas 0xE0–0xE2, las derechas 0xE4–0xE6. Identifican la posición
de la tecla en un teclado de EE. UU.

**La matriz del Spectrum** (`ZXV_I_KEYROWS[n]`, bits 0 a 4):

| Semifila | Bit 0 | Bit 1 | Bit 2 | Bit 3 | Bit 4 |
|---|---|---|---|---|---|
| 0 | CAPS SHIFT | Z | X | C | V |
| 1 | A | S | D | F | G |
| 2 | Q | W | E | R | T |
| 3 | 1 | 2 | 3 | 4 | 5 |
| 4 | 0 | 9 | 8 | 7 | 6 |
| 5 | P | O | I | U | Y |
| 6 | ENTER | L | K | J | H |
| 7 | SPACE | SYMBOL SHIFT | M | N | B |

Mayús es CAPS SHIFT y Ctrl, SYMBOL SHIFT; Retroceso, Esc y los cursores
pulsan sus combinaciones del Spectrum (CAPS SHIFT + 0, SPACE, 5–8), y los
signos de puntuación pulsan SYMBOL SHIFT con la tecla que toca.

## Registros de sonido

En `ZXV_SOUND_REGS` (`0x2000_0600`).

| Registro | Desplazamiento | Tamaño | Lo escribe | Contenido |
|---|---|---|---|---|
| `ZXV_S_BEEPER` | 0x00 | 8 bits | el programa | Bit 0: nivel del beeper |
| `ZXV_S_AY_STEREO` | 0x01 | 8 bits | el programa | `ZXV_AY_STEREO_MONO` (0), `ZXV_AY_STEREO_ABC` (1, al arrancar) o `ZXV_AY_STEREO_ACB` (2), para los dos chips |
| `ZXV_S_VOLUME` | 0x02 | 8 bits | el programa | Volumen general 0–255 (200 al arrancar) |
| `ZXV_S_AFLAGS` | 0x03 | 8 bits | el programa | Bit 0: silencio |
| `ZXV_S_AY_HEAD` | 0x04 | 16 bits | el programa | Dónde escribe el programa en la cola del AY (`ZXV_AY_FIFO`) |
| `ZXV_S_AY_TAIL` | 0x06 | 16 bits | el sistema | Dónde lee el sistema |
| `ZXV_S_BEEP_HEAD` | 0x08 | 16 bits | el programa | Dónde escribe el programa en la cola con hora (`ZXV_BEEP_QUEUE`) |
| `ZXV_S_BEEP_TAIL` | 0x0A | 16 bits | el sistema | Dónde lee el sistema |
| `ZXV_S_SMP_VOLUME` | 0x0C | 8 bits | el programa | Volumen de los canales de muestras, 0–255 (255 al arrancar) |
| `ZXV_S_AY_REGS` | 0x10 | 16 × 8 bits | el sistema | Copia de solo lectura de los registros del primer AY, ya aplicados |
| `ZXV_S_AY2_REGS` | 0x20 | 16 × 8 bits | el sistema | Lo mismo, del segundo AY |
| `ZXV_S_SMP_REGS` | 0x40 | 96 × 8 bits | el sistema | Copia de solo lectura de los registros de los 12 canales de muestras (8 cada uno, en el orden de `ZXV_SMP_SLOT`...; `ZXV_SMP_CTRL` sin `KEY` ni `STOP`) |
| `ZXV_S_SMP_SRC` | 0xA0 | 32 bits | el programa | Una subida a la memoria de sonido: desde dónde (una dirección de la RAM) |
| `ZXV_S_SMP_DST` | 0xA4 | 32 bits | el programa | ... adónde (un desplazamiento en la memoria de sonido, de 0 a `ZXV_SMP_SOUND_MEM`) |
| `ZXV_S_SMP_LEN` | 0xA8 | 32 bits | el programa | ... y cuántos bytes (múltiplo de 4) |
| `ZXV_S_SMP_UPLOAD` | 0xAC | 8 bits | los dos | 1 la empieza; el sistema deja 0 al acabar, o 0x80 si no ha podido |

Las colas son más fáciles de usar con `zxv_ay_write`, `zxv_beeper_event`,
`zxv_ay_event` y `zxv_beeper_event_at`; la subida, con `zxv_smp_upload`
(ver [Canales de muestras](#canales-de-muestras)).

**La subida** copia en unas pocas imágenes; el programa no debe tocar el
origen hasta que `ZXV_S_SMP_UPLOAD` vuelve a 0. Con `ZXV_S_SMP_LEN` a 0
acaba en el acto. Queda en 0x80, sin copiar nada, si `ZXV_S_SMP_LEN` no es
múltiplo de 4, si no cabe desde `ZXV_S_SMP_DST` en la memoria de sonido, si
el origen no está en la RAM de la máquina (de `ZXV_SRAM_BASE` a
`ZXV_RESERVED_BASE`) o, en la placa, si la Pico ULA no contesta en un
segundo (un firmware anterior).

**La memoria de sonido** es de la Pico ULA: los programas solo llegan a
ella por la subida y por los registros de los canales.

| Constante | Valor | Qué es |
|---|---|---|
| `ZXV_SMP_CHANNELS` | 12 | Canales de muestras |
| `ZXV_SMP_MEM` | 0x18000 (98.304) | Bytes para las muestras, desde el desplazamiento 0 |
| `ZXV_SMP_TABLE` | 0x18000 | Dónde empieza la tabla de muestras (detrás de las muestras) |
| `ZXV_SMP_SLOTS` | 256 | Entradas de la tabla de muestras |
| `ZXV_SMP_SLOT_SIZE` | 12 | Bytes por entrada |
| `ZXV_SMP_SOUND_MEM` | 0x18C00 (101.376) | La memoria de sonido entera: muestras y tabla |
| `ZXV_SMP_ADPCM_BLOCK` | 64 | Muestras por bloque de ADPCM IMA |
| `ZXV_SMP_ADPCM_BYTES` | 36 | Bytes por bloque de ADPCM |
| `ZXV_SMP_KEYON` | 37 | Número para las colas: arranca a la vez los canales 0–7 (bits 0–7) y los de `ZXV_SMP_KEYON_HI` |
| `ZXV_SMP_KEYON_HI` | 36 | Número para las colas: los canales 8–11 (bits 0–3) del siguiente `ZXV_SMP_KEYON` |
| `ZXV_AY_FIFO_LEN` | 256 | Entradas de la cola del AY (2 bytes cada una) |
| `ZXV_BEEP_QUEUE_LEN` | 256 | Entradas de la cola con hora (8 bytes cada una) |

`zxv_smp_define` escribe las entradas de la tabla y `zxv_smp_start`, el
`ZXV_SMP_KEYON`; los registros de cada canal están en
[Canales de muestras](#canales-de-muestras).

La mezcla estéreo: en ABC, izquierda = A + la mitad de B y derecha = C + la
mitad de B; en ACB, izquierda = A + la mitad de C y derecha = B + la mitad
de C; en mono, los tres en los dos lados. El beeper va a los dos lados. Los
canales de la misma letra de los dos chips se suman.

## Variables del sistema

En `ZXV_SYSVARS` (`0x2000_0700`), de 32 bits cada una. Los programas solo
las leen, salvo que se diga otra cosa.

| Variable | Desplazamiento | Contenido |
|---|---|---|
| `ZXV_SV_FRAMES` | 0x00 | Contador de frames, +1 en cada VBLANK (60 por segundo; no con las interrupciones desactivadas) |
| `ZXV_SV_VERSION` | 0x04 | Versión de la especificación (`mayor << 16 \| menor`): 3, la 0.3, con esta ROM (`ZXV_SPEC_VERSION`) |
| `ZXV_SV_FRAME_HOOK` | 0x08 | El gancho de frame actual (cámbialo con `zxv_set_frame_hook`) |
| `ZXV_SV_RAMTOP` | 0x0C | Tope de la RAM usable; aquí empieza la pila del sistema (`ZXV_URAM_END`) |
| `ZXV_SV_URAM_START`, `ZXV_SV_URAM_END` | 0x10, 0x14 | Principio y fin (excluido) de la RAM de usuario |
| `ZXV_SV_CPU_HZ` | 0x18 | Frecuencia del procesador (`ZXV_CPU_HZ_REF`, 144 MHz) |
| `ZXV_SV_SYSFLAGS` | 0x1C | `ZXV_SYSF_LINK_BUSY` (bit 0): se está enviando la página visible; `ZXV_SYSF_BREAK` (bit 1): la tecla BREAK está pulsada |
| `ZXV_SV_LANG` | 0x20 | El idioma de la máquina, `ZXV_LANG(a, b)` (usa `zxv_lang_get` y `zxv_lang_set`) |
| `ZXV_SV_EDITOR` | 0x24 | 1 mientras el BASIC espera una orden (en el editor o con un informe en la pantalla); 0 mientras ejecuta algo, también un programa nativo. La escribe la ROM; la página de los juegos para la web la mira para empezarlos y volver a empezarlos |
| `ZXV_SV_PREFS` | 0x28 | Las preferencias de la máquina, que cambia el usuario y siguen al apagarla (ver abajo). Las cambia la ROM |
| `ZXV_SV_PROG_FILE` | 0x2C | El fichero del programa `.zxv` en marcha, abierto mientras va si lleva [bancos](#bancos): su número + 1 (0 si no). La escribe el cargador. Desde la especificación 0.2 |
| `ZXV_SV_PROG_CRC` | 0x30 | El CRC de la imagen del programa en marcha, el de su cabecera (0 sin programa). La escribe el cargador. Desde la especificación 0.2 |

**Las preferencias** (`ZXV_SV_PREFS`):

| Constante | Valor | Qué es |
|---|---|---|
| `ZXV_PREF_NO_CLICK` | 0x01 | Las teclas del editor del BASIC (y de `INPUT`, del menú EDIT y de las esperas de una tecla) no hacen clic. Se cambia en el menú EDIT del editor. Los programas que leen el teclado no hacen clic nunca |
| `ZXV_PREFS_MASK` | 0x0F | Los bits que hay; los demás están siempre a 0. Los bits 1–3 están reservados (a 0) |

La ROM las lee de la plataforma al arrancar en frío y se las pasa al
cambiarlas; con un firmware anterior, empiezan a 0 cada vez que se
enciende.

## Códigos de error

Las funciones de la API que pueden fallar devuelven un número negativo.
`ZXV_OK` (0) es que todo ha ido bien; algunas devuelven además un número
positivo (un tamaño, un número de fichero).

| Código | Constante | Significado |
|---|---|---|
| 0 | `ZXV_OK` | Todo bien |
| −1 | `ZXV_E_NOENT` | No existe: el fichero, la carpeta, el programa o el banco |
| −2 | `ZXV_E_EXIST` | Ya existe (al crear una carpeta o renombrar sobre otra cosa) |
| −3 | `ZXV_E_NAME` | Nombre o ruta no válidos |
| −4 | `ZXV_E_NOTDIR` | Una parte de la ruta no es una carpeta |
| −5 | `ZXV_E_ISDIR` | Es una carpeta (y se esperaba un fichero) |
| −6 | `ZXV_E_NOTEMPTY` | La carpeta no está vacía |
| −7 | `ZXV_E_BADF` | Número de fichero o carpeta no válido (no está abierto) |
| −8 | `ZXV_E_MFILE` | Demasiados abiertos (`ZXV_MAX_FILES` ficheros o `ZXV_MAX_DIRS` carpetas por unidad) |
| −9 | `ZXV_E_ACCESS` | No permitido: por el modo de apertura, en `ROM:`, al borrar la raíz, un fichero que ya está abierto para escribir, o `zxv_exec` desde un programa nativo |
| −10 | `ZXV_E_FAULT` | Un buffer está fuera de la memoria permitida |
| −11 | `ZXV_E_IO` | Error de lectura o escritura |
| −12 | `ZXV_E_NOCARD` | No hay tarjeta SD (o la plataforma no tiene ficheros) |
| −13 | `ZXV_E_NOSPC` | La tarjeta está llena |
| −14 | `ZXV_E_FORMAT` | No es un `.zxv` válido: firma, cabecera, pantalla de carga o CRC |
| −15 | `ZXV_E_RANGE` | No cabe: el programa en la RAM de usuario, o el fichero o el banco en el buffer (`zxv_file_load`, `zxv_dir_current`, `zxv_bank_load`), o una subida a la memoria de sonido (`zxv_smp_upload`) |
| −16 | `ZXV_E_NOCLOCK` | El reloj no está en hora, o la máquina no tiene reloj |
| −17 | `ZXV_E_INVAL` | Valor no válido: una fecha que no existe (`zxv_clock_set`) o un idioma que no vale (`zxv_lang_set`) |
| −18 | `ZXV_E_OFFLINE` | La unidad `EXT:` no tiene servidor, o ha dejado de contestar |
| −19 | `ZXV_E_CHANGED` | El fichero del programa ha cambiado mientras iba: sus [bancos](#bancos) ya no son los del programa en marcha; o, en `zxv_exec`, mientras se cargaba |

Si se saca la tarjeta, o deja de responder, con ficheros o carpetas
abiertos, estos conservan su número (un fichero que se abra después no se
queda con ninguno de ellos) y todas las llamadas sobre ellos dan
`ZXV_E_NOCARD` o `ZXV_E_IO` hasta que los cierras. Cerrarlos da `ZXV_OK`:
lo que ya se había escrito está en la tarjeta.

Con la biblioteca de C, estos códigos llegan en `errno` (ver
[picolibc](#la-biblioteca-estándar-de-c-picolibc)).

## Números de las funciones de la API

La API es una tabla de 256 direcciones de función en `ZXV_API_TABLE`
(`0x2000_0000`). La entrada *n* está en `ZXV_API_TABLE + 4·n`. Desde C, las
funciones de las cabeceras usan la macro `ZXV_API_FN(n, tipo)`, que lee la
entrada y la convierte en un puntero a función de ese tipo. En
ensamblador:

```asm
    lui  t0, %hi(ZXV_API_TABLE)
    lw   t0, 4*ZXV_API_CLS(t0)
    jalr t0
```

| n | Constante | Función | Desde |
|---|---|---|---|
| 0 | `ZXV_API_VERSION` | `zxv_api_version` | 0.1 |
| 1 | `ZXV_API_WAIT_VBLANK` | `zxv_wait_vblank` | 0.1 |
| 2 | `ZXV_API_SET_FRAME_HOOK` | `zxv_set_frame_hook` | 0.1 |
| 3 | `ZXV_API_TIME_US` | `zxv_time_us` | 0.1 |
| 4 | `ZXV_API_SET_MODE` | `zxv_set_mode` | 0.1 |
| 5 | `ZXV_API_SET_PAGE` | `zxv_set_page` | 0.1 |
| 6 | `ZXV_API_CLS` | `zxv_cls` | 0.1 |
| 7 | `ZXV_API_PLOT` | `zxv_plot` | 0.1 |
| 8 | `ZXV_API_PRINT_CHAR` | `zxv_print_char` | 0.1 |
| 9 | `ZXV_API_PRINT_STR` | `zxv_print_str` | 0.1 |
| 10 | `ZXV_API_KEY_GET` | `zxv_key_get` | 0.1 |
| 11 | `ZXV_API_JOY_READ` | `zxv_joy_read` | 0.1 |
| 12 | `ZXV_API_AY_WRITE` | `zxv_ay_write` | 0.1 |
| 13 | `ZXV_API_BEEPER_EVENT` | `zxv_beeper_event` | 0.1 |
| 14 | `ZXV_API_FILE_OPEN` | `zxv_file_open` | 0.1 |
| 15 | `ZXV_API_FILE_READ` | `zxv_file_read` | 0.1 |
| 16 | `ZXV_API_FILE_WRITE` | `zxv_file_write` | 0.1 |
| 17 | `ZXV_API_FILE_CLOSE` | `zxv_file_close` | 0.1 |
| 18 | `ZXV_API_FILE_LOAD` | `zxv_file_load` | 0.1 |
| 19 | `ZXV_API_EXIT_TO_BASIC` | `zxv_exit_to_basic` | 0.1 |
| 20 | `ZXV_API_REPORT_ERROR` | `zxv_report_error` | 0.1 |
| 21 | `ZXV_API_FILE_SEEK` | `zxv_file_seek` | 0.1 |
| 22 | `ZXV_API_FILE_STAT` | `zxv_file_stat` | 0.1 |
| 23 | `ZXV_API_FILE_REMOVE` | `zxv_file_remove` | 0.1 |
| 24 | `ZXV_API_FILE_RENAME` | `zxv_file_rename` | 0.1 |
| 25 | `ZXV_API_DIR_MAKE` | `zxv_dir_make` | 0.1 |
| 26 | `ZXV_API_DIR_CHANGE` | `zxv_dir_change` | 0.1 |
| 27 | `ZXV_API_DIR_CURRENT` | `zxv_dir_current` | 0.1 |
| 28 | `ZXV_API_DIR_OPEN` | `zxv_dir_open` | 0.1 |
| 29 | `ZXV_API_DIR_READ` | `zxv_dir_read` | 0.1 |
| 30 | `ZXV_API_DIR_CLOSE` | `zxv_dir_close` | 0.1 |
| 31 | `ZXV_API_ZXV_EXEC` | `zxv_exec` | 0.1 |
| 32 | `ZXV_API_GFX_TARGET` | `zxv_gfx_target` | 0.1 |
| 33 | `ZXV_API_GFX_DRAW_PAGE` | `zxv_gfx_draw_page` | 0.1 |
| 34 | `ZXV_API_GFX_CLIP` | `zxv_gfx_clip` | 0.1 |
| 35 | `ZXV_API_GFX_BLIT` | `zxv_gfx_blit` | 0.1 |
| 36 | `ZXV_API_SPR_PREPARE` | `zxv_spr_prepare` | 0.1 |
| 37 | `ZXV_API_SPR_DRAW` | `zxv_spr_draw` | 0.1 |
| 38 | `ZXV_API_SPR_COLLIDE` | `zxv_spr_collide` | 0.1 |
| 39 | `ZXV_API_MAP_DRAW` | `zxv_map_draw` | 0.1 |
| 40 | `ZXV_API_MAP_CELL` | `zxv_map_cell` | 0.1 |
| 41 | `ZXV_API_M7_DRAW` | `zxv_m7_draw` | 0.1 |
| 42 | `ZXV_API_M7_CAMERA` | `zxv_m7_camera` | 0.1 |
| 43 | `ZXV_API_GFX_LINE` | `zxv_gfx_line` | 0.1 |
| 44 | `ZXV_API_GFX_RECT` | `zxv_gfx_rect` | 0.1 |
| 45 | `ZXV_API_GFX_TRI` | `zxv_gfx_tri` | 0.1 |
| 46 | `ZXV_API_GFX_POLY` | `zxv_gfx_poly` | 0.1 |
| 47 | `ZXV_API_GFX_CIRCLE` | `zxv_gfx_circle` | 0.1 |
| 48 | `ZXV_API_CLOCK_GET` | `zxv_clock_get` | 0.1 |
| 49 | `ZXV_API_CLOCK_SET` | `zxv_clock_set` | 0.1 |
| 50 | `ZXV_API_LANG_SET` | `zxv_lang_set` | 0.1 |
| 51 | `ZXV_API_AY_EVENT` | `zxv_ay_event` | 0.1 |
| 52 | `ZXV_API_SPR_DRAW_SCALED` | `zxv_spr_draw_scaled` | 0.1 |
| 53 | `ZXV_API_TEXT_COLORS` | `zxv_text_colors` | 0.1 |
| 54 | `ZXV_API_TEXT_AT` | `zxv_text_at` | 0.1 |
| 55 | `ZXV_API_BEEPER_EVENT_AT` | `zxv_beeper_event_at` | 0.1 |
| 56 | `ZXV_API_GFX_PATTERN` | `zxv_gfx_pattern` | 0.2 |
| 57 | `ZXV_API_M7_DRAW_MAP` | `zxv_m7_draw_map` | 0.2 |
| 58 | `ZXV_API_M7_AFFINE` | `zxv_m7_affine` | 0.2 |

Los números del 32 al 63 están reservados para los gráficos. Las entradas
sin usar devuelven −1 y no hacen nada: un programa que llama a una función
de la especificación 0.2 en una ROM de la 0.1 no se cuelga. Con una plataforma
sin ficheros, las funciones de ficheros devuelven `ZXV_E_NOCARD`, y con una
sin reloj, las del reloj, `ZXV_E_NOCLOCK`.

No tienen número, porque no llaman a la ROM: `zxv_lang_get` y las del
ratón (leen la memoria directamente), las de los canales de muestras
(`zxv_smp_*`, que escriben los registros por `zxv_ay_write` y la subida),
las de los [bancos](#bancos) (en `zxvbank.c`, dentro del programa) y las de
los [ficheros `.gfx`](#ficheros-de-gráficos-gfx) (en `gfxfile.c`).

Como la tabla está en la RAM, un programa puede cambiar una entrada para
interceptar una rutina.

## Mapa de memoria

| Desde | Hasta | Tamaño | Contenido | Acceso |
|---|---|---|---|---|
| `0x1000_0000` | `0x1003_FFFF` | 256 KB | Firmware de la plataforma | Ninguno |
| `0x1004_0000` | `0x100B_FFFF` | 512 KB | La ROM (`ZXV_ROM_BASE`): el BASIC y la API | Lectura, ejecución |
| `0x1040_0000` | `0x1067_AFFF` | 2.540 KB | La unidad `ROM:` (`ZXV_ROMDISK_BASE`, `ZXV_ROMDISK_SIZE`) | Con las funciones de ficheros |
| `0x2000_0000` | `0x2000_3FFF` | 16 KB | Área de sistema (`ZXV_SYS_BASE`) | Ver abajo |
| `0x2000_4000` | `0x2001_BFFF` | 96 KB | Página de vídeo 0 (`ZXV_VRAM0_BASE`) | Lectura, escritura |
| `0x2001_C000` | `0x2003_3FFF` | 96 KB | Página de vídeo 1 (`ZXV_VRAM1_BASE`) | Lectura, escritura |
| `0x2003_4000` | `0x2006_FFFF` | 240 KB | RAM de usuario (de `ZXV_URAM_BASE` a `ZXV_URAM_END`) | Lectura, escritura, ejecución |
| `0x2007_0000` | `0x2008_1FFF` | 72 KB | Reservada para el sistema (`ZXV_RESERVED_BASE`) | **Prohibido** |

**Área de sistema:**

| Dirección | Constante | Contenido |
|---|---|---|
| `0x2000_0000` | `ZXV_API_TABLE` | Tabla de la API (256 direcciones) |
| `0x2000_0400` | `ZXV_VIDEO_REGS` | [Registros de vídeo](#registros-de-vídeo) |
| `0x2000_0480` | `ZXV_PALETTES` | Paletas de las tablas de remapeo (4 × 16 colores RGB565) |
| `0x2000_0500` | `ZXV_INPUT_REGS` | [Registros de entrada](#registros-de-entrada) |
| `0x2000_0600` | `ZXV_SOUND_REGS` | [Registros de sonido](#registros-de-sonido) |
| `0x2000_0700` | `ZXV_SYSVARS` | [Variables del sistema](#variables-del-sistema) |
| `0x2000_0800` | `ZXV_AY_FIFO` | Cola del AY (256 × 2 bytes) |
| `0x2000_0A00` | `ZXV_BEEP_QUEUE` | Cola con hora (256 × 8 bytes) |
| `0x2000_1200` | `ZXV_REMAP_TABLES` | Tablas de remapeo de colores (4 × 16 bytes) |
| `0x2000_1240` | `ZXV_PLATFORM_IF` | Interfaz de la ROM con la plataforma ([interna](#definiciones-internas)) |
| `0x2000_1800` | `ZXV_LINE_TABLE` | Tabla por línea (480 × 8 bytes) |
| `0x2000_2700` | `ZXV_DBG_BOX` | Buzón del depurador del BASIC ([interno](#definiciones-internas): lo usan la ROM y el emulador) |
| `0x2000_2A00` | `ZXV_ROM_WORKSPACE` | Espacio de trabajo de la ROM, hasta `ZXV_ROM_WORKSPACE_END` (`0x2000_4000`, [interno](#definiciones-internas)) |

El resto del área de sistema es para uso interno de la ROM.

El procesador es un RISC-V de 32 bits (RV32IMAC con las extensiones Zba,
Zbb, Zbs, Zbkb y Zcb, `-march=rv32imac_zicsr_zifencei_zba_zbb_zbs_zbkb_zcb
-mabi=ilp32`), con el byte bajo primero. Los **accesos desalineados** a
datos de 16 o 32 bits detienen el programa. Los programas pueden activar y
desactivar las interrupciones (el bit 3 de `mstatus`) y leer
`mcycle`/`minstret`; los demás registros de control son del sistema.

### Cómo coloca el SDK un programa

`zxv.ld` pone, desde `0x2003_4000`: el código (empezando por `crt0.S`), las
constantes, las variables con valor inicial y luego las variables a cero
(el BSS). Define estos símbolos:

| Símbolo | Significado |
|---|---|
| `__image_end` | Final de lo que se guarda en el `.zxv` |
| `__bss_start`, `__bss_end` | Las variables a cero (el sistema las borra antes de empezar) |
| `__heap_start`, `__heap_end` | Memoria libre para el `malloc` de picolibc: del final del BSS a 32 KB por debajo del final de la RAM de usuario |
| `__tls_base` | Las variables de hilo (el `errno` de picolibc) |
| `__vram0_start`, `__vram0_end`, `__vram1_start`, `__vram1_end` | Lo declarado con `ZXV_VRAM0_DATA` y `ZXV_VRAM1_DATA`: en la VRAM oculta de cada página, desde `ZXV_VRAMn_BASE + ZXV_VRAM_HIDDEN` (72 KB, `ZXV_VRAM_HIDDEN_SIZE`); `crt0.S` lo pone a cero |

Lo declarado con `ZXV_VRAM0_DATA` o `ZXV_VRAM1_DATA` (por ejemplo,
`ZXV_VRAM0_DATA static uint8_t alturas[18 * 1024];`) no va en el fichero:
empieza a cero y no puede tener otro valor inicial (`elf2zxv.py` lo
rechaza). Se pierde si el programa pasa al modo 1 o usa otro tamaño de
página (`ZXV_V_VSIZE`), que ocupan la página entera, y un programa que lo
usa no puede empezar en el modo 1.

La pila es la del sistema: empieza cerca de `ZXV_URAM_END` y crece hacia
abajo.

## El formato `.zxv`

Una cabecera de 72 bytes, seguida de la pantalla de carga, si la hay, y de
la imagen del programa, que se copia en la dirección de carga; detrás, si
los hay, los bancos. Todos los números van con el byte bajo primero.

| Desplazamiento | Constante | Tamaño | Campo |
|---|---|---|---|
| 0x00 | `ZXV_H_MAGIC` | 4 bytes | `ZXV1` (`ZXV_HDR_MAGIC`, 0x3156585A) |
| 0x04 | `ZXV_H_HDR_SIZE` | 16 bits | Tamaño de la cabecera: 72 (`ZXV_HDR_SIZE`) |
| 0x06 | `ZXV_H_PROFILE` | 16 bits | Perfil de la especificación: 1 (`ZXV_HDR_PROFILE`) |
| 0x08 | `ZXV_H_LOAD` | 32 bits | Dirección de carga |
| 0x0C | `ZXV_H_ENTRY` | 32 bits | Punto de entrada |
| 0x10 | `ZXV_H_IMAGE_SIZE` | 32 bits | Tamaño de la imagen |
| 0x14 | `ZXV_H_BSS_SIZE` | 32 bits | Bytes que se ponen a cero detrás de la imagen (con las zonas de los bancos de código) |
| 0x18 | `ZXV_H_STACK` | 32 bits | Tope de la pila (0 = seguir con la del sistema) |
| 0x1C | `ZXV_H_MODE` | 8 bits | Modo de vídeo inicial (cuenta su bit 0) |
| 0x1D | `ZXV_H_FLAGS` | 8 bits | Reservado: se escribe a 0 y se ignora al cargar |
| 0x1E | | 16 bits | Reservado |
| 0x20 | `ZXV_H_CRC` | 32 bits | CRC-32 de la imagen (el de zlib) |
| 0x24 | `ZXV_H_NAME` | 28 bytes | Nombre (ASCII, terminado en cero) |
| 0x40 | `ZXV_H_SCREEN` | 32 bits | Tamaño de la pantalla de carga, que va detrás de la cabecera (0 = no hay) |
| 0x44 | | 32 bits | Reservado (0) |

Al cargarlo, la ROM comprueba:

| Comprobación | Si no |
|---|---|
| La firma, el tamaño de la cabecera (72) y el perfil (1) | `ZXV_E_FORMAT` |
| Que la dirección de carga esté alineada a 4 y el punto de entrada sea par y esté dentro de la imagen | `ZXV_E_FORMAT` |
| Que la imagen y el BSS empiecen en la RAM de usuario y quepan dejando 4 KB por debajo de la pila de la ROM | `ZXV_E_RANGE` |
| Que un tope de pila distinto de 0 esté alineado a 16 y quede entre el final del BSS y `ZXV_URAM_END` | `ZXV_E_RANGE` |
| La pantalla de carga, si la hay (abajo) | `ZXV_E_FORMAT` |
| Que se lea la imagen entera y su CRC | `ZXV_E_FORMAT` |

Después borra el BSS, pone el modo de vídeo (si la pantalla de carga no es
ya de ese modo; si lo es, el programa empieza con ella en la página 0), pone
`ZXV_V_VSIZE` a 0 y llama al punto de entrada como `int entry(void)`. Antes
de cargar, deja el motor gráfico en la página 0 sin recorte ni trama, y los
colores y el texto como al principio. Un `.zxv` con la cabecera de 64 bytes
de antes no carga (`ZXV_E_FORMAT`): hay que volver a pasarlo por
`elf2zxv.py`.

### La pantalla de carga

Son los datos de un objeto pantalla de un `.gfx`, de
`ZXV_SCREEN_SIZE_F(modo, marcas)` bytes:

| Desplazamiento | Tamaño | Campo |
|---|---|---|
| 0 | 8 bits | Modo, 0 o 1 |
| 1 | 8 bits | Color del borde, 0–15 |
| 2 | 8 bits | Marcas: `ZXV_SCREEN_F_PALETTE` (0x01), lleva paleta; las demás, a 0 |
| 3 | 8 bits | 0 |
| 4 | 32 bytes | Si lleva paleta, sus 16 colores RGB565 (`ZXV_SCREEN_PALETTE`, 32) |
| 4 o 36 | 24.576 o 98.304 bytes | Los píxeles de la página, como en la VRAM (`ZXV_SCREEN_PIXELS(modo)`) |

`ZXV_SCREEN_HEAD` (4) es el tamaño de la cabecera de la pantalla;
`ZXV_SCREEN_SIZE(modo)`, el de una pantalla sin paleta, y
`ZXV_SCREEN_SIZE_F(modo, marcas)`, el de la pantalla entera. La ROM
comprueba que el modo, el borde, las marcas y el 0 valen y que el tamaño de
`ZXV_H_SCREEN` es justo ese; si no, `ZXV_E_FORMAT`. La pone antes de cargar
la imagen, con su modo, su borde (sin scroll ni remapeo) y, si la trae, su
paleta en la tabla 0.

### Los bancos

Los [bancos](#bancos) van detrás de la imagen, que el cargador no mira
(así que las ROM anteriores cargan el programa igual). Al final del
fichero, el cierre, de `ZXV_BANK_TAIL` (16) bytes:

| Desplazamiento | Constante | Tamaño | Campo |
|---|---|---|---|
| 0 | `ZXV_BANK_MAGIC` | 8 bytes | `ZXVBANK1` |
| 8 | `ZXV_BANK_T_INDEX` | 32 bits | Dónde empieza el índice, desde el principio del fichero |
| 12 | `ZXV_BANK_T_INDEX_LEN` | 32 bits | Lo que ocupa el índice |

El índice: una cabecera de `ZXV_BANK_HEAD` (16) bytes y una entrada de
`ZXV_BANK_ENTRY` (48) bytes por banco.

| Desplazamiento | Constante | Tamaño | Campo |
|---|---|---|---|
| 0 | `ZXV_BANK_I_COUNT` | 32 bits | Cuántos bancos |
| 4 | `ZXV_BANK_I_PROG_CRC` | 32 bits | El CRC de la imagen del programa (su huella) |
| 8 | `ZXV_BANK_I_IMAGE_SIZE` | 32 bits | El tamaño de la imagen |
| 12 | `ZXV_BANK_I_CRC` | 32 bits | El CRC de las entradas |

Cada entrada:

| Desplazamiento | Constante | Tamaño | Campo |
|---|---|---|---|
| 0 | `ZXV_BANK_E_NAME` | 16 bytes | Nombre (ASCII, hasta `ZXV_BANK_NAME_MAX`, 15, terminado en cero) |
| 16 | `ZXV_BANK_E_KIND` | 8 bits | `ZXV_BANK_DATA` (0) o `ZXV_BANK_CODE` (1) |
| 17 | `ZXV_BANK_E_METHOD` | 8 bits | `ZXV_BANK_STORED` (0) o `ZXV_BANK_DEFLATE` (1, deflate sin envoltorio, RFC 1951) |
| 18 | `ZXV_BANK_E_REGION` | 8 bits | La zona, de 0 a 7 (los de código) |
| 20 | `ZXV_BANK_E_OFFSET` | 32 bits | Dónde empieza, desde el principio del fichero |
| 24 | `ZXV_BANK_E_STORED` | 32 bits | Lo que ocupa en el fichero |
| 28 | `ZXV_BANK_E_SIZE` | 32 bits | Lo que ocupa de verdad |
| 32 | `ZXV_BANK_E_CRC` | 32 bits | CRC-32 de lo de verdad |
| 36 | `ZXV_BANK_E_ADDR` | 32 bits | Los de código, dónde van: el principio de su zona (llevan su código, sus constantes, sus datos y su BSS a cero) |

Lo demás, reservado (0). Si el `.zxv` acaba en ese cierre, la ROM deja su
fichero abierto mientras va el programa (`ZXV_SV_PROG_FILE`).

## Herramientas

Las de Python están en `sdk/tools` y solo necesitan Python 3 (sin otros
paquetes). Las de C y C++ (`zxv-music`, `zxv-link`, `zxv-link-gui`,
`zxv-gfx` y `zxv-tracker`) van compiladas en los paquetes del emulador.
`elf2zxv.py`, `zxvbanks.py`, `png2gfx.py`, `mkweb.py`, `zxv-link` y los
editores hablan español si el sistema está en español (por `LC_ALL`,
`LC_MESSAGES` o `LANG` o, en Windows, el idioma del usuario), y si no,
inglés; `obj2c.py`, `fontconv.py` y `zxv-music`, solo español.

### `elf2zxv.py`

```sh
python3 sdk/tools/elf2zxv.py ENTRADA.elf SALIDA.zxv [--name NOMBRE] [--mode 0|1]
        [--stack DIRECCIÓN] [--screen PANTALLA] [--bank NOMBRE=FICHERO ...] [--pack]
```

Convierte un ELF de RISC-V de 32 bits, enlazado con `sdk/zxv.ld`, en un
programa `.zxv`: la cabecera, la pantalla de carga si la hay, la imagen y,
detrás, los bancos. El `Makefile` del SDK lo llama solo.

| Opción | Efecto |
|---|---|
| `--name NOMBRE` | Nombre en la cabecera: hasta 27 caracteres ASCII (los demás salen como `?`). Por defecto, el del fichero de salida sin la extensión (lo que va detrás de la última `/`) |
| `--mode 0\|1` | Modo de vídeo inicial (0 por defecto) |
| `--stack DIRECCIÓN` | Tope de la pila, en decimal o `0x...`; por defecto 0, la del sistema. Tiene que cumplir lo del [formato](#el-formato-zxv) o el programa no carga |
| `--screen PANTALLA` | La pantalla de carga: un PNG (de 256 × 192 o 512 × 384, o el cuadro entero con el borde, 320 × 240 o 640 × 480, como `png2gfx.py --screen`) o un `.gfx` con una pantalla (la primera). La ROM la pone antes de cargar el programa, que empieza con ella si es de su modo |
| `--bank NOMBRE=FICHERO` | Un banco de datos: el contenido de `FICHERO`, que el programa carga con `zxv_bank_load("NOMBRE", ...)`. Se puede repetir. El nombre, de 1 a 15 caracteres ASCII sin espacios, sin repetir |
| `--pack` | Comprime cada banco (deflate, nivel 9) si así ocupa menos |

Escribe `SALIDA.zxv` y dice la dirección de carga, el punto de entrada y
los tamaños de la imagen y del BSS, y de cada banco. Los bancos de código
los saca del propio ELF (las secciones `.zxvbank.ZONA.NOMBRE` que deja
[`zxvbanks.py`](#zxvbankspy)). Se para, sin escribir nada, si el ELF no es
de RISC-V de 32 bits, si el programa no cabe en la RAM de usuario, si la
dirección de carga no está alineada a 4 o el punto de entrada no está en la
imagen, si hay datos con valor inicial en la VRAM oculta, si el programa
usa la VRAM oculta y `--mode` es 1, o si hay más de 4096 bancos.

### `zxvbanks.py`

```sh
python3 sdk/tools/zxvbanks.py -o SALIDA.zxv [--cc GCC] [--cflags "..."] [--sdk CARPETA]
        RESIDENTE.c ... --code NOMBRE[@ZONA]=A.c,B.c ... [--bank NOMBRE=FICHERO ...]
        [--pack] [--name NOMBRE] [--mode 0|1] [--screen PANTALLA]
```

Compila un programa con **bancos de código**: funciones que no están en la
memoria a la vez, sino que se cargan del propio `.zxv` cuando se las llama
(ver [Bancos](#bancos)). Lo residente (lo que está siempre) va en la RAM
como siempre; cada banco se compila aparte y se enlaza en su zona, que
mide lo que su banco más grande y va detrás del BSS. El programa llama a
las funciones de un banco como a cualquier otra: un «puente» residente
carga el banco si hace falta y salta a ella.

| Opción | Efecto |
|---|---|
| `-o SALIDA.zxv` | El programa que se escribe (obligatoria) |
| `RESIDENTE.c ...` | Los ficheros de lo residente (`.c`, `.S`). `crt0.S`, `zxvlib.c`, `zxvbank.c` y `zxvbankcode.c` del SDK se añaden solos |
| `--code NOMBRE[@ZONA]=A.c,B.c` | Un banco de código y sus ficheros. El nombre, de 1 a 15 letras, cifras o `_`; la zona, de 0 a 7 (0 por defecto). Se puede repetir; hace falta al menos uno (sin bancos de código, `elf2zxv.py`) |
| `--bank NOMBRE=FICHERO` | Un banco de datos, como en `elf2zxv.py`. Se puede repetir |
| `--pack` | Comprime los bancos, como en `elf2zxv.py` |
| `--name`, `--mode`, `--screen` | Como en `elf2zxv.py` |
| `--cc GCC` | El compilador (`riscv64-unknown-elf-gcc` por defecto); `objcopy`, `nm`, `objdump` y `readelf`, los de su mismo prefijo |
| `--cflags "..."` | Las opciones del compilador (por defecto, las del SDK: `-march=... -mabi=ilp32 -Os -g -Wall -Wextra -std=gnu11 -ffreestanding ...`), partidas como en la línea de órdenes del sistema: en Windows, las `\` de las rutas se quedan y las comillas agrupan (`-I"C:\mis cosas\include"`) |
| `--sdk CARPETA` | La carpeta del SDK (por defecto, la de la herramienta) |

Escribe `SALIDA.zxv`, `SALIDA.elf` (para depurar) y, en `SALIDA.bancos/`,
lo de en medio y los símbolos para GDB (`gdb/residente.elf` y
`gdb/BANCO.elf`, para [`zxvbanks-gdb.py`](#zxvbanks-gdbpy)). Se para si dos
bancos tienen el mismo nombre o la misma función, si un banco exporta una
variable (hazla `static` o ponla en lo residente: se borraría al cargar
otro banco de su zona), tiene variables de hilo o las tiene en la VRAM
oculta (`ZXV_VRAM0_DATA`, `ZXV_VRAM1_DATA`: decláralas en lo residente,
donde tampoco ocupan RAM de usuario). Avisa si un banco toma
la dirección de una de sus funciones: ese puntero solo vale mientras el
banco está cargado.

### `zxvbanks-gdb.py`

```sh
gdb-multiarch juego.elf -x sdk/tools/zxvbanks-gdb.py -ex "target remote :1234"
```

Un guion de GDB (o, ya dentro, `source sdk/tools/zxvbanks-gdb.py`) para
depurar un programa con bancos de código: los bancos de una zona están en
las mismas direcciones, y GDB solo sabe tener una función por dirección.
Usa `juego.bancos/gdb/residente.elf` en vez del ELF entero y, cada vez que
se carga un banco y cada vez que el programa se para, añade los símbolos
de los bancos cargados y quita los de los demás. Así, `break FUNCIÓN` de un
banco queda pendiente hasta que se carga y para solo en él, y `bt`, `list`
y `print` van con lo que hay de verdad. Los puentes se llaman
`FUNCIÓN.puente`. Añade la orden `zxvbanks`, que enseña los bancos y
cuáles están cargados. Con un programa sin bancos de código no hace nada.

### `png2gfx.py`

```sh
python3 sdk/tools/png2gfx.py SALIDA.gfx ORDEN...
python3 sdk/tools/png2gfx.py --list FICHERO.gfx
```

Convierte imágenes PNG en un fichero `.gfx` (el de `LOAD "x" GRAPHICS` y
de [`zxv_gfxfile.h`](#ficheros-de-gráficos-gfx)). Las órdenes, con
ejemplos, están en
[Gráficos](graphics.md#imágenes-desde-ficheros-png-png2gfxpy):

| Orden | Números | Efecto |
|---|---|---|
| `--sprite N IMAGEN [X,Y,AN,AL]` | N: 0–255 | El sprite N es la imagen o ese rectángulo (de 1 a 1024 de lado) |
| `--sprites N IMAGEN AN,AL` | N: 0–255 | Una hoja: los sprites N, N+1... de AN × AL, fila a fila (la imagen, múltiplo de AN × AL) |
| `--tiles S N IMAGEN [X,Y,AN,AL]` | S: 8 o 16; N: 0–4095 | La imagen o el rectángulo (múltiplo de S) cortado en tiles, numerados desde N |
| `--level M S IMAGEN` | M: 0–15 | Un nivel: cortado en tiles de S × S, aprovechando los repetidos (y, con `--flip`, los espejados), que se añaden a los que haya; el mapa M queda con su distribución, con el bit de transparencia en las celdas de tiles con píxeles transparentes |
| `--map M S FICHERO.csv` | M: 0–15 | El mapa M de un CSV con el valor de cada celda (0–65535, como en `MAP POKE`), una fila por línea |
| `--texture N IMAGEN [C]` | N: 0–3; C: 0–15 | La textura N: un cuadrado de lado potencia de 2, de 2 a 256. Se repite o, con C, fuera es del color C. Lo transparente queda negro |
| `--wrap M` | | El mapa M se repite |
| `--transparent N` | | Lo transparente del PNG de la textura N es el color 8 y no se dibuja (con C = 8, tampoco lo de fuera) |
| `--screen N IMAGEN [B]` | N: 0–255; B: 0–15 | La pantalla N: 256 × 192 (modo 0), 512 × 384 (modo 1), o el cuadro entero (320 × 240 o 640 × 480) con el borde del color de su esquina; B, el color del borde. Con colores que no son los de siempre (16 como mucho), lleva su paleta |
| `--flip` | | `--level` aprovecha también los tiles espejados |
| `--key RRGGBB` | | Ese color es transparente, además de los píxeles con alfa menor que 128 |

Cada píxel toma el color de ZX-V más cercano (avisa si no era exacto).
Lee cualquier PNG menos los entrelazados. Avisa si un mapa usa un tile que
no está. `--list` enseña lo que hay en un `.gfx`. Sale con un error, sin
escribir nada, si una orden no vale, una imagen no se puede leer o no
tiene el tamaño que toca.

### `obj2c.py`

```sh
python3 sdk/tools/obj2c.py MODELO.obj SALIDA.c NOMBRE [opciones]
python3 sdk/tools/obj2c.py MODELO.obj SALIDA.bas NOMBRE --bas [opciones]
```

Convierte un modelo Wavefront `.obj` (el de Blender y casi todos los
programas de 3D) en C para [`zxv_3d.h`](libraries.md#3d-sencillo-zxv_3dh):
`const zxv_model NOMBRE`, con los vértices en enteros de 16 bits y las
caras con sus colores. O, con `--bas`, en ZX Basic para `zxv3d.bas`. Lo
cuenta, con ejemplos,
[Modelos de Blender](libraries.md#modelos-de-blender-obj2cpy).

| Opción | Efecto |
|---|---|
| `--size S` | El lado más largo del modelo, en unidades del mundo (2 por defecto) |
| `--keep-origin` | No lo centra: el origen del `.obj` sigue siendo el del modelo |
| `--mat NOMBRE=OSCURO,CLARO[,two][,flat][,decal]` | Los colores (0–15) de un material, y sus opciones: `two`, se ve por los dos lados; `flat`, sin luz; `decal`, va pegado encima de otra cara y se dibuja después. Se puede repetir |
| `--default OSCURO,CLARO` | Los colores de las caras sin material (1,9 por defecto: azul) |
| `--two-sided` | Todas las caras se ven por los dos lados |
| `--bas` | Sale ZX Basic: las matrices `NOMBRE_v` y `NOMBRE_f` y el `SUB NOMBRE_model(n)`, que lo pone como el modelo `n` de `MODEL3D` |

Los colores de una cara salen de su material: uno llamado
`zxv_OSCURO_CLARO[_two][_flat][_decal]` da esos; si no, el color difuso
(`Kd`) de su `.mtl`, el más parecido de los 16 y su pareja (con brillo y
sin él); si no, `--default`. Las caras no convexas y las de más de 32
lados se parten en trozos convexos de hasta 32 lados; las que no tienen
área se quitan. Avisa si hay más de 512 vértices (lo que admite `zxv_3d`)
o si una cara se cruza consigo misma. Se para, con un mensaje que dice qué
y dónde, si una cara usa un vértice que no existe, si un vértice no tiene
sus tres números, si no hay vértices o caras, si unos colores no valen, si
una opción no tiene valor o si `--size` no es un número mayor que 0 o es
demasiado pequeño. Los comentarios (`#`) pueden ir al final de una línea.

### `fontconv.py`

```sh
python3 sdk/tools/fontconv.py ENTRADA SALIDA.c NOMBRE [opciones]
python3 sdk/tools/fontconv.py ENTRADA SALIDA.txt [opciones]
```

Convierte fuentes de mapa de bits en C para
[`zxv_font.h`](libraries.md#fuentes-de-letra-zxv_fonth) (`NOMBRE_n`,
`NOMBRE_h`, `NOMBRE_codes[]` y `NOMBRE_bits[]`), o en texto para
retocarlas a mano (si la salida acaba en `.txt`). Lo cuenta
[Fuentes de otros sitios](libraries.md#fuentes-de-otros-sitios-fontconvpy).

| Entrada | Formato |
|---|---|
| `.txt` | La de ZX-V (`rom/font/zxv8x8.txt`): `== CÓDIGO` y las filas, `#` tinta y `.` fondo |
| `.ch8`, o 768 bytes | Del ZX Spectrum: 96 caracteres, del 32 al 127 (su 0x5E es ↑, el 0x60 £ y el 0x7F ©) |
| `.bin`, `.rom`, `.fnt`, otros | Un volcado de 8 bytes por carácter (o `--height`): PC (CP437), MSX, Amstrad CPC... |
| `.psf` | PC Screen Font 1 o 2 (las de la consola de Linux), con su tabla Unicode si la lleva |
| `.bdf` | Glyph Bitmap Distribution Format (X11) |
| `.hex` | Como GNU Unifont: `CÓDIGO:BYTES`; los caracteres anchos (los de 16 columnas) se quedan fuera, con un aviso |

| Opción | Efecto |
|---|---|
| `--format F` | `txt`, `zx`, `raw`, `psf`, `bdf` o `hex`, si la extensión (o el contenido) no basta |
| `--first N` | `raw`: el código del primer carácter (0 por defecto; 32 si son 96) |
| `--height H` | `raw`: las filas de cada carácter (8 por defecto). `hex`: las de los de 8 columnas (por defecto, las de los más estrechos del fichero; si todos son de más de 16 bytes, hay que decirlo) |
| `--charset C` | `raw`: el juego de caracteres de Python con el que pasar los códigos de más de 127 a Unicode (`cp437` si son 256 desde el 0; si no, `latin-1`) |
| `--ascii` | `zx`: deja `^`, `` ` `` y el 127 como en ASCII |
| `--range R` | Los caracteres que se quedan, en hexadecimal: `20-7E,A0-FF` por defecto (ASCII y Latin-1, con las tildes, la ñ, ¿ y ¡) |

Las fuentes tienen que ser de hasta 8 columnas y de 1 a 32 filas, todos los
caracteres de `--range` iguales (los de fuera no cuentan). Se para si
quedan más de 224 caracteres (lo que guarda `zxv_font`; usa `--range`) o
ninguno, o si un volcado pasa del código 255 (más de 256 caracteres desde
`--first`: suele ser un `--height` que no es el de la fuente). La licencia de una fuente sigue
siendo la suya: que se pueda convertir no quiere decir que se pueda
repartir.

### `mkweb.py`

```sh
python3 sdk/tools/mkweb.py SALIDA JUEGO [FICHERO...] --kit KIT [opciones]
python3 sdk/tools/mkweb.py SALIDA --config game.json [--kit KIT] [opciones]
```

Prepara un juego para la web: una página que solo tiene ese juego, que
empieza solo y vuelve a empezar al acabar, sin que se vea el BASIC. Para
subirla a itch.io o a cualquier servidor; también funciona abierta desde el
disco. Lo cuenta, con los controles táctiles y `game.json`,
[Publicar tu juego en la web](web.md).

| Argumento u opción | Efecto |
|---|---|
| `SALIDA` | La carpeta de la página. Se crea; si ya existe, se borra entera antes |
| `JUEGO` | El programa que arranca: `.zvb`, `.zxv`, `.tap`, `.tzx` o un listado `.bas` (o `.txt`). Si es un `.zxv` o un `.zvb` con pantalla de carga, la página la enseña mientras empieza |
| `FICHERO...` | Lo que necesita el juego (`.gfx`, `.pt3`, `.afb`...): va con él en la unidad `ROM:`, donde el BASIC lo encuentra solo. Nombres de hasta 63 caracteres ASCII imprimibles, sin `\ : * ? " < > \|`, sin repetir |
| `--kit KIT` | La carpeta `play/` de la versión web (`zxv-web.zip`, o `emulator/build-web/web/play`): tiene que tener `index.html`, `play.js`, `play.css`, `engine.js` y `zxv-core.js`. Obligatoria |
| `--config game.json` | Las opciones de un fichero (abajo) |
| `--title TEXTO` | El nombre de la portada y de la pestaña (por defecto, el del juego) |
| `--id NOMBRE` | El nombre de su tarjeta en el navegador: letras, números, `-` y `_`, hasta 40 (por defecto, el del juego). Cambiarlo es empezar con la tarjeta vacía |
| `--lang auto\|es\|en` | El idioma de la máquina y de la página (`auto`, el del navegador, por defecto) |
| `--border 0\|1\|2` | El borde: completo (0, por defecto), pequeño o ninguno |
| `--fit proporcion\|entera\|llenar` | Cómo se escala la imagen (también `aspect`, `integer` y `fill`); `proporcion` por defecto |
| `--touch PRESET\|JSON` | Los controles táctiles: `joystick` (por defecto), `arrows`, `qaop` o `none`, o un fichero o texto JSON |
| `--zip FICHERO.zip` | Además, la página en un `.zip` (con `index.html` en la raíz) |
| `--kit-url URL` | No copia el kit: la página usa el de `URL` (relativa a ella, como `../kit/`). En la carpeta solo van `index.html` y `game.js`. No va con `--zip` |

`game.json` lleva lo mismo: `start`, `files`, `title`, `id`, `lang`,
`border`, `fit`, `touch`, `kit` y `kit_url`, con las rutas relativas a él;
las opciones de la línea de órdenes mandan sobre las suyas. Escribe en
`SALIDA` el kit (con sus licencias), `game.js` (el juego, los ficheros de
`ROM:` y las opciones) y, con `--kit-url`, solo `index.html` y `game.js`.
Sale con el código 1 y un mensaje si algo no vale, y con el 2 sin
argumentos (enseña la ayuda).

### `zxv-music`

```sh
zxv-music [--free KB] [--channels LISTA] ENTRADA SALIDA.zxm
```

Convierte un PT3, un TurboSound (Pro Tracker 3 y Vortex Tracker II), un
MOD, un S3M o un XM (de hasta 4 MB) al formato propio, `.zxm`, y dice lo
que no ha podido pasar y lo que ocupan las muestras en la memoria de
sonido. Lo que pasa de cada formato está en la
[guía](README.md#el-formato-propio-zxm-y-zxv-music).

| Opción | Efecto |
|---|---|
| `--free KB` | Deja libres esos KB (0–96) de la memoria de sonido para otras muestras, como los sonidos de `SAMPLE` en el BASIC: las de la canción que no caben van en ADPCM, a la mitad de muestras o, si ni así, fuera |
| `--channels LISTA` | De un MOD, un S3M o un XM, los canales que van, hasta 12, con los números del tracker, desde 1 (`1-8,11,13`). Sin ella, de un módulo de más de 12 canales van los 12 que más notas tocan |

Sale con el código 2 si las opciones no valen (o `--channels` con un PT3),
y con el 1 si no puede leer, convertir o escribir.

### `zxv-link` y `zxv-link-gui`

```sh
zxv-link ports
zxv-link --port PUERTO info | send FICHERO [DESTINO] | run FICHERO [MÁS...] | run --pc FICHERO | log
zxv-link --port PUERTO exec RUTA | ls [RUTA] | get RUTA [FICHERO] | rm RUTA | mv RUTA NUEVA | mkdir RUTA
zxv-link --port PUERTO serve [--folder CARPETA [--run PROGRAMA]] [--basic-debug PUERTO_TCP] [FICHERO...]
zxv-link --port PUERTO [--yes] update zxv-update.bin
zxv-link --port MÁQUINA pair
```

La carga remota: hablan con la máquina por la tira J5, con un adaptador
USB-serie de 3,3 V, o por la red si la Pico CPU es una Pico 2 W. Mandan
programas, los ejecutan, manejan los ficheros de la tarjeta, sirven una
carpeta del PC como la unidad `EXT:` y actualizan el firmware.
`zxv-link-gui` hace lo mismo con ventana. Cómo se usan, en [Cargar
programas desde el PC (J5)](../hardware/firmware.md#cargar-programas-desde-el-pc-j5)
y [por Wi-Fi](../hardware/firmware.md#cargar-programas-por-wi-fi-pico-2-w).

| Orden | Efecto |
|---|---|
| `ports` | Los puertos serie que hay y las ZX-V de la red que contestan, uno por línea en la salida estándar, como los acepta `--port`. Las que contestan con otra versión del protocolo de la red no van ahí (no se puede hablar con ellas): van a la salida de errores, tras «No compatibles:», con su firmware y qué actualizar, ella o `zxv-link` |
| `info` | La versión del firmware, de la especificación y del protocolo, y si la máquina espera una orden o tiene un programa en marcha |
| `send FICHERO [DESTINO]` | Manda el fichero a la tarjeta (con su nombre, o `DESTINO`) |
| `run FICHERO [MÁS...]` | Manda el fichero (y los que carga) a la tarjeta y lo ejecuta; si hay un programa en marcha, antes lo para |
| `run --pc FICHERO` | Lo ejecuta directo a la memoria, sin copiarlo a la tarjeta: la máquina lo lee del PC por `EXT:`. Si el programa lo deja abierto (un `.zxv` con bancos de código), lo sigue sirviendo hasta que acaba o Ctrl+C, y escribe en la salida estándar la línea `zxv-link: serving-ext` |
| `exec RUTA` | Ejecuta lo que ya está en la máquina: `SD:/juegos/a.zxv`, `ROM:/b.bas` o `EXT:/c.zxv` |
| `ls [RUTA]` | Lo que hay en una carpeta de la tarjeta (`SD:/` si no se dice) o de `ROM:` |
| `get RUTA [FICHERO]` | Copia un fichero de la tarjeta (o de `ROM:`) al PC |
| `rm RUTA` | Borra un fichero o una carpeta vacía |
| `mv RUTA NUEVA` | Renombra o mueve, en la misma unidad |
| `mkdir RUTA` | Crea una carpeta |
| `log` | El informe de la máquina, hasta Ctrl+C |
| `serve ...` | Sirve `CARPETA` como la unidad `EXT:` hasta Ctrl+C (con `--run`, antes ejecuta `PROGRAMA`, de dentro de la carpeta, desde `EXT:`), y/o el depurador del BASIC por TCP en `PUERTO_TCP` (lo usa la extensión de VS Code), después de mandar los `FICHERO`. Hace falta `--folder` o `--basic-debug`; `--run` solo con `--folder` y sin `--basic-debug` |
| `update FICHERO` | Actualiza el firmware de la máquina con un `zxv-update.bin`, sin tarjeta: la máquina lo lee del PC. Antes enseña las versiones y pregunta |
| `pair` | Empareja este PC con una ZX-V de la red (con EDIT > Wi-Fi > Emparejar un PC abierto en ella): los dos enseñan un código y, si coincide, se acepta en la máquina |

| Opción | Efecto |
|---|---|
| `--port PUERTO` | El adaptador (`COM5` en Windows, `/dev/ttyUSB0` en Linux) o una ZX-V de la red (su nombre, `ZX-V-1A2B`, o su IP). `zxv-link ports` dice cuáles hay. Hace falta en todas las órdenes menos `ports` |
| `--overwrite` | Sustituye también en la tarjeta lo que no puso `zxv-link`, y deja que la máquina sustituya o borre en la carpeta de `EXT:` lo que no escribió ella |
| `--yes` | `update` sin preguntar |
| `--lang es\|en` | El idioma de los mensajes (por defecto, el del sistema) |

Lo que ya está en la tarjeta igual (el mismo tamaño y el mismo CRC) no se
vuelve a mandar. Sale con el código 2 si la orden no vale (enseña la
ayuda), con el 3 si no puede abrir el puerto, con el 4 si tendría que
sustituir en la tarjeta algo que no puso él (sin `--overwrite`), y con el 1
si falla otra cosa.

### `zxv-gfx`

```sh
zxv-gfx [FICHERO.gfx]
```

El editor de gráficos para el PC: sprites, tiles, mapas, texturas y
pantallas en ficheros `.gfx`. Lo cuenta
[Gráficos](graphics.md#el-editor-de-gráficos-para-el-pc-zxv-gfx).
Con `--lang es` o `--lang en` habla ese idioma (si no, el del sistema);
`--help` y `--version`, lo de siempre. Para las pruebas,
`--screenshot CAPTURA.png [--section N]` abre el fichero, guarda la ventana
(con la pestaña N, de 0 a 5) y acaba.

### `zxv-tracker`

```sh
zxv-tracker [FICHERO.zxm | .pt3 | .mod | .s3m | .xm | .afb]
zxv-tracker --wav SALIDA.wav FICHERO
```

El tracker para el PC: canciones `.zxm` con los dos AY y hasta 12 pistas
de muestras, módulos PT3 y efectos AYFX (`.afb`); abre también MOD, S3M y
XM. Lo cuenta la [guía](README.md#el-tracker-para-el-pc-zxv-tracker).

| Opción | Efecto |
|---|---|
| `--wav SALIDA.wav` | Toca la canción una vez a un WAV, sin ventana, y acaba |
| `--channels LISTA` | De un MOD, un S3M o un XM de más de 12 canales, los que van (`1,2,5-9`, desde 1, hasta 12), sin preguntar |
| `--lang es\|en` | El idioma (por defecto, el del sistema) |
| `--screenshot CAPTURA.png [--tab N]` | Para las pruebas: abre, guarda la ventana (con la pestaña N, de 0 a 5) y acaba |

## La biblioteca estándar de C (picolibc)

Los programas compilados con picolibc y `zxvc.c` (ver la
[guía](README.md#dos-formas-de-escribir-en-c)) pueden usar:

| Qué | Funciones |
|---|---|
| Ficheros | `fopen`, `fread`, `fwrite`, `fgets`, `fprintf`, `fseek`, `fclose`..., y `open`, `read`, `write`, `lseek`, `close` |
| Carpetas | `opendir`, `readdir`, `rewinddir`, `closedir`, `mkdir`, `rmdir`, `chdir`, `getcwd` |
| Nombres | `rename`, `unlink`/`remove`, `stat`, `fstat` |
| Consola | `printf`, `puts`, `getchar`, `fgets(..., stdin)`: la pantalla y el teclado (con eco; Intro es `'\n'`) |
| Memoria | `malloc`, `free`, `realloc` |
| Hora | `time`, `gettimeofday`, `localtime`: el reloj de la máquina, la hora local (el 1 de enero de 2026 a las 0:00 si no está en hora) |
| Salida | `exit`, `abort`: vuelven al BASIC (`exit(0)` da `0 OK` y cualquier otro código, un informe `W`, como [`zxv_exit_to_basic`](#zxv_exit_to_basic)) |

- Los errores de la API llegan en `errno`:

  | Código de ZX-V | `errno` |
  |---|---|
  | `ZXV_E_NOENT` | `ENOENT` |
  | `ZXV_E_EXIST` | `EEXIST` |
  | `ZXV_E_NAME`, `ZXV_E_INVAL` | `EINVAL` |
  | `ZXV_E_NOTDIR` | `ENOTDIR` |
  | `ZXV_E_ISDIR` | `EISDIR` |
  | `ZXV_E_NOTEMPTY` | `ENOTEMPTY` |
  | `ZXV_E_BADF` | `EBADF` |
  | `ZXV_E_MFILE` | `EMFILE` |
  | `ZXV_E_ACCESS` | `EACCES` |
  | `ZXV_E_FAULT` | `EFAULT` |
  | `ZXV_E_NOCARD` | `ENODEV` |
  | `ZXV_E_NOSPC` | `ENOSPC` |
  | `ZXV_E_FORMAT` | `ENOEXEC` |
  | `ZXV_E_RANGE` | `ERANGE` |
  | Los demás (`ZXV_E_IO`, `ZXV_E_NOCLOCK`, `ZXV_E_OFFLINE`, `ZXV_E_CHANGED`) | `EIO` |

- Los descriptores 0, 1 y 2 son la consola; los de los ficheros son los de
  la API más 3.
- Carpetas abiertas a la vez, como en la API: `ZXV_MAX_DIRS` (4).
  `rewinddir` cierra la carpeta y la vuelve a abrir en el siguiente
  `readdir`.
- `struct dirent` tiene `d_name`, `d_type` (`DT_DIR` o `DT_REG`) y, de más,
  el tamaño en `d_size`.
- `printf` con decimales ocupa unos 7 KB más; con
  `-DPICOLIBC_INTEGER_PRINTF_SCANF` al enlazar se usa el pequeño, solo de
  enteros.
- La consola habla UTF-8, como las cadenas de C: `printf("¿Qué tal?")`
  enseña las tildes, y `fgets` da en UTF-8 lo que se teclea (la `ñ` son
  dos bytes). Las letras que no tiene la fuente de la ROM salen como `?`.
- El montón (`malloc`) va del final del programa a 32 KB por debajo del
  final de la RAM de usuario, donde está la pila.

## Definiciones internas

`zxv.h` es compartido con la ROM, el firmware y el emulador, así que define
también cosas que los programas no deben usar. Están aquí solo para que las
reconozcas; pueden cambiar sin aviso:

| Qué | Constantes |
|---|---|
| Las guardas de las cabeceras, y la que usa la ROM para tener los tipos de `zxv_gfx.h` sin las llamadas | `ZXV_H`, `ZXV_GFX_H`, `ZXV_GFXFILE_H`...; `ZXV_GFX_NO_API` |
| La interfaz de la ROM con la plataforma (el firmware o el emulador) | `ZXV_PLATFORM_IF` y todas las `ZXV_PIF_...`, también las versiones (`ZXV_PIF_VERSION_FS`, `_CLOCK`, `_LANG`, `_PREFS`, `_UPDATE`, `_WIFI`) |
| Actualizar el firmware desde la tarjeta (el menú EDIT) | `ZXV_UPDATE_FILE` (`"zxv-update.bin"`), `ZXV_UPDATE_INFO_SIZE` y `ZXV_UI_FILE`, `ZXV_UI_CPU`, `ZXV_UI_ULA` |
| El Wi-Fi de una Pico 2 W (el cuadro Wi-Fi del menú EDIT) | Todas las `ZXV_WIFI_...` (las órdenes, `ZXV_WIFI_INFO_SIZE`, `ZXV_WIFI_LIST_MAX`, `_ENTRY`, `_SIZE`), `ZXV_WI_...` (los campos de la información), `ZXV_WS_...` (los estados) y `ZXV_WP_...` (el emparejamiento) |
| El buzón del depurador del BASIC | `ZXV_DBG_BOX` y todas las `ZXV_DBG_...`, `ZXV_DBGC_...`, `ZXV_DBGS_...`, `ZXV_DBGR_...` y `ZXV_DBGO_...` |
| El espacio de trabajo de la ROM | `ZXV_ROM_WORKSPACE`, `ZXV_ROM_WORKSPACE_END` |
| Las direcciones de la flash | `ZXV_FLASH_BASE`, `ZXV_FLASH_SIZE`, `ZXV_PLATFORM_BASE`, `ZXV_ROM_SIZE` |
| Las de la SRAM | `ZXV_SRAM_BASE`, `ZXV_SRAM_SIZE`, `ZXV_RESERVED_END` |
| Las cuentas de dentro de `ZXV_RGB` y `ZXV_STD_RGB565` | `ZXV_TO5`, `ZXV_TO6`, `ZXV_STD_CHAN5` (usa las macros de fuera) |
