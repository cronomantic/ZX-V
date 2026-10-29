/*
 * estela.h - Estela, un matamarcianos para ZX-V: lo que comparten sus partes.
 *
 *   draw.c   la pantalla: doble buffer, texto, sprites, estrellas
 *   sound.c  la música (primer AY), los efectos (segundo AY) y las muestras
 *            digitalizadas (con zxv_ay_event, spec §5.4)
 *   game.c   las fases, los enemigos, los jefes, el título y la demostración
 */
#ifndef ESTELA_H
#define ESTELA_H

#include "zxv_gfx.h"

#define W           256
#define H           192
#define HUD_H       8                   /* la línea del marcador */
#define FX          96                  /* las posiciones, en 1/96 de píxel */

/* El tiempo: el juego va a ZXV_FRAME_HZ imágenes por segundo (60), pero se
 * ajustó cuando iba a 50, y sus velocidades siguen en aquella unidad: V(v)
 * es una velocidad de v/16 de píxel cada 1/50 de segundo, en FX por frame
 * (exacta a 50 y a 60 Hz con FX = 96). MS(ms), unos milisegundos en frames. */
#define V(v)        ((v) * (FX / 16) * 50 / ZXV_FRAME_HZ)
#define MS(ms)      ((ms) * ZXV_FRAME_HZ / 1000)

/* --- draw.c ------------------------------------------------------------------ */

/* Un sprite preparado (zxv_spr_prepare) y su tamaño. */
typedef struct {
    const void *spr;
    uint8_t w, h;
} sprite_t;

/* La página en la que se dibuja este frame (la otra se ve). */
void video_init(void);
void video_begin(void);                 /* elige la página oculta y la borra */
void video_end(void);                   /* la enseña en el siguiente VBLANK y lo espera */
void video_done(void);                  /* vuelve a la página 0 */
uint32_t video_frames(void);            /* frames perdidos (el dibujo no cupo en uno) */

void pset(int x, int y, int c);
void text(int x, int y, const char *s, int c);
void text_center(int y, const char *s, int c);
int text_width(const char *s);
void label(int y, const char *s, int c);   /* centrado, sobre una franja negra */
/* El texto grande del título: cada píxel de la fuente, de s x s. */
void text_big(int x, int y, const char *s, int s_, const uint8_t *row_colours);
void number(char *out, uint32_t v, int digits);

/* Un sprite dibujado con caracteres: '.' transparente, '0'-'9' y 'a'-'f' colores. */
sprite_t sprite_ascii(const char *const *rows, int w, int h);
/* Un sprite dibujado con el motor: canvas() deja un lienzo transparente de
 * w x h como destino y canvas_sprite() lo prepara. */
void canvas(int w, int h);
sprite_t canvas_sprite(void);

void draw_sprite(const sprite_t *s, int x, int y, int flags, const uint8_t *colours);
int sprites_touch(const sprite_t *a, int ax, int ay, const sprite_t *b, int bx, int by);

/* Las estrellas del fondo, en tres capas. */
void stars_init(void);
void stars_draw(int speed);             /* speed en FX por frame, la de la capa cercana */

/* --- sound.c ------------------------------------------------------------------ */

enum { SONG_NONE, SONG_TITLE, SONG_STAGE, SONG_BOSS, SONG_OVER, SONG_CLEAR };
enum { FX_SHOT, FX_HIT, FX_BOOM, FX_BIG, FX_POWER, FX_DEATH, FX_ALARM };

void sound_init(void);
void sound_frame(void);                 /* una vez por frame: música, efectos y muestras */
void sound_song(int song);
void sound_fx(int fx);
void sound_mute(int on);                /* la pausa */
void sound_off(void);                   /* al salir */

/* --- game.c ---------------------------------------------------------------------- */

extern uint32_t rng_state;
static inline uint32_t rnd(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state >> 16;
}

extern const int8_t sin64[64];          /* seno x 64, 64 pasos por vuelta */
static inline int isin(int a) { return sin64[a & 63]; }
static inline int icos(int a) { return sin64[(a + 16) & 63]; }
/* El seno de a/d pasos (a >= 0), entre dos de la tabla: para lo que gira unos
 * pasos por segundo, que no son un número entero por frame. */
static inline int isin_f(int a, int d)
{
    int i = a / d, f = a % d;
    return (isin(i) * (d - f) + isin(i + 1) * f) / d;
}

/* El idioma de la máquina: 0 español, 1 inglés. */
extern int lang;
#define T(es, en) (lang ? (en) : (es))

#endif /* ESTELA_H */
