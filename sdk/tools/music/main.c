/*
 * zxv-music - Convierte música al formato propio de ZX-V (.zxm, D98):
 *
 *   zxv-music [--free KB] [--channels LISTA] ENTRADA SALIDA.zxm
 *
 * De PT3 y TurboSound (Pro Tracker 3 y Vortex Tracker II), MOD, S3M y XM. Dice lo
 * que no ha podido pasar y lo que ocupan las muestras en la memoria de sonido
 * (96 KB). Con --free, deja esos KB libres para los sonidos de SAMPLE: las
 * muestras que no caben van en ADPCM, a la mitad o, si ni así, fuera. De un
 * módulo de más de 12 canales van los 12 que más notas tocan; con --channels
 * (1,2,5-9: los números del tracker, desde 1), los que se digan. Licencia
 * MIT, como el resto del SDK.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zxm_format.h"
#include "zxm_song.h"

#define SOUND_MEM 0x18000u                  /* la memoria de sonido: 96 KB */

static void say(void *ctx, const char *msg)
{
    fprintf(stderr, "%s: %s\n", (const char *)ctx, msg);
}

static int usage(void)
{
    fprintf(stderr, "uso: zxv-music [--free KB] [--channels LISTA] ENTRADA SALIDA.zxm\n"
                    "  --free KB          deja KB de la memoria de sonido (96) libres para SAMPLE\n"
                    "  --channels LISTA   los canales que van, hasta 12 (1,2,5-9; desde 1), de un\n"
                    "                     MOD, S3M o XM; si no, de más de 12, los que más notas tocan\n");
    return 2;
}

/* "1,2,5-9" (desde 1, hasta 64): el bit de cada canal (desde 0), o 0 si no
 * vale o son más de 12. */
static uint64_t channel_list(const char *s)
{
    uint64_t set = 0;
    int n = 0;
    while (*s) {
        char *end;
        long a = strtol(s, &end, 10), b = a;
        if (end == s)
            return 0;
        if (*end == '-') {
            s = end + 1;
            b = strtol(s, &end, 10);
            if (end == s)
                return 0;
        }
        if (a < 1 || b < a || b > 64)
            return 0;
        for (long c = a; c <= b; c++)
            if (!(set >> (c - 1) & 1)) {
                set |= (uint64_t)1 << (c - 1);
                n++;
            }
        if (*end == ',')
            end++;
        else if (*end)
            return 0;
        s = end;
    }
    return n <= 12 ? set : 0;
}

int main(int argc, char **argv)
{
    uint32_t mem = SOUND_MEM;
    uint64_t channels = 0;
    int a = 1;
    while (argc - a > 2) {
        if (!strcmp(argv[a], "--free")) {
            char *end;
            long kb = strtol(argv[a + 1], &end, 10);
            if (kb < 0 || kb > 96 || *end || end == argv[a + 1])
                return usage();
            mem = SOUND_MEM - (uint32_t)kb * 1024u;
        } else if (!strcmp(argv[a], "--channels")) {
            if (!(channels = channel_list(argv[a + 1])))
                return usage();
        } else
            return usage();
        a += 2;
    }
    if (argc - a != 2)
        return usage();
    const char *src = argv[a], *dst = argv[a + 1];
    FILE *f = fopen(src, "rb");
    if (!f) {
        perror(src);
        return 1;
    }
    static uint8_t in[4 << 20];
    size_t n = fread(in, 1, sizeof in, f);
    fclose(f);
    size_t size = 0;
    uint8_t *out = NULL;
    if (zxm_conv_kind(in, (uint32_t)n, (uint32_t)n)) {
        /* Un MOD, un S3M o un XM. */
        out = zxm_from_module_ch(in, (uint32_t)n, mem, channels, &size, say, (void *)src);
        if (!out) {
            fprintf(stderr, "%s: no se puede convertir\n", src);
            return 1;
        }
    } else {
        if (channels) {
            fprintf(stderr, "%s: --channels solo vale para un MOD, un S3M o un XM\n", src);
            return 2;
        }
        zxms_t *s = malloc(sizeof *s);
        if (!s || zxms_from_pt3(s, in, n, say, (void *)src)) {
            fprintf(stderr, "%s: no es un PT3 ni un MOD que se pueda convertir\n", src);
            return 1;
        }
        out = zxms_save(s, &size);
        zxms_free(s);
        free(s);
    }
    if (!out || !(f = fopen(dst, "wb")) || fwrite(out, 1, size, f) != size || fclose(f)) {
        perror(dst);
        return 1;
    }
    const uint8_t *d = out + ZXM_H_OFFSETS + 4 * ZXM_O_DATA_SIZE;
    uint32_t smp = (uint32_t)d[0] | (uint32_t)d[1] << 8 | (uint32_t)d[2] << 16 | (uint32_t)d[3] << 24;
    printf("%s: %zu bytes; las muestras, %u de la memoria de sonido (quedan %u KB libres para SAMPLE)\n", dst,
           size, (unsigned)smp, (unsigned)((SOUND_MEM - (smp < SOUND_MEM ? smp : SOUND_MEM)) / 1024u));
    free(out);
    return 0;
}
