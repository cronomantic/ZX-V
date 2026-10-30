/*
 * zxv_gfxfile.h - Los gráficos de un fichero .gfx (los de zxv-gfx, el editor
 * GFX de la ROM y png2gfx.py), ya en memoria: sprites, tiles, mapas y
 * texturas, en los formatos de zxv_gfx.h.
 *
 * El fichero puede estar donde sea: leído de la tarjeta con zxv_file_load o
 * dentro del programa (.incbin). Sus objetos no van alineados, así que cada
 * función copia el suyo en un buffer del programa, alineado a 4. Todas
 * devuelven los bytes que necesita ese buffer, o 0 si el objeto no está o
 * no es válido; si el buffer es NULL o más pequeño, no copian nada (para
 * preguntar el tamaño antes).
 *
 *   static uint8_t art[8192];
 *   zxv_gfxfile f;
 *   int n = zxv_file_load("art.gfx", art, sizeof(art));
 *   if (n < 0 || !zxv_gfxf_open(&f, art, n))
 *       ...
 *   zxv_bitmap b;
 *   uint32_t need = zxv_gfxf_sprite(&f, 0, &b, NULL, 0);
 */
#ifndef ZXV_GFXFILE_H
#define ZXV_GFXFILE_H

#include <stdint.h>

#include "zxv_gfx.h"

/* Clases de objeto. */
#define ZXV_GFXF_SPRITE     1
#define ZXV_GFXF_TILES      2           /* su número es el lado: 8 o 16 */
#define ZXV_GFXF_MAP        3
#define ZXV_GFXF_TEXTURE    4

typedef struct {
    const uint8_t *data;
    uint32_t size;
} zxv_gfxfile;

/* 1 si data es un .gfx entero y bien formado (y f queda listo); 0 si no. */
int zxv_gfxf_open(zxv_gfxfile *f, const void *data, uint32_t size);

/* Los datos del objeto de esa clase y ese número, tal cual están en el
 * fichero (sin alinear), y su longitud en *len; NULL si no está. */
const uint8_t *zxv_gfxf_find(const zxv_gfxfile *f, int kind, int num, uint32_t *len);

/* El sprite num como bitmap: b recibe sus medidas y apunta a buf, donde se
 * copian sus píxeles con líneas de un múltiplo de 4 bytes. Listo para
 * zxv_spr_prepare, zxv_gfx_blit o zxv_gfx_target. */
uint32_t zxv_gfxf_sprite(const zxv_gfxfile *f, int num, zxv_bitmap *b, void *buf, uint32_t room);

/* Los tiles de lado size (8 o 16), seguidos en buf; *count: cuántos son. */
uint32_t zxv_gfxf_tiles(const zxv_gfxfile *f, int size, void *buf, uint32_t room, int *count);

/* El mapa num: sus celdas se copian en buf y m queda con ellas y con tiles,
 * los tiles de su lado (de zxv_gfxf_tiles). */
uint32_t zxv_gfxf_map(const zxv_gfxfile *f, int num, zxv_tilemap *m, const void *tiles,
                      void *buf, uint32_t room);

/* La textura num de modo 7: sus texels se copian en buf. */
uint32_t zxv_gfxf_texture(const zxv_gfxfile *f, int num, zxv_m7tex *t, void *buf, uint32_t room);

#endif /* ZXV_GFXFILE_H */
