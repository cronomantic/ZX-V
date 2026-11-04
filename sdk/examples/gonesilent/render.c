/*
 * render.c - La vista en 3D de Gone Silent: piezas convexas de caras planas.
 *
 * Cada pieza es un poliedro convexo (una caja, una pirámide, una cuña, un
 * prisma o un octaedro), así que de ella solo se ven las caras que miran
 * hacia la cámara y no hace falta ordenarlas entre sí. Las piezas sí se
 * ordenan: las cajas que las envuelven, si no se cortan, siempre tienen un
 * eje en el que una queda a un lado y la otra al otro, y la del lado de la
 * cámara es la de delante. Con eso, de cada pareja que se pisa en la
 * pantalla se sabe cuál va antes, y se dibujan de atrás adelante (el
 * algoritmo del pintor, con un orden topológico).
 *
 * Las caras se recortan contra los cinco planos de la pirámide de la vista
 * (el de cerca y los cuatro lados) en 3D, y se rellenan por filas con
 * palabras de 32 bits (8 píxeles de 4 bits). El color de cada fila es una de
 * las cuatro filas de una trama de 4 x 4 con dos colores: con ella se hacen
 * los tonos intermedios y los dibujos de algunos materiales.
 *
 * Las cuentas van en enteros; el mundo cabe en unos ±2 km alrededor de la
 * cámara (más lejos, las proyecciones se salen de 32 bits).
 */
#include "gonesilent.h"

object_t objects[OBJECTS_MAX];
int object_count;
struct render_stats stats;

#define NEAR        10                  /* el plano de cerca, a 10 cm */
#define FAR         250000              /* más lejos no se dibuja nada */

int32_t view_far = FAR;
int view_haze;

/* --- Las formas ---------------------------------------------------------------- */

typedef struct { uint8_t n, v[4]; } face_t;
typedef struct {
    uint8_t nv, nf;
    int8_t v[VERTS_MAX][3];
    face_t f[FACES_MAX];
} shape_t;

/* El orden de las caras es el de mat[] en las piezas. */
static const shape_t shapes[SHAPES] = {
    /* abajo, arriba, sur, norte, oeste, este */
    [SH_BOX] = { 8, 6, { { 0, 0, 0 }, { 4, 0, 0 }, { 4, 0, 4 }, { 0, 0, 4 },
                         { 0, 4, 0 }, { 4, 4, 0 }, { 4, 4, 4 }, { 0, 4, 4 } },
                 { { 4, { 0, 1, 2, 3 } }, { 4, { 4, 5, 6, 7 } }, { 4, { 0, 1, 5, 4 } },
                   { 4, { 3, 2, 6, 7 } }, { 4, { 0, 3, 7, 4 } }, { 4, { 1, 2, 6, 5 } } } },
    /* abajo, sur, este, norte, oeste */
    [SH_PYRAMID] = { 5, 5, { { 0, 0, 0 }, { 4, 0, 0 }, { 4, 0, 4 }, { 0, 0, 4 }, { 2, 4, 2 } },
                     { { 4, { 0, 1, 2, 3 } }, { 3, { 0, 1, 4 } }, { 3, { 1, 2, 4 } },
                       { 3, { 2, 3, 4 } }, { 3, { 3, 0, 4 } } } },
    /* sube hacia el norte: abajo, la rampa, norte, oeste, este */
    [SH_WEDGE] = { 6, 5, { { 0, 0, 0 }, { 4, 0, 0 }, { 4, 0, 4 }, { 0, 0, 4 }, { 4, 4, 4 }, { 0, 4, 4 } },
                   { { 4, { 0, 1, 2, 3 } }, { 4, { 0, 1, 4, 5 } }, { 4, { 3, 2, 4, 5 } },
                     { 3, { 0, 3, 5 } }, { 3, { 1, 2, 4 } } } },
    /* un tejado a dos aguas, con la cumbrera de sur a norte: abajo, sur,
       norte, la vertiente oeste, la este */
    [SH_PRISM] = { 6, 5, { { 0, 0, 0 }, { 4, 0, 0 }, { 4, 0, 4 }, { 0, 0, 4 }, { 2, 4, 0 }, { 2, 4, 4 } },
                   { { 4, { 0, 1, 2, 3 } }, { 3, { 0, 1, 4 } }, { 3, { 3, 2, 5 } },
                     { 4, { 0, 3, 5, 4 } }, { 4, { 1, 2, 5, 4 } } } },
    /* las cuatro de arriba y las cuatro de abajo */
    [SH_OCTA] = { 6, 8, { { 2, 0, 2 }, { 2, 4, 2 }, { 0, 2, 2 }, { 4, 2, 2 }, { 2, 2, 0 }, { 2, 2, 4 } },
                  { { 3, { 1, 3, 4 } }, { 3, { 1, 4, 2 } }, { 3, { 1, 2, 5 } }, { 3, { 1, 5, 3 } },
                    { 3, { 0, 4, 3 } }, { 3, { 0, 2, 4 } }, { 3, { 0, 5, 2 } }, { 3, { 0, 3, 5 } } } },
};

/* --- Los materiales ------------------------------------------------------------- */

/* La paleta de la vista es la misma en todas las zonas, con otros colores:
 * 0 negro, 1-4 roca (de oscuro a claro), 5-8 metal, 9-10 naranja, 11 cian, 12 el
 * verde que brilla (los ojos de las criaturas; el cian, sombreado, va hacia él),
 * 13 el cielo o el techo, 14 la bruma, 15 la luz. */
typedef struct {
    uint8_t base, n;                    /* la rampa: base, base + 1... */
    uint16_t pat;                       /* el dibujo (PAT_SOLID: ninguno) */
    uint8_t alt;                        /* su otro color; 0xFF: el tono de debajo */
} material_t;

/* Todos lisos, sombreados con la luz, menos las rayas de los avisos. */
static const material_t materials[MATS] = {
    [MAT_ROCK]   = { 1, 4, PAT_SOLID, 0 },
    [MAT_SAND]   = { 2, 3, PAT_SOLID, 0 },
    [MAT_METAL]  = { 5, 4, PAT_SOLID, 0 },
    [MAT_PANEL]  = { 5, 4, PAT_SOLID, 0 },
    [MAT_GRATE]  = { 5, 3, PAT_SOLID, 0 },     /* más oscura que el metal */
    [MAT_ORANGE] = { 9, 2, PAT_SOLID, 0 },
    [MAT_HAZARD] = { 9, 2, PAT_DIAG, 0 },
    [MAT_CYAN]   = { 11, 2, PAT_SOLID, 0 },
    [MAT_DOOR]   = { 11, 2, PAT_SOLID, 0 },
    [MAT_GLOW]   = { 15, 1, PAT_SOLID, 0 },
    [MAT_DARK]   = { 0, 2, PAT_SOLID, 0 },
    [MAT_EYE]    = { 12, 1, PAT_SOLID, 0 },    /* lo que brilla en verde de las criaturas */
    [MAT_ALARM]  = { 10, 1, PAT_SOLID, 0 },    /* las luces naranjas: balizas y avisos */
};

/* La luz, en doceavos de la rampa de cada material: 7, más 3 por lo que
 * mira al oeste (-x), 1 por lo que mira al sur (-z) y 5 por lo que mira
 * arriba. Así las paredes quedan este 4, norte 6, sur 8 y oeste 10, y arriba
 * 12: cuatro tonos distintos, dos pasos al menos entre las que hacen
 * esquina, y ninguna lisa con el tono más oscuro, que es el del suelo. */
static int light_level(const int16_t *n, int levels)
{
    int32_t v = 7 * Q14 - 3 * n[0] - n[2] + 5 * n[1];
    int l = (int)((v * (levels - 1) + 6 * Q14) / (12 * Q14));
    return l < 0 ? 0 : l > levels - 1 ? levels - 1 : l;
}

static void make_pat(uint32_t out[4], int c0, int c1, uint16_t pat)
{
    for (int r = 0; r < 4; r++) {
        uint32_t w = 0;
        for (int x = 0; x < 8; x++)
            w |= (uint32_t)((pat >> (r * 4 + (x & 3))) & 1 ? c1 : c0) << (x * 4);
        out[r] = w;
    }
}

/* La trama de una cara con la normal n: la luz da el tono. Los lisos tienen
 * cuatro pasos de cada tono al siguiente (el tono solo y tramas del 25, el 50
 * y el 75 %); los de dibujo, dos (el tono o el siguiente). */
static void face_colour(uint32_t out[4], int mat, const int16_t *n)
{
    static const uint16_t mix[4] = { PAT_SOLID, PAT_25, PAT_50, PAT_75 };
    const material_t *m = &materials[mat];
    if (m->pat == PAT_SOLID) {
        int l = light_level(n, 4 * (m->n - 1) + 1);
        int tone = m->base + l / 4;
        make_pat(out, tone, tone + 1, mix[l % 4]);
    } else {
        int l = light_level(n, 2 * m->n - 1);
        int tone = m->base + (l >> 1);
        tone += l & 1;
        int alt = m->alt != 0xFF ? m->alt : tone > m->base ? tone - 1 : tone;
        make_pat(out, tone, alt, m->pat);
    }
}

static uint32_t isqrt64(uint64_t v)
{
    uint64_t r = 0, bit = (uint64_t)1 << 62;
    while (bit > v)
        bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else
            r >>= 1;
        bit >>= 2;
    }
    return (uint32_t)r;
}

void object_setup(object_t *o)
{
    const shape_t *s = &shapes[o->p.shape];
    int32_t hx = o->p.sx / 2, hy = o->p.sy / 2, hz = o->p.sz / 2;
    int32_t c = icos(o->yaw), sn = isin(o->yaw);
    for (int i = 0; i < s->nv; i++) {
        int32_t x = s->v[i][0] * o->p.sx / 4, y = s->v[i][1] * o->p.sy / 4, z = s->v[i][2] * o->p.sz / 4;
        if ((o->p.flags & (OBJ_MOVES | OBJ_ROLL)) == (OBJ_MOVES | OBJ_ROLL)) {
            int32_t dx = x - hx, dy = y - hy;
            x = hx + (dx * c + dy * sn) / Q14;
            y = hy + (dy * c - dx * sn) / Q14;
        } else if (o->p.flags & OBJ_MOVES) {
            int32_t dx = x - hx, dz = z - hz;
            x = hx + (dx * c + dz * sn) / Q14;
            z = hz + (dz * c - dx * sn) / Q14;
        }
        o->v[i][0] = o->p.x + x;
        o->v[i][1] = o->p.y + y;
        o->v[i][2] = o->p.z + z;
    }
    int32_t cx = o->p.x + hx, cy = o->p.y + hy, cz = o->p.z + hz;
    for (int f = 0; f < s->nf; f++) {
        const face_t *fc = &s->f[f];
        const int32_t *a = o->v[fc->v[0]], *b = o->v[fc->v[1]], *d = o->v[fc->v[2]];
        int64_t e1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
        int64_t e2[3] = { d[0] - a[0], d[1] - a[1], d[2] - a[2] };
        int64_t n[3] = { e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2],
                         e1[0] * e2[1] - e1[1] * e2[0] };
        while (n[0] > (1 << 20) || n[0] < -(1 << 20) || n[1] > (1 << 20) || n[1] < -(1 << 20) ||
               n[2] > (1 << 20) || n[2] < -(1 << 20))
            for (int k = 0; k < 3; k++)
                n[k] /= 2;
        int64_t len = isqrt64((uint64_t)(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]));
        if (len == 0)
            len = 1;
        /* Hacia fuera: hacia el lado contrario del centro de la pieza. */
        int64_t mx = 0, my = 0, mz = 0;
        for (int k = 0; k < fc->n; k++) {
            mx += o->v[fc->v[k]][0];
            my += o->v[fc->v[k]][1];
            mz += o->v[fc->v[k]][2];
        }
        int64_t side = n[0] * (mx / fc->n - cx) + n[1] * (my / fc->n - cy) + n[2] * (mz / fc->n - cz);
        for (int k = 0; k < 3; k++)
            o->n[f][k] = (int16_t)((side < 0 ? -n[k] : n[k]) * Q14 / len);
        face_colour(o->pat[f], o->p.mat[f], o->n[f]);
    }
}

/* --- El relleno ---------------------------------------------------------------- */

static uint8_t *page;
static int row_words;                   /* palabras de 32 bits por fila */
static int32_t xl[384], xr[384];        /* los bordes de cada fila, 16.16 */

static void span(uint32_t *row, int a, int b, uint32_t pat)
{
    if (a >= b)
        return;
    int i = a >> 3, j = b >> 3;
    uint32_t ma = 0xFFFFFFFFu << ((a & 7) * 4);
    uint32_t mb = (b & 7) ? 0xFFFFFFFFu >> (32 - (b & 7) * 4) : 0;
    if (i == j) {
        uint32_t m = ma & mb;
        row[i] = (row[i] & ~m) | (pat & m);
        return;
    }
    if (a & 7) {
        row[i] = (row[i] & ~ma) | (pat & ma);
        i++;
    }
    while (i < j)
        row[i++] = pat;
    if (mb)
        row[j] = (row[j] & ~mb) | (pat & mb);
}

/* Un borde, de (x0, y0) a (x1, y1) en 1/16 de píxel: la x de cada fila que
 * cruza (las de centro y >= y0 y < y1, con la y de la fila en su borde de
 * arriba, como las de los píxeles). */
static void edge(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t *tab)
{
    if (y0 == y1)
        return;
    if (y0 > y1) {
        int32_t t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }
    int ys = (y0 + 15) >> 4, ye = (y1 + 15) >> 4;
    if (ys < 0)
        ys = 0;
    if (ye > view_h)
        ye = view_h;
    if (ys >= ye)
        return;
    int32_t slope = (x1 - x0) * 65536 / (y1 - y0);
    int32_t x = x0 * 4096 + (int32_t)(((int64_t)slope * (ys * 16 - y0)) >> 4);
    for (int y = ys; y < ye; y++) {
        tab[y] = x;
        x += slope;
    }
}

/* Un polígono convexo, de n vértices en 1/16 de píxel, en cualquier sentido. */
static void fill_poly(const int32_t *px, const int32_t *py, int n, const uint32_t pat[4])
{
    int64_t area = 0;
    int32_t ymin = py[0], ymax = py[0];
    for (int i = 0; i < n; i++) {
        int j = i + 1 == n ? 0 : i + 1;
        area += (int64_t)px[i] * py[j] - (int64_t)px[j] * py[i];
        if (py[i] < ymin) ymin = py[i];
        if (py[i] > ymax) ymax = py[i];
    }
    if (area == 0)
        return;
    /* Con la y hacia abajo y el área positiva (el sentido de las agujas del
     * reloj en la pantalla), los bordes que bajan son los de la derecha. */
    for (int i = 0; i < n; i++) {
        int j = i + 1 == n ? 0 : i + 1;
        int down = py[j] > py[i];
        edge(px[i], py[i], px[j], py[j], (down == (area > 0)) ? xr : xl);
    }
    int ys = (ymin + 15) >> 4, ye = (ymax + 15) >> 4;
    if (ys < 0)
        ys = 0;
    if (ye > view_h)
        ye = view_h;
    for (int y = ys; y < ye; y++) {
        int a = (xl[y] + 0xFFFF) >> 16, b = (xr[y] + 0xFFFF) >> 16;
        if (a < 0) a = 0;
        if (b > scr_w) b = scr_w;
        span((uint32_t *)(page + y * row_words * 4), a, b, pat[y & 3]);
    }
}

/* --- La bruma ------------------------------------------------------------------ */

/* Si view_far es menor que FAR, las piezas y el suelo se mezclan con el color
 * de la bruma (el 14) en una trama, más cuanto más lejos estén (en cuartos de
 * view_far); el cielo, con view_haze. */
#define HAZE        0xEEEEEEEEu         /* ocho píxeles del color 14 */
static const uint16_t haze_pat[4] = { PAT_SOLID, PAT_25, PAT_50, PAT_75 };

static int haze_level(int64_t d2)
{
    int64_t q = view_far / 4;
    return d2 >= 9 * q * q ? 3 : d2 >= 4 * q * q ? 2 : d2 >= q * q ? 1 : 0;
}

/* --- El fondo ------------------------------------------------------------------ */

static int focal, half_w, centre_y;     /* la proyección de este frame */

/* Las filas por encima del horizonte con las franjas up y por debajo con las
 * down, contando desde el horizonte (la línea centre_y). */
void render_background(const camera_t *cam, const band_t *up, const band_t *down)
{
    page = video_page();
    row_words = scr_w / 8;
    focal = scr_w * 55 / 100;
    half_w = scr_w / 2;
    centre_y = view_h / 2 + cam->look * (hires + 1);
    uint32_t mask[4][4];
    for (int k = 0; k < 4; k++)
        make_pat(mask[k], 0, 15, haze_pat[k]);
    for (int y = 0; y < view_h; y++) {
        const band_t *b;
        int d = y < centre_y ? centre_y - 1 - y : y - centre_y;
        int dp = d;
        d >>= hires;
        for (b = y < centre_y ? up : down; b->dist != 255 && d > b->dist; b++)
            ;
        uint32_t w[4];
        make_pat(w, b->c0, b->c1, b->pat);
        uint32_t *row = (uint32_t *)(page + y * row_words * 4), v = w[y & 3];
        if (view_far < FAR) {
            /* El suelo de esta fila está a la altura de los ojos por el foco
             * entre las filas hasta el horizonte. */
            int64_t g = (int64_t)cam->y * focal / (dp + 1);
            int k = y < centre_y ? view_haze : haze_level(g * g);
            v = (v & ~mask[k][y & 3]) | (HAZE & mask[k][y & 3]);
        }
        for (int i = 0; i < row_words; i++)
            row[i] = v;
    }
}

int project(const camera_t *cam, int32_t x, int32_t y, int32_t z, int *sx, int *sy)
{
    int32_t cs = icos(cam->yaw), sn = isin(cam->yaw);
    int32_t dx = x - cam->x, dz = z - cam->z;
    int32_t cx = (int32_t)(((int64_t)dx * cs - (int64_t)dz * sn) >> 14);
    int32_t cz = (int32_t)(((int64_t)dx * sn + (int64_t)dz * cs) >> 14);
    if (cz < NEAR)
        return 0;
    *sx = half_w + (int32_t)((int64_t)cx * focal / cz);
    *sy = centre_y - (int32_t)((int64_t)(y - cam->y) * focal / cz);
    return 1;
}

/* --- Las piezas ---------------------------------------------------------------- */

enum { OUT_NEAR = 1, OUT_LEFT = 2, OUT_RIGHT = 4, OUT_TOP = 8, OUT_BOTTOM = 16, OUT_FAR = 32 };

static int32_t plane(int k, const int32_t *p)
{
    switch (k) {
    case 0: return p[2] - NEAR;
    case 1: return p[0] * focal + half_w * p[2];
    case 2: return half_w * p[2] - p[0] * focal;
    case 3: return centre_y * p[2] - p[1] * focal;
    default: return p[1] * focal + (view_h - centre_y) * p[2];
    }
}

static int outcode(const int32_t *p)
{
    int c = 0;
    for (int k = 0; k < 5; k++)
        if (plane(k, p) < 0)
            c |= 1 << k;
    if (p[2] > view_far)
        c |= OUT_FAR;
    return c;
}

/* Sutherland-Hodgman contra el plano k. */
static int clip(int32_t (*in)[3], int n, int32_t (*out)[3], int k)
{
    int m = 0;
    for (int i = 0; i < n; i++) {
        const int32_t *a = in[i], *b = in[i + 1 == n ? 0 : i + 1];
        int32_t da = plane(k, a), db = plane(k, b);
        if (da >= 0) {
            out[m][0] = a[0]; out[m][1] = a[1]; out[m][2] = a[2];
            m++;
        }
        if ((da >= 0) != (db >= 0)) {
            for (int j = 0; j < 3; j++)
                out[m][j] = a[j] + (int32_t)((int64_t)(b[j] - a[j]) * da / (da - db));
            m++;
        }
    }
    return m;
}

/* Las caras de una pieza, con sus vértices en la cámara (cv) y por fuera de
 * qué planos queda cada uno (code); fog, cuánta bruma (de 0 a 3). */
static void draw_object(const object_t *o, int32_t (*cv)[3], const uint8_t *code, int fog, const camera_t *cam)
{
    const shape_t *s = &shapes[o->p.shape];
    uint32_t mask[4], fogged[4];
    if (fog)
        make_pat(mask, 0, 15, haze_pat[fog]);
    for (int f = 0; f < s->nf; f++) {
        const face_t *fc = &s->f[f];
        const int32_t *v0 = o->v[fc->v[0]];
        int64_t facing = (int64_t)o->n[f][0] * (cam->x - v0[0]) + (int64_t)o->n[f][1] * (cam->y - v0[1]) +
                         (int64_t)o->n[f][2] * (cam->z - v0[2]);
        if (facing <= 0)
            continue;
        int or = 0, and = 0x1F;
        for (int k = 0; k < fc->n; k++) {
            or |= code[fc->v[k]];
            and &= code[fc->v[k]];
        }
        if (and & 0x1F)
            continue;
        static int32_t buf[2][16][3];
        int n = fc->n, cur = 0;
        for (int k = 0; k < n; k++) {
            buf[0][k][0] = cv[fc->v[k]][0];
            buf[0][k][1] = cv[fc->v[k]][1];
            buf[0][k][2] = cv[fc->v[k]][2];
        }
        if (or & 0x1F) {
            stats.clipped++;
            for (int k = 0; k < 5 && n >= 3; k++)
                if (or & (1 << k)) {
                    n = clip(buf[cur], n, buf[cur ^ 1], k);
                    cur ^= 1;
                }
            if (n < 3)
                continue;
        }
        int32_t px[16], py[16];
        for (int k = 0; k < n; k++) {
            int32_t z = buf[cur][k][2];
            px[k] = half_w * 16 + buf[cur][k][0] * focal * 16 / z;
            py[k] = centre_y * 16 - buf[cur][k][1] * focal * 16 / z;
        }
        const uint32_t *pat = o->pat[f];
        if (fog) {
            for (int r = 0; r < 4; r++)
                fogged[r] = (pat[r] & ~mask[r]) | (HAZE & mask[r]);
            pat = fogged;
        }
        fill_poly(px, py, n, pat);
        stats.faces++;
    }
}

/* --- Los sprites ---------------------------------------------------------------- */

/* El sprite de o (ver SPRITE_CM), escalado con la distancia y recortado a la
 * vista: en espejo, todo blanco (spr_flash) o, con la bruma, mezclado con su
 * color (el 14) en una trama, como las caras. Está comprimido (mkspr.py): de
 * cada fila, sus tramos sin transparente. */
#define SPRITE_W_MAX 88
static void draw_sprite(const object_t *o, int fog, const camera_t *cam)
{
    const uint8_t *s = o->spr;
    int w = s[0], h = s[1];
    int32_t cs = icos(cam->yaw), sn = isin(cam->yaw);
    int32_t dx = o->p.x + o->p.sx / 2 - cam->x, dz = o->p.z + o->p.sz / 2 - cam->z;
    int32_t x = (int32_t)(((int64_t)dx * cs - (int64_t)dz * sn) >> 14);
    int32_t z = (int32_t)(((int64_t)dx * sn + (int64_t)dz * cs) >> 14);
    if (z < 40)
        return;
    int cm = o->spr_cm ? o->spr_cm : SPRITE_CM;
    int32_t dw = w * cm * focal / z, dh = h * cm * focal / z;
    if (dw < 1 || dh < 1)
        return;
    int32_t x0 = half_w + x * focal / z - dw / 2;
    int32_t y0 = centre_y - (o->p.y - cam->y) * focal / z - dh;
    int ax = x0 < 0 ? 0 : (int)x0, bx = x0 + dw > scr_w ? scr_w : (int)(x0 + dw);
    int ay = y0 < 0 ? 0 : (int)y0, by = y0 + dh > view_h ? view_h : (int)(y0 + dh);
    if (ax >= bx || ay >= by || w > SPRITE_W_MAX)
        return;
    /* Los bordes de las columnas, una vez; de cada fila del sprite, sus
     * tramos de un color, ya recortados, y con ellos todas las filas de la
     * pantalla que la repiten. La bruma, como una máscara en cada fila. */
    static int16_t edge[SPRITE_W_MAX + 1], ta[SPRITE_W_MAX], tb[SPRITE_W_MAX];
    static uint32_t tc[SPRITE_W_MAX];
    for (int j = 0, e = 0; j <= w; j++, e += dw) {
        int32_t a = x0 + e / w;
        edge[j] = (int16_t)(a < ax ? ax : a > bx ? bx : a);
    }
    uint32_t haze[4];
    make_pat(haze, 0, 15, o->spr_flash ? PAT_SOLID : haze_pat[fog]);
    uint32_t stepy = ((uint32_t)h << 16) / (uint32_t)dh;
    int last = -1, n = 0;
    for (int y = ay; y < by; y++) {
        int r = (int)((uint32_t)(y - y0) * stepy >> 16);
        if (r != last) {
            last = r;
            n = 0;
            const uint8_t *p = s + (s[2 + 2 * r] | s[3 + 2 * r] << 8);
            for (int g = *p++; g > 0; g--) {
                int x = p[0], len = p[1];
                p += 2;
                for (int k = 0; k < len; k++) {
                    int j = o->spr_flip ? w - 1 - (x + k) : x + k;
                    int a = edge[j], b = edge[j + 1];
                    if (a == b)
                        continue;
                    uint32_t c = (uint32_t)(o->spr_flash ? 15 : (p[k >> 1] >> ((k & 1) * 4)) & 15) * 0x11111111u;
                    if (n && tc[n - 1] == c && tb[n - 1] == a)
                        tb[n - 1] = (int16_t)b;
                    else if (n && tc[n - 1] == c && ta[n - 1] == b)
                        ta[n - 1] = (int16_t)a;         /* en espejo, de derecha a izquierda */
                    else {
                        ta[n] = (int16_t)a;
                        tb[n] = (int16_t)b;
                        tc[n++] = c;
                    }
                }
                p += (len + 1) >> 1;
            }
        }
        uint32_t *dst = (uint32_t *)(page + y * row_words * 4), m = haze[y & 3];
        for (int k = 0; k < n; k++)
            span(dst, ta[k], tb[k], (tc[k] & ~m) | (HAZE & m));
    }
}

/* --- El orden ------------------------------------------------------------------ */

typedef struct {
    const object_t *o;
    int16_t x0, y0, x1, y1;             /* lo que ocupa en la pantalla */
    int64_t dist;                       /* al cuadrado, hasta el centro */
    uint8_t fog;                        /* la bruma, de 0 a 3 */
    int32_t cv[VERTS_MAX][3];
    uint8_t code[VERTS_MAX];
} vis_t;

static vis_t vis[OBJECTS_MAX];
static uint32_t before[OBJECTS_MAX][(OBJECTS_MAX + 31) / 32];  /* before[i]: las que van después de i */
static uint8_t indeg[OBJECTS_MAX], done[OBJECTS_MAX];

/* Si a tapa a b, 1; si b tapa a a, -1; si no se pisan, 0; si las cajas se
 * cortan y no se sabe, 2. */
static int in_front(const object_t *a, const object_t *b, const camera_t *cam)
{
    const int32_t c[3] = { cam->x, cam->y, cam->z };
    const int32_t amin[3] = { a->p.x, a->p.y, a->p.z }, bmin[3] = { b->p.x, b->p.y, b->p.z };
    const int32_t asz[3] = { a->p.sx, a->p.sy, a->p.sz }, bsz[3] = { b->p.sx, b->p.sy, b->p.sz };
    for (int k = 0; k < 3; k++) {
        int32_t amax = amin[k] + asz[k], bmax = bmin[k] + bsz[k];
        if (amax <= bmin[k])
            return c[k] >= bmin[k] ? -1 : c[k] <= amax ? 1 : 0;
        if (bmax <= amin[k])
            return c[k] >= amin[k] ? 1 : c[k] <= bmax ? -1 : 0;
    }
    return 2;
}

void render_view(const camera_t *cam)
{
    int32_t cs = icos(cam->yaw), sn = isin(cam->yaw);
    int nv = 0;
    stats.objects = stats.faces = stats.clipped = 0;

    /* Lo que se ve: los vértices en la cámara y lo que ocupan en la pantalla. */
    for (int i = 0; i < object_count; i++) {
        const object_t *o = &objects[i];
        if ((o->p.flags & OBJ_HIDDEN) || ((o->p.flags & OBJ_SPRITE) && !o->spr))
            continue;
        const shape_t *s = &shapes[o->p.shape];
        vis_t *v = &vis[nv];
        int and = 0x3F, or = 0;
        int32_t x0 = scr_w, y0 = view_h, x1 = 0, y1 = 0;
        for (int k = 0; k < s->nv; k++) {
            int32_t dx = o->v[k][0] - cam->x, dy = o->v[k][1] - cam->y, dz = o->v[k][2] - cam->z;
            int32_t *p = v->cv[k];
            p[0] = (int32_t)(((int64_t)dx * cs - (int64_t)dz * sn) >> 14);
            p[1] = dy;
            p[2] = (int32_t)(((int64_t)dx * sn + (int64_t)dz * cs) >> 14);
            int c = v->code[k] = (uint8_t)outcode(p);
            and &= c;
            or |= c;
            if (c & OUT_NEAR)
                continue;
            int32_t sx = c & OUT_LEFT ? 0 : c & OUT_RIGHT ? scr_w : half_w + p[0] * focal / p[2];
            int32_t sy = c & OUT_TOP ? 0 : c & OUT_BOTTOM ? view_h : centre_y - p[1] * focal / p[2];
            if (sx < x0) x0 = sx;
            if (sx > x1) x1 = sx;
            if (sy < y0) y0 = sy;
            if (sy > y1) y1 = sy;
        }
        if (and)
            continue;                   /* fuera de la vista o demasiado lejos */
        if (or & OUT_NEAR) {            /* por detrás de la cámara: puede ocupar cualquier sitio */
            x0 = 0; y0 = 0; x1 = scr_w; y1 = view_h;
        }
        v->o = o;
        v->x0 = (int16_t)x0; v->y0 = (int16_t)y0; v->x1 = (int16_t)x1; v->y1 = (int16_t)y1;
        int64_t cx = o->p.x + o->p.sx / 2 - cam->x, cy = o->p.y + o->p.sy / 2 - cam->y,
                cz = o->p.z + o->p.sz / 2 - cam->z;
        v->dist = cx * cx + cy * cy + cz * cz;
        /* La bruma va por lo más cerca de la caja. */
        v->fog = 0;
        if (view_far < FAR) {
            const int32_t c[3] = { cam->x, cam->y, cam->z };
            const int32_t lo[3] = { o->p.x, o->p.y, o->p.z }, sz[3] = { o->p.sx, o->p.sy, o->p.sz };
            int64_t d2 = 0;
            for (int k = 0; k < 3; k++) {
                int64_t e = lo[k] - c[k] > 0 ? lo[k] - c[k] : c[k] - lo[k] - sz[k] > 0 ? c[k] - lo[k] - sz[k] : 0;
                d2 += e * e;
            }
            v->fog = (uint8_t)haze_level(d2);
        }
        nv++;
    }
    stats.objects = nv;

    /* El orden: de cada pareja que se pisa, la de detrás antes. */
    for (int i = 0; i < nv; i++) {
        indeg[i] = 0;
        done[i] = 0;
        for (int w = 0; w < (OBJECTS_MAX + 31) / 32; w++)
            before[i][w] = 0;
    }
    for (int i = 0; i < nv; i++)
        for (int j = i + 1; j < nv; j++) {
            if (vis[i].x1 <= vis[j].x0 || vis[j].x1 <= vis[i].x0 ||
                vis[i].y1 <= vis[j].y0 || vis[j].y1 <= vis[i].y0)
                continue;
            int r = in_front(vis[i].o, vis[j].o, cam);
            if (r == 2)
                r = vis[i].dist < vis[j].dist ? 1 : -1;
            if (r == 1) {               /* i tapa a j: j antes */
                before[j][i >> 5] |= 1u << (i & 31);
                indeg[i]++;
            } else if (r == -1) {
                before[i][j >> 5] |= 1u << (j & 31);
                indeg[j]++;
            }
        }
    for (int left = nv; left > 0; left--) {
        /* La más lejana de las que ya no esperan a ninguna; si todas esperan
         * (un ciclo, que con cajas casi nunca pasa), la más lejana. */
        int best = -1, wait = -1;
        for (int i = 0; i < nv; i++) {
            if (done[i])
                continue;
            if (indeg[i] == 0 && (best < 0 || vis[i].dist > vis[best].dist))
                best = i;
            if (wait < 0 || vis[i].dist > vis[wait].dist)
                wait = i;
        }
        if (best < 0)
            best = wait;
        done[best] = 1;
        for (int j = 0; j < nv; j++)
            if (before[best][j >> 5] & (1u << (j & 31)))
                indeg[j]--;
        if (vis[best].o->p.flags & OBJ_SPRITE)
            draw_sprite(vis[best].o, vis[best].fog, cam);
        else
            draw_object(vis[best].o, vis[best].cv, vis[best].code, vis[best].fog, cam);
    }
}
