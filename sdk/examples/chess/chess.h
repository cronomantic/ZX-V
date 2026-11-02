/*
 * chess.h - El motor de ajedrez: reglas completas y un jugador artificial.
 *
 * No depende de ZX-V ni de la biblioteca de C (solo de <stdint.h>), así que
 * las mismas reglas se prueban en el PC (ver test/test_chess.c).
 *
 * El tablero es un vector de 128 casillas ("0x88"): la casilla es
 * fila * 16 + columna, con la fila 0 = la primera (blancas) y la columna 0 = a.
 * Una casilla sin la máscara 0x88 es una casilla del tablero.
 */
#ifndef CHESS_H
#define CHESS_H

#include <stdint.h>

#define WHITE   1
#define BLACK   (-1)

/* Las piezas: positivas las blancas, negativas las negras. */
enum { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

/* Derechos de enroque. */
#define C_WK    1
#define C_WQ    2
#define C_BK    4
#define C_BQ    8

/* Marcas de una jugada. */
#define FL_CAP      1           /* captura (también al paso) */
#define FL_EP       2           /* captura al paso */
#define FL_CASTLE   4           /* enroque (el rey salta dos casillas) */
#define FL_DOUBLE   8           /* el peón avanza dos casillas */

#define SQ(file, rank)  ((rank) * 16 + (file))
#define SQ_FILE(s)      ((s) & 7)
#define SQ_RANK(s)      ((s) >> 4)
#define SQ_VALID(s)     (!((s) & 0x88))
#define NO_SQ           0xFF

typedef struct {
    uint8_t from, to;
    uint8_t promo;              /* 0, o KNIGHT..QUEEN */
    uint8_t flags;
} Move;

typedef struct {
    int8_t sq[128];             /* el tablero */
    int8_t side;                /* a quién le toca: WHITE o BLACK */
    uint8_t castle;             /* C_WK | C_WQ | C_BK | C_BQ */
    uint8_t ep;                 /* casilla de captura al paso, o NO_SQ */
    uint8_t half;               /* jugadas desde la última captura o de peón */
    uint8_t ksq[2];             /* dónde están los reyes: [0] blanco, [1] negro */
    uint16_t ply;               /* medias jugadas hechas */
    uint8_t last_from, last_to; /* la última jugada, para enseñarla (o NO_SQ) */
} Pos;

/* Estado de una posición (pos_status). */
enum {
    ST_PLAY,                    /* se sigue jugando */
    ST_CHECK,                   /* jaque */
    ST_MATE,                    /* jaque mate: pierde quien tiene el turno */
    ST_STALEMATE,               /* rey ahogado: tablas */
    ST_FIFTY,                   /* regla de las 50 jugadas: tablas */
    ST_MATERIAL                 /* material insuficiente: tablas */
};

#define MAX_MOVES   256

void pos_init(Pos *p);                          /* la posición inicial */
int pos_from_fen(Pos *p, const char *fen);      /* 0 si va bien (para las pruebas) */

int attacked(const Pos *p, int sq, int by_side);
int in_check(const Pos *p, int side);

/* Genera las jugadas pseudo-legales (sin comprobar si dejan al rey en jaque). */
int gen_moves(const Pos *p, Move *out, int caps_only);
/* Hace la jugada en el sitio; devuelve 0 si deja al rey propio en jaque (y entonces
 * la posición no vale para nada: trabaja sobre una copia). */
int make_move(Pos *p, Move m);
/* Las jugadas legales. */
int legal_moves(const Pos *p, Move *out);
/* Busca entre las legales la que sale de `from` y llega a `to` (con esa corona). */
int find_move(const Pos *p, int from, int to, int promo, Move *out);

int pos_status(const Pos *p);
int evaluate(const Pos *p);                     /* centipeones, desde las blancas */

/* Cuenta las posiciones a profundidad d (para comprobar las reglas). */
uint32_t perft(const Pos *p, int depth);

typedef struct {
    int depth;                  /* profundidad máxima (medias jugadas) */
    uint32_t node_cap;          /* nodos como mucho (0: sin límite) */
    int noise;                  /* ruido en la puntuación de cada jugada, en centipeones */
    uint32_t seed;              /* semilla del ruido */
} SearchOpts;

typedef struct {
    Move best;
    int score;                  /* desde el punto de vista de quien juega */
    int depth;                  /* profundidad completada */
    uint32_t nodes;
    int found;                  /* 0 si no hay jugadas legales */
} SearchResult;

void search(const Pos *p, const SearchOpts *o, SearchResult *r);

#endif /* CHESS_H */
