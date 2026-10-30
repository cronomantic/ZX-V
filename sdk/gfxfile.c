/*
 * gfxfile.c - Los gráficos de un fichero .gfx (include/zxv_gfxfile.h).
 *
 * El fichero: "ZXVG", versión (1), 0, número de objetos (u16), y cada
 * objeto con su cabecera (clase, 0, número u16, longitud u32) y sus datos.
 * Todo con el byte bajo primero. Se enlaza con todos los programas del SDK;
 * lo que no se use no entra en el .ZXV.
 */
#include <stddef.h>

#include "zxv_gfxfile.h"

static uint32_t rd16(const uint8_t *p) { return (uint32_t)(p[0] | p[1] << 8); }
static uint32_t rd32(const uint8_t *p) { return rd16(p) | rd16(p + 2) << 16; }

static void copy(void *d, const uint8_t *s, uint32_t n)
{
    uint8_t *o = d;
    while (n--)
        *o++ = *s++;
}

/* Si los datos de un objeto tienen la longitud que dicen sus medidas. */
static int valid(int kind, uint32_t num, const uint8_t *d, uint32_t len)
{
    switch (kind) {
    case ZXV_GFXF_SPRITE: {
        if (len < 4)
            return 0;
        uint32_t w = rd16(d), h = rd16(d + 2);
        return w >= 1 && w <= 1024 && h >= 1 && h <= 1024 && len == 4 + (w + 1) / 2 * h;
    }
    case ZXV_GFXF_TILES:
        return (num == 8 || num == 16) && len >= 4 && rd16(d) >= 1 &&
               len == 4 + rd16(d) * num * num / 2;
    case ZXV_GFXF_MAP:
        return len >= 8 && rd16(d) >= 1 && rd16(d + 2) >= 1 && (d[4] == 8 || d[4] == 16) &&
               len == 8 + rd16(d) * rd16(d + 2) * 2;
    case ZXV_GFXF_TEXTURE:
        return len >= 4 && d[0] >= 1 && d[0] <= 10 && len == 4 + (1u << (2 * d[0]));
    }
    return 1;                               /* clases nuevas: se saltan */
}

int zxv_gfxf_open(zxv_gfxfile *f, const void *data, uint32_t size)
{
    const uint8_t *p = data;
    f->data = NULL;
    f->size = 0;
    if (size < 8 || p[0] != 'Z' || p[1] != 'X' || p[2] != 'V' || p[3] != 'G' || p[4] != 1)
        return 0;
    uint32_t at = 8;
    for (uint32_t i = rd16(p + 6); i > 0; i--) {
        if (size - at < 8)
            return 0;
        uint32_t len = rd32(p + at + 4);
        if (len > size - at - 8 || !valid(p[at], rd16(p + at + 2), p + at + 8, len))
            return 0;
        at += 8 + len;
    }
    f->data = p;
    f->size = size;
    return 1;
}

const uint8_t *zxv_gfxf_find(const zxv_gfxfile *f, int kind, int num, uint32_t *len)
{
    if (!f->data)
        return NULL;
    const uint8_t *p = f->data + 8;
    for (uint32_t i = rd16(f->data + 6); i > 0; i--) {
        uint32_t n = rd32(p + 4);
        if (p[0] == kind && rd16(p + 2) == (uint32_t)num) {
            if (len)
                *len = n;
            return p + 8;
        }
        p += 8 + n;
    }
    return NULL;
}

uint32_t zxv_gfxf_sprite(const zxv_gfxfile *f, int num, zxv_bitmap *b, void *buf, uint32_t room)
{
    const uint8_t *d = zxv_gfxf_find(f, ZXV_GFXF_SPRITE, num, NULL);
    if (!d)
        return 0;
    uint32_t w = rd16(d), h = rd16(d + 2), row = (w + 1) / 2, stride = (row + 3) & ~3u;
    *b = (zxv_bitmap){ buf, (uint16_t)stride, (uint16_t)w, (uint16_t)h, 0 };
    if (buf && room >= stride * h)
        for (uint32_t y = 0; y < h; y++) {
            uint8_t *o = (uint8_t *)buf + y * stride;
            copy(o, d + 4 + y * row, row);
            if (w & 1)                      /* el píxel de relleno, transparente */
                o[row - 1] = (uint8_t)((o[row - 1] & 0x0F) | ZXV_TRANSPARENT << 4);
            for (uint32_t x = row; x < stride; x++)
                o[x] = ZXV_TRANSPARENT * 0x11;
        }
    return stride * h;
}

uint32_t zxv_gfxf_tiles(const zxv_gfxfile *f, int size, void *buf, uint32_t room, int *count)
{
    const uint8_t *d = zxv_gfxf_find(f, ZXV_GFXF_TILES, size, NULL);
    if (!d)
        return 0;
    uint32_t n = rd16(d), bytes = n * (uint32_t)size * (uint32_t)size / 2;
    if (count)
        *count = (int)n;
    if (buf && room >= bytes)
        copy(buf, d + 4, bytes);
    return bytes;
}

uint32_t zxv_gfxf_map(const zxv_gfxfile *f, int num, zxv_tilemap *m, const void *tiles,
                      void *buf, uint32_t room)
{
    const uint8_t *d = zxv_gfxf_find(f, ZXV_GFXF_MAP, num, NULL);
    if (!d)
        return 0;
    uint32_t w = rd16(d), h = rd16(d + 2), bytes = w * h * 2;
    *m = (zxv_tilemap){ buf, (uint16_t)w, (uint16_t)h, tiles, d[4], d[5] ? ZXV_MAP_WRAP : 0 };
    if (buf && room >= bytes) {
        uint16_t *c = buf;
        for (uint32_t i = 0; i < w * h; i++)
            c[i] = (uint16_t)rd16(d + 8 + 2 * i);
    }
    return bytes;
}

uint32_t zxv_gfxf_texture(const zxv_gfxfile *f, int num, zxv_m7tex *t, void *buf, uint32_t room)
{
    const uint8_t *d = zxv_gfxf_find(f, ZXV_GFXF_TEXTURE, num, NULL);
    if (!d)
        return 0;
    uint32_t bytes = 1u << (2 * d[0]);
    *t = (zxv_m7tex){ buf, d[0], d[1], d[2], 0 };
    if (buf && room >= bytes)
        copy(buf, d + 4, bytes);
    return bytes;
}
