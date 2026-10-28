/*
 * zxvc.c - La biblioteca estándar de C (picolibc) sobre la API de ZX-V: lo
 * que picolibc deja al sistema (open, read, write, lseek, close, unlink,
 * rename, mkdir, rmdir, stat, chdir, getcwd, gettimeofday, _exit), las
 * carpetas de <dirent.h> y la consola (stdin, stdout y stderr). Con ella
 * funcionan fopen, fgets, fprintf, printf, malloc, time...
 *
 * Los descriptores 0, 1 y 2 son la consola (la pantalla y el teclado, por
 * print_char y key_get); los de los ficheros son los de la API más 3. El
 * montón va del final del programa hasta 32 KB por debajo del final de la
 * RAM de usuario, donde está la pila (zxv.ld).
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "zxv_api.h"

#define FD_FIRST 3

/* --- Errores ------------------------------------------------------------------ */

static int fail(int r)
{
    static const struct { signed char zxv; unsigned char posix; } map[] = {
        { ZXV_E_NOENT, ENOENT }, { ZXV_E_EXIST, EEXIST }, { ZXV_E_NAME, EINVAL },
        { ZXV_E_NOTDIR, ENOTDIR }, { ZXV_E_ISDIR, EISDIR }, { ZXV_E_NOTEMPTY, ENOTEMPTY },
        { ZXV_E_BADF, EBADF }, { ZXV_E_MFILE, EMFILE }, { ZXV_E_ACCESS, EACCES },
        { ZXV_E_FAULT, EFAULT }, { ZXV_E_NOCARD, ENODEV }, { ZXV_E_NOSPC, ENOSPC },
        { ZXV_E_FORMAT, ENOEXEC }, { ZXV_E_RANGE, ERANGE }, { ZXV_E_INVAL, EINVAL },
    };
    errno = EIO;
    for (unsigned i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if (map[i].zxv == r)
            errno = map[i].posix;
    return -1;
}

/* Un resultado de la API: >= 0 tal cual, < 0 como errno. */
static int api(int r) { return r < 0 ? fail(r) : r; }

/* --- La consola ----------------------------------------------------------------- */

/* La consola habla UTF-8, como las cadenas de C: lo que se escribe se junta
 * por letras (print_char quiere el código Unicode) y lo que se teclea sale en
 * UTF-8 (fgets da "año" en 4 bytes). */
static int console_put(char ch, FILE *f)
{
    (void)f;
    static uint32_t cp;                 /* la letra a medias */
    static int more;                    /* los bytes que le faltan */
    unsigned char c = (unsigned char)ch;
    if (more && (c & 0xC0) == 0x80) {
        cp = cp << 6 | (c & 0x3F);
        if (--more == 0)
            zxv_print_char((int)cp);
        return c;
    }
    more = 0;                           /* una secuencia rota: lo que venga, solo */
    if (c >= 0xC2 && c <= 0xF4) {
        more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1;
        cp = c & (0x3F >> more);
    } else if (c < 0x80) {
        zxv_print_char(c == '\n' ? 13 : c);
    } else {
        zxv_print_char('?');            /* un byte suelto que no es UTF-8 */
    }
    return c;
}

/* Una tecla, esperando; con eco. Intro es '\n'. Las letras de fuera de ASCII
 * salen en UTF-8: el primer byte ahora y los demás en las llamadas siguientes. */
static int console_get(FILE *f)
{
    (void)f;
    static unsigned char pend[3];
    static int npend, next;
    if (next < npend)
        return pend[next++];
    int k;
    while ((k = zxv_key_get()) < 0)
        zxv_wait_vblank();
    if (k == 13)
        k = '\n';
    if (k >= (int)ZXV_KEY_SPECIAL)
        return ' ';                     /* las flechas, F1...: sin eco */
    if (k == '\n')
        zxv_print_char(13);
    else
        zxv_print_char(k);
    if (k < 0x80)
        return k;
    next = 0;
    if (k < 0x800) {
        pend[0] = (unsigned char)(0x80 | (k & 0x3F));
        npend = 1;
        return 0xC0 | k >> 6;
    }
    pend[0] = (unsigned char)(0x80 | (k >> 6 & 0x3F));
    pend[1] = (unsigned char)(0x80 | (k & 0x3F));
    npend = 2;
    return 0xE0 | k >> 12;
}

static FILE console = FDEV_SETUP_STREAM(console_put, console_get, NULL, _FDEV_SETUP_RW);
FILE *const stdin = &console;
FILE *const stdout = &console;
FILE *const stderr = &console;

/* --- Ficheros ------------------------------------------------------------------- */

int open(const char *path, int flags, ...)
{
    uint32_t f = 0;
    switch (flags & O_ACCMODE) {
    case O_RDONLY: f = ZXV_O_READ; break;
    case O_WRONLY: f = ZXV_O_WRITE; break;
    default: f = ZXV_O_READ | ZXV_O_WRITE; break;
    }
    if (flags & O_CREAT)
        f |= ZXV_O_CREATE;
    if (flags & O_TRUNC)
        f |= ZXV_O_TRUNC;
    if (flags & O_APPEND)
        f |= ZXV_O_APPEND;
    if ((flags & O_EXCL) && (flags & O_CREAT)) {
        zxv_dirent_t e;
        if (zxv_file_stat(path, &e) == ZXV_OK)
            return fail(ZXV_E_EXIST);
    }
    int h = zxv_file_open(path, f);
    return h < 0 ? fail(h) : h + FD_FIRST;
}

int close(int fd)
{
    if (fd < FD_FIRST)
        return 0;
    return api(zxv_file_close(fd - FD_FIRST));
}

ssize_t read(int fd, void *buf, size_t n)
{
    if (fd < FD_FIRST) {
        char *p = buf;
        size_t i = 0;
        while (i < n) {
            int c = console_get(NULL);
            p[i++] = (char)c;
            if (c == '\n')
                break;
        }
        return (ssize_t)i;
    }
    return api(zxv_file_read(fd - FD_FIRST, buf, n));
}

ssize_t write(int fd, const void *buf, size_t n)
{
    if (fd < FD_FIRST) {
        const char *p = buf;
        for (size_t i = 0; i < n; i++)
            console_put(p[i], NULL);
        return (ssize_t)n;
    }
    return api(zxv_file_write(fd - FD_FIRST, buf, n));
}

off_t lseek(int fd, off_t off, int whence)
{
    if (fd < FD_FIRST)
        return fail(ZXV_E_INVAL);
    int w = whence == SEEK_SET ? ZXV_SEEK_SET : whence == SEEK_CUR ? ZXV_SEEK_CUR : ZXV_SEEK_END;
    return api(zxv_file_seek(fd - FD_FIRST, (int32_t)off, w));
}

int isatty(int fd) { return fd < FD_FIRST; }

int unlink(const char *path) { return api(zxv_file_remove(path)) < 0 ? -1 : 0; }

int rename(const char *from, const char *to) { return api(zxv_file_rename(from, to)) < 0 ? -1 : 0; }

int mkdir(const char *path, mode_t mode)
{
    (void)mode;
    return api(zxv_dir_make(path)) < 0 ? -1 : 0;
}

static void fill_stat(const zxv_dirent_t *e, struct stat *st)
{
    memset(st, 0, sizeof(*st));
    st->st_size = (off_t)e->size;
    st->st_mode = (e->attr & ZXV_ATTR_DIR) ? (S_IFDIR | 0777) : (S_IFREG | 0666);
    st->st_nlink = 1;
}

int stat(const char *path, struct stat *st)
{
    zxv_dirent_t e;
    int r = zxv_file_stat(path, &e);
    if (r < 0)
        return fail(r);
    fill_stat(&e, st);
    return 0;
}

int fstat(int fd, struct stat *st)
{
    memset(st, 0, sizeof(*st));
    if (fd < FD_FIRST) {
        st->st_mode = S_IFCHR;
        return 0;
    }
    /* La API no tiene fstat: el tamaño, yendo al final y volviendo. */
    int h = fd - FD_FIRST, pos = zxv_file_seek(h, 0, ZXV_SEEK_CUR);
    if (pos < 0)
        return fail(pos);
    int end = zxv_file_seek(h, 0, ZXV_SEEK_END);
    zxv_file_seek(h, pos, ZXV_SEEK_SET);
    if (end < 0)
        return fail(end);
    st->st_size = end;
    st->st_mode = S_IFREG | 0666;
    st->st_nlink = 1;
    return 0;
}

int rmdir(const char *path)
{
    struct stat st;
    if (stat(path, &st))
        return -1;
    if (!S_ISDIR(st.st_mode))
        return fail(ZXV_E_NOTDIR);
    return unlink(path);
}

int chdir(const char *path) { return api(zxv_dir_change(path)) < 0 ? -1 : 0; }

char *getcwd(char *buf, size_t size)
{
    int r = zxv_dir_current(buf, (uint32_t)size);
    if (r < 0) {
        fail(r);
        return NULL;
    }
    return buf;
}

/* --- Carpetas ------------------------------------------------------------------- */

static DIR dirs[ZXV_MAX_DIRS];

DIR *opendir(const char *path)
{
    if (strlen(path) >= sizeof(dirs[0].path)) {
        fail(ZXV_E_NAME);
        return NULL;
    }
    for (int i = 0; i < ZXV_MAX_DIRS; i++)
        if (!dirs[i].path[0]) {
            int d = zxv_dir_open(path);
            if (d < 0) {
                fail(d);
                return NULL;
            }
            dirs[i].handle = d;
            strcpy(dirs[i].path, *path ? path : ".");
            return &dirs[i];
        }
    fail(ZXV_E_MFILE);
    return NULL;
}

struct dirent *readdir(DIR *d)
{
    zxv_dirent_t e;
    if (d->handle < 0 && (d->handle = zxv_dir_open(d->path)) < 0) {
        fail(d->handle);
        return NULL;
    }
    int r = zxv_dir_read(d->handle, &e);
    if (r <= 0) {
        if (r < 0)
            fail(r);
        return NULL;
    }
    memcpy(d->entry.d_name, e.name, sizeof(d->entry.d_name));
    d->entry.d_name[MAXNAMLEN] = 0;
    d->entry.d_size = e.size;
    d->entry.d_type = (e.attr & ZXV_ATTR_DIR) ? DT_DIR : DT_REG;
    return &d->entry;
}

void rewinddir(DIR *d)
{
    if (d->handle >= 0)
        zxv_dir_close(d->handle);
    d->handle = -1;                     /* se vuelve a abrir en el siguiente readdir */
}

int closedir(DIR *d)
{
    int r = d->handle >= 0 ? zxv_dir_close(d->handle) : 0;
    d->path[0] = 0;
    return r < 0 ? fail(r) : 0;
}

/* --- Hora y salida ------------------------------------------------------------------ */

/* Días desde el 1-1-1970 (calendario gregoriano, de Howard Hinnant). */
static int32_t days_from_civil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    int era = y / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int32_t)doe - 719468;
}

/* La hora del reloj de la máquina (la local, spec §7.9). Sin reloj en hora,
 * el 1 de enero de 2026. */
int gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    zxv_clock_t c;
    if (zxv_clock_get(&c) != ZXV_OK)
        c = (zxv_clock_t){ 2026, 1, 1, 0, 0, 0, 4 };
    tv->tv_sec = (time_t)days_from_civil(c.year, c.month, c.day) * 86400 + c.hour * 3600 + c.min * 60 + c.sec;
    tv->tv_usec = 0;
    return 0;
}

/* exit(): vuelve al BASIC con el código. */
void _exit(int code)
{
    zxv_exit_to_basic(code);
    for (;;)
        ;
}
