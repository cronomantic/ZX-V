/*
 * test_chess.c - Pruebas del motor de ajedrez en el PC.
 *
 *   gcc -O2 -Wall -Wextra -I.. test_chess.c ../chess.c -o test_chess && ./test_chess
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "chess.h"

static int fails;

static void check(const char *what, long got, long want)
{
    if (got != want) {
        printf("FALLA %s: salió %ld, debía ser %ld\n", what, got, want);
        fails++;
    }
}

static void perft_test(const char *name, const char *fen, const uint32_t *want, int n)
{
    Pos p;
    if (fen)
        pos_from_fen(&p, fen);
    else
        pos_init(&p);
    for (int d = 1; d <= n; d++) {
        char buf[64];
        snprintf(buf, sizeof buf, "perft %s d%d", name, d);
        check(buf, perft(&p, d), want[d - 1]);
    }
    printf("perft %-10s hasta d%d: hecho\n", name, n);
}

static const char *mstr(Move m, char *b)
{
    b[0] = (char)('a' + SQ_FILE(m.from));
    b[1] = (char)('1' + SQ_RANK(m.from));
    b[2] = (char)('a' + SQ_FILE(m.to));
    b[3] = (char)('1' + SQ_RANK(m.to));
    b[4] = m.promo ? "?pnbrqk"[m.promo] : 0;
    b[5] = 0;
    return b;
}

static void status_test(const char *name, const char *fen, int want)
{
    Pos p;
    pos_from_fen(&p, fen);
    check(name, pos_status(&p), want);
}

static void search_test(const char *name, const char *fen, int depth, const char *want)
{
    Pos p;
    SearchOpts o = { depth, 0, 0, 1 };
    SearchResult r;
    char b[8];
    pos_from_fen(&p, fen);
    search(&p, &o, &r);
    int ok = r.found && !strcmp(mstr(r.best, b), want);
    if (!ok) {
        printf("FALLA %s: jugó %s, debía ser %s (puntos %d)\n", name, r.found ? b : "nada", want, r.score);
        fails++;
    }
}

/* Fuerza bruta: ¿esta jugada da mate en dos (o en uno)? */
static int mate_in_1(const Pos *p)
{
    Move mv[MAX_MOVES];
    int n = legal_moves(p, mv);
    for (int i = 0; i < n; i++) {
        Pos q = *p;
        make_move(&q, mv[i]);
        if (pos_status(&q) == ST_MATE)
            return 1;
    }
    return 0;
}

static int forces_mate_in_2(const Pos *p, Move m)
{
    Pos q = *p;
    make_move(&q, m);
    int st = pos_status(&q);
    if (st == ST_MATE)
        return 1;
    if (st >= ST_STALEMATE)
        return 0;
    Move rep[MAX_MOVES];
    int n = legal_moves(&q, rep);
    for (int j = 0; j < n; j++) {
        Pos r = q;
        make_move(&r, rep[j]);
        if (!mate_in_1(&r))
            return 0;
    }
    return 1;
}

static void mate2_test(const char *name, const char *fen, int depth)
{
    Pos p;
    SearchOpts o = { depth, 0, 0, 1 };
    SearchResult r;
    pos_from_fen(&p, fen);
    search(&p, &o, &r);
    if (!r.found || !forces_mate_in_2(&p, r.best)) {
        printf("FALLA %s: la jugada elegida no fuerza mate en 2\n", name);
        fails++;
    }
}

static void mate_found_test(const char *name, const char *fen, int depth)
{
    Pos p;
    SearchOpts o = { depth, 0, 0, 1 };
    SearchResult r;
    pos_from_fen(&p, fen);
    search(&p, &o, &r);
    if (r.score < 29000) {
        printf("FALLA %s: no ve el mate (puntos %d)\n", name, r.score);
        fails++;
    }
}

int main(void)
{
    static const uint32_t start[] = { 20, 400, 8902, 197281, 4865609 };
    static const uint32_t kiwi[] = { 48, 2039, 97862, 4085603 };
    static const uint32_t pos3[] = { 14, 191, 2812, 43238, 674624 };
    static const uint32_t pos4[] = { 6, 264, 9467, 422333 };
    static const uint32_t pos5[] = { 44, 1486, 62379, 2103487 };
    static const uint32_t pos6[] = { 46, 2079, 89890, 3894594 };

    perft_test("inicial", 0, start, 5);
    perft_test("kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", kiwi, 4);
    perft_test("pos3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", pos3, 5);
    perft_test("pos4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", pos4, 4);
    perft_test("pos5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", pos5, 4);
    perft_test("pos6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", pos6, 4);

    /* Estados */
    status_test("inicial", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", ST_PLAY);
    status_test("mate del loco", "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", ST_MATE);
    status_test("jaque", "rnbqkbnr/ppp2ppp/3p4/4p3/Q7/2P5/PP1PPPPP/RNB1KBNR b KQkq - 1 3", ST_CHECK);
    status_test("ahogado", "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", ST_STALEMATE);
    status_test("rey solo", "8/8/4k3/8/8/3K4/8/8 w - - 0 1", ST_MATERIAL);
    status_test("rey y caballo", "8/8/4k3/8/8/3KN3/8/8 w - - 0 1", ST_MATERIAL);
    status_test("rey y peon", "8/8/4k3/8/8/3K4/4P3/8 w - - 0 1", ST_PLAY);
    status_test("50 jugadas", "8/8/4k3/8/8/3K4/4P3/8 w - - 100 80", ST_FIFTY);

    /* La IA: mates en uno y en dos, y no regalar la dama. */
    search_test("mate en 1 (Dh5#?)", "r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4", 3, "h5f7");
    search_test("mate en 1 (torre)", "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", 2, "a1a8");
    mate2_test("mate en 2 (rey y torre)", "7k/8/5K2/8/8/8/8/R7 w - - 0 1", 4);
    mate_found_test("mate en 3 (visto a profundidad 6)", "r5rk/5p1p/5R2/4B3/8/8/7P/7K w - - 0 1", 6);
    search_test("captura gratis", "4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1", 2, "d1d5");
    search_test("promociona", "8/P6k/8/8/8/8/8/K7 w - - 0 1", 3, "a7a8q");

    /* Partidas entre niveles: que no falle y que todas las jugadas sean legales. */
    static const struct { int d; uint32_t cap; int noise; } lv[4] = {
        { 1, 0, 100 }, { 2, 0, 30 }, { 4, 40000, 0 }, { 6, 100000, 0 }   /* los de game.c */
    };
    for (int g = 0; g < 4; g++) {
        Pos p;
        pos_init(&p);
        int la = g, lb = (g + 1) % 4, st = ST_PLAY, plies = 0;
        uint32_t maxn = 0, sum = 0;
        double tmax = 0;
        while (plies < 300) {
            st = pos_status(&p);
            if (st >= ST_MATE)
                break;
            const typeof(lv[0]) *L = &lv[(plies & 1) ? lb : la];
            SearchOpts o = { L->d, L->cap, L->noise, 12345u + (uint32_t)plies * 7919u };
            SearchResult r;
            clock_t t0 = clock();
            search(&p, &o, &r);
            double dt = (double)(clock() - t0) / CLOCKS_PER_SEC;
            if (dt > tmax) tmax = dt;
            if (r.nodes > maxn) maxn = r.nodes;
            sum += r.nodes;
            Move chk;
            if (!r.found || !find_move(&p, r.best.from, r.best.to, r.best.promo, &chk)) {
                printf("FALLA partida %d: jugada ilegal en la media jugada %d\n", g, plies);
                fails++;
                break;
            }
            if (!make_move(&p, r.best)) {
                printf("FALLA partida %d: make_move dice ilegal\n", g);
                fails++;
                break;
            }
            plies++;
        }
        printf("partida nivel %d (blancas) vs nivel %d: %d medias jugadas, estado %d, nodos max %u, media %u, tiempo max en PC %.3fs\n",
               la + 1, lb + 1, plies, st, maxn, plies ? sum / (uint32_t)plies : 0, tmax);
    }

    /* Velocidad bruta de nodos. */
    {
        Pos p;
        pos_init(&p);
        SearchOpts o = { 6, 0, 0, 1 };
        SearchResult r;
        clock_t t0 = clock();
        search(&p, &o, &r);
        double dt = (double)(clock() - t0) / CLOCKS_PER_SEC;
        printf("inicial, profundidad 6 sin límite: %u nodos en %.2fs (%.2f Mnodos/s)\n",
               r.nodes, dt, r.nodes / dt / 1e6);
    }

    printf(fails ? "\n%d FALLOS\n" : "\nTodo correcto\n", fails);
    return fails != 0;
}
