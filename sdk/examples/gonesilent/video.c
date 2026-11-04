/*
 * video.c - La pantalla de Gone Silent: modo 0 (256 x 192) o modo 1
 * (512 x 384), con doble buffer. En el modo 1 una pantalla ocupa la página
 * entera de 96 KB, así que también caben las dos.
 *
 * Los colores (spec §4.6 y §4.7, D90): con la tabla por línea, las líneas de
 * la vista toman la paleta de la tabla 1, con los colores de la zona, y las
 * del marcador la de la tabla 0, con los de siempre.
 *
 * La fuente es la de Umbra (5 x 7, con las letras del español); en el modo 1
 * cada punto es de 2 x 2.
 */
#include "gonesilent.h"

int scr_w = 256, scr_h = 192, view_h = 144, hires;

enum { PAL_UI, PAL_VIEW, PALETTES };

static uint8_t saved_flags, saved_remap, saved_border;
static uint8_t *page;                   /* la página de dibujo */
static int visible;
static uint32_t last_frame, lost;

#define REG8(a)     (*(volatile uint8_t *)(uintptr_t)(a))

/* Las tablas y la tabla por línea se escriben cuando el último frame ya se ha
 * enviado (spec §4.8). */
static void wait_link(void)
{
    while (*(volatile uint32_t *)(uintptr_t)ZXV_SV_SYSFLAGS & ZXV_SYSF_LINK_BUSY)
        ;
}

static uint32_t frames_now(void) { return *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES; }

static volatile uint8_t *line_entry(int y)
{
    return (volatile uint8_t *)(uintptr_t)(ZXV_LINE_TABLE + (uint32_t)y * ZXV_LINE_ENTRY_SIZE);
}

void video_init(int mode)
{
    static int first = 1;
    if (first) {
        first = 0;
        saved_flags = REG8(ZXV_V_FLAGS);
        saved_remap = REG8(ZXV_V_REMAP);
        saved_border = REG8(ZXV_V_BORDER);  /* el del BASIC, para volver */
    }
    hires = mode;
    scr_w = mode ? ZXV_MODE1_W : ZXV_MODE0_W;
    scr_h = mode ? ZXV_MODE1_H : ZXV_MODE0_H;
    view_h = scr_h * 3 / 4;
    REG8(ZXV_V_VSIZE) = ZXV_VSIZE_SCREEN;
    zxv_set_mode(mode);
    visible = 0;                            /* no hace falta borrar: cada frame se dibuja entero */
    zxv_set_page(0);
    wait_link();
    REG8(ZXV_V_REMAP) = PAL_UI;             /* el borde, negro hasta el primer frame */
    REG8(ZXV_V_BORDER) = 0;
    for (int t = 0; t < PALETTES; t++) {
        volatile uint8_t *r = (volatile uint8_t *)(uintptr_t)(ZXV_REMAP_TABLES + 16u * (uint32_t)t);
        for (int i = 0; i < 16; i++)
            r[i] = (uint8_t)i;
    }
    /* Los colores del marcador, el visor del traje (main.c, hud): 0 negro,
     * 1 el fondo de los paneles, 3 y 13 sus bordes, 2 rojo, 4 verde, 5 cian,
     * 6 ámbar, 7 el texto, 8 y 12 el sensor (verde oscuro y claro), 9-11 y
     * 14 los de los iconos y 15 blanco. */
    static const uint8_t ui[16][3] = {
        { 0, 0, 0 }, { 18, 24, 32 }, { 230, 60, 44 }, { 58, 70, 84 },
        { 70, 210, 100 }, { 100, 206, 224 }, { 244, 192, 70 }, { 214, 222, 228 },
        { 0, 100, 30 }, { 150, 70, 30 }, { 236, 138, 50 }, { 20, 96, 104 },
        { 90, 230, 140 }, { 124, 140, 158 }, { 92, 104, 118 }, { 250, 250, 240 },
    };
    volatile uint16_t *p = (volatile uint16_t *)(uintptr_t)ZXV_PALETTE(PAL_UI);
    for (int i = 0; i < 16; i++)
        p[i] = ZXV_RGB(ui[i][0], ui[i][1], ui[i][2]);
    int border = ZXV_BORDER_LINES(mode);
    for (int y = 0; y < ZXV_FRAME_LINES(mode); y++) {
        volatile uint8_t *e = line_entry(y);
        for (int i = 0; i < 6; i++)         /* todo como sin la tabla... */
            e[i] = 0xFF;
        e[6] = e[7] = 0;
        /* ...menos la tabla: la de la vista o la del marcador (la global
         * es la del borde: main.c, border_update) */
        e[ZXV_LT_REMAP] = y >= border && y < border + view_h ? PAL_VIEW : PAL_UI;
    }
    REG8(ZXV_V_FLAGS) = (uint8_t)(saved_flags | ZXV_VFLAG_LINETAB);
    zxv_wait_vblank();
    last_frame = frames_now();
    lost = 0;
}

void video_palette(const uint8_t (*rgb)[3])
{
    wait_link();
    volatile uint16_t *p = (volatile uint16_t *)(uintptr_t)ZXV_PALETTE(PAL_VIEW);
    for (int i = 0; i < 16; i++)
        p[i] = ZXV_RGB(rgb[i][0], rgb[i][1], rgb[i][2]);
}

void video_begin(void)
{
    page = (uint8_t *)(uintptr_t)(visible ? ZXV_VRAM0_BASE : ZXV_VRAM1_BASE);
}

void video_end(void)
{
    int draw = 1 - visible;
    zxv_set_page(draw);                     /* se ve desde el VBLANK */
    zxv_wait_vblank();
    visible = draw;
    uint32_t now = frames_now();
    if (now - last_frame > 1)
        lost += now - last_frame - 1;
    last_frame = now;
}

void video_done(void)
{
    zxv_set_mode(0);
    zxv_set_page(0);
    wait_link();
    for (int t = 0; t < 2; t++) {
        volatile uint16_t *p = (volatile uint16_t *)(uintptr_t)ZXV_PALETTE(t ? PAL_UI : PAL_VIEW);
        for (int i = 0; i < 16; i++)
            p[i] = ZXV_STD_RGB565(i);
    }
    REG8(ZXV_V_FLAGS) = saved_flags;
    REG8(ZXV_V_REMAP) = saved_remap;
    REG8(ZXV_V_BORDER) = saved_border;
    zxv_wait_vblank();
}

uint32_t video_lost(void) { return lost; }

/* El borde: el color c de la tabla de la vista (view) o la del marcador. */
void video_border(int c, int view)
{
    REG8(ZXV_V_REMAP) = view ? PAL_VIEW : PAL_UI;
    REG8(ZXV_V_BORDER) = (uint8_t)c;
}

uint8_t *video_page(void) { return page; }

void rect(int x, int y, int w, int h, int c)
{
    for (int j = y < 0 ? 0 : y; j < y + h && j < scr_h; j++) {
        uint8_t *row = page + j * (scr_w / 2);
        for (int i = x < 0 ? 0 : x; i < x + w && i < scr_w; i++) {
            uint8_t *p = row + (i >> 1);
            *p = i & 1 ? (uint8_t)((*p & 0x0F) | c << 4) : (uint8_t)((*p & 0xF0) | c);
        }
    }
}

/* Recortada a la vista (Liang-Barsky) y, en el modo 1, de dos píxeles. */
void line(int x0, int y0, int x1, int y1, int c)
{
    int32_t dx = x1 - x0, dy = y1 - y0;
    const int32_t p[4] = { -dx, dx, -dy, dy };
    const int32_t q[4] = { x0, scr_w - 1 - x0, y0, view_h - 1 - y0 };
    int64_t t0 = 0, t1 = 65536;
    for (int k = 0; k < 4; k++) {
        if (p[k] == 0) {
            if (q[k] < 0)
                return;
            continue;
        }
        int64_t r = (int64_t)q[k] * 65536 / p[k];
        if (p[k] < 0) {
            if (r > t1)
                return;
            if (r > t0)
                t0 = r;
        } else {
            if (r < t0)
                return;
            if (r < t1)
                t1 = r;
        }
    }
    int ax = x0 + (int)(dx * t0 / 65536), ay = y0 + (int)(dy * t0 / 65536);
    int bx = x0 + (int)(dx * t1 / 65536), by = y0 + (int)(dy * t1 / 65536);
    int sx = ax < bx ? 1 : -1, sy = ay < by ? 1 : -1, z = hires + 1;
    int ex = ax < bx ? bx - ax : ax - bx, ey = ay < by ? ay - by : by - ay, err = ex + ey;
    for (;;) {
        rect(ax, ay, z, z, c);
        if (ax == bx && ay == by)
            break;
        int e2 = 2 * err;
        if (e2 >= ey) {
            err += ey;
            ax += sx;
        }
        if (e2 <= ex) {
            err += ex;
            ay += sy;
        }
    }
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
    { 0x00, 0x00, 0x04, 0x0E, 0x1F, 0x00, 0x00 },   /* ^ (la flecha del récord) */
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
        if (c == '^')
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

/* x e y en píxeles de la pantalla; en el modo 1 la letra es el doble. */
void text(int x, int y, const char *s, int c)
{
    text_size(x, y, s, c, hires + 1);
}

/* Con puntos de z x z píxeles. */
void text_size(int x, int y, const char *s, int c, int z)
{
    while (*s) {
        const uint8_t *g = glyph(&s);
        for (int r = -2; r < 7; r++)
            for (int k = 0; k < 5; k++)
                if ((r < 0 ? marks[mark][r + 2] : g[r]) & (0x10 >> k))
                    rect(x + k * z, y + r * z, z, z, c);
        x += 6 * z;
    }
}

void number(char *out, uint32_t v, int digits)
{
    out[digits] = 0;
    for (int i = digits - 1; i >= 0; i--, v /= 10)
        out[i] = (char)('0' + v % 10);
}
