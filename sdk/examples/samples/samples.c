/*
 * samples.c - Los canales de muestras (spec §5.6): doce canales que tocan
 * sonidos grabados de la memoria de sonido, a cualquier altura, con su
 * volumen y su panorama. Cada tecla toca una cosa y la pantalla dice qué
 * debe sonar:
 *
 *   1  un La de 440 Hz (una onda de 32 muestras en bucle) que va de la
 *      izquierda a la derecha
 *   2  una cuerda pulsada (sin bucle), en 8 bits y luego en ADPCM
 *   3  un acorde con los 12 canales a la vez (zxv_smp_prepare y
 *      zxv_smp_start: empiezan en la misma muestra)
 *   4  el modo directo (el DAC): una cuadrada de 1 kHz con zxv_ay_event
 *   5  una nota grave, con y sin interpolación, por turnos
 *   6  subir 64 KB a la memoria de sonido, y cuánto tarda
 *   7  el límite: los 12 canales en ADPCM al paso más alto (2,0), 3 s
 *   8  lo mismo al paso 1,5
 *   Espacio para; Esc vuelve al BASIC.
 *
 * Los sonidos se hacen aquí: el seno, con una tabla, y la cuerda, con el
 * algoritmo de Karplus-Strong. La cuerda va también en ADPCM (IMA, en
 * bloques de 64 muestras: ocupa algo más de la mitad y suena como 16 bits),
 * con el codificador de abajo. Un programa que toca sonidos de verdad los
 * lleva en ficheros (un WAV, o el .zxm de zxv-tracker con zxv_music.h).
 *
 * MIT, como el resto del SDK.
 */
#include "zxv_api.h"

#define SINE_AT     0x0000u                 /* en la memoria de sonido */
#define PLUCK8_AT   0x0100u
#define PLUCK_N     12000u                  /* medio segundo a 24 kHz */
#define PLUCKA_AT   (PLUCK8_AT + PLUCK_N)
#define BIG_AT      0x8000u
#define BIG_N       0x10000u

enum { SLOT_SINE, SLOT_PLUCK8, SLOT_PLUCKA, SLOT_PLUCKA_LOOP };

static const int8_t sine[32] = {
    0, 24, 48, 70, 89, 105, 117, 124, 127, 124, 117, 105, 89, 70, 48, 24,
    0, -24, -48, -70, -89, -105, -117, -124, -127, -124, -117, -105, -89, -70, -48, -24,
};
static int16_t pluck[PLUCK_N];
static int8_t pluck8[PLUCK_N];
static uint8_t pluck_adpcm[(PLUCK_N + ZXV_SMP_ADPCM_BLOCK - 1) / ZXV_SMP_ADPCM_BLOCK * ZXV_SMP_ADPCM_BYTES];
static uint8_t big[BIG_N];

/* --- El ADPCM ---------------------------------------------------------------
 * IMA ADPCM en bloques independientes de 64 muestras (36 bytes): la primera
 * muestra (i16) y el índice del paso (u8) en la cabecera de 4 bytes, y las
 * otras 63, de 4 bits, desde el byte 4 (la de número impar en los 4 bits de
 * abajo). Cada nibble se descodifica al codificar, como lo hará la máquina,
 * para que el error no se acumule. */

static const int16_t ima_step[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
    4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767,
};
static const int8_t ima_index[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static uint32_t adpcm_encode(const int16_t *in, uint32_t n, uint8_t *out)
{
    int index = 0;
    uint32_t bytes = 0;
    for (uint32_t at = 0; at < n; at += ZXV_SMP_ADPCM_BLOCK, bytes += ZXV_SMP_ADPCM_BYTES) {
        uint8_t *o = out + bytes;
        uint32_t m = n - at < ZXV_SMP_ADPCM_BLOCK ? n - at : ZXV_SMP_ADPCM_BLOCK;
        int pred = in[at];
        for (int i = 0; i < ZXV_SMP_ADPCM_BYTES; i++)
            o[i] = 0;
        o[0] = (uint8_t)pred;
        o[1] = (uint8_t)(pred >> 8);
        o[2] = (uint8_t)index;
        for (uint32_t j = 1; j < m; j++) {
            int step = ima_step[index], diff = in[at + j] - pred;
            unsigned nib = 0;
            if (diff < 0) {
                nib = 8;
                diff = -diff;
            }
            if (diff >= step) {
                nib |= 4;
                diff -= step;
            }
            if (diff >= step >> 1) {
                nib |= 2;
                diff -= step >> 1;
            }
            if (diff >= step >> 2)
                nib |= 1;
            /* Lo que sacará el descodificador. */
            int d = step >> 3;
            if (nib & 4)
                d += step;
            if (nib & 2)
                d += step >> 1;
            if (nib & 1)
                d += step >> 2;
            pred += nib & 8 ? -d : d;
            pred = pred < -32768 ? -32768 : pred > 32767 ? 32767 : pred;
            index += ima_index[nib & 7];
            index = index < 0 ? 0 : index > 88 ? 88 : index;
            o[4 + (j - 1) / 2] |= (uint8_t)(nib << ((j - 1) & 1) * 4);
        }
    }
    return bytes;
}

/* --- Los textos, en español y en inglés --------------------------------------- */

enum {
    T_TITLE, T_K1, T_K2, T_K3, T_K4, T_K5, T_K6, T_K78, T_KEYS, T_UPLOADED, T_OK, T_ERROR,
    T_1A, T_1B, T_2A, T_2B, T_3A, T_3B, T_4A, T_4B, T_5A, T_5B, T_6, T_6MS, T_7, T_8, T_78, T_COUNT
};

static const char *const text_es[T_COUNT] = {
    "Canales de muestras",
    "1 La 440 Hz, izquierda->derecha",
    "2 Cuerda: 8 bits y ADPCM",
    "3 Acorde con los 12 canales",
    "4 DAC: cuadrada de 1 kHz",
    "5 Grave: interpola / no",
    "6 Subir 64 KB",
    "7/8 12 x ADPCM, paso 2 / 1,5",
    "Espacio para; Esc al BASIC",
    "Muestras subidas",
    ": bien",
    ": ERROR",
    "Suena: La de 440 Hz, de izq.",
    "a dcha. en 2 segundos",
    "Suena: la cuerda en 8 bits,",
    "y luego en ADPCM (igual)",
    "Suena: Do mayor en 4 octavas,",
    "12 canales repartidos",
    "Suena: cuadrada de 1 kHz por",
    "el DAC, medio segundo",
    "Suena: La de 55 Hz, 1 s con",
    "interpolacion y 1 s sin ella",
    "Subiendo 64 KB...",
    "64 KB en %u ms",
    "Suena: 12 ADPCM al paso 2, 3 s",
    "Suena: 12 ADPCM al paso 1,5",
    "sin cortes: es el limite",
};

static const char *const text_en[T_COUNT] = {
    "Sample channels",
    "1 A 440 Hz, left->right",
    "2 String: 8 bits and ADPCM",
    "3 Chord with the 12 channels",
    "4 DAC: 1 kHz square wave",
    "5 Low note: interpolated / not",
    "6 Upload 64 KB",
    "7/8 12 x ADPCM, step 2 / 1.5",
    "Space stops; Esc to BASIC",
    "Samples uploaded",
    ": fine",
    ": ERROR",
    "Playing: A at 440 Hz, from",
    "left to right in 2 seconds",
    "Playing: the string in 8 bits,",
    "then in ADPCM (the same)",
    "Playing: C major in 4 octaves,",
    "spread over 12 channels",
    "Playing: 1 kHz square wave",
    "through the DAC, half a second",
    "Playing: A at 55 Hz, 1 s",
    "interpolated and 1 s without",
    "Uploading 64 KB...",
    "64 KB in %u ms",
    "Playing: 12 ADPCM at step 2, 3 s",
    "Playing: 12 ADPCM at step 1.5",
    "no breaks: that is the limit",
};

static const char *const *text = text_en;

static void line(int row, const char *s)
{
    zxv_text_at(0, row);
    zxv_print_str("                                ");
    zxv_text_at(0, row);
    zxv_print_str(s);
}

/* Un texto con un número sin signo en su %u. */
static void line_num(int row, const char *fmt, uint32_t v)
{
    char s[40], b[12], *p = s;
    int i = 11;
    b[i] = 0;
    do
        b[--i] = (char)('0' + v % 10);
    while (v /= 10);
    for (; *fmt && p < s + 30; fmt++) {
        if (fmt[0] == '%' && fmt[1] == 'u') {
            for (const char *q = b + i; *q;)
                *p++ = *q++;
            fmt++;
        } else {
            *p++ = *fmt;
        }
    }
    *p = 0;
    line(row, s);
}

static void status(int row, int what, int r)
{
    char s[40], *p = s;
    for (const char *q = text[what]; *q;)
        *p++ = *q++;
    for (const char *q = text[r == ZXV_OK ? T_OK : T_ERROR]; *q;)
        *p++ = *q++;
    *p = 0;
    line(row, s);
}

static void stop_all(void)
{
    for (int ch = 0; ch < ZXV_SMP_CHANNELS; ch++) {
        zxv_smp_write(ch, ZXV_SMP_CTRL, ZXV_SMP_STOP);
        zxv_smp_write(ch, ZXV_SMP_OFFSET, 0);
    }
}

static void frames(int n)
{
    while (n--)
        zxv_wait_vblank();
}

/* La cuerda de Karplus-Strong: ruido en un retardo de un periodo (220 Hz a
 * 24 kHz) que se va suavizando y apagando. Y 64 KB cualesquiera para subir. */
static void make_samples(void)
{
    enum { P = 109 };                       /* 24.000 / 220 Hz */
    uint32_t r = 12345;
    for (int i = 0; i < P; i++) {
        r = r * 1103515245u + 12345u;
        pluck[i] = (int16_t)((r >> 16 & 0xFFFF) - 32768);
    }
    for (uint32_t i = P; i < PLUCK_N; i++)
        pluck[i] = (int16_t)((pluck[i - P] + pluck[i - P + 1]) * 499 / 1000);
    for (uint32_t i = 0; i < PLUCK_N; i++)
        pluck8[i] = (int8_t)(pluck[i] >> 8);
    adpcm_encode(pluck, PLUCK_N, pluck_adpcm);
    for (uint32_t i = 0; i < BIG_N; i++)
        big[i] = (uint8_t)i;
}

static void two(int a, int b)
{
    line(13, text[a]);
    line(14, text[b]);
}

int main(void)
{
    text = zxv_lang_get() == ZXV_LANG_ES ? text_es : text_en;
    zxv_set_mode(0);
    zxv_cls(0);
    zxv_text_colors(7, 0);
    line(0, text[T_TITLE]);
    for (int i = T_K1; i <= T_KEYS; i++)
        line(2 + i - T_K1, text[i]);
    make_samples();
    /* A la memoria de sonido, y a la tabla de muestras: dónde empieza cada
     * una, su formato, su longitud y desde dónde repite (= longitud: no
     * repite). */
    int r = zxv_smp_upload(SINE_AT, sine, sizeof sine);
    if (r == ZXV_OK)
        r = zxv_smp_upload(PLUCK8_AT, pluck8, sizeof pluck8);
    if (r == ZXV_OK)
        r = zxv_smp_upload(PLUCKA_AT, pluck_adpcm, sizeof pluck_adpcm);
    if (r == ZXV_OK)
        r = zxv_smp_define(SLOT_SINE, SINE_AT, ZXV_SMP_PCM8, 32, 0);
    if (r == ZXV_OK)
        r = zxv_smp_define(SLOT_PLUCK8, PLUCK8_AT, ZXV_SMP_PCM8, PLUCK_N, PLUCK_N);
    if (r == ZXV_OK)
        r = zxv_smp_define(SLOT_PLUCKA, PLUCKA_AT, ZXV_SMP_ADPCM, PLUCK_N, PLUCK_N);
    if (r == ZXV_OK)
        r = zxv_smp_define(SLOT_PLUCKA_LOOP, PLUCKA_AT, ZXV_SMP_ADPCM, PLUCK_N, 0);
    status(11, T_UPLOADED, r);

    for (;;) {
        int k = zxv_key_get();
        if (k < 0) {
            zxv_wait_vblank();
            continue;
        }
        if (k == 0x1B) {
            stop_all();
            return 0;
        }
        stop_all();
        if (k == '1') {
            /* 32 muestras por vuelta: a 440 x 32 muestras por segundo. */
            two(T_1A, T_1B);
            zxv_smp_play(0, SLOT_SINE, zxv_smp_step(440 * 32), 64, 0);
            for (int i = 0; i < 120; i++) {
                zxv_smp_write(0, ZXV_SMP_PAN, i * 255 / 119);
                zxv_wait_vblank();
            }
            zxv_smp_stop(0);
        } else if (k == '2') {
            two(T_2A, T_2B);
            zxv_smp_play(0, SLOT_PLUCK8, zxv_smp_step(24000), 64, 128);
            frames(45);
            zxv_smp_play(1, SLOT_PLUCKA, zxv_smp_step(24000), 64, 128);
        } else if (k == '3') {
            two(T_3A, T_3B);
            static const uint16_t hz[12] = { 131, 165, 196, 262, 330, 392, 523, 659, 784, 1047, 1319, 1568 };
            for (int ch = 0; ch < 12; ch++)
                zxv_smp_prepare(ch, SLOT_SINE, zxv_smp_step(hz[ch] * 32u), 40, ch * 23);
            zxv_smp_start(0xFFF);                   /* los 12 en la misma muestra */
        } else if (k == '4') {
            /* En modo directo, el canal saca lo que se escribe en OFFSET:
             * cada cambio, con su hora, por la cola del AY. */
            two(T_4A, T_4B);
            zxv_smp_write(0, ZXV_SMP_VOL, 64);
            zxv_smp_write(0, ZXV_SMP_PAN, 128);
            zxv_smp_write(0, ZXV_SMP_CTRL, ZXV_SMP_DIRECT);
            uint32_t t = zxv_time_us() + 5000;
            for (int i = 0; i < 1000; i++, t += 500)
                zxv_ay_event(t, (int)ZXV_SMP_REG(0, ZXV_SMP_OFFSET), i & 1 ? 90 : 256 - 90);
        } else if (k == '5') {
            two(T_5A, T_5B);
            zxv_smp_play(0, SLOT_SINE, zxv_smp_step(55 * 32), 64, 128);
            frames(60);
            zxv_smp_write(0, ZXV_SMP_CTRL, ZXV_SMP_NOINTERP);
            frames(60);
            zxv_smp_stop(0);
            zxv_smp_write(0, ZXV_SMP_CTRL, 0);
        } else if (k == '6') {
            line(13, text[T_6]);
            line(14, "");
            uint32_t t0 = zxv_time_us();
            r = zxv_smp_upload(BIG_AT, big, BIG_N);
            line_num(13, text[T_6MS], (zxv_time_us() - t0) / 1000);
            status(14, T_UPLOADED, r);
        } else if (k == '7' || k == '8') {
            /* La cuerda grabada a 24 kHz: a 48 kHz es el paso 1; el 2,0 y el
             * 1,5 son 96 y 72 kHz. */
            uint32_t step = zxv_smp_step(k == '7' ? 96000 : 72000);
            two(k == '7' ? T_7 : T_8, T_78);
            for (int ch = 0; ch < 12; ch++)
                zxv_smp_prepare(ch, SLOT_PLUCKA_LOOP, step, 40, ch * 23);
            zxv_smp_start(0xFFF);                   /* a la vez: suena igual cada vez */
            frames(180);
            stop_all();
        } else {
            line(13, "");
            line(14, "");
        }
    }
}
