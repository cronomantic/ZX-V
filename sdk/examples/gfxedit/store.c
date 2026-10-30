/*
 * store.c - Los gráficos del editor en memoria, el fichero .gfx y deshacer
 * (ver store.h).
 */
#include "store.h"

obj_t objs[N_SLOTS];

/* La zona, alineada a 4 bytes (los bitmaps del motor lo necesitan). */
static union {
    uint32_t align[POOL_SIZE / 4];
    uint8_t b[POOL_SIZE];
} zone;
#define pool zone.b
static uint32_t used;

static uint32_t round4(uint32_t n) { return (n + 3) & ~3u; }

uint8_t *obj_data(int slot) { return pool + objs[slot].off; }
int obj_exists(int slot) { return objs[slot].len != 0; }
uint32_t pool_free(void) { return POOL_SIZE - used; }

int obj_resize(int slot, uint32_t len)
{
    obj_t *o = &objs[slot];
    if (!len) {
        obj_delete(slot);
        return 0;
    }
    if (!o->len) {
        if (round4(len) > POOL_SIZE - used)
            return -1;
        o->off = used;
        o->len = len;
        memset(pool + used, 0, round4(len));
        used += round4(len);
        return 0;
    }
    uint32_t old = round4(o->len), now = round4(len), end = o->off + old;
    if (now > old && now - old > POOL_SIZE - used)
        return -1;
    memmove(pool + o->off + now, pool + end, used - end);
    for (int i = 0; i < N_SLOTS; i++)
        if (objs[i].len && objs[i].off > o->off)
            objs[i].off = objs[i].off + now - old;
    used = used + now - old;
    if (len > o->len)
        memset(pool + o->off + o->len, 0, now - o->len);
    o->len = len;
    return 0;
}

void obj_delete(int slot)
{
    obj_t *o = &objs[slot];
    if (!o->len)
        return;
    uint32_t sz = round4(o->len), end = o->off + sz;
    memmove(pool + o->off, pool + end, used - end);
    for (int i = 0; i < N_SLOTS; i++)
        if (objs[i].len && objs[i].off > o->off)
            objs[i].off -= sz;
    used -= sz;
    memset(o, 0, sizeof *o);
}

void store_clear(void)
{
    memset(objs, 0, sizeof objs);
    used = 0;
    undo_clear();
}

/* Cambia filas de s1 bytes (h1 filas) por filas de s2 bytes (h2 filas): lo
 * de arriba a la izquierda se queda; lo nuevo es fill. */
static int relayout(int slot, uint32_t s1, uint32_t h1, uint32_t s2, uint32_t h2, uint8_t fill)
{
    uint32_t l1 = objs[slot].len ? s1 * h1 : 0, l2 = s2 * h2;
    if (!objs[slot].len) {
        if (obj_resize(slot, l2))
            return -1;
        memset(obj_data(slot), fill, l2);
        return 0;
    }
    if (l2 > l1 && obj_resize(slot, l2))
        return -1;
    uint8_t *p = obj_data(slot);
    uint32_t rows = h1 < h2 ? h1 : h2, keep = s1 < s2 ? s1 : s2;
    if (s2 > s1) {
        for (uint32_t y = rows; y-- > 0;)
            memmove(p + y * s2, p + y * s1, keep);
    } else if (s2 < s1) {
        for (uint32_t y = 0; y < rows; y++)
            memmove(p + y * s2, p + y * s1, keep);
    }
    for (uint32_t y = 0; y < rows; y++)
        memset(p + y * s2 + keep, fill, s2 - keep);
    if (h2 > rows)
        memset(p + rows * s2, fill, (h2 - rows) * s2);
    if (l2 < l1)
        obj_resize(slot, l2);
    return 0;
}

/* Un bitmap con su nuevo tamaño: los píxeles nuevos, del color c. */
static int image_size(int slot, int w, int h, int c)
{
    obj_t *o = &objs[slot];
    int w1 = o->len ? o->w : 0, h1 = o->len ? o->h : 0;
    if (relayout(slot, stride_of(w1), h1, stride_of(w), h, (uint8_t)(c | c << 4)))
        return -1;
    o->w = (uint16_t)w;
    o->h = (uint16_t)h;
    /* A la derecha del ancho, hasta el final de la fila, siempre el color c
     * (al estrecharlo se quedaban ahí los píxeles de antes). */
    img_t im = { obj_data(slot), stride_of(w), w, h };
    int from = w < w1 ? w : w1;
    for (int y = 0; y < h1 && y < h; y++)
        for (int x = from; x < (int)im.stride * 2; x++)
            if (x >= w || x >= w1)
                img_set(&im, x, y, c);
    return 0;
}

int sprite_size(int n, int w, int h)
{
    return image_size(SLOT_SPRITE + n, w, h, TRANSP);
}

int texture_size(int t, int log2)
{
    int slot = SLOT_TEX + t;
    if (image_size(slot, 1 << log2, 1 << log2, 0))
        return -1;
    objs[slot].a = (uint8_t)log2;
    return 0;
}

int tiles_count(int s, int count)
{
    int slot = tiles_slot(s);
    if (obj_resize(slot, (uint32_t)count * s * s / 2))
        return -1;
    objs[slot].w = (uint16_t)s;
    objs[slot].h = (uint16_t)count;
    return 0;
}

int map_size(int m, int w, int h, int tile)
{
    int slot = SLOT_MAP + m;
    obj_t *o = &objs[slot];
    if (relayout(slot, o->len ? o->w * 2u : 0, o->len ? o->h : 0, w * 2u, h, 0))
        return -1;
    o->w = (uint16_t)w;
    o->h = (uint16_t)h;
    o->a = (uint8_t)tile;
    return 0;
}

int obj_image(int slot, int tile, img_t *im)
{
    obj_t *o = &objs[slot];
    if (!o->len)
        return -1;
    if (slot == SLOT_TILES8 || slot == SLOT_TILES16) {
        if (tile < 0 || tile >= o->h)
            return -1;
        int s = o->w;
        im->pix = obj_data(slot) + (uint32_t)tile * s * s / 2;
        im->stride = s / 2;
        im->w = im->h = s;
    } else {
        im->pix = obj_data(slot);
        im->stride = stride_of(o->w);
        im->w = o->w;
        im->h = o->h;
    }
    return 0;
}

/* --- El fichero .gfx ----------------------------------------------------------
 *
 * "ZXVG", versión (1), 0, número de objetos (u16); cada objeto: tipo (u8), 0,
 * número (u16), longitud (u32) y los datos (ver rom/src/basic/bgfx.c):
 *   1 sprite   ancho, alto (u16) y las filas de (ancho + 1) / 2 bytes
 *   2 tiles    (el número es el lado) cuántos (u16), 0 (u16) y los tiles
 *   3 mapa     ancho, alto (u16), lado, repetición (u8), 0 (u16), las celdas
 *   4 textura  log2 del lado, repetición, color de fuera, 0 y los texels
 */

static uint32_t get16(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8; }
static uint32_t get32(const uint8_t *p) { return get16(p) | get16(p + 2) << 16; }

static gfx_io_t io;
static int io_err;

static void rd(void *buf, uint32_t n)
{
    if (!io_err && io(buf, n) != (int)n)
        io_err = 1;
    if (io_err)
        memset(buf, 0, n);
}

static void wr(const void *buf, uint32_t n)
{
    if (!io_err && io((void *)buf, n) != (int)n)
        io_err = 1;
}

/* Un objeto del fichero, leído directamente a su sitio; 0, GFX_BAD o
 * GFX_FULL. Se comprueba como en el BASIC. */
static int load_object(int kind, uint32_t num, uint32_t len)
{
    uint8_t h[8];
    img_t im = { 0 };
    switch (kind) {
    case 1: {
        if (num >= N_SPRITES || len < 4)
            return GFX_BAD;
        rd(h, 4);
        int w = (int)get16(h), ht = (int)get16(h + 2), rb = (w + 1) / 2;
        if (w < 1 || w > 1024 || ht < 1 || ht > 1024 || len != 4 + (uint32_t)ht * rb)
            return GFX_BAD;
        obj_delete(SLOT_SPRITE + num);
        if (sprite_size((int)num, w, ht))
            return GFX_FULL;
        obj_image(SLOT_SPRITE + num, 0, &im);
        for (int y = 0; y < ht; y++)
            rd(im.pix + y * im.stride, rb);
        if (w & 1)                          /* el píxel de relleno, transparente */
            for (int y = 0; y < ht; y++)
                img_set(&im, w, y, TRANSP);
        return 0;
    }
    case 2: {
        if ((num != 8 && num != 16) || len < 4)
            return GFX_BAD;
        rd(h, 4);
        uint32_t n = get16(h);
        if (n < 1 || n > MAX_TILES || len != 4 + n * num * num / 2)
            return GFX_BAD;
        if (tiles_count((int)num, (int)n))
            return GFX_FULL;
        rd(obj_data(tiles_slot((int)num)), n * num * num / 2);
        return 0;
    }
    case 3: {
        if (num >= N_MAPS || len < 8)
            return GFX_BAD;
        rd(h, 8);
        uint32_t w = get16(h), ht = get16(h + 2);
        if (w < 1 || w > 32767 || ht < 1 || ht > 32767 || (h[4] != 8 && h[4] != 16) || h[5] > 1 ||
            len != 8 + w * ht * 2)
            return GFX_BAD;
        obj_delete(SLOT_MAP + num);
        if (map_size((int)num, (int)w, (int)ht, h[4]))
            return GFX_FULL;
        objs[SLOT_MAP + num].b = h[5];
        uint8_t *c = obj_data(SLOT_MAP + num);
        rd(c, w * ht * 2);
        for (uint32_t i = 0; i < w * ht; i++) {     /* de little-endian a u16 */
            uint16_t v = (uint16_t)get16(c + 2 * i);
            memcpy(c + 2 * i, &v, 2);
        }
        return 0;
    }
    case 4: {
        if (num >= N_TEXTURES || len < 4)
            return GFX_BAD;
        rd(h, 4);
        int l2 = h[0], side = 1 << (l2 & 15);
        if (l2 < 1 || l2 > 8 || h[1] > 1 || h[2] > 15 || len != 4 + (uint32_t)side * side)
            return GFX_BAD;
        obj_delete(SLOT_TEX + num);
        if (texture_size((int)num, l2))
            return GFX_FULL;
        objs[SLOT_TEX + num].b = h[1];
        objs[SLOT_TEX + num].c = h[2];
        obj_image(SLOT_TEX + num, 0, &im);
        uint8_t row[256];
        for (int y = 0; y < side; y++) {
            rd(row, (uint32_t)side);
            for (int x = 0; x < side; x++) {
                if (row[x] > 15)
                    return GFX_BAD;
                img_set(&im, x, y, row[x]);
            }
        }
        return 0;
    }
    }
    return GFX_BAD;
}

int gfx_load(gfx_io_t read)
{
    uint8_t h[8];
    io = read;
    io_err = 0;
    undo_clear();
    rd(h, 8);
    if (io_err || memcmp(h, "ZXVG", 4) || h[4] != 1)
        return GFX_BAD;
    uint32_t count = get16(h + 6);
    for (uint32_t i = 0; i < count; i++) {
        rd(h, 8);
        if (io_err || h[1])
            return GFX_BAD;
        int r = load_object(h[0], get16(h + 2), get32(h + 4));
        if (io_err)
            return GFX_BAD;
        if (r)
            return r;
    }
    return 0;
}

static void put_record(int kind, int num, uint32_t len)
{
    uint8_t h[8] = { (uint8_t)kind, 0, (uint8_t)num, (uint8_t)(num >> 8),
                     (uint8_t)len, (uint8_t)(len >> 8), (uint8_t)(len >> 16), (uint8_t)(len >> 24) };
    wr(h, 8);
}

int gfx_save(gfx_io_t write, uint32_t *size)
{
    uint32_t count = 0, total = 8;
    img_t im;
    for (int s = 0; s < N_SLOTS; s++) {
        obj_t *o = &objs[s];
        if (!o->len)
            continue;
        count++;
        if (s < N_SPRITES)
            total += 12 + o->h * ((o->w + 1u) / 2);
        else if (s == SLOT_TILES8 || s == SLOT_TILES16)
            total += 12 + o->len;
        else if (s < SLOT_TEX)
            total += 16 + o->len;
        else
            total += 12 + (uint32_t)o->w * o->h;
    }
    if (size)
        *size = total;
    if (!write)
        return 0;
    io = write;
    io_err = 0;
    uint8_t h[8] = { 'Z', 'X', 'V', 'G', 1, 0, (uint8_t)count, (uint8_t)(count >> 8) };
    wr(h, 8);
    /* En el orden de png2gfx.py: sprites, tiles de 8 y de 16, mapas y texturas. */
    for (int n = 0; n < N_SPRITES; n++) {
        if (obj_image(SLOT_SPRITE + n, 0, &im))
            continue;
        uint32_t rb = (im.w + 1) / 2;
        put_record(1, n, 4 + im.h * rb);
        uint8_t wh[4] = { (uint8_t)im.w, (uint8_t)(im.w >> 8), (uint8_t)im.h, (uint8_t)(im.h >> 8) };
        wr(wh, 4);
        for (int y = 0; y < im.h; y++) {
            uint8_t *row = im.pix + y * im.stride;
            if (im.w & 1) {                 /* el píxel de relleno, transparente */
                uint8_t last = (uint8_t)((row[rb - 1] & 0x0F) | TRANSP << 4);
                wr(row, rb - 1);
                wr(&last, 1);
            } else {
                wr(row, rb);
            }
        }
    }
    for (int s = 8; s <= 16; s += 8) {
        obj_t *o = &objs[tiles_slot(s)];
        if (!o->len)
            continue;
        put_record(2, s, 4 + o->len);
        uint8_t c[4] = { (uint8_t)o->h, (uint8_t)(o->h >> 8), 0, 0 };
        wr(c, 4);
        wr(obj_data(tiles_slot(s)), o->len);
    }
    for (int m = 0; m < N_MAPS; m++) {
        obj_t *o = &objs[SLOT_MAP + m];
        if (!o->len)
            continue;
        put_record(3, m, 8 + o->len);
        uint8_t mh[8] = { (uint8_t)o->w, (uint8_t)(o->w >> 8), (uint8_t)o->h, (uint8_t)(o->h >> 8),
                          o->a, o->b, 0, 0 };
        wr(mh, 8);
        const uint8_t *c = obj_data(SLOT_MAP + m);
        for (uint32_t i = 0; i < o->len / 2; i++) {
            uint16_t v;
            memcpy(&v, c + 2 * i, 2);
            uint8_t b[2] = { (uint8_t)v, (uint8_t)(v >> 8) };
            wr(b, 2);
        }
    }
    for (int t = 0; t < N_TEXTURES; t++) {
        obj_t *o = &objs[SLOT_TEX + t];
        if (obj_image(SLOT_TEX + t, 0, &im))
            continue;
        put_record(4, t, 4 + (uint32_t)im.w * im.h);
        uint8_t th[4] = { o->a, o->b, o->c, 0 };
        wr(th, 4);
        uint8_t row[256];
        for (int y = 0; y < im.h; y++) {
            for (int x = 0; x < im.w; x++)
                row[x] = (uint8_t)img_get(&im, x, y);
            wr(row, (uint32_t)im.w);
        }
    }
    return io_err ? -1 : 0;
}

/* --- Deshacer ---------------------------------------------------------------- */

#define UNDO_SIZE   (16u * 1024u)
#define UNDO_MAX    48

typedef struct {
    uint32_t at, off, len;              /* dónde está en ubuf, y qué guarda */
    uint16_t slot, w, h;
    uint8_t a, b, c, whole;
} undo_t;

static uint8_t ubuf[UNDO_SIZE];
static undo_t ustack[UNDO_MAX];
static int ucount;
static uint32_t uused;

void undo_clear(void)
{
    ucount = 0;
    uused = 0;
}

static void drop_oldest(void)
{
    uint32_t sz = round4(ustack[0].len);
    memmove(ubuf, ubuf + sz, uused - sz);
    uused -= sz;
    for (int i = 1; i < ucount; i++) {
        ustack[i - 1] = ustack[i];
        ustack[i - 1].at -= sz;
    }
    ucount--;
}

void undo_push(int slot, uint32_t off, uint32_t len)
{
    obj_t *o = &objs[slot];
    int whole = len == 0xFFFFFFFFu;
    if (whole) {
        off = 0;
        len = o->len;
    }
    if (round4(len) > UNDO_SIZE) {      /* no cabe: lo de antes ya no vale */
        undo_clear();
        return;
    }
    while (ucount && (ucount == UNDO_MAX || UNDO_SIZE - uused < round4(len)))
        drop_oldest();
    undo_t *u = &ustack[ucount++];
    u->at = uused;
    u->off = off;
    u->len = len;
    u->slot = (uint16_t)slot;
    u->w = o->w;
    u->h = o->h;
    u->a = o->a;
    u->b = o->b;
    u->c = o->c;
    u->whole = (uint8_t)whole;
    if (len)
        memcpy(ubuf + uused, obj_data(slot) + off, len);
    uused += round4(len);
}

int undo_pop(void)
{
    if (!ucount)
        return -1;
    undo_t *u = &ustack[--ucount];
    obj_t *o = &objs[u->slot];
    if (u->whole) {
        if (!u->len) {
            obj_delete(u->slot);
        } else if (obj_resize(u->slot, u->len) == 0) {
            memcpy(obj_data(u->slot), ubuf + u->at, u->len);
            o->w = u->w;
            o->h = u->h;
            o->a = u->a;
            o->b = u->b;
            o->c = u->c;
        }
    } else if (o->len >= u->off + u->len) {
        memcpy(obj_data(u->slot) + u->off, ubuf + u->at, u->len);
    }
    uused = u->at;
    return u->slot;
}
