/*
 * art.c - Los gráficos de Street Justice, dibujados con el motor al empezar:
 *
 *   - Los luchadores son un esqueleto (cadera, tronco, brazos y piernas en
 *     dos tramos) en 22 poses dadas por sus ángulos. Cada parte es un trazo
 *     grueso con su contorno negro. Hay dos cuerpos, el normal y el grueso,
 *     en colores «genéricos» que se cambian al dibujar (la tabla de colores
 *     de zxv_spr_draw): así los mismos sprites valen para todos. La cabeza va
 *     aparte, una por personaje, derecha o tumbada.
 *   - Los decorados: un fondo lejano de 256 píxeles que se repite, seis
 *     fachadas de 64 píxeles de ancho por fase y un suelo que se repite.
 */
#include "street.h"

/* --- Los colores genéricos de los luchadores ----------------------------------- */

enum {
    G_LINE = 0, G_SKIN = 1, G_HAIR = 2, G_TOP = 3, G_TOP2 = 4, G_LEGS = 5, G_LEGS2 = 6, G_SHOES = 7,
    G_ARM = 9, G_ARM2 = 10, G_BELT = 11, G_EXTRA = 13, G_EXTRA2 = 14, G_WHITE = 15,
};

frame_t frames[BODIES][POSES];
sprite_t heads[HEADS][2];
sprite_t items[ITEMS];

/* --- El esqueleto --------------------------------------------------------------- */

typedef struct {
    int8_t thigh, shin, torso, uarm, farm;      /* largos */
    int8_t wthigh, wshin, wuarm, wfarm, whip, wsh, belly;   /* anchos */
} body_t;

static const body_t body_defs[BODIES] = {
    { 11, 11, 15, 9, 8, 6, 5, 5, 4, 9, 11, 0 },
    { 10, 10, 15, 9, 8, 7, 6, 6, 5, 13, 15, 7 },
};

/* Ángulos en 64 pasos por vuelta: 0 abajo, 16 adelante, 32 arriba, 48 atrás.
 * hip: 0, los pies en el suelo; si no, la altura de la cadera. */
typedef struct {
    int8_t hip, tor, bu, bf, fu, ff, bt, bs, ft, fs;
} pose_t;

static const pose_t pose_defs[POSES] = {
    [P_IDLE] = { 0, 30, 8, 26, 12, 28, 60, 62, 5, 1 },
    [P_WALK1] = { 0, 30, 4, 22, 14, 28, 56, 60, 9, 3 },
    [P_WALK2] = { 0, 30, 8, 26, 12, 28, 4, 56, 63, 0 },
    [P_WALK3] = { 0, 30, 12, 28, 8, 24, 9, 3, 56, 60 },
    [P_WALK4] = { 0, 30, 8, 26, 12, 28, 63, 0, 4, 56 },
    [P_JAB] = { 0, 29, 8, 26, 16, 16, 58, 62, 6, 2 },
    [P_CROSS] = { 0, 27, 16, 16, 6, 30, 57, 61, 7, 3 },
    [P_UPPER] = { 0, 28, 4, 20, 20, 30, 58, 62, 8, 4 },
    [P_KICK] = { 0, 36, 48, 44, 20, 30, 62, 0, 18, 16 },
    [P_WINDUP] = { 0, 34, 10, 28, 44, 30, 58, 62, 6, 2 },
    [P_JUMP] = { 26, 31, 24, 34, 20, 30, 14, 56, 24, 62 },
    [P_JKICK] = { 26, 38, 44, 40, 12, 24, 6, 52, 17, 17 },
    [P_HURT] = { 0, 37, 2, 10, 6, 12, 60, 62, 4, 0 },
    [P_FALL] = { 14, 46, 40, 44, 36, 40, 16, 20, 12, 14 },
    [P_DOWN] = { 4, 48, 50, 50, 46, 46, 15, 17, 17, 15 },
    [P_GETUP] = { 5, 38, 54, 60, 12, 20, 16, 16, 22, 0 },
    [P_GRAB] = { 0, 29, 14, 20, 16, 22, 58, 62, 6, 2 },
    [P_KNEE] = { 0, 26, 16, 8, 15, 6, 62, 0, 28, 4 },
    [P_THROW] = { 0, 40, 36, 40, 38, 44, 56, 60, 8, 2 },
    [P_HELD] = { 0, 22, 2, 0, 4, 2, 62, 2, 4, 4 },
    [P_WIN] = { 0, 32, 4, 8, 32, 32, 60, 62, 4, 0 },
    [P_SPIN] = { 0, 26, 44, 48, 20, 16, 48, 48, 2, 0 },
};

#define CW          64                  /* el lienzo de un cuerpo */
#define CH          60
#define AX          32                  /* los pies */
#define AY          56

static int off_x, off_y;                /* el desplazamiento del contorno */

static void seg(int x0, int y0, int x1, int y1, int w, int c)
{
    stroke(x0 + off_x, y0 + off_y, x1 + off_x, y1 + off_y, w, c);
}

static void quad(int x0, int y0, int w0, int x1, int y1, int w1, int c)
{
    int dx = x1 - x0, dy = y1 - y0, len = isqrt(dx * dx + dy * dy);
    if (!len)
        return;
    int n0x = -dy * w0 * 8 / len, n0y = dx * w0 * 8 / len, n1x = -dy * w1 * 8 / len, n1y = dx * w1 * 8 / len;
    int16_t p[8] = {
        (int16_t)(x0 + off_x + ((n0x + 8) >> 4)), (int16_t)(y0 + off_y + ((n0y + 8) >> 4)),
        (int16_t)(x1 + off_x + ((n1x + 8) >> 4)), (int16_t)(y1 + off_y + ((n1y + 8) >> 4)),
        (int16_t)(x1 + off_x + ((8 - n1x) >> 4)), (int16_t)(y1 + off_y + ((8 - n1y) >> 4)),
        (int16_t)(x0 + off_x + ((8 - n0x) >> 4)), (int16_t)(y0 + off_y + ((8 - n0y) >> 4)),
    };
    zxv_gfx_poly(p, 4, c);
}

static void disc(int x, int y, int r, int c) { zxv_gfx_circle(x + off_x, y + off_y, r, c, 1); }

/* Un punto a len píxeles de (x, y) en el ángulo a. */
static void step(int *x, int *y, int a, int len)
{
    *x += (isin(a) * len + (isin(a) >= 0 ? 32 : -32)) / 64;
    *y += (icos(a) * len + (icos(a) >= 0 ? 32 : -32)) / 64;
}

/* Una parte, con su contorno: c < 0 es el contorno (cuatro veces, movido). */
typedef void (*part_fn)(const body_t *b, const int *j, int c);

enum { J_HX, J_HY, J_NX, J_NY, J_SX, J_SY, J_BEX, J_BEY, J_BHX, J_BHY, J_FEX, J_FEY, J_FHX, J_FHY,
       J_BKX, J_BKY, J_BAX, J_BAY, J_FKX, J_FKY, J_FAX, J_FAY, J_BA, J_FA, J_TOR, JOINTS };

static void part_arm(const body_t *b, const int *j, int front, int c)
{
    int e = front ? J_FEX : J_BEX, h = front ? J_FHX : J_BHX;
    seg(j[J_SX], j[J_SY], j[e], j[e + 1], b->wuarm + (c < 0 ? 2 : 0), c < 0 ? 0 : front ? G_TOP : G_TOP2);
    seg(j[e], j[e + 1], j[h], j[h + 1], b->wfarm + (c < 0 ? 2 : 0), c < 0 ? 0 : front ? G_ARM : G_ARM2);
    disc(j[h], j[h + 1], b->wfarm / 2 + 1 + (c < 0), c < 0 ? 0 : G_SKIN);
}

static void part_leg(const body_t *b, const int *j, int front, int c)
{
    int k = front ? J_FKX : J_BKX, a = front ? J_FAX : J_BAX;
    int col = front ? G_LEGS : G_LEGS2, o = c < 0 ? 2 : 0;
    seg(j[J_HX], j[J_HY], j[k], j[k + 1], b->wthigh + o, c < 0 ? 0 : col);
    seg(j[k], j[k + 1], j[a], j[a + 1], b->wshin + o, c < 0 ? 0 : col);
    int fx = j[a], fy = j[a + 1];
    step(&fx, &fy, j[front ? J_FA : J_BA] + 16, 4);
    seg(j[a], j[a + 1], fx, fy, 3 + o, c < 0 ? 0 : G_SHOES);
}

static void part_torso(const body_t *b, const int *j, int c)
{
    int o = c < 0 ? 2 : 0;
    quad(j[J_HX], j[J_HY], b->whip + o, j[J_NX], j[J_NY], b->wsh + o, c < 0 ? 0 : G_TOP);
    if (b->belly) {                     /* la barriga, hacia delante */
        int x = j[J_HX], y = j[J_HY];
        step(&x, &y, j[J_TOR], 5);
        step(&x, &y, j[J_TOR] - 16, 2);
        disc(x, y, b->belly + (c < 0), c < 0 ? 0 : G_TOP);
    }
    /* El cinturón y el cuello. */
    int bx = j[J_HX], by = j[J_HY];
    step(&bx, &by, j[J_TOR], 2);
    quad(j[J_HX], j[J_HY], b->whip + o, bx, by, b->whip + o, c < 0 ? 0 : G_LEGS);
    if (c >= 0) {
        int x0 = bx, y0 = by;
        step(&x0, &y0, j[J_TOR], 1);
        quad(bx, by, b->whip, x0, y0, b->whip, G_BELT);
    }
    int nx = j[J_NX], ny = j[J_NY];
    step(&nx, &ny, j[J_TOR], 3);
    seg(j[J_NX], j[J_NY], nx, ny, 3 + o, c < 0 ? 0 : G_SKIN);
}

/* Las articulaciones de una pose. */
static void joints(const body_t *b, const pose_t *p, int *j)
{
    int hx = AX, hy;
    if (p->hip) {
        hy = AY - p->hip;
    } else {                            /* el pie más bajo, en el suelo */
        int low = 0;
        for (int k = 0; k < 2; k++) {
            int x = 0, y = 0;
            step(&x, &y, k ? p->ft : p->bt, b->thigh);
            step(&x, &y, k ? p->fs : p->bs, b->shin);
            if (y > low)
                low = y;
        }
        hy = AY - low - 1;
    }
    j[J_HX] = hx;
    j[J_HY] = hy;
    j[J_TOR] = p->tor;
    j[J_NX] = hx, j[J_NY] = hy;
    step(&j[J_NX], &j[J_NY], p->tor, b->torso);
    j[J_SX] = hx, j[J_SY] = hy;
    step(&j[J_SX], &j[J_SY], p->tor, b->torso - 2);
    j[J_BEX] = j[J_SX], j[J_BEY] = j[J_SY];
    step(&j[J_BEX], &j[J_BEY], p->bu, b->uarm);
    j[J_BHX] = j[J_BEX], j[J_BHY] = j[J_BEY];
    step(&j[J_BHX], &j[J_BHY], p->bf, b->farm);
    j[J_FEX] = j[J_SX], j[J_FEY] = j[J_SY];
    step(&j[J_FEX], &j[J_FEY], p->fu, b->uarm);
    j[J_FHX] = j[J_FEX], j[J_FHY] = j[J_FEY];
    step(&j[J_FHX], &j[J_FHY], p->ff, b->farm);
    j[J_BKX] = hx - 1, j[J_BKY] = hy;
    step(&j[J_BKX], &j[J_BKY], p->bt, b->thigh);
    j[J_BAX] = j[J_BKX], j[J_BAY] = j[J_BKY];
    step(&j[J_BAX], &j[J_BAY], p->bs, b->shin);
    j[J_FKX] = hx + 1, j[J_FKY] = hy;
    step(&j[J_FKX], &j[J_FKY], p->ft, b->thigh);
    j[J_FAX] = j[J_FKX], j[J_FAY] = j[J_FKY];
    step(&j[J_FAX], &j[J_FAY], p->fs, b->shin);
    j[J_BA] = p->bs;
    j[J_FA] = p->fs;
}

static const int8_t around[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };

static void make_body(int bi, int pi)
{
    const body_t *b = &body_defs[bi];
    const pose_t *p = &pose_defs[pi];
    int j[JOINTS];
    joints(b, p, j);
    canvas(CW, CH);
    /* De atrás adelante: brazo de atrás, piernas, tronco, brazo de delante;
     * cada parte con su contorno (dibujada en negro cuatro veces, movida). */
    for (int part = 0; part < 5; part++)
        for (int pass = 0; pass < 5; pass++) {
            int c = pass < 4 ? -1 : 1;
            off_x = pass < 4 ? around[pass][0] : 0;
            off_y = pass < 4 ? around[pass][1] : 0;
            switch (part) {
            case 0: part_arm(b, j, 0, c); break;
            case 1: part_leg(b, j, 0, c); break;
            case 2: part_leg(b, j, 1, c); break;
            case 3: part_torso(b, j, c); break;
            default: part_arm(b, j, 1, c); break;
            }
        }
    off_x = off_y = 0;
    frame_t *f = &frames[bi][pi];
    int x0, y0, w, h;
    canvas_bounds(&x0, &y0, &w, &h);
    f->ox = (int8_t)(x0 - AX);
    f->oy = (int8_t)(y0 - AY);
    int hx = j[J_HX], hy = j[J_HY];     /* la cabeza, encima del cuello */
    step(&hx, &hy, p->tor, b->torso + 5 + bi);
    f->hx = (int8_t)(hx - AX);
    f->hy = (int8_t)(hy - AY);
    f->kx = (int8_t)(j[J_FHX] - AX);
    f->ky = (int8_t)(j[J_FHY] - AY);
    f->ka = (uint8_t)(p->ff & 63);
    f->lying = p->tor >= 42;
    f->s = canvas_sprite(x0, y0, w, h);
}

/* --- Las cabezas ------------------------------------------------------------------- */

/* Un contorno negro alrededor de lo dibujado en el lienzo. */
static void outline(int w, int h)
{
    static uint8_t mark[16 * 16];
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            mark[y * w + x] = 0;
            if (canvas_get(x, y) != ZXV_TRANSPARENT)
                continue;
            for (int k = 0; k < 4; k++) {
                int c = canvas_get(x + around[k][0], y + around[k][1]);
                if (c != ZXV_TRANSPARENT && c != 0)
                    mark[y * w + x] = 1;
            }
        }
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (mark[y * w + x])
                canvas_set(x, y, 0);
}

static void make_head(int style)
{
    static uint8_t keep[16 * 16];
    canvas(16, 16);
    /* La cara mira a la derecha; el centro, en (8, 8). */
    zxv_gfx_circle(8, 8, 4, G_SKIN, 1);
    zxv_gfx_rect(11, 8, 2, 2, G_SKIN);                  /* la nariz y la barbilla */
    switch (style) {
    case HEAD_HERO:                                     /* pelo corto y una cinta */
        zxv_gfx_circle(7, 6, 4, G_HAIR, 1);
        zxv_gfx_rect(4, 7, 3, 3, G_HAIR);
        zxv_gfx_rect(4, 5, 9, 2, G_EXTRA);
        zxv_gfx_rect(1, 6, 3, 1, G_EXTRA);
        zxv_gfx_rect(0, 8, 2, 1, G_EXTRA);
        break;
    case HEAD_PUNK:                                     /* la cresta */
        zxv_gfx_rect(6, 1, 3, 4, G_HAIR);
        zxv_gfx_rect(4, 2, 2, 3, G_HAIR);
        zxv_gfx_rect(9, 2, 2, 2, G_HAIR);
        zxv_gfx_rect(5, 9, 1, 2, G_EXTRA2);             /* el pendiente */
        break;
    case HEAD_CAP:                                      /* la gorra, con la visera */
        zxv_gfx_circle(7, 6, 4, G_EXTRA, 1);
        zxv_gfx_rect(3, 6, 9, 1, G_EXTRA);
        zxv_gfx_rect(10, 5, 5, 2, G_EXTRA2);
        zxv_gfx_rect(4, 7, 2, 3, G_HAIR);
        break;
    case HEAD_BALD:                                     /* calvo, gafas y barba */
        zxv_gfx_rect(8, 7, 5, 2, 0);
        zxv_gfx_rect(11, 7, 1, 1, G_WHITE);
        zxv_gfx_rect(7, 10, 6, 3, G_HAIR);
        break;
    case HEAD_BANDANA:                                  /* pañuelo y barba */
        zxv_gfx_circle(7, 6, 4, G_EXTRA, 1);
        zxv_gfx_rect(3, 6, 10, 1, G_EXTRA);
        zxv_gfx_rect(1, 7, 3, 2, G_EXTRA);
        zxv_gfx_rect(7, 10, 6, 3, G_HAIR);
        zxv_gfx_rect(8, 10, 3, 1, G_SKIN);
        break;
    case HEAD_LONG:                                     /* melena y gafas */
        zxv_gfx_circle(7, 6, 4, G_HAIR, 1);
        zxv_gfx_rect(3, 6, 4, 8, G_HAIR);
        zxv_gfx_rect(8, 7, 5, 2, 0);
        break;
    default:                                            /* el jefe: engominado y bigote */
        zxv_gfx_circle(7, 6, 4, G_HAIR, 1);
        zxv_gfx_rect(3, 5, 10, 2, G_HAIR);
        zxv_gfx_rect(4, 7, 2, 3, G_HAIR);
        zxv_gfx_rect(9, 10, 4, 1, G_HAIR);
        zxv_gfx_rect(9, 4, 3, 1, G_EXTRA);              /* las canas */
        break;
    }
    if (style != HEAD_BALD && style != HEAD_LONG)
        zxv_gfx_rect(10, 7, 1, 1, 0);                   /* el ojo */
    outline(16, 16);
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
            keep[y * 16 + x] = (uint8_t)canvas_get(x, y);
    heads[style][0] = canvas_sprite(0, 0, 16, 16);
    /* Tumbada: boca arriba, la coronilla hacia atrás. */
    canvas(16, 16);
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            int u = 8 - (y - 8), v = x;         /* (x', y') = (v, -u) desde el centro */
            if (u >= 0 && u < 16 && v >= 0 && v < 16)
                canvas_set(x, y, keep[v * 16 + u]);
        }
    heads[style][1] = canvas_sprite(0, 0, 16, 16);
}

/* --- Lo que se rompe y lo que se come ------------------------------------------------- */

static void make_items(void)
{
    canvas(16, 20);                                     /* el cubo de basura */
    zxv_gfx_rect(2, 4, 12, 15, 0);
    zxv_gfx_rect(3, 5, 10, 13, 7);
    for (int x = 4; x < 13; x += 3)
        zxv_gfx_rect(x, 6, 1, 11, 15);
    zxv_gfx_rect(1, 2, 14, 3, 0);
    zxv_gfx_rect(2, 3, 12, 1, 15);
    zxv_gfx_rect(6, 0, 4, 2, 0);
    items[PROP_BIN] = canvas_sprite(0, 0, 16, 20);

    canvas(18, 16);                                     /* la caja de fruta */
    zxv_gfx_rect(0, 3, 18, 13, 0);
    zxv_gfx_rect(1, 4, 16, 11, 6);
    zxv_gfx_rect(1, 8, 16, 1, 0);
    zxv_gfx_rect(1, 12, 16, 1, 0);
    for (int k = 0; k < 5; k++) {
        zxv_gfx_circle(3 + k * 3, 2, 2, k & 1 ? 12 : 10, 1);
        zxv_gfx_rect(3 + k * 3, 1, 1, 1, 15);
    }
    items[PROP_CRATE] = canvas_sprite(0, 0, 18, 16);

    canvas(16, 20);                                     /* el bidón */
    zxv_gfx_rect(1, 1, 14, 19, 0);
    zxv_gfx_rect(2, 2, 12, 17, 2);
    zxv_gfx_rect(2, 6, 12, 1, 10);
    zxv_gfx_rect(2, 13, 12, 1, 10);
    zxv_gfx_rect(4, 3, 2, 15, 10);
    items[PROP_BARREL] = canvas_sprite(0, 0, 16, 20);

    canvas(8, 8);                                       /* la manzana */
    zxv_gfx_circle(4, 4, 3, 10, 1);
    zxv_gfx_rect(4, 0, 1, 2, 4);
    zxv_gfx_rect(5, 0, 2, 1, 12);
    zxv_gfx_rect(2, 3, 1, 1, 15);
    outline(8, 8);
    items[FOOD_APPLE] = canvas_sprite(0, 0, 8, 8);

    canvas(16, 8);                                      /* el bocadillo */
    zxv_gfx_circle(4, 4, 3, 6, 1);
    zxv_gfx_circle(11, 4, 3, 6, 1);
    zxv_gfx_rect(4, 1, 8, 7, 6);
    zxv_gfx_rect(2, 4, 12, 1, 12);
    zxv_gfx_rect(3, 5, 10, 1, 10);
    zxv_gfx_rect(5, 2, 6, 1, 14);
    outline(16, 8);
    items[FOOD_BREAD] = canvas_sprite(0, 0, 16, 8);

    canvas(16, 10);                                     /* el pollo asado */
    zxv_gfx_circle(7, 5, 4, 6, 1);
    zxv_gfx_circle(5, 6, 3, 6, 1);
    zxv_gfx_rect(9, 2, 5, 2, 15);
    zxv_gfx_rect(13, 1, 2, 2, 15);
    zxv_gfx_rect(5, 3, 3, 1, 14);
    outline(16, 10);
    items[FOOD_CHICKEN] = canvas_sprite(0, 0, 16, 10);
}

void art_init(void)
{
    for (int b = 0; b < BODIES; b++)
        for (int p = 0; p < POSES; p++)
            make_body(b, p);
    for (int h = 0; h < HEADS; h++)
        make_head(h);
    make_items();
}

/* --- Los decorados ------------------------------------------------------------------- */

#define NFAC        6

static uint32_t far_mem[256 * SKY_H / 8];
static uint32_t floor_mem[256 * FLOOR_H / 8];
static uint32_t fac_mem[NFAC][SLOT_W * FACADE_H / 8];
static zxv_bitmap far_bmp, floor_bmp, fac_bmp[NFAC];
static int mid_colour;

const uint8_t *stage_slots;
int stage_nslots;

/* Las calles de cada fase: qué fachada va en cada hueco de 64 píxeles. */
static const uint8_t slots1[] = { 0, 1, 3, 0, 2, 4, 1, 5, 0, 3, 1, 2, 4, 0, 5, 1, 3, 0, 2, 4, 1, 0, 5, 3, 1, 4 };
static const uint8_t slots2[] = { 0, 1, 2, 4, 3, 0, 5, 1, 2, 0, 3, 4, 1, 5, 2, 0, 3, 1, 4, 2, 0, 5, 3, 1, 2, 4 };
static const uint8_t slots3[] = { 0, 3, 1, 0, 2, 4, 0, 3, 5, 1, 0, 2, 3, 4, 0, 1, 5, 3, 0, 2, 4, 1, 3, 0, 5, 2 };

static void bmp_init(zxv_bitmap *b, uint32_t *mem, int w, int h)
{
    b->pix = (uint8_t *)mem;
    b->stride = (uint16_t)(w / 2);
    b->w = (uint16_t)w;
    b->h = (uint16_t)h;
    for (int i = 0; i < w * h / 8; i++)
        mem[i] = 0x88888888u;
}

static zxv_bitmap *cur;

static void use(zxv_bitmap *b)
{
    cur = b;
    zxv_gfx_target(b);
    zxv_gfx_clip(0, 0, b->w, b->h);
}

static void px(int x, int y, int c)
{
    if ((unsigned)x >= cur->w || (unsigned)y >= cur->h)
        return;
    uint8_t *p = cur->pix + y * cur->stride + (x >> 1);
    *p = x & 1 ? (uint8_t)((*p & 0x0F) | c << 4) : (uint8_t)((*p & 0xF0) | c);
}

/* Un rectángulo a cuadros de dos colores (los tonos intermedios). */
static void dither(int x, int y, int w, int h, int a, int b)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            px(x + i, y + j, ((x + i + y + j) & 1) ? a : b);
}

static void rect(int x, int y, int w, int h, int c) { zxv_gfx_rect(x, y, w, h, c); }

/* El cielo: franjas de color, con un paso a cuadros entre una y otra. */
static void sky_bands(const uint8_t *c, int n, int h)
{
    int bh = h / n;
    for (int i = 0; i < n; i++) {
        rect(0, i * bh, 256, bh, c[i]);
        if (i)
            dither(0, i * bh - 1, 256, 2, c[i], c[i - 1]);
    }
    if (n * bh < h)
        rect(0, n * bh, 256, h - n * bh, c[n - 1]);
}

/* Siluetas de edificios lejanos con ventanas encendidas. */
static void skyline(int base, int lo, int hi, int col, int lit, int density)
{
    int x = 0;
    while (x < 256) {
        int w = 14 + (int)(rnd() % 20), h = lo + (int)(rnd() % (hi - lo));
        if (x + w > 256)
            w = 256 - x;
        rect(x, base - h, w, h + 40, col);
        if (rnd() % 3 == 0)
            rect(x + w / 2 - 1, base - h - 6, 1, 6, col);   /* una antena */
        for (int wy = base - h + 3; wy < SKY_H - 1; wy += 4)
            for (int wx = x + 2; wx < x + w - 2; wx += 3)
                if ((int)(rnd() % 100) < density)
                    px(wx, wy, lit);
        x += w;
    }
}

static void window(int x, int y, int w, int h, int frame, int glass)
{
    rect(x - 1, y - 1, w + 2, h + 2, frame);
    rect(x, y, w, h, glass);
    rect(x + w / 2, y, 1, h, frame);
    rect(x, y + h / 2, w, 1, frame);
}

/* Unas pintadas: trazos de colores vivos. */
static void graffiti(int x, int y, int w, int h)
{
    static const uint8_t cols[] = { 11, 12, 13, 14, 10, 9 };
    for (int k = 0; k < 3; k++) {
        int c = cols[rnd() % 6];
        int x0 = x + (int)(rnd() % (unsigned)w), y0 = y + (int)(rnd() % (unsigned)h);
        for (int s = 0; s < 4; s++) {
            int x1 = x + (int)(rnd() % (unsigned)w), y1 = y + (int)(rnd() % (unsigned)h);
            zxv_gfx_line(x0, y0, x1, y1, c);
            zxv_gfx_line(x0, y0 + 1, x1, y1 + 1, c);
            x0 = x1;
            y0 = y1;
        }
    }
}

static void sign(int x, int y, int w, const char *s, int bg, int fg)
{
    rect(x, y, w, 11, 0);
    rect(x + 1, y + 1, w - 2, 9, bg);
    text_target(cur);
    text(x + (w - text_width(s)) / 2, y + 2, s, fg);
    text_target(0);
}

/* Fase 1, la calle de noche. */
static void scene_street(void)
{
    static const uint8_t sky[] = { 0, 1, 1 };
    use(&far_bmp);
    sky_bands(sky, 3, SKY_H);
    for (int k = 0; k < 40; k++)
        px((int)(rnd() % 256), (int)(rnd() % 30), k & 3 ? 7 : 15);
    zxv_gfx_circle(200, 12, 7, 15, 1);
    zxv_gfx_circle(203, 10, 6, 1, 1);
    skyline(SKY_H - 2, 18, 44, 0, 6, 25);
    mid_colour = 0;

    /* Las fachadas, a la escala de los luchadores (unos 50 píxeles): solo se
     * ve la planta baja, con puertas y escaparates de 67 (1,35 veces un
     * luchador) y los letreros por encima de las cabezas, y el borde del
     * primer piso; los edificios siguen por encima de la pantalla. */
    for (int f = 0; f < NFAC; f++) {
        use(&fac_bmp[f]);
        static const uint8_t wall[NFAC] = { 2, 5, 0, 3, 2, 1 };
        int wc = wall[f];
        if (f == 2) {                                   /* el callejón, entre dos edificios */
            rect(0, 0, 64, FACADE_H, 0);
            dither(8, 0, 48, FACADE_H, 0, 1);
            for (int y = 6; y < 56; y += 10)            /* la escalera de incendios */
                rect(10, y, 24, 1, 7);
            zxv_gfx_line(12, 6, 32, 56, 7);
            rect(10, 56, 24, 2, 7);
            rect(28, 70, 30, 32, 4);                    /* el contenedor, a la cadera */
            rect(28, 70, 30, 3, 0);
            rect(30, 78, 26, 1, 0);
            rect(30, 88, 26, 1, 0);
            rect(0, 0, 6, FACADE_H, 2);
            rect(58, 0, 6, FACADE_H, 1);
            continue;
        }
        if (wc == 2)
            rect(0, 0, 64, FACADE_H, wc);
        else                                            /* de noche, más oscuras */
            dither(0, 0, 64, FACADE_H, wc, wc == 1 ? 0 : 1);
        if (wc == 2)                                    /* ladrillos */
            for (int y = 2; y < 34; y += 4)
                for (int x = (y / 4) & 1 ? 0 : 4; x < 64; x += 8) {
                    rect(x, y, 7, 1, 10);
                    px(x + 7, y + 1, 0);
                }
        for (int k = 0; k < 2; k++) {                   /* el primer piso: el pie de las ventanas */
            int lit = rnd() % 3 == 0;
            window(10 + k * 30, -8, 14, 18, 7, lit ? 14 : 1);
            if (lit)
                rect(11 + k * 30, 2, 6, 7, 6);          /* alguien dentro */
            rect(7 + k * 30, 14, 20, 1, 0);             /* el balcón */
            for (int x = 7 + k * 30; x < 27 + k * 30; x += 3)
                rect(x, 10, 1, 4, 0);
        }
        rect(0, 18, 64, 2, 7);                          /* la moldura de la tienda */
        rect(0, 20, 64, 1, 0);
        static const char *const names[NFAC] = { "BAR", "24H", "", "PIZZA", "GYM", "CAFE" };
        static const uint8_t sbg[NFAC] = { 10, 12, 0, 14, 13, 11 };
        sign(4, 22, f == 4 ? 44 : 56, names[f], sbg[f], 0);
        rect(2, 35, 60, FACADE_H - 35, 0);              /* el hueco de la planta baja */
        if (f == 1 || f == 5) {                         /* un escaparate encendido y la puerta */
            rect(3, 36, 38, FACADE_H - 36, f == 1 ? 5 : 6);
            dither(3, 36, 38, 14, f == 1 ? 13 : 14, f == 1 ? 5 : 6);
            rect(3, 74, 38, 2, 0);                      /* la repisa, a la cintura */
            for (int k = 0; k < 4; k++)                 /* lo que se vende */
                rect(6 + k * 9, 64, 6, 10, k & 1 ? 10 : 15);
            rect(3, 76, 38, FACADE_H - 76, 1);
            rect(44, 36, 16, FACADE_H - 36, 1);         /* la puerta */
            rect(45, 38, 14, 30, f == 1 ? 5 : 6);       /* con cristal */
            rect(56, 70, 2, 4, 14);                     /* el pomo */
        } else {                                        /* el cierre metálico, con pintadas */
            rect(3, 36, 58, FACADE_H - 36, 7);
            for (int y = 37; y < FACADE_H; y += 2)
                rect(3, y, 58, 1, 5);
            rect(3, 36, 58, 3, 0);                      /* la caja del cierre */
            graffiti(6, 60, 52, 34);
        }
        if (f == 4) {                                   /* un letrero de neón, de canto */
            rect(51, 0, 11, 44, 0);
            text_target(cur);
            const char *n = "HOTEL";
            for (int k = 0; n[k]; k++) {
                char ch[2] = { n[k], 0 };
                text(54, 2 + k * 8, ch, 11);
            }
            text_target(0);
        }
        if (f == 3) {                                   /* una farola */
            rect(60, 4, 2, FACADE_H - 4, 0);
            rect(53, 4, 9, 3, 0);
            rect(53, 7, 5, 2, 14);
        }
    }

    use(&floor_bmp);
    rect(0, 0, 256, 46, 7);                             /* la acera, ancha */
    for (int y = 0; y < 46; y++)
        for (int x = 0; x < 256; x += 32)
            px(x + y / 2, y, 1);
    rect(0, 15, 256, 1, 1);
    rect(0, 30, 256, 1, 1);
    rect(0, 46, 256, 3, 15);                            /* el bordillo */
    rect(0, 49, 256, 1, 0);
    rect(0, 50, 256, FLOOR_H - 50, 0);                  /* el asfalto */
    for (int k = 0; k < 200; k++)
        px((int)(rnd() % 256), 50 + (int)(rnd() % (FLOOR_H - 50)), 1);
    for (int x = 0; x < 256; x += 64)
        rect(x + 8, 63, 28, 2, 6);
}

/* Fase 2, el mercado de día. */
static void scene_market(void)
{
    static const uint8_t sky[] = { 5, 13, 13 };
    use(&far_bmp);
    sky_bands(sky, 3, SKY_H);
    for (int k = 0; k < 4; k++) {                       /* las nubes */
        int x = 20 + k * 64 + (int)(rnd() % 20), y = 6 + (int)(rnd() % 12);
        zxv_gfx_circle(x, y, 5, 15, 1);
        zxv_gfx_circle(x + 7, y - 2, 6, 15, 1);
        zxv_gfx_circle(x + 15, y, 5, 15, 1);
        rect(x, y, 16, 5, 15);
    }
    int x = 0;                                          /* los tejados de lejos */
    while (x < 256) {
        int w = 20 + (int)(rnd() % 24), h = 14 + (int)(rnd() % 18);
        rect(x, SKY_H - h, w, h, 6);
        int16_t roof[6] = { (int16_t)(x - 2), (int16_t)(SKY_H - h), (int16_t)(x + w / 2), (int16_t)(SKY_H - h - 8),
                            (int16_t)(x + w + 2), (int16_t)(SKY_H - h) };
        zxv_gfx_tri(roof, 2);
        for (int wx = x + 4; wx < x + w - 4; wx += 7)
            rect(wx, SKY_H - h + 4, 3, 4, 1);
        x += w + 2;
    }
    rect(150, 6, 14, SKY_H, 7);                         /* el campanario */
    int16_t spire[6] = { 148, 6, 157, -6, 166, 6 };
    zxv_gfx_tri(spire, 2);
    zxv_gfx_circle(157, 16, 4, 15, 1);
    zxv_gfx_line(157, 16, 157, 13, 0);
    zxv_gfx_line(157, 16, 159, 16, 0);
    rect(154, 26, 6, 10, 0);
    mid_colour = 6;

    for (int f = 0; f < NFAC; f++) {
        use(&fac_bmp[f]);
        rect(0, 20, 64, FACADE_H - 20, 6);              /* la nave del mercado */
        rect(0, 20, 64, 3, 14);
        rect(0, 23, 64, 1, 2);
        if (f == 0 || f == 4) {                         /* los arcos */
            for (int k = 0; k < 2; k++) {
                zxv_gfx_circle(16 + k * 32, 48, 10, 0, 1);
                rect(6 + k * 32, 48, 21, FACADE_H - 48, 0);
                zxv_gfx_circle(16 + k * 32, 48, 9, 1, 1);
                rect(7 + k * 32, 48, 19, FACADE_H - 48, 1);
            }
            if (f == 4) {                               /* banderines */
                for (int k = 0; k < 8; k++) {
                    int16_t t[6] = { (int16_t)(k * 8), 26, (int16_t)(k * 8 + 8), 26, (int16_t)(k * 8 + 4), 32 };
                    zxv_gfx_tri(t, k % 3 == 0 ? 10 : k % 3 == 1 ? 12 : 9);
                }
                zxv_gfx_line(0, 26, 63, 26, 0);
            }
            continue;
        }
        if (f == 5) {                                   /* cajas y un carro */
            sign(6, 30, 52, "MERCADO", 10, 15);
            for (int k = 0; k < 3; k++) {
                rect(4 + k * 18, 82, 16, 20, 0);
                rect(5 + k * 18, 83, 14, 18, 6);
                rect(5 + k * 18, 90, 14, 1, 0);
            }
            rect(10, 70, 16, 12, 0);
            rect(11, 71, 14, 10, 6);
            for (int k = 0; k < 4; k++)
                zxv_gfx_circle(13 + k * 3, 70, 2, 12, 1);
            continue;
        }
        /* Un puesto con su toldo de rayas. */
        static const uint8_t awn[NFAC] = { 0, 10, 12, 9, 0, 0 };
        rect(4, 40, 56, FACADE_H - 40, 1);
        for (int k = 0; k < 7; k++)
            rect(2 + k * 8, 36, 8, 12, k & 1 ? 15 : awn[f]);
        for (int k = 0; k < 7; k++)                     /* el borde ondulado */
            zxv_gfx_circle(6 + k * 8, 48, 3, k & 1 ? 15 : awn[f], 1);
        rect(2, 35, 60, 1, 0);
        rect(6, 76, 52, 26, 0);                         /* el mostrador */
        rect(7, 77, 50, 25, f == 3 ? 7 : 6);
        rect(7, 84, 50, 1, 0);
        for (int k = 0; k < 24; k++) {                  /* lo que se vende */
            int c = f == 1 ? (k & 1 ? 10 : 14) : f == 2 ? (k & 1 ? 12 : 4) : (k & 1 ? 15 : 13);
            int ix = 9 + (k % 12) * 4, iy = 70 + (k / 12) * 4;
            if (f == 3)
                rect(ix, iy + 1, 4, 2, c);
            else
                zxv_gfx_circle(ix + 1, iy + 1, 2, c, 1);
        }
        rect(30, 52, 4, 6, 14);                         /* la bombilla */
        rect(31, 48, 2, 4, 0);
    }

    use(&floor_bmp);                                    /* los adoquines */
    rect(0, 0, 256, FLOOR_H, 1);
    for (int y = 0; y < FLOOR_H; y += 6)
        for (int x = (y / 6) & 1 ? -4 : 0; x < 256; x += 8)
            rect(x + 1, y + 1, 7, 5, (x * 7 + y) % 3 ? 7 : 15);
    rect(0, 0, 256, 2, 0);
}

/* Fase 3, las azoteas al atardecer. */
static void scene_roofs(void)
{
    static const uint8_t sky[] = { 1, 3, 11, 10, 14 };
    use(&far_bmp);
    sky_bands(sky, 5, SKY_H);
    zxv_gfx_circle(64, SKY_H - 6, 14, 14, 1);
    zxv_gfx_circle(64, SKY_H - 6, 11, 15, 1);
    skyline(SKY_H, 8, 40, 1, 14, 18);
    mid_colour = 1;

    for (int f = 0; f < NFAC; f++) {
        use(&fac_bmp[f]);
        switch (f) {
        case 1:                                         /* un depósito de agua */
            rect(14, 16, 36, 30, 0);
            rect(15, 17, 34, 28, 7);
            for (int y = 20; y < 44; y += 6)
                rect(15, y, 34, 1, 5);
            rect(14, 12, 36, 5, 0);
            rect(18, 46, 2, 26, 0);
            rect(44, 46, 2, 26, 0);
            zxv_gfx_line(18, 50, 44, 68, 0);
            zxv_gfx_line(44, 50, 18, 68, 0);
            break;
        case 2:                                         /* chimeneas y antena */
            for (int k = 0; k < 2; k++) {
                rect(8 + k * 24, 40 - k * 8, 14, 40, 0);
                rect(9 + k * 24, 41 - k * 8, 12, 39, 2);
                for (int y = 44 - k * 8; y < 78; y += 4)
                    rect(9 + k * 24, y, 12, 1, 10);
                rect(7 + k * 24, 38 - k * 8, 16, 3, 0);
            }
            rect(52, 6, 1, 70, 0);
            for (int k = 0; k < 4; k++)
                rect(46 + k, 10 + k * 8, 12 - 2 * k, 1, 0);
            break;
        case 3: {                                       /* la ropa tendida */
            rect(4, 30, 2, 50, 0);
            rect(58, 30, 2, 50, 0);
            zxv_gfx_line(5, 32, 32, 36, 7);
            zxv_gfx_line(32, 36, 58, 32, 7);
            static const uint8_t cl[5] = { 10, 14, 9, 15, 12 };
            for (int k = 0; k < 5; k++) {
                int x = 9 + k * 10, y = 33 + (k == 2 || k == 1 ? 2 : 1);
                rect(x - 1, y, 9, 12 + (k & 1) * 4, 0);
                rect(x, y + 1, 7, 10 + (k & 1) * 4, cl[k]);
                if (!(k & 1))
                    rect(x - 2, y + 1, 11, 4, cl[k]);
            }
            break;
        }
        case 4:                                         /* la caseta de la escalera */
            rect(6, 22, 52, 60, 0);
            rect(7, 23, 50, 59, 6);
            rect(4, 18, 56, 5, 2);
            rect(4, 17, 56, 1, 0);
            rect(22, 44, 18, 38, 0);
            rect(23, 45, 16, 37, 1);
            rect(36, 62, 2, 2, 14);
            window(44, 32, 8, 8, 0, 13);
            break;
        case 5:                                         /* un cartel luminoso */
            rect(4, 8, 56, 30, 0);
            rect(6, 10, 52, 26, 1);
            sign(8, 12, 48, "CITY", 11, 15);
            sign(8, 24, 48, "NIGHTS", 14, 0);
            for (int k = 0; k < 3; k++)
                rect(12 + k * 18, 38, 2, 40, 0);
            break;
        default:
            break;
        }
        /* El pretil, en todas. */
        rect(0, 70, 64, FACADE_H - 70, 7);
        rect(0, 70, 64, 2, 15);
        rect(0, 72, 64, 1, 0);
        for (int y = 76; y < FACADE_H; y += 6)
            for (int x = (y / 6) & 1 ? 0 : 8; x < 64; x += 16)
                rect(x, y, 15, 1, 5);
    }

    use(&floor_bmp);                                    /* las baldosas de barro */
    rect(0, 0, 256, FLOOR_H, 0);
    for (int y = 0; y < FLOOR_H; y += 8)
        for (int x = (y / 8) & 1 ? -8 : 0; x < 256; x += 16) {
            rect(x + 1, y + 1, 15, 7, 2);
            rect(x + 1, y + 1, 15, 2, 10);
        }
}

void scenery_make(int stage)
{
    uint32_t seed = rng_state;
    rng_state = 1234u + (uint32_t)stage * 77u;          /* el mismo barrio cada vez */
    bmp_init(&far_bmp, far_mem, 256, SKY_H);
    bmp_init(&floor_bmp, floor_mem, 256, FLOOR_H);
    for (int f = 0; f < NFAC; f++)
        bmp_init(&fac_bmp[f], fac_mem[f], SLOT_W, FACADE_H);
    switch (stage) {
    case 0:
        scene_street();
        stage_slots = slots1;
        stage_nslots = sizeof(slots1);
        break;
    case 1:
        scene_market();
        stage_slots = slots2;
        stage_nslots = sizeof(slots2);
        break;
    default:
        scene_roofs();
        stage_slots = slots3;
        stage_nslots = sizeof(slots3);
        break;
    }
    zxv_gfx_target(0);
    zxv_gfx_clip(0, 0, W, H);
    rng_state = seed;
}

void scenery_draw(int cam)
{
    int off = (cam / 4) & 255;
    zxv_gfx_blit(&far_bmp, off, 0, 256 - off, SKY_H, 0, SKY_Y, 0);
    if (off)
        zxv_gfx_blit(&far_bmp, 0, 0, off, SKY_H, 256 - off, SKY_Y, 0);
    zxv_gfx_rect(0, SKY_Y + SKY_H, W, FLOOR_Y - SKY_Y - SKY_H, mid_colour);
    for (int s = cam / SLOT_W; s <= cam / SLOT_W + 4 && s < stage_nslots; s++)
        zxv_gfx_blit(&fac_bmp[stage_slots[s]], 0, 0, SLOT_W, FACADE_H, s * SLOT_W - cam, HUD_H, ZXV_BLIT_TRANSPARENT);
    off = cam & 255;
    zxv_gfx_blit(&floor_bmp, off, 0, 256 - off, FLOOR_H, 0, FLOOR_Y, 0);
    if (off)
        zxv_gfx_blit(&floor_bmp, 0, 0, off, FLOOR_H, 256 - off, FLOOR_Y, 0);
}
