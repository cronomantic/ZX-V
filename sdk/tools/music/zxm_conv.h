/*
 * zxm_conv.h - Lo de dentro de los conversores de módulos de muestras (MOD,
 * S3M y XM) a .zxm: zxm_conv.c tiene lo común (leer el fichero a trozos por
 * una caché, decidir cómo va cada muestra, escribir la canción y pasar las
 * muestras) y zxm_mod.c, zxm_s3m.c y zxm_xm.c, lo de cada formato. Sin
 * memoria dinámica: el estado (zxm_conv_t) lo da quien llama; así los usan
 * zxv-music y la ROM (LOAD "x" MUSIC), que no tiene el fichero entero en la
 * RAM.
 *
 * Licencia MIT, como el resto del SDK.
 */
#ifndef ZXM_CONV_H
#define ZXM_CONV_H

#include "zxm_song.h"

#define ZXC_TRACKS      12
#define ZXC_SAMPLES     255
#define ZXC_INS         128
#define ZXC_PATS        256
#define ZXC_ROWS        256
#define ZXC_BLOCKS      32
#define ZXC_DUP         0x80000000u     /* (en zxc_scan) */

/* Cómo están los datos de una muestra en el fichero. */
enum { ZXC_S8, ZXC_U8, ZXC_D8, ZXC_S16, ZXC_U16, ZXC_D16 };

/* Cómo va al .zxm. */
enum { ZXC_PCM8, ZXC_ADPCM, ZXC_HALF };

typedef struct {
    uint32_t file_at;                   /* sus datos, en el fichero */
    uint32_t frames;                    /* las muestras que tiene allí */
    uint32_t loop_start, loop_len;      /* el bucle, en muestras; loop_len 0: sin bucle */
    uint8_t enc, pingpong;              /* ZXC_S8...; el bucle, de ida y vuelta */
    uint8_t level;                      /* ZXC_PCM8, ZXC_ADPCM o ZXC_HALF */
    uint8_t used;                       /* suena en algún patrón */
    int8_t rel, fine;                   /* nota relativa y afinación (1/128 de semitono) */
    uint8_t vol, pan, flags;            /* 0-64, 0-255, ZXM_SF_* */
    int16_t top;                        /* lo más agudo que suena: (nota - 48 + rel) * 128 + fine */
    uint32_t at;                        /* sus datos, en el .zxm (desde ZXM_O_DATA) */
} zxc_smp_t;

/* Una casilla, ya en lo del .zxm. */
typedef struct {
    uint8_t has, note, ins, vol;
    uint8_t fx[2], fxp[2];
} zxc_cell_t;

enum { ZXC_MOD = 1, ZXC_S3M, ZXC_XM };

struct zxm_conv;
typedef void (*zxc_row_fn)(struct zxm_conv *cv, void *ctx, int row, const zxc_cell_t *cells);

struct zxm_conv {
    /* El fichero, por la caché (o entero en la memoria, sin rd). */
    zxm_read_fn rd;
    void *rd_ctx;
    uint32_t size;
    const uint8_t *mem;                 /* entero, si no hay rd */
    uint8_t *cache;                     /* en bloques (de 4 KB, o uno si es pequeña) */
    uint32_t cache_cap, blk, nblk, clock;
    uint32_t tag[ZXC_BLOCKS], age[ZXC_BLOCKS];  /* el bloque del fichero + 1 (0: vacío) y su último uso */
    uint32_t last;                      /* el último que sirvió */
    const uint8_t *fast;                /* lo que se lee sin buscar (zxc_b): el bloque de ahora */
    uint32_t fast_at, fast_n;
    int err;                            /* el error de rd */

    int kind;
    zxms_warn_fn warn;
    void *warn_ctx;
    uint32_t said;

    /* La canción. */
    char title[32];
    int channels;                       /* los del fichero */
    int tracks;                         /* las pistas del .zxm */
    uint8_t chan[ZXC_TRACKS];           /* el canal del fichero de cada pista */
    uint8_t pan[ZXC_TRACKS];
    uint8_t speed, tempo, flags, gvol;
    int npos, loop;
    uint8_t order[256];
    int npat;
    uint32_t pat_at[ZXC_PATS];          /* dónde empieza cada patrón (sus datos) */
    uint16_t pat_rows[ZXC_PATS];
    int nins;                           /* instrumentos de muestras (los del XM; 0: el instrumento es la muestra) */
    uint32_t ins_at[ZXC_INS];           /* el XM: dónde empieza cada instrumento */
    uint8_t ins_first[ZXC_INS], ins_n[ZXC_INS];
    int nsmp;
    zxc_smp_t smp[ZXC_SAMPLES];
    uint32_t data_size;
    uint32_t smp_mem;                   /* la memoria de sonido que puede usar (zxm_conv_limit) */
    uint64_t only;                      /* los canales que se quieren (0: los elige) */
    uint64_t ch_used;                   /* para elegirlos (zxm_module_channels): los que tienen algo, */
    uint32_t ch_notes[32];              /* ... sus notas en la canción (de más de 12) */
    int ch_n;                           /* ... y los del fichero */
    /* Los patrones de pista repetidos, una vez: el primero con el mismo
     * resumen (FNV-1a de sus bytes), o él mismo (patrón x pistas + pista).
     * Al escribirlos se comprueba que son iguales de verdad. */
    uint32_t pat_key[ZXC_PATS * ZXC_TRACKS];
    uint16_t pat_len[ZXC_PATS * ZXC_TRACKS];    /* lo que ocupa cada uno */
    uint32_t pat_bytes;                 /* lo que ocupan todos (sin los repetidos) */
    int collide;                        /* dos resúmenes iguales de pistas distintas */
    uint8_t ch_track[64];               /* la pista de cada canal del fichero (0xFF, ninguna) */
    int scanning;                       /* zxc_scan: las casillas dicen lo que suena */
    uint8_t ins_notes[ZXC_INS][12];     /* el XM: las notas que toca cada instrumento */
    uint8_t last_ins[64];               /* ... con el último instrumento de cada canal */
    unsigned how;                       /* ADPCM, a la mitad, fuera: para decirlo */

    /* Lo de cada formato: recorre el patrón p una vez, en orden, y da a fn
     * cada fila con algo, con la casilla de cada pista (cells[t]; has 0,
     * vacía). */
    void (*rows)(struct zxm_conv *cv, int p, zxc_row_fn fn, void *ctx);
    /* El formato (ZXC_MOD...). */
    uint32_t fmt_a, fmt_b;              /* lo que guarde cada uno */
};

/* El byte i del fichero (0 si no lo hay), y en little y big-endian. */
uint8_t zxc_b_slow(zxm_conv_t *cv, uint32_t i);
static inline uint8_t zxc_b(zxm_conv_t *cv, uint32_t i)
{
    uint32_t o = i - cv->fast_at;
    return o < cv->fast_n ? cv->fast[o] : zxc_b_slow(cv, i);
}
uint32_t zxc_le16(zxm_conv_t *cv, uint32_t i);
uint32_t zxc_le32(zxm_conv_t *cv, uint32_t i);
uint32_t zxc_be16(zxm_conv_t *cv, uint32_t i);

/* Lo que no se ha podido pasar, una vez cada cosa (bit, de ZXC_W_*). */
void zxc_warn(zxm_conv_t *cv, unsigned bit, const char *msg);
enum {
    ZXC_W_ADPCM = 1u << 0, ZXC_W_HALF = 1u << 1, ZXC_W_FIT = 1u << 2, ZXC_W_TRACKS = 1u << 3,
    ZXC_W_SAMPLES = 1u << 4,
    /* De cada formato, desde aquí. */
    ZXC_W_FMT = 1u << 8,
};

/* Cuántas veces suena el patrón p en la canción (sus posiciones). */
uint32_t zxc_plays(const zxm_conv_t *cv, int p);

/* Las pistas, de los canales con algo (el bit c de used, de los nch del
 * fichero): todos, si son 12 o menos; si no, los 12 que más notas tocan en
 * la canción (notes[c], contadas cada vez que suena su patrón; a igualdad,
 * el de delante), y lo dice. Con only (zxm_from_module_ch), solo esos.
 * Deja los canales en chan, en el orden del fichero, y da cuántos. */
int zxc_pick_tracks(zxm_conv_t *cv, uint64_t used, const uint32_t *notes, int nch, uint8_t *chan);

/* Pone una nota en las muestras que suenan (para el reparto y el techo del
 * ADPCM): la muestra s con la nota n (0-95). */
void zxc_note(zxm_conv_t *cv, int s, int n);

/* La nota relativa y la afinación de una muestra que suena a hz en el C-4. */
void zxc_c4hz(zxc_smp_t *s, uint32_t hz);

/* Los de cada formato: reconocerlo (con head, sus n primeros bytes, de un
 * fichero de size) y leer la cabecera, los patrones (con las casillas: las
 * muestras que suenan, cv->rows) y las muestras. 0 si lo es. */
int zxc_mod_probe(const uint8_t *head, uint32_t n, uint32_t size);
int zxc_mod_open(zxm_conv_t *cv);
int zxc_s3m_probe(const uint8_t *head, uint32_t n, uint32_t size);
int zxc_s3m_open(zxm_conv_t *cv);
int zxc_xm_probe(const uint8_t *head, uint32_t n, uint32_t size);
int zxc_xm_open(zxm_conv_t *cv);
/* El XM, tras zxc_scan: las muestras que suenan, con los mapas de notas
 * de los instrumentos (leídos una vez cada uno). */
void zxc_xm_resolve(zxm_conv_t *cv);
/* El instrumento i del XM, en out (ZXM_SINS_SIZE bytes). */
void zxc_xm_instrument(zxm_conv_t *cv, int i, uint8_t *out);

/* Las casillas de todos los patrones, una vez: lo que suena (zxc_note). */
void zxc_scan(zxm_conv_t *cv);

#endif /* ZXM_CONV_H */
