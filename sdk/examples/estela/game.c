/*
 * game.c - Estela: un matamarcianos de tres fases para ZX-V.
 *
 * La nave va de izquierda a derecha sobre un fondo de estrellas en tres
 * capas; en la segunda y la tercera fase, entre las paredes de un cañón y de
 * una fortaleza (un mapa de tiles que se desplaza píxel a píxel). Al final de
 * cada fase espera un jefe. Todo se dibuja cada frame en la página oculta y
 * se cambia de página en el VBLANK (doble buffer, a 60 imágenes por segundo,
 * ZXV_FRAME_HZ; las velocidades y los tiempos, en segundos, como cuando iba a
 * 50: V() y MS() en estela.h; el terreno, un píxel por frame, con PX()).
 *
 * Controles: cursores o OPQA para mover, espacio o M para disparar, Intro
 * para la pausa y Esc para salir; o un mando. Sin tocar nada en el título,
 * empieza una demostración que juega sola. El récord se guarda en
 * SD:estela.txt (en la tarjeta, aunque el juego se cargue desde ROM:).
 *
 * Compilado con -DESTELA_STATS (make EXTRA_CFLAGS=-DESTELA_STATS), el
 * marcador enseña los frames perdidos y la demostración juega las tres fases
 * enteras: para medir el rendimiento.
 *
 * Con -DESTELA_BOT=semilla, para medir la dificultad (con zxv-cli --test):
 * sin título, el piloto de la demostración juega las tres fases desde el
 * principio y recibe los golpes como un jugador, pero no se queda sin naves;
 * al final de cada fase escribe las vidas que ha perdido, de qué, cuántas
 * con el jefe y lo que ha tardado.
 *
 * main() devuelve el récord.
 */
#include "estela.h"

uint32_t rng_state = 1;

#ifdef ESTELA_BOT
#define ESTELA_STATS
static int bot_deaths, bot_hits, bot_now, bot_cause[4], bot_boss, bot_boss_f;
#define HIT(cause) (bot_now = (cause), player_hit())   /* 0 paredes, 1 balas, 2 enemigos, 3 jefe */

/* write(1, s, len) y exit(code) de zxv-cli --test. */
static void bot_write(const char *s)
{
    int n = 0;
    while (s[n])
        n++;
    register int a0 __asm__("a0") = 1;
    register const char *a1 __asm__("a1") = s;
    register int a2 __asm__("a2") = n;
    register int a7 __asm__("a7") = 64;
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7) : "memory");
}

static void bot_exit(int code)
{
    register int a0 __asm__("a0") = code;
    register int a7 __asm__("a7") = 93;
    __asm__ volatile("ecall" : : "r"(a0), "r"(a7) : "memory");
    for (;;)
        ;
}

static void bot_number(int v)
{
    char b[12];
    int n = 0;
    do {
        b[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    char s[12];
    for (int i = 0; i < n; i++)
        s[i] = b[n - 1 - i];
    s[n] = 0;
    bot_write(s);
}
#else
#define HIT(cause) player_hit()
#endif

int lang;

const int8_t sin64[64] = {
    0, 6, 12, 19, 24, 30, 36, 41, 45, 49, 53, 56, 59, 61, 63, 64, 64, 64, 63, 61, 59, 56,
    53, 49, 45, 41, 36, 30, 24, 19, 12, 6, 0, -6, -12, -19, -24, -30, -36, -41, -45, -49,
    -53, -56, -59, -61, -63, -64, -64, -64, -63, -61, -59, -56, -53, -49, -45, -41, -36,
    -30, -24, -19, -12, -6,
};

static int isqrt(int v)
{
    int r = 0;
    for (int b = 1 << 14; b; b >>= 1)
        if ((r + b) * (r + b) <= v)
            r += b;
    return r;
}

/* --- La entrada -------------------------------------------------------------------- */

enum { IN_UP = 1, IN_DOWN = 2, IN_LEFT = 4, IN_RIGHT = 8, IN_FIRE = 16, IN_PAUSE = 32, IN_ESC = 64 };

static int key(unsigned hid)
{
    const volatile uint8_t *map = (const volatile uint8_t *)(uintptr_t)ZXV_I_KEYMAP;
    return (map[hid >> 3] >> (hid & 7)) & 1;
}

static int input_now, input_edge;

static void read_input(void)
{
    int in = 0;
    if (key(0x52) || key(0x14))
        in |= IN_UP;                    /* cursor arriba, Q */
    if (key(0x51) || key(0x04))
        in |= IN_DOWN;                  /* abajo, A */
    if (key(0x50) || key(0x12))
        in |= IN_LEFT;                  /* izquierda, O */
    if (key(0x4F) || key(0x13))
        in |= IN_RIGHT;                 /* derecha, P */
    if (key(0x2C) || key(0x10))
        in |= IN_FIRE;                  /* espacio, M */
    if (key(0x28))
        in |= IN_PAUSE;                 /* Intro */
    if (key(0x29))
        in |= IN_ESC;
    uint16_t joy = *(volatile uint16_t *)(uintptr_t)ZXV_I_JOY0;
    if (joy & ZXV_JOY_CONNECTED) {
        if (joy & ZXV_JOY_UP) in |= IN_UP;
        if (joy & ZXV_JOY_DOWN) in |= IN_DOWN;
        if (joy & ZXV_JOY_LEFT) in |= IN_LEFT;
        if (joy & ZXV_JOY_RIGHT) in |= IN_RIGHT;
        if (joy & (ZXV_JOY_FIRE1 | ZXV_JOY_FIRE2)) in |= IN_FIRE;
        if (joy & ZXV_JOY_START) in |= IN_PAUSE;
    }
    input_edge = in & ~input_now;
    input_now = in;
    /* Lo tecleado no le sirve al juego: que no le llegue al BASIC al volver. */
    while (zxv_key_get() >= 0)
        ;
}

/* --- Los gráficos ------------------------------------------------------------------- */

static const char *const ship_art[10] = {
    "...77...........",
    "...777..........",
    "a..77777........",
    "aa.777755777....",
    "aaa77775555777f.",
    "aaa77777777777ff",
    "aa.1111111111...",
    "a..11111........",
    "...111..........",
    "...11...........",
};

static const char *const dart_art[8] = {
    "......bb....",
    "....bbbb3...",
    "..bbbbbb33..",
    "eebbbbbb333.",
    "eebbbbbb333.",
    "..bbbbbb33..",
    "....bbbb3...",
    "......bb....",
};

static const char *const turret_art[8] = {
    ".......77.......",
    ".......77.......",
    "......7777......",
    "....11777711....",
    "...1177aa77711..",
    "..117777777711..",
    ".11111111111111.",
    "1111111111111111",
};

enum { E_DART, E_WHEEL, E_ROCK, E_PEBBLE, E_TURRET, E_MINE, E_CARRIER };

static sprite_t s_ship, s_shield, s_dart, s_wheel[2], s_rock, s_pebble, s_turret, s_mine, s_carrier;
static sprite_t s_ebullet, s_power[2], s_boom[4], s_boss[3];

static void make_sprites(void)
{
    s_ship = sprite_ascii(ship_art, 16, 10);
    s_dart = sprite_ascii(dart_art, 12, 8);
    s_turret = sprite_ascii(turret_art, 16, 8);

    canvas(24, 18);                                     /* el escudo, alrededor de la nave */
    zxv_gfx_circle(11, 8, 11, 13, 0);
    zxv_gfx_circle(11, 8, 10, 5, 0);
    s_shield = canvas_sprite();

    for (int f = 0; f < 2; f++) {                       /* la rueda, en dos posiciones */
        canvas(14, 14);
        zxv_gfx_circle(6, 6, 6, 13, 0);
        zxv_gfx_circle(6, 6, 5, 5, 0);
        if (f) {
            zxv_gfx_line(2, 2, 10, 10, 15);
            zxv_gfx_line(2, 10, 10, 2, 15);
        } else {
            zxv_gfx_line(6, 0, 6, 12, 15);
            zxv_gfx_line(0, 6, 12, 6, 15);
        }
        zxv_gfx_circle(6, 6, 2, 10, 1);
        s_wheel[f] = canvas_sprite();
    }

    canvas(24, 24);                                     /* la roca grande */
    zxv_gfx_circle(11, 12, 11, 1, 1);
    zxv_gfx_circle(12, 11, 9, 7, 1);
    zxv_gfx_circle(8, 8, 3, 1, 1);
    zxv_gfx_circle(15, 15, 2, 1, 1);
    zxv_gfx_circle(16, 7, 2, 15, 1);
    s_rock = canvas_sprite();

    canvas(12, 12);                                     /* la pequeña */
    zxv_gfx_circle(5, 6, 5, 1, 1);
    zxv_gfx_circle(6, 5, 4, 7, 1);
    zxv_gfx_rect(4, 4, 2, 2, 1);
    s_pebble = canvas_sprite();

    canvas(15, 15);                                     /* la mina, con pinchos */
    zxv_gfx_line(7, 0, 7, 14, 6);
    zxv_gfx_line(0, 7, 14, 7, 6);
    zxv_gfx_line(2, 2, 12, 12, 6);
    zxv_gfx_line(2, 12, 12, 2, 6);
    zxv_gfx_circle(7, 7, 4, 2, 1);
    zxv_gfx_circle(7, 7, 1, 14, 1);
    s_mine = canvas_sprite();

    canvas(22, 12);                                     /* el portador de mejoras */
    zxv_gfx_rect(4, 3, 14, 7, 6);
    zxv_gfx_circle(4, 6, 3, 6, 1);
    zxv_gfx_circle(17, 6, 3, 6, 1);
    zxv_gfx_rect(7, 0, 8, 3, 14);
    zxv_gfx_rect(8, 5, 2, 2, 2);
    zxv_gfx_rect(12, 5, 2, 2, 2);
    zxv_gfx_line(0, 6, 3, 6, 14);
    s_carrier = canvas_sprite();

    canvas(5, 5);                                       /* las balas enemigas */
    zxv_gfx_circle(2, 2, 2, 10, 1);
    zxv_gfx_rect(2, 2, 1, 1, 15);
    s_ebullet = canvas_sprite();

    for (int k = 0; k < 2; k++) {                       /* las mejoras: arma y escudo */
        canvas(11, 11);
        zxv_gfx_circle(5, 5, 5, k ? 5 : 2, 1);
        zxv_gfx_circle(5, 5, 4, k ? 13 : 10, 0);
        if (k) {
            zxv_gfx_circle(5, 5, 2, 15, 0);
        } else {
            zxv_gfx_rect(2, 5, 7, 1, 15);
            zxv_gfx_rect(6, 3, 1, 5, 15);
        }
        s_power[k] = canvas_sprite();
    }

    for (int f = 0; f < 4; f++) {                       /* la explosión, en cuatro pasos */
        canvas(16, 16);
        static const uint8_t col[4][3] = { { 15, 14, 14 }, { 14, 10, 15 }, { 10, 2, 14 }, { 2, 0, 10 } };
        zxv_gfx_circle(8, 8, 3 + f * 2, col[f][0], 1);
        zxv_gfx_circle(8, 8, 1 + f * 2, col[f][1], 1);
        for (int i = 0; i < 6; i++) {
            int a = i * 11 + f * 5;
            zxv_gfx_rect(8 + icos(a) * (2 + f * 2) / 64, 8 + isin(a) * (2 + f * 2) / 64, 2, 2, col[f][2]);
        }
        if (f == 3)
            zxv_gfx_circle(8, 8, 3, ZXV_TRANSPARENT, 1);
        s_boom[f] = canvas_sprite();
    }

    /* Los jefes. */
    canvas(48, 48);                                     /* 1: la roca madre */
    zxv_gfx_circle(23, 24, 23, 1, 1);
    zxv_gfx_circle(24, 23, 21, 7, 1);
    zxv_gfx_circle(13, 12, 5, 1, 1);
    zxv_gfx_circle(34, 33, 4, 1, 1);
    zxv_gfx_circle(33, 12, 3, 1, 1);
    zxv_gfx_circle(12, 34, 3, 1, 1);
    zxv_gfx_circle(23, 23, 8, 0, 1);
    zxv_gfx_circle(23, 23, 6, 10, 1);
    zxv_gfx_circle(21, 22, 2, 15, 1);
    s_boss[0] = canvas_sprite();

    canvas(56, 36);                                     /* 2: el guardián */
    {
        static const int16_t hull[] = { 0, 18, 18, 4, 50, 2, 55, 18, 50, 33, 18, 31 };
        zxv_gfx_poly(hull, 6, 4);
        static const int16_t deck[] = { 6, 18, 20, 9, 44, 8, 47, 18, 44, 27, 20, 26 };
        zxv_gfx_poly(deck, 6, 12);
        zxv_gfx_rect(0, 16, 14, 5, 7);
        zxv_gfx_rect(0, 17, 4, 3, 10);
        zxv_gfx_circle(30, 17, 5, 1, 1);
        zxv_gfx_circle(30, 17, 3, 13, 1);
        zxv_gfx_rect(48, 6, 7, 4, 10);
        zxv_gfx_rect(48, 26, 7, 4, 10);
    }
    s_boss[1] = canvas_sprite();

    canvas(56, 56);                                     /* 3: el núcleo */
    zxv_gfx_rect(4, 4, 48, 48, 1);
    zxv_gfx_rect(6, 6, 44, 44, 9);
    zxv_gfx_rect(8, 8, 40, 40, 7);
    for (int i = 0; i < 4; i++) {
        zxv_gfx_rect(10 + i * 10, 10, 6, 4, 0);
        zxv_gfx_rect(10 + i * 10, 42, 6, 4, 0);
    }
    zxv_gfx_rect(0, 12, 4, 8, 7);
    zxv_gfx_rect(0, 36, 4, 8, 7);
    zxv_gfx_circle(27, 27, 12, 0, 1);
    zxv_gfx_circle(27, 27, 10, 10, 1);
    zxv_gfx_circle(27, 27, 6, 14, 1);
    zxv_gfx_circle(25, 25, 2, 15, 1);
    s_boss[2] = canvas_sprite();
}

/* Recolorear: todo blanco (al recibir un disparo) o el núcleo cerrado. */
static const uint8_t c_white[16] = { 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 };
static const uint8_t c_closed[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 11, 12, 13, 9, 1 };

/* --- El terreno -------------------------------------------------------------------- */

#define MAP_COLS    512
#define MAP_ROWS    (H / 8)
enum { T_EMPTY, T_ROCK, T_ROCK_TOP, T_ROCK_BOTTOM, T_METAL, T_METAL_TOP, T_METAL_BOTTOM, T_LIGHT, TILES };

static uint32_t tile_mem[TILES * 8];
static uint16_t cells[MAP_COLS * MAP_ROWS];
static uint8_t ceil_rows[MAP_COLS], ground_row[MAP_COLS];    /* filas sólidas: 1..ceil-1 y ground..23 */
static int band_top, band_bottom;
static const zxv_tilemap terrain = { cells, MAP_COLS, MAP_ROWS, (const uint8_t *)tile_mem, 8, 0 };

static void tile_px(int t, int x, int y, int c)
{
    uint8_t *p = (uint8_t *)tile_mem + t * 32 + y * 4 + (x >> 1);
    *p = x & 1 ? (uint8_t)((*p & 0x0F) | c << 4) : (uint8_t)((*p & 0xF0) | c);
}

static void make_tiles(void)
{
    for (int t = 0; t < TILES; t++)
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
                int c = ZXV_TRANSPARENT;
                uint32_t r = rnd() & 15;
                switch (t) {
                case T_ROCK:
                    c = r < 3 ? 9 : r < 5 ? 0 : 1;
                    break;
                case T_ROCK_TOP:                /* el borde de arriba, con picos */
                    c = y < ((x * 5 + 3) % 4) ? ZXV_TRANSPARENT : y < ((x * 5 + 3) % 4) + 2 ? 13 : r < 3 ? 9 : 1;
                    break;
                case T_ROCK_BOTTOM:             /* el de abajo (el techo) */
                    c = 7 - y < ((x * 3 + 1) % 4) ? ZXV_TRANSPARENT : 7 - y < ((x * 3 + 1) % 4) + 2 ? 13 : r < 3 ? 9 : 1;
                    break;
                case T_METAL:
                    c = x == 0 || y == 0 ? 15 : x == 7 || y == 7 ? 1 : 7;
                    break;
                case T_METAL_TOP:
                    c = y < 2 ? ZXV_TRANSPARENT : y == 2 ? 15 : y == 3 ? 14 : x == 7 ? 1 : 7;
                    break;
                case T_METAL_BOTTOM:
                    c = y > 5 ? ZXV_TRANSPARENT : y == 5 ? 15 : y == 4 ? 14 : x == 7 ? 1 : 7;
                    break;
                case T_LIGHT:
                    c = (x - 3) * (x - 3) + (y - 3) * (y - 3) < 5 ? 10 : x == 0 || y == 0 ? 15 : 1;
                    break;
                default:
                    break;
                }
                tile_px(t, x, y, c);
            }
}

/* El terreno avanza un píxel por frame. Lo que mide el recorrido de una fase
 * (dónde salen las oleadas, dónde espera el jefe...) va en cincuentavos de
 * segundo, que eran sus píxeles cuando el juego iba a 50 Hz: PX() lo pasa a
 * los píxeles de ahora, y todo sale a los mismos segundos (a 60 Hz, con las
 * fases un 20 % más largas). */
#define PX(t)       ((t) * ZXV_FRAME_HZ / 50)

/* El terreno de una fase: 0 sin paredes, 1 el cañón, 2 la fortaleza. Las
 * paredes cambian de media cada 5 (o 12) cincuentavos de segundo. */
static void make_terrain(int style, int length)
{
    int ceil = 2, ground = 21;
    band_top = 0;
    band_bottom = MAP_ROWS;
    for (int c = 0; c < MAP_COLS; c++) {
        if (style && c > PX(40) && c * 8 < length - PX(160) &&
            rnd() % ((style == 1 ? 5 : 12) * ZXV_FRAME_HZ) < 50) {
            int step = style == 1 ? 1 : 2;
            ceil += rnd() & 1 ? step : -step;
            ground += rnd() & 1 ? step : -step;
            if (ceil < 1) ceil = 1;
            if (ceil > 7) ceil = 7;
            if (ground > 23) ground = 23;
            if (ground < 15) ground = 15;
            if (ground - ceil < 11) {           /* siempre cabe la nave, de sobra */
                ceil = ground - 11;
                if (ceil < 1) {
                    ceil = 1;
                    ground = 12;
                }
            }
        }
        if (c * 8 >= length - PX(160)) {    /* donde espera el jefe, sin estrecheces */
            ceil += ceil > 2 ? -1 : 0;
            ground += ground < 21 ? 1 : 0;
        }
        ceil_rows[c] = (uint8_t)(style ? ceil : 0);
        ground_row[c] = (uint8_t)(style ? ground : MAP_ROWS);
        for (int r = 0; r < MAP_ROWS; r++) {
            uint16_t t = T_EMPTY | ZXV_CELL_TRANSP;
            if (style && r > 0 && r < ceil)
                t = r == ceil - 1 ? (style == 1 ? T_ROCK_BOTTOM : T_METAL_BOTTOM) | ZXV_CELL_TRANSP
                                  : style == 1 ? T_ROCK : (c % 6 == 2 && r == ceil - 2 ? T_LIGHT : T_METAL);
            if (style && r >= ground)
                t = r == ground ? (style == 1 ? T_ROCK_TOP : T_METAL_TOP) | ZXV_CELL_TRANSP
                                : style == 1 ? T_ROCK : (c % 6 == 4 && r == ground + 1 ? T_LIGHT : T_METAL);
            cells[r * MAP_COLS + c] = t;
        }
        if (style && ceil > band_top)
            band_top = ceil;
        if (style && ground < band_bottom)
            band_bottom = ground;
    }
}

static int scroll;                      /* píxeles desplazados */

static int solid(int x, int y)
{
    if (y < HUD_H)
        return 0;
    return (zxv_map_cell(&terrain, scroll + x, y) & ZXV_CELL_TILE) != T_EMPTY;
}

/* --- Las fases ------------------------------------------------------------------- */

enum { P_LINE, P_COLUMN, P_RANDOM, P_GROUND, P_CEIL };

typedef struct {
    uint16_t at;                        /* cuándo aparece, en cincuentavos de segundo (PX()) */
    uint8_t type, n, pattern, y;
} wave_t;

static const wave_t waves1[] = {
    { 60, E_DART, 5, P_LINE, 60 }, { 220, E_DART, 5, P_LINE, 140 }, { 380, E_ROCK, 3, P_RANDOM, 0 },
    { 520, E_WHEEL, 4, P_LINE, 90 }, { 680, E_CARRIER, 1, P_LINE, 100 }, { 760, E_DART, 6, P_COLUMN, 100 },
    { 900, E_ROCK, 4, P_RANDOM, 0 }, { 1040, E_PEBBLE, 8, P_RANDOM, 0 }, { 1180, E_WHEEL, 5, P_LINE, 60 },
    { 1300, E_WHEEL, 5, P_LINE, 140 }, { 1460, E_DART, 8, P_LINE, 100 }, { 1600, E_CARRIER, 1, P_LINE, 60 },
    { 1680, E_ROCK, 5, P_RANDOM, 0 }, { 1840, E_DART, 6, P_COLUMN, 100 }, { 1980, E_WHEEL, 6, P_LINE, 100 },
    { 2120, E_PEBBLE, 10, P_RANDOM, 0 }, { 2280, E_DART, 6, P_LINE, 50 }, { 2300, E_DART, 6, P_LINE, 150 },
    { 2450, E_CARRIER, 1, P_LINE, 140 },
};

static const wave_t waves2[] = {
    { 80, E_DART, 5, P_LINE, 100 }, { 200, E_TURRET, 1, P_GROUND, 0 }, { 260, E_TURRET, 1, P_CEIL, 0 },
    { 340, E_WHEEL, 4, P_LINE, 100 }, { 460, E_TURRET, 2, P_GROUND, 0 }, { 560, E_MINE, 3, P_RANDOM, 0 },
    { 700, E_CARRIER, 1, P_LINE, 100 }, { 780, E_DART, 6, P_COLUMN, 100 }, { 900, E_TURRET, 2, P_CEIL, 0 },
    { 1000, E_TURRET, 2, P_GROUND, 0 }, { 1100, E_MINE, 4, P_RANDOM, 0 }, { 1250, E_WHEEL, 5, P_LINE, 90 },
    { 1380, E_DART, 8, P_LINE, 110 }, { 1500, E_TURRET, 3, P_GROUND, 0 }, { 1560, E_TURRET, 3, P_CEIL, 0 },
    { 1700, E_CARRIER, 1, P_LINE, 100 }, { 1800, E_MINE, 5, P_RANDOM, 0 }, { 1950, E_DART, 6, P_COLUMN, 100 },
    { 2100, E_WHEEL, 6, P_LINE, 100 }, { 2250, E_TURRET, 3, P_GROUND, 0 }, { 2400, E_DART, 8, P_LINE, 90 },
};

static const wave_t waves3[] = {
    { 60, E_WHEEL, 5, P_LINE, 100 }, { 180, E_TURRET, 2, P_GROUND, 0 }, { 220, E_TURRET, 2, P_CEIL, 0 },
    { 320, E_DART, 8, P_LINE, 80 }, { 420, E_MINE, 4, P_RANDOM, 0 }, { 540, E_CARRIER, 1, P_LINE, 100 },
    { 600, E_DART, 6, P_COLUMN, 100 }, { 720, E_TURRET, 3, P_GROUND, 0 }, { 760, E_TURRET, 3, P_CEIL, 0 },
    { 880, E_WHEEL, 6, P_LINE, 70 }, { 960, E_WHEEL, 6, P_LINE, 130 }, { 1100, E_MINE, 5, P_RANDOM, 0 },
    { 1240, E_DART, 8, P_LINE, 100 }, { 1300, E_DART, 8, P_LINE, 60 }, { 1420, E_CARRIER, 1, P_LINE, 90 },
    { 1500, E_TURRET, 4, P_GROUND, 0 }, { 1540, E_TURRET, 4, P_CEIL, 0 }, { 1700, E_DART, 6, P_COLUMN, 100 },
    { 1760, E_DART, 6, P_COLUMN, 100 }, { 1900, E_MINE, 6, P_RANDOM, 0 }, { 2050, E_WHEEL, 8, P_LINE, 100 },
    { 2200, E_CARRIER, 1, P_LINE, 110 }, { 2300, E_TURRET, 4, P_GROUND, 0 }, { 2350, E_TURRET, 4, P_CEIL, 0 },
    { 2500, E_DART, 10, P_LINE, 100 }, { 2650, E_MINE, 6, P_RANDOM, 0 },
};

typedef struct {
    const char *name_es, *name_en;
    int terrain;
    const wave_t *waves;
    int nwaves, length;                 /* length, en cincuentavos de segundo (PX()) */
} stage_t;

static const stage_t stages[3] = {
    { "CINTURÓN", "THE BELT", 0, waves1, sizeof(waves1) / sizeof(waves1[0]), 2650 },
    { "EL CAÑÓN", "THE CANYON", 1, waves2, sizeof(waves2) / sizeof(waves2[0]), 2650 },
    { "LA FORTALEZA", "THE FORTRESS", 2, waves3, sizeof(waves3) / sizeof(waves3[0]), 2900 },
};

/* --- Los objetos ---------------------------------------------------------------- */

typedef struct {
    uint8_t alive, type, flash, flip;
    int16_t hp;
    int x, y, vx, vy;                   /* en FX, y FX por frame */
    int t, base, amp;                   /* t en frames; amp, el vaivén en píxeles */
    const sprite_t *spr;
} enemy_t;

typedef struct {
    uint8_t alive;
    int x, y, vx, vy;
} shot_t;

typedef struct {
    uint8_t alive, t;
    int x, y;
} boom_t;

typedef struct {
    uint8_t alive, kind;
    int x, y, t;
} power_t;

#define MAX_ENEMIES 28
#define MAX_SHOTS   20
#define MAX_BULLETS 56
#define MAX_BOOMS   24
#define MAX_POWERS  4

static enemy_t enemies[MAX_ENEMIES];
static shot_t shots[MAX_SHOTS], bullets[MAX_BULLETS];
static boom_t booms[MAX_BOOMS];
static power_t powers[MAX_POWERS];

static struct {
    int x, y;                           /* en FX */
    int lives, weapon, shield, invuln, dead, cooldown;  /* cooldown, en milésimas de frame */
    uint32_t score, next_life;
} pl;

static struct {
    int alive, type, x, y, hp, maxhp, t, flash, dying, open;
    const sprite_t *spr;
} boss;

static int stage, wave_i, speed, demo, frame;
static uint32_t record;
static int message_t;                   /* el letrero de la fase o del jefe */
static const char *message, *message2;

static void add_boom(int x, int y)
{
    for (int i = 0; i < MAX_BOOMS; i++)
        if (!booms[i].alive) {
            booms[i] = (boom_t){ 1, 0, x, y };
            return;
        }
}

static void add_score(int v)
{
    if (demo)
        return;
    pl.score += (uint32_t)v;
    if (pl.score >= pl.next_life) {
        pl.next_life += 20000;
        pl.lives++;
        sound_fx(FX_POWER);
    }
}

/* Una bala enemiga desde (x, y) hacia el jugador, a v/16 de píxel cada 1/50
 * de segundo, girada da pasos de 1/64 de vuelta. Las cuentas, en esa unidad
 * (con sus redondeos, los de siempre), y luego a FX por frame con V(). */
static void fire_at(int x, int y, int v, int da)
{
    int dx = (pl.x - x) / FX, dy = (pl.y - y) / FX + 4;
    int len = isqrt(dx * dx + dy * dy);
    if (len < 1)
        len = 1;
    int vx = v * dx / len, vy = v * dy / len;
    if (da) {                           /* girar: (vx, vy) por el ángulo da */
        int c = icos(da), s = isin(da);
        int rx = (vx * c - vy * s) / 64, ry = (vx * s + vy * c) / 64;
        vx = rx;
        vy = ry;
    }
    for (int i = 0; i < MAX_BULLETS; i++)
        if (!bullets[i].alive) {
            bullets[i] = (shot_t){ 1, x, y, V(vx), V(vy) };
            return;
        }
}

/* Un anillo de n balas, a v como en fire_at(). */
static void ring(int x, int y, int n, int v, int phase)
{
    for (int k = 0; k < n; k++) {
        int a = phase + k * 64 / n;
        for (int i = 0; i < MAX_BULLETS; i++)
            if (!bullets[i].alive) {
                bullets[i] = (shot_t){ 1, x, y, V(v * icos(a) / 64), V(v * isin(a) / 64) };
                break;
            }
    }
}

static enemy_t *spawn(int type, int x, int y)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (!enemies[i].alive) {
            enemy_t *e = &enemies[i];
            static const int16_t hp[] = { 1, 2, 4, 1, 3, 2, 3 };
            *e = (enemy_t){ 0 };
            e->alive = 1;
            e->type = (uint8_t)type;
            e->hp = hp[type];
            e->x = x * FX;
            e->y = y * FX;
            e->base = y * FX;
            e->t = (int)(rnd() & 63) * ZXV_FRAME_HZ / 50;          /* hasta 1,26 s */
            switch (type) {
            case E_DART: e->vx = V(-28); e->amp = 40; e->spr = &s_dart; break;
            case E_WHEEL: e->vx = V(-20); e->amp = 96; e->spr = &s_wheel[0]; break;
            case E_ROCK: e->vx = V(-12 - (int)(rnd() % 10)); e->vy = V((int)(rnd() % 9) - 4); e->spr = &s_rock; break;
            case E_PEBBLE: e->vx = V(-24 - (int)(rnd() % 12)); e->vy = V((int)(rnd() % 13) - 6); e->spr = &s_pebble; break;
            case E_TURRET: e->vx = 0; e->spr = &s_turret; break;  /* va con el terreno */
            case E_MINE: e->vx = V(-10); e->spr = &s_mine; break;
            default: e->vx = V(-16); e->spr = &s_carrier; break;
            }
            return e;
        }
    return 0;
}

static void spawn_wave(const wave_t *w)
{
    for (int k = 0; k < w->n; k++) {
        int x = W + 8, y = w->y;
        switch (w->pattern) {
        case P_LINE:
            x += k * 24;
            break;
        case P_COLUMN:
            y = w->y - (w->n - 1) * 10 + k * 20;
            x += (k & 1) * 6;
            break;
        case P_RANDOM:
            x += k * 30;
            y = HUD_H + 16 + (int)(rnd() % (H - HUD_H - 40));
            break;
        default: {                          /* torretas sobre el suelo o en el techo */
            int col = (scroll + W + 8 + k * PX(40)) / 8;  /* separadas como en 4/5 s */
            if (col >= MAP_COLS)
                col = MAP_COLS - 1;
            x = W + 8 + k * PX(40);
            if (w->pattern == P_GROUND)
                y = ground_row[col] * 8 - 7;
            else
                y = ceil_rows[col] * 8 - 1;
            break;
        }
        }
        /* Que no aparezcan dentro de las paredes. */
        int top = (band_top + 1) * 8, bottom = band_bottom * 8 - 16;
        if (w->pattern != P_GROUND && w->pattern != P_CEIL && stages[stage].terrain) {
            if (y < top)
                y = top;
            if (y > bottom)
                y = bottom;
        }
        enemy_t *e = spawn(w->type, x, y);
        if (e && w->pattern == P_CEIL)
            e->flip = ZXV_FLIP_V;
    }
}

static void kill_enemy(enemy_t *e)
{
    static const int16_t points[] = { 100, 150, 200, 50, 250, 150, 300 };
    e->alive = 0;
    add_score(points[e->type]);
    add_boom(e->x / FX + e->spr->w / 2 - 8, e->y / FX + e->spr->h / 2 - 8);
    sound_fx(FX_BOOM);
    if (e->type == E_ROCK)
        for (int k = 0; k < 2; k++) {
            enemy_t *p = spawn(E_PEBBLE, e->x / FX + 6, e->y / FX + 6);
            if (p)
                p->vy = V(k ? 14 : -14);
        }
    if (e->type == E_CARRIER)
        for (int i = 0; i < MAX_POWERS; i++)
            if (!powers[i].alive) {
                powers[i] = (power_t){ 1, (uint8_t)(rnd() % 3 == 0), e->x, e->y, 0 };
                break;
            }
}

/* --- La nave ------------------------------------------------------------------- */

static void player_hit(void)
{
#ifdef ESTELA_BOT
    if (pl.invuln || pl.dead || pl.lives <= 0)
        return;
    bot_hits++;
    if (!pl.shield) {
        bot_deaths++;
        bot_cause[bot_now]++;
        bot_boss += boss.alive != 0;
        if (pl.lives == 1)              /* sin quedarse sin naves */
            pl.lives++;
    }
#else
    if (pl.invuln || pl.dead || pl.lives <= 0 || demo)
        return;
#endif
    if (pl.shield) {
        pl.shield = 0;
        pl.invuln = MS(1800);
        sound_fx(FX_HIT);
        return;
    }
    pl.dead = MS(2000);
    pl.lives--;
    add_boom(pl.x / FX, pl.y / FX - 3);
    add_boom(pl.x / FX - 4, pl.y / FX + 2);
    sound_fx(FX_DEATH);
}

/* El piloto de la demostración: hacia el enemigo más cercano, lejos de las
 * balas y de las paredes, disparando siempre. */
static int autopilot(void)
{
    int px = pl.x / FX, py = pl.y / FX, target = 96, best = 1 << 30;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const enemy_t *e = &enemies[i];
        int ex = e->x / FX;
        if (e->alive && ex > px && ex < W && ex - px < best) {
            best = ex - px;
            target = e->y / FX + e->spr->h / 2 - 5;
        }
    }
    if (boss.alive && !boss.dying)
        target = boss.y / FX + boss.spr->h / 2 - 5 + isin_f(frame * 50, ZXV_FRAME_HZ) / 3;
    for (int i = 0; i < MAX_POWERS; i++)        /* las mejoras, antes que nada */
        if (powers[i].alive && powers[i].x / FX > px - 4)
            target = powers[i].y / FX;
    for (int i = 0; i < MAX_BULLETS; i++) {
        const shot_t *b = &bullets[i];
        int bx = b->x / FX - px, by = b->y / FX - py - 5;
        if (b->alive && bx > -8 && bx < 40 && by > -14 && by < 14)
            target = py + (by > 0 ? -24 : 24);
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {     /* y de los que se le echan encima */
        const enemy_t *e = &enemies[i];
        int ex = e->x / FX - px, ey = e->y / FX + e->spr->h / 2 - py - 5;
        if (e->alive && ex > -e->spr->w && ex < 28 && ey > -e->spr->h / 2 - 8 && ey < e->spr->h / 2 + 8)
            target = py + (ey > 0 ? -24 : 24);
    }
    /* Las paredes: las de la columna de delante y las de la que aún tiene
     * encima o debajo (si no, va hacia un hueco antes de dejar atrás la pared). */
    int c0 = (scroll + px + 4) / 8 % MAP_COLS, c1 = (scroll + px + 8) / 8 % MAP_COLS;
    int lo = (ceil_rows[c0] > ceil_rows[c1] ? ceil_rows[c0] : ceil_rows[c1]) * 8 + 6;
    int hi = (ground_row[c0] < ground_row[c1] ? ground_row[c0] : ground_row[c1]) * 8 - 16;
    if (target < lo)
        target = lo;
    if (target > hi)
        target = hi;
    int in = IN_FIRE;
    if (py < target - 2)
        in |= IN_DOWN;
    if (py > target + 2)
        in |= IN_UP;
    if (px < 36)
        in |= IN_RIGHT;
    if (px > 52)
        in |= IN_LEFT;
    return in;
}

static void player_frame(void)
{
    if (pl.dead) {
        if (--pl.dead == 0 && pl.lives > 0) {
            pl.x = 24 * FX;
            pl.y = 96 * FX;
            pl.invuln = MS(3000);
        }
        return;
    }
    if (pl.lives <= 0)                  /* fin de la partida */
        return;
    int in = demo ? autopilot() : input_now;
    int v = V(32);                      /* 100 píxeles por segundo */
    if (in & IN_UP) pl.y -= v;
    if (in & IN_DOWN) pl.y += v;
    if (in & IN_LEFT) pl.x -= v;
    if (in & IN_RIGHT) pl.x += v;
    if (pl.x < 0) pl.x = 0;
    if (pl.x > (W - 16) * FX) pl.x = (W - 16) * FX;
    if (pl.y < (HUD_H + 1) * FX) pl.y = (HUD_H + 1) * FX;
    if (pl.y > (H - 10) * FX) pl.y = (H - 10) * FX;
    if (pl.invuln)
        pl.invuln--;

    if (pl.cooldown > 0)
        pl.cooldown -= 1000;
    if ((in & IN_FIRE) && pl.cooldown <= 0) {
        static const int8_t pattern[3][3][2] = {
            { { 0, 0 }, { 0, 0 }, { 0, 0 } }, { { -3, 0 }, { 3, 0 }, { 0, 0 } }, { { 0, -16 }, { 0, 0 }, { 0, 16 } },
        };
        int n = pl.weapon == 1 ? 1 : pl.weapon == 2 ? 2 : 3;
        for (int k = 0; k < n; k++)
            for (int i = 0; i < MAX_SHOTS; i++)
                if (!shots[i].alive) {
                    const int8_t *p = pattern[pl.weapon - 1][k];
                    shots[i] = (shot_t){ 1, pl.x + 14 * FX, pl.y + (4 + p[0]) * FX, V(80), V(p[1]) };
                    break;
                }
        pl.cooldown += 140 * ZXV_FRAME_HZ;  /* uno cada 140 ms, aunque no sean frames enteros */
        sound_fx(FX_SHOT);
    }

    /* Las paredes. */
    int px = pl.x / FX, py = pl.y / FX;
    if (solid(px + 15, py + 5) || solid(px + 4, py) || solid(px + 4, py + 9) || solid(px, py + 5))
        HIT(0);
}

/* --- El jefe ------------------------------------------------------------------- */

static void boss_start(void)
{
    static const int hp[3] = { 40, 55, 70 };
    boss = (typeof(boss)){ 0 };
    boss.alive = 1;
    boss.type = stage;
    boss.spr = &s_boss[stage];
    boss.x = (W + 8) * FX;
    boss.y = (H / 2 + HUD_H / 2 - boss.spr->h / 2) * FX;
    boss.hp = boss.maxhp = hp[stage];
    sound_song(SONG_BOSS);
}

/* Lo que le da al jefe: entero, o solo el núcleo abierto. */
static int boss_hit_box(int x, int y)
{
    int bx = boss.x / FX, by = boss.y / FX;
    if (boss.type == 2)
        return boss.open && x >= bx + 15 && x < bx + 40 && y >= by + 15 && y < by + 40;
    return x >= bx + 2 && x < bx + boss.spr->w - 2 && y >= by + 2 && y < by + boss.spr->h - 2;
}

static void boss_frame(void)
{
    if (!boss.alive)
        return;
    int bx = boss.x / FX, by = boss.y / FX, cx = bx + boss.spr->w / 2, cy = by + boss.spr->h / 2;
    if (boss.dying) {
        if (boss.dying % (ZXV_FRAME_HZ / 12) == 0)  /* unas 12 por segundo */
            add_boom(bx + (int)(rnd() % boss.spr->w) - 8, by + (int)(rnd() % boss.spr->h) - 8);
        if (--boss.dying == 0)
            boss.alive = 0;
        return;
    }
    boss.t++;
    if (boss.flash)
        boss.flash--;
    int home = (W - boss.spr->w - 12) * FX;
    if (boss.x > home) {
        boss.x -= V(16);
        return;                         /* llega */
    }
    int mid = (H + HUD_H) / 2 - boss.spr->h / 2;
    int amp = stage == 1 ? 50 : 36;
    /* A saltos de un paso (de 64 por vuelta) cada 3/50 de segundo, como
     * siempre: con un vaivén suave, al núcleo se le acierta más (con el piloto
     * automático, dura un 15 % menos). */
    boss.y = (mid + isin(boss.t * 50 / (3 * ZXV_FRAME_HZ)) * amp / 64) * FX;
    int t = boss.t;
    switch (boss.type) {
    case 0:                             /* la roca madre: anillos y piedras */
        if (t % MS(2000) == 0)
            ring(cx * FX, cy * FX, 10, 22, t * 50 / (7 * ZXV_FRAME_HZ));
        if (t % MS(1000) == MS(500)) {
            enemy_t *e = spawn(E_PEBBLE, bx - 4, cy - 6);
            if (e) {
                e->vx = V(-30);
                e->vy = V(((pl.y / FX) - cy) * 30 / 120);
            }
        }
        break;
    case 1:                             /* el guardián: ráfagas y dardos */
        if (t % MS(1200) == 0)
            for (int k = -1; k <= 1; k++)
                fire_at(bx * FX, (by + 18) * FX, 30, k * 3);
        if (t % MS(3000) == MS(1500))
            for (int k = 0; k < 3; k++)
                spawn(E_DART, bx, by + 6 + k * 12);
        break;
    default:                            /* el núcleo: se abre y se cierra */
        boss.open = (t / MS(600)) % 7 < 4;
        if (!boss.open && t % MS(900) == 0)
            ring(cx * FX, cy * FX, 12, 20, t * 10 / ZXV_FRAME_HZ);
        if (boss.open && t % MS(800) == 0)
            for (int k = -1; k <= 1; k++)
                fire_at((bx + 2) * FX, (by + 16) * FX, 28, k * 3);
        if (t % MS(4000) == MS(2000))
            spawn(E_MINE, bx - 8, cy - 7);
        break;
    }
}

/* --- La partida ------------------------------------------------------------------- */

enum { R_CLEAR, R_OVER, R_QUIT };

static void stage_start(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
        enemies[i].alive = 0;
    for (int i = 0; i < MAX_SHOTS; i++)
        shots[i].alive = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
        bullets[i].alive = 0;
    for (int i = 0; i < MAX_BOOMS; i++)
        booms[i].alive = 0;
    for (int i = 0; i < MAX_POWERS; i++)
        powers[i].alive = 0;
    boss.alive = 0;
    scroll = 0;
    wave_i = 0;
    speed = FX;                         /* las estrellas cercanas, con el terreno */
    make_terrain(stages[stage].terrain, PX(stages[stage].length));
#ifndef ESTELA_STATS
    if (demo)                           /* la demostración: el final de la fase y el jefe */
        scroll = PX(stages[stage].length - 900);
#endif
    while (wave_i < stages[stage].nwaves && PX(stages[stage].waves[wave_i].at) <= scroll)
        wave_i++;
    pl.x = 24 * FX;
    pl.y = 96 * FX;
    pl.invuln = MS(1200);
    pl.dead = 0;
    message_t = MS(3000);
    static char title[24];
    char *p = title;
    const char *f = T("FASE ", "STAGE ");
    while (*f)
        *p++ = *f++;
    *p++ = (char)('1' + stage);
    *p = 0;
    message = title;
    message2 = T(stages[stage].name_es, stages[stage].name_en);
    sound_song(SONG_STAGE);
}

static void update(void)
{
    frame++;
    if (message_t)
        message_t--;

    /* El desplazamiento, un píxel por frame, y las oleadas; al final, el jefe. */
    const stage_t *st = &stages[stage];
    int moving = !boss.alive && scroll < PX(st->length);
    if (moving) {
        scroll++;
        while (wave_i < st->nwaves && PX(st->waves[wave_i].at) <= scroll)
            spawn_wave(&st->waves[wave_i++]);
        if (scroll == PX(st->length)) {
            boss_start();
            message = T("¡PELIGRO!", "WARNING!");
            message2 = 0;
            message_t = MS(2000);
            sound_fx(FX_ALARM);
            speed = FX / 2;
        }
    }

    player_frame();
    boss_frame();

    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemy_t *e = &enemies[i];
        if (!e->alive)
            continue;
        e->t++;
        if (e->flash)
            e->flash--;
        switch (e->type) {
        case E_DART:                    /* el vaivén, a 100 pasos (de 64 por vuelta) por segundo */
            e->x += e->vx;
            e->y = e->base + isin_f(e->t * 100, ZXV_FRAME_HZ) * e->amp * FX / 64;
            break;
        case E_WHEEL:                   /* a 50; la rueda cambia de dibujo 12,5 veces por segundo */
            e->x += e->vx;
            e->y = e->base + isin_f(e->t * 50, ZXV_FRAME_HZ) * e->amp * FX / 64;
            e->spr = &s_wheel[(e->t * 25 / (2 * ZXV_FRAME_HZ)) & 1];
            if (e->t % MS(1800) == MS(900) && e->x < W * FX)
                fire_at(e->x + 6 * FX, e->y + 6 * FX, 20, 0);
            break;
        case E_TURRET:                  /* pegada al terreno */
            e->x -= moving ? FX : 0;
            if (e->t % MS(2000) == MS(1000) && e->x < (W - 8) * FX && e->x > 0)
                fire_at(e->x + 8 * FX, e->y + (e->flip ? 7 : 0) * FX, 22, 0);
            break;
        case E_MINE:
            e->x += e->vx + (pl.x > e->x ? V(4) : V(-4));
            e->y += pl.y > e->y ? V(6) : V(-6);
            break;
        default:                        /* rocas y portador: en línea recta */
            e->x += e->vx;
            e->y += e->vy;
            break;
        }
        int ex = e->x / FX, ey = e->y / FX;
        if (ex < -32 || ex > W + 300 || ey < -32 || ey > H + 32)
            e->alive = 0;
        else if ((e->type == E_ROCK || e->type == E_PEBBLE) && (ey < HUD_H - 4 || ey > H - e->spr->h + 4))
            e->vy = -e->vy;             /* las rocas rebotan en los bordes */
    }

    for (int i = 0; i < MAX_SHOTS; i++) {
        shot_t *s = &shots[i];
        if (!s->alive)
            continue;
        s->x += s->vx;
        s->y += s->vy;
        int sx = s->x / FX, sy = s->y / FX;
        if (sx > W || sy < HUD_H || sy > H || solid(sx + 5, sy + 1)) {
            s->alive = 0;
            continue;
        }
        for (int k = 0; k < MAX_ENEMIES; k++) {
            enemy_t *e = &enemies[k];
            int ex = e->x / FX, ey = e->y / FX;
            if (e->alive && sx + 6 > ex && sx < ex + e->spr->w && sy + 2 > ey && sy < ey + e->spr->h) {
                s->alive = 0;
                e->flash = (uint8_t)MS(60);
                if (--e->hp <= 0)
                    kill_enemy(e);
                else
                    sound_fx(FX_HIT);
                break;
            }
        }
        if (s->alive && boss.alive && !boss.dying && boss_hit_box(sx + 5, sy + 1)) {
            s->alive = 0;
            boss.flash = MS(40);
            add_score(10);
            sound_fx(FX_HIT);
            if (--boss.hp <= 0) {
                boss.dying = MS(1800);
                add_score(5000 * (stage + 1));
                sound_fx(FX_BIG);
                for (int k = 0; k < MAX_BULLETS; k++)
                    bullets[k].alive = 0;
            }
        } else if (s->alive && boss.alive && !boss.dying && boss.type == 2) {
            /* El blindaje del núcleo: todo él si está cerrado; abierto, deja
             * pasar lo que va a la altura del centro. */
            int bx = boss.x / FX, by = boss.y / FX, y = sy + 1;
            int in_core_band = y >= by + 15 && y < by + 40;
            if (sx + 6 > bx && sx < bx + boss.spr->w && y > by + 4 && y < by + boss.spr->h - 4 &&
                (!boss.open || !in_core_band))
                s->alive = 0;
        }
    }

    int px = pl.x / FX, py = pl.y / FX;
    for (int i = 0; i < MAX_BULLETS; i++) {
        shot_t *b = &bullets[i];
        if (!b->alive)
            continue;
        b->x += b->vx;
        b->y += b->vy;
        int bx = b->x / FX, by = b->y / FX;
        if (bx < -6 || bx > W || by < HUD_H - 4 || by > H || solid(bx + 2, by + 2))
            b->alive = 0;
        else if (!pl.dead && pl.lives > 0 && bx + 3 > px + 5 && bx + 1 < px + 12 && by + 3 > py + 4 &&
                 by + 1 < py + 7) {
            b->alive = 0;
            HIT(1);
        }
    }

    if (!pl.dead && pl.lives > 0) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            enemy_t *e = &enemies[i];
            if (e->alive && sprites_touch(&s_ship, px, py, e->spr, e->x / FX, e->y / FX)) {
                HIT(2);
#ifndef ESTELA_BOT
                if (!demo)
#endif
                    kill_enemy(e);
            }
        }
        if (boss.alive && !boss.dying && sprites_touch(&s_ship, px, py, boss.spr, boss.x / FX, boss.y / FX))
            HIT(3);
        for (int i = 0; i < MAX_POWERS; i++) {
            power_t *p = &powers[i];
            if (p->alive && sprites_touch(&s_ship, px, py, &s_power[p->kind], p->x / FX, p->y / FX)) {
                p->alive = 0;
                if (p->kind)
                    pl.shield = 1;
                else if (pl.weapon < 3)
                    pl.weapon++;
                add_score(500);
                sound_fx(FX_POWER);
            }
        }
    }

    for (int i = 0; i < MAX_POWERS; i++) {
        power_t *p = &powers[i];
        if (p->alive) {
            p->t++;
            p->x -= V(10);
            p->y += V(isin_f(p->t * 50, ZXV_FRAME_HZ)) / 8;
            if (p->x < -16 * FX)
                p->alive = 0;
        }
    }
    for (int i = 0; i < MAX_BOOMS; i++)                 /* las explosiones duran 0,32 s */
        if (booms[i].alive && ++booms[i].t >= MS(320))
            booms[i].alive = 0;
}

/* --- El dibujo ------------------------------------------------------------------- */

static void draw_hud(void)
{
    char buf[16];
    zxv_gfx_rect(0, 0, W, HUD_H, 0);
    number(buf, demo ? 0 : pl.score, 6);
    text(2, 0, buf, 15);
    for (int i = 0; i < pl.lives && i < 6; i++) {           /* las naves que quedan */
        zxv_gfx_rect(48 + i * 9, 3, 5, 2, 7);
        zxv_gfx_rect(48 + i * 9 + 5, 3, 2, 1, 5);
    }
    for (int i = 0; i < pl.weapon; i++)                     /* el arma */
        zxv_gfx_rect(106 + i * 4, 2, 2, 4, 14);
    if (pl.shield)
        zxv_gfx_rect(120, 2, 4, 4, 13);
#ifdef ESTELA_STATS
    number(buf, video_frames(), 4);     /* los frames perdidos */
    text(126, 0, buf, 2);
#endif
    if (boss.alive && !boss.dying && boss.x <= (W - boss.spr->w - 12) * FX) {
        text(140, 0, T("JEFE", "BOSS"), 10);
        zxv_gfx_rect(168, 2, 84, 4, 1);
        zxv_gfx_rect(168, 2, 84 * boss.hp / boss.maxhp, 4, 10);
    } else {
        number(buf, record > pl.score || demo ? record : pl.score, 6);
        text(W - 38, 0, buf, 6);
        text(W - 38 - 6 * 4, 0, T("REC", "HI"), 6);
    }
}

static void draw_world(void)
{
    /* El fondo: un planeta lejano en la primera fase, las estrellas siempre. */
    if (stage == 0) {
        int x = 200 - scroll / 6;
        zxv_gfx_circle(x, 140, 40, 1, 1);
        zxv_gfx_circle(x - 6, 134, 34, 9, 1);
        zxv_gfx_circle(x - 14, 126, 12, 13, 1);
        zxv_gfx_circle(x + 70, 92, 6, 5, 1);
        zxv_gfx_circle(x + 68, 90, 3, 13, 1);
    }
    stars_draw(speed);

    /* Las paredes: solo las franjas de arriba y de abajo. */
    if (stages[stage].terrain) {
        zxv_map_draw(&terrain, scroll, 0, 0, 0, W, band_top * 8, ZXV_MAP_ALL);
        zxv_map_draw(&terrain, scroll, band_bottom * 8, 0, band_bottom * 8, W, H - band_bottom * 8, ZXV_MAP_ALL);
    }

    for (int i = 0; i < MAX_POWERS; i++)
        if (powers[i].alive)
            draw_sprite(&s_power[powers[i].kind], powers[i].x / FX, powers[i].y / FX, 0, 0);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const enemy_t *e = &enemies[i];
        if (e->alive)
            draw_sprite(e->spr, e->x / FX, e->y / FX, e->flip, e->flash ? c_white : 0);
    }
    if (boss.alive && (!boss.dying || (boss.dying & 4)))
        draw_sprite(boss.spr, boss.x / FX, boss.y / FX, 0,
                    boss.flash ? c_white : boss.type == 2 && !boss.open ? c_closed : 0);
    for (int i = 0; i < MAX_SHOTS; i++)
        if (shots[i].alive) {
            zxv_gfx_rect(shots[i].x / FX, shots[i].y / FX, 5, 2, 14);
            zxv_gfx_rect(shots[i].x / FX + 5, shots[i].y / FX, 1, 2, 15);
        }
    if (!pl.dead && pl.lives > 0 && (!pl.invuln || (frame & 4))) {
        draw_sprite(&s_ship, pl.x / FX, pl.y / FX, 0, 0);
        if (pl.shield)
            draw_sprite(&s_shield, pl.x / FX - 4, pl.y / FX - 4, frame & 8 ? ZXV_FLIP_H : 0, 0);
    }
    for (int i = 0; i < MAX_BULLETS; i++)
        if (bullets[i].alive)
            draw_sprite(&s_ebullet, bullets[i].x / FX, bullets[i].y / FX, 0, 0);
    for (int i = 0; i < MAX_BOOMS; i++)
        if (booms[i].alive)
            draw_sprite(&s_boom[booms[i].t * 4 / MS(320)], booms[i].x, booms[i].y, 0, 0);

    draw_hud();
    if (message_t && message) {
        label(80, message, (frame & 8) ? 15 : 14);
        if (message2)
            label(92, message2, 13);
    }
    if (demo && (frame & 32))
        label(H - 12, T("DEMOSTRACIÓN", "DEMO"), 13);
}

/* --- El récord -------------------------------------------------------------------- */

#define RECORD_FILE "SD:estela.txt"

static void load_record(void)
{
    char buf[12];
    int f = zxv_file_open(RECORD_FILE, ZXV_O_READ);
    if (f < 0)
        return;
    int n = zxv_file_read(f, buf, sizeof(buf) - 1);
    zxv_file_close(f);
    record = 0;
    for (int i = 0; i < n && buf[i] >= '0' && buf[i] <= '9'; i++)
        record = record * 10 + (uint32_t)(buf[i] - '0');
}

static void save_record(void)
{
    char buf[12];
    number(buf, record, 8);
    buf[8] = '\n';
    int f = zxv_file_open(RECORD_FILE, ZXV_O_WRITE | ZXV_O_CREATE | ZXV_O_TRUNC);
    if (f < 0)
        return;                         /* sin tarjeta: el récord dura lo que la partida */
    zxv_file_write(f, buf, 9);
    zxv_file_close(f);
}

/* --- Las pantallas ---------------------------------------------------------------- */

/* Unos frames con un letrero (fin de partida, fase superada...). Devuelve 1 si se pulsó Esc. */
static int banner(const char *a, const char *b, int frames)
{
    for (int t = 0; t < frames; t++) {
        read_input();
        video_begin();
        draw_world();
        label(80, a, (t & 8) ? 15 : 14);
        if (b)
            label(92, b, 13);
        sound_frame();
        video_end();
        update();                       /* las explosiones y las balas siguen */
        if (t > ZXV_FRAME_HZ && (input_edge & (IN_FIRE | IN_ESC)))
            return (input_edge & IN_ESC) != 0;
    }
    return 0;
}

static int run_stage(void)
{
    stage_start();
    for (;;) {
        read_input();
        if (demo && (input_edge & (IN_FIRE | IN_ESC | IN_PAUSE)))
            return R_QUIT;
#ifndef ESTELA_STATS
        if (demo && frame > 32 * ZXV_FRAME_HZ)
            return R_QUIT;
#endif
        if (input_edge & IN_ESC)
            return R_QUIT;
        if (input_edge & IN_PAUSE) {    /* la pausa: hasta otra vez Intro (Esc sale) */
            sound_mute(1);
            for (;;) {
                read_input();
                video_begin();
                draw_world();
                label(84, T("PAUSA", "PAUSED"), (frame & 16) ? 15 : 5);
                video_end();
                frame++;
                if (input_edge & (IN_PAUSE | IN_ESC))
                    break;
            }
            sound_mute(0);
            if (input_edge & IN_ESC)
                return R_QUIT;
        }
        update();
        video_begin();
        draw_world();
        sound_frame();
        video_end();
#ifdef ESTELA_BOT
        bot_boss_f += boss.alive != 0;
        if (frame > 1200 * ZXV_FRAME_HZ)    /* el jefe, imposible: se da por perdida */
            return R_OVER;
#endif
        if (pl.lives <= 0 && !pl.dead)
            return R_OVER;
        if (boss.alive == 0 && boss.dying == 0 && scroll >= PX(stages[stage].length) && boss.hp <= 0 && boss.maxhp)
            return R_CLEAR;
    }
}

static void play(int demo_mode)
{
    static int demos;
    demo = demo_mode;
#ifdef ESTELA_BOT
    rng_state = ESTELA_BOT;
#else
    rng_state = demo ? 4242 : (rng_state ^ *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES);
#endif
    pl = (typeof(pl)){ 0 };
    pl.lives = 3;
    pl.weapon = 1;
    pl.next_life = 20000;
    frame = 0;
    for (stage = demo ? demos++ % 3 : 0; stage < 3; stage++) {
        boss.maxhp = 0;
#ifdef ESTELA_BOT
        int f0 = frame;
        bot_deaths = bot_hits = 0;
#endif
        int r = run_stage();
#ifdef ESTELA_BOT
        bot_write("fase ");
        bot_number(stage + 1);
        bot_write(r == R_CLEAR ? ": superada" : ": SIN TERMINAR");
        bot_write(", vidas perdidas ");
        bot_number(bot_deaths);
        bot_write(", golpes ");
        bot_number(bot_hits);
        bot_write(", segundos ");
        bot_number((frame - f0) / ZXV_FRAME_HZ);
        bot_write(" [paredes ");
        bot_number(bot_cause[0]);
        bot_write(", balas ");
        bot_number(bot_cause[1]);
        bot_write(", enemigos ");
        bot_number(bot_cause[2]);
        bot_write(", jefe ");
        bot_number(bot_cause[3]);
        bot_write("] con el jefe ");
        bot_number(bot_boss);
        bot_write(" en ");
        bot_number(bot_boss_f / ZXV_FRAME_HZ);
        bot_write(" s");
        bot_boss = bot_boss_f = 0;
        bot_cause[0] = bot_cause[1] = bot_cause[2] = bot_cause[3] = 0;
        bot_write(", arma ");
        bot_number(pl.weapon);
        bot_write("\n");
        if (r != R_CLEAR)
            bot_exit(1);
        continue;
#endif
        if (r == R_QUIT)
            return;
#ifndef ESTELA_STATS
        if (demo)                       /* cada demostración, una fase */
            return;
#endif
        if (r == R_OVER) {
            sound_song(SONG_OVER);
            int better = pl.score > record;
            if (better) {
                record = pl.score;
                save_record();
            }
            banner(T("FIN DE LA PARTIDA", "GAME OVER"), better ? T("¡NUEVO RÉCORD!", "NEW HIGH SCORE!") : 0,
                   6 * ZXV_FRAME_HZ);
            return;
        }
        sound_song(SONG_CLEAR);
        if (stage < 2 && banner(T("¡FASE SUPERADA!", "STAGE CLEAR!"), 0, MS(3600)))
            return;
    }
    /* Las tres fases: la bonificación por las naves que quedan. */
    add_score(pl.lives * 5000);
    int better = pl.score > record;
    if (better) {
        record = pl.score;
        save_record();
    }
    banner(T("¡MISIÓN CUMPLIDA!", "MISSION COMPLETE!"), better ? T("¡NUEVO RÉCORD!", "NEW HIGH SCORE!") : 0,
           8 * ZXV_FRAME_HZ);
}

/* El título: 0 para jugar, 1 para la demostración, -1 para salir. */
static int title(void)
{
    static const uint8_t logo[7] = { 15, 14, 14, 6, 10, 2, 2 };
    sound_song(SONG_TITLE);
    for (int t = 0;; t++) {
        read_input();
        if (input_edge & IN_ESC)
            return -1;
        if (t > MS(400) && (input_edge & IN_FIRE))
            return 0;
        if (t > 12 * ZXV_FRAME_HZ)
            return 1;
        video_begin();
        stars_draw(V(24));
        int y = 26 + isin_f(t * 50, ZXV_FRAME_HZ) / 16;
        text_big((W - 6 * 6 * 4 + 4) / 2, y, "ESTELA", 4, logo);
        draw_sprite(&s_ship, t * 100 / ZXV_FRAME_HZ % (W + 64) - 32,       /* a 100 píxeles por segundo */
                    74 + isin_f(t * 100, ZXV_FRAME_HZ) / 6, 0, 0);
        text_center(94, T("UN MATAMARCIANOS PARA ZX-V", "A SHOOT 'EM UP FOR ZX-V"), 5);
        char buf[16], line[32];
        number(buf, record, 6);
        const char *label = T("RÉCORD ", "HIGH SCORE ");
        int n = 0;
        while (*label)
            line[n++] = *label++;
        for (int i = 0; buf[i]; i++)
            line[n++] = buf[i];
        line[n] = 0;
        text_center(110, line, 14);
        text_center(130, T("CURSORES O OPQA: MOVER", "ARROWS OR OPQA: MOVE"), 7);
        text_center(140, T("ESPACIO O M: DISPARAR", "SPACE OR M: FIRE"), 7);
        text_center(150, T("INTRO: PAUSA   ESC: SALIR", "ENTER: PAUSE   ESC: QUIT"), 7);
        if (t & 32)
            text_center(172, T("PULSA ESPACIO O FUEGO", "PRESS SPACE OR FIRE"), 15);
        sound_frame();
        video_end();
    }
}

int main(void)
{
    lang = zxv_lang_get() == ZXV_LANG_ES ? 0 : 1;
    video_init();
    sound_init();
    make_sprites();
    make_tiles();
    stars_init();
    load_record();
    input_now = 0x7F;                   /* lo que esté pulsado al empezar no cuenta */
#ifdef ESTELA_BOT
    play(1);
    bot_exit(0);
#endif
    for (;;) {
        int r = title();
        if (r < 0)
            break;
        play(r);
    }
    sound_off();
    video_done();
    zxv_set_mode(0);
    while (zxv_key_get() >= 0)
        ;
    return 0;                                   /* el récord ya está en la tarjeta */
}
