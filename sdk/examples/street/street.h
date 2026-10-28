/*
 * street.h - Street Justice, un juego de peleas por las calles para ZX-V: lo
 * que comparten sus partes.
 *
 *   draw.c   la pantalla: doble buffer, texto, sprites y sombras
 *   art.c    los luchadores (un esqueleto en poses, dibujado con el motor) y
 *            los decorados de las tres fases
 *   sound.c  la música (primer AY) y los efectos (segundo AY)
 *   fight.c  los luchadores: el jugador, los enemigos, los golpes y la comida
 *   game.c   las fases, la cámara, el marcador, el título y la demostración
 */
#ifndef STREET_H
#define STREET_H

#include "zxv_gfx.h"

#define W           256
#define H           192
#define HUD_H       18                  /* el marcador, arriba */
#define FX          16                  /* las posiciones, en 1/16 de píxel */

#define SKY_Y       HUD_H               /* el fondo lejano */
#define SKY_H       64
#define FLOOR_Y     120                 /* el suelo */
#define FLOOR_H     (H - FLOOR_Y)
#define BAND_TOP    128                 /* por donde andan (los pies) */
#define BAND_BOTTOM 186
#define SLOT_W      64                  /* el ancho de cada fachada */
#define FACADE_H    (FLOOR_Y - HUD_H)

/* --- draw.c ------------------------------------------------------------------ */

typedef struct {
    const void *spr;
    uint8_t w, h;
} sprite_t;

void video_init(void);
void video_begin(void);                 /* elige la página oculta */
void video_end(void);                   /* la enseña en el siguiente VBLANK y lo espera */
void video_done(void);                  /* vuelve a la página 0 */
uint32_t video_lost(void);              /* frames perdidos (el dibujo no cupo en uno) */
void video_sync(void);                  /* después de un trabajo largo: no cuenta como perdido */
void video_pset(int x, int y, int c);   /* en la página, o en el bitmap de text_target */
void text_target(const zxv_bitmap *b);  /* el texto, en un bitmap (0: en la página) */

void text(int x, int y, const char *s, int c);
void text_center(int y, const char *s, int c);
int text_width(const char *s);
void label(int y, const char *s, int c);   /* centrado, sobre una franja negra */
void text_big(int x, int y, const char *s, int sz, const uint8_t *row_colours);
void number(char *out, uint32_t v, int digits);

/* Un lienzo transparente de w x h en memoria como destino del motor. */
void canvas(int w, int h);
int canvas_get(int x, int y);
void canvas_set(int x, int y, int c);
/* El rectángulo (x, y, w, h) del lienzo como sprite; vuelve a la página. */
sprite_t canvas_sprite(int x, int y, int w, int h);
/* El rectángulo con algo dibujado, en *x0, *y0, *w, *h (0 si está vacío). */
int canvas_bounds(int *x0, int *y0, int *w, int *h);
void canvas_end(void);

void draw_sprite(const sprite_t *s, int x, int y, int flags, const uint8_t *colours);
void shadow(int x, int y, int w);       /* una sombra plana en el suelo */
/* Un trazo grueso (los brazos, las piernas, las armas). */
void stroke(int x0, int y0, int x1, int y1, int w, int c);

/* --- art.c -------------------------------------------------------------------- */

/* Las poses de los luchadores. */
enum {
    P_IDLE, P_WALK1, P_WALK2, P_WALK3, P_WALK4, P_JAB, P_CROSS, P_UPPER, P_KICK, P_WINDUP,
    P_JUMP, P_JKICK, P_HURT, P_FALL, P_DOWN, P_GETUP, P_GRAB, P_KNEE, P_THROW, P_HELD,
    P_WIN, P_SPIN, POSES
};

/* Los cuerpos: normal y grueso. */
enum { BODY_NORMAL, BODY_FAT, BODIES };

/* Las cabezas (y lo que llevan encima). */
enum { HEAD_HERO, HEAD_PUNK, HEAD_CAP, HEAD_BALD, HEAD_BANDANA, HEAD_LONG, HEAD_BOSS, HEADS };

/* Una pose ya dibujada: el sprite, su esquina respecto a los pies, el centro
 * de la cabeza, la mano de delante y hacia dónde apunta el antebrazo. */
typedef struct {
    sprite_t s;
    int8_t ox, oy, hx, hy, kx, ky;
    uint8_t ka, lying;
} frame_t;

extern frame_t frames[BODIES][POSES];
extern sprite_t heads[HEADS][2];        /* derecha y tumbada */

void art_init(void);
/* El decorado de una fase: el fondo lejano, las fachadas y el suelo. */
void scenery_make(int stage);
void scenery_draw(int cam);             /* cam: el píxel del mundo a la izquierda */
extern const uint8_t *stage_slots;      /* las fachadas de la fase, de 64 en 64 */
extern int stage_nslots;

/* Lo que se rompe y lo que se come. */
enum { PROP_BIN, PROP_CRATE, PROP_BARREL, FOOD_APPLE, FOOD_BREAD, FOOD_CHICKEN, ITEMS };
extern sprite_t items[ITEMS];

/* --- sound.c ------------------------------------------------------------------ */

enum { SONG_NONE, SONG_TITLE, SONG_STREET, SONG_MARKET, SONG_ROOFS, SONG_BOSS, SONG_OVER, SONG_CLEAR };
enum { FX_SWING, FX_HIT, FX_HEAVY, FX_HURT, FX_EAT, FX_GO, FX_BREAK, FX_JUMP, FX_LIFE };

void sound_init(void);
void sound_frame(void);
void sound_song(int song);
void sound_fx(int fx);
void sound_mute(int on);
void sound_off(void);

/* --- fight.c ------------------------------------------------------------------ */

enum { IN_UP = 1, IN_DOWN = 2, IN_LEFT = 4, IN_RIGHT = 8, IN_A = 16, IN_B = 32, IN_PAUSE = 64, IN_ESC = 128 };

/* Lo que está haciendo un luchador. */
enum { ST_IDLE, ST_WALK, ST_ATTACK, ST_JUMP, ST_HURT, ST_FALL, ST_DOWN, ST_GETUP, ST_GRAB, ST_HELD,
       ST_THROWN, ST_DEAD, ST_WIN };

/* Los tipos de luchador. */
enum { K_HERO, K_PUNK, K_RUNNER, K_BRUTE, K_BOSS1, K_BOSS2, K_BOSS3, KINDS };

typedef struct {
    uint8_t alive, kind, variant, state, face_left, flash, attack, engaged;
    uint8_t kick, combo, buffered, knees;
    int16_t hp, maxhp, t, ai_t, cool, invuln, hits, hits_t, side, grab_t, retreat;
    int x, y, z, vx, vy, vz;            /* en 1/16 de píxel; y es la profundidad */
    uint32_t hitmask;                   /* a quién ha dado ya este golpe */
    int8_t held;                        /* a quién tiene agarrado, o quién le agarra */
} fighter_t;

#define MAX_F       8                   /* el jugador es el 0 */
#define MAX_ITEMS   8

typedef struct {
    uint8_t alive, kind, drop, flash;   /* drop: la comida que hay dentro */
    int x, y, z, vz;
} item_t;

extern fighter_t fighters[MAX_F];
extern item_t things[MAX_ITEMS];
extern int lives, demo, frame, stage, cam, cam_max, last_foe, last_foe_t;
extern uint32_t score;

void fight_reset(void);
void hero_spawn(int x, int y);
fighter_t *foe_spawn(int kind, int x, int y);
void item_spawn(int kind, int x, int y, int drop);
void fight_update(int input);           /* un frame de todos */
void fight_draw(void);
int foes_left(void);
int autopilot(int go);                  /* la entrada del piloto de la demostración */
const char *kind_name(int kind);
void add_score(int v);

#ifdef STREET_BOT
extern int bot_deaths, bot_hits, bot_boss_deaths;
#endif

/* --- game.c ------------------------------------------------------------------- */

extern uint32_t rng_state;
static inline uint32_t rnd(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state >> 16;
}

extern const int8_t sin64[64];          /* seno x 64, 64 pasos por vuelta */
static inline int isin(int a) { return sin64[a & 63]; }
static inline int icos(int a) { return sin64[(a + 16) & 63]; }
int isqrt(int v);

/* El idioma de la máquina: 0 español, 1 inglés. */
extern int lang;
#define T(es, en) (lang ? (en) : (es))

#endif /* STREET_H */
