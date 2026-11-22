/*
 * zxm_mod.c - Los MOD, para los conversores de zxm_conv.c (D98).
 *
 * El MOD (https://www.aes.id.au/modformat.html): 31 muestras (o 15, el
 * antiguo sin firma), 128 posiciones y patrones de 64 filas; 4 canales
 * (M.K., M!K!, FLT4, 4CHN), 6 u 8 (6CHN, 8CHN, OCTA, CD81) o los xxCH de
 * otros trackers, hasta 12. Cada casilla: el periodo del Amiga, la muestra y
 * un efecto (0-F). Pasan:
 *   - las notas, del periodo a la más cercana (el C-2 del MOD, periodo 428,
 *     es el C-4 del .zxm), en modo Amiga con el reloj PAL;
 *   - los efectos, con las mismas letras, y la bandera ZXM_F_MODFX (los que
 *     no tienen memoria en el ProTracker); Dxx, con la fila en decimal;
 *   - el panorama del Amiga (izquierda, derecha, derecha, izquierda), con
 *     una separación del 75 %;
 *   - las muestras, de 8 bits con signo; la afinación, x 16.
 * Se pierden E0x (el filtro del Amiga), EFx (invertir el bucle) y F00
 * (parar la canción), y lo dice.
 *
 * Licencia MIT, como el resto del SDK.
 */
#include "zxm_conv.h"

#define ROWS        64

/* El periodo del Amiga de cada nota del .zxm (C-4, el 48: 428). */
static const uint16_t periods[96] = {
    6848, 6464, 6101, 5758, 5435, 5130, 4842, 4570, 4314, 4072, 3843, 3628, 3424, 3232, 3050, 2879,
    2718, 2565, 2421, 2285, 2157, 2036, 1922, 1814, 1712, 1616, 1525, 1440, 1359, 1283, 1211, 1143,
    1078, 1018, 961, 907, 856, 808, 763, 720, 679, 641, 605, 571, 539, 509, 480, 453, 428, 404, 381,
    360, 340, 321, 303, 286, 270, 254, 240, 227, 214, 202, 191, 180, 170, 160, 151, 143, 135, 127, 120,
    113, 107, 101, 95, 90, 85, 80, 76, 71, 67, 64, 60, 57, 54, 50, 48, 45, 42, 40, 38, 36, 34, 32, 30,
    28,
};

enum { W_FILTER = ZXC_W_FMT, W_INVLOOP = ZXC_W_FMT << 1, W_STOP = ZXC_W_FMT << 2 };

static int sig_channels(const uint8_t *s)
{
    static const char *four[] = { "M.K.", "M!K!", "FLT4", "4CHN" }, *eight[] = { "8CHN", "OCTA", "CD81", "FLT8" };
    for (int i = 0; i < 4; i++) {
        int a = 1, b = 1;
        for (int k = 0; k < 4; k++) {
            a &= s[k] == (uint8_t)four[i][k];
            b &= s[k] == (uint8_t)eight[i][k];
        }
        if (a)
            return 4;
        if (b)
            return 8;
    }
    if (s[0] >= '1' && s[0] <= '9' && s[1] == 'C' && s[2] == 'H' && s[3] == 'N')
        return s[0] - '0';
    if (s[0] >= '1' && s[0] <= '9' && s[1] >= '0' && s[1] <= '9' && s[2] == 'C' && s[3] == 'H')
        return (s[0] - '0') * 10 + s[1] - '0';
    return 0;
}

/* Lo de la cabecera que hace falta para reconocerlo. */
typedef struct {
    int channels, samples, npos, npat;
    uint32_t pos_at, pat_at, data_at;
} head_t;

static uint8_t hb(const uint8_t *h, uint32_t n, uint32_t i)
{
    return i < n ? h[i] : 0;
}

static int parse_head(head_t *d, const uint8_t *m, uint32_t n, uint32_t size)
{
    d->channels = n >= 1084 ? sig_channels(m + 1080) : 0;
    if (d->channels) {
        d->samples = 31;
        d->pos_at = 950;
        d->pat_at = 1084;
    } else {
        /* El antiguo, de 15 muestras y sin firma: que lo parezca. */
        d->channels = 4;
        d->samples = 15;
        d->pos_at = 470;
        d->pat_at = 600;
        if (n < 600 || hb(m, n, 470) == 0 || hb(m, n, 470) > 128)
            return -1;
        for (int i = 0; i < 128; i++)
            if (hb(m, n, 472 + (uint32_t)i) > 63)
                return -1;
        for (int i = 0; i < 15; i++)
            if (hb(m, n, 20 + 30u * (uint32_t)i + 25) > 64)
                return -1;
    }
    if (d->channels < 1 || d->channels > 32)
        return -1;
    d->npos = hb(m, n, d->pos_at);
    if (!d->npos || d->npos > 128)
        return -1;
    d->npat = 0;
    for (int i = 0; i < 128; i++)
        if (hb(m, n, d->pos_at + 2 + (uint32_t)i) + 1 > d->npat)
            d->npat = hb(m, n, d->pos_at + 2 + (uint32_t)i) + 1;
    d->data_at = d->pat_at + (uint32_t)d->npat * ROWS * (uint32_t)d->channels * 4u;
    return d->data_at > size ? -1 : 0;
}

int zxc_mod_probe(const uint8_t *head, uint32_t n, uint32_t size)
{
    head_t d;
    return !parse_head(&d, head, n, size);
}

/* La nota del .zxm (1-96) del periodo más cercano, o 0. */
static int period_note(uint32_t p)
{
    if (!p)
        return 0;
    int best = 0;
    uint32_t bd = 0xFFFFFFFFu;
    for (int i = 0; i < 96; i++) {
        uint32_t d = p > periods[i] ? p - periods[i] : periods[i] - p;
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best + 1;
}

/* El efecto del .zxm de uno del MOD: el código, o 0 si no hace nada (o no
 * se puede). */
static int effect(zxm_conv_t *cv, int e, int p, uint8_t *param)
{
    static const char code[] = "0123456789ABCDEF";
    *param = (uint8_t)p;
    switch (e) {
    case 0x0:
        return p ? '0' : 0;
    case 0xC:
        *param = (uint8_t)(p > 64 ? 64 : p);
        return 'C';
    case 0xD:
        *param = (uint8_t)((p >> 4) * 10 + (p & 15));      /* la fila, en decimal */
        if (*param > 63)
            *param = 0;
        return 'D';
    case 0xE:
        if (p >> 4 == 0x0) {
            zxc_warn(cv, W_FILTER, "E0x (el filtro del Amiga): no hay filtro");
            return 0;
        }
        if (p >> 4 == 0xF) {
            zxc_warn(cv, W_INVLOOP, "EFx (invertir el bucle): no se puede");
            return 0;
        }
        return 'E';
    case 0xF:
        if (!p) {
            zxc_warn(cv, W_STOP, "F00 (parar la canción): sigue");
            return 0;
        }
        return 'F';
    default:
        return code[e];
    }
}

/* La casilla de la fila r del patrón p en la pista t (fmt_a: dónde
 * empiezan los patrones). */
static void decode(zxm_conv_t *cv, int p, int r, int t, zxc_cell_t *c)
{
    int ch = cv->channels;
    uint32_t a = cv->fmt_a + (((uint32_t)p * ROWS + (uint32_t)r) * (uint32_t)ch + cv->chan[t]) * 4u;
    uint8_t b0 = zxc_b(cv, a), b1 = zxc_b(cv, a + 1), b2 = zxc_b(cv, a + 2), b3 = zxc_b(cv, a + 3);
    int note = period_note(((uint32_t)b0 & 15) << 8 | b1);
    int smp = (b0 & 0xF0) | b2 >> 4;
    c->has = 0;
    if (smp > cv->nsmp)
        smp = 0;
    if (cv->scanning) {
        if (smp)
            cv->last_ins[t] = (uint8_t)smp;
        if (note && cv->last_ins[t])
            zxc_note(cv, cv->last_ins[t] - 1, note - 1);
    }
    if (note) {
        c->has |= ZXM_C_NOTE;
        c->note = (uint8_t)note;
    }
    if (smp) {
        c->has |= ZXM_C_INS;
        c->ins = (uint8_t)smp;
    }
    int fx = effect(cv, b2 & 15, b3, &c->fxp[0]);
    if (fx) {
        c->has |= ZXM_C_FX1;
        c->fx[0] = (uint8_t)fx;
    }
}

static void rows(zxm_conv_t *cv, int p, zxc_row_fn fn, void *ctx)
{
    zxc_cell_t cells[ZXC_TRACKS];
    for (int r = 0; r < ROWS; r++) {
        for (int t = 0; t < cv->tracks; t++)
            decode(cv, p, r, t, &cells[t]);
        fn(cv, ctx, r, cells);
    }
}

int zxc_mod_open(zxm_conv_t *cv)
{
    uint8_t head[1084];
    uint32_t n = cv->size < sizeof head ? cv->size : (uint32_t)sizeof head;
    for (uint32_t i = 0; i < n; i++)
        head[i] = zxc_b(cv, i);
    head_t d;
    if (parse_head(&d, head, n, cv->size))
        return -1;
    for (int i = 0; i < 20; i++)
        cv->title[i] = (char)head[i];
    cv->speed = 6;
    cv->tempo = 125;
    cv->flags = ZXM_F_MODFX;            /* el Amiga PAL, con los efectos del ProTracker */
    cv->npos = d.npos;
    cv->loop = head[d.pos_at + 1] < d.npos ? head[d.pos_at + 1] : 0;
    for (int i = 0; i < d.npos; i++)
        cv->order[i] = head[d.pos_at + 2 + (uint32_t)i];
    cv->fmt_a = d.pat_at;

    /* Las pistas: todos los canales, hasta 12; de más, los que más notas
     * tocan en la canción. */
    cv->channels = d.channels;
    uint64_t used = 0;
    uint32_t notes[32] = { 0 };
    if (d.channels <= ZXC_TRACKS && !cv->only)
        used = ((uint64_t)1 << d.channels) - 1;
    else {
        for (int p = 0; p < d.npat; p++)
            for (uint32_t r = 0, w = zxc_plays(cv, p); w && r < ROWS; r++)
                for (int c = 0; c < d.channels; c++) {
                    uint32_t a = d.pat_at + (((uint32_t)p * ROWS + r) * (uint32_t)d.channels + (uint32_t)c) * 4u;
                    uint8_t b0 = zxc_b(cv, a), b1 = zxc_b(cv, a + 1), b2 = zxc_b(cv, a + 2), b3 = zxc_b(cv, a + 3);
                    if (b0 | b1 | b2 | b3)
                        used |= (uint64_t)1 << c;
                    if (period_note(((uint32_t)b0 & 15) << 8 | b1))
                        notes[c] += w;
                }
    }
    cv->tracks = zxc_pick_tracks(cv, used, notes, d.channels, cv->chan);
    for (int t = 0; t < cv->tracks; t++) {  /* izquierda, derecha, derecha, izquierda, al 75 % */
        int c = cv->chan[t];
        cv->pan[t] = c % 4 == 0 || c % 4 == 3 ? 32 : 224;
    }
    cv->npat = d.npat;
    for (int p = 0; p < d.npat; p++)
        cv->pat_rows[p] = ROWS;
    cv->nsmp = d.samples;
    uint32_t at = d.data_at;
    for (int s = 0; s < d.samples; s++) {
        const uint8_t *h = head + 20 + 30 * s;
        zxc_smp_t *m = &cv->smp[s];
        uint32_t len = ((uint32_t)h[22] << 8 | h[23]) * 2, ls = ((uint32_t)h[26] << 8 | h[27]) * 2;
        uint32_t ll = ((uint32_t)h[28] << 8 | h[29]) * 2;
        m->file_at = at;
        at += len;
        if (m->file_at + len > cv->size)    /* un fichero cortado: lo que haya */
            len = m->file_at < cv->size ? cv->size - m->file_at : 0;
        m->frames = len;
        if (ll > 2 && ls < len) {           /* con bucle: lo de detrás no suena */
            m->loop_start = ls;
            m->loop_len = ls + ll < len ? ll : len - ls;
        }
        m->enc = ZXC_S8;
        int ft = h[24] & 15;
        m->fine = (int8_t)((ft >= 8 ? ft - 16 : ft) * 16);
        m->vol = h[25] > 64 ? 64 : h[25];
    }
    cv->rows = rows;
    return 0;
}
