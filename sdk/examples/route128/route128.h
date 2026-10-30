/*
 * route128.h - Route 128, un juego de carreras para ZX-V: lo que comparten sus
 * partes.
 *
 *   video.c  la pantalla: doble buffer, los tramos de color de la carretera
 *            (con tramado), el texto y los sprites
 *   art.c    los gráficos, los de route128.gfx
 *   road.c   el circuito (curvas y cambios de rasante) y su proyección
 *   sound.c  el motor, el derrape y los golpes (segundo AY) y la música (primero)
 *
 * La memoria de vídeo que no se ve también se usa: en el modo 0, de cada
 * página de 96 KB la pantalla solo enseña los primeros 24; el resto es memoria
 * como la otra. Ahí van los sprites preparados (art.c), las músicas de la
 * tarjeta (sound.c) y el paisaje de la etapa (art.c).
 *   game.c   la carrera, el tráfico, el marcador, el título y la demostración
 */
#ifndef RUTA128_H
#define RUTA128_H

#include "zxv_gfx.h"

#define W           256
#define H           192
#define HORIZON     96                  /* la línea a la altura de la cámara */

#define SPARE_SIZE  (ZXV_VRAM_PAGE_SIZE - W * H / 2)
#define SPARE0      ((uint8_t *)(uintptr_t)(ZXV_VRAM0_BASE + W * H / 2))
#define SPARE1      ((uint8_t *)(uintptr_t)(ZXV_VRAM1_BASE + W * H / 2))
#define BG_ROOM     (24 * 1024)         /* el paisaje: tres capas de 512 x 96 en total */
#define BG_MEM      (SPARE1 + SPARE_SIZE - BG_ROOM)
#define MUSIC_ROOM  (24 * 1024)
#define MUSIC_MEM   (BG_MEM - MUSIC_ROOM)

/* --- video.c ------------------------------------------------------------------ */

/* Un relleno de 2 x 2 píxeles: el byte bajo para las líneas pares, el alto para
 * las impares; en cada byte, el nibble bajo es el píxel par. Con dos colores
 * salen los tramados que hacen de grises (la paleta no los tiene). */
typedef uint16_t pat_t;
#define PB(a, b)        ((a) | (b) << 4)
#define SOLID(c)        ((pat_t)(PB(c, c) | PB(c, c) << 8))
#define CHECK(a, b)     ((pat_t)(PB(a, b) | PB(b, a) << 8))     /* la mitad de cada uno */
#define SPARSE(a, b)    ((pat_t)(PB(a, b) | PB(a, a) << 8))     /* una cuarta parte de b */

typedef struct {
    const void *spr;
    uint16_t w, h;
} sprite_t;

void video_init(void);
void video_begin(void);                 /* elige la página oculta */
void video_end(void);                   /* la enseña en el siguiente VBLANK y lo espera */
void video_done(void);                  /* vuelve a la página 0 */
uint32_t video_lost(void);              /* frames perdidos (el dibujo no cupo en uno) */

void span(int y, int x0, int x1, pat_t p);      /* de x0 a x1 (sin x1) */
void rows(int y0, int y1, pat_t p);             /* líneas enteras, de y0 a y1 (sin y1) */
void pset(int x, int y, int c);

void text(int x, int y, const char *s, int c);
void text_shadow(int x, int y, const char *s, int c);  /* con sombra negra, sobre el paisaje */
void text_center(int y, const char *s, int c);
int text_width(const char *s);
void text_big(int x, int y, const char *s, int size, const uint8_t *row_colours);
void number(char *out, uint32_t v, int digits);

void draw_sprite(const sprite_t *s, int x, int y, int flags, const uint8_t *colours);
void draw_scaled(const sprite_t *s, int x, int y, int w, int h, int flags, const uint8_t *colours);

/* --- art.c -------------------------------------------------------------------- */

enum {                                  /* lo que hay junto a la carretera */
    OBJ_PALM, OBJ_BUSH, OBJ_UMBRELLA, OBJ_CACTUS, OBJ_ROCK, OBJ_PINE, OBJ_TREE,
    OBJ_SIGN_L, OBJ_SIGN_R, OBJ_BOARD, OBJ_START, OBJ_GATE, OBJ_GOAL, OBJ_COUNT
};
enum { CAR_COUPE, CAR_VAN, CAR_TRUCK, CAR_TYPES };

typedef struct {
    sprite_t s;
    uint16_t world_w;                   /* el ancho en el mundo (el de la carretera es 2 * ROAD_HW) */
    uint8_t solid;                      /* se choca con él */
    uint8_t mirror;                     /* a la derecha de la carretera, en espejo */
} object_art_t;

extern object_art_t obj_art[OBJ_COUNT];
extern sprite_t car_art[CAR_TYPES], player_art[5];  /* el coche: recto, izquierda y derecha, y más */
extern const uint8_t car_colours[6][16];
extern zxv_bitmap bg_far, bg_near, bg_clouds;   /* el paisaje de la etapa (art_background) */

void art_init(void);
void art_background(int theme);         /* el paisaje de una etapa */

/* --- road.c ------------------------------------------------------------------- */

#define SEG_LEN     200                 /* lo que mide un segmento */
#define ROAD_HW     1100                /* la mitad del ancho de la carretera */
#define CAM_H       1000                /* la altura de la cámara sobre la carretera */
#define DEPTH       110                 /* la distancia focal, en píxeles */
#define PLAYER_Z    1300                /* lo que va el coche por delante de la cámara */
#define DRAW_SEGS   160                 /* los segmentos que se ven */
#define X1          1024                /* una media carretera, en las posiciones laterales */

enum { THEME_COAST, THEME_DESERT, THEME_FOREST, THEMES };

typedef struct {
    int32_t z;                          /* a lo largo del circuito */
    int32_t speed;                      /* en unidades por segundo x 16 */
    int z_rest;                         /* lo que sobra del avance (per_frame) */
    int16_t x;                          /* lateral, en 1/X1 de media carretera */
    int16_t target;                     /* el carril al que va */
    uint8_t type, colour;
} car_t;

#define CARS        40
extern car_t cars[CARS];

extern int track_segs;                  /* los segmentos del circuito */
extern int32_t track_len;               /* lo que mide */
extern int goal_seg;                    /* el segmento de la meta */
extern int checkpoint_seg[3];           /* los controles (el último, la meta) */

void track_build(void);
int seg_curve(int seg);                 /* x 16 */
int32_t seg_height(int seg);
int seg_theme(int seg);
int32_t height_at(int32_t z);           /* la altura de la carretera en z */

/* Los objetos del segmento: cuántos, y el primero en *first. */
int seg_objects(int seg, int *first);
int obj_type(int i);
int obj_x(int i);

/* Dibuja la carretera vista desde la cámara (z, x en 1/X1; cam_y, su altura:
 * la de la carretera bajo el coche más CAM_H), el paisaje de la etapa theme
 * desplazado bg_x y lo que hay en la carretera, de lejos a cerca. player
 * dibuja el coche: road_draw lo llama en su sitio, delante de lo que queda
 * detrás de él y detrás de lo que hay entre él y la cámara. */
void road_draw(int32_t z, int x, int32_t cam_y, int theme, int bg_x, void (*player)(void));

/* --- sound.c ------------------------------------------------------------------ */

enum { SONG_NONE, SONG_RACE, SONG_TITLE, SONG_GOAL, SONG_OVER };
enum { FX_CHECKPOINT, FX_CRASH, FX_BUMP, FX_BEEP, FX_GO, FX_SHIFT };

void sound_init(void);
void sound_frame(void);                 /* una vez por frame (la música va a 50 pasos por segundo) */
void sound_song(int song);
void sound_fx(int fx);
void sound_engine(int rpm, int skid, int offroad);  /* rpm 0-1000; skid y offroad 0-15 */
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

extern const int8_t sin64[64];          /* seno x 64, 64 pasos por vuelta */
static inline int isin(int a) { return sin64[a & 63]; }

extern int lang;                        /* 0 español, 1 inglés */
#define T(es, en) (lang ? (en) : (es))

#endif /* RUTA128_H */
