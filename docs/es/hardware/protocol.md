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
- **La red, si la Pico CPU es una Pico 2 W** (D106): las mismas tramas, cada
  una en un paquete UDP firmado ([abajo](#por-la-red-wi-fi)). Si hay un
  servidor de `EXT:` por cada camino a la vez, la máquina usa el del cable.

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
| `HELLO` (1) | — | Versión del protocolo (u16), si el BASIC espera una orden (u8: 1 sí, 0 no, 2 está actualizando el firmware), versión de la especificación (u32) y del firmware (texto) |
| `OPEN` (2) | tamaño (u32), ruta en la tarjeta | — (lo que llega se escribe aparte y pasa a su nombre al cerrar) |
| `DATA` (3) | posición (u32), hasta 512 bytes | La posición siguiente (u32). Fuera de orden: `ZXV_E_INVAL` y la que espera; sin fichero abierto, `ZXV_E_BADF` |
| `CLOSE` (4) | CRC-32 de todo (u32) | — (si no cuadra, `ZXV_E_IO` y no se escribe nada) |
| `RUN` (5) | el texto que se teclea, con Intro detrás | — (con un programa en marcha, estado 1: abajo) |
| `STAT` (6) | ruta en la tarjeta | Tamaño (u32) y CRC-32 (u32) |
| `READY` (7) | — | — (cuando el BASIC espera una orden; con un programa en marcha, estado 1) |
| `DBG_READ` (8) | desde (u16), cuántos (u16) | Esos bytes del buzón del depurador del BASIC |
| `DBG_WRITE` (9) | desde (u16), los bytes | — |
| `SERVE` (10) | arriba | arriba |
| `UPDATE` (11) | — | — (la máquina se reinicia y actualiza su firmware con `EXT:/zxv-update.bin`: abajo) |
| `LIST` (12) | desde (u16), ruta | Cuántas entradas van en esta respuesta (en el estado), si hay más (u8) y cada una: tamaño (u32), atributos (u8: 1 carpeta), largo del nombre (u8) y el nombre |
| `GET` (13) | posición (u32), ruta | Cuántos bytes (en el estado, hasta 508), el tamaño del fichero (u32) y esos bytes |
| `REMOVE` (14) | ruta | — (un fichero o una carpeta vacía) |
| `RENAME` (15) | ruta, 0, ruta nueva | — (en la misma unidad; si ya hay otra cosa con el nombre nuevo, `ZXV_E_EXIST`; el mismo nombre con otras mayúsculas lo cambia) |
| `MKDIR` (16) | ruta | — |

**La tarjeta y la ROM como un explorador** (`LIST`, `GET`, `REMOVE`,
`RENAME`, `MKDIR`, desde la versión 6): las rutas llevan su unidad, `SD:` o
`ROM:` (sin unidad, la tarjeta); `EXT:` no, porque es la carpeta del propio
equipo de fuera. En `ROM:` solo se lee: lo demás da un error. `LIST`
devuelve las entradas que caben en una trama, empezando por la número
«desde»: si dice que hay más, se pide otra vez desde la siguiente. `GET`
devuelve un trozo desde «posición»; mientras se pide el mismo fichero
seguido, la máquina lo deja abierto, y cualquier otra petición lo cierra.
Ejecutar lo que ya está en la máquina es `RUN` con la orden:
`CD "SD:/juegos": LOAD "juego.zxv"`.

**Estado 1 en `RUN` y `READY`:** había un programa en marcha y la máquina se
reinicia entera, como con Ctrl+Alt+Supr; lo pedido no se hace. Contesta otra
vez al cabo de un arranque (uno o dos segundos): hay que mandar `HELLO`
hasta que diga que el BASIC espera una orden, y pedirlo de nuevo.

**`UPDATE`:** actualizar el firmware de las dos Pico desde fuera, sin tarjeta
(`zxv-link update`). Antes hay que servir `EXT:` con el fichero en
`/zxv-update.bin`, y el BASIC tiene que esperar una orden. Con el estado 0,
la máquina se reinicia en su pantalla de actualización y lee el fichero de
`EXT:`. Se vuelve a reiniciar tras escribir el firmware de la CPU y otra vez
al acabar, así que hay que seguir sirviéndolo (y anunciándose) hasta el
final. Mientras tanto, `HELLO` dice 2, el informe (canal `LOG`) cuenta los
pasos (un paso que falla acaba en «: error») y lo demás, salvo `SERVE`,
contesta `ZXV_E_ACCESS`. Al final la máquina vuelve al BASIC: si `HELLO` da
la versión del fichero, ha ido bien. Errores: `ZXV_E_OFFLINE` si no ve el
servidor de `EXT:`; `ZXV_E_ACCESS` con un programa en marcha, si la Pico no
tiene particiones (hay que grabarla una vez por USB) o si ya está
actualizando.

## Por la red (Wi-Fi)

Con una Pico 2 W en el zócalo de la CPU, la máquina se une a la red que se
elige en **EDIT > Wi-Fi** y escucha en el **puerto UDP 5612**. Cada trama de
arriba (en COBS, con su cero) va **entera en un paquete**; la máquina contesta
al puerto del que le llegó. Todo lo que no sea buscar y emparejar va
**firmado** con una clave que solo tienen la máquina y ese PC, así que nadie
más de la red puede mandarle programas ni hacerse pasar por su servidor de
`EXT:`. No va cifrado: lo que viaja se puede leer. La referencia está en
`zlnet.h`, `zlnet.c` y `netpc.c` (el PC) y en `pico/common/netsvc.c` (la
máquina).

Cada paquete empieza por `ZXV` y una letra, el tipo. Los números, en little
endian:

| Tipo | Quién | Qué lleva |
|---|---|---|
| `F` | PC | Buscar: el id del PC (8 bytes) y la versión del protocolo de la red del PC (u8). A la difusión de la red o a una dirección |
| `f` | Máquina | El id de la máquina (8), su arranque (8, al azar en cada encendido), si está emparejada con ese PC (u8), la versión de las tramas (u16), su nombre (16, con ceros: `ZX-V-1A2B`, de su MAC), el firmware (texto: `ZX-V 0.11.0`), un 0 y la versión del protocolo de la red (u8) |
| `P` | PC | Empezar a emparejar: id del PC (8), su clave pública X25519 (32) y su nombre |
| `p` | Máquina | Estado (u8), su clave pública (32) y su compromiso (32) |
| `N` | PC | id del PC (8) y su número al azar Npc (16). Desde la dirección y el puerto del `P` |
| `n` | Máquina | Estado (u8) y su número al azar Nm (16) |
| `Q` | PC | id del PC (8) y su prueba (16, abajo): ¿ha contestado el usuario? |
| `q` | Máquina | Estado (u8) y, si es 1, su prueba (16) |
| `D` | PC | Datos: id del PC (8), arranque de la máquina (8), contador (u64), una trama y la firma (16) |
| `d` | Máquina | Datos, como `D` |
| `b` | Máquina | id del PC (8), su arranque nuevo (8), el contador del `D` al que contesta (u64) y la firma (16): contesta así a un `D` con otro arranque |

**Los estados:** 0 sigue (espera al usuario o al paso siguiente), 1
aceptado, 2 rechazado (o se cerró el cuadro), 3 la máquina no está
emparejando, 4 empareja con otro PC.

### Emparejar

Como la comparación de números de Bluetooth. El usuario abre **EDIT > Wi-Fi >
Emparejar un PC** en la máquina; fuera de ahí contesta a `P` con el estado 3.

1. El PC manda `P` con su clave pública Ppc.
2. La máquina saca una clave X25519 nueva Pm y un número Nm, y contesta `p`
   con Pm y el compromiso `SHA-256("ZX-V commit" | Nm | Pm | Ppc)`.
3. El PC manda `N` con su número Npc; la máquina contesta `n` con Nm. El PC
   comprueba que Nm da el compromiso de antes.
4. Los dos enseñan el código: los 4 primeros bytes de
   `SHA-256("ZX-V code" | Ppc | Pm | Npc | Nm)`, como número big endian, módulo
   1.000.000 (seis cifras).
5. El usuario mira que coinciden y acepta en la máquina. El PC pregunta con
   `Q` hasta tener 1 o 2. En `Q` va la prueba de la clave que tiene el PC:
   los 16 primeros bytes de `HMAC-SHA-256(clave, "ZX-V query" | id del PC |
   id de la máquina)`. La máquina contesta 1 solo si tiene guardada esa
   clave (el usuario ha aceptado), con su prueba, `"ZX-V paired"` en lugar
   de `"ZX-V query"`; el PC no guarda la clave si no cuadra. Contesta 2 si
   no la tiene (se rechazó o se cerró el cuadro), aunque ese PC ya
   estuviera emparejado con otra clave.
6. La clave del par es `SHA-256("ZX-V key" | X25519(su clave, la del otro) |
   Ppc | Pm | Npc | Nm)`. La máquina la guarda con el id y el nombre del PC
   (recuerda 8 PC; con uno más se olvida el más antiguo).

Las etiquetas van en ASCII, sin cero. Como la máquina se compromete con Nm
antes de ver Npc, quien se ponga en medio no puede buscar unas claves con las
que los dos códigos coincidan: tendría una oportunidad entre un millón.

### Los datos

`D` (del PC a la máquina), `d` (de la máquina al PC) y `b` van firmados con
la clave del par: la firma son los 16 primeros bytes de `HMAC-SHA-256(clave,
"ZX-V net" | la versión del protocolo de la red (u8) | todo lo anterior del
paquete)`. La letra va firmada, así que un paquete devuelto a quien lo mandó
no vale, y con la versión dentro, una máquina y un PC de versiones distintas
no se aceptan nada. En los dos sentidos lleva el id del PC y el arranque de
la máquina. **Los contadores no se
repiten:** lo que llega con uno que la máquina ya ha visto, con la firma mal
o de un PC que no conoce se tira sin contestar (no se puede repetir un
paquete grabado). Como en un PC puede haber varios programas a la vez (por
ejemplo `zxv-link serve` y `zxv-link log`), cada uno con su puerto UDP, sus
paquetes pueden llegar desordenados: la máquina recuerda los 16 últimos
contadores aceptados y, por debajo, un suelo por PC, y tira lo que no pasa
del suelo. El de la máquina empieza en cada arranque; el del PC sale de su
reloj, para que siga subiendo aunque se vuelva a abrir: `zxv-link` usa los
milisegundos por 1024 con una vía al azar de cada programa en los bits 4 a 9,
así que dos programas no dan el mismo contador en el mismo milisegundo. Y
**nunca baja para una máquina**, aunque el reloj del PC se atrase: `zxv-link`
apunta el último que ha usado con cada una (cada segundo, o cada 256 si el
reloj se ha atrasado) y sigue por encima de lo apuntado; si el reloj está
por detrás, de uno en uno.

Si la máquina se ha reiniciado, el arranque ya no es el que tiene el PC: a un
`D` bien firmado con el arranque de antes, la máquina contesta `b` con el
nuevo y el contador de ese `D`. El PC solo lo cree si contesta a uno de sus
últimos `D` y trae otro arranque; entonces lo cambia y repite lo último. Un
`b` grabado no sirve: ya no contesta a ninguno de esos.

**A cada programa del PC, lo suyo** (cada uno es una dirección y un puerto
UDP):

- **La respuesta** de una petición va al programa que la hizo, también la de
  `RUN`, que sale cuando el BASIC ha tecleado la orden, aunque mientras
  hablen otros.
- **Lo que piden los programas a `EXT:`** va al programa que la sirve: el del
  último `SERVE`.
- **El informe** va a todos los que han dicho algo en los últimos 15
  segundos (como mucho 4). Un programa que solo escucha el informe manda un
  `HELLO` cada 5 segundos para seguir en la lista.

**En el PC,** `zxv-link` guarda su id y las claves de las máquinas en
`zxv-link-net.txt` (en `%APPDATA%\ZX-V` en Windows y en `~/.config/ZX-V` en
los demás), y el último contador de cada máquina, al lado, en
`zxv-link-net-counters.txt`.

**La versión del protocolo de la red** va en `F` y en `f`; esta es la 2. Una
máquina que no la dice en `f` tiene la 1 (las primeras de prueba). Con otra
versión, `zxv-link` no habla con la máquina: dice que no son compatibles y
qué hay que actualizar, la máquina o `zxv-link`. Las tramas de J5 no cambian.

## Las versiones

`HELLO` dice la versión del protocolo de la máquina. **Esta es la 6:**
`LIST`, `GET`, `REMOVE`, `RENAME` y `MKDIR`. La 5 trajo `UPDATE`; la 4,
la unidad `EXT:` (el canal `EXT` y `SERVE`); la 3, el reinicio de la
máquina en `RUN` y `READY`; la 2, `STAT`, `READY` y el depurador. Un servidor de `EXT:` necesita una máquina con la 4 o más;
actualizar por J5, con la 5, y explorar la tarjeta, con la 6.
