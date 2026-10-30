/*
 * art.c - Los gráficos de The Keep: los de keep.gfx, que se edita con zxv-gfx
 * en el PC o con gfxedit en ZX-V. El programa lo lleva dentro y, si en la
 * carpeta desde la que se carga hay otro keep.gfx, usa ese.
 *
 * Los sprites del fichero, por su número (los S_... de keep.h):
 *
 *    0-8    el héroe: abajo (andar, andar, atacar), arriba y a la derecha (a
 *           la izquierda, en espejo)
 *    9-10   la espada, horizontal (hacia la derecha) y vertical (hacia abajo)
 *   11-18   los enemigos, dos dibujos de cada uno: el limo, el murciélago,
 *           el esqueleto y el arquero
 *   19-20   la flecha, horizontal y vertical
 *   21-22   el sabio
 *   23-25   el hechicero: quieto (dos) y lanzando un hechizo
 *   26-29   la bola de fuego (dos) y la nube de cuando muere algo (dos)
 *   30-39   los objetos: llave, poción, corazón, oro, moneda, corazoncito,
 *           botas, espada grande, farol y llave grande
 *   40-49   los iconos del marcador (8 x 8), en el orden de I_HEART...
 *   50      la torre del título
 *   51      la fuente: glifos de 6 x 9 seguidos (el orden, en video.c)
 *
 * Los tiles de 16 x 16 son los de las plantas (keep.h, T_...) y los mapas
 * 0 a 3, las cuatro plantas (world.c).
 */
#include "zxv_gfxfile.h"

#include "keep.h"

sprite_t spr[SPRITES];

/* El fichero, dentro del programa, con 32 KB más detrás para que quepa uno
 * de la tarjeta más grande (con más tiles o sprites mayores). */
#define GFX_SLACK   32768
#define STR_(x)     #x
#define STR(x)      STR_(x)
__asm__(".section .data.keep_gfx, \"aw\"\n"
        ".balign 4\n"
        "keep_gfx:\n"
        ".incbin \"examples/keep/keep.gfx\"\n"
        "keep_gfx_end:\n"
        ".space " STR(GFX_SLACK) "\n"
        ".previous\n");
extern uint8_t keep_gfx[], keep_gfx_end[];

static zxv_gfxfile gfx;

/* Los sprites preparados para el motor, uno detrás de otro. */
static uint32_t pool[32 * 1024 / 4];
static uint32_t pool_used;
static uint32_t scratch[8 * 1024 / 4];  /* un sprite del fichero, alineado, para prepararlo */

#define GLYPHS      64
static const void *glyphs[GLYPHS];

/* Los tiles de 16 x 16, hasta 128. */
static uint32_t tiles[128 * 128 / 4];
static int ntiles;

static void *pool_take(uint32_t bytes)
{
    bytes = (bytes + 3) & ~3u;
    if (pool_used + bytes > sizeof(pool))
        return 0;
    void *p = (uint8_t *)pool + pool_used;
    pool_used += bytes;
    return p;
}

/* El rectángulo (x, y, w, h) del bitmap, preparado; 0 si no cabe. */
static const void *prepare(const zxv_bitmap *b, int x, int y, int w, int h)
{
    void *p = pool_take(zxv_spr_prepare(b, x, y, w, h, 0));
    if (p)
        zxv_spr_prepare(b, x, y, w, h, p);
    return p;
}

/* El sprite num del fichero, preparado; si no está o no cabe, no se dibuja. */
static sprite_t load_sprite(int num)
{
    sprite_t s = { 0, 0, 0 };
    zxv_bitmap b;
    if (zxv_gfxf_sprite(&gfx, num, &b, scratch, sizeof(scratch)) - 1 >= sizeof(scratch))
        return s;
    const void *p = prepare(&b, 0, 0, b.w, b.h);
    if (p)
        s = (sprite_t){ p, b.w, b.h };
    return s;
}

const void *art_glyph(int g) { return g >= 0 && g < GLYPHS ? glyphs[g] : 0; }

int art_floor(int f, uint16_t *out)
{
    zxv_tilemap m;
    uint32_t need = zxv_gfxf_map(&gfx, f, &m, tiles, out, MAP_W * MAP_H * 2);
    return need && need <= MAP_W * MAP_H * 2 && m.w == MAP_W && m.h == MAP_H && m.size == TILE;
}

static void bad_gfx(const char *why_es, const char *why_en)
{
    static char msg[64];
    append(append(msg, "keep.gfx: "), T(why_es, why_en));
    zxv_report_error(0, msg);
}

void art_init(void)
{
    /* El keep.gfx de la tarjeta, si hay uno, en el sitio del de dentro. */
    uint32_t room = (uint32_t)(keep_gfx_end - keep_gfx) + GFX_SLACK;
    int n = zxv_file_load("keep.gfx", keep_gfx, room);
    if (n == ZXV_E_RANGE)
        bad_gfx("demasiado grande", "too big");
    if (n < 0)
        n = (int)(keep_gfx_end - keep_gfx);
    if (!zxv_gfxf_open(&gfx, keep_gfx, (uint32_t)n))
        zxv_report_error(0, T("keep.gfx no vale", "keep.gfx is not valid"));

    uint32_t need = zxv_gfxf_tiles(&gfx, TILE, tiles, sizeof(tiles), &ntiles);
    if (!need || need > sizeof(tiles) || ntiles < T_MARK + MARKS)
        bad_gfx("faltan tiles de 16", "16-pixel tiles missing");
    floor_map = (zxv_tilemap){ &cells[0][0], MAP_W, MAP_H, (const uint8_t *)tiles, TILE, 0 };
    static uint16_t check[MAP_W * MAP_H];
    for (int f = 0; f < FLOORS; f++)
        if (!art_floor(f, check))
            bad_gfx("las plantas son los mapas 0-3, de 48 x 33", "the floors are maps 0-3, 48 x 33");

    pool_used = 0;
    for (int i = 0; i < SPRITES; i++)
        if (i != S_FONT)
            spr[i] = load_sprite(i);
    zxv_bitmap b;
    if (zxv_gfxf_sprite(&gfx, S_FONT, &b, scratch, sizeof(scratch)) - 1 < sizeof(scratch))
        for (int g = 0; g < GLYPHS && (g + 1) * 6 <= b.w; g++)
            glyphs[g] = prepare(&b, g * 6, 0, 6, b.h < 9 ? b.h : 9);
}
