/*
 * road.c - La carretera de Route 128, en pseudo-3D por segmentos.
 *
 * El circuito es una tira de segmentos de SEG_LEN, cada uno con su curva y la
 * altura de su principio. Cada frame se proyectan los extremos de los
 * DRAW_SEGS segmentos que hay delante de la cámara: la curva se acumula
 * (cada segmento desvía un poco más los siguientes) y la altura sube o baja
 * la línea de cada uno. La carretera se pinta de cerca a lejos, línea a línea:
 * cada línea de la pantalla la pinta el segmento más cercano que la cubre, así
 * que lo que hay detrás de un cambio de rasante no se ve hasta coronarlo.
 * Encima, el paisaje (cielo y montañas en dos capas, que se mueven con las
 * curvas) y, de lejos a cerca, los objetos y los coches, escalados según la
 * distancia y recortados por la cresta que tengan delante. El coche del
 * jugador va entre ellos, en su sitio: lo que hay entre él y la cámara (el
 * arco de la salida al pasar por debajo) lo tapa.
 *
 * La carretera entre la cámara y el coche se pinta a la altura del coche,
 * plana, como en los juegos de este tipo: la cámara va detrás y más baja que
 * una cima que el coche ya ha pasado, y con la carretera de verdad la cima
 * taparía la carretera bajo el coche (que se vería flotando al coronar).
 */
#include "route128.h"

#define MAX_SEGS    4600
#define MAX_OBJS    2600
#define RUMBLE      3                   /* segmentos de cada banda de color */

static int8_t curves[MAX_SEGS];         /* x 16 */
static int32_t heights[MAX_SEGS + 1];   /* la altura al principio de cada segmento */
static uint8_t themes[MAX_SEGS];
static uint16_t first_obj[MAX_SEGS + 1];
static struct { int16_t x; uint8_t type; } objs[MAX_OBJS];
static int n_objs;

static int abs_i(int v) { return v < 0 ? -v : v; }

int track_segs;
int32_t track_len;
int goal_seg;
int checkpoint_seg[3];

int seg_curve(int seg) { return curves[seg % track_segs]; }
int32_t seg_height(int seg) { return heights[seg % track_segs]; }
int seg_theme(int seg) { return themes[seg % track_segs]; }
int obj_type(int i) { return objs[i].type; }
int obj_x(int i) { return objs[i].x; }

int seg_objects(int seg, int *first)
{
    seg %= track_segs;
    *first = first_obj[seg];
    return first_obj[seg + 1] - first_obj[seg];
}

int32_t height_at(int32_t z)
{
    int seg = (int)(z / SEG_LEN), f = (int)(z % SEG_LEN);
    int32_t a = heights[seg % track_segs], b = heights[(seg + 1) % track_segs];
    return a + (b - a) * f / SEG_LEN;
}

/* --- El circuito ----------------------------------------------------------- */

/* Curvas suaves: de a a b con p de 0 a 65536. */
static int32_t ease_in(int32_t a, int32_t b, int32_t p)
{
    return a + (int32_t)((int64_t)(b - a) * p / 65536 * p / 65536);
}

static int32_t ease_inout(int32_t a, int32_t b, int32_t p)
{
    int64_t s = (int64_t)p * p / 65536 * (3 * 65536 - 2 * p) / 65536;     /* smoothstep */
    return a + (int32_t)((int64_t)(b - a) * s / 65536);
}

static int n_segs;
static int cur_theme;

static void add_segment(int curve, int32_t y)
{
    if (n_segs >= MAX_SEGS)
        return;
    curves[n_segs] = (int8_t)curve;
    heights[n_segs + 1] = y;
    themes[n_segs] = (uint8_t)cur_theme;
    n_segs++;
}

/* Un tramo: entra, sigue y sale de la curva (en x 16; como en Javascript
 * Racer, 2, 4 o 6 por segmento al cuadrado) mientras sube o baja hill
 * segmentos de altura. */
static void add_road(int enter, int hold, int leave, int curve, int hill)
{
    int32_t y0 = heights[n_segs], y1 = y0 + hill * SEG_LEN;
    int total = enter + hold + leave, n = 0;
    curve *= 16;
    for (int i = 0; i < enter; i++, n++)
        add_segment(ease_in(0, curve, i * 65536 / enter), ease_inout(y0, y1, n * 65536 / total));
    for (int i = 0; i < hold; i++, n++)
        add_segment(curve, ease_inout(y0, y1, n * 65536 / total));
    for (int i = 0; i < leave; i++, n++)
        add_segment(ease_inout(curve, 0, i * 65536 / leave), ease_inout(y0, y1, n * 65536 / total));
}

typedef struct { uint8_t enter, hold, leave; int8_t curve, hill; } section_t;

/* Las tres etapas: la costa, suave; el desierto, con rectas largas y dunas; y
 * el bosque, un puerto de montaña lleno de curvas y rasantes. */
static const section_t stage_coast[] = {
    { 10, 50, 10, 0, 0 }, { 25, 50, 25, 0, 20 }, { 25, 50, 25, 2, 0 }, { 20, 40, 20, 0, -20 },
    { 25, 70, 25, -4, 0 }, { 15, 15, 15, 0, 30 }, { 15, 15, 15, 0, -30 }, { 20, 40, 20, 4, 20 },
    { 30, 60, 30, -2, -20 }, { 20, 30, 20, 6, 0 }, { 10, 40, 10, 0, 0 }, { 15, 20, 15, -4, 0 },
    { 15, 20, 15, 4, 0 }, { 15, 20, 15, -4, 0 }, { 20, 40, 20, 0, 40 }, { 20, 40, 20, 0, -40 },
    { 25, 60, 25, -6, 0 }, { 20, 70, 20, 0, 0 },
};
static const section_t stage_desert[] = {
    { 30, 80, 30, 0, 0 }, { 20, 20, 20, 0, 30 }, { 20, 20, 20, 0, -30 }, { 20, 20, 20, 0, 30 },
    { 20, 20, 20, 0, -30 }, { 30, 80, 30, 3, 0 }, { 30, 60, 30, -3, 10 }, { 20, 40, 20, 0, -10 },
    { 25, 30, 25, 5, 0 }, { 25, 30, 25, -5, 0 }, { 40, 90, 40, 0, 0 }, { 20, 30, 20, 0, 50 },
    { 20, 30, 20, 0, -50 }, { 25, 50, 25, 4, -20 }, { 20, 40, 20, -2, 20 }, { 20, 70, 20, 0, 0 },
};
static const section_t stage_forest[] = {
    { 15, 30, 15, 4, 20 }, { 15, 30, 15, -4, 20 }, { 15, 30, 15, 6, -10 }, { 15, 30, 15, -6, -30 },
    { 20, 40, 20, 0, 40 }, { 20, 20, 20, 3, -40 }, { 15, 30, 15, -5, 0 }, { 15, 30, 15, 5, 10 },
    { 10, 20, 10, -6, 0 }, { 10, 20, 10, 6, 0 }, { 20, 40, 20, 0, -30 }, { 25, 50, 25, -3, 30 },
    { 20, 40, 20, 0, 0 }, { 20, 40, 20, 4, -20 }, { 15, 25, 15, -6, 0 }, { 20, 70, 20, 0, 0 },
};

static void add_stage(const section_t *s, int n, int theme)
{
    cur_theme = theme;
    for (int i = 0; i < n; i++)
        add_road(s[i].enter, s[i].hold, s[i].leave, s[i].curve, s[i].hill);
}

static uint32_t track_rng = 7;
static int trnd(int n)
{
    track_rng = track_rng * 1103515245u + 12345u;
    return (int)((track_rng >> 16) % (uint32_t)n);
}

static void add_obj(int seg, int x, int type)
{
    (void)seg;
    if (n_objs >= MAX_OBJS)
        return;
    objs[n_objs].x = (int16_t)x;
    objs[n_objs].type = (uint8_t)type;
    n_objs++;
}

/* ¿Viene una curva cerrada? El lado de fuera (-1 izquierda, 1 derecha) o 0. */
static int curve_ahead(int s)
{
    if (abs_i(curves[s]) > 16)
        return 0;
    for (int d = 18; d < 30 && s + d < n_segs; d++)
        if (curves[s + d] >= 56 || curves[s + d] <= -56)
            return curves[s + d] > 0 ? -1 : 1;
    return 0;
}

static void place_objects(void)
{
    n_objs = 0;
    for (int s = 0; s < n_segs; s++) {
        first_obj[s] = (uint16_t)n_objs;
        int t = themes[s];
        if (s == 20)
            add_obj(s, 0, OBJ_START);
        else if (s == checkpoint_seg[0] || s == checkpoint_seg[1])
            add_obj(s, 0, OBJ_GATE);
        else if (s == goal_seg)
            add_obj(s, 0, OBJ_GOAL);
        int side = curve_ahead(s);
        if (side && s % 6 == 0) {       /* las flechas, por fuera de la curva */
            add_obj(s, side * (X1 * 135 / 100), side < 0 ? OBJ_SIGN_R : OBJ_SIGN_L);
            continue;
        }
        if (s < 30 || (s > goal_seg - 4 && s < goal_seg + 4))
            continue;
        if (s % 160 == 80) {            /* un cartel de vez en cuando */
            add_obj(s, (s & 1 ? 1 : -1) * (X1 * 17 / 10), OBJ_BOARD);
            continue;
        }
        int r = trnd(100);
        int sgn = trnd(2) ? 1 : -1;
        switch (t) {
        case THEME_COAST:
            if (s % 5 == 0)
                add_obj(s, -(X1 * 15 / 10 + trnd(X1)), OBJ_PALM);    /* el mar, a la izquierda */
            else if (s % 5 == 2 && r < 60)
                add_obj(s, X1 * 14 / 10 + trnd(X1 * 2), r < 25 ? OBJ_UMBRELLA : r < 45 ? OBJ_PALM : OBJ_BUSH);
            break;
        case THEME_DESERT:
            if (s % 7 == 0)
                add_obj(s, sgn * (X1 * 14 / 10 + trnd(X1 * 2)), r < 60 ? OBJ_CACTUS : OBJ_ROCK);
            break;
        case THEME_FOREST:              /* uno cada tres segmentos: de cerca, los pinos son grandes */
            if (s % 6 == 0)
                add_obj(s, -(X1 * 14 / 10 + trnd(X1 * 3 / 2)), r < 70 ? OBJ_PINE : OBJ_TREE);
            else if (s % 6 == 3)
                add_obj(s, X1 * 14 / 10 + trnd(X1 * 3 / 2), r < 50 ? OBJ_PINE : r < 85 ? OBJ_TREE : OBJ_ROCK);
            break;
        }
    }
    first_obj[n_segs] = (uint16_t)n_objs;
}

void track_build(void)
{
    n_segs = 0;
    heights[0] = 0;
    add_stage(stage_coast, sizeof(stage_coast) / sizeof(stage_coast[0]), THEME_COAST);
    checkpoint_seg[0] = n_segs - 30;
    add_stage(stage_desert, sizeof(stage_desert) / sizeof(stage_desert[0]), THEME_DESERT);
    checkpoint_seg[1] = n_segs - 30;
    add_stage(stage_forest, sizeof(stage_forest) / sizeof(stage_forest[0]), THEME_FOREST);
    goal_seg = n_segs - 30;
    checkpoint_seg[2] = goal_seg;
    add_road(0, DRAW_SEGS + 300, 0, 0, 0);     /* después de la meta: la carretera sigue */
    track_segs = n_segs;
    track_len = (int32_t)n_segs * SEG_LEN;
    place_objects();
}

/* --- Los colores de cada etapa ------------------------------------------------ */

typedef struct {
    pat_t ground[2], rumble[2], road[2], lane, far;     /* claro y oscuro */
    pat_t sky[6];                       /* bandas de arriba abajo */
    uint8_t sky_rows[6];
} theme_t;

static const theme_t theme_col[THEMES] = {
    {   /* La costa: hierba, el mar y un cielo despejado. */
        { CHECK(4, 12), SOLID(4) }, { SOLID(15), SOLID(2) }, { CHECK(0, 7), SPARSE(0, 7) },
        SOLID(15), SOLID(4),
        { SOLID(1), CHECK(1, 9), SOLID(9), CHECK(9, 13), SOLID(13), CHECK(13, 15) },
        { 22, 10, 18, 12, 20, 14 },
    },
    {   /* El desierto: arena y un cielo blanco de calor. */
        { CHECK(6, 14), SOLID(6) }, { SOLID(15), SOLID(2) }, { CHECK(0, 7), SPARSE(0, 7) },
        SOLID(15), SOLID(6),
        { SOLID(9), CHECK(9, 13), SOLID(13), CHECK(13, 15), SOLID(15), CHECK(15, 14) },
        { 14, 10, 16, 14, 26, 16 },
    },
    {   /* El bosque: verde oscuro y la tarde. */
        { SOLID(4), CHECK(0, 4) }, { SOLID(15), SOLID(2) }, { CHECK(0, 7), SPARSE(0, 7) },
        SOLID(15), CHECK(0, 4),
        { SOLID(1), CHECK(1, 3), SOLID(3), CHECK(3, 11), SOLID(11), CHECK(11, 14) },
        { 20, 12, 18, 12, 20, 14 },
    },
};

/* --- La proyección ----------------------------------------------------------- */

typedef struct {
    int16_t sx, sy;                     /* el centro de la carretera en la pantalla */
    int32_t sw;                         /* la media carretera, en píxeles x 16 */
} point_t;

static point_t pts[DRAW_SEGS + 2];
static int16_t clip_y[DRAW_SEGS + 1];   /* lo que tapa la carretera de delante */

static int clamp16(int32_t v) { return v < -30000 ? -30000 : v > 30000 ? 30000 : (int)v; }

/* La carretera de un segmento, línea a línea de la cercana a la lejana. */
static void draw_segment(const point_t *n, const point_t *f, int top, int band, const theme_t *t)
{
    /* t: los colores de la etapa del segmento (el cambio de etapa se ve llegar). */
    int dy = n->sy - f->sy;
    if (dy <= 0)
        return;
    int y0 = top < n->sy ? top : n->sy;
    if (y0 > H)
        y0 = H;
    int y1 = f->sy < 0 ? 0 : f->sy;
    for (int y = y0 - 1; y >= y1; y--) {
        int32_t p = (int32_t)(n->sy - y) * 65536 / dy;  /* de 0 en la cercana a 1 en la lejana */
        int x = n->sx + (int)((int64_t)(f->sx - n->sx) * p >> 16);
        int w = (int)((n->sw + (int64_t)(f->sw - n->sw) * p / 65536) >> 4);
        int r = w / 6 + 1;
        span(y, 0, W, t->ground[band]);
        span(y, x - w - r, x + w + r, t->rumble[band]);
        span(y, x - w, x + w, t->road[band]);
        if (!band) {                    /* las líneas de los carriles, en las bandas claras */
            int l = w / 28 + 1, c = w / 3;
            span(y, x - c - l / 2, x - c + (l + 1) / 2, t->lane);
            span(y, x + c - l / 2, x + c + (l + 1) / 2, t->lane);
        }
    }
}

/* El paisaje: el cielo en bandas y dos capas de montañas que se mueven con
 * las curvas, hasta la línea `bottom` (lo de debajo es carretera). */
static void layer(const zxv_bitmap *b, int x, int y)
{
    if (!b->w)
        return;                         /* no está en route128.gfx */
    x %= b->w;
    if (x < 0)
        x += b->w;
    for (int dx = 0; dx < W; dx += b->w - x, x = 0)  /* repetida hasta llenar la línea */
        zxv_gfx_blit(b, x, 0, b->w - x, b->h, dx, y, ZXV_BLIT_TRANSPARENT);
}

static void draw_background(int bottom, int theme, int bg_x)
{
    const theme_t *t = &theme_col[theme];
    int y = 0;
    for (int i = 0; i < 6 && y < bottom; i++) {
        int y1 = y + t->sky_rows[i];
        rows(y, y1 < bottom ? y1 : bottom, t->sky[i]);
        y = y1;
    }
    if (y < bottom)
        rows(y, bottom, t->far);        /* entre el horizonte y la carretera */
    if (bottom > 0) {
        zxv_gfx_clip(0, 0, W, bottom);
        layer(&bg_clouds, bg_x / 8, 18);
        layer(&bg_far, bg_x / 4, HORIZON - bg_far.h);
        layer(&bg_near, bg_x / 2, HORIZON - bg_near.h);
        zxv_gfx_clip(0, 0, W, H);
    }
}

/* Un objeto o un coche: centrado en x y apoyado en y, de ancho world_w en el
 * mundo cuando la media carretera mide sw (x 16) en la pantalla. */
static void draw_thing(const sprite_t *s, int x, int y, int32_t sw, int world_w, int clip, int flags,
                       const uint8_t *colours)
{
    int w = (int)((int64_t)sw * world_w / (ROAD_HW * 16));
    if (w < 3 || !s->spr)
        return;
    int h = w * s->h / s->w;
    if (h < 2)
        return;
    x -= w / 2;
    y -= h;
    if (x >= W || x + w <= 0 || y >= clip || y + h <= 0)
        return;
    if (clip < H)
        zxv_gfx_clip(0, 0, W, clip);
    draw_scaled(s, x, y, w, h, flags, colours);
    if (clip < H)
        zxv_gfx_clip(0, 0, W, H);
}

static const int16_t car_world_w[CAR_TYPES] = { 700, 760, 820 };

void road_draw(int32_t z, int px, int32_t cam_y, int theme, int bg_x, void (*player)(void))
{
    int32_t player_h = cam_y - CAM_H;
    int base = (int)(z / SEG_LEN), frac = (int)(z % SEG_LEN);
    int32_t cam_x = (int32_t)px * ROAD_HW / X1;
    int32_t x = 0, dx = -(int32_t)curves[base % track_segs] * frac / SEG_LEN;
    for (int k = 0; k <= DRAW_SEGS; k++) {
        int seg = (base + k) % track_segs;
        int32_t cz = (int32_t)k * SEG_LEN - frac;
        if (cz < 1)
            cz = 1;
        pts[k].sx = (int16_t)clamp16(W / 2 + ((x >> 4) - cam_x) * DEPTH / cz);
        int32_t h = cz < PLAYER_Z ? player_h : heights[seg];    /* hasta el coche, plana */
        pts[k].sy = (int16_t)clamp16(HORIZON - (h - cam_y) * DEPTH / cz);
        pts[k].sw = (int32_t)ROAD_HW * DEPTH * 16 / cz;
        x += dx;
        dx += curves[seg];
    }

    /* La carretera, de cerca a lejos. */
    int max_y = H;
    for (int k = 1; k < DRAW_SEGS; k++) {
        clip_y[k] = (int16_t)max_y;
        const point_t *n = &pts[k], *f = &pts[k + 1];
        if (max_y <= 0 || f->sy >= n->sy || f->sy >= max_y)
            continue;                   /* de espaldas, o tapado por una cresta */
        draw_segment(n, f, max_y, ((base + k) / RUMBLE) & 1, &theme_col[themes[(base + k) % track_segs]]);
        max_y = f->sy;
    }
    draw_background(max_y, theme, bg_x);

    /* Los coches que se ven, ordenados de lejos a cerca. */
    static uint8_t vis[CARS], vis_k[CARS];
    int nv = 0;
    for (int i = 0; i < CARS; i++) {
        int k = (int)(cars[i].z / SEG_LEN) - base;
        if (k < 0)
            k += track_segs;
        if (k < 1 || k >= DRAW_SEGS - 1)
            continue;
        int j = nv++;
        while (j > 0 && vis_k[j - 1] < k) {
            vis[j] = vis[j - 1];
            vis_k[j] = vis_k[j - 1];
            j--;
        }
        vis[j] = (uint8_t)i;
        vis_k[j] = (uint8_t)k;
    }

    /* Los objetos y los coches, de lejos a cerca, y el del jugador en su sitio. */
    int cv = 0, drawn = 0;
    for (int k = DRAW_SEGS - 2; k >= 1; k--) {
        const point_t *n = &pts[k];
        int clip = clip_y[k];
        int seg = (base + k) % track_segs;
        if (!drawn && (int32_t)k * SEG_LEN - frac < PLAYER_Z) {
            player();
            drawn = 1;
        }
        for (int i = first_obj[seg]; clip > 0 && i < first_obj[seg + 1]; i++) {
            const object_art_t *a = &obj_art[objs[i].type];
            int sx = n->sx + (int)((int64_t)objs[i].x * n->sw / (X1 * 16));
            draw_thing(&a->s, sx, n->sy, n->sw, a->world_w, clip, objs[i].x > 0 && a->mirror ? ZXV_FLIP_H : 0, 0);
        }
        while (cv < nv && vis_k[cv] == k) {
            const car_t *c = &cars[vis[cv++]];
            if (clip <= 0)
                continue;
            const point_t *f = &pts[k + 1];
            int p = (int)(c->z % SEG_LEN) * 256 / SEG_LEN;
            int sx = n->sx + (f->sx - n->sx) * p / 256;
            int sy = n->sy + (f->sy - n->sy) * p / 256;
            int32_t sw = n->sw + (f->sw - n->sw) * p / 256;
            sx += (int)((int64_t)c->x * sw / (X1 * 16));
            draw_thing(&car_art[c->type], sx, sy, sw, car_world_w[c->type], clip, 0, car_colours[c->colour]);
        }
    }
    if (!drawn)
        player();
}
