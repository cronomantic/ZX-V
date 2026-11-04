/*
 * gonesilent.h - Gone Silent, una aventura en 3D sólido de polígonos
 * rellenos para ZX-V: lo que comparten sus partes.
 *
 *   video.c   la pantalla en modo 0 (256 x 192) o 1 (512 x 384), con doble
 *             buffer; los colores por línea (la vista y el marcador) y el texto
 *   render.c  la vista: las piezas convexas, su orden, el recorte contra la
 *             pirámide de la vista y el relleno con tramas
 *   world.c   las zonas (el exterior y las instalaciones de la colonia),
 *             los objetos, las marcas y los registros
 *   main.c    el jugador, las colisiones, las puertas, la energía del traje,
 *             el disparo, las criaturas, las defensas, los objetos, los
 *             registros, la atmósfera tóxica, las tormentas y la medida de
 *             tiempos
 *   sound.c   la música y los efectos (los hace mksound.py)
 *
 * Las posiciones van en centímetros, en enteros: x hacia el este, y hacia
 * arriba, z hacia el norte. Los ángulos, en 1/1024 de vuelta: 0 mira al norte
 * y 256, al este.
 */
#ifndef GONESILENT_H
#define GONESILENT_H

#include "zxv_gfx.h"

#define TURN        1024
#define Q14         16384               /* el 1 de los senos y las normales */

extern const int16_t sin_q[257];        /* un cuarto de vuelta, x 16384 */
static inline int32_t isin(int a)
{
    a &= TURN - 1;
    if (a < 256) return sin_q[a];
    if (a < 512) return sin_q[512 - a];
    if (a < 768) return -sin_q[a - 512];
    return -sin_q[TURN - a];
}
static inline int32_t icos(int a) { return isin(a + TURN / 4); }

extern int lang;                        /* 0 español, 1 inglés */
#define T(es, en) (lang ? (en) : (es))

/* --- video.c ------------------------------------------------------------------ */

extern int scr_w, scr_h;                /* 256 x 192 o 512 x 384 */
extern int view_h;                      /* la vista en 3D; debajo, el marcador */
extern int hires;                       /* 1 en el modo 1 */

void video_init(int mode);
void video_begin(void);                 /* elige la página oculta */
void video_end(void);                   /* la enseña en el siguiente VBLANK y lo espera */
void video_done(void);                  /* vuelve al modo 0, la página 0 y los colores de siempre */
uint32_t video_lost(void);              /* frames perdidos (el dibujo no cupo en uno) */
uint8_t *video_page(void);              /* la página de dibujo */
void video_palette(const uint8_t (*rgb)[3]);    /* los 16 colores de la vista */

/* El texto, de 5 x 7 (de 10 x 14 en el modo 1), con los colores de siempre. */
void text(int x, int y, const char *s, int c);
void text_size(int x, int y, const char *s, int c, int z);     /* con puntos de z x z */
void rect(int x, int y, int w, int h, int c);
void line(int x0, int y0, int x1, int y1, int c);   /* recortada a la vista */
void number(char *out, uint32_t v, int digits);

/* --- render.c ----------------------------------------------------------------- */

/* Las formas de las piezas, en cuartos de su caja: un vértice (4, 4, 4) es la
 * esquina de enfrente de la del origen. */
enum { SH_BOX, SH_PYRAMID, SH_WEDGE, SH_PRISM, SH_OCTA, SHAPES };

/* Los materiales: una rampa de tonos de la paleta de la vista; la luz elige
 * el tono, y entre dos tonos van tramas del 25, el 50 y el 75 %. Solo los
 * avisos (MAT_HAZARD) llevan dibujo: rayas del tono y otro color. */
enum { MAT_ROCK, MAT_SAND, MAT_METAL, MAT_PANEL, MAT_GRATE, MAT_ORANGE, MAT_HAZARD,
       MAT_CYAN, MAT_DOOR, MAT_GLOW, MAT_DARK, MAT_EYE, MAT_ALARM, MATS };

#define OBJ_SOLID   0x01                /* choca con el jugador */
#define OBJ_DOOR    0x02                /* lleva a otra zona */
#define OBJ_MOVES   0x04                /* se mueve y gira: se recalcula en cada frame */
#define OBJ_HIDDEN  0x08                /* no está (una criatura destruida) */
#define OBJ_USE     0x10                /* se activa (los mandos de las defensas) */
#define OBJ_CHARGE  0x20                /* recarga el traje a quien se pone encima */
#define OBJ_TURRET  0x40                /* una torreta de las defensas */
#define OBJ_CREATURE 0x80               /* una criatura */
#define OBJ_ITEM    0x100               /* un objeto: se coge al pasar por encima */
#define OBJ_LOG     0x200               /* un registro: se lee con Intro */
#define OBJ_SPRITE  0x400               /* se dibuja como un sprite (spr), no con sus caras */
#define OBJ_SPIT    0x800               /* lo que escupe la incubadora */
#define OBJ_LAMP    0x1000              /* una luz: su clase (L_...) en log */
#define OBJ_DECO    0x2000              /* el mobiliario: no sale en el mapa */
#define OBJ_ROLL    0x4000              /* con OBJ_MOVES: gira de lado, en el plano x-y, no alrededor de y */

/* Las clases de luces (main.c, lamp_update). */
enum { L_CEIL, L_FLICKER, L_BEACON, L_WARN, L_SCREEN, L_WHEEL };

#define FACES_MAX   8
#define VERTS_MAX   8

/* need y give son marcas del juego (F_...): lo que hace falta para cruzar
 * una puerta o usar algo, y lo que se consigue al coger un objeto, usar algo
 * o leer un registro. */
typedef struct {
    uint8_t shape;
    uint16_t flags;
    int8_t target, entry;               /* las puertas: la zona y la entrada */
    int32_t x, y, z;                    /* la esquina de abajo, al suroeste */
    int32_t sx, sy, sz;                 /* el tamaño de la caja */
    uint8_t mat[FACES_MAX];             /* el de cada cara */
    uint16_t need, give;
    uint8_t log;                        /* los registros: cuál */
} piece_t;

typedef struct {
    piece_t p;
    int yaw;                            /* las que se mueven, alrededor de su centro (ver OBJ_ROLL) */
    int32_t v[VERTS_MAX][3];            /* los vértices, en el mundo */
    int16_t n[FACES_MAX][3];            /* las normales, hacia fuera, x 16384 */
    uint32_t pat[FACES_MAX][4];         /* las cuatro filas de la trama de cada cara */
    const uint8_t *spr;                 /* OBJ_SPRITE: el sprite (art_sprite) */
    uint8_t spr_flip, spr_flash;        /* en espejo; todo blanco (herido) */
    uint8_t spr_cm;                     /* cm por píxel, si no es SPRITE_CM */
} object_t;

/* Un sprite del .gfx: su ancho y su alto (16 bits) y sus filas de píxeles de
 * 4 bits (el primero, en los bits de abajo), sin alinear; el 8 es
 * transparente. Cada píxel mide SPRITE_CM cm (o spr_cm); el sprite se dibuja centrado
 * en el centro de la caja de su pieza, con su fila de abajo en el suelo. */
#define SPRITE_CM   4

typedef struct {
    int32_t x, y, z;                    /* los ojos */
    int yaw;
    int look;                           /* arriba (+) o abajo, en píxeles del modo 0 */
} camera_t;

/* Las tramas de 4 x 4: el bit 4 * fila + columna elige el segundo color. Las
 * del 25, el 50 y el 75 % son las de Bayer, para los tonos intermedios; la
 * diagonal, las rayas de los avisos. */
#define PAT_SOLID   0x0000
#define PAT_25      0x0505
#define PAT_50      0xA5A5
#define PAT_75      0xAFAF
#define PAT_DIAG    0x8421

/* Las franjas del fondo, desde el horizonte: hasta dist píxeles (del modo 0)
 * se pinta c0 con c1 en la trama pat. */
typedef struct { uint8_t dist, c0, c1; uint16_t pat; } band_t;

#define OBJECTS_MAX 120

extern object_t objects[OBJECTS_MAX];
extern int object_count;

extern struct render_stats {
    int objects, faces, clipped;
} stats;

extern int32_t view_far;                /* más lejos no se dibuja (con las tormentas, menos) */
extern int view_haze;                   /* con view_far menor, la bruma del cielo, de 0 a 3 */

void object_setup(object_t *o);         /* los vértices, las normales y las tramas */
void render_background(const camera_t *cam, const band_t *up, const band_t *down);
void render_view(const camera_t *cam);
/* Dónde se ve el punto (x, y, z) con la proyección del último fondo; 0 si
 * queda detrás de la cámara. */
int project(const camera_t *cam, int32_t x, int32_t y, int32_t z, int *sx, int *sy);

/* --- art.c -------------------------------------------------------------------- */

/* Los sprites de las criaturas (gonesilent.gfx, de mkcreature.py): el número
 * n, comprimido por mkspr.py (ver SPRITE_CM), o NULL. */
enum { SPR_WALK_A = 0, SPR_WALK_B = 5, SPR_REAR = 10, SPR_STRIKE = 15, SPR_HURT = 20, SPR_DEATH = 25,
       SPR_MOTHER = 30, SPR_MOTHER_SPIT = 32, SPR_MOTHER_DEATH = 33,
       SPR_CARD = 36, SPR_FILTER, SPR_CUTTER, SPR_PART1, SPR_PART2, SPR_PART3, SPR_PART4 };
void art_init(void);
const uint8_t *art_sprite(int n);
/* El sprite n en pequeño, en (x, y): de cada step x step píxeles, uno, con
 * los colores cambiados por remap (para el marcador). */
void art_icon(int n, int x, int y, int step, const uint8_t remap[16]);

/* --- sound.c ------------------------------------------------------------------ */

/* La música: la del título (y la introducción) y la del final. */
enum { SONG_NONE = -1, SONG_TITLE, SONG_END };

/* Los efectos, en el orden del banco gonesilent.afb. */
enum {
    FX_SHOT, FX_BITE, FX_TURRET, FX_KILL, FX_PICKUP, FX_USE, FX_LOG, FX_DENIED,
    FX_DOOR, FX_CHARGE, FX_LOW, FX_SIREN, FX_WIND, FX_WIND_STRONG, FX_HUM, FX_DEATH,
    FX_LOCK, FX_SCREECH, FX_PING_FAR, FX_PING_MID, FX_PING_NEAR, FX_GURGLE, FX_SPIT, FX_FLICKER
};

void sound_init(void);
void sound_song(int song);              /* SONG_...; no hace nada si ya suena */
void sound_fx(int fx);
void sound_ambient(int fx);             /* un efecto que se repite sin cortes; -1, ninguno */
void sound_frame(void);                 /* en cada frame */
void sound_done(void);

/* --- world.c ------------------------------------------------------------------ */

typedef struct { int32_t x, z; int yaw; int32_t feet; } entry_t;   /* feet: la altura del suelo */

typedef struct {
    const char *name_es, *name_en;
    const piece_t *pieces;
    int count;
    const uint8_t (*pal)[3];
    const band_t *up, *down;            /* el cielo (o el techo) y el suelo */
    const entry_t *entries;
    int outside;                        /* fuera, andar gasta energía y hay tormentas */
    int toxic;                          /* la atmósfera tóxica: lo que gasta por frame */
    uint16_t lit;                       /* a oscuras hasta tener estas marcas (0: con luz) */
} zone_t;

extern const zone_t zones[];
extern const int zone_count;
extern const int plant_zone;            /* la planta, que zumba mientras funciona */

/* Cómo caza cada criatura (en el campo log de su pieza). */
enum { CR_CHASE, CR_STALK, CR_AMBUSH, CR_MOTHER };

/* Las marcas del juego: lo que se lleva, lo que se sabe y lo que se ha hecho. */
enum {
    F_PART1 = 1 << 0, F_PART2 = 1 << 1, F_PART3 = 1 << 2, F_PART4 = 1 << 3,    /* las piezas del módulo */
    F_CARD = 1 << 4,                    /* la tarjeta de acceso */
    F_FILTER = 1 << 5,                  /* el filtro del traje */
    F_CUTTER = 1 << 6,                  /* el cortador */
    F_CODE = 1 << 7,                    /* el código de una puerta, leído en un registro */
    F_POWER = 1 << 8,                   /* la energía restablecida */
    F_DEFENCES_OFF = 1 << 9,            /* las defensas desactivadas */
    F_ESCAPED = 1 << 10,                /* el módulo reparado: el final */
    F_PUMP1 = 1 << 11, F_PUMP2 = 1 << 12, F_PUMP3 = 1 << 13,   /* las bombas de la planta, cerradas */
    F_PLANT_OFF = 1 << 14,              /* la planta parada: las criaturas se calman */
    F_TRUTH = 1 << 15,                  /* las pruebas del sabotaje, leídas en control */
    FLAGS = 16
};
#define F_PARTS     (F_PART1 | F_PART2 | F_PART3 | F_PART4)
#define F_PUMPS     (F_PUMP1 | F_PUMP2 | F_PUMP3)
#define F_STRONG_STORM  F_PART4         /* al conseguirla llega la tormenta fuerte */

/* De cada marca: su nombre en la lista de objetos (0 si no va), el aviso al
 * conseguirla y el de cuando hace falta. */
typedef struct {
    const char *name_es, *name_en, *got_es, *got_en, *need_es, *need_en;
} flag_info_t;
extern const flag_info_t flag_info[FLAGS];

/* Los registros: líneas separadas por '\n', de 40 letras como mucho. */
typedef struct { const char *es, *en; } log_text_t;
extern const log_text_t logs[];

/* La introducción, al empezar, y las partes del final: el despegue, la
 * planta parada o en marcha y con las pruebas o sin ellas. */
extern const log_text_t intro_text;
enum { END_LIFT, END_PLANT_OFF, END_PLANT_ON, END_TRUTH, END_NO_TRUTH, END_PARTS };
extern const log_text_t ending[END_PARTS];

#endif /* GONESILENT_H */
