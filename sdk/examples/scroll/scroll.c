/*
 * scroll.c - Desplazamiento sin fin con el campo virtual (spec §4.9).
 *
 * La página mide 1024x192 (VSIZE = 2) y la pantalla es una ventana de 256
 * píxeles que se mueve con SCROLL_X: el escenario no se redibuja, solo se
 * dibuja la columna de tiles que va a entrar por la derecha, justo fuera
 * de la ventana. Las montañas de arriba van a la mitad de velocidad
 * (parallax) con la tabla por línea, sin coste de CPU.
 *
 * Pulsa una tecla para volver.
 */
#include "zxv_gfx.h"

#define PAGE_W      1024
#define LEVEL_W     512                 /* columnas del nivel (más largo que la página) */
#define ROWS        24
#define HILLS       48                  /* líneas de montañas arriba */

static uint32_t tile_mem[6 * 8];
static uint16_t cells[LEVEL_W * ROWS];

static void make_level(void)
{
    zxv_bitmap b = { (uint8_t *)tile_mem, 4, 8, 48, 0 };
    zxv_gfx_target(&b);
    zxv_gfx_rect(0, 0, 8, 8, 5);                    /* 0: cielo */
    zxv_gfx_rect(0, 8, 8, 8, 4);                    /* 1: tierra */
    zxv_gfx_rect(0, 16, 8, 8, 12);                  /* 2: hierba */
    zxv_gfx_rect(0, 22, 8, 2, 4);
    zxv_gfx_rect(0, 24, 8, 8, 2);                   /* 3: columna */
    zxv_gfx_rect(2, 24, 4, 8, 10);
    zxv_gfx_rect(0, 32, 8, 8, 5);                   /* 4: cielo con nube */
    zxv_gfx_circle(4, 36, 3, 15, 1);
    zxv_gfx_rect(0, 40, 8, 8, 6);                   /* 5: moneda */
    zxv_gfx_circle(4, 44, 3, 14, 1);
    zxv_gfx_target(0);

    uint32_t s = 5;
    for (int x = 0; x < LEVEL_W; x++) {
        s = s * 1103515245u + 12345u;
        int ground = 17 + (int)((s >> 16) % 3) - ((x / 40) & 1) * 2;   /* el suelo sube y baja */
        for (int y = 0; y < ROWS; y++) {
            uint16_t t = y > ground ? 1 : y == ground ? 2 : 0;
            if (y < 6 && (int)((s >> 20) % 13) == y)
                t = 4;
            if (x % 23 == 0 && y > ground - 3 && y < ground)
                t = 3;
            if (x % 7 == 3 && y == ground - 4)
                t = 5;
            cells[y * LEVEL_W + x] = t;
        }
    }
}

static const zxv_tilemap level = { cells, LEVEL_W, ROWS, (const uint8_t *)tile_mem, 8, ZXV_MAP_WRAP };

/* Montañas (las 48 líneas de arriba de la página, 1024 de ancho). */
static void draw_hills(void)
{
    zxv_gfx_rect(0, 0, PAGE_W, HILLS, 5);
    for (int i = 0; i < 16; i++) {
        int cx = i * 64 + 32;
        int16_t p[6] = { (int16_t)(cx - 48), HILLS, (int16_t)cx, (int16_t)(8 + (i * 7) % 20), (int16_t)(cx + 48), HILLS };
        zxv_gfx_tri(p, i & 1 ? 4 : 12);
    }
}

/* La línea y de la pantalla (las del borde superior van antes: 48 en modo 0). */
static volatile uint8_t *line_entry(int y)
{
    return (volatile uint8_t *)(uintptr_t)(ZXV_LINE_TABLE + (uint32_t)(48 + y) * ZXV_LINE_ENTRY_SIZE);
}

int main(void)
{
    zxv_set_mode(0);
    *(volatile uint8_t *)(uintptr_t)ZXV_V_VSIZE = ZXV_VSIZE_WIDE;
    zxv_gfx_draw_page(0);
    zxv_gfx_target(0);
    make_level();

    /* La página entera al empezar: las montañas arriba y el nivel debajo
     * (las primeras 128 columnas). */
    draw_hills();
    zxv_map_draw(&level, 0, HILLS, 0, HILLS, PAGE_W, 192 - HILLS, ZXV_MAP_ALL);

    /* Parallax: las líneas de las montañas toman su propio scroll. */
    for (int y = 0; y < 192; y++) {
        volatile uint8_t *e = line_entry(y);
        e[ZXV_LT_BORDER] = 0xFF;
        e[ZXV_LT_REMAP] = 0xFF;
        e[ZXV_LT_SCROLL_X] = 0xFF;
        e[ZXV_LT_SCROLL_X + 1] = 0xFF;
        e[ZXV_LT_SRC_LINE] = 0xFF;
        e[ZXV_LT_SRC_LINE + 1] = 0xFF;
    }
    *(volatile uint8_t *)(uintptr_t)ZXV_V_FLAGS = ZXV_VFLAG_LINETAB;

    int frames = 0;
    uint32_t pos = 0;                   /* píxel del nivel en el borde izquierdo */
    while (zxv_key_get() < 0) {
        zxv_wait_vblank();
        /* La página visible se está enviando: se espera a que acabe. */
        while (*(volatile uint32_t *)(uintptr_t)ZXV_SV_SYSFLAGS & ZXV_SYSF_LINK_BUSY)
            ;
        pos += 2;
        /* Al entrar en una columna nueva, se dibuja la que aparecerá por la
         * derecha (fuera de la ventana de 256). */
        if ((pos & 7) < 2) {
            uint32_t col = pos / 8 + 32;
            zxv_map_draw(&level, (int)col * 8, HILLS, (int)(col * 8) % PAGE_W, HILLS, 8, 192 - HILLS, ZXV_MAP_ALL);
        }
        *(volatile uint16_t *)(uintptr_t)ZXV_V_SCROLL_X = (uint16_t)(pos % PAGE_W);
        for (int y = 0; y < HILLS; y++) {
            volatile uint8_t *e = line_entry(y);
            uint16_t sx = (uint16_t)((pos / 2) % PAGE_W);
            e[ZXV_LT_SCROLL_X] = (uint8_t)sx;
            e[ZXV_LT_SCROLL_X + 1] = (uint8_t)(sx >> 8);
        }
        frames++;
    }
    *(volatile uint8_t *)(uintptr_t)ZXV_V_FLAGS = 0;
    *(volatile uint8_t *)(uintptr_t)ZXV_V_VSIZE = 0;
    *(volatile uint16_t *)(uintptr_t)ZXV_V_SCROLL_X = 0;
    return 0;                                   /* 0: acabó bien (otro número es un error) */
}
