/*
 * view.c - La vista en 3D de Umbra, en las 144 líneas de arriba de la
 * pantalla:
 *
 *   - Las paredes, por raycasting: un rayo por cada columna de 2 píxeles
 *     (128), que avanza por la cuadrícula del laberinto hasta dar con una
 *     pared. La columna se escribe directamente en la página, byte a byte:
 *     cada byte son sus dos píxeles, del mismo color, sin leer lo que había.
 *   - El suelo y el techo, con el modo 7 del motor (zxv_m7_camera y
 *     zxv_m7_draw), con la misma cámara: el techo usa las líneas del suelo
 *     del otro lado del horizonte. Solo se dibujan las líneas que alguna
 *     pared deja ver.
 *   - La niebla: lo que alumbra la lámpara se ve con sus colores; más lejos,
 *     sin brillo, luego con un tramado de negro y al final negro. Las
 *     paredes y los objetos cambian de color al dibujarse (tablas de 16);
 *     el suelo y el techo, una línea entera de una vez, con una tabla de
 *     256 por byte (sus dos píxeles), que además le pone los colores del
 *     nivel.
 *   - Los objetos y las sombras, sprites escalados de lejos a cerca, cortados
 *     columna a columna por las paredes que tienen delante (zxv_gfx_clip con
 *     los tramos en los que la pared está más lejos que el objeto).
 */
#include "umbra.h"

/* El seno, el mismo que usa el motor para el modo 7 (así el suelo y las
 * paredes casan): un cuarto de vuelta, en 256 pasos. */
const int32_t sin_q[257] = {
    0, 402, 804, 1206, 1608, 2010, 2412, 2814, 3216, 3617,
    4019, 4420, 4821, 5222, 5623, 6023, 6424, 6824, 7224, 7623,
    8022, 8421, 8820, 9218, 9616, 10014, 10411, 10808, 11204, 11600,
    11996, 12391, 12785, 13180, 13573, 13966, 14359, 14751, 15143, 15534,
    15924, 16314, 16703, 17091, 17479, 17867, 18253, 18639, 19024, 19409,
    19792, 20175, 20557, 20939, 21320, 21699, 22078, 22457, 22834, 23210,
    23586, 23961, 24335, 24708, 25080, 25451, 25821, 26190, 26558, 26925,
    27291, 27656, 28020, 28383, 28745, 29106, 29466, 29824, 30182, 30538,
    30893, 31248, 31600, 31952, 32303, 32652, 33000, 33347, 33692, 34037,
    34380, 34721, 35062, 35401, 35738, 36075, 36410, 36744, 37076, 37407,
    37736, 38064, 38391, 38716, 39040, 39362, 39683, 40002, 40320, 40636,
    40951, 41264, 41576, 41886, 42194, 42501, 42806, 43110, 43412, 43713,
    44011, 44308, 44604, 44898, 45190, 45480, 45769, 46056, 46341, 46624,
    46906, 47186, 47464, 47741, 48015, 48288, 48559, 48828, 49095, 49361,
    49624, 49886, 50146, 50404, 50660, 50914, 51166, 51417, 51665, 51911,
    52156, 52398, 52639, 52878, 53114, 53349, 53581, 53812, 54040, 54267,
    54491, 54714, 54934, 55152, 55368, 55582, 55794, 56004, 56212, 56418,
    56621, 56823, 57022, 57219, 57414, 57607, 57798, 57986, 58172, 58356,
    58538, 58718, 58896, 59071, 59244, 59415, 59583, 59750, 59914, 60075,
    60235, 60392, 60547, 60700, 60851, 60999, 61145, 61288, 61429, 61568,
    61705, 61839, 61971, 62101, 62228, 62353, 62476, 62596, 62714, 62830,
    62943, 63054, 63162, 63268, 63372, 63473, 63572, 63668, 63763, 63854,
    63944, 64031, 64115, 64197, 64277, 64354, 64429, 64501, 64571, 64639,
    64704, 64766, 64827, 64884, 64940, 64993, 65043, 65091, 65137, 65180,
    65220, 65259, 65294, 65328, 65358, 65387, 65413, 65436, 65457, 65476,
    65492, 65505, 65516, 65525, 65531, 65535, 65536
};

int32_t isin(int a)
{
    a &= TURN - 1;
    int q = a >> 8, o = a & 255;
    int32_t v = sin_q[q & 1 ? 256 - o : o];
    return q & 2 ? -v : v;
}

/* --- La niebla ------------------------------------------------------------------- */

enum { FOG_LEVELS = 5 };               /* 0 los colores, 4 negro */

/* Un píxel de color c en (x, y) con la niebla f: sin brillo (1); con la
 * mitad de los píxeles en negro, en tablero (2); con tres de cada cuatro en
 * negro (3). */
static int fog(int f, int x, int y, int c)
{
    if (c == ZXV_TRANSPARENT)
        c = 0;
    switch (f) {
    case 0: return c;
    case 1: return c & 7;
    case 2: return ((x + y) & 1) ? 0 : c & 7;
    case 3: return ((x + 2 * y) & 3) ? 0 : c & 7;
    default: return 0;
    }
}

/* El nivel de niebla a la distancia z (16.16, en celdas) con la lámpara de
 * alcance light. */
static int fog_level(int32_t z, int32_t light)
{
    if (z >= light)
        return 4;
    int32_t r = (z << 8) / light;       /* de 0 a 256; light no llega a 128 celdas */
    return r < 120 ? 0 : r < 170 ? 1 : r < 212 ? 2 : 3;
}

/* Las paredes: por nivel de niebla, la columna (x mod 4) y la línea (y mod
 * 4), el byte de cada color (los dos píxeles iguales). */
static uint8_t wall_lut[FOG_LEVELS][4][4][16];
/* El suelo y el techo: por nivel (sin el 4, que es negro), la línea (y mod
 * 4) y el byte (par o impar: x mod 4), el byte que queda. Llevan los colores
 * del nivel. */
static uint8_t floor_lut[4][4][2][256], ceiling_lut[4][4][2][256];
/* Los objetos: por nivel, su tabla de colores para zxv_spr_draw; las sombras
 * tienen la suya, con los ojos que se ven a oscuras. */
static uint8_t item_fog[FOG_LEVELS][16], shadow_fog[FOG_LEVELS][16];

/* Los colores de cada nivel para el suelo y el techo, según la pared que más
 * hay: el oscuro (1), el medio (7) y el claro (15) del dibujo. */
static const uint8_t floor_colours[5][3] = {
    { 1, 7, 15 }, { 2, 6, 14 }, { 4, 7, 12 }, { 1, 5, 13 }, { 3, 6, 14 },
};
static const uint8_t ceiling_colours[5][3] = {
    { 1, 7, 15 }, { 2, 3, 11 }, { 4, 1, 12 }, { 1, 5, 13 }, { 3, 1, 11 },
};

static void make_surface(uint8_t lut[4][4][2][256], const uint8_t *colours)
{
    uint8_t rc[16];
    for (int c = 0; c < 16; c++)
        rc[c] = (uint8_t)c;
    rc[1] = colours[0];
    rc[7] = colours[1];
    rc[15] = colours[2];
    for (int f = 0; f < 4; f++)
        for (int y = 0; y < 4; y++)
            for (int k = 0; k < 2; k++)
                for (int b = 0; b < 256; b++)
                    lut[f][y][k][b] = (uint8_t)(fog(f, 2 * k, y, rc[b & 15]) |
                                                fog(f, 2 * k + 1, y, rc[b >> 4]) << 4);
}

void view_level(int wall)
{
    static int made;
    if (!made) {
        made = 1;
        for (int f = 0; f < FOG_LEVELS; f++)
            for (int x = 0; x < 4; x++)
                for (int y = 0; y < 4; y++)
                    for (int c = 0; c < 16; c++)
                        wall_lut[f][x][y][c] = (uint8_t)(fog(f, x, y, c) * 0x11);
        for (int f = 0; f < FOG_LEVELS; f++)
            for (int c = 0; c < 16; c++) {
                int t = c == ZXV_TRANSPARENT;
                item_fog[f][c] = (uint8_t)(t ? c : f == 0 ? c : f < 3 ? c & 7 : c ? 1 : 0);
                shadow_fog[f][c] = (uint8_t)(t ? c : c == 10 ? (f < 2 ? 10 : 2)
                                             : f == 0 ? c : f == 1 ? c & 7 : f == 2 ? (c ? 1 : 0)
                                             : ZXV_TRANSPARENT);
            }
    }
    make_surface(floor_lut, floor_colours[wall]);
    make_surface(ceiling_lut, ceiling_colours[wall]);
}

/* --- Los rayos ------------------------------------------------------------------- */

static int32_t col_off[COLS];           /* el desplazamiento lateral de cada rayo, 16.16 */

static struct {
    int32_t z;                          /* la distancia perpendicular, 16.16 en celdas */
    int16_t y0, y1;                     /* las líneas de la pared, sin y1 */
    int32_t ty0, step;                  /* el texel de la línea y0 y el paso por línea, 16.16 */
    const uint8_t *tex;                 /* la columna de la textura (filas de 128) */
    uint8_t fog, side;
} col[COLS];

static int32_t abs32(int32_t v) { return v < 0 ? -v : v; }

/* El rayo de la columna c desde (px, py) en la dirección (rx, ry). */
static void cast(int c, int32_t px, int32_t py, int32_t rx, int32_t ry, int32_t light)
{
    int mx = px >> 16, my = py >> 16;
    int sx = rx < 0 ? -1 : 1, sy = ry < 0 ? -1 : 1;
    uint32_t ddx = rx ? 0xFFFFFFFFu / (uint32_t)abs32(rx) : 0x7FFFFFFFu;
    uint32_t ddy = ry ? 0xFFFFFFFFu / (uint32_t)abs32(ry) : 0x7FFFFFFFu;
    if (ddx > 0x7FFFFFFFu)
        ddx = 0x7FFFFFFFu;
    if (ddy > 0x7FFFFFFFu)
        ddy = 0x7FFFFFFFu;
    int32_t fx = px & 0xFFFF, fy = py & 0xFFFF;
    int64_t side_x = (int64_t)(rx < 0 ? fx : ONE - fx) * ddx >> 16;
    int64_t side_y = (int64_t)(ry < 0 ? fy : ONE - fy) * ddy >> 16;
    int side = 0, cell = CELL_FLOOR;
    for (int steps = 0; steps < 2 * MAZE_MAX; steps++) {
        if (side_x < side_y) {
            side_x += ddx;
            mx += sx;
            side = 0;
        } else {
            side_y += ddy;
            my += sy;
            side = 1;
        }
        cell = cell_at(mx, my);
        if ((unsigned)mx < MAZE_MAX && (unsigned)my < MAZE_MAX)
            seen[my * MAZE_MAX + mx] = 1;
        if (cell != CELL_FLOOR)
            break;
    }
    int64_t z = side ? side_y - ddy : side_x - ddx;
    if (z < ONE / 16)
        z = ONE / 16;
    if (z > 0x7FFFFFFF)
        z = 0x7FFFFFFF;
    /* Dónde da en la pared, de 0 a 63, de izquierda a derecha mirándola. */
    int32_t hit = side ? px + (int32_t)(z * rx >> 16) : py + (int32_t)(z * ry >> 16);
    int tx = (hit & 0xFFFF) >> 10;
    if ((side == 0 && rx > 0) || (side == 1 && ry < 0))
        tx = 63 - tx;
    int wall = cell == CELL_GATE ? (gate_open ? WALL_OPEN : WALL_GATE) : cell - CELL_WALL;

    /* La altura en pantalla, en 1/256 de píxel: una celda a la distancia z
     * mide FOCAL / z. */
    uint32_t h8 = ((uint32_t)FOCAL << 24) / (uint32_t)z;
    int32_t top8 = HORIZON * 256 - (int32_t)(h8 / 2);
    int y0 = top8 < 0 ? 0 : (top8 + 255) >> 8;
    int32_t bottom8 = HORIZON * 256 + (int32_t)(h8 / 2);
    int y1 = bottom8 > VIEW_H * 256 ? VIEW_H : (bottom8 + 255) >> 8;
    uint32_t step = ((uint32_t)64 << 24) / (h8 ? h8 : 1);
    col[c].z = (int32_t)z;
    col[c].y0 = (int16_t)y0;
    col[c].y1 = (int16_t)(y1 > y0 ? y1 : y0);
    col[c].step = (int32_t)step;
    col[c].ty0 = (int32_t)(((int64_t)(y0 * 256 + 128 - top8) * step) >> 8);
    col[c].tex = art_wall(wall) + tx;
    col[c].side = (uint8_t)side;
    col[c].fog = (uint8_t)fog_level((int32_t)z, light);
}

static void draw_wall(uint8_t *page, int c)
{
    int f = col[c].fog;
    uint8_t *p = page + col[c].y0 * (W / 2) + c;
    int y = col[c].y0, n = col[c].y1 - y;
    if (f == 4) {                       /* a oscuras: negro */
        for (; n > 0; n--, p += W / 2)
            *p = 0;
        return;
    }
    if (f == 0 && col[c].side)          /* las caras de un lado, algo más oscuras */
        f = 1;
    const uint8_t (*lut)[16] = wall_lut[f][c & 3];
    const uint8_t *tex = col[c].tex;
    uint32_t ty = (uint32_t)col[c].ty0, step = (uint32_t)col[c].step;
    for (; n > 0; n--, y++, p += W / 2, ty += step)
        *p = lut[y & 3][tex[((ty >> 16) & 63) * 128]];
}

/* --- El suelo y el techo ------------------------------------------------------------- */

static zxv_m7line lines[VIEW_H + 1], ceiling_lines[HORIZON];

static void black_rows(uint8_t *page, int y0, int y1)
{
    for (int y = y0; y < y1; y++) {
        uint32_t *d = (uint32_t *)(page + y * (W / 2));
        for (int i = 0; i < W / 8; i++)
            d[i] = 0;
    }
}

/* La niebla y los colores del nivel en la línea y. */
static void tint_row(uint8_t *page, int y, uint8_t lut[4][2][256])
{
    uint8_t *r = page + y * (W / 2);
    const uint8_t *a = lut[y & 3][0], *b = lut[y & 3][1];
    for (int k = 0; k < W / 2; k += 2) {
        r[k] = a[r[k]];
        r[k + 1] = b[r[k + 1]];
    }
}

/* Las líneas de y0 a y1 del suelo (o del techo), la primera con las líneas
 * del modo 7 l, con la niebla de cada una (row_fog[y]). */
static void surface(uint8_t *page, const zxv_m7tex *t, const zxv_m7line *l, int y0, int y1,
                    const uint8_t *row_fog, uint8_t lut[4][4][2][256])
{
    int y = y0;
    while (y < y1) {
        int f = row_fog[y], e = y + 1;
        while (e < y1 && row_fog[e] == f)
            e++;
        if (f == 4 || !t->texels) {
            black_rows(page, y, e);
        } else {
            zxv_m7_draw(t, l + (y - y0), y, e - y);
            for (int k = y; k < e; k++)
                tint_row(page, k, lut[f]);
        }
        y = e;
    }
}

/* --- Los objetos -------------------------------------------------------------------- */

typedef struct {
    int32_t z;
    int x, w, h, bottom;
    const sprite_t *s;
    const uint8_t *colours;
} thing_t;

static void draw_thing(const thing_t *o)
{
    int x0 = o->x - o->w / 2, x1 = x0 + o->w;
    int c0 = x0 < 0 ? 0 : x0 >> 1, c1 = x1 >= W ? COLS - 1 : (x1 - 1) >> 1;
    /* Los tramos de columnas en los que la pared está detrás. */
    for (int c = c0; c <= c1;) {
        while (c <= c1 && col[c].z <= o->z)
            c++;
        int s = c;
        while (c <= c1 && col[c].z > o->z)
            c++;
        if (c > s) {
            zxv_gfx_clip(2 * s, 0, 2 * c, VIEW_H);
            zxv_spr_draw_scaled(o->s->spr, x0, o->bottom - o->h, o->w, o->h, 0, o->colours);
        }
    }
}

/* Lo que hay en el laberinto, visto desde la cámara: en things, de lejos a
 * cerca. */
static int gather(thing_t *things, const cam_t *cam, int32_t light, int t)
{
    int n = 0;
    int32_t ca = icos(cam->ang), sa = isin(cam->ang);
    for (int i = 0; i < item_count + shadow_count; i++) {
        int32_t ox, oy;
        const sprite_t *s;
        int lift = 0, shadow = i >= item_count;
        if (!shadow) {
            const item_t *it = &items[i];
            if (it->taken)
                continue;
            ox = it->x;
            oy = it->y;
            if (it->kind == ITEM_RUNE) {
                s = &art[SPR_RUNE];
                lift = 12 + (isin(t * 6 + i * 200) >> 14);   /* flota */
            } else {
                s = &art[SPR_FLASK];
            }
        } else {
            const shadow_t *sh = &shadows[i - item_count];
            ox = sh->x;
            oy = sh->y;
            s = &art[SPR_SHADOW + (t / 8 + i) % 3];
            lift = 2 + (isin(t * 4 + i * 300) >> 15);
        }
        if (!s->spr)
            continue;
        int32_t dx = ox - cam->x, dy = oy - cam->y;
        int32_t z = (int32_t)(((int64_t)dx * ca + (int64_t)dy * sa) >> 16);
        if (z < ONE / 5)
            continue;
        int32_t u = (int32_t)(((int64_t)dy * ca - (int64_t)dx * sa) >> 16);
        int f = fog_level(z, light);
        if (f == 4 && !shadow)
            continue;
        thing_t *o = &things[n];
        /* Un píxel de una sombra es un texel, 1/64 de celda; los objetos son
         * algo más pequeños, 1/96. */
        int32_t scale = (FOCAL << 16) / z;  /* píxeles por celda */
        int per_cell = shadow ? 64 : 96;
        o->z = z;
        o->x = W / 2 + (int)(((int64_t)u * scale) >> 16);
        o->w = (int)(s->w * scale / per_cell);
        o->h = (int)(s->h * scale / per_cell);
        o->bottom = HORIZON + scale / 2 - lift * scale / 64;
        o->s = s;
        o->colours = shadow ? shadow_fog[f] : item_fog[f];
        if (o->w < 1 || o->h < 1 || o->x + o->w / 2 < 0 || o->x - o->w / 2 >= W)
            continue;
        n++;
    }
    for (int i = 1; i < n; i++)         /* de lejos a cerca */
        for (int j = i; j > 0 && things[j].z > things[j - 1].z; j--) {
            thing_t tmp = things[j];
            things[j] = things[j - 1];
            things[j - 1] = tmp;
        }
    return n;
}

/* --- Todo ------------------------------------------------------------------------- */

void view_draw(const cam_t *cam, int32_t light, int t)
{
    static int ready;
    if (!ready) {
        ready = 1;
        for (int c = 0; c < COLS; c++)
            col_off[c] = (int32_t)(((int32_t)(2 * c + 1 - W / 2) << 16) / FOCAL);
    }
    uint8_t *page = video_page();
    int32_t ca = icos(cam->ang), sa = isin(cam->ang);
    if ((uint32_t)(cam->x >> 16) < MAZE_MAX && (uint32_t)(cam->y >> 16) < MAZE_MAX)
        seen[(cam->y >> 16) * MAZE_MAX + (cam->x >> 16)] = 1;   /* donde se está, también */

    /* Los rayos, y las líneas que todas las paredes tapan: el techo se ve
     * hasta la pared más baja (hidden0) y el suelo desde ella (hidden1). */
    int hidden0 = 0, hidden1 = VIEW_H;
    for (int c = 0; c < COLS; c++) {
        int32_t o = col_off[c];
        cast(c, cam->x, cam->y, ca - (int32_t)(((int64_t)sa * o) >> 16),
             sa + (int32_t)(((int64_t)ca * o) >> 16), light);
        if (col[c].y0 > hidden0)
            hidden0 = col[c].y0;
        if (col[c].y1 < hidden1)
            hidden1 = col[c].y1;
    }

    /* El suelo y el techo, con la niebla de cada línea: la del suelo a la
     * distancia de esa línea. */
    static uint8_t row_fog[VIEW_H];
    for (int y = 0; y < VIEW_H; y++) {
        int dy = y >= HORIZON ? y - HORIZON : HORIZON - y;
        row_fog[y] = (uint8_t)(dy ? fog_level((int32_t)(((int32_t)FOCAL << 15) / dy), light) : 4);
    }
    zxv_m7cam m7 = {
        .x = cam->x * 64, .y = cam->y * 64,         /* en texels: una celda son 64 */
        .height = 32 << 16,                         /* los ojos, a media pared */
        .angle = cam->ang * (0x10000 / TURN),
        .focal = FOCAL << 16,
        .horizon = HORIZON,
    };
    zxv_m7_camera(lines, VIEW_H + 1, &m7);
    for (int y = 0; y < hidden0 && y < HORIZON; y++)
        ceiling_lines[y] = lines[2 * HORIZON - y];
    surface(page, &tex_ceiling, ceiling_lines, 0, hidden0 < HORIZON ? hidden0 : HORIZON, row_fog,
            ceiling_lut);
    int f0 = hidden1 > HORIZON ? hidden1 : HORIZON;
    surface(page, &tex_floor, lines + f0, f0, VIEW_H, row_fog, floor_lut);

    for (int c = 0; c < COLS; c++)
        draw_wall(page, c);

    static thing_t things[ITEMS + SHADOWS];
    int n = gather(things, cam, light, t);
    for (int i = 0; i < n; i++)
        draw_thing(&things[i]);
    zxv_gfx_clip(0, 0, W, H);
}
