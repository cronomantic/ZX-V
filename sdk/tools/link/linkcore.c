/*
 * linkcore.c - La conversación con la máquina por J5 (ver linkcore.h).
 */
#ifndef _WIN32
#define _DEFAULT_SOURCE                 /* clock_gettime, dirent */
#endif
#include "linkcore.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

bool lc_spanish;
#define TR(en, es) (lc_spanish ? (es) : (en))

static unsigned long now_ms(void)
{
#ifdef _WIN32
    return (unsigned long)GetTickCount64();
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (unsigned long)t.tv_sec * 1000ul + (unsigned long)(t.tv_nsec / 1000000);
#endif
}

static int fail(lc_link_t *l, int code, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(l->error, sizeof l->error, fmt, ap);
    va_end(ap);
    return code;
}

/* Lo que quiere decir un error de la máquina. */
static const char *machine_error(int32_t e)
{
    switch (e) {
    case ZXV_E_NOENT: return TR("it does not exist", "no existe");
    case ZXV_E_EXIST: return TR("there is already one with that name", "ya hay uno con ese nombre");
    case ZXV_E_NOTDIR: return TR("it is not a folder", "no es una carpeta");
    case ZXV_E_NOTEMPTY: return TR("the folder is not empty", "la carpeta no está vacía");
    case ZXV_E_NAME: return TR("the name is not valid on the card", "el nombre no vale en la tarjeta");
    case ZXV_E_ISDIR: return TR("there is a folder with that name", "hay una carpeta con ese nombre");
    case ZXV_E_BADF: return TR("the file is no longer open on the board (has it restarted?)",
                               "el fichero ya no está abierto en la placa (¿se ha reiniciado?)");
    case ZXV_E_MFILE: return TR("too many open files on the board", "demasiados ficheros abiertos en la placa");
    case ZXV_E_ACCESS: return TR("not allowed (is a program using it?)", "no se puede (¿lo está usando un programa?)");
    case ZXV_E_IO: return TR("read or write error", "error de lectura o escritura");
    case ZXV_E_NOCARD: return TR("there is no SD card in the board", "no hay tarjeta SD en la placa");
    case ZXV_E_NOSPC: return TR("the card is full", "la tarjeta está llena");
    case ZXV_E_INVAL: return TR("not valid", "no es válido");
    default: return TR("error", "error");
    }
}

/* El fichero de lo que recuerda este PC de la red (su id y las claves). */
static bool net_cfg(char *out, size_t n)
{
    return lc_config_file("zxv-link-net.txt", out, n);
}

int lc_open(lc_link_t *l, const char *port, int *why_kind)
{
    char why[160] = "";
    int kind = 0;
    zl_rx_init(&l->rx);
    l->pend_n = l->pend_at = 0;
    l->version = 0;
    l->unpaired = false;
    if (netpc_is_target(port)) {
        char cfg[1024];
        bool paired = false;
        if (why_kind)
            *why_kind = SERIAL_E_NONE;
        if (!net_cfg(cfg, sizeof cfg))
            return fail(l, LC_E_PORT, "%s", TR("no folder for zxv-link's settings", "no hay carpeta para la configuración de zxv-link"));
        unsigned version = 0;
        l->net = netpc_open(cfg, port, &paired, why, &version);
        char mismatch[256];
        if (!l->net && version && !lc_net_compatible(version, mismatch, sizeof mismatch))
            return fail(l, LC_E_PORT, "%s: %s", port, mismatch);
        if (!l->net)
            return fail(l, LC_E_PORT, "%s: %s", port,
                        TR("no ZX-V answers there: is it on, with Wi-Fi, on the same network?",
                           "no contesta ninguna ZX-V: ¿está encendida, con el Wi-Fi, en la misma red?"));
        l->unpaired = !paired;
        return 0;
    }
    l->s = serial_open(port, ZL_BAUD, &kind, why);
    if (why_kind)
        *why_kind = kind;
    if (l->s)
        return 0;
    return fail(l, LC_E_PORT, "%s: %s", port,
                kind == SERIAL_E_NONE ? TR("there is no such port: is the USB-serial adapter plugged in?",
                                           "no existe ese puerto: ¿está enchufado el adaptador USB-serie?")
                : kind == SERIAL_E_BUSY ? TR("another program is using it (a terminal, or another zxv-link)",
                                             "lo está usando otro programa (un terminal, u otro zxv-link)")
                : kind == SERIAL_E_DENIED ? TR("no permission to open it (on Linux, add yourself to the dialout group)",
                                               "no hay permiso para abrirlo (en Linux, entra en el grupo dialout)")
                : why);
}

void lc_close(lc_link_t *l)
{
    if (l->ext)
        lc_unserve(l);
    if (l->net)
        netpc_close(l->net);
    else
        serial_close(l->s);
    l->net = NULL;
    l->s = NULL;
    l->pend_n = l->pend_at = 0;
}

/* Una trama, por el puerto o por la red. */
static int io_write(lc_link_t *l, const uint8_t *f, size_t n)
{
    if (!l->net)
        return serial_write(l->s, f, n);
    l->net_quiet = now_ms() + ZLN_KEEPALIVE_MS;
    return netpc_write(l->net, f, n);
}

/* Por la red, el informe va a los programas que han hablado hace poco: si
 * este solo escucha, saluda de vez en cuando (la respuesta se tira). */
static void net_keepalive(lc_link_t *l)
{
    if (!l->net || l->unpaired || (long)(now_ms() - l->net_quiet) < 0)
        return;
    uint8_t f[ZL_FRAME_MAX];
    size_t fn = zl_pack(ZL_CH_CTRL, ZL_HELLO, ++l->seq, NULL, 0, f);
    if (fn)
        io_write(l, f, fn);
}

/* Lo que pide la máquina a la unidad EXT: (si se sirve), contestado ya. */
static void serve_ext(lc_link_t *l, const zl_msg_t *m)
{
    if (!l->ext)
        return;
    uint8_t out[ZL_MAX_DATA], f[ZL_FRAME_MAX];
    unsigned n = ext_server_handle(l->ext, m, out);
    size_t fn = zl_pack(ZL_CH_EXT, (uint8_t)(m->type | ZL_REPLY), m->seq, out, n, f);
    if (fn)
        io_write(l, f, fn);
}

/* Lo que llega hasta deadline (ms): el informe, a on_log; una respuesta a
 * (type, seq), en *m (1). -1 si el puerto falla, -2 si se para (cancel: se
 * mira cada 100 ms). */
static int wait_reply(lc_link_t *l, uint8_t type, uint8_t seq, unsigned long deadline, zl_msg_t *m)
{
    for (;;) {
        if (l->cancel)
            return -2;
        if (l->pend_at >= l->pend_n) {  /* nada guardado: del puerto */
            unsigned long t = now_ms();
            if ((long)(deadline - t) <= 0)
                return 0;
            unsigned slice = deadline - t > 100 ? 100 : (unsigned)(deadline - t);
            int n = l->net ? netpc_read(l->net, l->pend, sizeof l->pend, slice)
                           : serial_read(l->s, l->pend, sizeof l->pend, slice);
            if (n < 0)
                return -1;
            l->pend_n = n;
            l->pend_at = 0;
        }
        while (l->pend_at < l->pend_n) {
            if (!zl_feed(&l->rx, l->pend[l->pend_at++], m))
                continue;
            if (m->ch == ZL_CH_LOG && m->type == ZL_LOG) {
                if (l->on_log)
                    l->on_log(l->ctx, (const char *)m->data, m->len);
                continue;
            }
            if (m->ch == ZL_CH_EXT && !(m->type & ZL_REPLY)) {
                serve_ext(l, m);        /* la máquina pide a EXT: */
                continue;
            }
            if (m->ch == ZL_CH_CTRL && m->type == (type | ZL_REPLY) && m->seq == seq)
                return 1;               /* lo que viene detrás se queda para luego */
        }
    }
}

/* El anuncio de que se sirve EXT: (cada ZL_SERVE_EVERY_MS): la máquina
 * contesta su número de arranque; si cambia, lo abierto de antes, fuera. */
static void ext_announce(lc_link_t *l)
{
    if (!l->ext || l->ext_busy || (!l->s && !l->net) || l->unpaired || (long)(now_ms() - l->ext_next) < 0)
        return;
    l->ext_busy = true;
    uint8_t on = 1, more[16];
    unsigned mn = 0;
    char keep[sizeof l->error];
    memcpy(keep, l->error, sizeof keep);        /* no estropea el error de otra cosa */
    int32_t st = lc_request(l, ZL_SERVE, &on, 1, 1000, 1, more, &mn);
    memcpy(l->error, keep, sizeof keep);
    if (st == 0 && mn >= 4)
        ext_server_boot(l->ext, zl_get32(more));
    l->ext_next = now_ms() + ZL_SERVE_EVERY_MS;
    l->ext_busy = false;
}

int lc_pump(lc_link_t *l, unsigned ms)
{
    zl_msg_t m;
    ext_announce(l);
    net_keepalive(l);
    if (wait_reply(l, 0xFF, 0, now_ms() + ms, &m) == -1)
        return fail(l, LC_E_SERIAL, "%s", TR("the serial port has failed", "el puerto serie ha fallado"));
    return 0;
}

int32_t lc_request(lc_link_t *l, uint8_t type, const void *data, unsigned len, unsigned timeout_ms, int tries,
                   uint8_t *more, unsigned *more_len)
{
    if (l->net && l->unpaired)
        return fail(l, LC_E_UNPAIRED, "%s",
                    TR("this machine is not paired with this PC: pair it first (zxv-link pair, with EDIT > Wi-Fi > "
                       "Pair a PC open on the machine)",
                       "esta máquina no está emparejada con este PC: emparéjala antes (zxv-link pair, con EDIT > "
                       "Wi-Fi > Emparejar un PC abierto en la máquina)"));
    ext_announce(l);                    /* si toca, antes */
    uint8_t seq = ++l->seq, f[ZL_FRAME_MAX];
    size_t fn = zl_pack(ZL_CH_CTRL, type, seq, data, len, f);
    for (int i = 0; i < tries; i++) {
        if (!fn || io_write(l, f, fn) < 0)
            return fail(l, LC_E_SERIAL, "%s", TR("cannot write to the serial port", "no se puede escribir en el puerto serie"));
        zl_msg_t m;
        int r = wait_reply(l, type, seq, now_ms() + timeout_ms, &m);
        if (r == -2)
            return fail(l, LC_E_CANCEL, "%s", TR("stopped", "parado"));
        if (r < 0)
            return fail(l, LC_E_SERIAL, "%s", TR("the serial port has failed", "el puerto serie ha fallado"));
        if (r == 0)
            continue;
        if (m.len < 4)
            return fail(l, LC_E_REPLY, "%s", TR("a reply that is too short", "una respuesta demasiado corta"));
        if (more) {
            *more_len = m.len - 4;
            memcpy(more, m.data + 4, m.len - 4);
        }
        int32_t st = (int32_t)zl_get32(m.data);
        if (st < 0)
            fail(l, st, "%s", machine_error(st));
        return st;
    }
    return fail(l, LC_E_NOANSWER, "%s",
                TR("the machine does not answer (is the adapter on J5, with TX to RX, and the machine on?)",
                   "la máquina no contesta (¿está el adaptador en J5, con TX a RX, y la máquina encendida?)"));
}

int lc_hello(lc_link_t *l, lc_info_t *info)
{
    uint8_t more[ZL_MAX_DATA + 1];
    unsigned n = 0;
    int32_t st = lc_request(l, ZL_HELLO, NULL, 0, 1000, 3, more, &n);
    if (st < 0)
        return st;
    if (n < 7)
        return fail(l, LC_E_REPLY, "%s", TR("unexpected reply", "respuesta inesperada"));
    l->version = (unsigned)(more[0] | more[1] << 8);
    if (info) {
        more[n] = 0;
        info->protocol = l->version;
        info->waiting = more[2] == 1;
        info->updating = more[2] == 2;
        info->spec = zl_get32(more + 3);
        snprintf(info->firmware, sizeof info->firmware, "%.63s", (const char *)more + 7);
    }
    return 0;
}

int lc_serve(lc_link_t *l, const char *folder, bool overwrite)
{
    int r;
    if (!l->version && (r = lc_hello(l, NULL)) < 0)
        return r;
    if (l->version < 4)
        return fail(l, LC_E_OLD, "%s", TR("the machine's firmware is too old for the EXT: drive: update it",
                                          "el firmware de la máquina es demasiado antiguo para la unidad EXT: "
                                          "actualízalo"));
    ext_server_t *x = l->ext ? l->ext : calloc(1, sizeof *x);
    if (!x)
        return fail(l, LC_E_FILE, "%s", TR("out of memory", "sin memoria"));
    if (ext_server_start(x, folder) < 0) {
        if (!l->ext)
            free(x);
        return fail(l, LC_E_FILE, "%s: %s", folder, TR("the folder does not exist", "la carpeta no existe"));
    }
    x->overwrite = overwrite;
    l->ext = x;
    l->ext_next = 0;
    ext_announce(l);                    /* ya */
    return 0;
}

int lc_serve_file(lc_link_t *l, const char *file, const char *name)
{
    int r;
    if (!l->version && (r = lc_hello(l, NULL)) < 0)
        return r;
    if (l->version < 4)
        return fail(l, LC_E_OLD, "%s", TR("the machine's firmware is too old for the EXT: drive: update it",
                                          "el firmware de la máquina es demasiado antiguo para la unidad EXT: "
                                          "actualízalo"));
    ext_server_t *x = l->ext ? l->ext : calloc(1, sizeof *x);
    if (!x)
        return fail(l, LC_E_FILE, "%s", TR("out of memory", "sin memoria"));
    if (ext_server_start_file(x, file, name) < 0) {
        if (!l->ext)
            free(x);
        return fail(l, LC_E_FILE, "%s: %s", file, TR("cannot be read", "no se puede leer"));
    }
    x->overwrite = false;
    l->ext = x;
    l->ext_next = 0;
    ext_announce(l);                    /* ya */
    return 0;
}

void lc_unserve(lc_link_t *l)
{
    if (!l->ext)
        return;
    ext_server_t *x = l->ext;
    uint8_t off = 0;
    l->ext_busy = true;                 /* sin anunciarse más */
    if (l->s || (l->net && !l->unpaired))
        lc_request(l, ZL_SERVE, &off, 1, 500, 1, NULL, NULL);
    l->ext_busy = false;
    l->ext = NULL;
    ext_server_close_all(x);
    free(x);
}

static uint8_t *read_file(lc_link_t *l, const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fail(l, LC_E_FILE, "%s: %s", path, TR("cannot open it", "no se puede abrir"));
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *d = n >= 0 && n <= 16 * 1024 * 1024 ? malloc((size_t)n + 1) : NULL;
    if (!d || fread(d, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        free(d);
        fail(l, LC_E_FILE, "%s: %s", path, TR("cannot read it", "no se puede leer"));
        return NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return d;
}

const char *lc_base_name(const char *p)
{
    const char *b = p;
    for (const char *q = p; *q; q++)
        if (*q == '/' || *q == '\\')
            b = q + 1;
    return b;
}

/* El error de la petición sobre dest, con su nombre delante. */
static int on_file(lc_link_t *l, int32_t st, const char *dest)
{
    char why[sizeof l->error];
    snprintf(why, sizeof why, "%s", st <= LC_E_PORT ? l->error : machine_error(st));
    return fail(l, st, "%s: %s", dest, why);
}

/* --- Lo que puso zxv-link en la tarjeta -------------------------------------------- */

/*
 * Apuntado en el PC: el nombre, el tamaño y el CRC de lo último que se mandó
 * (o que ya estaba igual) con cada nombre. Un fichero de la tarjeta distinto
 * del que se manda, que no coincide con lo apuntado, no lo puso zxv-link: no
 * se sustituye sin preguntar. El que sí (el programa que se edita y se vuelve
 * a ejecutar), sí. En ZXV_HOME si está (las pruebas), en %APPDATA%\ZX-V en
 * Windows y en ~/.config/ZX-V en los demás. Los nombres, sin distinguir
 * mayúsculas (FAT tampoco).
 */
#define SENT_MAX        512

typedef struct {
    uint32_t size, crc;
    char name[256];
} sent_t;

bool lc_config_file(const char *name, char *out, size_t n)
{
    char dir[600];
    const char *home = getenv("ZXV_HOME");
    if (home && *home) {
        snprintf(dir, sizeof dir, "%s", home);
    } else {
#ifdef _WIN32
        const char *app = getenv("APPDATA");
        if (!app || !*app)
            return false;
        snprintf(dir, sizeof dir, "%s\\ZX-V", app);
        CreateDirectoryA(dir, NULL);
#else
        const char *x = getenv("XDG_CONFIG_HOME"), *h = getenv("HOME");
        if (x && *x) {
            snprintf(dir, sizeof dir, "%s/ZX-V", x);
        } else if (h && *h) {
            snprintf(dir, sizeof dir, "%s/.config", h);
            mkdir(dir, 0755);
            snprintf(dir, sizeof dir, "%s/.config/ZX-V", h);
        } else {
            return false;
        }
        mkdir(dir, 0755);
#endif
    }
    return snprintf(out, n, "%s/%s", dir, name) < (int)n;
}

static bool sent_file(char *out, size_t n)
{
    return lc_config_file("zxv-link-sent.txt", out, n);
}

static bool same_name(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return false;
    return *a == *b;
}

static int sent_load(sent_t *list, int max)
{
    char path[700], line[300];
    if (!sent_file(path, sizeof path))
        return 0;
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;
    int n = 0;
    while (n < max && fgets(line, sizeof line, f)) {
        unsigned long crc, size;
        int at = 0;
        line[strcspn(line, "\r\n")] = 0;
        if (sscanf(line, "%lx %lu %n", &crc, &size, &at) < 2 || !at || !line[at])
            continue;
        list[n].crc = (uint32_t)crc;
        list[n].size = (uint32_t)size;
        snprintf(list[n].name, sizeof list[n].name, "%s", line + at);
        n++;
    }
    fclose(f);
    return n;
}

static bool sent_by_us(const char *dest, uint32_t size, uint32_t crc)
{
    sent_t *list = malloc(sizeof *list * SENT_MAX);
    if (!list)
        return false;
    int n = sent_load(list, SENT_MAX);
    bool ours = false;
    for (int i = 0; i < n && !ours; i++)
        ours = same_name(list[i].name, dest) && list[i].size == size && list[i].crc == crc;
    free(list);
    return ours;
}

/* El último con ese nombre, el de ahora (y, como mucho, SENT_MAX). */
static void sent_note(const char *dest, uint32_t size, uint32_t crc)
{
    char path[700];
    sent_t *list = malloc(sizeof *list * SENT_MAX);
    if (!list || !sent_file(path, sizeof path)) {
        free(list);
        return;
    }
    int n = sent_load(list, SENT_MAX), others = 0;
    for (int i = 0; i < n; i++)
        others += !same_name(list[i].name, dest);
    int skip = others >= SENT_MAX ? others - SENT_MAX + 1 : 0;   /* los más viejos, fuera */
    FILE *f = fopen(path, "w");
    if (f) {
        for (int i = 0; i < n; i++) {
            if (same_name(list[i].name, dest))
                continue;
            if (skip) {
                skip--;
                continue;
            }
            fprintf(f, "%08lx %lu %s\n", (unsigned long)list[i].crc, (unsigned long)list[i].size, list[i].name);
        }
        fprintf(f, "%08lx %lu %s\n", (unsigned long)crc, (unsigned long)size, dest);
        fclose(f);
    }
    free(list);
}

/* Lo que hay en la tarjeta con el nombre dest frente a un fichero de ese
 * tamaño y ese CRC (versión 2): LC_CARD_*, o el error (del puerto, parado,
 * sin tarjeta...). */
static int card_state(lc_link_t *l, const char *dest, size_t size, uint32_t crc)
{
    uint8_t more[16];
    unsigned mn = 0;
    int32_t st = lc_request(l, ZL_STAT, dest, (unsigned)strlen(dest), 5000, 1, more, &mn);
    if (st <= LC_E_PORT || st == ZXV_E_NOCARD)
        return st;
    if (st == ZXV_E_NOENT)
        return LC_CARD_ABSENT;
    if (st < 0)
        return on_file(l, st, dest);
    if (mn < 8)
        return fail(l, LC_E_REPLY, "%s", TR("a reply that is too short", "una respuesta demasiado corta"));
    uint32_t cs = zl_get32(more), cc = zl_get32(more + 4);
    if (cs == size && cc == crc)
        return LC_CARD_SAME;
    return sent_by_us(dest, cs, cc) ? LC_CARD_OURS : LC_CARD_OTHER;
}

int lc_card_state(lc_link_t *l, const char *file, const char *dest)
{
    if (strlen(dest) > 250)
        return fail(l, LC_E_FILE, "%s: %s", dest, TR("name too long", "nombre demasiado largo"));
    int r;
    if (!l->version && (r = lc_hello(l, NULL)) < 0)
        return r;
    if (l->version < 2)
        return fail(l, LC_E_OLD, "%s", TR("the firmware cannot tell", "el firmware no sabe decirlo"));
    size_t size;
    uint8_t *d = read_file(l, file, &size);
    if (!d)
        return LC_E_FILE;
    r = card_state(l, dest, size, zl_crc32(0, d, size));
    free(d);
    return r;
}

static int refuse_other(lc_link_t *l, const char *names)
{
    return fail(l, LC_E_EXISTS, "%s: %s", names,
                TR("on the card there is already a different file with that name, not put there by zxv-link",
                   "en la tarjeta ya hay otro distinto con ese nombre, que no puso zxv-link"));
}

int lc_send(lc_link_t *l, const char *file, const char *dest)
{
    size_t dn = strlen(dest);
    if (dn > 250)
        return fail(l, LC_E_FILE, "%s: %s", dest, TR("name too long", "nombre demasiado largo"));
    size_t size;
    uint8_t *d = read_file(l, file, &size);
    if (!d)
        return LC_E_FILE;
    uint32_t crc = zl_crc32(0, d, size);
    int r = 0;
    if (!l->version && (r = lc_hello(l, NULL)) < 0)
        goto out;

    /* Si ya está en la tarjeta igual (versión 2), no hace falta mandarlo; si
     * hay otro que no puso zxv-link, no se sustituye sin permiso. */
    if (l->version >= 2) {
        int state = card_state(l, dest, size, crc);
        if (state < 0) {
            r = state;
            goto out;
        }
        if (state == LC_CARD_SAME) {
            sent_note(dest, (uint32_t)size, crc);
            if (l->on_progress)
                l->on_progress(l->ctx, dest, size, size, true);
            goto out;
        }
        if (state == LC_CARD_OTHER && !l->overwrite) {
            r = refuse_other(l, dest);
            goto out;
        }
    }

    uint8_t open[4 + 256];
    zl_put32(open, (uint32_t)size);
    memcpy(open + 4, dest, dn);
    int32_t st = lc_request(l, ZL_OPEN, open, (unsigned)(4 + dn), 2000, 3, NULL, NULL);
    if (st < 0) {
        r = on_file(l, st, dest);
        goto out;
    }
    uint8_t chunk[4 + ZL_CHUNK], more[16];
    int resync = 0;                     /* veces seguidas que la máquina pide otra posición */
    if (l->on_progress)
        l->on_progress(l->ctx, dest, 0, size, false);
    for (size_t at = 0; at < size;) {
        size_t k = size - at < ZL_CHUNK ? size - at : ZL_CHUNK;
        zl_put32(chunk, (uint32_t)at);
        memcpy(chunk + 4, d + at, k);
        unsigned mn = 0;
        st = lc_request(l, ZL_DATA, chunk, (unsigned)(4 + k), 3000, 5, more, &mn);
        if (st == ZXV_E_INVAL && mn >= 4 && zl_get32(more) <= size && ++resync <= 3) {
            at = zl_get32(more);        /* la máquina espera otra posición: desde ahí */
            continue;
        }
        if (st < 0) {
            r = on_file(l, st, dest);
            goto out;
        }
        at += k;
        resync = 0;
        if (l->on_progress)
            l->on_progress(l->ctx, dest, at, size, false);
    }
    uint8_t c[4];
    zl_put32(c, crc);
    st = lc_request(l, ZL_CLOSE, c, 4, 3000, 1, NULL, NULL);
    if (st < 0)
        r = on_file(l, st, dest);
    else
        sent_note(dest, (uint32_t)size, crc);
out:
    free(d);
    return r;
}

/* Un programa grabado sin LINE (.zvb o .tap): al cargarlo no arranca. */
static bool without_line(const char *file)
{
    const char *dot = strrchr(file, '.');
    if (!dot)
        return false;
    bool zvb = !strcmp(dot, ".zvb") || !strcmp(dot, ".ZVB"), tap = !strcmp(dot, ".tap") || !strcmp(dot, ".TAP");
    if (!zvb && !tap)
        return false;
    FILE *f = fopen(file, "rb");
    if (!f)
        return false;
    uint8_t h[24];
    size_t n = fread(h, 1, sizeof h, f);
    fclose(f);
    if (zvb)
        return n >= 24 && !memcmp(h, "ZXVB", 4) && h[4] == 1 && h[5] == 0 && (h[8] | h[9] << 8) == 0xFFFF;
    return n >= 18 && (h[0] | h[1] << 8) == 19 && h[2] == 0 && h[3] == 0 && (h[16] | h[17] << 8) >= 32768;
}

/* Lo que va detrás de LOAD "x": RUN un .bas, GO TO 0 uno sin LINE. */
static const char *load_tail(const char *file)
{
    const char *name = lc_base_name(file), *dot = strrchr(name, '.');
    bool bas = dot && (!strcmp(dot, ".bas") || !strcmp(dot, ".BAS") || !strcmp(dot, ".txt") || !strcmp(dot, ".TXT"));
    return bas ? ": RUN" : without_line(file) ? ": GO TO 0" : "";
}

void lc_load_command(const char *file, char *cmd, size_t n)
{
    snprintf(cmd, n, "LOAD \"SD:%s\"%s", lc_base_name(file), load_tail(file));
}

int lc_ext_command(const char *folder, const char *file, char *cmd, size_t n)
{
    /* La ruta del programa dentro de la carpeta, con "/" (también en
     * Windows, y sin distinguir mayúsculas allí). */
    size_t fl = strlen(folder);
    while (fl && (folder[fl - 1] == '/' || folder[fl - 1] == '\\'))
        fl--;
    bool inside = strlen(file) > fl + 1 && (file[fl] == '/' || file[fl] == '\\');
    for (size_t i = 0; inside && i < fl; i++) {
        char a = folder[i], b = file[i];
#ifdef _WIN32
        a = (char)tolower((unsigned char)(a == '\\' ? '/' : a));
        b = (char)tolower((unsigned char)(b == '\\' ? '/' : b));
#endif
        inside = a == b;
    }
    if (!inside)
        return LC_E_FILE;
    char dir[260];
    size_t o = 0;
    const char *rel = file + fl, *base = lc_base_name(file);
    for (const char *p = rel; p < base - 1 && o < sizeof dir - 1; p++)
        dir[o++] = *p == '\\' ? '/' : *p;
    dir[o] = 0;
    snprintf(cmd, n, "CD \"EXT:%s\": LOAD \"%s\"%s", dir, base, load_tail(file));
    return 0;
}

/* La máquina se reinicia entera (ZL_ST_RESTART): se la saluda hasta que
 * el BASIC espere una orden, como mucho 15 s (un arranque tarda uno o dos). */
static int wait_back(lc_link_t *l)
{
    unsigned long until = now_ms() + 15000;
    while ((long)(until - now_ms()) > 0) {
        lc_info_t info;
        int r = lc_hello(l, &info);
        if (r == LC_E_SERIAL || r == LC_E_CANCEL)
            return r;
        if (r == 0 && info.waiting) {
            /* Reiniciada, ha olvidado quién sirve EXT:: se le dice ya, antes
             * de teclear nada que la use (si no, a veces el LOAD llegaba
             * antes del anuncio de cada segundo y no había servidor). */
            if (l->ext) {
                l->ext_next = 0;
                ext_announce(l);
            }
            return 0;
        }
        if (r == 0 && lc_pump(l, 300) < 0)  /* contesta, pero aún sin el BASIC */
            return LC_E_SERIAL;
    }
    return fail(l, LC_E_NOANSWER, "%s", TR("the machine has not come back after restarting",
                                           "la máquina no ha vuelto tras reiniciarse"));
}

int lc_type(lc_link_t *l, const char *cmd)
{
    for (int i = 0; i < 2; i++) {
        int32_t st = lc_request(l, ZL_RUN, cmd, (unsigned)strlen(cmd), 8000, 1, NULL, NULL);
        if (st == ZXV_E_IO)
            return fail(l, st, "%s", TR("BASIC does not come back after the reset", "el BASIC no vuelve tras reiniciar"));
        if (st != ZL_ST_RESTART)
            return st < 0 ? st : 0;
        int r = wait_back(l);           /* había un programa en marcha: otra vez, ya en el BASIC */
        if (r < 0)
            return r;
    }
    return fail(l, LC_E_REPLY, "%s", TR("a program is still running", "sigue habiendo un programa en marcha"));
}

int lc_ready(lc_link_t *l)
{
    if (l->version < 2)
        return 0;
    int32_t st = lc_request(l, ZL_READY, NULL, 0, 8000, 1, NULL, NULL);
    if (st == ZXV_E_IO)
        return fail(l, st, "%s", TR("the running program cannot be stopped (BASIC does not come back after the reset)",
                                    "el programa en marcha no se puede parar (el BASIC no vuelve tras reiniciar)"));
    if (st == ZL_ST_RESTART)
        return wait_back(l);
    return st < 0 ? st : 0;
}

/* Antes de tocar nada (versión 2): que haya tarjeta, que se puedan leer
 * los del PC y, sin overwrite, que no se vaya a sustituir lo que no puso
 * zxv-link (todos los que sí, en l->error). */
static int preflight(lc_link_t *l, const char *const *files, int n)
{
    if (l->version < 2)
        return 0;
    char names[200] = "";
    size_t used = 0;
    int others = 0;
    for (int i = 0; i < n; i++) {
        const char *dest = lc_base_name(files[i]);
        int r = lc_card_state(l, files[i], dest);
        if (r < 0)
            return r;
        if (r == LC_CARD_OTHER && !l->overwrite) {
            int k = snprintf(names + used, sizeof names - used, "%s%s", others ? ", " : "", dest);
            if (k > 0 && (size_t)k < sizeof names - used)
                used += (size_t)k;
            others++;
        }
    }
    return others ? refuse_other(l, names) : 0;
}

int lc_copy(lc_link_t *l, const char *const *files, int n)
{
    int r;
    if (!l->version && (r = lc_hello(l, NULL)) < 0)
        return r;
    if ((r = preflight(l, files, n)) < 0)
        return r;
    for (int i = 0; i < n; i++)
        if ((r = lc_send(l, files[i], lc_base_name(files[i]))) < 0)
            return r;
    return 0;
}

int lc_run_ext(lc_link_t *l, const char *folder, const char *file)
{
    char cmd[400];
    if (lc_ext_command(folder, file, cmd, sizeof cmd) < 0)
        return fail(l, LC_E_FILE, "%s: %s", file, TR("it is not inside the folder of the EXT: drive",
                                                     "no está dentro de la carpeta de la unidad EXT:"));
    if (!l->ext)
        return fail(l, LC_E_FILE, "%s", TR("the EXT: drive is not being served", "no se está sirviendo la unidad EXT:"));
    lc_info_t info;
    int r;
    if ((r = lc_hello(l, &info)) < 0)
        return r;
    if (!info.waiting && !l->allow_stop)
        return fail(l, LC_E_RUNNING, "%s", TR("a program is running on the board", "hay un programa en marcha en la placa"));
    if ((r = lc_ready(l)) < 0)
        return r;
    return lc_type(l, cmd);
}

int lc_run(lc_link_t *l, const char *const *files, int n)
{
    lc_info_t info;
    int r;
    if ((r = lc_hello(l, &info)) < 0)   /* también, si hay un programa en marcha ahora */
        return r;
    /* Sin tarjeta, o con algo que no se puede sustituir, ni se para lo que
     * está en marcha. */
    if ((r = preflight(l, files, n)) < 0)
        return r;
    if (!info.waiting && !l->allow_stop)
        return fail(l, LC_E_RUNNING, "%s", TR("a program is running on the board", "hay un programa en marcha en la placa"));
    if ((r = lc_ready(l)) < 0)
        return r;
    for (int i = 0; i < n; i++)
        if ((r = lc_send(l, files[i], lc_base_name(files[i]))) < 0)
            return r;
    char cmd[200];
    lc_load_command(files[0], cmd, sizeof cmd);
    return lc_type(l, cmd);
}

/* Lo que puede cargar un programa: lo mismo que la extensión de VS Code
 * (RESOURCE_EXTS, sin los programas). */
static bool loadable(const char *name)
{
    static const char *const ext[] = { ".gfx", ".zxm", ".pt3", ".pt2", ".stc", ".asc", ".psg", ".mod", ".s3m",
                                       ".xm", ".vgm", ".vgz", ".afb", ".afx", ".wav", ".zvb", ".zvd", ".tap",
                                       ".tzx", ".scr", ".bin", ".dat" };
    const char *dot = strrchr(name, '.');
    if (!dot)
        return false;
    for (size_t i = 0; i < sizeof ext / sizeof ext[0]; i++) {
        size_t k = 0;
        while (ext[i][k] && dot[k] && (dot[k] | 0x20) == ext[i][k])
            k++;
        if (!ext[i][k] && !dot[k])
            return true;
    }
    return false;
}

int lc_folder_files(const char *program, char **out, int max)
{
    char dir[1024];
    size_t dn = (size_t)(lc_base_name(program) - program);
    if (dn >= sizeof dir - 2)
        return 0;
    memcpy(dir, program, dn);
    dir[dn] = 0;
    int count = 0;
#ifdef _WIN32
    char pattern[1100];
    snprintf(pattern, sizeof pattern, "%s*", dn ? dir : "");
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    do {
        const char *name = fd.cFileName;
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !loadable(name) ||
            !_stricmp(name, lc_base_name(program)) || count >= max)
            continue;
        size_t k = dn + strlen(name) + 1;
        out[count] = malloc(k);
        snprintf(out[count++], k, "%s%s", dir, name);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR *d = opendir(dn ? dir : ".");
    if (!d)
        return 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && count < max) {
        const char *name = e->d_name;
        if (!loadable(name) || !strcmp(name, lc_base_name(program)))
            continue;
        size_t k = dn + strlen(name) + 1;
        char *p = malloc(k);
        snprintf(p, k, "%s%s", dir, name);
        struct stat st;
        if (stat(p, &st) || !S_ISREG(st.st_mode)) {
            free(p);
            continue;
        }
        out[count++] = p;
    }
    closedir(d);
#endif
    /* En orden, como la extensión. */
    for (int i = 1; i < count; i++)
        for (int j = i; j > 0 && strcmp(out[j - 1], out[j]) > 0; j--) {
            char *t = out[j];
            out[j] = out[j - 1];
            out[j - 1] = t;
        }
    return count;
}

/* --- La tarjeta y la ROM, como un explorador (versión 6, D112) ------------ */

static int need6(lc_link_t *l)
{
    int r;
    if (!l->version && (r = lc_hello(l, NULL)) < 0)
        return r;
    if (l->version < 6)
        return fail(l, LC_E_OLD, "%s", TR("the machine's firmware is too old to browse the card: update it",
                                          "el firmware de la máquina es demasiado antiguo para ver la tarjeta: "
                                          "actualízalo"));
    return 0;
}

/* Una petición con una ruta (y antes, head): el estado, con el error en
 * texto si falla. */
static int32_t path_request(lc_link_t *l, uint8_t type, const void *head, unsigned hn, const char *path,
                            unsigned timeout, uint8_t *more, unsigned *mn)
{
    size_t pn = strlen(path);
    if (pn > 255 || hn + pn > ZL_MAX_DATA)
        return fail(l, LC_E_FILE, "%s: %s", path, TR("name too long", "nombre demasiado largo"));
    uint8_t d[ZL_MAX_DATA];
    if (hn)
        memcpy(d, head, hn);
    memcpy(d + hn, path, pn);
    int32_t st = lc_request(l, type, d, (unsigned)(hn + pn), timeout, 3, more, mn);
    return st < 0 ? on_file(l, st, path) : st;
}

int lc_list(lc_link_t *l, const char *path, lc_entry_t **out)
{
    int r;
    *out = NULL;
    if ((r = need6(l)) < 0)
        return r;
    lc_entry_t *e = NULL;
    unsigned n = 0, cap = 0;
    for (;;) {
        uint8_t head[2] = { (uint8_t)n, (uint8_t)(n >> 8) }, more[ZL_MAX_DATA];
        unsigned mn = 0;
        int32_t st = path_request(l, ZL_LIST, head, 2, path, 3000, more, &mn);
        if (st < 0) {
            free(e);
            return st;
        }
        if (mn < 1) {
            free(e);
            return fail(l, LC_E_REPLY, "%s", TR("a reply that is too short", "una respuesta demasiado corta"));
        }
        unsigned o = 1;
        for (int32_t i = 0; i < st; i++) {
            if (o + 6 > mn || o + 6 + more[o + 5] > mn) {
                free(e);
                return fail(l, LC_E_REPLY, "%s", TR("a reply that is too short", "una respuesta demasiado corta"));
            }
            if (n == cap) {
                cap = cap ? cap * 2 : 32;
                lc_entry_t *g = realloc(e, cap * sizeof *e);
                if (!g) {
                    free(e);
                    return fail(l, LC_E_FILE, "%s", TR("out of memory", "sin memoria"));
                }
                e = g;
            }
            unsigned nl = more[o + 5] < 63 ? more[o + 5] : 63;
            e[n].size = zl_get32(more + o);
            e[n].dir = (more[o + 4] & 1) != 0;
            memcpy(e[n].name, more + o + 6, nl);
            e[n].name[nl] = 0;
            n++;
            o += 6 + more[o + 5];
        }
        if (!more[0] || st == 0)
            break;
    }
    *out = e;
    return (int)n;
}

int lc_get(lc_link_t *l, const char *path, const char *local)
{
    int r;
    if ((r = need6(l)) < 0)
        return r;
    char tmp[1100];
    snprintf(tmp, sizeof tmp, "%s.zxv-link-part", local);
    FILE *f = fopen(tmp, "wb");
    if (!f)
        return fail(l, LC_E_FILE, "%s: %s", local, TR("cannot write it", "no se puede escribir"));
    const char *name = lc_base_name(path);
    uint32_t at = 0, size = 0;
    for (;;) {
        uint8_t head[4], more[ZL_MAX_DATA];
        unsigned mn = 0;
        zl_put32(head, at);
        int32_t st = path_request(l, ZL_GET, head, 4, path, 3000, more, &mn);
        if (st >= 0 && (mn < 4 || (unsigned)st > mn - 4))
            st = fail(l, LC_E_REPLY, "%s", TR("a reply that is too short", "una respuesta demasiado corta"));
        if (st < 0) {
            fclose(f);
            remove(tmp);
            return st;
        }
        size = zl_get32(more);
        if (st && fwrite(more + 4, 1, (size_t)st, f) != (size_t)st) {
            fclose(f);
            remove(tmp);
            return fail(l, LC_E_FILE, "%s: %s", local, TR("cannot write it", "no se puede escribir"));
        }
        at += (uint32_t)st;
        if (l->on_progress)
            l->on_progress(l->ctx, name, at, size, false);
        if (st == 0 || at >= size)
            break;
    }
    if (fclose(f) != 0 || at != size) {
        remove(tmp);
        return fail(l, LC_E_FILE, "%s: %s", local, TR("cannot write it", "no se puede escribir"));
    }
    remove(local);
    if (rename(tmp, local) != 0) {
        remove(tmp);
        return fail(l, LC_E_FILE, "%s: %s", local, TR("cannot write it", "no se puede escribir"));
    }
    return 0;
}

int lc_remove(lc_link_t *l, const char *path)
{
    int r;
    if ((r = need6(l)) < 0)
        return r;
    int32_t st = path_request(l, ZL_REMOVE, NULL, 0, path, 3000, NULL, NULL);
    return st < 0 ? st : 0;
}

int lc_mkdir(lc_link_t *l, const char *path)
{
    int r;
    if ((r = need6(l)) < 0)
        return r;
    int32_t st = path_request(l, ZL_MKDIR, NULL, 0, path, 3000, NULL, NULL);
    return st < 0 ? st : 0;
}

int lc_rename(lc_link_t *l, const char *from, const char *to)
{
    int r;
    if ((r = need6(l)) < 0)
        return r;
    size_t fn = strlen(from);
    if (fn > 255)
        return fail(l, LC_E_FILE, "%s: %s", from, TR("name too long", "nombre demasiado largo"));
    uint8_t head[256 + 1];
    memcpy(head, from, fn);
    head[fn] = 0;
    int32_t st = path_request(l, ZL_RENAME, head, (unsigned)fn + 1, to, 3000, NULL, NULL);
    return st < 0 ? st : 0;
}

void lc_drive_command(const char *path, char *cmd, size_t n)
{
    const char *base = lc_base_name(path), *colon = strchr(path, ':');
    if (colon && base <= colon)
        base = colon + 1;               /* "SD:a.zxv": la raíz */
    char dir[300];
    size_t dl = 0;
    if (!colon) {                       /* sin unidad: la tarjeta */
        memcpy(dir, "SD:", 3);
        dl = 3;
    }
    size_t pl = (size_t)(base - path);
    if (pl > sizeof dir - dl - 2)
        pl = sizeof dir - dl - 2;
    memcpy(dir + dl, path, pl);
    dl += pl;
    while (dl > 1 && dir[dl - 1] == '/' && dir[dl - 2] != ':')
        dl--;                           /* "SD:/juegos/" -> "SD:/juegos"; "SD:/" se queda */
    if (dir[dl - 1] == ':')
        dir[dl++] = '/';                /* "SD:" -> "SD:/" */
    dir[dl] = 0;
    snprintf(cmd, n, "CD \"%s\": LOAD \"%s\"%s", dir, base, load_tail(path));
}

/* Que el BASIC espere una orden (si hay un programa en marcha, sin
 * allow_stop, LC_E_RUNNING) y teclear cmd. */
static int run_typed(lc_link_t *l, const char *cmd)
{
    lc_info_t info;
    int r;
    if ((r = lc_hello(l, &info)) < 0)
        return r;
    if (!info.waiting && !l->allow_stop)
        return fail(l, LC_E_RUNNING, "%s", TR("a program is running on the board", "hay un programa en marcha en la placa"));
    if ((r = lc_ready(l)) < 0)
        return r;
    return lc_type(l, cmd);
}

int lc_run_path(lc_link_t *l, const char *path)
{
    char cmd[400];
    lc_drive_command(path, cmd, sizeof cmd);
    return run_typed(l, cmd);
}

int lc_run_pc(lc_link_t *l, const char *file)
{
    const char *name = lc_base_name(file);
    char cmd[400];
    int r;
    if (l->ext && !l->ext->only[0]) {   /* ya se sirve una carpeta: además, él */
        if (ext_server_set_extra(l->ext, file, name) < 0)
            return fail(l, LC_E_FILE, "%s: %s", file, TR("cannot be read", "no se puede leer"));
        snprintf(cmd, sizeof cmd, "LOAD \"EXT:%s/%s\"%s", EXT_EXTRA_DIR, name, load_tail(file));
    } else {
        if ((r = lc_serve_file(l, file, name)) < 0)
            return r;
        snprintf(cmd, sizeof cmd, "LOAD \"EXT:/%s\"%s", name, load_tail(file));
    }
    return run_typed(l, cmd);
}

bool lc_ext_busy(const lc_link_t *l)
{
    if (!l->ext)
        return false;
    for (int i = 0; i < 8; i++)
        if (l->ext->files[i].f)
            return true;
    return false;
}

int lc_dbg_read(lc_link_t *l, unsigned off, unsigned n, uint8_t *out)
{
    while (n) {
        unsigned k = n > ZL_CHUNK ? ZL_CHUNK : n, mn = 0;
        uint8_t q[4] = { (uint8_t)off, (uint8_t)(off >> 8), (uint8_t)k, (uint8_t)(k >> 8) };
        uint8_t more[ZL_MAX_DATA];
        int32_t st = lc_request(l, ZL_DBG_READ, q, 4, 2000, 3, more, &mn);
        if (st < 0)
            return st;
        if (mn != k)
            return fail(l, LC_E_REPLY, "%s", TR("unexpected reply", "respuesta inesperada"));
        memcpy(out, more, k);
        out += k;
        off += k;
        n -= k;
    }
    return 0;
}

int lc_dbg_write(lc_link_t *l, unsigned off, const uint8_t *p, unsigned n)
{
    uint8_t d[2 + ZL_CHUNK];
    if (n > ZL_CHUNK)
        return fail(l, LC_E_REPLY, "%s", TR("too much at once", "demasiado de una vez"));
    d[0] = (uint8_t)off;
    d[1] = (uint8_t)(off >> 8);
    memcpy(d + 2, p, n);
    int32_t st = lc_request(l, ZL_DBG_WRITE, d, 2 + n, 2000, 3, NULL, NULL);
    return st < 0 ? st : 0;
}

/* --- Actualizar el firmware (versión 5, D105) ----------------------------------------- */

#define UPD_NAME        "zxv-update.bin"        /* ZXV_UPDATE_FILE en la máquina */
#define UPD_HEADER_MIN  48                      /* la cabecera de zxvupd.h, sin las partes */
#define UPD_START_MS    30000u                  /* lo que tarda en decir que actualiza */
#define UPD_SILENT_MS   60000u                  /* sin contestar más tiempo: algo va mal */
#define UPD_TOTAL_MS    900000u                 /* todo, como mucho (15 minutos) */

int lc_update_version(lc_link_t *l, const char *file, char version[17])
{
    uint8_t h[256];
    FILE *f = fopen(file, "rb");
    size_t n = f ? fread(h, 1, sizeof h, f) : 0;
    if (f)
        fclose(f);
    if (!f)
        return fail(l, LC_E_FILE, "%s: %s", file, TR("cannot be read", "no se puede leer"));
    uint32_t hs = n >= 12 ? zl_get32(h + 8) : 0;
    if (n < UPD_HEADER_MIN || memcmp(h, "ZXVUPD01", 8) || hs < UPD_HEADER_MIN || hs > n ||
        zl_crc32(0, h, hs - 4) != zl_get32(h + hs - 4))
        return fail(l, LC_E_FILE, "%s: %s", file,
                    TR("not a ZX-V update file (zxv-update.bin)",
                       "no es un fichero de actualización de ZX-V (zxv-update.bin)"));
    memcpy(version, h + 16, 16);
    version[16] = 0;
    return 0;
}

/* Un hola que puede no tener respuesta (la máquina se reinicia): el error,
 * sin tocar el que hubiera. */
static int quiet_hello(lc_link_t *l, lc_info_t *i)
{
    char keep[sizeof l->error];
    memcpy(keep, l->error, sizeof keep);
    int r = lc_hello(l, i);
    memcpy(l->error, keep, sizeof keep);
    return r;
}

/* Lo que cuenta la máquina mientras actualiza: va a quien lo enseña, y si
 * un paso dice «error» (en los dos idiomas), se apunta. */
static struct {
    void (*on_log)(void *ctx, const char *text, unsigned n);
    bool failed;
} upd_watch;

static void update_log(void *ctx, const char *text, unsigned n)
{
    for (unsigned k = 0; k + 7 <= n; k++)
        if (!memcmp(text + k, ": error", 7))
            upd_watch.failed = true;
    if (upd_watch.on_log)
        upd_watch.on_log(ctx, text, n);
}

static int update_follow(lc_link_t *l, const char *version);

int lc_update(lc_link_t *l, const char *file)
{
    /* Tras reiniciarse, el modo de actualización solo escucha a J5 (sin la
     * radio): por la red esperaría al PC y fallaría. */
    if (l->net)
        return fail(l, LC_E_UPDATE, "%s",
                    TR("the firmware is updated over J5 (with the USB-serial adapter) or from the card, "
                       "not over the network",
                       "el firmware se actualiza por J5 (con el adaptador USB-serie) o desde la tarjeta, "
                       "no por la red"));
    char version[17];
    int r = lc_update_version(l, file, version);
    if (r < 0)
        return r;
    lc_info_t i;
    if ((r = lc_hello(l, &i)) < 0)
        return r;
    if (l->version < 5)
        return fail(l, LC_E_OLD, "%s",
                    TR("the machine's firmware is too old to be updated over J5: update it once from the card "
                       "(EDIT menu) or over USB",
                       "el firmware de la máquina es demasiado antiguo para actualizarlo por J5: actualízalo una "
                       "vez desde la tarjeta (menú EDIT) o por USB"));
    if (i.updating)
        return fail(l, LC_E_UPDATE, "%s", TR("the machine is already updating its firmware",
                                             "la máquina ya está actualizando el firmware"));
    if ((r = lc_ready(l)) < 0)          /* con un programa en marcha, la máquina se reinicia */
        return r;
    if ((r = lc_serve_file(l, file, UPD_NAME)) < 0)
        return r;
    int32_t st = lc_request(l, ZL_UPDATE, NULL, 0, 2000, 3, NULL, NULL);
    if (st < 0) {
        lc_unserve(l);
        if (st == ZXV_E_OFFLINE)
            return fail(l, st, "%s", TR("the machine does not see this PC as the EXT: drive",
                                        "la máquina no ve este PC como la unidad EXT:"));
        if (st == ZXV_E_ACCESS)
            return fail(l, st, "%s",
                        TR("the machine cannot update over J5 (no partitions: load its firmware once over USB)",
                           "la máquina no puede actualizarse por J5 (sin particiones: grábale el firmware una "
                           "vez por USB)"));
        return st <= LC_E_PORT ? st : fail(l, st, "%s", machine_error(st));
    }

    upd_watch.on_log = l->on_log;
    upd_watch.failed = false;
    l->on_log = update_log;
    r = update_follow(l, version);
    l->on_log = upd_watch.on_log;
    lc_unserve(l);
    return r;
}

/* La máquina se reinicia en el modo de actualización (lee el fichero de
 * aquí), otra vez tras escribir su firmware, y al final vuelve al BASIC:
 * cada pocos segundos, cómo está. */
static int update_follow(lc_link_t *l, const char *version)
{
    lc_info_t i;
    int r;
    unsigned long t0 = now_ms(), heard = t0, next = t0 + 2000;
    bool seen = false;
    for (;;) {
        if ((r = lc_pump(l, 200)) < 0)
            break;
        if (l->cancel) {
            r = fail(l, LC_E_CANCEL, "%s", TR("stopped (the machine goes on: look at its screen)",
                                              "parado (la máquina sigue: mira su pantalla)"));
            break;
        }
        if (upd_watch.failed) {         /* la máquina se reinicia sola al cabo de 30 s */
            r = fail(l, LC_E_UPDATE, "%s", TR("the update failed: look at the machine's screen",
                                              "la actualización ha fallado: mira la pantalla de la máquina"));
            break;
        }
        unsigned long now = now_ms();
        if ((long)(now - next) < 0)
            continue;
        next = now + 2000;
        if (quiet_hello(l, &i) == 0) {
            heard = now;
            if (i.updating) {
                seen = true;
            } else if (seen && i.waiting) {
                char want[64];
                snprintf(want, sizeof want, "ZX-V %s", version);
                r = strcmp(i.firmware, want) ? fail(l, LC_E_UPDATE, TR("the machine came back with %s, not %s: "
                                                                       "look at its screen",
                                                                       "la máquina ha vuelto con %s, no con %s: "
                                                                       "mira su pantalla"),
                                                     i.firmware, want)
                                             : 0;
                break;
            }
        }
        if (!seen && now - t0 > UPD_START_MS) {
            r = fail(l, LC_E_UPDATE, "%s", TR("the machine did not start updating: look at its screen",
                                              "la máquina no ha empezado a actualizar: mira su pantalla"));
            break;
        }
        if (now - heard > UPD_SILENT_MS || now - t0 > UPD_TOTAL_MS) {
            r = fail(l, LC_E_UPDATE, "%s", TR("the machine stopped answering: look at its screen",
                                              "la máquina ha dejado de contestar: mira su pantalla"));
            break;
        }
    }
    return r;
}

/* --- La red (D106) ------------------------------------------------------------------------- */

bool lc_net_compatible(unsigned version, char *why, size_t n)
{
    if (version == ZLN_VERSION)
        return true;
    if (version < ZLN_VERSION)
        snprintf(why, n,
                 TR("the machine's firmware speaks an older version of the network protocol (%u; this zxv-link, %u): "
                    "they are not compatible. Update the machine (zxv-link update over J5, or from the SD card)",
                    "el firmware de la máquina habla una versión anterior del protocolo de la red (la %u; este "
                    "zxv-link, la %u): no son compatibles. Actualiza la máquina (zxv-link update por J5, o desde la "
                    "tarjeta)"),
                 version, (unsigned)ZLN_VERSION);
    else
        snprintf(why, n,
                 TR("the machine speaks a newer version of the network protocol (%u; this zxv-link, %u): they are not "
                    "compatible. Update zxv-link",
                    "la máquina habla una versión más nueva del protocolo de la red (la %u; este zxv-link, la %u): no "
                    "son compatibles. Actualiza zxv-link"),
                 version, (unsigned)ZLN_VERSION);
    return false;
}

/* Una línea más detrás de lo que hay en out (n bytes), si cabe entera. */
static bool add_line(char *out, size_t n, const char *fmt, ...)
{
    size_t used = strlen(out);
    va_list ap;
    va_start(ap, fmt);
    int w = vsnprintf(out + used, n - used, fmt, ap);
    va_end(ap);
    if (w < 0 || (size_t)w >= n - used) {
        out[used] = 0;
        return false;
    }
    return true;
}

int lc_list_found(const netpc_found_t *list, int k, char *out, size_t n, char *other, size_t on)
{
    int count = 0;
    for (int i = 0; i < k; i++) {
        char why[256];
        if (lc_net_compatible(list[i].version, why, sizeof why))
            count += add_line(out, n, "%s (%s)\n", list[i].name, list[i].addr);
        else if (other && on)
            add_line(other, on, "%s (%s): %s: %s\n", list[i].name, list[i].addr, list[i].firmware, why);
    }
    return count;
}

int lc_list_ports(char *out, size_t n, char *other, size_t on, unsigned ms)
{
    int count = serial_list(out, n);
    if (other && on)
        other[0] = 0;
    char cfg[1024];
    if (!net_cfg(cfg, sizeof cfg))
        return count;
    netpc_found_t list[16];
    int k = netpc_find(cfg, list, 16, ms);
    return count + lc_list_found(list, k, out, n, other, on);
}

int lc_pair_begin(lc_link_t *l, const char *pc_name, uint32_t *code)
{
    if (!l->net)
        return fail(l, LC_E_PORT, "%s", TR("pairing is only for machines on the network",
                                          "emparejar es solo para las máquinas de la red"));
    char why[160] = "";
    int r = netpc_pair_begin(l->net, pc_name, code, why);
    if (r == 0)
        return 0;
    if (r == -100)
        return fail(l, LC_E_NOANSWER, "%s", TR("the machine does not answer", "la máquina no contesta"));
    if (r == -ZLN_PAIR_CLOSED)
        return fail(l, LC_E_UNPAIRED, "%s", TR("the machine is not pairing: open EDIT > Wi-Fi > Pair a PC on it",
                                               "la máquina no está emparejando: abre en ella EDIT > Wi-Fi > Emparejar un PC"));
    if (r == -ZLN_PAIR_BUSY)
        return fail(l, LC_E_UNPAIRED, "%s", TR("the machine is pairing with another PC", "la máquina está emparejando con otro PC"));
    return fail(l, LC_E_UNPAIRED, "%s",
                !strcmp(why, "commit") ? TR("the machine's answers do not add up: someone may be in the middle; "
                                            "pairing stopped",
                                            "las respuestas de la máquina no cuadran: puede haber alguien en medio; "
                                            "no se empareja")
                                       : TR("the machine said no", "la máquina ha dicho que no"));
}

int lc_pair_poll(lc_link_t *l)
{
    if (!l->net)
        return ZLN_PAIR_NO;
    int r = netpc_pair_poll(l->net);
    if (r == ZLN_PAIR_OK)
        l->unpaired = false;
    return r;
}
