/*
 * game.c - Route 128: un juego de carreras para ZX-V, al estilo de los de
 * recreativa de los 80.
 *
 * Una carretera de tres etapas (la costa, el desierto y el bosque) con
 * curvas y cambios de rasante, contra el reloj: cada control da tiempo extra
 * y hay que llegar a la meta antes de que se acabe. Todo lo que hay junto a
 * la carretera y los coches del tráfico son sprites escalados según la
 * distancia; la carretera se pinta línea a línea cada frame, en la página
 * oculta, y se cambia de página en el VBLANK (doble buffer, a 60 imágenes por
 * segundo). Las velocidades van por segundo y los tiempos en segundos
 * (ZXV_FRAME_HZ frames): lo de cada frame sale de ahí (per_frame).
 *
 * Controles: cursores o OPQA; arriba, Q o espacio acelera y abajo o A
 * frena; Z cambia de marcha; Intro para la pausa y Esc para salir; o un mando
 * (fuego 1 acelera, fuego 2 frena, fuego 3 o Select cambia de marcha). LO
 * tira fuerte pero no pasa de 190 km/h; HI llega a 290, pero por debajo de
 * 120 apenas tira; y pasar a LO yendo deprisa frena con el motor, para
 * entrar en las curvas y salir de ellas tirando fuerte. Cada cambio da medio
 * segundo de agarre en las curvas. Sin tocar nada en el
 * título empieza una demostración que juega sola. El récord se guarda en
 * SD:route128.txt (en la tarjeta, aunque el juego se cargue desde ROM:).
 *
 * Compilado con -DRUTA128_STATS (make EXTRA_CFLAGS=-DRUTA128_STATS), el
 * marcador enseña los frames perdidos y la demostración recorre el circuito
 * entero: para medir el rendimiento.
 */
#include "route128.h"
#include "zxv_input.h"
#include "zxv_save.h"

uint32_t rng_state = 1;
int lang;
car_t cars[CARS];

const int8_t sin64[64] = {
    0, 6, 12, 19, 24, 30, 36, 41, 45, 49, 53, 56, 59, 61, 63, 64, 64, 64, 63, 61, 59, 56,
    53, 49, 45, 41, 36, 30, 24, 19, 12, 6, 0, -6, -12, -19, -24, -30, -36, -41, -45, -49,
    -53, -56, -59, -61, -63, -64, -64, -64, -63, -61, -59, -56, -53, -49, -45, -41, -36,
    -30, -24, -19, -12, -6,
};

static int abs_i(int v) { return v < 0 ? -v : v; }

/* Lo que toca en este frame de algo que va a per_second por segundo; *rest
 * guarda lo que sobra, para el siguiente (así no se pierde nada al dividir). */
static int per_frame(int *rest, int per_second)
{
    int v = per_second + *rest;
    int q = v / ZXV_FRAME_HZ;
    *rest = v - q * ZXV_FRAME_HZ;
    return q;
}

/* --- La entrada -------------------------------------------------------------------- */

enum { IN_GAS = 1, IN_BRAKE = 2, IN_LEFT = 4, IN_RIGHT = 8, IN_START = 16, IN_PAUSE = 32, IN_ESC = 64,
       IN_SHIFT = 128, IN_MUSIC = 256 };

static int input_now, input_edge;

/* Las teclas y los botones de cada acción (zxv_input.h), en el orden de
 * sus bits: los de siempre de este juego. */
static const zxv_input_map keys = {
    .keys = {
        { 0x52, 0x14, 0x2C },    /* IN_GAS: cursor arriba, Q, espacio */
        { 0x51, 0x04 },          /* IN_BRAKE: abajo, A */
        { 0x50, 0x12 },          /* IN_LEFT: izquierda, O */
        { 0x4F, 0x13 },          /* IN_RIGHT: derecha, P */
        { 0x2C, 0x10 },          /* IN_START: espacio, M */
        { 0x28 },                /* IN_PAUSE: Intro */
        { 0x29 },                /* IN_ESC */
        { 0x1D },                /* IN_SHIFT: Z */
        { 0x16 },                /* IN_MUSIC: S */
    },
    .joy = { ZXV_JOY_UP | ZXV_JOY_FIRE1, ZXV_JOY_DOWN | ZXV_JOY_FIRE2, ZXV_JOY_LEFT, ZXV_JOY_RIGHT,
             ZXV_JOY_FIRE1, ZXV_JOY_START, 0, ZXV_JOY_FIRE3 | ZXV_JOY_SELECT },
    .pad = 0,
};

static zxv_input input;

static void read_input(void)
{
    zxv_input_update(&input);           /* y vacía la cola de teclas */
    input_edge = input.pressed;
    input_now = input.held;
    if (input_edge & IN_MUSIC)          /* la música, sí o no, en cualquier momento */
        sound_music(!sound_music_on());
}

/* --- La carrera ------------------------------------------------------------------ */

#define SPEED_MAX   (SEG_LEN * 16 * 50) /* 50 segmentos por segundo: unos 290 km/h */
#define LO_TOP      (SPEED_MAX * 66 / 100)  /* lo más que da LO: unos 190 km/h */
#define HI_LUG      (SPEED_MAX * 40 / 100)  /* por debajo, HI apenas tira: unos 120 */
/* Una aceleración de n/64 de SPEED_MAX por segundo, en lo que cambia la
 * velocidad en un frame. */
#define ACCEL(n)    (SPEED_MAX / 64 * (n) / ZXV_FRAME_HZ)
#define GRIP_FRAMES (ZXV_FRAME_HZ / 2)  /* el agarre tras cambiar de marcha */
#define CRASH_FRAMES (ZXV_FRAME_HZ * 3 / 2) /* lo que dura el trompo */
#define BUMP_FRAMES (ZXV_FRAME_HZ / 4)  /* y el golpe con otro coche */
#define LANE_SPEED  600                 /* el tráfico al cambiar de carril, en 1/X1 por segundo */
#define CAR_HW      330                 /* la mitad del ancho del coche, en 1/X1 */
#define START_Z     ((int32_t)8 * SEG_LEN)
#define TIME_START  (50 * ZXV_FRAME_HZ)
#define TIME_EXTEND (42 * ZXV_FRAME_HZ)
#define PLAYER_Y    (HORIZON + CAM_H * DEPTH / PLAYER_Z)    /* la carretera bajo el coche */

static struct {
    int32_t z;                          /* la cámara; el coche va PLAYER_Z por delante */
    int x;                              /* en 1/X1 de media carretera */
    int speed;                          /* en unidades por segundo x 16 */
    int z_rest, x_rest, bg_rest, score_rest;    /* lo que sobra de cada frame (per_frame) */
    int gear;                           /* 0 LO, 1 HI */
    int time;                           /* en frames */
    uint32_t score;
    int crash;                          /* frames que le quedan al trompo */
    int bump;                           /* frames del golpe */
    int next_cp;                        /* el siguiente control */
    int theme;                          /* la etapa del paisaje */
    int bg_x;                           /* el desplazamiento del paisaje */
    int steer;                          /* -1, 0, 1: hacia donde gira */
    int grip;                           /* frames de agarre tras cambiar de marcha */
    int finished;
} pl;

static uint32_t record, frame;
#ifdef RUTA128_STATS
static uint32_t stats_us, stats_max, stats_t0;
#endif
static int demo, banner_t;
static const char *banner_a, *banner_b;

static int speed_kmh(void) { return pl.speed * 293 / SPEED_MAX; }
static int32_t player_z(void) { return pl.z + PLAYER_Z; }
static int player_seg(void) { return (int)(player_z() / SEG_LEN); }

static void cars_init(void)
{
    for (int i = 0; i < CARS; i++) {
        car_t *c = &cars[i];
        c->z = (int32_t)(60 + i * (goal_seg - 80) / CARS + (int)(rnd() % 20)) * SEG_LEN;
        c->target = (int16_t)(((int)(rnd() % 3) - 1) * X1 * 2 / 3);
        c->x = c->target;
        c->z_rest = 0;
        c->speed = SPEED_MAX * (35 + (int)(rnd() % 35)) / 100;
        c->type = (uint8_t)(rnd() % 10 < 6 ? CAR_COUPE : rnd() % 2 ? CAR_VAN : CAR_TRUCK);
        if (c->type == CAR_TRUCK)
            c->speed = c->speed * 3 / 4;
        c->colour = (uint8_t)(rnd() % 6);
    }
}

static void cars_update(void)
{
    int32_t pz = player_z();
    for (int i = 0; i < CARS; i++) {
        car_t *c = &cars[i];
        c->z += per_frame(&c->z_rest, c->speed / 16);
        if (c->z >= track_len - SEG_LEN)
            c->z -= track_len - SEG_LEN;
        /* Uno que ha quedado muy atrás vuelve a salir por delante, fuera de la vista. */
        if (c->z < pz - 20 * SEG_LEN && !pl.finished) {
            c->z = pz + (int32_t)(DRAW_SEGS + (int)(rnd() % 160)) * SEG_LEN;
            if (c->z > (int32_t)goal_seg * SEG_LEN)
                c->z = pz - 30 * SEG_LEN;   /* pasada la meta, que no salga más */
        }
        /* Si alcanza a otro en su carril, cambia de carril. */
        for (int j = 0; j < CARS; j++) {
            const car_t *o = &cars[j];
            if (o == c || o->speed >= c->speed || abs_i(o->x - c->target) > X1 / 3)
                continue;
            int32_t d = o->z - c->z;
            if (d > 0 && d < 6 * SEG_LEN) {
                c->target = (int16_t)(c->target <= -X1 / 2 ? 0 : c->target >= X1 / 2 ? 0
                                      : (rnd() & 1 ? X1 * 2 / 3 : -X1 * 2 / 3));
                break;
            }
        }
        int step = LANE_SPEED / ZXV_FRAME_HZ;
        if (c->x < c->target)
            c->x = (int16_t)(c->x + step > c->target ? c->target : c->x + step);
        else if (c->x > c->target)
            c->x = (int16_t)(c->x - step < c->target ? c->target : c->x - step);
    }
}

/* El piloto automático de la demostración: busca un carril libre y frena
 * antes de las curvas cerradas. Gira como una persona: una vez que empieza,
 * sigue hasta llegar al carril, y no cambia de idea antes de AP_HOLD (con
 * el volante) o AP_REST (sin él); si no, en las curvas pulsaba una imagen
 * de cada dos o tres y el coche temblaba (la postura sigue al giro).
 * Y no corrige lo que no hace falta: solo gira si va a quedar a más de
 * AP_SLACK de su carril. Las dos cosas van juntas: un toque de AP_HOLD lo
 * mueve unos 180 a toda velocidad, y el margen tiene que ser más de la
 * mitad (si no, cada toque se pasa y el siguiente va al otro lado: con 50 y
 * toques de 1/8 s, en las curvas giraba a izquierda y derecha sin parar) y
 * menos de 122 (lo que separa a dos coches en carriles vecinos de
 * tocarse: 2/3 de X1 menos 1,7 CAR_HW). */
#define AP_HOLD (ZXV_FRAME_HZ / 12)
#define AP_REST (ZXV_FRAME_HZ / 15)
#define AP_SLACK 100
#define AP_LOOK (14 * SEG_LEN)          /* lo que mira por delante en cada carril */
static int ap_steer, ap_hold, ap_lane = 1;
static int32_t ap_ahead;                /* el coche más cercano delante, en su carril o el que busca */

/* El carril al que va, de 0 a 2. Si en él hay un coche cerca, el que lo
 * tiene más lejos (a igualdad, el más cercano al suyo), pero solo si es
 * mejor por más de dos segmentos: con dos carriles tapados casi igual, iba
 * de uno a otro. */
static int autopilot_lane(void)
{
    int32_t clear[3] = { AP_LOOK, AP_LOOK, AP_LOOK };
    for (int i = 0; i < CARS; i++) {
        int32_t d = cars[i].z - player_z();
        if (d <= 0 || d >= AP_LOOK)
            continue;
        for (int l = 0; l < 3; l++)
            if (abs_i(cars[i].x - (l - 1) * X1 * 2 / 3) < X1 / 2 && d < clear[l])
                clear[l] = d;
    }
    if (clear[ap_lane] < AP_LOOK) {
        int lane = pl.x > X1 / 3 ? 2 : pl.x < -X1 / 3 ? 0 : 1, best = ap_lane;
        for (int l = 0; l < 3; l++)
            if (clear[l] > clear[best] || (clear[l] == clear[best] && abs_i(l - lane) < abs_i(best - lane)))
                best = l;
        if (clear[best] > clear[ap_lane] + 2 * SEG_LEN)
            ap_lane = best;
    }
    int now = pl.x > X1 / 3 ? 2 : pl.x < -X1 / 3 ? 0 : 1;
    ap_ahead = clear[now] < clear[ap_lane] ? clear[now] : clear[ap_lane];
    return ap_lane;
}

static int autopilot(void)
{
    int in = IN_GAS;
    int ps = player_seg();
    int target = (autopilot_lane() - 1) * X1 * 2 / 3;
    int curve = 0;
    for (int d = 2; d < 22; d++) {
        int c = seg_curve(ps + d);
        if (abs_i(c) > abs_i(curve))
            curve = c;
    }
    /* Dónde estará dentro de un quinto de segundo si no gira: la curva lo
     * empuja hacia fuera (drift, lo que lo mueve en un cincuentavo; véase
     * race_update). */
    int sp = pl.speed * 256 / SPEED_MAX;
    int drift = 44 * sp / 256 * sp / 256 * seg_curve(ps + 2) * 22 / 1600;
    int x = pl.x - drift * 10;
    int want = x < target - AP_SLACK ? 1 : x > target + AP_SLACK ? -1 : 0;
    if (!want && ap_steer && (ap_steer > 0 ? x < target : x > target))
        want = ap_steer;                /* sigue hasta llegar */
    if (ap_hold > 0)
        ap_hold--;
    else if (want != ap_steer) {
        ap_steer = want;
        ap_hold = want ? AP_HOLD : AP_REST;
    }
    if (ap_steer > 0)
        in |= IN_RIGHT;
    else if (ap_steer < 0)
        in |= IN_LEFT;
    int limit = abs_i(curve) >= 90 ? 66 : abs_i(curve) >= 60 ? 80 : 101;
    if (pl.speed > SPEED_MAX * limit / 100 || abs_i(pl.x) > X1)
        in = (in & ~IN_GAS) | (pl.speed > SPEED_MAX * (limit + 6) / 100 ? IN_BRAKE : 0);
    /* Un coche cerca, delante, sin un carril mejor: suelta el gas y, más
     * cerca, frena (si no, lo golpeaba una y otra vez). */
    if (ap_ahead < 5 * SEG_LEN)
        in = (in & ~IN_GAS) | (ap_ahead < 3 * SEG_LEN ? IN_BRAKE : 0);
    /* Las marchas: LO para arrancar, HI al llegar al tope de LO. */
    if (pl.gear ? pl.speed < HI_LUG : pl.speed >= LO_TOP - SPEED_MAX / 50)
        in |= IN_SHIFT;
    return in;
}

static void crash_into(void)
{
    pl.crash = CRASH_FRAMES;
    pl.grip = 0;
    sound_fx(FX_CRASH);
}

/* Lo que pasa en un frame de carrera: la conducción, los choques, los
 * controles. IN_SHIFT en in es cambiar de marcha (en ese frame). */
static void race_update(int in)
{
    int sp = pl.speed * 256 / SPEED_MAX;             /* de 0 a 256 */
    int ps = player_seg();
    int curve = seg_curve(ps);
    int off = abs_i(pl.x) > X1 + X1 / 6;
    if (pl.time <= 0 || pl.finished)
        in = 0;                         /* se acabó: el coche sigue rodando */
    if (pl.crash) {
        pl.crash--;
        pl.speed -= pl.speed * 50 / (10 * ZXV_FRAME_HZ);   /* una décima cada cincuentavo */
        in = 0;
        if (pl.crash < ZXV_FRAME_HZ * 3 / 5)               /* vuelve a la carretera */
            pl.x -= pl.x * 50 / (12 * ZXV_FRAME_HZ);
    } else {
        /* Cambiar de marcha da medio segundo de agarre: la curva empuja
         * menos. Cambiar otra vez mientras dura no lo alarga (de HI a LO y
         * a HI, para pasar una curva sin perder casi nada). */
        if (in & IN_SHIFT) {
            pl.gear ^= 1;
            sound_fx(FX_SHIFT);
            if (!pl.grip)
                pl.grip = GRIP_FRAMES;
        }
        /* El motor: LO tira fuerte hasta su tope (de 0 a 190 en un segundo
         * y medio); HI, hasta el final, pero con poca fuerza y casi nada por
         * debajo de HI_LUG. */
        if (in & IN_GAS) {
            if (!pl.gear)
                pl.speed += pl.speed < LO_TOP ? ACCEL(29) : 0;
            else
                pl.speed += pl.speed < HI_LUG ? ACCEL(4) : ACCEL(11);
        } else if (in & IN_BRAKE) {
            pl.speed -= ACCEL(58);
        } else {
            pl.speed -= ACCEL(7);
        }
        /* LO por encima de su tope: el motor frena, más que soltar el gas y
         * menos que el freno, hasta el tope. */
        if (!pl.gear && pl.speed > LO_TOP) {
            pl.speed -= ACCEL(20);
            if (pl.speed < LO_TOP)
                pl.speed = LO_TOP;
        }
        if (off && pl.speed > SPEED_MAX / 4)     /* la cuneta frena */
            pl.speed -= ACCEL(45);
        /* La dirección y la fuerza de la curva, más fuertes cuanto más
         * deprisa: lo que mueven en un cincuentavo de segundo. */
        int steer = 44 * sp / 256;
        pl.steer = 0;
        if (in & IN_LEFT)
            pl.steer = -1;
        else if (in & IN_RIGHT)
            pl.steer = 1;
        int push = 44 * sp / 256 * sp / 256 * curve * 22 / 1600;
        if (pl.grip) {
            push /= 3;
            pl.grip--;
        }
        pl.x += per_frame(&pl.x_rest, (pl.steer * steer - push) * 50);
    }
    if (pl.speed < 0)
        pl.speed = 0;
    if (pl.speed > SPEED_MAX)
        pl.speed = SPEED_MAX;
    if (pl.x < -X1 * 5 / 2)
        pl.x = -X1 * 5 / 2;
    if (pl.x > X1 * 5 / 2)
        pl.x = X1 * 5 / 2;

    /* Avanza; el paisaje se mueve con la curva. */
    pl.z += per_frame(&pl.z_rest, pl.speed / 16);
    pl.bg_x += per_frame(&pl.bg_rest, curve * pl.speed / SPEED_MAX * 50);
    if (!pl.finished && pl.time > 0)
        pl.score += (uint32_t)per_frame(&pl.score_rest, pl.speed / 64);

    /* Los choques: con lo que hay junto a la carretera... */
    ps = player_seg();
    if (!pl.crash && off) {
        int first, n = seg_objects(ps, &first);
        for (int i = 0; i < n; i++) {
            const object_art_t *a = &obj_art[obj_type(first + i)];
            int hw = a->world_w * X1 / ROAD_HW * 3 / 10;      /* el tronco, no las hojas */
            if (a->solid && abs_i(pl.x - obj_x(first + i)) < hw + CAR_HW) {
                crash_into();
                break;
            }
        }
    }
    /* ...y con los coches. */
    for (int i = 0; i < CARS && !pl.crash; i++) {
        car_t *c = &cars[i];
        int32_t d = c->z - player_z();
        if (d > -SEG_LEN / 2 && d < SEG_LEN / 2 && pl.speed > c->speed &&
            abs_i(pl.x - c->x) < CAR_HW + CAR_HW * 7 / 10) {
            pl.speed = c->speed * 6 / 10;
            pl.z = c->z - PLAYER_Z - SEG_LEN / 2;
            pl.bump = BUMP_FRAMES;
            sound_fx(FX_BUMP);
        }
    }
    if (pl.bump)
        pl.bump--;

    /* Los controles y la meta. */
    if (!pl.finished && pl.next_cp < 3 && ps >= checkpoint_seg[pl.next_cp]) {
        if (pl.next_cp == 2) {
            pl.finished = 1;
            sound_song(SONG_GOAL);
        } else {
            pl.time += TIME_EXTEND;
            banner_a = T("¡TIEMPO EXTRA!", "EXTENDED PLAY!");
            banner_b = 0;
            banner_t = ZXV_FRAME_HZ * 12 / 5;   /* 2,4 s */
            sound_fx(FX_CHECKPOINT);
        }
        pl.next_cp++;
    }
    if (!pl.finished && pl.time > 0) {
        pl.time--;
        if (pl.time > 0 && pl.time <= 5 * ZXV_FRAME_HZ && pl.time % ZXV_FRAME_HZ == 0)
            sound_fx(FX_BEEP);
    }
    /* El paisaje, el de la etapa. */
    int t = seg_theme(ps);
    if (t != pl.theme) {
        pl.theme = t;
        art_background(t);
    }

    /* El sonido del coche. */
    int rpm = pl.gear ? pl.speed * 1000 / SPEED_MAX : pl.speed * 1000 / LO_TOP;
    int skid = !pl.crash && !pl.grip && pl.steer && abs_i(curve) > 48 && sp > 180 ? (sp - 160) / 8 : 0;
    sound_engine(rpm > 1000 ? 1000 : rpm, skid, off && pl.speed > 0 ? 6 + sp / 32 : 0);
}

/* --- La pantalla -------------------------------------------------------------- */

static void hud(void)
{
    char buf[16];
    /* El tiempo, grande y en el centro; en rojo los últimos segundos. */
    static const uint8_t time_ok[7] = { 14, 14, 14, 14, 6, 6, 6 };
    static const uint8_t time_low[7] = { 15, 10, 10, 10, 10, 2, 2 };
    int secs = (pl.time + ZXV_FRAME_HZ - 1) / ZXV_FRAME_HZ;
    number(buf, (uint32_t)secs, 2);
    text_shadow(116, 2, T("TIEMPO", "TIME"), 15);
    text_big(116, 11, buf, 2, secs <= 5 && (frame & 16) ? time_low : time_ok);
    text_shadow(4, 2, T("PUNTOS", "SCORE"), 15);
    number(buf, pl.score, 7);
    text_shadow(4, 11, buf, 14);
    text_shadow(196, 2, T("ETAPA", "STAGE"), 15);
    number(buf, (uint32_t)(pl.next_cp < 3 ? pl.next_cp + 1 : 3), 1);
    text_shadow(232, 2, buf, 14);
#ifdef RUTA128_STATS
    number(buf, video_lost(), 5);
    text_shadow(196, 11, buf, 10);
    number(buf, stats_us, 5);           /* el frame más lento del último segundo, en us */
    text_shadow(196, 20, buf, 13);
#endif
    /* La velocidad y la marcha, abajo. */
    number(buf, (uint32_t)speed_kmh(), 3);
    static const uint8_t speed_col[7] = { 15, 15, 13, 13, 13, 5, 5 };
    text_big(4, 170, buf, 2, speed_col);
    text_shadow(42, 176, "KM/H", 15);
    /* La marcha; parpadea cuando toca cambiar: HI que apenas tira, LO a tope. */
    int hint = pl.gear ? pl.speed < HI_LUG : pl.speed >= LO_TOP;
    text_shadow(228, 176, pl.gear ? "HI" : "LO", hint && (frame & 8) ? 10 : pl.gear ? 14 : 13);
}

static int show_car = 1;

/* El coche: el de la postura según el giro y la curva; dando vueltas si ha
 * chocado; botando en la cuneta. road_draw lo dibuja en su sitio. */
static void draw_player(void)
{
    int pose = 2 + pl.steer;
    int curve = seg_curve(player_seg());
    if (pl.steer && abs_i(curve) > 40 && (curve > 0) == (pl.steer > 0))
        pose += pl.steer;               /* bien girado */
    else if (!pl.steer && abs_i(curve) > 40)
        pose += curve > 0 ? 1 : -1;
    int flags = 0, dx = 0, dy = 0;
    if (pl.crash) {
        static const uint8_t spin[4] = { 2, 4, 2, 0 };
        int c = pl.crash * 50 / ZXV_FRAME_HZ;   /* en cincuentavos: de 75 a 0 */
        pose = spin[(c / 4) & 3];
        dy = c > 40 ? -(int)((75 - c) * (c - 40) / 60) : 0;
    }
    if (abs_i(pl.x) > X1 + X1 / 6 && pl.speed > 0)
        dy -= (int)(frame & 2) >> 1;    /* el traqueteo de la cuneta */
    if (pl.bump)
        dx = (pl.bump & 2) ? 2 : -2;
    if (!show_car)
        return;
    const sprite_t *s = &player_art[pose];
    draw_sprite(s, W / 2 - s->w / 2 + dx, PLAYER_Y - s->h + 3 + dy, flags, 0);
}

static void draw_world(void)
{
    int32_t cam_y = height_at(player_z()) + CAM_H;
    video_border(show_car ? road_sky(pl.theme) : 0);    /* en el título, negro */
    road_draw(pl.z, pl.x, cam_y, pl.theme, pl.bg_x, draw_player);
}

static void frame_end(void)
{
#ifdef RUTA128_STATS
    uint32_t us = zxv_time_us() - stats_t0;
    if (us > stats_max)
        stats_max = us;
    if (frame % ZXV_FRAME_HZ == 0) {
        stats_us = stats_max;
        stats_max = 0;
    }
#endif
    if (banner_t) {
        banner_t--;
        if (banner_a)
            text_center(56, banner_a, (frame & 8) ? 15 : 14);
        if (banner_b)
            text_center(68, banner_b, 13);
    }
    if (demo) {
        text_center(40, T("DEMOSTRACIÓN", "DEMO"), (frame & 32) ? 13 : 5);
        text_center(132, T("PULSA ESPACIO", "PRESS SPACE"), 15);
    }
    sound_frame();
    video_end();
    frame++;
}

/* --- El récord -------------------------------------------------------------------- */

/* El récord, con zxv_save.h (si se va la luz a medias, se queda el de antes).
 * Hasta la 0.11 iba en texto en route128.txt: si no hay route128.sav, se lee ese
 * una vez (y se deja donde está). */
#define RECORD_FILE "SD:route128.sav"
#define RECORD_OLD "SD:route128.txt"
#define RECORD_VERSION 1

static void load_record(void)
{
    if (zxv_load(RECORD_FILE, RECORD_VERSION, &record, sizeof record) == ZXV_OK)
        return;
    char buf[12];
    int f = zxv_file_open(RECORD_OLD, ZXV_O_READ);
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
    /* Sin tarjeta, el récord dura lo que la partida. */
    zxv_save(RECORD_FILE, RECORD_VERSION, &record, sizeof record);
}

/* --- La partida --------------------------------------------------------------- */

static void race_reset(void)
{
    pl = (typeof(pl)){ 0 };
    pl.z = START_Z;
    pl.x = -X1 / 3;
    pl.time = TIME_START;
    pl.theme = THEME_COAST;
    art_background(THEME_COAST);
    banner_t = 0;
    cars_init();
}

/* La cuenta atrás: 3, 2, 1, ¡ya!, cada uno 0,8 s. Devuelve 1 si se pulsó Esc. */
#define COUNT_STEP  (ZXV_FRAME_HZ * 4 / 5)

static int countdown(void)
{
    static const char *const label[4] = { "3", "2", "1", 0 };
    sound_engine(0, 0, 0);
    for (int t = 0; t < 4 * COUNT_STEP; t++) {
        read_input();
        if (input_edge & IN_ESC)
            return 1;
        if (t % COUNT_STEP == 0)
            sound_fx(t == 3 * COUNT_STEP ? FX_GO : FX_BEEP);
        if (input_edge & IN_SHIFT)      /* se sale en LO, pero se puede elegir */
            pl.gear ^= 1;
        video_begin();
        draw_world();
        hud();
        static const uint8_t col[7] = { 15, 14, 14, 14, 6, 6, 6 };
        const char *s = label[t / COUNT_STEP];
        if (s)
            text_big(W / 2 - 15, 60, s, 5, col);
        else
            text_big(W / 2 - 3 * 6 * 3, 64, T("¡YA!", "GO!"), 3, col);
        /* El motor, revolucionado sin moverse. */
        sound_engine((input_now & IN_GAS) ? 700 : 150, 0, 0);
        frame_end();
    }
    return 0;
}

/* Una carrera; 1 si se pulsó Esc. */
static int race(void)
{
    race_reset();
    sound_song(demo ? SONG_RACE : SONG_NONE);
    if (!demo && countdown())
        return 1;
    sound_song(SONG_RACE);
    int over_t = 0;
    for (;;) {
        read_input();
        if (input_edge & IN_ESC)
            return 1;
        if (demo && (input_edge & (IN_START | IN_PAUSE)))
            return 0;
#ifndef RUTA128_STATS
        if (demo && frame > 48 * ZXV_FRAME_HZ)  /* con RUTA128_STATS, el circuito entero */
            return 0;
#endif
        /* Acabada, al título con una tecla o a los 10 s: aquí, antes de
         * dibujar el frame, que si no no cabe con el primero del título. */
        if (over_t > 10 * ZXV_FRAME_HZ ||
            (over_t > 2 * ZXV_FRAME_HZ && pl.speed == 0 && (input_edge & (IN_START | IN_GAS))))
            return 0;
        if (input_edge & IN_PAUSE) {    /* la pausa: hasta otra vez Intro (Esc sale) */
            sound_mute(1);
            for (;;) {
                read_input();
                video_begin();
                draw_world();
                hud();
                text_center(84, T("PAUSA", "PAUSED"), (frame & 16) ? 15 : 5);
                video_end();
                frame++;
                if (input_edge & (IN_PAUSE | IN_ESC))
                    break;
            }
            sound_mute(0);
            if (input_edge & IN_ESC)
                return 1;
        }
#ifdef RUTA128_STATS
        stats_t0 = zxv_time_us();
#endif
        race_update(demo ? autopilot() : (input_now & ~IN_SHIFT) | (input_edge & IN_SHIFT));
        cars_update();
        video_begin();
        draw_world();
        hud();
        if (pl.finished || pl.time <= 0) {
            if (over_t == 0) {
                banner_t = 0;           /* el de ¡TIEMPO EXTRA! iría en el mismo sitio */
                if (pl.finished)
                    pl.score += (uint32_t)((pl.time + ZXV_FRAME_HZ - 1) / ZXV_FRAME_HZ) * 1000;  /* lo que sobra */
                else
                    sound_song(SONG_OVER);
                if (!demo && pl.score > record) {
                    record = pl.score;
                    save_record();
                }
            }
            over_t++;
            if (pl.finished) {
                text_center(56, T("¡META!", "GOAL!"), (frame & 8) ? 15 : 14);
                text_center(70, T("ENHORABUENA", "CONGRATULATIONS"), 13);
            } else {
                text_center(56, T("SE ACABÓ EL TIEMPO", "TIME UP"), (frame & 8) ? 15 : 10);
            }
            if (pl.score >= record && record && !demo)
                text_center(84, T("¡NUEVO RÉCORD!", "NEW HIGH SCORE!"), 14);
        }
        frame_end();
    }
}

/* El título: la carretera de la salida, el nombre y las teclas. 0 para
 * jugar, 1 para la demostración, -1 para salir. */
static int title(void)
{
    static const uint8_t logo[7] = { 15, 14, 14, 6, 10, 2, 2 };
    race_reset();
    pl.z = 0;
    pl.x = 0;
    show_car = 0;
    sound_engine(-1, 0, 0);
    sound_song(SONG_TITLE);
    for (int t = 0;; t++) {
        read_input();
        if (input_edge & IN_ESC) {
            show_car = 1;
            return -1;
        }
        if ((t > ZXV_FRAME_HZ * 2 / 5 && (input_edge & IN_START)) || t > 12 * ZXV_FRAME_HZ) {
            show_car = 1;
            return t > 12 * ZXV_FRAME_HZ;
        }
        int t50 = t * 50 / ZXV_FRAME_HZ;    /* en cincuentavos de segundo */
        pl.bg_x = t50;                  /* el paisaje se mueve despacio */
        video_begin();
        draw_world();
        int y = 26 + isin(t50) / 16;
        text_big((W - 9 * 6 * 4 + 4) / 2, y, "ROUTE 128", 4, logo);
        text_center(64, T("CARRERAS PARA ZX-V", "A RACING GAME FOR ZX-V"), 15);
        char buf[16], line[32];
        number(buf, record, 7);
        const char *label = T("RÉCORD ", "HIGH SCORE ");
        int n = 0;
        while (*label)
            line[n++] = *label++;
        for (int i = 0; buf[i]; i++)
            line[n++] = buf[i];
        line[n] = 0;
        text_center(76, line, 14);
        rows(86, 148, SOLID(0));        /* una franja para las instrucciones; tapa la */
        rows(86, 87, SOLID(15));        /* pancarta de salida, que asomaría a medias */
        rows(147, 148, SOLID(15));
        text_center(102, T("ARRIBA O ESPACIO: ACELERAR", "UP OR SPACE: ACCELERATE"), 15);
        text_center(111, T("ABAJO: FRENAR", "DOWN: BRAKE"), 15);
        text_center(120, T("IZQUIERDA Y DERECHA: GIRAR", "LEFT AND RIGHT: STEER"), 15);
        text_center(129, T("Z: MARCHA, LO O HI", "Z: GEAR, LO OR HI"), 15);
        text_center(138, T("INTRO: PAUSA  S: MÚSICA  ESC: SALIR", "ENTER: PAUSE  S: MUSIC  ESC: QUIT"), 13);
        if (t & 32)
            text_center(160, T("PULSA ESPACIO O FUEGO", "PRESS SPACE OR FIRE"), 14);
        sound_frame();
        video_end();
        frame++;
    }
}

int main(void)
{
    lang = zxv_lang_get() == ZXV_LANG_ES ? 0 : 1;
    video_init();                       /* antes: lo de abajo usa la memoria de vídeo que no se ve */
    sound_init();
    art_init();
    track_build();
    load_record();
    video_init();                       /* otra vez: lo que tarda lo de antes no son frames perdidos */
    zxv_input_init(&input, &keys);
    zxv_input_update(&input);           /* lo que esté pulsado al empezar no cuenta */
    input_now = input.held;
    for (;;) {
        int r = title();
        if (r < 0)
            break;
        demo = r;
        frame = 0;
        ap_steer = ap_hold = 0;
        ap_lane = 1;
        rng_state = demo ? 4242 : (rng_state ^ *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES);
        if (race() && !demo)
            continue;                   /* Esc en la carrera: al título */
        demo = 0;
    }
    sound_off();
    video_done();
    zxv_set_mode(0);
    while (zxv_key_get() >= 0)
        ;
    return 0;                           /* el récord ya está en la tarjeta */
}
