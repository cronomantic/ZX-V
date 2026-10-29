/*
 * art.c - Los gráficos de Route 128, dibujados con el motor (polígonos,
 * rectángulos y círculos) sobre un lienzo al empezar. Son más grandes de lo
 * que se ven de lejos para que aguanten el escalado de cerca.
 *
 * Los coches rivales se dibujan con el 2 (la carrocería) y el 3 (sus
 * brillos), que car_colours cambia por los de cada coche al dibujarlos.
 */
#include "route128.h"

object_art_t obj_art[OBJ_COUNT];
sprite_t car_art[CAR_TYPES], player_art[5];

const uint8_t car_colours[6][16] = {
#define CC(d, b) { 0, 1, d, b, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
    CC(1, 9), CC(4, 12), CC(6, 14), CC(3, 11), CC(7, 15), CC(5, 13),
#undef CC
};

static void rect(int x, int y, int w, int h, int c) { zxv_gfx_rect(x, y, w, h, c); }
static void disc(int x, int y, int r, int c) { zxv_gfx_circle(x, y, r, c, 1); }

static void poly(int c, int n, ...)
{
    int16_t p[32];
    __builtin_va_list ap;
    __builtin_va_start(ap, n);
    for (int i = 0; i < 2 * n && i < 32; i++)
        p[i] = (int16_t)__builtin_va_arg(ap, int);
    __builtin_va_end(ap);
    zxv_gfx_poly(p, n, c);
}

static int icos(int a) { return sin64[(a + 16) & 63]; }

/* --- El coche ---------------------------------------------------------------- */

/* Un descapotable rojo visto por detrás, de 64 x 32. turn (-2 a 2) desplaza la
 * parte de arriba hacia el lado de la curva, que es como se ve girar. */
static sprite_t make_player(int turn)
{
    int s = turn * 2;
    canvas(64, 32);
    rect(3, 28, 58, 4, 0);                              /* la sombra */
    rect(2, 19, 13, 11, 0);                             /* las ruedas */
    rect(49, 19, 13, 11, 0);
    rect(4, 21, 9, 1, 7);
    rect(51, 21, 9, 1, 7);
    poly(2, 4, 8 + s, 11, 56 + s, 11, 62, 24, 2, 24);   /* la carrocería */
    poly(10, 4, 9 + s, 11, 55 + s, 11, 57 + s / 2, 14, 7 + s / 2, 14);
    rect(3, 24, 58, 3, 0);                              /* el parachoques */
    for (int y = 17; y < 23; y += 2)                    /* la rejilla */
        rect(16 + s / 2, y, 32, 1, 0);
    rect(4, 15, 11, 5, 0);                              /* los pilotos */
    rect(5, 16, 9, 3, 10);
    rect(49, 15, 11, 5, 0);
    rect(50, 16, 9, 3, 10);
    rect(27, 23, 10, 4, 15);                            /* la matrícula */
    rect(29, 24, 6, 1, 0);
    poly(5, 4, 16 + s * 2, 3, 48 + s * 2, 3, 52 + s, 11, 12 + s, 11);  /* el parabrisas */
    poly(13, 3, 18 + s * 2, 4, 30 + s * 2, 4, 20 + s, 9);
    disc(24 + s * 2, 7, 4, 0);                          /* él */
    disc(40 + s * 2, 7, 4, 14);                         /* ella, rubia */
    rect(42 + s * 2, 5, 6 + (s > 0 ? 0 : 2), 2, 14);    /* el pelo al viento */
    rect(10 + s, 10, 44, 2, 2);
    return canvas_sprite();
}

/* Los rivales: un cupé, una furgoneta y un camión, en el 2 y el 3. */
static sprite_t make_coupe(void)
{
    canvas(48, 24);
    rect(2, 21, 44, 3, 0);
    rect(2, 15, 9, 8, 0);
    rect(37, 15, 9, 8, 0);
    rect(2, 10, 44, 10, 2);
    rect(3, 10, 42, 2, 3);
    rect(10, 2, 28, 9, 2);
    rect(12, 3, 24, 6, 5);
    rect(13, 3, 10, 2, 13);
    rect(3, 13, 8, 3, 10);
    rect(37, 13, 8, 3, 10);
    rect(19, 16, 10, 3, 15);
    rect(2, 19, 44, 2, 0);
    return canvas_sprite();
}

static sprite_t make_van(void)
{
    canvas(48, 34);
    rect(2, 31, 44, 3, 0);
    rect(3, 25, 9, 8, 0);
    rect(36, 25, 9, 8, 0);
    rect(2, 1, 44, 27, 2);
    rect(2, 1, 44, 3, 3);
    rect(5, 6, 17, 9, 5);
    rect(26, 6, 17, 9, 5);
    rect(6, 7, 6, 2, 13);
    rect(23, 4, 2, 23, 0);
    rect(3, 18, 5, 5, 10);
    rect(40, 18, 5, 5, 10);
    rect(19, 22, 10, 3, 15);
    rect(2, 27, 44, 2, 0);
    return canvas_sprite();
}

static sprite_t make_truck(void)
{
    canvas(56, 44);
    rect(2, 41, 52, 3, 0);
    rect(3, 33, 10, 9, 0);
    rect(43, 33, 10, 9, 0);
    rect(14, 34, 6, 7, 0);
    rect(36, 34, 6, 7, 0);
    rect(1, 0, 54, 34, 2);
    rect(1, 0, 54, 3, 3);
    for (int x = 7; x < 54; x += 8)
        rect(x, 4, 1, 28, 3);
    rect(27, 3, 2, 30, 0);
    rect(1, 34, 54, 3, 0);
    rect(3, 35, 6, 2, 10);
    rect(47, 35, 6, 2, 10);
    rect(23, 37, 10, 3, 15);
    return canvas_sprite();
}

/* --- Los objetos ----------------------------------------------------------- */

/* Una palmera inclinada hacia la derecha (hacia la carretera, a su izquierda). */
static sprite_t make_palm(void)
{
    canvas(72, 112);
    int x = 16, y = 111;
    for (int i = 0; i < 44; i++) {                      /* el tronco, curvado */
        int px = 16 + (i * i) / 72 + i / 6, py = 111 - i * 2;
        int w = 8 - i / 11;
        rect(px - w / 2, py - 2, w, 3, (i % 4 == 0) ? 2 : 6);
        x = px;
        y = py;
    }
    int cx = x + 1, cy = y - 2;
    static const int8_t ang[9] = { 30, 34, 38, 44, 50, 56, 60, 4, 12 };   /* en 1/64 de vuelta */
    for (int i = 0; i < 9; i++) {                       /* las hojas */
        int a = ang[i], len = 26 + (i & 1) * 6;
        int tx = cx + icos(a) * len / 64, ty = cy - isin(a) * len / 64 + len / 3;
        int mx = (cx + tx) / 2 - isin(a) * 5 / 64, my = (cy + ty) / 2 - icos(a) * 5 / 64 - 4;
        int nx = (cx + tx) / 2 + isin(a) * 3 / 64, ny = (cy + ty) / 2 + icos(a) * 3 / 64 - 2;
        poly(i & 1 ? 4 : 12, 4, cx, cy, mx, my, tx, ty, nx, ny);
    }
    disc(cx - 2, cy + 2, 3, 2);                         /* los cocos */
    disc(cx + 3, cy + 3, 3, 2);
    return canvas_sprite();
}

static sprite_t make_bush(void)
{
    canvas(48, 28);
    disc(12, 18, 10, 4);
    disc(26, 14, 13, 4);
    disc(38, 19, 9, 4);
    disc(20, 11, 6, 12);
    disc(32, 9, 5, 12);
    rect(4, 25, 40, 3, 4);
    return canvas_sprite();
}

static sprite_t make_umbrella(void)
{
    canvas(48, 56);
    rect(23, 14, 2, 42, 7);
    for (int i = 0; i < 8; i++) {                       /* gajos rojos y blancos */
        int x0 = 2 + i * 44 / 8, x1 = 2 + (i + 1) * 44 / 8;
        int y0 = 20 - (i == 0 || i == 7 ? 3 : 0), y1 = 20 - (i + 1 == 8 || i + 1 == 1 ? 3 : 0);
        poly(i & 1 ? 15 : 2, 3, 24, 3, x0, y0, x1, y1);
    }
    disc(24, 3, 2, 15);
    rect(14, 52, 20, 4, 6);                             /* la toalla */
    return canvas_sprite();
}

static sprite_t make_cactus(void)
{
    canvas(40, 80);
    rect(14, 8, 12, 72, 4);
    disc(19, 8, 6, 4);
    rect(4, 38, 12, 6, 4);                              /* los brazos */
    rect(4, 22, 6, 20, 4);
    disc(6, 22, 3, 4);
    rect(26, 46, 10, 6, 4);
    rect(30, 30, 6, 20, 4);
    disc(32, 30, 3, 4);
    rect(16, 6, 2, 72, 12);
    rect(5, 22, 1, 18, 12);
    rect(31, 30, 1, 18, 12);
    for (int y = 14; y < 76; y += 9)
        rect(24, y, 1, 3, 0);
    return canvas_sprite();
}

static sprite_t make_rock(void)
{
    canvas(56, 36);
    poly(7, 7, 3, 35, 2, 22, 10, 8, 24, 2, 40, 5, 52, 16, 55, 35);
    poly(15, 4, 10, 9, 24, 3, 32, 6, 14, 13);
    rect(2, 33, 54, 3, 0);
    zxv_gfx_line(26, 10, 22, 30, 0);
    zxv_gfx_line(38, 12, 44, 30, 0);
    return canvas_sprite();
}

static sprite_t make_pine(void)
{
    canvas(48, 112);
    rect(21, 96, 6, 16, 2);
    poly(4, 3, 2, 100, 46, 100, 24, 56);
    poly(4, 3, 5, 78, 43, 78, 24, 34);
    poly(4, 3, 8, 56, 40, 56, 24, 14);
    poly(4, 3, 12, 36, 36, 36, 24, 0);
    poly(12, 3, 4, 99, 18, 99, 24, 60);
    poly(12, 3, 7, 77, 18, 77, 24, 38);
    poly(12, 3, 10, 55, 18, 55, 24, 18);
    poly(12, 3, 14, 35, 19, 35, 24, 4);
    return canvas_sprite();
}

static sprite_t make_tree(void)
{
    canvas(64, 88);
    rect(28, 54, 8, 34, 2);
    rect(29, 56, 2, 30, 6);
    disc(32, 28, 20, 4);
    disc(16, 42, 14, 4);
    disc(48, 42, 14, 4);
    disc(32, 48, 14, 4);
    disc(24, 20, 9, 12);
    disc(42, 34, 8, 12);
    disc(14, 40, 6, 12);
    return canvas_sprite();
}

/* La flecha de curva: dir -1, a la izquierda; 1, a la derecha. */
static sprite_t make_sign(int dir)
{
    canvas(48, 48);
    rect(10, 24, 3, 24, 7);
    rect(35, 24, 3, 24, 7);
    rect(2, 2, 44, 24, 2);
    rect(4, 4, 40, 20, 15);
    for (int i = 0; i < 2; i++) {                       /* dos galones */
        int x = 10 + i * 14;
        if (dir < 0)
            poly(2, 6, x + 12, 6, x + 4, 14, x + 12, 22, x + 8, 22, x, 14, x + 8, 6);
        else
            poly(2, 6, x, 6, x + 8, 14, x, 22, x + 4, 22, x + 12, 14, x + 4, 6);
    }
    return canvas_sprite();
}

static sprite_t make_board(void)
{
    static const uint8_t ink[7] = { 14, 14, 14, 14, 6, 6, 6 };
    canvas(96, 56);
    rect(14, 40, 4, 16, 7);
    rect(78, 40, 4, 16, 7);
    rect(0, 0, 96, 42, 15);
    rect(2, 2, 92, 38, 1);
    text_big(10, 12, "ZX-V", 3, ink);
    static const uint8_t stripe[4] = { 2, 6, 4, 5 };    /* las franjas del Spectrum */
    for (int i = 0; i < 4; i++) {
        int x = 70 + i * 6;
        poly(stripe[i], 4, x, 38, x + 6, 38, x + 18, 4, x + 12, 4);
    }
    rect(94, 2, 2, 38, 15);
    return canvas_sprite();
}

/* Los arcos sobre la carretera: la salida, los controles y la meta. */
static sprite_t make_gate(const char *label, int paper, int checks)
{
    static const uint8_t ink[7] = { 15, 15, 15, 15, 15, 15, 15 };
    canvas(128, 48);
    for (int y = 6; y < 48; y += 8) {                   /* los postes */
        rect(0, y, 8, 4, 15);
        rect(0, y + 4, 8, 4, 2);
        rect(120, y, 8, 4, 15);
        rect(120, y + 4, 8, 4, 2);
    }
    rect(0, 0, 128, 20, paper);
    if (checks)                                         /* la meta, a cuadros */
        for (int y = 0; y < 20; y += 4)
            for (int x = (y / 4 & 1) * 4; x < 128; x += 8)
                rect(x, y, 4, 4, 0);
    int w = text_width(label) * 2 + 2;                  /* text_width cuenta 6 por letra */
    if (checks)
        rect((128 - w) / 2 - 3, 2, w + 4, 16, 0);
    text_big((128 - w) / 2, 3, label, 2, ink);
    return canvas_sprite();
}

/* --- El paisaje ------------------------------------------------------------- */

/* Dos juegos de capas: las que se ven (bg_far, bg_near y bg_clouds) y las de
 * la etapa que viene, que se dibujan poco a poco, un trozo por frame
 * (art_prepare), y se cambian por las otras al llegar a ella: dibujarlas de
 * una vez no cabe en un frame junto con la carretera. */
#define BG_W    512
static uint32_t far_mem[2][BG_W * 40 / 8], near_mem[2][BG_W * 24 / 8], cloud_mem[2][BG_W * 20 / 8];
zxv_bitmap bg_far = { (uint8_t *)far_mem[0], BG_W / 2, BG_W, 40, 0 };
zxv_bitmap bg_near = { (uint8_t *)near_mem[0], BG_W / 2, BG_W, 24, 0 };
zxv_bitmap bg_clouds = { (uint8_t *)cloud_mem[0], BG_W / 2, BG_W, 20, 0 };
static zxv_bitmap next_far = { (uint8_t *)far_mem[1], BG_W / 2, BG_W, 40, 0 };
static zxv_bitmap next_near = { (uint8_t *)near_mem[1], BG_W / 2, BG_W, 24, 0 };
static zxv_bitmap next_clouds = { (uint8_t *)cloud_mem[1], BG_W / 2, BG_W, 20, 0 };
static int shown_theme = -1;            /* la etapa de las que se ven */
static int next_theme = -1, next_step;  /* la de las otras, y los trozos que van */

/* Una silueta que se repite cada BG_W píxeles: unas ondas de periodos que lo dividen. */
static int ridge(int x, int a, int b, int c)
{
    return (isin(x * 64 * 2 / BG_W) * a + isin(x * 64 * 5 / BG_W + 9) * b + isin(x * 64 * 13 / BG_W + 23) * c) / 64;
}

static void bg_clear(zxv_bitmap *b)
{
    uint32_t *p = (uint32_t *)(void *)b->pix;
    for (int i = 0; i < b->stride * b->h / 4; i++)
        p[i] = 0x88888888u;             /* todo transparente */
}

static void bg_target(zxv_bitmap *b)
{
    zxv_gfx_target(b);
    zxv_gfx_clip(0, 0, b->w, b->h);
}

/* Arriba: unas nubes, que van más despacio que todo. */
static void draw_clouds(zxv_bitmap *b, int theme)
{
    static const uint8_t cloud[THEMES][2] = { { 15, 13 }, { 15, 15 }, { 15, 14 } };
    bg_clear(b);
    bg_target(b);
    for (int i = 0; i < 6; i++) {
        int cx = 40 + i * 83 + (i * 37) % 23, cy = 8 + (i * 5) % 7, w = 14 + (i * 11) % 12;
        zxv_gfx_circle(cx, cy + 3, 5, cloud[theme][1], 1);
        zxv_gfx_circle(cx + w, cy + 4, 4, cloud[theme][1], 1);
        rect(cx - 4, cy + 4, w + 8, 4, cloud[theme][1]);
        zxv_gfx_circle(cx + w / 2, cy, 6, cloud[theme][0], 1);
        rect(cx, cy + 3, w, 3, cloud[theme][0]);
    }
}

/* Lejos: montañas; las columnas de x0 a x1 (sin x1). */
static void draw_far(zxv_bitmap *b, int theme, int x0, int x1)
{
    bg_target(b);
    for (int x = x0; x < x1; x++) {
        int h;
        switch (theme) {
        case THEME_COAST:
            h = 22 + ridge(x, 9, 5, 2);
            rect(x, 40 - h, 1, h, 9);
            if (h > 26)
                rect(x, 40 - h, 1, 2, 13);
            break;
        case THEME_DESERT: {                            /* mesetas rojas, de cima plana */
            int m = ridge(x, 12, 9, 4);
            h = 10 + m * 2;
            if (h > 31)
                h = 31;                                 /* la cima, plana */
            if (h < 6)
                h = 6;
            rect(x, 40 - h, 1, h, 2);
            for (int y = 40 - h + 4; y < 38; y += 6)    /* los estratos */
                rect(x, y, 1, 1, 10);
            rect(x, 40 - h, 1, 1, 10);
            break;
        }
        default:                                        /* el bosque: picos nevados */
            h = 26 + ridge(x, 10, 6, 3);
            rect(x, 40 - h, 1, h, 1);
            if (h > 30)
                rect(x, 40 - h, 1, h - 30 + 2, 15);
            break;
        }
    }
}

/* Cerca: el mar con brillos, dunas o lomas con árboles; de x0 a x1. */
static void draw_near(zxv_bitmap *b, int theme, int x0, int x1)
{
    bg_target(b);
    for (int x = x0; x < x1; x++) {
        switch (theme) {
        case THEME_COAST:
            rect(x, 16, 1, 8, 1);
            if ((x * 7) % 23 == 0)
                rect(x, 18 + x % 5, 3, 1, 15);
            break;
        case THEME_DESERT: {
            int h = 8 + ridge(x * 3, 4, 3, 0);
            rect(x, 24 - h, 1, h, 6);
            rect(x, 24 - h, 1, 1, 14);
            break;
        }
        default: {
            int h = 9 + ridge(x * 2, 5, 3, 1);
            rect(x, 24 - h, 1, h, 4);
            if (x % 9 == 0)                             /* árboles lejanos */
                poly(0, 3, x - 3, 24 - h + 1, x + 3, 24 - h + 1, x, 24 - h - 7);
            break;
        }
        }
    }
}

/* Uno de los BG_STEPS trozos de las capas de la etapa que viene: las nubes,
 * y las montañas y lo de cerca en cuartos de 128 columnas (borrando la capa
 * con el primero). Cada uno tarda alrededor de un milisegundo. */
#define BG_STEPS    9
#define BG_PART     (BG_W / 4)

static void bg_step(int theme, int step)
{
    if (step == 0) {
        draw_clouds(&next_clouds, theme);
    } else if (step <= 4) {
        if (step == 1)
            bg_clear(&next_far);
        draw_far(&next_far, theme, (step - 1) * BG_PART, step * BG_PART);
    } else {
        if (step == 5)
            bg_clear(&next_near);
        draw_near(&next_near, theme, (step - 5) * BG_PART, (step - 4) * BG_PART);
    }
    zxv_gfx_target(0);
    zxv_gfx_clip(0, 0, W, H);
}

static void swap_pix(zxv_bitmap *a, zxv_bitmap *b)
{
    uint8_t *p = a->pix;
    a->pix = b->pix;
    b->pix = p;
}

void art_prepare(int theme)
{
    if (theme == shown_theme)
        return;
    if (theme != next_theme) {
        next_theme = theme;
        next_step = 0;
    }
    if (next_step < BG_STEPS)
        bg_step(theme, next_step++);
}

void art_background(int theme)
{
    if (theme == shown_theme)
        return;
    while (next_theme != theme || next_step < BG_STEPS)
        art_prepare(theme);             /* lo que falte, de una vez */
    swap_pix(&bg_far, &next_far);
    swap_pix(&bg_near, &next_near);
    swap_pix(&bg_clouds, &next_clouds);
    next_theme = shown_theme;           /* las otras se quedan con la de antes, enteras */
    next_step = BG_STEPS;
    shown_theme = theme;
}

/* --- Todo ------------------------------------------------------------------- */

void art_init(void)
{
    static const struct { uint16_t w; uint8_t solid, mirror; } info[OBJ_COUNT] = {
        [OBJ_PALM] = { 1500, 1, 1 }, [OBJ_BUSH] = { 800, 0, 0 }, [OBJ_UMBRELLA] = { 650, 1, 0 },
        [OBJ_CACTUS] = { 650, 1, 1 }, [OBJ_ROCK] = { 1000, 1, 1 }, [OBJ_PINE] = { 1100, 1, 0 },
        [OBJ_TREE] = { 1400, 1, 1 }, [OBJ_SIGN_L] = { 800, 1, 0 }, [OBJ_SIGN_R] = { 800, 1, 0 },
        [OBJ_BOARD] = { 1900, 1, 0 }, [OBJ_START] = { 3100, 0, 0 }, [OBJ_GATE] = { 3100, 0, 0 },
        [OBJ_GOAL] = { 3100, 0, 0 },
    };
    obj_art[OBJ_PALM].s = make_palm();
    obj_art[OBJ_BUSH].s = make_bush();
    obj_art[OBJ_UMBRELLA].s = make_umbrella();
    obj_art[OBJ_CACTUS].s = make_cactus();
    obj_art[OBJ_ROCK].s = make_rock();
    obj_art[OBJ_PINE].s = make_pine();
    obj_art[OBJ_TREE].s = make_tree();
    obj_art[OBJ_SIGN_L].s = make_sign(-1);
    obj_art[OBJ_SIGN_R].s = make_sign(1);
    obj_art[OBJ_BOARD].s = make_board();
    obj_art[OBJ_START].s = make_gate(T("SALIDA", "START"), 1, 0);
    obj_art[OBJ_GATE].s = make_gate(T("CONTROL", "CHECKPOINT"), 2, 0);
    obj_art[OBJ_GOAL].s = make_gate(T("META", "GOAL"), 15, 1);
    for (int i = 0; i < OBJ_COUNT; i++) {
        obj_art[i].world_w = info[i].w;
        obj_art[i].solid = info[i].solid;
        obj_art[i].mirror = info[i].mirror;
    }
    car_art[CAR_COUPE] = make_coupe();
    car_art[CAR_VAN] = make_van();
    car_art[CAR_TRUCK] = make_truck();
    for (int t = -2; t <= 2; t++)
        player_art[t + 2] = make_player(t);
    art_background(THEME_COAST);
}
