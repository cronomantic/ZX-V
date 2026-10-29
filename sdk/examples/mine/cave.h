/*
 * cave.h - Las reglas de la mina: la cuadrícula, lo que cae, lo que rueda,
 * los bichos, el musgo, el tamiz y el piloto automático.
 *
 * Sin nada de la máquina (ni la ROM ni la pantalla): cave.c se compila
 * también en el anfitrión, para que el piloto pruebe las cuevas.
 */
#ifndef CAVE_H
#define CAVE_H

#include <stdint.h>

#define CW          40                  /* la cuadrícula: 40 x 22 celdas como mucho */
#define CH          22
#define DONE        0x80                /* la celda ya se ha movido en este paso */

/* Lo que hay en una celda. */
enum {
    O_EMPTY, O_DIRT, O_WALL, O_STEEL,
    O_BOULDER, O_BOULDER_F, O_DIAMOND, O_DIAMOND_F,     /* quieto y cayendo */
    O_MINER, O_ENTRY, O_EXIT, O_EXIT_OPEN, O_MOSS, O_SIEVE,
    O_EMBER, O_BAT = O_EMBER + 4,       /* + la dirección en la que van */
    O_BOOM = O_BAT + 4,                 /* explosión que deja hueco, 4 pasos */
    O_DBOOM = O_BOOM + 4,               /* la que deja diamantes */
    O_COUNT = O_DBOOM + 4
};

enum { D_UP, D_RIGHT, D_DOWN, D_LEFT, D_NONE };

/* Lo que ha pasado en un paso (para el sonido). */
enum {
    EV_DIG = 1, EV_COLLECT = 2, EV_LAND = 4, EV_TING = 8, EV_BOOM = 16, EV_OPEN = 32,
    EV_PUSH = 64, EV_SIEVE = 128, EV_EXIT = 256, EV_BORN = 512, EV_MOSS = 1024, EV_CRUSH = 2048,
    EV_CHANGE = 4096                    /* el musgo se ha vuelto diamantes o piedras */
};

enum { ST_BORN, ST_PLAY, ST_DEAD, ST_OUT };
enum { SV_IDLE, SV_ACTIVE, SV_DONE };

typedef struct {
    const char *es, *en;
    uint8_t w, h;                       /* con el borde de acero */
    uint8_t need, time;                 /* diamantes; segundos */
    uint8_t value, extra;               /* puntos por diamante, antes y después de los necesarios */
    uint8_t tick;                       /* lo que dura un paso, en milisegundos */
    uint8_t p_boulder, p_diamond, p_empty;  /* el relleno de los '.', en 1/256 */
    uint8_t seed;
    uint8_t moss_slow, sieve_time;      /* segundos */
    uint8_t dirt, dirt2, wall;          /* los colores */
    uint8_t bonus;                      /* cueva de premio: sin perder vidas */
    const char *const *rows;            /* h - 2 filas de w - 2 */
} cave_t;

extern const cave_t caves[];
extern const int ncaves;

typedef struct {
    uint8_t g[CW * CH];
    uint8_t w, h, cave, tick;
    uint8_t state, open, sieve, rec;
    int16_t mi;                         /* la celda del minero (o la entrada) */
    int16_t got, need, born, push;
    int16_t sieve_t, moss_slow, moss_n;
    uint8_t facing, walked;
    uint32_t rng, ev, points, ticks;
} world_t;

/* Los movimientos del último paso, para dibujarlos entre una celda y otra. */
typedef struct {
    uint16_t at;                        /* la celda de llegada */
    uint8_t dir;
} move_t;

#define MAX_MOVES   256
extern move_t moves[MAX_MOVES];
extern int nmoves;

void world_load(world_t *w, int cave);
void world_tick(world_t *w, int dir, int grab);
void world_kill(world_t *w);           /* sin tiempo: el minero estalla */
int world_diamonds(const world_t *w);  /* los que hay en la cueva */

/* El piloto automático: la dirección (o D_NONE) y si coge sin moverse. */
int bot_input(const world_t *w, int *grab);

static inline int is_enemy(int t) { return t >= O_EMBER && t < O_BOOM; }
static inline int is_falling(int t) { return t == O_BOULDER_F || t == O_DIAMOND_F; }

#endif /* CAVE_H */
