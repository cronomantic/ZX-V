/*
 * zxm_song.c - Una canción .zxm en memoria y sus bytes (ver zxm_song.h).
 * Licencia MIT, como el resto del SDK.
 */
#include "zxm_song.h"

#include <stdlib.h>
#include <string.h>

void zxms_init(zxms_t *s)
{
    memset(s, 0, sizeof *s);
    s->speed = 6;
    s->tempo = 125;
    s->smp_volume = 255;
    s->global_volume = 64;
    s->ay2 = -1;
    for (int i = 0; i < 12; i++)
        s->pan[i] = 128;
}

void zxms_free(zxms_t *s)
{
    for (int i = 0; i < s->npat; i++)
        free(s->pat[i].cells);
    free(s->pat);
    for (int i = 0; i < 63; i++)
        free(s->ayins[i].data);
    for (int i = 0; i < 64; i++)
        free(s->orns[i].data);
    for (int i = 0; i < s->n_samples; i++)
        free(s->samples[i].data);
    free(s->samples);
    free(s->smpins);
    memset(s, 0, sizeof *s);
}

int zxms_add_smp_instrument(zxms_t *s, int smp)
{
    if (s->n_smpins >= 128)
        return -1;
    uint8_t (*p)[ZXM_SINS_SIZE] = realloc(s->smpins, (size_t)(s->n_smpins + 1) * ZXM_SINS_SIZE);
    if (!p)
        return -1;
    s->smpins = p;
    memset(p[s->n_smpins], 0, ZXM_SINS_SIZE);
    memset(p[s->n_smpins] + ZXM_SI_MAP, smp, 96);
    return ++s->n_smpins;
}

int zxms_add_sample(zxms_t *s, const zxms_sample_t *smp)
{
    if (s->n_samples >= 255)
        return -1;
    zxms_sample_t *p = realloc(s->samples, (size_t)(s->n_samples + 1) * sizeof *p);
    if (!p)
        return -1;
    s->samples = p;
    p[s->n_samples] = *smp;
    p[s->n_samples].data = malloc(smp->size ? smp->size : 1);
    if (!p[s->n_samples].data)
        return -1;
    memcpy(p[s->n_samples].data, smp->data, smp->size);
    return s->n_samples++;
}

int zxms_add_pattern(zxms_t *s, int kind, int rows)
{
    zxms_pattern_t *p = realloc(s->pat, (size_t)(s->npat + 1) * sizeof *p);
    if (!p)
        return -1;
    s->pat = p;
    p[s->npat].kind = (uint8_t)kind;
    p[s->npat].rows = rows;
    p[s->npat].cells = calloc((size_t)(rows ? rows : 1), sizeof(zxms_cell_t));
    return s->pat[s->npat].cells ? s->npat++ : -1;
}

/* Las filas que hay que guardar: hasta la última con algo. */
static int used_rows(const zxms_pattern_t *p)
{
    int n = p->rows;
    while (n > 0 && !p->cells[n - 1].has)
        n--;
    return n;
}

static int same(const zxms_pattern_t *a, const zxms_pattern_t *b)
{
    int n = used_rows(a);
    if (a->kind != b->kind || n != used_rows(b))
        return 0;
    for (int i = 0; i < n; i++)
        if (memcmp(&a->cells[i], &b->cells[i], sizeof(zxms_cell_t)))
            return 0;
    return 1;
}

void zxms_dedup(zxms_t *s)
{
    int *map = malloc((size_t)(s->npat ? s->npat : 1) * sizeof *map), n = 0;
    char *used = calloc((size_t)(s->npat ? s->npat : 1), 1);
    if (!map || !used) {
        free(map);
        free(used);
        return;
    }
    for (int p = 0; p < s->npos; p++)
        for (int t = 0; t < ZXMS_TRACKS; t++)
            if (s->pos[p].pat[t] >= 0)
                used[s->pos[p].pat[t]] = 1;
    for (int i = 0; i < s->npat; i++) {
        map[i] = -1;
        if (!used[i]) {
            free(s->pat[i].cells);
            continue;
        }
        for (int j = 0; j < n; j++)
            if (same(&s->pat[j], &s->pat[i])) {
                map[i] = j;
                break;
            }
        if (map[i] >= 0) {
            free(s->pat[i].cells);
            continue;
        }
        s->pat[n] = s->pat[i];
        map[i] = n++;
    }
    s->npat = n;
    for (int p = 0; p < s->npos; p++)
        for (int t = 0; t < ZXMS_TRACKS; t++)
            if (s->pos[p].pat[t] >= 0)
                s->pos[p].pat[t] = map[s->pos[p].pat[t]];
    free(map);
    free(used);
}

/* --- Los bytes ---------------------------------------------------------------------- */

typedef struct {
    uint8_t *b;
    size_t n, cap;
    int fail;
} buf_t;

static void put(buf_t *o, const void *d, size_t n)
{
    if (o->fail)
        return;
    if (o->n + n > o->cap) {
        size_t cap = o->cap ? o->cap * 2 : 4096;
        while (cap < o->n + n)
            cap *= 2;
        uint8_t *b = realloc(o->b, cap);
        if (!b) {
            o->fail = 1;
            return;
        }
        o->b = b;
        o->cap = cap;
    }
    memcpy(o->b + o->n, d, n);
    o->n += n;
}

static void put8(buf_t *o, unsigned v)
{
    uint8_t b = (uint8_t)v;
    put(o, &b, 1);
}

static void put16(buf_t *o, unsigned v)
{
    put8(o, v);
    put8(o, v >> 8);
}

static void put32(buf_t *o, uint32_t v)
{
    put16(o, v);
    put16(o, v >> 16);
}

static void set32(buf_t *o, size_t at, uint32_t v)
{
    if (!o->fail)
        for (int i = 0; i < 4; i++)
            o->b[at + i] = (uint8_t)(v >> 8 * i);
}

static void align4(buf_t *o)
{
    while (o->n & 3)
        put8(o, 0);
}

static void put_cell(buf_t *o, const zxms_cell_t *c)
{
    put8(o, 0x80 | (c->has & 0x7F));
    if (c->has & ZXM_C_NOTE)
        put8(o, c->note);
    if (c->has & ZXM_C_INS)
        put8(o, c->ins);
    if (c->has & ZXM_C_VOL)
        put8(o, c->vol);
    if (c->has & ZXM_C_ORN)
        put8(o, c->orn);
    if (c->has & ZXM_C_ENV) {
        put8(o, c->env);
        put16(o, c->env_period);
    }
    for (int k = 0; k < 2; k++)
        if (c->has & (ZXM_C_FX1 << k)) {
            put8(o, c->fx[k]);
            put(o, c->fxp[k], (size_t)zxm_fx_len(c->fx[k]));
        }
}

static void put_pattern(buf_t *o, const zxms_pattern_t *p)
{
    int rows = used_rows(p);
    put8(o, p->kind);
    put16(o, (unsigned)rows);
    for (int r = 0; r < rows;) {
        if (p->cells[r].has) {
            put_cell(o, &p->cells[r++]);
            continue;
        }
        int n = 0;
        while (r + n < rows && !p->cells[r + n].has && n < 128)
            n++;
        put8(o, (unsigned)(n - 1));
        r += n;
    }
}

/* Una tabla de u32 (dónde empieza cada bloque) y los bloques detrás: los
 * iguales, una vez (en un PT3, varios instrumentos suelen ser el mismo); y
 * los que no usa ninguna casilla (used[i] 0), donde el primero que sí: los
 * números no cambian, y las casillas tampoco. */
static void put_blobs(buf_t *o, const zxms_blob_t *b, int n, const uint8_t *used)
{
    size_t table = o->n;
    uint32_t at[64];
    int keep = -1;
    for (int i = 0; i < n; i++)
        put32(o, 0);
    for (int i = 0; i < n; i++) {
        at[i] = UINT32_MAX;
        if (!used[i] && !(keep < 0 && i == n - 1))
            continue;                   /* (si no se usa ninguno, el último va) */
        int j = 0;
        while (j < i && !(at[j] != UINT32_MAX && b[j].size == b[i].size &&
                          !memcmp(b[j].data, b[i].data, (size_t)b[i].size)))
            j++;
        if (j < i) {
            at[i] = at[j];
        } else {
            at[i] = (uint32_t)o->n;
            put(o, b[i].data, (size_t)b[i].size);
        }
        if (keep < 0)
            keep = i;
    }
    for (int i = 0; i < n; i++)
        set32(o, table + 4u * (unsigned)i, at[i] != UINT32_MAX ? at[i] : at[keep]);
    align4(o);
}

/* Los instrumentos del AY y los ornamentos que usa alguna casilla (y los
 * de partida: el instrumento 0 y el 1 y el ornamento 0, los del PT3 al
 * empezar). */
static void used_blobs(const zxms_t *s, uint8_t *ins, uint8_t *orn)
{
    memset(ins, 0, 64);
    memset(orn, 0, 64);
    ins[0] = ins[1] = orn[0] = 1;
    for (int p = 0; p < s->npat; p++)
        for (int r = 0; r < s->pat[p].rows; r++) {
            const zxms_cell_t *c = &s->pat[p].cells[r];
            if (s->pat[p].kind == 0 && (c->has & ZXM_C_INS) && c->ins < 64)
                ins[c->ins] = 1;
            if ((c->has & ZXM_C_ORN) && c->orn < 64)
                orn[c->orn] = 1;
        }
}

uint8_t *zxm_from_module(const uint8_t *m, uint32_t n, size_t *size, zxms_warn_fn warn, void *ctx)
{
    return zxm_from_module_mem(m, n, UINT32_MAX, size, warn, ctx);
}

uint8_t *zxm_from_module_mem(const uint8_t *m, uint32_t n, uint32_t smp_mem, size_t *size, zxms_warn_fn warn,
                             void *ctx)
{
    return zxm_from_module_ch(m, n, smp_mem, 0, size, warn, ctx);
}

uint8_t *zxm_from_module_ch(const uint8_t *m, uint32_t n, uint32_t smp_mem, uint64_t channels, size_t *size,
                            zxms_warn_fn warn, void *ctx)
{
    zxm_conv_t *cv = malloc(zxm_conv_size());
    uint8_t *out = NULL;
    uint32_t len = 0;
    int ok = cv && !zxm_conv_open_mem_ch(cv, m, n, channels, warn, ctx);
    if (ok && smp_mem != UINT32_MAX)
        zxm_conv_limit(cv, smp_mem);
    if (ok && (len = zxm_conv_whole(cv, NULL, 0)) != 0 && (out = malloc(len)) != NULL &&
        zxm_conv_whole(cv, out, len) != len) {
        free(out);
        out = NULL;
    }
    free(cv);
    if (out && size)
        *size = len;
    return out;
}

int zxm_module_channels(const uint8_t *m, uint32_t n, uint64_t *used, uint64_t *picked, uint32_t notes[32])
{
    zxm_conv_t *cv = malloc(zxm_conv_size());
    int nch = cv && !zxm_conv_open_mem(cv, m, n, NULL, NULL) ? zxm_conv_channels(cv, used, picked, notes) : 0;
    free(cv);
    return nch;
}

uint8_t *zxms_save(const zxms_t *s, size_t *size)
{
    buf_t o = { 0 };
    int tracks = s->ay_tracks + s->smp_tracks;
    uint8_t h[ZXM_HEADER] = { 0 };
    memcpy(h, ZXM_MAGIC, 8);
    for (int i = 0; i < 32 && s->title[i]; i++)
        h[ZXM_H_TITLE + i] = (uint8_t)s->title[i];
    for (int i = 0; i < 32 && s->author[i]; i++)
        h[ZXM_H_AUTHOR + i] = (uint8_t)s->author[i];
    h[ZXM_H_AY] = (uint8_t)s->ay_tracks;
    h[ZXM_H_SMP] = (uint8_t)s->smp_tracks;
    h[ZXM_H_SPEED] = (uint8_t)s->speed;
    h[ZXM_H_TEMPO] = (uint8_t)s->tempo;
    h[ZXM_H_FLAGS] = (uint8_t)s->flags;
    h[ZXM_H_AYTABLE] = (uint8_t)s->ay_table;
    h[ZXM_H_SMPVOL] = (uint8_t)s->smp_volume;
    h[ZXM_H_GLOBAL] = (uint8_t)s->global_volume;
    h[ZXM_H_POSITIONS] = (uint8_t)s->npos;
    h[ZXM_H_POSITIONS + 1] = (uint8_t)(s->npos >> 8);
    h[ZXM_H_LOOP] = (uint8_t)s->loop;
    h[ZXM_H_LOOP + 1] = (uint8_t)(s->loop >> 8);
    h[ZXM_H_PATTERNS] = (uint8_t)s->npat;
    h[ZXM_H_PATTERNS + 1] = (uint8_t)(s->npat >> 8);
    h[ZXM_H_AYINS] = (uint8_t)s->n_ayins;
    h[ZXM_H_ORNS] = (uint8_t)s->n_orns;
    h[ZXM_H_SMPINS] = (uint8_t)s->n_smpins;
    h[ZXM_H_SAMPLES] = (uint8_t)s->n_samples;
    h[ZXM_H_AY2] = (uint8_t)(s->ay2 >= 0 ? s->ay2 : (s->ay_table & 3) | (s->flags & 0x70));
    memcpy(h + ZXM_H_PAN, s->pan, 12);
    put(&o, h, sizeof h);

    uint32_t off[8] = { 0 };
    off[ZXM_O_POSITIONS] = (uint32_t)o.n;
    for (int p = 0; p < s->npos; p++) {
        put16(&o, (unsigned)s->pos[p].rows);
        for (int t = 0; t < tracks; t++)
            put16(&o, s->pos[p].pat[t] < 0 ? 0xFFFFu : (unsigned)s->pos[p].pat[t]);
    }
    align4(&o);
    off[ZXM_O_PATTERNS] = (uint32_t)o.n;
    for (int i = 0; i < s->npat; i++)
        put32(&o, 0);
    off[ZXM_O_AYINS] = (uint32_t)o.n;
    uint8_t used_ins[64], used_orn[64];
    used_blobs(s, used_ins, used_orn);
    put_blobs(&o, s->ayins, s->n_ayins, used_ins);
    off[ZXM_O_SMPINS] = (uint32_t)o.n;
    if (s->n_smpins)
        put(&o, s->smpins, (size_t)s->n_smpins * ZXM_SINS_SIZE);
    off[ZXM_O_ORNS] = (uint32_t)o.n;
    put_blobs(&o, s->orns, s->n_orns, used_orn);
    for (int i = 0; i < s->npat; i++) {
        set32(&o, off[ZXM_O_PATTERNS] + 4u * (unsigned)i, (uint32_t)o.n);
        put_pattern(&o, &s->pat[i]);
    }
    align4(&o);
    /* La tabla de muestras y, al final, sus datos (cada una desde un múltiplo
     * de 4). */
    off[ZXM_O_SAMPLES] = (uint32_t)o.n;
    uint32_t at = 0;
    for (int i = 0; i < s->n_samples; i++) {
        const zxms_sample_t *m = &s->samples[i];
        put32(&o, at);
        put32(&o, m->len);
        put32(&o, m->loop);
        put8(&o, m->format);
        put8(&o, (unsigned)(uint8_t)m->relnote);
        put8(&o, (unsigned)(uint8_t)m->fine);
        put8(&o, m->vol);
        put8(&o, m->pan);
        put8(&o, m->flags);
        put16(&o, 0);
        at += (m->size + 3) & ~3u;
    }
    off[ZXM_O_DATA] = (uint32_t)o.n;
    for (int i = 0; i < s->n_samples; i++) {
        put(&o, s->samples[i].data, s->samples[i].size);
        align4(&o);
    }
    off[ZXM_O_DATA_SIZE] = (uint32_t)o.n - off[ZXM_O_DATA];
    for (int i = 0; i < 8; i++)
        set32(&o, ZXM_H_OFFSETS + 4u * (unsigned)i, off[i]);
    if (o.fail) {
        free(o.b);
        return NULL;
    }
    *size = o.n;
    return o.b;
}

/* --- Leer un .zxm ------------------------------------------------------------------- */

typedef struct {
    const uint8_t *m;
    size_t n;
} rd_t;

static int rd_ok(const rd_t *r, size_t at, size_t len) { return at <= r->n && len <= r->n - at; }
static unsigned rd8(const rd_t *r, size_t at) { return at < r->n ? r->m[at] : 0; }
static unsigned rd16(const rd_t *r, size_t at) { return rd8(r, at) | rd8(r, at + 1) << 8; }
static uint32_t rd32(const rd_t *r, size_t at) { return rd16(r, at) | (uint32_t)rd16(r, at + 2) << 16; }

/* Un bloque de la tabla de u32 en at (el i-ésimo): u8 vuelta, u8 largo y
 * largo x line bytes. */
static int load_blob(const rd_t *r, size_t table, int i, int line, zxms_blob_t *b)
{
    uint32_t at = rd32(r, table + 4u * (unsigned)i);
    if (!rd_ok(r, at, 2))
        return -1;
    size_t size = 2u + rd8(r, at + 1) * (size_t)line;
    if (!rd_ok(r, at, size) || !(b->data = malloc(size)))
        return -1;
    memcpy(b->data, r->m + at, size);
    b->size = (int)size;
    return 0;
}

/* Un patrón desde at: sus casillas, en p (con rows filas ya puestas). */
static int load_pattern(const rd_t *r, size_t at, zxms_pattern_t *p)
{
    if (!rd_ok(r, at, 3))
        return -1;
    p->kind = (uint8_t)rd8(r, at);
    unsigned stored = rd16(r, at + 1);
    if (p->rows < (int)stored)
        p->rows = (int)stored;
    if (!(p->cells = calloc((size_t)(p->rows ? p->rows : 1), sizeof(zxms_cell_t))))
        return -1;
    size_t q = at + 3;
    for (unsigned row = 0; row < stored;) {
        if (!rd_ok(r, q, 1))
            return -1;
        unsigned b = rd8(r, q++);
        if (b < 0x80) {
            row += b + 1;
            continue;
        }
        zxms_cell_t *c = &p->cells[row++];
        c->has = (uint8_t)(b & 0x7F);
        if (c->has & ZXM_C_NOTE)
            c->note = (uint8_t)rd8(r, q++);
        if (c->has & ZXM_C_INS)
            c->ins = (uint8_t)rd8(r, q++);
        if (c->has & ZXM_C_VOL)
            c->vol = (uint8_t)rd8(r, q++);
        if (c->has & ZXM_C_ORN)
            c->orn = (uint8_t)rd8(r, q++);
        if (c->has & ZXM_C_ENV) {
            c->env = (uint8_t)rd8(r, q);
            c->env_period = (uint16_t)rd16(r, q + 1);
            q += 3;
        }
        for (int k = 0; k < 2; k++)
            if (c->has & (ZXM_C_FX1 << k)) {
                c->fx[k] = (uint8_t)rd8(r, q++);
                int len = zxm_fx_len(c->fx[k]);
                for (int i = 0; i < len; i++)
                    c->fxp[k][i] = (uint8_t)rd8(r, q++);
            }
        if (!rd_ok(r, q, 0))
            return -1;
    }
    return 0;
}

int zxms_load(zxms_t *s, const uint8_t *m, size_t n)
{
    zxms_init(s);
    rd_t r = { m, n };
    if (n < ZXM_HEADER || memcmp(m, ZXM_MAGIC, 8))
        return -1;
    for (int i = 0; i < 32; i++) {
        s->title[i] = (char)m[ZXM_H_TITLE + i];
        s->author[i] = (char)m[ZXM_H_AUTHOR + i];
    }
    s->ay_tracks = m[ZXM_H_AY];
    s->smp_tracks = m[ZXM_H_SMP];
    s->speed = m[ZXM_H_SPEED];
    s->tempo = m[ZXM_H_TEMPO];
    s->flags = m[ZXM_H_FLAGS];
    s->ay_table = m[ZXM_H_AYTABLE];
    s->smp_volume = m[ZXM_H_SMPVOL];
    s->global_volume = m[ZXM_H_GLOBAL];
    s->npos = (int)rd16(&r, ZXM_H_POSITIONS);
    s->loop = (int)rd16(&r, ZXM_H_LOOP);
    int npat = (int)rd16(&r, ZXM_H_PATTERNS);
    s->n_ayins = m[ZXM_H_AYINS];
    s->n_smpins = m[ZXM_H_SMPINS];
    s->n_orns = m[ZXM_H_ORNS];
    int nsmp = m[ZXM_H_SAMPLES];
    /* El segundo AY: -1 si es como el primero (así cambia con él). */
    int ay2 = m[ZXM_H_AY2];
    s->ay2 = ay2 == ((s->ay_table & 3) | (s->flags & 0x70)) ? -1 : ay2;
    memcpy(s->pan, m + ZXM_H_PAN, 12);
    uint32_t off[8];
    for (int i = 0; i < 8; i++)
        off[i] = rd32(&r, ZXM_H_OFFSETS + 4u * (unsigned)i);
    int tracks = s->ay_tracks + s->smp_tracks;
    if ((s->ay_tracks != 0 && s->ay_tracks != 3 && s->ay_tracks != 6) || s->smp_tracks > 12 || s->npos < 1 ||
        s->npos > ZXMS_POSITIONS || s->n_ayins > 63 || s->n_orns > 64 || s->n_smpins > 128 ||
        !rd_ok(&r, off[ZXM_O_POSITIONS], (size_t)s->npos * (2u + 2u * (unsigned)tracks)) ||
        !rd_ok(&r, off[ZXM_O_PATTERNS], 4u * (unsigned)npat))
        return -1;

    /* Las posiciones, y las filas de cada patrón: las de la más larga que lo usa. */
    int *rows = calloc((size_t)(npat ? npat : 1), sizeof *rows);
    if (!rows)
        return -1;
    for (int p = 0; p < s->npos; p++) {
        size_t at = off[ZXM_O_POSITIONS] + (size_t)p * (2u + 2u * (unsigned)tracks);
        s->pos[p].rows = (int)rd16(&r, at);
        for (int t = 0; t < ZXMS_TRACKS; t++) {
            unsigned pat = t < tracks ? rd16(&r, at + 2 + 2u * (unsigned)t) : 0xFFFFu;
            s->pos[p].pat[t] = pat == 0xFFFFu || (int)pat >= npat ? -1 : (int)pat;
            if (s->pos[p].pat[t] >= 0 && rows[pat] < s->pos[p].rows)
                rows[pat] = s->pos[p].rows;
        }
    }
    int bad = !(s->pat = calloc((size_t)(npat ? npat : 1), sizeof *s->pat));
    for (int i = 0; !bad && i < npat; i++) {
        s->npat = i + 1;
        s->pat[i].rows = rows[i];
        bad = load_pattern(&r, rd32(&r, off[ZXM_O_PATTERNS] + 4u * (unsigned)i), &s->pat[i]);
    }
    free(rows);
    if (bad)
        return -1;

    /* Los instrumentos del AY, los ornamentos y los de muestras. */
    for (int i = 0; i < s->n_ayins; i++)
        if (load_blob(&r, off[ZXM_O_AYINS], i, 4, &s->ayins[i]))
            return -1;
    for (int i = 0; i < s->n_orns; i++)
        if (load_blob(&r, off[ZXM_O_ORNS], i, 1, &s->orns[i]))
            return -1;
    if (s->n_smpins) {
        if (!rd_ok(&r, off[ZXM_O_SMPINS], (size_t)s->n_smpins * ZXM_SINS_SIZE) ||
            !(s->smpins = malloc((size_t)s->n_smpins * ZXM_SINS_SIZE)))
            return -1;
        memcpy(s->smpins, m + off[ZXM_O_SMPINS], (size_t)s->n_smpins * ZXM_SINS_SIZE);
    }

    /* Las muestras, con sus datos (los del fichero: un .zxm sin ellos no vale). */
    if (!rd_ok(&r, off[ZXM_O_SAMPLES], (size_t)nsmp * ZXM_SMP_SIZE))
        return -1;
    for (int i = 0; i < nsmp; i++) {
        size_t at = off[ZXM_O_SAMPLES] + (size_t)i * ZXM_SMP_SIZE;
        zxms_sample_t smp = { 0 };
        uint32_t start = rd32(&r, at + ZXM_S_START);
        smp.len = rd32(&r, at + ZXM_S_LEN);
        smp.loop = rd32(&r, at + ZXM_S_LOOP);
        smp.format = (uint8_t)rd8(&r, at + ZXM_S_FORMAT);
        smp.relnote = (int8_t)rd8(&r, at + ZXM_S_RELNOTE);
        smp.fine = (int8_t)rd8(&r, at + ZXM_S_FINE);
        smp.vol = (uint8_t)rd8(&r, at + ZXM_S_VOL);
        smp.pan = (uint8_t)rd8(&r, at + ZXM_S_PAN);
        smp.flags = (uint8_t)rd8(&r, at + ZXM_S_FLAGS);
        uint64_t size = smp.format ? ((uint64_t)smp.len + 63) / 64 * 36 : smp.len;
        if (smp.format > 1 || start > off[ZXM_O_DATA_SIZE] || size > off[ZXM_O_DATA_SIZE] - start ||
            !rd_ok(&r, (size_t)off[ZXM_O_DATA] + start, (size_t)size))
            return -1;
        smp.size = (uint32_t)size;
        smp.data = (uint8_t *)(m + off[ZXM_O_DATA] + start);
        if (zxms_add_sample(s, &smp) < 0)
            return -1;
    }
    return 0;
}
