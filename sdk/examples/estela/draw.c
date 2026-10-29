/*
 * draw.c - La pantalla de Estela: modo 0 (256 x 192) con doble buffer, una
 * fuente de 5 x 7 con las letras del español, sprites hechos con caracteres o
 * dibujados con el motor, y las estrellas del fondo.
 */
#include "estela.h"

/* --- El doble buffer --------------------------------------------------------- */

static uint8_t *page;                   /* la página de dibujo */
static int visible;
static uint32_t last_frame, lost;

static uint32_t frames_now(void) { return *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES; }

void video_init(void)
{
    zxv_set_mode(0);
    visible = 0;
    zxv_set_page(0);
    last_frame = frames_now();
    lost = 0;
}

void video_begin(void)
{
    int draw = 1 - visible;
    zxv_gfx_draw_page(draw);
    zxv_gfx_target(0);
    zxv_gfx_clip(0, 0, W, H);
    page = (uint8_t *)(uintptr_t)(draw ? ZXV_VRAM1_BASE : ZXV_VRAM0_BASE);
    zxv_gfx_rect(0, 0, W, H, 0);
}

void video_end(void)
{
    int draw = 1 - visible;
    zxv_set_page(draw);                 /* se ve desde el VBLANK */
    zxv_wait_vblank();
    visible = draw;
    uint32_t now = frames_now();
    if (now - last_frame > 1)
        lost += now - last_frame - 1;
    last_frame = now;
}

void video_done(void)
{
    zxv_set_page(0);
    zxv_gfx_draw_page(0);
    zxv_gfx_target(0);
}

uint32_t video_frames(void) { return lost; }

void pset(int x, int y, int c)
{
    if ((unsigned)x >= W || (unsigned)y >= H)
        return;
    uint8_t *p = page + y * (W / 2) + (x >> 1);
    *p = x & 1 ? (uint8_t)((*p & 0x0F) | c << 4) : (uint8_t)((*p & 0xF0) | c);
}

/* --- El texto ---------------------------------------------------------------- */

/* 5 x 7, una fila por byte (el bit 4 es la columna de la izquierda). */
static const uint8_t font[][7] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   /*   */
    { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E },   /* 0 */
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },   /* 1 */
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F },   /* 2 */
    { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E },   /* 3 */
    { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 },   /* 4 */
    { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E },   /* 5 */
    { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E },   /* 6 */
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },   /* 7 */
    { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E },   /* 8 */
    { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C },   /* 9 */
    { 0x0E, 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11 },   /* A */
    { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E },   /* B */
    { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E },   /* C */
    { 0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C },   /* D */
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F },   /* E */
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 },   /* F */
    { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F },   /* G */
    { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 },   /* H */
    { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E },   /* I */
    { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C },   /* J */
    { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 },   /* K */
    { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F },   /* L */
    { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 },   /* M */
    { 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11 },   /* N */
    { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E },   /* O */
    { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 },   /* P */
    { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D },   /* Q */
    { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 },   /* R */
    { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E },   /* S */
    { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },   /* T */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E },   /* U */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 },   /* V */
    { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A },   /* W */
    { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 },   /* X */
    { 0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04 },   /* Y */
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F },   /* Z */
    { 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x04 },   /* ! */
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },   /* ? */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C },   /* . */
    { 0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08 },   /* , */
    { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 },   /* : */
    { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 },   /* - */
    { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00 },   /* / */
    { 0x0C, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00 },   /* ' */
    { 0x04, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04 },   /* ¡ */
    { 0x04, 0x00, 0x04, 0x08, 0x10, 0x11, 0x0E },   /* ¿ */
    { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 },   /* + */
    { 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00 },   /* x (el aspa de las vidas) */
};

/* Las marcas sobre la letra (en las dos filas de encima): acento, tilde y diéresis. */
enum { M_NONE, M_ACUTE, M_TILDE, M_DIAERESIS };
static const uint8_t marks[4][2] = { { 0, 0 }, { 0x02, 0x04 }, { 0x0D, 0x16 }, { 0x00, 0x0A } };
static int mark;

/* El glifo del siguiente carácter de s (en UTF-8), y lo avanza; la marca de
 * encima, en mark. */
static const uint8_t *glyph(const char **s)
{
    unsigned c = (unsigned char)*(*s)++;
    int i = 0;
    mark = M_NONE;
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c >= '0' && c <= '9')
        i = 1 + (int)(c - '0');
    else if (c >= 'A' && c <= 'Z')
        i = 11 + (int)(c - 'A');
    else if (c < 0x80) {
        static const char other[] = "!?.,:-/'";
        for (int k = 0; other[k]; k++)
            if ((unsigned)other[k] == c)
                i = 37 + k;
        if (c == '+')
            i = 47;
        if (c == '*')
            i = 48;
    } else if ((c == 0xC2 || c == 0xC3) && **s) {
        unsigned d = (unsigned char)*(*s)++ | (c == 0xC2 ? 0 : 0x40);
        if (d >= 0xE0)
            d -= 0x20;                  /* á -> Á... */
        switch (d) {
        case 0xA1: i = 45; break;       /* ¡ */
        case 0xBF: i = 46; break;       /* ¿ */
        case 0xC1: i = 11; mark = M_ACUTE; break;               /* Á */
        case 0xC9: i = 11 + 'E' - 'A'; mark = M_ACUTE; break;   /* É */
        case 0xCD: i = 11 + 'I' - 'A'; mark = M_ACUTE; break;   /* Í */
        case 0xD3: i = 11 + 'O' - 'A'; mark = M_ACUTE; break;   /* Ó */
        case 0xDA: i = 11 + 'U' - 'A'; mark = M_ACUTE; break;   /* Ú */
        case 0xD1: i = 11 + 'N' - 'A'; mark = M_TILDE; break;   /* Ñ */
        case 0xDC: i = 11 + 'U' - 'A'; mark = M_DIAERESIS; break; /* Ü */
        default: break;
        }
    }
    return font[i];
}

int text_width(const char *s)
{
    int n = 0;
    while (*s) {
        glyph(&s);
        n++;
    }
    return n ? n * 6 - 1 : 0;
}

void text(int x, int y, const char *s, int c)
{
    while (*s) {
        const uint8_t *g = glyph(&s);
        for (int r = -2; r < 7; r++)
            for (int k = 0; k < 5; k++)
                if ((r < 0 ? marks[mark][r + 2] : g[r]) & (0x10 >> k))
                    pset(x + k, y + r, c);
        x += 6;
    }
}

void text_center(int y, const char *s, int c) { text((W - text_width(s)) / 2, y, s, c); }

void label(int y, const char *s, int c)
{
    int w = text_width(s);
    zxv_gfx_rect((W - w) / 2 - 3, y - 3, w + 6, 12, 0);
    text((W - w) / 2, y, s, c);
}

void text_big(int x, int y, const char *s, int sz, const uint8_t *row_colours)
{
    while (*s) {
        const uint8_t *g = glyph(&s);
        for (int r = 0; r < 7; r++)
            for (int k = 0; k < 5; k++)
                if (g[r] & (0x10 >> k))
                    zxv_gfx_rect(x + k * sz, y + r * sz, sz, sz, row_colours[r]);
        x += 6 * sz;
    }
}

void number(char *out, uint32_t v, int digits)
{
    out[digits] = 0;
    for (int i = digits - 1; i >= 0; i--, v /= 10)
        out[i] = (char)('0' + v % 10);
}

/* --- Los sprites --------------------------------------------------------------- */

static uint32_t pool[12288];            /* los sprites preparados (48 KB) */
static uint32_t pool_used;
static uint32_t canvas_mem[64 * 64 / 8];
static zxv_bitmap canvas_bmp;

static sprite_t prepare(int w, int h)
{
    sprite_t s = { 0, (uint8_t)w, (uint8_t)h };
    uint32_t bytes = zxv_spr_prepare(&canvas_bmp, 0, 0, w, h, 0);
    if (pool_used + (bytes + 3) / 4 > sizeof(pool) / 4)
        return s;                       /* no cabe: no se dibuja */
    zxv_spr_prepare(&canvas_bmp, 0, 0, w, h, &pool[pool_used]);
    s.spr = &pool[pool_used];
    pool_used += (bytes + 3) / 4;
    return s;
}

void canvas(int w, int h)
{
    canvas_bmp.pix = (uint8_t *)canvas_mem;
    canvas_bmp.stride = (uint16_t)((w + 7) / 8 * 4);
    canvas_bmp.w = (uint16_t)w;
    canvas_bmp.h = (uint16_t)h;
    uint8_t *p = canvas_bmp.pix;
    for (int i = 0; i < canvas_bmp.stride * h; i++)
        p[i] = 0x88;                    /* todo transparente */
    zxv_gfx_target(&canvas_bmp);
    zxv_gfx_clip(0, 0, w, h);
}

sprite_t canvas_sprite(void)
{
    zxv_gfx_target(0);
    zxv_gfx_clip(0, 0, W, H);
    return prepare(canvas_bmp.w, canvas_bmp.h);
}

sprite_t sprite_ascii(const char *const *rows, int w, int h)
{
    canvas(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char ch = rows[y][x];
            int c = ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : ZXV_TRANSPARENT;
            uint8_t *p = canvas_bmp.pix + y * canvas_bmp.stride + (x >> 1);
            *p = x & 1 ? (uint8_t)((*p & 0x0F) | c << 4) : (uint8_t)((*p & 0xF0) | c);
        }
    return canvas_sprite();
}

void draw_sprite(const sprite_t *s, int x, int y, int flags, const uint8_t *colours)
{
    if (s->spr)
        zxv_spr_draw(s->spr, x, y, flags, colours);
}

int sprites_touch(const sprite_t *a, int ax, int ay, const sprite_t *b, int bx, int by)
{
    if (ax + a->w <= bx || bx + b->w <= ax || ay + a->h <= by || by + b->h <= ay)
        return 0;
    return a->spr && b->spr && zxv_spr_collide(a->spr, ax, ay, b->spr, bx, by);
}

/* --- Las estrellas ------------------------------------------------------------- */

#define STARS       60

static struct { int x; uint8_t y, layer; } stars[STARS];

void stars_init(void)
{
    for (int i = 0; i < STARS; i++) {
        stars[i].x = (int)(rnd() % W) * FX;
        stars[i].y = (uint8_t)(HUD_H + rnd() % (H - HUD_H));
        stars[i].layer = (uint8_t)(i % 3);
    }
}

void stars_draw(int speed)
{
    static const uint8_t colour[3] = { 1, 5, 15 };
    for (int i = 0; i < STARS; i++) {
        int l = stars[i].layer;
        stars[i].x -= speed >> (2 - l);
        if (stars[i].x < 0) {
            stars[i].x += W * FX;
            stars[i].y = (uint8_t)(HUD_H + rnd() % (H - HUD_H));
        }
        int x = stars[i].x / FX;
        pset(x, stars[i].y, colour[l]);
        if (l == 2 && speed > V(32))
            pset(x + 1, stars[i].y, 7);     /* las cercanas dejan estela si se va deprisa */
    }
}
