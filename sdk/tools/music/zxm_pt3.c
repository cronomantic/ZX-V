/*
 * zxm_pt3.c - Un PT3 o un TurboSound a una canción .zxm (zxm_song.h).
 *
 * Hace lo que pt3_step con los patrones, fila a fila (cada canal con sus
 * filas por nota, y el canal A diciendo dónde acaba la posición), y apunta
 * la casilla de cada canal en vez de hacerla. Los instrumentos y los
 * ornamentos pasan byte a byte. Lo que hacen las casillas lo hace el mismo
 * código en los dos (sdk/music/pt3.c), así que la canción suena registro a
 * registro igual que el PT3 (lo comprueba emulator/tests/music/zxm_test.c),
 * hasta que vuelve a empezar: al volver, los canales B y C pueden ir
 * desfasados respecto a la primera vez, y el .zxm repite la primera.
 *
 * Un TurboSound son dos PT3, uno por AY, cada uno con su tempo y sus
 * posiciones; el .zxm tiene uno: se convierte si los dos van igual (lo
 * normal: se hacen juntos en Vortex Tracker).
 *
 * Licencia MIT, como el resto del SDK.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "music_int.h"
#include "zxm_song.h"

#define MAX_ROWS    256         /* filas por posición en un .zxm */

/* Lo que se apunta de cada fila de un PT3. */
typedef struct {
    zxms_cell_t c[3];
    uint8_t delay;              /* la velocidad tras la fila */
} row_t;

typedef struct {
    int rows;
    row_t *row;                 /* las de una posición */
} pos_t;

typedef struct {
    pt3_t h;                    /* la cabecera, leída por pt3_init */
    int npos;
    pos_t pos[256];
} half_t;

#define B(i)    mus_b(m, n, i)
#define W(i)    mus_w(m, n, i)

typedef struct {
    zxms_warn_fn fn;
    void *ctx;
    unsigned said;              /* lo que ya ha dicho (bit por cosa) */
} warn_t;

static void warn(warn_t *w, unsigned bit, const char *msg)
{
    if (w->fn && !(w->said & bit))
        w->fn(w->ctx, msg);
    w->said |= bit;
}

enum { W_FX = 1, W_ROWS = 4, W_BROKEN = 8, W_INS = 16 };

/* Un efecto en la casilla: los dos huecos, del último al primero (el PT3 los
 * hace en ese orden). fx: los que lleva ya, en el orden en que aparecen. */
typedef struct {
    uint8_t code, p[3];
} fx_t;

/* Quitar la envolvente en el canal: si la casilla ya la había puesto, se
 * queda puesta en el AY (su forma y su periodo, y R13 se escribe). */
static void env_off(zxms_cell_t *c)
{
    c->env = c->env & 15 ? (uint8_t)(c->env | ZXM_ENV_SET_ONLY) : 0;
}

/* Las órdenes de una fila de un canal, como pattern() en pt3.c, pero
 * apuntadas. Da dónde sigue el canal. */
static uint16_t parse(const uint8_t *m, uint32_t n, uint16_t a, zxms_cell_t *c, uint8_t *skip, uint8_t *delay,
                      warn_t *w)
{
    int order[9] = { 0 }, cnt = 0;      /* el efecto 1-9 que va en cada sitio */
    fx_t fx[12];
    int nfx = 0;
    memset(c, 0, sizeof *c);
    for (int quit = 0; !quit; a++) {
        uint8_t v = B(a);
        if (v >= 0xF0) {
            c->has |= ZXM_C_ORN | ZXM_C_INS | ZXM_C_ENV;
            c->orn = v - 0xF0;
            c->ins = B(++a) / 2;
            env_off(c);
        } else if (v >= 0xD1) {
            c->has |= ZXM_C_INS;
            c->ins = v - 0xD0;
        } else if (v == 0xD0) {
            quit = 1;
        } else if (v >= 0xC1) {
            c->has |= ZXM_C_VOL;
            c->vol = v - 0xC0;
        } else if (v == 0xC0) {
            c->has |= ZXM_C_NOTE;
            c->note = ZXM_NOTE_CUT;
            quit = 1;
        } else if (v >= 0xB2) {
            c->has |= ZXM_C_ENV;
            c->env = v - 0xB1;
            c->env_period = (uint16_t)(B(a + 1) << 8 | B(a + 2));
            a += 2;
        } else if (v == 0xB1) {
            *skip = B(++a);
        } else if (v == 0xB0) {
            c->has |= ZXM_C_ENV;
            env_off(c);
        } else if (v >= 0x50) {
            c->has |= ZXM_C_NOTE;
            c->note = (uint8_t)(v - 0x50 + 1);
            quit = 1;
        } else if (v >= 0x40) {
            c->has |= ZXM_C_ORN;
            c->orn = v - 0x40;
        } else if (v >= 0x20) {
            if (nfx < 12)
                fx[nfx++] = (fx_t){ 'n', { (uint8_t)(v - 0x20), 0, 0 } };
        } else if (v >= 0x10) {
            c->has |= ZXM_C_ENV | ZXM_C_INS;
            if (v == 0x10)
                env_off(c);
            else {
                c->env = v - 0x10;
                c->env_period = (uint16_t)(B(a + 1) << 8 | B(a + 2));
                a += 2;
            }
            c->ins = B(++a) / 2;
        } else if (v >= 1 && v <= 9 && v != 6 && v != 7 && cnt < 9) {
            order[cnt++] = v;
        }
    }
    /* Los parámetros, tras la nota, del último al primero. */
    for (int i = cnt - 1; i >= 0; i--) {
        fx_t f = { 0, { 0, 0, 0 } };
        switch (order[i]) {
        case 1:
            f = (fx_t){ 'g', { B(a), B(a + 1), B(a + 2) } };
            a += 3;
            break;
        case 2:
            f = (fx_t){ 'p', { B(a), B(a + 3), B(a + 4) } };
            a += 5;
            break;
        case 3:
            f = (fx_t){ '9', { B(a), 0, 0 } };
            a++;
            break;
        case 4:
            f = (fx_t){ 'o', { B(a), 0, 0 } };
            a++;
            break;
        case 5:
            f = (fx_t){ 'i', { B(a), B(a + 1), 0 } };
            a += 2;
            break;
        case 8:
            f = (fx_t){ 'e', { B(a), B(a + 1), B(a + 2) } };
            a += 3;
            break;
        case 9:
            *delay = B(a);
            f = (fx_t){ 't', { B(a), 0, 0 } };
            a++;
            break;
        }
        if (f.code && nfx < 12)
            fx[nfx++] = f;
    }
    /* El ruido va antes que los efectos, y no importa su orden. Caben dos. */
    if (nfx > 2)
        warn(w, W_FX, "una casilla con más de dos efectos: sobran los primeros");
    for (int k = 0; k < 2 && k < nfx; k++) {
        const fx_t *f = &fx[nfx > 2 ? nfx - 2 + k : k];
        c->has |= (uint8_t)(ZXM_C_FX1 << k);
        c->fx[k] = f->code;
        memcpy(c->fxp[k], f->p, 3);
    }
    return a;
}

/* Las posiciones de un PT3, fila a fila, hasta que vuelve a empezar. */
static int read_half(half_t *h, const uint8_t *m, uint32_t n, warn_t *w)
{
    if (pt3_init(&h->h, m, n))
        return -1;
    struct {
        uint16_t addr;
        uint8_t skip, skip_cnt;
    } ch[3];
    int pos = 0, total = 0;
    uint8_t delay = B(0x64);
    uint16_t pat = W(0x67) + B(0xC9) * 2;
    for (int i = 0; i < 3; i++) {
        ch[i].addr = W(pat + i * 2);
        ch[i].skip = ch[i].skip_cnt = 1;
    }
    h->npos = 0;
    pos_t *cur = NULL;
    for (;;) {
        row_t r;
        memset(&r, 0, sizeof r);
        int has[3] = { 0 };
        if (--ch[0].skip_cnt == 0) {
            if (B(ch[0].addr) == 0 || !cur) {   /* fin del patrón, o el principio */
                if (cur && ++pos >= h->h.npos)
                    break;
                if (cur) {
                    pat = W(0x67) + B(0xC9 + pos) * 2;
                    for (int i = 0; i < 3; i++)
                        ch[i].addr = W(pat + i * 2);
                }
                if (h->npos >= 256)
                    return -1;
                cur = &h->pos[h->npos++];
                cur->rows = 0;
                cur->row = calloc(MAX_ROWS, sizeof(row_t));
                if (!cur->row)
                    return -1;
            }
            ch[0].addr = parse(m, n, ch[0].addr, &r.c[0], &ch[0].skip, &delay, w);
            ch[0].skip_cnt = ch[0].skip;
            has[0] = 1;
        }
        for (int i = 1; i < 3; i++)
            if (--ch[i].skip_cnt == 0) {
                ch[i].addr = parse(m, n, ch[i].addr, &r.c[i], &ch[i].skip, &delay, w);
                ch[i].skip_cnt = ch[i].skip;
                has[i] = 1;
            }
        (void)has;
        r.delay = delay;
        if (cur->rows >= MAX_ROWS) {
            warn(w, W_ROWS, "un patrón de más de 256 filas: se corta");
            continue;
        }
        cur->row[cur->rows++] = r;
        if (++total > 65536) {
            warn(w, W_BROKEN, "el PT3 no acaba sus patrones");
            return -1;
        }
    }
    return 0;
}

static void free_half(half_t *h)
{
    for (int i = 0; i < h->npos; i++)
        free(h->pos[i].row);
}

/* El instrumento k del PT3 (en su tabla, en 0x69) o su ornamento (0xA9). */
static zxms_blob_t blob(const uint8_t *m, uint32_t n, uint32_t p, int line)
{
    int len = B(p + 1);
    zxms_blob_t b = { malloc((size_t)(2 + line * len)), 2 + line * len };
    if (b.data)
        for (int i = 0; i < b.size; i++)
            b.data[i] = B(p + (uint32_t)i);
    return b;
}

/* El número que le toca a b entre los de la canción: el mismo si ya está. */
static int add_blob(zxms_blob_t *list, int *count, int max, zxms_blob_t b)
{
    for (int i = 0; i < *count; i++)
        if (list[i].size == b.size && !memcmp(list[i].data, b.data, (size_t)b.size)) {
            free(b.data);
            return i;
        }
    if (*count >= max) {
        free(b.data);
        return -1;
    }
    list[*count] = b;
    return (*count)++;
}

static void text(char *d, const uint8_t *m, uint32_t n, uint32_t at)
{
    int end = 0;
    for (int i = 0; i < 32; i++) {
        uint8_t c = B(at + (uint32_t)i);
        d[i] = c >= 32 && c < 127 ? (char)c : ' ';
        if (d[i] != ' ')
            end = i + 1;
    }
    d[end] = 0;
}

int zxms_from_pt3(zxms_t *s, const uint8_t *data, size_t size, zxms_warn_fn fn, void *ctx)
{
    warn_t w = { fn, ctx, 0 };
    uint32_t n1, n2;
    zxms_init(s);                       /* también si no se puede: zxms_free vale siempre */
    const uint8_t *part[2] = { data, NULL };
    uint32_t len[2] = { (uint32_t)size, 0 };
    int halves = 1;
    if (pt3_ts_split(data, (uint32_t)size, &n1, &n2)) {
        part[1] = data + n1;
        len[0] = n1;
        len[1] = n2;
        halves = 2;
    }
    half_t *h = calloc(2, sizeof *h);
    if (!h)
        return -1;
    int ok = 1;
    for (int i = 0; i < halves && ok; i++)
        ok = !read_half(&h[i], part[i], len[i], &w);
    if (ok && halves == 2) {
        /* Los dos, al mismo paso: las mismas posiciones, filas y velocidades. */
        ok = h[0].npos == h[1].npos && h[0].h.loop == h[1].h.loop;
        for (int p = 0; ok && p < h[0].npos; p++) {
            ok = h[0].pos[p].rows == h[1].pos[p].rows;
            for (int r = 0; ok && r < h[0].pos[p].rows; r++)
                ok = h[0].pos[p].row[r].delay == h[1].pos[p].row[r].delay;
        }
        if (!ok)
            warn(&w, W_BROKEN, "un TurboSound cuyos dos módulos no van al mismo paso: no se puede");
    }
    if (!ok) {
        for (int i = 0; i < halves; i++)
            free_half(&h[i]);
        free(h);
        return -1;
    }

    const uint8_t *m = data;
    uint32_t n = len[0];
    text(s->title, m, n, 0x1E);
    text(s->author, m, n, 0x42);
    s->ay_tracks = 3 * halves;
    s->flags = (h[0].h.version & 7) << 4;
    s->ay_table = B(0x63) & 3;
    if (halves == 2)
        s->ay2 = (mus_b(part[1], len[1], 0x63) & 3) | (h[1].h.version & 7) << 4;
    s->speed = B(0x64);
    s->npos = h[0].npos;
    s->loop = h[0].h.loop < h[0].npos ? h[0].h.loop : 0;

    /* Los instrumentos y los ornamentos: los del primero con sus números, y
     * los del segundo, detrás (o el mismo número si son iguales). */
    int ins_map[2][32], orn_map[2][16];
    for (int i = 0; i < halves; i++) {
        const uint8_t *mm = part[i];
        uint32_t nn = len[i];
        for (int k = 0; k < 32; k++) {
            zxms_blob_t b = blob(mm, nn, mus_w(mm, nn, 0x69 + (uint32_t)k * 2), 4);
            if (i == 0) {
                s->ayins[k] = b;
                ins_map[0][k] = k;
                s->n_ayins = 32;
            } else {
                ins_map[1][k] = add_blob(s->ayins, &s->n_ayins, 63, b);
                if (ins_map[1][k] < 0)
                    warn(&w, W_INS, "más de 63 instrumentos distintos entre los dos módulos");
            }
        }
        for (int k = 0; k < 16; k++) {
            zxms_blob_t b = blob(mm, nn, mus_w(mm, nn, 0xA9 + (uint32_t)k * 2), 1);
            if (i == 0) {
                s->orns[k] = b;
                orn_map[0][k] = k;
                s->n_orns = 16;
            } else {
                orn_map[1][k] = add_blob(s->orns, &s->n_orns, 64, b);
            }
        }
    }

    for (int p = 0; p < s->npos; p++) {
        int rows = h[0].pos[p].rows;
        s->pos[p].rows = rows ? rows : 1;
        for (int t = 0; t < ZXMS_TRACKS; t++)
            s->pos[p].pat[t] = -1;
        for (int i = 0; i < halves; i++)
            for (int c = 0; c < 3; c++) {
                int pat = zxms_add_pattern(s, 0, s->pos[p].rows);
                if (pat < 0)
                    continue;
                s->pos[p].pat[i * 3 + c] = pat;
                for (int r = 0; r < rows; r++) {
                    zxms_cell_t *d = &s->pat[pat].cells[r];
                    *d = h[i].pos[p].row[r].c[c];
                    if ((d->has & ZXM_C_INS) && i == 1)
                        d->ins = (uint8_t)(ins_map[1][d->ins & 31] < 0 ? 0 : ins_map[1][d->ins & 31]);
                    if ((d->has & ZXM_C_ORN) && i == 1)
                        d->orn = (uint8_t)orn_map[1][d->orn & 15];
                }
                /* El segundo empieza con su instrumento 1 y su ornamento 0,
                 * que en la canción pueden tener otro número. */
                if (p == 0 && i == 1 && rows) {
                    zxms_cell_t *d = &s->pat[pat].cells[0];
                    if (!(d->has & ZXM_C_INS) && ins_map[1][1] != 1) {
                        d->has |= ZXM_C_INS;
                        d->ins = (uint8_t)ins_map[1][1];
                    }
                    if (!(d->has & ZXM_C_ORN) && orn_map[1][0] != 0) {
                        d->has |= ZXM_C_ORN;
                        d->orn = (uint8_t)orn_map[1][0];
                    }
                }
            }
    }
    for (int i = 0; i < halves; i++)
        free_half(&h[i]);
    free(h);
    zxms_dedup(s);
    return 0;
}
