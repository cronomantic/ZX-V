/*
 * extserve.c - El servidor de referencia de la unidad EXT: (ver extserve.h).
 */
#ifndef _WIN32
#define _DEFAULT_SOURCE
#endif
#include "extserve.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define mkdir_(p)       _mkdir(p)
#define rmdir_(p)       _rmdir(p)
#else
#include <dirent.h>
#include <unistd.h>
#define mkdir_(p)       mkdir(p, 0777)
#define rmdir_(p)       rmdir(p)
#endif

#include "linkcore.h"

/* Los errores de la máquina que da (spec §7.7). */
#define E_NOENT         (-1)
#define E_EXIST         (-2)
#define E_NAME          (-3)
#define E_NOTDIR        (-4)
#define E_ISDIR         (-5)
#define E_NOTEMPTY      (-6)
#define E_BADF          (-7)
#define E_ACCESS        (-9)
#define E_IO            (-11)
#define E_NOSPC         (-13)
#define E_INVAL         (-17)

#define O_READ          0x01u           /* los ZXV_O_* de file_open */
#define O_WRITE         0x02u
#define O_CREATE        0x04u
#define O_TRUNC         0x08u
#define O_APPEND        0x10u

#define WRITTEN_FILE    "zxv-link-ext.txt"      /* lo que escribió la máquina */

static int from_errno(void)
{
    switch (errno) {
    case ENOENT: return E_NOENT;
    case EEXIST: return E_EXIST;
    case ENOTDIR: return E_NOTDIR;
    case EISDIR: return E_ISDIR;
#ifdef ENOTEMPTY
    case ENOTEMPTY: return E_NOTEMPTY;
#endif
    case EACCES:
    case EPERM: return E_ACCESS;
    case ENOSPC: return E_NOSPC;
    default: return E_IO;
    }
}

static void event(ext_server_t *x, const char *what, const char *zpath)
{
    if (x->on_event)
        x->on_event(x->ctx, what, zpath);
}

/* --- Los nombres ---------------------------------------------------------------------- */

/* Los de la tarjeta (spec §7.7): 1-63 ASCII imprimibles, sin \ / : * ? " < > |,
 * sin acabar en espacio ni en punto. */
static bool valid_name(const char *s, size_t n)
{
    if (n == 0 || n > 63 || s[n - 1] == ' ' || s[n - 1] == '.')
        return false;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x20 || c > 0x7E || strchr("\\/:*?\"<>|", c))
            return false;
    }
    return true;
}

static bool same_ci(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; i++)
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
            return false;
    return b[n] == 0;
}

static int cmp_ci(const void *a, const void *b)
{
    const char *p = ((const ext_entry_t *)a)->name, *q = ((const ext_entry_t *)b)->name;
    for (;; p++, q++) {
        int d = tolower((unsigned char)*p) - tolower((unsigned char)*q);
        if (d || !*p)
            return d;
    }
}

/* --- La carpeta del anfitrión ---------------------------------------------------------- */

static bool host_stat(const char *path, bool *dir, uint32_t *size)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return false;
    if (dir)
        *dir = (st.st_mode & S_IFMT) == S_IFDIR;
    if (size)
        *size = (uint32_t)st.st_size;
    return true;
}

/* Lo que hay en una carpeta que la máquina puede ver (sin los ocultos, ni
 * los temporales, ni los nombres que no valen): en *out (malloc), cuántos, o
 * -1. */
static int list_dir(const char *host, ext_entry_t **out)
{
    unsigned n = 0, cap = 16;
    ext_entry_t *e = malloc(cap * sizeof *e);
    if (!e)
        return -1;
#ifdef _WIN32
    char pat[EXT_HOST_PATH + 4];
    snprintf(pat, sizeof pat, "%s\\*", host);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        free(e);
        return -1;
    }
    do {
        const char *name = fd.cFileName;
        if (name[0] == '.' || (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) ||
            !valid_name(name, strlen(name)))
            continue;
        bool dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        uint32_t size = dir ? 0 : fd.nFileSizeLow;
#else
    DIR *dh = opendir(host);
    if (!dh) {
        free(e);
        return -1;
    }
    struct dirent *de;
    while ((de = readdir(dh)) != NULL) {
        const char *name = de->d_name;
        if (name[0] == '.' || !valid_name(name, strlen(name)))
            continue;
        char full[EXT_HOST_PATH];
        bool dir;
        uint32_t size;
        if (snprintf(full, sizeof full, "%s/%s", host, name) >= (int)sizeof full || !host_stat(full, &dir, &size))
            continue;
        if (dir)
            size = 0;
#endif
        if (n == cap) {
            ext_entry_t *bigger = realloc(e, (cap *= 2) * sizeof *e);
            if (!bigger)
                break;
            e = bigger;
        }
        snprintf(e[n].name, sizeof e[n].name, "%.63s", name);  /* ya comprobado: 63 como mucho */
        e[n].size = size;
        e[n].attr = dir ? 1u : 0u;
        n++;
#ifdef _WIN32
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    }
    closedir(dh);
#endif
    *out = e;
    return (int)n;
}

/*
 * La ruta de la máquina ("/a/b", absoluta desde la raíz de lo que se sirve)
 * en el anfitrión, buscando cada parte sin distinguir mayúsculas: en out, y
 * si existe y es carpeta. La última parte puede no existir (para crear); las
 * de antes, sí. 0 o un error.
 */
static int host_path(const ext_server_t *x, const char *zpath, char *out, bool *exists, bool *dir)
{
    if (zpath[0] != '/' || strlen(zpath) > 255)
        return E_NAME;
    size_t o = (size_t)snprintf(out, EXT_HOST_PATH, "%s", x->root);
    *exists = true;
    *dir = true;
    const char *p = zpath;
    while (*p) {
        while (*p == '/')
            p++;
        if (!*p)
            break;
        const char *q = p;
        while (*q && *q != '/')
            q++;
        size_t n = (size_t)(q - p);
        if (!valid_name(p, n))
            return E_NAME;
        if (!*exists)
            return E_NOENT;             /* una carpeta de la ruta no existe */
        if (!*dir)
            return E_NOTDIR;
        ext_entry_t *e;
        int k = list_dir(out, &e);
        if (k < 0)
            return E_IO;
        const char *found = NULL;
        for (int i = 0; i < k && !found; i++)
            if (same_ci(p, e[i].name, n))
                found = e[i].name;
        if (o + 1 + n + 1 > EXT_HOST_PATH) {
            free(e);
            return E_NAME;
        }
        out[o++] = '/';
        if (found) {
            memcpy(out + o, found, n);
            *dir = false;
            for (int i = 0; i < k; i++)
                if (e[i].name == found)
                    *dir = e[i].attr & 1u;
        } else {
            memcpy(out + o, p, n);      /* como la pide: es la grafía al crear */
            *exists = false;
            *dir = false;
        }
        o += n;
        out[o] = 0;
        free(e);
        p = q;
    }
    return 0;
}

/* --- Lo que escribió la máquina ---------------------------------------------------------- */

static bool file_crc(const char *path, uint32_t *size, uint32_t *crc)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    uint8_t buf[4096];
    size_t k;
    *size = 0;
    *crc = 0;
    while ((k = fread(buf, 1, sizeof buf, f)) > 0) {
        *crc = zl_crc32(*crc, buf, k);
        *size += (uint32_t)k;
    }
    fclose(f);
    return true;
}

/* Si host (con ese tamaño y CRC) es lo último que escribió la máquina con
 * ese nombre; y apuntarlo, o quitarlo (size == ~0). */
static bool written(const char *host, uint32_t size, uint32_t crc, bool note)
{
    char path[700], line[EXT_HOST_PATH + 40];
    bool found = false;
    if (!lc_config_file(WRITTEN_FILE, path, sizeof path))
        return false;
    FILE *f = fopen(path, "r");
    char *keep = NULL;
    size_t kn = 0, kcap = 0;
    if (f) {
        while (fgets(line, sizeof line, f)) {
            unsigned long c, s;
            int at = 0;
            line[strcspn(line, "\r\n")] = 0;
            if (sscanf(line, "%lx %lu %n", &c, &s, &at) < 2 || !at)
                continue;
            bool same = !strcmp(line + at, host);
            if (same && (uint32_t)c == crc && (uint32_t)s == size)
                found = true;
            if (note && !same) {        /* los demás se quedan */
                size_t ln = strlen(line);
                if (kn + ln + 2 > kcap) {
                    char *bigger = realloc(keep, kcap = (kcap + ln + 2) * 2);
                    if (!bigger)
                        break;
                    keep = bigger;
                }
                memcpy(keep + kn, line, ln);
                kn += ln;
                keep[kn++] = '\n';
            }
        }
        fclose(f);
    }
    if (note && (f = fopen(path, "w")) != NULL) {
        if (kn)
            fwrite(keep, 1, kn, f);
        if (size != ~0u)
            fprintf(f, "%08lx %lu %s\n", (unsigned long)crc, (unsigned long)size, host);
        fclose(f);
    }
    free(keep);
    return found;
}

/* Lo que hay en host y no escribió la máquina (si no se permite
 * sustituirlo). */
static bool foreign(const ext_server_t *x, const char *host)
{
    uint32_t size, crc;
    if (x->overwrite || !file_crc(host, &size, &crc))
        return false;
    return !written(host, size, crc, false);
}

/* --- Las peticiones --------------------------------------------------------------------- */

int ext_server_start(ext_server_t *x, const char *folder)
{
    bool dir;
    if (!host_stat(folder, &dir, NULL) || !dir)
        return -1;
    ext_server_close_all(x);
    snprintf(x->root, sizeof x->root, "%s", folder);
    size_t n = strlen(x->root);
    while (n > 1 && (x->root[n - 1] == '/' || x->root[n - 1] == '\\'))
        x->root[--n] = 0;
    x->has_last = false;
    return 0;
}

static void close_file(ext_server_t *x, int i, bool keep)
{
    if (!x->files[i].f)
        return;
    fclose(x->files[i].f);
    x->files[i].f = NULL;
    if (!x->files[i].writing)
        return;
    if (!keep) {
        remove(x->files[i].tmp);        /* sin acabar: el de antes se queda */
        return;
    }
    remove(x->files[i].path);
    if (rename(x->files[i].tmp, x->files[i].path) != 0) {
        remove(x->files[i].tmp);
        return;
    }
    uint32_t size, crc;
    if (file_crc(x->files[i].path, &size, &crc))
        written(x->files[i].path, size, crc, true);
    event(x, "write", x->files[i].zpath);
}

static void close_dir(ext_server_t *x, int i)
{
    free(x->dirs[i].e);
    x->dirs[i].e = NULL;
    x->dirs[i].used = false;
}

void ext_server_close_all(ext_server_t *x)
{
    for (int i = 0; i < 8; i++)
        close_file(x, i, false);
    for (int i = 0; i < 4; i++)
        close_dir(x, i);
    x->has_last = false;
}

void ext_server_boot(ext_server_t *x, uint32_t boot_id)
{
    if (x->boot_id && boot_id != x->boot_id)
        ext_server_close_all(x);
    x->boot_id = boot_id;
}

static bool copy_file(const char *from, const char *to)
{
    FILE *a = fopen(from, "rb"), *b = a ? fopen(to, "wb") : NULL;
    bool ok = a && b;
    uint8_t buf[4096];
    size_t k;
    while (ok && (k = fread(buf, 1, sizeof buf, a)) > 0)
        ok = fwrite(buf, 1, k, b) == k;
    if (a)
        fclose(a);
    if (b && fclose(b) != 0)
        ok = false;
    return ok;
}

/* El temporal de host (EXT_HOST_PATH + 16 bytes): el mismo nombre con un
 * punto delante (oculto). */
static void tmp_of(const char *host, char *tmp)
{
    const char *slash = strrchr(host, '/');
    size_t d = slash ? (size_t)(slash - host + 1) : 0, n = strlen(host);
    memcpy(tmp, host, d);
    tmp[d] = '.';
    memcpy(tmp + d + 1, host + d, n - d);
    memcpy(tmp + n + 1, ".zxv-tmp", 9);
}

static int32_t op_open(ext_server_t *x, int slot, uint32_t mode, const char *zpath)
{
    if (slot < 0 || slot >= 8)
        return E_BADF;
    close_file(x, slot, false);
    char host[EXT_HOST_PATH];
    bool exists, dir;
    int r = host_path(x, zpath, host, &exists, &dir);
    if (r)
        return r;
    bool w = mode & O_WRITE;
    if (exists && dir)
        return E_ISDIR;
    if (!exists && !(w && (mode & O_CREATE)))
        return E_NOENT;
    if (!w && !(mode & O_READ))
        return E_INVAL;
    if (!w) {
        FILE *f = fopen(host, "rb");
        if (!f)
            return from_errno();
        x->files[slot].f = f;
        x->files[slot].writing = false;
        event(x, "read", zpath);
        return 0;
    }
    if (exists && foreign(x, host))
        return E_ACCESS;
    char tmp[EXT_HOST_PATH + 16];
    tmp_of(host, tmp);
    if (exists && !(mode & O_TRUNC) && !copy_file(host, tmp))
        return E_IO;
    FILE *f = fopen(tmp, exists && !(mode & O_TRUNC) ? "r+b" : "w+b");
    if (!f)
        return from_errno();
    x->files[slot].f = f;
    x->files[slot].writing = true;
    x->files[slot].append = (mode & O_APPEND) != 0;
    snprintf(x->files[slot].path, EXT_HOST_PATH, "%s", host);
    memcpy(x->files[slot].tmp, tmp, sizeof tmp);
    snprintf(x->files[slot].zpath, sizeof x->files[slot].zpath, "%.255s", zpath);
    return 0;
}

static unsigned handle(ext_server_t *x, const zl_msg_t *m, uint8_t *out)
{
    const uint8_t *d = m->data;
    unsigned n = m->len;
    int32_t st = E_INVAL;
    unsigned more = 0;                  /* lo que va detrás del estado */
    char zpath[260], zto[260];
    int slot = n ? d[0] : -1;
    bool file_ok = slot >= 0 && slot < 8 && x->files[slot].f;

    switch (m->type) {
    case ZLX_OPEN:
        if (n >= 5 && n - 5 < sizeof zpath) {
            memcpy(zpath, d + 5, n - 5);
            zpath[n - 5] = 0;
            st = op_open(x, slot, zl_get32(d + 1), zpath);
        }
        break;
    case ZLX_READ:
        if (!file_ok) {
            st = E_BADF;
        } else if (n == 3) {
            unsigned want = (unsigned)(d[1] | d[2] << 8);
            if (want > ZL_CHUNK)
                want = ZL_CHUNK;
            size_t k = fread(out + 4, 1, want, x->files[slot].f);
            st = (int32_t)k;
            more = (unsigned)k;
        }
        break;
    case ZLX_WRITE:
        if (!file_ok) {
            st = E_BADF;
        } else if (!x->files[slot].writing) {
            st = E_ACCESS;
        } else if (n >= 1) {
            FILE *f = x->files[slot].f;
            if (x->files[slot].append)
                fseek(f, 0, SEEK_END);
            size_t k = fwrite(d + 1, 1, n - 1, f);
            st = k == n - 1 ? (int32_t)k : (k ? (int32_t)k : E_NOSPC);
        }
        break;
    case ZLX_CLOSE:
        if (!file_ok) {
            st = E_BADF;
        } else {
            close_file(x, slot, true);
            st = 0;
        }
        break;
    case ZLX_SEEK:
        if (!file_ok) {
            st = E_BADF;
        } else if (n == 6 && d[5] <= 2) {
            static const int whence[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
            long to = (long)(int32_t)zl_get32(d + 1);
            FILE *f = x->files[slot].f;
            if (fseek(f, to, whence[d[5]]) != 0) {
                st = E_INVAL;
            } else {
                long at = ftell(f);
                st = at < 0 ? E_IO : (int32_t)at;
            }
        }
        break;
    case ZLX_STAT:
    case ZLX_REMOVE:
    case ZLX_MKDIR: {
        if (n >= sizeof zpath)
            break;
        memcpy(zpath, d, n);
        zpath[n] = 0;
        char host[EXT_HOST_PATH];
        bool exists, dir;
        if ((st = host_path(x, zpath, host, &exists, &dir)) != 0)
            break;
        if (m->type == ZLX_MKDIR) {
            st = exists ? E_EXIST : mkdir_(host) == 0 ? 0 : from_errno();
            if (!st)
                event(x, "mkdir", zpath);
        } else if (!exists) {
            st = E_NOENT;
        } else if (m->type == ZLX_STAT) {
            uint32_t size = 0;
            host_stat(host, NULL, &size);
            zl_put32(out + 4, dir ? 0 : size);
            zl_put32(out + 8, dir ? 1u : 0u);
            more = 8;
            st = 0;
        } else if (!strcmp(zpath, "/")) {
            st = E_ACCESS;              /* la raíz, no */
        } else if (dir) {
            st = rmdir_(host) == 0 ? 0 : from_errno();
            if (st == E_EXIST || st == E_ACCESS)
                st = E_NOTEMPTY;        /* Windows lo dice así */
        } else if (foreign(x, host)) {
            st = E_ACCESS;
        } else {
            st = remove(host) == 0 ? 0 : from_errno();
            if (!st) {
                written(host, ~0u, 0, true);
                event(x, "remove", zpath);
            }
        }
        break;
    }
    case ZLX_RENAME: {
        const uint8_t *zero = memchr(d, 0, n);
        if (!zero || (size_t)(zero - d) >= sizeof zpath || n - (size_t)(zero - d) - 1 >= sizeof zto)
            break;
        memcpy(zpath, d, (size_t)(zero - d) + 1);
        size_t tn = n - (size_t)(zero - d) - 1;
        memcpy(zto, zero + 1, tn);
        zto[tn] = 0;
        char a[EXT_HOST_PATH], b[EXT_HOST_PATH];
        bool ae, ad, be, bd;
        if ((st = host_path(x, zpath, a, &ae, &ad)) != 0 || (st = host_path(x, zto, b, &be, &bd)) != 0)
            break;
        if (!ae)
            st = E_NOENT;
        else if (be)
            st = E_EXIST;
        else if (!ad && foreign(x, a))
            st = E_ACCESS;
        else if (rename(a, b) != 0)
            st = from_errno();
        else {
            st = 0;
            uint32_t size, crc;
            if (!ad && file_crc(b, &size, &crc)) {
                written(a, ~0u, 0, true);
                written(b, size, crc, true);
            }
            event(x, "rename", zto);
        }
        break;
    }
    case ZLX_OPENDIR:
        if (n >= 1 && n - 1 < sizeof zpath && slot < 4) {
            memcpy(zpath, d + 1, n - 1);
            zpath[n - 1] = 0;
            char host[EXT_HOST_PATH];
            bool exists, dir;
            if ((st = host_path(x, zpath, host, &exists, &dir)) != 0)
                break;
            if (!exists)
                st = E_NOENT;
            else if (!dir)
                st = E_NOTDIR;
            else {
                close_dir(x, slot);
                ext_entry_t *e;
                int k = list_dir(host, &e);
                if (k < 0) {
                    st = E_IO;
                } else {
                    qsort(e, (size_t)k, sizeof *e, cmp_ci);
                    x->dirs[slot].e = e;
                    x->dirs[slot].n = (unsigned)k;
                    x->dirs[slot].next = 0;
                    x->dirs[slot].used = true;
                    st = 0;
                }
            }
        }
        break;
    case ZLX_READDIR:
        if (n != 1 || slot >= 4 || !x->dirs[slot].used) {
            st = E_BADF;
        } else if (x->dirs[slot].next >= x->dirs[slot].n) {
            st = 0;
        } else {
            const ext_entry_t *e = &x->dirs[slot].e[x->dirs[slot].next++];
            zl_put32(out + 4, e->size);
            zl_put32(out + 8, e->attr);
            size_t k = strlen(e->name);
            memcpy(out + 12, e->name, k);
            more = 8 + (unsigned)k;
            st = 1;
        }
        break;
    case ZLX_CLOSEDIR:
        if (n != 1 || slot >= 4 || !x->dirs[slot].used) {
            st = E_BADF;
        } else {
            close_dir(x, slot);
            st = 0;
        }
        break;
    }
    zl_put32(out, (uint32_t)st);
    return 4 + more;
}

unsigned ext_server_handle(ext_server_t *x, const zl_msg_t *m, uint8_t *out)
{
    /* La misma otra vez (se perdió la respuesta): la misma respuesta. */
    if (x->has_last && m->type == x->last_type && m->seq == x->last_seq) {
        memcpy(out, x->last, x->last_len);
        return x->last_len;
    }
    unsigned n = handle(x, m, out);
    x->has_last = true;
    x->last_type = m->type;
    x->last_seq = m->seq;
    x->last_len = n;
    memcpy(x->last, out, n);
    return n;
}
