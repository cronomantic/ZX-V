/*
 * zxm_xm.c - Los XM (FastTracker 2), para los conversores de zxm_conv.c
 * (D98).
 *
 * El XM (The Unofficial XM File Format Specification, en MilkyTracker): de
 * 2 a 32 canales, patrones de 1 a 256 filas empaquetados, hasta 128
 * instrumentos con hasta 16 muestras cada uno, su mapa de notas, sus
 * envolventes de volumen y de panorama, el desvanecer y el vibrato
 * automático. Pasan:
 *   - los canales que tienen algo, hasta 12, en el centro;
 *   - los instrumentos, tal cual (el .zxm tiene los del XM), si entre todos
 *     no pasan de 255 muestras;
 *   - las muestras, de 8 o 16 bits en deltas, con su nota relativa, su
 *     afinación, su volumen y su panorama; los bucles de ida y vuelta,
 *     desenrollados;
 *   - el modo lineal o el Amiga (con el reloj del S3M y el XM);
 *   - los efectos 0-F, E, G, H, K, L, P, R, T (a I) y X1/X2, con su letra;
 *     la columna de volumen, al volumen o a uno de los dos efectos (A, EA/EB,
 *     4, 8, P y 3).
 * Se pierden E0x, E3x (glissando), F00 (parar la canción), la velocidad del
 * vibrato en la columna de volumen y los efectos de ModPlug, y lo dice. La
 * memoria de los efectos es la del reproductor: 1 y 2 la comparten (en el
 * FT2, cada uno la suya).
 *
 * Licencia MIT, como el resto del SDK.
 */
#include "zxm_conv.h"

enum {
    W_FX = ZXC_W_FMT, W_STOP = ZXC_W_FMT << 1, W_VIBSPEED = ZXC_W_FMT << 2, W_PACKED = ZXC_W_FMT << 3,
};

static const char magic[] = "Extended Module: ";

int zxc_xm_probe(const uint8_t *head, uint32_t n, uint32_t size)
{
    (void)size;
    if (n < 80)
        return 0;
    for (int i = 0; i < 17; i++)
        if (head[i] != (uint8_t)magic[i])
            return 0;
    return head[58] == 0x04 && head[59] == 0x01;        /* la versión 1.04, la de siempre */
}

/* El efecto del .zxm de uno del XM (0-35): el código, o 0. */
static int effect(zxm_conv_t *cv, int e, int p, uint8_t *param)
{
    int x = p >> 4;
    *param = (uint8_t)p;
    if (e <= 10 && e) {
        static const char code[] = "0123456789A";
        return code[e];
    }
    switch (e) {
    case 0: return p ? '0' : 0;
    case 11: return 'B';
    case 12:
        *param = (uint8_t)(p > 64 ? 64 : p);
        return 'C';
    case 13:
        *param = (uint8_t)(x * 10 + (p & 15));          /* la fila, en decimal */
        if (*param > 63)
            *param = 0;
        return 'D';
    case 14:
        if (x == 0 || x == 3 || x == 15) {
            zxc_warn(cv, W_FX, "E0x (filtro), E3x (glissando), los de ModPlug: se pierden");
            return 0;
        }
        return 'E';
    case 15:
        if (!p) {
            zxc_warn(cv, W_STOP, "F00 (parar la canción): sigue");
            return 0;
        }
        return 'F';
    case 16:
        *param = (uint8_t)(p > 64 ? 64 : p);
        return 'G';
    case 17: return 'H';
    case 20: return 'K';
    case 21: return 'L';
    case 25: return 'P';
    case 27: return 'R';
    case 29: return 'I';
    case 33:
        if (x == 1 || x == 2)
            return 'X';
        break;
    }
    zxc_warn(cv, W_FX, "E0x (filtro), E3x (glissando), los de ModPlug: se pierden");
    return 0;
}

/* La columna de volumen: el volumen o un efecto (en fx, fxp). */
static void volume(zxm_conv_t *cv, int v, zxc_cell_t *c, uint8_t *fx, uint8_t *fxp)
{
    int y = v & 15;
    *fx = 0;
    if (v >= 0x10 && v <= 0x50) {
        c->has |= ZXM_C_VOL;
        c->vol = (uint8_t)(v - 0x10);
        return;
    }
    switch (v >> 4) {
    case 0x6: *fx = 'A'; *fxp = (uint8_t)y; break;
    case 0x7: *fx = 'A'; *fxp = (uint8_t)(y << 4); break;
    case 0x8: *fx = 'E'; *fxp = (uint8_t)(0xB0 | y); break;
    case 0x9: *fx = 'E'; *fxp = (uint8_t)(0xA0 | y); break;
    case 0xA: zxc_warn(cv, W_VIBSPEED, "la velocidad del vibrato en la columna de volumen: se pierde"); break;
    case 0xB: *fx = '4'; *fxp = (uint8_t)y; break;
    case 0xC: *fx = '8'; *fxp = (uint8_t)(y * 17); break;
    case 0xD: *fx = 'P'; *fxp = (uint8_t)y; break;
    case 0xE: *fx = 'P'; *fxp = (uint8_t)(y << 4); break;
    case 0xF: *fx = '3'; *fxp = (uint8_t)(y << 4); break;
    }
}

/* La muestra del instrumento ins (1-) con la nota n (0-95), o -1. */
static int ins_sample(zxm_conv_t *cv, int ins, int n)
{
    if (ins < 1 || ins > cv->nins || !cv->ins_n[ins - 1])
        return -1;
    int k = zxc_b(cv, cv->ins_at[ins - 1] + 33 + (uint32_t)n);
    return k < cv->ins_n[ins - 1] ? cv->ins_first[ins - 1] + k : -1;
}

static uint32_t entry(zxm_conv_t *cv, uint32_t a, uint8_t e[5], uint8_t *has);

typedef void (*visit_fn)(zxm_conv_t *cv, void *ctx, int row, int ch, const uint8_t e[5], uint8_t has);

/* Recorre el patrón p: cada casilla de cada canal (e: nota, instrumento,
 * volumen, efecto y parámetro; has, cuáles trae). */
static void walk(zxm_conv_t *cv, int p, visit_fn fn, void *ctx)
{
    uint32_t a = cv->pat_at[p], end;
    if (!a)
        return;
    uint32_t hl = zxc_le32(cv, a), psize = zxc_le16(cv, a + 7);
    a += hl;
    end = a + psize;
    if (end > cv->size)
        end = cv->size;
    for (int row = 0; row < cv->pat_rows[p]; row++)
        for (int ch = 0; ch < cv->channels; ch++) {
            uint8_t e[5], has;
            if (a >= end)
                return;
            a = entry(cv, a, e, &has);
            fn(cv, ctx, row, ch, e, has);
        }
}

/* Una casilla del XM en a: e (nota, instrumento, volumen, efecto y
 * parámetro) y cuáles trae (has); da dónde empieza la siguiente. */
static uint32_t entry(zxm_conv_t *cv, uint32_t a, uint8_t e[5], uint8_t *has)
{
    uint8_t b = zxc_b(cv, a++);
    *has = 0;
    for (int k = 0; k < 5; k++)
        e[k] = 0;
    if (b & 0x80) {
        for (int k = 0; k < 5; k++)
            if (b & (1 << k)) {
                e[k] = zxc_b(cv, a++);
                *has |= (uint8_t)(1 << k);
            }
    } else {
        e[0] = b;
        for (int k = 1; k < 5; k++)
            e[k] = zxc_b(cv, a++);
        *has = 0x1F;
    }
    return a;
}

static void fill(zxm_conv_t *cv, int t, zxc_cell_t *c, const uint8_t e[5], uint8_t has)
{
    c->has = 0;
    int note = e[0], ins = e[1] <= cv->nins ? e[1] : 0;
    if (cv->scanning && ins)
        cv->last_ins[t] = (uint8_t)ins;
    if (note >= 1 && note <= 96) {
        c->has |= ZXM_C_NOTE;
        c->note = (uint8_t)note;
        int li = cv->last_ins[t];
        if (cv->scanning && li)             /* (los mapas, luego: zxc_xm_resolve) */
            cv->ins_notes[li - 1][(note - 1) >> 3] |= (uint8_t)(1 << ((note - 1) & 7));
    } else if (note == 97) {
        c->has |= ZXM_C_NOTE;
        c->note = ZXM_NOTE_OFF;
    }
    if (ins) {
        c->has |= ZXM_C_INS;
        c->ins = (uint8_t)ins;
    }
    int k = 0;
    if (has & 4) {
        volume(cv, e[2], c, &c->fx[0], &c->fxp[0]);
        if (c->fx[0]) {
            c->has |= ZXM_C_FX1;
            k = 1;
        }
    }
    int fx = (has & 0x18) ? effect(cv, e[3], e[4], &c->fxp[k]) : 0;
    if (fx) {
        c->has |= (uint8_t)(ZXM_C_FX1 << k);
        c->fx[k] = (uint8_t)fx;
    }
}

typedef struct {
    zxc_row_fn fn;
    void *ctx;
    int row;
    zxc_cell_t cells[ZXC_TRACKS];
} row_t;

static void gather(zxm_conv_t *cv, void *ctx, int row, int ch, const uint8_t e[5], uint8_t has)
{
    row_t *r = ctx;
    if (row != r->row) {                /* la fila de antes, acabada */
        r->fn(cv, r->ctx, r->row, r->cells);
        for (int t = 0; t < cv->tracks; t++)
            r->cells[t].has = 0;
        r->row = row;
    }
    int t = cv->ch_track[ch];
    if (t != 0xFF)
        fill(cv, t, &r->cells[t], e, has);
}

static void rows(zxm_conv_t *cv, int p, zxc_row_fn fn, void *ctx)
{
    row_t r;
    r.fn = fn;
    r.ctx = ctx;
    r.row = 0;
    for (int t = 0; t < cv->tracks; t++)
        r.cells[t].has = 0;
    walk(cv, p, gather, &r);
    fn(cv, ctx, r.row, r.cells);
}

/* Los canales que tienen algo (used) y cuántas notas toca cada uno en la
 * canción (notes, cada una tantas veces como suena su patrón, w). */
typedef struct {
    uint32_t used, w, notes[32];
} busy_t;

static void mark(zxm_conv_t *cv, void *ctx, int row, int ch, const uint8_t e[5], uint8_t has)
{
    (void)cv, (void)row;
    busy_t *b = ctx;
    if (e[0] || e[1] || e[2] || ((has & 0x18) && (e[3] || e[4])))
        b->used |= 1u << ch;
    if (e[0] >= 1 && e[0] <= 96)
        b->notes[ch] += b->w;
}

void zxc_xm_resolve(zxm_conv_t *cv)
{
    cv->scanning = 1;
    for (int i = 0; i < cv->nins; i++)
        for (int n = 0; n < 96; n++)
            if (cv->ins_notes[i][n >> 3] & (1 << (n & 7)))
                zxc_note(cv, ins_sample(cv, i + 1, n), n);
    cv->scanning = 0;
}

void zxc_xm_instrument(zxm_conv_t *cv, int i, uint8_t *out)
{
    for (int k = 0; k < ZXM_SINS_SIZE; k++)
        out[k] = 0;
    for (int n = 0; n < 96; n++) {
        int s = ins_sample(cv, i + 1, n);
        out[ZXM_SI_MAP + n] = (uint8_t)(s < 0 ? 255 : s);
    }
    if (!cv->ins_n[i])
        return;
    uint32_t a = cv->ins_at[i];
    for (int e = 0; e < 2; e++) {
        uint32_t pts = a + (e ? 177 : 129);
        uint8_t *o = out + (e ? ZXM_SI_PENV : ZXM_SI_VENV);
        for (int k = 0; k < 48; k++)
            o[k] = zxc_b(cv, pts + (uint32_t)k);
        uint8_t n = zxc_b(cv, a + 225 + (uint32_t)e);
        o[48] = n > 12 ? 12 : n;
        o[49] = zxc_b(cv, a + (e ? 230 : 227));
        o[50] = zxc_b(cv, a + (e ? 231 : 228));
        o[51] = zxc_b(cv, a + (e ? 232 : 229));
        uint8_t type = zxc_b(cv, a + 233 + (uint32_t)e);
        if (type & 1)
            out[ZXM_SI_FLAGS] |= (uint8_t)((e ? ZXM_SIF_PENV : ZXM_SIF_VENV) |
                                           (type & 2 ? (e ? ZXM_SIF_PSUS : ZXM_SIF_VSUS) : 0) |
                                           (type & 4 ? (e ? ZXM_SIF_PLOOP : ZXM_SIF_VLOOP) : 0));
    }
    uint32_t fade = zxc_le16(cv, a + 239);
    out[ZXM_SI_FADE] = (uint8_t)fade;
    out[ZXM_SI_FADE + 1] = (uint8_t)(fade >> 8);
    static const uint8_t wave[4] = { 0, 2, 1, 1 };      /* seno, cuadrada, sierra (las dos rampas) */
    out[ZXM_SI_VIBRATO] = wave[zxc_b(cv, a + 235) & 3];
    out[ZXM_SI_VIBRATO + 1] = zxc_b(cv, a + 236);
    out[ZXM_SI_VIBRATO + 2] = zxc_b(cv, a + 237);
    out[ZXM_SI_VIBRATO + 3] = zxc_b(cv, a + 238);
}

int zxc_xm_open(zxm_conv_t *cv)
{
    for (int i = 0; i < 20; i++)
        cv->title[i] = (char)zxc_b(cv, 17 + (uint32_t)i);
    uint32_t hsize = zxc_le32(cv, 60);
    int npos = (int)zxc_le16(cv, 64), restart = (int)zxc_le16(cv, 66), chans = (int)zxc_le16(cv, 68);
    int npat = (int)zxc_le16(cv, 70), nins = (int)zxc_le16(cv, 72);
    uint32_t xflags = zxc_le16(cv, 74), speed = zxc_le16(cv, 76), bpm = zxc_le16(cv, 78);
    if (!npos || npos > 256 || chans < 1 || chans > 32 || npat > ZXC_PATS || nins > ZXC_INS)
        return -1;
    cv->channels = chans;
    cv->speed = (uint8_t)(speed && speed < 256 ? speed : 6);
    cv->tempo = (uint8_t)(bpm < 32 ? 125 : bpm > 255 ? 255 : bpm);
    cv->flags = xflags & 1 ? ZXM_F_LINEAR : ZXM_F_S3MCLOCK;
    cv->npat = npat;
    cv->npos = 0;
    for (int i = 0; i < npos; i++) {
        int o = zxc_b(cv, 80 + (uint32_t)i);
        if (o < npat)
            cv->order[cv->npos++] = (uint8_t)o;
    }
    if (!cv->npos)
        return -1;
    cv->loop = restart < cv->npos ? restart : 0;

    /* Los patrones, uno detrás de otro. */
    uint32_t a = 60 + hsize;
    for (int p = 0; p < npat; p++) {
        uint32_t hl = zxc_le32(cv, a), rows = zxc_le16(cv, a + 5), psize = zxc_le16(cv, a + 7);
        if (hl < 9 || a + hl > cv->size)
            return -1;
        cv->pat_at[p] = a;
        cv->pat_rows[p] = (uint16_t)(rows && rows <= 256 ? rows : 64);
        a += hl + psize;
    }

    /* Los instrumentos, cada uno con sus muestras detrás. */
    cv->nins = nins;
    cv->nsmp = 0;
    for (int i = 0; i < nins; i++) {
        uint32_t isize = zxc_le32(cv, a), n = zxc_le16(cv, a + 27);
        if (isize < 29 || a + isize > cv->size)
            return -1;
        cv->ins_at[i] = a;
        cv->ins_first[i] = (uint8_t)cv->nsmp;
        if (!n) {
            a += isize;
            continue;
        }
        uint32_t shsize = zxc_le32(cv, a + 29), sh = a + isize, data = sh + n * shsize;
        if (n > 16)
            n = 16;
        for (uint32_t k = 0; k < n; k++, sh += shsize) {
            uint32_t len = zxc_le32(cv, sh), ls = zxc_le32(cv, sh + 4), ll = zxc_le32(cv, sh + 8);
            uint8_t type = zxc_b(cv, sh + 14);
            uint32_t at = data;
            data += len;
            if (cv->nsmp == ZXC_SAMPLES) {
                zxc_warn(cv, ZXC_W_SAMPLES, "más de 255 muestras: las de detrás no suenan");
                continue;
            }
            zxc_smp_t *m = &cv->smp[cv->nsmp];
            cv->ins_n[i]++;
            cv->nsmp++;
            if (zxc_b(cv, sh + 17) == 0xAD) {
                zxc_warn(cv, W_PACKED, "muestras comprimidas (ModPlug ADPCM): no suenan");
                continue;
            }
            int bytes = type & 0x10 ? 2 : 1;
            m->enc = (uint8_t)(bytes == 2 ? ZXC_D16 : ZXC_D8);
            m->file_at = at;
            if (at >= cv->size)
                len = 0;
            else if (len > cv->size - at)
                len = cv->size - at;
            m->frames = len / (uint32_t)bytes;
            ls /= (uint32_t)bytes;
            ll /= (uint32_t)bytes;
            if ((type & 3) && ll && ls < m->frames) {
                m->loop_start = ls;
                m->loop_len = ls + ll < m->frames ? ll : m->frames - ls;
                m->pingpong = (type & 3) == 2;
            }
            m->vol = zxc_b(cv, sh + 12) > 64 ? 64 : zxc_b(cv, sh + 12);
            m->fine = (int8_t)zxc_b(cv, sh + 13);
            m->pan = zxc_b(cv, sh + 15);
            m->rel = (int8_t)zxc_b(cv, sh + 16);
            m->flags = ZXM_SF_PAN;
        }
        a = data;
    }

    /* Las pistas: los canales con algo, hasta 12 (de más, los que más notas
     * tocan), en el centro. */
    busy_t b = { 0 };
    for (int p = 0; p < npat; p++) {
        b.w = zxc_plays(cv, p);
        walk(cv, p, mark, &b);
    }
    cv->tracks = zxc_pick_tracks(cv, b.used, b.notes, chans, cv->chan);
    for (int t = 0; t < cv->tracks; t++)
        cv->pan[t] = 128;
    if (!cv->tracks) {
        cv->tracks = 1;
        cv->pan[0] = 128;
    }
    for (int c = 0; c < 64; c++)
        cv->ch_track[c] = 0xFF;
    for (int t = 0; t < cv->tracks; t++)
        cv->ch_track[cv->chan[t]] = (uint8_t)t;
    cv->rows = rows;
    return 0;
}
