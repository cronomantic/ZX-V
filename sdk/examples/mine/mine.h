/*
 * mine.h - The Mine, un juego de cavar para ZX-V: lo que comparten sus partes.
 *
 *   cave.c   las reglas: lo que cae, lo que rueda, los bichos, el musgo, el
 *            tamiz, y el piloto automático de la demostración
 *   caves.c  las doce cuevas
 *   draw.c   la pantalla: doble buffer, texto, sprites
 *   art.c    los tiles y los sprites de la cueva, dibujados al empezar
 *   sound.c  la música (primer AY) y los efectos (segundo AY)
 *   game.c   la partida, el desplazamiento, el título y la demostración
 */
#ifndef MINA_H
#define MINA_H

#include "zxv_gfx.h"
#include "cave.h"

#define W           256
#define H           192
#define HUD_H       16                  /* el marcador, arriba */
#define VIEW_H      (H - HUD_H)         /* la ventana de la cueva: 256 x 176 */
#define CELL        16

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
uint32_t video_busy(int reset);         /* lo más que ha tardado un frame en dibujarse, en µs */

void text(int x, int y, const char *s, int c);
void text_center(int y, const char *s, int c);
int text_width(const char *s);
void label(int y, const char *s, int c);   /* centrado, sobre una franja negra */
void text_big(int x, int y, const char *s, int s_, const uint8_t *row_colours);
void number(char *out, uint32_t v, int digits);

sprite_t sprite_from(const zxv_bitmap *b, int x, int y, int w, int h);
sprite_t sprite_ascii(const char *const *rows, int w, int h);
void draw_sprite(const sprite_t *s, int x, int y, int flags, const uint8_t *colours);

/* --- art.c ------------------------------------------------------------------- */

/* Los tiles del mapa. */
enum {
    T_EMPTY, T_DIRT, T_WALL, T_STEEL, T_BOULDER, T_DIAMOND, T_EXIT = T_DIAMOND + 4,
    T_EXIT_OPEN, T_ENTRY = T_EXIT_OPEN + 2, T_MOSS = T_ENTRY + 2, T_SIEVE = T_MOSS + 2,
    T_SIEVE_ON, T_BOOM = T_SIEVE_ON + 2, T_FLASH = T_BOOM + 4, T_EMBER, T_BAT = T_EMBER + 4,
    TILES = T_BAT + 4
};

/* Los dibujos del minero. */
enum { M_STAND, M_BLINK, M_WALK1, M_WALK2, M_PUSH, M_UP1, M_UP2, M_CHEER, MINER_FRAMES };

extern const zxv_tilemap cave_map;      /* la cueva, en celdas de 16 x 16 */
extern uint16_t view[CW * CH];
extern sprite_t s_tile[TILES], s_miner[MINER_FRAMES], s_icon[3];

void art_init(void);
void art_cave(const cave_t *c);         /* los colores de la cueva */
void draw_tiles(const uint16_t *cells, int w, int h, int x, int y);

/* --- sound.c ------------------------------------------------------------------ */

enum { SONG_NONE, SONG_TITLE, SONG_CLEAR, SONG_OVER, SONG_WIN };
enum { FX_DIG, FX_DIAMOND, FX_LAND, FX_TING, FX_BOOM, FX_OPEN, FX_PUSH, FX_SIEVE, FX_TICK,
       FX_BONUS, FX_BORN, FX_MAGIC, FX_LIFE };

void sound_init(void);
void sound_frame(void);
void sound_song(int song);
void sound_fx(int fx);
void sound_mute(int on);
void sound_off(void);

/* --- game.c ------------------------------------------------------------------- */

extern uint32_t rng_state;
static inline uint32_t rnd(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state >> 16;
}

/* El idioma de la máquina: 0 español, 1 inglés. */
extern int lang;
#define T(es, en) (lang ? (en) : (es))

#endif /* MINA_H */
