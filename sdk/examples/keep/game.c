/*
 * game.c - The Keep: una aventura de mazmorras para ZX-V, vista desde arriba.
 *
 * Una torre de cuatro plantas, cada una de 3 x 3 salas: se pasa de una sala
 * a otra por sus puertas (la pantalla se desliza) y se sube por la escalera.
 * En cada planta hay un tesoro que abre el camino: las botas (los pinchos
 * ya no hacen daño), la espada grande (pega más y corta las zarzas), el
 * farol (la biblioteca está a oscuras) y la llave grande (la puerta del
 * hechicero). Arriba espera el hechicero.
 *
 * El héroe sube de nivel con lo que vence: más corazones y más fuerza. Las
 * llaves abren las puertas cerradas; las pociones llenan los corazones.
 *
 * Controles: cursores u OPQA para andar; espacio (o M) la espada; Z una
 * poción; Intro la pausa, S la música y Esc para salir; o un mando (fuego 1
 * la espada, fuego 2 la poción, Start la pausa). Sin tocar nada en el título
 * empieza una demostración que juega sola. El récord (la victoria más
 * rápida y el que más oro ha juntado) se guarda en SD:keep.txt.
 *
 * Compilado con -DKEEP_STATS (make EXTRA_CFLAGS=-DKEEP_STATS), el marcador
 * enseña los frames perdidos y la demostración juega la partida entera sin
 * perder vida, hasta vencer al hechicero, y lo apunta en SD:keepstat.txt:
 * para medir el rendimiento. Con -DKEEP_BOT, la demostración juega la
 * partida entera como cualquiera (hasta ganar o perder) y también lo apunta:
 * para ver lo difícil que es.
 */
#include "keep.h"

uint32_t rng_state = 1;
int lang;
int cur_floor, room_x, room_y;
uint32_t frame;

const int8_t sin64[64] = {
    0, 6, 12, 19, 24, 30, 36, 41, 45, 49, 53, 56, 59, 61, 63, 64, 64, 64, 63, 61, 59, 56,
    53, 49, 45, 41, 36, 30, 24, 19, 12, 6, 0, -6, -12, -19, -24, -30, -36, -41, -45, -49,
    -53, -56, -59, -61, -63, -64, -64, -64, -63, -61, -59, -56, -53, -49, -45, -41, -36,
    -30, -24, -19, -12, -6,
};

static int abs_i(int v) { return v < 0 ? -v : v; }

static int demo;
static uint32_t play_frames;            /* el reloj de la partida */
static uint32_t best_time, best_gold;   /* el récord: segundos y oro */
#if defined(KEEP_STATS) || defined(KEEP_BOT)
#define KEEP_WHOLE                      /* la demostración, la partida entera */
#endif
#ifdef KEEP_STATS
static uint32_t stats_us, stats_max, stats_t0, stats_worst;
#endif

#define LAST_FLOOR  (FLOORS - 1)
#define DARK_FLOOR  2                   /* la biblioteca */

/* --- La entrada -------------------------------------------------------------------- */

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
        in |= IN_ATTACK;                /* espacio, M */
    if (key(0x1D))
        in |= IN_POTION;                /* Z */
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
        if (joy & ZXV_JOY_FIRE1) in |= IN_ATTACK;
        if (joy & ZXV_JOY_FIRE2) in |= IN_POTION;
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

/* --- Los mensajes ------------------------------------------------------------------ */

static const char *msg_line[3];
static int msg_on, msg_t;
static int hints_seen;

void message(const char *l1, const char *l2, const char *l3)
{
    msg_line[0] = l1;
    msg_line[1] = l2;
    msg_line[2] = l3;
    msg_on = 1;
    msg_t = 0;
    sound_fx(FX_TALK);
}

void hint(int which)
{
    if (hints_seen & (1 << which))
        return;
    hints_seen |= 1 << which;
    switch (which) {
    case HINT_DOOR:
        message(T("LA PUERTA ESTÁ CERRADA.", "THE DOOR IS LOCKED."),
                T("HACE FALTA UNA LLAVE.", "YOU NEED A KEY."), 0);
        break;
    case HINT_BOSS_DOOR:
        message(T("UNA PUERTA ENORME, CON UNA", "A HUGE DOOR WITH A HUGE"),
                T("CERRADURA ENORME.", "LOCK."), 0);
        break;
    case HINT_SPIKES:
        message(T("¡AY! LOS PINCHOS NO TE", "OUCH! THE SPIKES WON'T"),
                T("DEJAN PASAR.", "LET YOU THROUGH."), 0);
        break;
    case HINT_BRAMBLE:
        message(T("LAS ZARZAS SON DEMASIADO", "THE BRAMBLES ARE TOO"),
                T("DURAS PARA TU ESPADA.", "TOUGH FOR YOUR SWORD."), 0);
        break;
    }
}

/* El recuadro del mensaje, en la sala, con la marca de seguir en su propia
 * línea. Caben 35 letras por línea. */
static void draw_message(void)
{
    int n = msg_line[2] ? 3 : msg_line[1] ? 2 : 1;
    int h = 14 + n * 11 + 8, y = 112 - h / 2;
    box(16, y, W - 32, h);
    for (int i = 0; i < n; i++)
        text_shadow(26, y + 8 + i * 11, msg_line[i], 15);
    if (msg_t > 20 && (frame & 16))
        text(W - 32, y + 8 + n * 11, "+", 14);
}

/* --- Los objetos ---------------------------------------------------------------- */

static int item_mark(int m) { return m >= M_KEY && m <= M_SPECIAL; }

void pickup(int mark)
{
    switch (mark) {
    case M_KEY:
        hero.keys++;
        sound_fx(FX_ITEM);
        break;
    case M_POTION:
        hero.potions++;
        sound_fx(FX_ITEM);
        break;
    case M_HEART:
        if (hero.hp_max < 20)
            hero.hp_max += 2;
        hero.hp = hero.hp_max;
        sound_fx(FX_LEVEL);
        message(T("¡UN CORAZÓN MÁS!", "ONE MORE HEART!"), 0, 0);
        break;
    case M_GOLD:
        hero.gold += 10 + (int)(rnd() % 16);
        sound_fx(FX_COIN);
        break;
    case M_SPECIAL:
        sound_fx(FX_LEVEL);
        switch (cur_floor) {
        case 0:
            hero.items |= ITEM_BOOTS;
            message(T("¡LAS BOTAS DE HIERRO!", "THE IRON BOOTS!"),
                    T("LOS PINCHOS YA NO TE", "THE SPIKES CAN'T HURT"), T("HACEN DAÑO.", "YOU ANY MORE."));
            break;
        case 1:
            hero.items |= ITEM_SWORD;
            message(T("¡LA ESPADA GRANDE!", "THE GREAT SWORD!"),
                    T("PEGA MÁS FUERTE Y CORTA", "IT HITS HARDER AND CUTS"), T("LAS ZARZAS.", "THROUGH BRAMBLES."));
            break;
        case 2:
            hero.items |= ITEM_LANTERN;
            message(T("¡EL FAROL!", "THE LANTERN!"), T("AHORA SE VE TODA LA SALA.", "NOW YOU CAN SEE THE ROOM."), 0);
            break;
        default:
            hero.items |= ITEM_BIG_KEY;
            message(T("¡LA LLAVE GRANDE!", "THE BIG KEY!"),
                    T("ABRE LA PUERTA DEL", "IT OPENS THE SORCERER'S"), T("HECHICERO.", "DOOR."));
            break;
        }
        break;
    }
    bot_dirty();
}

static int item_sprite(int mark)
{
    switch (mark) {
    case M_KEY: return S_KEY;
    case M_POTION: return S_POTION;
    case M_HEART: return S_HEART;
    case M_GOLD: return S_GOLD;
    default: return S_BOOTS + (cur_floor < 4 ? cur_floor : 3);
    }
}

static void items_update(void)
{
    int hx = hero.x / FIX + 3, hy = hero.y / FIX + 2;
    for (int i = 0; i < nspawns; i++) {
        spawn_t *s = &spawns[i];
        if (!s->alive || !item_mark(s->mark) || s->cx / ROOM_W != room_x || s->cy / ROOM_H != room_y)
            continue;
        int x = s->cx * TILE + 2, y = s->cy * TILE + 2;
        if (hx < x + 12 && x < hx + 10 && hy < y + 12 && y < hy + 13) {
            s->alive = 0;
            pickup(s->mark);
        }
    }
}

static void items_draw(int cam_x, int cam_y)
{
    for (int i = 0; i < nspawns; i++) {
        spawn_t *s = &spawns[i];
        if (!s->alive || !item_mark(s->mark) || s->cx / ROOM_W != room_x || s->cy / ROOM_H != room_y)
            continue;
        int bob = (int)((frame + i * 9) >> 4) & 1;
        draw_sprite(&spr[item_sprite(s->mark)], s->cx * TILE - cam_x, s->cy * TILE - cam_y + VIEW_TOP - bob, 0, 0);
    }
}

/* --- El piloto de la demostración --------------------------------------------- */

/* Va a lo que quede por coger de la planta y luego a la escalera (o al
 * hechicero), por el camino más corto que se puede andar con lo que lleva;
 * y pelea con lo que se le acerque. */
static uint16_t dist[MAP_H][MAP_W];
static uint16_t queue[MAP_W * MAP_H];
static int bot_need, bot_stuck, bot_last_x, bot_last_y, bot_wander, bot_wander_in;

void bot_dirty(void) { bot_need = 1; }

static int bot_pass(int t)
{
    if (t == T_SPIKES)
        return hero.items & ITEM_BOOTS;
    if (t == T_DOOR)
        return hero.keys > 0;
    if (t == T_BOSS_DOOR)
        return hero.items & ITEM_BIG_KEY;
    if (t == T_BRAMBLE)
        return hero.items & ITEM_SWORD;
    return !solid_tile(t);
}

/* Las distancias a lo que se busca (pass 0: los objetos; 1: la escalera o
 * el hechicero). */
static void bot_field(int pass)
{
    int head = 0, tail = 0;
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            dist[y][x] = 0xFFFF;
    if (pass == 0) {
        for (int i = 0; i < nspawns; i++)
            if (spawns[i].alive && item_mark(spawns[i].mark)) {
                dist[spawns[i].cy][spawns[i].cx] = 0;
                queue[tail++] = (uint16_t)(spawns[i].cy * MAP_W + spawns[i].cx);
            }
    } else {
        for (int y = 0; y < MAP_H; y++)
            for (int x = 0; x < MAP_W; x++)
                if (cells[y][x] == T_STAIRS) {
                    dist[y][x] = 0;
                    queue[tail++] = (uint16_t)(y * MAP_W + x);
                }
        for (int i = 0; i < nspawns; i++)
            if (spawns[i].alive && spawns[i].mark == M_BOSS) {
                dist[spawns[i].cy][spawns[i].cx] = 0;
                queue[tail++] = (uint16_t)(spawns[i].cy * MAP_W + spawns[i].cx);
            }
    }
    while (head < tail) {
        int c = queue[head++], x = c % MAP_W, y = c / MAP_W;
        static const int8_t d[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int k = 0; k < 4; k++) {
            int nx = x + d[k][0], ny = y + d[k][1];
            if (nx < 0 || ny < 0 || nx >= MAP_W || ny >= MAP_H || dist[ny][nx] != 0xFFFF ||
                !bot_pass(cells[ny][nx]))
                continue;
            dist[ny][nx] = (uint16_t)(dist[y][x] + 1);
            queue[tail++] = (uint16_t)(ny * MAP_W + nx);
        }
    }
}

/* Hacia (tx, ty), en píxeles del mundo, desde (fx, fy): primero alinea el eje
 * de través (para no engancharse en las esquinas). */
static int steer(int fx, int fy, int tx, int ty, int horiz)
{
    int in = 0, dx = tx - fx, dy = ty - fy;
    if (horiz) {
        if (dy > 2) in |= IN_DOWN;
        else if (dy < -2) in |= IN_UP;
        else in |= dx > 0 ? IN_RIGHT : IN_LEFT;
    } else {
        if (dx > 2) in |= IN_RIGHT;
        else if (dx < -2) in |= IN_LEFT;
        else in |= dy > 0 ? IN_DOWN : IN_UP;
    }
    return in;
}

static int bot_input(void)
{
    int fx = hero_feet_x(), fy = hero.y / FIX + 11;
    int hx = hero.x / FIX + 8, hy = hero.y / FIX + 8;
    if (hero.potions && hero.hp <= 2)
        return IN_POTION;
    if (bot_wander) {                   /* atascado: un rato a otro lado */
        bot_wander--;
        return bot_wander_in;
    }
    /* Lo que tenga cerca, a por ello (al hechicero, siempre). */
    int best = -1, best_d = 0x7FFF;
    for (int i = 0; i < ACTORS; i++) {
        int t = actors[i].type;
        if (t < A_SLIME || t > A_BOSS)
            continue;
        int ex = actors[i].x / FIX + (t == A_BOSS ? 12 : 8), ey = actors[i].y / FIX + (t == A_BOSS ? 18 : 8);
        int d = abs_i(ex - hx) + abs_i(ey - hy);
        if (t == A_BOSS)
            d = d > 40 ? 40 : d;
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    if (best >= 0 && best_d < 44) {
        actor_t *a = &actors[best];
        int ex = a->x / FIX + (a->type == A_BOSS ? 12 : 8), ey = a->y / FIX + (a->type == A_BOSS ? 18 : 8);
        int dx = ex - hx, dy = ey - hy, reach = a->type == A_BOSS ? 24 : 18;
        int in = 0;
        if (abs_i(dx) >= abs_i(dy)) {
            if (abs_i(dy) > 6) in = dy > 0 ? IN_DOWN : IN_UP;
            else if (abs_i(dx) > reach) in = dx > 0 ? IN_RIGHT : IN_LEFT;
            else if (hero.dir != (dx > 0 ? D_RIGHT : D_LEFT)) in = dx > 0 ? IN_RIGHT : IN_LEFT;
            else if (!hero.attack) in = IN_ATTACK;
        } else {
            if (abs_i(dx) > 6) in = dx > 0 ? IN_RIGHT : IN_LEFT;
            else if (abs_i(dy) > reach) in = dy > 0 ? IN_DOWN : IN_UP;
            else if (hero.dir != (dy > 0 ? D_DOWN : D_UP)) in = dy > 0 ? IN_DOWN : IN_UP;
            else if (!hero.attack) in = IN_ATTACK;
        }
        if (in)
            return in;
    }
    /* El camino. */
    if (bot_need || frame % 60 == 0) {
        bot_need = 0;
        bot_field(0);
        if (dist[fy / TILE][fx / TILE] == 0xFFFF)
            bot_field(1);
    }
    int cx = fx / TILE, cy = fy / TILE, d = dist[cy][cx];
    int in = 0;
    if (d != 0xFFFF && d > 0) {
        static const int8_t dv[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        int nx = cx, ny = cy;
        for (int k = 0; k < 4; k++) {
            int x = cx + dv[k][0], y = cy + dv[k][1];
            if (x >= 0 && y >= 0 && x < MAP_W && y < MAP_H && dist[y][x] < d) {
                nx = x;
                ny = y;
                break;
            }
        }
        if (cells[ny][nx] == T_BRAMBLE) {   /* zarzas delante: a espadazos */
            int want = nx > cx ? D_RIGHT : nx < cx ? D_LEFT : ny > cy ? D_DOWN : D_UP;
            if (hero.dir != want)
                return want == D_RIGHT ? IN_RIGHT : want == D_LEFT ? IN_LEFT : want == D_DOWN ? IN_DOWN : IN_UP;
            return hero.attack ? 0 : IN_ATTACK;
        }
        in = steer(fx, fy, nx * TILE + 8, ny * TILE + 8, nx != cx);
    } else if (d == 0) {
        in = steer(fx, fy, cx * TILE + 8, cy * TILE + 8, 1);    /* encima: al centro */
        bot_need = 1;
    }
    /* Si no se mueve en un rato, a un lado al azar. */
    if (hero.x == bot_last_x && hero.y == bot_last_y && !hero.attack) {
        if (++bot_stuck > ZXV_FRAME_HZ * 3 / 4) {
            static const int dirs[4] = { IN_UP, IN_DOWN, IN_LEFT, IN_RIGHT };
            bot_wander_in = dirs[rnd() % 4];
            bot_wander = 20;
            bot_stuck = 0;
        }
    } else {
        bot_stuck = 0;
    }
    bot_last_x = hero.x;
    bot_last_y = hero.y;
    return in;
}

/* --- La pantalla -------------------------------------------------------------- */

/* El marcador: dos líneas de HUD_H, que no tapa nada más.
 *
 *   1: los corazones (hasta 10), las llaves, las pociones, el oro, los
 *      tesoros y el nivel, cada uno en su sitio
 *   2: a la izquierda, lo que mide KEEP_STATS, el letrero de la
 *      demostración o la planta; en medio, la vida del hechicero o el nivel
 *      nuevo; a la derecha, en la demostración, cómo salir de ella
 */
#define LINE2       15
#define MID_X0      86                  /* el sitio de en medio de la línea 2 */
#define MID_X1      170

static void hud(void)
{
    char buf[16];
    zxv_gfx_rect(0, 0, W, HUD_H, 0);
    zxv_gfx_rect(0, HUD_H - 1, W, 1, 1);
    for (int i = 0; i < hero.hp_max / 2; i++) {
        int h = hero.hp - i * 2;
        draw_sprite(&spr[S_ICONS + (h >= 2 ? I_HEART : h == 1 ? I_HALF : I_EMPTY)], 3 + i * 9, 3, 0, 0);
    }
    static const struct { uint8_t icon; uint8_t x; } counts[3] = {
        { I_KEY, 98 }, { I_POTION, 124 }, { I_COIN, 150 },
    };
    int values[3] = { hero.keys, hero.potions, hero.gold };
    for (int i = 0; i < 3; i++) {
        draw_sprite(&spr[S_ICONS + counts[i].icon], counts[i].x, 3, 0, 0);
        if (i < 2) {
            buf[0] = '*';
            number(buf + 1, (uint32_t)(values[i] > 9 ? 9 : values[i]), 1);
        } else {
            number(buf, (uint32_t)(values[i] > 999 ? 999 : values[i]), 3);
        }
        text(counts[i].x + 10, 4, buf, 15);
    }
    for (int i = 0, x = 184; i < 4; i++)
        if (hero.items & (1 << i)) {
            draw_sprite(&spr[S_ICONS + I_BOOTS + i], x, 3, 0, 0);
            x += 9;
        }
    char *p = append(buf, T("NV", "LV"));
    number(p, (uint32_t)(hero.level > 9 ? 9 : hero.level), 1);
    text(W - 20, 4, buf, level_up_t && (frame & 8) ? 15 : 14);

    /* La línea 2, a la izquierda. */
#ifdef KEEP_STATS
    number(buf, video_lost(), 4);       /* los frames perdidos */
    text(3, LINE2, buf, 10);
    number(buf, stats_us, 5);           /* el frame más lento del último segundo, en us */
    text(33, LINE2, buf, 13);
#else
    if (demo) {
        text(3, LINE2, T("DEMOSTRACIÓN", "DEMO"), (frame & 32) ? 13 : 5);
    } else {
        p = append(buf, T("PLANTA ", "FLOOR "));
        number(p, (uint32_t)cur_floor + 1, 1);
        text(3, LINE2, buf, 5);
    }
#endif
    /* En medio. */
    if (boss_alive()) {
        int w = boss_hp() * (MID_X1 - MID_X0 - 4) / 30;
        zxv_gfx_rect(MID_X0, LINE2, MID_X1 - MID_X0, 7, 7);
        zxv_gfx_rect(MID_X0 + 1, LINE2 + 1, MID_X1 - MID_X0 - 2, 5, 0);
        zxv_gfx_rect(MID_X0 + 2, LINE2 + 2, MID_X1 - MID_X0 - 4, 3, 2);
        zxv_gfx_rect(MID_X0 + 2, LINE2 + 2, w < 0 ? 0 : w, 3, 10);
    } else if (level_up_t) {
        p = append(buf, T("¡NIVEL ", "LEVEL "));
        number(p, (uint32_t)(hero.level > 9 ? 9 : hero.level), 1);
        append(p + 1, "!");
        text((MID_X0 + MID_X1 - text_width(buf)) / 2, LINE2, buf, (frame & 8) ? 15 : 14);
    }
    /* A la derecha. */
    if (demo) {
        const char *s = T("PULSA ESPACIO", "PRESS SPACE");
        text(W - 3 - text_width(s), LINE2, s, 15);
    }
}

/* Un letrero de una línea en un recuadro, en medio de la sala. */
static void banner(const char *s, int c)
{
    int w = text_width(s) + 24;
    box((W - w) / 2, 92, w, 25);
    text((W - text_width(s)) / 2, 101, s, c);
}

static int cam_x, cam_y;

static void draw_world(void)
{
    zxv_gfx_clip(0, HUD_H, W, H);
    zxv_map_draw(&floor_map, cam_x, cam_y + HUD_H - VIEW_TOP, 0, HUD_H, W, H - HUD_H, ZXV_MAP_ALL);
    items_draw(cam_x, cam_y);
    actors_draw(cam_x, cam_y);
    if (cur_floor == DARK_FLOOR && !(hero.items & ITEM_LANTERN)) {
        /* A oscuras: solo se ve alrededor del héroe (el recorte quita lo
         * que sale de la sala). */
        int x = hero.x / FIX - cam_x + 8, y = hero.y / FIX - cam_y + VIEW_TOP + 8, r = 40;
        zxv_gfx_rect(0, 0, W, y - r, 0);
        zxv_gfx_rect(0, y + r, W, H, 0);
        zxv_gfx_rect(0, y - r, x - r, 2 * r, 0);
        zxv_gfx_rect(x + r, y - r, W, 2 * r, 0);
    }
    zxv_gfx_clip(0, 0, W, H);
}

static void frame_end(void)
{
#ifdef KEEP_STATS
    if (stats_t0) {                     /* solo los frames de la partida */
        uint32_t us = zxv_time_us() - stats_t0;
        if (us > stats_max)
            stats_max = us;
        if (us > stats_worst)
            stats_worst = us;
        stats_t0 = 0;
    }
    if (frame % ZXV_FRAME_HZ == 0) {
        stats_us = stats_max;
        stats_max = 0;
    }
#endif
    sound_frame();
    video_end();
    frame++;
}

/* --- El récord -------------------------------------------------------------------- */

#define RECORD_FILE "SD:keep.txt"

static void load_record(void)
{
    char buf[24];
    int f = zxv_file_open(RECORD_FILE, ZXV_O_READ);
    if (f < 0)
        return;
    int n = zxv_file_read(f, buf, sizeof(buf) - 1);
    zxv_file_close(f);
    uint32_t v[2] = { 0, 0 };
    int k = 0;
    for (int i = 0; i < n && k < 2; i++) {
        if (buf[i] >= '0' && buf[i] <= '9')
            v[k] = v[k] * 10 + (uint32_t)(buf[i] - '0');
        else if (buf[i] == ' ')
            k++;
        else
            break;
    }
    best_time = v[0];
    best_gold = v[1];
}

static void save_record(void)
{
    char buf[16];
    number(buf, best_time, 5);
    buf[5] = ' ';
    number(buf + 6, best_gold, 5);
    buf[11] = '\n';
    int f = zxv_file_open(RECORD_FILE, ZXV_O_WRITE | ZXV_O_CREATE | ZXV_O_TRUNC);
    if (f < 0)
        return;                         /* sin tarjeta: el récord dura lo que la partida */
    zxv_file_write(f, buf, 12);
    zxv_file_close(f);
}

/* Lo de esta partida, en el récord; 1 si es nuevo. */
static int update_record(int won)
{
    if (demo)
        return 0;
    int better = 0;
    uint32_t secs = play_frames / ZXV_FRAME_HZ;
    if (won && (!best_time || secs < best_time)) {
        best_time = secs;
        better = 1;
    }
    if ((uint32_t)hero.gold > best_gold) {
        best_gold = (uint32_t)hero.gold;
        better = 1;
    }
    if (better)
        save_record();
    return better;
}

static char *time_text(char *out, uint32_t secs)
{
    if (secs >= 6000)
        secs = 5999;
    number(out, secs / 60, 2);
    out[2] = ':';
    number(out + 3, secs % 60, 2);
    return out + 5;
}

/* --- La partida --------------------------------------------------------------- */

static const char *floor_name(int f)
{
    switch (f) {
    case 0: return T("LAS MAZMORRAS", "THE DUNGEONS");
    case 1: return T("LA ARMERÍA", "THE ARMOURY");
    case 2: return T("LA BIBLIOTECA", "THE LIBRARY");
    default: return T("LA CIMA", "THE SUMMIT");
    }
}

static void enter_room(int rx, int ry)
{
    room_x = rx;
    room_y = ry;
    actors_room(rx, ry);
    sound_song(boss_alive() ? SONG_BOSS : SONG_TOWER);
}

static int hero_room_x(void) { return (hero.x / FIX + 8) / (ROOM_W * TILE); }
static int hero_room_y(void) { return (hero.y / FIX + 10) / (ROOM_H * TILE); }

/* El letrero de la planta, dos segundos (o hasta la espada). */
static void floor_intro(void)
{
    char buf[24];
    char *p = append(buf, T("PLANTA ", "FLOOR "));
    number(p, (uint32_t)cur_floor + 1, 1);
    for (int t = 0; t < 2 * ZXV_FRAME_HZ; t++) {
        read_input();
        if (t > 20 && (input_edge & IN_ATTACK) && !demo)
            break;
        video_begin();
        zxv_gfx_rect(0, 0, W, H, 0);
        text_big((W - text_width(buf) * 2) / 2, 74, buf, 2, 14);
        text_center(100, floor_name(cur_floor), 15);
        frame_end();
    }
}

static void start_floor(int f)
{
    cur_floor = f;
    world_load(f);
    hints_seen = 0;
    int px, py;
    find_start(&px, &py);
    hero_place(px, py);
    hero.dir = D_UP;
    floor_intro();
    enter_room(hero_room_x(), hero_room_y());
    cam_x = room_x * ROOM_W * TILE;
    cam_y = room_y * ROOM_H * TILE;
    bot_dirty();
}

static void drink(void)
{
    if (hero.potions && hero.hp < hero.hp_max) {
        hero.potions--;
        hero.hp = hero.hp_max;
        sound_fx(FX_POTION);
    }
}

#define SLIDE       24                  /* frames del paso de una sala a otra */

#ifdef KEEP_WHOLE
static void save_stats(int won)
{
    char buf[80], *p = buf;
    p = append(p, won ? "won" : "lost");
    p = append(p, " floor ");
    number(p, (uint32_t)cur_floor + 1, 1);
    p = append(p + 1, " frames ");
    number(p, play_frames, 6);
    p = append(p + 6, " lost ");
    number(p, video_lost(), 5);
    p += 5;
#ifdef KEEP_STATS
    p = append(p, " worst_us ");
    number(p, stats_worst, 6);
    p += 6;
#endif
    p = append(p, " level ");
    number(p, (uint32_t)hero.level, 1);
    p = append(p + 1, " gold ");
    number(p, (uint32_t)hero.gold, 4);
    p = append(p + 4, "\n");
    int f = zxv_file_open("SD:keepstat.txt", ZXV_O_WRITE | ZXV_O_CREATE | ZXV_O_TRUNC);
    if (f >= 0) {
        zxv_file_write(f, buf, (uint32_t)(p - buf));
        zxv_file_close(f);
    }
}
#endif

/* La victoria: el tiempo, el oro y el récord, hasta una tecla. */
static void victory(int record)
{
    char buf[24], line[40];
    for (int t = 0; t < 15 * ZXV_FRAME_HZ; t++) {
        read_input();
        if (t > 2 * ZXV_FRAME_HZ && (input_edge & (IN_ATTACK | IN_ESC | IN_PAUSE)))
            break;
        if (demo && t > 5 * ZXV_FRAME_HZ)
            break;
        video_begin();
        zxv_gfx_rect(0, 0, W, H, 0);
        draw_sprite(&spr[S_TOWER], (W - 48) / 2, 96, 0, 0);
        text_center(20, T("¡EL HECHICERO HA CAÍDO!", "THE SORCERER HAS FALLEN!"), (frame & 8) ? 15 : 14);
        text_center(34, T("THE KEEP VUELVE A SER LIBRE.", "THE KEEP IS FREE AGAIN."), 13);
        time_text(append(line, T("TIEMPO ", "TIME ")), play_frames / ZXV_FRAME_HZ);
        text_center(54, line, 15);
        number(buf, (uint32_t)hero.gold, 3);
        append(append(line, T("ORO ", "GOLD ")), buf);
        text_center(66, line, 14);
        if (record)
            text_center(80, T("¡NUEVO RÉCORD!", "NEW RECORD!"), (frame & 8) ? 14 : 10);
        frame_end();
    }
}

/* Una partida; 1 si se pulsó Esc. */
static int play(void)
{
    hero_reset();
    hero_god = 0;
#ifdef KEEP_STATS
    hero_god = demo;
    stats_worst = 0;
#endif
    boss_dead = 0;
    play_frames = 0;
    msg_on = 0;
    level_up_t = 0;
    start_floor(0);
    int slide = 0, from_x = 0, from_y = 0, end_t = 0;
    for (;;) {
        read_input();
        if (input_edge & IN_ESC) {
            update_record(0);
            return 1;
        }
        if (demo && (input_edge & (IN_ATTACK | IN_PAUSE)))
            return 0;
#ifndef KEEP_WHOLE
        if (demo && frame > 40 * ZXV_FRAME_HZ)  /* con KEEP_STATS o KEEP_BOT, la partida entera */
            return 0;
#endif
        if (!demo && (input_edge & IN_PAUSE)) { /* la pausa: hasta otra vez Intro (Esc sale) */
            sound_mute(1);
            for (;;) {
                read_input();
                video_begin();
                draw_world();
                hud();
                char line[32];
                time_text(append(line, T("PAUSA  ", "PAUSED  ")), play_frames / ZXV_FRAME_HZ);
                banner(line, (frame & 16) ? 15 : 5);
                video_end();
                frame++;
                if (input_edge & (IN_PAUSE | IN_ESC))
                    break;
            }
            sound_mute(0);
            if (input_edge & IN_ESC) {
                update_record(0);
                return 1;
            }
        }
#ifdef KEEP_STATS
        stats_t0 = zxv_time_us();
#endif
        if (msg_on && (hero.dead || boss_dead))
            msg_on = 0;                 /* lo último (un pinchazo) se queda sin mensaje */
        if (msg_on) {                   /* el mensaje para el juego hasta pulsar */
            msg_t++;
            if ((msg_t > 20 && (input_edge & IN_ATTACK)) || (demo && msg_t > ZXV_FRAME_HZ * 3 / 2))
                msg_on = 0;
        } else if (slide) {             /* de una sala a otra */
            slide--;
            cam_x = room_x * ROOM_W * TILE + (from_x - room_x * ROOM_W * TILE) * slide / SLIDE;
            cam_y = room_y * ROOM_H * TILE + (from_y - room_y * ROOM_H * TILE) * slide / SLIDE;
            if (slide & 1) {            /* el héroe entra en la sala nueva */
                int to_x = room_x * ROOM_W * TILE, to_y = room_y * ROOM_H * TILE;
                hero.x += ((to_x > from_x) - (to_x < from_x)) * FIX;
                hero.y += ((to_y > from_y) - (to_y < from_y)) * FIX;
                hero.walk += 2;
            }
        } else if (hero.dead || boss_dead) {
            msg_on = 0;
            actors_update();
            end_t++;
            if (end_t == 1) {
                sound_song(hero.dead ? SONG_OVER : SONG_NONE);
                if (hero.dead)
                    update_record(0);
            }
            if (boss_dead && end_t > 2 * ZXV_FRAME_HZ) {
#ifdef KEEP_WHOLE
                if (demo)
                    save_stats(1);
#endif
                sound_song(SONG_WIN);
                victory(update_record(1));
                return 0;
            }
            if (hero.dead && (end_t > 6 * ZXV_FRAME_HZ ||
                              (end_t > 2 * ZXV_FRAME_HZ && (input_edge & IN_ATTACK)))) {
#ifdef KEEP_WHOLE
                if (demo)
                    save_stats(0);
#endif
                return 0;
            }
        } else {
            int in = demo ? bot_input() : (input_now & ~(IN_ATTACK | IN_POTION)) |
                                          (input_edge & (IN_ATTACK | IN_POTION));
            if (in & IN_POTION)
                drink();
            hero_update(in);
            if (!msg_on)
                actors_update();
            items_update();
            play_frames++;
            if (level_up_t)
                level_up_t--;
            if (hero_event == EV_STAIRS && cur_floor < LAST_FLOOR) {
                sound_fx(FX_STAIRS);
                start_floor(cur_floor + 1);
                continue;
            }
            int rx = hero_room_x(), ry = hero_room_y();
            if (rx != room_x || ry != room_y) {
                from_x = room_x * ROOM_W * TILE;
                from_y = room_y * ROOM_H * TILE;
                enter_room(rx, ry);
                slide = SLIDE;
                hero.attack = 0;
            }
        }
        video_begin();
        draw_world();
        hud();
        if (hero.dead)                  /* un recuadro a la vez: el final o el mensaje */
            banner(T("FIN DE LA PARTIDA", "GAME OVER"), (frame & 8) ? 15 : 10);
        else if (msg_on)
            draw_message();
        frame_end();
    }
}

/* El título: la torre, el nombre y las teclas. 0 para jugar, 1 para la
 * demostración, -1 para salir. */
static int title(void)
{
    sound_song(SONG_TITLE);
    for (int t = 0;; t++) {
        read_input();
        if (input_edge & IN_ESC)
            return -1;
        if (t > ZXV_FRAME_HZ * 2 / 5 && (input_edge & IN_ATTACK))
            return 0;
        if (t > 12 * ZXV_FRAME_HZ)
            return 1;
        video_begin();
        zxv_gfx_rect(0, 0, W, H, 0);
        for (int i = 0; i < 20; i++) {  /* las estrellas, donde no hay letras */
            uint32_t s = (uint32_t)i * 2654435761u;
            zxv_gfx_rect(150 + (int)(s % 100), 64 + (int)((s >> 12) % 34), 1, 1, (t / 20 + i) % 7 ? 7 : 15);
        }
        draw_sprite(&spr[S_TOWER], W - 64, H - 88, 0, 0);
        int y = 14 + isin(t / 2) / 32;
        text_big((W - 8 * 6 * 4) / 2, y, "THE KEEP", 4, 14);
        text_center(52, T("UNA AVENTURA PARA ZX-V", "AN ADVENTURE FOR ZX-V"), 13);
        char line[40];
        if (best_time)
            time_text(append(line, T("MEJOR TIEMPO ", "BEST TIME ")), best_time);
        else
            append(line, T("MEJOR TIEMPO --:--", "BEST TIME --:--"));
        text(12, 70, line, 14);
        char buf[8];
        number(buf, best_gold, 3);
        append(append(line, T("MÁS ORO ", "MOST GOLD ")), buf);
        text(12, 80, line, 14);
        text(12, 100, T("CURSORES U OPQA: ANDAR", "ARROWS OR OPQA: WALK"), 15);
        text(12, 110, T("ESPACIO: LA ESPADA", "SPACE: SWORD"), 15);
        text(12, 120, T("Z: UNA POCIÓN", "Z: DRINK A POTION"), 15);
        text(12, 130, T("INTRO: PAUSA  S: MÚSICA", "ENTER: PAUSE  S: MUSIC"), 15);
        text(12, 140, T("ESC: SALIR", "ESC: QUIT"), 15);
        if (t & 32)
            text(12, 164, T("PULSA ESPACIO O FUEGO", "PRESS SPACE OR FIRE"), 14);
        sound_frame();
        video_end();
        frame++;
    }
}

int main(void)
{
    lang = zxv_lang_get() == ZXV_LANG_ES ? 0 : 1;
    video_init();
    art_init();                         /* antes del sonido: sus informes, en su idioma */
    sound_init();
    load_record();
    video_init();                       /* otra vez: lo que tarda lo de antes no son frames perdidos */
    input_now = 0x1FF;                  /* lo que esté pulsado al empezar no cuenta */
    for (;;) {
        int r = title();
        if (r < 0)
            break;
        demo = r;
        frame = 0;
        rng_state = demo ? 4242 : (rng_state ^ *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES);
        play();
        demo = 0;
        actors_clear();
    }
    sound_off();
    video_done();
    zxv_set_mode(0);
    while (zxv_key_get() >= 0)
        ;
    return 0;                           /* el récord ya está en la tarjeta */
}
