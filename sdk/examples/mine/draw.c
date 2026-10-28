/*
 * draw.c - La pantalla de la mina: modo 0 (256 x 192) con doble buffer, una
 * fuente de 5 x 7 con las letras del español y los sprites.
 */
#include "mine.h"

/* --- El doble buffer --------------------------------------------------------- */

static uint8_t *page;                   /* la página de dibujo */
static int visible;
static uint32_t last_frame, lost, frame_start, busy_max;

static uint32_t frames_now(void) { return *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES; }

void video_init(void)
{
    zxv_set_mode(0);
    visible = 0;
    zxv_set_page(0);
    last_frame = frames_now();
    lost = 0;
}

/* Sin borrar: la cueva tapa toda la pantalla. */
void video_begin(void)
{
    int draw = 1 - visible;
    zxv_gfx_draw_page(draw);
    zxv_gfx_target(0);
    zxv_gfx_clip(0, 0, W, H);
    page = (uint8_t *)(uintptr_t)(draw ? ZXV_VRAM1_BASE : ZXV_VRAM0_BASE);
}

void video_end(void)
{
    int draw = 1 - visible;
    uint32_t busy = zxv_time_us() - frame_start;
    if (busy > busy_max)
        busy_max = busy;
    zxv_set_page(draw);                 /* se ve desde el VBLANK */
    zxv_wait_vblank();
    frame_start = zxv_time_us();
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

uint32_t video_lost(void) { return lost; }

uint32_t video_busy(int reset)
{
    uint32_t b = busy_max;
    if (reset) {
        busy_max = 0;
        frame_start = zxv_time_us();
    }
    return b;
}

static void pset(int x, int y, int c)
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
    { 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02 },   /* < */
    { 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08 },   /* > */
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
        if (c == '<' || c == '>')
            i = c == '<' ? 49 : 50;
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

static uint32_t pool[8192];             /* los sprites preparados (32 KB) */
static uint32_t pool_used;

sprite_t sprite_from(const zxv_bitmap *b, int x, int y, int w, int h)
{
    sprite_t s = { 0, (uint8_t)w, (uint8_t)h };
    uint32_t bytes = zxv_spr_prepare(b, x, y, w, h, 0);
    if (pool_used + (bytes + 3) / 4 > sizeof(pool) / 4)
        return s;                       /* no cabe: no se dibuja */
    zxv_spr_prepare(b, x, y, w, h, &pool[pool_used]);
    s.spr = &pool[pool_used];
    pool_used += (bytes + 3) / 4;
    return s;
}

sprite_t sprite_ascii(const char *const *rows, int w, int h)
{
    static uint32_t mem[32 * 32 / 8];
    zxv_bitmap b = { (uint8_t *)mem, (uint16_t)((w + 7) / 8 * 4), (uint16_t)w, (uint16_t)h, 0 };
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            char ch = rows[y][x];
            int c = ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : ZXV_TRANSPARENT;
            uint8_t *p = b.pix + y * b.stride + (x >> 1);
            *p = x & 1 ? (uint8_t)((*p & 0x0F) | c << 4) : (uint8_t)((*p & 0xF0) | c);
        }
    return sprite_from(&b, 0, 0, w, h);
}

void draw_sprite(const sprite_t *s, int x, int y, int flags, const uint8_t *colours)
{
    if (s->spr)
        zxv_spr_draw(s->spr, x, y, flags, colours);
}
