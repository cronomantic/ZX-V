/*
 * ui.c - La pantalla del editor: texto, botones, el puntero del ratón, el
 * dibujo de los bitmaps ampliados y los diálogos.
 *
 * Se dibuja todo en la página oculta y luego se enseña (sin parpadeos). El
 * puntero va encima, en la página visible, guardando lo que tapa.
 */
#include "gfxedit.h"

#define REG8(a)     (*(volatile uint8_t *)(uintptr_t)(a))
#define KEY_ESC     0x1B
#define KEY_ENTER   0x0D
#define KEY_BS      0x08
#define KEY_UP      (ZXV_KEY_SPECIAL | 0x52)
#define KEY_DOWN    (ZXV_KEY_SPECIAL | 0x51)
#define KEY_PGUP    (ZXV_KEY_SPECIAL | 0x4B)
#define KEY_PGDN    (ZXV_KEY_SPECIAL | 0x4E)

/* --- Texto ------------------------------------------------------------------- */

char *num(char *p, int v)
{
    char t[12];
    int n = 0;
    if (v < 0) {
        *p++ = '-';
        v = -v;
    }
    do {
        t[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n)
        *p++ = t[--n];
    *p = 0;
    return p;
}

char *cat(char *p, const char *s)
{
    while (*s)
        *p++ = *s++;
    *p = 0;
    return p;
}

void text(int col, int row, int ink, int paper, const char *s)
{
    zxv_text_colors(ink, paper);
    zxv_text_at(col, row);
    zxv_print_str(s);
}

/* Los caracteres de una cadena UTF-8 (las tildes ocupan dos bytes). */
static int width(const char *s)
{
    int n = 0;
    for (; *s; s++)
        if ((*s & 0xC0) != 0x80)
            n++;
    return n;
}

void field(int col, int row, int w, int ink, int paper, const char *s)
{
    char buf[80], *p = buf;
    int n = 0;
    for (; *s && n < w && p < buf + 76; s++) {
        *p++ = *s;
        if ((s[1] & 0xC0) != 0x80)
            n++;
    }
    for (; n < w && p < buf + 79; n++)
        *p++ = ' ';
    *p = 0;
    text(col, row, ink, paper, buf);
}

/* --- Botones ----------------------------------------------------------------- */

#define MAX_BUTTONS 96

static struct {
    uint8_t col, row, w;
    int16_t id;
} buttons[MAX_BUTTONS];
static int nbuttons;

void buttons_clear(void) { nbuttons = 0; }

int button(int col, int row, const char *label, int id, int on)
{
    char buf[40], *p = cat(buf, " ");
    p = cat(p, label);
    cat(p, " ");
    int w = width(buf);
    text(col, row, on ? 7 : C_INK, on ? C_ON : C_BTN, buf);
    if (nbuttons < MAX_BUTTONS && id >= 0) {
        buttons[nbuttons].col = (uint8_t)col;
        buttons[nbuttons].row = (uint8_t)row;
        buttons[nbuttons].w = (uint8_t)w;
        buttons[nbuttons].id = (int16_t)id;
        nbuttons++;
    }
    return w;
}

int button_at(int x, int y)
{
    int col = x / 8, row = y / 8;
    for (int i = 0; i < nbuttons; i++)
        if (row == buttons[i].row && col >= buttons[i].col && col < buttons[i].col + buttons[i].w)
            return buttons[i].id;
    return -1;
}

static char msg[64];

void message(const char *s)
{
    cat(msg, s);
    dirty = 1;
}

const char *message_text(void) { return msg; }

/* --- Bitmaps ------------------------------------------------------------------ */

void box(int x, int y, int w, int h, int fill, int border)
{
    if (border >= 0) {
        zxv_gfx_rect(x, y, w, h, border);
        x++, y++, w -= 2, h -= 2;
    }
    if (w > 0 && h > 0)
        zxv_gfx_rect(x, y, w, h, fill);
}

void checker(int x, int y, int w, int h)
{
    zxv_gfx_rect(x, y, w, h, 7);
    for (int j = 0; j < h; j += 4)
        for (int i = (j / 4 & 1) * 4; i < w; i += 8)
            zxv_gfx_rect(x + i, y + j, i + 4 <= w ? 4 : w - i, j + 4 <= h ? 4 : h - j, 15);
}

/* Lo preparado para el motor (lo visible del lienzo, a 2x como mucho:
 * 8 + 160 · 20 · 8 bytes); fuera del dibujo, para lo que haga falta. */
uint8_t work[WORK_SIZE] __attribute__((aligned(4)));
#define prep work

void draw_image(const img_t *im, int x, int y, int w, int h, int z, int ox, int oy, int transp)
{
    int vw = (w + z - 1) / z, vh = (h + z - 1) / z;
    if (vw > im->w - ox)
        vw = im->w - ox;
    if (vh > im->h - oy)
        vh = im->h - oy;
    if (vw <= 0 || vh <= 0)
        return;
    if (transp)
        checker(x, y, vw * z < w ? vw * z : w, vh * z < h ? vh * z : h);
    zxv_bitmap b = { im->pix, (uint16_t)im->stride, (uint16_t)im->w, (uint16_t)im->h, 0 };
    zxv_gfx_clip(x, y, x + w, y + h);
    if (z == 1) {
        zxv_gfx_blit(&b, ox, oy, vw, vh, x, y, transp ? ZXV_BLIT_TRANSPARENT : 0);
    } else {
        /* Por tiras, para que quepa en prep. */
        int rows = (int)((sizeof prep - 8) / (((vw + 7) / 8) * 8u));
        for (int j = 0; j < vh; j += rows) {
            int n = vh - j < rows ? vh - j : rows;
            zxv_spr_prepare(&b, ox, oy + j, vw, n, prep);
            if (!transp) {
                /* Sin transparencia: el 8 se dibuja como un color más. */
                zxv_gfx_rect(x, y + j * z, vw * z, n * z, TRANSP);
            }
            zxv_spr_draw_scaled(prep, x, y + j * z, vw * z, n * z, 0, NULL);
        }
    }
    zxv_gfx_clip(0, 0, SW, SH);
}

/* --- Las páginas y el puntero ------------------------------------------------- */

static int visible;

/* Espera al siguiente frame y a que la página visible se haya mandado: así
 * el puntero se puede mover en ella. */
void wait_frame(void)
{
    zxv_wait_vblank();
    while (REG8(ZXV_SV_SYSFLAGS) & ZXV_SYSF_LINK_BUSY)
        ;
}

void page_begin(void)
{
    zxv_gfx_draw_page(visible ^ 1);
    zxv_gfx_target(NULL);
    zxv_gfx_clip(0, 0, SW, SH);
    zxv_gfx_rect(0, 0, SW, SH, C_BG);
    buttons_clear();
}

void page_end(void)
{
    visible ^= 1;
    zxv_set_page(visible);
    zxv_gfx_draw_page(visible ^ 1);
}

static const char *const arrow[11] = {
    "B.......", "BB......", "BWB.....", "BWWB....", "BWWWB...", "BWWWWB..",
    "BWWWWWB.", "BWWBBBB.", "BWB.....", "BB......", "B.......",
};

static uint8_t under[11][5];
static int under_x = -1, under_y, under_page;

static volatile uint8_t *vram(int page)
{
    return (volatile uint8_t *)(uintptr_t)(page ? ZXV_VRAM1_BASE : ZXV_VRAM0_BASE);
}

void pointer_hide(void)
{
    if (under_x < 0)
        return;
    volatile uint8_t *v = vram(under_page);
    for (int j = 0; j < 11 && under_y + j < SH; j++)
        for (int i = 0; i < 5 && under_x / 2 + i < SW / 2; i++)
            v[(under_y + j) * (SW / 2) + under_x / 2 + i] = under[j][i];
    under_x = -1;
}

void pointer_show(void)
{
    int x = mouse.x, y = mouse.y;
    volatile uint8_t *v = vram(visible);
    under_x = x & ~1;
    under_y = y;
    under_page = visible;
    for (int j = 0; j < 11 && y + j < SH; j++)
        for (int i = 0; i < 5 && under_x / 2 + i < SW / 2; i++)
            under[j][i] = v[(y + j) * (SW / 2) + under_x / 2 + i];
    for (int j = 0; j < 11 && y + j < SH; j++)
        for (int i = 0; i < 8 && x + i < SW; i++) {
            char c = arrow[j][i];
            if (c == '.')
                continue;
            volatile uint8_t *p = &v[(y + j) * (SW / 2) + (x + i) / 2];
            int col = c == 'B' ? 0 : 15;
            *p = (x + i) & 1 ? (uint8_t)((*p & 0x0F) | col << 4) : (uint8_t)((*p & 0xF0) | col);
        }
}

/* --- Diálogos ----------------------------------------------------------------- */

void draw_all(void);                    /* main.c */

/* Un recuadro en el centro con unas líneas de texto. */
static void panel(int rows, const char *title)
{
    int y = (SH - rows * 8 - 24) / 2 / 8 * 8;
    box(64, y, SW - 128, rows * 8 + 24, C_BG, C_INK);
    field(9, y / 8 + 1, 46, 7, C_TITLE, title);
}

/* Espera un frame: el puntero, el ratón y una tecla (o -1). */
static int frame(void)
{
    wait_frame();
    pointer_hide();
    int old = mouse.buttons;
    zxv_mouse_update(&mouse, SW, SH);
    press = mouse.buttons & ~old;
    release = old & ~mouse.buttons;
    pointer_show();
    return zxv_key_get();
}

int ask(const char *question)
{
    int answer = -1;
    while (answer < 0) {
        page_begin();
        draw_all();
        panel(3, question);
        int row = (SH - 48) / 2 / 8 + 3;
        buttons_clear();
        int col = 20;
        col += button(col, row, TR("Yes", "Sí"), 1, 0) + 2;
        button(col, row, "No", 0, 0);
        page_end();
        for (;;) {
            int k = frame();
            if (k == KEY_ENTER || k == 's' || k == 'S' || k == 'y' || k == 'Y')
                answer = 1;
            else if (k == KEY_ESC || k == 'n' || k == 'N')
                answer = 0;
            else if (press & 1)
                answer = button_at(mouse.x, mouse.y);
            if (answer >= 0)
                break;
        }
    }
    pointer_hide();
    dirty = 1;
    return answer;
}

int input(const char *prompt, char *buf, int max)
{
    int n = 0;
    while (buf[n])
        n++;
    for (;;) {
        page_begin();
        draw_all();
        panel(3, prompt);
        int row = (SH - 48) / 2 / 8 + 3;
        char line[64], *p = cat(line, buf);
        cat(p, "_");
        field(10, row, 44, C_INK, 15, line);
        page_end();
        int k;
        while ((k = frame()) < 0 && !(press & 1))
            ;
        if (k == KEY_ENTER && n) {
            pointer_hide();
            dirty = 1;
            return 1;
        }
        if (k == KEY_ESC || (press & 1)) {
            pointer_hide();
            dirty = 1;
            return 0;
        }
        if (k == KEY_BS && n)
            buf[--n] = 0;
        else if (k > 32 && k < 127 && n < max - 1 && k != '\\' && k != '*' && k != '?' && k != '"' &&
                 k != '<' && k != '>' && k != '|') {
            buf[n++] = (char)k;
            buf[n] = 0;
        }
    }
}

/* Los nombres de la carpeta: las carpetas y los .gfx. */
#define MAX_FILES   64
static zxv_dirent_t files[MAX_FILES];
static int nfiles;

static int is_gfx(const char *s)
{
    int n = 0;
    while (s[n])
        n++;
    return n > 4 && s[n - 4] == '.' && (s[n - 3] | 0x20) == 'g' && (s[n - 2] | 0x20) == 'f' &&
           (s[n - 1] | 0x20) == 'x';
}

static void read_dir(void)
{
    char cur[64];
    nfiles = 0;
    if (zxv_dir_current(cur, sizeof cur) >= 0 && !(cur[0] == '/' && cur[1] == 0)) {
        cat(files[0].name, "..");
        files[0].attr = ZXV_ATTR_DIR;
        nfiles = 1;
    }
    int d = zxv_dir_open("");
    if (d < 0)
        return;
    while (nfiles < MAX_FILES && zxv_dir_read(d, &files[nfiles]) > 0)
        if ((files[nfiles].attr & ZXV_ATTR_DIR) || is_gfx(files[nfiles].name))
            nfiles++;
    zxv_dir_close(d);
}

int choose_file(char *name, int max)
{
    enum { ROWS = 20 };
    int sel = 0, top = 0;
    read_dir();
    for (;;) {
        page_begin();
        draw_all();
        panel(ROWS + 1, TR("Load: Enter opens, Esc cancels", "Cargar: Enter abre, Esc vuelve"));
        int row0 = (SH - (ROWS + 1) * 8 - 24) / 2 / 8 + 3;
        buttons_clear();
        for (int i = 0; i < ROWS; i++) {
            int n = top + i;
            char line[72] = "";
            if (n < nfiles) {
                char *p = cat(line, files[n].attr & ZXV_ATTR_DIR ? "/" : " ");
                cat(p, files[n].name);
            }
            field(10, row0 + i, 44, n == sel ? 7 : C_INK, n == sel ? C_ON : C_BG, line);
        }
        if (!nfiles)
            text(10, row0, C_INK, C_BG, TR("(no .gfx files here)", "(aquí no hay ficheros .gfx)"));
        page_end();
        int k, open = 0;
        for (;;) {
            k = frame();
            if (press & 1) {
                int r = mouse.y / 8 - row0;
                if (r >= 0 && r < ROWS && top + r < nfiles) {
                    open = sel == top + r;      /* el segundo clic abre */
                    sel = top + r;
                    break;
                }
                if (r < -2 || r > ROWS) {
                    k = KEY_ESC;
                    break;
                }
            }
            if (mouse.wheel) {
                top -= mouse.wheel * 3;
                mouse.wheel = 0;
                break;
            }
            if (k >= 0)
                break;
        }
        if (k == KEY_ESC) {
            pointer_hide();
            dirty = 1;
            return 0;
        }
        if (k == KEY_UP && sel > 0)
            sel--;
        if (k == KEY_DOWN && sel < nfiles - 1)
            sel++;
        if (k == KEY_PGUP)
            sel = sel > ROWS ? sel - ROWS : 0;
        if (k == KEY_PGDN)
            sel = sel + ROWS < nfiles ? sel + ROWS : nfiles - 1;
        if ((k == KEY_ENTER || open) && nfiles) {
            if (files[sel].attr & ZXV_ATTR_DIR) {
                zxv_dir_change(files[sel].name);
                read_dir();
                sel = top = 0;
                continue;
            }
            int i = 0;
            for (; files[sel].name[i] && i < max - 1; i++)
                name[i] = files[sel].name[i];
            name[i] = 0;
            pointer_hide();
            dirty = 1;
            return 1;
        }
        if (sel < top)
            top = sel;
        if (sel >= top + ROWS)
            top = sel - ROWS + 1;
        if (top > nfiles - ROWS)
            top = nfiles - ROWS;
        if (top < 0)
            top = 0;
    }
}
