/*
 * map.c - Los mapas de tiles: se pintan con el tile elegido en el panel y
 * sus atributos (espejo horizontal y vertical, el 8 transparente y la
 * prioridad, como en MAP POKE). El botón izquierdo pone el tile y el
 * derecho, el 0.
 */
#include "gfxedit.h"

#define KEY_LEFT    (ZXV_KEY_SPECIAL | 0x50)
#define KEY_RIGHT   (ZXV_KEY_SPECIAL | 0x4F)
#define KEY_UP      (ZXV_KEY_SPECIAL | 0x52)
#define KEY_DOWN    (ZXV_KEY_SPECIAL | 0x51)
#define KEY_PGUP    (ZXV_KEY_SPECIAL | 0x4B)
#define KEY_PGDN    (ZXV_KEY_SPECIAL | 0x4E)

static int cur;                         /* el mapa */
static int vx, vy;                      /* la primera celda que se ve */
static int tool = T_PEN;
static int tile;                        /* el tile con el que se pinta */
static int flags;                       /* sus atributos (ZXV_CELL_...) */
static int grid = 1;
static int pick_top;                    /* la primera fila de tiles del panel */
static int drag, sx, sy, lx, ly, pan_x, pan_y;

/* Los tiles del panel, en celdas de 16 x 16. */
#define PICK_X      (PCOL * 8)
#define PICK_Y      ((PROW + 13) * 8)
#define PICK_COLS   11
#define PICK_ROWS   14

static int slot(void) { return SLOT_MAP + cur; }
static int have(void) { return obj_exists(slot()); }
static int side(void) { return objs[slot()].a ? objs[slot()].a : 8; }
static uint16_t *cells(void) { return (uint16_t *)(void *)obj_data(slot()); }

static void fix_view(void)
{
    obj_t *o = &objs[slot()];
    int mx = o->w - CW / side(), my = o->h - CH / side();
    vx = vx > mx ? mx : vx;
    vy = vy > my ? my : vy;
    vx = vx < 0 ? 0 : vx;
    vy = vy < 0 ? 0 : vy;
}

/* Un tile del juego de lado s en (x, y), a z aumentos, con los atributos de
 * la celda v. */
static void draw_tile(int s, int t, int x, int y, int z, int v)
{
    img_t im;
    if (obj_image(tiles_slot(s), t, &im)) {
        box(x, y, s * z, s * z, 15, 2);
        zxv_gfx_line(x, y, x + s * z - 1, y + s * z - 1, 2);
        return;
    }
    if (v & ZXV_CELL_TRANSP)
        checker(x, y, s * z, s * z);
    zxv_bitmap b = { im.pix, (uint16_t)im.stride, (uint16_t)im.w, (uint16_t)im.h, 0 };
    int f = (v & ZXV_CELL_FLIP_H ? ZXV_FLIP_H : 0) | (v & ZXV_CELL_FLIP_V ? ZXV_FLIP_V : 0) |
            (v & ZXV_CELL_TRANSP ? ZXV_BLIT_TRANSPARENT : 0);
    if (z == 1) {
        zxv_gfx_blit(&b, 0, 0, s, s, x, y, f);
    } else {
        static uint8_t prep[8 + 16 * 2 * 8] __attribute__((aligned(4)));
        zxv_spr_prepare(&b, 0, 0, s, s, prep);
        if (!(v & ZXV_CELL_TRANSP))
            zxv_gfx_rect(x, y, s * z, s * z, TRANSP);
        zxv_spr_draw_scaled(prep, x, y, s * z, s * z, f & 3, NULL);
    }
}

static void draw_canvas(void)
{
    box(CX, CY, CW + 2, CH + 2, C_BG, C_INK);
    if (!have()) {
        text(2, CY / 8 + 2, C_INK, C_BG, TR("Empty: Create", "No existe: Crear"));
        return;
    }
    obj_t *o = &objs[slot()];
    int s = side(), cols = CW / s, rows = CH / s;
    const uint16_t *c = cells();
    zxv_gfx_clip(CX + 1, CY + 1, CX + 1 + CW, CY + 1 + CH);
    for (int j = 0; j < rows && vy + j < o->h; j++)
        for (int i = 0; i < cols && vx + i < o->w; i++) {
            uint16_t v = c[(vy + j) * o->w + vx + i];
            int x = CX + 1 + i * s, y = CY + 1 + j * s;
            draw_tile(s, v & ZXV_CELL_TILE, x, y, 1, v);
            if (grid && v & ZXV_CELL_PRIO)
                zxv_gfx_rect(x + s - 3, y, 3, 3, 10);
        }
    if (grid) {
        int vw = (o->w - vx) * s < CW ? (o->w - vx) * s : CW;
        int vh = (o->h - vy) * s < CH ? (o->h - vy) * s : CH;
        for (int i = s; i < vw; i += s)
            zxv_gfx_line(CX + 1 + i, CY + 1, CX + 1 + i, CY + vh, 13);
        for (int j = s; j < vh; j += s)
            zxv_gfx_line(CX + 1, CY + 1 + j, CX + vw, CY + 1 + j, 13);
    }
    if (drag && tool == T_FRECT) {
        int x0 = sx < lx ? sx : lx, x1 = sx < lx ? lx : sx, y0 = sy < ly ? sy : ly, y1 = sy < ly ? ly : sy;
        int x = CX + 1 + (x0 - vx) * s, y = CY + 1 + (y0 - vy) * s, w = (x1 - x0 + 1) * s, h = (y1 - y0 + 1) * s;
        zxv_gfx_rect(x, y, w, 2, 2);
        zxv_gfx_rect(x, y + h - 2, w, 2, 2);
        zxv_gfx_rect(x, y, 2, h, 2);
        zxv_gfx_rect(x + w - 2, y, 2, h, 2);
    }
    zxv_gfx_clip(0, 0, SW, SH);
}

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
    obj_t *o = &objs[slot()];
    p = cat(buf, TR("Map ", "Mapa "));
    num(p, cur);
    text(PCOL, PROW, C_TITLE, C_BG, buf);
    int col = PCOL + 14;
    col += button(col, PROW, "<", B_PREV, 0) + 1;
    button(col, PROW, ">", B_NEXT, 0);
    if (!have()) {
        button(PCOL, PROW + 3, TR("Create", "Crear"), B_CREATE, 0);
        return;
    }
    stepper(PROW + 1, TR("Width", "Ancho"), o->w, B_WMINUS);
    stepper(PROW + 2, TR("Height", "Alto"), o->h, B_HMINUS);
    col = PCOL;
    col += button(col, PROW + 3, TR("Delete", "Borrar"), B_DELETE, 0) + 1;
    button(col, PROW + 3, side() == 8 ? "Tiles 8" : "Tiles 16", B_TILESIZE, 0);
    col = PCOL;
    col += button(col, PROW + 4, TR("Wraps", "Se repite"), B_WRAP, o->b) + 1;
    button(col, PROW + 4, TR("Grid", "Rejilla"), B_GRID, grid);

    static const int tools[] = { T_PEN, T_FRECT, T_FILL, T_PICK };
    static const char *const names_en[] = { "Pen", "Box", "Flood", "Pick" };
    static const char *const names_es[] = { "Lápiz", "Caja", "Rellenar", "Coger" };
    for (int i = 0; i < 4; i++) {
        if (!(i & 1))
            col = PCOL;
        col += button(col, PROW + 6 + i / 2, es ? names_es[i] : names_en[i], B_TOOL + tools[i], tool == tools[i]) + 1;
    }

    static const char *const flag_en[] = { "Flip H", "Flip V", "Transp", "Front" };
    static const char *const flag_es[] = { "Espejo H", "Espejo V", "Transp", "Delante" };
    for (int i = 0; i < 4; i++) {
        if (!(i & 1))
            col = PCOL;
        col += button(col, PROW + 9 + i / 2, es ? flag_es[i] : flag_en[i], B_FLAG + i, flags >> (12 + i) & 1) + 1;
    }
    p = cat(buf, "Tile ");
    num(p, tile);
    text(PCOL, PROW + 11, C_INK, C_BG, buf);
    draw_tile(side(), tile, (PCOL + 8) * 8, (PROW + 11) * 8, side() == 8 ? 2 : 1, flags);

    /* Los tiles para elegir. */
    int s = side(), z = s == 8 ? 2 : 1, count = objs[tiles_slot(s)].len ? objs[tiles_slot(s)].h : 0;
    box(PICK_X - 1, PICK_Y - 1, PICK_COLS * 16 + 2, PICK_ROWS * 16 + 2, C_BG, C_INK);
    if (!count)
        text(PCOL + 1, PICK_Y / 8 + 1, C_INK, C_BG, TR("No tiles yet", "Aún no hay tiles"));
    for (int r = 0; r < PICK_ROWS; r++)
        for (int c = 0; c < PICK_COLS; c++) {
            int n = (pick_top + r) * PICK_COLS + c;
            if (n >= count)
                break;
            int x = PICK_X + c * 16, y = PICK_Y + r * 16;
            draw_tile(s, n, x, y, z, 0);
            if (n == tile) {
                zxv_gfx_rect(x, y, 16, 1, 2);
                zxv_gfx_rect(x, y + 15, 16, 1, 2);
                zxv_gfx_rect(x, y, 1, 16, 2);
                zxv_gfx_rect(x + 15, y, 1, 16, 2);
            }
        }
}

/* La celda bajo el puntero, o 0 si no está sobre el mapa. */
static int at_cell(int *cx, int *cy)
{
    if (!have() || mouse.x <= CX || mouse.y <= CY || mouse.x > CX + CW || mouse.y > CY + CH)
        return 0;
    *cx = vx + (mouse.x - CX - 1) / side();
    *cy = vy + (mouse.y - CY - 1) / side();
    return *cx < objs[slot()].w && *cy < objs[slot()].h;
}

void map_draw_tab(void)
{
    draw_canvas();
    draw_panel();
    int cx, cy;
    if (at_cell(&cx, &cy)) {
        char buf[64], *p = cat(buf, "x ");
        uint16_t v = cells()[cy * objs[slot()].w + cx];
        p = num(p, cx);
        p = cat(p, "  y ");
        p = num(p, cy);
        p = cat(p, "  tile ");
        p = num(p, v & ZXV_CELL_TILE);
        if (v & ZXV_CELL_FLIP_H)
            p = cat(p, " H");
        if (v & ZXV_CELL_FLIP_V)
            p = cat(p, " V");
        if (v & ZXV_CELL_TRANSP)
            p = cat(p, " T");
        if (v & ZXV_CELL_PRIO)
            cat(p, TR(" F", " D"));
        field(0, 45, 40, C_INK, C_BG, buf);
    }
}

static void set_cell(int x, int y, uint16_t v)
{
    obj_t *o = &objs[slot()];
    if (x >= 0 && y >= 0 && x < o->w && y < o->h)
        cells()[y * o->w + x] = v;
}

static void flood(int x, int y, uint16_t v)
{
    obj_t *o = &objs[slot()];
    uint16_t *c = cells(), from = c[y * o->w + x];
    if (from == v)
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
        if (c[y * o->w + x] != from)
            continue;
        int l = x, r = x;
        while (l > 0 && c[y * o->w + l - 1] == from)
            l--;
        while (r < o->w - 1 && c[y * o->w + r + 1] == from)
            r++;
        for (int i = l; i <= r; i++)
            c[y * o->w + i] = v;
        for (int d = -1; d <= 1; d += 2) {
            int ny = y + d;
            if (ny < 0 || ny >= o->h)
                continue;
            for (int i = l; i <= r; i++)
                if (c[ny * o->w + i] == from && (i == l || c[ny * o->w + i - 1] != from) && n < 1024) {
                    stack[n][0] = (int16_t)i;
                    stack[n][1] = (int16_t)ny;
                    n++;
                }
        }
    }
}

static void resize(int dw, int dh)
{
    obj_t *o = &objs[slot()];
    int w = o->w + dw, h = o->h + dh;
    w = w < 1 ? 1 : w > 32767 ? 32767 : w;
    h = h < 1 ? 1 : h > 32767 ? 32767 : h;
    undo_push_all(slot());
    if (map_size(cur, w, h, side()))
        message(TR("Out of memory", "No queda memoria"));
    fix_view();
    modified = dirty = 1;
}

void map_click(int id, int right)
{
    int step = right ? 10 : 1;
    if (id >= B_TOOL && id < B_TOOL + N_TOOLS) {
        tool = id - B_TOOL;
    } else if (id >= B_FLAG && id < B_FLAG + 4) {
        flags ^= 1 << (12 + id - B_FLAG);
    } else {
        switch (id) {
        case B_PREV:
        case B_NEXT:
            cur = (cur + (id == B_NEXT ? step : -step) + N_MAPS * 10) % N_MAPS;
            vx = vy = 0;
            break;
        case B_CREATE:
            undo_push_all(slot());
            if (map_size(cur, 32, 24, 8))
                message(TR("Out of memory", "No queda memoria"));
            modified = 1;
            break;
        case B_DELETE:
            undo_push_all(slot());
            obj_delete(slot());
            modified = 1;
            break;
        case B_WMINUS: resize(-step, 0); break;
        case B_WPLUS: resize(step, 0); break;
        case B_HMINUS: resize(0, -step); break;
        case B_HPLUS: resize(0, step); break;
        case B_TILESIZE:
            undo_push_all(slot());
            objs[slot()].a = (uint8_t)(side() == 8 ? 16 : 8);
            pick_top = 0;
            fix_view();
            modified = 1;
            break;
        case B_WRAP:
            undo_push_all(slot());
            objs[slot()].b ^= 1;
            modified = 1;
            break;
        case B_GRID:
            grid = !grid;
            break;
        }
    }
    dirty = 1;
}

void map_mouse(void)
{
    int cx, cy;
    int on = at_cell(&cx, &cy);
    int s = side();

    /* Los tiles del panel: elegir y, con la rueda, bajar. */
    if (have() && mouse.x >= PICK_X && mouse.x < PICK_X + PICK_COLS * 16 && mouse.y >= PICK_Y &&
        mouse.y < PICK_Y + PICK_ROWS * 16) {
        int count = objs[tiles_slot(s)].len ? objs[tiles_slot(s)].h : 0;
        if (press & 1) {
            int n = (pick_top + (mouse.y - PICK_Y) / 16) * PICK_COLS + (mouse.x - PICK_X) / 16;
            if (n < count) {
                tile = n;
                dirty = 1;
            }
        }
        if (mouse.wheel) {
            pick_top -= mouse.wheel;
            int last = (count + PICK_COLS - 1) / PICK_COLS - PICK_ROWS;
            pick_top = pick_top > last ? last : pick_top;
            pick_top = pick_top < 0 ? 0 : pick_top;
            mouse.wheel = 0;
            dirty = 1;
        }
        return;
    }
    if (!have())
        return;
    if (mouse.wheel && on) {
        vy -= mouse.wheel * 2;
        mouse.wheel = 0;
        fix_view();
        dirty = 1;
    }
    /* El botón central arrastra el mapa. */
    if (press & 4) {
        pan_x = mouse.x;
        pan_y = mouse.y;
    }
    if (mouse.buttons & 4) {
        int dx = (pan_x - mouse.x) / s, dy = (pan_y - mouse.y) / s;
        if (dx || dy) {
            vx += dx;
            vy += dy;
            pan_x -= dx * s;
            pan_y -= dy * s;
            fix_view();
            dirty = 1;
        }
        return;
    }

    if (!drag && (press & 3) && on) {
        uint16_t v = press & 1 ? (uint16_t)(tile | flags) : 0;
        drag = press & 1 ? 1 : 2;
        sx = lx = cx;
        sy = ly = cy;
        switch (tool) {
        case T_PEN:
            undo_push_all(slot());
            set_cell(cx, cy, v);
            modified = 1;
            break;
        case T_FILL:
            undo_push_all(slot());
            flood(cx, cy, v);
            modified = 1;
            drag = 0;
            break;
        case T_PICK: {
            uint16_t c = cells()[cy * objs[slot()].w + cx];
            tile = c & ZXV_CELL_TILE;
            flags = c & ~ZXV_CELL_TILE;
            drag = 0;
            break;
        }
        }
        dirty = 1;
        return;
    }
    if (drag) {
        uint16_t v = drag == 1 ? (uint16_t)(tile | flags) : 0;
        obj_t *o = &objs[slot()];
        int mx = (mouse.x - CX - 1) / s, my = (mouse.y - CY - 1) / s;
        cx = vx + (mx < 0 ? 0 : mx);
        cy = vy + (my < 0 ? 0 : my);
        cx = cx >= o->w ? o->w - 1 : cx;
        cy = cy >= o->h ? o->h - 1 : cy;
        if (cx != lx || cy != ly) {
            if (tool == T_PEN) {
                /* Todas las celdas entre la anterior y esta. */
                int n = (cx > lx ? cx - lx : lx - cx) > (cy > ly ? cy - ly : ly - cy)
                            ? (cx > lx ? cx - lx : lx - cx) : (cy > ly ? cy - ly : ly - cy);
                for (int i = 1; i <= n; i++)
                    set_cell(lx + (cx - lx) * i / n, ly + (cy - ly) * i / n, v);
            }
            lx = cx;
            ly = cy;
            dirty = 1;
        }
        if (!(mouse.buttons & drag)) {
            if (tool == T_FRECT) {
                undo_push_all(slot());
                for (int y = sy < ly ? sy : ly; y <= (sy < ly ? ly : sy); y++)
                    for (int x = sx < lx ? sx : lx; x <= (sx < lx ? lx : sx); x++)
                        set_cell(x, y, v);
                modified = 1;
            }
            drag = 0;
            dirty = 1;
        }
    }
}

void map_key(int k)
{
    int d = 640 / side() / 8;
    switch (k) {
    case KEY_LEFT: vx -= d; break;
    case KEY_RIGHT: vx += d; break;
    case KEY_UP: vy -= d; break;
    case KEY_DOWN: vy += d; break;
    case KEY_PGUP: map_click(B_PREV, 0); break;
    case KEY_PGDN: map_click(B_NEXT, 0); break;
    case 'g': case 'G': grid = !grid; break;
    default: return;
    }
    if (have())
        fix_view();
    dirty = 1;
}
