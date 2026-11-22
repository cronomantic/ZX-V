/*
 * smpsynth.h - Los canales de muestras de ZX-V (spec §5.6): 12
 * canales que reproducen muestras de una memoria de 96 KB, cada uno con su
 * nota (el paso), su volumen y su panorama, como el Paula del Amiga. Los
 * sintetiza la Pico ULA, junto a los AY (sndsynth.c); el emulador suena con
 * lo mismo.
 *
 * La memoria de sonido: las muestras (SMP_MEM bytes) y detrás la tabla de
 * muestras (SMP_SLOTS entradas de SMP_SLOT_SIZE bytes, little endian):
 *
 *   +0  dónde empieza en la memoria (u24, en bytes)
 *   +3  formato: SMP_FMT_PCM8 (8 bits con signo) o SMP_FMT_ADPCM
 *   +4  cuántas muestras tiene (u32)
 *   +8  desde qué muestra vuelve a empezar al llegar al final (u32); si no es
 *       menor que la longitud, sin bucle: se para al acabar
 *
 * El ADPCM es el IMA, en bloques independientes de SMP_ADPCM_BLOCK muestras
 * (SMP_ADPCM_BYTES bytes): la primera, en la cabecera (i16 y el índice del
 * paso, u8, y un byte a 0), y las demás, de 4 bits (la de abajo primero). Así
 * se puede empezar en cualquier sitio. Suena como 16 bits y ocupa algo más de
 * la mitad que 8. Cada muestra se calcula desde la anterior, así que una nota
 * aguda cuesta más (las que se salta también se descodifican): en ADPCM el
 * paso no pasa de SMP_ADPCM_MAX_STEP (una octava más aguda que como se
 * grabó; lo que pase, cuenta como eso). En 8 bits, cualquier paso.
 *
 * Con interpolación, la última muestra va hacia la del principio del bucle
 * (o se queda en ella si no hay bucle): una onda corta en bucle suena
 * redonda.
 *
 * Los registros de cada canal (SMP_CH_REGS bytes; el canal c, desde
 * c * SMP_CH_REGS):
 *
 *   +0  SLOT    la muestra (0-255): cuenta al dispararla
 *   +1  VOL     volumen, 0-64
 *   +2  PAN     panorama: 0 izquierda, 128 centro, 255 derecha
 *   +3  CTRL    bit 0 (KEY): la dispara al escribirlo (desde el principio, o
 *               desde OFFSET); bit 1 (STOP): la para; bit 2 (NOINTERP): sin
 *               interpolar; bit 3 (DIRECT): modo directo. Se guardan los bits
 *               2 y 3
 *   +4  STEP    el paso por cada muestra de 48 kHz (u24, 8.16): 0x010000 es
 *               la muestra a 48 kHz. Da la nota
 *   +7  OFFSET  desde dónde se dispara, de 256 en 256 muestras (el 9xx de un
 *               MOD). En modo directo, lo que suena (i8): el DAC
 *
 * Para arrancar varios canales en la misma muestra (los de una fila de un
 * MOD), se escriben sus registros sin KEY y luego smp_key con todos (en la
 * línea del sonido, SMPKEY_HI y SMPKEY: sndregs.h).
 *
 * Sin nada de la Pico: se comprueba en el PC frente a un modelo de
 * referencia. MIT, como el resto del SDK.
 */
#ifndef SMPSYNTH_H
#define SMPSYNTH_H

#include <stdbool.h>
#include <stdint.h>

#define SMP_CHANNELS    12
#define SMP_MEM         0x18000u        /* 96 KB de muestras */
#define SMP_SLOTS       256
#define SMP_SLOT_SIZE   12
#define SMP_SOUND_MEM   (SMP_MEM + SMP_SLOTS * SMP_SLOT_SIZE)   /* con la tabla */

#define SMP_FMT_PCM8    0
#define SMP_FMT_ADPCM   1
#define SMP_ADPCM_BLOCK 64              /* muestras por bloque */
#define SMP_ADPCM_BYTES 36              /* 4 de cabecera y 63 de 4 bits (y uno sin usar) */
#define SMP_ADPCM_MAX_STEP 0x20000u     /* el paso más alto en ADPCM: 2,0 (lo que cabe en la ULA) */

#define SMP_CH_REGS     8
#define SMP_REGS        (SMP_CHANNELS * SMP_CH_REGS)
#define SMP_R_SLOT      0
#define SMP_R_VOL       1
#define SMP_R_PAN       2
#define SMP_R_CTRL      3
#define SMP_R_STEP      4               /* 3 bytes */
#define SMP_R_OFFSET    7

#define SMP_KEY         0x01
#define SMP_STOP        0x02
#define SMP_NOINTERP    0x04
#define SMP_DIRECT      0x08

typedef struct {
    uint8_t playing, fmt, interp, direct;
    uint32_t base;                      /* dónde empieza, en bytes */
    uint32_t len, loop;                 /* en muestras; loop >= len: sin bucle */
    uint32_t pos, frac;                 /* la muestra que suena y la fracción (16 bits) */
    uint32_t step;                      /* 8.16 */
    int32_t gl, gr;                     /* volumen y panorama: 0..256 cada uno */
    int32_t a, b;                       /* ADPCM: las muestras en pos y en pos + 1 */
    int32_t loopv;                      /* la muestra donde empieza el bucle (en 16 bits) */
    int32_t pred, index;                /* ADPCM: el descodificador... */
    uint32_t blk, di;                   /* ... dónde va: el bloque (en bytes) y la muestra de
                                           él que da después (0..SMP_ADPCM_BLOCK) */
} smp_channel_t;

typedef struct {
    uint8_t *mem;                       /* SMP_SOUND_MEM bytes: las muestras y la tabla */
    uint8_t regs[SMP_REGS];
    smp_channel_t ch[SMP_CHANNELS];
} smpsynth_t;

/* Todo parado, con la memoria de sonido en mem (que no se borra). */
void smp_reset(smpsynth_t *s, uint8_t *mem);

/* Una escritura en un registro (0..SMP_REGS-1). */
void smp_write(smpsynth_t *s, unsigned reg, uint8_t val);

/* Dispara a la vez los canales de mask (bit c, el canal c), como KEY en
 * CTRL de cada uno. */
void smp_key(smpsynth_t *s, uint32_t mask);

/* Suma n muestras de todos los canales a l y r (cada canal, hasta ±32767 *
 * 256 / 256 por volumen y panorama: la escala la pone la mezcla). */
void smp_mix(smpsynth_t *s, unsigned n, int32_t *l, int32_t *r);

/* El ADPCM de la tabla: codifica n muestras de 16 bits en bloques (out:
 * (n + SMP_ADPCM_BLOCK - 1) / SMP_ADPCM_BLOCK * SMP_ADPCM_BYTES bytes). Para
 * las herramientas y las pruebas. */
uint32_t smp_adpcm_encode(const int16_t *in, uint32_t n, uint8_t *out);
/* Un bloque (m muestras, hasta SMP_ADPCM_BLOCK), con el índice del paso
 * que sigue de un bloque al otro (empieza en 0): para codificar a trozos,
 * sin tener la muestra entera en 16 bits. Da SMP_ADPCM_BYTES. */
uint32_t smp_adpcm_block(const int16_t *in, uint32_t m, int32_t *index, uint8_t *out);

#endif /* SMPSYNTH_H */
