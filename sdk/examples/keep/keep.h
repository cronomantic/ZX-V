/*
 * keep.h - The Keep, una aventura de mazmorras para ZX-V: lo que comparten
 * sus partes.
 *
 *   video.c   la pantalla: doble buffer, el texto (con la fuente de keep.gfx),
 *             los sprites y los recuadros
 *   art.c     los gráficos, los de keep.gfx: sprites, tiles, fuente y plantas
 *   world.c   la planta: el mapa, lo que se pisa, las puertas, lo que sale en
 *             cada sala y dónde está cada cosa
 *   actors.c  el héroe, los enemigos, el hechicero, las flechas y lo que cae
 *   sound.c   la música (keep-*.pt3) y los efectos (keep.afb)
 *   game.c    la partida, el marcador, los mensajes, el título, el récord y
 *             el piloto de la demostración
 *
 * El mundo va en píxeles (una planta de 3 x 3 salas son 768 x 528) y las
 * posiciones en 1/16 de píxel (FIX): todo se mueve en enteros por frame, a
 * 60 imágenes por segundo.
 */
#ifndef KEEP_H
#define KEEP_H

#include "zxv_gfx.h"

#define W           256
#define H           192
#define HUD_H       24                  /* el marcador, arriba: dos líneas */
#define VIEW_TOP    16                  /* la línea de la pantalla del borde de arriba de la sala */
/* Una sala mide 256 x 176 y va de la línea VIEW_TOP hasta abajo: el marcador
 * tapa la mitad de fuera de su pared de arriba, que solo es pared. */

#define TILE        16
#define ROOM_W      16                  /* una sala, en celdas */
#define ROOM_H      11
#define ROOMS       3                   /* 3 x 3 salas por planta */
#define MAP_W       (ROOM_W * ROOMS)    /* 48 x 33 celdas */
#define MAP_H       (ROOM_H * ROOMS)
#define FLOORS      4
#define FIX         16                  /* 1/16 de píxel */

/* Los tiles de las plantas (los números de keep.gfx). */
enum {
    T_FLOOR0, T_FLOOR1, T_FLOOR2, T_FLOOR3,     /* los suelos de cada planta */
    T_WALL, T_FACE, T_PILLAR, T_BARREL, T_SHELF, T_TORCH,
    T_WATER, T_SPIKES, T_BRAMBLE, T_DOOR, T_BOSS_DOOR, T_STAIRS,
    T_RUG, T_CRACK, T_ALTAR,
    T_MARK = 32,                        /* las marcas: lo que sale en esa celda */
};
enum {
    M_START, M_SAGE, M_SLIME, M_BAT, M_SKELETON, M_ARCHER, M_BOSS,
    M_KEY, M_POTION, M_HEART, M_GOLD, M_SPECIAL, MARKS
};

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

void text(int x, int y, const char *s, int c);
void text_shadow(int x, int y, const char *s, int c);
void text_center(int y, const char *s, int c);
int text_width(const char *s);
void text_big(int x, int y, const char *s, int size, int c);
void number(char *out, uint32_t v, int digits);
char *append(char *out, const char *s);         /* copia s y da el final */
void box(int x, int y, int w, int h);           /* un recuadro para los mensajes */

void draw_sprite(const sprite_t *s, int x, int y, int flags, const uint8_t *colours);
void draw_scaled(const sprite_t *s, int x, int y, int w, int h, int flags, const uint8_t *colours);

/* --- art.c -------------------------------------------------------------------- */

enum {                                  /* los sprites de keep.gfx */
    S_HERO = 0,                         /* abajo, arriba y a un lado: andar 2, atacar 1 */
    S_SWORD_H = 9, S_SWORD_V = 10,
    S_SLIME = 11, S_BAT = 13, S_SKELETON = 15, S_ARCHER = 17,
    S_ARROW_H = 19, S_ARROW_V = 20, S_SAGE = 21, S_BOSS = 23, S_FIRE = 26, S_PUFF = 28,
    S_KEY = 30, S_POTION, S_HEART, S_GOLD, S_COIN, S_SMALL_HEART,
    S_BOOTS, S_BIG_SWORD, S_LANTERN, S_BIG_KEY,
    S_ICONS = 40,                       /* el marcador, de 8 x 8 */
    S_TOWER = 50, S_FONT = 51,
    SPRITES
};
enum {                                  /* los iconos del marcador */
    I_HEART, I_HALF, I_EMPTY, I_KEY, I_POTION, I_COIN, I_BOOTS, I_SWORD, I_LANTERN, I_BIG_KEY
};

extern sprite_t spr[SPRITES];
extern zxv_tilemap floor_map;           /* la planta, la de world.c */

void art_init(void);
/* La planta f de keep.gfx en cells (MAP_W x MAP_H); 0 si no está o no es de
 * ese tamaño. */
int art_floor(int f, uint16_t *cells);
/* El glifo del carácter g de la fuente (0 si no está). */
const void *art_glyph(int g);

/* --- world.c ------------------------------------------------------------------ */

enum { SP_START, SP_SAGE, SP_ENEMY, SP_ITEM };

typedef struct {
    uint8_t mark;                       /* M_... */
    uint8_t cx, cy;                     /* la celda */
    uint8_t alive;                      /* el enemigo vive, el objeto sigue ahí */
} spawn_t;

#define SPAWNS      96
extern spawn_t spawns[SPAWNS];
extern int nspawns;
extern uint16_t cells[MAP_H][MAP_W];
extern int floor_base;                  /* el suelo de la planta, el que queda bajo las marcas */

void world_load(int f);                 /* la planta f, con todo como al empezar */
int cell_at(int px, int py);            /* el tile del píxel del mundo (paredes fuera) */
int solid_tile(int t);                  /* no se pasa (los murciélagos sí, por agua y pinchos) */
int wall_tile(int t);                   /* ni volando */
void open_gate(int cx, int cy);         /* una puerta o zarza: suelo, con las que la tocan */
int find_start(int *px, int *py);       /* dónde empieza la planta, en píxeles */

/* --- actors.c ----------------------------------------------------------------- */

enum {
    A_NONE, A_SLIME, A_SLIMELET, A_BAT, A_SKELETON, A_ARCHER, A_BOSS,
    A_ARROW, A_FIRE, A_PUFF, A_COIN, A_SMALL_HEART, A_SAGE,
};
enum { D_DOWN, D_UP, D_RIGHT, D_LEFT };

typedef struct {
    uint8_t type;
    int8_t hp;
    uint8_t dir;
    uint8_t spawn;                      /* su sitio en spawns, o 255 */
    int32_t x, y;                       /* arriba a la izquierda, en FIX */
    int16_t vx, vy;                     /* en FIX por frame */
    uint16_t t;                         /* el reloj de lo que esté haciendo */
    uint8_t state;
    uint8_t hurt;                       /* frames de retroceso tras un golpe */
    uint8_t swing;                      /* el último tajo que le dio */
    uint8_t value;                      /* el oro de una moneda */
} actor_t;

#define ACTORS      40
extern actor_t actors[ACTORS];

enum { ITEM_BOOTS = 1, ITEM_SWORD = 2, ITEM_LANTERN = 4, ITEM_BIG_KEY = 8 };

typedef struct {
    int32_t x, y;                       /* arriba a la izquierda, en FIX */
    int32_t safe_x, safe_y;             /* el último sitio sin pinchos */
    uint8_t dir;
    int hp, hp_max;                     /* en medios corazones */
    int keys, potions, gold, xp, level;
    int items;                          /* ITEM_... */
    int attack;                         /* frames del tajo */
    uint8_t swing;                      /* el número de tajo */
    int hurt;                           /* frames de invulnerabilidad */
    int kx, ky;                         /* el retroceso, en FIX por frame */
    int walk;                           /* para la animación */
    int moving;
    int dead;
} hero_t;

extern hero_t hero;
extern int boss_dead;
extern int hero_god;                    /* el piloto de KEEP_STATS no pierde vida */
extern int hero_event;                  /* lo que ha pisado el héroe en este frame */
extern int level_up_t;                  /* frames del letrero del nivel nuevo */
enum { EV_NONE, EV_STAIRS };

void hero_reset(void);
void hero_place(int px, int py);
void hero_update(int in);               /* IN_... de game.c */
void hero_damage(int halves, int from_x, int from_y);
int hero_feet_x(void);                  /* el centro de los pies, en píxeles */
int hero_feet_y(void);
int hero_damage_points(void);
void actors_clear(void);
void actors_room(int rx, int ry);       /* los de la sala, al entrar */
void actors_update(void);
void actors_draw(int cam_x, int cam_y);
void hero_draw(int cam_x, int cam_y);
int actor_spawn(int type, int px, int py);
int boss_alive(void);
int boss_hp(void);

/* --- sound.c ------------------------------------------------------------------ */

enum { SONG_NONE, SONG_TITLE, SONG_TOWER, SONG_BOSS, SONG_WIN, SONG_OVER };
enum {                                  /* los efectos de keep.afb */
    FX_SWING, FX_HIT, FX_KILL, FX_HURT, FX_COIN, FX_ITEM, FX_DOOR, FX_STAIRS,
    FX_ARROW, FX_CAST, FX_LEVEL, FX_POTION, FX_TALK, FX_BOSS_HIT, FX_CUT, FX_SPIKES,
};

void sound_init(void);
void sound_frame(void);
void sound_song(int song);
int sound_song_now(void);
void sound_fx(int fx);
void sound_mute(int on);                /* la pausa */
void sound_music(int on);               /* la música sí o no (la tecla S) */
int sound_music_on(void);
void sound_off(void);

/* --- game.c ------------------------------------------------------------------- */

enum { IN_UP = 1, IN_DOWN = 2, IN_LEFT = 4, IN_RIGHT = 8, IN_ATTACK = 16, IN_POTION = 32,
       IN_PAUSE = 64, IN_ESC = 128, IN_MUSIC = 256 };

extern uint32_t rng_state;
static inline uint32_t rnd(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state >> 16;
}

extern const int8_t sin64[64];          /* seno x 64, 64 pasos por vuelta */
static inline int isin(int a) { return sin64[a & 63]; }
static inline int icos(int a) { return sin64[(a + 16) & 63]; }

extern int lang;                        /* 0 español, 1 inglés */
#define T(es, en) (lang ? (en) : (es))

extern int cur_floor, room_x, room_y;
extern uint32_t frame;

void message(const char *l1, const char *l2, const char *l3);   /* un mensaje, hasta pulsar */
enum { HINT_DOOR, HINT_BOSS_DOOR, HINT_SPIKES, HINT_BRAMBLE };
void hint(int which);                   /* el mensaje de lo que no deja pasar, una vez */
void pickup(int mark);                  /* coge el objeto de esa marca */
void bot_dirty(void);                   /* algo ha cambiado: el piloto vuelve a mirar el camino */

#endif /* KEEP_H */
