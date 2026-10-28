/*
 * digital.c - Sonido digitalizado con el AY: muestras de 8 kHz por el
 * volumen del canal A, cada una a su hora con zxv_ay_event (spec §5.4).
 *
 * Con el tono y el ruido apagados, el canal da su volumen tal cual: es un DAC
 * de 4 bits (logarítmico; la tabla de abajo ya lo tiene en cuenta). Suena
 * medio segundo un La de 440 Hz y medio segundo uno de 880 Hz, y vuelve al
 * BASIC.
 */
#include "zxv_api.h"

#define RATE    8000                    /* muestras por segundo */
#define STEP    (1000000 / RATE)        /* 125 µs entre una y otra */

/* Un periodo de seno en 32 pasos, en volúmenes del AY: los más cercanos a
 * 0,5 + 0,5 sen(x) en su DAC. */
static const uint8_t sine[32] = {
    12, 13, 13, 14, 14, 15, 15, 15, 15, 15, 15, 15, 14, 14, 13, 13,
    12, 11, 10, 9, 8, 6, 4, 1, 0, 1, 4, 6, 8, 9, 10, 11,
};

/* n muestras de un tono de f Hz desde la hora t; devuelve la hora siguiente. */
static uint32_t tone(uint32_t t, unsigned f, unsigned n)
{
    uint32_t phase = 0, inc = (uint32_t)(((uint64_t)f << 32) / RATE);
    for (unsigned i = 0; i < n; i++, t += STEP, phase += inc)
        zxv_ay_event(t, 8, sine[phase >> 27]);      /* espera si la cola está llena */
    return t;
}

int main(void)
{
    zxv_cls(7);
    zxv_print_str("Sonido digitalizado: 8 kHz por el volumen del AY\n");
    zxv_ay_write(7, 0x3F);              /* sin tonos ni ruido: el volumen es la salida */
    zxv_ay_write(9, 0);
    zxv_ay_write(10, 0);

    uint32_t t = zxv_time_us() + 5000;  /* un poco por delante, para no ir tarde */
    t = tone(t, 440, RATE / 2);
    t = tone(t, 880, RATE / 2);
    zxv_ay_event(t, 8, 0);

    /* Hasta que suene la última (con el retraso de la cola). */
    while ((int32_t)(zxv_time_us() - (t + ZXV_BEEP_LATENCY_US)) < 0)
        zxv_wait_vblank();
    return 0;
}
