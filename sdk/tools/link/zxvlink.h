/*
 * zxvlink.h - El protocolo de la carga remota de ZX-V (D101): lo que se
 * dicen el PC (zxv-link) y la Pico CPU por la UART de la tira J5, y la unidad
 * EXT: (desde la versión 4). Documentado para quien haga su propio servidor
 * en docs/es/hardware/protocol.md.
 *
 * Cada mensaje va en una trama: sus bytes en COBS entre dos ceros (como la
 * línea del sonido), así que el que recibe se engancha solo en el cero
 * siguiente aunque se pierda algo. Antes de COBS (little endian):
 *
 *   +0   canal: ZL_CH_CTRL (peticiones del PC y sus respuestas),
 *        ZL_CH_LOG (el informe de la máquina, sin que se lo pidan) o
 *        ZL_CH_EXT (peticiones de la máquina a la unidad EXT: y sus
 *        respuestas)
 *   +1   tipo: ZL_* en una petición; el mismo | ZL_REPLY en su respuesta
 *   +2   número de la petición (el PC lo pone; la respuesta lo repite)
 *   +3   los datos (hasta ZL_MAX_DATA bytes)
 *   ...  CRC-32 de todo lo anterior (el de zlib)
 *
 * Las peticiones (canal ZL_CH_CTRL) y sus datos; cada respuesta empieza por
 * un estado (i32: 0 bien, un error ZXV_E_*, negativo, o ZL_ST_RESTART):
 *
 *   ZL_HELLO   -                 estado, versión del protocolo (u16), si el
 *                                BASIC espera una orden (u8), la versión de
 *                                la spec (u32, ZXV_SV_VERSION) y la de
 *                                ZX-V (texto: "ZX-V 0.10.0")
 *   ZL_OPEN    tamaño (u32), ruta   estado (lo que llega va a SD:/zxv-link.tmp;
 *                                la ruta, con SD: o desde la raíz; el que
 *                                hubiera con ese nombre sigue igual)
 *   ZL_DATA    posición (u32), bytes   estado (tienen que ir seguidos: si no,
 *                                ZXV_E_INVAL y la posición que espera, u32;
 *                                sin fichero abierto, ZXV_E_BADF)
 *   ZL_CLOSE   CRC-32 de todo (u32)   estado (si cuadra, el temporal pasa a
 *                                la ruta, en lugar del que hubiera; si no,
 *                                ZXV_E_IO y el temporal se borra)
 *   ZL_RUN     texto             estado (lo teclea, con Enter, cuando el
 *                                BASIC espera una orden; si hay un programa
 *                                en marcha, no teclea nada: ZL_ST_RESTART)
 *
 * Desde la versión 2:
 *
 *   ZL_STAT    ruta              estado, tamaño (u32) y CRC-32 (u32) de un
 *                                fichero de la tarjeta (para no mandar otra
 *                                vez lo que ya está)
 *   ZL_READY   -                 estado: 0 si el BASIC espera una orden; si
 *                                hay un programa en marcha, ZL_ST_RESTART
 *   ZL_DBG_READ   desde (u16), cuántos (u16)   estado y esos bytes del buzón
 *                                del depurador del BASIC (ZXV_DBG_BOX, spec
 *                                §8.2: 0x300 bytes)
 *   ZL_DBG_WRITE  desde (u16), bytes   estado (1, 2 o 4 bytes alineados van de
 *                                una vez, como una palabra)
 *
 * Desde la versión 4, la unidad EXT: (spec §7.7):
 *
 *   ZL_SERVE   u8: 1 sirvo EXT:, 0 ya no   estado y el número de arranque
 *                                de la máquina (u32: si cambia, se ha
 *                                reiniciado y el servidor cierra lo suyo).
 *                                Cada ZL_SERVE_EVERY_MS mientras se sirve.
 *
 *   Y en el canal ZL_CH_EXT pide la máquina (ZLX_*, abajo; una petición
 *   pendiente como mucho, ZL_EXT_TRIES veces de ZL_EXT_WAIT_MS cada una; si
 *   llega otra vez la misma, el servidor repite la respuesta sin volver a
 *   hacerlo). Los ficheros y carpetas van por ranuras (0-7 y 0-3), y las
 *   rutas, absolutas desde la raíz de lo que se sirve.
 *
 *   ZLX_OPEN      ranura (u8), modo (u32), ruta        estado
 *   ZLX_READ      ranura (u8), cuántos (u16)           estado = bytes, y esos bytes
 *   ZLX_WRITE     ranura (u8), bytes                   estado = bytes escritos
 *   ZLX_CLOSE     ranura (u8)                          estado
 *   ZLX_SEEK      ranura (u8), desp. (i32), desde (u8) estado = la posición
 *   ZLX_STAT      ruta                                 estado, tamaño, atributos (u32)
 *   ZLX_REMOVE    ruta                                 estado
 *   ZLX_RENAME    ruta, 0, ruta                        estado
 *   ZLX_MKDIR     ruta                                 estado
 *   ZLX_OPENDIR   ranura (u8), ruta                    estado
 *   ZLX_READDIR   ranura (u8)                          estado (1 hay, 0 no), tamaño,
 *                                                      atributos (u32) y nombre
 *   ZLX_CLOSEDIR  ranura (u8)                          estado
 *
 * Y sin petición: ZL_CH_LOG, ZL_LOG con el texto del informe.
 *
 * MIT, como el resto del SDK.
 */
#ifndef ZXVLINK_H
#define ZXVLINK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ZL_VERSION      4

/* Desde la versión 3: había un programa en marcha y la máquina se reinicia
 * entera, como con Ctrl+Alt+Supr (lo que se pidió no se hace). Vuelve a
 * contestar al cabo de un arranque: hay que saludarla otra vez (ZL_HELLO)
 * hasta que el BASIC espere una orden. */
#define ZL_ST_RESTART   1
#define ZL_BAUD         921600
#define ZL_DBG_SIZE     0x300           /* el buzón del depurador (ZXV_DBG_BOX) */
#define ZL_CHUNK        512             /* los bytes de un fichero en cada ZL_DATA */
#define ZL_MAX_DATA     (4 + ZL_CHUNK)  /* datos de una trama, como mucho */
#define ZL_RAW_MAX      (3 + ZL_MAX_DATA + 4)
#define ZL_FRAME_MAX    (ZL_RAW_MAX + ZL_RAW_MAX / 254 + 3)     /* en COBS, con sus dos ceros */

enum { ZL_CH_CTRL = 0, ZL_CH_LOG = 1, ZL_CH_EXT = 2 };
enum { ZL_HELLO = 1, ZL_OPEN = 2, ZL_DATA = 3, ZL_CLOSE = 4, ZL_RUN = 5, ZL_STAT = 6, ZL_READY = 7, ZL_DBG_READ = 8,
       ZL_DBG_WRITE = 9, ZL_SERVE = 10, ZL_LOG = 0x40 };
enum { ZLX_OPEN = 1, ZLX_READ = 2, ZLX_WRITE = 3, ZLX_CLOSE = 4, ZLX_SEEK = 5, ZLX_STAT = 6, ZLX_REMOVE = 7,
       ZLX_RENAME = 8, ZLX_MKDIR = 9, ZLX_OPENDIR = 10, ZLX_READDIR = 11, ZLX_CLOSEDIR = 12 };

#define ZL_SERVE_EVERY_MS   1000        /* el servidor se anuncia */
#define ZL_SERVE_LOST_MS    3000        /* sin anuncio en este tiempo, EXT: no tiene servidor */
#define ZL_EXT_WAIT_MS      500         /* la máquina espera cada respuesta... */
#define ZL_EXT_TRIES        3           /* ... y lo pide hasta estas veces */
#define ZL_REPLY        0x80

/* Un mensaje: lo que lleva una trama, sin el CRC. */
typedef struct {
    uint8_t ch, type, seq;
    const uint8_t *data;
    unsigned len;
} zl_msg_t;

uint32_t zl_crc32(uint32_t crc, const void *data, size_t n);

/* La trama de un mensaje (len <= ZL_MAX_DATA) en out (ZL_FRAME_MAX bytes),
 * con sus dos ceros: devuelve cuántos bytes, o 0 si no cabe. */
size_t zl_pack(uint8_t ch, uint8_t type, uint8_t seq, const void *data, unsigned len, uint8_t *out);

/* Lo mismo con los datos en dos trozos (una cabecera y lo que va detrás:
 * así no hace falta juntarlos antes en otro buffer). */
size_t zl_pack2(uint8_t ch, uint8_t type, uint8_t seq, const void *head, unsigned hlen, const void *data,
                unsigned len, uint8_t *out);

/* El receptor, byte a byte (deshace COBS en su sitio: lo de dentro ocupa
 * menos). */
typedef struct {
    uint16_t len;
    bool overflow;
    uint8_t buf[ZL_FRAME_MAX];
    uint32_t good, bad;                 /* tramas buenas y malas (CRC, formato, cortadas) */
} zl_rx_t;

void zl_rx_init(zl_rx_t *rx);

/* Un byte más: true si con él se completa una trama buena (en *m, que
 * apunta a rx hasta el byte siguiente). */
bool zl_feed(zl_rx_t *rx, uint8_t byte, zl_msg_t *m);

/* Para leer y escribir los datos en little endian. */
static inline uint32_t zl_get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static inline void zl_put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

#endif /* ZXVLINK_H */
