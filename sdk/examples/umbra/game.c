/*
 * game.c - Umbra: un laberinto en 3D para ZX-V.
 *
 * Cada nivel es un laberinto a oscuras con unas runas escondidas: al
 * reunirlas todas se abre la puerta de salida. La única luz es la de tu
 * lámpara, que se va apagando: cuanto menos aceite le queda, menos lejos se
 * ve (la niebla de view.c llega antes) y más deprisa van las sombras que
 * vagan por el laberinto. Las sombras te huelen a unos pasos y te persiguen;
 * si te tocan, se beben parte del aceite. Los frascos lo rellenan. Si la
 * lámpara se apaga del todo, pierdes una de tus tres lámparas. Los niveles
 * son los mapas de umbra.gfx (maze.c).
 *
 * Controles: cursores o QAOP para andar y girar, Z y X para ir de lado, M
 * para el mapa de lo que ya has visto, Intro para la pausa, S para la música
 * y Esc para salir; o un mando (fuego 2 enseña el mapa). Sin tocar nada en
 * el título empieza una demostración que juega sola. El récord se guarda en
 * SD:umbra.txt (en la tarjeta, aunque el juego se cargue desde ROM:).
 *
 * Compilado con -DUMBRA_STATS (make EXTRA_CFLAGS=-DUMBRA_STATS), el marcador
 * enseña los frames perdidos y lo que tarda el frame más lento, y la
 * demostración juega todos los niveles sin perder aceite: para medir. Con
 * -DUMBRA_SCORE=n la partida empieza con n puntos: para ver el marcador con
 * los números más grandes.
 *
 * Dónde va cada cosa en la pantalla: cada texto tiene su sitio y nada se
 * dibuja encima de otra cosa.
 *
 *   - La vista en 3D (las líneas 0-143) y, encima, solo el cuadro del centro
 *     (box): una franja negra con hasta tres líneas (los avisos, la pausa, el
 *     fin de la partida) o el nombre del nivel en grande.
 *   - Las pantallas del mapa (M, nivel superado, la victoria) ocupan la
 *     vista: arriba, una cabecera de tres líneas, y debajo el mapa.
 *   - El marcador (144-191): el mapa pequeño a la izquierda y, a su derecha,
 *     tres líneas: el nivel y los puntos (y en medio, con UMBRA_STATS, la
 *     medida); la lámpara, el aceite y las de repuesto; las runas y un aviso
 *     (la demostración o la puerta abierta).
 */
#include "umbra.h"

uint32_t rng_state = 1;
int lang;

static int abs_i(int v) { return v < 0 ? -v : v; }

/* --- La entrada -------------------------------------------------------------------- */

enum { IN_UP = 1, IN_DOWN = 2, IN_LEFT = 4, IN_RIGHT = 8, IN_SLEFT = 16, IN_SRIGHT = 32,
       IN_START = 64, IN_PAUSE = 128, IN_ESC = 256, IN_MAP = 512, IN_MUSIC = 1024 };

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
    if (key(0x1D))
        in |= IN_SLEFT;                 /* Z */
    if (key(0x1B))
        in |= IN_SRIGHT;                /* X */
    if (key(0x2C))
        in |= IN_START;                 /* espacio */
    if (key(0x10))
        in |= IN_MAP;                   /* M */
    if (key(0x28))
        in |= IN_PAUSE;                 /* Intro */
    if (key(0x29))
        in |= IN_ESC;
    if (key(0x16))
        in |= IN_MUSIC;                 /* S */
    uint16_t joy = *(volatile uint16_t *)(uintptr_t)ZXV_I_JOY0;
    if (joy & ZXV_JOY_CONNECTED) {
        if (joy & ZXV_JOY_UP) in |= IN_UP;
        if (joy & ZXV_JOY_DOWN) in |= IN_DOWN;
        if (joy & ZXV_JOY_LEFT) in |= IN_LEFT;
        if (joy & ZXV_JOY_RIGHT) in |= IN_RIGHT;
        if (joy & ZXV_JOY_FIRE1) in |= IN_START;
        if (joy & ZXV_JOY_FIRE2) in |= IN_MAP;
        if (joy & ZXV_JOY_START) in |= IN_PAUSE;
    }
    input_edge = in & ~input_now;
    input_now = in;
    if (input_edge & IN_MUSIC)          /* la música, sí o no, en cualquier momento */
        sound_music(!sound_music_on());
    /* Lo tecleado no le sirve al juego: que no le llegue al BASIC al volver. */
    while (zxv_key_get() >= 0)
        ;
}

/* --- El jugador ---------------------------------------------------------------------- */

#define OIL_MAX     10000
#define OIL_BURN    (OIL_MAX / 80)      /* por segundo: una lámpara llena dura 80 s */
#define OIL_FLASK   4000
#define OIL_BITE    2500                /* lo que se bebe una sombra */
#define OIL_REVIVE  6000
#define WALK        (ONE * 12 / 5)      /* 2,4 celdas por segundo */
#define SIDESTEP    (ONE * 2)
#define TURN_SPEED  (TURN * 2 / 5)      /* 144 grados por segundo */
#define RADIUS      (ONE * 22 / 100)    /* lo que ocupa el jugador */
#define REACH       (ONE * 2 / 5)       /* lo cerca que hay que pasar de algo para cogerlo */

static struct {
    int32_t x, y;
    int ang;
    int turn_rest, walk_rest, side_rest, oil_rest;
    int oil;
    int lives;
    uint32_t score;
    int hurt;                           /* frames sin que las sombras hagan daño */
    int cx, cy;                         /* la celda (para dist[]) */
    int32_t walked;                     /* para los pasos */
} pl;

static uint32_t record, frame;
static int demo, banner_t, flash_t, border;
static const char *banner_a, *banner_b;
#ifdef UMBRA_STATS
static uint32_t stats_us, stats_max, stats_t0;
#endif

static int32_t light(void) { return ONE * 5 / 4 + (int32_t)((int64_t)pl.oil * (ONE * 19 / 4) / OIL_MAX); }

static void banner(const char *a, const char *b, int seconds)
{
    banner_a = a;
    banner_b = b;
    banner_t = seconds * ZXV_FRAME_HZ;
}

static int free_at(int32_t x, int32_t y)
{
    return !cell_solid((x - RADIUS) >> 16, (y - RADIUS) >> 16) &&
           !cell_solid((x + RADIUS) >> 16, (y - RADIUS) >> 16) &&
           !cell_solid((x - RADIUS) >> 16, (y + RADIUS) >> 16) &&
           !cell_solid((x + RADIUS) >> 16, (y + RADIUS) >> 16);
}

/* Anda (dx, dy), resbalando por las paredes. */
static void walk(int32_t dx, int32_t dy)
{
    if (free_at(pl.x + dx, pl.y))
        pl.x += dx;
    if (free_at(pl.x, pl.y + dy))
        pl.y += dy;
    pl.walked += abs_i(dx) + abs_i(dy);
    if (pl.walked > ONE * 2 / 5) {      /* un paso cada 0,4 celdas */
        pl.walked = 0;
        sound_fx(FX_STEP);
    }
}

static void place_player(void)
{
    pl.x = start_x * ONE + ONE / 2;
    pl.y = start_y * ONE + ONE / 2;
    pl.ang = start_ang;
    pl.cx = start_x;
    pl.cy = start_y;
    maze_distances(pl.cx, pl.cy);
}

/* --- La demostración: va a por lo que tiene más cerca ------------------------------- */

static uint8_t bot_map[MAZE_MAX * MAZE_MAX];
static int bot_target = -1, bot_tx, bot_ty;

static int autopilot(void)
{
    /* El objetivo: un frasco si queda poco aceite, si no una runa y, con todas,
     * la puerta. Lo más cerca (dist[], desde el jugador). */
    int best = -1, bd = 255, tx = gate_x, ty = gate_y;
    for (int i = 0; i < item_count; i++) {
        const item_t *it = &items[i];
        if (it->taken || (it->kind == ITEM_FLASK && pl.oil > OIL_MAX * 2 / 5))
            continue;
        int d = dist[(it->y >> 16) * MAZE_MAX + (it->x >> 16)];
        if (it->kind == ITEM_RUNE && pl.oil > OIL_MAX * 2 / 5)
            d = d * 2 / 3;              /* las runas, antes */
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    if (best >= 0) {
        tx = items[best].x >> 16;
        ty = items[best].y >> 16;
    } else if (!gate_open) {
        return 0;
    }
    int target = ty * MAZE_MAX + tx;
    if (target != bot_target || bot_tx != pl.cx || bot_ty != pl.cy) {
        bot_target = target;
        bot_tx = pl.cx;
        bot_ty = pl.cy;
        maze_flood(bot_map, tx, ty);
    }
    /* La celda siguiente del camino, y hacia su centro. */
    static const int8_t dx4[4] = { 1, 0, -1, 0 }, dy4[4] = { 0, 1, 0, -1 };
    int here = bot_map[pl.cy * MAZE_MAX + pl.cx], nx = pl.cx, ny = pl.cy;
    for (int d = 0; d < 4; d++) {
        int x = pl.cx + dx4[d], y = pl.cy + dy4[d];
        if ((unsigned)x < MAZE_MAX && (unsigned)y < MAZE_MAX && bot_map[y * MAZE_MAX + x] < here) {
            here = bot_map[y * MAZE_MAX + x];
            nx = x;
            ny = y;
        }
    }
    int32_t gx = nx * ONE + ONE / 2 - pl.x, gy = ny * ONE + ONE / 2 - pl.y;
    /* Si aún no está centrado en su celda, primero al centro de la suya. */
    if (nx != pl.cx && abs_i(pl.y - (pl.cy * ONE + ONE / 2)) > ONE / 8)
        gy = pl.cy * ONE + ONE / 2 - pl.y, gx = (nx - pl.cx) * (ONE / 8);
    if (ny != pl.cy && abs_i(pl.x - (pl.cx * ONE + ONE / 2)) > ONE / 8)
        gx = pl.cx * ONE + ONE / 2 - pl.x, gy = (ny - pl.cy) * (ONE / 8);
    int32_t ca = icos(pl.ang), sa = isin(pl.ang);
    int64_t ahead = ((int64_t)gx * ca + (int64_t)gy * sa) >> 16;
    int64_t side = ((int64_t)gy * ca - (int64_t)gx * sa) >> 16;
    int in = 0;
    if (side > ahead / 6 || (ahead <= 0 && side >= 0))
        in |= IN_RIGHT;
    else if (side < -ahead / 6 || ahead <= 0)
        in |= IN_LEFT;
    if (ahead > 0 && (side < 0 ? -side : side) < ahead / 2)
        in |= IN_UP;
    return in;
}

/* --- El marcador y el mapa ---------------------------------------------------------- */

/* El color de una celda en los mapas. */
static int cell_colour(int x, int y)
{
    int c = cell_at(x, y);
    if (c == CELL_FLOOR)
        return 1;
    if (c == CELL_GATE)
        return gate_open ? 13 : 10;
    return 7;
}

/* Las celdas de (x0, y0) en adelante, de s x s píxeles, en (px, py): solo lo
 * que se ha visto; n x m celdas. */
static void draw_map(int x0, int y0, int n, int m, int px, int py, int s)
{
    for (int j = 0; j < m; j++)
        for (int i = 0; i < n; i++) {
            int x = x0 + i, y = y0 + j;
            if ((unsigned)x >= (unsigned)maze_w || (unsigned)y >= (unsigned)maze_h || !seen[y * MAZE_MAX + x])
                continue;
            zxv_gfx_rect(px + i * s, py + j * s, s, s, cell_colour(x, y));
        }
    for (int k = 0; k < item_count; k++) {      /* lo que queda, si se ha visto */
        const item_t *it = &items[k];
        int x = (it->x >> 16) - x0, y = (it->y >> 16) - y0;
        if (it->taken || x < 0 || y < 0 || x >= n || y >= m || !seen[(it->y >> 16) * MAZE_MAX + (it->x >> 16)])
            continue;
        int d = s > 4 ? s / 2 : 2;
        zxv_gfx_rect(px + x * s + (s - d) / 2, py + y * s + (s - d) / 2, d, d, it->kind == ITEM_RUNE ? 11 : 14);
    }
    /* El jugador, y un píxel hacia donde mira. */
    int ex = (int)(((int64_t)(pl.x - x0 * ONE) * s) >> 16) + px;
    int ey = (int)(((int64_t)(pl.y - y0 * ONE) * s) >> 16) + py;
    zxv_gfx_rect(ex - 1, ey - 1, 3, 3, 14);
    zxv_gfx_rect(ex + (int)(icos(pl.ang) * 3 >> 16), ey + (int)(isin(pl.ang) * 3 >> 16), 1, 1, 15);
}

/* El marcador: el mapa pequeño (11 x 11 celdas de 4 píxeles) en su recuadro y
 * las tres líneas de su derecha. */
#define BAR_X       54                  /* donde empiezan las líneas del marcador */
#define LINE1_Y     148
#define LINE3_Y     181
#define NOTE_X      102                 /* el aviso de la tercera línea: hasta 25 letras */

static void hud(void)
{
    char buf[16];
    zxv_gfx_rect(0, VIEW_H, W, H - VIEW_H, 0);
    zxv_gfx_rect(0, VIEW_H, W, 1, 1);
    zxv_gfx_rect(3, VIEW_H + 2, 46, 46, 1);
    zxv_gfx_rect(4, VIEW_H + 3, 44, 44, 0);
    zxv_gfx_clip(4, VIEW_H + 3, 48, VIEW_H + 47);
    draw_map(pl.cx - 5, pl.cy - 5, 11, 11, 4, VIEW_H + 3, 4);
    zxv_gfx_clip(0, 0, W, H);

    /* Primera línea: el nivel a la izquierda y los puntos a la derecha. */
    text(BAR_X, LINE1_Y, T("NIVEL", "LEVEL"), 5);
    number(buf, (uint32_t)level + 1, 2);
    text(BAR_X + 36, LINE1_Y, buf, 15);
    number(buf, pl.score, 7);
    text(W - 3 - 6 * 7 + 1, LINE1_Y, buf, 15);
    const char *label = T("PUNTOS", "SCORE");
    text(W - 3 - 6 * 8 + 1 - text_width(label), LINE1_Y, label, 5);
#ifdef UMBRA_STATS
    /* En medio: los frames perdidos y el frame más lento del último segundo, en us. */
    number(buf, video_lost(), 4);
    text(106, LINE1_Y, buf, 10);
    number(buf, stats_us, 5);
    text(106 + 6 * 5, LINE1_Y, buf, 13);
#endif

    /* Segunda: la lámpara, el aceite que queda y las de repuesto. */
    draw_sprite(&art[SPR_LAMP], BAR_X, 157, 0, 0);
    int bar = pl.oil * 116 / OIL_MAX;
    int c = pl.oil > OIL_MAX / 2 ? 14 : pl.oil > OIL_MAX / 4 ? 6 : (frame & 16) ? 10 : 2;
    zxv_gfx_rect(74, 162, 120, 8, 1);
    zxv_gfx_rect(76, 164, 116, 4, 0);
    zxv_gfx_rect(76, 164, bar, 4, c);
    for (int i = 1; i < pl.lives && i < 5; i++)
        if (art[SPR_LAMP].spr)
            zxv_spr_draw_scaled(art[SPR_LAMP].spr, 200 + (i - 1) * 13, 160, 10, 10, 0, 0);

    /* Tercera: las runas (hasta 24) y el aviso. */
    draw_sprite(&art[SPR_RUNE_ICON], BAR_X, LINE3_Y - 2, 0, 0);
    int got = runes_total - runes_left, x = BAR_X + 12;
    number(buf, (uint32_t)got, got > 9 ? 2 : 1);
    text(x, LINE3_Y, buf, gate_open ? 13 : 15);
    x += text_width(buf) + 1;
    text(x, LINE3_Y, "/", 5);
    number(buf, (uint32_t)runes_total, runes_total > 9 ? 2 : 1);
    text(x + 6, LINE3_Y, buf, gate_open ? 13 : 15);
    if (demo)
        text(NOTE_X, LINE3_Y, T("DEMO: PULSA ESPACIO", "DEMO: PRESS SPACE"), (frame & 32) ? 13 : 5);
    else if (gate_open && runes_total)
        text(NOTE_X, LINE3_Y, T("LA PUERTA ESTÁ ABIERTA", "THE GATE IS OPEN"), (frame & 32) ? 13 : 5);
}

/* El cuadro del centro de la vista: una franja negra con hasta tres líneas
 * (las que no son NULL, de arriba abajo). La primera parpadea. */
#define BOX_Y       46

static void box(const char *a, const char *b, const char *c)
{
    const char *lines[3] = { a, b, c };
    int n = 0;
    for (int i = 0; i < 3; i++)
        n += lines[i] != 0;
    if (!n)
        return;
    zxv_gfx_rect(0, BOX_Y, W, 6 + 10 * n, 0);
    int y = BOX_Y + 4;
    for (int i = 0; i < 3; i++)
        if (lines[i]) {
            text_center(y, lines[i], i == 0 ? ((frame & 8) ? 15 : 14) : i == 1 ? 13 : 14);
            y += 10;
        }
}

/* El nombre del nivel, en grande, en el mismo sitio. */
static void box_big(const char *s)
{
    static const uint8_t col[7] = { 15, 15, 13, 13, 5, 5, 1 };
    zxv_gfx_rect(0, BOX_Y, W, 30, 0);
    text_big((W - text_width(s) * 3) / 2, BOX_Y + 4, s, 3, col);
}

/* Una pantalla del mapa: la cabecera de tres líneas (las que no son NULL) y
 * el laberinto entero debajo, con lo que se ha visto. */
#define MAP_Y       34

static void map_screen(const char *a, const char *b, const char *c)
{
    zxv_gfx_rect(0, 0, W, VIEW_H, 0);
    const char *lines[3] = { a, b, c };
    for (int i = 0; i < 3; i++)
        if (lines[i])
            text_center(4 + 10 * i, lines[i], i == 0 ? 13 : i == 1 ? 15 : 14);
    int s = W / maze_w < (VIEW_H - MAP_Y) / maze_h ? W / maze_w : (VIEW_H - MAP_Y) / maze_h;
    if (s > 8)
        s = 8;
    int x = (W - maze_w * s) / 2, y = MAP_Y + (VIEW_H - MAP_Y - maze_h * s) / 2;
    zxv_gfx_clip(x, y, x + maze_w * s, y + maze_h * s);
    draw_map(0, 0, maze_w, maze_h, x, y, s);
    zxv_gfx_clip(0, 0, W, H);
}

static void draw_world(void)
{
    cam_t cam = { pl.x, pl.y, pl.ang };
    view_draw(&cam, light(), (int)frame);
}

static void frame_end(void)
{
#ifdef UMBRA_STATS
    uint32_t us = zxv_time_us() - stats_t0;
    if (us > stats_max)
        stats_max = us;
    if (frame % ZXV_FRAME_HZ == 0) {
        stats_us = stats_max;
        stats_max = 0;
    }
#endif
    /* El destello rojo del borde cuando una sombra te toca. */
    *(volatile uint8_t *)(uintptr_t)ZXV_V_BORDER = (uint8_t)(flash_t ? 2 : border);
    if (flash_t)
        flash_t--;
    sound_frame();
    video_end();
    frame++;
#ifdef UMBRA_STATS
    stats_t0 = zxv_time_us();           /* el frame siguiente empieza aquí */
#endif
}

/* --- El récord -------------------------------------------------------------------- */

#define RECORD_FILE "SD:umbra.txt"

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

/* --- Un nivel ------------------------------------------------------------------------ */

enum { END_CLEAR, END_DEAD, END_ESC, END_DEMO };

static void start_level(int lvl)
{
    maze_build(lvl);
    view_level(level_wall);
    place_player();
    pl.oil = OIL_MAX;
    pl.hurt = 2 * ZXV_FRAME_HZ;
    bot_target = -1;
    banner_t = 0;
}

/* Lo que pasa en un frame del nivel: la lámpara, las cosas, las sombras.
 * Devuelve -1 si sigue, o cómo acaba. */
static int level_update(int in)
{
    /* Girar y andar. */
    int turn = per_frame(&pl.turn_rest, TURN_SPEED);
    if (in & IN_LEFT)
        pl.ang -= turn;
    if (in & IN_RIGHT)
        pl.ang += turn;
    pl.ang &= TURN - 1;
    int fwd = per_frame(&pl.walk_rest, WALK), side = per_frame(&pl.side_rest, SIDESTEP);
    int32_t ca = icos(pl.ang), sa = isin(pl.ang);
    int32_t dx = 0, dy = 0;
    if (in & IN_UP) {
        dx += (int32_t)(((int64_t)ca * fwd) >> 16);
        dy += (int32_t)(((int64_t)sa * fwd) >> 16);
    }
    if (in & IN_DOWN) {
        dx -= (int32_t)(((int64_t)ca * side) >> 16);
        dy -= (int32_t)(((int64_t)sa * side) >> 16);
    }
    if (in & (IN_SLEFT | IN_SRIGHT)) {
        int s = in & IN_SRIGHT ? side : -side;
        dx -= (int32_t)(((int64_t)sa * s) >> 16);
        dy += (int32_t)(((int64_t)ca * s) >> 16);
    }
    if (dx || dy) {
        int32_t ox = pl.x, oy = pl.y;
        walk(dx, dy);
        if (cell_at(pl.x >> 16, pl.y >> 16) == CELL_GATE) {
            pl.x = ox;                  /* por la puerta abierta: la vista, desde fuera */
            pl.y = oy;
            return END_CLEAR;
        }
    }
    int cx = pl.x >> 16, cy = pl.y >> 16;
    if (cx != pl.cx || cy != pl.cy) {
        pl.cx = cx;
        pl.cy = cy;
        maze_distances(cx, cy);
    }

    /* Lo que se coge. */
    for (int i = 0; i < item_count; i++) {
        item_t *it = &items[i];
        if (it->taken || abs_i(it->x - pl.x) > REACH || abs_i(it->y - pl.y) > REACH)
            continue;
        it->taken = 1;
        if (it->kind == ITEM_FLASK) {
            pl.oil = pl.oil + OIL_FLASK > OIL_MAX ? OIL_MAX : pl.oil + OIL_FLASK;
            pl.score += 50;
            sound_fx(FX_FLASK);
        } else {
            pl.score += 100 * (uint32_t)(level + 1);
            if (--runes_left == 0) {
                gate_open = 1;
                maze_distances(cx, cy);
                sound_fx(FX_GATE);
                banner(T("¡LA PUERTA SE ABRE!", "THE GATE OPENS!"), 0, 3);
            } else {
                sound_fx(FX_RUNE);
            }
        }
    }

    /* La lámpara se gasta. */
#ifndef UMBRA_STATS
    pl.oil -= per_frame(&pl.oil_rest, OIL_BURN);
#endif
    /* Las sombras: más deprisa con poca luz y en los niveles de más adelante. */
    shadows_update(282 + level * 38 + (OIL_MAX - pl.oil) * 230 / OIL_MAX);
    int near = 255;
    if (pl.hurt)
        pl.hurt--;
    for (int i = 0; i < shadow_count; i++) {
        shadow_t *s = &shadows[i];
        int d = dist[(s->y >> 16) * MAZE_MAX + (s->x >> 16)];
        if (d < near)
            near = d;
        if (pl.hurt || abs_i(s->x - pl.x) > REACH || abs_i(s->y - pl.y) > REACH)
            continue;
#ifndef UMBRA_STATS
        pl.oil -= OIL_BITE;
#endif
        pl.hurt = 3 * ZXV_FRAME_HZ / 2;
        flash_t = ZXV_FRAME_HZ / 4;
        sound_fx(FX_HIT);
        shadows_respawn(s, 6);
    }
    if (pl.oil <= 0) {                  /* la lámpara se apaga */
        pl.oil = 0;
        pl.lives--;
        sound_fx(FX_DEATH);
        if (pl.lives <= 0)
            return END_DEAD;
        pl.oil = OIL_REVIVE;
        pl.hurt = 3 * ZXV_FRAME_HZ;
        for (int i = 0; i < shadow_count; i++)
            shadows_respawn(&shadows[i], 7);
        banner(T("TU LÁMPARA SE APAGA", "YOUR LAMP GOES OUT"), T("ENCIENDES OTRA", "YOU LIGHT ANOTHER"), 3);
    }
    sound_ambient(pl.oil < OIL_MAX * 7 / 100 ? 3 : pl.oil < OIL_MAX * 15 / 100 ? 2 : pl.oil < OIL_MAX / 4,
                  near <= 2 ? 3 : near <= 4 ? 2 : near <= 7);
    return -1;
}

/* Un nivel entero. */
static int play_level(void)
{
    char name[12];
    const char *label = T("NIVEL ", "LEVEL ");
    int n = 0;
    while (*label)
        name[n++] = *label++;
    if (level >= 9)
        name[n++] = (char)('0' + (level + 1) / 10);
    name[n++] = (char)('0' + (level + 1) % 10);
    name[n] = 0;
    int intro = 2 * ZXV_FRAME_HZ, show_map = 0;
    for (;;) {
        read_input();
        if (input_edge & IN_ESC)
            return END_ESC;
        if (demo && (input_edge & (IN_START | IN_PAUSE)))
            return END_DEMO;
#ifndef UMBRA_STATS
        if (demo && frame > 60 * ZXV_FRAME_HZ)
            return END_DEMO;
#endif
        if (input_edge & IN_PAUSE) {    /* la pausa: hasta otra vez Intro (Esc sale) */
            sound_mute(1);
            for (;;) {
                read_input();
                video_begin();
                draw_world();
                hud();
                box(T("PAUSA", "PAUSED"), 0, 0);
                frame_end();
                if (input_edge & (IN_PAUSE | IN_ESC))
                    break;
            }
            sound_mute(0);
            if (input_edge & IN_ESC)
                return END_ESC;
        }
        if (!demo && (input_edge & IN_MAP))
            show_map ^= 1;
        int r = -1;
        if (!show_map)
            r = level_update(demo ? autopilot() : input_now);
        if (r == END_DEAD || r == END_CLEAR) {
            sound_ambient(0, 0);
            return r;
        }
        video_begin();
        if (show_map) {
            map_screen(T("MAPA", "MAP"), T("M: VOLVER AL LABERINTO", "M: BACK TO THE MAZE"), 0);
        } else {
            draw_world();
            if (banner_t) {             /* un aviso quita el nombre del nivel */
                intro = 0;
                box(banner_a, banner_b, 0);
            } else if (intro) {
                box_big(name);
            }
        }
        if (banner_t)
            banner_t--;
        if (intro)
            intro--;
        hud();
        frame_end();
    }
}

/* Unos segundos con un letrero sobre el mapa del nivel que se acaba de
 * pasar. */
static int level_clear(const char *a, const char *b, int seconds)
{
    banner_t = 0;
    for (int t = 0; t < seconds * ZXV_FRAME_HZ; t++) {
        read_input();
        if (input_edge & IN_ESC)
            return 1;
        if (t > ZXV_FRAME_HZ && (input_edge & IN_START) && !demo)
            break;
        video_begin();
        map_screen(a, b, 0);
        hud();
        frame_end();
    }
    return 0;
}

/* Una partida; 1 si se pulsó Esc. */
static int game(void)
{
    pl = (typeof(pl)){ 0 };
    pl.lives = 3;
#ifdef UMBRA_SCORE
    pl.score = UMBRA_SCORE;
#endif
    banner_t = 0;
    for (int lvl = 0; lvl < levels; lvl++) {
        start_level(lvl);
        sound_song(SONG_MAZE);
        int r = play_level();
        if (r == END_ESC)
            return 1;
        if (r == END_DEMO)
            return 0;
        if (r == END_DEAD) {
            sound_song(SONG_OVER);
            break;
        }
        pl.score += 500 * (uint32_t)(level + 1) + (uint32_t)pl.oil / 10;
        sound_song(SONG_CLEAR);
        if (lvl + 1 < levels &&
            level_clear(T("¡NIVEL SUPERADO!", "LEVEL CLEAR!"), T("LA SALIDA BAJA MÁS HONDO", "THE WAY LEADS DEEPER"), 4))
            return 1;
    }
    int won = pl.lives > 0;
    int best = !demo && pl.score > record;
    if (best) {
        record = pl.score;
        save_record();
    }
    /* "PUNTOS 1234567" */
    char score_line[24], digits[8];
    const char *label = T("PUNTOS ", "SCORE ");
    int n = 0;
    while (*label)
        score_line[n++] = *label++;
    number(digits, pl.score, 7);
    for (int i = 0; digits[i]; i++)
        score_line[n++] = digits[i];
    score_line[n] = 0;
    for (int t = 0; t < 8 * ZXV_FRAME_HZ; t++) {
        read_input();
        if (input_edge & IN_ESC)
            return 1;
        if (t > 2 * ZXV_FRAME_HZ && (input_edge & IN_START))
            break;
        if (demo && t > 3 * ZXV_FRAME_HZ)
            break;
        video_begin();
        const char *record_line = best ? T("¡NUEVO RÉCORD!", "NEW HIGH SCORE!") : 0;
        if (won) {                      /* desde fuera, el último laberinto */
            map_screen(T("¡HAS ESCAPADO DE LA OSCURIDAD!", "YOU ESCAPED THE DARKNESS!"), score_line, record_line);
        } else {
            draw_world();
            box(T("FIN DE LA PARTIDA", "GAME OVER"), score_line, record_line);
        }
        hud();
        frame_end();
    }
    return 0;
}

/* --- El título ------------------------------------------------------------------- */

/* El primer nivel, girando despacio, el nombre y las teclas. 0 para jugar, 1
 * para la demostración, -1 para salir. */
static int title(void)
{
    static const uint8_t logo[7] = { 15, 15, 13, 13, 5, 5, 1 };
    start_level(0);
    sound_song(SONG_TITLE);
    sound_ambient(0, 0);
    for (int t = 0;; t++) {
        read_input();
        if (input_edge & IN_ESC)
            return -1;
        if (t > ZXV_FRAME_HZ * 2 / 5 && (input_edge & IN_START)) {
            sound_fx(FX_BEEP);
            return 0;
        }
        if (t > 12 * ZXV_FRAME_HZ)
            return 1;
        pl.ang = (start_ang + t * TURN / (20 * ZXV_FRAME_HZ)) & (TURN - 1);  /* una vuelta en 20 s */
        video_begin();
        draw_world();
        text_big((W - 5 * 6 * 5 + 5) / 2, 14, "UMBRA", 5, logo);
        zxv_gfx_rect(0, 52, W, 24, 0);      /* franjas para que se lean */
        zxv_gfx_rect(0, 116, W, 13, 0);
        text_center(56, T("REÚNE LAS RUNAS Y BUSCA LA SALIDA", "GATHER THE RUNES AND FIND THE EXIT"), 15);
        text_center(66, T("ANTES DE QUE SE APAGUE TU LÁMPARA", "BEFORE YOUR LAMP GOES OUT"), 15);
        if (t & 32)
            text_center(119, T("PULSA ESPACIO O FUEGO", "PRESS SPACE OR FIRE"), 14);
        zxv_gfx_rect(0, VIEW_H, W, H - VIEW_H, 0);
        zxv_gfx_rect(0, VIEW_H, W, 1, 1);
        text_center(148, T("CURSORES O QAOP: ANDAR Y GIRAR", "ARROWS OR QAOP: WALK AND TURN"), 15);
        text_center(158, T("Z Y X: DE LADO   M: MAPA", "Z AND X: SIDESTEP   M: MAP"), 15);
        text_center(168, T("INTRO: PAUSA  S: MÚSICA  ESC: SALIR", "ENTER: PAUSE  S: MUSIC  ESC: QUIT"), 13);
        char buf[16], line[32];
        number(buf, record, 7);
        const char *label = T("RÉCORD ", "HIGH SCORE ");
        int n = 0;
        while (*label)
            line[n++] = *label++;
        for (int i = 0; buf[i]; i++)
            line[n++] = buf[i];
        line[n] = 0;
        text_center(180, line, 14);
        frame_end();
    }
}

int main(void)
{
    lang = zxv_lang_get() == ZXV_LANG_ES ? 0 : 1;
    border = *(volatile uint8_t *)(uintptr_t)ZXV_V_BORDER;     /* el del BASIC, para volver */
    video_init();                       /* antes: lo de abajo usa la memoria de vídeo que no se ve */
    sound_init();
    art_init();
    maze_init();
    load_record();
    video_init();                       /* otra vez: lo que tarda lo de antes no son frames perdidos */
    input_now = 0x7FF;                  /* lo que esté pulsado al empezar no cuenta */
    for (;;) {
        int r = title();
        if (r < 0)
            break;
        demo = r;
        frame = 0;
        rng_state = demo ? 4242 : (rng_state ^ *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES);
        game();
        demo = 0;
    }
    *(volatile uint8_t *)(uintptr_t)ZXV_V_BORDER = (uint8_t)border;
    sound_off();
    video_done();
    zxv_set_mode(0);
    while (zxv_key_get() >= 0)
        ;
    return 0;                           /* el récord ya está en la tarjeta */
}
