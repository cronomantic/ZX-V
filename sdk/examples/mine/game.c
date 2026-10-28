/*
 * game.c - The Mine: un juego de cavar, de piedras y diamantes, para ZX-V.
 *
 * El minero tiene que juntar los diamantes que pide cada cueva antes de que
 * se acabe el tiempo; entonces se abre la salida. Las piedras y los
 * diamantes caen y ruedan, y lo que cae encima, mata. Hay ascuas y
 * murciélagos que siguen las paredes, musgo que crece y un tamiz que cambia
 * piedras por diamantes (ver cave.c).
 *
 * El mundo avanza a pasos de 5 o 6 frames, pero lo que se mueve se dibuja
 * entre una celda y otra, frame a frame, y la ventana de 256 x 176 sigue al
 * minero píxel a píxel por la cueva de 640 x 352. Todo se dibuja cada frame
 * en la página oculta (doble buffer, a 50 imágenes por segundo).
 *
 * Controles: cursores o OPQA para moverse, espacio o M con una dirección
 * para cavar o coger sin moverse, Intro para la pausa y Esc para salir; o
 * un mando. Sin tocar nada en el título, empieza una demostración que juega
 * sola. El récord se guarda en SD:mine.txt (en la tarjeta, aunque el juego
 * se cargue desde ROM:).
 *
 * Compilado con -DMINA_STATS (make EXTRA_CFLAGS=-DMINA_STATS), el marcador
 * enseña los frames perdidos.
 *
 * Con -DMINA_BOT (y zxv-cli --test), sin título: el piloto de la
 * demostración juega las doce cuevas seguidas, con todo su dibujo, y al
 * final de cada una escribe si la ha terminado, en cuántos segundos, las
 * vidas que ha perdido y los frames perdidos.
 */
#include "mine.h"


uint32_t rng_state = 1;
int lang;

#ifdef MINA_BOT
#define MINA_STATS
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
    char s[12];
    int n = 0;
    do {
        s[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    char r[12];
    for (int i = 0; i < n; i++)
        r[i] = s[n - 1 - i];
    r[n] = 0;
    bot_write(r);
}
#endif

/* --- La entrada -------------------------------------------------------------------- */

enum { IN_UP = 1, IN_DOWN = 2, IN_LEFT = 4, IN_RIGHT = 8, IN_FIRE = 16, IN_PAUSE = 32, IN_ESC = 64 };

static int key(unsigned hid)
{
    const volatile uint8_t *map = (const volatile uint8_t *)(uintptr_t)ZXV_I_KEYMAP;
    return (map[hid >> 3] >> (hid & 7)) & 1;
}

static int input_now, input_edge, last_dir = D_NONE, tap_dir = D_NONE;

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
    /* La última dirección pulsada manda; una pulsación corta no se pierde. */
    static const uint8_t bit_dir[4][2] = { { IN_UP, D_UP }, { IN_RIGHT, D_RIGHT }, { IN_DOWN, D_DOWN }, { IN_LEFT, D_LEFT } };
    for (int k = 0; k < 4; k++)
        if (input_edge & bit_dir[k][0])
            last_dir = tap_dir = bit_dir[k][1];
    /* Lo tecleado no le sirve al juego: que no le llegue al BASIC al volver. */
    while (zxv_key_get() >= 0)
        ;
}

/* La dirección de este paso. */
static int player_dir(void)
{
    static const uint8_t dir_bit[4] = { IN_UP, IN_RIGHT, IN_DOWN, IN_LEFT };
    int d = D_NONE;
    if (last_dir != D_NONE && (input_now & dir_bit[last_dir]))
        d = last_dir;
    else
        for (int k = 0; k < 4 && d == D_NONE; k++)
            if (input_now & dir_bit[k])
                d = k;
    if (d == D_NONE)
        d = tap_dir;
    tap_dir = D_NONE;
    return d;
}

/* --- La partida -------------------------------------------------------------------- */

static world_t wd;
static uint8_t moved_dir[CW * CH];      /* 1 + la dirección con la que llegó en el último paso */
static int sub, frame, demo, cam_x, cam_y, flash, reveal, paused;
static int time_left, time_frames, miner_dir, dead_t, idle_t;
static int lives, cave_n;
static uint32_t score, record, next_life;
static const char *msg, *msg2;
static int msg_t;

#define RECORD_FILE "SD:mine.txt"

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
        return;                         /* sin tarjeta: el récord dura lo que la sesión */
    zxv_file_write(f, buf, 9);
    zxv_file_close(f);
}

/* Al acabar la partida (o salir de ella): ¿récord? */
static int end_game(void)
{
    if (demo || score <= record)
        return 0;
    record = score;
    save_record();
    return 1;
}

static void add_score(uint32_t v)
{
    if (demo)
        return;
    score += v;
    if (score >= next_life) {
        next_life += 500;
        lives++;
        sound_fx(FX_LIFE);
    }
}

/* El píxel del minero en la cueva, entre una celda y otra. */
static int slide(int d, int axis)
{
    static const int8_t dx[4] = { 0, 1, 0, -1 }, dy[4] = { -1, 0, 1, 0 };
    int off = CELL * (wd.tick - 1 - sub) / wd.tick;
    return -(axis ? dy[d] : dx[d]) * off;
}

static void miner_px(int *x, int *y)
{
    *x = (wd.mi % CW) * CELL;
    *y = (wd.mi / CW) * CELL;
    if (moved_dir[wd.mi]) {
        *x += slide(moved_dir[wd.mi] - 1, 0);
        *y += slide(moved_dir[wd.mi] - 1, 1);
    }
}

/* La ventana sigue al minero cuando se aleja del centro. */
static void camera(int snap)
{
    int px, py;
    miner_px(&px, &py);
    px += CELL / 2;
    py += CELL / 2;
    int tx = cam_x, ty = cam_y;
    if (snap) {
        tx = px - W / 2;
        ty = py - VIEW_H / 2;
    }
    if (px - tx < W / 2 - 40) tx = px - (W / 2 - 40);
    if (px - tx > W / 2 + 40) tx = px - (W / 2 + 40);
    if (py - ty < VIEW_H / 2 - 24) ty = py - (VIEW_H / 2 - 24);
    if (py - ty > VIEW_H / 2 + 24) ty = py - (VIEW_H / 2 + 24);
    int mx = wd.w * CELL - W, my = wd.h * CELL - VIEW_H;
    cam_x = tx < 0 ? 0 : tx > mx ? (mx > 0 ? mx : 0) : tx;
    cam_y = ty < 0 ? 0 : ty > my ? (my > 0 ? my : 0) : ty;
}

/* --- El dibujo ------------------------------------------------------------------- */

static int cell_tile(int t, int i)
{
    switch (t) {
    case O_EMPTY: return flash ? T_FLASH : T_EMPTY;
    case O_DIRT: return T_DIRT;
    case O_WALL: return T_WALL;
    case O_STEEL: return T_STEEL;
    case O_BOULDER:
    case O_BOULDER_F: return moved_dir[i] ? T_EMPTY : T_BOULDER;
    case O_DIAMOND:
    case O_DIAMOND_F: return moved_dir[i] ? T_EMPTY : T_DIAMOND + (((frame >> 2) + (i ^ (i >> 3))) & 3);
    case O_ENTRY: return T_ENTRY + ((frame >> 3) & 1);
    case O_EXIT: return T_EXIT;
    case O_EXIT_OPEN: return T_EXIT_OPEN + ((frame >> 3) & 1);
    case O_MOSS: return T_MOSS + (((frame >> 4) + i) & 1);
    case O_SIEVE: return wd.sieve == SV_ACTIVE ? T_SIEVE_ON + ((frame >> 1) & 1) : T_SIEVE;
    default:
        if (t >= O_BOOM)
            return (T_BOOM + ((t - O_BOOM) & 3)) | (i & 1 ? ZXV_CELL_FLIP_H : 0) | ((i / CW) & 1 ? ZXV_CELL_FLIP_V : 0);
        return T_EMPTY;                 /* el minero y los bichos son sprites */
    }
}

static void draw_hud(void)
{
    char buf[16];
    zxv_gfx_rect(0, 0, W, HUD_H, 1);
    zxv_gfx_rect(0, HUD_H - 1, W, 1, 9);
    draw_sprite(&s_icon[0], 3, 4, 0, 0);
    number(buf, (uint32_t)wd.got, 2);
    buf[2] = '/';
    number(buf + 3, (uint32_t)wd.need, 2);
    text(13, 5, buf, wd.open ? 14 : 15);
    draw_sprite(&s_icon[1], 50, 4, 0, 0);
    number(buf, (uint32_t)time_left, 3);
    text(60, 5, buf, time_left <= 10 && (frame & 8) ? 10 : 15);
    draw_sprite(&s_icon[2], 86, 4, 0, 0);
    number(buf, (uint32_t)(lives > 9 ? 9 : lives), 1);
    text(96, 5, buf, 15);
#ifdef MINA_STATS
    number(buf, video_lost(), 4);       /* los frames perdidos */
    text(108, 5, buf, 10);
#else
    const char *c = T("CUEVA ", "CAVE ");
    int n = 0;
    while (*c)
        buf[n++] = *c++;
    if (cave_n >= 9)
        buf[n++] = (char)('0' + (cave_n + 1) / 10);
    buf[n++] = (char)('0' + (cave_n + 1) % 10);
    buf[n] = 0;
    text(110, 5, buf, caves[cave_n].bonus ? 14 : 13);
#endif
    number(buf, demo ? record : score, 6);
    text(W - 39, 5, buf, demo ? 6 : 15);
}

static void draw_cave(void)
{
    /* Las celdas del mapa, solo las que se ven. */
    int x0 = cam_x / CELL, y0 = cam_y / CELL, x1 = (cam_x + W - 1) / CELL, y1 = (cam_y + VIEW_H - 1) / CELL;
    int ex = wd.mi % CW, ey = wd.mi / CW;
    for (int y = y0; y <= y1 && y < CH; y++)
        for (int x = x0; x <= x1 && x < CW; x++) {
            int i = y * CW + x, t = cell_tile(wd.g[i], i);
            if (reveal < 64 && (x - ex) * (x - ex) + (y - ey) * (y - ey) > reveal * reveal / 4)
                t = T_STEEL;            /* la cueva se destapa desde la entrada */
            view[i] = (uint16_t)t;
        }
    zxv_gfx_clip(0, HUD_H, W, H);
    zxv_map_draw(&cave_map, cam_x, cam_y, 0, HUD_H, W, VIEW_H, ZXV_MAP_ALL);

    /* Lo que se mueve, entre su celda de antes y la de ahora. */
    for (int y = y0 - 1; y <= y1 + 1; y++) {
        if (y < 0 || y >= CH)
            continue;
        for (int x = x0 - 1; x <= x1 + 1; x++) {
            if (x < 0 || x >= CW)
                continue;
            int i = y * CW + x, t = wd.g[i], spr = -1;
            if (t >= O_BOULDER && t <= O_DIAMOND_F && moved_dir[i])
                spr = t <= O_BOULDER_F ? T_BOULDER : T_DIAMOND + ((frame >> 2) & 3);
            else if (t >= O_EMBER && t < O_BAT)
                spr = T_EMBER + ((frame >> 2) & 3);
            else if (t >= O_BAT && t < O_BOOM)
                spr = T_BAT + ((frame >> 2) & 3);
            if (spr < 0 || reveal < 64)
                continue;
            int px = x * CELL - cam_x, py = y * CELL - cam_y + HUD_H;
            if (moved_dir[i]) {
                px += slide(moved_dir[i] - 1, 0);
                py += slide(moved_dir[i] - 1, 1);
            }
            draw_sprite(&s_tile[spr], px, py, 0, 0);
        }
    }

    /* El minero. */
    if (wd.state == ST_PLAY && wd.g[wd.mi] == O_MINER) {
        int px, py, f = M_STAND, flip = wd.facing == D_LEFT ? ZXV_FLIP_H : 0;
        miner_px(&px, &py);
        if (moved_dir[wd.mi]) {
            int d = moved_dir[wd.mi] - 1, step = sub * 2 >= wd.tick;
            if (d == D_LEFT || d == D_RIGHT)
                f = step ? M_WALK2 : M_WALK1;
            else
                f = step ? M_UP2 : M_UP1;
        } else if (wd.push) {
            f = M_PUSH;
        } else {
            flip = 0;
            if (idle_t % 150 > 140)
                f = M_BLINK;
        }
        draw_sprite(&s_miner[f], px - cam_x, py - cam_y + HUD_H, flip, 0);
    } else if (wd.state == ST_OUT) {    /* en la salida, contento */
        draw_sprite(&s_miner[M_CHEER], (wd.mi % CW) * CELL - cam_x, (wd.mi / CW) * CELL - cam_y + HUD_H - (frame & 4) / 2, 0, 0);
    }
    zxv_gfx_clip(0, 0, W, H);

    draw_hud();
    if (msg_t && msg) {
        label(70, msg, (frame & 8) ? 15 : 14);
        if (msg2)
            label(84, msg2, 13);
    }
    if (paused)
        label(90, T("PAUSA", "PAUSED"), (frame & 16) ? 15 : 5);
    if (demo)
        label(H - 12, T("DEMOSTRACIÓN", "DEMO"), 13);
}

/* --- Un paso ----------------------------------------------------------------- */

static void step_world(void)
{
    int grab = 0, dir;
    if (demo)
        dir = bot_input(&wd, &grab);
    else {
        dir = player_dir();
        grab = (input_now & IN_FIRE) != 0;
    }
    uint32_t before = wd.points;
    world_tick(&wd, dir, grab);
    add_score(wd.points - before);

    __builtin_memset(moved_dir, 0, sizeof(moved_dir));
    for (int k = 0; k < nmoves; k++)
        moved_dir[moves[k].at] = (uint8_t)(moves[k].dir + 1);
    if (moved_dir[wd.mi])
        miner_dir = moved_dir[wd.mi] - 1;
    idle_t = moved_dir[wd.mi] || wd.push ? 0 : idle_t;

    uint32_t ev = wd.ev;
    if (ev & EV_BOOM) sound_fx(FX_BOOM);
    else if (ev & EV_LAND) sound_fx(FX_LAND);
    else if (ev & EV_PUSH) sound_fx(FX_PUSH);
    else if (ev & EV_DIG) sound_fx(FX_DIG);
    if (ev & EV_OPEN) {
        sound_fx(FX_OPEN);
        flash = 3;
    } else if (ev & EV_CHANGE) {
        sound_fx(FX_MAGIC);
    } else if (ev & EV_BORN) {
        sound_fx(FX_BORN);
    } else if (ev & EV_COLLECT) {
        sound_fx(FX_DIAMOND);
    }
    if (ev & EV_SIEVE) sound_fx(FX_SIEVE);
    else if (ev & EV_TING) sound_fx(FX_TING);
}

/* --- Una cueva ------------------------------------------------------------------- */

enum { R_CLEAR, R_DEAD, R_QUIT };

static int run_cave(int n)
{
    const cave_t *c = &caves[n];
    cave_n = n;
    world_load(&wd, n);
    wd.rec = 1;
    nmoves = 0;
    __builtin_memset(moved_dir, 0, sizeof(moved_dir));
    art_cave(c);
    time_left = c->time;
    time_frames = 0;
    sub = 0;
    dead_t = 0;
    idle_t = 0;
    flash = 0;
    paused = 0;
    tap_dir = D_NONE;
    camera(1);

    /* El nombre de la cueva mientras se destapa. */
    static char title[32];
    const char *p = T("CUEVA ", "CAVE ");
    int k = 0;
    if (c->bonus)
        p = T("PREMIO: ", "BONUS: ");
    else {
        while (*p)
            title[k++] = *p++;
        if (n >= 9)
            title[k++] = (char)('0' + (n + 1) / 10);
        title[k++] = (char)('0' + (n + 1) % 10);
        title[k++] = ':';
        title[k++] = ' ';
        p = "";
    }
    while (*p)
        title[k++] = *p++;
    for (p = T(c->es, c->en); *p;)
        title[k++] = *p++;
    title[k] = 0;
    static char need[32];
    p = T("DIAMANTES: ", "DIAMONDS: ");
    for (k = 0; *p;)
        need[k++] = *p++;
    number(need + k, c->need, 2);
    msg = title;
    msg2 = need;
    msg_t = 120;

    for (reveal = 0;; frame++) {
        read_input();
        if ((input_edge & IN_ESC) || (demo && input_edge))
            return R_QUIT;
        if (input_edge & IN_PAUSE && !demo && reveal >= 64) {
            paused = !paused;
            sound_mute(paused);
        }
        if (msg_t)
            msg_t--;
        if (paused) {
            video_begin();
            draw_cave();
            video_end();
            continue;
        }
        if (reveal < 64) {
            reveal++;
        } else {
            if (sub == 0)
                step_world();
            if (++sub >= wd.tick)
                sub = 0;
            idle_t++;
            if (flash)
                flash--;

            /* El tiempo corre con el minero en la cueva. */
            if (wd.state == ST_PLAY && ++time_frames >= 50) {
                time_frames = 0;
                if (time_left > 0)
                    time_left--;
                if (time_left > 0 && time_left <= 10)
                    sound_fx(FX_TICK);
                if (time_left == 0) {
                    world_kill(&wd);
                    sound_fx(FX_BOOM);
                    msg = T("¡SIN TIEMPO!", "OUT OF TIME!");
                    msg2 = 0;
                    msg_t = 120;
                }
            }
        }
        if (wd.state == ST_PLAY || wd.state == ST_BORN)
            camera(0);
        video_begin();
        draw_cave();
        sound_frame();
        video_end();

        if (wd.state == ST_DEAD && ++dead_t > 110)
            return R_DEAD;
        if (wd.state == ST_OUT) {
            /* La bonificación por el tiempo que queda, segundo a segundo. */
            if (time_left > 0) {
                int s = time_left > 2 ? 2 : time_left;
                time_left -= s;
                add_score((uint32_t)s);
                sound_fx(FX_BONUS);
            } else if (++dead_t > 40) {
                return R_CLEAR;
            }
        }
#ifndef MINA_BOT
        if (demo && frame > 1500)
            return R_QUIT;
#endif
    }
}

/* Unos frames con un letrero sobre la cueva. Devuelve 1 si se pulsó Esc. */
static int banner(const char *a, const char *b, int frames)
{
    msg = 0;
    for (int t = 0; t < frames; t++, frame++) {
        read_input();
        video_begin();
        draw_cave();
        label(70, a, (t & 8) ? 15 : 14);
        if (b)
            label(84, b, 13);
        sound_frame();
        video_end();
        if (t > 40 && (input_edge & (IN_FIRE | IN_ESC | IN_PAUSE)))
            return (input_edge & IN_ESC) != 0;
    }
    return 0;
}

static void play(int start, int demo_mode)
{
    demo = demo_mode;
    rng_state ^= *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES;
    score = 0;
    lives = 3;
    next_life = 500;
    frame = 0;
    sound_song(demo ? SONG_TITLE : SONG_NONE);
#ifdef MINA_BOT
    int lost = 0;
#endif
    for (int n = start; n < ncaves;) {
#ifdef MINA_BOT
        int f0 = frame;
        uint32_t lost0 = video_lost();
        video_busy(1);
#endif
        int r = run_cave(n);
#ifdef MINA_BOT
        if (r == R_DEAD) {
            lost++;
            if (lost < 6 && !caves[n].bonus) {  /* lo vuelve a intentar */
                bot_write("cueva ");
                bot_number(n + 1);
                bot_write(": muere y lo vuelve a intentar\n");
                continue;
            }
        }
        bot_write("cueva ");
        bot_number(n + 1);
        bot_write(r == R_CLEAR ? ": superada" : ": SIN TERMINAR");
        bot_write(", segundos ");
        bot_number((frame - f0) / 50);
        bot_write(", diamantes ");
        bot_number(wd.got);
        bot_write("/");
        bot_number(wd.need);
        bot_write(", vidas perdidas ");
        bot_number(lost);
        bot_write(", frames perdidos ");
        bot_number((int)(video_lost() - lost0));
        bot_write(", el frame más lento ");
        bot_number((int)video_busy(1));
        bot_write(" us");
        bot_write("\n");
        lost = 0;
        if (r != R_CLEAR && !caves[n].bonus)
            bot_exit(1);
        n++;
        continue;
#endif
        if (r == R_QUIT) {
            end_game();
            return;
        }
        if (demo)                       /* la demostración: una cueva */
            return;
        if (r == R_DEAD) {
            if (!caves[n].bonus && --lives <= 0) {
                sound_song(SONG_OVER);
                int better = end_game();
                banner(T("FIN DE LA PARTIDA", "GAME OVER"), better ? T("¡NUEVO RÉCORD!", "NEW HIGH SCORE!") : 0, 300);
                return;
            }
            if (caves[n].bonus)
                n++;
            continue;
        }
        sound_song(SONG_CLEAR);
        if (n + 1 < ncaves && banner(caves[n].bonus ? T("¡PREMIO!", "BONUS!") : T("¡CUEVA SUPERADA!", "CAVE CLEAR!"), 0, 120)) {
            end_game();
            return;
        }
        n++;
    }
    /* Todas: la bonificación por las vidas que quedan. */
    add_score((uint32_t)lives * 100);
    sound_song(SONG_WIN);
    int better = end_game();
    banner(T("¡LA MINA ES TUYA!", "THE MINE IS YOURS!"), better ? T("¡NUEVO RÉCORD!", "NEW HIGH SCORE!") : 0, 400);
}

/* --- El título ---------------------------------------------------------------- */

#define STARTS      4                   /* se puede empezar en las cuatro primeras */

/* El título: la cueva elegida (0..), STARTS + n para la demostración, -1 para salir. */
static int title(int *choice)
{
    static const uint8_t logo[7] = { 15, 14, 14, 6, 6, 2, 2 };
    static int demos;
    sound_song(SONG_TITLE);
    int idle = 0;
    for (int t = 0;; t++) {
        read_input();
        if (input_edge & IN_ESC)
            return -1;
        if (input_now)
            idle = 0;
        if (t > 20 && (input_edge & IN_FIRE))
            return *choice;
        if (input_edge & IN_LEFT)
            *choice = (*choice + STARTS - 1) % STARTS;
        if (input_edge & IN_RIGHT)
            *choice = (*choice + 1) % STARTS;
        if (++idle > 600)
            return STARTS + demos++ % 3;

        video_begin();
        zxv_gfx_rect(0, 0, W, H, 0);
        const char *name = "THE MINE";
        text_big((W - text_width(name) * 4) / 2, 12, name, 4, logo);

        /* Una galería: el minero cava una veta de diamantes. */
        uint16_t band[16 * 3];
        int mx = (t * 2) % (W + 96) - 48;
        for (int x = 0; x < 16; x++) {
            band[x] = (x * 5) % 7 == 2 ? T_BOULDER : T_STEEL;
            band[16 + x] = x * CELL + 8 < mx ? T_EMPTY : x % 3 == 1 ? T_DIAMOND + (((t >> 2) + x) & 3) : T_DIRT;
            band[32 + x] = T_WALL;
        }
        draw_tiles(band, 16, 3, 0, 46);
        draw_sprite(&s_miner[(t >> 2) & 1 ? M_WALK1 : M_WALK2], mx, 62, 0, 0);

        text_center(98, T("UN JUEGO DE CAVAR PARA ZX-V", "A DIGGING GAME FOR ZX-V"), 5);
        char buf[16], line[40];
        number(buf, record, 6);
        const char *l = T("RÉCORD ", "HIGH SCORE ");
        int n = 0;
        while (*l)
            line[n++] = *l++;
        for (int i = 0; buf[i]; i++)
            line[n++] = buf[i];
        line[n] = 0;
        text_center(110, line, 14);
        text_center(126, T("CURSORES O OPQA: MOVERSE", "ARROWS OR OPQA: MOVE"), 7);
        text_center(136, T("ESPACIO + DIRECCIÓN: CAVAR SIN MOVERSE", "SPACE + DIRECTION: DIG WITHOUT MOVING"), 7);
        text_center(146, T("INTRO: PAUSA   ESC: SALIR", "ENTER: PAUSE   ESC: QUIT"), 7);

        /* La cueva de salida. */
        n = 0;
        line[n++] = '<';
        line[n++] = ' ';
        for (l = T("CUEVA ", "CAVE "); *l;)
            line[n++] = *l++;
        line[n++] = (char)('1' + *choice);
        line[n++] = ':';
        line[n++] = ' ';
        for (l = T(caves[*choice].es, caves[*choice].en); *l;)
            line[n++] = *l++;
        line[n++] = ' ';
        line[n++] = '>';
        line[n] = 0;
        text_center(162, line, 13);
        if (t & 32)
            text_center(178, T("PULSA ESPACIO O FUEGO", "PRESS SPACE OR FIRE"), 15);
        sound_frame();
        video_end();
    }
}

int main(void)
{
    lang = zxv_lang_get() == ZXV_LANG_ES ? 0 : 1;
    video_init();
    sound_init();
    art_init();
    load_record();
    input_now = 0x7F;                   /* lo que esté pulsado al empezar no cuenta */
#ifdef MINA_BOT
    play(0, 1);
    bot_exit(0);
#endif
    int choice = 0;
    for (;;) {
        int r = title(&choice);
        if (r < 0)
            break;
        if (r >= STARTS)
            play(r - STARTS, 1);
        else
            play(r, 0);
    }
    sound_off();
    video_done();
    zxv_set_mode(0);
    while (zxv_key_get() >= 0)
        ;
    return 0;                                   /* el récord ya está en la tarjeta */
}
