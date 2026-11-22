/*
 * sndsynth.h - El sonido de ZX-V (spec §5): los dos AY, los canales de
 * muestras y el beeper a 48 kHz en estéreo, con aysynth y smpsynth, a partir
 * de tramos de sonido (snd_span_t, sndregs.h): el estado de los registros al
 * empezar, el nivel del beeper en cada muestra y las escrituras, cada una
 * antes de su muestra.
 *
 * Es el código que suena en la máquina (la Pico ULA, con lo que le manda la
 * CPU), en el emulador y en zxv-tracker: lo que se oye en uno es lo que
 * suena en los otros.
 *
 * Con el estado de cada tramo, el sintetizador se pone al día solo tras
 * perder uno: solo se escriben los registros que no cuadran, así que, si no
 * se ha perdido nada, no cambia nada (y la envolvente no vuelve a empezar).
 *
 * MIT, como el resto del SDK.
 */
#ifndef SNDSYNTH_H
#define SNDSYNTH_H

#include <stdint.h>

#include "aysynth.h"
#include "smpsynth.h"
#include "sndregs.h"

typedef struct {
    aysynth_t ay[2];
    smpsynth_t smp;                     /* con su memoria de sonido (smp.mem), o sin ella: NULL */
    aymix_t mix;
    uint8_t stereo, volume, aflags, smpvol;
    uint8_t smpkey_hi;                  /* SMPKEY_HI, hasta el SMPKEY de su tramo (sndregs.h) */
    int32_t beep;                       /* el nivel del beeper, 0..65536 */
    snd_span_t f;                       /* el tramo en curso */
    unsigned pos, ev;                   /* su muestra siguiente y su escritura siguiente */
} sndsynth_t;

/* Todo en silencio, sin tramo. La memoria de sonido de los canales de
 * muestras sigue siendo la que era (smp.mem; sin ella, no suenan). */
void sndsynth_reset(sndsynth_t *s);

/* La memoria de sonido (SMP_SOUND_MEM bytes): la del dueño. */
static inline void sndsynth_memory(sndsynth_t *s, uint8_t *mem) { s->smp.mem = mem; }

/* Empieza un tramo (lo que apunta f tiene que durar hasta acabarlo) en su
 * muestra offset: las escrituras de antes se aplican al llegar a ella. Con
 * f NULL, un tramo sin nada: los AY siguen como estaban y el beeper en su
 * nivel (cuando se ha perdido uno). */
void sndsynth_begin(sndsynth_t *s, const snd_span_t *f, unsigned offset);

/* Las n muestras siguientes (izquierda en los bits 0-15). Pasado el final
 * del tramo, siguen como con NULL. */
void sndsynth_render(sndsynth_t *s, unsigned n, uint32_t *out);

/* Los registros como van en el estado de un tramo (SND_STATE). */
void sndsynth_state(const sndsynth_t *s, uint8_t state[SND_STATE]);

/* Una muestra estéreo para una salida PWM: los niveles de los dos canales de
 * un bloque que cuenta de 0 a top, como los quiere su registro CC (la
 * izquierda, el canal A, en los bits 0-15). El silencio queda en la mitad. */
static inline uint32_t sndsynth_pwm(uint32_t sample, uint32_t top)
{
    uint32_t l = (uint32_t)((int16_t)sample + 32768) * (top + 1) >> 16;
    uint32_t r = (uint32_t)((int16_t)(sample >> 16) + 32768) * (top + 1) >> 16;
    return l | r << 16;
}

#endif /* SNDSYNTH_H */
