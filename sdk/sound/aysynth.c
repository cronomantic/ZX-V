/*
 * aysynth.c - El AY-3-8912 y la mezcla en enteros (ver aysynth.h).
 */
#include "aysynth.h"

#include <string.h>

/* Con AYS_IN_RAM (en una Pico que se ejecuta desde la flash) lo que se usa
 * en cada muestra va a la RAM: así no espera a la caché de la flash. */
#ifdef AYS_IN_RAM
#define AYS_FAST        __attribute__((section(".time_critical.aysynth")))
#define AYS_TABLE
#else
#define AYS_FAST
#define AYS_TABLE       const
#endif

/* El DAC del AY (el del modelo de referencia de las pruebas, por
 * 32767). */
static AYS_TABLE uint16_t ay_dac[16] = {
    0, 328, 475, 691, 1006, 1491, 2113, 3519,
    4148, 6717, 9575, 12215, 16138, 20816, 26397, 32767,
};

static const uint8_t reg_mask[16] = {
    0xFF, 0x0F, 0xFF, 0x0F, 0xFF, 0x0F, 0x1F, 0xFF,
    0x1F, 0x1F, 0x1F, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF,
};

#define TICKS_DIV   (2u * 8u * AYS_RATE)

static void env_restart(aysynth_t *a)
{
    a->env_count = 0;
    a->env_pos = 0;
    a->env_attack = (a->regs[13] & 0x04) != 0;
    a->env_holding = 0;
    a->env_level = a->env_attack ? 0 : 15;
}

static void decode(aysynth_t *a)
{
    const uint8_t *r = a->regs;
    for (int c = 0; c < 3; c++) {
        uint32_t tp = r[c * 2] | r[c * 2 + 1] << 8;
        a->tp[c] = tp ? tp : 1;
        a->amp[c] = ay_dac[r[8 + c] & 15];
    }
    a->np = r[6] ? r[6] : 1;
    uint32_t ep = (uint32_t)(r[11] | r[12] << 8);
    a->ep = ep ? ep : 1;
    a->tmask = r[7] & 7u;
    a->nmask = (r[7] >> 3) & 7u;
    a->envbits = (uint8_t)((r[8] >> 4 & 1) | (r[9] >> 3 & 2) | (r[10] >> 2 & 4));
}

void ays_reset(aysynth_t *a)
{
    memset(a, 0, sizeof(*a));
    a->regs[7] = 0x3F;
    a->noise_lfsr = 1;
    env_restart(a);
    decode(a);
}

void ays_write(aysynth_t *a, unsigned reg, uint8_t val)
{
    reg &= 15;
    a->regs[reg] = val & reg_mask[reg];
    if (reg == 13)
        env_restart(a);
    decode(a);
}

uint8_t ays_mask(unsigned reg) { return reg_mask[reg & 15]; }

AYS_FAST static void env_step(aysynth_t *a)
{
    if (a->env_holding)
        return;
    if (++a->env_pos < 16) {
        a->env_level = a->env_attack ? a->env_pos : (uint8_t)(15 - a->env_pos);
        return;
    }
    uint8_t shape = a->regs[13];
    int cont = shape & 0x08, alt = shape & 0x02, hold = shape & 0x01;
    if (!cont) {
        a->env_holding = 1;
        a->env_level = 0;
    } else if (hold) {
        a->env_holding = 1;
        a->env_level = (a->env_attack != (alt != 0)) ? 15 : 0;
    } else {
        if (alt)
            a->env_attack = !a->env_attack;
        a->env_pos = 0;
        a->env_level = a->env_attack ? 0 : 15;
    }
}

AYS_FAST void ays_step(aysynth_t *a, int32_t ch[3])
{
    /* Los pasos a reloj/8 de esta muestra: 4 o 5. */
    uint32_t acc = a->tick_acc + AYS_CLOCK_X2;
    unsigned n = 0;
    while (acc >= TICKS_DIV) {
        acc -= TICKS_DIV;
        n++;
    }
    a->tick_acc = acc;

    /* El estado, en variables locales durante la muestra (en la estructura,
     * el compilador no se atreve a dejarlo en registros). */
    uint32_t tp0 = a->tp[0], tp1 = a->tp[1], tp2 = a->tp[2], np = a->np, ep = a->ep;
    uint32_t tmask = a->tmask, nmask = a->nmask, envbits = a->envbits;
    int32_t ea = ay_dac[a->env_level];
    int32_t amp0 = envbits & 1 ? ea : a->amp[0], amp1 = envbits & 2 ? ea : a->amp[1],
            amp2 = envbits & 4 ? ea : a->amp[2];
    uint32_t c0 = a->tone_count[0], c1 = a->tone_count[1], c2 = a->tone_count[2];
    uint32_t tone = (uint32_t)a->tone_out[0] | (uint32_t)a->tone_out[1] << 1 | (uint32_t)a->tone_out[2] << 2;
    uint32_t noise = a->noise_out ? 7u : 0u;
    uint32_t pre = a->prescale;
    uint32_t nc = a->noise_count, lfsr = a->noise_lfsr, ec = a->env_count;
    int32_t s0 = 0, s1 = 0, s2 = 0;
    /* Cuántos pasos lleva encendido cada canal, un byte por canal: los
     * niveles se multiplican al final o cuando cambia la envolvente. */
    static AYS_TABLE uint32_t spread[8] = {
        0x000000, 0x000001, 0x000100, 0x000101, 0x010000, 0x010001, 0x010100, 0x010101,
    };
    uint32_t cnt = 0;

    for (unsigned k = 0; k < n; k++) {
        if (++c0 >= tp0) {
            c0 = 0;
            tone ^= 1;
        }
        if (++c1 >= tp1) {
            c1 = 0;
            tone ^= 2;
        }
        if (++c2 >= tp2) {
            c2 = 0;
            tone ^= 4;
        }
        pre ^= 1;
        if (pre) {                              /* reloj/16: ruido y envolvente */
            if (++nc >= np) {
                nc = 0;
                lfsr = (lfsr >> 1) | (((lfsr ^ (lfsr >> 3)) & 1u) << 16);
                noise = (lfsr & 1u) ? 7u : 0u;
            }
            if (++ec >= ep && (ec = 0, !a->env_holding)) {
                env_step(a);
                s0 += amp0 * (int32_t)(cnt & 0xFF);
                s1 += amp1 * (int32_t)((cnt >> 8) & 0xFF);
                s2 += amp2 * (int32_t)(cnt >> 16);
                cnt = 0;
                ea = ay_dac[a->env_level];
                if (envbits & 1)
                    amp0 = ea;
                if (envbits & 2)
                    amp1 = ea;
                if (envbits & 4)
                    amp2 = ea;
            }
        }
        cnt += spread[(tone | tmask) & (noise | nmask)];
    }
    s0 += amp0 * (int32_t)(cnt & 0xFF);
    s1 += amp1 * (int32_t)((cnt >> 8) & 0xFF);
    s2 += amp2 * (int32_t)(cnt >> 16);

    a->tone_count[0] = (uint16_t)c0;
    a->tone_count[1] = (uint16_t)c1;
    a->tone_count[2] = (uint16_t)c2;
    a->tone_out[0] = tone & 1u;
    a->tone_out[1] = (tone >> 1) & 1u;
    a->tone_out[2] = (tone >> 2) & 1u;
    a->noise_out = noise & 1u;
    a->prescale = (uint8_t)pre;
    a->noise_count = (uint16_t)nc;
    a->noise_lfsr = lfsr;
    a->env_count = ec;

    /* La media: n es 4 o 5. */
    static AYS_TABLE int32_t inv[6] = { 0, 65536, 32768, 21845, 16384, 13107 };
    ch[0] = (s0 * inv[n]) >> 16;
    ch[1] = (s1 * inv[n]) >> 16;
    ch[2] = (s2 * inv[n]) >> 16;
}

void aymix_reset(aymix_t *m)
{
    memset(m, 0, sizeof(*m));
}

/* (a·g) >> 16 con g de 0 a 65535, sin perder nada aunque a pase de 2^15: la
 * mitad alta de a·(g·2^16), una sola instrucción (mulhsu) en el RISC-V. */
static inline int32_t mul_gain(int32_t a, uint32_t g16)
{
    return (int32_t)(((int64_t)a * (int64_t)g16) >> 32);
}

AYS_FAST void aymix_sample(aymix_t *m, const int32_t ch[3], const int32_t *ch2, int32_t beeper,
                  unsigned stereo, unsigned volume, bool mute, int16_t out[2])
{
    aymix_sample_x(m, ch, ch2, beeper, stereo, volume, mute, 0, 0, out);
}

AYS_FAST void aymix_sample_x(aymix_t *m, const int32_t ch[3], const int32_t *ch2, int32_t beeper,
                    unsigned stereo, unsigned volume, bool mute, int32_t xl, int32_t xr, int16_t out[2])
{
    int32_t a = ch[0], b = ch[1], c = ch[2];
    if (ch2) {
        a += ch2[0];
        b += ch2[1];
        c += ch2[2];
    }
    int32_t l, r;
    switch (stereo) {
    case 1: l = a + (b >> 1); r = c + (b >> 1); break;
    case 2: l = a + (c >> 1); r = b + (c >> 1); break;
    default: l = r = (a + b + c) >> 1; break;
    }
    /* 0,42 del AY y 0,35 del beeper, como en el emulador (en Q15). */
    int32_t bp = (beeper * 22938) >> 16;
    int32_t mix[2] = { ((l * 13763) >> 15) + bp + xl, ((r * 13763) >> 15) + bp + xr };
    uint32_t gain = mute ? 0 : (volume * 257u) << 16;           /* 0..65535, por 2^16 */
    for (int i = 0; i < 2; i++) {
        /* Paso alto de unos 3,7 Hz: y = x - x' + (1 - 2^-11) y' (y con 8 bits
         * de fracción). */
        int32_t y = (mix[i] - m->dc_in[i]) * 256 + m->dc_out[i] - (m->dc_out[i] >> 11);
        m->dc_in[i] = mix[i];
        m->dc_out[i] = y;
        /* Con los dos AY y el beeper a tope, y >> 8 pasa de 2^15: el
         * producto no cabe en 32 bits, y el resultado hay que recortarlo. */
        int32_t v = mul_gain(y >> 8, gain);
        out[i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
    }
}
