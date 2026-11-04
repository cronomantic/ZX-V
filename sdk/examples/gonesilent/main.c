/*
 * main.c - Gone Silent, una aventura en 3D sólido: el jugador, enviado por el
 * Servicio de Rescate Colonial, explora la colonia de Better Worlds (el
 * exterior y las cuatro instalaciones: habitáculos, control, mina y planta)
 * para reparar su módulo y escapar.
 *
 * Teclas: Q/A o arriba/abajo, andar; O/P o izquierda/derecha, girar; Z/X, de
 * lado; W/S, mirar arriba y abajo (C, al frente); espacio, disparar; Intro o
 * U, usar o leer lo que se tiene delante; I, los objetos que se llevan; M,
 * el mapa; H, la ayuda; Tab, cambia entre el modo 0 (256 x 192) y el 1
 * (512 x 384); Esc, vuelve al BASIC. También con un joystick (el primer
 * botón dispara y el segundo usa). Las puertas se cruzan andando hacia ellas.
 *
 * La energía del traje se gasta al andar fuera, al disparar, con cada golpe,
 * en la atmósfera tóxica (menos con el filtro) y en las tormentas, y se
 * recarga encima de los puntos de recarga (algunos, solo con la energía de
 * la colonia restablecida); si se acaba, se vuelve al último punto de recarga
 * usado (o al módulo), con todo lo conseguido. Las criaturas cazan por el
 * suelo, cada una a su manera (emboscada, acecho o persecución), se yerguen
 * antes de golpear y caen con tres disparos; con la planta de terraformación
 * parada, despiertan más cerca y van más despacio.
 * Las torretas disparan mientras las defensas estén activas; se desactivan
 * desde seguridad, en control. Control está a oscuras hasta que vuelve la
 * energía.
 *
 * Lo que se lleva, lo que se sabe y lo que se ha hecho son marcas (F_...):
 * los objetos se cogen al pasar por encima, los registros se leen con Intro
 * y las consolas se usan; las puertas, las consolas y las torretas miran las
 * marcas. Fuera llega una tormenta de vez en cuando: tiñe los colores, deja
 * ver menos lejos y gasta energía. Con la última pieza del módulo llega la
 * tormenta fuerte, que ya no se va.
 *
 * El marcador es el visor del traje (hud): la zona, la energía, la brújula,
 * lo que se lleva, el sensor de movimiento y los avisos.
 */
#include "gonesilent.h"

int lang;

const int16_t sin_q[257] = {
    0, 101, 201, 302, 402, 503, 603, 704, 804, 904, 1005, 1105,
    1205, 1306, 1406, 1506, 1606, 1706, 1806, 1906, 2006, 2105, 2205, 2305,
    2404, 2503, 2603, 2702, 2801, 2900, 2999, 3098, 3196, 3295, 3393, 3492,
    3590, 3688, 3786, 3883, 3981, 4078, 4176, 4273, 4370, 4467, 4563, 4660,
    4756, 4852, 4948, 5044, 5139, 5235, 5330, 5425, 5520, 5614, 5708, 5803,
    5897, 5990, 6084, 6177, 6270, 6363, 6455, 6547, 6639, 6731, 6823, 6914,
    7005, 7096, 7186, 7276, 7366, 7456, 7545, 7635, 7723, 7812, 7900, 7988,
    8076, 8163, 8250, 8337, 8423, 8509, 8595, 8680, 8765, 8850, 8935, 9019,
    9102, 9186, 9269, 9352, 9434, 9516, 9598, 9679, 9760, 9841, 9921, 10001,
    10080, 10159, 10238, 10316, 10394, 10471, 10549, 10625, 10702, 10778, 10853, 10928,
    11003, 11077, 11151, 11224, 11297, 11370, 11442, 11514, 11585, 11656, 11727, 11797,
    11866, 11935, 12004, 12072, 12140, 12207, 12274, 12340, 12406, 12472, 12537, 12601,
    12665, 12729, 12792, 12854, 12916, 12978, 13039, 13100, 13160, 13219, 13279, 13337,
    13395, 13453, 13510, 13567, 13623, 13678, 13733, 13788, 13842, 13896, 13949, 14001,
    14053, 14104, 14155, 14206, 14256, 14305, 14354, 14402, 14449, 14497, 14543, 14589,
    14635, 14680, 14724, 14768, 14811, 14854, 14896, 14937, 14978, 15019, 15059, 15098,
    15137, 15175, 15213, 15250, 15286, 15322, 15357, 15392, 15426, 15460, 15493, 15525,
    15557, 15588, 15619, 15649, 15679, 15707, 15736, 15763, 15791, 15817, 15843, 15868,
    15893, 15917, 15941, 15964, 15986, 16008, 16029, 16049, 16069, 16088, 16107, 16125,
    16143, 16160, 16176, 16192, 16207, 16221, 16235, 16248, 16261, 16273, 16284, 16295,
    16305, 16315, 16324, 16332, 16340, 16347, 16353, 16359, 16364, 16369, 16373, 16376,
    16379, 16381, 16383, 16384, 16384,
};

#define EYE         160                 /* la altura de los ojos */
#define BODY        180                 /* la del cuerpo, para chocar */
#define RADIUS      35                  /* el radio del cuerpo */
#define STEP        45                  /* lo que se sube sin saltar */
#define WALK        350                 /* cm por segundo */
#define SPIN        400                 /* giro, en 1/1024 de vuelta por segundo */
#define LOOK_MAX    48                  /* píxeles del modo 0 arriba o abajo */

#define ENERGY_MAX  10000               /* la energía del traje; el marcador enseña el % */
#define COST_WALK   1                   /* por frame andando fuera: unos tres minutos */
#define COST_SHOT   100
#define COST_BITE   500                 /* el golpe de una criatura */
#define BITE_BACK   120                 /* frames que se aparta después */
#define COST_TURRET 400                 /* un disparo de la torreta */
#define CHARGE_RATE 40                  /* por frame encima de un punto de recarga */
#define SHOT_RANGE  4000                /* cm */
#define SHOT_DELAY  12                  /* frames entre disparos */
#define USE_RANGE   400                 /* cm hasta lo que se usa */
#define WAKE_RANGE  1500                /* las criaturas despiertan a esta distancia */
#define WAKE_CALM   700                 /* y con la planta parada */
#define CREATURE_HP 3
#define MOTHER_HP   8                   /* la incubadora aguanta más */
#define SPIT_RANGE  2000                /* hasta dónde escupe */
#define SPIT_DELAY  150                 /* frames entre escupitajos */
#define SPIT_WARN   36                  /* lo que borbotea antes */
#define SPIT_SPEED  22                  /* cm por frame */
#define COST_SPIT   250
#define TURRET_RANGE 2000
#define TURRET_DELAY 120                /* frames entre disparos de la torreta */
#define TURRET_LOCK 40                  /* lo que avisa (parpadea) antes de disparar */
#define FILTERED    3                   /* el filtro divide entre esto lo que gasta la atmósfera tóxica */
#define STORM_CALM  3600                /* frames sin tormenta (un minuto) */
#define STORM_WARN  600                 /* el aviso, diez segundos antes */
#define STORM_LEN   1200                /* lo que dura (veinte segundos) */
#define STORM_RAMP  180                 /* lo que tarda en llegar y en irse */
#define STORM_MAX   208                 /* su fuerza (de 256); la fuerte, 256 */
#define STORM_COST  1                   /* por frame fuera, también con la tormenta fuerte */
#define ZONES_MAX   10

static int zone;
static int32_t px, pz, feet, fall;      /* fall: la velocidad de caída, en 1/16 de cm por frame */
static int32_t move_x, move_z;          /* lo que sobra del paso, en 1/16384 de cm */
static int yaw, look, spin_rest;
static uint32_t frame;

static int32_t energy;
static uint16_t got;                    /* las marcas del juego (F_...) */
static int dead;                        /* frames que quedan del aviso de energía agotada */
static int charging;                    /* este frame, encima de un punto de recarga */
static int shot_delay, shot_show, shot_hit;     /* el disparo: la espera, lo que se ve, si dio */
static int hurt_show;                   /* el borde rojo de un golpe */
static int beam_show, beam_from;        /* el disparo de la torreta y la torreta que disparó */
static int use_target;                  /* lo que se usaría, o -1 */
static const char *msg;                 /* el aviso del marcador */
static int msg_time;
#define HINT_TIME   (20 * ZXV_FRAME_HZ) /* lo que se ve «H: AYUDA» al empezar */
static int hint_time;
static uint8_t gone[ZONES_MAX][OBJECTS_MAX];    /* las criaturas destruidas y los objetos cogidos */

/* Lo que se ve encima de la vista, con el juego parado: el título, la
 * introducción, un registro, los objetos que se llevan o el final. */
enum { P_NONE, P_TITLE, P_INTRO, P_LOG, P_ITEMS, P_MAP, P_HELP, P_END };
static int panel, panel_log;

/* La tormenta: el reloj, su fuerza (de 0 a 256) y si ya es la fuerte; y
 * cómo se pusieron los colores (la tormenta y si estaba a oscuras). */
static uint32_t storm_clock;
static int storm, storm_strong, pal_shown = -1;

/* Lo que va con cada pieza que actúa: las criaturas y las torretas. */
enum { A_IDLE, A_HIDE, A_STALK, A_CHASE, A_LUNGE, A_REAR, A_BACK, A_DEAD };
typedef struct {
    int8_t hp, state;
    int16_t timer, flash;
    int32_t x, z;                       /* el centro de las criaturas, en 1/16 de cm */
    int16_t yaw, walk;                  /* hacia dónde miran; los frames que llevan andando */
    uint8_t kind;                       /* cómo cazan */
    int8_t side;                        /* por qué lado rodean lo que tienen delante */
} actor_t;
static actor_t actors[OBJECTS_MAX];
static int still;                       /* los frames que lleva quieto el jugador */

/* El sensor de movimiento: lo que vio el último pulso, a qué distancia y en
 * qué ángulo desde la mirada; age, los frames desde que el pulso llegó a ello
 * (-1, aún no ha llegado). */
#define TRACK_MAX   8
typedef struct {
    int16_t d, rel;
    int8_t age;
} blip_t;
static blip_t blips[TRACK_MAX];
static int blip_count, track_t, track_period;

/* --- El mapa ------------------------------------------------------------------- */

/* De cada zona, una rejilla de MAP_N x MAP_N casillas con lo que se ha visto
 * (un bit por casilla). La de la zona actual empieza en (map_x0, map_z0),
 * con casillas de map_cell cm: lo justo para que quepa todo lo que hay, sin
 * las montañas, que son piezas de kilómetros. */
#define MAP_N       64
#define MAP_HUGE    10000               /* las piezas así de grandes no cuentan */
static uint8_t seen[ZONES_MAX][MAP_N * MAP_N / 8];
static int32_t map_x0, map_z0, map_cell;
static int32_t map_bx, map_bz, map_bw, map_bh;  /* lo que ocupa la zona, para dibujarla */
static int map_ray;                     /* el siguiente rayo con el que se mira */

static void map_setup(int z)
{
    const zone_t *zn = &zones[z];
    int32_t x0 = 0, z0 = 0, x1 = 0, z1 = 0;
    int any = 0;
    for (int i = 0; i < zn->count; i++) {
        const piece_t *p = &zn->pieces[i];
        if (p->sx > MAP_HUGE || p->sz > MAP_HUGE)
            continue;
        if (!any || p->x < x0) x0 = p->x;
        if (!any || p->z < z0) z0 = p->z;
        if (!any || p->x + p->sx > x1) x1 = p->x + p->sx;
        if (!any || p->z + p->sz > z1) z1 = p->z + p->sz;
        any = 1;
    }
    map_bx = x0;
    map_bz = z0;
    map_bw = x1 - x0 > 1 ? x1 - x0 : 1;
    map_bh = z1 - z0 > 1 ? z1 - z0 : 1;
    int32_t ext = x1 - x0 > z1 - z0 ? x1 - x0 : z1 - z0;
    map_cell = ext / MAP_N + 1;
    map_x0 = x0 - (map_cell * MAP_N - (x1 - x0)) / 2;   /* centrada */
    map_z0 = z0 - (map_cell * MAP_N - (z1 - z0)) / 2;
}

static int map_seen(int i, int j)
{
    if (i < 0 || j < 0 || i >= MAP_N || j >= MAP_N)
        return 0;
    return (seen[zone][(j * MAP_N + i) >> 3] >> (i & 7)) & 1;
}

static void map_mark(int32_t x, int32_t z)
{
    int32_t i = x - map_x0, j = z - map_z0;
    if (i < 0 || j < 0 || i >= map_cell * MAP_N || j >= map_cell * MAP_N)
        return;
    i /= map_cell;
    j /= map_cell;
    seen[zone][(j * MAP_N + i) >> 3] |= (uint8_t)(1u << (i & 7));
}

/* --- La partida guardada --------------------------------------------------------- */

/* Una sola, en la tarjeta: se guarda al llegar a un punto de recarga (el
 * mismo sitio al que se vuelve al agotar la energía) y se borra al acabar. */
void *memset(void *s, int c, unsigned n);
void *memcpy(void *d, const void *s, unsigned n);
int memcmp(const void *a, const void *b, unsigned n);

#define SAVE_FILE   "SD:gonesilent.sav"
#define SAVE_MAGIC  0x34565347u         /* "GSV4" */
typedef struct {
    uint32_t magic;
    uint16_t got;
    uint8_t zone, storm_strong;
    int32_t x, z, feet, energy;
    int32_t yaw;
    uint32_t storm_clock;
} save_t;                               /* y detrás, gone y seen tal cual */

static int saved;                       /* si hay una guardada */
static int title_pick;                  /* en el título: 0 continuarla, 1 partida nueva */

static int save_exists(void)
{
    int f = zxv_file_open(SAVE_FILE, ZXV_O_READ);
    if (f < 0)
        return 0;
    zxv_file_close(f);
    return 1;
}

static void creatures_spawn(void);
static void creature_kill(int i);

/* La incubadora de la zona (o -1) y lo que escupe (o -1): de dónde sale,
 * hacia dónde va (todo en 1/16 de cm) y cuánto le queda. */
static int mother = -1, spit = -1;
static int32_t spit_x, spit_y, spit_z, spit_dx, spit_dy, spit_dz;
static int spit_life;

static void say(const char *s, int frames)
{
    msg = s;
    msg_time = frames;
}

/* --- Las zonas ---------------------------------------------------------------- */

/* Si se tienen todas las marcas de m. */
static int have(uint16_t m) { return (got & m) == m; }

/* --- Las luces ------------------------------------------------------------------ */

/* Las del techo (L_CEIL) se apagan mientras a la zona le falte la energía;
 * las que fallan (L_FLICKER), además, se apagan a ratos, cada una a su ritmo,
 * con un chasquido si se está cerca; las balizas de emergencia (L_BEACON)
 * giran mientras falta la energía, con dos caras naranjas; los avisos
 * (L_WARN) parpadean hasta tener need (la planta, hasta pararla); las
 * pantallas (L_SCREEN) están en naranja hasta tener need y luego en verde;
 * y las ruedas de las bombas (L_WHEEL) giran un rato al cerrarlas
 * (wheels_turn). */
#define WHEEL_TURN  90
#define FLICKER_RANGE 1500
static const uint8_t flicker_off[] = { 1, 1, 0, 1, 0, 0, 0, 1, 1, 1 };     /* de 4 en 4 frames */

static void lamp_update(int i)
{
    object_t *o = &objects[i];
    int lit = !zones[zone].lit || have(zones[zone].lit);
    uint8_t m[FACES_MAX];
    int on = 1, spin = 0;
    if (o->p.log == L_WHEEL) {
        if (actors[i].timer) {
            actors[i].timer--;
            o->yaw = (o->yaw + 12) & (TURN - 1);
            object_setup(o);
        }
        return;
    }
    switch (o->p.log) {
    case L_FLICKER: {
        uint32_t t = (frame + (uint32_t)i * 97) % (240 + (uint32_t)(i % 5) * 37);
        on = lit && !(t < 4 * sizeof flicker_off && flicker_off[t / 4]);
        break;
    }
    case L_CEIL:
        on = lit;
        break;
    case L_BEACON:
        on = spin = !lit;
        break;
    case L_WARN:
        on = !have(o->p.need) && (frame / 20) & 1;
        break;
    }
    for (int f = 0; f < FACES_MAX; f++)
        m[f] = o->p.log == L_SCREEN ? (have(o->p.need) ? MAT_EYE : MAT_ALARM) :
               !on ? MAT_GRATE : o->p.log == L_WARN ? MAT_ALARM : o->p.log != L_BEACON ? MAT_GLOW :
               f == 2 || f == 3 ? MAT_ALARM : f >= 4 ? MAT_DARK : MAT_GRATE;
    if (o->p.log == L_FLICKER && !on && o->p.mat[0] == MAT_GLOW) {
        int32_t dx = px - (o->p.x + o->p.sx / 2), dz = pz - (o->p.z + o->p.sz / 2);
        if (dx > -FLICKER_RANGE && dx < FLICKER_RANGE && dz > -FLICKER_RANGE && dz < FLICKER_RANGE)
            sound_fx(FX_FLICKER);
    }
    if (spin)
        o->yaw = (o->yaw + 24) & (TURN - 1);
    else if (!memcmp(m, o->p.mat, FACES_MAX))
        return;
    memcpy(o->p.mat, m, FACES_MAX);
    object_setup(o);
}

/* Los colores que dicen cómo están las cosas: las torretas, negras si están
 * apagadas; las puertas que piden algo, con rayas mientras falte; los puntos
 * de recarga sin energía, apagados; y las luces (también las pantallas de lo
 * que se usa, que dicen si ya se usó). */
static void looks_update(void)
{
    for (int i = 0; i < object_count; i++) {
        object_t *o = &objects[i];
        int m;
        if ((o->p.flags & OBJ_CHARGE) && o->p.need) {
            const piece_t *b = &zones[zone].pieces[i];
            for (int f = 0; f < FACES_MAX; f++)
                o->p.mat[f] = have(o->p.need) || f == 0 ? b->mat[f] : MAT_DARK;
            object_setup(o);
            continue;
        }
        if (o->p.flags & OBJ_TURRET)
            m = have(o->p.need) && o->p.need ? MAT_DARK : MAT_HAZARD;
        else if ((o->p.flags & OBJ_DOOR) && o->p.need)
            m = have(o->p.need) ? MAT_DOOR : MAT_HAZARD;
        else
            continue;
        for (int f = 0; f < FACES_MAX; f++)
            o->p.mat[f] = (uint8_t)m;
        object_setup(o);
    }
    for (int i = 0; i < object_count; i++)
        if (objects[i].p.flags & OBJ_LAMP)
            lamp_update(i);
}

/* Los colores de la zona: a oscuras (al 40 %) mientras le falte la energía,
 * menos el naranja claro de las alarmas, que siguen encendidas; fuera,
 * teñidos con la bruma según la tormenta (como mucho, un 37 %), que también
 * deja ver menos lejos (render.c mezcla las piezas con la bruma según su
 * distancia). El negro y la luz se quedan como están, para los recuadros y
 * la cruz. */
static void palette_update(void)
{
    const zone_t *zn = &zones[zone];
    int level = zn->outside ? storm : 0;
    int dark = zn->lit && !have(zn->lit);
    if ((level | dark << 9) == pal_shown)
        return;
    pal_shown = level | dark << 9;
    uint8_t pal[16][3];
    const uint8_t *dust = zn->pal[14];
    for (int i = 0; i < 16; i++)
        for (int k = 0; k < 3; k++) {
            int c = zn->pal[i][k];
            if (i != 0 && i != 15) {
                if (dark && i != 10)
                    c = c * 2 / 5;
                c += (dust[k] - c) * level * 3 / 2048;
            }
            pal[i][k] = (uint8_t)c;
        }
    video_palette(pal);
    /* Lo que se ve: de 2,5 km sin tormenta a 25 m con la fuerte. */
    int32_t clear = 256 - level;
    view_far = level ? 2500 + clear * clear * 15 / 4 : 250000;
    view_haze = level * 4 / 257;
}

/* El aviso de la primera marca de m que falte. */
static const char *missing(uint16_t m)
{
    for (int b = 0; b < FLAGS; b++)
        if ((m & ~got) & (1u << b) && flag_info[b].need_es)
            return T(flag_info[b].need_es, flag_info[b].need_en);
    return T("NO SE PUEDE", "YOU CAN'T");
}

/* Consigue las marcas m: avisa de la primera nueva y lo que cambia con ellas. */
static void gain(uint16_t m)
{
    uint16_t now = m & ~got;
    if (!now)
        return;
    got |= now;
    for (int b = 0; b < FLAGS; b++)
        if (now & (1u << b)) {
            say(T(flag_info[b].got_es, flag_info[b].got_en), 150);
            break;
        }
    if (now & F_STRONG_STORM)
        storm_strong = 1;
    if (now & F_ESCAPED) {
        panel = P_END;
        zxv_file_remove(SAVE_FILE);
        saved = 0;
    }
    looks_update();
}

static void items_setup(void);

static void zone_enter(int z, int entry)
{
    const zone_t *zn = &zones[z];
    zone = z;
    map_setup(z);
    object_count = zn->count < OBJECTS_MAX ? zn->count : OBJECTS_MAX;
    for (int i = 0; i < object_count; i++) {
        objects[i].p = zn->pieces[i];
        objects[i].yaw = 0;
        if (gone[z][i] && !(objects[i].p.flags & OBJ_SPRITE))
            objects[i].p.flags |= OBJ_HIDDEN;
        object_setup(&objects[i]);
        actors[i] = (actor_t){ .hp = CREATURE_HP, .side = 1 };
    }
    creatures_spawn();
    items_setup();
    looks_update();
    use_target = -1;
    px = zn->entries[entry].x;
    pz = zn->entries[entry].z;
    yaw = zn->entries[entry].yaw;
    feet = zn->entries[entry].feet;
    fall = 0;
    look = 0;
    pal_shown = -1;
    palette_update();
}

/* El último punto de recarga usado: la zona (-1 si aún ninguno), dónde y
 * hacia dónde se miraba. */
static int cp_zone = -1, cp_yaw;
static int32_t cp_x, cp_z, cp_feet;

static void game_start(void)
{
    energy = ENERGY_MAX;
    got = 0;
    cp_zone = -1;
    panel = P_TITLE;
    storm_clock = 0;
    storm = storm_strong = 0;
    for (int z = 0; z < ZONES_MAX; z++)
        for (int i = 0; i < OBJECTS_MAX; i++)
            gone[z][i] = 0;
    memset(seen, 0, sizeof seen);
    zone_enter(0, 0);
    msg_time = 0;
    saved = save_exists();
    title_pick = 0;
}

/* Después de agotar la energía: en el último punto de recarga, o junto al
 * módulo, con el traje cargado y todo lo conseguido. */
static void respawn(void)
{
    energy = ENERGY_MAX;
    if (cp_zone < 0) {
        zone_enter(0, 0);
        say(T("HAS VUELTO AL MÓDULO", "BACK AT THE LANDER"), 150);
        return;
    }
    zone_enter(cp_zone, 0);
    px = cp_x;
    pz = cp_z;
    feet = cp_feet;
    yaw = cp_yaw;
    say(T("HAS VUELTO AL ÚLTIMO PUNTO DE RECARGA", "BACK AT THE LAST CHARGER"), 150);
}

/* Devuelve 1 si se guardó (sin tarjeta, no se puede). */
static int game_save(void)
{
    save_t sv = { .magic = SAVE_MAGIC, .got = got, .zone = (uint8_t)zone, .storm_strong = (uint8_t)storm_strong,
                  .x = px, .z = pz, .feet = feet, .energy = energy, .yaw = yaw, .storm_clock = storm_clock };
    int f = zxv_file_open(SAVE_FILE, ZXV_O_WRITE | ZXV_O_CREATE | ZXV_O_TRUNC);
    if (f < 0)
        return 0;
    int ok = zxv_file_write(f, &sv, sizeof sv) == (int)sizeof sv &&
             zxv_file_write(f, gone, sizeof gone) == (int)sizeof gone &&
             zxv_file_write(f, seen, sizeof seen) == (int)sizeof seen;
    zxv_file_close(f);
    saved = ok;
    return ok;
}

/* La guardada, en el punto de recarga donde se guardó; 0 si no se puede leer. */
static int game_load(void)
{
    save_t sv;
    int f = zxv_file_open(SAVE_FILE, ZXV_O_READ);
    if (f < 0)
        return 0;
    int ok = zxv_file_read(f, &sv, sizeof sv) == (int)sizeof sv && sv.magic == SAVE_MAGIC &&
             sv.zone < zone_count && zxv_file_read(f, gone, sizeof gone) == (int)sizeof gone &&
             zxv_file_read(f, seen, sizeof seen) == (int)sizeof seen;
    zxv_file_close(f);
    if (!ok) {                          /* lo que se leyera a medias, fuera: partida nueva */
        memset(gone, 0, sizeof gone);
        memset(seen, 0, sizeof seen);
        return 0;
    }
    got = sv.got;
    energy = sv.energy;
    storm_clock = sv.storm_clock;
    storm_strong = sv.storm_strong;
    storm = storm_strong ? 256 : 0;
    zone_enter(sv.zone, 0);
    px = sv.x;
    pz = sv.z;
    feet = sv.feet;
    yaw = sv.yaw;
    cp_zone = zone;
    cp_x = px;
    cp_z = pz;
    cp_feet = feet;
    cp_yaw = yaw;
    charging = 1;                       /* ya encima: no vuelve a guardar al llegar */
    return 1;
}

static void hurt(int32_t cost, const char *why)
{
    if (dead)
        return;
    energy -= cost;
    hurt_show = 8;
    say(why, 90);
}

/* --- Los rayos ---------------------------------------------------------------- */

/* Si el segmento de o a o + d corta la caja de la pieza p; en *t, dónde (en
 * 1/16384 del segmento). */
static int ray_box(const int32_t o[3], const int32_t d[3], const piece_t *p, int32_t *t)
{
    const int32_t lo[3] = { p->x, p->y, p->z }, sz[3] = { p->sx, p->sy, p->sz };
    int64_t tn = 0, tf = Q14;
    for (int k = 0; k < 3; k++) {
        int64_t a = lo[k] - o[k], b = a + sz[k];
        if (d[k] == 0) {
            if (a > 0 || b < 0)
                return 0;
            continue;
        }
        a = a * Q14 / d[k];
        b = b * Q14 / d[k];
        if (a > b) {
            int64_t s = a;
            a = b;
            b = s;
        }
        if (a > tn)
            tn = a;
        if (b < tf)
            tf = b;
        if (tn > tf)
            return 0;
    }
    *t = (int32_t)tn;
    return 1;
}

/* La primera pieza (sólida o criatura, y que no tenga ninguno de los flags
 * skip) que corta el segmento de o a o + d, o -1; en *t, dónde. */
static int ray_first(const int32_t o[3], const int32_t d[3], int skip, int32_t *t)
{
    int best = -1;
    int32_t bt = Q14 + 1, h;
    for (int i = 0; i < object_count; i++) {
        const piece_t *p = &objects[i].p;
        if (!(p->flags & (OBJ_SOLID | OBJ_CREATURE)) || (p->flags & (OBJ_HIDDEN | skip)))
            continue;
        if (ray_box(o, d, p, &h) && h < bt) {
            bt = h;
            best = i;
        }
    }
    *t = bt;
    return best;
}

/* Lo que se ve desde aquí, para el mapa: cada frame, MAP_STEP de los
 * MAP_RAYS rayos alrededor, a la altura de los ojos, hasta la primera pared
 * o hasta el alcance (más fuera que dentro). */
#define MAP_RAYS    64
#define MAP_STEP    8
static void map_reveal(void)
{
    int32_t r = zones[zone].outside ? 1500 : 800, eye[3] = { px, feet + EYE, pz }, t;
    for (int k = 0; k < MAP_STEP; k++, map_ray = (map_ray + 1) % MAP_RAYS) {
        int a = map_ray * (TURN / MAP_RAYS);
        int32_t d[3] = { isin(a) * r / Q14, 0, icos(a) * r / Q14 };
        ray_first(eye, d, OBJ_CREATURE | OBJ_SPRITE | OBJ_DECO, &t);
        int32_t len = t > Q14 ? r : r * t / Q14;
        for (int32_t s = 0; s <= len; s += map_cell / 2 + 1)
            map_mark(px + isin(a) * s / Q14, pz + icos(a) * s / Q14);
    }
}

/* El segmento desde los ojos hacia la cruz de la vista, de range cm de largo
 * (en el plano). */
static void view_ray(int32_t o[3], int32_t d[3], int32_t range)
{
    o[0] = px;
    o[1] = feet + EYE;
    o[2] = pz;
    d[0] = isin(yaw) * range / Q14;
    d[1] = look * range * 100 / 14080;  /* la cruz, a look píxeles del horizonte con el foco del modo 0 */
    d[2] = icos(yaw) * range / Q14;
}

static void fire(void)
{
    int32_t o[3], d[3], t;
    energy -= COST_SHOT;
    sound_fx(FX_SHOT);
    shot_delay = SHOT_DELAY;
    shot_show = 4;
    view_ray(o, d, SHOT_RANGE);
    int i = ray_first(o, d, 0, &t);
    shot_hit = i >= 0;
    if (i < 0 || !(objects[i].p.flags & OBJ_CREATURE))
        return;
    actor_t *a = &actors[i];
    a->flash = 6;
    if (a->kind != CR_MOTHER && (a->state == A_IDLE || a->state == A_HIDE || a->state == A_STALK)) {
        a->state = A_LUNGE;             /* herida, se lanza */
        a->timer = 90;
    }
    if (--a->hp == 0) {
        creature_kill(i);
        if (a->kind == CR_MOTHER)
            say(T("LA INCUBADORA HA MUERTO: EL NIDO SE CALMA", "THE INCUBATOR IS DEAD: THE NEST CALMS"), 150);
        else
            say(T("CRIATURA DESTRUIDA", "CREATURE DESTROYED"), 90);
        sound_fx(FX_KILL);
    }
}

/* Un registro se lee (y da lo que se sabe por él); una consola hace lo suyo
 * si se tiene lo que pide. */
/* Las ruedas de la bomba que se acaba de cerrar (la de la marca flag): giran. */
static void wheels_turn(uint16_t flag)
{
    for (int k = 0; k < object_count; k++)
        if (objects[k].p.flags & OBJ_LAMP && objects[k].p.log == L_WHEEL && objects[k].p.need == flag)
            actors[k].timer = WHEEL_TURN;
}

static void use(void)
{
    if (use_target < 0)
        return;
    const piece_t *p = &objects[use_target].p;
    if (p->flags & OBJ_LOG) {
        sound_fx(FX_LOG);
        panel = P_LOG;
        panel_log = p->log;
        gain(p->give);
    } else if (have(p->give)) {
        sound_fx(FX_DENIED);
        say(T("YA ESTÁ HECHO", "ALREADY DONE"), 120);
    } else if (!have(p->need)) {
        sound_fx(FX_DENIED);
        say(missing(p->need), 120);
    } else {
        sound_fx(FX_USE);
        gain(p->give);
        wheels_turn(p->give);
    }
}

/* --- Las criaturas y las torretas --------------------------------------------- */

static uint32_t isqrt(uint32_t v)
{
    uint32_t r = 0, bit = 1u << 30;
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
    return r;
}

/* --- Las criaturas ------------------------------------------------------------- */

/* Una criatura es una caja (la de los disparos y la que se ordena con las
 * piezas) que se ve como un sprite (art.c): el de su postura y del ángulo
 * desde el que la ve la cámara. */
#define CR_FOOT     40                  /* medio lado de lo que ocupa en el suelo */
#define CR_HIGH     150                 /* y su altura, para chocar */
#define CR_REACH    (RADIUS + 90)       /* hasta dónde llega su golpe */
#define CR_TURN     24                  /* lo que gira por frame */
#define REAR_TIME   30                  /* lo que tarda en erguirse y golpear */

/* El ángulo (en 1/1024 de vuelta, 0 al norte) hacia (x, z). */
static int iatan2(int32_t x, int32_t z)
{
    int32_t ax = x < 0 ? -x : x, az = z < 0 ? -z : z;
    if (!ax && !az)
        return 0;
    int swap = ax > az;
    int64_t r = swap ? (int64_t)az * Q14 / ax : (int64_t)ax * Q14 / az;    /* de 0 a 1 */
    /* atan(r) ~ (pi/4) r + 0,273 r (1 - r), en 1/1024 de vuelta. */
    int a = (int)((128 * r + 44 * r * (Q14 - r) / Q14) / Q14);
    if (swap)
        a = 256 - a;
    if (z < 0)
        a = 512 - a;
    if (x < 0)
        a = -a;
    return a & (TURN - 1);
}

/* Si una criatura en (x, z) choca con algo sólido. */
static int creature_blocked(int32_t x, int32_t z)
{
    for (int i = 0; i < object_count; i++) {
        const piece_t *p = &objects[i].p;
        if (!(p->flags & OBJ_SOLID) || p->y >= CR_HIGH)
            continue;
        if (x - CR_FOOT < p->x + p->sx && x + CR_FOOT > p->x && z - CR_FOOT < p->z + p->sz &&
            z + CR_FOOT > p->z)
            return 1;
    }
    return 0;
}

/* La caja de la criatura i donde está y el sprite que le toca: su postura
 * (andando, erguida, golpeando, herida o muriendo) y el ángulo desde el que
 * la ve el jugador, de los ocho (cinco dibujos y tres en espejo). */
static void creature_place(int i)
{
    const actor_t *a = &actors[i];
    object_t *o = &objects[i];
    if (a->kind == CR_MOTHER) {         /* no se mueve: solo late, escupe o muere */
        int k = a->timer > 20 ? 0 : a->timer > 10 ? 1 : 2;
        o->spr = art_sprite(a->state == A_DEAD ? SPR_MOTHER_DEATH + k
                            : a->state == A_REAR ? SPR_MOTHER_SPIT : SPR_MOTHER + (int)(frame / 24 & 1));
        o->spr_flip = 0;
        o->spr_flash = (uint8_t)(a->state != A_DEAD && a->flash > 3);
        o->spr_cm = 6;                  /* más grande que las demás */
        return;
    }
    int32_t cx = a->x / 16, cz = a->z / 16;
    o->p.x = cx - 60;
    o->p.z = cz - 60;
    object_setup(o);
    o->spr_flip = 0;
    o->spr_flash = 0;
    if (a->state == A_DEAD) {
        int k = a->timer > 20 ? 0 : a->timer > 10 ? 1 : 2;
        o->spr = art_sprite(SPR_DEATH + k);
        return;
    }
    int rel = (iatan2(px - cx, pz - cz) - a->yaw) & (TURN - 1);
    int sector = ((rel + TURN / 16) / (TURN / 8)) & 7;
    int ang = sector > 4 ? 8 - sector : sector;
    int pose = SPR_WALK_A;
    if (a->state == A_REAR)
        pose = a->timer > 10 ? SPR_REAR : SPR_STRIKE;
    else if (a->flash)
        pose = SPR_HURT;
    else if (a->walk)
        pose = (a->walk / 8) & 1 ? SPR_WALK_B : SPR_WALK_A;
    o->spr = art_sprite(pose + ang);
    o->spr_flip = (uint8_t)(sector > 4);
    o->spr_flash = (uint8_t)(a->flash > 3);
}

/* Las criaturas de la zona, al entrar: dormidas, escondidas o muertas. */
static void creatures_spawn(void)
{
    blip_count = 0;
    mother = spit = -1;
    for (int i = 0; i < object_count; i++)
        if (objects[i].p.flags & OBJ_SPIT)
            spit = i;
    for (int i = 0; i < object_count; i++) {
        object_t *o = &objects[i];
        if (!(o->p.flags & OBJ_SPRITE))
            continue;
        actor_t *a = &actors[i];
        a->kind = o->p.log;
        a->state = a->kind == CR_AMBUSH ? A_HIDE : A_IDLE;
        if (a->kind == CR_MOTHER) {
            mother = i;
            a->hp = MOTHER_HP;
            a->timer = SPIT_DELAY;
        }
        a->x = (o->p.x + 60) * 16;
        a->z = (o->p.z + 60) * 16;
        a->yaw = (int16_t)((i * 137) & (TURN - 1));
        if (gone[zone][i]) {            /* la que se mató, en el suelo */
            o->p.flags &= (uint16_t)~(OBJ_CREATURE | OBJ_HIDDEN);
            a->state = A_DEAD;
            a->timer = 0;
        }
        creature_place(i);
    }
}

/* La criatura i, destruida: cae (A_DEAD) y se queda en el suelo. */
static void creature_kill(int i)
{
    objects[i].p.flags &= (uint16_t)~OBJ_CREATURE;
    actors[i].state = A_DEAD;
    actors[i].timer = 30;
    gone[zone][i] = 1;
}

/* La que cae: sus tres sprites, y luego el último. */
static void corpse_update(int i)
{
    if (actors[i].timer > 0)
        actors[i].timer--;
    creature_place(i);
}

/* La incubadora no se mueve (el sensor no la ve). Cada SPIT_DELAY frames, si
 * tiene al jugador a tiro y a la vista, borbotea (A_REAR) SPIT_WARN frames
 * y le escupe: una bola que va hacia donde estaba, así que se puede esquivar
 * apartándose. */
static void mother_update(int i)
{
    actor_t *a = &actors[i];
    const piece_t *p = &objects[i].p;
    int32_t m[3] = { p->x + p->sx / 2, p->y + 220, p->z - 20 };    /* la boca, hacia el sur */
    int32_t d[3] = { px - m[0], feet + 100 - m[1], pz - m[2] }, t;
    if (a->flash)
        a->flash--;
    a->walk = 0;
    if (a->state == A_REAR) {
        if (--a->timer <= 0) {
            a->state = A_IDLE;
            a->timer = SPIT_DELAY;
            if (spit >= 0) {
                int32_t len = (int32_t)isqrt((uint32_t)(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
                if (len < 1)
                    len = 1;
                spit_x = m[0] * 16;
                spit_y = m[1] * 16;
                spit_z = m[2] * 16;
                spit_dx = d[0] * SPIT_SPEED * 16 / len;
                spit_dy = d[1] * SPIT_SPEED * 16 / len;
                spit_dz = d[2] * SPIT_SPEED * 16 / len;
                spit_life = len / SPIT_SPEED + 40;
                objects[spit].p.flags &= (uint16_t)~OBJ_HIDDEN;
                sound_fx(FX_SPIT);
            }
        }
    } else if (a->timer > 0) {
        a->timer--;
    } else if (spit >= 0 && objects[spit].p.flags & OBJ_HIDDEN &&
               (int64_t)d[0] * d[0] + (int64_t)d[2] * d[2] < (int64_t)SPIT_RANGE * SPIT_RANGE &&
               ray_first(m, d, OBJ_CREATURE | OBJ_SPRITE, &t) < 0) {
        a->state = A_REAR;
        a->timer = SPIT_WARN;
        sound_fx(FX_GURGLE);
    }
    creature_place(i);
}

/* Lo que escupió: avanza hasta dar al jugador, a una pared o perderse. */
static void spit_update(int i)
{
    object_t *o = &objects[i];
    int32_t from[3] = { spit_x / 16, spit_y / 16, spit_z / 16 };
    int32_t d[3] = { spit_dx / 16, spit_dy / 16, spit_dz / 16 }, t;
    spit_x += spit_dx;
    spit_y += spit_dy;
    spit_z += spit_dz;
    int32_t x = spit_x / 16, y = spit_y / 16, z = spit_z / 16;
    int hit = x > px - RADIUS - 25 && x < px + RADIUS + 25 && z > pz - RADIUS - 25 && z < pz + RADIUS + 25 &&
              y > feet && y < feet + BODY;
    if (hit) {
        hurt(COST_SPIT, T("¡TE HA ALCANZADO SU ESCUPITAJO!", "ITS SPIT HIT YOU!"));
        sound_fx(FX_BITE);
    }
    if (hit || --spit_life <= 0 || y < 0 || ray_first(from, d, OBJ_CREATURE | OBJ_SPRITE, &t) >= 0) {
        o->p.flags |= OBJ_HIDDEN;
        return;
    }
    o->p.x = x - o->p.sx / 2;
    o->p.y = y - o->p.sy / 2;
    o->p.z = z - o->p.sz / 2;
    object_setup(o);
}

/* Cada criatura caza a su manera (kind):
 *   - CR_AMBUSH espera quieta (A_HIDE) y sale de golpe (A_LUNGE, muy deprisa)
 *     cuando el jugador pasa cerca o le da la espalda; luego persigue;
 *   - CR_STALK lo sigue a distancia (A_STALK), se aparta si la mira y se
 *     lanza (A_LUNGE) cuando se para o le da la espalda;
 *   - CR_CHASE va directa a por él (A_CHASE), a su misma velocidad.
 * Cerca de él se yergue (A_REAR, con su chillido) y golpea si sigue al
 * alcance; luego se aparta (A_BACK). Van por el suelo, rodeando lo que
 * encuentran; con la planta parada, despiertan más cerca y van más despacio. */
static void creature_update(int i)
{
    actor_t *a = &actors[i];
    if (a->kind == CR_MOTHER) {
        mother_update(i);
        return;
    }
    /* Con la planta parada, o muerta la incubadora, se calman. */
    int calm = have(F_PLANT_OFF) || (mother >= 0 && actors[mother].state == A_DEAD);
    int32_t dx = px - a->x / 16, dz = pz - a->z / 16;
    int32_t d = (int32_t)isqrt((uint32_t)(dx * dx + dz * dz));
    int toward = iatan2(dx, dz);
    /* El jugador le da la espalda si ella queda detrás de su mirada. */
    int behind = (int64_t)isin(yaw) * dx + (int64_t)icos(yaw) * dz > 0;
    int looked = !behind && d < 1600 &&
                 (int64_t)isin(yaw) * -dx + (int64_t)icos(yaw) * -dz > (int64_t)d * Q14 * 9 / 10;
    int32_t wake = calm ? WAKE_CALM : WAKE_RANGE;
    int sp = 0, dir = toward;

    switch (a->state) {
    case A_IDLE:
        if (d < wake)
            a->state = a->kind == CR_STALK ? A_STALK : A_CHASE;
        break;
    case A_HIDE:
        if (d < 350 || (d < 800 && behind)) {
            a->state = A_LUNGE;
            a->timer = 90;
            sound_fx(FX_SCREECH);
        }
        break;
    case A_STALK:
        if ((still > 60 || behind) && d < 1400) {
            a->state = A_LUNGE;
            a->timer = 120;
        } else if (looked || d < 800) {
            sp = -50;                   /* se aparta */
        } else if (d > 1000) {
            sp = 60;
        }
        break;
    case A_CHASE:
        sp = 80;                        /* algo menos que el jugador (93) */
        break;
    case A_LUNGE:
        sp = a->kind == CR_AMBUSH ? 140 : 120;
        if (--a->timer <= 0)
            a->state = a->kind == CR_STALK ? A_STALK : A_CHASE;
        break;
    case A_REAR:
        if (--a->timer <= 0) {
            if (d < CR_REACH + 40) {
                hurt(COST_BITE, T("¡UNA CRIATURA TE HA ALCANZADO!", "A CREATURE HIT YOU!"));
                sound_fx(FX_BITE);
            }
            a->state = A_BACK;
            a->timer = BITE_BACK;
        }
        break;
    case A_BACK:
        sp = -50;
        if (--a->timer <= 0)
            a->state = a->kind == CR_STALK ? A_STALK : A_CHASE;
        break;
    }
    if ((a->state == A_CHASE || a->state == A_LUNGE) && d < CR_REACH) {
        a->state = A_REAR;
        a->timer = REAR_TIME;
        sound_fx(FX_SCREECH);
        sp = 0;
    }
    if (calm)
        sp = sp * 6 / 10;

    /* Hacia donde va (o, si se aparta, de espaldas a él); si choca, prueba a
     * los lados, primero por el que probó la última vez. */
    if (sp) {
        int go = dir;
        int moved = 0;
        static const int tries[5] = { 0, 64, 128, 192, 256 };
        for (int k = 0; k < 9 && !moved; k++) {
            int an = go + (k ? (k & 1 ? 1 : -1) * a->side * tries[(k + 1) / 2] : 0);
            int32_t nx = a->x + isin(an) * sp / Q14, nz = a->z + icos(an) * sp / Q14;
            if (!creature_blocked(nx / 16, nz / 16)) {
                a->x = nx;
                a->z = nz;
                moved = 1;
                if (k)
                    dir = an;           /* mira hacia donde rodea */
            }
        }
        if (!moved)
            a->side = (int8_t)-a->side;
        a->walk++;
    } else {
        a->walk = 0;
    }

    /* Gira poco a poco hacia donde va o hacia el jugador. */
    int want = sp < 0 ? toward : dir;
    if (a->state == A_HIDE || a->state == A_IDLE)
        want = a->yaw;
    int diff = ((want - a->yaw + TURN / 2) & (TURN - 1)) - TURN / 2;
    a->yaw = (int16_t)((a->yaw + (diff > CR_TURN ? CR_TURN : diff < -CR_TURN ? -CR_TURN : diff)) & (TURN - 1));
    if (a->flash)
        a->flash--;
    creature_place(i);
}

/* El sensor de movimiento. Cada pulso sale del jugador y llega a TRACK_RANGE
 * en TRACK_SWEEP frames; lo que se mueve delante de él (las criaturas que
 * andan o se yerguen, no las quietas) aparece cuando el pulso lo alcanza y se
 * apaga poco a poco. Con lo más cercano, un pitido, más agudo y con los
 * pulsos más seguidos cuanto más cerca. */
#define TRACK_RANGE 2000
#define TRACK_SWEEP 24                  /* los frames que tarda el pulso */
#define TRACK_IDLE  40                  /* un pulso cada tanto, si no hay nada */
#define BLIP_LIFE   40

/* Para lo que inventa el sensor: no cuenta para nada más del juego. */
static uint32_t track_seed = 1;
static int track_rand(int n)
{
    track_seed = track_seed * 1103515245u + 12345u;
    return (int)((track_seed >> 16) % (uint32_t)n);
}

/* Fuera, con tormenta, el sensor falla: de 0 a 256, más cuanto más fuerte. */
static int track_noise(void)
{
    if (!zones[zone].outside || storm <= STORM_MAX / 2)
        return 0;
    return (storm - STORM_MAX / 2) * 256 / (256 - STORM_MAX / 2);
}

static void tracker_update(void)
{
    for (int k = 0; k < blip_count; k++)
        if (blips[k].age >= 0 && blips[k].age < BLIP_LIFE)
            blips[k].age++;
    if (++track_t >= track_period) {
        /* Un pulso nuevo, con lo que se mueve ahora. */
        track_t = 0;
        blip_count = 0;
        int noise = track_noise();
        int32_t near = TRACK_RANGE;
        for (int i = 0; i < object_count && blip_count < TRACK_MAX; i++) {
            if ((objects[i].p.flags & (OBJ_CREATURE | OBJ_HIDDEN)) != OBJ_CREATURE)
                continue;
            const actor_t *a = &actors[i];
            if ((!a->walk && a->state != A_REAR) || a->kind == CR_MOTHER)
                continue;
            int32_t dx = a->x / 16 - px, dz = a->z / 16 - pz;
            int32_t d = (int32_t)isqrt((uint32_t)(dx * dx + dz * dz));
            int rel = ((iatan2(dx, dz) - yaw + TURN / 2) & (TURN - 1)) - TURN / 2;
            if (d >= TRACK_RANGE || rel < -TURN / 4 || rel > TURN / 4)
                continue;
            if (track_rand(384) < noise)
                continue;                   /* la tormenta lo tapa */
            blips[blip_count++] = (blip_t){ (int16_t)d, (int16_t)rel, -1 };
            if (d < near)
                near = d;
        }
        /* Y lo que no está. */
        for (int k = noise ? track_rand(1 + noise / 80) : 0; k > 0 && blip_count < TRACK_MAX; k--) {
            int32_t d = 300 + track_rand(TRACK_RANGE - 300);
            blips[blip_count++] = (blip_t){ (int16_t)d, (int16_t)(track_rand(TURN / 2) - TURN / 4), -1 };
            if (d < near)
                near = d;
        }
        track_period = blip_count ? TRACK_SWEEP + 8 + (int)(near * 40 / TRACK_RANGE) : TRACK_IDLE;
    }
    /* Lo que alcanza el pulso en este frame; con lo primero, el pitido. */
    int32_t r0 = (int32_t)track_t * TRACK_RANGE / TRACK_SWEEP;
    int32_t r1 = (int32_t)(track_t + 1) * TRACK_RANGE / TRACK_SWEEP;
    int first = 1;
    for (int k = 0; k < blip_count; k++) {
        if (blips[k].age >= 0)
            first = 0;
        else if (blips[k].d >= r0 && blips[k].d < r1)
            blips[k].age = 0;
    }
    for (int k = 0; k < blip_count && first; k++)
        if (blips[k].age == 0) {
            sound_fx(blips[k].d < 700 ? FX_PING_NEAR : blips[k].d < 1300 ? FX_PING_MID : FX_PING_FAR);
            first = 0;
        }
}

/* La cabeza de la torreta: con rayas, o parpadeando mientras apunta. */
static void turret_look(object_t *o, const actor_t *a)
{
    int m = a->state && (a->state / 4) & 1 ? MAT_GLOW : MAT_HAZARD;
    if (o->p.mat[0] == m)
        return;
    for (int f = 0; f < FACES_MAX; f++)
        o->p.mat[f] = (uint8_t)m;
    object_setup(o);
}

/* Mientras las defensas están activas, la torreta gira hacia el jugador si
 * lo tiene a tiro y, cuando le apunta y lo ve, avisa (parpadea y pita)
 * durante TURRET_LOCK frames antes de disparar, siguiéndolo; si deja de verlo
 * o de tenerlo a tiro, vuelve a empezar. En state, lo que lleva avisando. */
static void turret_update(int i)
{
    object_t *o = &objects[i];
    actor_t *a = &actors[i];
    if (a->timer)
        a->timer--;
    if (o->p.need && have(o->p.need))
        return;                         /* apagada */
    int lock = a->state;
    a->state = 0;
    int32_t c[3] = { o->p.x + o->p.sx / 2, o->p.y + o->p.sy / 2, o->p.z + o->p.sz / 2 };
    int32_t vx = px - c[0], vz = pz - c[2];
    int32_t d = (int32_t)isqrt((uint32_t)(vx * vx + vz * vz));
    if (d > TURRET_RANGE) {
        turret_look(o, a);
        return;
    }
    int32_t fx = isin(o->yaw), fz = icos(o->yaw);
    int64_t cross = (int64_t)fz * vx - (int64_t)fx * vz, dot = (int64_t)fx * vx + (int64_t)fz * vz;
    int64_t aim = (int64_t)d * 820;     /* unos 3 grados */
    if (dot < 0 || cross > aim)
        o->yaw += 4;
    else if (cross < -aim)
        o->yaw -= 4;
    object_setup(o);
    int32_t dv[3] = { px - c[0], feet + EYE - c[1], pz - c[2] }, t;
    int aimed = dot >= 0 && cross <= aim && cross >= -aim;
    if (a->timer || (!lock && !aimed) || ray_first(c, dv, OBJ_TURRET | OBJ_CREATURE, &t) >= 0) {
        turret_look(o, a);              /* sin apuntar todavía, o algo tapa al jugador */
        return;
    }
    if (!lock)
        sound_fx(FX_LOCK);
    a->state = (int8_t)(lock + 1);
    turret_look(o, a);
    if (a->state < TURRET_LOCK)
        return;
    a->state = 0;
    turret_look(o, a);
    a->timer = TURRET_DELAY;
    beam_show = 5;
    beam_from = i;
    hurt(COST_TURRET, T("¡LA TORRETA TE HA ALCANZADO!", "THE TURRET HIT YOU!"));
    sound_fx(FX_TURRET);
}

/* Los objetos se ven como sprites (los de art.c, a 2 cm por píxel): el de
 * lo que dan. */
static void items_setup(void)
{
    static const struct { uint16_t give; uint8_t spr; } look[] = {
        { F_CARD, SPR_CARD }, { F_FILTER, SPR_FILTER }, { F_CUTTER, SPR_CUTTER }, { F_PART1, SPR_PART1 },
        { F_PART2, SPR_PART2 }, { F_PART3, SPR_PART3 }, { F_PART4, SPR_PART4 },
    };
    for (int i = 0; i < object_count; i++) {
        object_t *o = &objects[i];
        if (!(o->p.flags & OBJ_ITEM))
            continue;
        o->spr = 0;
        for (unsigned k = 0; k < sizeof look / sizeof look[0]; k++)
            if (o->p.give == look[k].give)
                o->spr = art_sprite(look[k].spr);
        o->p.flags |= OBJ_SPRITE;
        o->spr_flip = o->spr_flash = 0;
        o->spr_cm = 2;
    }
}

/* Los objetos flotan, subiendo y bajando; se cogen al pasar por encima. */
static void item_update(int i)
{
    object_t *o = &objects[i];
    const piece_t *b = &zones[zone].pieces[i];
    o->p.y = b->y + 15 * isin((int)frame * 10) / Q14;
    object_setup(o);
    int32_t reach = RADIUS + o->p.sx / 2 + 40;     /* lo que hay en una mesa, desde delante */
    int32_t dx = px - (o->p.x + o->p.sx / 2), dz = pz - (o->p.z + o->p.sz / 2);
    if (dx < -reach || dx > reach || dz < -reach || dz > reach || o->p.y > feet + BODY ||
        o->p.y + o->p.sy < feet)
        return;
    o->p.flags |= OBJ_HIDDEN;
    gone[zone][i] = 1;
    sound_fx(FX_PICKUP);
    gain(b->give);
}

/* --- Las colisiones ----------------------------------------------------------- */

static int32_t clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

/* Lo que sube una ladera de altura h que baja hasta 0 a una distancia half
 * desde lo más alto, a la distancia d. */
static int32_t slope(int32_t h, int32_t d, int32_t half)
{
    if (d < 0)
        d = -d;
    return d >= half ? 0 : (int32_t)((int64_t)h * (half - d) / half);
}

/* Lo más alto de la pieza bajo el cuerpo del jugador en (x, z). */
static int32_t top_under(const piece_t *p, int32_t x, int32_t z)
{
    int32_t cx = p->x + p->sx / 2, cz = p->z + p->sz / 2;
    int32_t dx = clamp(cx, x - RADIUS, x + RADIUS) - cx, dz = clamp(cz, z - RADIUS, z + RADIUS) - cz;
    switch (p->shape) {
    case SH_PYRAMID: {
        int32_t a = slope(p->sy, dx, p->sx / 2), b = slope(p->sy, dz, p->sz / 2);
        return p->y + (a < b ? a : b);
    }
    case SH_PRISM:
        return p->y + slope(p->sy, dx, p->sx / 2);
    case SH_WEDGE:
        return p->y + p->sy * clamp(z + RADIUS - p->z, 0, p->sz) / p->sz;
    default:
        return p->y + p->sy;
    }
}

static const piece_t *door_hit;

/* Si el jugador cabe en (x, z) con los pies en feet; en *ground, lo más
 * alto sobre lo que puede quedarse (subiendo un escalón como mucho). Si choca
 * con una puerta, aunque también choque con la pared de al lado, en door_hit. */
static int fits(int32_t x, int32_t z, int32_t *ground)
{
    int32_t g = 0;
    int ok = 1;
    for (int i = 0; i < object_count; i++) {
        const piece_t *p = &objects[i].p;
        if (!(p->flags & OBJ_SOLID))
            continue;
        if (x + RADIUS <= p->x || x - RADIUS >= p->x + p->sx || z + RADIUS <= p->z || z - RADIUS >= p->z + p->sz)
            continue;
        int32_t top = top_under(p, x, z);
        if (top <= feet + STEP) {
            if (top > g)
                g = top;
        } else if (feet + BODY > p->y) {
            ok = 0;
            if (p->flags & OBJ_DOOR)
                door_hit = p;
        }
    }
    *ground = g;
    return ok;
}

static void walk(int32_t dx, int32_t dz)
{
    int32_t g;
    if (dx && fits(px + dx, pz, &g))
        px += dx;
    if (dz && fits(px, pz + dz, &g))
        pz += dz;
}

/* --- La entrada --------------------------------------------------------------- */

enum { IN_FWD = 1, IN_BACK = 2, IN_LEFT = 4, IN_RIGHT = 8, IN_SLEFT = 16, IN_SRIGHT = 32,
       IN_UP = 64, IN_DOWN = 128, IN_CENTRE = 256, IN_MODE = 512, IN_ESC = 1024, IN_FIRE = 2048,
       IN_USE = 4096, IN_ITEMS = 8192, IN_MAP = 16384, IN_HELP = 32768 };

static int key(unsigned hid)
{
    const volatile uint8_t *map = (const volatile uint8_t *)(uintptr_t)ZXV_I_KEYMAP;
    return (map[hid >> 3] >> (hid & 7)) & 1;
}

static int read_input(void)
{
    int in = 0;
    if (key(0x14) || key(0x52)) in |= IN_FWD;
    if (key(0x04) || key(0x51)) in |= IN_BACK;
    if (key(0x12) || key(0x50)) in |= IN_LEFT;
    if (key(0x13) || key(0x4F)) in |= IN_RIGHT;
    if (key(0x1D)) in |= IN_SLEFT;
    if (key(0x1B)) in |= IN_SRIGHT;
    if (key(0x1A)) in |= IN_UP;
    if (key(0x16)) in |= IN_DOWN;
    if (key(0x06)) in |= IN_CENTRE;
    if (key(0x2C)) in |= IN_FIRE;
    if (key(0x10)) in |= IN_MAP;
    if (key(0x28) || key(0x18)) in |= IN_USE;
    if (key(0x0C)) in |= IN_ITEMS;
    if (key(0x0B)) in |= IN_HELP;
    if (key(0x2B)) in |= IN_MODE;
    if (key(0x29)) in |= IN_ESC;
    uint16_t joy = *(volatile uint16_t *)(uintptr_t)ZXV_I_JOY0;
    if (joy & ZXV_JOY_CONNECTED) {
        if (joy & ZXV_JOY_UP) in |= IN_FWD;
        if (joy & ZXV_JOY_DOWN) in |= IN_BACK;
        if (joy & ZXV_JOY_LEFT) in |= IN_LEFT;
        if (joy & ZXV_JOY_RIGHT) in |= IN_RIGHT;
        if (joy & ZXV_JOY_FIRE1) in |= IN_FIRE;
        if (joy & ZXV_JOY_FIRE2) in |= IN_USE;
    }
    return in;
}

/* Si el jugador está encima de un punto de recarga. */
static int on_charger(void)
{
    for (int i = 0; i < object_count; i++) {
        const piece_t *p = &objects[i].p;
        if ((p->flags & OBJ_CHARGE) && have(p->need) && px >= p->x && px < p->x + p->sx && pz >= p->z &&
            pz < p->z + p->sz && feet == p->y + p->sy)
            return 1;
    }
    return 0;
}

/* La tormenta de este frame: fuera, cada STORM_CALM frames llega una de
 * STORM_LEN, con un aviso antes; la fuerte llega y se queda. */
static void storm_update(void)
{
    int out = zones[zone].outside;
    uint32_t t = storm_clock++ % (STORM_CALM + STORM_LEN);
    int level = 0;
    if (t >= STORM_CALM) {
        t -= STORM_CALM;
        level = STORM_MAX;
        if (t < STORM_RAMP)
            level = (int)(STORM_MAX * t / STORM_RAMP);
        else if (t >= STORM_LEN - STORM_RAMP)
            level = (int)(STORM_MAX * (STORM_LEN - t) / STORM_RAMP);
    } else if (t == STORM_CALM - STORM_WARN && out && !storm_strong) {
        say(T("SE ACERCA UNA TORMENTA", "A STORM IS COMING"), 180);
        sound_fx(FX_SIREN);
    }
    if (storm_strong) {
        if (storm < 256)
            storm += 2;
        if (storm > 256)
            storm = 256;
    } else
        storm = level;
    if (out && storm > STORM_MAX / 2)
        energy -= STORM_COST;
}

static void update(int in, int edge, int dt)
{
    if (dead) {
        dead -= dt;
        if (dead <= 0) {
            dead = 0;
            respawn();
        }
        return;
    }
    if (panel) {
        if (panel == P_TITLE && saved && edge & (IN_FWD | IN_BACK))
            title_pick ^= 1;
        if (edge & (IN_USE | IN_FIRE | IN_ITEMS) || (panel == P_MAP && edge & IN_MAP) ||
            (panel == P_HELP && edge & IN_HELP)) {
            if (panel == P_END)
                game_start();           /* y vuelve al título */
            else if (panel == P_TITLE && saved && !title_pick && game_load()) {
                panel = P_NONE;         /* sin la introducción */
                hint_time = HINT_TIME;
            } else {
                if (panel == P_INTRO)
                    hint_time = HINT_TIME;
                panel = panel == P_TITLE ? P_INTRO : P_NONE;
            }
        }
        return;
    }
    if (edge & IN_ITEMS) {
        panel = P_ITEMS;
        return;
    }
    if (edge & IN_MAP) {
        panel = P_MAP;
        return;
    }
    if (edge & IN_HELP) {
        panel = P_HELP;
        return;
    }
    if (edge & IN_USE) {
        use();
        if (panel)
            return;
    }
    for (int f = 0; f < dt; f++) {
        frame++;
        if (shot_delay)
            shot_delay--;
        if (in & IN_FIRE && !shot_delay)
            fire();
        /* El paso de este frame, en 1/16384 de cm; lo que no llega a un cm
         * se guarda para el siguiente. */
        int32_t step = WALK * Q14 / ZXV_FRAME_HZ;
        int64_t fx = isin(yaw), fz = icos(yaw);
        int64_t fwd = (in & IN_FWD ? step : 0) - (in & IN_BACK ? step : 0);
        int64_t side = (in & IN_SRIGHT ? step : 0) - (in & IN_SLEFT ? step : 0);
        move_x += (fwd * fx + side * fz) / Q14;
        move_z += (fwd * fz - side * fx) / Q14;
        int32_t dx = move_x / Q14, dz = move_z / Q14;
        move_x -= dx * Q14;
        move_z -= dz * Q14;
        door_hit = 0;
        walk(dx, dz);
        if (dx || dz)
            still = 0;
        else if (still < 1000)
            still++;
        if (door_hit) {
            if (have(door_hit->need)) {
                sound_fx(FX_DOOR);
                zone_enter(door_hit->target, door_hit->entry);
                return;
            }
            if (!msg_time || msg != missing(door_hit->need)) {
                sound_fx(FX_DENIED);
                say(missing(door_hit->need), 90);
            }
        }
        if ((dx || dz) && zones[zone].outside)
            energy -= COST_WALK;
        if (zones[zone].toxic)
            energy -= have(F_FILTER) ? (zones[zone].toxic + FILTERED - 1) / FILTERED : zones[zone].toxic;
        storm_update();
        int turn = ((in & IN_RIGHT ? SPIN : 0) - (in & IN_LEFT ? SPIN : 0)) + spin_rest;
        yaw += turn / ZXV_FRAME_HZ;
        spin_rest = turn % ZXV_FRAME_HZ;
        if (in & IN_UP && look < LOOK_MAX) look++;
        if (in & IN_DOWN && look > -LOOK_MAX) look--;
        if (in & IN_CENTRE) look = 0;
        /* Sube los escalones; si no hay nada debajo, cae. */
        int32_t g;
        if (!fits(px, pz, &g))
            g = feet;
        if (g >= feet) {
            feet = g;
            fall = 0;
        } else {
            fall += 4;
            feet -= fall / 16;
            if (feet < g) {
                feet = g;
                fall = 0;
            }
        }
        map_reveal();
        int was_charging = charging;
        charging = on_charger();
        if (charging && !was_charging && game_save())
            say(T("PARTIDA GUARDADA", "GAME SAVED"), 90);
        if (charging) {
            cp_zone = zone;
            cp_x = px;
            cp_z = pz;
            cp_feet = feet;
            cp_yaw = yaw;
        }
        if (charging && energy < ENERGY_MAX) {
            if (frame % 30 == 0)
                sound_fx(FX_CHARGE);
            energy += CHARGE_RATE;
            if (energy > ENERGY_MAX)
                energy = ENERGY_MAX;
        } else if (energy < ENERGY_MAX / 4 && frame % 120 == 0)
            sound_fx(FX_LOW);
        for (int i = 0; i < object_count; i++) {
            int fl = objects[i].p.flags;
            if (fl & OBJ_HIDDEN)
                continue;
            if (fl & OBJ_CREATURE)
                creature_update(i);
            else if (fl & OBJ_ITEM)
                item_update(i);
            else if (fl & OBJ_SPRITE)
                corpse_update(i);
            else if (fl & OBJ_SPIT)
                spit_update(i);
            else if (fl & OBJ_TURRET)
                turret_update(i);
            else if (fl & OBJ_LAMP)
                lamp_update(i);
            if (panel)
                return;                 /* el final */
        }
        tracker_update();
        if (shot_show) shot_show--;
        if (hurt_show) hurt_show--;
        if (beam_show) beam_show--;
        if (msg_time) msg_time--;
        if (hint_time) hint_time--;
        if (energy <= 0) {
            energy = 0;
            dead = 180;
            shot_show = hurt_show = beam_show = 0;
            sound_ambient(-1);
            sound_fx(FX_DEATH);
            say(T("ENERGÍA AGOTADA", "SUIT ENERGY DEPLETED"), 180);
            return;
        }
    }
    /* Lo que se usaría con Intro, para el aviso. */
    int32_t o[3], d[3], t;
    view_ray(o, d, USE_RANGE);
    use_target = ray_first(o, d, 0, &t);
    if (use_target >= 0 && !(objects[use_target].p.flags & (OBJ_USE | OBJ_LOG)))
        use_target = -1;
}

/* --- El marcador -------------------------------------------------------------- */

static char *put(char *p, const char *s)
{
    while (*s)
        *p++ = *s++;
    *p = 0;
    return p;
}

static char *put_num(char *p, int32_t v)
{
    char tmp[12];
    int n = 0;
    if (v < 0) {
        *p++ = '-';
        v = -v;
    }
    do {
        tmp[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n)
        *p++ = tmp[--n];
    *p = 0;
    return p;
}


/* El sensor, en la esquina del marcador: el abanico de delante (la mitad del
 * alcance, de puntos), el pulso que sale y lo que ha visto, que se apaga:
 * blanco, verde y verde oscuro (el 8 del marcador). Con la tormenta, ruido. */
static void tracker_draw(void)
{
    int z = hires + 1, cx = 230 * z, cy = view_h + 25 * z, R = 22 * z;
    for (int a = -TURN / 4; a <= TURN / 4; a += 8)
        rect(cx + R * isin(a) / Q14, cy - R * icos(a) / Q14, z, z, 8);
    for (int a = -TURN / 4; a <= TURN / 4; a += 32)
        rect(cx + R / 2 * isin(a) / Q14, cy - R / 2 * icos(a) / Q14, z, z, 8);
    rect(cx - R, cy, 2 * R + z, z, 8);
    for (int k = track_noise() / 8; k > 0; k--) {
        int r = track_rand(R), a = track_rand(TURN / 2) - TURN / 4;
        rect(cx + r * isin(a) / Q14, cy - r * icos(a) / Q14, z, z, track_rand(3) ? 8 : 4);
    }
    if (track_t < TRACK_SWEEP) {
        int r = R * track_t / TRACK_SWEEP;
        for (int a = -TURN / 4; a <= TURN / 4; a += 16)
            rect(cx + r * isin(a) / Q14, cy - r * icos(a) / Q14, z, z, 4);
    }
    for (int k = 0; k < blip_count; k++) {
        const blip_t *b = &blips[k];
        if (b->age < 0 || b->age >= BLIP_LIFE)
            continue;
        int r = R * b->d / TRACK_RANGE;
        rect(cx + r * isin(b->rel) / Q14 - z, cy - r * icos(b->rel) / Q14 - z, 2 * z, 2 * z,
             b->age < 8 ? 15 : b->age < 24 ? 12 : 8);
    }
}

/* Con los colores de siempre: 1 azul, 2 rojo, 4 verde, 5 cian, 6 amarillo, 7 blanco. */
/* El marcador es el visor del traje: paneles con el borde en relieve, con
 * los colores del marcador (video.c): a la izquierda, la zona y la energía;
 * en el centro, la brújula y lo que se lleva; a la derecha, el sensor; y
 * debajo, los avisos. Medidas en píxeles del modo 0 (por z). */
static void hud_panel(int x, int y, int w, int h)
{
    int z = hires + 1;
    x *= z;
    y = view_h + y * z;
    w *= z;
    h *= z;
    rect(x, y, w, h, 1);
    rect(x, y, w, z, 13);
    rect(x, y, z, h, 13);
    rect(x, y + h - z, w, z, 3);
    rect(x + w - z, y, z, h, 3);
}

/* La energía: diez segmentos, en verde, ámbar o rojo (que parpadea), y el
 * tanto por ciento (la fuente no tiene el signo). */
static void hud_energy(int x, int y)
{
    int z = hires + 1, pct = (int)(energy * 100 / ENERGY_MAX);
    int c = pct > 50 ? 4 : pct > 25 ? 6 : 2;
    if (c == 2 && frame / 15 % 2)
        c = 3;
    for (int k = 0; k < 10; k++)
        rect(x + 6 * k * z, y, 5 * z, 7 * z, pct > 10 * k ? c : 3);
    char s[8];
    put_num(s, pct);
    text(x + (pct >= 100 ? 68 : pct >= 10 ? 74 : 80) * z, y, s, c == 3 ? 2 : c);
}

/* La brújula: los rumbos de 45 en 45 grados y, debajo, marcas cada 22,5,
 * con el norte en rojo; se ve un cuarto de vuelta a cada lado. */
static void hud_compass(int x0, int x1, int y)
{
    static const char *const es[8] = { "N", "NE", "E", "SE", "S", "SO", "O", "NO" };
    static const char *const en[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    int z = hires + 1, cx = (x0 + x1) / 2, half = (x1 - x0) / 2;
    for (int h = 0; h < TURN; h += TURN / 16) {
        int off = ((h - yaw + TURN / 2) & (TURN - 1)) - TURN / 2;
        if (off < -TURN / 4 || off > TURN / 4)
            continue;
        int x = cx + off * half / (TURN / 4);
        int major = h % (TURN / 8) == 0;
        rect(x, y + (major ? 7 : 8) * z, z, (major ? 2 : 1) * z, 13);
        if (!major)
            continue;
        const char *s = (lang ? en : es)[h / (TURN / 8)];
        int n = 0;
        while (s[n])
            n++;
        int w = (n * 6 - 1) * z;
        if (x - w / 2 >= x0 && x + w / 2 <= x1)
            text(x - w / 2, y, s, h ? 7 : 2);
    }
    rect(cx - z, y + 7 * z, 3 * z, 3 * z, 6);   /* hacia dónde se mira */
}

/* Lo que se lleva: la tarjeta, el filtro y el cortador, y aparte las cuatro
 * piezas, en pequeño (los sprites de art.c); lo que falta, un hueco. */
static void hud_items(int x, int y)
{
    static const uint8_t remap[16] = { 0, 3, 3, 3, 3, 3, 14, 13, 3, 9, 10, 11, 12, 3, 3, 15 };
    static const uint16_t flag[7] = { F_CARD, F_FILTER, F_CUTTER, F_PART1, F_PART2, F_PART3, F_PART4 };
    int z = hires + 1;
    for (int k = 0; k < 7; k++) {
        int ix = x + (14 * k + (k >= 3 ? 2 : 0)) * z;
        rect(ix, y, 12 * z, 12 * z, 0);
        if (have(flag[k]))
            art_icon(SPR_CARD + k, ix, y, 3 - z, remap);
    }
}

static void hud(void)
{
    int z = hires + 1;
    rect(0, view_h, scr_w, scr_h - view_h, 0);
    hud_panel(2, 2, 96, 25);
    hud_panel(100, 2, 104, 25);
    hud_panel(206, 2, 48, 25);
    hud_panel(2, 29, 252, 17);
    text(6 * z, view_h + 6 * z, lang ? zones[zone].name_en : zones[zone].name_es, 6);
    hud_energy(6 * z, view_h + 17 * z);
    hud_compass(104 * z, 200 * z, view_h + 4 * z);
    hud_items(103 * z, view_h + 14 * z);
    int low = energy < ENERGY_MAX / 4;
    tracker_draw();
    /* El aviso, o lo que pasa ahora. */
    const char *m = 0;
    int c = 6;
    if (msg_time)
        m = msg;
    else if (charging)
        m = energy < ENERGY_MAX ? T("RECARGANDO EL TRAJE", "RECHARGING THE SUIT") : T("TRAJE CARGADO", "SUIT CHARGED");
    else if (use_target >= 0)
        m = objects[use_target].p.flags & OBJ_LOG ? T("INTRO: LEER EL REGISTRO", "ENTER: READ THE LOG")
                                                   : T("INTRO: USAR", "ENTER: USE");
    else if (low && frame / 30 % 2) {
        m = T("ENERGÍA BAJA", "LOW ENERGY");
        c = 2;
    } else if (zones[zone].toxic) {
        m = have(F_FILTER) ? T("ATMÓSFERA TÓXICA: FILTRO PUESTO", "TOXIC AIR: FILTER ON")
                           : T("ATMÓSFERA TÓXICA", "TOXIC AIR");
        c = 4;
    } else if (zones[zone].outside && storm > STORM_MAX / 2) {
        m = T("TORMENTA", "STORM");
        c = 2;
    } else if (hint_time) {
        m = T("H: AYUDA", "H: HELP");
        c = 13;
    }
    if (m)
        text(6 * z, view_h + 34 * z, m, c);
}

/* Un recuadro sobre la vista con el texto s (líneas separadas por '\n'),
 * con los colores de la vista: 0 negro, 12 el de las pantallas, 15 la luz. */
static void panel_draw(const char *s)
{
    int z = hires + 1, x = 8 * z, y = 8 * z;
    rect(4 * z, 4 * z, scr_w - 8 * z, view_h - 8 * z, 12);
    rect(5 * z, 5 * z, scr_w - 10 * z, view_h - 10 * z, 0);
    while (*s) {
        char row[48];
        int n = 0;
        while (*s && *s != '\n' && n < 47)
            row[n++] = *s++;
        row[n] = 0;
        if (*s == '\n')
            s++;
        text(x, y + 2 * z, row, 15);
        y += 11 * z;
    }
    text(x, view_h - 16 * z, panel == P_END ? T("INTRO: VOLVER A EMPEZAR", "ENTER: START AGAIN")
                                            : T("INTRO: SEGUIR", "ENTER: CONTINUE"), 12);
}

/* El título: el nombre, grande, en una franja negra sobre la vista. */
/* Una línea centrada, contando los caracteres y no los bytes. */
static void title_line(const char *s, int y, int c)
{
    int z = hires + 1, m = 0;
    for (const char *q = s; *q; q++)
        m += (*q & 0xC0) != 0x80;
    text((scr_w - (m * 6 - 1) * z) / 2, y, s, c);
}

static void title_draw(void)
{
    static const char name[] = "GONE SILENT";
    int z = hires + 1, big = 3 * z, n = (int)sizeof name - 1;
    int y = view_h / 2 - 16 * z;
    rect(0, y - 10 * z, scr_w, 52 * z, 0);
    text_size((scr_w - (n * 6 - 1) * big) / 2, y, name, 15, big);
    if (!saved) {
        title_line(T("INTRO: EMPEZAR", "ENTER: START"), y + 30 * z, 12);
        return;
    }
    /* Con una guardada: continuarla o empezar otra, la elegida en blanco. */
    title_line(T("CONTINUAR", "CONTINUE"), y + 26 * z, title_pick ? 12 : 15);
    title_line(T("PARTIDA NUEVA", "NEW GAME"), y + 36 * z, title_pick ? 15 : 12);
}

/* El final: el despegue y una parte de cada pareja. */
static void ending_draw(void)
{
    static const uint8_t parts[3][2] = {
        { END_LIFT, END_LIFT }, { END_PLANT_ON, END_PLANT_OFF }, { END_NO_TRUTH, END_TRUTH },
    };
    const int pick[3] = { 0, have(F_PLANT_OFF), have(F_TRUTH) };
    char s[640], *p = s;
    *p = 0;
    for (int i = 0; i < 3; i++) {
        const log_text_t *e = &ending[parts[i][pick[i]]];
        if (i)
            p = put(p, "\n\n");
        p = put(p, T(e->es, e->en));
    }
    panel_draw(s);
}

/* La lista de lo que se lleva y se sabe. */
static void items_draw(void)
{
    char s[512], *p = s;
    int parts = 0;
    for (int b = 0; b < 4; b++)
        parts += (got >> b) & 1;
    p = put(p, T("OBJETOS\n\nPIEZAS DEL MÓDULO: ", "ITEMS\n\nLANDER PARTS: "));
    p = put_num(p, parts);
    p = put(p, T(" DE 4", " OF 4"));
    for (int b = 0; b < 4; b++)
        if (got & (1u << b)) {
            p = put(p, "\n  ");
            p = put(p, T(flag_info[b].name_es, flag_info[b].name_en));
        }
    for (int b = 4; b < FLAGS; b++)
        if ((got & (1u << b)) && flag_info[b].name_es) {
            p = put(p, "\n");
            p = put(p, T(flag_info[b].name_es, flag_info[b].name_en));
        }
    panel_draw(s);
}

/* El mapa de la zona, visto desde arriba con el norte (+z) hacia arriba, con
 * los colores de la vista: lo visto del suelo en el tono más oscuro (1), las
 * paredes a la altura del jugador (las que tocan algo visto) en metal claro
 * (7), los puntos de recarga en luz (15) y el jugador, una flecha verde (12). */
static void map_draw(void)
{
    int z = hires + 1;
    rect(4 * z, 4 * z, scr_w - 8 * z, view_h - 8 * z, 12);
    rect(5 * z, 5 * z, scr_w - 10 * z, view_h - 10 * z, 0);
    text(8 * z, 10 * z, lang ? zones[zone].name_en : zones[zone].name_es, 15);
    text(8 * z, view_h - 16 * z, T("M: CERRAR", "M: CLOSE"), 12);
    /* El área, entre las dos líneas: la zona, lo más grande que quepa. */
    int ax = 8 * z, ay = 20 * z, aw = scr_w - 16 * z, ah = view_h - 40 * z;
    int32_t num = aw, den = map_bw;
    if ((int64_t)aw * map_bh > (int64_t)ah * map_bw) {
        num = ah;
        den = map_bh;
    }
    int ww = (int)((int64_t)map_bw * num / den), hh = (int)((int64_t)map_bh * num / den);
    int ox = ax + (aw - ww) / 2, oy = ay + (ah - hh) / 2;
#define MX(x) (ox + (int)((int64_t)((x) - map_bx) * num / den))
#define MY(zz) (oy + hh - (int)((int64_t)((zz) - map_bz) * num / den))
    for (int j = 0; j < MAP_N; j++)
        for (int i = 0; i < MAP_N; i++)
            if (map_seen(i, j)) {
                int x0 = MX(map_x0 + i * map_cell), x1 = MX(map_x0 + (i + 1) * map_cell);
                int y0 = MY(map_z0 + (j + 1) * map_cell), y1 = MY(map_z0 + j * map_cell);
                if (x0 < ox) x0 = ox;
                if (x1 > ox + ww) x1 = ox + ww;
                if (y0 < oy) y0 = oy;
                if (y1 > oy + hh) y1 = oy + hh;
                if (x1 > x0 && y1 > y0)
                    rect(x0, y0, x1 - x0, y1 - y0, 1);
            }
    for (int k = 0; k < object_count; k++) {
        const piece_t *p = &objects[k].p;
        if (!(p->flags & OBJ_SOLID) || p->flags & (OBJ_HIDDEN | OBJ_SPRITE | OBJ_ITEM | OBJ_DECO) ||
            p->sx > MAP_HUGE || p->sz > MAP_HUGE)
            continue;
        int charger = p->flags & OBJ_CHARGE;
        if (!charger && (p->y + p->sy <= feet + STEP || p->y >= feet + BODY))
            continue;                   /* ni es pared ni está a su altura */
        /* Solo si toca algo visto (o lo tiene al lado). */
        int i0 = (p->x - map_x0) / map_cell - 1, i1 = (p->x + p->sx - map_x0) / map_cell + 1;
        int j0 = (p->z - map_z0) / map_cell - 1, j1 = (p->z + p->sz - map_z0) / map_cell + 1;
        int near = 0;
        for (int j = j0; j <= j1 && !near; j++)
            for (int i = i0; i <= i1 && !near; i++)
                near = map_seen(i, j);
        if (!near)
            continue;
        int x0 = MX(p->x), x1 = MX(p->x + p->sx), y0 = MY(p->z + p->sz), y1 = MY(p->z);
        if (x0 < ox) x0 = ox;
        if (x1 > ox + ww) x1 = ox + ww;
        if (y0 < oy) y0 = oy;
        if (y1 > oy + hh) y1 = oy + hh;
        rect(x0, y0, x1 > x0 ? x1 - x0 : z, y1 > y0 ? y1 - y0 : z, charger ? 15 : 7);
    }
    /* El jugador: una flecha hacia donde mira, dentro del área. */
    int cx = MX(px), cy = MY(pz), l = 5 * z;
    cx = cx < ox ? ox : cx > ox + ww ? ox + ww : cx;
    cy = cy < oy ? oy : cy > oy + hh ? oy + hh : cy;
    int fx = isin(yaw) * l / Q14, fy = -icos(yaw) * l / Q14;
    int bx = cx - fx, by = cy - fy;
    line(cx + fx, cy + fy, bx - fy * 2 / 3, by + fx * 2 / 3, 12);
    line(cx + fx, cy + fy, bx + fy * 2 / 3, by - fx * 2 / 3, 12);
    line(bx - fy * 2 / 3, by + fx * 2 / 3, bx + fy * 2 / 3, by - fx * 2 / 3, 12);
#undef MX
#undef MY
}

/* Lo que se dibuja encima de la vista, con los colores de la vista: 0 negro,
 * 10 el naranja de las alarmas, 15 la luz. */
static void overlay(const camera_t *cam)
{
    int z = hires + 1, cx = scr_w / 2, cy = view_h / 2;
    if (shot_show) {
        line(scr_w * 3 / 10, view_h - 1, cx, cy, 15);
        line(scr_w * 7 / 10, view_h - 1, cx, cy, 15);
        if (shot_hit)
            rect(cx - 2 * z, cy - 2 * z, 5 * z, 5 * z, 15);
    }
    if (beam_show) {
        const object_t *o = &objects[beam_from];
        int sx, sy;
        if (project(cam, o->p.x + o->p.sx / 2, o->p.y + o->p.sy / 2, o->p.z + o->p.sz / 2, &sx, &sy)) {
            line(sx, sy, cx - 2 * z, view_h - 1, 10);
            line(sx, sy, cx + 2 * z, view_h - 1, 10);
        }
    }
    if (hurt_show) {
        int b = 3 * z;
        rect(0, 0, scr_w, b, 10);
        rect(0, view_h - b, scr_w, b, 10);
        rect(0, 0, b, view_h, 10);
        rect(scr_w - b, 0, b, view_h, 10);
    }
    if (dead) {
        const char *s = T("ENERGÍA AGOTADA", "SUIT ENERGY DEPLETED");
        int n = 0;
        for (const char *q = s; *q; q++)
            n += (*q & 0xC0) != 0x80;   /* los caracteres, no los bytes */
        rect(cx - (n * 3 + 8) * z, cy - 10 * z, (n * 6 + 16) * z, 20 * z, 0);
        text(cx - n * 3 * z, cy - 3 * z, s, 15);
        return;
    }
    if (panel == P_LOG)
        panel_draw(T(logs[panel_log].es, logs[panel_log].en));
    else if (panel == P_ITEMS)
        items_draw();
    else if (panel == P_MAP)
        map_draw();
    else if (panel == P_HELP)
        panel_draw(T("AYUDA\n\n"
                     "Q A O P, FLECHAS: ANDAR Y GIRAR\n"
                     "Z X: PASO A UN LADO   W S C: MIRAR\n"
                     "ESPACIO: DISPARAR   INTRO O U: USAR\n"
                     "I: OBJETOS   M: MAPA   H: ESTA AYUDA\n"
                     "TAB: MODO DE VÍDEO   ESC: SALIR\n"
                     "MANDO: BOTÓN 1 DISPARA, BOTÓN 2 USA",
                     "HELP\n\n"
                     "Q A O P, ARROWS: WALK AND TURN\n"
                     "Z X: SIDESTEP   W S C: LOOK\n"
                     "SPACE: FIRE   ENTER OR U: USE\n"
                     "I: ITEMS   M: MAP   H: THIS HELP\n"
                     "TAB: VIDEO MODE   ESC: QUIT\n"
                     "JOYSTICK: BUTTON 1 FIRES, BUTTON 2 USES"));
    else if (panel == P_TITLE)
        title_draw();
    else if (panel == P_INTRO)
        panel_draw(T(intro_text.es, intro_text.en));
    else if (panel == P_END)
        ending_draw();
    if (panel)
        return;
    rect(cx - 6 * z, cy, 4 * z, z, 15);
    rect(cx + 3 * z, cy, 4 * z, z, 15);
    rect(cx, cy - 6 * z, z, 4 * z, 15);
    rect(cx, cy + 3 * z, z, 4 * z, 15);
}

/* --- El paseo ----------------------------------------------------------------- */

static uint32_t frames_now(void) { return *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES; }

/* La música del título (y la introducción) o del final; jugando, el
 * ambiente: el viento de la tormenta fuera y el zumbido de la planta
 * mientras funciona. */
static void sound_update(void)
{
    sound_song(panel == P_TITLE || panel == P_INTRO ? SONG_TITLE : panel == P_END ? SONG_END : SONG_NONE);
    int amb = -1;
    if (!panel && !dead) {
        if (zones[zone].outside && storm)
            amb = storm > STORM_MAX ? FX_WIND_STRONG : FX_WIND;
        else if (zone == plant_zone && !have(F_PLANT_OFF))
            amb = FX_HUM;
    }
    if (panel != P_TITLE && panel != P_INTRO && panel != P_END)
        sound_ambient(amb);
    sound_frame();
}

int main(void)
{
    lang = zxv_lang_get() == ZXV_LANG_ES ? 0 : 1;
    int mode = 0, prev = IN_ESC | IN_MODE;  /* lo que esté pulsado al empezar no cuenta */
    video_init(mode);
    art_init();
    sound_init();
    game_start();
    uint32_t last = frames_now();
    for (;;) {
        int in = read_input(), edge = in & ~prev;
        prev = in;
        if (edge & IN_ESC)
            break;
        if (edge & IN_MODE) {
            mode ^= 1;
            video_init(mode);
            pal_shown = -1;
            palette_update();
            last = frames_now();
            continue;
        }
        uint32_t now = frames_now();
        int dt = (int)(now - last);
        last = now;
        update(in, edge, dt < 1 ? 1 : dt > 4 ? 4 : dt);

        video_begin();
        camera_t cam = { px, feet + EYE, pz, yaw, look };
        render_background(&cam, zones[zone].up, zones[zone].down);
        render_view(&cam);
        overlay(&cam);
        hud();
        sound_update();
        video_end();
        palette_update();               /* recién empezado el frame: sin cortes */
    }
    sound_done();
    video_done();
    while (zxv_key_get() >= 0)
        ;
    return 0;
}
