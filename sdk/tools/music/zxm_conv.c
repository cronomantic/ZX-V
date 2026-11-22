/*
 * zxm_conv.c - Lo común de los conversores de módulos de muestras (MOD, S3M
 * y XM) a .zxm (D98), sin memoria dinámica (ver zxm_conv.h):
 *   - el fichero se lee por una caché (o entero, si está en la memoria);
 *   - las muestras que no suenan en ningún patrón no van; si las demás no
 *     caben en la memoria de sonido, las más grandes en ADPCM hasta que
 *     caben, y si ni así, las más grandes, además, a la mitad (la media de
 *     cada dos: una octava más grave de grabadas, que se toca una más aguda;
 *     suenan igual de alto, con menos agudos); y si ni así, fuera las
 *     últimas. Una muestra que suena por encima del techo del ADPCM (96 kHz)
 *     no va en ADPCM sin ir a la mitad;
 *   - las de 16 bits, sin signo o en deltas pasan a lo que vayan; los bucles
 *     de ida y vuelta, desenrollados (la vuelta, detrás);
 *   - la canción, con los patrones de cada pista, y las muestras, a trozos.
 *
 * Licencia MIT, como el resto del SDK.
 */
#include "zxm_conv.h"
#include "smpsynth.h"

#define SMP_MEM_MAX 0x18000u            /* la memoria de sonido (SMP_MEM), si no se dice otra */
#define ADPCM_TOP   5408                /* 96 kHz: (nota - 48) * 128 desde 8.363 Hz */

/* --- Leer el fichero -------------------------------------------------------------- */

uint8_t zxc_b_slow(zxm_conv_t *cv, uint32_t i)
{
    if (i >= cv->size || !cv->rd)
        return 0;
    uint32_t tag = i / cv->blk + 1, k;
    uint32_t old = 0;
    for (k = 0; k < cv->nblk && cv->tag[k] != tag; k++)
        if (cv->age[k] < cv->age[old])
            old = k;
    uint32_t at = (tag - 1) * cv->blk, n = cv->size - at < cv->blk ? cv->size - at : cv->blk;
    uint8_t *b = cv->cache + (k == cv->nblk ? old : k) * cv->blk;
    if (k == cv->nblk) {                /* no está: al que lleva más sin usarse */
        k = old;
        int r = cv->rd(cv->rd_ctx, at, b, n);
        if (r < 0) {
            cv->err = r;
            r = 0;
        }
        for (uint32_t j = (uint32_t)r; j < n; j++)
            b[j] = 0;
        cv->tag[k] = tag;
    }
    cv->age[k] = ++cv->clock;
    cv->last = k;
    cv->fast = b;
    cv->fast_at = at;
    cv->fast_n = n;
    return b[i - at];
}

/* La caché, en bloques de 4 KB (hasta ZXC_BLOCKS), o uno si es pequeña. */
static void set_cache(zxm_conv_t *cv, uint8_t *cache, uint32_t cap)
{
    cv->cache = cache;
    cv->cache_cap = cap;
    cv->nblk = cap / 4096 < 2 ? 1 : cap / 4096 > ZXC_BLOCKS ? ZXC_BLOCKS : cap / 4096;
    cv->blk = cv->nblk == 1 ? cap : 4096;
    for (uint32_t k = 0; k < ZXC_BLOCKS; k++)
        cv->tag[k] = cv->age[k] = 0;
    cv->last = 0;
    cv->fast_n = 0;                     /* nada a mano */
}

uint32_t zxc_le16(zxm_conv_t *cv, uint32_t i)
{
    return zxc_b(cv, i) | (uint32_t)zxc_b(cv, i + 1) << 8;
}

uint32_t zxc_le32(zxm_conv_t *cv, uint32_t i)
{
    return zxc_le16(cv, i) | zxc_le16(cv, i + 2) << 16;
}

uint32_t zxc_be16(zxm_conv_t *cv, uint32_t i)
{
    return (uint32_t)zxc_b(cv, i) << 8 | zxc_b(cv, i + 1);
}

void zxc_warn(zxm_conv_t *cv, unsigned bit, const char *msg)
{
    if (cv->warn && !(cv->said & bit))
        cv->warn(cv->warn_ctx, msg);
    cv->said |= bit;
}

uint32_t zxc_plays(const zxm_conv_t *cv, int p)
{
    uint32_t n = 0;
    for (int i = 0; i < cv->npos; i++)
        n += cv->order[i] == p;
    return n;
}

int zxc_pick_tracks(zxm_conv_t *cv, uint64_t used, const uint32_t *notes, int nch, uint8_t *chan)
{
    int n = 0;
    cv->ch_used = used;
    cv->ch_n = nch;
    for (int c = 0; c < 32; c++)
        cv->ch_notes[c] = c < nch ? notes[c] : 0;
    if (cv->only) {
        if ((cv->only & used) != cv->only)
            zxc_warn(cv, ZXC_W_TRACKS, "de los canales pedidos, los que no tienen notas (o no hay) no van");
        used &= cv->only;
    }
    for (int c = 0; c < nch; c++)
        n += (int)(used >> c & 1);
    if (n > ZXC_TRACKS) {
        uint64_t keep = 0;
        for (int k = 0; k < ZXC_TRACKS; k++) {
            int best = -1;
            for (int c = 0; c < nch; c++)
                if ((used >> c & 1) && !(keep >> c & 1) && (best < 0 || notes[c] > notes[best]))
                    best = c;
            keep |= (uint64_t)1 << best;
        }
        used = keep;
        zxc_warn(cv, ZXC_W_TRACKS, "más de 12 canales con notas: suenan los 12 que más notas tocan");
    }
    n = 0;
    for (int c = 0; c < nch; c++)
        if (used >> c & 1)
            chan[n++] = (uint8_t)c;
    if (!n)                             /* sin notas: una pista callada */
        chan[n++] = 0;
    return n;
}

void zxc_note(zxm_conv_t *cv, int s, int n)
{
    if (!cv->scanning || s < 0 || s >= cv->nsmp)
        return;
    zxc_smp_t *m = &cv->smp[s];
    int top = (n - 48 + m->rel) * 128 + m->fine;
    if (!m->used || top > m->top)
        m->top = (int16_t)top;
    m->used = 1;
}

/* 2^(1/1536) en 2.30. */
#define STEP_1536   1074226478u

void zxc_c4hz(zxc_smp_t *s, uint32_t hz)
{
    s->rel = s->fine = 0;
    if (!hz || hz == 8363)
        return;
    /* hz / 8363 en 2.30, llevado a [1, 2) con las octavas. */
    int t = 0;
    uint64_t r = ((uint64_t)hz << 30) / 8363u;
    while (r >= (2ull << 30)) {
        r >>= 1;
        t += 1536;
    }
    while (r < (1ull << 30)) {
        r <<= 1;
        t -= 1536;
    }
    uint64_t v = 1ull << 30;
    int k = 0;
    while (k < 1536) {
        uint64_t nv = v * STEP_1536 >> 30;
        if (nv > r) {
            if (r - v > nv - r)         /* el más cercano */
                k++;
            break;
        }
        v = nv;
        k++;
    }
    t += k;
    int rel = (t >= 0 ? t + 64 : t - 63) / 128;
    if (rel < -96)
        rel = -96;
    if (rel > 96)
        rel = 96;
    int fine = t - rel * 128;
    s->rel = (int8_t)rel;
    s->fine = (int8_t)(fine < -128 ? -128 : fine > 127 ? 127 : fine);
}

/* --- Cómo va cada muestra --------------------------------------------------------- */

/* Las muestras que da en el .zxm (antes de ir a la mitad). */
static uint32_t out_frames(const zxc_smp_t *s)
{
    if (!s->used || !s->frames)
        return 0;
    if (!s->loop_len)
        return s->frames;
    uint32_t le = s->loop_start + s->loop_len;
    return s->pingpong ? le + s->loop_len : le;
}

static uint32_t level_bytes(const zxc_smp_t *s, int level)
{
    uint32_t len = out_frames(s) >> (level == ZXC_HALF);
    if (level == ZXC_PCM8)
        return (len + 3) & ~3u;
    return (len + SMP_ADPCM_BLOCK - 1) / SMP_ADPCM_BLOCK * SMP_ADPCM_BYTES;
}

static uint32_t samples_total(const zxm_conv_t *cv)
{
    uint32_t t = 0;
    for (int s = 0; s < cv->nsmp; s++)
        t += level_bytes(&cv->smp[s], cv->smp[s].level);
    return t;
}

/* Si la muestra puede ir con ese nivel: el ADPCM tiene techo (y a la mitad
 * suena una octava más abajo). */
static int level_ok(const zxc_smp_t *s, int level)
{
    if (level == ZXC_ADPCM)
        return s->top <= ADPCM_TOP;
    return level != ZXC_HALF || s->top - 1536 <= ADPCM_TOP;
}

static void plan(zxm_conv_t *cv)
{
    for (int s = 0; s < cv->nsmp; s++)
        cv->smp[s].level = ZXC_PCM8;
    cv->how = 0;
    for (int level = ZXC_ADPCM; level <= ZXC_HALF; level++)
        while (samples_total(cv) > cv->smp_mem) {
            int big = -1;
            uint32_t bb = 0;
            for (int s = 0; s < cv->nsmp; s++) {
                zxc_smp_t *m = &cv->smp[s];
                uint32_t b = level_bytes(m, m->level);
                if (b && m->level < level && level_ok(m, level) && (big < 0 || b > bb)) {
                    big = s;
                    bb = b;
                }
            }
            if (big < 0)
                break;
            cv->smp[big].level = (uint8_t)level;
            cv->how |= level == ZXC_ADPCM ? ZXC_W_ADPCM : ZXC_W_HALF;
        }
    for (int s = cv->nsmp - 1; s >= 0 && samples_total(cv) > cv->smp_mem; s--)
        if (out_frames(&cv->smp[s])) {
            cv->smp[s].used = 0;
            cv->how |= ZXC_W_FIT;
        }
    uint32_t at = 0;
    for (int s = 0; s < cv->nsmp; s++) {
        cv->smp[s].at = at;
        at += level_bytes(&cv->smp[s], cv->smp[s].level);
    }
    cv->data_size = at;
    if (cv->how & ZXC_W_ADPCM)
        zxc_warn(cv, ZXC_W_ADPCM, "las muestras no caben en 8 bits: las más grandes van en ADPCM");
    if (cv->how & ZXC_W_HALF)
        zxc_warn(cv, ZXC_W_HALF, "ni en ADPCM: las más grandes, a la mitad de muestras (con menos agudos)");
    if (cv->how & ZXC_W_FIT)
        zxc_warn(cv, ZXC_W_FIT, "ni así caben todas: las últimas no suenan");
}

/* --- Abrir ------------------------------------------------------------------------ */

size_t zxm_conv_size(void)
{
    return sizeof(zxm_conv_t);
}

int zxm_conv_kind(const uint8_t *head, uint32_t n, uint32_t size)
{
    if (!head)
        return 0;
    if (zxc_xm_probe(head, n, size))
        return ZXC_XM;
    if (zxc_s3m_probe(head, n, size))
        return ZXC_S3M;
    if (zxc_mod_probe(head, n, size))
        return ZXC_MOD;
    return 0;
}

static int open_any(zxm_conv_t *cv, const uint8_t *head, uint32_t n)
{
    cv->kind = zxm_conv_kind(head, n, cv->size);
    int r = cv->kind == ZXC_XM ? zxc_xm_open(cv) : cv->kind == ZXC_S3M ? zxc_s3m_open(cv) :
            cv->kind == ZXC_MOD ? zxc_mod_open(cv) : -1;
    if (r || cv->err)
        return -1;
    zxc_scan(cv);
    if (cv->kind == ZXC_XM)
        zxc_xm_resolve(cv);
    plan(cv);
    return cv->err ? -1 : 0;
}

static void init(zxm_conv_t *cv, uint32_t size, zxms_warn_fn warn, void *wctx)
{
    uint8_t *p = (uint8_t *)cv;
    for (size_t i = 0; i < sizeof *cv; i++)
        p[i] = 0;
    cv->size = size;
    cv->warn = warn;
    cv->warn_ctx = wctx;
    cv->speed = 6;
    cv->tempo = 125;
    cv->gvol = 64;
    cv->smp_mem = SMP_MEM_MAX;
}

void zxm_conv_limit(zxm_conv_t *cv, uint32_t bytes)
{
    cv->smp_mem = bytes < SMP_MEM_MAX ? bytes : SMP_MEM_MAX;
    plan(cv);
}

int zxm_conv_open(zxm_conv_t *cv, zxm_read_fn rd, void *ctx, uint32_t size, uint8_t *cache, uint32_t cache_cap,
                  zxms_warn_fn warn, void *wctx)
{
    if (!rd || !cache || cache_cap < 1084)
        return -1;
    init(cv, size, warn, wctx);
    cv->rd = rd;
    cv->rd_ctx = ctx;
    set_cache(cv, cache, cache_cap);
    uint8_t head[1084];
    uint32_t n = size < sizeof head ? size : (uint32_t)sizeof head;
    for (uint32_t i = 0; i < n; i++)
        head[i] = zxc_b(cv, i);
    return open_any(cv, head, n);
}

void zxm_conv_cache(zxm_conv_t *cv, uint8_t *cache, uint32_t cache_cap)
{
    if (cv->rd && cache && cache_cap >= 64)
        set_cache(cv, cache, cache_cap);
}

int zxm_conv_channels(const zxm_conv_t *cv, uint64_t *used, uint64_t *picked, uint32_t notes[32])
{
    if (used)
        *used = cv->ch_used;
    if (picked) {
        *picked = 0;
        for (int t = 0; t < cv->tracks; t++)
            *picked |= (uint64_t)1 << cv->chan[t];
    }
    if (notes)
        for (int c = 0; c < 32; c++)
            notes[c] = cv->ch_notes[c];
    return cv->ch_n;
}

int zxm_conv_open_mem(zxm_conv_t *cv, const uint8_t *m, uint32_t n, zxms_warn_fn warn, void *wctx)
{
    return zxm_conv_open_mem_ch(cv, m, n, 0, warn, wctx);
}

int zxm_conv_open_mem_ch(zxm_conv_t *cv, const uint8_t *m, uint32_t n, uint64_t channels, zxms_warn_fn warn,
                         void *wctx)
{
    if (!m)
        return -1;
    init(cv, n, warn, wctx);
    cv->only = channels;
    cv->mem = m;
    cv->fast = m;                       /* entero a mano */
    cv->fast_at = 0;
    cv->fast_n = n;
    return open_any(cv, m, n < 1084 ? n : 1084);
}

/* --- Escribir la canción ---------------------------------------------------------- */

typedef struct {
    uint8_t *out;
    uint32_t cap, n;
} w_t;

static void put8(w_t *w, unsigned v)
{
    if (w->out && w->n < w->cap)
        w->out[w->n] = (uint8_t)v;
    w->n++;
}

static void put16(w_t *w, unsigned v)
{
    put8(w, v);
    put8(w, v >> 8);
}

static void put32(w_t *w, uint32_t v)
{
    put16(w, v);
    put16(w, v >> 16);
}

static void set32(w_t *w, uint32_t at, uint32_t v)
{
    for (int i = 0; i < 4; i++)
        if (w->out && at + (uint32_t)i < w->cap)
            w->out[at + (uint32_t)i] = (uint8_t)(v >> 8 * i);
}

static uint32_t L32(const uint8_t *p)
{
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void align4(w_t *w)
{
    while (w->n & 3)
        put8(w, 0);
}

/* Una pista de un patrón en los bytes del .zxm, según llegan sus filas: la
 * clase (1), las filas que guarda (u16, hasta la última con algo) y las
 * casillas, con las vacías contadas juntas (de 128 en 128 como mucho). Se
 * resume (FNV-1a) y se cuenta; al escribir, va a out desde at (la cabecera,
 * al acabar), o se compara con cmp (cmp_n bytes: una repetida). */
typedef struct {
    uint32_t h, n;                      /* el resumen y los bytes, sin la cabecera */
    int last;                           /* la última fila con algo */
    uint8_t *out;                       /* dónde (NULL: solo resumir y contar) */
    uint32_t at, cap;
    const uint8_t *cmp;
    uint32_t cmp_n;
    int bad;
} enc_t;

static void enc_init(enc_t *e)
{
    e->h = 2166136261u;
    e->n = 0;
    e->last = -1;
    e->out = NULL;
    e->cmp = NULL;
    e->bad = 0;
}

static void enc_byte(enc_t *e, unsigned v)
{
    uint32_t k = 3 + e->n++;
    v &= 0xFF;
    e->h = (e->h ^ v) * 16777619u;
    if (e->cmp) {
        if (k >= e->cmp_n || e->cmp[k] != v)
            e->bad = 1;
    } else if (e->out && e->at + k < e->cap) {
        e->out[e->at + k] = (uint8_t)v;
    }
}

static void enc_cell(enc_t *e, int row, const zxc_cell_t *c)
{
    if (!c->has)
        return;
    for (int gap = row - e->last - 1; gap > 0; gap -= 128)
        enc_byte(e, (unsigned)((gap > 128 ? 128 : gap) - 1));
    enc_byte(e, 0x80u | c->has);
    if (c->has & ZXM_C_NOTE)
        enc_byte(e, c->note);
    if (c->has & ZXM_C_INS)
        enc_byte(e, c->ins);
    if (c->has & ZXM_C_VOL)
        enc_byte(e, c->vol);
    for (int k = 0; k < 2; k++)
        if (c->has & (ZXM_C_FX1 << k)) {
            enc_byte(e, c->fx[k]);
            enc_byte(e, c->fxp[k]);
        }
    e->last = row;
}

/* Al acabar: la cabecera (en out, o comparada), y el resumen de todo. */
static uint32_t enc_end(enc_t *e)
{
    uint8_t hd[3] = { 1, (uint8_t)(e->last + 1), (uint8_t)((e->last + 1) >> 8) };
    uint32_t h = e->h;
    for (int k = 0; k < 3; k++) {
        h = (h ^ hd[k]) * 16777619u;
        if (e->cmp) {
            if ((uint32_t)k >= e->cmp_n || e->cmp[k] != hd[k])
                e->bad = 1;
        } else if (e->out && e->at + (uint32_t)k < e->cap) {
            e->out[e->at + (uint32_t)k] = hd[k];
        }
    }
    if (e->cmp && 3 + e->n != e->cmp_n)
        e->bad = 1;
    return h;
}

static void on_row(zxm_conv_t *cv, void *ctx, int row, const zxc_cell_t *cells)
{
    enc_t *e = ctx;
    for (int t = 0; t < cv->tracks; t++)
        enc_cell(&e[t], row, &cells[t]);
}

/* Las casillas de todos los patrones, una vez (cada patrón, recorrido una
 * vez): lo que suena (zxc_note), el resumen de cada pista y lo que ocupa; y
 * lo que ocupan las que no se repiten. Las repetidas, al primero igual (de
 * atrás adelante, para que los resúmenes de delante sigan ahí). */
void zxc_scan(zxm_conv_t *cv)
{
    enc_t e[ZXC_TRACKS];
    int tr = cv->tracks, n = cv->npat * tr;
    cv->scanning = 1;
    cv->pat_bytes = 0;
    for (int p = 0; p < cv->npat; p++) {
        for (int t = 0; t < tr; t++)
            enc_init(&e[t]);
        cv->rows(cv, p, on_row, e);
        for (int t = 0; t < tr; t++) {
            int i = p * tr + t, j = 0;
            cv->pat_key[i] = enc_end(&e[t]) & ~ZXC_DUP;
            cv->pat_len[i] = (uint16_t)(3 + e[t].n);
            while (j < i && cv->pat_key[j] != cv->pat_key[i])
                j++;
            if (j == i)
                cv->pat_bytes += 3 + e[t].n;
        }
    }
    cv->scanning = 0;
    for (int i = n - 1; i > 0; i--)
        for (int j = 0; j < i; j++)
            if (cv->pat_key[j] == cv->pat_key[i]) {
                cv->pat_key[i] = ZXC_DUP | (uint32_t)j;
                break;
            }
    for (int i = 0; i < n; i++)
        cv->pat_key[i] = cv->pat_key[i] & ZXC_DUP ? cv->pat_key[i] & ~ZXC_DUP : (uint32_t)i;
}

/* Los patrones, desde w->n, con su tabla en tab: cada patrón del fichero,
 * recorrido una vez, con las pistas a la vez, cada una en su sitio (lo que
 * ocupa, de zxc_scan). Las repetidas no se escriben: se comparan con la de
 * antes, y si no son iguales (dos resúmenes iguales por casualidad), la
 * canción no se puede hacer (cv->collide). */
static void write_patterns(zxm_conv_t *cv, w_t *w, uint32_t tab)
{
    if (!w->out) {
        w->n += cv->pat_bytes;          /* solo lo que ocupa */
        return;
    }
    enc_t e[ZXC_TRACKS];
    int tr = cv->tracks;
    for (int p = 0; p < cv->npat; p++) {
        for (int t = 0; t < tr; t++) {
            uint32_t i = (uint32_t)(p * tr + t), ref = cv->pat_key[i];
            enc_init(&e[t]);
            if (ref == i) {
                set32(w, tab + 4u * i, w->n);
                e[t].out = w->out;
                e[t].at = w->n;
                e[t].cap = w->cap;
                w->n += cv->pat_len[i];
            } else {
                uint32_t from = tab + 4u * ref + 4u <= w->cap ? L32(w->out + tab + 4u * ref) : w->cap;
                set32(w, tab + 4u * i, from);
                e[t].cmp = w->out + (from < w->cap ? from : 0);
                e[t].cmp_n = from + cv->pat_len[ref] <= w->cap ? cv->pat_len[ref] : 0;
            }
        }
        cv->rows(cv, p, on_row, e);
        for (int t = 0; t < tr; t++) {
            uint32_t i = (uint32_t)(p * tr + t);
            enc_end(&e[t]);
            if (e[t].bad || (!e[t].cmp && 3 + e[t].n != cv->pat_len[i]))
                cv->collide = 1;
        }
    }
}

static uint32_t sample_len(const zxc_smp_t *s)
{
    return out_frames(s) >> (s->level == ZXC_HALF);
}

static uint32_t sample_loop(const zxc_smp_t *s)
{
    uint32_t len = out_frames(s);
    if (!len || !s->loop_len)
        return sample_len(s);
    return s->loop_start >> (s->level == ZXC_HALF);
}

static uint32_t song(zxm_conv_t *cv, uint8_t *out, uint32_t cap)
{
    w_t w = { out, cap, 0 };
    int tr = cv->tracks;
    for (const char *s = ZXM_MAGIC; *s; s++)
        put8(&w, (uint8_t)*s);
    for (int i = 0; i < 32; i++) {
        uint8_t c = (uint8_t)cv->title[i];
        put8(&w, c >= 32 && c < 127 ? c : 0);
    }
    for (int i = 0; i < 32; i++)
        put8(&w, 0);
    put8(&w, 0);                        /* pistas del AY */
    put8(&w, (unsigned)tr);
    put8(&w, cv->speed);
    put8(&w, cv->tempo);
    put8(&w, cv->flags);
    put8(&w, 0);
    put8(&w, 255);
    put8(&w, cv->gvol);
    put16(&w, (unsigned)cv->npos);
    put16(&w, (unsigned)cv->loop);
    put16(&w, (unsigned)(cv->npat * tr));
    put8(&w, 0);                        /* instrumentos del AY */
    put8(&w, (unsigned)cv->nins);
    put8(&w, 0);                        /* ornamentos */
    put8(&w, (unsigned)cv->nsmp);
    put16(&w, 0);
    for (int c = 0; c < 12; c++)
        put8(&w, c < tr ? cv->pan[c] : 128);
    uint32_t offs = w.n, off[8];
    for (int i = 0; i < 8; i++)
        put32(&w, 0);

    off[ZXM_O_POSITIONS] = w.n;
    for (int p = 0; p < cv->npos; p++) {
        put16(&w, cv->pat_rows[cv->order[p]]);
        for (int t = 0; t < tr; t++)
            put16(&w, (unsigned)(cv->order[p] * tr + t));
    }
    align4(&w);
    off[ZXM_O_PATTERNS] = w.n;
    for (int i = 0; i < cv->npat * tr; i++)
        put32(&w, 0);
    off[ZXM_O_AYINS] = w.n;
    off[ZXM_O_SMPINS] = w.n;
    for (int i = 0; i < cv->nins; i++) {
        uint8_t ins[ZXM_SINS_SIZE];
        zxc_xm_instrument(cv, i, ins);
        for (int k = 0; k < ZXM_SINS_SIZE; k++)
            put8(&w, ins[k]);
    }
    off[ZXM_O_ORNS] = w.n;

    write_patterns(cv, &w, off[ZXM_O_PATTERNS]);
    align4(&w);

    off[ZXM_O_SAMPLES] = w.n;
    for (int s = 0; s < cv->nsmp; s++) {
        const zxc_smp_t *m = &cv->smp[s];
        int half = m->level == ZXC_HALF;
        put32(&w, m->at);
        put32(&w, sample_len(m));
        put32(&w, sample_loop(m));
        put8(&w, m->level != ZXC_PCM8 ? 1 : 0);
        put8(&w, (unsigned)(uint8_t)(int8_t)(m->rel - (half ? 12 : 0)));   /* a la mitad: una octava más grave */
        put8(&w, (unsigned)(uint8_t)m->fine);
        put8(&w, m->vol > 64 ? 64 : m->vol);
        put8(&w, m->pan);
        put8(&w, m->flags);
        put16(&w, 0);
    }
    off[ZXM_O_DATA] = w.n;
    off[ZXM_O_DATA_SIZE] = 0;
    for (int i = 0; i < 8; i++)
        set32(&w, offs + 4u * (uint32_t)i, off[i]);
    return w.n;
}

/* --- Las muestras ----------------------------------------------------------------- */

/* Una muestra del fichero, en 16 bits, en el orden en que va: la ida y, si
 * el bucle es de ida y vuelta, la vuelta. */
typedef struct {
    const zxc_smp_t *s;
    uint32_t pos, n;                    /* la siguiente y cuántas da */
    int32_t acc;                        /* en deltas: la última */
} gen_t;

static int bytes_per(int enc)
{
    return enc >= ZXC_S16 ? 2 : 1;
}

/* La muestra i tal como está en el fichero (en deltas, la diferencia). */
static int32_t raw(zxm_conv_t *cv, const zxc_smp_t *s, uint32_t i)
{
    uint32_t a = s->file_at + i * (uint32_t)bytes_per(s->enc);
    switch (s->enc) {
    case ZXC_U8: return ((int)zxc_b(cv, a) - 128) * 256;
    case ZXC_S8:
    case ZXC_D8: return (int8_t)zxc_b(cv, a) * 256;
    case ZXC_U16: return (int32_t)zxc_le16(cv, a) - 32768;
    default: return (int16_t)zxc_le16(cv, a);
    }
}

static int16_t gen_next(zxm_conv_t *cv, gen_t *g)
{
    const zxc_smp_t *s = g->s;
    uint32_t le = s->loop_len ? s->loop_start + s->loop_len : s->frames;
    int delta = s->enc == ZXC_D8 || s->enc == ZXC_D16;
    int32_t v;
    if (g->pos < le) {                  /* la ida */
        v = raw(cv, s, g->pos);
        if (delta) {
            g->acc = s->enc == ZXC_D8 ? (int8_t)((g->acc >> 8) + (v >> 8)) * 256 : (int16_t)(g->acc + v);
            v = g->acc;
        }
    } else {                            /* la vuelta: le - 1 hacia atrás */
        uint32_t j = le - 1 - (g->pos - le);
        if (delta) {
            v = g->acc;                 /* la j; la anterior, quitando su diferencia */
            int32_t d = raw(cv, s, j);
            g->acc = s->enc == ZXC_D8 ? (int8_t)((g->acc >> 8) - (d >> 8)) * 256 : (int16_t)(g->acc - d);
        } else {
            v = raw(cv, s, j);
        }
    }
    g->pos++;
    return (int16_t)(v < -32768 ? -32768 : v > 32767 ? 32767 : v);
}

typedef struct {
    zxm_put_fn put;
    void *ctx;
    uint8_t *buf;
    uint32_t cap, n, at;
    int err;
} out_t;

static void out_flush(out_t *o)
{
    if (o->n && !o->err) {
        int r = o->put(o->ctx, o->at, o->buf, o->n);
        if (r < 0)
            o->err = r;
    }
    o->at += o->n;
    o->n = 0;
}

static void out8(out_t *o, uint8_t v)
{
    o->buf[o->n++] = v;
    if (o->n == o->cap)
        out_flush(o);
}

int zxm_conv_samples(zxm_conv_t *cv, zxm_put_fn put, void *ctx, uint8_t *buf, uint32_t cap)
{
    cap &= ~3u;
    if (!put || !buf || cap < 64)
        return -1;
    out_t o = { put, ctx, buf, cap, 0, 0, 0 };
    for (int s = 0; s < cv->nsmp && !o.err; s++) {
        const zxc_smp_t *m = &cv->smp[s];
        uint32_t n = out_frames(m);
        if (!n)
            continue;
        gen_t g = { m, 0, n, 0 };
        if (m->level == ZXC_PCM8) {
            for (uint32_t i = 0; i < n; i++) {
                int v = (gen_next(cv, &g) + 128) >> 8;
                out8(&o, (uint8_t)(int8_t)(v > 127 ? 127 : v));
            }
            for (uint32_t i = n; i & 3; i++)
                out8(&o, 0);
        } else {
            uint32_t step = m->level == ZXC_HALF ? 2 : 1, total = n / step;
            int32_t index = 0;
            for (uint32_t i = 0; i < total; i += SMP_ADPCM_BLOCK) {
                int16_t blk[SMP_ADPCM_BLOCK];
                uint8_t enc[SMP_ADPCM_BYTES];
                uint32_t bm = total - i < SMP_ADPCM_BLOCK ? total - i : SMP_ADPCM_BLOCK;
                for (uint32_t j = 0; j < bm; j++) {
                    int32_t v = gen_next(cv, &g);
                    if (step == 2)
                        v = (v + gen_next(cv, &g)) / 2;
                    blk[j] = (int16_t)v;
                }
                smp_adpcm_block(blk, bm, &index, enc);
                for (int j = 0; j < SMP_ADPCM_BYTES; j++)
                    out8(&o, enc[j]);
            }
        }
    }
    out_flush(&o);
    return cv->err ? cv->err : o.err;
}

uint32_t zxm_conv_song(zxm_conv_t *cv, uint8_t *out, uint32_t cap)
{
    uint32_t n = song(cv, out, cap);
    return cv->err || cv->collide ? 0 : n;
}

/* Con los datos detrás (zxv-music). */
typedef struct {
    uint8_t *out;
    uint32_t cap, base;
} whole_t;

static int whole_put(void *ctx, uint32_t at, const uint8_t *buf, uint32_t n)
{
    whole_t *w = ctx;
    for (uint32_t i = 0; i < n; i++)
        if (w->out && w->base + at + i < w->cap)
            w->out[w->base + at + i] = buf[i];
    return 0;
}

uint32_t zxm_conv_whole(zxm_conv_t *cv, uint8_t *out, uint32_t cap)
{
    uint32_t n = song(cv, out, cap);
    if (cv->err || cv->collide)
        return 0;
    if (out && n <= cap) {
        uint8_t buf[2048];
        whole_t w = { out, cap, n };
        if (zxm_conv_samples(cv, whole_put, &w, buf, sizeof buf))
            return 0;
        w_t o = { out, cap, 0 };
        set32(&o, ZXM_H_OFFSETS + 4 * ZXM_O_DATA_SIZE, cv->data_size);
    }
    return n + cv->data_size;
}
