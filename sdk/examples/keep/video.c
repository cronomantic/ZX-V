/*
 * video.c - La pantalla de The Keep: modo 0 (256 x 192) con doble buffer, el
 * texto con la fuente de keep.gfx (con las letras del español), los sprites
 * y los recuadros de los mensajes.
 */
#include "keep.h"

/* --- El doble buffer --------------------------------------------------------- */

static int visible;
static uint32_t last_frame, lost;

static uint32_t frames_now(void) { return *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES; }

void video_init(void)
{
    zxv_set_mode(0);
    visible = 0;
    zxv_set_page(0);
    zxv_wait_vblank();                  /* el primer frame, entero */
    last_frame = frames_now();
    lost = 0;
}

void video_begin(void)
{
    zxv_gfx_draw_page(1 - visible);
    zxv_gfx_target(0);
    zxv_gfx_clip(0, 0, W, H);
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

uint32_t video_lost(void) { return lost; }

/* --- El texto ---------------------------------------------------------------- */

/* La fuente es el sprite S_FONT de keep.gfx: glifos de 6 x 9 seguidos, en
 * este orden, con la letra en las 7 filas de abajo (el 15 es la tinta); los
 * tres últimos son las marcas de encima de la letra: acento, tilde y
 * diéresis. El * es el signo de multiplicar. */
static const char font_order[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!?.,:-/'+()*";
enum { G_INV_EXCL = sizeof(font_order) - 1, G_INV_QUEST, G_ACUTE, G_TILDE, G_DIAERESIS };

static uint8_t ink[16][16];             /* el 15 del glifo en cada color */

static int mark;

/* El glifo del siguiente carácter de s (en UTF-8), y lo avanza; la marca de
 * encima, en mark (0 si no lleva). */
static int glyph(const char **s)
{
    unsigned c = (unsigned char)*(*s)++;
    mark = 0;
    if (c >= 'a' && c <= 'z')
        c -= 32;
    if (c < 0x80) {
        for (int i = 0; font_order[i]; i++)
            if ((unsigned char)font_order[i] == c)
                return i;
        return 0;
    }
    if ((c == 0xC2 || c == 0xC3) && **s) {
        unsigned d = (unsigned char)*(*s)++ | (c == 0xC2 ? 0 : 0x40);
        if (d >= 0xE0)
            d -= 0x20;                  /* á -> Á... */
        static const struct { uint8_t code; char base; uint8_t mark; } acc[] = {
            { 0xC1, 'A', G_ACUTE }, { 0xC9, 'E', G_ACUTE }, { 0xCD, 'I', G_ACUTE },
            { 0xD3, 'O', G_ACUTE }, { 0xDA, 'U', G_ACUTE }, { 0xD1, 'N', G_TILDE },
            { 0xDC, 'U', G_DIAERESIS },
        };
        if (d == 0xA1)
            return G_INV_EXCL;
        if (d == 0xBF)
            return G_INV_QUEST;
        for (unsigned i = 0; i < sizeof(acc) / sizeof(acc[0]); i++)
            if (acc[i].code == d) {
                mark = acc[i].mark;
                return 11 + acc[i].base - 'A';
            }
    }
    while ((**s & 0xC0) == 0x80)        /* lo demás, un espacio */
        (*s)++;
    return 0;
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

/* y es la línea de arriba de las letras; las marcas van en las dos de encima. */
void text(int x, int y, const char *s, int c)
{
    if (!ink[1][15])
        for (int k = 0; k < 16; k++)
            for (int i = 0; i < 16; i++)
                ink[k][i] = (uint8_t)(i == 15 ? k : i);
    while (*s) {
        int g = glyph(&s);
        const void *gl = art_glyph(g);
        if (g && gl)
            zxv_spr_draw(gl, x, y - 2, 0, ink[c]);
        if (mark && art_glyph(mark))
            zxv_spr_draw(art_glyph(mark), x, y - 2, 0, ink[c]);
        x += 6;
    }
}

void text_shadow(int x, int y, const char *s, int c)
{
    text(x + 1, y + 1, s, 0);
    text(x, y, s, c);
}

void text_center(int y, const char *s, int c) { text_shadow((W - text_width(s)) / 2, y, s, c); }

void text_big(int x, int y, const char *s, int sz, int c)
{
    while (*s) {
        int g = glyph(&s);
        int m = mark;
        for (int pass = 0; pass < 2; pass++) {      /* la sombra y la letra */
            const uint8_t *col = ink[pass ? c : 0];
            int d = pass ? 0 : sz;
            if (g && art_glyph(g))
                zxv_spr_draw_scaled(art_glyph(g), x + d, y - 2 * sz + d, 6 * sz, 9 * sz, 0, col);
            if (m && art_glyph(m))
                zxv_spr_draw_scaled(art_glyph(m), x + d, y - 2 * sz + d, 6 * sz, 9 * sz, 0, col);
        }
        x += 6 * sz;
    }
}

void number(char *out, uint32_t v, int digits)
{
    out[digits] = 0;
    for (int i = digits - 1; i >= 0; i--, v /= 10)
        out[i] = (char)('0' + v % 10);
}

char *append(char *out, const char *s)
{
    while (*s)
        *out++ = *s++;
    *out = 0;
    return out;
}

void box(int x, int y, int w, int h)
{
    zxv_gfx_rect(x, y, w, h, 0);
    zxv_gfx_rect(x + 1, y + 1, w - 2, 1, 15);
    zxv_gfx_rect(x + 1, y + h - 2, w - 2, 1, 15);
    zxv_gfx_rect(x + 1, y + 1, 1, h - 2, 15);
    zxv_gfx_rect(x + w - 2, y + 1, 1, h - 2, 15);
    zxv_gfx_rect(x + 3, y + 3, w - 6, 1, 1);
    zxv_gfx_rect(x + 3, y + h - 4, w - 6, 1, 1);
}

/* --- Los sprites --------------------------------------------------------------- */

void draw_sprite(const sprite_t *s, int x, int y, int flags, const uint8_t *colours)
{
    if (s->spr)
        zxv_spr_draw(s->spr, x, y, flags, colours);
}

void draw_scaled(const sprite_t *s, int x, int y, int w, int h, int flags, const uint8_t *colours)
{
    if (!s->spr || w < 1 || h < 1)
        return;
    if (w == s->w && h == s->h)
        zxv_spr_draw(s->spr, x, y, flags, colours);
    else
        zxv_spr_draw_scaled(s->spr, x, y, w, h, flags, colours);
}
