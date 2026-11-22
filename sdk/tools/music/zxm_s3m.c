/*
 * zxm_s3m.c - Los S3M (Scream Tracker 3), para los conversores de
 * zxm_conv.c (D98).
 *
 * El S3M (Scream Tracker 3.20 File Formats; S3M Format, ModdingWiki): hasta
 * 32 canales (16 de muestras, 8 a la izquierda y 8 a la derecha, y 9 de FM),
 * hasta 99 instrumentos, patrones de 64 filas empaquetados y los efectos de
 * letra (A-Z). Pasan:
 *   - los canales de muestras que tienen algo, hasta 12, con el panorama de
 *     la tabla del final de la cabecera o, si no la hay, el de su lado;
 *   - las muestras, de 8 o 16 bits, con signo o sin él, y su C4Spd en nota
 *     relativa y afinación; el bucle, solo hacia delante, como en el S3M;
 *   - las notas (el C-4 del S3M es el del .zxm), en modo Amiga con el reloj
 *     del S3M (8.363 x 1.712);
 *   - los efectos, cada uno a su pareja del .zxm (A a t, T a F, D a A o a
 *     los finos EAx/EBx, E y F a 2 y 1 o a los finos E2x/E1x y extrafinos
 *     X2x/X1x, G a 3, H a 4, I, J a 0, K a 6, L a 5, O a 9, Q a R, R a 7, S a
 *     los E, U, V a G, X a 8); la marca de ST3.00 (deslizar el volumen
 *     también en el primer tick), a ZXM_F_ST300.
 * Se pierden los canales FM (AdLib), el estéreo de las muestras (se queda
 * el izquierdo), S1x (glissando), SAx (el estéreo antiguo), SFx (funk), los
 * T de menos de 0x20 y Y y Z, y lo dice. La memoria de los efectos es la de
 * cada uno (en el ST3, casi todos comparten una).
 *
 * Licencia MIT, como el resto del SDK.
 */
#include "zxm_conv.h"

#define ROWS        64

enum {
    W_ADLIB = ZXC_W_FMT, W_STEREO = ZXC_W_FMT << 1, W_FX = ZXC_W_FMT << 2, W_TEMPO = ZXC_W_FMT << 3,
    W_PACKED = ZXC_W_FMT << 4,
};

int zxc_s3m_probe(const uint8_t *head, uint32_t n, uint32_t size)
{
    (void)size;
    return n >= 0x60 && head[0x2C] == 'S' && head[0x2D] == 'C' && head[0x2E] == 'R' && head[0x2F] == 'M' &&
           head[0x1D] == 16;
}

/* El efecto del .zxm de uno del S3M (cmd 1-26, A-Z): el código, o 0. */
static int effect(zxm_conv_t *cv, int cmd, int p, uint8_t *param)
{
    int x = p >> 4, y = p & 15;
    *param = (uint8_t)p;
    switch (cmd + '@') {
    case 'A':
        return p ? 't' : 0;
    case 'B':
        return 'B';
    case 'C':
        *param = (uint8_t)(x * 10 + y);         /* la fila, en decimal */
        if (*param > 63)
            *param = 0;
        return 'D';
    case 'D':
        if (y == 15 && x) {
            *param = (uint8_t)(0xA0 | x);
            return 'E';
        }
        if (x == 15 && y) {
            *param = (uint8_t)(0xB0 | y);
            return 'E';
        }
        return 'A';
    case 'E':
    case 'F': {
        int up = cmd + '@' == 'F';
        if (x == 15 || x == 14) {               /* fino (E1x/E2x) o extrafino (X1x/X2x) */
            *param = (uint8_t)((up ? 0x10 : 0x20) | y);
            return x == 15 ? 'E' : 'X';
        }
        return up ? '1' : '2';
    }
    case 'G': return '3';
    case 'H': return '4';
    case 'I': return 'I';
    case 'J': return p ? '0' : 0;
    case 'K': return '6';
    case 'L': return '5';
    case 'O': return '9';
    case 'Q': return 'R';
    case 'R': return '7';
    case 'U': return 'U';
    case 'V':
        *param = (uint8_t)(p > 64 ? 64 : p);
        return 'G';
    case 'W': return 'H';
    case 'X':
        *param = (uint8_t)(p == 0xA4 ? 128 : p * 2 > 255 ? 255 : p * 2);
        return '8';
    case 'T':
        if (p >= 0x20)
            return 'F';
        zxc_warn(cv, W_TEMPO, "T de menos de 0x20: se pierde");
        return 0;
    case 'S':
        switch (x) {
        case 0x2: *param = (uint8_t)(0x50 | y); return 'E';
        case 0x3: *param = (uint8_t)(0x40 | y); return 'E';
        case 0x4: *param = (uint8_t)(0x70 | y); return 'E';
        case 0x8: *param = (uint8_t)(0x80 | y); return 'E';
        case 0xB: *param = (uint8_t)(0x60 | y); return 'E';
        case 0xC: *param = (uint8_t)(0xC0 | y); return 'E';
        case 0xD: *param = (uint8_t)(0xD0 | y); return 'E';
        case 0xE: *param = (uint8_t)(0xE0 | y); return 'E';
        case 0x0: return 0;
        }
        zxc_warn(cv, W_FX, "S1x (glissando), SAx (estéreo antiguo), SFx (funk), Y o Z: se pierden");
        return 0;
    case '@':
        return 0;
    default:
        zxc_warn(cv, W_FX, "S1x (glissando), SAx (estéreo antiguo), SFx (funk), Y o Z: se pierden");
        return 0;
    }
}

/* Recorre el patrón p (empaquetado): con cell, la casilla de cada canal que
 * tenga algo. */
typedef void (*visit_fn)(zxm_conv_t *cv, void *ctx, int row, int ch, int note, int ins, int vol, int cmd, int info);

static void walk(zxm_conv_t *cv, int p, visit_fn fn, void *ctx)
{
    uint32_t a = cv->pat_at[p], end = a ? a + zxc_le16(cv, a - 2) : 0;    /* (con o sin la palabra del largo) */
    if (!a)
        return;
    if (end <= a || end > cv->size)
        end = cv->size;
    for (int row = 0; row < ROWS && a < end;) {
        int w = zxc_b(cv, a++);
        if (!w) {
            row++;
            continue;
        }
        int note = 255, ins = 0, vol = 255, cmd = 0, info = 0;
        if (w & 32) {
            note = zxc_b(cv, a);
            ins = zxc_b(cv, a + 1);
            a += 2;
        }
        if (w & 64)
            vol = zxc_b(cv, a++);
        if (w & 128) {
            cmd = zxc_b(cv, a);
            info = zxc_b(cv, a + 1);
            a += 2;
        }
        fn(cv, ctx, row, w & 31, note, ins, vol, cmd, info);
    }
}

static void fill(zxm_conv_t *cv, int t, zxc_cell_t *c, int note, int ins, int vol, int cmd, int info)
{
    c->has = 0;
    if (ins > cv->nsmp)
        ins = 0;
    if (cv->scanning && ins)
        cv->last_ins[t] = (uint8_t)ins;
    if (note < 254 && (note & 15) < 12 && (note >> 4) * 12 + (note & 15) < 96) {
        int n = (note >> 4) * 12 + (note & 15);
        c->has |= ZXM_C_NOTE;
        c->note = (uint8_t)(n + 1);
        if (cv->scanning && cv->last_ins[t])
            zxc_note(cv, cv->last_ins[t] - 1, n);
    } else if (note == 254) {
        c->has |= ZXM_C_NOTE;
        c->note = ZXM_NOTE_CUT;
    }
    if (ins) {
        c->has |= ZXM_C_INS;
        c->ins = (uint8_t)ins;
    }
    if (vol <= 64) {
        c->has |= ZXM_C_VOL;
        c->vol = (uint8_t)vol;
    }
    int fx = cmd ? effect(cv, cmd, info, &c->fxp[0]) : 0;
    if (fx) {
        c->has |= ZXM_C_FX1;
        c->fx[0] = (uint8_t)fx;
    }
}

typedef struct {
    zxc_row_fn fn;
    void *ctx;
    int row;
    zxc_cell_t cells[ZXC_TRACKS];
} row_t;

static void clear(zxm_conv_t *cv, row_t *r)
{
    for (int t = 0; t < cv->tracks; t++)
        r->cells[t].has = 0;
}

static void gather(zxm_conv_t *cv, void *ctx, int row, int ch, int note, int ins, int vol, int cmd, int info)
{
    row_t *r = ctx;
    if (row != r->row) {                /* la fila de antes, acabada */
        r->fn(cv, r->ctx, r->row, r->cells);
        clear(cv, r);
        r->row = row;
    }
    int t = cv->ch_track[ch];
    if (t != 0xFF)
        fill(cv, t, &r->cells[t], note, ins, vol, cmd, info);
}

static void rows(zxm_conv_t *cv, int p, zxc_row_fn fn, void *ctx)
{
    row_t r;
    r.fn = fn;
    r.ctx = ctx;
    r.row = 0;
    clear(cv, &r);
    walk(cv, p, gather, &r);
    fn(cv, ctx, r.row, r.cells);
}

/* Los canales que tienen algo (used) y cuántas notas toca cada uno en la
 * canción (notes, cada una tantas veces como suena su patrón, w). */
typedef struct {
    uint32_t used, w, notes[32];
} busy_t;

static void mark(zxm_conv_t *cv, void *ctx, int row, int ch, int note, int ins, int vol, int cmd, int info)
{
    (void)cv, (void)row, (void)info;
    busy_t *b = ctx;
    if (note != 255 || ins || vol != 255 || cmd)
        b->used |= 1u << ch;
    if (note < 254)
        b->notes[ch] += b->w;
}

int zxc_s3m_open(zxm_conv_t *cv)
{
    if (cv->size < 0x60)
        return -1;
    int nord = (int)zxc_le16(cv, 0x20), nins = (int)zxc_le16(cv, 0x22), npat = (int)zxc_le16(cv, 0x24);
    uint32_t hflags = zxc_le16(cv, 0x26), cwtv = zxc_le16(cv, 0x28), ffi = zxc_le16(cv, 0x2A);
    if (nord > 256 || nins > 255 || npat > ZXC_PATS)
        return -1;
    for (int i = 0; i < 28 && i < 32; i++)
        cv->title[i] = (char)zxc_b(cv, (uint32_t)i);
    uint8_t gv = zxc_b(cv, 0x30), is = zxc_b(cv, 0x31), it = zxc_b(cv, 0x32), mv = zxc_b(cv, 0x33);
    uint8_t dp = zxc_b(cv, 0x35);
    cv->gvol = gv > 64 ? 64 : gv;
    cv->speed = is && is != 255 ? is : 6;
    cv->tempo = it >= 33 ? it : 125;
    cv->flags = ZXM_F_S3MCLOCK | (cwtv == 0x1300 || (hflags & 0x40) ? ZXM_F_ST300 : 0);
    uint32_t ord_at = 0x60, ins_at = ord_at + (uint32_t)nord, pat_at = ins_at + 2u * (uint32_t)nins;
    uint32_t pan_at = pat_at + 2u * (uint32_t)npat;

    cv->npat = npat;
    for (int p = 0; p < npat; p++) {
        uint32_t para = zxc_le16(cv, pat_at + 2u * (uint32_t)p) * 16u;
        cv->pat_at[p] = para && para + 2 < cv->size ? para + 2 : 0;
        cv->pat_rows[p] = ROWS;
    }
    cv->npos = 0;
    for (int i = 0; i < nord; i++) {
        int o = zxc_b(cv, ord_at + (uint32_t)i);
        if (o == 255)
            break;
        if (o < npat)
            cv->order[cv->npos++] = (uint8_t)o;
    }
    if (!cv->npos)
        return -1;
    cv->loop = 0;

    /* Las pistas: los canales de muestras con algo, hasta 12 (de más, los
     * que más notas tocan). */
    busy_t b = { 0 };
    for (int p = 0; p < npat; p++) {
        b.w = zxc_plays(cv, p);
        walk(cv, p, mark, &b);
    }
    cv->channels = 32;
    uint64_t used = 0;
    for (int c = 0; c < 32; c++) {
        uint8_t set = zxc_b(cv, 0x40 + (uint32_t)c);
        if (!(b.used & (1u << c)) || (set & 0x80))
            continue;
        if ((set & 0x7F) >= 16)
            zxc_warn(cv, W_ADLIB, "los canales FM (AdLib): no hay FM");
        else
            used |= (uint64_t)1 << c;
    }
    cv->tracks = zxc_pick_tracks(cv, used, b.notes, 32, cv->chan);
    for (int t = 0; t < cv->tracks; t++) {
        int c = cv->chan[t];
        uint8_t set = zxc_b(cv, 0x40 + (uint32_t)c);
        uint8_t pan = (set & 0x7F) < 8 ? 3 * 17 : 12 * 17;
        if (dp == 252 && (zxc_b(cv, pan_at + (uint32_t)c) & 0x20))
            pan = (uint8_t)((zxc_b(cv, pan_at + (uint32_t)c) & 15) * 17);
        if (!(mv & 0x80))
            pan = 128;                  /* en mono */
        cv->pan[t] = pan;
    }
    if (!cv->tracks) {                  /* sin notas: una pista callada */
        cv->tracks = 1;
        cv->chan[0] = 0;
        cv->pan[0] = 128;
    }
    for (int c = 0; c < 64; c++)
        cv->ch_track[c] = 0xFF;
    for (int t = 0; t < cv->tracks; t++)
        cv->ch_track[cv->chan[t]] = (uint8_t)t;

    /* Las muestras: un instrumento del S3M es una muestra. */
    cv->nsmp = nins;
    for (int i = 0; i < nins; i++) {
        zxc_smp_t *m = &cv->smp[i];
        uint32_t a = zxc_le16(cv, ins_at + 2u * (uint32_t)i) * 16u;
        int type = a ? zxc_b(cv, a) : 0;
        if (type >= 2)
            zxc_warn(cv, W_ADLIB, "los instrumentos FM (AdLib): no hay FM");
        if (type != 1)
            continue;
        uint32_t data = ((uint32_t)zxc_b(cv, a + 0x0D) << 16 | zxc_le16(cv, a + 0x0E)) * 16u;
        uint32_t len = zxc_le32(cv, a + 0x10), lb = zxc_le32(cv, a + 0x14), le = zxc_le32(cv, a + 0x18);
        uint8_t fl = zxc_b(cv, a + 0x1F);
        if (zxc_b(cv, a + 0x1E)) {
            zxc_warn(cv, W_PACKED, "muestras comprimidas (DP30ADPCM): no suenan");
            continue;
        }
        if (fl & 2)
            zxc_warn(cv, W_STEREO, "muestras en estéreo: se queda el izquierdo");
        int bytes = fl & 4 ? 2 : 1;
        m->enc = (uint8_t)(fl & 4 ? (ffi == 2 ? ZXC_U16 : ZXC_S16) : (ffi == 2 ? ZXC_U8 : ZXC_S8));
        m->file_at = data;
        if (data >= cv->size)
            len = 0;
        else if (len > (cv->size - data) / (uint32_t)bytes)
            len = (cv->size - data) / (uint32_t)bytes;
        m->frames = len;
        if ((fl & 1) && le > lb && lb < len) {
            m->loop_start = lb;
            m->loop_len = (le < len ? le : len) - lb;
        }
        m->vol = zxc_b(cv, a + 0x1C) > 64 ? 64 : zxc_b(cv, a + 0x1C);
        zxc_c4hz(m, zxc_le32(cv, a + 0x20));
    }
    cv->rows = rows;
    return 0;
}
