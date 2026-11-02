/*
 * chess.c - Reglas del ajedrez y un jugador artificial (ver chess.h).
 *
 * Sin dependencias: ni la API de ZX-V ni la biblioteca de C. Se prueba en el
 * PC con test/test_chess.c (perft contra los números conocidos).
 */
#include "chess.h"

/* --- Direcciones en el tablero 0x88 ------------------------------------------ */

static const int8_t KN_OFF[8] = { 33, 31, 18, 14, -14, -18, -31, -33 };
/* Las 8 del rey y la dama: [0..3] las de la torre, [4..7] las del alfil. */
static const int8_t KG_OFF[8] = { 16, -16, 1, -1, 17, 15, -15, -17 };

#define ABS(x)      ((x) < 0 ? -(x) : (x))
#define IDX(side)   ((side) > 0 ? 0 : 1)

/* --- La posición ---------------------------------------------------------------- */

static void pos_clear(Pos *p)
{
    for (int i = 0; i < 128; i++)
        p->sq[i] = 0;
    p->side = WHITE;
    p->castle = 0;
    p->ep = NO_SQ;
    p->half = 0;
    p->ksq[0] = p->ksq[1] = NO_SQ;
    p->ply = 0;
    p->last_from = p->last_to = NO_SQ;
}

void pos_init(Pos *p)
{
    static const int8_t back[8] = { ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK };

    pos_clear(p);
    for (int f = 0; f < 8; f++) {
        p->sq[SQ(f, 0)] = back[f];
        p->sq[SQ(f, 1)] = PAWN;
        p->sq[SQ(f, 6)] = -PAWN;
        p->sq[SQ(f, 7)] = -back[f];
    }
    p->castle = C_WK | C_WQ | C_BK | C_BQ;
    p->ksq[0] = SQ(4, 0);
    p->ksq[1] = SQ(4, 7);
}

int pos_from_fen(Pos *p, const char *s)
{
    int r = 7, f = 0;

    pos_clear(p);
    for (; *s && *s != ' '; s++) {
        char c = *s;
        if (c == '/') {
            r--;
            f = 0;
        } else if (c >= '1' && c <= '8') {
            f += c - '0';
        } else {
            int side = (c >= 'a') ? BLACK : WHITE;
            int t;
            switch (c | 0x20) {
            case 'p': t = PAWN; break;
            case 'n': t = KNIGHT; break;
            case 'b': t = BISHOP; break;
            case 'r': t = ROOK; break;
            case 'q': t = QUEEN; break;
            case 'k': t = KING; break;
            default: return -1;
            }
            if (f > 7 || r < 0)
                return -1;
            p->sq[SQ(f, r)] = (int8_t)(side * t);
            if (t == KING)
                p->ksq[IDX(side)] = (uint8_t)SQ(f, r);
            f++;
        }
    }
    if (p->ksq[0] == NO_SQ || p->ksq[1] == NO_SQ)
        return -1;
    while (*s == ' ')
        s++;
    p->side = (*s == 'b') ? BLACK : WHITE;
    while (*s && *s != ' ')
        s++;
    while (*s == ' ')
        s++;
    for (; *s && *s != ' '; s++) {
        switch (*s) {
        case 'K': p->castle |= C_WK; break;
        case 'Q': p->castle |= C_WQ; break;
        case 'k': p->castle |= C_BK; break;
        case 'q': p->castle |= C_BQ; break;
        default: break;
        }
    }
    while (*s == ' ')
        s++;
    if (*s >= 'a' && *s <= 'h' && s[1] >= '1' && s[1] <= '8')
        p->ep = (uint8_t)SQ(*s - 'a', s[1] - '1');
    while (*s && *s != ' ')
        s++;
    while (*s == ' ')
        s++;
    int h = 0;
    while (*s >= '0' && *s <= '9')
        h = h * 10 + (*s++ - '0');
    p->half = (uint8_t)h;
    return 0;
}

/* --- Ataques y jaques --------------------------------------------------------- */

int attacked(const Pos *p, int sq, int by)
{
    const int8_t *b = p->sq;
    int s;

    if (by == WHITE) {                          /* peones: atacan hacia arriba */
        s = sq - 15;
        if (SQ_VALID(s) && b[s] == PAWN)
            return 1;
        s = sq - 17;
        if (SQ_VALID(s) && b[s] == PAWN)
            return 1;
    } else {
        s = sq + 15;
        if (SQ_VALID(s) && b[s] == -PAWN)
            return 1;
        s = sq + 17;
        if (SQ_VALID(s) && b[s] == -PAWN)
            return 1;
    }
    for (int i = 0; i < 8; i++) {
        s = sq + KN_OFF[i];
        if (SQ_VALID(s) && b[s] == by * KNIGHT)
            return 1;
        s = sq + KG_OFF[i];
        if (SQ_VALID(s) && b[s] == by * KING)
            return 1;
    }
    for (int i = 0; i < 8; i++) {               /* alfiles, torres y damas */
        int off = KG_OFF[i];
        int minor = (i < 4) ? ROOK : BISHOP;
        for (s = sq + off; SQ_VALID(s); s += off) {
            int pc = b[s];
            if (pc) {
                if (pc == by * minor || pc == by * QUEEN)
                    return 1;
                break;
            }
        }
    }
    return 0;
}

int in_check(const Pos *p, int side)
{
    return attacked(p, p->ksq[IDX(side)], -side);
}

/* --- Generación de jugadas ------------------------------------------------------ */

static void add(Move *out, int *n, int from, int to, int promo, int flags, int caps_only)
{
    if (caps_only && !(flags & FL_CAP) && !promo)
        return;
    out[*n].from = (uint8_t)from;
    out[*n].to = (uint8_t)to;
    out[*n].promo = (uint8_t)promo;
    out[*n].flags = (uint8_t)flags;
    (*n)++;
}

static void add_pawn(Move *out, int *n, int from, int to, int last, int flags, int caps_only)
{
    if (SQ_RANK(to) == last) {
        add(out, n, from, to, QUEEN, flags, caps_only);
        add(out, n, from, to, ROOK, flags, caps_only);
        add(out, n, from, to, BISHOP, flags, caps_only);
        add(out, n, from, to, KNIGHT, flags, caps_only);
    } else {
        add(out, n, from, to, 0, flags, caps_only);
    }
}

int gen_moves(const Pos *p, Move *out, int caps_only)
{
    const int8_t *b = p->sq;
    int side = p->side;
    int n = 0;

    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            int from = SQ(f, r);
            int pc = b[from];
            if (!pc || (pc > 0) != (side > 0))
                continue;
            int t = ABS(pc);

            if (t == PAWN) {
                int dir = 16 * side;
                int start = (side > 0) ? 1 : 6;
                int last = (side > 0) ? 7 : 0;
                int to = from + dir;
                if (!b[to]) {
                    add_pawn(out, &n, from, to, last, 0, caps_only);
                    if (r == start && !b[to + dir])
                        add(out, &n, from, to + dir, 0, FL_DOUBLE, caps_only);
                }
                for (int d = -1; d <= 1; d += 2) {
                    to = from + dir + d;
                    if (!SQ_VALID(to))
                        continue;
                    int tp = b[to];
                    if (tp && (tp > 0) != (side > 0))
                        add_pawn(out, &n, from, to, last, FL_CAP, caps_only);
                    else if (!tp && to == p->ep)
                        add(out, &n, from, to, 0, FL_CAP | FL_EP, caps_only);
                }
            } else if (t == KNIGHT || t == KING) {
                const int8_t *offs = (t == KNIGHT) ? KN_OFF : KG_OFF;
                for (int i = 0; i < 8; i++) {
                    int to = from + offs[i];
                    if (!SQ_VALID(to))
                        continue;
                    int tp = b[to];
                    if (!tp)
                        add(out, &n, from, to, 0, 0, caps_only);
                    else if ((tp > 0) != (side > 0))
                        add(out, &n, from, to, 0, FL_CAP, caps_only);
                }
                if (t == KING && !caps_only) {
                    int rank = (side > 0) ? 0 : 7;
                    int ks = (side > 0) ? C_WK : C_BK;
                    int qs = (side > 0) ? C_WQ : C_BQ;
                    if (from == SQ(4, rank) && !attacked(p, from, -side)) {
                        if ((p->castle & ks) && b[SQ(7, rank)] == side * ROOK
                            && !b[SQ(5, rank)] && !b[SQ(6, rank)]
                            && !attacked(p, SQ(5, rank), -side))
                            add(out, &n, from, SQ(6, rank), 0, FL_CASTLE, 0);
                        if ((p->castle & qs) && b[SQ(0, rank)] == side * ROOK
                            && !b[SQ(1, rank)] && !b[SQ(2, rank)] && !b[SQ(3, rank)]
                            && !attacked(p, SQ(3, rank), -side))
                            add(out, &n, from, SQ(2, rank), 0, FL_CASTLE, 0);
                    }
                }
            } else {                            /* alfil, torre, dama */
                int d0 = (t == BISHOP) ? 4 : 0;
                int d1 = (t == ROOK) ? 4 : 8;
                for (int i = d0; i < d1; i++) {
                    int off = KG_OFF[i];
                    for (int to = from + off; SQ_VALID(to); to += off) {
                        int tp = b[to];
                        if (!tp) {
                            add(out, &n, from, to, 0, 0, caps_only);
                        } else {
                            if ((tp > 0) != (side > 0))
                                add(out, &n, from, to, 0, FL_CAP, caps_only);
                            break;
                        }
                    }
                }
            }
        }
    }
    return n;
}

static int castle_loss(int sq)
{
    switch (sq) {
    case 0x04: return C_WK | C_WQ;
    case 0x00: return C_WQ;
    case 0x07: return C_WK;
    case 0x74: return C_BK | C_BQ;
    case 0x70: return C_BQ;
    case 0x77: return C_BK;
    default: return 0;
    }
}

int make_move(Pos *p, Move m)
{
    int side = p->side;
    int8_t *b = p->sq;
    int pc = b[m.from];
    int t = ABS(pc);
    int cap = b[m.to];

    b[m.to] = (int8_t)pc;
    b[m.from] = 0;
    if (m.flags & FL_EP)
        b[m.to - 16 * side] = 0;
    if (m.promo)
        b[m.to] = (int8_t)(side * m.promo);
    if (m.flags & FL_CASTLE) {
        if (m.to > m.from) {
            b[m.from + 1] = b[m.from + 3];
            b[m.from + 3] = 0;
        } else {
            b[m.from - 1] = b[m.from - 4];
            b[m.from - 4] = 0;
        }
    }
    if (t == KING)
        p->ksq[IDX(side)] = m.to;
    p->castle = (uint8_t)(p->castle & ~(castle_loss(m.from) | castle_loss(m.to)));
    p->ep = (m.flags & FL_DOUBLE) ? (uint8_t)(m.from + 16 * side) : NO_SQ;
    p->half = (t == PAWN || cap || (m.flags & FL_EP)) ? 0 : (uint8_t)(p->half + 1);
    p->ply++;
    p->last_from = m.from;
    p->last_to = m.to;
    p->side = (int8_t)-side;
    return !attacked(p, p->ksq[IDX(side)], p->side);
}

int legal_moves(const Pos *p, Move *out)
{
    Move tmp[MAX_MOVES];
    int n = gen_moves(p, tmp, 0);
    int k = 0;

    for (int i = 0; i < n; i++) {
        Pos q = *p;
        if (make_move(&q, tmp[i]))
            out[k++] = tmp[i];
    }
    return k;
}

int find_move(const Pos *p, int from, int to, int promo, Move *out)
{
    Move mv[MAX_MOVES];
    int n = legal_moves(p, mv);

    for (int i = 0; i < n; i++) {
        if (mv[i].from == from && mv[i].to == to && mv[i].promo == promo) {
            *out = mv[i];
            return 1;
        }
    }
    return 0;
}

static int insufficient(const Pos *p)
{
    int minors = 0;

    for (int s = 0; s < 128; s++) {
        if (s & 0x88)
            continue;
        int t = ABS(p->sq[s]);
        if (t == PAWN || t == ROOK || t == QUEEN)
            return 0;
        if (t == KNIGHT || t == BISHOP)
            minors++;
    }
    return minors <= 1;
}

int pos_status(const Pos *p)
{
    Move mv[MAX_MOVES];
    int n = legal_moves(p, mv);
    int chk = in_check(p, p->side);

    if (!n)
        return chk ? ST_MATE : ST_STALEMATE;
    if (p->half >= 100)
        return ST_FIFTY;
    if (insufficient(p))
        return ST_MATERIAL;
    return chk ? ST_CHECK : ST_PLAY;
}

uint32_t perft(const Pos *p, int depth)
{
    Move mv[MAX_MOVES];
    uint32_t total = 0;
    int n = gen_moves(p, mv, 0);

    for (int i = 0; i < n; i++) {
        Pos q = *p;
        if (!make_move(&q, mv[i]))
            continue;
        total += (depth <= 1) ? 1u : perft(&q, depth - 1);
    }
    return total;
}

/* --- Evaluación ------------------------------------------------------------------ */

static const int PV[7] = { 0, 100, 320, 330, 500, 900, 20000 };

/* Tablas de casillas (desde las blancas, la fila 8 arriba). */
static const int8_t PST_P[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 25, 25, 10,  5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5, -5,-10,  0,  0,-10, -5,  5,
      5, 10, 10,-20,-20, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
};
static const int8_t PST_N[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};
static const int8_t PST_B[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};
static const int8_t PST_R[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};
static const int8_t PST_Q[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
static const int8_t PST_K[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};
static const int8_t PST_KE[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

static const int8_t *const PST[7] = { 0, PST_P, PST_N, PST_B, PST_R, PST_Q, PST_K };

int evaluate(const Pos *p)
{
    int score = 0;
    int npm = 0;                                /* material sin peones, de los dos */

    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            int pc = p->sq[SQ(f, r)];
            if (!pc)
                continue;
            int t = ABS(pc);
            if (t == KING)
                continue;
            int idx = (pc > 0) ? (7 - r) * 8 + f : r * 8 + f;
            int v = PV[t] + PST[t][idx];
            score += (pc > 0) ? v : -v;
            if (t != PAWN)
                npm += PV[t];
        }
    }
    const int8_t *kt = (npm <= 2600) ? PST_KE : PST_K;
    int wk = p->ksq[0], bk = p->ksq[1];
    score += kt[(7 - SQ_RANK(wk)) * 8 + SQ_FILE(wk)];
    score -= kt[SQ_RANK(bk) * 8 + SQ_FILE(bk)];
    return score;
}

/* --- Búsqueda -------------------------------------------------------------------- */

#define INF         32000
#define MATE        30000
#define MAX_QPLY    8

static uint32_t g_nodes, g_cap;
static int g_abort;

static int out_of_nodes(void)
{
    if (g_cap && g_nodes > g_cap) {
        g_abort = 1;
        return 1;
    }
    return 0;
}

/* Puntúa las jugadas para probar antes las mejores: capturas (la víctima más
 * valiosa y el atacante más barato) y coronaciones. (La mejor de la iteración
 * anterior va primero en la raíz: ver search.) */
static void score_moves(const Pos *p, const Move *mv, int n, int *sc)
{
    for (int i = 0; i < n; i++) {
        int s = 0;
        if (mv[i].flags & FL_CAP) {
            int victim = (mv[i].flags & FL_EP) ? PAWN : ABS(p->sq[mv[i].to]);
            s = 10000 + PV[victim] * 16 - ABS(p->sq[mv[i].from]);
        }
        if (mv[i].promo)
            s += PV[mv[i].promo];
        sc[i] = s;
    }
}

/* Deja en la posición i la mejor de las jugadas desde i. */
static void pick(Move *mv, int *sc, int n, int i)
{
    int best = i;
    for (int j = i + 1; j < n; j++)
        if (sc[j] > sc[best])
            best = j;
    if (best != i) {
        Move tm = mv[i];
        int ts = sc[i];
        mv[i] = mv[best];
        sc[i] = sc[best];
        mv[best] = tm;
        sc[best] = ts;
    }
}

static int qsearch(const Pos *p, int alpha, int beta, int qply)
{
    g_nodes++;
    if (out_of_nodes())
        return 0;

    int stand = evaluate(p) * p->side;
    if (stand >= beta)
        return stand;
    if (stand > alpha)
        alpha = stand;
    if (qply >= MAX_QPLY)
        return stand;

    Move mv[MAX_MOVES];
    int sc[MAX_MOVES];
    int n = gen_moves(p, mv, 1);
    score_moves(p, mv, n, sc);
    for (int i = 0; i < n; i++) {
        pick(mv, sc, n, i);
        Pos q = *p;
        if (!make_move(&q, mv[i]))
            continue;
        int s = -qsearch(&q, -beta, -alpha, qply + 1);
        if (g_abort)
            return 0;
        if (s >= beta)
            return s;
        if (s > alpha)
            alpha = s;
    }
    return alpha;
}

static int negamax(const Pos *p, int depth, int alpha, int beta, int ply)
{
    if (g_abort)
        return 0;
    int chk = in_check(p, p->side);
    if (chk && ply < 12)
        depth++;                                /* en jaque, se mira un poco más */
    if (depth <= 0)
        return qsearch(p, alpha, beta, 0);
    g_nodes++;
    if (out_of_nodes())
        return 0;
    if (p->half >= 100)
        return 0;

    Move mv[MAX_MOVES];
    int sc[MAX_MOVES];
    int n = gen_moves(p, mv, 0);
    score_moves(p, mv, n, sc);

    int best = -INF, legal = 0;
    for (int i = 0; i < n; i++) {
        pick(mv, sc, n, i);
        Pos q = *p;
        if (!make_move(&q, mv[i]))
            continue;
        legal++;
        int s = -negamax(&q, depth - 1, -beta, -alpha, ply + 1);
        if (g_abort)
            return 0;
        if (s > best) {
            best = s;
            if (s > alpha) {
                alpha = s;
                if (alpha >= beta)
                    break;
            }
        }
    }
    if (!legal)
        return chk ? -MATE + ply : 0;
    return best;
}

static uint32_t rnd(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}

void search(const Pos *p, const SearchOpts *o, SearchResult *r)
{
    Move mv[MAX_MOVES];
    int nz[MAX_MOVES];
    int n = legal_moves(p, mv);

    g_nodes = 0;
    g_cap = o->node_cap;
    g_abort = 0;
    r->found = n > 0;
    r->nodes = 0;
    r->depth = 0;
    r->score = 0;
    if (!n)
        return;
    r->best = mv[0];
    if (n == 1)
        return;

    uint32_t seed = o->seed | 1u;
    for (int i = 0; i < n; i++)
        nz[i] = o->noise ? (int)(rnd(&seed) % (uint32_t)(2 * o->noise + 1)) - o->noise : 0;

    for (int d = 1; d <= o->depth; d++) {
        int best = -INF, bi = 0, alpha = -INF;
        for (int i = 0; i < n; i++) {
            Pos q = *p;
            make_move(&q, mv[i]);
            int s = -negamax(&q, d - 1, -INF, o->noise ? INF : -alpha, 1);
            if (g_abort)
                break;
            s += nz[i];
            if (s > best) {
                best = s;
                bi = i;
                if (!o->noise && s > alpha)
                    alpha = s;
            }
        }
        if (g_abort)
            break;
        r->best = mv[bi];
        r->score = best;
        r->depth = d;
        if (best > MATE - 100 || best < -MATE + 100)
            break;                              /* mate a la vista: no hace falta más */
        /* La mejor, la primera de la próxima iteración (con ruido, los
         * índices del ruido se mueven con las jugadas). */
        Move tm = mv[0];
        int tz = nz[0];
        mv[0] = mv[bi];
        nz[0] = nz[bi];
        mv[bi] = tm;
        nz[bi] = tz;
    }
    r->nodes = g_nodes;
}
