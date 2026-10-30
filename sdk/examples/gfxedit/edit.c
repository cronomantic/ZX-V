/*
 * edit.c - Los sprites, los tiles y las texturas: el lienzo ampliado, las
 * herramientas de dibujo y las operaciones con la imagen entera.
 *
 * El botón izquierdo pinta con color_l y el derecho con color_r. En los
 * tiles, el lienzo enseña el tile ampliado, al lado el tile repetido (para
 * ver si casa con sus vecinos) y debajo todo el juego, para elegir.
 */
#include "gfxedit.h"

#define KEY_LEFT    (ZXV_KEY_SPECIAL | 0x50)
#define KEY_RIGHT   (ZXV_KEY_SPECIAL | 0x4F)
#define KEY_UP      (ZXV_KEY_SPECIAL | 0x52)
#define KEY_DOWN    (ZXV_KEY_SPECIAL | 0x51)
#define KEY_PGUP    (ZXV_KEY_SPECIAL | 0x4B)
#define KEY_PGDN    (ZXV_KEY_SPECIAL | 0x4E)

static int cur[N_TABS];                 /* el sprite, el tile o la textura */
static int zoom[N_TABS] = { 8, 20, 10, 1, 4 };
static int ox[N_TABS], oy[N_TABS];      /* la esquina del lienzo, en la imagen */
static int top[N_TABS];                 /* la primera fila del juego de tiles */
static int tool = T_PEN;
static int anim, anim_frames = 4, anim_speed = 8, anim_step, anim_count;

static int drag;                        /* el botón que se arrastra (1 o 2), o 0 */
static int sx, sy, lx, ly;              /* dónde empezó y el último píxel */

/* Lo copiado, empaquetado como la imagen (hasta 8 KB). */
static uint8_t clip[8192] __attribute__((aligned(4)));
static int clip_w, clip_h;

static const int zooms[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32 };

static int is_tiles(void) { return tab == TAB_T8 || tab == TAB_T16; }
static int tile_side(void) { return tab == TAB_T8 ? 8 : 16; }

int edit_slot(void)
{
    switch (tab) {
    case TAB_SPR: return SLOT_SPRITE + cur[tab];
    case TAB_T8: return SLOT_TILES8;
    case TAB_T16: return SLOT_TILES16;
    case TAB_TEX: return SLOT_TEX + cur[tab];
    }
    return -1;
}

static int image(img_t *im)
{
    return obj_image(edit_slot(), cur[tab], im) == 0;
}

/* El lienzo: dónde está y a cuánto se amplía. */
static void canvas(int *x, int *y, int *w, int *h, int *z)
{
    *x = CX;
    *y = CY;
    if (is_tiles()) {
        *z = 160 / tile_side();
        *w = *h = 160;
    } else {
        *z = zoom[tab];
        *w = CW;
        *h = CH;
    }
}

/* El juego de tiles, debajo del tile: celdas de 2x el tile. */
#define GRID_Y      (CY + 168)
#define GRID_H      (CY + CH - GRID_Y)

static int grid_cols(void) { return CW / (tile_side() * 2); }
static int grid_rows(void) { return GRID_H / (tile_side() * 2); }

/* Guarda para deshacer lo que se va a cambiar de la imagen. */
static void save_undo(void)
{
    if (is_tiles()) {
        uint32_t n = (uint32_t)tile_side() * tile_side() / 2;
        undo_push(edit_slot(), (uint32_t)cur[tab] * n, n);
    } else {
        undo_push_all(edit_slot());
    }
    modified = 1;
    dirty = 1;
}

/* --- Dibujo de herramientas ------------------------------------------------ */

static void plot(img_t *im, int x, int y, int c)
{
    if (x >= 0 && y >= 0 && x < im->w && y < im->h)
        img_set(im, x, y, c);
}

static void line(img_t *im, int x0, int y0, int x1, int y1, int c)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int stx = x0 < x1 ? 1 : -1, sty = y0 < y1 ? 1 : -1, err = dx + dy;
    for (;;) {
        plot(im, x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += stx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sty;
        }
    }
}

static void rect(img_t *im, int x0, int y0, int x1, int y1, int c, int fill)
{
    if (x0 > x1) {
        int t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y0 > y1) {
        int t = y0;
        y0 = y1;
        y1 = t;
    }
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (fill || y == y0 || y == y1 || x == x0 || x == x1)
                plot(im, x, y, c);
}

/* Rellena por tramos de fila, con una pila de semillas. */
static void flood(img_t *im, int x, int y, int c)
{
    int from = img_get(im, x, y);
    if (from == c)
        return;
    static int16_t stack[1024][2];
    int n = 0;
    stack[n][0] = (int16_t)x;
    stack[n][1] = (int16_t)y;
    n++;
    while (n) {
        n--;
        x = stack[n][0];
        y = stack[n][1];
        if (img_get(im, x, y) != from)
            continue;
        int l = x, r = x;
        while (l > 0 && img_get(im, l - 1, y) == from)
            l--;
        while (r < im->w - 1 && img_get(im, r + 1, y) == from)
            r++;
        for (int i = l; i <= r; i++)
            img_set(im, i, y, c);
        for (int d = -1; d <= 1; d += 2) {
            int ny = y + d;
            if (ny < 0 || ny >= im->h)
                continue;
            for (int i = l; i <= r; i++)
                if (img_get(im, i, ny) == from && (i == l || img_get(im, i - 1, ny) != from) && n < 1024) {
                    stack[n][0] = (int16_t)i;
                    stack[n][1] = (int16_t)ny;
                    n++;
                }
        }
    }
}

/* --- Operaciones con la imagen entera ------------------------------------------ */

static void flip(img_t *im, int vertical)
{
    for (int y = 0; y < (vertical ? im->h / 2 : im->h); y++)
        for (int x = 0; x < (vertical ? im->w : im->w / 2); x++) {
            int x2 = vertical ? x : im->w - 1 - x, y2 = vertical ? im->h - 1 - y : y;
            int a = img_get(im, x, y);
            img_set(im, x, y, img_get(im, x2, y2));
            img_set(im, x2, y2, a);
        }
}

static void shift(img_t *im, int dx, int dy)
{
    if (dx) {
        for (int y = 0; y < im->h; y++) {
            int w = im->w, keep = img_get(im, dx > 0 ? w - 1 : 0, y);
            if (dx > 0)
                for (int x = w - 1; x > 0; x--)
                    img_set(im, x, y, img_get(im, x - 1, y));
            else
                for (int x = 0; x < w - 1; x++)
                    img_set(im, x, y, img_get(im, x + 1, y));
            img_set(im, dx > 0 ? 0 : w - 1, y, keep);
        }
    } else {
        for (int x = 0; x < im->w; x++) {
            int h = im->h, keep = img_get(im, x, dy > 0 ? h - 1 : 0);
            if (dy > 0)
                for (int y = h - 1; y > 0; y--)
                    img_set(im, x, y, img_get(im, x, y - 1));
            else
                for (int y = 0; y < h - 1; y++)
                    img_set(im, x, y, img_get(im, x, y + 1));
            img_set(im, x, dy > 0 ? 0 : h - 1, keep);
        }
    }
}

/* Gira 90 grados a la derecha (un sprite cambia de ancho y alto). */
static void rotate(void)
{
    img_t im;
    if (!image(&im))
        return;
    uint32_t bytes = im.stride * (uint32_t)im.h;
    if (bytes > WORK_SIZE) {
        message(TR("Too big to rotate", "Demasiado grande para girarlo"));
        return;
    }
    img_t src = { work, im.stride, im.w, im.h };
    memcpy(work, im.pix, bytes);
    save_undo();
    if (tab == TAB_SPR && im.w != im.h) {
        if (sprite_size(cur[tab], im.h, im.w)) {
            message(TR("Out of memory", "No queda memoria"));
            return;
        }
        image(&im);
    }
    for (int y = 0; y < src.h; y++)
        for (int x = 0; x < src.w; x++)
            img_set(&im, src.h - 1 - y, x, img_get(&src, x, y));
}

static void copy(void)
{
    img_t im;
    if (!image(&im))
        return;
    if (im.stride * (uint32_t)im.h > sizeof clip) {
        message(TR("Too big to copy", "Demasiado grande para copiarlo"));
        return;
    }
    img_t dst = { clip, stride_of(im.w), im.w, im.h };
    for (int y = 0; y < im.h; y++)
        for (int x = 0; x < im.w; x++)
            img_set(&dst, x, y, img_get(&im, x, y));
    clip_w = im.w;
    clip_h = im.h;
    message(TR("Copied", "Copiado"));
}

static void paste(void)
{
    img_t im;
    if (!clip_w) {
        message(TR("Nothing copied", "No hay nada copiado"));
        return;
    }
    if (tab == TAB_SPR) {
        undo_push_all(edit_slot());
        if (sprite_size(cur[tab], clip_w, clip_h)) {
            message(TR("Out of memory", "No queda memoria"));
            return;
        }
        modified = dirty = 1;
    } else {
        if (!image(&im))
            return;
        save_undo();
    }
    image(&im);
    img_t src = { clip, stride_of(clip_w), clip_w, clip_h };
    for (int y = 0; y < im.h && y < clip_h; y++)
        for (int x = 0; x < im.w && x < clip_w; x++)
            img_set(&im, x, y, img_get(&src, x, y));
}

/* --- El panel ---------------------------------------------------------------- */

#define PAL_ROW     (PROW + 5)          /* la paleta */
#define VIEW_ROW    (PROW + 26)         /* la vista a tamaño real */

static void line_text(int row, const char *s)
{
    field(PCOL, row, 64 - PCOL, C_INK, C_BG, s);
}

/* Un número con sus botones - y + (que mandan id y id + 1). */
static void stepper(int row, const char *label, int value, int id)
{
    char buf[24], *p = cat(buf, label);
    *p++ = ' ';
    num(p, value);
    text(PCOL, row, C_INK, C_BG, buf);
    int col = PCOL + 14;
    col += button(col, row, "-", id, 0) + 1;
    button(col, row, "+", id + 1, 0);
}

static void draw_panel(void)
{
    char buf[64], *p;
    img_t im;
    int have = image(&im), slot = edit_slot(), col;

    /* Qué se edita. */
    static const char *const what_en[] = { "Sprite ", "Tile ", "Tile ", "", "Texture " };
    static const char *const what_es[] = { "Sprite ", "Tile ", "Tile ", "", "Textura " };
    p = cat(buf, es ? what_es[tab] : what_en[tab]);
    p = num(p, cur[tab]);
    if (is_tiles() && objs[slot].len) {
        p = cat(p, " / ");
        num(p, objs[slot].h);
    }
    text(PCOL, PROW, C_TITLE, C_BG, buf);
    col = PCOL + 14;
    col += button(col, PROW, "<", B_PREV, 0) + 1;
    button(col, PROW, ">", B_NEXT, 0);

    /* El tamaño, y crear o borrar. */
    if (tab == TAB_SPR && have) {
        stepper(PROW + 1, TR("Width", "Ancho"), im.w, B_WMINUS);
        stepper(PROW + 2, TR("Height", "Alto"), im.h, B_HMINUS);
    } else if (tab == TAB_TEX && have) {
        stepper(PROW + 1, TR("Side", "Lado"), im.w, B_WMINUS);
        p = cat(buf, TR("Outside", "Fuera"));
        text(PCOL, PROW + 2, C_INK, C_BG, buf);
        box((PCOL + 9) * 8, (PROW + 2) * 8, 16, 8, objs[slot].c, C_INK);
        col = PCOL + 14;
        col += button(col, PROW + 2, "-", B_OUTMINUS, 0) + 1;
        button(col, PROW + 2, "+", B_OUTPLUS, 0);
    } else if (is_tiles() && objs[slot].len) {
        stepper(PROW + 1, "Tiles", objs[slot].h, B_WMINUS);
    }
    col = PCOL;
    if (is_tiles()) {
        button(col, PROW + 3, TR("Add a tile", "Añadir un tile"), B_CREATE, 0);
    } else if (!have) {
        button(col, PROW + 3, TR("Create", "Crear"), B_CREATE, 0);
    } else {
        col += button(col, PROW + 3, TR("Delete", "Borrar"), B_DELETE, 0) + 1;
        if (tab == TAB_TEX)
            button(col, PROW + 3, TR("Wraps", "Se repite"), B_WRAP, objs[slot].b);
    }

    /* La paleta: el botón izquierdo y el derecho. */
    for (int c = 0; c < 16; c++) {
        int x = (PCOL + (c & 7) * 2) * 8 + 4, y = (PAL_ROW + (c >> 3) * 2) * 8;
        if (c == TRANSP)
            checker(x, y, 16, 16);
        else
            box(x, y, 16, 16, c, C_INK);
        int ink = c == 0 || c == 1 || c == 9 ? 7 : 0, paper = c == TRANSP ? 15 : c;
        if (c == color_l)
            text(x / 8, y / 8, ink, paper, TR("L", "I"));
        if (c == color_r)
            text(x / 8 + 1, y / 8 + 1, ink, paper, TR("R", "D"));
    }
    line_text(PAL_ROW + 4, TR("L left, R right button", "I izquierdo, D derecho"));

    /* Las herramientas, de dos en dos. */
    static const char *const tools_en[] = { "Pen", "Line", "Box", "Full box", "Flood", "Pick" };
    static const char *const tools_es[] = { "Lápiz", "Línea", "Caja", "Caja llena", "Rellenar", "Coger" };
    for (int t = 0; t < N_TOOLS; t++) {
        if (!(t & 1))
            col = PCOL;
        col += button(col, PROW + 11 + t / 2, es ? tools_es[t] : tools_en[t], B_TOOL + t, t == tool) + 1;
    }

    /* Lo que se hace con la imagen entera. */
    col = PCOL;
    col += button(col, PROW + 15, TR("Flip H", "Espejo H"), B_FLIPH, 0) + 1;
    button(col, PROW + 15, TR("Flip V", "Espejo V"), B_FLIPV, 0);
    col = PCOL;
    col += button(col, PROW + 16, TR("Turn", "Girar"), B_ROTATE, 0) + 1;
    button(col, PROW + 16, TR("Clear", "Vaciar"), B_CLEAR, 0);
    text(PCOL, PROW + 17, C_INK, C_BG, TR("Shift", "Mover"));
    col = PCOL + 7;
    col += button(col, PROW + 17, "<", B_LEFT, 0);
    col += button(col, PROW + 17, ">", B_RIGHT, 0);
    col += button(col, PROW + 17, "^", B_UP, 0);
    button(col, PROW + 17, "v", B_DOWN, 0);
    col = PCOL;
    col += button(col, PROW + 18, TR("Copy", "Copiar"), B_COPY, 0) + 1;
    button(col, PROW + 18, TR("Paste", "Pegar"), B_PASTE, 0);
    if (!is_tiles())
        stepper(PROW + 20, "Zoom", zoom[tab], B_ZOOMOUT);

    /* La animación de los sprites: el actual y los siguientes. */
    if (tab == TAB_SPR) {
        button(PCOL, PROW + 22, TR("Animate", "Animar"), B_ANIM, anim);
        stepper(PROW + 23, TR("Frames", "Imágenes"), anim_frames, B_FRMINUS);
        stepper(PROW + 24, TR("Wait", "Espera"), anim_speed, B_SPMINUS);
    }

    /* La vista a tamaño real (o a 2x si es pequeña). */
    if (!is_tiles()) {
        int px = PCOL * 8, py = VIEW_ROW * 8, pw = (64 - PCOL) * 8, ph = (45 - VIEW_ROW) * 8 - 4;
        box(px, py, pw, ph, C_BG, C_INK);
        int n = tab == TAB_SPR && anim ? (cur[tab] + anim_step) % N_SPRITES : cur[tab];
        img_t v;
        if (obj_image(tab == TAB_SPR ? SLOT_SPRITE + n : slot, 0, &v) == 0) {
            int z = v.w <= pw / 2 - 2 && v.h <= ph / 2 - 2 ? 2 : 1;
            draw_image(&v, px + 2, py + 2, pw - 4, ph - 4, z, 0, 0, tab == TAB_SPR);
        }
    }
}

/* --- El lienzo --------------------------------------------------------------- */

static void draw_canvas(void)
{
    int x, y, w, h, z;
    img_t im;
    canvas(&x, &y, &w, &h, &z);
    box(x, y, w + 2, h + 2, C_BG, C_INK);
    x++, y++;
    if (!image(&im)) {
        text(x / 8 + 2, y / 8 + 2, C_INK, C_BG,
             is_tiles() ? TR("No tiles: Add a tile", "No hay tiles: Añadir un tile")
                        : TR("Empty: Create", "No existe: Crear"));
        return;
    }
    int t = tab == TAB_SPR || is_tiles();
    draw_image(&im, x, y, w, h, z, ox[tab], oy[tab], t);
    /* La rejilla, si se ve bien. */
    if (z >= 6) {
        int vw = (im.w - ox[tab]) * z < w ? (im.w - ox[tab]) * z : w;
        int vh = (im.h - oy[tab]) * z < h ? (im.h - oy[tab]) * z : h;
        for (int i = z; i < vw; i += z)
            zxv_gfx_line(x + i, y, x + i, y + vh - 1, 13);
        for (int j = z; j < vh; j += z)
            zxv_gfx_line(x, y + j, x + vw - 1, y + j, 13);
    }
    /* La línea o la caja que se está trazando. */
    if (drag && (tool == T_LINE || tool == T_RECT || tool == T_FRECT)) {
        int c = drag == 1 ? color_l : color_r;
        int ax = x + (sx - ox[tab]) * z, ay = y + (sy - oy[tab]) * z;
        int bx = x + (lx - ox[tab]) * z, by = y + (ly - oy[tab]) * z;
        zxv_gfx_clip(x, y, x + w, y + h);
        if (tool == T_LINE) {
            zxv_gfx_line(ax + z / 2, ay + z / 2, bx + z / 2, by + z / 2, c);
        } else {
            int x0 = ax < bx ? ax : bx, y0 = ay < by ? ay : by;
            int x1 = (ax > bx ? ax : bx) + z, y1 = (ay > by ? ay : by) + z;
            if (tool == T_FRECT) {
                zxv_gfx_rect(x0, y0, x1 - x0, y1 - y0, c);
            } else {
                int t = z >= 4 ? z / 2 : 1;
                zxv_gfx_rect(x0, y0, x1 - x0, t, c);
                zxv_gfx_rect(x0, y1 - t, x1 - x0, t, c);
                zxv_gfx_rect(x0, y0, t, y1 - y0, c);
                zxv_gfx_rect(x1 - t, y0, t, y1 - y0, c);
            }
        }
        zxv_gfx_clip(0, 0, SW, SH);
    }

    if (is_tiles()) {
        /* El tile repetido 3 x 3, a 2x. */
        int s = tile_side(), rx = CX + 176, ry = CY;
        for (int j = 0; j < 3; j++)
            for (int i = 0; i < 3; i++)
                draw_image(&im, rx + i * s * 2, ry + j * s * 2, s * 2, s * 2, 2, 0, 0, 1);
        /* El juego, para elegir. */
        int cols = grid_cols(), rows = grid_rows(), cell = s * 2, count = objs[edit_slot()].h;
        box(CX, GRID_Y - 1, cols * cell + 2, rows * cell + 2, C_BG, C_INK);
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++) {
                int n = (top[tab] + r) * cols + c;
                img_t t2;
                if (n >= count || obj_image(edit_slot(), n, &t2))
                    continue;
                draw_image(&t2, CX + 1 + c * cell, GRID_Y + r * cell, cell, cell, 2, 0, 0, 1);
                if (n == cur[tab]) {
                    zxv_gfx_rect(CX + 1 + c * cell, GRID_Y + r * cell, cell, 1, 2);
                    zxv_gfx_rect(CX + 1 + c * cell, GRID_Y + r * cell + cell - 1, cell, 1, 2);
                    zxv_gfx_rect(CX + 1 + c * cell, GRID_Y + r * cell, 1, cell, 2);
                    zxv_gfx_rect(CX + c * cell + cell, GRID_Y + r * cell, 1, cell, 2);
                }
            }
    }
}

/* El píxel de la imagen bajo el puntero, o 0 si no está sobre ella. */
static int at_pixel(int *px, int *py)
{
    int x, y, w, h, z;
    img_t im;
    canvas(&x, &y, &w, &h, &z);
    x++, y++;
    if (!image(&im) || mouse.x < x || mouse.y < y || mouse.x >= x + w || mouse.y >= y + h)
        return 0;
    *px = ox[tab] + (mouse.x - x) / z;
    *py = oy[tab] + (mouse.y - y) / z;
    return *px < im.w && *py < im.h;
}

void edit_draw(void)
{
    draw_canvas();
    draw_panel();
    int px, py;
    img_t im;
    if (at_pixel(&px, &py) && image(&im)) {
        char buf[64], *p = cat(buf, "x ");
        p = num(p, px);
        p = cat(p, "  y ");
        p = num(p, py);
        p = cat(p, TR("  colour ", "  color "));
        num(p, img_get(&im, px, py));
        field(0, 45, 40, C_INK, C_BG, buf);
    }
}

/* --- Ratón y teclado ----------------------------------------------------------- */

static void fix_view(void)
{
    img_t im;
    int x, y, w, h, z;
    canvas(&x, &y, &w, &h, &z);
    if (!image(&im) || is_tiles()) {
        ox[tab] = oy[tab] = 0;
        return;
    }
    int mx = im.w - w / z, my = im.h - h / z;
    if (ox[tab] > mx)
        ox[tab] = mx;
    if (oy[tab] > my)
        oy[tab] = my;
    if (ox[tab] < 0)
        ox[tab] = 0;
    if (oy[tab] < 0)
        oy[tab] = 0;
}

static void set_zoom(int d)
{
    int i = 0;
    while (i < 9 && zooms[i] < zoom[tab])
        i++;
    i += d;
    i = i < 0 ? 0 : i > 9 ? 9 : i;
    zoom[tab] = zooms[i];
    fix_view();
    dirty = 1;
}

/* El zoom más grande con el que se ve entera (al cambiar de imagen). */
static void fit(void)
{
    img_t im;
    if (is_tiles() || !image(&im))
        return;
    int i = 9;
    while (i > 0 && (im.w * zooms[i] > CW || im.h * zooms[i] > CH))
        i--;
    zoom[tab] = zooms[i];
    ox[tab] = oy[tab] = 0;
}

static void select_obj(int n)
{
    int max = tab == TAB_SPR ? N_SPRITES : tab == TAB_TEX ? N_TEXTURES : objs[edit_slot()].h;
    if (max <= 0)
        return;
    cur[tab] = (n % max + max) % max;
    if (is_tiles()) {
        int cols = grid_cols();
        if (cur[tab] / cols < top[tab])
            top[tab] = cur[tab] / cols;
        if (cur[tab] / cols >= top[tab] + grid_rows())
            top[tab] = cur[tab] / cols - grid_rows() + 1;
    }
    fit();
    dirty = 1;
}

static void resize(int dw, int dh)
{
    img_t im;
    int slot = edit_slot();
    if (is_tiles()) {
        int n = objs[slot].h + dw;
        if (n < 1 || n > MAX_TILES)
            return;
        undo_push_all(slot);
        if (tiles_count(tile_side(), n))
            message(TR("Out of memory", "No queda memoria"));
        if (cur[tab] >= n)
            select_obj(n - 1);
    } else if (image(&im)) {
        undo_push_all(slot);
        int r;
        if (tab == TAB_TEX) {
            int l2 = objs[slot].a + (dw > 0 ? 1 : -1);
            if (l2 < 1 || l2 > 8)
                return;
            r = texture_size(cur[tab], l2);
        } else {
            int w = im.w + dw, h = im.h + dh;
            w = w < 1 ? 1 : w > 1024 ? 1024 : w;
            h = h < 1 ? 1 : h > 1024 ? 1024 : h;
            r = sprite_size(cur[tab], w, h);
        }
        if (r)
            message(TR("Out of memory", "No queda memoria"));
        fix_view();
    }
    modified = dirty = 1;
}

void edit_click(int id, int right)
{
    img_t im;
    int step = right ? 10 : 1, slot = edit_slot();
    if (id >= B_TOOL && id < B_TOOL + N_TOOLS) {
        tool = id - B_TOOL;
        dirty = 1;
        return;
    }
    switch (id) {
    case B_PREV:
        select_obj(cur[tab] - step);
        break;
    case B_NEXT:
        select_obj(cur[tab] + step);
        break;
    case B_CREATE:
        undo_push_all(slot);
        if (is_tiles()) {
            int n = objs[slot].len ? objs[slot].h : 0;
            if (n < MAX_TILES && tiles_count(tile_side(), n + 1) == 0)
                select_obj(n);
            else
                message(TR("Out of memory", "No queda memoria"));
        } else if ((tab == TAB_SPR ? sprite_size(cur[tab], 16, 16) : texture_size(cur[tab], 5)) == 0) {
            fit();
        } else {
            message(TR("Out of memory", "No queda memoria"));
        }
        modified = dirty = 1;
        break;
    case B_DELETE:
        undo_push_all(slot);
        obj_delete(slot);
        modified = dirty = 1;
        break;
    case B_WMINUS:
        resize(-(right ? 8 : 1), 0);
        break;
    case B_WPLUS:
        resize(right ? 8 : 1, 0);
        break;
    case B_HMINUS:
        resize(0, -(right ? 8 : 1));
        break;
    case B_HPLUS:
        resize(0, right ? 8 : 1);
        break;
    case B_FLIPH:
    case B_FLIPV:
        if (image(&im)) {
            save_undo();
            flip(&im, id == B_FLIPV);
        }
        break;
    case B_ROTATE:
        rotate();
        break;
    case B_LEFT:
    case B_RIGHT:
    case B_UP:
    case B_DOWN:
        if (image(&im)) {
            save_undo();
            shift(&im, id == B_LEFT ? -1 : id == B_RIGHT ? 1 : 0, id == B_UP ? -1 : id == B_DOWN ? 1 : 0);
        }
        break;
    case B_CLEAR:
        if (image(&im)) {
            save_undo();
            rect(&im, 0, 0, im.w - 1, im.h - 1, tab == TAB_SPR ? TRANSP : color_r, 1);
        }
        break;
    case B_COPY:
        copy();
        break;
    case B_PASTE:
        paste();
        break;
    case B_ZOOMOUT:
        set_zoom(-1);
        break;
    case B_ZOOMIN:
        set_zoom(1);
        break;
    case B_ANIM:
        anim = !anim;
        anim_step = anim_count = 0;
        dirty = 1;
        break;
    case B_FRMINUS:
    case B_FRPLUS:
        anim_frames += id == B_FRPLUS ? 1 : -1;
        anim_frames = anim_frames < 1 ? 1 : anim_frames > 64 ? 64 : anim_frames;
        dirty = 1;
        break;
    case B_SPMINUS:
    case B_SPPLUS:
        anim_speed += id == B_SPPLUS ? 1 : -1;
        anim_speed = anim_speed < 1 ? 1 : anim_speed > 60 ? 60 : anim_speed;
        dirty = 1;
        break;
    case B_WRAP:
        undo_push_all(slot);
        objs[slot].b ^= 1;
        modified = dirty = 1;
        break;
    case B_OUTMINUS:
    case B_OUTPLUS:
        undo_push_all(slot);
        objs[slot].c = (uint8_t)((objs[slot].c + (id == B_OUTPLUS ? 1 : 15)) & 15);
        modified = dirty = 1;
        break;
    }
    if (id >= B_COLOR && id < B_COLOR + 16) {
        if (right)
            color_r = id - B_COLOR;
        else
            color_l = id - B_COLOR;
        dirty = 1;
    }
}

/* Lo que pasa con el ratón fuera de los botones: pintar, elegir un color de
 * la paleta o un tile del juego. */
void edit_mouse(void)
{
    int px, py;
    img_t im;
    int on = at_pixel(&px, &py);

    /* La paleta (no son botones de texto). */
    if (press && mouse.y >= PAL_ROW * 8 && mouse.y < (PAL_ROW + 4) * 8 && mouse.x >= PCOL * 8 + 4 &&
        mouse.x < PCOL * 8 + 4 + 8 * 16) {
        int c = (mouse.x - PCOL * 8 - 4) / 16 + (mouse.y - PAL_ROW * 8) / 16 * 8;
        edit_click(B_COLOR + c, press & 2);
        return;
    }
    /* El juego de tiles. */
    if (is_tiles() && mouse.x >= CX + 1 && mouse.x < CX + 1 + grid_cols() * tile_side() * 2 &&
        mouse.y >= GRID_Y && mouse.y < GRID_Y + grid_rows() * tile_side() * 2) {
        if (press & 1) {
            int n = (top[tab] + (mouse.y - GRID_Y) / (tile_side() * 2)) * grid_cols() +
                    (mouse.x - CX - 1) / (tile_side() * 2);
            if (n < objs[edit_slot()].h)
                select_obj(n);
        }
        if (mouse.wheel) {
            top[tab] -= mouse.wheel;
            int last = (objs[edit_slot()].h + grid_cols() - 1) / grid_cols() - grid_rows();
            top[tab] = top[tab] > last ? last : top[tab];
            top[tab] = top[tab] < 0 ? 0 : top[tab];
            mouse.wheel = 0;
            dirty = 1;
        }
        return;
    }
    /* La rueda sobre el lienzo: el zoom. */
    if (mouse.wheel && on && !is_tiles()) {
        set_zoom(mouse.wheel > 0 ? 1 : -1);
        mouse.wheel = 0;
    }

    if (!drag && (press & 3) && on && image(&im)) {
        int c = press & 1 ? color_l : color_r;
        drag = press & 1 ? 1 : 2;
        sx = lx = px;
        sy = ly = py;
        switch (tool) {
        case T_PEN:
            save_undo();
            img_set(&im, px, py, c);
            break;
        case T_FILL:
            save_undo();
            flood(&im, px, py, c);
            drag = 0;
            break;
        case T_PICK:
            if (press & 1)
                color_l = img_get(&im, px, py);
            else
                color_r = img_get(&im, px, py);
            drag = 0;
            dirty = 1;
            break;
        }
        dirty = 1;
        return;
    }
    if (drag && image(&im)) {
        int c = drag == 1 ? color_l : color_r;
        int held = mouse.buttons & drag;
        /* Fuera del lienzo, el punto más cercano. */
        int x, y, w, h, z;
        canvas(&x, &y, &w, &h, &z);
        int mx = mouse.x < x + 1 ? x + 1 : mouse.x >= x + 1 + w ? x + w : mouse.x;
        int my = mouse.y < y + 1 ? y + 1 : mouse.y >= y + 1 + h ? y + h : mouse.y;
        px = ox[tab] + (mx - x - 1) / z;
        py = oy[tab] + (my - y - 1) / z;
        px = px >= im.w ? im.w - 1 : px;
        py = py >= im.h ? im.h - 1 : py;
        if (px != lx || py != ly) {
            if (tool == T_PEN)
                line(&im, lx, ly, px, py, c);
            lx = px;
            ly = py;
            dirty = 1;
        }
        if (!held) {
            if (tool == T_LINE || tool == T_RECT || tool == T_FRECT) {
                save_undo();
                if (tool == T_LINE)
                    line(&im, sx, sy, lx, ly, c);
                else
                    rect(&im, sx, sy, lx, ly, c, tool == T_FRECT);
            }
            drag = 0;
            dirty = 1;
        }
    }
}

void edit_key(int k)
{
    int d = zoom[tab] >= 8 ? 1 : 32 / zoom[tab];
    switch (k) {
    case KEY_LEFT: ox[tab] -= d; break;
    case KEY_RIGHT: ox[tab] += d; break;
    case KEY_UP: oy[tab] -= d; break;
    case KEY_DOWN: oy[tab] += d; break;
    case KEY_PGUP: select_obj(cur[tab] - 1); break;
    case KEY_PGDN: select_obj(cur[tab] + 1); break;
    case '+': set_zoom(1); break;
    case '-': set_zoom(-1); break;
    case 'p': case 'P': tool = T_PEN; break;
    case 'l': case 'L': tool = T_LINE; break;
    case 'b': case 'B': tool = T_RECT; break;
    case 'f': case 'F': tool = T_FILL; break;
    case 'k': case 'K': tool = T_PICK; break;
    case 'c': case 'C': copy(); break;
    case 'v': case 'V': paste(); break;
    default: return;
    }
    fix_view();
    dirty = 1;
}

/* Un frame: la animación. */
void edit_tick(void)
{
    if (tab != TAB_SPR || !anim)
        return;
    if (++anim_count >= anim_speed) {
        anim_count = 0;
        anim_step = (anim_step + 1) % anim_frames;
        dirty = 1;
    }
}
