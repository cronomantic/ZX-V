/*
 * art.c - Los dibujos de la mina: los tiles de 16 x 16 de la cueva, hechos
 * con el motor (rectángulos, círculos y polígonos) sobre un bitmap, y el
 * minero, dibujado con caracteres.
 *
 * Las piedras, los diamantes y los bichos son tiles con el fondo
 * transparente (el 8, que en el mapa es negro): de los mismos se sacan los
 * sprites que se mueven entre una celda y otra.
 */
#include "mine.h"

static uint32_t tile_mem[TILES * CELL * CELL / 8];
static const zxv_bitmap tiles_bmp = { (uint8_t *)tile_mem, CELL / 2, CELL, TILES * CELL, 0 };

uint16_t view[CW * CH];
const zxv_tilemap cave_map = { view, CW, CH, (const uint8_t *)tile_mem, CELL, 0 };

sprite_t s_tile[TILES], s_miner[MINER_FRAMES], s_icon[3];

static int base;                        /* la primera línea del tile en el bitmap */

static void R(int x, int y, int w, int h, int c) { zxv_gfx_rect(x, base + y, w, h, c); }
static void C(int x, int y, int r, int c, int fill) { zxv_gfx_circle(x, base + y, r, c, fill); }
static void L(int x0, int y0, int x1, int y1, int c) { zxv_gfx_line(x0, base + y0, x1, base + y1, c); }

static void P(const int16_t *p, int n, int c)
{
    int16_t q[16];
    for (int i = 0; i < n; i++) {
        q[i * 2] = p[i * 2];
        q[i * 2 + 1] = (int16_t)(p[i * 2 + 1] + base);
    }
    zxv_gfx_poly(q, n, c);
}

static void tile(int t, int bg)
{
    base = t * CELL;
    R(0, 0, CELL, CELL, bg);
}

static uint32_t art_seed;
static int arnd(int n)
{
    art_seed = art_seed * 1103515245u + 12345u;
    return (int)((art_seed >> 16) % (uint32_t)n);
}

/* --- Los tiles que cambian de color con la cueva ---------------------------- */

static int bright(int c) { return c && c < 8 ? c + 8 : c; }
static int dark(int c) { return c > 8 ? c - 8 : c == 0 ? 0 : 1; }

void art_cave(const cave_t *c)
{
    zxv_gfx_target(&tiles_bmp);
    zxv_gfx_clip(0, 0, CELL, TILES * CELL);

    /* La tierra: el color con motas, que no se note la cuadrícula. */
    art_seed = 99;
    tile(T_DIRT, c->dirt);
    for (int i = 0; i < 46; i++) {
        int x = arnd(16), y = arnd(16);
        R(x, y, 1 + arnd(2), 1, i < 30 ? c->dirt2 : 0);
    }

    /* El ladrillo: hileras de 8 x 4 desplazadas. */
    tile(T_WALL, 0);
    for (int r = 0; r < 4; r++)
        for (int k = -1; k < 2; k++) {
            int x = k * 8 + (r & 1) * 4;
            R(x, r * 4, 7, 3, c->wall);
            R(x, r * 4, 7, 1, bright(c->wall));
            R(x + 6, r * 4 + 1, 1, 2, dark(c->wall));
        }
    zxv_gfx_target(0);
    zxv_gfx_clip(0, 0, W, H);
}

/* --- Los que no cambian ------------------------------------------------------ */

static void make_tiles(void)
{
    zxv_gfx_target(&tiles_bmp);
    zxv_gfx_clip(0, 0, CELL, TILES * CELL);

    tile(T_EMPTY, 0);
    tile(T_FLASH, 15);

    /* El acero: una plancha con remaches. */
    tile(T_STEEL, 7);
    R(0, 0, 16, 1, 15);
    R(0, 0, 1, 16, 15);
    R(0, 15, 16, 1, 1);
    R(15, 0, 1, 16, 1);
    for (int k = 0; k < 4; k++) {
        int x = k & 1 ? 11 : 3, y = k & 2 ? 11 : 3;
        R(x, y, 2, 2, 1);
        R(x, y, 1, 1, 15);
    }

    /* La piedra: redonda, con luz arriba a la izquierda. */
    tile(T_BOULDER, ZXV_TRANSPARENT);
    C(8, 8, 7, 1, 1);
    C(7, 7, 6, 7, 1);
    C(5, 5, 2, 15, 1);
    L(9, 9, 11, 11, 1);
    L(10, 6, 12, 7, 1);
    R(4, 11, 2, 1, 1);

    /* El diamante, en cuatro brillos. */
    static const int16_t crown[] = { 4, 2, 11, 2, 14, 6, 1, 6 };
    static const int16_t pav[] = { 1, 7, 14, 7, 8, 14, 7, 14 };
    for (int f = 0; f < 4; f++) {
        tile(T_DIAMOND + f, ZXV_TRANSPARENT);
        P(pav, 4, 5);
        P(crown, 4, 13);
        L(4, 7, 7, 13, 13);
        L(11, 7, 8, 13, 1);
        L(5, 2, 6, 6, 15);
        L(10, 2, 9, 6, 5);
        R(1, 6, 14, 1, 15);
        static const int8_t sx[4] = { 4, 10, 7, 12 }, sy[4] = { 3, 8, 4, 4 };
        int x = sx[f], y = sy[f];
        R(x - 1, y, 3, 1, 15);
        R(x, y - 1, 1, 3, 15);
    }

    /* La salida cerrada (una puerta de acero) y abierta (una luz que late). */
    tile(T_EXIT, 7);
    R(0, 0, 16, 1, 15);
    R(0, 0, 1, 16, 15);
    R(3, 2, 10, 14, 1);
    R(4, 3, 8, 13, 7);
    R(9, 9, 2, 2, 1);
    for (int f = 0; f < 2; f++) {
        tile(T_EXIT_OPEN + f, 0);
        R(2, 1, 12, 15, f ? 14 : 6);
        R(4, 3, 8, 13, f ? 15 : 14);
        R(6, 5, 4, 11, f ? 14 : 15);
    }

    /* La entrada: un marco que parpadea. */
    for (int f = 0; f < 2; f++) {
        tile(T_ENTRY + f, 0);
        R(1, 1, 14, 14, f ? 14 : 10);
        R(3, 3, 10, 10, 0);
        C(7, 7, f ? 3 : 2, f ? 15 : 14, 1);
    }

    /* El musgo: burbujas verdes. */
    for (int f = 0; f < 2; f++) {
        tile(T_MOSS + f, 4);
        art_seed = 7 + f * 3;
        for (int i = 0; i < 7; i++) {
            int x = 2 + arnd(12), y = 2 + arnd(12), r = 1 + arnd(2);
            C(x, y, r, 12, 1);
            R(x - 1, y - 1, 1, 1, 15);
        }
        R(0, 0, 16, 1, 0);
        R(0, 0, 1, 16, 0);
    }

    /* El tamiz: una malla; encendido, brilla. */
    for (int f = 0; f < 3; f++) {
        tile(f ? T_SIEVE_ON + f - 1 : T_SIEVE, 1);
        int a = f == 0 ? 7 : f == 1 ? 13 : 11, b = f == 0 ? 5 : f == 1 ? 15 : 13;
        for (int k = 0; k < 16; k += 4) {
            R(k + (f == 2 ? 2 : 0), 0, 1, 16, a);
            R(0, k + (f == 1 ? 2 : 0), 16, 1, a);
        }
        for (int k = 0; k < 16; k += 4)
            R(k + 2, (k * 3) & 15, 1, 1, b);
    }

    /* La explosión, en cuatro pasos. */
    static const uint8_t col[4][3] = { { 15, 14, 15 }, { 14, 10, 15 }, { 10, 2, 14 }, { 2, 1, 10 } };
    for (int f = 0; f < 4; f++) {
        tile(T_BOOM + f, 0);
        C(8, 8, 4 + f, col[f][0], 1);
        C(8, 8, 2 + f, col[f][1], 1);
        for (int i = 0; i < 6; i++) {
            int x = (i * 5 + f * 3) % 14, y = (i * 9 + f * 7) % 14;
            R(x, y, 2, 2, col[f][2]);
        }
        if (f == 3)
            C(8, 8, 2, 0, 1);
    }

    /* El ascua: una brasa que gira. */
    for (int f = 0; f < 4; f++) {
        tile(T_EMBER + f, ZXV_TRANSPARENT);
        int r = f & 1 ? 6 : 5;
        if (f & 1) {
            static const int16_t d[] = { 8, 1, 15, 8, 8, 15, 1, 8 };
            P(d, 4, 2);
        } else {
            R(2, 2, 12, 12, 2);
        }
        C(8, 8, r - 1, 10, 1);
        C(8, 8, r - 3, 14, 1);
        C(8 + (f == 2) - (f == 0), 8 + (f == 3) - (f == 1), 1, 15, 1);
    }

    /* El murciélago: alas arriba y abajo. */
    for (int f = 0; f < 4; f++) {
        tile(T_BAT + f, ZXV_TRANSPARENT);
        int up = f == 0 ? -4 : f == 1 || f == 3 ? 0 : 4;
        const int16_t l[] = { 7, 7, 1, (int16_t)(5 + up), 0, (int16_t)(9 + up), 6, 11 };
        const int16_t r[] = { 9, 7, 15, (int16_t)(5 + up), 16, (int16_t)(9 + up), 10, 11 };
        P(l, 4, 3);
        P(r, 4, 3);
        C(8, 9, 3, 11, 1);
        R(6, 4, 1, 3, 11);
        R(10, 4, 1, 3, 11);
        R(6, 8, 2, 1, 15);
        R(9, 8, 2, 1, 15);
        R(7, 11, 3, 1, 10);
    }

    zxv_gfx_target(0);
    zxv_gfx_clip(0, 0, W, H);
}

/* --- El minero, de 16 x 16 ----------------------------------------------------- */

static const char *const miner_art[MINER_FRAMES][16] = {
    {   /* de frente */
        "......eeee......", "....eeeffeee....", "...eeeeffeeee...", "..eeeeeeeeeeee..",
        "....ffffffff....", "....f00ff00f....", "....ffffffff....", ".....ff22ff.....",
        "...aaaaaaaaaa...", "..faa9aaaa9aaf..", "..f.99999999.f..", "....99999999....",
        "....999..999....", "....999..999....", "...1111..1111...", "...1111..1111...",
    },
    {   /* parpadea */
        "......eeee......", "....eeeffeee....", "...eeeeffeeee...", "..eeeeeeeeeeee..",
        "....ffffffff....", "....ffffffff....", "....f00ff00f....", ".....ff22ff.....",
        "...aaaaaaaaaa...", "..faa9aaaa9aaf..", "..f.99999999.f..", "....99999999....",
        "....999..999....", "....999..999....", "...1111..1111...", "...1111..1111...",
    },
    {   /* andando, 1 */
        ".....eeee.......", "...eeeeeeef.....", "..eeeeeeeeff....", "..eeeeeeeeeee...",
        "....fffffff.....", "....fffff0f.....", "....ffffffff....", ".....ffff2f.....",
        "....aaaaaa......", "....aaaaaaff....", "....99999.......", "....999999......",
        "...999..99......", "..999....99.....", ".111......11....", ".111......111...",
    },
    {   /* andando, 2 */
        ".....eeee.......", "...eeeeeeef.....", "..eeeeeeeeff....", "..eeeeeeeeeee...",
        "....fffffff.....", "....fffff0f.....", "....ffffffff....", ".....ffff2f.....",
        "....aaaaaa......", "...faaaaaa......", "....99999.......", "....999999......",
        ".....9999.......", ".....999........", "....1111........", "....11111.......",
    },
    {   /* empujando */
        "......eeee......", "....eeeeeeef....", "...eeeeeeeeff...", "...eeeeeeeeeee..",
        ".....fffffff....", ".....fffff0f....", ".....ffffffff...", "......ffff2f....",
        "....aaaaaaaafff.", "...aaaaaaaa.fff.", "...99999........", "..999999........",
        ".999...99.......", "999.....99......", "11.......11.....", "1........111....",
    },
    {   /* subiendo, 1 */
        "......eeee......", "....eeeeeeee....", "...eeeeeeeeee...", "..eeeeeeeeeeee..",
        "....ffffffff....", "....ffffffff....", "....ffffffff....", ".....ffffff.....",
        "..f.aaaaaaaa....", "..faaaaaaaaaaf..", "....99999999.f..", "....99999999....",
        "....999..999....", "....999..1111...", "...1111..1111...", "...1111.........",
    },
    {   /* subiendo, 2 */
        "......eeee......", "....eeeeeeee....", "...eeeeeeeeee...", "..eeeeeeeeeeee..",
        "....ffffffff....", "....ffffffff....", "....ffffffff....", ".....ffffff.....",
        "....aaaaaaaa.f..", "..faaaaaaaaaaf..", "..f.99999999....", "....99999999....",
        "....999..999....", "...1111..999....", "...1111..1111...", ".........1111...",
    },
    {   /* contento */
        "......eeee......", "....eeeffeee....", "...eeeeffeeee...", "f.eeeeeeeeeeee.f",
        "f...ffffffff...f", "a...f00ff00f...a", "aa..ffffffff..aa", ".aa..f2222f..aa.",
        "..aaaaaaaaaaaa..", "....aa9aa9aa....", "....99999999....", "....99999999....",
        "....999..999....", "....999..999....", "...1111..1111...", "...1111..1111...",
    },
};

static const char *const icon_art[3][8] = {
    { "..dddd..", ".dffddd.", "dddddddd", "ffffffff", ".5d5d5d.", "..5d5d..", "...55...", "........" },
    { "..ffff..", ".f0000f.", "f00f000f", "f00f000f", "f00fff0f", "f000000f", ".f0000f.", "..ffff.." },
    { "..eeee..", ".eeffee.", "eeeeeeee", ".ffffff.", ".f0ff0f.", ".ffffff.", "..f22f..", "........" },
};

/* Unas celdas sueltas con los tiles de la cueva (el título). */
void draw_tiles(const uint16_t *cells, int w, int h, int x, int y)
{
    const zxv_tilemap m = { cells, (uint16_t)w, (uint16_t)h, (const uint8_t *)tile_mem, CELL, 0 };
    zxv_map_draw(&m, 0, 0, x, y, w * CELL, h * CELL, ZXV_MAP_ALL);
}

void art_init(void)
{
    make_tiles();
    art_cave(&caves[0]);
    for (int t = 0; t < TILES; t++)
        if (t == T_BOULDER || (t >= T_DIAMOND && t < T_DIAMOND + 4) || t >= T_EMBER)
            s_tile[t] = sprite_from(&tiles_bmp, 0, t * CELL, CELL, CELL);
    for (int f = 0; f < MINER_FRAMES; f++)
        s_miner[f] = sprite_ascii(miner_art[f], 16, 16);
    for (int i = 0; i < 3; i++)
        s_icon[i] = sprite_ascii(icon_art[i], 8, 8);
}
