/*
 * sndregs.h - Los registros de sonido de ZX-V como los recibe el
 * sintetizador (sndsynth.h): los números de registro y un tramo de sonido
 * (snd_span_t), el estado al empezar, el beeper de cada muestra y las
 * escrituras con su hora. La CPU de la máquina los manda así a la ULA, y el
 * emulador y zxv-tracker los dan igual.
 *
 * MIT, como el resto del SDK.
 */
#ifndef SNDREGS_H
#define SNDREGS_H

#include <stdbool.h>
#include <stdint.h>

/* Los registros de sonido: los 32 de los dos AY (16 cada uno) y estos
 * cuatro... */
#define SND_REG_STEREO  32
#define SND_REG_VOLUME  33
#define SND_REG_AFLAGS  34
#define SND_REG_SMPVOL  35              /* el volumen de los canales de muestras */
#define SND_REGS        36
#define SND_STATE       37              /* el estado: los registros y un cero */

/* ... y los de los canales de muestras (smpsynth.h), desde el 64: 8 por
 * canal. */
#define SND_REG_SMP     64
#define SND_SMP_CHANNELS 12
#define SND_SMP_CH_REGS 8
#define SND_SMP_REGS    (SND_SMP_CHANNELS * SND_SMP_CH_REGS)
#define SND_ALL         (SND_REG_SMP + SND_SMP_REGS)    /* todos los números de registro */

/* ... y dos que no son estado, solo escrituras: arrancar varios canales de
 * muestras en la misma muestra (spec §5.6). SMPKEY_HI guarda los canales
 * 8-11 (bits 0-3), y SMPKEY arranca a la vez los 0-7 (bits 0-7) y los
 * guardados. Van siempre juntos, en la misma muestra del mismo tramo: el
 * sintetizador no guarda nada de un tramo a otro. */
#define SND_REG_SMPKEY_HI 36
#define SND_REG_SMPKEY  37

/* Si reg es un registro (AY_STEREO y los demás, o un canal de muestras). */
static inline bool snd_reg_valid(unsigned reg)
{
    return reg < SND_REGS || reg == SND_REG_SMPKEY_HI || reg == SND_REG_SMPKEY ||
           (reg >= SND_REG_SMP && reg < SND_ALL);
}

/* Lo que se queda de una escritura en el estado: de CTRL de un canal, solo
 * sus bits que duran (sin disparar ni parar). */
static inline uint8_t snd_persist(unsigned reg, uint8_t val)
{
    return reg >= SND_REG_SMP && (reg - SND_REG_SMP) % SND_SMP_CH_REGS == 3 ? (uint8_t)(val & 0x0C) : val;
}

/* Una escritura: número de la muestra antes de la que va (bits 0-15, los de
 * abajo del número entero), registro (16-23) y valor (24-31). */
#define SND_EVENT(sample, reg, val) \
    ((uint32_t)(uint16_t)(sample) | (uint32_t)(uint8_t)(reg) << 16 | (uint32_t)(uint8_t)(val) << 24)

/* Un tramo de sonido: el estado al empezar, el beeper de cada muestra y las
 * escrituras, en orden. */
typedef struct {
    const uint8_t *state;               /* SND_STATE bytes */
    const uint16_t *beeper;
    unsigned samples;
    const uint32_t *ay;                 /* SND_EVENT */
    unsigned events;
    uint32_t first;                     /* el número de la primera muestra */
    const uint8_t *smp;                 /* el estado de un canal de muestras (8 bytes), o NULL */
    unsigned smp_ch;                    /* cuál */
} snd_span_t;

#endif /* SNDREGS_H */
