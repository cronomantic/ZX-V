/*
 * fight.c - Los luchadores de Street Justice: el jugador, los matones y los
 * jefes, sus golpes, la inteligencia de los enemigos, lo que se rompe y lo
 * que se come, y el piloto de la demostración.
 *
 * Todo está en un suelo con profundidad: x a lo largo de la calle, y hacia
 * el fondo o hacia delante (la línea de los pies) y z la altura de un salto.
 * Un golpe da si el otro está delante, a su alcance y casi a la misma
 * profundidad. Los enemigos se reparten «permisos» para atacar: solo dos a
 * la vez se acercan a pegar, los demás rondan a distancia.
 */
#include "street.h"

fighter_t fighters[MAX_F];
item_t things[MAX_ITEMS];
int lives, last_foe, last_foe_t;
static int prev_in = IN_A | IN_B;      /* la entrada del frame anterior */
uint32_t score;

#ifdef STREET_BOT
int bot_deaths, bot_hits, bot_boss_deaths;
#endif

/* --- Los golpes ------------------------------------------------------------------ */

enum { A_JAB, A_CROSS, A_UPPER, A_KNEE, A_SPIN, A_PUNCH, A_PUNCH2, A_KICK, A_SLAM, A_CHARGE, A_SWING,
       A_FLURRY, A_THROW, ATTACKS };

typedef struct {
    uint8_t pose_pre, pose_hit, pre, act, post;
    int8_t rmin, rmax;                  /* el alcance, delante (en píxeles) */
    uint8_t dmg, knock;
} attack_t;

static const attack_t attacks[ATTACKS] = {
    [A_JAB] = { P_IDLE, P_JAB, 2, 3, 7, 4, 25, 4, 0 },
    [A_CROSS] = { P_IDLE, P_CROSS, 2, 3, 8, 4, 25, 5, 0 },
    [A_UPPER] = { P_JAB, P_UPPER, 4, 4, 14, 4, 24, 9, 1 },
    [A_KNEE] = { P_GRAB, P_KNEE, 3, 3, 9, 0, 22, 6, 0 },
    [A_SPIN] = { P_IDLE, P_SPIN, 3, 18, 8, -28, 28, 8, 1 },
    [A_PUNCH] = { P_WINDUP, P_JAB, 16, 4, 18, 4, 24, 7, 0 },
    [A_PUNCH2] = { P_IDLE, P_CROSS, 6, 4, 22, 4, 24, 6, 0 },
    [A_KICK] = { P_WINDUP, P_KICK, 12, 5, 20, 4, 30, 6, 0 },
    [A_SLAM] = { P_WINDUP, P_UPPER, 22, 5, 26, 4, 27, 13, 1 },
    [A_CHARGE] = { P_WINDUP, P_CROSS, 18, 40, 20, -2, 18, 10, 1 },
    [A_SWING] = { P_WINDUP, P_JAB, 18, 5, 22, 4, 34, 14, 1 },
    [A_FLURRY] = { P_IDLE, P_JAB, 8, 3, 6, 4, 26, 5, 0 },
    [A_THROW] = { P_THROW, P_THROW, 0, 0, 16, 0, 0, 0, 0 },
};

/* --- Los tipos ------------------------------------------------------------------- */

#define SK          6                   /* la piel */

typedef struct {
    const char *es, *en;
    uint8_t body, head, scale, weapon;  /* scale en 1/16 */
    int16_t hp;
    uint8_t vx, vy;                     /* en 1/16 de píxel por frame */
    uint8_t attack, attack2;
    uint16_t points;
    uint8_t col[2][16];
} kind_t;

/* Colores: contorno, piel, pelo, ropa de arriba (delante, detrás), piernas
 * (delante, detrás), zapatos, -, antebrazos (delante, detrás), cinturón, -,
 * lo de la cabeza (dos) y blanco. */
static const kind_t kinds[KINDS] = {
    [K_HERO] = { "KAI", "KAI", BODY_NORMAL, HEAD_HERO, 16, 0, 100, 24, 16, A_JAB, A_JAB, 0,
                 { { 0, SK, 0, 15, 7, 9, 1, 10, 8, SK, SK, 0, 0, 10, 10, 15 },
                   { 0, SK, 0, 15, 7, 9, 1, 10, 8, SK, SK, 0, 0, 10, 10, 15 } } },
    [K_PUNK] = { "MACARRA", "PUNK", BODY_NORMAL, HEAD_PUNK, 16, 0, 30, 16, 11, A_PUNCH, A_PUNCH2, 500,
                 { { 0, SK, 12, 11, 3, 1, 0, 7, 8, SK, SK, 6, 0, 14, 14, 15 },
                   { 0, SK, 14, 12, 4, 1, 0, 7, 8, SK, SK, 6, 0, 13, 13, 15 } } },
    [K_RUNNER] = { "CORREDOR", "RUNNER", BODY_NORMAL, HEAD_CAP, 15, 0, 22, 30, 18, A_KICK, A_KICK, 600,
                   { { 0, SK, 0, 10, 2, 4, 0, 15, 8, 10, 2, 10, 0, 9, 9, 15 },
                     { 0, SK, 0, 13, 5, 1, 0, 15, 8, 13, 5, 13, 0, 10, 10, 15 } } },
    [K_BRUTE] = { "ARMARIO", "BRUISER", BODY_FAT, HEAD_BALD, 17, 0, 55, 12, 9, A_SLAM, A_CHARGE, 1000,
                  { { 0, SK, 2, 15, 7, 4, 0, 0, 8, SK, SK, 0, 0, 0, 0, 15 },
                    { 0, SK, 0, 10, 2, 1, 0, 0, 8, SK, SK, 0, 0, 0, 0, 15 } } },
    [K_BOSS1] = { "LLAVE", "WRENCH", BODY_FAT, HEAD_BANDANA, 20, 1, 100, 14, 10, A_SWING, A_CHARGE, 5000,
                  { { 0, SK, 0, 9, 1, 1, 0, 0, 8, SK, SK, 14, 0, 10, 10, 15 },
                    { 0, SK, 0, 9, 1, 1, 0, 0, 8, SK, SK, 14, 0, 10, 10, 15 } } },
    [K_BOSS2] = { "CADENAS", "CHAINS", BODY_NORMAL, HEAD_LONG, 20, 2, 120, 20, 13, A_SWING, A_FLURRY, 10000,
                  { { 0, SK, 14, 1, 0, 3, 3, 7, 8, 1, 0, 14, 0, 0, 0, 15 },
                    { 0, SK, 14, 1, 0, 3, 3, 7, 8, 1, 0, 14, 0, 0, 0, 15 } } },
    [K_BOSS3] = { "EL JEFE", "THE BOSS", BODY_NORMAL, HEAD_BOSS, 22, 3, 140, 20, 13, A_SWING, A_FLURRY, 15000,
                  { { 0, SK, 7, 15, 7, 15, 7, 0, 8, 15, 7, 10, 0, 15, 15, 15 },
                    { 0, SK, 7, 15, 7, 15, 7, 0, 8, 15, 7, 10, 0, 15, 15, 15 } } },
};

const char *kind_name(int k) { return T(kinds[k].es, kinds[k].en); }

static const uint8_t c_flash[16] = { 0, 15, 15, 15, 15, 15, 15, 15, 8, 15, 15, 15, 15, 15, 15, 15 };

static int is_boss(const fighter_t *f) { return f->kind >= K_BOSS1; }

void add_score(int v)
{
    if (demo)
        return;
    uint32_t before = score / 30000;
    score += (uint32_t)v;
    if (score / 30000 != before) {      /* una vida más cada 30.000 */
        lives++;
        sound_fx(FX_LIFE);
    }
}

/* --- Altas -------------------------------------------------------------------------- */

void fight_reset(void)
{
    for (int i = 0; i < MAX_F; i++)
        fighters[i].alive = 0;
    for (int i = 0; i < MAX_ITEMS; i++)
        things[i].alive = 0;
    last_foe_t = 0;
    prev_in = IN_A | IN_B;              /* lo que ya esté pulsado no cuenta */
}

static void set_state(fighter_t *f, int s)
{
    f->state = (uint8_t)s;
    f->t = 0;
}

void hero_spawn(int x, int y)
{
    fighter_t *f = &fighters[0];
    int hp = f->alive ? f->hp : kinds[K_HERO].hp;
    *f = (fighter_t){ 0 };
    f->alive = 1;
    f->kind = K_HERO;
    f->hp = (int16_t)(hp > 0 ? hp : kinds[K_HERO].hp);
    f->maxhp = kinds[K_HERO].hp;
    f->x = x * FX;
    f->y = y * FX;
    f->held = -1;
    f->invuln = 60;
}

fighter_t *foe_spawn(int kind, int x, int y)
{
    for (int i = 1; i < MAX_F; i++)
        if (!fighters[i].alive) {
            fighter_t *f = &fighters[i];
            *f = (fighter_t){ 0 };
            f->alive = 1;
            f->kind = (uint8_t)kind;
            f->variant = (uint8_t)(rnd() & 1);
            f->hp = f->maxhp = kinds[kind].hp;
            f->x = x * FX;
            f->y = y * FX;
            f->held = -1;
            f->face_left = x > fighters[0].x / FX;
            f->side = f->face_left ? 1 : -1;
            f->ai_t = (int16_t)(20 + rnd() % 30);
            f->cool = 30;
            return f;
        }
    return 0;
}

void item_spawn(int kind, int x, int y, int drop)
{
    for (int i = 0; i < MAX_ITEMS; i++)
        if (!things[i].alive) {
            things[i] = (item_t){ 1, (uint8_t)kind, (uint8_t)drop, 0, x * FX, y * FX, 0, 0 };
            if (kind >= FOOD_APPLE) {   /* la comida salta al salir */
                things[i].z = 8 * FX;
                things[i].vz = 24;
            }
            return;
        }
}

int foes_left(void)
{
    int n = 0;
    for (int i = 1; i < MAX_F; i++)
        n += fighters[i].alive;
    return n;
}

/* --- Golpes y caídas --------------------------------------------------------------- */

static void knock_down(fighter_t *f, int dir, int strong)
{
    if (f->held >= 0) {                 /* suelta o le sueltan */
        fighter_t *o = &fighters[f->held];
        if (o->held == f - fighters) {
            o->held = -1;
            if (o->state == ST_GRAB || o->state == ST_HELD)
                set_state(o, ST_IDLE);
        }
        f->held = -1;
    }
    set_state(f, ST_FALL);
    f->vx = dir * (strong ? 30 : 20);
    f->vz = strong ? 44 : 36;
    f->vy = 0;
    f->engaged = 0;
    f->hits = 0;
}

static void hurt(fighter_t *t, const fighter_t *a, int dmg, int knock)
{
    int dir = t->x >= a->x ? 1 : -1;
    t->hp = (int16_t)(t->hp - dmg);
    t->flash = 4;
    if (t == &fighters[0]) {
#ifdef STREET_BOT
        bot_hits++;
#endif
#ifndef STREET_BOT
        if (demo && t->hp < 10)         /* la demostración no pierde */
            t->hp = 10;
#endif
        sound_fx(FX_HURT);
    } else {
        add_score(dmg * 10);
        last_foe = (int)(t - fighters);
        last_foe_t = 150;
    }
    sound_fx(knock || t->hp <= 0 ? FX_HEAVY : FX_HIT);
    if (t->state == ST_HELD && !knock && t->hp > 0)
        return;                         /* el rodillazo: sigue agarrado */
    /* Cuatro golpes seguidos también tiran al suelo. */
    if (t->hits_t > 0)
        t->hits++;
    else
        t->hits = 1;
    t->hits_t = 50;
    if (t->hp <= 0 || knock || (t->hits >= 4 && t != &fighters[0]) || t->state == ST_JUMP) {
        knock_down(t, dir, knock || t->hp <= 0);
        return;
    }
    if (t->state == ST_GRAB && t->held >= 0) {    /* le pegan mientras agarra: suelta */
        fighter_t *o = &fighters[t->held];
        o->held = -1;
        set_state(o, ST_IDLE);
        t->held = -1;
    }
    /* Los grandes no se paran con cualquier cosa. */
    if (is_boss(t) && t->state == ST_ATTACK && t->t >= attacks[t->attack].pre - 6 && t->hits < 3)
        return;
    set_state(t, ST_HURT);
    t->vx = dir * 12;
    t->face_left = dir > 0;
}

static int hittable(const fighter_t *t)
{
    return t->alive && t->invuln == 0 && t->state != ST_FALL && t->state != ST_DOWN && t->state != ST_GETUP &&
           t->state != ST_DEAD && t->state != ST_THROWN && t->state != ST_HELD;
}

/* ¿Le da a (tx, ty, tz)? */
static int in_reach(const fighter_t *a, int rmin, int rmax, int tx, int ty, int tz, int scale)
{
    int dx = (tx - a->x) / FX * (a->face_left ? -1 : 1), dy = (ty - a->y) / FX;
    rmin = rmin * scale / 16;
    rmax = rmax * scale / 16;
    return dx >= rmin && dx <= rmax && dy >= -6 && dy <= 6 && (tz - a->z) / FX < 24 && (a->z - tz) / FX < 24;
}

static void break_prop(item_t *p)
{
    p->alive = 0;
    sound_fx(FX_BREAK);
    add_score(100);
    if (p->drop)
        item_spawn(p->drop, p->x / FX, p->y / FX, 0);
}

/* Lo que alcanza un golpe activo de a. */
static void strike(fighter_t *a, int rmin, int rmax, int dmg, int knock)
{
    int scale = kinds[a->kind].scale, ai = (int)(a - fighters);
    for (int i = 0; i < MAX_F; i++) {
        fighter_t *t = &fighters[i];
        if (i == ai || (a->hitmask & (1u << i)) || !hittable(t))
            continue;
        if ((ai == 0) == (i == 0) && a->state != ST_THROWN)
            continue;                   /* los enemigos no se pegan entre ellos */
        if (!in_reach(a, rmin, rmax, t->x, t->y, t->z, scale))
            continue;
        a->hitmask |= 1u << i;
        hurt(t, a, dmg, knock);
        if (ai == 0)
            a->combo++;
    }
    if (ai != 0)
        return;
    for (int i = 0; i < MAX_ITEMS; i++) {
        item_t *p = &things[i];
        if (p->alive && p->kind < FOOD_APPLE && !(a->hitmask & (256u << i)) &&
            in_reach(a, rmin, rmax, p->x, p->y, 0, 16)) {
            a->hitmask |= 256u << i;
            break_prop(p);
        }
    }
}

static void start_attack(fighter_t *f, int at)
{
    set_state(f, ST_ATTACK);
    f->attack = (uint8_t)at;
    f->hitmask = 0;
    f->vx = f->vy = 0;
}

/* --- El jugador ----------------------------------------------------------------- */


static void hero_grab_check(fighter_t *p, int in)
{
    int dir = in & IN_RIGHT ? 1 : in & IN_LEFT ? -1 : 0;
    if (!dir)
        return;
    for (int i = 1; i < MAX_F; i++) {
        fighter_t *e = &fighters[i];
        if (!e->alive || is_boss(e) || e->z || (e->state != ST_IDLE && e->state != ST_WALK && e->state != ST_HURT))
            continue;
        int dx = (e->x - p->x) / FX * dir, dy = (e->y - p->y) / FX;
        if (dx >= 8 && dx <= 15 && dy >= -3 && dy <= 3) {
            if (++p->grab_t >= 8) {
                set_state(p, ST_GRAB);
                set_state(e, ST_HELD);
                p->held = (int8_t)i;
                e->held = 0;
                e->face_left = dir > 0;
                p->knees = 0;
                p->grab_t = 0;
                e->engaged = 0;
            }
            return;
        }
    }
    p->grab_t = 0;
}

static void hero_frame(fighter_t *p, int in)
{
    int edge = in & ~prev_in;
    prev_in = in;
    int dx = (in & IN_RIGHT ? 1 : 0) - (in & IN_LEFT ? 1 : 0);
    int dy = (in & IN_DOWN ? 1 : 0) - (in & IN_UP ? 1 : 0);

    switch (p->state) {
    case ST_IDLE:
    case ST_WALK:
        if (p->cool && --p->cool == 0)
            p->combo = 0;               /* la serie se acaba si se espera */
        if (dx)
            p->face_left = dx < 0;      /* se vuelve antes de pegar */
        if ((edge & IN_B) && (in & IN_A) && p->hp > 10) {   /* los dos: la patada giratoria */
            start_attack(p, A_SPIN);
            p->hp = (int16_t)(p->hp - 6);
            sound_fx(FX_SWING);
            break;
        }
        if ((edge & IN_A) || p->buffered) {
            p->buffered = 0;
            int at = p->combo >= 3 ? A_UPPER : p->combo & 1 ? A_CROSS : A_JAB;
            start_attack(p, at);
            sound_fx(FX_SWING);
            break;
        }
        if (edge & IN_B) {
            set_state(p, ST_JUMP);
            p->vz = 60;
            p->vx = dx * 26;
            p->kick = 0;
            p->hitmask = 0;
            if (dx)
                p->face_left = dx < 0;
            sound_fx(FX_JUMP);
            break;
        }
        p->vx = dx * kinds[K_HERO].vx;
        p->vy = dy * kinds[K_HERO].vy;
        if (dx)
            p->face_left = dx < 0;
        if (dx || dy) {
            if (p->state != ST_WALK)
                set_state(p, ST_WALK);
            hero_grab_check(p, in);
        } else if (p->state != ST_IDLE) {
            set_state(p, ST_IDLE);
        }
        break;
    case ST_ATTACK: {
        const attack_t *a = &attacks[p->attack];
        if ((edge & IN_A) && p->t >= a->pre)
            p->buffered = 1;
        if (p->attack == A_KNEE && p->held >= 0) {
            fighter_t *e = &fighters[p->held];
            if (p->t == a->pre) {
                hurt(e, p, a->dmg, ++p->knees >= 3);
                if (e->state == ST_HELD)
                    e->flash = 4;
            }
            if (p->t >= a->pre + a->act + a->post) {
                set_state(p, p->held >= 0 ? ST_GRAB : ST_IDLE);
                p->grab_t = 0;
            }
            break;
        }
        if (p->t >= a->pre && p->t < a->pre + a->act)
            strike(p, a->rmin, a->rmax, a->dmg, a->knock);
        if (p->attack == A_SPIN && p->t >= a->pre && ((p->t - a->pre) & 3) == 0)
            p->face_left ^= 1;          /* gira */
        if (p->t >= a->pre + a->act + a->post) {
            if (!p->hitmask)
                p->combo = 0;           /* al aire: la serie vuelve a empezar */
            if (p->combo > 3 || p->attack == A_UPPER)
                p->combo = 0;
            p->cool = 24;
            set_state(p, ST_IDLE);
            if (p->buffered)
                hero_frame(p, in);
        }
        break;
    }
    case ST_JUMP:
        if ((edge & (IN_A | IN_B)) && !p->kick && p->t > 2) {
            p->kick = 1;
            sound_fx(FX_SWING);
        }
        if (p->kick && p->z > 2 * FX)
            strike(p, 4, 30, 10, 1);
        break;
    case ST_GRAB: {
        fighter_t *e = &fighters[p->held];
        if (p->held < 0 || !e->alive || e->state != ST_HELD) {
            p->held = -1;
            set_state(p, ST_IDLE);
            break;
        }
        if (dx)
            p->face_left = dx < 0;      /* de la vuelta: se lanza hacia el otro lado */
        e->x = p->x + (p->face_left ? -13 : 13) * FX;
        e->y = p->y;
        e->face_left = !p->face_left;
        if (edge & IN_A) {
            start_attack(p, A_KNEE);
            break;
        }
        if (edge & IN_B) {              /* el lanzamiento */
            start_attack(p, A_THROW);
            set_state(e, ST_THROWN);
            e->held = -1;
            p->held = -1;
            e->vx = p->face_left ? -44 : 44;
            e->vz = 40;
            e->z = 12 * FX;
            e->hitmask = 1;
            e->face_left = !p->face_left;
            sound_fx(FX_SWING);
            break;
        }
        if (++p->grab_t > 90) {         /* se suelta */
            p->held = -1;
            e->held = -1;
            set_state(e, ST_HURT);
            set_state(p, ST_IDLE);
        }
        break;
    }
    default:
        break;
    }
}

/* --- Los enemigos ----------------------------------------------------------------- */

/* Un permiso para atacar: los más cercanos, hasta max a la vez. */
static void give_tokens(void)
{
    const fighter_t *p = &fighters[0];
    int n = 0, max = stage == 0 ? 1 + (cam > 600) : 2;
    for (int i = 1; i < MAX_F; i++) {
        fighter_t *e = &fighters[i];
        if (e->alive && is_boss(e))
            e->engaged = 1;
        else if (e->alive && e->engaged)
            n++;
    }
    while (n < max) {
        int best = -1, bd = 1 << 30;
        for (int i = 1; i < MAX_F; i++) {
            const fighter_t *e = &fighters[i];
            if (!e->alive || e->engaged || e->state == ST_DOWN || e->state == ST_DEAD || e->state == ST_FALL ||
                e->retreat)
                continue;
            int d = (e->x > p->x ? e->x - p->x : p->x - e->x) + 2 * (e->y > p->y ? e->y - p->y : p->y - e->y);
            if (d < bd) {
                bd = d;
                best = i;
            }
        }
        if (best < 0)
            break;
        fighters[best].engaged = 1;
        n++;
    }
}

/* El lado del jugador en el que se pone: si ya hay otro, el contrario. */
static void choose_side(fighter_t *e)
{
    const fighter_t *p = &fighters[0];
    int mine = e->x >= p->x ? 1 : -1, same = 0;
    for (int i = 1; i < MAX_F; i++) {
        const fighter_t *o = &fighters[i];
        if (o != e && o->alive && o->engaged && o->side == mine)
            same++;
    }
    e->side = (int16_t)(same && (e->kind == K_RUNNER || rnd() % 2) ? -mine : mine);
}

static void foe_frame(fighter_t *e)
{
    const kind_t *k = &kinds[e->kind];
    fighter_t *p = &fighters[0];
    if (e->cool)
        e->cool--;
    if (e->retreat)
        e->retreat--;

    if (e->state == ST_ATTACK) {
        const attack_t *a = &attacks[e->attack];
        int pre = a->pre, end = a->pre + a->act + a->post;
        if (e->attack == A_CHARGE) {
            if (e->t >= pre && e->t < pre + a->act) {
                e->vx = e->face_left ? -44 : 44;
                strike(e, a->rmin, a->rmax, a->dmg, a->knock);
                int sx = e->x / FX - cam;
                if (e->hitmask || sx < 8 || sx > W - 8)
                    e->t = (int16_t)(pre + a->act);
            } else {
                e->vx = 0;
            }
        } else if (e->t >= pre && e->t < pre + a->act) {
            if (e->t == pre)
                sound_fx(FX_SWING);
            strike(e, a->rmin, a->rmax, a->dmg, a->knock);
        }
        if (e->t >= end) {
            set_state(e, ST_IDLE);
            /* El macarra encadena un segundo golpe si el primero dio. */
            if (e->attack == A_PUNCH && e->hitmask && rnd() % 2) {
                start_attack(e, A_PUNCH2);
                return;
            }
            if (e->kind == K_BOSS2 || e->kind == K_BOSS3) {
                if (e->attack == A_FLURRY && e->hits_t == 0 && rnd() % 3) {
                    start_attack(e, A_FLURRY);  /* ráfagas de golpes */
                    return;
                }
            }
            e->cool = (int16_t)(stage == 0 ? 50 + rnd() % 50 : 36 + rnd() % 40);
            if (e->kind == K_RUNNER) {  /* pega y se va */
                e->retreat = 50;
                e->engaged = 0;
            } else if (!is_boss(e) && rnd() % 3 == 0) {
                e->engaged = 0;         /* deja sitio a otro */
                e->retreat = 30;
            }
        }
        return;
    }
    if (e->state == ST_JUMP) {          /* el jefe salta y patea */
        if (e->z > 2 * FX)
            strike(e, 4, 28, 10, 1);
        return;
    }
    if (e->state != ST_IDLE && e->state != ST_WALK)
        return;

    /* Adónde va. */
    int px = p->x / FX, py = p->y / FX, ex = e->x / FX, ey = e->y / FX;
    if (e->engaged && (e->ai_t-- <= 0)) {
        choose_side(e);
        e->ai_t = (int16_t)(60 + rnd() % 60);
    }
    int reach = attacks[k->attack].rmax * k->scale / 16, tx, ty, dist = reach - 6;
    if (e->retreat) {
        tx = px + (ex >= px ? 70 : -70);
        ty = py + (e->retreat & 32 ? 14 : -14);
    } else if (e->engaged) {
        tx = px + e->side * dist;
        ty = py;
    } else {
        int slot = (int)(e - fighters);
        tx = px + (ex >= px ? 1 : -1) * (48 + slot * 6);
        ty = py + (slot & 1 ? 12 : -12);
    }
    /* No se meten donde no se ve demasiado. */
    if (tx < cam + 6)
        tx = cam + 6;
    if (tx > cam + W - 6)
        tx = cam + W - 6;
    if (ty < BAND_TOP)
        ty = BAND_TOP;
    if (ty > BAND_BOTTOM)
        ty = BAND_BOTTOM;

    int dx = tx - ex, dy = ty - ey;
    e->vx = dx > 1 ? k->vx : dx < -1 ? -k->vx : 0;
    /* Si ya lo tiene a tiro, no se aleja: espera ahí. */
    if (e->engaged && !e->retreat && (ex - px) * e->side > 8 && (ex - px) * e->side <= dist)
        e->vx = 0;
    e->vy = dy > 1 ? k->vy : dy < -1 ? -k->vy : 0;
    /* Al rodear al jugador para cambiar de lado, se aparta de su línea. */
    if (e->engaged && ((ex < px) != (tx < px)) && (ey - py) * (ey - py) < 100 && (ex - px) * (ex - px) < 900)
        e->vy = ey >= py ? k->vy : -k->vy;
    if (!e->retreat)
        e->face_left = px < ex;
    else
        e->face_left = e->vx < 0;
    set_state(e, e->vx || e->vy ? ST_WALK : ST_IDLE);
    if (e->state == ST_WALK)
        e->t = (int16_t)(frame & 255);  /* el paso sigue */

    /* Atacar. */
    int adx = px - ex, ady = py - ey;
    if (adx < 0)
        adx = -adx;
    if (ady < 0)
        ady = -ady;
    int ready = !e->cool && e->engaged && !e->retreat && hittable(p) && p->state != ST_GETUP;
    if (ready && ady <= 3 && adx >= 6 && adx <= reach - 2 && rnd() % 4 == 0) {
        e->face_left = px < ex;
        start_attack(e, k->attack);
        return;
    }
    /* La carga, desde lejos, en la misma línea. */
    if (ready && k->attack2 == A_CHARGE && ady <= 4 && adx > 50 && adx < 150 && rnd() % 40 == 0) {
        e->face_left = px < ex;
        start_attack(e, A_CHARGE);
        return;
    }
    if (ready && e->kind == K_BOSS3 && ady <= 6 && adx > 40 && adx < 90 && rnd() % 50 == 0) {
        set_state(e, ST_JUMP);          /* la patada voladora del jefe */
        e->face_left = px < ex;
        e->vz = 56;
        e->vx = e->face_left ? -30 : 30;
        e->hitmask = 0;
        e->kick = 1;
        e->cool = 80;
        return;
    }
    if (ready && k->attack2 == A_FLURRY && ady <= 3 && adx <= 26 * k->scale / 16 && rnd() % 6 == 0) {
        e->face_left = px < ex;
        start_attack(e, A_FLURRY);
    }
}

/* --- La física de todos ------------------------------------------------------------- */

static void body_frame(fighter_t *f, int i)
{
    f->t++;
    if (f->flash)
        f->flash--;
    if (f->hits_t)
        f->hits_t--;
    if (f->invuln && f->state != ST_DOWN && f->state != ST_GETUP)
        f->invuln--;

    switch (f->state) {
    case ST_HURT:
        if (f->t > 3)
            f->vx = 0;
        if (f->t >= (i == 0 ? 14 : is_boss(f) ? 10 : 18))
            set_state(f, ST_IDLE);
        break;
    case ST_JUMP:
    case ST_FALL:
    case ST_THROWN:
        f->vz -= 4;
        if (f->state == ST_THROWN)
            strike(f, -16, 16, 10, 1);
        if (f->z + f->vz <= 0) {
            f->z = 0;
            f->vz = 0;
            f->vx = 0;
            if (f->state == ST_JUMP) {
                set_state(f, ST_IDLE);
                f->kick = 0;
            } else {
                if (f->state == ST_THROWN) {
                    f->hp = (int16_t)(f->hp - 12);
                    sound_fx(FX_HEAVY);
                }
                set_state(f, ST_DOWN);
            }
        }
        break;
    case ST_DOWN:
        if (f->t >= (i == 0 ? 50 : 40)) {
            if (f->hp <= 0) {
                if (i == 0) {           /* el jugador pierde una vida */
#ifdef STREET_BOT
                    bot_deaths++;
                    bot_boss_deaths += fighters[1].alive && is_boss(&fighters[1]);
                    if (lives == 1)     /* sin quedarse sin vidas */
                        lives++;
#endif
                    if (demo && lives == 1)
                        lives++;
                    if (--lives <= 0) {
                        set_state(f, ST_DEAD);
                        break;
                    }
                    f->hp = f->maxhp;
                    /* Los de cerca se apartan. */
                    for (int k = 1; k < MAX_F; k++)
                        if (fighters[k].alive && fighters[k].state < ST_HURT) {
                            fighters[k].retreat = 60;
                            fighters[k].engaged = 0;
                        }
                } else {
                    set_state(f, ST_DEAD);
                    add_score(kinds[f->kind].points);
                    break;
                }
            }
            set_state(f, ST_GETUP);
        }
        break;
    case ST_GETUP:
        if (f->t >= 16) {
            set_state(f, ST_IDLE);
            f->invuln = (int16_t)(i == 0 ? 100 : 20);
        }
        break;
    case ST_DEAD:
        if (i && f->t >= 40)
            f->alive = 0;
        break;
    case ST_HELD:
        f->vx = f->vy = 0;
        break;
    default:
        break;
    }

    f->x += f->vx;
    f->y += f->vy;
    f->z += f->vz;
    if (f->z < 0)
        f->z = 0;
    if (f->y < BAND_TOP * FX)
        f->y = BAND_TOP * FX;
    if (f->y > BAND_BOTTOM * FX)
        f->y = BAND_BOTTOM * FX;
    int lo = cam + (i == 0 ? 10 : -40), hi = cam + W - (i == 0 ? 10 : -40);
    if (f->x < lo * FX)
        f->x = lo * FX;
    if (f->x > hi * FX)
        f->x = hi * FX;
}

void fight_update(int input)
{
    give_tokens();
    fighter_t *p = &fighters[0];
    if (p->state != ST_DEAD)
        hero_frame(p, input);
    for (int i = 1; i < MAX_F; i++)
        if (fighters[i].alive)
            foe_frame(&fighters[i]);
    for (int i = 0; i < MAX_F; i++)
        if (fighters[i].alive)
            body_frame(&fighters[i], i);
    if (last_foe_t)
        last_foe_t--;

    for (int i = 0; i < MAX_ITEMS; i++) {
        item_t *t = &things[i];
        if (!t->alive)
            continue;
        if (t->flash)
            t->flash--;
        if (t->z || t->vz) {
            t->vz -= 3;
            t->z += t->vz;
            if (t->z <= 0)
                t->z = t->vz = 0;
        }
        if (t->x / FX < cam - 40)
            t->alive = 0;
        /* La comida, al pasar por encima. */
        if (t->kind >= FOOD_APPLE && !t->z && p->state != ST_DEAD && p->z == 0) {
            int dx = (t->x - p->x) / FX, dy = (t->y - p->y) / FX;
            if (dx > -10 && dx < 10 && dy > -6 && dy < 6) {
                static const int8_t heal[3] = { 25, 45, 100 };
                p->hp = (int16_t)(p->hp + heal[t->kind - FOOD_APPLE]);
                if (p->hp > p->maxhp)
                    p->hp = p->maxhp;
                t->alive = 0;
                add_score(200);
                sound_fx(FX_EAT);
            }
        }
    }
}

/* --- El dibujo ------------------------------------------------------------------------ */

static int pose_of(const fighter_t *f)
{
    switch (f->state) {
    case ST_WALK: {
        static const uint8_t walk[4] = { P_WALK1, P_WALK2, P_WALK3, P_WALK4 };
        return walk[(f->t / (f->kind == K_RUNNER ? 4 : 6)) & 3];
    }
    case ST_ATTACK: {
        const attack_t *a = &attacks[f->attack];
        if (f->t < a->pre)
            return a->pose_pre;
        if (f->t < a->pre + a->act + a->post / 2)
            return a->pose_hit;
        return P_IDLE;
    }
    case ST_JUMP: return f->kick ? P_JKICK : P_JUMP;
    case ST_HURT: return P_HURT;
    case ST_FALL:
    case ST_THROWN: return P_FALL;
    case ST_DOWN:
    case ST_DEAD: return P_DOWN;
    case ST_GETUP: return f->t < 8 ? P_DOWN : P_GETUP;
    case ST_GRAB: return P_GRAB;
    case ST_HELD: return P_HELD;
    case ST_WIN: return P_WIN;
    default: return P_IDLE;
    }
}

static void draw_weapon(const fighter_t *f, const frame_t *fr, int sx, int sy, int sc)
{
    int w = kinds[f->kind].weapon, dir = f->face_left ? -1 : 1;
    int hx = sx + dir * fr->kx * sc / 16, hy = sy + fr->ky * sc / 16;
    int a = fr->ka, len = w == 2 ? 20 : 15;
    int ex = hx + dir * isin(a) * len / 64, ey = hy + icos(a) * len / 64;
    int bx = hx - dir * isin(a) * 3 / 64, by = hy - icos(a) * 3 / 64;
    switch (w) {
    case 1:                             /* la llave inglesa */
        stroke(bx, by, ex, ey, 5, 0);
        stroke(bx, by, ex, ey, 3, 7);
        zxv_gfx_circle(ex, ey, 3, 0, 1);
        zxv_gfx_circle(ex, ey, 2, 15, 1);
        break;
    case 2:                             /* la cadena */
        for (int k = 0; k <= 5; k++) {
            int cx = hx + (ex - hx) * k / 5, cy = hy + (ey - hy) * k / 5 + (k * (5 - k)) / 3;
            zxv_gfx_circle(cx, cy, 2, 0, 1);
            zxv_gfx_rect(cx - 1, cy - 1, 2, 2, k & 1 ? 15 : 7);
        }
        break;
    default:                            /* el bastón */
        stroke(bx, by, ex, ey, 4, 0);
        stroke(bx, by, ex, ey, 2, 14);
        zxv_gfx_circle(ex, ey, 2, 6, 1);
        break;
    }
}

static void draw_fighter(const fighter_t *f, int blink)
{
    const kind_t *k = &kinds[f->kind];
    int pose = pose_of(f);
    const frame_t *fr = &frames[k->body][pose];
    int sc = k->scale, flip = f->face_left ? ZXV_FLIP_H : 0;
    int sx = f->x / FX - cam, sy = f->y / FX - f->z / FX;
    shadow(sx, f->y / FX, 22 * sc / 16);
    if (blink)
        return;
    const uint8_t *col = f->flash ? c_flash : k->col[f->variant];
    int w = fr->s.w * sc / 16, h = fr->s.h * sc / 16;
    int ox = fr->ox * sc / 16, oy = fr->oy * sc / 16;
    int x = flip ? sx - ox - w + 1 : sx + ox, y = sy + oy;
    if (fr->s.spr) {
        if (sc == 16)
            zxv_spr_draw(fr->s.spr, x, y, flip, col);
        else
            zxv_spr_draw_scaled(fr->s.spr, x, y, w, h, flip, col);
    }
    const sprite_t *hd = &heads[k->head][fr->lying];
    int hs = 16 * sc / 16;
    int hx = sx + (flip ? -fr->hx : fr->hx) * sc / 16 - hs / 2, hy = sy + fr->hy * sc / 16 - hs / 2;
    if (hd->spr) {
        if (sc == 16)
            zxv_spr_draw(hd->spr, hx, hy, flip, col);
        else
            zxv_spr_draw_scaled(hd->spr, hx, hy, hs, hs, flip, col);
    }
    if (k->weapon && f->state != ST_DOWN && f->state != ST_DEAD && f->state != ST_FALL && f->state != ST_GETUP)
        draw_weapon(f, fr, sx, sy, sc);
}

void fight_draw(void)
{
    /* Por profundidad: lo del fondo, antes. */
    int order[MAX_F + MAX_ITEMS], key[MAX_F + MAX_ITEMS], n = 0;
    for (int i = 0; i < MAX_F; i++)
        if (fighters[i].alive) {
            order[n] = i;
            key[n++] = fighters[i].y + (fighters[i].state == ST_HELD ? 1 : 0);
        }
    for (int i = 0; i < MAX_ITEMS; i++)
        if (things[i].alive) {
            order[n] = MAX_F + i;
            key[n++] = things[i].y - 2;
        }
    for (int a = 1; a < n; a++)         /* por inserción: son pocos */
        for (int b = a; b > 0 && key[b - 1] > key[b]; b--) {
            int t = key[b];
            key[b] = key[b - 1];
            key[b - 1] = t;
            t = order[b];
            order[b] = order[b - 1];
            order[b - 1] = t;
        }
    for (int j = 0; j < n; j++) {
        int i = order[j];
        if (i < MAX_F) {
            const fighter_t *f = &fighters[i];
            int blink = (f->invuln && (frame & 4)) || (f->state == ST_DEAD && (f->t & 4));
            draw_fighter(f, blink);
        } else {
            const item_t *t = &things[i - MAX_F];
            const sprite_t *s = &items[t->kind];
            int sx = t->x / FX - cam, sy = t->y / FX;
            if (t->kind >= FOOD_APPLE)
                shadow(sx, sy, s->w);
            draw_sprite(s, sx - s->w / 2, sy - t->z / FX - s->h + 1, 0, 0);
        }
    }
}

/* --- El piloto de la demostración ---------------------------------------------------- */

int autopilot(int go)
{
    const fighter_t *p = &fighters[0];
    int px = p->x / FX, py = p->y / FX, in = 0;
    static int phase;
    phase++;
    if (p->state == ST_GRAB)            /* agarrado: rodillazos */
        return phase & 1 ? IN_A : 0;

    /* El enemigo más cercano y lo que amenaza. */
    int best = -1, bd = 1 << 30, threat = 0, left = 0, right = 0;
    for (int i = 1; i < MAX_F; i++) {
        const fighter_t *e = &fighters[i];
        if (!e->alive || e->state == ST_DEAD)
            continue;
        int ex = e->x / FX, ey = e->y / FX, dx = ex - px, dy = ey - py;
        int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
        if (ex < cam - 4 || ex > cam + W + 4)
            continue;
        if (adx < 44 && ady < 12)
            dx < 0 ? left++ : right++;
        if (e->state == ST_ATTACK && e->t < attacks[e->attack].pre && ady < 9 && adx < 46)
            threat = dy >= 0 ? 1 : -1;  /* va a pegar: hay que salir de su línea */
        if (e->state == ST_DOWN || e->state == ST_FALL || e->state == ST_GETUP)
            continue;
        int d = adx + 3 * ady;
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    int tx = px, ty = py;
    /* Con poca vida, a por la comida; sin enemigos, a romper cajas. */
    for (int i = 0; i < MAX_ITEMS; i++) {
        const item_t *t = &things[i];
        int food = t->kind >= FOOD_APPLE;
        if (t->alive && ((food && p->hp < p->maxhp * 2 / 3) || (!food && best < 0)) && t->x / FX > cam &&
            t->x / FX < cam + W) {
            tx = t->x / FX + (food ? 0 : -14);
            ty = t->y / FX;
            best = -2;
            if (!food && (tx - px) * (tx - px) <= 25 && (ty - py) * (ty - py) < 4)
                return p->face_left ? IN_RIGHT | IN_A : phase & 1 ? IN_A : 0;   /* se vuelve y pega */
            break;
        }
    }
    if (best == -1) {                   /* nadie: hacia la derecha */
        if (go)
            in |= IN_RIGHT;
        ty = (BAND_TOP + BAND_BOTTOM) / 2;
    } else if (best > 0) {
        const fighter_t *e = &fighters[best];
        int ex = e->x / FX, ey = e->y / FX, side = ex >= px ? 1 : -1;
        tx = ex - side * 18;
        ty = ey;
        int dx = ex - px, adx = dx < 0 ? -dx : dx, ady = ey > py ? ey - py : py - ey;
        if (left && right && p->hp > 30 && p->state <= ST_WALK && phase % 40 == 0)
            return IN_A | IN_B;         /* rodeado: la patada giratoria */
        if (ady <= 3 && adx >= 6 && adx <= 24 && hittable(e)) {
            in |= side > 0 ? (p->face_left ? IN_RIGHT : 0) : (p->face_left ? 0 : IN_LEFT);
            if (phase & 1)
                in |= IN_A;
            if (!threat)
                return in;
        }
        if (ady <= 3 && adx > 34 && adx < 60 && p->state <= ST_WALK && phase % 50 == 0 && hittable(e))
            return (side > 0 ? IN_RIGHT : IN_LEFT) | IN_B;      /* la patada voladora */
        if (p->state == ST_JUMP)
            return phase & 1 ? IN_A : 0;
    }
    if (threat && p->state <= ST_WALK) {
        ty = py + (threat > 0 ? -16 : 16);
        if (ty < BAND_TOP + 2)
            ty = py + 16;
        if (ty > BAND_BOTTOM - 2)
            ty = py - 16;
        tx = px;
    }
    if (tx > px + 1) in |= IN_RIGHT;
    if (tx < px - 1) in |= IN_LEFT;
    if (ty > py + 1) in |= IN_DOWN;
    if (ty < py - 1) in |= IN_UP;
    return in;
}
