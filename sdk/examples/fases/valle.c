/*
 * valle.c - La fase 1, banco de código "valle": tres pelotas que botan en la
 * hierba. Sus variables vuelven a empezar cada vez que se carga el banco.
 */
#include "fases.h"
#include "zxv_gfx.h"

#define BALLS   3

static struct {
    int32_t x, y, vx, vy;               /* en 1/256 de píxel */
    int r, c;
} ball[BALLS];
static int ready;

void valle_draw(int frame)
{
    (void)frame;
    if (!ready) {
        static const int col[BALLS] = { 10, 14, 11 };
        for (int i = 0; i < BALLS; i++) {
            ball[i].x = (40 + i * 80) << 8;
            ball[i].y = (GROUND_Y + 10 + i * 6) << 8;
            ball[i].vx = (i & 1 ? -1 : 1) * (180 + i * 60);
            ball[i].vy = 0;
            ball[i].r = 9 - i;
            ball[i].c = col[i];
        }
        ready = 1;
    }
    zxv_gfx_rect(0, GROUND_Y, W, H - GROUND_Y, 4);         /* la hierba */
    for (int y = GROUND_Y + 6; y < H; y += 12)
        zxv_gfx_rect(0, y, W, 2, 12);
    for (int i = 0; i < BALLS; i++) {
        ball[i].vy += 40;                                   /* la gravedad */
        ball[i].x += ball[i].vx;
        ball[i].y += ball[i].vy;
        int floor = (H - 6 - ball[i].r) << 8;
        if (ball[i].y > floor) {
            ball[i].y = floor;
            ball[i].vy = -ball[i].vy * 15 / 16;
            if (ball[i].vy > -600)
                ball[i].vy = -1600 - i * 120;               /* que no se paren */
        }
        if (ball[i].x < ball[i].r << 8 || ball[i].x > (W - ball[i].r) << 8)
            ball[i].vx = -ball[i].vx;
        int x = ball[i].x >> 8, y = ball[i].y >> 8;
        zxv_gfx_rect(x - ball[i].r, H - 7, 2 * ball[i].r, 2, 0);   /* la sombra */
        zxv_gfx_circle(x, y, ball[i].r, ball[i].c, 1);
        zxv_gfx_circle(x - ball[i].r / 3, y - ball[i].r / 3, ball[i].r / 3, 15, 1);
    }
}
