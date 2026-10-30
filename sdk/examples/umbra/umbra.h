/*
 * umbra.h - Umbra, un laberinto en 3D para ZX-V: lo que comparten sus partes.
 *
 *   video.c  la pantalla: doble buffer, el texto y los sprites
 *   art.c    los gráficos y los niveles, los de umbra.gfx: las texturas, los
 *            sprites y un mapa por nivel
 *   view.c   la vista en 3D: las paredes por raycasting, el suelo y el techo
 *            con el modo 7, la niebla de la lámpara y los objetos
 *   maze.c   los laberintos (los mapas de umbra.gfx), lo que hay en ellos y
 *            las sombras
 *   sound.c  la música (primer AY, módulos .pt3) y los efectos (segundo AY,
 *            el banco umbra.afb)
 *   game.c   la partida, el marcador, el mapa, el título y la demostración
 *
 * La memoria de vídeo que no se ve también se usa: en el modo 0, de cada
 * página de 96 KB la pantalla solo enseña los primeros 24. En la página 0 van
 * las texturas (art.c); en la 1, los sprites preparados y las músicas de la
 * tarjeta (sound.c).
 */
#ifndef UMBRA_H
#define UMBRA_H

#include "zxv_gfx.h"

#define W           256
#define H           192
#define VIEW_H      144                 /* la vista en 3D; debajo, el marcador */
#define HORIZON     72                  /* la línea a la altura de los ojos */
#define FOCAL       160                 /* la distancia focal, en píxeles */
#define COLS        128                 /* columnas de 2 píxeles de la vista */

/* Las posiciones van en celdas del laberinto, en coma fija 16.16 (ONE es una
 * celda); una celda son 64 texels de las texturas. Los ángulos, en 1/1024 de
 * vuelta: 0 mira hacia x creciente y 256, hacia y creciente. */
#define ONE         65536
#define TURN        1024

#define SPARE_SIZE  (ZXV_VRAM_PAGE_SIZE - W * H / 2)
#define SPARE0      ((uint8_t *)(uintptr_t)(ZXV_VRAM0_BASE + W * H / 2))
#define SPARE1      ((uint8_t *)(uintptr_t)(ZXV_VRAM1_BASE + W * H / 2))
#define MUSIC_ROOM  (24 * 1024)
#define MUSIC_MEM   (SPARE1 + SPARE_SIZE - MUSIC_ROOM)

/* --- video.c ------------------------------------------------------------------ */

typedef struct {
    const void *spr;
    uint16_t w, h;
} sprite_t;

void video_init(void);
void video_begin(void);                 /* elige la página oculta */
void video_end(void);                   /* la enseña en el siguiente VBLANK y lo espera */
void video_done(void);                  /* vuelve a la página 0 */
uint32_t video_lost(void);              /* frames perdidos (el dibujo no cupo en uno) */
uint8_t *video_page(void);              /* la página de dibujo */

void text(int x, int y, const char *s, int c);
void text_shadow(int x, int y, const char *s, int c);
void text_center(int y, const char *s, int c);
int text_width(const char *s);
void text_big(int x, int y, const char *s, int size, const uint8_t *row_colours);
void number(char *out, uint32_t v, int digits);
void draw_sprite(const sprite_t *s, int x, int y, int flags, const uint8_t *colours);

/* --- art.c -------------------------------------------------------------------- */

enum { SPR_RUNE, SPR_FLASK, SPR_SHADOW, SPR_LAMP = 5, SPR_RUNE_ICON, SPRITES };

/* Las paredes: dos texturas de 128 x 128 con cuatro dibujos de 64 x 64. */
enum { WALL_STONE, WALL_BRICK, WALL_MOSS, WALL_CRYSTAL, WALL_TEMPLE, WALL_GATE, WALL_OPEN, WALL_RUNE };

extern sprite_t art[SPRITES];
extern zxv_m7tex tex_floor, tex_ceiling;

void art_init(void);
const uint8_t *art_wall(int n);         /* el texel de arriba a la izquierda; filas de 128 */
int art_levels(void);                   /* los mapas 0, 1, 2... seguidos que hay */
/* Las celdas del mapa del nivel en buf (de room bytes), su ancho y su alto;
 * 0 si no está o no cabe. */
int art_level(int n, uint16_t *buf, uint32_t room, int *w, int *h);

/* --- maze.c ------------------------------------------------------------------- */

#define MAZE_MAX    32                  /* celdas de lado, paredes incluidas */
#define ITEMS       24
#define SHADOWS     8

/* Las celdas del laberinto: el suelo, las paredes (CELL_WALL + el dibujo de
 * la pared, WALL_STONE...) y la puerta de salida. */
enum { CELL_FLOOR, CELL_WALL, CELL_GATE = CELL_WALL + 8 };
static inline int cell_is_wall(int c) { return c != CELL_FLOOR; }

/* Los valores de las celdas en los mapas de umbra.gfx (el número de tile). */
enum {
    MAP_FLOOR, MAP_STONE, MAP_BRICK, MAP_MOSS, MAP_CRYSTAL, MAP_TEMPLE, MAP_RUNE_WALL, MAP_EXIT,
    MAP_RUNE, MAP_FLASK, MAP_SHADOW, MAP_START_E, MAP_START_S, MAP_START_W, MAP_START_N
};

enum { ITEM_RUNE, ITEM_FLASK };

typedef struct {
    int32_t x, y;                       /* el centro de su celda */
    uint8_t kind, taken;
} item_t;

typedef struct {
    int32_t x, y;
    int8_t tx, ty;                      /* la celda a la que va */
    int8_t dir;                         /* hacia dónde va: 0-3 */
    uint8_t chasing;
    int rest;                           /* lo que sobra del avance (per_frame) */
} shadow_t;

extern int maze_w, maze_h, level, levels, level_wall;
extern uint8_t maze[MAZE_MAX * MAZE_MAX];
extern uint8_t seen[MAZE_MAX * MAZE_MAX];       /* lo que ya se ha visto: el mapa */
extern uint8_t dist[MAZE_MAX * MAZE_MAX];       /* pasos hasta el jugador (255: lejos) */
extern item_t items[ITEMS];
extern int item_count, runes_total, runes_left;
extern shadow_t shadows[SHADOWS];
extern int shadow_count;
extern int start_x, start_y, start_ang, gate_x, gate_y, gate_open;

void maze_init(void);                   /* cuántos niveles hay; mira que todos valgan */
void maze_build(int lvl);               /* el laberinto del nivel, con lo que hay en él */
int cell_at(int x, int y);              /* fuera del laberinto, pared */
int cell_solid(int x, int y);           /* las paredes y la puerta cerrada */
void maze_distances(int x, int y);      /* dist[], desde esa celda */
/* Pasos hasta la celda (x, y) desde cada una, en out (255: no se llega). */
void maze_flood(uint8_t *out, int x, int y);
void shadows_respawn(shadow_t *s, int min_dist);    /* lejos del jugador (dist[]) */
void shadows_update(int speed);         /* speed: celdas por segundo x 256 */

/* --- view.c ------------------------------------------------------------------- */

typedef struct {
    int32_t x, y;
    int ang;
} cam_t;

extern const int32_t sin_q[257];        /* seno de un cuarto de vuelta x 65536 */
int32_t isin(int a);                    /* a en 1/1024 de vuelta; x 65536 */
static inline int32_t icos(int a) { return isin(a + TURN / 4); }

void view_level(int lvl);               /* los colores del suelo y del techo del nivel */
/* Dibuja lo que se ve desde cam con la lámpara de alcance light (en celdas,
 * 16.16), y los objetos y las sombras; t, el tiempo, para animarlos. */
void view_draw(const cam_t *cam, int32_t light, int t);

/* --- sound.c ------------------------------------------------------------------ */

enum { SONG_NONE, SONG_TITLE, SONG_MAZE, SONG_CLEAR, SONG_OVER, SONGS };
/* Los efectos, por su número en umbra.afb. */
enum { FX_RUNE, FX_FLASK, FX_HIT, FX_GATE, FX_DEATH, FX_STEP, FX_BEEP, FX_HEART,
       FX_WHISPER_1, FX_WHISPER_2, FX_WHISPER_3, FX_COUNT };

void sound_init(void);
void sound_frame(void);                 /* una vez por frame */
void sound_song(int song);
void sound_fx(int fx);
/* Lo que suena sin parar: el latido de la lámpara que se apaga (0 nada; 1-3,
 * más deprisa) y el susurro de la sombra más cercana (0 nada; 1-3, más fuerte). */
void sound_ambient(int heart, int whisper);
void sound_mute(int on);                /* la pausa */
void sound_music(int on);               /* la música sí o no (la tecla S) */
int sound_music_on(void);
void sound_off(void);

/* --- game.c ------------------------------------------------------------------- */

extern uint32_t rng_state;
static inline uint32_t rnd(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state >> 16;
}

extern int lang;                        /* 0 español, 1 inglés */
#define T(es, en) (lang ? (en) : (es))

/* Lo que toca en este frame de algo que va a per_second por segundo; *rest
 * guarda lo que sobra, para el siguiente. */
static inline int per_frame(int *rest, int per_second)
{
    int v = per_second + *rest;
    int q = v / ZXV_FRAME_HZ;
    *rest = v - q * ZXV_FRAME_HZ;
    return q;
}

#endif /* UMBRA_H */
