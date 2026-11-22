/*
 * sndsynth.c - El sonido de ZX-V (ver sndsynth.h).
 */
#include "sndsynth.h"

#include <string.h>

_Static_assert(SMP_CHANNELS == SND_SMP_CHANNELS && SMP_CH_REGS == SND_SMP_CH_REGS,
               "los canales de muestras: los mismos en smpsynth.h y sndregs.h");

void sndsynth_reset(sndsynth_t *s)
{
    uint8_t *mem = s->smp.mem;
    memset(s, 0, sizeof(*s));
    ays_reset(&s->ay[0]);
    ays_reset(&s->ay[1]);
    smp_reset(&s->smp, mem);
    aymix_reset(&s->mix);
}

static void write_reg(sndsynth_t *s, unsigned reg, uint8_t val)
{
    if (reg < 32)
        ays_write(&s->ay[reg >> 4], reg & 15, val);
    else if (reg == SND_REG_STEREO)
        s->stereo = val;
    else if (reg == SND_REG_VOLUME)
        s->volume = val;
    else if (reg == SND_REG_AFLAGS)
        s->aflags = val;
    else if (reg == SND_REG_SMPVOL)
        s->smpvol = val;
    else if (reg == SND_REG_SMPKEY_HI)
        s->smpkey_hi = (uint8_t)(val & 0x0F);
    else if (reg == SND_REG_SMPKEY) {
        smp_key(&s->smp, val | (uint32_t)s->smpkey_hi << 8);
        s->smpkey_hi = 0;
    } else if (reg >= SND_REG_SMP && reg < SND_ALL)
        smp_write(&s->smp, reg - SND_REG_SMP, val);
}

void sndsynth_state(const sndsynth_t *s, uint8_t state[SND_STATE])
{
    memcpy(state, s->ay[0].regs, 16);
    memcpy(state + 16, s->ay[1].regs, 16);
    state[SND_REG_STEREO] = s->stereo;
    state[SND_REG_VOLUME] = s->volume;
    state[SND_REG_AFLAGS] = s->aflags;
    state[SND_REG_SMPVOL] = s->smpvol;
    state[SND_REGS] = 0;
}

void sndsynth_begin(sndsynth_t *s, const snd_span_t *f, unsigned offset)
{
    if (f && f->state) {
        uint8_t now[SND_STATE];
        sndsynth_state(s, now);
        for (unsigned r = 0; r < SND_REGS; r++) {
            uint8_t want = r < 32 ? (uint8_t)(f->state[r] & ays_mask(r)) : f->state[r];
            if (want != now[r])
                write_reg(s, r, want);
        }
    }
    if (f && f->smp && f->smp_ch < SND_SMP_CHANNELS) {
        /* El estado de un canal de muestras (CTRL, sin disparar ni parar). */
        unsigned base = f->smp_ch * SND_SMP_CH_REGS;
        for (unsigned r = 0; r < SND_SMP_CH_REGS; r++)
            if (f->smp[r] != s->smp.regs[base + r])
                smp_write(&s->smp, base + r, snd_persist(SND_REG_SMP + base + r, f->smp[r]));
    }
    if (f)
        s->f = *f;
    else
        memset(&s->f, 0, sizeof(s->f));
    s->pos = offset;
    s->ev = 0;
    s->smpkey_hi = 0;                   /* llega con su SMPKEY, en el mismo tramo */
}

/* Las escrituras que van antes de la muestra pos. */
static void events_due(sndsynth_t *s)
{
    const snd_span_t *f = &s->f;
    while (s->ev < f->events) {
        uint32_t e = f->ay[s->ev];
        int32_t at = (int16_t)(uint16_t)(e - f->first);
        if (at > (int32_t)s->pos)
            break;
        write_reg(s, e >> 16 & 0xFF, (uint8_t)(e >> 24));
        s->ev++;
    }
}

/* Cuántas muestras, desde pos y hasta n, sin ninguna escritura en medio. */
static unsigned run_length(const sndsynth_t *s, unsigned n)
{
    const snd_span_t *f = &s->f;
    if (s->ev >= f->events)
        return n;
    int32_t at = (int16_t)(uint16_t)(f->ay[s->ev] - f->first) - (int32_t)s->pos;
    return at <= 0 ? 1 : (unsigned)at < n ? (unsigned)at : n;
}

void sndsynth_render(sndsynth_t *s, unsigned n, uint32_t *out)
{
    const snd_span_t *f = &s->f;
    while (n) {
        /* Los canales de muestras, de una vez para todo el trozo sin
         * escrituras; los AY, muestra a muestra. */
        events_due(s);
        unsigned run = run_length(s, n < 64 ? n : 64);
        int32_t xl[64], xr[64];
        memset(xl, 0, run * sizeof(int32_t));
        memset(xr, 0, run * sizeof(int32_t));
        if (s->smp.mem)
            smp_mix(&s->smp, run, xl, xr);
        /* En la escala de la mezcla, por SMP_VOLUME (0-255): un canal a tope en
         * un lado va de -6.881 a 6.881, lo mismo de punta a punta que un canal
         * del AY (de 0 a 13.763). Doce a tope saturan. */
        int32_t g = s->smpvol * 13763 / 255;
        for (unsigned i = 0; i < run; i++, s->pos++) {
            if (s->pos < f->samples) {
                uint32_t b = f->beeper[s->pos];
                s->beep = b == 0xFFFF ? 65536 : (int32_t)b;
            }
            int32_t ch[3], ch2[3];
            int16_t o[2];
            ays_step(&s->ay[0], ch);
            ays_step(&s->ay[1], ch2);
            aymix_sample_x(&s->mix, ch, ch2, s->beep, s->stereo, s->volume, s->aflags & 1,
                           (xl[i] >> 2) * g >> 14, (xr[i] >> 2) * g >> 14, o);
            *out++ = (uint16_t)o[0] | (uint32_t)(uint16_t)o[1] << 16;
        }
        n -= run;
    }
}
