/*
 * netpc.h - La carga remota por la red, en el PC (D106; el protocolo, en
 * zlnet.h): buscar las máquinas de la red, emparejarse con una y hablar con
 * ella con las mismas tramas que por J5 (linkcore.c lo usa como el puerto
 * serie). Lo que recuerda (el id de este PC y la clave de cada máquina
 * emparejada) va en un fichero de texto de la carpeta de zxv-link, y el
 * último contador usado con cada máquina, en otro al lado (zlnet.h).
 *
 * MIT, como el resto del SDK.
 */
#ifndef ZXV_NETPC_H
#define ZXV_NETPC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zlnet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct netpc netpc_t;

/* Una máquina que ha contestado. */
typedef struct {
    char name[ZLN_NAME + 1];            /* "ZX-V-1A2B" */
    char addr[24];                      /* "192.168.0.24" (con ":puerto" si no es ZLN_PORT) */
    char firmware[48];                  /* "ZX-V 0.11.0" */
    bool paired;                        /* con este PC */
    unsigned version;                   /* su protocolo de la red: si no es ZLN_VERSION, no se habla con ella */
} netpc_found_t;

/* Si port es una máquina de la red (no un puerto serie): una dirección IPv4
 * (con :puerto, si acaso), un nombre "ZX-V-..." o lo que lista netpc_find
 * ("ZX-V-1A2B (192.168.0.24)"). */
bool netpc_is_target(const char *port);

/* Las máquinas de la red que contestan en ms: cuántas (hasta max). cfg: el
 * fichero de lo que recuerda este PC. */
int netpc_find(const char *cfg, netpc_found_t *out, int max, unsigned ms);

/* Hablar con una máquina (target, como netpc_is_target): NULL si no
 * contesta o no se encuentra (el porqué en why, 160 bytes), o si habla otra
 * versión del protocolo de la red. *version (si no es NULL): la de la
 * máquina, si ha contestado (si no, 0). *paired dice si ya está emparejada
 * con este PC (si no, solo se puede emparejar). */
netpc_t *netpc_open(const char *cfg, const char *target, bool *paired, char *why, unsigned *version);
void netpc_close(netpc_t *n);

/* Las tramas, como un puerto serie: lo que llega (firmado y en orden) en
 * timeout_ms, y mandar una (una trama entera de zxvlink.h en cada
 * llamada). -1 si falla la red. */
int netpc_read(netpc_t *n, uint8_t *buf, size_t max, unsigned timeout_ms);
int netpc_write(netpc_t *n, const uint8_t *frame, size_t len);

/* Emparejar (con el cuadro EDIT > Wi-Fi > Emparejar un PC abierto en la máquina):
 * empezar, con el nombre de este PC, y el código de 6 cifras que enseñan los
 * dos; luego, cómo va (ZLN_PAIR_WAIT, ZLN_PAIR_OK: guardado; ZLN_PAIR_NO).
 * Los errores, negativos (el porqué en why): ZLN_PAIR_CLOSED y
 * ZLN_PAIR_BUSY en negativo, o -100 si no contesta. */
int netpc_pair_begin(netpc_t *n, const char *pc_name, uint32_t *code, char *why);
int netpc_pair_poll(netpc_t *n);

/* El nombre de la máquina y su dirección. */
const char *netpc_name(const netpc_t *n);
const char *netpc_addr(const netpc_t *n);

#ifdef __cplusplus
}
#endif

#endif /* ZXV_NETPC_H */
