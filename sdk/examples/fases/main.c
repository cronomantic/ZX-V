/*
 * fases - Un ejemplo de los bancos (D109, zxv_bank.h). Dos fases: el valle
 * (pelotas que botan) y el espacio (estrellas y una nave). Lo de cada fase
 * va en el propio fichero, en dos bancos: el código (valle.c, espacio.c, que
 * comparten una zona de la memoria) y el fondo (de mkfondos.py). Al empezar
 * una fase se cargan los suyos, con el cartel en la pantalla y lo que han
 * tardado. Se compila con tools/zxvbanks.py (ver el Makefile del SDK).
 *
 * Una tecla pasa a la otra fase (y solas, a los 8 segundos); Esc vuelve al
 * BASIC.
 */
#include "fases.h"
#include "zxv.h"
#include "zxv_api.h"
#include "zxv_bank.h"
#include "zxv_gfx.h"

#define KEY_ESC     27
#define SHOW_FRAMES (8 * ZXV_FRAME_HZ)

/* El fondo de la fase, en la VRAM oculta de la página 1 (D108). */
ZXV_VRAM1_DATA static uint32_t sky[W * SKY_H / 2 / 4];

static const struct {
    const char *code, *bg;              /* sus bancos */
    const char *name_es, *name_en;
    void (*draw)(int frame);            /* su función: pasa por el puente */
} stages[2] = {
    { "valle", "fondo1", "FASE 1: EL VALLE", "STAGE 1: THE VALLEY", valle_draw },
    { "espacio", "fondo2", "FASE 2: EL ESPACIO", "STAGE 2: OUTER SPACE", espacio_draw },
};

static int visible;

static int spanish(void)
{
    return zxv_lang_get() == ZXV_LANG_ES;
}

static void put_num(uint32_t v)
{
    char s[12];
    int n = 0;
    do {
        s[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n)
        zxv_print_char(s[--n]);
}

/* Dibuja en la página que no se ve y luego la enseña. */
static void begin(void)
{
    zxv_gfx_draw_page(1 - visible);
}

static void end(void)
{
    visible = 1 - visible;
    zxv_set_page(visible);
    zxv_wait_vblank();
}

/* Carga los dos bancos de la fase s, con su cartel: 0, o un error. */
static int load(int s, uint32_t *ms)
{
    for (int p = 0; p < 2; p++) {
        begin();
        zxv_cls(0);
        zxv_text_at(8, 11);
        zxv_text_colors(14, 0);
        zxv_print_str(spanish() ? stages[s].name_es : stages[s].name_en);
        end();
    }
    uint32_t t = zxv_time_us();
    int r = zxv_bank_preload(stages[s].code);
    if (r >= 0)
        r = zxv_bank_load(stages[s].bg, sky, sizeof sky);
    *ms = (zxv_time_us() - t + 500) / 1000;
    return r < 0 ? r : 0;
}

int main(void)
{
    *(volatile uint8_t *)(uintptr_t)ZXV_V_VSIZE = ZXV_VSIZE_SCREEN;    /* para la VRAM oculta */
    zxv_set_mode(0);
    zxv_set_page(0);
    zxv_bitmap bg = { (uint8_t *)sky, W / 2, W, SKY_H, 0 };
    for (int s = 0;; s = 1 - s) {
        uint32_t ms;
        int r = load(s, &ms);
        if (r < 0)
            zxv_report_error(r, spanish() ? "No se pueden cargar los bancos de la fase"
                                          : "Cannot load the stage's banks");
        for (int f = 0; f < SHOW_FRAMES; f++) {
            begin();
            zxv_gfx_rect(0, 0, W, SKY_Y, 1);
            zxv_text_at(1, 0);
            zxv_text_colors(15, 1);
            zxv_print_str(spanish() ? stages[s].name_es : stages[s].name_en);
            zxv_text_at(1, 1);
            zxv_text_colors(5, 1);
            zxv_print_str(spanish() ? "Bancos cargados en " : "Banks loaded in ");
            put_num(ms);
            zxv_print_str(" ms");
            zxv_gfx_blit(&bg, 0, 0, W, SKY_H, 0, SKY_Y, 0);
            stages[s].draw(f);
            end();
            int k = zxv_key_get();
            if (k == KEY_ESC)
                return 0;
            if (k >= 0)
                break;
        }
    }
}
