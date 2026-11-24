/*
 * zlnet.h - La carga remota por la red (D106): lo que se dicen el PC y la
 * máquina por UDP, en el puerto ZLN_PORT de la máquina. Lo de dentro son las
 * mismas tramas que por J5 (zxvlink.h); aquí, cómo se encuentran, cómo se
 * emparejan y cómo va firmado cada paquete. Documentado para quien haga su
 * propio programa en docs/es/hardware/protocol.md.
 *
 * Cada paquete empieza por "ZXV" y una letra (el tipo). En little endian:
 *
 *   Buscar (sin firmar; el PC, a la red o a una dirección):
 *     F  id del PC (8), versión del protocolo de la red del PC (u8, ZLN_VERSION)
 *     f  id de la máquina (8), arranque (8), emparejado con ese PC (u8),
 *        versión del protocolo de las tramas (u16), nombre (16, con ceros),
 *        firmware (texto: "ZX-V 0.11.0"), un 0 y la versión del protocolo de
 *        la red (u8, ZLN_VERSION). Sin el 0 y la versión, es la 1 (las
 *        primeras versiones de prueba). Con otra versión, el PC no habla con
 *        ella: dice que no son compatibles y cuál hay que actualizar.
 *
 *   Emparejar (sin firmar; solo con el cuadro Wi-Fi de la máquina abierto en
 *   «Emparejar»). Como la comparación de números de Bluetooth: la máquina se
 *   compromete con su número antes de ver el del PC, así que quien se ponga
 *   en medio no puede buscar unas claves con las que los dos códigos
 *   coincidan:
 *     P  id del PC (8), clave pública del PC (32, X25519), nombre del PC
 *     p  estado (u8, ZLN_PAIR_*), clave pública de la máquina (32),
 *        compromiso: SHA-256("ZX-V commit" | Nm | clave de la máquina | clave del PC)
 *     N  id del PC (8), Npc (16, al azar); solo desde la dirección y el
 *        puerto del P
 *     n  estado (u8), Nm (16)
 *     Q  id del PC (8), prueba del PC (16: zln_proof("ZX-V query", clave
 *        nueva, id del PC, id de la máquina)): la máquina sabe así de qué
 *        clave le pregunta
 *     q  estado (u8): ZLN_PAIR_WAIT mientras el usuario no contesta,
 *        ZLN_PAIR_NO rechazado (o la máquina no tiene esa clave: el cuadro
 *        se cerró), ZLN_PAIR_OK aceptado y guardado, y detrás la prueba de la
 *        máquina (16: zln_proof("ZX-V paired", ...)): el PC solo guarda la
 *        clave si la máquina demuestra que la tiene
 *   Los dos enseñan el código (zln_code, 6 cifras); el usuario mira que
 *   coinciden y acepta en la máquina. La clave del par, zln_key.
 *
 *   Datos (firmados con la clave del par; la letra dice el sentido y va
 *   firmada, así que un paquete devuelto a quien lo mandó no vale):
 *     D  del PC a la máquina, d  de la máquina al PC: id del PC (8),
 *        arranque de la máquina (8), contador (u64), una trama de zxvlink.h
 *        (con sus ceros), firma (16)
 *   La firma: los primeros bytes de HMAC-SHA-256(clave, "ZX-V net" |
 *   ZLN_VERSION (u8) | todo lo anterior del paquete). Los contadores no se
 *   repiten: lo que llega con uno ya visto, o con otro arranque, se tira (no
 *   se puede repetir un paquete grabado). En un PC puede haber varios
 *   programas a la vez, cada uno con su puerto UDP, y sus paquetes se
 *   cruzan: la máquina recuerda los últimos y, por debajo, un suelo por PC.
 *   El del PC sale de su reloj, en ms << 10, con una vía al azar de cada
 *   programa en los bits 4 a 9 (así dos programas no dan el mismo en el
 *   mismo ms), y nunca baja para una máquina, aunque el reloj del PC se
 *   atrase: zxv-link apunta el último en el PC (netpc.c) y sigue por encima;
 *   el de la máquina empieza en cada arranque.
 *   A cada programa, lo suyo: la respuesta, al puerto de la petición (la de
 *   ZL_RUN, al de su petición, aunque hablen otros mientras); lo que pide
 *   EXT:, al del último ZL_SERVE; el informe, a los que han dicho algo en
 *   ZLN_RECENT_MS. Un programa que solo escucha el informe manda ZL_HELLO
 *   cada ZLN_KEEPALIVE_MS.
 *     b  id del PC (8), arranque nuevo (8), el contador del D al que
 *        contesta (u64), firma (16, como la de D): la máquina contesta así a
 *        un D bien firmado de un PC emparejado con otro arranque (se ha
 *        reiniciado). El PC solo lo cree si contesta a un D suyo de los
 *        últimos con el arranque de antes; entonces lo cambia y repite lo
 *        último. Uno grabado no sirve: ya no contesta a ninguno de esos.
 *
 * MIT, como el resto del SDK.
 */
#ifndef ZXV_ZLNET_H
#define ZXV_ZLNET_H

#include <stddef.h>
#include <stdint.h>

#include "zxvlink.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZLN_PORT        5612            /* el de la máquina */
#define ZLN_VERSION     2               /* el protocolo de la red (no el de las tramas, ZL_VERSION) */
#define ZLN_ID          8
#define ZLN_BOOT        8
#define ZLN_KEY         32
#define ZLN_NONCE       16
#define ZLN_TAG         16
#define ZLN_PROOF       16              /* las pruebas de Q y q */
#define ZLN_NAME        16
#define ZLN_HEAD        (4 + ZLN_ID + ZLN_BOOT + 8)     /* un D hasta la trama */
#define ZLN_DATAGRAM_MAX (ZLN_HEAD + ZL_FRAME_MAX + ZLN_TAG)
#define ZLN_BOOT_SIZE   (4 + ZLN_ID + ZLN_BOOT + 8 + ZLN_TAG)   /* un b */
#define ZLN_PEERS       8               /* los PC emparejados que recuerda la máquina */
#define ZLN_RECENT_MS   15000           /* el informe, a los que han hablado en este tiempo... */
#define ZLN_KEEPALIVE_MS 5000           /* ... así que quien solo escucha saluda cada tanto */

enum { ZLN_FIND = 'F', ZLN_FOUND = 'f', ZLN_PAIR = 'P', ZLN_PAIR_R = 'p', ZLN_NONCE_T = 'N', ZLN_NONCE_R = 'n',
       ZLN_QUERY = 'Q', ZLN_QUERY_R = 'q', ZLN_DATA = 'D', ZLN_DATA_R = 'd', ZLN_BOOT_T = 'b' };

/* El estado de un emparejamiento. */
enum {
    ZLN_PAIR_WAIT = 0,                  /* sigue: espera al usuario o al paso siguiente */
    ZLN_PAIR_OK = 1,                    /* aceptado */
    ZLN_PAIR_NO = 2,                    /* rechazado, o el cuadro se cerró */
    ZLN_PAIR_CLOSED = 3,                /* la máquina no está emparejando (abre EDIT > Wi-Fi > Emparejar un PC) */
    ZLN_PAIR_BUSY = 4,                  /* empareja con otro PC */
};

/* Si un paquete es de ZX-V: su tipo (ZLN_*), o 0. */
int zln_type(const uint8_t *p, size_t n);

/* Lo que se calcula al emparejar (pk: claves públicas; nm, npc: los números). */
void zln_commit(const uint8_t nm[ZLN_NONCE], const uint8_t pk_machine[32], const uint8_t pk_pc[32],
                uint8_t out[32]);
uint32_t zln_code(const uint8_t pk_pc[32], const uint8_t pk_machine[32], const uint8_t npc[ZLN_NONCE],
                  const uint8_t nm[ZLN_NONCE]);
void zln_key(const uint8_t shared[32], const uint8_t pk_pc[32], const uint8_t pk_machine[32],
             const uint8_t npc[ZLN_NONCE], const uint8_t nm[ZLN_NONCE], uint8_t out[ZLN_KEY]);

/* Un D (type ZLN_DATA, del PC) o un d (ZLN_DATA_R, de la máquina): la
 * cabecera y la trama en buf (ZLN_DATAGRAM_MAX bytes) y la firma detrás;
 * cuántos bytes. */
size_t zln_seal(uint8_t *buf, int type, const uint8_t id[ZLN_ID], const uint8_t boot[ZLN_BOOT], uint64_t counter,
                const uint8_t *frame, size_t n, const uint8_t key[ZLN_KEY]);

/* Si un paquete de n bytes es del tipo type (ZLN_DATA o ZLN_DATA_R) y su
 * firma es buena con key: la trama en *frame, *frame_n, y el contador en
 * *counter. */
int zln_open(const uint8_t *p, size_t n, int type, const uint8_t key[ZLN_KEY], const uint8_t **frame,
             size_t *frame_n, uint64_t *counter);

/* Un b (ZLN_BOOT_SIZE bytes en buf): el arranque nuevo de la máquina, en
 * respuesta al D con ese contador. */
size_t zln_seal_boot(uint8_t *buf, const uint8_t id[ZLN_ID], const uint8_t boot[ZLN_BOOT], uint64_t counter,
                     const uint8_t key[ZLN_KEY]);

/* Si un b de n bytes está bien firmado con key: el arranque en boot y el
 * contador al que contesta en *counter. */
int zln_open_boot(const uint8_t *p, size_t n, const uint8_t key[ZLN_KEY], uint8_t boot[ZLN_BOOT],
                  uint64_t *counter);

/* Las pruebas de Q y q (ZLN_PROOF bytes): los primeros de HMAC-SHA-256(clave,
 * label | id del PC | id de la máquina). */
void zln_proof(const char *label, const uint8_t key[ZLN_KEY], const uint8_t pc_id[ZLN_ID],
               const uint8_t machine_id[ZLN_ID], uint8_t out[ZLN_PROOF]);

static inline uint64_t zln_get64(const uint8_t *p) { return zl_get32(p) | (uint64_t)zl_get32(p + 4) << 32; }
static inline void zln_put64(uint8_t *p, uint64_t v)
{
    zl_put32(p, (uint32_t)v);
    zl_put32(p + 4, (uint32_t)(v >> 32));
}

#ifdef __cplusplus
}
#endif

#endif /* ZXV_ZLNET_H */
