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
 *   - Los colores (D90): cada nivel tiene su paleta, hecha con los colores
 *     de sus dibujos a cada distancia de la lámpara (view_level). La vista
 *     usa dos tablas, la del techo y la del suelo, con los mismos colores
 *     para las paredes y los objetos y cuatro propios cada una; en cada
 *     frame se ponen con la luz que da la lámpara (view_colours).
 *   - La niebla: lo que alumbra la lámpara se ve con sus colores; más lejos,
 *     cada vez más oscuro, con colores de verdad de la paleta (mezclados de
 *     dos en dos con un tramado de Bayer), y al final negro. Las paredes y
 *     los objetos cambian de color al dibujarse (tablas de 16); el suelo y el
 *     techo, una línea entera de una vez, con una tabla de 256 por byte (sus
 *     dos píxeles), que además le pone los colores del nivel.
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

/* --- Los colores y la niebla ----------------------------------------------------- */

/* La niebla: a cada distancia, según el alcance de la lámpara, un nivel. El
 * 0 tiene los colores de los dibujos; el 1, sin brillo (también las caras de
 * un lado, de cerca); del 2 al 5 cada vez más oscuros, y más rojizos, que es
 * la luz de una llama; el 6, negro. */
enum { FOG_LEVELS = 7, FOG_BLACK = FOG_LEVELS - 1 };

static int fog_level(int32_t z, int32_t light)
{
    if (z >= light)
        return FOG_BLACK;
    int32_t r = (z << 8) / light;       /* de 0 a 256; light no llega a 128 celdas */
    return r < 110 ? 0 : r < 140 ? 1 : r < 166 ? 2 : r < 190 ? 3 : r < 214 ? 4 : 5;
}

typedef struct {
    uint8_t r, g, b;
} rgb_t;

/* Lo que queda del rojo, el verde y el azul en cada nivel oscuro, x 256. */
static const uint8_t dim[FOG_BLACK][3] = {
    { 0, 0, 0 }, { 0, 0, 0 }, { 206, 198, 186 }, { 156, 146, 132 }, { 104, 96, 84 }, { 58, 52, 44 },
};

/* El color c de un dibujo con la niebla f, de 0 a 255 por canal (la escala
 * de ZXV_RGB: 215 es el nivel normal). */
static rgb_t fog_rgb(int c, int f)
{
    rgb_t o = { 0, 0, 0 };
    if (c == ZXV_TRANSPARENT || f >= FOG_BLACK)
        return o;
    if (f > 0)
        c &= 7;
    int v = c & 8 ? 255 : 215;
    o = (rgb_t){ (uint8_t)(c & 2 ? v : 0), (uint8_t)(c & 4 ? v : 0), (uint8_t)(c & 1 ? v : 0) };
    if (f >= 2)
        o = (rgb_t){ (uint8_t)(o.r * dim[f][0] >> 8), (uint8_t)(o.g * dim[f][1] >> 8),
                     (uint8_t)(o.b * dim[f][2] >> 8) };
    return o;
}

/* Lo lejos que están dos colores, más o menos como se ven. */
static uint32_t rgb_dist(rgb_t a, rgb_t b)
{
    int dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
    return (uint32_t)(3 * dr * dr + 4 * dg * dg + 2 * db * db);
}

static int nearest(rgb_t c, const rgb_t *pal, int n, uint32_t *d)
{
    int best = 0;
    uint32_t bd = 0xFFFFFFFFu;
    for (int i = 0; i < n; i++) {
        uint32_t e = rgb_dist(c, pal[i]);
        if (e < bd) {
            bd = e;
            best = i;
        }
    }
    if (d)
        *d = bd;
    return best;
}

/* Un color, como sale por la pantalla: en RGB565 y de vuelta. */
static uint16_t rgb565(rgb_t c) { return ZXV_RGB(c.r, c.g, c.b); }

static rgb_t shown(rgb_t c)
{
    uint32_t v = rgb565(c), r = v >> 11, g = (v >> 5) & 63, b = v & 31;
    return (rgb_t){ (uint8_t)(r <= 27 ? (r * 215 + 13) / 27 : 215 + (r - 27) * 10),
                    (uint8_t)(g <= 54 ? (g * 215 + 27) / 54 : 215 + ((g - 54) * 40 + 4) / 9),
                    (uint8_t)(b <= 27 ? (b * 215 + 13) / 27 : 215 + (b - 27) * 10) };
}

/* Los colores de cada nivel (D90): los 16 de la tabla del techo (la 1) y los
 * de la del suelo (la 2). Los 12 primeros son los mismos en las dos: los de
 * las paredes y los objetos, que pasan por las dos mitades de la vista. El
 * 0 es el negro y el 1, el rojo de los ojos de las sombras, que no se apaga
 * con la lámpara. Los 4 últimos, solo para el techo o solo para el suelo. Se
 * eligen con los colores que usan sus dibujos, en todos los niveles de
 * niebla: lo que más se ve, más exacto. */
enum { EYES = 1, COMMON = 12, FIXED = 5 };  /* FIXED: los de cerca más vistos, tal cual */
static rgb_t pal_ceiling[16], pal_floor[16];
static int border_colour;               /* el del borde, de la tabla del techo */
#define FOG_BORDER  4                   /* la penumbra del borde */

typedef struct {
    rgb_t c;
    uint32_t w;                         /* cuánto se ve */
} target_t;

#define TARGETS 160
static target_t targets[TARGETS];
static int ntargets;

static void add_target(rgb_t c, uint32_t w)
{
    if (!w)
        return;
    for (int i = 0; i < ntargets; i++)
        if (targets[i].c.r == c.r && targets[i].c.g == c.g && targets[i].c.b == c.b) {
            targets[i].w += w;
            return;
        }
    if (ntargets < TARGETS)
        targets[ntargets++] = (target_t){ c, w };
}

/* Lo que pesa cada nivel de niebla: de cerca se ve más grande. */
static const uint8_t fog_weight[FOG_BLACK] = { 8, 6, 4, 3, 2, 1 };

/* Los colores de un dibujo (count) con los niveles de niebla de 0 a fogs - 1,
 * con el peso weight; map, si no es NULL, cambia cada color por otro. */
static void add_drawing(const uint16_t *count, uint32_t weight, const uint8_t *map, int fogs)
{
    uint32_t total = 0;
    for (int c = 0; c < 16; c++)
        total += count[c];
    for (int c = 0; total && c < 16; c++) {
        uint32_t w = count[c] * 1024u / total * weight;
        for (int f = 0; w && f < fogs; f++)
            add_target(fog_rgb(map ? map[c] : c, f), w * fog_weight[f]);
    }
}

/* Las paredes del laberinto (según cuántas celdas hay de cada una), la
 * puerta y los objetos. */
static void add_world(int fogs)
{
    int cells[8] = { 0 }, total = 0;
    for (int y = 0; y < maze_h; y++)
        for (int x = 0; x < maze_w; x++) {
            int c = cell_at(x, y);
            if (c >= CELL_WALL && c < CELL_GATE) {
                cells[c - CELL_WALL]++;
                total++;
            }
        }
    for (int n = 0; n < 8; n++)
        if (cells[n])
            add_drawing(art_wall_count[n], 1 + cells[n] * 32u / (uint32_t)total, 0, fogs);
    add_drawing(art_wall_count[WALL_GATE], 1, 0, fogs);
    add_drawing(art_wall_count[WALL_OPEN], 1, 0, fogs);
    add_drawing(art_sprite_count[SPR_RUNE], 2, 0, fogs);
    add_drawing(art_sprite_count[SPR_FLASK], 1, 0, fogs);
    /* Las sombras, sin los ojos (el 10), que tienen el suyo. */
    colour_count body;
    for (int c = 0; c < 16; c++)
        body[c] = c == 10 ? 0 : art_sprite_count[SPR_SHADOW][c];
    add_drawing(body, 2, 0, fogs < 4 ? fogs : 4);
}

/* Los colores pal[fixed] a pal[n - 1] para los objetivos (k-medias con
 * pesos); los de antes ya están puestos. */
static void quantize(rgb_t *pal, int fixed, int n)
{
    /* Para empezar, uno a uno, el objetivo peor representado (d: lo lejos
     * que está cada uno del más cercano que ya hay). */
    static uint32_t d[TARGETS];
    static uint8_t near_k[TARGETS];
    for (int i = 0; i < ntargets; i++)
        nearest(targets[i].c, pal, fixed, &d[i]);
    for (int k = fixed; k < n; k++) {
        uint64_t worst = 0;
        int at = -1;
        for (int i = 0; i < ntargets; i++) {
            uint64_t e = (uint64_t)d[i] * targets[i].w;
            if (e > worst) {
                worst = e;
                at = i;
            }
        }
        pal[k] = at < 0 ? pal[0] : targets[at].c;
        for (int i = 0; i < ntargets; i++) {
            uint32_t e = rgb_dist(targets[i].c, pal[k]);
            if (e < d[i])
                d[i] = e;
        }
    }
    /* Luego, cada uno en el centro de los que tiene más cerca, hasta que no
     * cambia nada. */
    for (int it = 0; it < 8; it++) {
        uint64_t sr[16] = { 0 }, sg[16] = { 0 }, sb[16] = { 0 }, sw[16] = { 0 };
        int moved = 0;
        for (int i = 0; i < ntargets; i++) {
            int k = nearest(targets[i].c, pal, n, 0);
            moved |= it == 0 || k != near_k[i];
            near_k[i] = (uint8_t)k;
            uint32_t w = targets[i].w;
            sr[k] += (uint64_t)targets[i].c.r * w;
            sg[k] += (uint64_t)targets[i].c.g * w;
            sb[k] += (uint64_t)targets[i].c.b * w;
            sw[k] += w;
        }
        if (!moved)
            break;
        for (int k = fixed; k < n; k++)
            if (sw[k])
                pal[k] = (rgb_t){ (uint8_t)((sr[k] + sw[k] / 2) / sw[k]), (uint8_t)((sg[k] + sw[k] / 2) / sw[k]),
                                  (uint8_t)((sb[k] + sw[k] / 2) / sw[k]) };
    }
}

/* Un color hecho con dos de la paleta: j en a / 4 de los píxeles, con el
 * tramado de Bayer (x e y, mod 4). */
typedef struct {
    uint8_t i, j, a;
} mix_t;

static const uint8_t bayer[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };

static int mix_at(mix_t m, int x, int y) { return bayer[y & 3][x & 3] < 4 * m.a ? m.j : m.i; }

/* La mejor manera de hacer c con los n primeros colores de pal: uno solo, o
 * dos de los cuatro más cercanos mezclados. */
static mix_t best_mix(rgb_t c, const rgb_t *pal, int n)
{
    int near[4] = { 0 }, m = 0;
    uint32_t nd[4] = { 0 };
    for (int i = 0; i < n; i++) {
        uint32_t d = rgb_dist(c, pal[i]);
        int k = m < 4 ? m++ : 4;
        for (; k > 0 && nd[k - 1] > d; k--)
            if (k < 4) {
                near[k] = near[k - 1];
                nd[k] = nd[k - 1];
            }
        if (k < 4) {
            near[k] = i;
            nd[k] = d;
        }
    }
    mix_t best = { (uint8_t)near[0], (uint8_t)near[0], 0 };
    uint32_t be = nd[0];
    for (int a = 1; a <= 2; a++)
        for (int x = 0; x < m; x++)
            for (int y = 0; y < m; y++) {
                if (x == y)
                    continue;
                rgb_t p = pal[near[x]], q = pal[near[y]];
                rgb_t mixed = { (uint8_t)((p.r * (4 - a) + q.r * a + 2) / 4), (uint8_t)((p.g * (4 - a) + q.g * a + 2) / 4),
                                (uint8_t)((p.b * (4 - a) + q.b * a + 2) / 4) };
                uint32_t e = rgb_dist(c, mixed);
                if (e < be) {
                    be = e;
                    best = (mix_t){ (uint8_t)near[x], (uint8_t)near[y], (uint8_t)a };
                }
            }
    return best;
}

/* Las mezclas de los 16 colores (cambiados por map, si no es NULL) con la
 * niebla f; cada color distinto, una vez. */
static void mixes(mix_t m[16], const uint8_t *map, int f, const rgb_t *pal, int n)
{
    rgb_t c[16];
    for (int i = 0; i < 16; i++) {
        c[i] = fog_rgb(map ? map[i] : i, f);
        int j = 0;
        while (j < i && (c[j].r != c[i].r || c[j].g != c[i].g || c[j].b != c[i].b))
            j++;
        m[i] = j < i ? m[j] : best_mix(c[i], pal, n);
    }
}

/* Las paredes: por nivel de niebla, la columna (x mod 4) y la línea (y mod
 * 4), el byte de cada color (los dos píxeles iguales). */
static uint8_t wall_lut[FOG_LEVELS][4][4][16];
/* El suelo y el techo: por nivel (sin el negro), la línea (y mod 4) y el
 * byte (par o impar: x mod 4), el byte que queda. Llevan los colores del
 * nivel. */
static uint8_t floor_lut[FOG_BLACK][4][2][256], ceiling_lut[FOG_BLACK][4][2][256];
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

static void surface_map(uint8_t map[16], const uint8_t *colours)
{
    for (int c = 0; c < 16; c++)
        map[c] = (uint8_t)c;
    map[1] = colours[0];
    map[7] = colours[1];
    map[15] = colours[2];
}

static void make_surface(uint8_t lut[FOG_BLACK][4][2][256], const uint8_t *map, const rgb_t *pal)
{
    for (int f = 0; f < FOG_BLACK; f++) {
        mix_t m[16];
        mixes(m, map, f, pal, 16);
        for (int y = 0; y < 4; y++)
            for (int k = 0; k < 2; k++) {
                uint8_t lo[16], hi[16], *p = lut[f][y][k];
                for (int c = 0; c < 16; c++) {
                    lo[c] = (uint8_t)mix_at(m[c], 2 * k, y);
                    hi[c] = (uint8_t)(mix_at(m[c], 2 * k + 1, y) << 4);
                }
                for (int h = 0; h < 16; h++)
                    for (int l = 0; l < 16; l++)
                        *p++ = lo[l] | hi[h];
            }
    }
}

/* Los colores de una mitad de la vista: los comunes y 4 para su superficie. */
static void surface_palette(rgb_t *pal, const rgb_t *common, const uint16_t *count, const uint8_t *map)
{
    for (int i = 0; i < COMMON; i++)
        pal[i] = common[i];
    ntargets = 0;
    add_drawing(count, 1, map, FOG_BLACK);
    quantize(pal, COMMON, 16);
    for (int i = 0; i < 16; i++)
        pal[i] = shown(pal[i]);
}

void view_level(int wall)
{
    uint8_t fmap[16], cmap[16];
    surface_map(fmap, floor_colours[wall]);
    surface_map(cmap, ceiling_colours[wall]);

    /* Los comunes: el negro, los ojos, los colores de cerca que más se ven,
     * tal cual, y los demás para lo que queda (lo de lejos, lo que menos se
     * ve). */
    rgb_t common[COMMON] = { { 0, 0, 0 }, fog_rgb(10, 0) };
    int fixed = EYES + 1;
    ntargets = 0;
    add_world(2);
    while (fixed < EYES + 1 + FIXED) {
        int best = -1;
        for (int i = 0; i < ntargets; i++)
            if (targets[i].w && (best < 0 || targets[i].w > targets[best].w))
                best = i;
        if (best < 0)
            break;
        uint32_t d;
        nearest(targets[best].c, common, fixed, &d);
        if (d)                          /* el negro y los ojos ya están */
            common[fixed++] = targets[best].c;
        targets[best].w = 0;
    }
    ntargets = 0;
    add_world(FOG_BLACK);
    quantize(common, fixed, COMMON);
    for (int i = 0; i < COMMON; i++)
        common[i] = shown(common[i]);
    surface_palette(pal_ceiling, common, art_ceiling_count, cmap);
    surface_palette(pal_floor, common, art_floor_count, fmap);

    /* Las paredes y los objetos, con los comunes. */
    for (int f = 0; f < FOG_LEVELS; f++) {
        mix_t m[16];
        mixes(m, 0, f, common, COMMON);
        for (int c = 0; c < 16; c++) {
            for (int x = 0; x < 4; x++)
                for (int y = 0; y < 4; y++)
                    wall_lut[f][x][y][c] = (uint8_t)(mix_at(m[c], x, y) * 0x11);
            int t = c == ZXV_TRANSPARENT;
            item_fog[f][c] = (uint8_t)(t ? c : nearest(fog_rgb(c, f), common, COMMON, 0));
            if (c == 10)
                shadow_fog[f][c] = EYES;
            else
                shadow_fog[f][c] = (uint8_t)(t || f >= 4 ? ZXV_TRANSPARENT : item_fog[f][c]);
        }
    }
    make_surface(ceiling_lut, cmap, pal_ceiling);

    /* El borde: el color de la pared del nivel que más se ve (sin contar el
     * negro, el blanco y los grises) en la penumbra, el más parecido de los
     * comunes (sin el negro ni los ojos): está en las dos tablas de la vista
     * y cambia con ellas, con la luz de la lámpara y el rojo de un golpe. */
    int c = -1;
    for (int i = 1; i < 16; i++)
        if ((i & 7) != 7 && i != 8 && (c < 0 || art_wall_count[wall][i] > art_wall_count[wall][c]))
            c = i;
    border_colour = c < 0 || !art_wall_count[wall][c] ? 0
                  : EYES + 1 + nearest(fog_rgb(c, FOG_BORDER), pal_ceiling + EYES + 1, COMMON - EYES - 1, 0);
    make_surface(floor_lut, fmap, pal_floor);
}

/* Los colores de la vista en este frame: los del nivel, con la luz de la
 * lámpara (bright, x 256) y el rojo de un golpe (flash, x 256). */
void view_colours(int bright, int flash)
{
    const rgb_t *src[2] = { pal_ceiling, pal_floor };
    for (int t = 0; t < 2; t++)
        for (int i = 0; i < 16; i++) {
            rgb_t c = src[t][i];
            int k = i == EYES ? 256 : bright;
            int r = c.r * k >> 8, g = c.g * k >> 8, b = c.b * k >> 8;
            r += (255 - r) * flash >> 8;
            g -= g * flash >> 8;
            b -= b * flash >> 8;
            video_pal[PAL_CEILING + t][i] = rgb565((rgb_t){ (uint8_t)(r > 255 ? 255 : r), (uint8_t)g, (uint8_t)b });
        }
}

/* El color de la vista más parecido al color c de siempre: para lo que se
 * escribe sobre el techo. */
int view_border(void) { return border_colour; }

int view_ui_colour(int c) { return nearest(fog_rgb(c, 0), pal_ceiling, 16, 0); }

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
    if (f == FOG_BLACK) {               /* a oscuras: negro */
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
                    const uint8_t *row_fog, uint8_t lut[FOG_BLACK][4][2][256])
{
    int y = y0;
    while (y < y1) {
        int f = row_fog[y], e = y + 1;
        while (e < y1 && row_fog[e] == f)
            e++;
        if (f == FOG_BLACK || !t->texels) {
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
        if (f == FOG_BLACK && !shadow)
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
        row_fog[y] = (uint8_t)(dy ? fog_level((int32_t)(((int32_t)FOCAL << 15) / dy), light) : FOG_BLACK);
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
