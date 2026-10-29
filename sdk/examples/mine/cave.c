/*
 * cave.c - Las reglas de la mina y el piloto automático.
 *
 * El mundo avanza a pasos (cada cave_t.tick ms): se recorre la cueva de
 * arriba abajo y de izquierda a derecha, y cada cosa hace lo suyo una vez
 * (lo que se mueve lleva la marca DONE hasta el final del paso).
 *
 *   - Las piedras y los diamantes caen al hueco y ruedan desde lo redondo
 *     (piedras, diamantes y ladrillos) si al lado y debajo hay hueco. Lo
 *     que cae sobre el minero o un bicho, estalla.
 *   - Las ascuas siguen la pared por la izquierda y estallan en hueco; los
 *     murciélagos, por la derecha, y estallan en diamantes. Los dos estallan
 *     si tocan al minero o el musgo.
 *   - El musgo crece despacio y luego deprisa. Encerrado, se vuelve
 *     diamantes; si se hace demasiado grande, piedras.
 *   - El tamiz se pone en marcha con lo primero que le cae encima: durante un
 *     tiempo, lo que lo atraviesa sale por debajo cambiado (las piedras,
 *     diamantes; los diamantes, piedras). Después es una pared más.
 */
#include "cave.h"


move_t moves[MAX_MOVES];
int nmoves;

static const int16_t off[4] = { -CW, 1, CW, -1 };

/* Las cosas redondas: lo de encima rueda. */
static int round_(int t)
{
    return t == O_BOULDER || t == O_DIAMOND || t == O_WALL;
}

static uint32_t wrand(world_t *w)
{
    w->rng = w->rng * 1103515245u + 12345u;
    return (w->rng >> 16) & 0x7FFF;
}

static void moved(world_t *w, int at, int dir)
{
    if (w->rec && nmoves < MAX_MOVES) {
        moves[nmoves].at = (uint16_t)at;
        moves[nmoves].dir = (uint8_t)dir;
        nmoves++;
    }
}

/* --- La carga ------------------------------------------------------------------ */

void world_load(world_t *w, int n)
{
    const cave_t *c = &caves[n];
    __builtin_memset(w, 0, sizeof(*w));
    w->cave = (uint8_t)n;
    w->w = c->w;
    w->h = c->h;
    w->need = c->need;
    w->tick = c->tick;
    w->moss_slow = (int16_t)(c->moss_slow * 1000 / c->tick);  /* en pasos */
    w->rng = 0x51ED0000u + c->seed;
    uint32_t fill = 0x1234u * c->seed + 7;      /* el relleno, siempre igual */
    for (int i = 0; i < CW * CH; i++)
        w->g[i] = O_STEEL;
    for (int y = 1; y < c->h - 1; y++)
        for (int x = 1; x < c->w - 1; x++) {
            int i = y * CW + x, t = O_DIRT;
            switch (c->rows[y - 1][x - 1]) {
            case ' ': t = O_EMPTY; break;
            case ':': t = O_DIRT; break;
            case 'w': t = O_WALL; break;
            case 'W': t = O_STEEL; break;
            case 'o': t = O_BOULDER; break;
            case 'd': t = O_DIAMOND; break;
            case 'X': t = O_EXIT; break;
            case 'm': t = O_MOSS; break;
            case 's': t = O_SIEVE; break;
            case 'f': t = O_EMBER + D_LEFT; break;
            case 'b': t = O_BAT + D_DOWN; break;
            case 'P':
                t = O_ENTRY;
                w->mi = (int16_t)i;
                break;
            case '.': {
                fill = fill * 1103515245u + 12345u;
                unsigned r = (fill >> 16) & 255;
                if (r < c->p_boulder)
                    t = O_BOULDER;
                else if (r < (unsigned)c->p_boulder + c->p_diamond)
                    t = O_DIAMOND;
                else if (r < (unsigned)c->p_boulder + c->p_diamond + c->p_empty)
                    t = O_EMPTY;
                break;
            }
            default: break;
            }
            w->g[i] = (uint8_t)t;
        }
    w->state = ST_BORN;
    w->born = 8;
    w->facing = D_RIGHT;
}

int world_diamonds(const world_t *w)
{
    int n = 0;
    for (int i = 0; i < CW * CH; i++)
        n += (w->g[i] & 0x7F) == O_DIAMOND || (w->g[i] & 0x7F) == O_DIAMOND_F;
    return n;
}

/* --- Las explosiones --------------------------------------------------------- */

static void explode(world_t *w, int at, int kind)
{
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            int k = at + dy * CW + dx, t = w->g[k] & 0x7F;
            if (t == O_STEEL || t == O_EXIT || t == O_EXIT_OPEN || t == O_ENTRY)
                continue;
            if (t == O_MINER)
                w->state = ST_DEAD;
            w->g[k] = (uint8_t)(kind | DONE);
        }
    w->ev |= EV_BOOM;
}

void world_kill(world_t *w)
{
    if (w->state == ST_PLAY)
        explode(w, w->mi, O_BOOM);
}

/* --- Lo que cae ------------------------------------------------------------- */

/* Rodar a un lado si al lado y debajo hay hueco; sigue cayendo. */
static int roll(world_t *w, int i, int falling)
{
    uint8_t *g = w->g;
    if (g[i - 1] == O_EMPTY && g[i - 1 + CW] == O_EMPTY) {
        g[i] = O_EMPTY;
        g[i - 1] = (uint8_t)(falling | DONE);
        moved(w, i - 1, D_LEFT);
        return 1;
    }
    if (g[i + 1] == O_EMPTY && g[i + 1 + CW] == O_EMPTY) {
        g[i] = O_EMPTY;
        g[i + 1] = (uint8_t)(falling | DONE);
        moved(w, i + 1, D_RIGHT);
        return 1;
    }
    return 0;
}

static void fall(world_t *w, int i, int t)
{
    uint8_t *g = w->g;
    int j = i + CW, b = g[j] & 0x7F;
    int falling = t == O_BOULDER || t == O_BOULDER_F ? O_BOULDER_F : O_DIAMOND_F;
    if (b == O_EMPTY) {
        g[i] = O_EMPTY;
        g[j] = (uint8_t)(falling | DONE);
        moved(w, j, D_DOWN);
        return;
    }
    if (!is_falling(t)) {               /* quieto: solo rueda */
        if (round_(b))
            roll(w, i, falling);
        return;
    }
    if (b == O_MINER) {
        explode(w, j, O_BOOM);
        w->ev |= EV_CRUSH;
        return;
    }
    if (is_enemy(b)) {
        explode(w, j, b >= O_BAT ? O_DBOOM : O_BOOM);
        w->ev |= EV_CRUSH;
        return;
    }
    if (b == O_SIEVE && w->sieve != SV_DONE) {
        if (w->sieve == SV_IDLE) {
            w->sieve = SV_ACTIVE;
            w->sieve_t = (int16_t)(caves[w->cave].sieve_time * 1000 / w->tick);
        }
        g[i] = O_EMPTY;
        if (g[j + CW] == O_EMPTY) {     /* sale por debajo, cambiado */
            g[j + CW] = (uint8_t)((falling == O_BOULDER_F ? O_DIAMOND_F : O_BOULDER_F) | DONE);
            moved(w, j + CW, D_DOWN);
        }
        w->ev |= EV_SIEVE;
        return;
    }
    if (round_(b) && roll(w, i, falling))
        return;
    g[i] = (uint8_t)(falling - 1);      /* se para */
    w->ev |= falling == O_BOULDER_F ? EV_LAND : EV_TING;
}

/* --- Los bichos ------------------------------------------------------------- */

static void creature(world_t *w, int i, int t)
{
    uint8_t *g = w->g;
    int bat = t >= O_BAT, base = bat ? O_BAT : O_EMBER, d = t - base;
    for (int k = 0; k < 4; k++) {
        int n = g[i + off[k]] & 0x7F;
        if (n == O_MINER || n == O_MOSS) {
            explode(w, i, bat ? O_DBOOM : O_BOOM);
            return;
        }
    }
    int side = bat ? (d + 1) & 3 : (d + 3) & 3;     /* el lado de su pared */
    if (g[i + off[side]] == O_EMPTY) {
        g[i] = O_EMPTY;
        g[i + off[side]] = (uint8_t)((base + side) | DONE);
        moved(w, i + off[side], side);
    } else if (g[i + off[d]] == O_EMPTY) {
        g[i] = O_EMPTY;
        g[i + off[d]] = (uint8_t)(t | DONE);
        moved(w, i + off[d], d);
    } else {
        g[i] = (uint8_t)(base + (bat ? (d + 3) & 3 : (d + 1) & 3));    /* gira al otro lado */
    }
}

/* --- El minero ------------------------------------------------------------ */

static void step(world_t *w, int i, int j, int dir)
{
    w->g[i] = O_EMPTY;
    w->g[j] = O_MINER | DONE;
    w->mi = (int16_t)j;
    w->walked = 1;
    moved(w, j, dir);
}

static void collect(world_t *w)
{
    const cave_t *c = &caves[w->cave];
    w->got++;
    w->points += w->got > w->need ? c->extra : c->value;
    w->ev |= EV_COLLECT;
}

static void miner(world_t *w, int i, int dir, int grab)
{
    uint8_t *g = w->g;
    w->walked = 0;
    if (dir == D_NONE) {
        w->push = 0;
        return;
    }
    if (dir == D_LEFT || dir == D_RIGHT)
        w->facing = (uint8_t)dir;
    int j = i + off[dir], t = g[j] & 0x7F;
    switch (t) {
    case O_DIRT:
        w->ev |= EV_DIG;
        if (grab) {
            g[j] = O_EMPTY;
            break;
        }
        step(w, i, j, dir);
        break;
    case O_EMPTY:
        if (!grab)
            step(w, i, j, dir);
        break;
    case O_DIAMOND:
    case O_DIAMOND_F:
        collect(w);
        if (grab)
            g[j] = O_EMPTY;
        else
            step(w, i, j, dir);
        break;
    case O_EXIT_OPEN:
        if (!grab) {
            g[i] = O_EMPTY;
            w->mi = (int16_t)j;
            w->state = ST_OUT;
            w->ev |= EV_EXIT;
        }
        break;
    case O_BOULDER:                     /* empujar: hace falta insistir un paso */
        if ((dir == D_LEFT || dir == D_RIGHT) && g[j + off[dir]] == O_EMPTY) {
            if (++w->push < 2)
                return;
            g[j + off[dir]] = O_BOULDER | DONE;
            moved(w, j + off[dir], dir);
            w->ev |= EV_PUSH;
            if (grab)
                g[j] = O_EMPTY;
            else
                step(w, i, j, dir);
        }
        break;
    default:
        break;
    }
    w->push = 0;
}

/* --- El paso -------------------------------------------------------------- */

void world_tick(world_t *w, int dir, int grab)
{
    uint8_t *g = w->g;
    if (w->rec)
        nmoves = 0;
    w->ev = 0;
    w->ticks++;
    if (w->state == ST_BORN && --w->born <= 0) {
        g[w->mi] = O_MINER;
        w->state = ST_PLAY;
        w->ev |= EV_BORN;
    }
    if (w->sieve == SV_ACTIVE && --w->sieve_t <= 0)
        w->sieve = SV_DONE;
    if (w->moss_slow > 0)
        w->moss_slow--;

    int moss_n = 0, moss_room = 0;
    int rate = w->moss_slow > 0 ? 2 : 8;       /* en 1/256 por paso y celda */
    for (int i = CW + 1; i < (w->h - 1) * CW; i++) {
        int t = g[i];
        if (t & DONE)
            continue;
        switch (t) {
        case O_BOULDER:
        case O_BOULDER_F:
        case O_DIAMOND:
        case O_DIAMOND_F:
            fall(w, i, t);
            break;
        case O_MINER:
            if (w->state == ST_PLAY)
                miner(w, i, dir, grab);
            break;
        case O_MOSS:
            moss_n++;
            for (int k = 0; k < 4; k++)
                if (g[i + off[k]] == O_EMPTY || g[i + off[k]] == O_DIRT)
                    moss_room = 1;
            if ((int)(wrand(w) & 255) < rate) {
                int k = (int)(wrand(w) & 3), n = g[i + off[k]];
                if (n == O_EMPTY || n == O_DIRT) {
                    g[i + off[k]] = O_MOSS | DONE;
                    w->ev |= EV_MOSS;
                }
            }
            break;
        default:
            if (is_enemy(t))
                creature(w, i, t);
            else if (t >= O_BOOM && t < O_COUNT)
                g[i] = (uint8_t)(((t - O_BOOM) & 3) == 3 ? (t >= O_DBOOM ? O_DIAMOND : O_EMPTY) : t + 1);
            break;
        }
    }
    for (int i = 0; i < CW * CH; i++)
        g[i] &= (uint8_t)~DONE;

    /* El musgo: demasiado grande, piedras; encerrado, diamantes. */
    if (moss_n) {
        int to = moss_n >= 160 ? O_BOULDER : !moss_room ? O_DIAMOND : -1;
        if (to >= 0) {
            for (int i = 0; i < CW * CH; i++)
                if (g[i] == O_MOSS)
                    g[i] = (uint8_t)to;
            w->ev |= EV_CHANGE;
        }
    }
    w->moss_n = (int16_t)moss_n;

    if (!w->open && w->got >= w->need) {
        w->open = 1;
        for (int i = 0; i < CW * CH; i++)
            if (g[i] == O_EXIT)
                g[i] = O_EXIT_OPEN;
        w->ev |= EV_OPEN;
    }
}

/* --- El piloto automático --------------------------------------------------- */

/* Busca hacia atrás, desde los objetivos, lo que cuesta llegar desde cada
 * celda (cavar bajo una piedra cuesta más: al irse, cae y tapa el paso), y
 * da el paso que más acerca... si la simulación dice que después sigue vivo
 * (quieto, o apartándose en el paso siguiente). */

#define FAR     0x7FFF
#define POOL    (CW * CH * 4)

static int16_t dist[CW * CH], head[8], node[POOL], next_[POOL];
static uint8_t danger[CW * CH], pushdir[CW * CH];
static world_t sim, sim2;
static int pool_n, waited;
static int16_t recent[4];               /* las últimas celdas del minero */

static int walkable(int t)
{
    return t == O_EMPTY || t == O_DIRT || t == O_DIAMOND || t == O_EXIT_OPEN;
}

/* Algo va a caer ahí. */
static int threatened(const uint8_t *g, int x)
{
    int a = g[x - CW];
    if (is_falling(a))
        return 1;
    if (a == O_EMPTY && x >= 2 * CW && is_falling(g[x - 2 * CW]))
        return 1;
    return (a == O_BOULDER || a == O_DIAMOND) && g[x] == O_EMPTY;
}

static int ok_cell(const uint8_t *g, int x)
{
    return walkable(g[x]) && !danger[x] && !threatened(g, x);
}

/* Lo que cuesta entrar en x. */
static int cost(const uint8_t *g, int x)
{
    int a = g[x - CW];
    return a == O_BOULDER || a == O_DIAMOND ? 6 : 1;
}

static int idle_ok(world_t *s)
{
    for (int k = 0; k < 2 && s->state == ST_PLAY; k++)
        world_tick(s, D_NONE, 0);
    return s->state != ST_DEAD;
}

static int survives(const world_t *w, int dir, int grab)
{
    sim = *w;
    sim.rec = 0;
    world_tick(&sim, dir, grab);
    if (sim.state != ST_PLAY)
        return sim.state != ST_DEAD;
    for (int a = D_NONE; a >= 0; a--) {         /* quieto o apartándose */
        sim2 = sim;
        world_tick(&sim2, a, 0);
        if (sim2.state != ST_DEAD && idle_ok(&sim2))
            return 1;
    }
    return 0;
}

static void put(int v, int d)
{
    if (pool_n >= POOL)
        return;
    dist[v] = (int16_t)d;
    node[pool_n] = (int16_t)v;
    next_[pool_n] = head[d & 7];
    head[d & 7] = (int16_t)pool_n++;
}

/* Las distancias desde los objetivos que ya están puestos (cubos por
 * distancia: los costes son pequeños). */
static void spread(const uint8_t *g)
{
    for (int cur = 0, empty = 0; empty < 8; cur++) {
        int b = cur & 7;
        if (head[b] < 0) {
            empty++;
            continue;
        }
        empty = 0;
        while (head[b] >= 0) {
            int e = head[b], u = node[e];
            head[b] = next_[e];
            if (dist[u] != cur)
                continue;
            int nd = cur + cost(g, u);
            for (int d = 0; d < 4; d++) {
                int v = u - off[d];
                if (v < CW || v >= CW * (CH - 1) || nd >= dist[v] || !ok_cell(g, v))
                    continue;
                put(v, nd);
            }
        }
    }
}

int bot_input(const world_t *w, int *grab)
{
    const uint8_t *g = w->g;
    *grab = 0;
    if (w->state != ST_PLAY)
        return D_NONE;
    int m = w->mi;

    __builtin_memset(danger, 0, sizeof(danger));
    for (int i = CW; i < CW * (CH - 1); i++)
        if (is_enemy(g[i]) || (g[i] >= O_BOOM && g[i] < O_COUNT))
            for (int k = 0; k < 4; k++) {
                int n = i + off[k];
                danger[i] = danger[n] = 1;
                if (g[n] == O_EMPTY && is_enemy(g[i]))
                    for (int j = 0; j < 4; j++)
                        danger[n + off[j]] = 1;
            }

    /* Por orden: la salida abierta, los diamantes, empujar una piedra, cavar. */
    for (int stage = 0; stage < 4; stage++) {
        int n = 0;
        pool_n = 0;
        for (int b = 0; b < 8; b++)
            head[b] = -1;
        for (int i = 0; i < CW * CH; i++)
            dist[i] = FAR;
        for (int i = CW; i < CW * (CH - 1); i++) {
            int t = g[i], target = 0;
            pushdir[i] = D_NONE;
            if (stage == 0)
                target = t == O_EXIT_OPEN;
            else if (stage == 1)
                target = t == O_DIAMOND;
            else if (stage == 2) {
                for (int d = D_RIGHT; d <= D_LEFT; d += 2)
                    if (g[i + off[d]] == O_BOULDER && g[i + 2 * off[d]] == O_EMPTY && !danger[i + 2 * off[d]])
                        pushdir[i] = (uint8_t)d;
                target = pushdir[i] != D_NONE && (i == m || walkable(t));
            } else {
                target = t == O_DIRT;
            }
            if (target && (i == m || ok_cell(g, i))) {
                put(i, 0);
                n++;
            }
        }
        if (!n)
            continue;
        if (stage == 2 && dist[m] == 0) {   /* ya está al lado: empuja */
            if (survives(w, pushdir[m], 0))
                return pushdir[m];
            continue;
        }
        spread(g);
        /* Los pasos que acercan, del mejor al peor. */
        int best[4], score[4], nb = 0;
        for (int d = 0; d < 4; d++) {
            int v = m + off[d];
            if (dist[v] == FAR || !ok_cell(g, v))
                continue;
            int s = dist[v] + cost(g, v), k = nb++;
            for (int j = 0; j < 4; j++)
                if (recent[j] == v)
                    s += 3;             /* volver atrás, solo si compensa */
            while (k > 0 && score[k - 1] > s) {
                best[k] = best[k - 1];
                score[k] = score[k - 1];
                k--;
            }
            best[k] = d;
            score[k] = s;
        }
        for (int k = 0; k < nb; k++) {
            if (k > 0 && score[k] > score[0] + 2 && waited < 12)
                break;                  /* el buen camino, peligroso un momento: espera */
            if (survives(w, best[k], 0)) {
                waited = 0;
                recent[w->ticks & 3] = (int16_t)m;
                return best[k];
            }
        }
        if (nb && survives(w, D_NONE, 0)) {
            waited++;
            return D_NONE;
        }
    }
    /* Nada que hacer sin peligro: quieto, o lo que le salve. */
    if (survives(w, D_NONE, 0))
        return D_NONE;
    for (int d = 0; d < 4; d++)
        if (survives(w, d, 0))
            return d;
    return D_NONE;
}
