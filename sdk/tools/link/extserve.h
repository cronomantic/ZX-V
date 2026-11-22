/*
 * extserve.h - El servidor de referencia de la unidad EXT: (spec §7.7, D101
 * fase 4; el protocolo, en docs/es/hardware/protocol.md): sirve una carpeta
 * a la máquina. Sin nada del enlace: recibe una petición del canal
 * ZL_CH_EXT y da la respuesta; linkcore.c la manda (y se anuncia con
 * ZL_SERVE).
 *
 *   - La carpeta, encerrada: las rutas que llegan son absolutas desde su
 *     raíz y no salen de ella.
 *   - Los nombres, como en la tarjeta: ASCII, sin distinguir mayúsculas (se
 *     buscan así aunque el sistema las distinga), sin los ocultos.
 *   - Escribir entero o nada: lo que se escribe va a un temporal que, al
 *     cerrar, pasa a su nombre.
 *   - No sustituye lo que no escribió la máquina (ZXV_E_ACCESS), salvo con
 *     overwrite: apunta en el PC lo que escribe ella (nombre, tamaño, CRC).
 *   - Si llega otra vez la misma petición, la misma respuesta, sin hacerla
 *     otra vez.
 *
 * MIT, como el resto del SDK.
 */
#ifndef ZXV_EXTSERVE_H
#define ZXV_EXTSERVE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "zxvlink.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXT_HOST_PATH   1024

typedef struct {
    char name[64];
    uint32_t size, attr;
} ext_entry_t;

typedef struct {
    char root[EXT_HOST_PATH];           /* la carpeta que se sirve */
    bool overwrite;                     /* puede sustituir lo que no escribió la máquina */
    uint32_t boot_id;                   /* el de la máquina (ZL_SERVE), 0 si aún no */
    struct {
        FILE *f;
        bool writing, append;
        char path[EXT_HOST_PATH];       /* el de verdad */
        char tmp[EXT_HOST_PATH + 16];   /* donde se escribe */
        char zpath[260];                /* el de la máquina, para apuntarlo */
    } files[8];
    struct {
        ext_entry_t *e;
        unsigned n, next;
        bool used;
    } dirs[4];
    /* La última respuesta, por si llega otra vez la misma petición. */
    bool has_last;
    uint8_t last_type, last_seq;
    unsigned last_len;
    uint8_t last[ZL_MAX_DATA];
    /* Lo que ha hecho (para quien lo enseña): nombre y qué. */
    void *ctx;
    void (*on_event)(void *ctx, const char *what, const char *zpath);
} ext_server_t;

/* Empieza a servir folder (que exista): 0, o -1. */
int ext_server_start(ext_server_t *x, const char *folder);

/* Cierra lo abierto (lo que se escribía, sin acabar, se tira): la máquina
 * se ha reiniciado o se deja de servir. */
void ext_server_close_all(ext_server_t *x);

/* Una petición del canal ZL_CH_EXT: la respuesta (el estado y lo que va
 * detrás) en out (ZL_MAX_DATA bytes); cuántos bytes. */
unsigned ext_server_handle(ext_server_t *x, const zl_msg_t *m, uint8_t *out);

/* El número de arranque que contesta la máquina a ZL_SERVE: si cambia, se
 * ha reiniciado y lo abierto, fuera. */
void ext_server_boot(ext_server_t *x, uint32_t boot_id);

#ifdef __cplusplus
}
#endif

#endif /* ZXV_EXTSERVE_H */
