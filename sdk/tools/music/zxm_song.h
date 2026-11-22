/*
 * zxm_song.h - Una canción .zxm en memoria (D98, sdk/music/zxm_format.h):
 * la escriben los conversores y zxv-tracker, y zxm_save la pasa a bytes.
 * Licencia MIT, como el resto del SDK.
 */
#ifndef ZXM_SONG_H
#define ZXM_SONG_H

#include <stddef.h>
#include <stdint.h>

#include "zxm_format.h"

#define ZXMS_TRACKS     18
#define ZXMS_POSITIONS  256

/* Una casilla: has dice qué campos tiene (ZXM_C_*). */
typedef struct {
    uint8_t has;
    uint8_t note, ins, vol, orn, env;
    uint16_t env_period;
    uint8_t fx[2], fxp[2][3];
} zxms_cell_t;

typedef struct {
    uint8_t kind;               /* 0 del AY, 1 de muestras */
    int rows;
    zxms_cell_t *cells;         /* rows casillas */
} zxms_pattern_t;

typedef struct {
    int rows;                   /* 1-256 */
    int pat[ZXMS_TRACKS];       /* el patrón de cada pista, o -1 */
} zxms_position_t;

/* Una muestra: lo de su entrada en la tabla (ZXM_S_*) y sus datos, como
 * los quiere el chip (8 bits con signo, o ADPCM: smp_adpcm_encode). */
typedef struct {
    uint32_t len, loop;         /* en muestras; loop >= len: sin bucle */
    uint8_t format;             /* 0 8 bits, 1 ADPCM */
    int8_t relnote, fine;
    uint8_t vol, pan;
    uint8_t *data;
    uint32_t size;              /* bytes de data */
    uint8_t flags;              /* ZXM_SF_*: ZXM_SF_PAN si cuenta su panorama */
} zxms_sample_t;

/* Un bloque de bytes tal cual (un instrumento del AY o un ornamento). */
typedef struct {
    uint8_t *data;
    int size;
} zxms_blob_t;

typedef struct {
    char title[33], author[33];
    int ay_tracks, smp_tracks;
    int speed, tempo, flags, ay_table, smp_volume, global_volume;
    int ay2;                    /* el segundo AY: su tabla y su PT3 (ZXM_H_AY2); -1, como el primero */
    uint8_t pan[12];
    int npos, loop;
    zxms_position_t pos[ZXMS_POSITIONS];
    int npat;
    zxms_pattern_t *pat;
    int n_ayins, n_orns;
    zxms_blob_t ayins[63], orns[64];
    int n_smpins, n_samples;
    uint8_t (*smpins)[ZXM_SINS_SIZE];   /* n_smpins x ZXM_SINS_SIZE bytes (ZXM_SI_*) */
    zxms_sample_t *samples;
} zxms_t;

/* Añade un instrumento de muestras (todas las notas con la muestra smp, sin
 * envolventes) y da su número (1-), o -1. */
int zxms_add_smp_instrument(zxms_t *s, int smp);
/* Añade una muestra (copia data) y da su número (0-), o -1. */
int zxms_add_sample(zxms_t *s, const zxms_sample_t *smp);

/* Una canción vacía, con lo de por defecto (velocidad 6, tempo 125). */
void zxms_init(zxms_t *s);
void zxms_free(zxms_t *s);

/* Añade un patrón de rows filas vacías y da su número. */
int zxms_add_pattern(zxms_t *s, int kind, int rows);
/* Quita los patrones repetidos (y los que no usa nadie). */
void zxms_dedup(zxms_t *s);

/* Los bytes del .zxm (malloc; los libera quien llama), o NULL. */
uint8_t *zxms_save(const zxms_t *s, size_t *size);

/* Al revés: un .zxm (m, n bytes) a una canción (s, sin iniciar), para
 * editarla (zxv-tracker). Cada patrón tiene las filas de la posición más
 * larga que lo usa. 0, o -1 si no es un .zxm o está dañado; en los dos
 * casos, s se libera con zxms_free. Guardarla da un .zxm que suena igual
 * (y, si lo escribió zxms_save, los mismos bytes). */
int zxms_load(zxms_t *s, const uint8_t *m, size_t n);

/* Lo que no se ha podido pasar: el conversor lo dice con esta función
 * (puede ser NULL), una vez por cosa. */
typedef void (*zxms_warn_fn)(void *ctx, const char *msg);

/* Un PT3 o un TurboSound a una canción (s, sin iniciar): 0, o -1 si no es un
 * PT3 o no se puede convertir (y lo dice warn). En los dos casos, s se libera
 * con zxms_free. */
int zxms_from_pt3(zxms_t *s, const uint8_t *m, size_t n, zxms_warn_fn warn, void *ctx);

/* Los módulos de muestras (MOD, S3M y XM) a .zxm (zxm_conv.c), sin memoria
 * dinámica y sin tener el fichero entero en la memoria, como los usa la ROM:
 *
 *   zxm_conv_kind     si un fichero de size bytes que empieza con head (n
 *                     bytes; con ZXM_CONV_HEAD bastan) es uno de ellos: 1
 *                     MOD, 2 S3M, 3 XM, o 0;
 *   zxm_conv_open     lo abre en cv (zxm_conv_size bytes, alineado a 8): rd
 *                     lee n bytes del fichero desde at (da cuántos, o menos
 *                     de 0 si falla), por cache (cache_cap bytes, 1.084 o
 *                     más). Dice con warn lo que no podrá pasar. 0, o -1 si
 *                     no es un módulo que se pueda convertir;
 *   zxm_conv_cache    cambia la caché (otra, más pequeña: la ROM usa toda
 *                     la memoria libre para leer los patrones, y luego deja
 *                     sitio a la canción);
 *   zxm_conv_open_mem lo mismo, con el fichero entero en m (n bytes);
 *   zxm_conv_song     la canción sin los datos de las muestras
 *                     (ZXM_O_DATA_SIZE 0), en out (cap bytes): lo que ocupa
 *                     (con out NULL o sin sitio, solo lo cuenta), o 0;
 *   zxm_conv_samples  los datos de las muestras, a trozos: put los entrega
 *                     desde el principio de los datos (at), en orden y en
 *                     múltiplos de 4 bytes (0, o menos de 0 si falla); buf
 *                     (cap bytes, 64 o más) es para juntarlos. 0, o el error;
 *   zxm_conv_whole    la canción con los datos detrás (lo que guarda
 *                     zxv-music), como zxm_conv_song.
 */
typedef struct zxm_conv zxm_conv_t;
typedef int (*zxm_read_fn)(void *ctx, uint32_t at, uint8_t *buf, uint32_t n);
typedef int (*zxm_put_fn)(void *ctx, uint32_t at, const uint8_t *buf, uint32_t n);
#define ZXM_CONV_HEAD 1084
size_t zxm_conv_size(void);
int zxm_conv_kind(const uint8_t *head, uint32_t n, uint32_t size);
int zxm_conv_open(zxm_conv_t *cv, zxm_read_fn rd, void *ctx, uint32_t size, uint8_t *cache, uint32_t cache_cap,
                  zxms_warn_fn warn, void *wctx);
void zxm_conv_cache(zxm_conv_t *cv, uint8_t *cache, uint32_t cache_cap);
/* Que use solo bytes de la memoria de sonido (lo demás es del programa):
 * vuelve a decidir cómo va cada muestra. */
void zxm_conv_limit(zxm_conv_t *cv, uint32_t bytes);
int zxm_conv_open_mem(zxm_conv_t *cv, const uint8_t *m, uint32_t n, zxms_warn_fn warn, void *wctx);
/* ... con los canales que van (el bit c, el canal c del módulo, desde 0;
 * hasta 12), en vez de que los elija: los de más de 12 canales. */
int zxm_conv_open_mem_ch(zxm_conv_t *cv, const uint8_t *m, uint32_t n, uint64_t channels, zxms_warn_fn warn,
                         void *wctx);
/* Los canales del módulo abierto (como zxm_module_channels). */
int zxm_conv_channels(const zxm_conv_t *cv, uint64_t *used, uint64_t *picked, uint32_t notes[32]);
uint32_t zxm_conv_song(zxm_conv_t *cv, uint8_t *out, uint32_t cap);
int zxm_conv_samples(zxm_conv_t *cv, zxm_put_fn put, void *ctx, uint8_t *buf, uint32_t cap);
uint32_t zxm_conv_whole(zxm_conv_t *cv, uint8_t *out, uint32_t cap);

/* Todo de una vez (zxm_song.c, con malloc): el .zxm de un módulo de
 * muestras en m (n bytes), con malloc (lo libera quien llama) y su tamaño en
 * *size, o NULL. */
uint8_t *zxm_from_module(const uint8_t *m, uint32_t n, size_t *size, zxms_warn_fn warn, void *ctx);
/* Lo mismo, con las muestras en smp_mem bytes de la memoria de sonido como
 * mucho (zxm_conv_limit: lo demás queda libre, para SAMPLE). */
uint8_t *zxm_from_module_mem(const uint8_t *m, uint32_t n, uint32_t smp_mem, size_t *size, zxms_warn_fn warn,
                             void *ctx);
/* ... y con los canales que van (zxm_conv_open_mem_ch; 0, los elige). */
uint8_t *zxm_from_module_ch(const uint8_t *m, uint32_t n, uint32_t smp_mem, uint64_t channels, size_t *size,
                            zxms_warn_fn warn, void *ctx);
/* Los canales de un módulo de muestras en m (n bytes), para elegir cuáles
 * van (zxm_from_module_ch): da cuántos tiene el fichero (0 si no se puede
 * convertir); en *used, los que tienen algo (los que pueden sonar); en
 * *picked, los que irían sin elegirlos; y en notes (32), las notas que toca
 * cada uno en la canción, tantas veces como suena su patrón (solo de un
 * módulo con más de 12 con algo: los demás van todos). */
int zxm_module_channels(const uint8_t *m, uint32_t n, uint64_t *used, uint64_t *picked, uint32_t notes[32]);

#endif /* ZXM_SONG_H */
