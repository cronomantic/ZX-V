/*
 * smpsynth.c - Los canales de muestras de ZX-V (ver smpsynth.h).
 *
 * Cada canal se calcula entero para todo el trozo (smp_mix), no muestra a
 * muestra: así el bucle interno es corto. La 8 bits se interpola en su
 * escala (cabe en 32 bits) y luego se pasa a 16.
 */
#include "smpsynth.h"

#include <string.h>

#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
#include "pico.h"
#define SMP_FAST(f)     __not_in_flash_func(f)
#else
#define SMP_FAST(f)     f
#endif

/* --- El ADPCM IMA ------------------------------------------------------------------- */

static const int8_t ima_index[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

static const uint16_t ima_step[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724,
    796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
    4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500,
    20350, 22385, 24623, 27086, 29794, 32767,
};

/* Un paso del descodificador: lo que vale la muestra con el nibble nib. */
static inline int32_t ima_decode(int32_t *pred, int32_t *index, unsigned nib)
{
    int32_t step = ima_step[*index], diff = step >> 3;
    if (nib & 4)
        diff += step;
    if (nib & 2)
        diff += step >> 1;
    if (nib & 1)
        diff += step >> 2;
    int32_t p = nib & 8 ? *pred - diff : *pred + diff;
    p = p > 32767 ? 32767 : p < -32768 ? -32768 : p;
    int32_t i = *index + ima_index[nib];
    *index = i < 0 ? 0 : i > 88 ? 88 : i;
    *pred = p;
    return p;
}

/* Lo mismo con tablas, para la mezcla: por cada índice del paso y los tres
 * bits de abajo del nibble, la diferencia y el índice siguiente (se hacen la
 * primera vez, en smp_reset). */
static uint16_t ima_diff[89 * 8];
static uint8_t ima_next[89 * 8];

static void ima_tables(void)
{
    for (int32_t i = 0; i < 89; i++)
        for (unsigned nib = 0; nib < 8; nib++) {
            int32_t step = ima_step[i], diff = step >> 3, next = i + ima_index[nib];
            if (nib & 4)
                diff += step;
            if (nib & 2)
                diff += step >> 1;
            if (nib & 1)
                diff += step >> 2;
            ima_diff[i * 8 + nib] = (uint16_t)diff;     /* hasta 61.436: cabe */
            ima_next[i * 8 + nib] = (uint8_t)(next < 0 ? 0 : next > 88 ? 88 : next);
        }
}

static inline int32_t SMP_FAST(ima_fast)(int32_t *pred, int32_t *index, unsigned nib)
{
    uint32_t k = (uint32_t)*index * 8 + (nib & 7);
    int32_t diff = ima_diff[k], p = nib & 8 ? *pred - diff : *pred + diff;
    p = p > 32767 ? 32767 : p < -32768 ? -32768 : p;
    *index = ima_next[k];
    *pred = p;
    return p;
}

uint32_t smp_adpcm_encode(const int16_t *in, uint32_t n, uint8_t *out)
{
    int32_t index = 0;
    uint32_t bytes = 0;
    for (uint32_t blk = 0; blk * SMP_ADPCM_BLOCK < n; blk++) {
        uint32_t m = n - blk * SMP_ADPCM_BLOCK < SMP_ADPCM_BLOCK ? n - blk * SMP_ADPCM_BLOCK : SMP_ADPCM_BLOCK;
        bytes += smp_adpcm_block(in + blk * SMP_ADPCM_BLOCK, m, &index, out + bytes);
    }
    return bytes;
}

uint32_t smp_adpcm_block(const int16_t *s, uint32_t m, int32_t *index_io, uint8_t *o)
{
    int32_t index = *index_io;
    {
        memset(o, 0, SMP_ADPCM_BYTES);
        int32_t pred = s[0];
        o[0] = (uint8_t)pred;
        o[1] = (uint8_t)(pred >> 8);
        o[2] = (uint8_t)index;
        for (uint32_t j = 1; j < m; j++) {
            int32_t diff = s[j] - pred, step = ima_step[index];
            unsigned nib = 0;
            if (diff < 0) {
                nib = 8;
                diff = -diff;
            }
            if (diff >= step) {
                nib |= 4;
                diff -= step;
            }
            if (diff >= step >> 1) {
                nib |= 2;
                diff -= step >> 1;
            }
            if (diff >= step >> 2)
                nib |= 1;
            ima_decode(&pred, &index, nib);        /* como lo hará el descodificador */
            o[4 + (j - 1) / 2] |= (uint8_t)(nib << ((j - 1) & 1) * 4);
        }
    }
    *index_io = index;
    return SMP_ADPCM_BYTES;
}

/* El descodificador de un canal, en orden: la muestra siguiente (que no
 * pasa del final). Al empezar un bloque, la de su cabecera. Con el estado en
 * variables, para que en el bucle de la mezcla vaya en registros. */
typedef struct {
    const uint8_t *mem;
    int32_t pred, index;
    uint32_t blk, di;
} adpcm_t;

static inline int32_t SMP_FAST(adpcm_get)(adpcm_t *d)
{
    if (d->di == SMP_ADPCM_BLOCK) {
        d->blk += SMP_ADPCM_BYTES;
        d->di = 0;
    }
    const uint8_t *p = d->mem + d->blk;
    if (!d->di++) {
        d->pred = (int16_t)(p[0] | p[1] << 8);
        d->index = p[2] > 88 ? 88 : p[2];
        return d->pred;
    }
    uint32_t j = d->di - 2;
    return ima_fast(&d->pred, &d->index, p[4 + (j >> 1)] >> (j & 1) * 4 & 15);
}

static inline void adpcm_load(adpcm_t *d, const uint8_t *mem, const smp_channel_t *c)
{
    *d = (adpcm_t){ mem, c->pred, c->index, c->blk, c->di };
}

static inline void adpcm_save(const adpcm_t *d, smp_channel_t *c)
{
    c->pred = d->pred;
    c->index = d->index;
    c->blk = d->blk;
    c->di = d->di;
}

/* La muestra siguiente a pos: la de pos + 1, o al final, la del bucle (o
 * la misma si no hay). */
static inline int32_t SMP_FAST(adpcm_next)(adpcm_t *d, const smp_channel_t *c, uint32_t pos, int32_t a)
{
    if (pos + 1 < c->len)
        return adpcm_get(d);
    return c->loop < c->len ? c->loopv : a;
}

/* El descodificador, puesto en la muestra pos: deja en a la de pos y en b
 * la siguiente. */
static void adpcm_seek(const uint8_t *mem, smp_channel_t *c)
{
    adpcm_t d = { mem, 0, 0, c->base + c->pos / SMP_ADPCM_BLOCK * SMP_ADPCM_BYTES, 0 };
    for (uint32_t i = 0; i <= c->pos % SMP_ADPCM_BLOCK; i++)
        c->a = adpcm_get(&d);
    c->b = adpcm_next(&d, c, c->pos, c->a);
    adpcm_save(&d, c);
}

/* --- Los registros ---------------------------------------------------------------- */

void smp_reset(smpsynth_t *s, uint8_t *mem)
{
    memset(s->regs, 0, sizeof(s->regs));
    memset(s->ch, 0, sizeof(s->ch));
    s->mem = mem;
    if (!ima_diff[8 * 88 + 7])
        ima_tables();
    for (unsigned c = 0; c < SMP_CHANNELS; c++) {
        s->regs[c * SMP_CH_REGS + SMP_R_PAN] = 128;
        s->ch[c].interp = 1;
    }
}

static uint32_t get(const uint8_t *p, unsigned n)
{
    uint32_t v = 0;
    while (n--)
        v = v << 8 | p[n];
    return v;
}

/* La muestra de SLOT, desde OFFSET. */
static void trigger(smpsynth_t *s, unsigned ch)
{
    smp_channel_t *c = &s->ch[ch];
    c->playing = 0;
    if (!s->mem)
        return;                         /* sin memoria de sonido, nada */
    const uint8_t *r = s->regs + ch * SMP_CH_REGS;
    const uint8_t *t = s->mem + SMP_MEM + r[SMP_R_SLOT] * SMP_SLOT_SIZE;
    uint32_t base = get(t, 3), fmt = t[3], len = get(t + 4, 4), loop = get(t + 8, 4);
    uint64_t bytes = fmt == SMP_FMT_ADPCM ? ((uint64_t)len + SMP_ADPCM_BLOCK - 1) / SMP_ADPCM_BLOCK * SMP_ADPCM_BYTES
                                          : (uint64_t)len;
    uint32_t pos = (uint32_t)r[SMP_R_OFFSET] << 8;
    c->playing = 0;
    if (fmt > SMP_FMT_ADPCM || !len || base > SMP_MEM || bytes > SMP_MEM - base || pos >= len)
        return;                         /* una entrada que no vale: no suena */
    c->fmt = (uint8_t)fmt;
    c->base = base;
    c->len = len;
    c->loop = loop;
    c->frac = 0;
    c->playing = 1;
    if (fmt == SMP_FMT_ADPCM) {
        c->loopv = 0;
        if (loop < len) {                   /* la del bucle, una vez */
            c->pos = loop;
            adpcm_seek(s->mem, c);
            c->loopv = c->a;
        }
        c->pos = pos;
        adpcm_seek(s->mem, c);
    } else {
        c->loopv = loop < len ? ((const int8_t *)s->mem)[base + loop] * 256 : 0;
        c->pos = pos;
    }
}

static void gains(smpsynth_t *s, unsigned ch)
{
    const uint8_t *r = s->regs + ch * SMP_CH_REGS;
    int32_t vol = r[SMP_R_VOL] > 64 ? 64 : r[SMP_R_VOL], pan = r[SMP_R_PAN];
    s->ch[ch].gl = vol * (256 - pan) >> 6;
    s->ch[ch].gr = vol * (pan + 1) >> 6;
}

void smp_write(smpsynth_t *s, unsigned reg, uint8_t val)
{
    if (reg >= SMP_REGS)
        return;
    unsigned ch = reg / SMP_CH_REGS, f = reg % SMP_CH_REGS;
    smp_channel_t *c = &s->ch[ch];
    s->regs[reg] = f == SMP_R_CTRL ? (uint8_t)(val & (SMP_NOINTERP | SMP_DIRECT)) : val;
    switch (f) {
    case SMP_R_VOL:
    case SMP_R_PAN:
        gains(s, ch);
        break;
    case SMP_R_CTRL:
        c->interp = !(val & SMP_NOINTERP);
        c->direct = (val & SMP_DIRECT) != 0;
        if (val & SMP_STOP)
            c->playing = 0;
        if (val & SMP_KEY)
            trigger(s, ch);
        break;
    case SMP_R_STEP:
    case SMP_R_STEP + 1:
    case SMP_R_STEP + 2:
        c->step = get(s->regs + ch * SMP_CH_REGS + SMP_R_STEP, 3);
        break;
    default:
        break;
    }
}

void smp_key(smpsynth_t *s, uint32_t mask)
{
    for (unsigned ch = 0; ch < SMP_CHANNELS; ch++)
        if (mask >> ch & 1)
            trigger(s, ch);
}

/* --- La mezcla -------------------------------------------------------------------- */

/* Al pasar del final: vuelve al bucle, o se para (false). */
static inline bool wrap(smp_channel_t *c)
{
    if (c->loop >= c->len) {
        c->playing = 0;
        return false;
    }
    c->pos = c->loop + (c->pos - c->len) % (c->len - c->loop);
    return true;
}

static void SMP_FAST(mix_pcm8)(const uint8_t *mem, smp_channel_t *c, unsigned n, int32_t *l, int32_t *r)
{
    const int8_t *m = (const int8_t *)mem + c->base;
    uint32_t pos = c->pos, frac = c->frac, istep = c->step >> 16, fstep = c->step & 0xFFFF;
    uint32_t len = c->len, last = len - 1;
    int32_t gl = c->gl, gr = c->gr, after = c->loop < len ? c->loopv / 256 : m[last];
    bool interp = c->interp;
    for (unsigned i = 0; i < n; i++) {
        int32_t a = m[pos], v = a * 256;
        if (interp) {
            int32_t b = pos < last ? m[pos + 1] : after;
            v += (b - a) * (int32_t)frac >> 8;
        }
        l[i] += v * gl >> 8;
        r[i] += v * gr >> 8;
        frac += fstep;
        pos += istep + (frac >> 16);
        frac &= 0xFFFF;
        if (pos >= len) {
            c->pos = pos;
            if (!wrap(c))
                return;
            pos = c->pos;
        }
    }
    c->pos = pos;
    c->frac = frac;
}

static void SMP_FAST(mix_adpcm)(const uint8_t *mem, smp_channel_t *c, unsigned n, int32_t *l, int32_t *r)
{
    uint32_t step = c->step < SMP_ADPCM_MAX_STEP ? c->step : SMP_ADPCM_MAX_STEP;
    uint32_t istep = step >> 16, fstep = step & 0xFFFF, pos = c->pos, frac = c->frac, len = c->len;
    int32_t gl = c->gl, gr = c->gr, a = c->a, b = c->b, interp = c->interp;
    adpcm_t d;
    adpcm_load(&d, mem, c);
    for (unsigned i = 0; i < n; i++) {
        int32_t v = a;
        if (interp)
            v += (b - a) * (int32_t)(frac >> 1) >> 15;
        l[i] += v * gl >> 8;
        r[i] += v * gr >> 8;
        frac += fstep;
        uint32_t k = istep + (frac >> 16);
        frac &= 0xFFFF;
        while (k) {
            k--;
            if (++pos >= len) {
                c->pos = pos + k;       /* lo que falta, desde el principio del bucle */
                if (!wrap(c))
                    return;
                adpcm_seek(mem, c);
                pos = c->pos;
                a = c->a;
                b = c->b;
                adpcm_load(&d, mem, c);
                break;
            }
            a = b;
            b = adpcm_next(&d, c, pos, a);
        }
    }
    c->pos = pos;
    c->frac = frac;
    c->a = a;
    c->b = b;
    adpcm_save(&d, c);
}

void SMP_FAST(smp_mix)(smpsynth_t *s, unsigned n, int32_t *l, int32_t *r)
{
    for (unsigned ch = 0; ch < SMP_CHANNELS; ch++) {
        smp_channel_t *c = &s->ch[ch];
        if (c->direct) {
            int32_t v = (int8_t)s->regs[ch * SMP_CH_REGS + SMP_R_OFFSET] * 256;
            int32_t vl = v * c->gl >> 8, vr = v * c->gr >> 8;
            for (unsigned i = 0; i < n; i++) {
                l[i] += vl;
                r[i] += vr;
            }
        } else if (c->playing) {
            if (c->fmt == SMP_FMT_ADPCM)
                mix_adpcm(s->mem, c, n, l, r);
            else
                mix_pcm8(s->mem, c, n, l, r);
        }
    }
}
