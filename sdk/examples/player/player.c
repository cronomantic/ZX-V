/*
 * player.c - Reproductor de música y efectos del AY con sdk/music: recorre
 * las carpetas de la tarjeta y toca los módulos (PT3, TurboSound, PT2, STC,
 * ASC y PSG) y los efectos AYFX (.afb, .afx), con los registros de los dos AY a la vista.
 *
 *   flechas, RePág, AvPág   elegir
 *   Enter                   abrir la carpeta, tocar el módulo, cargar el
 *                           banco de efectos o tocar el efecto
 *   Espacio                 pausa
 *   S                       parar
 *   L                       repetir al acabar (sí / no)
 *   2                       un módulo de un AY, en el primero o el segundo
 *   1-9, 0                  con un banco cargado, sus efectos (de diez en
 *   , .                     diez: la decena anterior y la siguiente)
 *   Esc                     volver al BASIC
 */
#include "zxv_api.h"
#include "zxv_music.h"

#define REG8(a)     (*(volatile uint8_t *)(uintptr_t)(a))

#define KEY_ESC     0x1B
#define KEY_ENTER   0x0D
#define KEY_UP      (ZXV_KEY_SPECIAL | 0x52)
#define KEY_DOWN    (ZXV_KEY_SPECIAL | 0x51)
#define KEY_PGUP    (ZXV_KEY_SPECIAL | 0x4B)
#define KEY_PGDN    (ZXV_KEY_SPECIAL | 0x4E)

#define COLS        64
#define LIST_ROW    2
#define LIST_ROWS   26
#define MAX_FILES   256

static int es;
#define TR(en, sp)  (es ? (sp) : (en))

static zxv_dirent_t files[MAX_FILES];
static int nfiles, sel, top;

static uint8_t song[0x20000];           /* hasta dos módulos de 64 KB */
static uint8_t bank[0x8000];
static uint8_t afx[0x2000];
static int song_size, bank_n, bank_page;
static char song_name[64], bank_name[64];
static zxv_music_info_t info;
static int loop = 1, on_ay2, paused;

/* --- Texto ------------------------------------------------------------------- */

static void at(int col, int row, int ink, int paper, const char *s)
{
    zxv_text_colors(ink, paper);
    zxv_text_at(col, row);
    zxv_print_str(s);
}

/* El texto s en una línea de w columnas, rellena con espacios. */
static void field(int col, int row, int w, int ink, int paper, const char *s)
{
    char buf[COLS + 1];
    int i = 0;
    for (; i < w && s[i]; i++)
        buf[i] = s[i];
    for (; i < w; i++)
        buf[i] = ' ';
    buf[w] = 0;
    at(col, row, ink, paper, buf);
}

static char *num(char *p, unsigned v, int digits)
{
    char t[10];
    int n = 0;
    do {
        t[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v || n < digits);
    while (n)
        *p++ = t[--n];
    *p = 0;
    return p;
}

static char *cat(char *p, const char *s)
{
    while (*s)
        *p++ = *s++;
    *p = 0;
    return p;
}

/* --- Ficheros -------------------------------------------------------------- */

static int ext(const char *name, const char *e)
{
    const char *dot = 0;
    for (const char *p = name; *p; p++)
        if (*p == '.')
            dot = p;
    if (!dot)
        return 0;
    for (dot++; *dot && *e; dot++, e++)
        if ((*dot | 0x20) != *e)
            return 0;
    return !*dot && !*e;
}

static int wanted(const zxv_dirent_t *e)
{
    static const char *const exts[] = { "pt3", "pt2", "stc", "asc", "psg", "afb", "afx" };
    if (e->attr & ZXV_ATTR_DIR)
        return 1;
    for (unsigned i = 0; i < sizeof exts / sizeof *exts; i++)
        if (ext(e->name, exts[i]))
            return 1;
    return 0;
}

static void read_dir(void)
{
    char cur[64];
    nfiles = sel = top = 0;
    if (zxv_dir_current(cur, sizeof cur) >= 0 && !(cur[0] == '/' && cur[1] == 0)) {
        cat(files[0].name, "..");
        files[0].attr = ZXV_ATTR_DIR;
        nfiles = 1;
    }
    int d = zxv_dir_open("");
    if (d < 0)
        return;
    while (nfiles < MAX_FILES && zxv_dir_read(d, &files[nfiles]) > 0)
        if (wanted(&files[nfiles]))
            nfiles++;
    zxv_dir_close(d);
}

/* --- Pantalla ---------------------------------------------------------------- */

static void draw_list(void)
{
    for (int i = 0; i < LIST_ROWS; i++) {
        int n = top + i;
        char buf[COLS];
        buf[0] = 0;
        if (n < nfiles) {
            char *p = cat(buf, files[n].attr & ZXV_ATTR_DIR ? "/" : " ");
            cat(p, files[n].name);
        }
        int on = n == sel && n < nfiles;
        field(1, LIST_ROW + i, 46, on ? 7 : 0, on ? 1 : 7, buf);
    }
}

static void draw_info(void)
{
    char buf[COLS], *p;
    static const char *const fmt[] = { "", "PT3", "TurboSound (PT3 x 2)", "PSG", "PT2", "STC", "ASC" };
    field(1, 30, 62, 1, 7, song_size ? song_name : TR("(no module)", "(ningún módulo)"));
    field(1, 31, 62, 0, 7, song_size ? info.title : "");
    field(1, 32, 62, 0, 7, song_size ? info.author : "");
    p = cat(buf, song_size ? fmt[info.format] : "");
    if (song_size && info.chips == 1)
        p = cat(p, TR(on_ay2 ? ", second AY" : ", first AY", on_ay2 ? ", segundo AY" : ", primer AY"));
    field(1, 33, 40, 0, 7, buf);
    p = cat(buf, TR("Repeat: ", "Repetir: "));
    cat(p, loop ? TR("yes", "sí") : "no");
    field(44, 33, 19, 0, 7, buf);

    p = cat(buf, TR("Effects: ", "Efectos: "));
    if (bank_n) {
        p = cat(p, bank_name);
        p = cat(p, " (");
        p = num(p, bank_n, 1);
        p = cat(p, TR("), keys 1-0 play ", "), las teclas 1-0 tocan "));
        p = num(p, bank_page * 10 + 1, 1);
        p = cat(p, "-");
        num(p, bank_page * 10 + 10, 1);
    } else {
        cat(p, TR("(no bank)", "(ningún banco)"));
    }
    field(1, 35, 62, 0, 7, buf);
}

/* El estado: el tiempo y los registros que suenan en cada AY. */
static void draw_status(void)
{
    char buf[COLS], *p;
    uint32_t s = zxv_music_steps() / 50;
    p = cat(buf, !zxv_music_playing() ? TR("Stopped ", "Parado  ")
                 : paused ? TR("Paused  ", "Pausa   ") : TR("Playing ", "Sonando "));
    p = num(p, s / 60, 2);
    p = cat(p, ":");
    num(p, s % 60, 2);
    field(48, 2, 15, zxv_music_playing() ? 4 : 2, 7, buf);

    for (int a = 0; a < 2; a++) {
        uint32_t base = a ? ZXV_S_AY2_REGS : ZXV_S_AY_REGS;
        uint8_t mix = REG8(base + 7);
        for (int c = 0; c < 3; c++) {
            int row = 38 + a * 4 + c;
            uint8_t v = REG8(base + 8 + c);
            int vol = v & 16 ? 15 : v & 15;
            uint16_t tone = REG8(base + c * 2) | (REG8(base + c * 2 + 1) & 15) << 8;
            p = cat(buf, a ? "AY2 " : "AY1 ");
            *p++ = (char)('A' + c);
            *p++ = ' ';
            *p++ = mix >> c & 1 ? '-' : 'T';
            *p++ = mix >> (c + 3) & 1 ? '-' : 'N';
            *p++ = v & 16 ? 'E' : '-';
            *p++ = ' ';
            p = num(p, tone, 4);
            *p = 0;
            field(1, row, 14, 0, 7, buf);
            zxv_text_colors(0, a ? 3 : 5);
            zxv_text_at(16, row);
            for (int i = 0; i < 15; i++) {
                if (i == vol)
                    zxv_text_colors(0, 7);
                zxv_print_char(' ');
            }
        }
    }
}

static void draw_all(void)
{
    zxv_text_colors(0, 7);
    zxv_set_mode(1);
    at(1, 0, 7, 1, TR(" ZX-V player: PT3, TurboSound, PT2, STC, ASC, PSG and AYFX ",
                      " Reproductor de ZX-V: PT3, TurboSound, PT2, STC, ASC, PSG y AYFX "));
    at(1, 28, 1, 7, TR("Enter play  Space pause  S stop  L repeat  2 AY  Esc exit",
                       "Enter tocar  Espacio pausa  S parar  L repetir  2 AY  Esc salir"));
    draw_list();
    draw_info();
}

/* --- Acciones ---------------------------------------------------------------- */

static void play_song(void)
{
    paused = 0;
    if (song_size)
        zxv_music_play(song, song_size, (loop ? ZXV_MUSIC_LOOP : 0) | (on_ay2 ? ZXV_MUSIC_AY2 : 0));
}

static void open_file(void)
{
    zxv_dirent_t *e = &files[sel];
    if (e->attr & ZXV_ATTR_DIR) {
        zxv_dir_change(e->name);
        read_dir();
        draw_list();
        return;
    }
    if (ext(e->name, "afb")) {
        int n = zxv_file_load(e->name, bank, sizeof bank);
        bank_n = n > 0 ? zxv_sfx_bank(bank, n) : 0;
        bank_page = 0;
        cat(bank_name, e->name);
    } else if (ext(e->name, "afx")) {
        int n = zxv_file_load(e->name, afx, sizeof afx);
        if (n > 0)
            zxv_sfx_play_data(afx, n, ZXV_SFX_AUTO);
    } else {
        zxv_music_stop();
        int n = zxv_file_load(e->name, song, sizeof song);
        song_size = 0;
        if (n > 0 && zxv_music_info(song, n, &info) != ZXV_MUSIC_NONE) {
            song_size = n;
            cat(song_name, e->name);
            play_song();
        }
    }
    draw_info();
}

int main(void)
{
    es = zxv_lang_get() == ZXV_LANG_ES;
    uint8_t stereo = REG8(ZXV_S_AY_STEREO);
    REG8(ZXV_S_AY_STEREO) = ZXV_AY_STEREO_ABC;
    read_dir();
    draw_all();
    for (;;) {
        zxv_music_update();
        zxv_wait_vblank();
        draw_status();
        int k = zxv_key_get();
        if (k < 0)
            continue;
        if (k == KEY_ESC)
            break;
        int old = sel;
        if (k == KEY_UP && sel > 0)
            sel--;
        else if (k == KEY_DOWN && sel < nfiles - 1)
            sel++;
        else if (k == KEY_PGUP)
            sel = sel > LIST_ROWS ? sel - LIST_ROWS : 0;
        else if (k == KEY_PGDN)
            sel = sel + LIST_ROWS < nfiles ? sel + LIST_ROWS : (nfiles ? nfiles - 1 : 0);
        else if (k == KEY_ENTER && nfiles)
            open_file();
        else if (k == ' ')
            zxv_music_pause(paused = !paused);
        else if (k == 's' || k == 'S')
            zxv_music_stop();
        else if (k == 'l' || k == 'L') {
            loop = !loop;
            play_song();
            draw_info();
        } else if (k == '2') {
            on_ay2 = !on_ay2;
            play_song();
            draw_info();
        } else if (k >= '0' && k <= '9' && bank_n) {
            zxv_sfx_play(bank_page * 10 + (k == '0' ? 9 : k - '1'), ZXV_SFX_AUTO);
        } else if (k == ',' && bank_page > 0) {
            bank_page--;
            draw_info();
        } else if (k == '.' && (bank_page + 1) * 10 < bank_n) {
            bank_page++;
            draw_info();
        }
        if (sel != old) {
            if (sel < top)
                top = sel;
            if (sel >= top + LIST_ROWS)
                top = sel - LIST_ROWS + 1;
            draw_list();
        }
    }
    zxv_music_stop();
    zxv_sfx_stop(ZXV_SFX_AUTO);
    for (int i = 0; i < 10; i++) {          /* que salga el silencio */
        zxv_music_update();
        zxv_wait_vblank();
    }
    REG8(ZXV_S_AY_STEREO) = stereo;
    return 0;
}
