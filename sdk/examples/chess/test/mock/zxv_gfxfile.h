#ifndef ZXV_GFXFILE_H
#define ZXV_GFXFILE_H
#include "zxv_gfx.h"
#define ZXV_GFXF_SPRITE 1
typedef struct {
    const uint8_t *data;
    uint32_t size;
} zxv_gfxfile;
int zxv_gfxf_open(zxv_gfxfile *f, const void *data, uint32_t size);
uint32_t zxv_gfxf_sprite(const zxv_gfxfile *f, int num, zxv_bitmap *b, void *buf, uint32_t room);
#endif
