/*
 * aysynth.h - El AY-3-8912 y la mezcla de la spec §5, en enteros, para las
 * Pico (el Hazard3 no tiene coma flotante): el sonido de la Pico ULA
 * (sndsynth.c).
 *
 * Lo usan también el emulador y zxv-tracker. Es el algoritmo del modelo de
 * referencia de la spec (con float, en las pruebas del emulador): los generadores
 * avanzan a reloj/8 (unos 221,7 kHz) y cada muestra de 48 kHz es la media de
 * los pasos que caen en ella. La prueba ays-vs-emulador comprueba que dan lo
 * mismo.
 *
 * MIT, como el resto del SDK.
 */
#ifndef AYSYNTH_H
#define AYSYNTH_H

#include <stdbool.h>
#include <stdint.h>

#define AYS_RATE        48000
#define AYS_CLOCK_X2    3546895u        /* 2 x 1.773.447,5 Hz */

typedef struct {
    uint8_t regs[16];
    uint16_t tone_count[3];
    uint8_t tone_out[3];
    uint16_t noise_count;
    uint32_t noise_lfsr;
    uint8_t noise_out;
    uint32_t env_count;
    uint8_t env_pos, env_attack, env_holding, env_level;
    uint8_t prescale;
    uint32_t tick_acc;
    /* Los registros ya decodificados (se recalculan en cada escritura). */
    uint32_t tp[3], np, ep;             /* periodos, al menos 1 */
    uint8_t tmask, nmask, envbits;      /* bits 0-2: canales A, B y C */
    uint16_t amp[3];                    /* nivel fijo de cada canal */
} aysynth_t;

/* El filtro de continua de la salida (como el condensador de acoplo). */
typedef struct {
    int32_t dc_in[2];
    int32_t dc_out[2];                  /* con 8 bits de fracción */
} aymix_t;

void ays_reset(aysynth_t *a);
void ays_write(aysynth_t *a, unsigned reg, uint8_t val);   /* R13 reinicia la envolvente */

/* Los bits que tiene cada registro (0-15): lo que guarda ays_write. */
uint8_t ays_mask(unsigned reg);

/* Avanza una muestra y deja el nivel medio de cada canal (0..32767). */
void ays_step(aysynth_t *a, int32_t ch[3]);

/* Mezcla una muestra estéreo (spec §5.5). ch2: el segundo AY, o NULL.
 * beeper: nivel medio del beeper en la muestra, 0..65536 (0..2: directo y
 * cola). stereo: 0 mono, 1 ABC, 2 ACB. */
void aymix_reset(aymix_t *m);
void aymix_sample(aymix_t *m, const int32_t ch[3], const int32_t *ch2, int32_t beeper,
                  unsigned stereo, unsigned volume, bool mute, int16_t out[2]);

/* Lo mismo con algo más en cada lado (los canales de muestras), ya en la
 * escala de la mezcla (un canal del AY a tope son 13.763). */
void aymix_sample_x(aymix_t *m, const int32_t ch[3], const int32_t *ch2, int32_t beeper,
                    unsigned stereo, unsigned volume, bool mute, int32_t xl, int32_t xr, int16_t out[2]);

#endif /* AYSYNTH_H */
