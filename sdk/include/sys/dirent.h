/*
 * sys/dirent.h - Las carpetas de la tarjeta para <dirent.h> (picolibc trae
 * aquí un #error: no sabe de qué sistema son). Las implementa zxvc.c sobre
 * dir_open, dir_read y dir_close de la API (spec §7.7).
 */
#ifndef ZXV_SYS_DIRENT_H
#define ZXV_SYS_DIRENT_H

#include <stdint.h>
#include <sys/types.h>                  /* ssize_t y size_t: los usa <dirent.h> desde picolibc 1.8.10 */

#define MAXNAMLEN 63

struct dirent {
    char d_name[MAXNAMLEN + 1];
    uint32_t d_size;                    /* propio de ZX-V: el tamaño */
    unsigned char d_type;               /* DT_DIR o DT_REG */
};

#define DT_UNKNOWN 0
#define DT_DIR 4
#define DT_REG 8

typedef struct {
    int handle;                         /* el de la API, o -1 tras rewinddir */
    struct dirent entry;
    char path[256];
} DIR;

#endif
