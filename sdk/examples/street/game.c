/*
 * game.c - Street Justice: un juego de peleas por las calles para ZX-V.
 *
 * Se anda hacia la derecha por tres fases, la calle de noche, el mercado y
 * las azoteas, peleando con los matones de las bandas. La pantalla se queda
 * quieta hasta acabar con cada grupo; entonces «GO» deja seguir. Al final de
 * cada fase espera el jefe de la banda. Todo se dibuja cada frame en la
 * página oculta y se cambia de página en el VBLANK (doble buffer, a 50
 * imágenes por segundo).
 *
 * Controles: cursores o OPQA para andar, espacio o M para pegar, N o Z para
 * saltar (y en el aire, patada), los dos a la vez para la patada giratoria;
 * Intro para la pausa y Esc para salir; o un mando. Andando contra un
 * enemigo se le agarra: entonces pegar da rodillazos y saltar lo lanza.
 * Sin tocar nada en el título, empieza una demostración que juega sola. El
 * récord se guarda en SD:street.txt (en la tarjeta, aunque el juego se
 * cargue desde ROM:).
 *
 * Con -DSTREET_STATS, el marcador enseña los frames perdidos.
 *
 * Con -DSTREET_BOT=semilla, para medir la dificultad (con zxv-cli --test):
 * sin título, el piloto de la demostración juega las tres fases desde el
 * principio y recibe los golpes como un jugador, pero no se queda sin
 * vidas; al final de cada fase escribe las vidas que ha perdido, cuántas con
 * el jefe, los golpes recibidos, lo que ha tardado y los frames perdidos.
 *
 * main() devuelve 0.
 */
#include "street.h"

uint32_t rng_state = 1;
int lang, demo, frame, stage, cam, cam_max;

#ifdef STREET_BOT
#define STREET_STATS
#endif

#if defined(STREET_BOT) || defined(STREET_TRACE)
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

static __attribute__((unused)) void bot_exit(int code)
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
#endif

const int8_t sin64[64] = {
    0, 6, 12, 19, 24, 30, 36, 41, 45, 49, 53, 56, 59, 61, 63, 64, 64, 64, 63, 61, 59, 56,
    53, 49, 45, 41, 36, 30, 24, 19, 12, 6, 0, -6, -12, -19, -24, -30, -36, -41, -45, -49,
    -53, -56, -59, -61, -63, -64, -64, -64, -63, -61, -59, -56, -53, -49, -45, -41, -36,
    -30, -24, -19, -12, -6,
};

int isqrt(int v)
{
    int r = 0;
    for (int b = 1 << 14; b; b >>= 1)
        if ((r + b) * (r + b) <= v)
            r += b;
    return r;
}

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
        in |= IN_A;                     /* espacio, M */
    if (key(0x11) || key(0x1D))
        in |= IN_B;                     /* N, Z */
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
        if (joy & ZXV_JOY_FIRE1) in |= IN_A;
        if (joy & ZXV_JOY_FIRE2) in |= IN_B;
        if (joy & ZXV_JOY_START) in |= IN_PAUSE;
    }
    input_edge = in & ~input_now;
    input_now = in;
    /* Lo tecleado no le sirve al juego: que no le llegue al BASIC al volver. */
    while (zxv_key_get() >= 0)
        ;
}

/* --- Las fases ------------------------------------------------------------------- */

typedef struct {
    uint8_t kind, right, delay;         /* delay en 1/12,5 s (de 4 en 4 frames) */
} foe_def;

typedef struct {
    uint16_t at;                        /* el píxel de la cámara en el que se para */
    uint8_t n;
    foe_def foes[6];
} group_t;

typedef struct {
    uint16_t x;
    uint8_t y, kind, drop;
} prop_def;

typedef struct {
    const char *name_es, *name_en;
    int song;
    const group_t *groups;
    int ngroups;
    const prop_def *props;
    int nprops;
} stage_t;

static const group_t groups1[] = {
    { 160, 2, { { K_PUNK, 1, 0 }, { K_PUNK, 0, 15 } } },
    { 448, 3, { { K_PUNK, 1, 0 }, { K_RUNNER, 0, 5 }, { K_PUNK, 1, 25 } } },
    { 768, 4, { { K_RUNNER, 1, 0 }, { K_RUNNER, 0, 5 }, { K_PUNK, 1, 15 }, { K_PUNK, 0, 30 } } },
    { 1088, 3, { { K_BRUTE, 1, 0 }, { K_PUNK, 0, 5 }, { K_RUNNER, 1, 20 } } },
    { 1408, 3, { { K_BOSS1, 1, 0 }, { K_PUNK, 0, 40 }, { K_PUNK, 1, 120 } } },
};
static const prop_def props1[] = {
    { 300, 140, PROP_BIN, FOOD_APPLE }, { 620, 134, PROP_BIN, 0 }, { 700, 170, PROP_BIN, FOOD_BREAD },
    { 1000, 136, PROP_BIN, FOOD_APPLE }, { 1300, 150, PROP_BIN, FOOD_CHICKEN },
};

static const group_t groups2[] = {
    { 160, 3, { { K_PUNK, 1, 0 }, { K_RUNNER, 0, 5 }, { K_PUNK, 0, 20 } } },
    { 448, 3, { { K_BRUTE, 1, 0 }, { K_RUNNER, 0, 5 }, { K_RUNNER, 1, 25 } } },
    { 768, 4, { { K_PUNK, 1, 0 }, { K_PUNK, 0, 5 }, { K_BRUTE, 1, 25 }, { K_RUNNER, 0, 40 } } },
    { 1088, 3, { { K_BRUTE, 1, 0 }, { K_BRUTE, 0, 30 }, { K_RUNNER, 1, 15 } } },
    { 1408, 3, { { K_BOSS2, 1, 0 }, { K_RUNNER, 0, 60 }, { K_RUNNER, 1, 120 } } },
};
static const prop_def props2[] = {
    { 250, 138, PROP_CRATE, FOOD_APPLE }, { 560, 172, PROP_CRATE, FOOD_BREAD }, { 900, 140, PROP_CRATE, FOOD_APPLE },
    { 1180, 170, PROP_CRATE, FOOD_CHICKEN }, { 1330, 136, PROP_CRATE, FOOD_BREAD },
};

static const group_t groups3[] = {
    { 160, 3, { { K_RUNNER, 1, 0 }, { K_RUNNER, 0, 5 }, { K_PUNK, 1, 20 } } },
    { 448, 4, { { K_BRUTE, 1, 0 }, { K_PUNK, 0, 5 }, { K_PUNK, 1, 20 }, { K_RUNNER, 0, 40 } } },
    { 768, 3, { { K_BRUTE, 1, 0 }, { K_BRUTE, 0, 25 }, { K_RUNNER, 1, 45 } } },
    { 1088, 5, { { K_PUNK, 1, 0 }, { K_PUNK, 0, 5 }, { K_RUNNER, 1, 20 }, { K_BRUTE, 0, 40 }, { K_RUNNER, 1, 60 } } },
    { 1408, 4, { { K_BOSS3, 1, 0 }, { K_PUNK, 0, 50 }, { K_BRUTE, 1, 150 }, { K_RUNNER, 0, 220 } } },
};
static const prop_def props3[] = {
    { 260, 136, PROP_BARREL, FOOD_BREAD }, { 600, 170, PROP_BARREL, FOOD_APPLE }, { 940, 140, PROP_BARREL, FOOD_CHICKEN },
    { 1200, 170, PROP_BARREL, FOOD_BREAD }, { 1350, 140, PROP_BARREL, FOOD_CHICKEN },
};

#define N(a) (int)(sizeof(a) / sizeof((a)[0]))

static const stage_t stages[3] = {
    { "LA CALLE", "DOWNTOWN", SONG_STREET, groups1, N(groups1), props1, N(props1) },
    { "EL MERCADO", "THE MARKET", SONG_MARKET, groups2, N(groups2), props2, N(props2) },
    { "LAS AZOTEAS", "THE ROOFTOPS", SONG_ROOFS, groups3, N(groups3), props3, N(props3) },
};

#define STAGE_END   (26 * SLOT_W - W)   /* el final de la calle, para la cámara */

static int group_i, group_on, group_t0, group_out, go_t, props_i, boss_on;
static uint32_t record;
static int message_t;
static const char *message, *message2;

/* --- El marcador ------------------------------------------------------------------- */

static void bar(int x, int y, int w, int v, int max, int c)
{
    zxv_gfx_rect(x - 1, y - 1, w + 2, 6, 7);
    zxv_gfx_rect(x, y, w, 4, 1);
    if (v > 0)
        zxv_gfx_rect(x, y, w * v / max, 4, c);
}

static void draw_hud(void)
{
    char buf[16];
    zxv_gfx_rect(0, 0, W, HUD_H, 0);
    const fighter_t *p = &fighters[0];
    text(2, 2, kind_name(K_HERO), 15);
    int hp = p->hp > 0 ? p->hp : 0;
    bar(24, 3, 64, hp, p->maxhp, hp > p->maxhp / 2 ? 12 : hp > p->maxhp / 4 ? 14 : 10);
    for (int i = 0; i < lives - 1 && i < 5; i++) {      /* las vidas de reserva */
        zxv_gfx_circle(96 + i * 8, 4, 2, 6, 1);
        zxv_gfx_rect(95 + i * 8, 2, 3, 1, 2);
    }
    number(buf, demo ? 0 : score, 6);
    text(W - 38, 2, buf, 15);
    /* Debajo: el último enemigo golpeado (o el jefe), o el récord. */
    int f = -1;
    for (int i = 1; i < MAX_F; i++)
        if (fighters[i].alive && fighters[i].kind >= K_BOSS1 && fighters[i].hp > 0)
            f = i;
    if (f < 0 && last_foe_t && fighters[last_foe].alive)
        f = last_foe;
    if (f >= 0) {
        const fighter_t *e = &fighters[f];
        text(2, 10, kind_name(e->kind), e->kind >= K_BOSS1 ? 10 : 13);
        bar(64, 11, 64, e->hp, e->maxhp, 10);
    }
    number(buf, record > score || demo ? record : score, 6);
    text(W - 38, 10, buf, 6);
    text(W - 38 - 6 * 4, 10, T("REC", "HI"), 6);
#ifdef STREET_STATS
    number(buf, video_lost(), 4);       /* los frames perdidos */
    text(140, 2, buf, 2);
#endif
}

/* La flecha de seguir. */
static void draw_go(void)
{
    if (!(frame & 16))
        return;
    text(W - 40, 50, "GO", 14);
    int16_t a[6] = { W - 24, 46, W - 14, 53, W - 24, 60 };
    zxv_gfx_rect(W - 34, 51, 10, 4, 14);
    zxv_gfx_tri(a, 14);
}

static void draw_world(void)
{
    scenery_draw(cam);
    fight_draw();
    draw_hud();
    if (go_t)
        draw_go();
    if (message_t && message) {
        label(64, message, (frame & 8) ? 15 : 14);
        if (message2)
            label(78, message2, 13);
    }
    if (demo && (frame & 32))
        label(H - 10, T("DEMOSTRACIÓN", "DEMO"), 13);
}

/* --- La partida ------------------------------------------------------------------- */

enum { R_CLEAR, R_OVER, R_QUIT };

static void stage_start(void)
{
    fight_reset();
    scenery_make(stage);
    cam = 0;
    group_i = 0;
    group_on = 0;
    go_t = 0;
    props_i = 0;
    boss_on = 0;
    cam_max = stages[stage].groups[0].at;
    hero_spawn(40, 156);
    message_t = 150;
    static char title[24];
    char *p = title;
    const char *f = T("FASE ", "STAGE ");
    while (*f)
        *p++ = *f++;
    *p++ = (char)('1' + stage);
    *p = 0;
    message = title;
    message2 = T(stages[stage].name_es, stages[stage].name_en);
    sound_song(stages[stage].song);
    video_sync();                       /* el decorado tarda: no es un frame perdido */
}

/* Los del grupo que aún no han salido: salen cuando les toca. */
static void group_frame(void)
{
    const stage_t *st = &stages[stage];
    while (props_i < st->nprops && st->props[props_i].x < cam + W + 40) {
        const prop_def *d = &st->props[props_i++];
        item_spawn(d->kind, d->x, d->y, d->drop);
    }
    if (!group_on) {
        if (group_i < st->ngroups && cam >= st->groups[group_i].at) {
            group_on = 1;
            group_t0 = frame;
            group_out = 0;
            go_t = 0;
            if (group_i == st->ngroups - 1) {
                boss_on = 1;
                sound_song(SONG_BOSS);
                message = T("¡EL JEFE!", "THE BOSS!");
                message2 = 0;
                message_t = 90;
            }
        }
        return;
    }
    const group_t *g = &st->groups[group_i];
    int pending = 0;
    for (int k = 0; k < g->n; k++) {
        const foe_def *d = &g->foes[k];
        int when = d->delay * 4;
        if (group_out & (1 << k))
            continue;
        pending = 1;
        if (frame - group_t0 >= when) {
            int y = BAND_TOP + 8 + (int)(rnd() % (BAND_BOTTOM - BAND_TOP - 16));
            if (foe_spawn(d->kind, d->right ? cam + W + 24 : cam - 24, y))
                group_out |= 1 << k;    /* si no cabe, sale más tarde */
        }
    }
    if (!pending && !foes_left()) {     /* el grupo, fuera */
        group_on = 0;
        group_i++;
        if (group_i < st->ngroups) {
            cam_max = st->groups[group_i].at;
            go_t = 200;
            sound_fx(FX_GO);
        }
    }
}

static void update(int in)
{
    frame++;
    if (message_t)
        message_t--;
    if (go_t)
        go_t--;
    fight_update(in);
    group_frame();
    /* La cámara: solo hacia delante, hasta donde deje el grupo. */
    int want = fighters[0].x / FX - 110;
    if (want > cam_max)
        want = cam_max;
    if (want > cam)
        cam += want - cam > 2 ? 2 : want - cam;
}

#ifdef STREET_BOT
static int bot_boss_f;
#endif

/* Unos frames con un letrero (fin de la partida, fase superada...). Devuelve 1 si se pulsó Esc. */
static int banner(const char *a, const char *b, int frames)
{
    for (int t = 0; t < frames; t++) {
        read_input();
        video_begin();
        draw_world();
        label(64, a, (t & 8) ? 15 : 14);
        if (b)
            label(78, b, 13);
        sound_frame();
        video_end();
        frame++;
#ifndef STREET_BOT
        if (t > 50 && (input_edge & (IN_A | IN_B | IN_ESC)))
            return (input_edge & IN_ESC) != 0;
#endif
    }
    return 0;
}

static int run_stage(void)
{
    stage_start();
    for (;;) {
        read_input();
#ifndef STREET_BOT
        if (demo && (input_edge & (IN_A | IN_B | IN_ESC | IN_PAUSE)))
            return R_QUIT;
        if (demo && frame > 1500)
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
        int in_now = demo ? autopilot(go_t || (!group_on && cam < cam_max)) : input_now;
        update(in_now);
#ifdef STREET_TRACE
        if (frame % 10 == 0 || (frame > 1500 && frame < 1512)) {
            bot_write("in ");
            bot_number(in_now);
            bot_write(" x16 ");
            bot_number(fighters[0].x);
            bot_write(" vx ");
            bot_number(fighters[0].vx + 1000);
            bot_write(" fl ");
            bot_number(fighters[0].face_left);
            bot_write(" st ");
            bot_number(fighters[0].state);
            bot_write(" t ");
            bot_number(fighters[0].t);
            bot_write(" | ");
            bot_number(frame);
            bot_write(" cam ");
            bot_number(cam);
            for (int i = 0; i < MAX_F; i++)
                if (fighters[i].alive) {
                    const fighter_t *f = &fighters[i];
                    bot_write(" | ");
                    bot_number(f->kind);
                    bot_write(":");
                    bot_number(f->state);
                    bot_write(" x");
                    bot_number(f->x / FX);
                    bot_write(" y");
                    bot_number(f->y / FX);
                    bot_write(" hp");
                    bot_number(f->hp < 0 ? 0 : f->hp);
                    bot_write(f->engaged ? " E" : "");
                }
            for (int i = 0; i < MAX_ITEMS; i++)
                if (things[i].alive) {
                    bot_write(" | item");
                    bot_number(things[i].kind);
                    bot_write(" x");
                    bot_number(things[i].x / FX);
                    bot_write(" y");
                    bot_number(things[i].y / FX);
                }
            bot_write("\n");
        }
#endif
        video_begin();
        draw_world();
        sound_frame();
        video_end();
#ifdef STREET_BOT
        bot_boss_f += boss_on;
        if (frame > 90000)              /* atascado: se da por perdida */
            return R_OVER;
#endif
        if (lives <= 0 && fighters[0].t > 60)
            return R_OVER;
        if (boss_on && !group_on && group_i >= stages[stage].ngroups)
            return R_CLEAR;
    }
}

#define RECORD_FILE "SD:street.txt"

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

/* El jugador celebra la victoria. */
static void hero_wins(void)
{
    fighter_t *p = &fighters[0];
    p->state = ST_WIN;
    p->t = 0;
    p->vx = p->vy = 0;
}

static void play(int demo_mode)
{
    static int demos;
    demo = demo_mode;
#ifdef STREET_BOT
    rng_state = STREET_BOT;
#else
    rng_state = demo ? 4242u + (uint32_t)demos : (rng_state ^ *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES);
#endif
    score = 0;
    lives = 3;
    frame = 0;
    fighters[0].alive = 0;              /* la vida, entera */
    for (stage = demo ? demos++ % 3 : 0; stage < 3; stage++) {
#ifdef STREET_BOT
        int f0 = frame;
        bot_deaths = bot_hits = bot_boss_deaths = bot_boss_f = 0;
        uint32_t lost0 = video_lost();
#endif
        int r = run_stage();
#ifdef STREET_BOT
        bot_write("fase ");
        bot_number(stage + 1);
        bot_write(r == R_CLEAR ? ": superada" : ": SIN TERMINAR");
        bot_write(", vidas perdidas ");
        bot_number(bot_deaths);
        bot_write(" (con el jefe ");
        bot_number(bot_boss_deaths);
        bot_write("), golpes recibidos ");
        bot_number(bot_hits);
        bot_write(", segundos ");
        bot_number((frame - f0) / 50);
        bot_write(" (el jefe ");
        bot_number(bot_boss_f / 50);
        bot_write("), frames perdidos ");
        bot_number((int)(video_lost() - lost0));
        bot_write("\n");
        if (r != R_CLEAR)
            bot_exit(1);
        continue;
#endif
        if (r == R_QUIT)
            return;
        if (demo)                       /* cada demostración, una fase */
            return;
        if (r == R_OVER) {
            sound_song(SONG_OVER);
            int better = score > record;
            if (better) {
                record = score;
                save_record();
            }
            banner(T("FIN DE LA PARTIDA", "GAME OVER"), better ? T("¡NUEVO RÉCORD!", "NEW HIGH SCORE!") : 0, 300);
            return;
        }
        add_score(5000 * (stage + 1) + fighters[0].hp * 20);
        hero_wins();
        sound_song(SONG_CLEAR);
        if (stage < 2 && banner(T("¡FASE SUPERADA!", "STAGE CLEAR!"), 0, 200))
            return;
    }
    /* Las tres fases: la bonificación por las vidas que quedan. */
    add_score(lives * 10000);
    int better = score > record;
    if (better) {
        record = score;
        save_record();
    }
    hero_wins();
    banner(T("¡LAS CALLES SON TUYAS!", "THE STREETS ARE YOURS!"), better ? T("¡NUEVO RÉCORD!", "NEW HIGH SCORE!") : 0,
           400);
}

/* El título: 0 para jugar, 1 para la demostración, -1 para salir. */
static int title(void)
{
    static const uint8_t logo[7] = { 15, 14, 14, 6, 10, 10, 2 };
    static const uint8_t logo2[7] = { 15, 15, 13, 13, 5, 5, 1 };
    sound_song(SONG_TITLE);
    scenery_make(0);
    video_sync();
    fight_reset();
    demo = 1;
    fighter_t *a = &fighters[0], *b = foe_spawn(K_PUNK, 0, 0);
    hero_spawn(0, 0);
    for (int t = 0;; t++) {
        read_input();
        if (input_edge & IN_ESC)
            return -1;
        if (t > 20 && (input_edge & (IN_A | IN_B)))
            return 0;
        if (t > 600)
            return 1;
        frame = t;
        cam = (t / 2) % STAGE_END;
        video_begin();
        scenery_draw(cam);
        zxv_gfx_rect(0, 0, W, HUD_H, 0);
        /* Los dos del título, uno frente al otro. */
        a->x = (cam + 92) * FX;
        a->y = 127 * FX;
        a->face_left = 0;
        b->x = (cam + 164) * FX;
        b->y = 127 * FX;
        b->face_left = 1;
        int beat = t % 120;
        a->state = beat < 60 ? ST_IDLE : ST_ATTACK;     /* en guardia, o un puñetazo */
        a->attack = 0;
        a->t = (int16_t)(beat - 60);
        b->state = beat >= 64 && beat < 82 ? ST_HURT : ST_IDLE;
        b->t = 0;
        b->flash = (uint8_t)(beat >= 64 && beat < 67 ? 3 : 0);
        fight_draw();
        zxv_gfx_rect(0, HUD_H, W, 60, 0);
        text_big((W - 6 * 6 * 4 + 4) / 2, 21 + isin(t) / 32, "STREET", 4, logo);
        text_big((W - 7 * 6 * 3 + 3) / 2, 53, "JUSTICE", 3, logo2);
        zxv_gfx_rect(0, 128, W, H - 128, 0);
        text_center(132, T("CURSORES O OPQA: ANDAR", "ARROWS OR OPQA: MOVE"), 7);
        text_center(141, T("ESPACIO O M: PEGAR   N O Z: SALTAR", "SPACE OR M: PUNCH   N OR Z: JUMP"), 7);
        text_center(150, T("INTRO: PAUSA   ESC: SALIR", "ENTER: PAUSE   ESC: QUIT"), 7);
        char buf[16], line[32];
        number(buf, record, 6);
        const char *lb = T("RÉCORD ", "HIGH SCORE ");
        int n = 0;
        while (*lb)
            line[n++] = *lb++;
        for (int i = 0; buf[i]; i++)
            line[n++] = buf[i];
        line[n] = 0;
        text_center(164, line, 14);
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
    input_now = 0xFF;                   /* lo que esté pulsado al empezar no cuenta */
#ifdef STREET_SHEET
    for (;;) {
        video_begin();
        zxv_gfx_rect(0, 0, W, H, 7);
        fight_reset();
        cam = 0;
        for (int i = 0; i < 22; i++) {
            fighter_t *f = &fighters[0];
            *f = (fighter_t){ 0 };
            f->alive = 1;
            f->kind = (uint8_t)(STREET_SHEET);
            f->held = -1;
            f->x = (20 + (i % 8) * 31) * FX;
            f->y = (60 + (i / 8) * 64) * FX;
            static const uint8_t st[22][3] = { {0,0,0},{1,0,0},{1,0,6},{1,0,12},{1,0,18},{2,0,3},{2,1,3},{2,2,5},
                {2,7,13},{2,7,0},{3,0,0},{3,0,0},{4,0,0},{5,0,0},{6,0,0},{7,0,10},{8,0,0},{2,3,4},{2,11,1},{9,0,0},{12,0,0},{2,4,5}};
            f->state = st[i][0]; f->attack = st[i][1]; f->t = st[i][2];
            if (i == 11) f->kick = 1;
            fight_draw();
        }
        video_end();
    }
#endif
#ifdef STREET_BOT
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
    return 0;
}
