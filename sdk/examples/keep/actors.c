/*
 * actors.c - Lo que se mueve en The Keep: el héroe, los enemigos de la sala,
 * el hechicero, las flechas y bolas de fuego, y lo que dejan al morir.
 *
 * Solo viven los de la sala en la que se está: al entrar salen los de sus
 * marcas que siguen vivos (actors_room). Un enemigo vencido no vuelve
 * mientras se esté en la planta.
 *
 * Las cajas, en píxeles desde la esquina del sprite: los pies (con lo que se
 * choca con el mapa) y el cuerpo (con lo que se reciben los golpes).
 */
#include "keep.h"

actor_t actors[ACTORS];
hero_t hero;
int boss_dead;
int hero_god;
int hero_event;
int level_up_t;

static int abs_i(int v) { return v < 0 ? -v : v; }

static const uint8_t flash[16] = { 15, 15, 15, 15, 15, 15, 15, 15, 8, 15, 15, 15, 15, 15, 15, 15 };
static const uint8_t blade_big[16] = { 0, 1, 2, 3, 4, 5, 6, 13, 8, 9, 10, 11, 12, 13, 14, 15 };

/* --- Las clases de enemigo ---------------------------------------------------- */

static const struct {
    int8_t hp;
    uint8_t dmg;                        /* lo que quita al tocarlo, en medios corazones */
    uint8_t xp;
    uint8_t sprite;
    uint8_t speed;                      /* en FIX por frame */
} kinds[] = {
    [A_SLIME] = { 2, 1, 1, S_SLIME, 7 },
    [A_SLIMELET] = { 1, 1, 0, S_SLIME, 13 },
    [A_BAT] = { 1, 1, 1, S_BAT, 18 },
    [A_SKELETON] = { 2, 2, 3, S_SKELETON, 10 },
    [A_ARCHER] = { 2, 1, 2, S_ARCHER, 6 },
    [A_BOSS] = { 30, 2, 0, S_BOSS, 8 },
};

static int is_enemy(int t) { return t >= A_SLIME && t <= A_BOSS; }

/* El cuerpo: x, y, ancho y alto, desde la esquina del sprite. */
static void body(const actor_t *a, int *x, int *y, int *w, int *h)
{
    int px = a->x / FIX, py = a->y / FIX;
    switch (a->type) {
    case A_BOSS: *x = px + 4; *y = py + 6; *w = 16; *h = 24; break;
    case A_SLIMELET: *x = px + 4; *y = py + 8; *w = 8; *h = 7; break;
    case A_ARROW: *x = px; *y = py; *w = a->dir >= D_RIGHT ? 10 : 3; *h = a->dir >= D_RIGHT ? 3 : 10; break;
    case A_FIRE: *x = px + 1; *y = py + 1; *w = 6; *h = 6; break;
    case A_COIN: case A_SMALL_HEART: *x = px; *y = py; *w = 8; *h = 8; break;
    default: *x = px + 2; *y = py + 3; *w = 12; *h = 12; break;
    }
}

static int overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static int touches_hero(const actor_t *a)
{
    int x, y, w, h;
    body(a, &x, &y, &w, &h);
    return overlap(x, y, w, h, hero.x / FIX + 3, hero.y / FIX + 2, 10, 13);
}

/* --- El mapa ------------------------------------------------------------------- */

/* La caja (en píxeles del mundo) choca: con el mapa (volando, solo con las
 * paredes) y, si no es el héroe, con los bordes de la sala. */
static int blocked(int x, int y, int w, int h, int fly, int in_room)
{
    if (in_room) {
        int rx = room_x * ROOM_W * TILE, ry = room_y * ROOM_H * TILE;
        if (x < rx + TILE || y < ry + TILE || x + w > rx + (ROOM_W - 1) * TILE ||
            y + h > ry + (ROOM_H - 1) * TILE)
            return 1;
    }
    int xs[2] = { x, x + w - 1 }, ys[2] = { y, y + h - 1 };
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++) {
            int t = cell_at(xs[i], ys[j]);
            if (fly ? wall_tile(t) : solid_tile(t))
                return 1;
        }
    return 0;
}

/* Los pies de un enemigo de 16 x 16 (el hechicero, más grandes). */
static void feet(const actor_t *a, int32_t x, int32_t y, int *fx, int *fy, int *fw, int *fh)
{
    if (a->type == A_BOSS) {
        *fx = x / FIX + 4; *fy = y / FIX + 20; *fw = 16; *fh = 10;
    } else {
        *fx = x / FIX + 3; *fy = y / FIX + 8; *fw = 10; *fh = 7;
    }
}

/* Mueve el actor con su velocidad; da 1 si algo lo paró. */
static int move(actor_t *a, int vx, int vy, int fly)
{
    int hit = 0, fx, fy, fw, fh;
    feet(a, a->x + vx, a->y, &fx, &fy, &fw, &fh);
    if (vx && !blocked(fx, fy, fw, fh, fly, 1))
        a->x += vx;
    else if (vx)
        hit = 1;
    feet(a, a->x, a->y + vy, &fx, &fy, &fw, &fh);
    if (vy && !blocked(fx, fy, fw, fh, fly, 1))
        a->y += vy;
    else if (vy)
        hit = 1;
    return hit;
}

/* --- El héroe ----------------------------------------------------------------- */

void hero_reset(void)
{
    hero = (hero_t){ 0 };
    hero.hp = hero.hp_max = 6;
    hero.level = 1;
    hero.dir = D_UP;
}

void hero_place(int px, int py)
{
    hero.x = hero.safe_x = px * FIX;
    hero.y = hero.safe_y = py * FIX;
    hero.attack = 0;
    hero.kx = hero.ky = 0;
}

int hero_feet_x(void) { return hero.x / FIX + 8; }
int hero_feet_y(void) { return hero.y / FIX + 12; }

int hero_damage_points(void)
{
    return 1 + ((hero.items & ITEM_SWORD) ? 1 : 0) + (hero.level >= 3) + (hero.level >= 6);
}

static const uint8_t xp_next[] = { 5, 12, 22, 35, 52, 75 };

static void gain_xp(int n)
{
    hero.xp += n;
    while (hero.level <= (int)sizeof(xp_next) && hero.xp >= xp_next[hero.level - 1]) {
        hero.level++;
        if (!(hero.level & 1) && hero.hp_max < 20)
            hero.hp_max += 2;           /* un corazón más en los niveles pares */
        hero.hp = hero.hp_max;
        level_up_t = ZXV_FRAME_HZ * 2;
        sound_fx(FX_LEVEL);
    }
}

void hero_damage(int halves, int from_x, int from_y)
{
    if (hero.hurt || hero.dead)
        return;
    hero.hurt = ZXV_FRAME_HZ * 6 / 5;
    int dx = hero.x / FIX + 8 - from_x, dy = hero.y / FIX + 8 - from_y;
    if (abs_i(dx) > abs_i(dy))
        hero.kx = dx > 0 ? 40 : -40, hero.ky = 0;
    else
        hero.ky = dy > 0 ? 40 : -40, hero.kx = 0;
    sound_fx(FX_HURT);
    if (hero_god)
        return;
    hero.hp -= halves;
    if (hero.hp <= 0) {
        hero.hp = 0;
        hero.dead = 1;
        actor_spawn(A_PUFF, hero.x / FIX, hero.y / FIX);
    }
}

/* El sitio de la espada, en píxeles del mundo. */
static void sword_box(int *x, int *y, int *w, int *h)
{
    int px = hero.x / FIX, py = hero.y / FIX, e = (hero.items & ITEM_SWORD) ? 4 : 0;
    switch (hero.dir) {
    case D_DOWN: *x = px + 3; *y = py + 12; *w = 10; *h = 14 + e; break;
    case D_UP: *x = px + 3; *y = py - 10 - e; *w = 10; *h = 14 + e; break;
    case D_RIGHT: *x = px + 12; *y = py + 3; *w = 14 + e; *h = 11; break;
    default: *x = px - 10 - e; *y = py + 3; *w = 14 + e; *h = 11; break;
    }
}

static void kill(actor_t *a);

static void hero_strike(void)
{
    int x, y, w, h;
    sword_box(&x, &y, &w, &h);
    for (int i = 0; i < ACTORS; i++) {
        actor_t *a = &actors[i];
        if (!is_enemy(a->type) || a->swing == hero.swing)
            continue;
        if (a->type == A_BOSS && a->state >= 3 && a->state <= 4)
            continue;                   /* desvanecido: no se le toca */
        int bx, by, bw, bh;
        body(a, &bx, &by, &bw, &bh);
        if (!overlap(x, y, w, h, bx, by, bw, bh))
            continue;
        a->swing = hero.swing;
        a->hp -= (int8_t)hero_damage_points();
        a->hurt = 10;
        if (a->type != A_BOSS) {
            static const int8_t push[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };
            a->vx = (int16_t)(push[hero.dir][0] * 48);
            a->vy = (int16_t)(push[hero.dir][1] * 48);
        }
        sound_fx(a->type == A_BOSS ? FX_BOSS_HIT : FX_HIT);
        if (a->hp <= 0)
            kill(a);
    }
    /* Las zarzas: la espada grande las corta, de una en una. */
    for (int cy = y / TILE; cy <= (y + h - 1) / TILE; cy++)
        for (int cx = x / TILE; cx <= (x + w - 1) / TILE; cx++)
            if (cell_at(cx * TILE, cy * TILE) == T_BRAMBLE) {
                if (hero.items & ITEM_SWORD) {
                    cells[cy][cx] = (uint16_t)floor_base;
                    actor_spawn(A_PUFF, cx * TILE, cy * TILE);
                    sound_fx(FX_CUT);
                    bot_dirty();
                } else {
                    hint(HINT_BRAMBLE);
                }
            }
}

/* Lo que hay delante del héroe parado contra algo: una puerta se abre con
 * una llave; unas zarzas, sin la espada grande, lo dicen. */
static void hero_push(int dir)
{
    int fx = hero_feet_x(), fy = hero_feet_y();
    static const int8_t d[4][2] = { { 0, 7 }, { 0, -7 }, { 8, 0 }, { -8, 0 } };
    for (int k = -4; k <= 4; k += 8) {
        int px = fx + d[dir][0] + (dir >= D_RIGHT ? 0 : k), py = fy + d[dir][1] + (dir >= D_RIGHT ? k / 2 : 0);
        int t = cell_at(px, py);
        if (t == T_DOOR) {
            if (hero.keys > 0) {
                hero.keys--;
                open_gate(px / TILE, py / TILE);
                sound_fx(FX_DOOR);
                bot_dirty();
            } else {
                hint(HINT_DOOR);
            }
            return;
        }
        if (t == T_BOSS_DOOR) {
            if (hero.items & ITEM_BIG_KEY) {
                open_gate(px / TILE, py / TILE);
                sound_fx(FX_DOOR);
                bot_dirty();
            } else {
                hint(HINT_BOSS_DOOR);
            }
            return;
        }
        if (t == T_BRAMBLE && !(hero.items & ITEM_SWORD)) {
            hint(HINT_BRAMBLE);
            return;
        }
    }
}

/* Mueve al héroe; en una esquina, lo empuja hacia el hueco (para entrar
 * por las puertas sin ir al píxel). */
static int hero_move(int vx, int vy)
{
    int stopped = 0;
    if (vx) {
        int nx = (hero.x + vx) / FIX;
        if (!blocked(nx + 3, hero.y / FIX + 8, 10, 7, 0, 0)) {
            hero.x += vx;
        } else {
            stopped = 1;
            for (int k = 1; k <= 7 && !vy; k++) {
                int s = 0;
                if (!blocked(nx + 3, hero.y / FIX + 8 + k, 10, 7, 0, 0))
                    s = 1;
                else if (!blocked(nx + 3, hero.y / FIX + 8 - k, 10, 7, 0, 0))
                    s = -1;
                if (s && !blocked(hero.x / FIX + 3, hero.y / FIX + 8 + s, 10, 7, 0, 0)) {
                    hero.y += s * FIX;
                    break;
                }
            }
        }
    }
    if (vy) {
        int ny = (hero.y + vy) / FIX;
        if (!blocked(hero.x / FIX + 3, ny + 8, 10, 7, 0, 0)) {
            hero.y += vy;
        } else {
            stopped = 1;
            for (int k = 1; k <= 7 && !vx; k++) {
                int s = 0;
                if (!blocked(hero.x / FIX + 3 + k, ny + 8, 10, 7, 0, 0))
                    s = 1;
                else if (!blocked(hero.x / FIX + 3 - k, ny + 8, 10, 7, 0, 0))
                    s = -1;
                if (s && !blocked(hero.x / FIX + 3 + s, hero.y / FIX + 8, 10, 7, 0, 0)) {
                    hero.x += s * FIX;
                    break;
                }
            }
        }
    }
    return stopped;
}

void hero_update(int in)
{
    hero_event = EV_NONE;
    if (hero.dead)
        return;
    if (hero.hurt)
        hero.hurt--;
    if (hero.kx || hero.ky) {           /* el retroceso de un golpe */
        hero_move(hero.kx, hero.ky);
        hero.kx = hero.kx * 3 / 4;
        hero.ky = hero.ky * 3 / 4;
        if (abs_i(hero.kx) < 4 && abs_i(hero.ky) < 4)
            hero.kx = hero.ky = 0;
    }
    if (hero.attack) {
        hero.attack--;
        if (hero.attack >= 4 && hero.attack <= 13)
            hero_strike();
    } else if (in & IN_ATTACK) {
        hero.attack = 16;
        hero.swing++;
        sound_fx(FX_SWING);
    }
    hero.moving = 0;
    if (!hero.attack) {
        int vx = 0, vy = 0, sp = 21;
        if (in & IN_LEFT) vx = -sp, hero.dir = D_LEFT;
        else if (in & IN_RIGHT) vx = sp, hero.dir = D_RIGHT;
        if (in & IN_UP) vy = -sp, hero.dir = vx ? hero.dir : D_UP;
        else if (in & IN_DOWN) vy = sp, hero.dir = vx ? hero.dir : D_DOWN;
        if (vx && vy) {
            vx = vx * 3 / 4;
            vy = vy * 3 / 4;
        }
        if (vx || vy) {
            hero.moving = 1;
            hero.walk++;
            if (hero_move(vx, vy))
                hero_push(vx ? (vx > 0 ? D_RIGHT : D_LEFT) : (vy > 0 ? D_DOWN : D_UP));
        }
    }
    /* Los pinchos sin botas: daño y de vuelta a donde se estaba. */
    int t = cell_at(hero_feet_x(), hero_feet_y());
    if (t == T_SPIKES && !(hero.items & ITEM_BOOTS)) {
        hero.x = hero.safe_x;
        hero.y = hero.safe_y;
        hero.kx = hero.ky = 0;
        if (!hero.hurt) {
            hero_damage(1, hero_feet_x(), hero_feet_y());
            hero.kx = hero.ky = 0;
        }
        sound_fx(FX_SPIKES);
        hint(HINT_SPIKES);
    } else if (t != T_SPIKES) {
        hero.safe_x = hero.x;
        hero.safe_y = hero.y;
    }
    if (t == T_STAIRS)
        hero_event = EV_STAIRS;
}

void hero_draw(int cam_x, int cam_y)
{
    if (hero.dead)
        return;
    if (hero.hurt && (hero.hurt & 4))
        return;                         /* parpadea mientras no se le puede dar */
    int x = hero.x / FIX - cam_x, y = hero.y / FIX - cam_y + VIEW_TOP;
    int base = hero.dir == D_DOWN ? 0 : hero.dir == D_UP ? 3 : 6;
    int f = hero.attack ? 2 : hero.moving ? (hero.walk >> 3) & 1 : 0;
    int flip = hero.dir == D_LEFT ? ZXV_FLIP_H : 0;
    const uint8_t *blade = (hero.items & ITEM_SWORD) ? blade_big : 0;
    if (hero.attack && hero.dir == D_UP)        /* la espada, detrás */
        draw_sprite(&spr[S_SWORD_V], x + 5, y - 9, ZXV_FLIP_V, blade);
    draw_sprite(&spr[S_HERO + base + f], x, y, flip, 0);
    if (hero.attack) {
        switch (hero.dir) {
        case D_DOWN: draw_sprite(&spr[S_SWORD_V], x + 6, y + 13, 0, blade); break;
        case D_RIGHT: draw_sprite(&spr[S_SWORD_H], x + 12, y + 8, 0, blade); break;
        case D_LEFT: draw_sprite(&spr[S_SWORD_H], x - 8, y + 8, ZXV_FLIP_H, blade); break;
        default: break;
        }
    }
}

/* --- Los actores -------------------------------------------------------------- */

void actors_clear(void)
{
    for (int i = 0; i < ACTORS; i++)
        actors[i].type = A_NONE;
}

int actor_spawn(int type, int px, int py)
{
    for (int i = 0; i < ACTORS; i++) {
        actor_t *a = &actors[i];
        if (a->type != A_NONE)
            continue;
        *a = (actor_t){ 0 };
        a->type = (uint8_t)type;
        a->x = px * FIX;
        a->y = py * FIX;
        a->spawn = 255;
        a->hp = is_enemy(type) ? kinds[type].hp : 1;
        a->swing = (uint8_t)(hero.swing - 1);   /* el tajo de ahora no le ha dado */
        a->t = (uint16_t)(rnd() % 40);
        return i;
    }
    return -1;
}

void actors_room(int rx, int ry)
{
    actors_clear();
    static const uint8_t type_of[MARKS] = {
        [M_SAGE] = A_SAGE, [M_SLIME] = A_SLIME, [M_BAT] = A_BAT, [M_SKELETON] = A_SKELETON,
        [M_ARCHER] = A_ARCHER, [M_BOSS] = A_BOSS,
    };
    for (int i = 0; i < nspawns; i++) {
        spawn_t *s = &spawns[i];
        if (!s->alive || !type_of[s->mark] || s->cx / ROOM_W != rx || s->cy / ROOM_H != ry)
            continue;
        int px = s->cx * TILE, py = s->cy * TILE;
        if (s->mark == M_BOSS) {
            px -= 4;
            py -= 16;
        }
        int k = actor_spawn(type_of[s->mark], px, py);
        if (k >= 0)
            actors[k].spawn = (uint8_t)i;
    }
}

static void kill(actor_t *a)
{
    int px = a->x / FIX, py = a->y / FIX;
    if (a->spawn != 255)
        spawns[a->spawn].alive = 0;
    gain_xp(kinds[a->type].xp);
    sound_fx(FX_KILL);
    int type = a->type;
    a->type = A_NONE;
    if (type == A_BOSS) {
        boss_dead = 1;
        for (int i = 0; i < 5; i++)
            actor_spawn(A_PUFF, px + (int)(rnd() % 16), py + (int)(rnd() % 24));
        for (int i = 0; i < ACTORS; i++)            /* y se va todo lo suyo */
            if (actors[i].type == A_FIRE || actors[i].type == A_BAT)
                actors[i].type = A_NONE;
        return;
    }
    actor_spawn(A_PUFF, px, py);
    if (type == A_SLIME) {              /* el limo grande se parte en dos */
        actor_spawn(A_SLIMELET, px - 4, py);
        actor_spawn(A_SLIMELET, px + 4, py);
        return;
    }
    unsigned r = rnd() % 100;
    if (r < 35) {
        int k = actor_spawn(A_COIN, px + 4, py + 6);
        if (k >= 0)
            actors[k].value = (uint8_t)(1 + rnd() % 3);
    } else if (r < 52) {
        actor_spawn(A_SMALL_HEART, px + 4, py + 6);
    }
}

/* El ángulo (de 64 por vuelta) de (dx, dy). */
static int angle_to(int dx, int dy)
{
    int best = 0, best_dot = -0x7FFFFFFF;
    for (int a = 0; a < 64; a++) {
        int d = icos(a) * dx + isin(a) * dy;
        if (d > best_dot) {
            best_dot = d;
            best = a;
        }
    }
    return best;
}

static void shoot(int type, int px, int py, int a, int speed)
{
    int k = actor_spawn(type, px, py);
    if (k < 0)
        return;
    actors[k].vx = (int16_t)(icos(a) * speed / 64);
    actors[k].vy = (int16_t)(isin(a) * speed / 64);
}

static void wander(actor_t *a, int speed)
{
    if (a->t == 0) {
        int d = (int)(rnd() % 5);
        static const int8_t dv[5][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 }, { 0, 0 } };
        a->vx = (int16_t)(dv[d][0] * speed);
        a->vy = (int16_t)(dv[d][1] * speed);
        if (d < 4)
            a->dir = (uint8_t)d;
        a->t = (uint16_t)(40 + rnd() % 80);
    }
    a->t--;
    if (move(a, a->vx, a->vy, 0))
        a->t = 0;
}

static void boss_update(actor_t *a)
{
    int cx = a->x / FIX + 12, cy = a->y / FIX + 16;
    int hx = hero.x / FIX + 8, hy = hero.y / FIX + 8;
    int angry = a->hp <= kinds[A_BOSS].hp / 2;
    if (a->t)
        a->t--;
    switch (a->state) {
    case 0:                             /* flota por la sala */
        if ((a->t & 31) == 0) {
            int ang = angle_to(hx - cx, hy - cy) + (int)(rnd() % 32) - 16 + (rnd() & 1 ? 32 : 0);
            a->vx = (int16_t)(icos(ang) * kinds[A_BOSS].speed / 64);
            a->vy = (int16_t)(isin(ang) * kinds[A_BOSS].speed / 64);
        }
        move(a, a->vx, a->vy, 1);
        if (a->t == 0) {
            int bats = 0;
            for (int i = 0; i < ACTORS; i++)
                bats += actors[i].type == A_BAT;
            int r = (int)(rnd() % 4);
            if (angry && bats < 2 && r == 3)
                a->state = 5;
            else
                a->state = (uint8_t)(1 + r % 3);
            a->t = a->state >= 3 ? 30 : 36;
            if (a->state == 3)
                sound_fx(FX_CAST);
        }
        break;
    case 1:                             /* un anillo de fuego */
    case 2:                             /* tres bolas hacia el héroe */
    case 5:                             /* llama a los murciélagos */
        if (a->t == 0) {
            if (a->state == 1)
                for (int k = 0; k < 64; k += angry ? 6 : 8)
                    shoot(A_FIRE, cx - 4, cy - 4, k, 20);
            else if (a->state == 2)
                for (int k = -5; k <= 5; k += 5)
                    shoot(A_FIRE, cx - 4, cy - 4, angle_to(hx - cx, hy - cy) + k, angry ? 30 : 26);
            else
                for (int k = 0; k < 2; k++)
                    actor_spawn(A_BAT, cx - 24 + k * 32, cy);
            sound_fx(FX_CAST);
            a->state = 0;
            a->t = (uint16_t)(angry ? 60 : 90);
        }
        break;
    case 3:                             /* se desvanece... */
        if (a->t == 0) {
            int rx = room_x * ROOM_W * TILE, ry = room_y * ROOM_H * TILE;
            for (int k = 0; k < 20; k++) {
                int nx = rx + 24 + (int)(rnd() % 184), ny = ry + 32 + (int)(rnd() % 96);
                if (abs_i(nx + 12 - hx) + abs_i(ny + 16 - hy) < 90)
                    continue;
                if (!blocked(nx + 4, ny + 20, 16, 10, 1, 1)) {
                    a->x = nx * FIX;
                    a->y = ny * FIX;
                    break;
                }
            }
            a->state = 4;
            a->t = 30;
        }
        break;
    case 4:                             /* ...y aparece en otro sitio */
        if (a->t == 0) {
            a->state = 0;
            a->t = 40;
        }
        break;
    }
}

void actors_update(void)
{
    int hx = hero.x / FIX + 8, hy = hero.y / FIX + 8;
    for (int i = 0; i < ACTORS; i++) {
        actor_t *a = &actors[i];
        if (a->type == A_NONE)
            continue;
        int px = a->x / FIX, py = a->y / FIX;
        if (a->hurt) {                  /* el retroceso del golpe */
            a->hurt--;
            if (a->type != A_BOSS) {
                move(a, a->vx, a->vy, a->type == A_BAT);
                a->vx = (int16_t)(a->vx * 3 / 4);
                a->vy = (int16_t)(a->vy * 3 / 4);
                if (!a->hurt)
                    a->vx = a->vy = 0, a->t = 0;
            }
        }
        switch (a->type) {
        case A_SLIME:
        case A_SLIMELET:
            if (!a->hurt)
                wander(a, kinds[a->type].speed);
            break;
        case A_BAT:
            if (a->hurt)
                break;
            if (a->t == 0) {
                int dx = hx - (px + 8), dy = hy - (py + 8);
                int ang = abs_i(dx) + abs_i(dy) < 120 ? angle_to(dx, dy) + (int)(rnd() % 24) - 12
                                                      : (int)(rnd() % 64);
                a->vx = (int16_t)(icos(ang) * kinds[A_BAT].speed / 64);
                a->vy = (int16_t)(isin(ang) * kinds[A_BAT].speed / 64);
                a->t = (uint16_t)(20 + rnd() % 20);
            }
            a->t--;
            if (move(a, a->vx, 0, 1))
                a->vx = (int16_t)-a->vx;
            if (move(a, 0, a->vy, 1))
                a->vy = (int16_t)-a->vy;
            break;
        case A_SKELETON:
            if (a->hurt)
                break;
            if ((a->t++ & 7) == 0) {    /* hacia el héroe, por el eje más largo */
                int dx = hx - (px + 8), dy = hy - (py + 8), sp = kinds[A_SKELETON].speed;
                int horiz = abs_i(dx) > abs_i(dy);
                if (a->state)
                    horiz = !horiz;     /* atascado: por el otro */
                a->vx = (int16_t)(horiz ? (dx > 0 ? sp : -sp) : 0);
                a->vy = (int16_t)(horiz ? 0 : (dy > 0 ? sp : -sp));
                a->dir = (uint8_t)(a->vx > 0 ? D_RIGHT : a->vx < 0 ? D_LEFT : a->vy > 0 ? D_DOWN : D_UP);
                if (a->state)
                    a->state--;
            }
            if (move(a, a->vx, a->vy, 0))
                a->state = 3;
            break;
        case A_ARCHER:
            if (a->hurt)
                break;
            if (a->value)
                a->value--;             /* lo que falta para volver a disparar */
            if (a->state) {             /* apuntando */
                if (--a->state == 0) {
                    static const uint8_t ang[4] = { 16, 48, 0, 32 };
                    int ox = a->dir >= D_RIGHT ? (a->dir == D_RIGHT ? 12 : -6) : 6;
                    int oy = a->dir >= D_RIGHT ? 7 : (a->dir == D_DOWN ? 12 : -6);
                    int k = actor_spawn(A_ARROW, px + ox, py + oy);
                    if (k >= 0) {
                        actors[k].dir = a->dir;
                        actors[k].vx = (int16_t)(icos(ang[a->dir]) * 40 / 64);
                        actors[k].vy = (int16_t)(isin(ang[a->dir]) * 40 / 64);
                    }
                    a->value = 100;
                    sound_fx(FX_ARROW);
                }
                break;
            }
            {
                int dx = hx - (px + 8), dy = hy - (py + 8);
                if (!a->value && abs_i(dx) + abs_i(dy) < 180 && (abs_i(dx) < 7 || abs_i(dy) < 7)) {
                    a->dir = (uint8_t)(abs_i(dx) < 7 ? (dy > 0 ? D_DOWN : D_UP) : (dx > 0 ? D_RIGHT : D_LEFT));
                    a->state = 24;
                    break;
                }
            }
            wander(a, kinds[A_ARCHER].speed);
            break;
        case A_BOSS:
            boss_update(a);
            break;
        case A_ARROW:
        case A_FIRE: {
            a->x += a->vx;
            a->y += a->vy;
            int bx, by, bw, bh;
            body(a, &bx, &by, &bw, &bh);
            int rx = room_x * ROOM_W * TILE, ry = room_y * ROOM_H * TILE;
            if (wall_tile(cell_at(bx + bw / 2, by + bh / 2)) || bx < rx || by < ry ||
                bx > rx + ROOM_W * TILE || by > ry + ROOM_H * TILE) {
                a->type = A_NONE;
                continue;
            }
            if (touches_hero(a) && !hero.hurt) {
                hero_damage(a->type == A_FIRE ? 2 : 1, bx + bw / 2, by + bh / 2);
                a->type = A_NONE;
                continue;
            }
            a->t++;
            break;
        }
        case A_PUFF:
            if (++a->t >= 16)
                a->type = A_NONE;
            continue;
        case A_COIN:
        case A_SMALL_HEART:
            if (++a->t >= 8 * ZXV_FRAME_HZ) {
                a->type = A_NONE;
                continue;
            }
            if (touches_hero(a)) {
                if (a->type == A_COIN)
                    hero.gold += a->value;
                else
                    hero.hp = hero.hp + 2 > hero.hp_max ? hero.hp_max : hero.hp + 2;
                sound_fx(FX_COIN);
                a->type = A_NONE;
            }
            continue;
        case A_SAGE:
            a->t++;
            if (touches_hero(a)) {
                if (!a->state) {
                    a->state = 1;
                    message(T("BIENVENIDO A THE KEEP. EL", "WELCOME TO THE KEEP. THE"),
                            T("HECHICERO ESPERA ARRIBA. CADA", "SORCERER WAITS AT THE TOP. EACH"),
                            T("PLANTA GUARDA UN TESORO: ÚSALO.", "FLOOR HIDES A TREASURE: USE IT."));
                }
            } else {
                a->state = 0;
            }
            continue;
        }
        /* Tocar a un enemigo hace daño. */
        if (is_enemy(a->type) && a->type != A_NONE && !hero.hurt && touches_hero(a) &&
            !(a->type == A_BOSS && (a->state == 3 || a->state == 4)))
            hero_damage(kinds[a->type].dmg, px + 8, py + 8);
    }
}

int boss_alive(void)
{
    for (int i = 0; i < ACTORS; i++)
        if (actors[i].type == A_BOSS)
            return 1;
    return 0;
}

int boss_hp(void)
{
    for (int i = 0; i < ACTORS; i++)
        if (actors[i].type == A_BOSS)
            return actors[i].hp;
    return 0;
}

/* Lo que va de pie se dibuja de arriba abajo: lo de más abajo, delante. */
void actors_draw(int cam_x, int cam_y)
{
    int order[ACTORS + 1], n = 0;
    for (int i = 0; i < ACTORS; i++) {
        int t = actors[i].type;
        if (t != A_NONE && t != A_ARROW && t != A_FIRE && t != A_PUFF)
            order[n++] = i;
    }
    order[n++] = -1;                    /* el héroe */
    for (int i = 1; i < n; i++)         /* por la línea de los pies */
        for (int j = i; j > 0; j--) {
            int a = order[j - 1], b = order[j];
            int ya = a < 0 ? hero.y : actors[a].y + (actors[a].type == A_BOSS ? 16 * FIX : 0);
            int yb = b < 0 ? hero.y : actors[b].y + (actors[b].type == A_BOSS ? 16 * FIX : 0);
            if (ya <= yb)
                break;
            order[j - 1] = b;
            order[j] = a;
        }
    for (int k = 0; k < n; k++) {
        if (order[k] < 0) {
            hero_draw(cam_x, cam_y);
            continue;
        }
        actor_t *a = &actors[order[k]];
        int x = a->x / FIX - cam_x, y = a->y / FIX - cam_y + VIEW_TOP;
        const uint8_t *col = a->hurt && (a->hurt & 2) ? flash : 0;
        int anim = (int)((frame + order[k] * 7) >> 4) & 1;
        switch (a->type) {
        case A_SLIME: draw_sprite(&spr[S_SLIME + anim], x, y, 0, col); break;
        case A_SLIMELET: draw_scaled(&spr[S_SLIME + anim], x + 3, y + 6, 10, 10, 0, col); break;
        case A_BAT: draw_sprite(&spr[S_BAT + (int)((frame >> 3) & 1)], x, y, 0, col); break;
        case A_SKELETON:
            draw_sprite(&spr[S_SKELETON + anim], x, y, a->dir == D_LEFT ? ZXV_FLIP_H : 0, col);
            break;
        case A_ARCHER:
            draw_sprite(&spr[S_ARCHER + (a->state ? 0 : anim)], x, y, a->dir == D_LEFT ? ZXV_FLIP_H : 0, col);
            break;
        case A_SAGE: draw_sprite(&spr[S_SAGE + ((a->t % 180) < 8)], x, y, 0, 0); break;
        case A_BOSS:
            if ((a->state == 3 || a->state == 4) && (frame & 2))
                break;                  /* desvaneciéndose */
            draw_sprite(&spr[S_BOSS + (a->state == 1 || a->state == 2 || a->state == 5 ? 2 : anim)], x, y,
                        hero.x / FIX < a->x / FIX + 4 ? ZXV_FLIP_H : 0, col);
            break;
        case A_COIN:
        case A_SMALL_HEART:
            if (a->t < 6 * ZXV_FRAME_HZ || (frame & 4))
                draw_sprite(&spr[a->type == A_COIN ? S_COIN : S_SMALL_HEART], x, y, 0, 0);
            break;
        }
    }
    for (int i = 0; i < ACTORS; i++) {                 /* encima, lo que vuela */
        actor_t *a = &actors[i];
        int x = a->x / FIX - cam_x, y = a->y / FIX - cam_y + VIEW_TOP;
        switch (a->type) {
        case A_ARROW:
            if (a->dir >= D_RIGHT)
                draw_sprite(&spr[S_ARROW_H], x, y, a->dir == D_LEFT ? ZXV_FLIP_H : 0, 0);
            else
                draw_sprite(&spr[S_ARROW_V], x, y, a->dir == D_UP ? ZXV_FLIP_V : 0, 0);
            break;
        case A_FIRE: draw_sprite(&spr[S_FIRE + (int)((frame >> 2) & 1)], x, y, 0, 0); break;
        case A_PUFF: draw_sprite(&spr[S_PUFF + (a->t >= 8)], x, y, 0, 0); break;
        }
    }
}
