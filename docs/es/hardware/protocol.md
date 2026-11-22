[English](../../en/hardware/protocol.md) · **Español**

# El protocolo de la unidad EXT: y de la carga remota

Este documento es para quien quiera hacer **un servidor de la unidad `EXT:`**
(un programa que sirve una carpeta a la ZX-V: en un PC, una Raspberry Pi, un
móvil, otra máquina...) o hablar con la placa por su tira **J5** por su cuenta.
`zxv-link` (en [`sdk/tools/link`](../../../sdk/tools/link/), MIT) es la
implementación de referencia de los dos lados que tocan al PC: `zxvlink.c`
(las tramas), `linkcore.c` (la carga remota) y `extserve.c` (el servidor de
`EXT:`).

Lo que es `EXT:` para los programas de la máquina (las rutas, los nombres, los
números de los ficheros) está en
[la especificación, §7.7](../../../dev/spec.md#77-ficheros-tarjeta-sd-disco-rom-y-unidad-ext).
Aquí, lo que viaja.

## El enlace

- **Hoy, la tira J5 de la placa:** la UART0 de la Pico CPU (GP12 sale de la
  máquina, GP13 entra), a **921.600 baudios**, 8 bits, sin paridad, 1 bit de
  parada, sin control de flujo, a **3,3 V**. Con un adaptador USB-serie de
  3,3 V: su RX al TX de la placa, su TX al RX, y masa con masa.
- **Más adelante, otros** (el Wi-Fi de una Pico 2 W): los mismos mensajes por
  otro camino. Si hay un servidor por cada uno a la vez, la máquina usa el del
  cable.

## Las tramas

Cada mensaje va en una trama: sus bytes en **COBS**, con un **cero** detrás.
El que recibe se engancha solo en el cero siguiente aunque se pierda algo.
Antes de COBS, en little endian:

| Desplazamiento | Qué |
|---|---|
| +0 | El canal (abajo) |
| +1 | El tipo: el de la petición; en su respuesta, el mismo con el bit 7 puesto (más `0x80`) |
| +2 | El número de la petición (0-255): lo pone quien pide y la respuesta lo repite |
| +3 | Los datos: hasta **516 bytes** |
| ... | El **CRC-32** de todo lo anterior (el de zlib: polinomio `0xEDB88320`, empezando y acabando con `0xFFFFFFFF`) |

Una trama con el CRC mal, cortada o más larga de lo que cabe se tira sin
contestar: quien pidió repite la petición cuando se le acaba la espera.

**Las respuestas** empiezan siempre por un **estado** (i32): 0 o más, bien
(en algunas, un número: bytes, una posición); negativo, un error de la
máquina (`ZXV_E_*`, ver [Códigos de error](../sdk/reference.md#códigos-de-error);
por ejemplo, −1 no existe).

## Los canales

| Canal | Valor | Quién pide | Para qué |
|---|---|---|---|
| `CTRL` | 0 | El equipo de fuera | La carga remota: mandar ficheros a la tarjeta, ejecutar, depurar; y anunciarse como servidor de `EXT:` |
| `LOG` | 1 | Nadie: la máquina lo manda sola | El informe de la máquina (texto UTF-8, tipo `0x40`): no se contesta |
| `EXT` | 2 | La máquina | Lo que los programas piden a la unidad `EXT:` |

Cada lado numera sus propias peticiones, y en cada canal hay **una petición
pendiente como mucho**: no se pide la siguiente hasta tener la respuesta (o
rendirse).

## Anunciarse como servidor

Un servidor dice a la máquina que está con una petición del canal `CTRL`:

| Tipo | Datos | Respuesta |
|---|---|---|
| `SERVE` (10) | `u8`: 1 sirvo `EXT:`, 0 dejo de servirla | estado; el número de arranque de la máquina (u32) |

- **Cada segundo** mientras sirve. La máquina tiene la unidad por servida
  mientras le llegue un `SERVE` al menos cada **3 segundos**; si no, `EXT:` da
  `ZXV_E_OFFLINE` sin esperar a nadie.
- **El número de arranque** cambia cada vez que la máquina arranca (al
  encenderla, con Ctrl+Alt+Supr o al reiniciarse para ejecutar otro
  programa). Si cambia, el servidor **cierra todo lo que tenía abierto** de
  ella: la máquina ya no sabe nada de eso.
- Al dejar de servir (`SERVE` con 0, o al cerrar el puerto), lo mismo.

## Lo que pide la máquina (canal EXT)

La máquina pide en el canal `EXT` y el servidor contesta con el mismo tipo
más `0x80` y el mismo número. **Si llega otra vez la misma petición** (el mismo
número, después de una respuesta que se perdió), el servidor **repite la
respuesta sin volver a hacerlo**: escribir, borrar o renombrar dos veces no
sería lo mismo. La máquina espera cada respuesta **medio segundo**, lo pide
hasta **tres veces** y, si no, da la unidad por perdida (`ZXV_E_OFFLINE`)
hasta el siguiente `SERVE`.

Los ficheros y las carpetas abiertos van por **ranura**: la máquina tiene 8
ranuras de ficheros (0-7) y 4 de carpetas (0-3) en `EXT:`, y dice en cada
petición cuál. El servidor guarda lo que hay en cada una.

| Tipo | Datos | Respuesta, tras el estado |
|---|---|---|
| `OPEN` (1) | ranura (u8), modo (u32: los `ZXV_O_*` de `file_open`), ruta | — |
| `READ` (2) | ranura (u8), cuántos (u16, hasta 512) | Estado = bytes leídos (0, el final); detrás, esos bytes |
| `WRITE` (3) | ranura (u8), los bytes (hasta 512) | Estado = bytes escritos |
| `CLOSE` (4) | ranura (u8) | — |
| `SEEK` (5) | ranura (u8), desplazamiento (i32), desde (u8: 0 el principio, 1 donde está, 2 el final) | Estado = la posición nueva |
| `STAT` (6) | ruta | Tamaño (u32) y atributos (u32: bit 0, carpeta) |
| `REMOVE` (7) | ruta | — (borra un fichero o una carpeta vacía) |
| `RENAME` (8) | ruta de origen, un 0, ruta de destino | — |
| `MKDIR` (9) | ruta | — |
| `OPENDIR` (10) | ranura de carpeta (u8), ruta | — |
| `READDIR` (11) | ranura de carpeta (u8) | Estado = 1 y la entrada: tamaño (u32), atributos (u32) y nombre (sin el cero); 0, se acabó |
| `CLOSEDIR` (12) | ranura de carpeta (u8) | — |

La carpeta actual la lleva la máquina: cambiar de carpeta es un `STAT` que
comprueba que existe y es una carpeta.

## Las rutas

- **Absolutas desde la raíz** de lo que sirve el servidor, con `/` (`/juegos/a.bas`;
  `/` es la raíz). La máquina ya ha resuelto `.`, `..` y la carpeta actual: no
  llegan.
- **El servidor no sale de su carpeta.** Una ruta que lo intentaría (con `..`,
  `\` o una letra de unidad) es `ZXV_E_NAME`.
- **Los nombres**, como en la tarjeta ([spec §7.7](../../../dev/spec.md#77-ficheros-tarjeta-sd-disco-rom-y-unidad-ext)): hasta 63 caracteres ASCII
  imprimibles, sin `\ : * ? " < > |`, sin acabar en espacio ni en punto; rutas
  de hasta 255. **Sin distinguir mayúsculas**: `JUEGO.BAS` abre `juego.bas`;
  al crear, se conserva la grafía que pide la máquina.
- **Lo que no cumple las reglas** (un nombre con acentos, por ejemplo) no sale
  en los listados y no se puede abrir. Tampoco salen los ficheros ocultos o
  de sistema.
- **`READDIR`** da las entradas en orden alfabético, sin distinguir
  mayúsculas, como la tarjeta.

## Lo que se recomienda a un servidor

- **No sustituir lo que no escribió la máquina:** si un `OPEN` para escribir,
  un `REMOVE` o un `RENAME` estropearía un fichero de la carpeta que la
  máquina no escribió (lo que ya había, con otro contenido), contestar
  `ZXV_E_ACCESS`, salvo que quien maneja el servidor lo permita. El de
  referencia apunta lo que escribe la máquina.
- **Escribir entero o no escribir:** guardar lo que llega aparte y ponerle su
  nombre al cerrar, para que un corte no deje el fichero a medias.
- **Contestar deprisa:** la máquina espera medio segundo por respuesta.

## Lo que pide el equipo de fuera (canal CTRL)

La carga remota, para quien quiera hacerla por su cuenta (`zxv-link` es la de
referencia):

| Tipo | Datos | Respuesta, tras el estado |
|---|---|---|
| `HELLO` (1) | — | Versión del protocolo (u16), si el BASIC espera una orden (u8), versión de la especificación (u32) y del firmware (texto) |
| `OPEN` (2) | tamaño (u32), ruta en la tarjeta | — (lo que llega se escribe aparte y pasa a su nombre al cerrar) |
| `DATA` (3) | posición (u32), hasta 512 bytes | La posición siguiente (u32). Fuera de orden: `ZXV_E_INVAL` y la que espera; sin fichero abierto, `ZXV_E_BADF` |
| `CLOSE` (4) | CRC-32 de todo (u32) | — (si no cuadra, `ZXV_E_IO` y no se escribe nada) |
| `RUN` (5) | el texto que se teclea, con Intro detrás | — (con un programa en marcha, estado 1: abajo) |
| `STAT` (6) | ruta en la tarjeta | Tamaño (u32) y CRC-32 (u32) |
| `READY` (7) | — | — (cuando el BASIC espera una orden; con un programa en marcha, estado 1) |
| `DBG_READ` (8) | desde (u16), cuántos (u16) | Esos bytes del buzón del depurador del BASIC |
| `DBG_WRITE` (9) | desde (u16), los bytes | — |
| `SERVE` (10) | arriba | arriba |

**Estado 1 en `RUN` y `READY`:** había un programa en marcha y la máquina se
reinicia entera, como con Ctrl+Alt+Supr; lo pedido no se hace. Contesta otra
vez al cabo de un arranque (uno o dos segundos): hay que mandar `HELLO`
hasta que diga que el BASIC espera una orden, y pedirlo de nuevo.

## Las versiones

`HELLO` dice la versión del protocolo de la máquina. **Esta es la 4:** la
unidad `EXT:` (el canal `EXT` y `SERVE`). La 3 trajo el reinicio de la
máquina en `RUN` y `READY`; la 2, `STAT`, `READY` y el depurador. Un servidor
de `EXT:` necesita una máquina con la 4 o más.
