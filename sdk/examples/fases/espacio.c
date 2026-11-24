/*
 * espacio.c - La fase 2, banco de código "espacio": estrellas que pasan a
 * tres velocidades y una nave que sube y baja.
 */
#include "fases.h"
#include "zxv_gfx.h"

#define STARS   48

static struct {
    int32_t x;                          /* en 1/16 de píxel */
    int y, speed;
} star[STARS];
static int ready;
static uint32_t seed = 12345;

static int rnd(int n)
{
    seed = seed * 1103515245u + 12345u;
    return (int)((seed >> 16) % (uint32_t)n);
}

void espacio_draw(int frame)
{
    if (!ready) {
        for (int i = 0; i < STARS; i++) {
            star[i].x = rnd(W) << 4;
            star[i].y = GROUND_Y + 2 + rnd(H - GROUND_Y - 4);
            star[i].speed = 1 + i % 3;
        }
        ready = 1;
    }
    zxv_gfx_rect(0, GROUND_Y, W, H - GROUND_Y, 0);
    for (int i = 0; i < STARS; i++) {
        star[i].x -= star[i].speed * 24;
        if (star[i].x < 0)
            star[i].x += W << 4;
        zxv_gfx_rect(star[i].x >> 4, star[i].y, star[i].speed, 1, star[i].speed == 3 ? 15 : 7);
    }
    /* La nave: sube y baja, 18 píxeles a cada lado, en 72 frames. */
    int ph = frame % 72;
    int y = GROUND_Y + 34 + (ph < 36 ? ph - 18 : 54 - ph);
    int16_t top[6] = { 56, (int16_t)(y - 15), 56, (int16_t)(y - 4), 72, (int16_t)(y - 4) };
    int16_t bottom[6] = { 56, (int16_t)(y + 15), 56, (int16_t)(y + 4), 72, (int16_t)(y + 4) };
    zxv_gfx_tri(top, 5);                                    /* las aletas */
    zxv_gfx_tri(bottom, 5);
    int16_t hull[6] = { 56, (int16_t)(y - 8), 56, (int16_t)(y + 8), 94, (int16_t)y };
    zxv_gfx_tri(hull, 13);
    zxv_gfx_rect(70, y - 2, 8, 3, 15);                      /* la cabina */
    int flame = frame & 4 ? 8 : 4;                          /* el motor */
    zxv_gfx_rect(56 - flame, y - 3, flame, 6, frame & 2 ? 14 : 10);
}
