/*
 * graphics.c - Demostración del motor gráfico de la ROM (spec §7.8).
 *
 * Arriba, un escenario de tiles que se desplaza sin fin con pelotas que
 * rebotan contra él y entre ellas (se colorean al chocar) y una estrella
 * que gira; abajo, un suelo en modo 7 que gira, con conos que se acercan y
 * se alejan (sprites escalados con la perspectiva). Todo se dibuja en la página
 * oculta y se cambia de página en el VBLANK (doble buffer), así que no hay
 * parpadeo ni hace falta esperar a LINK_BUSY.
 *
 * Los gráficos se generan al empezar dibujando con el propio motor en
 * bitmaps en memoria. Pulsa una tecla para volver.
 */
#include "zxv_gfx.h"

#define W           256
#define MAP_H       136             /* líneas del escenario; debajo, el suelo */
#define TILES       8
#define MAP_W       64
#define MAP_ROWS    17
#define BALLS       12
#define FIRST_SOLID 2               /* tiles 2-4: hierba, ladrillo y bloque */
#define LAST_SOLID  4

/* --- Gráficos generados ------------------------------------------------------ */

static uint32_t tile_mem[TILES * 8];                /* 8 tiles de 8x8 */
static uint16_t cells[MAP_W * MAP_ROWS];
static uint32_t ball_mem[16 * 16 / 8];
static uint32_t ball_spr[80];                       /* sprite preparado */
static uint8_t floor_tex[64 * 64];
static zxv_m7line floor_lines[192];
static uint32_t cone_mem[16 * 24 / 8];
static uint32_t cone_spr[(8 + 24 * 2 * 8) / 4];      /* sprite preparado: 8 + h·⌈w/8⌉·8 bytes */

static uint32_t seed = 1;

static uint32_t rnd(void)
{
    seed = seed * 1103515245u + 12345u;
    return seed >> 16;
}

static void make_tiles(void)
{
    zxv_bitmap b = { (uint8_t *)tile_mem, 4, 8, 8 * TILES, 0 };
    zxv_gfx_target(&b);
    zxv_gfx_rect(0, 0, 8, 8 * TILES, 1);                    /* 0: cielo */
    zxv_gfx_rect(0, 8, 8, 8, 1);                             /* 1: cielo con estrella */
    zxv_gfx_rect(3, 11, 2, 2, 15);
    zxv_gfx_rect(0, 16, 8, 8, 4);                            /* 2: hierba */
    zxv_gfx_line(0, 16, 7, 16, 12);
    zxv_gfx_line(1, 19, 2, 18, 12);
    zxv_gfx_line(5, 21, 6, 20, 12);
    zxv_gfx_rect(0, 24, 8, 8, 2);                            /* 3: ladrillo */
    zxv_gfx_line(0, 27, 7, 27, 7);
    zxv_gfx_line(3, 24, 3, 27, 7);
    zxv_gfx_line(7, 28, 7, 31, 7);
    zxv_gfx_rect(0, 32, 8, 8, 6);                            /* 4: bloque */
    zxv_gfx_rect(1, 33, 6, 6, 14);
    zxv_gfx_rect(3, 35, 2, 2, 6);
    zxv_gfx_rect(0, 40, 8, 8, 1);                            /* 5: nube */
    zxv_gfx_circle(4, 44, 3, 15, 1);
    zxv_gfx_target(0);

    /* Un nivel: cielo con estrellas y nubes, suelo, plataformas y bloques. */
    for (int y = 0; y < MAP_ROWS; y++)
        for (int x = 0; x < MAP_W; x++) {
            uint16_t t = (rnd() % 23) == 0 ? 1 : 0;
            if (y < 6 && rnd() % 19 == 0)
                t = 5;
            if (y == MAP_ROWS - 2)
                t = 2;
            if (y == MAP_ROWS - 1)
                t = 3;
            if ((y == 9 || y == 12) && (x / 5) % 3 == y % 2)
                t = 3;
            if (y == 6 && x % 16 == 7)
                t = 4;
            cells[y * MAP_W + x] = t;
        }
}

static void make_ball(void)
{
    zxv_bitmap b = { (uint8_t *)ball_mem, 8, 16, 16, 0 };
    zxv_gfx_target(&b);
    zxv_gfx_rect(0, 0, 16, 16, ZXV_TRANSPARENT);
    zxv_gfx_circle(7, 7, 7, 2, 1);
    zxv_gfx_circle(5, 5, 2, 10, 1);
    zxv_gfx_target(0);
    zxv_spr_prepare(&b, 0, 0, 15, 15, ball_spr);
}

static void make_floor(void)
{
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++) {
            int c = ((x >> 3) ^ (y >> 3)) & 1 ? 12 : 4;
            if (x == 0 || y == 0)
                c = 14;
            floor_tex[y * 64 + x] = (uint8_t)c;
        }
}

static void make_cone(void)
{
    zxv_bitmap b = { (uint8_t *)cone_mem, 8, 16, 24, 0 };
    zxv_gfx_target(&b);
    zxv_gfx_rect(0, 0, 16, 24, ZXV_TRANSPARENT);
    static const int16_t cone[6] = { 7, 0, 1, 21, 14, 21 };
    zxv_gfx_tri(cone, 10);
    zxv_gfx_rect(4, 8, 7, 3, 15);                       /* las franjas */
    zxv_gfx_rect(2, 15, 11, 3, 15);
    zxv_gfx_rect(0, 21, 15, 3, 2);                      /* la base */
    zxv_gfx_target(0);
    zxv_spr_prepare(&b, 0, 0, 15, 24, cone_spr);
}

/* --- Pelotas -------------------------------------------------------------------- */

typedef struct {
    int x, y, vx, vy;               /* 1/16 de píxel */
    int hit;                        /* frames coloreada tras un choque */
} ball_t;

static ball_t balls[BALLS];
static int scroll;                  /* píxeles de mapa desplazados */

static const zxv_tilemap level = { cells, MAP_W, MAP_ROWS, (const uint8_t *)tile_mem, 8, ZXV_MAP_WRAP };

/* ¿Hay un tile sólido en el punto (x, y) de la pantalla? */
static int solid(int x, int y)
{
    unsigned t = zxv_map_cell(&level, x + scroll, y) & ZXV_CELL_TILE;
    return t >= FIRST_SOLID && t <= LAST_SOLID;
}

static void move_balls(void)
{
    for (int i = 0; i < BALLS; i++) {
        ball_t *b = &balls[i];
        b->vy += 3;                                     /* gravedad */
        int nx = b->x + b->vx, ny = b->y + b->vy;
        int px = nx >> 4, py = ny >> 4;
        if (b->vy > 0 && (solid(px + 7, py + 15) || py > MAP_H - 16)) {
            b->vy = -b->vy * 7 / 8;
            if (b->vy > -24)
                b->vy = -40 - (int)(rnd() % 24);        /* un empujón para no pararse */
            ny = b->y;
        } else if (b->vy < 0 && (solid(px + 7, py) || py < 0)) {
            b->vy = -b->vy;
            ny = b->y;
        }
        if ((b->vx > 0 && (solid(px + 15, py + 7) || px > W - 16)) ||
            (b->vx < 0 && (solid(px, py + 7) || px < 0))) {
            b->vx = -b->vx;
            nx = b->x;
        }
        b->x = nx;
        b->y = ny;
        if (b->hit)
            b->hit--;
    }
    for (int i = 0; i < BALLS; i++)
        for (int j = i + 1; j < BALLS; j++)
            if (zxv_spr_collide(ball_spr, balls[i].x >> 4, balls[i].y >> 4, ball_spr, balls[j].x >> 4, balls[j].y >> 4)) {
                balls[i].hit = balls[j].hit = 10;
                int t = balls[i].vx;
                balls[i].vx = balls[j].vx;
                balls[j].vx = t;
            }
}

/* --- Un frame --------------------------------------------------------------------- */

static void draw_star(int frame)
{
    int16_t p[20];
    int32_t a = frame * 300;
    for (int i = 0; i < 10; i++) {
        int r = i & 1 ? 9 : 22;
        int32_t ang = a + i * 0x10000 / 10;
        /* Seno y coseno aproximados con una parábola: basta para una estrella. */
        int32_t t = ang & 0xFFFF, s;
        int32_t h = t & 0x7FFF;                             /* media vuelta */
        s = (int32_t)(((int64_t)4 * h * (0x8000 - h)) >> 14);  /* 0..0x10000 */
        if (t & 0x8000)
            s = -s;
        int32_t tc = (ang + 0x4000) & 0xFFFF, c;
        h = tc & 0x7FFF;
        c = (int32_t)(((int64_t)4 * h * (0x8000 - h)) >> 14);
        if (tc & 0x8000)
            c = -c;
        p[2 * i] = (int16_t)(210 + ((r * c) >> 16));
        p[2 * i + 1] = (int16_t)(32 + ((r * s) >> 16));
    }
    zxv_gfx_poly(p, 10, 14);
    for (int i = 0; i < 10; i++) {
        int j = (i + 1) % 10;
        zxv_gfx_line(p[2 * i], p[2 * i + 1], p[2 * j], p[2 * j + 1], 10);
    }
}

/* Los conos del suelo: uno en el centro de cada cuadro de 64 x 64 texels de
 * la textura (que se repite). Se proyectan con la misma cámara que el suelo:
 * la última línea calculada da la derecha de la cámara (du, dv) multiplicada
 * por lo que mide un píxel allí, height / dy; de ella sale la profundidad z
 * de cada cono, y con z su línea, su x y su tamaño (focal / z). */
#define CONES_MAX   81
#define CONE_SIZE   10                                  /* de alto, en texels */

static void draw_cones(const zxv_m7cam *cam)
{
    const zxv_m7line *l = &floor_lines[191];
    int dy0 = 191 - cam->horizon, focal = cam->focal >> 16;
    int64_t step = cam->height / dy0;                   /* texels por píxel, 16.16 */
    int32_t rx = l->du, ry = l->dv;
    struct { int32_t z, lat; } c[CONES_MAX];
    int n = 0;
    int bx = (cam->x >> 22) - 4, by = (cam->y >> 22) - 4;   /* el cuadro de la cámara */
    for (int j = 0; j < 9; j++)
        for (int i = 0; i < 9; i++) {
            int32_t dx = ((bx + i) * 64 + 32) * 65536 - cam->x;
            int32_t dy = ((by + j) * 64 + 32) * 65536 - cam->y;
            int64_t fz = ((int64_t)dx * ry - (int64_t)dy * rx) >> 16;
            int64_t fl = ((int64_t)dx * rx + (int64_t)dy * ry) >> 16;
            int32_t z = (int32_t)((fz << 16) / step);
            if (z < (24 << 16) || z > (240 << 16))      /* detrás, o más allá del horizonte */
                continue;
            int k = n++;                                /* ordenados de lejos a cerca */
            for (; k > 0 && c[k - 1].z < z; k--)
                c[k] = c[k - 1];
            c[k].z = z;
            c[k].lat = (int32_t)((fl << 16) / step);
        }
    zxv_gfx_clip(0, MAP_H, W, 192);                     /* solo sobre el suelo */
    for (int k = 0; k < n; k++) {
        int64_t z = c[k].z;
        int y = cam->horizon + (int)((cam->height * (int64_t)focal) / z);
        int x = W / 2 + (int)(((int64_t)c[k].lat * focal) / z);
        int h = (int)(((int64_t)CONE_SIZE * 65536 * focal) / z), w = h * 15 / 24;
        if (h > 0 && w > 0)
            zxv_spr_draw_scaled(cone_spr, x - w / 2, y - h, w, h, 0, 0);
    }
    zxv_gfx_clip(0, 0, W, 192);
}

static void draw_frame(int frame)
{
    /* Escenario: desplazamiento sin fin (el mapa se repite). */
    zxv_map_draw(&level, scroll, 0, 0, 0, W, MAP_H, ZXV_MAP_ALL);
    draw_star(frame);

    static const uint8_t hit_colours[16] = { 0, 1, 14, 3, 4, 5, 6, 7, 8, 9, 15, 11, 12, 13, 14, 15 };
    for (int i = 0; i < BALLS; i++)
        zxv_spr_draw(ball_spr, balls[i].x >> 4, balls[i].y >> 4, i & 1 ? ZXV_FLIP_H : 0,
                     balls[i].hit ? hit_colours : 0);

    /* Suelo en modo 7 que gira, con el horizonte en el borde del escenario. */
    zxv_m7tex tex = { floor_tex, 6, 1, 0, 0 };
    zxv_m7cam cam = { frame << 14, frame << 13, 12 << 16, frame * 90, 140 << 16, MAP_H - 8, 0 };
    zxv_m7_camera(floor_lines, 192, &cam);
    zxv_m7_draw(&tex, floor_lines + MAP_H, MAP_H, 192 - MAP_H);
    draw_cones(&cam);
    zxv_gfx_line(0, MAP_H, W - 1, MAP_H, 0);
}

int main(void)
{
    zxv_set_mode(0);
    make_tiles();
    make_ball();
    make_floor();
    make_cone();
    for (int i = 0; i < BALLS; i++) {
        balls[i].x = (int)(16 + rnd() % (W - 40)) << 4;
        balls[i].y = (int)(rnd() % 40) << 4;
        balls[i].vx = (int)(rnd() % 40) - 20;
        balls[i].vy = 0;
    }

    int visible = 0, frame = 0;
    while (zxv_key_get() < 0) {
        int draw = 1 - visible;
        zxv_gfx_draw_page(draw);
        draw_frame(frame);
        zxv_set_page(draw);                 /* se verá a partir del VBLANK */
        zxv_wait_vblank();
        visible = draw;
        move_balls();
        scroll++;
        frame++;
    }
    zxv_set_page(0);
    zxv_gfx_draw_page(0);
    return 0;                                   /* 0: acabó bien (otro número es un error) */
}
