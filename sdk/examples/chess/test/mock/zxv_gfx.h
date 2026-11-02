#ifndef ZXV_GFX_H
#define ZXV_GFX_H
#include "zxv_api.h"
#define ZXV_TRANSPARENT 8
typedef struct {
    uint8_t *pix;
    uint16_t stride;
    uint16_t w, h;
    uint16_t reserved;
} zxv_bitmap;
void zxv_gfx_draw_page(int page);
void zxv_gfx_rect(int x, int y, int w, int h, int c);
void zxv_gfx_circle(int x, int y, int r, int c, int fill);
uint32_t zxv_spr_prepare(const zxv_bitmap *src, int sx, int sy, int w, int h, void *buf);
void zxv_spr_draw(const void *spr, int x, int y, int flags, const uint8_t *colours);
void zxv_spr_draw_scaled(const void *spr, int x, int y, int w, int h, int flags, const uint8_t *colours);
#endif
