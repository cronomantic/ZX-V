/*
 * art.c - Los gráficos y los niveles de Umbra: los de umbra.gfx, que se
 * edita con zxv-gfx en el PC o con gfxedit en ZX-V. El programa lo lleva
 * dentro y, si en la carpeta desde la que se carga hay otro umbra.gfx, usa
 * ese.
 *
 * Las texturas:
 *
 *    0, 1   las paredes, de 128 x 128: cuatro de 64 x 64 en cada una. En la
 *           0, la piedra, el ladrillo, el musgo y el cristal (las de los
 *           niveles 1 a 4); en la 1, el templo (nivel 5), la puerta cerrada,
 *           la puerta abierta y la pared con una runa tallada
 *    2      el suelo, de 128 x 128: cuatro losas de 64, que es una celda
 *    3      el techo, de 64 x 64
 *
 * El suelo y el techo se dibujan con el 1 (oscuro), el 7 (medio) y el 15
 * (claro), y cada nivel los cambia por los suyos (view.c).
 *
 * Los sprites: 0 la runa, 1 el frasco de aceite, 2-4 la sombra (se anima
 * con los tres), 5 la lámpara y 6 la runa pequeña del marcador. En el
 * laberinto, una celda mide lo que 64 píxeles de una sombra o 96 de un
 * objeto.
 *
 * Los niveles son los mapas 0, 1, 2... (tantos como haya seguidos, hasta
 * 16), con los tiles de 8 x 8 del fichero: cada tile dice lo que hay en su
 * celda (MAP_FLOOR... en umbra.h; maze.c los lee). Los tiles solo están para
 * ver el mapa en el editor: el juego no los dibuja.
 *
 * La memoria: el fichero va con el programa; las texturas, copiadas para el
 * motor, van en la memoria de vídeo que no se ve de la página 0, y los
 * sprites preparados en la de la página 1 (umbra.h).
 */
#include "zxv_gfxfile.h"

#include "umbra.h"

sprite_t art[SPRITES];
zxv_m7tex tex_floor, tex_ceiling;
static zxv_m7tex tex_walls[2];

/* El fichero, dentro del programa, con 16 KB más detrás para que quepa uno
 * de la tarjeta algo más grande. */
#define GFX_SLACK   16384
#define STR_(x)     #x
#define STR(x)      STR_(x)
__asm__(".section .data.umbra_gfx, \"aw\"\n"
        ".balign 4\n"
        "umbra_gfx:\n"
        ".incbin \"examples/umbra/umbra.gfx\"\n"
        "umbra_gfx_end:\n"
        ".space " STR(GFX_SLACK) "\n"
        ".previous\n");
extern uint8_t umbra_gfx[], umbra_gfx_end[];

static zxv_gfxfile gfx;

/* Una pared de 64 x 64 que no está: un tablero, para que se note. */
static uint8_t missing[64 * 128];

/* La textura n, con el lado que se pide, en *at (que avanza); si no está o
 * es de otro tamaño, la textura queda sin texels. */
static void load_texture(int n, int side, zxv_m7tex *t, uint8_t **at, uint8_t *end)
{
    uint32_t need = zxv_gfxf_texture(&gfx, n, t, *at, (uint32_t)(end - *at));
    if (!need || *at + need > end || (1 << t->log2size) != side) {
        t->texels = 0;
        return;
    }
    *at += (need + 3) & ~3u;
}

static uint8_t *pool_at;

static sprite_t load_sprite(int num)
{
    sprite_t s = { 0, 0, 0 };
    zxv_bitmap b;
    static uint32_t pix[64 * 64 / 8];   /* hasta 64 x 64, para prepararlo */
    if (zxv_gfxf_sprite(&gfx, num, &b, pix, sizeof(pix)) - 1 >= sizeof(pix))
        return s;
    uint32_t need = zxv_spr_prepare(&b, 0, 0, b.w, b.h, 0);
    if (pool_at + need > MUSIC_MEM)
        return s;
    zxv_spr_prepare(&b, 0, 0, b.w, b.h, pool_at);
    s = (sprite_t){ pool_at, b.w, b.h };
    pool_at += (need + 3) & ~3u;
    return s;
}

const uint8_t *art_wall(int n)
{
    const zxv_m7tex *t = &tex_walls[n >> 2];
    if (!t->texels)
        return missing;
    return t->texels + (n & 2 ? 64 * 128 : 0) + (n & 1 ? 64 : 0);
}

int art_levels(void)
{
    int n = 0;
    while (n < 16 && zxv_gfxf_find(&gfx, ZXV_GFXF_MAP, n, NULL))
        n++;
    return n;
}

int art_level(int n, uint16_t *buf, uint32_t room, int *w, int *h)
{
    zxv_tilemap m;
    uint32_t need = zxv_gfxf_map(&gfx, n, &m, NULL, buf, room);
    if (!need || need > room)
        return 0;
    *w = m.w;
    *h = m.h;
    return 1;
}

void art_init(void)
{
    /* El umbra.gfx de la tarjeta, si hay uno, en el sitio del de dentro. */
    uint32_t room = (uint32_t)(umbra_gfx_end - umbra_gfx) + GFX_SLACK;
    int n = zxv_file_load("umbra.gfx", umbra_gfx, room);
    if (n == ZXV_E_RANGE)
        zxv_report_error(0, T("umbra.gfx es demasiado grande", "umbra.gfx is too big"));
    if (n < 0)
        n = (int)(umbra_gfx_end - umbra_gfx);
    if (!zxv_gfxf_open(&gfx, umbra_gfx, (uint32_t)n))
        zxv_report_error(0, T("umbra.gfx no vale", "umbra.gfx is not valid"));

    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++)
            missing[y * 128 + x] = ((x ^ y) & 8) ? 11 : 0;
    uint8_t *at = SPARE0, *end = SPARE0 + SPARE_SIZE;
    load_texture(0, 128, &tex_walls[0], &at, end);
    load_texture(1, 128, &tex_walls[1], &at, end);
    load_texture(2, 128, &tex_floor, &at, end);
    load_texture(3, 64, &tex_ceiling, &at, end);

    pool_at = SPARE1;
    for (int i = 0; i < SPRITES; i++)
        art[i] = load_sprite(i);
}
