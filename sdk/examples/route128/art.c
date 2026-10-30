/*
 * art.c - Los gráficos de Route 128: los de route128.gfx, que se edita con
 * zxv-gfx en el PC o con gfxedit en ZX-V. El programa lo lleva dentro y, si
 * en la carpeta desde la que se carga hay otro route128.gfx, usa ese.
 *
 * Los sprites del fichero, por su número:
 *
 *    0-9    lo que hay junto a la carretera, en el orden de OBJ_PALM a OBJ_BOARD
 *   10-12   la salida, los controles y la meta, en español
 *   13-15   los mismos, en inglés
 *   16-18   los rivales: el cupé, la furgoneta y el camión
 *   20-24   el coche, del más girado a la izquierda al más girado a la derecha
 *   30-38   el paisaje de cada etapa (costa, desierto, bosque): de cada una,
 *           las montañas, lo de cerca y las nubes; de 256 píxeles de ancho
 *           o más, y se repiten
 *
 * Los coches rivales se dibujan con el 2 (la carrocería) y el 3 (sus
 * brillos), que car_colours cambia por los de cada coche al dibujarlos.
 *
 * La memoria: el fichero va con el programa, y lo que se hace con él, en la
 * memoria de vídeo que no se ve (route128.h): los sprites preparados para el
 * motor y el paisaje de la etapa en la que se está.
 */
#include "zxv_gfxfile.h"

#include "route128.h"

object_art_t obj_art[OBJ_COUNT];
sprite_t car_art[CAR_TYPES], player_art[5];
zxv_bitmap bg_far, bg_near, bg_clouds;

const uint8_t car_colours[6][16] = {
#define CC(d, b) { 0, 1, d, b, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
    CC(1, 9), CC(4, 12), CC(6, 14), CC(3, 11), CC(7, 15), CC(5, 13),
#undef CC
};

enum { SPR_GATES = 10, SPR_CARS = 16, SPR_PLAYER = 20, SPR_BACKGROUND = 30 };

/* El fichero, dentro del programa, con 16 KB más detrás para que quepa uno
 * de la tarjeta algo más grande (con más dibujo o sprites mayores). */
#define GFX_SLACK   16384
#define STR_(x)     #x
#define STR(x)      STR_(x)
__asm__(".section .data.route128_gfx, \"aw\"\n"
        ".balign 4\n"
        "route128_gfx:\n"
        ".incbin \"examples/route128/route128.gfx\"\n"
        "route128_gfx_end:\n"
        ".space " STR(GFX_SLACK) "\n"
        ".previous\n");
extern uint8_t route128_gfx[], route128_gfx_end[];

static zxv_gfxfile gfx;

/* Los sprites preparados, uno detrás de otro: primero en la página 0 y
 * luego en la 1, hasta las músicas (route128.h). */
static uint8_t *pool_at, *pool_end;

static void *pool_take(uint32_t bytes)
{
    bytes = (bytes + 3) & ~3u;
    if (pool_at < SPARE1 && pool_at + bytes > SPARE0 + SPARE_SIZE) {   /* a la página 1 */
        pool_at = SPARE1;
        pool_end = MUSIC_MEM;
    }
    if (pool_at + bytes > pool_end)
        return 0;
    void *p = pool_at;
    pool_at += bytes;
    return p;
}

/* El sprite num del fichero, preparado; si no está o no cabe, no se dibuja.
 * Se copia en el sitio del paisaje, que aún no se usa, para prepararlo. */
static sprite_t load_sprite(int num)
{
    sprite_t s = { 0, 0, 0 };
    zxv_bitmap b;
    if (zxv_gfxf_sprite(&gfx, num, &b, BG_MEM, BG_ROOM) - 1 >= BG_ROOM)
        return s;
    void *p = pool_take(zxv_spr_prepare(&b, 0, 0, b.w, b.h, 0));
    if (!p)
        return s;
    zxv_spr_prepare(&b, 0, 0, b.w, b.h, p);
    return (sprite_t){ p, b.w, b.h };
}

/* --- El paisaje -------------------------------------------------------------- */

static int shown_theme = -1;

void art_background(int theme)
{
    if (theme == shown_theme)
        return;
    shown_theme = theme;
    zxv_bitmap *layers[3] = { &bg_far, &bg_near, &bg_clouds };
    uint8_t *at = BG_MEM;
    for (int i = 0; i < 3; i++) {
        zxv_bitmap *b = layers[i];
        uint32_t need = zxv_gfxf_sprite(&gfx, SPR_BACKGROUND + theme * 3 + i, b, at,
                                        (uint32_t)(BG_MEM + BG_ROOM - at));
        if (!need || at + need > BG_MEM + BG_ROOM || b->w < W) {
            b->w = b->h = 0;                /* sin esa capa */
            continue;
        }
        at += need;
    }
}

/* --- Todo -------------------------------------------------------------------- */

void art_init(void)
{
    /* El route128.gfx de la tarjeta, si hay uno, en el sitio del de dentro. */
    uint32_t room = (uint32_t)(route128_gfx_end - route128_gfx) + GFX_SLACK;
    int n = zxv_file_load("route128.gfx", route128_gfx, room);
    if (n == ZXV_E_RANGE)
        zxv_report_error(0, T("route128.gfx es demasiado grande", "route128.gfx is too big"));
    if (n < 0)
        n = (int)(route128_gfx_end - route128_gfx);
    if (!zxv_gfxf_open(&gfx, route128_gfx, (uint32_t)n))
        zxv_report_error(0, T("route128.gfx no vale", "route128.gfx is not valid"));

    static const struct { uint16_t w; uint8_t solid, mirror; } info[OBJ_COUNT] = {
        [OBJ_PALM] = { 1500, 1, 1 }, [OBJ_BUSH] = { 800, 0, 0 }, [OBJ_UMBRELLA] = { 650, 1, 0 },
        [OBJ_CACTUS] = { 650, 1, 1 }, [OBJ_ROCK] = { 1000, 1, 1 }, [OBJ_PINE] = { 1100, 1, 0 },
        [OBJ_TREE] = { 1400, 1, 1 }, [OBJ_SIGN_L] = { 800, 1, 0 }, [OBJ_SIGN_R] = { 800, 1, 0 },
        [OBJ_BOARD] = { 1900, 1, 0 }, [OBJ_START] = { 3100, 0, 0 }, [OBJ_GATE] = { 3100, 0, 0 },
        [OBJ_GOAL] = { 3100, 0, 0 },
    };
    pool_at = SPARE0;
    pool_end = SPARE0 + SPARE_SIZE;
    for (int i = 0; i < OBJ_COUNT; i++) {
        int num = i < OBJ_START ? i : SPR_GATES + lang * 3 + i - OBJ_START;
        obj_art[i].s = load_sprite(num);
        obj_art[i].world_w = info[i].w;
        obj_art[i].solid = info[i].solid;
        obj_art[i].mirror = info[i].mirror;
    }
    for (int i = 0; i < CAR_TYPES; i++)
        car_art[i] = load_sprite(SPR_CARS + i);
    for (int i = 0; i < 5; i++)
        player_art[i] = load_sprite(SPR_PLAYER + i);
    shown_theme = -1;
    art_background(THEME_COAST);
}
