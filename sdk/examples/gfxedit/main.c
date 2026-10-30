/*
 * main.c - El editor de gráficos de ZX-V: sprites, tiles, mapas y texturas
 * para el BASIC (LOAD "x" GRAPHICS) y para los programas nativos, en
 * ficheros .gfx.
 *
 *   Tab             la pestaña siguiente
 *   U o Ctrl+Z      deshacer
 *   RePág, AvPág    el objeto anterior y el siguiente
 *   flechas         mover la vista
 *   + -             el zoom
 *   P L B F K       lápiz, línea, caja, rellenar, coger un color
 *   C V             copiar y pegar
 *   Esc             salir
 */
#include "gfxedit.h"

#define KEY_ESC     0x1B
#define KEY_TAB     0x09
#define KEY_CTRL_Z  0x1A

int es, tab, dirty = 1, modified, color_l = 0, color_r = TRANSP;
zxv_mouse_t mouse;
int press, release;

static char file_name[64];
static int file;                        /* el fichero que se lee o se escribe */

static int file_io(void *buf, uint32_t n) { return zxv_file_read(file, buf, n); }
static int file_out(void *buf, uint32_t n) { return zxv_file_write(file, buf, n); }

/* --- Ficheros ------------------------------------------------------------------ */

static int lose_changes(void)
{
    return !modified || ask(TR("Discard the changes?", "¿Tirar los cambios?"));
}

static void load(void)
{
    char name[64];
    if (!lose_changes() || !choose_file(name, sizeof name))
        return;
    file = zxv_file_open(name, ZXV_O_READ);
    if (file < 0) {
        message(TR("Cannot open it", "No se puede abrir"));
        return;
    }
    store_clear();
    int r = gfx_load(file_io);
    zxv_file_close(file);
    cat(file_name, name);
    modified = 0;
    if (r == GFX_BAD)
        message(TR("Not a graphics file, or damaged", "No es de gráficos o está dañado"));
    else if (r == GFX_FULL)
        message(TR("Too big: only part of it fits", "Demasiado grande: solo cabe una parte"));
    else
        message(TR("Loaded", "Cargado"));
}

static void save(void)
{
    char name[64];
    cat(name, file_name[0] ? file_name : "graficos.gfx");
    if (!input(TR("Save as (Enter saves, Esc cancels)", "Grabar como (Enter graba, Esc vuelve)"), name, 60))
        return;
    /* Sin extensión, .gfx. */
    int n = 0, dot = 0;
    while (name[n])
        if (name[n++] == '.')
            dot = 1;
    if (!dot && n < 59)
        cat(name + n, ".gfx");
    zxv_dirent_t e;
    int same = 1;
    for (int i = 0; name[i] || file_name[i]; i++)
        if ((name[i] | 0x20) != (file_name[i] | 0x20))
            same = 0;
    if (!same && zxv_file_stat(name, &e) >= 0 && !ask(TR("It exists: overwrite it?", "Ya existe: ¿lo cambio?")))
        return;
    file = zxv_file_open(name, ZXV_O_WRITE | ZXV_O_CREATE | ZXV_O_TRUNC);
    int r = file < 0 ? -1 : gfx_save(file_out, NULL);
    if (file >= 0)
        zxv_file_close(file);
    if (r) {
        message(TR("Could not save it", "No se ha podido grabar"));
        return;
    }
    cat(file_name, name);
    modified = 0;
    message(TR("Saved", "Grabado"));
}

/* --- La pantalla ---------------------------------------------------------------- */

void draw_all(void)
{
    static const char *const tabs_en[] = { "Sprites", "Tiles 8", "Tiles 16", "Maps", "Textures" };
    static const char *const tabs_es[] = { "Sprites", "Tiles 8", "Tiles 16", "Mapas", "Texturas" };
    char buf[64], *p;
    int col = 0;
    for (int t = 0; t < N_TABS; t++)
        col += button(col, 0, es ? tabs_es[t] : tabs_en[t], B_TAB + t, t == tab) + 1;

    p = cat(buf, file_name[0] ? file_name : TR("(new)", "(nuevo)"));
    if (modified)
        cat(p, " *");
    field(0, 1, 29, C_TITLE, C_BG, buf);
    col = 30;
    col += button(col, 1, TR("New", "Nuevo"), B_NEW, 0) + 1;
    col += button(col, 1, TR("Load", "Cargar"), B_LOAD, 0) + 1;
    col += button(col, 1, TR("Save", "Grabar"), B_SAVE, 0) + 1;
    button(col, 1, TR("Exit", "Salir"), B_QUIT, 0);

    if (tab == TAB_MAP)
        map_draw_tab();
    else
        edit_draw();

    button(41, 45, TR("Undo", "Deshacer"), B_UNDO, 0);
    p = cat(buf, TR("Free ", "Libre "));
    p = num(p, (int)(pool_free() / 1024));
    cat(p, "K");
    text(52, 45, C_INK, C_BG, buf);
    field(0, 46, 63, 2, C_BG, message_text());
    /* Sin llegar a la última columna de la última fila: si no, sube todo. */
    field(0, 47, 63, 1, C_BG, TR("Tab: tab  U: undo  PgUp/PgDn: object  +/-: zoom  Esc: exit",
                                 "Tab: pestaña  U: deshacer  RePág/AvPág: objeto  Esc: salir"));
}

static void undo(void)
{
    int s = undo_pop();
    if (s < 0) {
        message(TR("Nothing to undo", "No hay nada que deshacer"));
        return;
    }
    modified = dirty = 1;
}

/* Un clic en un botón. */
static void click(int id, int right)
{
    message("");
    if (id >= B_TAB && id < B_TAB + N_TABS) {
        tab = id - B_TAB;
        dirty = 1;
        return;
    }
    switch (id) {
    case B_NEW:
        if (lose_changes()) {
            store_clear();
            file_name[0] = 0;
            modified = 0;
        }
        dirty = 1;
        return;
    case B_LOAD:
        load();
        return;
    case B_SAVE:
        save();
        return;
    case B_UNDO:
        undo();
        return;
    }
    if (tab == TAB_MAP)
        map_click(id, right);
    else
        edit_click(id, right);
}

int main(void)
{
    es = zxv_lang_get() == ZXV_LANG_ES;
    zxv_text_colors(C_INK, C_BG);
    zxv_set_mode(1);
    zxv_mouse_init(&mouse, SW / 2, SH / 2);
    store_clear();
    /* Cargado desde ROM:, se graba en la tarjeta. */
    char here[64];
    if (zxv_dir_current(here, sizeof here) >= 0 && (here[0] | 0x20) == 'r')
        zxv_dir_change("SD:");
    int quit = 0, lastx = -1, lasty = -1;
    while (!quit) {
        wait_frame();
        pointer_hide();
        int old = mouse.buttons;
        zxv_mouse_update(&mouse, SW, SH);
        press = mouse.buttons & ~old;
        release = old & ~mouse.buttons;

        int b = press & 3 ? button_at(mouse.x, mouse.y) : -1;
        if (b >= 0) {
            if (b == B_QUIT)
                quit = lose_changes();
            else
                click(b, press & 2);
        } else if (tab == TAB_MAP) {
            map_mouse();
        } else {
            edit_mouse();
        }
        int k = zxv_key_get();
        if (k == KEY_ESC) {
            quit = lose_changes();
        } else if (k == KEY_TAB) {
            tab = (tab + 1) % N_TABS;
            dirty = 1;
        } else if (k == 'u' || k == 'U' || k == KEY_CTRL_Z) {
            undo();
        } else if (k >= 0) {
            if (tab == TAB_MAP)
                map_key(k);
            else
                edit_key(k);
        }
        edit_tick();
        /* Las coordenadas del estado siguen al puntero. */
        if (mouse.x / 8 != lastx || mouse.y / 8 != lasty) {
            lastx = mouse.x / 8;
            lasty = mouse.y / 8;
            if (mouse.x < CX + CW + 2 && mouse.y < CY + CH + 2)
                dirty = 1;
        }
        if (dirty) {
            page_begin();
            draw_all();
            page_end();
            dirty = 0;
        }
        pointer_show();
    }
    pointer_hide();
    zxv_set_page(0);
    zxv_gfx_draw_page(0);
    zxv_set_mode(0);
    return 0;
}
