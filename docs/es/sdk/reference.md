[English](../../en/sdk/reference.md) · **Español**

# Referencia del SDK

Todo lo que trae el SDK, agrupado por temas: las funciones del sistema (la
API), el motor gráfico, los registros y las constantes de la máquina, el
mapa de memoria y los formatos de fichero. Para aprender a usarlo, lee la
[guía del SDK](README.md) y [Gráficos](graphics.md).

**Contenido**

- [Los ficheros del SDK](#los-ficheros-del-sdk)
- [Convenciones](#convenciones)
- [Sistema y tiempo](#sistema-y-tiempo)
- [Pantalla y texto](#pantalla-y-texto)
- [Teclado, mando y ratón](#teclado-mando-y-ratón)
- [Sonido](#sonido)
- [Ficheros y carpetas](#ficheros-y-carpetas)
- [Reloj e idioma](#reloj-e-idioma)
- [Terminar un programa y ejecutar otros](#terminar-un-programa-y-ejecutar-otros)
- [Motor gráfico](#motor-gráfico)
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

| Fichero | Contenido |
|---|---|
| `include/zxv_api.h` | La API como funciones `static inline` de C; el puntero del ratón (`zxv_mouse_t`); los tipos `zxv_dirent_t` y `zxv_clock_t`. Incluye `zxv.h`, `<stdint.h>` y `<stddef.h>` (así que `NULL` y `size_t` están disponibles) |
| `include/zxv_gfx.h` | El motor gráfico: tipos, constantes y funciones. Incluye `zxv_api.h` (define `ZXV_GFX_NO_API` para tener solo los tipos y las constantes) |
| `include/zxv.h` | Direcciones, registros y constantes, solo con `#define`, así que sirve también en ensamblador (ficheros `.S`) |
| `include/sys/dirent.h` | `struct dirent` y `DIR` para el `<dirent.h>` de picolibc |
| `crt0.S` | Código de arranque: pone el registro `tp` (las variables de hilo, que usa el `errno` de picolibc) y salta a `main` |
| `zxv.ld` | Script de enlazado: todo seguido al principio de la RAM de usuario; el montón para picolibc |
| `zxvlib.c` | `memcpy`, `memmove`, `memset`, `memcmp`, para los programas sin la biblioteca de C |
| `include/zxv_gfxfile.h`, `gfxfile.c` | Lee los ficheros de gráficos `.gfx` (de `zxv-gfx`, `gfxedit` y `png2gfx.py`): sprites, tiles, mapas y texturas, en los formatos de `zxv_gfx.h` |
| `zxvc.c` | La unión entre picolibc y la API (ficheros, carpetas, consola, montón, hora, salida) |
| `tools/elf2zxv.py` | Convierte el `.elf` enlazado en un `.zxv` |
| `tools/gfx/` | `zxv-gfx`, el editor de gráficos para el PC |
| `tools/tracker/` | `zxv-tracker`, el tracker para el PC: música PT3 y efectos AYFX |
| `tools/png2gfx.py` | Convierte imágenes PNG en un fichero de gráficos `.gfx` |
| `Makefile` | Compila los ejemplos (`make`, `make run SD=carpeta`, `make debug SD=carpeta`) |

## Convenciones

- Todas las funciones siguen la convención de llamada estándar de RISC-V
  `ilp32`; las funciones `inline` de las cabeceras llaman a la ROM a través
  de la [tabla de la API](#números-de-las-funciones-de-la-api).
- Las funciones que pueden fallar devuelven un `int`: **0 o más** si van
  bien y un [código de error](#códigos-de-error) **negativo** si fallan.
- «La página de dibujo» es la página de vídeo elegida con
  `zxv_gfx_draw_page` (la 0 al empezar un programa); «la página visible»
  es la elegida con `zxv_set_page`.
- Los registros son memoria: se leen y se escriben con punteros
  `volatile`, por ejemplo `*(volatile uint8_t *)(uintptr_t)ZXV_V_BORDER = 2;`.
  Los registros de vídeo se aplican en el siguiente VBLANK.

## Sistema y tiempo

### `zxv_wait_vblank`

```c
void zxv_wait_vblank(void);
```

Espera al siguiente VBLANK, el principio de un frame (60 por segundo).
Activa las interrupciones si estaban desactivadas. No la llames desde un
gancho de frame.

### `zxv_set_frame_hook`

```c
void zxv_set_frame_hook(void (*fn)(void));
```

Hace que el sistema llame a `fn` en cada VBLANK, con las interrupciones
desactivadas; `0` lo quita. La función debe ser breve y no puede llamar a
`zxv_wait_vblank`. Quítalo antes de que termine tu programa. Las variables
que comparta con el gancho deben ser `volatile`.

### `zxv_time_us`

```c
uint32_t zxv_time_us(void);
```

Un contador de microsegundos. Da la vuelta cada 2³² µs (unos 71 minutos):
compara tiempos restándolos como números sin signo.

```c
uint32_t t0 = zxv_time_us();
do_something();
uint32_t elapsed = zxv_time_us() - t0;
```

### `zxv_api_version`

```c
uint32_t zxv_api_version(void);
```

La versión de la especificación de la máquina, como `mayor << 16 | menor`
(ahora 1, la versión 0.1); lo mismo que `ZXV_SPEC_VERSION` y que la
variable del sistema `ZXV_SV_VERSION`.

### Constantes

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_FRAME_HZ` | 60 | Frames por segundo |
| `ZXV_CPU_HZ_REF` | 144000000 | Velocidad de referencia del procesador (Hz) |
| `ZXV_SPEC_VERSION` | 1 | Versión de la especificación (0.1) |

## Pantalla y texto

### `zxv_set_mode`

```c
void zxv_set_mode(int mode);
```

Cambia el modo de vídeo (0: 256 × 192; 1: 512 × 384) y borra la página de
dibujo con el color del papel del texto (ver
[`zxv_text_colors`](#zxv_text_colors-zxv_text_at)). Solo cuenta el bit 0 de
`mode`.

| Constante | Valor |
|---|---|
| `ZXV_MODE0_W`, `ZXV_MODE0_H` | 256, 192 |
| `ZXV_MODE1_W`, `ZXV_MODE1_H` | 512, 384 |

### `zxv_set_page`

```c
void zxv_set_page(int page);
```

Hace visible la página 0 o la 1 desde el siguiente VBLANK (escribe
`ZXV_V_PAGE`).

### `zxv_cls`

```c
void zxv_cls(int colour);
```

Llena la página de dibujo con un color (0–15) y lleva el cursor de texto
arriba a la izquierda.

### `zxv_plot`

```c
void zxv_plot(int x, int y, int colour);
```

Pinta un píxel de la página de dibujo. Los puntos de fuera de la página se
ignoran.

### `zxv_print_char`, `zxv_print_str`

```c
void zxv_print_char(int c);
void zxv_print_str(const char *s);
```

Escriben un carácter o una cadena terminada en cero en el cursor de texto
de la página de dibujo, en celdas de 8 × 8 píxeles, con los colores de
`zxv_text_colors`. `'\n'` y `'\r'` van al principio de la línea siguiente,
`'\b'` borra el carácter anterior, y la pantalla sube cuando el texto llega
abajo.

`zxv_print_char` recibe un código Unicode y `zxv_print_str`, una cadena en
UTF-8 (la codificación de los ficheros de C, así que puedes escribir
`"¡Hola!"` en tu programa). Dibujan los caracteres ASCII del 32 al 126, `©`
y las letras del español de la fuente de la ROM: `á é í ó ú ü ñ`, sus
mayúsculas `Á É Í Ó Ú Ü Ñ`, `¿` y `¡`. Cualquier otro carácter sale como `?`.

### `zxv_text_colors`, `zxv_text_at`

```c
void zxv_text_colors(int ink, int paper);
void zxv_text_at(int col, int row);
```

`zxv_text_colors` elige los colores (0–15) del texto que escriban desde
entonces `zxv_print_char` y `zxv_print_str`: cada celda se llena con
`paper` (el papel) y el carácter se dibuja con `ink` (la tinta). Cada
programa empieza con negro sobre blanco (0 y 7), deje lo que deje el
anterior.

`zxv_text_at` lleva el cursor de texto a la columna `col` y la fila `row`,
contadas desde 0 arriba a la izquierda: 32 × 24 celdas en el modo 0 y
64 × 48 en el modo 1. `zxv_cls` y `zxv_set_mode` también devuelven el cursor
arriba a la izquierda.

```c
zxv_text_colors(14, 1);                 /* amarillo brillante sobre azul */
zxv_text_at(8, 10);
zxv_print_str("¡Hola, señor Núñez!");
```

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

## Teclado, mando y ratón

### `zxv_key_get`

```c
int zxv_key_get(void);
```

Saca la siguiente tecla de la cola de teclas (64 entradas), o devuelve −1
si está vacía. Las teclas mantenidas se repiten (a los 500 ms, y luego 20
veces por segundo).

| Código | Tecla |
|---|---|
| El código Unicode del carácter | Una tecla que escribe un carácter (`'a'`, `'ñ'` = 0xF1...) |
| 0x0D, 0x08, 0x09, 0x1B | Intro, Retroceso, Tab, Esc |
| `ZXV_KEY_SPECIAL` (0xF000) \| código HID | Cualquier otra tecla: F1–F12, cursores, Inicio, Fin... |

Ctrl+Alt+Supr no llega nunca: vuelve al BASIC.

### `zxv_joy_read`

```c
uint32_t zxv_joy_read(int n);
```

El estado del mando `n` (0 o 1; el primero que se conecta es el 0). Un
mando desconectado da 0.

| Constante | Bit | Constante | Bit |
|---|---|---|---|
| `ZXV_JOY_RIGHT` | 0 | `ZXV_JOY_FIRE2` | 5 |
| `ZXV_JOY_LEFT` | 1 | `ZXV_JOY_FIRE3` | 6 |
| `ZXV_JOY_DOWN` | 2 | `ZXV_JOY_START` | 7 |
| `ZXV_JOY_UP` | 3 | `ZXV_JOY_SELECT` | 8 |
| `ZXV_JOY_FIRE1` | 4 | `ZXV_JOY_CONNECTED` | 15 |

La dirección sale de la cruceta o del stick izquierdo. En un mando de
PlayStation, ✕, ○ y □ son los fuegos 1, 2 y 3; Share/Create es Select y
Options, Start. En un mando genérico, los botones 1–3 son los fuegos, el 9
es Select y el 10, Start.

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

`zxv_mouse_init` pone el puntero en (x, y) y se queda con los contadores
del ratón. `zxv_mouse_update`, una vez por frame, suma el movimiento desde
la llamada anterior (una unidad del ratón es un píxel, sin aceleración) y
mantiene el puntero dentro de [0, w) × [0, h). Lee directamente los
[registros de entrada](#registros-de-entrada) `ZXV_I_MOUSE_...`.

## Sonido

### `zxv_ay_write`

```c
void zxv_ay_write(int reg, int val);
```

Escribe `val` en el registro `reg` de un AY: 0–15 para el primer chip,
`ZXV_AY2` + 0–15 (16–31) para el segundo. La escritura pasa por una cola
de 256 entradas y suena unos milisegundos después; si la cola está llena,
la función espera. Escribir el registro 13 reinicia la envolvente, como en
el chip de verdad.

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
empiezan en silencio.

### `zxv_beeper_event`

```c
void zxv_beeper_event(int level);
```

Pone el beeper a `level` (0, o cualquier otra cosa para 1) exactamente a la
hora actual más `ZXV_BEEP_LATENCY_US` (2000 µs). Los cambios pasan por una
cola de 256 entradas (compartida con `zxv_ay_event`); si está llena, la
función espera. Para sonidos sencillos puedes escribir en el registro
`ZXV_S_BEEPER`; las dos formas se suman.

### `zxv_ay_event`

```c
void zxv_ay_event(uint32_t t_us, int reg, int val);
```

Escribe `val` en el registro `reg` de un AY (0–31, como en `zxv_ay_write`)
a la hora `t_us` (en el reloj de `zxv_time_us`) más 2 ms. Las horas deben
ir de menor a mayor: un cambio espera a los que tiene delante. Si `t_us` ya
ha pasado, suena enseguida. Si la cola está llena, la función espera. Está
pensada para el sonido digitalizado: ver el ejemplo
[`digital`](../../../sdk/examples/digital/digital.c).

### `zxv_beeper_event_at`

```c
void zxv_beeper_event_at(uint32_t t_us, int level);
```

Como `zxv_beeper_event`, pero a la hora `t_us` (más 2 ms), con las mismas
reglas que `zxv_ay_event`: las horas de menor a mayor. Con la música de
`zxv_music.h` sonando, que va 35 ms por delante, el beeper tiene que ir
igual: `zxv_beeper_event_at(zxv_time_us() + ZXV_MUSIC_AHEAD_US, nivel)`.

### Constantes

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_AY2` | 16 | Se suma al número de registro para llegar al segundo AY |
| `ZXV_AY_FIFO_LEN` | 256 | Entradas de la cola del AY |
| `ZXV_BEEP_QUEUE_LEN` | 256 | Entradas de la cola con hora |
| `ZXV_BEEP_LATENCY_US` | 2000 | Retraso fijo de la cola con hora, en µs |
| `ZXV_AY_STEREO_MONO`, `_ABC`, `_ACB` | 0, 1, 2 | Valores de `ZXV_S_AY_STEREO` |
| `ZXV_EV_AY`, `ZXV_EV_AY_WRITE(reg, val)` | | El dato de una escritura en el AY en la cola con hora (con el bit 31); solo hace falta si llenas la cola tú mismo |

## Ficheros y carpetas

Las rutas pueden empezar por una unidad: `SD:` (la tarjeta SD) o `ROM:` (la
unidad de solo lectura de serie), en mayúsculas o minúsculas; `ROM:graphics`
es lo mismo que `ROM:/graficos`. Sin unidad, una ruta es relativa a la
carpeta actual, o absoluta si empieza por `/`. `.` y `..` funcionan, y `..`
en la raíz se queda en la raíz. Las mayúsculas dan igual. Los nombres
tienen hasta 63 caracteres ASCII imprimibles (sin `\ : * ? " < > |`, y sin
acabar en espacio ni en punto); las rutas, hasta 255.

Puede haber hasta 8 ficheros y 4 carpetas abiertos a la vez en cada
unidad. Los buffers tienen que estar en la RAM (o, para leer, en la ROM);
si no, la función devuelve `ZXV_E_FAULT`. En una tarjeta SD de verdad, un
fichero abierto no se puede borrar, renombrar ni volver a abrir para
escribir, y los ficheros ocultos o de sistema no salen en los listados.

### `zxv_file_open`

```c
int zxv_file_open(const char *name, uint32_t flags);
```

Abre un fichero y devuelve su número, o un error. `flags` combina:

| Indicador | Valor | Efecto |
|---|---|---|
| `ZXV_O_READ` | 0x01 | Leer |
| `ZXV_O_WRITE` | 0x02 | Escribir |
| `ZXV_O_CREATE` | 0x04 | Crearlo si no existe (necesita `ZXV_O_WRITE`) |
| `ZXV_O_TRUNC` | 0x08 | Vaciarlo al abrir |
| `ZXV_O_APPEND` | 0x10 | Empezar al final |

Abrir para escribir en `ROM:` devuelve `ZXV_E_ACCESS`.

### `zxv_file_read`, `zxv_file_write`

```c
int zxv_file_read(int f, void *buf, uint32_t n);
int zxv_file_write(int f, const void *buf, uint32_t n);
```

Leen o escriben hasta `n` bytes en la posición actual y devuelven
cuántos, o un error. `zxv_file_read` devuelve 0 al final del fichero. Si
lees código en memoria para ejecutarlo, ejecuta después una instrucción
`fence.i`.

### `zxv_file_seek`

```c
int zxv_file_seek(int f, int32_t off, int whence);
```

Lleva la posición a `off` bytes desde el principio (`ZXV_SEEK_SET`, 0),
la posición actual (`ZXV_SEEK_CUR`, 1) o el final (`ZXV_SEEK_END`, 2), y
devuelve la nueva posición.

### `zxv_file_close`

```c
int zxv_file_close(int f);
```

Cierra el fichero. Cierra siempre los ficheros en los que escribas: es
entonces cuando todo llega a la tarjeta.

### `zxv_file_load`

```c
int zxv_file_load(const char *name, void *addr, uint32_t max);
```

Lee un fichero entero en la memoria, en `addr`, y devuelve su tamaño. `max`
es el sitio que tienes allí: si el fichero ocupa más de `max` bytes, no se
carga nada y la función devuelve `ZXV_E_RANGE`. Pásale el tamaño de tu
buffer:

```c
static uint8_t nivel[8192];
int tam = zxv_file_load("nivel1.dat", nivel, sizeof(nivel));
if (tam < 0)
    zxv_report_error(tam, "No puedo cargar nivel1.dat");
```

Las versiones anteriores del SDK no tenían el argumento `max`; un programa
compilado con ellas hay que volver a compilarlo.

### `zxv_file_stat`

```c
int zxv_file_stat(const char *name, zxv_dirent_t *e);
```

Rellena `*e` con el nombre, el tamaño y los atributos de un fichero o una
carpeta.

```c
typedef struct {
    char name[64];              /* terminado en cero */
    uint32_t size;              /* en bytes */
    uint32_t attr;              /* ZXV_ATTR_DIR (bit 0): es una carpeta */
} zxv_dirent_t;
```

Los mismos 72 bytes, descritos para el ensamblador: `ZXV_DIRENT_SIZE` (72),
`ZXV_DE_NAME` (0), `ZXV_DE_SIZE` (64), `ZXV_DE_ATTR` (68).

### `zxv_file_remove`, `zxv_file_rename`

```c
int zxv_file_remove(const char *name);
int zxv_file_rename(const char *from, const char *to);
```

Borran un fichero o una carpeta **vacía**; cambian el nombre de un fichero
o una carpeta, o lo mueven.

### Carpetas

```c
int zxv_dir_make(const char *name);             /* crea una carpeta */
int zxv_dir_change(const char *name);           /* cambia la carpeta actual */
int zxv_dir_current(char *buf, uint32_t len);   /* la carpeta actual; devuelve su longitud */
int zxv_dir_open(const char *name);             /* abre una carpeta para listarla ("" = la actual) */
int zxv_dir_read(int d, zxv_dirent_t *e);       /* la siguiente entrada: 1, o 0 al final */
int zxv_dir_close(int d);
```

- `zxv_dir_change("ROM:")` entra en la unidad ROM y `zxv_dir_change("SD:")`
  vuelve a la raíz de la tarjeta.
- `zxv_dir_current` da `/...` en la tarjeta y `ROM:/...` en la unidad ROM;
  si `buf` es pequeño devuelve `ZXV_E_RANGE`.
- `zxv_dir_read` da las entradas en orden alfabético, sin distinguir
  mayúsculas.

### Constantes

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_NAME_MAX` | 63 | Caracteres por nombre |
| `ZXV_PATH_MAX` | 255 | Caracteres por ruta |
| `ZXV_MAX_FILES` | 8 | Ficheros abiertos a la vez (por unidad) |
| `ZXV_MAX_DIRS` | 4 | Carpetas abiertas a la vez (por unidad) |
| `ZXV_ATTR_DIR` | 0x01 | Atributo: carpeta |

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

`zxv_clock_get` devuelve 0, o `ZXV_E_NOCLOCK` si el reloj no está en hora
(y entonces no toca `*c`). `zxv_clock_set` devuelve 0, o `ZXV_E_INVAL` si
la fecha no existe o está fuera de 2000–2099; no mira `wday`. La hora es la
local, sin zonas horarias. Las dos son lentas: para medir tiempos, usa
`zxv_time_us` o `ZXV_SV_FRAMES`. Los ficheros que se crean en la tarjeta
llevan esta fecha.

Para el ensamblador: `ZXV_CLOCK_SIZE` (8) y los desplazamientos
`ZXV_CK_YEAR` (0), `ZXV_CK_MONTH` (2), `ZXV_CK_DAY` (3), `ZXV_CK_HOUR` (4),
`ZXV_CK_MIN` (5), `ZXV_CK_SEC` (6), `ZXV_CK_WDAY` (7).

### `zxv_lang_get`, `zxv_lang_set`

```c
uint32_t zxv_lang_get(void);
int zxv_lang_set(uint32_t lang);
```

El idioma de la máquina: un código ISO 639-1 de dos letras minúsculas, la
primera en el byte bajo, que se forma con `ZXV_LANG(a, b)`. `ZXV_LANG_EN`
es `ZXV_LANG('e', 'n')` y `ZXV_LANG_ES` es `ZXV_LANG('e', 's')`. La ROM
tiene sus textos en inglés y en español, pero cualquier código vale, así
que los programas pueden usarlo para elegir los suyos. `zxv_lang_set` lo
cambia (y la máquina lo recuerda); devuelve `ZXV_E_INVAL` si el código no
son dos letras minúsculas.

```c
const char *hello = zxv_lang_get() == ZXV_LANG_ES ? "Hola" : "Hello";
```

## Terminar un programa y ejecutar otros

### `zxv_exit_to_basic`

```c
void zxv_exit_to_basic(int code);
```

Termina el programa desde cualquier sitio y vuelve al BASIC, igual que
devolver `code` desde `main`. **El 0 significa que todo ha ido bien**: el
BASIC enseña `0 OK, 0:1`. Cualquier otro código es un error, y el BASIC
enseña `W Error en el código nativo (code), 0:1` (en inglés,
`W Native code error (code), 0:1`). En los dos casos el programa BASIC que
hubiera en la memoria ha desaparecido, porque el programa nativo ha usado
esa memoria.

El valor que devuelve `main` funciona igual. El código nativo al que se
llama desde el BASIC con `USR` es distinto: lo que devuelve es sin más el
valor de `USR`.

### `zxv_exec`

```c
int zxv_exec(const char *name, int *result);
```

Carga y ejecuta un `.zxv` (si `name` no tiene extensión y no existe, se le
añade `.zxv`), deja en `*result` lo que devolvió y devuelve 0 o un error
(`ZXV_E_FORMAT` si no es un programa válido, `ZXV_E_RANGE` si no cabe).

**Un programa nativo no puede usarla**: llamada desde un `.zxv`, devuelve
`ZXV_E_ACCESS` (−9) y no hace nada. Todos los programas se cargan en la
misma dirección, al principio de la RAM de usuario, así que el nuevo pisaría
al que lo llama. Solo el BASIC (con `LOAD`) y el monitor del sistema lanzan
programas.

### `zxv_report_error`

```c
void zxv_report_error(int code, const char *msg);
```

Termina el programa y vuelve al BASIC con el informe `W` y tu propio
mensaje en vez del del BASIC: `msg` es una cadena en UTF-8, de la que el
BASIC enseña hasta 47 bytes. Si `code` no es 0, va detrás entre paréntesis.
Por ejemplo, `zxv_report_error(5, "Fichero dañado")` da
`W Fichero dañado (5), 0:1`, y `zxv_report_error(0, "Faltan los niveles")`
da `W Faltan los niveles, 0:1`. El mensaje se copia antes de que el BASIC
recupere la memoria, así que puede estar en una matriz local. Un mensaje
vacío deja el texto de siempre del BASIC
(`W Error en el código nativo (5), 0:1`).

La fuente de la pantalla del BASIC tiene las letras del español, así que el
mensaje puede llevar `ñ`, `¿` o `á`; los demás caracteres que no son ASCII
salen como `?`.

## Motor gráfico

Declarado en `zxv_gfx.h`. Ver [Gráficos](graphics.md) para las
explicaciones y los ejemplos. Todas las funciones dibujan en el destino
actual y recortan contra él y contra el rectángulo de recorte.

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
    uint8_t wrap;           /* 1: se repite; 0: fuera es del color 'out' */
    uint8_t out;
    uint8_t reserved;
} zxv_m7tex;

typedef struct { int32_t u, v, du, dv; } zxv_m7line;   /* coma fija 16.16 */

typedef struct {            /* una cámara del modo 7, coma fija 16.16 */
    int32_t x, y;           /* posición sobre la textura */
    int32_t height;         /* altura sobre el suelo */
    int32_t angle;          /* 0x10000 = una vuelta; 0 mira hacia u creciente */
    int32_t focal;          /* distancia focal, en píxeles */
    int16_t horizon;        /* línea de la pantalla del horizonte (entera) */
    int16_t reserved;
} zxv_m7cam;
```

Los píxeles son de 4 bits, dos por byte, el par en los 4 bits bajos. Los
bitmaps, los tiles y los sprites preparados tienen que estar alineados a 4
bytes.

### Constantes

| Constante | Valor | Significado |
|---|---|---|
| `ZXV_TRANSPARENT` | 8 | El color transparente |
| `ZXV_FLIP_H`, `ZXV_FLIP_V` | 0x01, 0x02 | Espejo horizontal, vertical (copias y sprites) |
| `ZXV_BLIT_TRANSPARENT` | 0x04 | `zxv_gfx_blit`: no copiar el color 8 |
| `ZXV_MAP_WRAP` | 0x01 | `zxv_tilemap.flags`: el mapa se repite |
| `ZXV_CELL_TILE` | 0x0FFF | Celda: número de tile |
| `ZXV_CELL_FLIP_H`, `ZXV_CELL_FLIP_V` | 0x1000, 0x2000 | Celda: con espejo |
| `ZXV_CELL_TRANSP` | 0x4000 | Celda: el color 8 es transparente |
| `ZXV_CELL_PRIO` | 0x8000 | Celda: prioridad |
| `ZXV_MAP_ALL`, `ZXV_MAP_BACK`, `ZXV_MAP_FRONT` | 0, 1, 2 | Filtro de `zxv_map_draw`: todas las celdas, sin prioridad, con prioridad |

### Destino y recorte

```c
void zxv_gfx_target(const zxv_bitmap *b);
void zxv_gfx_draw_page(int page);
void zxv_gfx_clip(int x0, int y0, int x1, int y1);
```

- `zxv_gfx_target(b)` hace destino al bitmap `b`; `0` vuelve a la página
  de dibujo. Quita el rectángulo de recorte. La estructura del bitmap se
  copia, así que puede ser una variable local.
- `zxv_gfx_draw_page(página)` elige la página de dibujo (0 o 1), también
  para `zxv_cls`, `zxv_plot` y el texto. Su tamaño sigue al modo de vídeo y
  a `ZXV_V_VSIZE`.
- `zxv_gfx_clip` limita el dibujo al rectángulo de (x0, y0) a (x1, y1), sin
  incluir x1 ni y1, en coordenadas del destino.

Al empezar un programa, el destino es la página de dibujo 0, sin
rectángulo de recorte.

### Figuras

```c
void zxv_gfx_line(int x0, int y0, int x1, int y1, int c);
void zxv_gfx_rect(int x, int y, int w, int h, int c);
void zxv_gfx_tri(const int16_t *p, int c);
void zxv_gfx_poly(const int16_t *p, int n, int c);
void zxv_gfx_circle(int x, int y, int r, int c, int fill);
```

- `zxv_gfx_line`: una línea con sus dos extremos.
- `zxv_gfx_rect`: un rectángulo relleno de w × h con la esquina de arriba a
  la izquierda en (x, y). Nada si w o h valen 0 o menos.
- `zxv_gfx_tri`: un triángulo relleno; `p` tiene 3 pares x, y.
- `zxv_gfx_poly`: un polígono relleno de `n` vértices (de 3 a 64) dados
  como pares x, y, con la regla par-impar (una estrella de cinco puntas
  dibujada de un trazo tiene un hueco en el centro).
- `zxv_gfx_circle`: un círculo de radio `r` con centro en (x, y): solo el
  contorno si `fill` es 0, y un disco si no.

Los triángulos y los polígonos siguen una regla de arriba a la izquierda:
dos que compartan un lado ni se solapan ni dejan huecos.

### Copias

```c
void zxv_gfx_blit(const zxv_bitmap *src, int sx, int sy, int w, int h, int x, int y, int flags);
```

Copia el rectángulo (sx, sy, w, h) de `src` en (x, y) del destino, hasta
1024 píxeles de ancho. `flags`: `ZXV_FLIP_H`, `ZXV_FLIP_V`,
`ZXV_BLIT_TRANSPARENT`. La parte del rectángulo que cae fuera de `src` no
se copia. Copiar un bitmap sobre sí mismo con zonas que se solapan no está
definido.

### Sprites

```c
uint32_t zxv_spr_prepare(const zxv_bitmap *src, int sx, int sy, int w, int h, void *buf);
void zxv_spr_draw(const void *spr, int x, int y, int flags, const uint8_t *colours);
void zxv_spr_draw_scaled(const void *spr, int x, int y, int w, int h, int flags, const uint8_t *colours);
int zxv_spr_collide(const void *a, int ax, int ay, const void *b, int bx, int by);
```

- `zxv_spr_prepare` convierte el rectángulo (sx, sy, w, h) de `src` (hasta
  1024 × 1024; lo que cae fuera de `src` es transparente) en un sprite en
  `buf`, que tiene que estar alineado a 4 bytes. Devuelve el tamaño en
  bytes, 8 + h × ⌈w / 8⌉ × 8; con `buf` = `NULL` solo devuelve el tamaño.
  Devuelve 0 si el tamaño no vale.
- `zxv_spr_draw` dibuja un sprite con la esquina de arriba a la izquierda
  en (x, y). `flags`: `ZXV_FLIP_H`, `ZXV_FLIP_V`. `colours`: `NULL`, o una
  tabla de 16 bytes; el color *c* se dibuja como `colours[c]` (los píxeles
  transparentes siguen siéndolo).
- `zxv_spr_draw_scaled` lo dibuja estirado o encogido a w × h (de 1 a 4096
  cada uno; si no, nada), tomando para cada píxel el más cercano del
  sprite. A su tamaño es lo mismo que `zxv_spr_draw`. Usa unos 2,5 KB de
  pila.
- `zxv_spr_collide` devuelve 1 si algún píxel opaco de `a` en (ax, ay)
  coincide con un píxel opaco de `b` en (bx, by), a su tamaño.

### Mapas de tiles

```c
void zxv_map_draw(const zxv_tilemap *m, int mx, int my, int x, int y, int w, int h, int filter);
uint16_t zxv_map_cell(const zxv_tilemap *m, int px, int py);
```

- `zxv_map_draw` dibuja, en el rectángulo (x, y, w, h) del destino, la
  parte del mapa que empieza en el píxel (mx, my) del mapa. `filter`:
  `ZXV_MAP_ALL`, `ZXV_MAP_BACK`, `ZXV_MAP_FRONT`. Las celdas de fuera de un
  mapa sin `ZXV_MAP_WRAP` no se dibujan. Una sola llamada puede cubrir hasta
  1024 píxeles de ancho.
- `zxv_map_cell` devuelve la celda del píxel (px, py) del mapa (0 fuera de
  un mapa que no se repite).

### Modo 7

```c
void zxv_m7_draw(const zxv_m7tex *t, const zxv_m7line *l, int y, int n);
void zxv_m7_camera(zxv_m7line *l, int n, const zxv_m7cam *c);
```

- `zxv_m7_draw` dibuja `n` líneas desde la línea `y` del destino; la línea
  `y + i` usa `l[i]`. El píxel x de una línea enseña el texel
  (u + x·du, v + x·dv), con la parte entera de cada coordenada.
- `zxv_m7_camera` rellena `l[0..n-1]` para las líneas 0 a n−1 del destino:
  la línea *i* por debajo del horizonte está a la distancia
  altura × focal / (i − horizonte), con el centro del destino en la x de la
  cámara. Las líneas del horizonte para arriba quedan a cero. Se dibuja con
  `zxv_m7_draw(t, l + y0, y0, n)`.

## Registros de vídeo

Escríbelos cuando quieras; el sistema los aplica en el siguiente VBLANK.

| Registro | Tamaño | Contenido |
|---|---|---|
| `ZXV_V_MODE` | 8 bits | Modo de vídeo, 0 o 1 (usa `zxv_set_mode`) |
| `ZXV_V_PAGE` | 8 bits | Página visible, 0 o 1 (usa `zxv_set_page`) |
| `ZXV_V_BORDER` | 8 bits | Color del borde, 0–15 |
| `ZXV_V_FLAGS` | 8 bits | `ZXV_VFLAG_LINETAB` (0x01): usar la tabla por línea; `ZXV_VFLAG_BLANK` (0x02): enseñar solo el borde |
| `ZXV_V_SCROLL_X` | 16 bits | Scroll horizontal de la imagen, circular |
| `ZXV_V_SCROLL_Y` | 16 bits | Scroll vertical, circular |
| `ZXV_V_REMAP` | 8 bits | Tabla de remapeo de colores en uso, 0–3 |
| `ZXV_V_VSIZE` | 8 bits | Tamaño de la página en el modo 0 (ver abajo) |
| `ZXV_V_OUT_INFO` | 32 bits | Solo lectura: bits 0–3, la salida (`ZXV_OUT_DVI` 1, sin sonido; `ZXV_OUT_HDMI` 2, con sonido); bits 8–15, la frecuencia de refresco (siempre 60 Hz); bits 16–17, el monitor (`ZXV_OUT_SCREEN_ON` conectado, `ZXV_OUT_SCREEN_NONE` desconectado, `ZXV_OUT_SCREEN_UNKNOWN` no se sabe: en una protoboard, o hasta que se conecta uno). La salida la elige el usuario, no los programas |

Con scroll, el píxel (x, y) de la pantalla enseña el píxel
((x + SCROLL_X) mod ancho, (y + SCROLL_Y) mod alto) de la página.

**Tamaños de página** (`ZXV_V_VSIZE`, solo en el modo 0; en el modo 1 se
ignora):

| Constante | Valor | Página | Bytes por línea |
|---|---|---|---|
| `ZXV_VSIZE_SCREEN` | 0 | 256 × 192 | 128 |
| `ZXV_VSIZE_2X2` | 1 | 512 × 384 | 256 |
| `ZXV_VSIZE_WIDE` | 2 | 1024 × 192 | 512 |
| `ZXV_VSIZE_TALL` | 3 | 256 × 768 | 128 |

**Tablas de remapeo:** cuatro tablas de 16 bytes desde `ZXV_REMAP_TABLES`
(la tabla *t* en `ZXV_REMAP_TABLES + 16·t`). La entrada *v* es el color que
se ve para el valor de píxel *v*. Empiezan siendo la identidad.

**Tabla por línea:** en `ZXV_LINE_TABLE`, una entrada de
`ZXV_LINE_ENTRY_SIZE` (8) bytes por línea del cuadro entero, borde
incluido: 240 líneas en el modo 0 (la imagen empieza en la línea 24) y 480
en el modo 1 (la imagen, desde la línea 48): `ZXV_FRAME_LINES(modo)` y
`ZXV_BORDER_LINES(modo)`. El cuadro es de `ZXV_FRAME_W` × `ZXV_FRAME_H`
(640 × 480), con un borde de `ZXV_BORDER_X` (64) píxeles a cada lado y
`ZXV_BORDER_Y` (48) líneas arriba y abajo.

| Desplazamiento | Tamaño | Campo | Valor «como siempre» |
|---|---|---|---|
| `ZXV_LT_BORDER` (0) | 8 bits | Color del borde | `0xFF` |
| `ZXV_LT_REMAP` (1) | 8 bits | Tabla de remapeo | `0xFF` |
| `ZXV_LT_SCROLL_X` (2) | 16 bits | Scroll horizontal (de 0 al ancho de la página − 1) | `0xFFFF` |
| `ZXV_LT_SRC_LINE` (4) | 16 bits | Línea de la página que se enseña (de 0 al alto de la página − 1) | `0xFFFF` (= (y + SCROLL_Y) mod alto) |
| 6 | 16 bits | Reservado, escribe 0 | |

En las líneas del borde solo cuenta el campo del borde.

**Cuándo es seguro cambiar las cosas:** justo después de cada VBLANK el
sistema lee la página visible, los registros y las tablas. Mientras el bit
`ZXV_SYSF_LINK_BUSY` de `ZXV_SV_SYSFLAGS` valga 1 (hasta 1,5 ms en el modo
0 y 6 ms en el modo 1 o con una página grande), no escribas en la página
visible ni en la tabla por línea.

**Memoria de vídeo:** `ZXV_VRAM0_BASE` (`0x2000_4000`) y `ZXV_VRAM1_BASE`
(`0x2001_C000`), de `ZXV_VRAM_PAGE_SIZE` (96 KB) cada una.

## Registros de entrada

| Registro | Tamaño | Contenido |
|---|---|---|
| `ZXV_I_KEYROWS` | 8 × 8 bits | La matriz del teclado del Spectrum por semifilas; bits 0–4, 0 = pulsada |
| `ZXV_I_KEY_HEAD`, `ZXV_I_KEY_TAIL` | 8 bits cada uno | Posiciones de escritura y lectura de la cola de teclas (usa `zxv_key_get`) |
| `ZXV_I_KEY_FIFO` | 64 × 16 bits | La cola de teclas (`ZXV_KEY_FIFO_LEN` = 64) |
| `ZXV_I_KEYMAP` | 32 bytes | 256 bits, uno por código HID de tecla; 1 = pulsada |
| `ZXV_I_KEY_MODS` | 8 bits | Modificadores: bits 0–3 Ctrl, Mayús, Alt y GUI izquierdas; bits 4–7 los mismos a la derecha |
| `ZXV_I_JOY0`, `ZXV_I_JOY1` | 16 bits cada uno | Los mandos (como `zxv_joy_read`) |
| `ZXV_I_MOUSE_X`, `ZXV_I_MOUSE_Y` | 16 bits cada uno | Contadores del movimiento del ratón, circulares (x a la derecha, y hacia abajo) |
| `ZXV_I_MOUSE_BTN` | 8 bits | Bits 0–2: botón izquierdo, derecho y central |
| `ZXV_I_MOUSE_WHEEL` | 8 bits | Contador de la rueda (positivo hacia arriba) |

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

| Registro | Tamaño | Lo escribe | Contenido |
|---|---|---|---|
| `ZXV_S_BEEPER` | 8 bits | el programa | Bit 0: nivel del beeper |
| `ZXV_S_AY_STEREO` | 8 bits | el programa | 0 mono, 1 ABC, 2 ACB (los dos chips) |
| `ZXV_S_VOLUME` | 8 bits | el programa | Volumen general 0–255 (200 al arrancar) |
| `ZXV_S_AFLAGS` | 8 bits | el programa | Bit 0: silencio |
| `ZXV_S_AY_HEAD`, `ZXV_S_AY_TAIL` | 16 bits cada uno | el programa, el sistema | Posiciones de la cola del AY (`ZXV_AY_FIFO`) |
| `ZXV_S_BEEP_HEAD`, `ZXV_S_BEEP_TAIL` | 16 bits cada uno | el programa, el sistema | Posiciones de la cola con hora (`ZXV_BEEP_QUEUE`) |
| `ZXV_S_AY_REGS` | 16 × 8 bits | el sistema | Copia de solo lectura de los registros del primer AY, ya aplicados |
| `ZXV_S_AY2_REGS` | 16 × 8 bits | el sistema | Lo mismo, del segundo AY |

Las colas son más fáciles de usar con `zxv_ay_write`, `zxv_beeper_event` y
`zxv_ay_event`.

La mezcla estéreo: en ABC, izquierda = A + la mitad de B y derecha = C + la
mitad de B; en ACB, izquierda = A + la mitad de C y derecha = B + la mitad
de C; en mono, los tres en los dos lados. El beeper va a los dos lados. Los
canales de la misma letra de los dos chips se suman.

## Variables del sistema

En `ZXV_SYSVARS` (`0x2000_0700`), de 32 bits cada una:

| Variable | Contenido |
|---|---|
| `ZXV_SV_FRAMES` | Contador de frames, +1 en cada VBLANK (no con las interrupciones desactivadas) |
| `ZXV_SV_VERSION` | Versión de la especificación (`mayor << 16 \| menor`) |
| `ZXV_SV_FRAME_HOOK` | El gancho de frame actual (cámbialo con `zxv_set_frame_hook`) |
| `ZXV_SV_RAMTOP` | Tope de la RAM usable; aquí empieza la pila del sistema |
| `ZXV_SV_URAM_START`, `ZXV_SV_URAM_END` | Principio y fin (excluido) de la RAM de usuario |
| `ZXV_SV_CPU_HZ` | Frecuencia del procesador |
| `ZXV_SV_SYSFLAGS` | `ZXV_SYSF_LINK_BUSY` (bit 0): se está enviando la página visible; `ZXV_SYSF_BREAK` (bit 1): la tecla BREAK está pulsada |
| `ZXV_SV_LANG` | El idioma de la máquina (solo lectura: usa `zxv_lang_set`) |

## Códigos de error

| Código | Constante | Significado |
|---|---|---|
| 0 | `ZXV_OK` | Todo bien |
| −1 | `ZXV_E_NOENT` | No existe |
| −2 | `ZXV_E_EXIST` | Ya existe |
| −3 | `ZXV_E_NAME` | Nombre o ruta no válidos |
| −4 | `ZXV_E_NOTDIR` | Una parte de la ruta no es una carpeta |
| −5 | `ZXV_E_ISDIR` | Es una carpeta |
| −6 | `ZXV_E_NOTEMPTY` | La carpeta no está vacía |
| −7 | `ZXV_E_BADF` | Número de fichero o carpeta no válido |
| −8 | `ZXV_E_MFILE` | Demasiados abiertos |
| −9 | `ZXV_E_ACCESS` | No permitido (por el modo de apertura, en `ROM:`, al borrar la raíz, o `zxv_exec` desde un programa nativo) |
| −10 | `ZXV_E_FAULT` | El buffer está fuera de la memoria permitida |
| −11 | `ZXV_E_IO` | Error de lectura o escritura |
| −12 | `ZXV_E_NOCARD` | No hay tarjeta SD |
| −13 | `ZXV_E_NOSPC` | La tarjeta está llena |
| −14 | `ZXV_E_FORMAT` | No es un `.zxv` válido |
| −15 | `ZXV_E_RANGE` | No cabe (en la RAM de usuario, o en el buffer: `zxv_file_load`, `zxv_dir_current`) |
| −16 | `ZXV_E_NOCLOCK` | El reloj no está en hora |
| −17 | `ZXV_E_INVAL` | Valor no válido |

Si se saca la tarjeta, o deja de responder, con ficheros o carpetas
abiertos, estos conservan su número (un fichero que se abra después no se
queda con ninguno de ellos) y todas las llamadas sobre ellos dan
`ZXV_E_NOCARD` o `ZXV_E_IO` hasta que los cierras. Cerrarlos da `ZXV_OK`:
lo que ya se había escrito está en la tarjeta.

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

| n | Constante | Función |
|---|---|---|
| 0 | `ZXV_API_VERSION` | `zxv_api_version` |
| 1 | `ZXV_API_WAIT_VBLANK` | `zxv_wait_vblank` |
| 2 | `ZXV_API_SET_FRAME_HOOK` | `zxv_set_frame_hook` |
| 3 | `ZXV_API_TIME_US` | `zxv_time_us` |
| 4 | `ZXV_API_SET_MODE` | `zxv_set_mode` |
| 5 | `ZXV_API_SET_PAGE` | `zxv_set_page` |
| 6 | `ZXV_API_CLS` | `zxv_cls` |
| 7 | `ZXV_API_PLOT` | `zxv_plot` |
| 8 | `ZXV_API_PRINT_CHAR` | `zxv_print_char` |
| 9 | `ZXV_API_PRINT_STR` | `zxv_print_str` |
| 10 | `ZXV_API_KEY_GET` | `zxv_key_get` |
| 11 | `ZXV_API_JOY_READ` | `zxv_joy_read` |
| 12 | `ZXV_API_AY_WRITE` | `zxv_ay_write` |
| 13 | `ZXV_API_BEEPER_EVENT` | `zxv_beeper_event` |
| 14–17 | `ZXV_API_FILE_OPEN`, `_READ`, `_WRITE`, `_CLOSE` | `zxv_file_open`, `zxv_file_read`, `zxv_file_write`, `zxv_file_close` |
| 18 | `ZXV_API_FILE_LOAD` | `zxv_file_load` |
| 19 | `ZXV_API_EXIT_TO_BASIC` | `zxv_exit_to_basic` |
| 20 | `ZXV_API_REPORT_ERROR` | `zxv_report_error` |
| 21–24 | `ZXV_API_FILE_SEEK`, `_STAT`, `_REMOVE`, `_RENAME` | `zxv_file_seek`, `zxv_file_stat`, `zxv_file_remove`, `zxv_file_rename` |
| 25–30 | `ZXV_API_DIR_MAKE`, `_CHANGE`, `_CURRENT`, `_OPEN`, `_READ`, `_CLOSE` | `zxv_dir_make` ... `zxv_dir_close` |
| 31 | `ZXV_API_ZXV_EXEC` | `zxv_exec` |
| 32 | `ZXV_API_GFX_TARGET` | `zxv_gfx_target` |
| 33 | `ZXV_API_GFX_DRAW_PAGE` | `zxv_gfx_draw_page` |
| 34 | `ZXV_API_GFX_CLIP` | `zxv_gfx_clip` |
| 35 | `ZXV_API_GFX_BLIT` | `zxv_gfx_blit` |
| 36 | `ZXV_API_SPR_PREPARE` | `zxv_spr_prepare` |
| 37 | `ZXV_API_SPR_DRAW` | `zxv_spr_draw` |
| 38 | `ZXV_API_SPR_COLLIDE` | `zxv_spr_collide` |
| 39 | `ZXV_API_MAP_DRAW` | `zxv_map_draw` |
| 40 | `ZXV_API_MAP_CELL` | `zxv_map_cell` |
| 41 | `ZXV_API_M7_DRAW` | `zxv_m7_draw` |
| 42 | `ZXV_API_M7_CAMERA` | `zxv_m7_camera` |
| 43 | `ZXV_API_GFX_LINE` | `zxv_gfx_line` |
| 44 | `ZXV_API_GFX_RECT` | `zxv_gfx_rect` |
| 45 | `ZXV_API_GFX_TRI` | `zxv_gfx_tri` |
| 46 | `ZXV_API_GFX_POLY` | `zxv_gfx_poly` |
| 47 | `ZXV_API_GFX_CIRCLE` | `zxv_gfx_circle` |
| 48 | `ZXV_API_CLOCK_GET` | `zxv_clock_get` |
| 49 | `ZXV_API_CLOCK_SET` | `zxv_clock_set` |
| 50 | `ZXV_API_LANG_SET` | `zxv_lang_set` |
| 51 | `ZXV_API_AY_EVENT` | `zxv_ay_event` |
| 52 | `ZXV_API_SPR_DRAW_SCALED` | `zxv_spr_draw_scaled` |
| 53 | `ZXV_API_TEXT_COLORS` | `zxv_text_colors` |
| 54 | `ZXV_API_TEXT_AT` | `zxv_text_at` |
| 55 | `ZXV_API_BEEPER_EVENT_AT` | `zxv_beeper_event_at` |

`zxv_lang_get` y las funciones del ratón leen la memoria directamente y no
tienen número. Las entradas sin usar devuelven −1. Como la tabla está en la
RAM, un programa puede cambiar una entrada para interceptar una rutina.

## Mapa de memoria

| Desde | Hasta | Tamaño | Contenido | Acceso |
|---|---|---|---|---|
| `0x1000_0000` | `0x1003_FFFF` | 256 KB | Firmware de la plataforma | Ninguno |
| `0x1004_0000` | `0x100B_FFFF` | 512 KB | La ROM (`ZXV_ROM_BASE`): el BASIC y la API | Lectura, ejecución |
| `0x100C_0000` | `0x103F_FFFF` | 3,25 MB | La unidad `ROM:` (`ZXV_ROMDISK_BASE`) | Con las funciones de ficheros |
| `0x2000_0000` | `0x2000_3FFF` | 16 KB | Área de sistema (`ZXV_SYS_BASE`) | Ver abajo |
| `0x2000_4000` | `0x2001_BFFF` | 96 KB | Página de vídeo 0 (`ZXV_VRAM0_BASE`) | Lectura, escritura |
| `0x2001_C000` | `0x2003_3FFF` | 96 KB | Página de vídeo 1 (`ZXV_VRAM1_BASE`) | Lectura, escritura |
| `0x2003_4000` | `0x2007_1FFF` | 248 KB | RAM de usuario (de `ZXV_URAM_BASE` a `ZXV_URAM_END`) | Lectura, escritura, ejecución |
| `0x2007_2000` | `0x2008_1FFF` | 64 KB | Reservada para el sistema (`ZXV_RESERVED_BASE`) | **Prohibido** |

**Área de sistema:**

| Dirección | Constante | Contenido |
|---|---|---|
| `0x2000_0000` | `ZXV_API_TABLE` | Tabla de la API (256 direcciones) |
| `0x2000_0400` | `ZXV_VIDEO_REGS` | [Registros de vídeo](#registros-de-vídeo) |
| `0x2000_0500` | `ZXV_INPUT_REGS` | [Registros de entrada](#registros-de-entrada) |
| `0x2000_0600` | `ZXV_SOUND_REGS` | [Registros de sonido](#registros-de-sonido) |
| `0x2000_0700` | `ZXV_SYSVARS` | [Variables del sistema](#variables-del-sistema) |
| `0x2000_0800` | `ZXV_AY_FIFO` | Cola del AY (256 × 2 bytes) |
| `0x2000_0A00` | `ZXV_BEEP_QUEUE` | Cola con hora (256 × 8 bytes) |
| `0x2000_1200` | `ZXV_REMAP_TABLES` | Tablas de remapeo de colores (4 × 16 bytes) |
| `0x2000_1800` | `ZXV_LINE_TABLE` | Tabla por línea (480 × 8 bytes) |

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

La pila es la del sistema: empieza cerca de `ZXV_URAM_END` y crece hacia
abajo.

## El formato `.zxv`

Una cabecera de 64 bytes seguida de la imagen del programa, que se copia
en la dirección de carga. Todos los números van con el byte bajo primero.

| Desplazamiento | Constante | Tamaño | Campo |
|---|---|---|---|
| 0x00 | `ZXV_H_MAGIC` | 4 bytes | `ZXV1` (`ZXV_HDR_MAGIC`) |
| 0x04 | `ZXV_H_HDR_SIZE` | 16 bits | Tamaño de la cabecera: 64 (`ZXV_HDR_SIZE`) |
| 0x06 | `ZXV_H_PROFILE` | 16 bits | Perfil de la especificación: 1 (`ZXV_HDR_PROFILE`) |
| 0x08 | `ZXV_H_LOAD` | 32 bits | Dirección de carga |
| 0x0C | `ZXV_H_ENTRY` | 32 bits | Punto de entrada |
| 0x10 | `ZXV_H_IMAGE_SIZE` | 32 bits | Tamaño de la imagen |
| 0x14 | `ZXV_H_BSS_SIZE` | 32 bits | Bytes que se ponen a cero detrás de la imagen |
| 0x18 | `ZXV_H_STACK` | 32 bits | Tope de la pila (0 = seguir con la del sistema) |
| 0x1C | `ZXV_H_MODE` | 8 bits | Modo de vídeo inicial |
| 0x1D | `ZXV_H_FLAGS` | 8 bits | Reservado: se escribe a 0 y se ignora al cargar |
| 0x1E | | 16 bits | Reservado |
| 0x20 | `ZXV_H_CRC` | 32 bits | CRC-32 de la imagen (el de zlib) |
| 0x24 | `ZXV_H_NAME` | 28 bytes | Nombre (ASCII, terminado en cero) |

Al cargarlo, el sistema comprueba la firma, el tamaño de la cabecera, el
perfil, que la dirección de carga esté alineada a 4 y el punto de entrada
dentro de la imagen, que la imagen y el BSS quepan en la RAM de usuario
dejando 4 KB por debajo de la pila del sistema, y el CRC. Después borra el
BSS, pone el modo de vídeo y llama al punto de entrada como
`int entry(void)`. Un tope de pila distinto de 0 tiene que estar alineado a
16 y quedar entre el final del BSS y `ZXV_URAM_END`.

## Herramientas

### `elf2zxv.py`

```sh
python3 sdk/tools/elf2zxv.py ENTRADA.elf SALIDA.zxv [--name NOMBRE] [--mode 0|1] [--stack DIRECCIÓN]
```

| Opción | Efecto |
|---|---|
| `--name NOMBRE` | Nombre en la cabecera (hasta 27 caracteres; por defecto, el del fichero de salida sin extensión) |
| `--mode 0\|1` | Modo de vídeo inicial (el 0 por defecto) |
| `--stack DIRECCIÓN` | Tope de la pila (en decimal o `0x...`; por defecto 0, la del sistema) |

Comprueba que el programa está dentro de la RAM de usuario y escribe la
dirección de carga, el punto de entrada y los tamaños. Como `png2gfx.py`,
habla en español si tu sistema está en español, y si no, en inglés.

### `png2gfx.py`

```sh
python3 sdk/tools/png2gfx.py SALIDA.gfx ORDEN...
python3 sdk/tools/png2gfx.py --list FICHERO.gfx
```

Las órdenes están en [Gráficos](graphics.md#imágenes-desde-ficheros-png-png2gfxpy),
junto con el formato `.gfx`. Los números de sprite van del 0 al 255, los de
tile del 0 al 4095, los de mapa del 0 al 15 y los de textura del 0 al 3,
como en el BASIC. Lee cualquier PNG menos los entrelazados, sin otros
paquetes de Python.

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
| Hora | `time`, `gettimeofday`, `localtime`: el reloj de la máquina (el 1 de enero de 2026 si no está en hora) |
| Salida | `exit`, `abort`: vuelven al BASIC (`exit(0)` da `0 OK` y cualquier otro código, un informe `W`, como [`zxv_exit_to_basic`](#zxv_exit_to_basic)) |

- Los errores de la API llegan en `errno` (`ENOENT`, `EEXIST`, `ENOTEMPTY`,
  `ENOSPC`...; sin tarjeta, `ENODEV`).
- Los descriptores 0, 1 y 2 son la consola; los de los ficheros empiezan en
  el 3.
- `struct dirent` tiene `d_name`, `d_type` (`DT_DIR` o `DT_REG`) y, de más,
  el tamaño en `d_size`.
- `printf` con decimales ocupa unos 7 KB más; con
  `-DPICOLIBC_INTEGER_PRINTF_SCANF` al enlazar se usa el pequeño, solo de
  enteros.
- La consola habla UTF-8, como las cadenas de C: `printf("¿Qué tal?")`
  enseña las tildes, y `fgets` da en UTF-8 lo que se teclea (la `ñ` son
  dos bytes). Las letras que no tiene la fuente de la ROM salen como `?`.

## Definiciones internas

`zxv.h` es compartido con la ROM y el emulador, así que define también
cosas que los programas no deben usar: la interfaz de la plataforma
(`ZXV_PLATFORM_IF` y todas las `ZXV_PIF_...`), el espacio de trabajo de la
ROM (`ZXV_ROM_WORKSPACE`, `ZXV_ROM_WORKSPACE_END`), las direcciones de la
flash (`ZXV_FLASH_BASE`, `ZXV_FLASH_SIZE`, `ZXV_PLATFORM_BASE`,
`ZXV_ROM_SIZE`) y las de la SRAM (`ZXV_SRAM_BASE`, `ZXV_SRAM_SIZE`,
`ZXV_RESERVED_END`). Están aquí solo para que las reconozcas.
