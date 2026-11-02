/*
 * ui_test.c - Ejecuta el game.c de verdad en el PC, con la API de ZX-V
 * sustituida por una pantalla simulada y unas pulsaciones guionizadas, y
 * guarda capturas (PPM + texto) para revisar la interfaz.
 *
 *   gcc -O1 -g -Wall -Wextra -Imock -I.. ui_test.c ../chess.c -o ui_test
 *   ./ui_test <escenario> <carpeta>
 *   python3 shots_to_png.py <carpeta>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zxv_gfxfile.h"

uint8_t mock_keymap[32];
uint32_t mock_frames;
uint8_t mock_border;

/* --- La pantalla simulada ------------------------------------------------------- */

uint8_t mock_vram[2][192 * 128];                /* como la real, en modo 0: 128 bytes por línea, 2 píxeles por byte */
typedef struct { uint32_t cp; uint8_t ink; } cell_t;
static cell_t cells[2][24][32];
static int draw_pg, vis_pg, pending_vis;
static int t_col, t_row, t_ink = 0, t_paper = 7;
static int viol_visible, viol_scroll, viol_font, viol_oob;
static int script_started;
static int ptr_draw;                           /* se está dibujando el puntero del ratón */

static int px_get(int pg, int x, int y)
{
    uint8_t v = mock_vram[pg][y * 128 + x / 2];
    return (x & 1) ? v >> 4 : v & 15;
}

/* Mini-fuente de 8 x 8 solo para las letras del título (la real es la de la ROM de ZX-V). Se dibujan
 * sus píxeles solo en las filas 22 y 23, la zona de trabajo del texto en doble altura. */
static const struct { char ch; uint8_t rows[8]; } MFONT[] = {
    { ' ', { 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 'Z', { 0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0 } },
    { 'X', { 0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0 } },
    { 'V', { 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0 } },
    { 'C', { 0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0 } },
    { 'H', { 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0 } },
    { 'E', { 0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0 } },
    { 'S', { 0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0 } },
};
#define N_MFONT ((int)(sizeof MFONT / sizeof MFONT[0]))

static void put(int x, int y, int c)
{
    if (x < 0 || y < 0 || x >= 256 || y >= 192)
        return;
    if (script_started && draw_pg == vis_pg)
        viol_visible++;
    uint8_t *b = &mock_vram[draw_pg][y * 128 + x / 2];
    *b = (x & 1) ? (uint8_t)((*b & 0x0F) | (c & 15) << 4) : (uint8_t)((*b & 0xF0) | (c & 15));
    if (!ptr_draw)
        cells[draw_pg][y >> 3][x >> 3].cp = 0;  /* lo dibujado encima tapa el texto (salvo el puntero) */
}

void zxv_gfx_draw_page(int p) { draw_pg = p; }
void zxv_set_page(int p) { pending_vis = p; }

void zxv_cls(int c)
{
    for (int y = 0; y < 192; y++)
        for (int x = 0; x < 256; x++)
            put(x, y, c);
    memset(cells[draw_pg], 0, sizeof cells[draw_pg]);
    t_col = t_row = 0;
}

void zxv_gfx_rect(int x, int y, int w, int h, int c)
{
    if (x < 0 || y < 0 || x + w > 256 || y + h > 192)
        viol_oob++;                             /* el motor real recorta; aquí lo anotamos */
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            put(x + i, y + j, c);
}

void zxv_gfx_circle(int cx, int cy, int r, int c, int fill)
{
    (void)fill;
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r + r / 2)
                put(cx + x, cy + y, c);
}

uint32_t zxv_spr_prepare(const zxv_bitmap *b, int sx, int sy, int w, int h, void *buf)
{
    uint32_t need = 8 + (uint32_t)h * (uint32_t)((w + 7) / 8) * 8;
    if (!buf)
        return need;
    uint8_t *o = buf;
    uint16_t *hdr = (uint16_t *)o;
    hdr[0] = (uint16_t)w;
    hdr[1] = (uint16_t)h;
    if (8 + (uint32_t)w * (uint32_t)h > need)
        viol_oob++;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t byte = b->pix[(sy + y) * b->stride + (sx + x) / 2];
            o[8 + y * w + x] = ((sx + x) & 1) ? (uint8_t)(byte >> 4) : (uint8_t)(byte & 15);
        }
    }
    return need;
}

static void spr_scaled(const void *spr, int x, int y, int dw, int dh, const uint8_t *col)
{
    const uint8_t *o = spr;
    int w = ((const uint16_t *)o)[0], h = ((const uint16_t *)o)[1];
    for (int j = 0; j < dh; j++) {
        for (int i = 0; i < dw; i++) {
            int c = o[8 + (j * h / dh) * w + (i * w / dw)];
            if (c == ZXV_TRANSPARENT)
                continue;
            put(x + i, y + j, col ? col[c] : c);
        }
    }
}

void zxv_spr_draw(const void *spr, int x, int y, int flags, const uint8_t *col)
{
    (void)flags;
    const uint8_t *o = spr;
    int w = ((const uint16_t *)o)[0], h = ((const uint16_t *)o)[1];
    ptr_draw = (w == 16 && h == 17);            /* las piezas son de 24 x 24: esto es el puntero */
    spr_scaled(spr, x, y, w, h, col);
    ptr_draw = 0;
}

void zxv_spr_draw_scaled(const void *spr, int x, int y, int w, int h, int flags, const uint8_t *col)
{
    (void)flags;
    spr_scaled(spr, x, y, w, h, col);
}

/* --- Texto ---------------------------------------------------------------------- */

void zxv_text_colors(int ink, int paper) { t_ink = ink; t_paper = paper; }
void zxv_text_at(int col, int row) { t_col = col; t_row = row; }

static void print_cp(uint32_t cp)
{
    static const uint32_t ok[] = { 0xA9, 0xE1, 0xE9, 0xED, 0xF3, 0xFA, 0xFC, 0xF1, 0xBF, 0xA1,
                                   0xC1, 0xC9, 0xCD, 0xD3, 0xDA, 0xDC, 0xD1 };
    if (cp > 0x7E) {
        int found = 0;
        for (unsigned i = 0; i < sizeof ok / sizeof ok[0]; i++)
            found |= ok[i] == cp;
        if (!found) {
            viol_font++;
            cp = '?';
        }
    }
    if (cp == '\n') {
        t_col = 0;
        t_row++;
        return;
    }
    if (t_row >= 24) {
        viol_scroll++;
        return;
    }
    const uint8_t *glyph = 0;
    if (t_row >= 22)
        for (int g = 0; g < N_MFONT; g++)
            if (MFONT[g].ch == (int)cp)
                glyph = MFONT[g].rows;
    for (int j = 0; j < 8; j++)
        for (int i = 0; i < 8; i++)
            put(t_col * 8 + i, t_row * 8 + j, (glyph && (glyph[j] & (0x80 >> i))) ? t_ink : t_paper);
    cells[draw_pg][t_row][t_col].cp = cp;
    cells[draw_pg][t_row][t_col].ink = (uint8_t)t_ink;
    if (++t_col >= 32) {
        t_col = 0;
        t_row++;
    }
}

void zxv_print_str(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    while (*p) {
        uint32_t cp = *p++;
        if (cp >= 0xC0) {                       /* UTF-8 de dos bytes */
            cp = ((cp & 0x1F) << 6) | (*p++ & 0x3F);
        }
        print_cp(cp);
    }
}

/* --- Ratón (el guion lo coloca; el puntero es absoluto) ------------------------------ */

static int mock_mx = 128, mock_my = 96, mock_mb, mock_wheel;

void zxv_mouse_init(zxv_mouse_t *p, int x, int y)
{
    p->x = x;
    p->y = y;
    p->wheel = 0;
    p->buttons = 0;
    mock_mx = x;
    mock_my = y;
    mock_mb = 0;
    mock_wheel = 0;
}

void zxv_mouse_update(zxv_mouse_t *p, int w, int h)
{
    p->x = mock_mx < 0 ? 0 : mock_mx >= w ? w - 1 : mock_mx;
    p->y = mock_my < 0 ? 0 : mock_my >= h ? h - 1 : mock_my;
    p->wheel = mock_wheel;
    p->buttons = (uint8_t)(mock_mb & 7);
}

/* --- Entrada y sonido ------------------------------------------------------------ */

int zxv_key_get(void) { return -1; }
uint32_t zxv_joy_read(int n) { (void)n; return 0; }
uint32_t zxv_lang_get(void)
{
    const char *l = getenv("ZXV_LANG");
    return (l && !strcmp(l, "en")) ? ZXV_LANG_EN : ZXV_LANG_ES;
}

static int ay_writes, ay_bad;
void zxv_ay_write(int reg, int val)
{
    ay_writes++;
    if (reg < 0 || reg > 31 || val < 0 || val > 255)
        ay_bad++;
}

/* --- Los gráficos: el chess.gfx que hace make (o el de CHESS_GFX) ------------------- */

static uint8_t gfx_buf[16384];
static uint32_t gfx_len;
#define GFX_DATA    gfx_buf
#define GFX_SIZE    gfx_len

static void gfx_read(void)
{
    const char *path = getenv("CHESS_GFX") ? getenv("CHESS_GFX") : "../../../build/chess.gfx";
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("no está %s: make build/chess.zxv en sdk/, o CHESS_GFX=fichero\n", path);
        exit(1);
    }
    gfx_len = (uint32_t)fread(gfx_buf, 1, sizeof gfx_buf, f);
    fclose(f);
}

int zxv_file_load(const char *name, void *addr, uint32_t max)
{
    (void)name, (void)addr, (void)max;
    return ZXV_E_NOENT;                         /* en la tarjeta no hay otro */
}

void zxv_report_error(int code, const char *msg)
{
    printf("informe del juego (%d): %s\n", code, msg);
    exit(1);
}

/* Lo de gfxfile.c que usa el juego, con el formato de png2gfx.py: "ZXVG", la versión 1,
 * el número de objetos y cada uno con su clase, su número, su longitud y sus datos. */
int zxv_gfxf_open(zxv_gfxfile *f, const void *data, uint32_t size)
{
    f->data = data;
    f->size = size;
    return size >= 8 && !memcmp(data, "ZXVG", 4) && f->data[4] == 1;
}

uint32_t zxv_gfxf_sprite(const zxv_gfxfile *f, int num, zxv_bitmap *b, void *buf, uint32_t room)
{
    const uint8_t *d = f->data;
    uint32_t pos = 8;
    for (int i = 0; i < (d[6] | d[7] << 8) && pos + 8 <= f->size; i++) {
        uint32_t len = d[pos + 4] | d[pos + 5] << 8 | d[pos + 6] << 16 | (uint32_t)d[pos + 7] << 24;
        if (d[pos] == ZXV_GFXF_SPRITE && (d[pos + 2] | d[pos + 3] << 8) == num) {
            const uint8_t *s = d + pos + 8;
            int w = s[0] | s[1] << 8, h = s[2] | s[3] << 8, line = (w + 1) / 2;
            uint32_t stride = (uint32_t)(line + 3) & ~3u;
            if (!buf || room < stride * h)
                return stride * h;
            for (int y = 0; y < h; y++)
                memcpy((uint8_t *)buf + y * stride, s + 4 + y * line, line);
            *b = (zxv_bitmap){ buf, (uint16_t)stride, (uint16_t)w, (uint16_t)h, 0 };
            return stride * h;
        }
        pos += 8 + len;
    }
    return 0;
}

/* --- El juego ---------------------------------------------------------------------- */

#define main game_main
#include "../game.c"
#undef main

/* --- El guion ----------------------------------------------------------------------- */

static const char *outdir = ".";

enum { EV_DOWN, EV_UP, EV_SHOT, EV_CALL, EV_QUIT, EV_MOUSE, EV_WHEEL };
typedef struct { int frame, kind, a, b, c; const char *s; void (*fn)(int); } Ev;
static Ev evs[1024];
static int nev;

static void ev(int frame, int kind, int a, const char *s, void (*fn)(int))
{
    evs[nev++] = (Ev){ frame, kind, a, 0, 0, s, fn };
}

static void mset(int frame, int x, int y, int buttons)
{
    evs[nev++] = (Ev){ frame, EV_MOUSE, x, y, buttons, 0, 0 };
}
static void mwheel(int frame, int d) { ev(frame, EV_WHEEL, d, 0, 0); }
/* Mover, pulsar y soltar el botón izquierdo (o el que se diga) sobre un punto. */
static void mclick(int frame, int x, int y, int btn)
{
    mset(frame, x, y, 0);
    mset(frame + 2, x, y, btn);
    mset(frame + 4, x, y, 0);
}

static void tap(int frame, int hid) { ev(frame, EV_DOWN, hid, 0, 0); ev(frame + 2, EV_UP, hid, 0, 0); }
static void shot(int frame, const char *name) { ev(frame, EV_SHOT, 0, name, 0); }
static void call(int frame, void (*fn)(int), int a) { ev(frame, EV_CALL, a, 0, fn); }

static void set_key(int hid, int down)
{
    if (down)
        mock_keymap[hid >> 3] |= (uint8_t)(1u << (hid & 7));
    else
        mock_keymap[hid >> 3] &= (uint8_t)~(1u << (hid & 7));
}

static void save_shot(const char *name)
{
    char path[256];
    snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb");
    for (int y = 0; y < 192; y++)
        for (int x = 0; x < 256; x++)
            fputc(px_get(vis_pg, x, y), f);
    fclose(f);
    snprintf(path, sizeof path, "%s/%s.txt", outdir, name);
    f = fopen(path, "w");
    for (int r = 0; r < 24; r++)
        for (int c = 0; c < 32; c++)
            if (cells[vis_pg][r][c].cp)
                fprintf(f, "%d %d %d %u\n", r, c, cells[vis_pg][r][c].ink, cells[vis_pg][r][c].cp);
    fclose(f);
}

static void finish(void)
{
    printf("frames=%u  escrituras en la página visible=%d  scrolls=%d  caracteres sin fuente=%d  fuera de pantalla=%d  AY=%d (malas %d)\n",
           mock_frames, viol_visible, viol_scroll, viol_font, viol_oob, ay_writes, ay_bad);
}

void zxv_wait_vblank(void)
{
    vis_pg = pending_vis;
    mock_frames++;
    for (int i = 0; i < nev; i++) {
        if (evs[i].frame != (int)mock_frames)
            continue;
        switch (evs[i].kind) {
        case EV_DOWN: set_key(evs[i].a, 1); break;
        case EV_UP: set_key(evs[i].a, 0); break;
        case EV_SHOT: save_shot(evs[i].s); break;
        case EV_CALL: evs[i].fn(evs[i].a); break;
        case EV_MOUSE: mock_mx = evs[i].a; mock_my = evs[i].b; mock_mb = evs[i].c; break;
        case EV_WHEEL: mock_wheel += evs[i].a; break;
        case EV_QUIT: finish(); exit(0);
        }
    }
    if (mock_frames > 20000) {
        printf("DEMASIADOS FRAMES: el guion no terminó\n");
        finish();
        exit(2);
    }
}

/* Utilidades del guion. */
static int failures;
static void expect(const char *what, long got, long want)
{
    if (got != want) {
        printf("FALLA %s: %ld, debía ser %ld\n", what, got, want);
        failures++;
    } else {
        printf("ok    %s = %ld\n", what, got);
    }
}

static void cb_cursor(int sq) { cursor = sq; dirty = 1; }

static int t_now;                               /* el "reloj" del guion */

static void sclick(int sq)                       /* mueve el cursor a la casilla y pulsa Intro */
{
    call(t_now, cb_cursor, sq);
    tap(t_now + 1, K_ENTER);
    t_now += 8;
}

static void sclick_ai(int sq)                    /* igual, pero espera la respuesta de la máquina */
{
    sclick(sq);
    t_now += 6;
}

static void cb_fen_two(int unused)
{
    (void)unused;
}

static void check_ia(int unused)
{
    (void)unused;
    expect("ia: posiciones en el historial tras e4 y respuesta", nhist, 3);
    expect("ia: le toca a las blancas", cur()->side, WHITE);
}
static void check_undo(int unused)
{
    (void)unused;
    expect("undo: vuelve a la posición inicial", nhist, 1);
}
static void check_confirm(int unused)
{
    (void)unused;
    expect("nueva partida: pide confirmación", confirm_new > 0, 1);
}
static void check_confirm_gone(int unused)
{
    (void)unused;
    expect("nueva partida: la confirmación caduca", confirm_new, 0);
}
static void check_mate(int unused)
{
    (void)unused;
    expect("mate del loco: estado", status, ST_MATE);
    expect("mate del loco: gana el negro (toca al blanco)", cur()->side, WHITE);
}
static void setup_promo(int unused)
{
    (void)unused;
    pos_from_fen(&hist[0], "8/P6k/8/8/8/8/8/K7 w - - 0 1");
    nhist = 1;
    status = pos_status(&hist[0]);
    cursor = SQ(0, 6);
    dirty = 1;
}
static void check_promo_open(int unused)
{
    (void)unused;
    expect("coronación: se abre el menú de corona", promo_on, 1);
}
static void check_promo_done(int unused)
{
    (void)unused;
    expect("coronación: sale una torre en a8", cur()->sq[SQ(0, 7)], ROOK);
    expect("coronación: ya no está el menú", promo_on, 0);
}
static void check_black(int unused)
{
    (void)unused;
    expect("negras: tablero girado", flipped, 1);
    expect("negras: inicial + blancas + e5 + respuesta = 4 posiciones", nhist, 4);
    expect("negras: me toca otra vez (negras)", cur()->side, BLACK);
}
static void check_cont(int unused)
{
    (void)unused;
    expect("menú: hay partida para continuar", menu_items[0], M_CONTINUE);
}
static void check_over_menu(int unused)
{
    (void)unused;
    expect("menú tras el mate: sin continuar", menu_items[0], M_WHITE);
}

static void check_hover(int unused)
{
    (void)unused;
    expect("ratón: el puntero se ve tras moverlo", mouse_vis, 1);
    expect("ratón: el menú resalta 'Dos jugadores'", menu_items[menu_pos], M_TWO);
}
static void check_nopointer(int unused)
{
    (void)unused;
    expect("ratón: sin tocarlo, no hay puntero", mouse_vis, 0);
}
static void check_wheel(int expect_level)
{
    expect("ratón: nivel tras la rueda / los botones", level, expect_level);
}
static void check_two(int unused)
{
    (void)unused;
    expect("ratón: clic en 'Dos jugadores' empieza la partida", mode, MODE_TWO);
}
static void check_cursor_f4(int unused)
{
    (void)unused;
    expect("ratón: el cursor sigue al puntero (f4)", cursor, SQ(5, 3));
}
static void check_sel_e2(int unused)
{
    (void)unused;
    expect("ratón: clic en e2 elige la pieza", sel, SQ(4, 1));
    expect("ratón: e2 tiene 2 jugadas", nsel, 2);
}
static void check_moved_e4(int unused)
{
    (void)unused;
    expect("ratón: clic en e4 mueve el peón", cur()->sq[SQ(4, 3)], PAWN);
    expect("ratón: e2 queda vacía", cur()->sq[SQ(4, 1)], EMPTY);
    expect("ratón: una jugada hecha", nhist, 2);
}
static void check_sel_d7(int unused)
{
    (void)unused;
    expect("ratón: clic en d7 elige el peón negro", sel, SQ(3, 6));
}
static void check_unsel(int unused)
{
    (void)unused;
    expect("ratón: botón derecho anula la elección", sel, NO_SQ);
}
static void check_btn_undo(int unused)
{
    (void)unused;
    expect("ratón: botón U:deshac del panel", nhist, 1);
}
static void check_kbd_hides(int unused)
{
    (void)unused;
    expect("ratón: el teclado esconde el puntero", mouse_vis, 0);
}
static void check_black_sel(int unused)
{
    (void)unused;
    expect("ratón (negras): clic en e7 con el tablero girado", sel, SQ(4, 6));
}
static void check_black_move(int unused)
{
    (void)unused;
    /* hist[2] es la posición justo tras mi jugada, antes de que la máquina responda
     * (que puede capturar: no se comprueba lo que hace ella). */
    expect("ratón (negras): clic en e5 mueve el peón negro a e5", hist[2].sq[SQ(4, 4)], -PAWN);
    expect("ratón (negras): e7 queda vacía", hist[2].sq[SQ(4, 6)], EMPTY);
}
static void check_promo_knight(int unused)
{
    (void)unused;
    expect("ratón: clic en el caballo de la rejilla corona caballo", cur()->sq[SQ(0, 7)], KNIGHT);
}
static void check_promo_hover(int unused)
{
    (void)unused;
    expect("ratón: pasar sobre la rejilla elige la pieza", promo_idx, 3);
}

static int count_color(int x, int y, int c)     /* píxeles de ese color en la casilla de 24 x 24 */
{
    int n = 0;
    for (int j = 0; j < CELL; j++)
        for (int i = 0; i < CELL; i++)
            n += px_get(vis_pg, x + i, y + j) == c;
    return n;
}
static void check_level_start(int unused)
{
    (void)unused;
    expect("inicio: el nivel es 1", level, 1);
    expect("inicio: el tablero es el primero", board_sel, 0);
}
static void check_board(int want) { expect("tablero elegido", board_sel, want); }
static void check_lvl(int want) { expect("nivel elegido", level, want); }
static void check_pos(int want) { expect("foco del menú", menu_pos, want); }
static void check_focus_level(int unused)
{
    (void)unused;
    expect("teclado: abajo x4 llega al selector de nivel", menu_pos, menu_n);
}
static void check_focus_board(int unused)
{
    (void)unused;
    expect("teclado: abajo x5 llega al selector de tablero", menu_pos, menu_n + 1);
}
static void check_hover_level(int unused)
{
    (void)unused;
    expect("ratón: pasar sobre el selector de nivel lo enfoca", menu_pos, menu_n);
}
static void check_hover_board(int unused)
{
    (void)unused;
    expect("ratón: pasar sobre el selector de tablero lo enfoca", menu_pos, menu_n + 1);
}
/* Columnas de un selector, como las calcula draw_menu: el texto centrado y, dos columnas
 * antes, el '>' del foco de teclado. */
static int selector_col(const char *label, int value, int marker)
{
    char buf[40];

    selector_text(buf, label, value);
    return (32 - ulen(buf)) / 2 - marker;
}
static void check_focus_level_marker(int unused)
{
    int c = selector_col(T("Nivel ", "Level "), level, 2);

    (void)unused;
    expect("teclado: el selector de nivel enfocado muestra el '>' amarillo",
           cells[vis_pg][LEVEL_ROW][c].cp == '>' && cells[vis_pg][LEVEL_ROW][c].ink == INK_HILITE, 1);
}
static void check_focus_board_marker(int unused)
{
    int c = selector_col(T("Tablero ", "Board "), board_sel + 1, 2);

    (void)unused;
    expect("teclado: el selector de tablero enfocado muestra el '>' amarillo",
           cells[vis_pg][BOARD_ROW][c].cp == '>' && cells[vis_pg][BOARD_ROW][c].ink == INK_HILITE, 1);
}
static void check_hover_level_nomarker(int unused)
{
    int c = selector_col(T("Nivel ", "Level "), level, 2);

    (void)unused;
    expect("ratón: el selector de nivel enfocado por el ratón no muestra '>'", cells[vis_pg][LEVEL_ROW][c].cp, 0);
}
static void check_hover_board_nomarker(int unused)
{
    int c = selector_col(T("Tablero ", "Board "), board_sel + 1, 2);

    (void)unused;
    expect("ratón: el selector de tablero enfocado por el ratón no muestra '>'", cells[vis_pg][BOARD_ROW][c].cp, 0);
}
/* La tinta del texto del selector de nivel: magenta (3) en reposo y brillante (11) con el foco. */
static void check_level_ink(int want)
{
    int c = selector_col(T("Nivel ", "Level "), level, 0);

    expect("nivel: el texto del selector está donde toca ('<')", cells[vis_pg][LEVEL_ROW][c].cp == '<', 1);
    expect("nivel: tinta del selector de nivel", cells[vis_pg][LEVEL_ROW][c].ink, want);
}
static void check_level_ink_rest(int unused) { (void)unused; check_level_ink(3); }
static void check_level_ink_focus(int unused) { (void)unused; check_level_ink(11); }
static void check_colors(int unused)
{
    (void)unused;
    expect("tablero: casilla oscura (d4, vacía) con el color oscuro", px_get(vis_pg, 80, 100), C_DARK);
    expect("tablero: casilla clara (e4, vacía) con el color claro", px_get(vis_pg, 100, 100), C_LIGHT);
    expect("cursor: marco negro por fuera (e2)", px_get(vis_pg, 96, 144), 0);
    expect("cursor: marco blanco por dentro (e2)", px_get(vis_pg, 97, 145), 15);
    expect("colores sin brillo en las casillas", (C_DARK < 8) && (C_LIGHT < 8), 1);
}
static void check_sel_color(int unused)
{
    (void)unused;
    expect("selección: la casilla elegida (e2) se pinta con el color de selección",
           count_color(96, 144, C_SELECT) > 150, 1);
}
static void check_last_color(int unused)
{
    (void)unused;
    expect("última jugada: e2 (vacía) con el color de última jugada", count_color(96, 144, C_LAST) > 500, 1);
}

static void scenario_tablero(void)
{
    shot(3, "tablero_01_menu_inicial");
    call(4, check_level_start, 0);
    call(4, check_level_ink_rest, 0);           /* sin foco: magenta normal */
    for (int i = 0; i < 4; i++)
        tap(6 + 4 * i, K_DOWN);                 /* hasta el selector de nivel */
    call(20, check_focus_level, 0);             /* el foco pasa por el selector de nivel */
    call(20, check_focus_level_marker, 0);      /* con el teclado, el '>' está */
    call(20, check_level_ink_focus, 0);         /* y el texto sube a magenta brillante */
    shot(21, "tablero_02_foco_nivel");
    tap(22, K_DOWN);                            /* al selector de tablero */
    call(24, check_focus_board, 0);             /* abajo x5: el selector de tablero */
    call(24, check_focus_board_marker, 0);      /* con el teclado, el '>' está */
    shot(25, "tablero_03_foco_tablero");
    tap(26, K_RIGHT); tap(30, K_RIGHT); tap(34, K_RIGHT); tap(38, K_RIGHT);   /* 4 veces: se queda en el último */
    call(42, check_board, 3);
    tap(43, K_LEFT);
    call(47, check_board, 2);
    tap(48, K_ENTER);                           /* Intro en el selector: el siguiente */
    call(52, check_board, 3);
    tap(53, K_ENTER);                           /* y da la vuelta */
    call(57, check_board, 0);
    tap(58, K_UP);                              /* al selector de nivel */
    call(62, check_focus_level, 0);
    tap(63, K_ENTER);                           /* Intro en el nivel: el siguiente */
    call(67, check_lvl, 2);
    tap(68, K_DOWN); tap(72, K_DOWN);           /* de vuelta a la primera opción */
    call(76, check_pos, 0);
    tap(77, K_RIGHT);                           /* en las opciones, izquierda y derecha cambian el nivel */
    call(81, check_lvl, 3);
    call(81, check_board, 0);
    mset(84, 128, 146, 0);                      /* ratón sobre el selector de nivel */
    call(88, check_hover_level, 0);             /* lo enfoca */
    call(88, check_hover_level_nomarker, 0);    /* sin '>' */
    call(88, check_level_ink_focus, 0);         /* y sube a magenta brillante */
    shot(89, "tablero_04_hover_nivel");
    mset(90, 200, 164, 0);                      /* y sobre el selector de tablero */
    call(94, check_hover_board, 0);
    call(94, check_hover_board_nomarker, 0);    /* sin '>' amarillo */
    shot(95, "tablero_05_hover_tablero");
    mclick(96, 200, 164, 1);                    /* ">" */
    call(104, check_board, 1);
    mclick(108, 40, 164, 1);                    /* "<" */
    call(116, check_board, 0);
    mset(118, 128, 168, 0);
    mwheel(120, 1);                             /* rueda sobre el tablero: el siguiente */
    call(123, check_board, 1);
    mset(124, 128, 146, 0);
    mwheel(126, 1);                             /* rueda sobre el nivel: nivel 4 */
    call(129, check_lvl, 4);
    call(129, check_board, 1);
    mclick(132, 200, 164, 1);                   /* ">": el tercero (magenta-cian) */
    call(140, check_board, 2);
    mclick(142, 40, 164, 1);
    call(147, check_board, 1);                  /* "<": el segundo (magenta-amarillo) */
    mclick(148, 40, 164, 1);                    /* y otro "<": el primero, el azul-cian */
    call(152, check_board, 0);
    shot(153, "tablero_06_azul_cian_menu");     /* el azul-cian, el primero de la lista */
    mset(154, 128, 84, 0);                      /* "Dos jugadores" */
    mclick(156, 128, 84, 1);
    shot(168, "tablero_07_partida_azul_cian");
    call(169, check_colors, 0);
    t_now = 172;
    sclick(SQ(4, 1));                           /* elegir e2 */
    call(t_now - 2, check_sel_color, 0);
    shot(t_now - 1, "tablero_08_seleccion");
    sclick(SQ(4, 3));                           /* e4 */
    call(t_now, check_last_color, 0);
    shot(t_now + 1, "tablero_09_ultima_jugada");
    tap(t_now + 3, K_M);
    ev(t_now + 12, EV_QUIT, 0, 0, 0);
}

static void cb_board(int b) { board_sel = b; dirty = 1; }

/* La misma partida (el mate del loco) en cada uno de los cuatro tableros, para revisar a
 * simple vista que la selección, la última jugada, el cursor y el jaque se ven en todos. */
static void scenario_tableros(void)
{
    static const char *const sel_names[4] = { "tableros_1_seleccion", "tableros_2_seleccion",
                                              "tableros_3_seleccion", "tableros_4_seleccion" };
    static const char *const mate_names[4] = { "tableros_1_mate", "tableros_2_mate",
                                               "tableros_3_mate", "tableros_4_mate" };
    int t0 = 3;

    for (int b = 0; b < 4; b++) {
        call(t0, cb_board, b);
        tap(t0 + 2, K_DOWN);
        tap(t0 + 6, K_DOWN);
        tap(t0 + 10, K_ENTER);                  /* Dos jugadores */
        t_now = t0 + 20;
        sclick(SQ(5, 1)); sclick(SQ(5, 2));     /* f3 */
        sclick(SQ(4, 6)); sclick(SQ(4, 4));     /* e5 */
        sclick(SQ(6, 1)); sclick(SQ(6, 3));     /* g4 */
        sclick(SQ(3, 7));                       /* la dama negra, elegida */
        shot(t_now - 1, sel_names[b]);
        sclick(SQ(7, 3));                       /* Dh4# */
        shot(t_now, mate_names[b]);
        call(t_now, check_mate, 0);
        t0 = t_now + 4;
        tap(t0, K_M);
        t0 += 8;
    }
    ev(t0, EV_QUIT, 0, 0, 0);
}

static void cb_pos(int idx)
{
    static const char *const FEN[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",          /* 0 inicial */
        "rnbqkbnr/ppp2ppp/3p4/4p3/Q7/2P5/PP1PPPPP/RNB1KBNR b KQkq - 1 3",   /* 1 jaque a las negras */
        "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3",    /* 2 mate: ganan negras */
        "R5k1/5ppp/8/8/8/8/8/6K1 b - - 1 1",                                /* 3 mate: ganan blancas */
        "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",                                   /* 4 rey ahogado */
        "8/8/4k3/8/8/3K4/4P3/8 w - - 100 80",                               /* 5 regla de las 50 jugadas */
        "8/8/4k3/8/8/3KN3/8/8 w - - 0 1",                                   /* 6 material insuficiente */
    };
    pos_from_fen(&hist[0], FEN[idx]);
    nhist = 1;
    status = pos_status(&hist[0]);
    sel = NO_SQ;
    nsel = 0;
    promo_on = 0;
    thinking = 0;
    confirm_new = 0;
    dirty = 1;
}
static void cb_mode(int m) { mode = m; dirty = 1; }
static void cb_level(int l) { level = l; dirty = 1; }
static void cb_think(int on) { thinking = on; dirty = 1; }
static void cb_confirm(int v) { confirm_new = v; dirty = 1; }
static void cb_promo(int on) { promo_on = on; promo_idx = 0; dirty = 1; }
static void cb_cursor_h8(int unused) { (void)unused; cursor = SQ(7, 7); dirty = 1; }
static void cb_cursor_e2(int unused) { (void)unused; cursor = SQ(4, 1); dirty = 1; }

static void check_no_cancel_zone(int unused)
{
    (void)unused;
    expect("zona del antiguo botón Esc (fila 17): ya no anula la elección (sigue elegida d7)", sel, SQ(3, 6));
}
static void check_new_btn(int unused)
{
    (void)unused;
    expect("ratón: botón N: nueva (fila 20) empieza otra partida (el cursor vuelve a e2)", cursor, SQ(4, 1));
}

/* El título del menú principal, píxel a píxel: cada fila de la letra, repetida dos veces, "Z X V" en
 * rojo y "C H E S S" en magenta, centrados, y la zona de trabajo otra vez en negro (solo queda
 * libre la fila 23: la 22 la pinta ahora el nombre del tablero del selector). */
static void check_title(int unused)
{
    (void)unused;
    static const char *const LINE[2] = { "Z X V", "C H E S S" };
    static const int YT[2] = { 2, 18 };
    int ink[2] = { C_TITLE_ZXV, C_TITLE_CHESS };
    int bad = 0, lit[2] = { 0, 0 };
    static uint8_t want[40][256];

    memset(want, 0, sizeof want);
    for (int l = 0; l < 2; l++) {
        int n = (int)strlen(LINE[l]), x0 = (256 - 8 * n) / 2;
        for (int c = 0; c < n; c++)
            for (int g = 0; g < N_MFONT; g++)
                if (MFONT[g].ch == LINE[l][c])
                    for (int r = 0; r < 8; r++)
                        for (int b = 0; b < 8; b++)
                            if (MFONT[g].rows[r] & (0x80 >> b))
                                want[YT[l] + 2 * r][x0 + c * 8 + b] = want[YT[l] + 2 * r + 1][x0 + c * 8 + b] = (uint8_t)ink[l];
    }
    for (int y = 0; y < 40; y++)
        for (int x = 0; x < 256; x++) {
            bad += px_get(vis_pg, x, y) != want[y][x];
            if (want[y][x])
                lit[y >= 18]++;
        }
    int scratch = 0;
    for (int y = 184; y < 192; y++)             /* la fila 23: de la zona de trabajo, la única que el menú no pinta */
        for (int x = 0; x < 256; x++)
            scratch += px_get(vis_pg, x, y) != 0;
    expect("título del menú: píxeles distintos de los esperados (doble altura, rojo y magenta)", bad, 0);
    expect("título del menú: hay píxeles de ZXV y de CHESS", lit[0] > 100 && lit[1] > 150, 1);
    expect("título del menú: la zona de trabajo libre (fila 23) queda en negro", scratch, 0);
    expect("título del menú: ZXV rojo brillante (10)", ink[0], 10);
    expect("título del menú: CHESS magenta brillante (11)", ink[1], 11);
}

/* Recorre todos los estados del panel y guarda una captura de cada uno (para mirarlas con
 * shots_to_png.py). Se ejecuta en español y en inglés. */
static void scenario_panel(void)
{
    int f = 3;
    call(2, check_title, 0);
    shot(2, "panel_menu_principal");
    tap(f, K_DOWN); tap(f + 4, K_DOWN); tap(f + 8, K_ENTER);          /* dos jugadores */
    f = 20;
#define STATE(name, ...) do { __VA_ARGS__; shot(f + 3, name); f += 8; } while (0)
    STATE("panel_1p_blancas", call(f, cb_pos, 0), call(f, cb_mode, MODE_WHITE), call(f, cb_level, 4), call(f, cb_cursor_h8, 0));
    STATE("panel_2p_blancas", call(f, cb_mode, MODE_TWO), call(f, cb_cursor_e2, 0));
    STATE("panel_jaque", call(f, cb_pos, 1));
    STATE("panel_pensando", call(f, cb_think, 1));
    call(f, cb_think, 0);
    STATE("panel_seguro", call(f, cb_confirm, 100));
    call(f, cb_confirm, 0);
    STATE("panel_corona", call(f, cb_pos, 0), call(f, cb_promo, 1));
    call(f, cb_promo, 0);
    STATE("panel_mate_negras", call(f, cb_pos, 2));
    STATE("panel_mate_blancas", call(f, cb_pos, 3));
    STATE("panel_ahogado", call(f, cb_pos, 4));
    STATE("panel_50", call(f, cb_pos, 5));
    STATE("panel_material", call(f, cb_pos, 6));
    STATE("panel_hover_u", call(f, cb_pos, 0), mset(f + 1, 230, 146, 0));      /* el puntero sobre la fila 18 */
    STATE("panel_hover_n", mset(f, 230, 162, 0));                              /* fila 20 */
    STATE("panel_hover_m", mset(f, 230, 178, 0));                              /* fila 22 */
#undef STATE
    tap(f, K_M);
    ev(f + 8, EV_QUIT, 0, 0, 0);
}

static void scenario_raton(void)
{
    shot(4, "raton_01_menu_sin_puntero");
    call(4, check_nopointer, 0);
    mset(6, 128, 84, 0);                        /* sobre "Dos jugadores" */
    shot(9, "raton_02_menu_hover");
    call(9, check_hover, 0);
    call(5, check_level_start, 0);
    mwheel(10, 1);                              /* rueda arriba: nivel 2 */
    call(12, check_wheel, 2);
    mclick(14, 100, 140, 1);                    /* "<" del selector de nivel: nivel 1 */
    call(20, check_wheel, 1);
    shot(21, "raton_03_menu_nivel");
    mclick(24, 128, 84, 1);                     /* clic en "Dos jugadores" */
    call(32, check_two, 0);
    mset(34, 130, 110, 0);                      /* el cursor sigue al puntero (casilla f4) */
    call(37, check_cursor_f4, 0);
    mclick(40, 108, 156, 1);                    /* e2 */
    call(47, check_sel_e2, 0);
    shot(48, "raton_04_seleccion");
    mclick(50, 108, 108, 1);                    /* e4 */
    call(57, check_moved_e4, 0);
    mclick(60, 84, 36, 1);                      /* d7 (negras) */
    call(67, check_sel_d7, 0);
    mclick(70, 84, 36, 2);                      /* botón derecho: anula */
    call(77, check_unsel, 0);
    mclick(78, 84, 36, 1);                      /* elegir d7 (negras) */
    mclick(86, 230, 139, 1);                    /* la fila 17, donde estaba Esc:no: ya no hace nada */
    call(94, check_no_cancel_zone, 0);
    mclick(96, 84, 36, 2);                      /* botón derecho: anula la elección */
    mset(106, 200, 147, 0);                     /* sobre "U: atras" */
    shot(109, "raton_05_boton_panel");
    mclick(110, 200, 147, 1);                   /* deshacer */
    call(117, check_btn_undo, 0);
    tap(120, K_RIGHT);                          /* se vuelve al teclado */
    call(125, check_kbd_hides, 0);
    shot(126, "raton_06_teclado");
    mclick(130, 200, 163, 1);                   /* "N: nueva" (fila 20): sin nada que perder, empieza ya */
    call(137, check_new_btn, 0);
    mclick(140, 200, 179, 1);                   /* "M: menu" (fila 22) */
    shot(149, "raton_07_menu_continuar");
    mclick(152, 128, 116, 1);                   /* "Salir" (la quinta opción) */
    ev(176, EV_QUIT, 0, 0, 0);                  /* no se debería llegar */
}

static void scenario_raton_promo(void)
{
    tap(3, K_DOWN);
    tap(7, K_DOWN);
    tap(11, K_ENTER);
    call(16, setup_promo, 0);
    mclick(20, 12, 36, 1);                      /* a7 */
    mclick(30, 12, 12, 1);                      /* a8: se abre la corona */
    call(38, check_promo_open, 0);
    mset(40, 238, 154, 0);                      /* sobre el caballo */
    call(43, check_promo_hover, 0);
    shot(44, "ratonpromo_01_hover");
    mclick(46, 238, 154, 1);
    call(54, check_promo_knight, 0);
    shot(55, "ratonpromo_02_hecho");
    ev(58, EV_QUIT, 0, 0, 0);
}

static void scenario_raton_negras(void)
{
    tap(3, K_DOWN);                             /* con negras (la máquina abre) */
    tap(7, K_ENTER);
    mclick(30, 84, 156, 1);                     /* e7, con el tablero girado */
    call(37, check_black_sel, 0);
    shot(38, "ratonnegras_01_seleccion");
    mclick(40, 84, 108, 1);                     /* e5 */
    call(47, check_black_move, 0);
    shot(60, "ratonnegras_02_tras_respuesta");
    ev(62, EV_QUIT, 0, 0, 0);
}

static void scenario_ia(void)
{
    t_now = 3;
    shot(t_now, "ia_01_menu");
    tap(5, K_RIGHT);                            /* nivel 3 -> 4 */
    shot(8, "ia_02_menu_nivel2");
    tap(10, K_LEFT);
    tap(14, K_ENTER);                           /* jugar con blancas */
    shot(20, "ia_03_inicio");
    t_now = 24;
    sclick(SQ(4, 1));                            /* e2 */
    shot(t_now - 2, "ia_04_seleccion");
    sclick_ai(SQ(4, 3));                         /* e4 */
    shot(t_now, "ia_05_respuesta");
    call(t_now, check_ia, 0);
    t_now += 2;
    sclick(SQ(6, 0));                            /* g1 */
    shot(t_now - 2, "ia_06_caballo");
    sclick(SQ(5, 2));                            /* f3 */
    t_now += 6;
    shot(t_now, "ia_07_tras_nf3");
    tap(t_now + 2, K_U);                        /* deshacer */
    shot(t_now + 8, "ia_08_tras_undo");
    t_now += 10;
    tap(t_now, K_U);
    call(t_now + 6, check_undo, 0);
    t_now += 8;
    sclick(SQ(4, 1)); sclick_ai(SQ(4, 3));      /* otra vez e4, para tener algo que perder */
    tap(t_now, K_N);                            /* nueva partida: pide confirmar */
    call(t_now + 3, check_confirm, 0);
    shot(t_now + 4, "ia_09_confirmar");
    t_now += 200;                               /* caduca */
    call(t_now, check_confirm_gone, 0);
    shot(t_now + 1, "ia_10_caducado");
    t_now += 4;
    tap(t_now, K_M);                            /* al menú */
    call(t_now + 6, check_cont, 0);
    shot(t_now + 6, "ia_11_menu_continuar");
    tap(t_now + 10, K_ESC);                     /* salir */
    ev(t_now + 30, EV_QUIT, 0, 0, 0);
}

static void scenario_mate(void)
{
    t_now = 3;
    tap(t_now, K_DOWN);
    tap(t_now + 4, K_DOWN);                     /* "Dos jugadores" */
    shot(t_now + 7, "mate_01_menu");
    tap(t_now + 8, K_ENTER);
    t_now = 20;
    sclick(SQ(5, 1)); sclick(SQ(5, 2));           /* f3 */
    sclick(SQ(4, 6)); sclick(SQ(4, 4));           /* e5 */
    sclick(SQ(6, 1)); sclick(SQ(6, 3));           /* g4 */
    sclick(SQ(3, 7)); shot(t_now - 2, "mate_02_dama_elegida");
    sclick(SQ(7, 3));                            /* Dh4# */
    shot(t_now, "mate_03_mate");
    call(t_now, check_mate, 0);
    t_now += 3;
    tap(t_now, K_ENTER);                        /* en pantalla de fin: no hace nada */
    tap(t_now + 6, K_U);                        /* deshacer el mate */
    shot(t_now + 12, "mate_04_deshecho");
    t_now += 14;
    tap(t_now, K_M);
    ev(t_now + 10, EV_QUIT, 0, 0, 0);
}

static void scenario_promo(void)
{
    t_now = 3;
    tap(t_now, K_DOWN);
    tap(t_now + 4, K_DOWN);
    tap(t_now + 8, K_ENTER);
    call(t_now + 14, setup_promo, 0);
    t_now = 24;
    sclick(SQ(0, 6));                            /* a7 */
    sclick(SQ(0, 7));                            /* a8 */
    call(t_now, check_promo_open, 0);
    shot(t_now + 1, "promo_01_corona");
    tap(t_now + 2, K_RIGHT);
    shot(t_now + 8, "promo_02_torre");
    tap(t_now + 10, K_ENTER);
    t_now += 20;
    call(t_now, check_promo_done, 0);
    shot(t_now + 1, "promo_03_hecho");
    ev(t_now + 5, EV_QUIT, 0, 0, 0);
}

static void scenario_negras(void)
{
    t_now = 3;
    tap(t_now, K_DOWN);                         /* "Jugar con negras" */
    tap(t_now + 4, K_ENTER);
    shot(t_now + 12, "negras_01_apertura_blanca");
    t_now = 22;
    sclick(SQ(4, 6));                            /* e7 */
    shot(t_now - 2, "negras_02_seleccion");
    sclick_ai(SQ(4, 4));                         /* e5 */
    call(t_now, check_black, 0);
    shot(t_now + 1, "negras_03_respuesta");
    ev(t_now + 3, EV_QUIT, 0, 0, 0);
}

static void scenario_fin(void)
{
    /* Partida de dos jugadores hasta el mate y vuelta al menú. */
    t_now = 3;
    tap(t_now, K_DOWN);
    tap(t_now + 4, K_DOWN);
    tap(t_now + 8, K_ENTER);
    t_now = 20;
    sclick(SQ(5, 1)); sclick(SQ(5, 2));
    sclick(SQ(4, 6)); sclick(SQ(4, 4));
    sclick(SQ(6, 1)); sclick(SQ(6, 3));
    sclick(SQ(3, 7)); sclick(SQ(7, 3));
    tap(t_now, K_M);
    call(t_now + 6, check_over_menu, 0);
    shot(t_now + 6, "fin_01_menu_tras_mate");
    tap(t_now + 10, K_ESC);
    ev(t_now + 20, EV_QUIT, 0, 0, 0);
}

int main(int argc, char **argv)
{
    const char *sc = argc > 1 ? argv[1] : "ia";
    if (argc > 2)
        outdir = argv[2];
    (void)cb_fen_two;

    if (!strcmp(sc, "ia")) scenario_ia();
    else if (!strcmp(sc, "mate")) scenario_mate();
    else if (!strcmp(sc, "promo")) scenario_promo();
    else if (!strcmp(sc, "negras")) scenario_negras();
    else if (!strcmp(sc, "fin")) scenario_fin();
    else if (!strcmp(sc, "raton")) scenario_raton();
    else if (!strcmp(sc, "ratonpromo")) scenario_raton_promo();
    else if (!strcmp(sc, "ratonnegras")) scenario_raton_negras();
    else if (!strcmp(sc, "tablero")) scenario_tablero();
    else if (!strcmp(sc, "tableros")) scenario_tableros();
    else if (!strcmp(sc, "panel")) scenario_panel();
    else { printf("escenario desconocido\n"); return 1; }

    gfx_read();
    script_started = 1;
    game_main();                                /* vuelve cuando se sale del menú con Esc */
    finish();
    printf(failures ? "%d FALLOS\n" : "sin fallos en las comprobaciones\n", failures);
    return failures != 0;
}
