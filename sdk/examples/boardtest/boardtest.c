/*
 * boardtest.c - La prueba de la placa: lo que se puede comprobar desde un
 * programa, en una pantalla, en el idioma de la máquina.
 *
 * Al empezar: la salida de vídeo y el monitor (OUT_INFO), el reloj (que esté
 * en hora y que avance), la tarjeta (escribe, lee y borra un fichero; el LED
 * SD parpadea) y la versión. Luego, en vivo: el teclado, los dos mandos y el
 * ratón. Con 1, 2 y 3 suenan el beeper, el AY por la izquierda y por la
 * derecha; con T se repiten la tarjeta y el reloj; con P, la página de las
 * paletas (spec §4.6); con Esc se vuelve al BASIC. Lo que no se ve desde aquí
 * (MODO, RESET, el LED del enlace) va en las instrucciones de abajo.
 *
 * El clic de las teclas (spec §7.11) es el que leyó la ROM de la plataforma
 * al arrancar: se cambia en el menú EDIT y, tras RESET (que reinicia el
 * firmware y lo vuelve a leer del reloj), tiene que seguir igual. Un
 * programa no lo puede probar solo: lo que escribiera lo leería de la copia
 * del firmware, no del reloj.
 */
#include "zxv_api.h"
#include "zxv_gfx.h"

#define REG8(a)         (*(volatile uint8_t *)(uintptr_t)(a))
#define REG16(a)        (*(volatile uint16_t *)(uintptr_t)(a))
#define REG32(a)        (*(volatile uint32_t *)(uintptr_t)(a))
#define KEY_ESC         0x1B

/* Los colores: tinta sobre blanco */
enum { NEGRO = 0, AZUL = 1, ROJO = 2, VERDE = 4, BLANCO = 7 };

static int es;                          /* ¿en español? */
#define TR(en, s)       (es ? (s) : (en))

/* --- Escribir ------------------------------------------------------------------ */

static void at(int col, int row, int ink, const char *s)
{
    zxv_text_colors(ink, BLANCO);
    zxv_text_at(col, row);
    zxv_print_str(s);
}

/* Un número con relleno de ceros hasta `width` cifras (o signo menos). */
static char *num(char *p, int n, int width)
{
    char t[12];
    int i = 0, neg = n < 0;
    unsigned u = neg ? (unsigned)-n : (unsigned)n;
    do t[i++] = (char)('0' + u % 10); while ((u /= 10) && i < 11);
    while (i < width)
        t[i++] = '0';
    if (neg)
        *p++ = '-';
    while (i)
        *p++ = t[--i];
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

/* Borra el resto de la fila desde `col` (para lo que cambia en vivo). */
static void rest(int col, int row)
{
    zxv_text_colors(NEGRO, BLANCO);
    zxv_text_at(col, row);
    for (int i = col; i < 32; i++)
        zxv_print_char(' ');
}

/* --- Lo que se comprueba una vez -------------------------------------------------- */

static void video(void)
{
    uint32_t o = REG32(ZXV_V_OUT_INFO);
    char s[40], *p = s;
    p = cat(p, (o & 0xFF) == ZXV_OUT_HDMI ? "HDMI " : (o & 0xFF) == ZXV_OUT_DVI ? "DVI " : "? ");
    unsigned hz = (o >> 8) & 0xFF;
    p = cat(p, "640x480 ");
    p = num(p, (int)hz, 1);
    cat(p, " Hz");
    rest(10, 2);
    at(10, 2, NEGRO, s);
    uint32_t m = o & ZXV_OUT_SCREEN_MASK;
    rest(10, 3);
    if (m == ZXV_OUT_SCREEN_ON)
        at(10, 3, VERDE, TR("connected", "conectado"));
    else if (m == ZXV_OUT_SCREEN_NONE)
        at(10, 3, ROJO, TR("unplugged", "desconectado"));
    else
        at(10, 3, AZUL, TR("unknown (HPD)", "no se sabe (HPD)"));
}

static void clock_test(void)
{
    zxv_clock_t a, b;
    char s[40], *p = s;
    rest(10, 4);
    int r = zxv_clock_get(&a);
    if (r == ZXV_E_NOCLOCK) {
        at(10, 4, ROJO, TR("not set (CLOCK)", "sin hora (CLOCK)"));
        return;
    }
    if (r) {
        at(10, 4, ROJO, TR("error ", "error "));
        num(s, r, 1);
        zxv_print_str(s);
        return;
    }
    uint32_t t0 = zxv_time_us();
    do {                                    /* hasta que cambie el segundo, o 1,5 s */
        zxv_wait_vblank();
        zxv_clock_get(&b);
    } while (b.sec == a.sec && zxv_time_us() - t0 < 1500000);
    p = num(p, b.hour, 2);
    *p++ = ':';
    p = num(p, b.min, 2);
    *p++ = ':';
    p = num(p, b.sec, 2);
    *p++ = ' ';
    *p = 0;
    at(10, 4, NEGRO, s);
    if (b.sec != a.sec)
        zxv_print_str(TR("runs", "avanza"));
    else
        at(19, 4, ROJO, TR("stopped", "parado"));
}

static uint8_t buf[4096], back[4096];

static void card_test(void)
{
    static const char *const name = "SD:prueba.tmp";
    char s[40];
    rest(10, 5);
    at(10, 5, AZUL, TR("testing...", "probando..."));
    for (unsigned i = 0; i < sizeof buf; i++)
        buf[i] = (uint8_t)(i * 7 + (i >> 8));
    int r, f = zxv_file_open(name, ZXV_O_WRITE | ZXV_O_CREATE | ZXV_O_TRUNC);
    if (f >= 0) {
        r = zxv_file_write(f, buf, sizeof buf);
        int c = zxv_file_close(f);
        if (r == (int)sizeof buf && !c) {
            r = zxv_file_load(name, back, sizeof back);
            if (r == (int)sizeof back) {
                r = 0;
                for (unsigned i = 0; i < sizeof buf; i++)
                    if (buf[i] != back[i])
                        r = ZXV_E_IO;
            } else if (r >= 0) {
                r = ZXV_E_IO;
            }
        } else if (r >= 0) {
            r = c ? c : ZXV_E_IO;
        }
        zxv_file_remove(name);
    } else {
        r = f;
    }
    rest(10, 5);
    if (!r) {
        at(10, 5, VERDE, TR("OK (4 KB)", "bien (4 KB)"));
    } else if (r == ZXV_E_NOCARD) {
        at(10, 5, ROJO, TR("no card", "sin tarjeta"));
    } else {
        at(10, 5, ROJO, "error ");
        num(s, r, 1);
        zxv_print_str(s);
    }
}

/* --- Lo de en vivo --------------------------------------------------------------- */

static void joystick(int n)
{
    static const char marks[] = "<>v^123Ss";
    uint32_t j = zxv_joy_read(n);
    int row = 9 + n;
    rest(10, row);
    if (!(j & ZXV_JOY_CONNECTED)) {
        at(10, row, AZUL, TR("not connected", "no conectado"));
        return;
    }
    static const uint16_t bits[] = { ZXV_JOY_LEFT, ZXV_JOY_RIGHT, ZXV_JOY_DOWN, ZXV_JOY_UP, ZXV_JOY_FIRE1,
                                     ZXV_JOY_FIRE2, ZXV_JOY_FIRE3, ZXV_JOY_START, ZXV_JOY_SELECT };
    for (int i = 0; i < 9; i++) {
        zxv_text_colors((j & bits[i]) ? BLANCO : NEGRO, (j & bits[i]) ? VERDE : BLANCO);
        zxv_text_at(10 + 2 * i, row);
        zxv_print_char(marks[i]);
    }
}

static void mouse(zxv_mouse_t *m)
{
    char s[40], *p = s;
    zxv_mouse_update(m, 256, 192);
    p = num(p, m->x, 3);
    *p++ = ',';
    p = num(p, m->y, 3);
    p = cat(p, TR(" wheel ", " rueda "));
    p = num(p, m->wheel, 1);
    *p++ = ' ';
    *p++ = (m->buttons & 1) ? 'I' : '.';
    *p++ = (m->buttons & 4) ? 'C' : '.';
    *p++ = (m->buttons & 2) ? 'D' : '.';
    *p = 0;
    rest(10, 12);
    at(10, 12, NEGRO, s);
}

static void key(int k)
{
    char s[24];
    rest(10, 8);
    zxv_text_colors(NEGRO, BLANCO);
    zxv_text_at(10, 8);
    if (k >= (int)ZXV_KEY_SPECIAL) {
        zxv_print_str(TR("special ", "especial "));
        num(s, k & 0xFF, 1);
        zxv_print_str(s);
    } else if (k >= 32) {
        zxv_print_char('\'');
        zxv_print_char(k);                  /* su código Unicode: también la ñ */
        zxv_print_char('\'');
    } else {
        zxv_print_str(TR("control", "control"));
    }
    zxv_print_str(" (");
    num(s, k, 1);
    zxv_print_str(s);
    zxv_print_str(")");
}

/* --- El sonido ------------------------------------------------------------------- */

static void beeper(void)
{
    uint32_t t = zxv_time_us(), end = t + 400000;
    int level = 0;
    while ((int32_t)(zxv_time_us() - end) < 0) {
        level ^= 1;
        zxv_beeper_event(level);
        t += 500;                           /* 1 kHz */
        while ((int32_t)(zxv_time_us() - t) < 0)
            ;
    }
    zxv_beeper_event(0);
}

static uint32_t ay_until;               /* cuándo se calla el AY (0: callado) */

static void ay(int right)
{
    zxv_ay_write(right ? 4 : 0, 252);       /* La de 440 Hz: 1773400 / (16 × 440) ≈ 252 */
    zxv_ay_write(right ? 5 : 1, 0);
    zxv_ay_write(7, right ? 0x3B : 0x3E);   /* solo el tono de C, o de A */
    zxv_ay_write(8, right ? 0 : 15);
    zxv_ay_write(10, right ? 15 : 0);
    ay_until = zxv_time_us() + 500000;
}

static void ay_off(void)
{
    zxv_ay_write(7, 0x3F);
    zxv_ay_write(8, 0);
    zxv_ay_write(10, 0);
    ay_until = 0;
}

/* --- Las paletas (spec §4.6) ------------------------------------------------------- */

/* Tras el VBLANK, cuando el enlace ya ha leído la imagen: ahora se pueden
 * cambiar las paletas y la tabla por línea (spec §4.8). */
static void frame(void)
{
    zxv_wait_vblank();
    while (REG32(ZXV_SV_SYSFLAGS) & ZXV_SYSF_LINK_BUSY)
        ;
}

/* El CRC-32 de zlib de las 4 paletas: el mismo que da la ULA en su página de
 * diagnóstico (Ctrl+Alt+F12) con lo que le llega por el enlace. */
static uint32_t palettes_crc(void)
{
    uint32_t c = 0xFFFFFFFFu;
    for (uint32_t a = ZXV_PALETTES; a < ZXV_PALETTES + 128; a++) {
        c ^= REG8(a);
        for (int k = 0; k < 8; k++)
            c = c >> 1 ^ (0xEDB88320u & -(c & 1));
    }
    return ~c;
}

/* La tabla 3: una rampa de fuego en los índices 1 a 15, girada `step`. */
static void fire(int step)
{
    for (int i = 1; i < 16; i++) {
        unsigned v = 17u * (unsigned)(1 + (i - 1 + step) % 15);
        REG16(ZXV_PALETTE(3) + 2 * i) = ZXV_RGB(v, v * v / 255u, 0);
    }
}

/* La página, en la página 1 de la VRAM (la de la prueba sigue en la 0). La
 * tabla 0 es la de la ROM, para las letras. Una banda de las tablas 0 y 1
 * línea a línea (la 1, con los colores de siempre puestos a mano: no se
 * deben ver rayas), una de la 2 (una rampa gris) y una de la 3 (una de
 * fuego, que gira en el VBLANK). El borde, con el índice 0 y la paleta de
 * REMAP, que cambia cada segundo; las letras y las barras van con su tabla
 * en la tabla por línea, así que no cambian. */
static void palette_page(void)
{
    static const uint32_t usual[16] = {                 /* spec §4.1 */
        0x000000, 0x0000D7, 0xD70000, 0xD700D7, 0x00D700, 0x00D7D7, 0xD7D700, 0xD7D7D7,
        0x000000, 0x0000FF, 0xFF0000, 0xFF00FF, 0x00FF00, 0x00FFFF, 0xFFFF00, 0xFFFFFF,
    };
    static uint16_t saved[64];
    uint8_t border = REG8(ZXV_V_BORDER), remap = REG8(ZXV_V_REMAP), flags = REG8(ZXV_V_FLAGS);
    for (int i = 0; i < 64; i++)
        saved[i] = REG16(ZXV_PALETTES + 2 * i);

    zxv_gfx_draw_page(1);
    zxv_cls(BLANCO);
    zxv_text_colors(BLANCO, AZUL);
    zxv_text_at(0, 0);
    zxv_print_str(TR(" ZX-V: palettes", " ZX-V: paletas"));
    for (int i = es ? 14 : 15; i < 32; i++)
        zxv_print_char(' ');
    at(0, 2, AZUL, TR("Table 0 (ROM) and 1 (by hand):", "Tabla 0 (ROM) y 1 (a mano):"));
    at(0, 6, NEGRO, TR("Alternating lines: no stripes.", "Líneas alternas: sin rayas."));
    at(0, 8, AZUL, TR("Table 2: grey ramp, 1 to 15", "Tabla 2: rampa gris, 1 a 15"));
    at(0, 11, AZUL, TR("Table 3: it moves (Space)", "Tabla 3: se mueve (Espacio)"));
    at(0, 15, NEGRO, TR("Border, every second: black,", "Borde, cada segundo: negro,"));
    at(0, 16, NEGRO, TR("black, orange, purple. Text and", "negro, naranja, morado. Lo demás"));
    at(0, 17, NEGRO, TR("bars do not change.", "no cambia."));
    at(0, 19, AZUL, TR("Palettes CRC", "CRC paletas"));
    at(0, 20, NEGRO, TR("Stopped, the same as in the", "Parado, el mismo que en el"));
    at(0, 21, NEGRO, TR("emulator and in Ctrl+Alt+F12.", "emulador y en Ctrl+Alt+F12."));
    at(0, 23, ROJO, TR("P or Esc: back", "P o Esc: volver"));
    for (int i = 0; i < 16; i++) {
        zxv_gfx_rect(16 * i, 24, 16, 24, i);
        zxv_gfx_rect(16 * i, 72, 16, 16, i);
        zxv_gfx_rect(16 * i, 96, 16, 16, i);
    }

    frame();
    for (int i = 0; i < 16; i++) {
        REG16(ZXV_PALETTE(1) + 2 * i) = ZXV_RGB(usual[i] >> 16, (usual[i] >> 8) & 255, usual[i] & 255);
        unsigned v = 17u * (unsigned)i;
        REG16(ZXV_PALETTE(2) + 2 * i) = ZXV_RGB(v, v, v);
    }
    REG16(ZXV_PALETTE(2)) = ZXV_RGB(215, 108, 0);       /* el índice 0, el del borde: naranja */
    REG16(ZXV_PALETTE(3)) = ZXV_RGB(108, 0, 215);       /* y morado */
    fire(0);
    for (int ly = 0; ly < ZXV_FRAME_LINES(0); ly++) {
        int y = ly - ZXV_BORDER_LINES(0);
        uint8_t *e = (uint8_t *)(uintptr_t)(ZXV_LINE_TABLE + ZXV_LINE_ENTRY_SIZE * (uint32_t)ly);
        e[ZXV_LT_BORDER] = 0xFF;
        e[ZXV_LT_REMAP] = (uint8_t)(y < 0 || y >= 192 ? 0xFF
                                    : y >= 24 && y < 48 ? (y & 1)
                                    : y >= 72 && y < 88 ? 2
                                    : y >= 96 && y < 112 ? 3 : 0);
        e[ZXV_LT_SCROLL_X] = e[ZXV_LT_SCROLL_X + 1] = 0xFF;
        e[ZXV_LT_SRC_LINE] = e[ZXV_LT_SRC_LINE + 1] = 0xFF;
        e[6] = e[7] = 0;
    }
    REG8(ZXV_V_BORDER) = 0;
    REG8(ZXV_V_FLAGS) = flags | ZXV_VFLAG_LINETAB;
    zxv_set_page(1);

    int run = 1, step = 0;
    uint32_t shown = 0;
    for (unsigned n = 0;; n++) {
        frame();
        REG8(ZXV_V_REMAP) = (uint8_t)(n / 60 % 4);
        if (run && !(n & 1))
            fire(step = (step + 14) % 15);
        uint32_t crc = palettes_crc();
        if (crc != shown || n == 0) {
            char s[12], *p = s;
            for (int k = 28; k >= 0; k -= 4)
                *p++ = "0123456789ABCDEF"[(crc >> k) & 15];
            *p = 0;
            rest(13, 19);
            at(13, 19, NEGRO, s);
            if (!run)
                zxv_print_str(TR(" (stopped)", " (parado)"));
            shown = crc;
        }
        int k = zxv_key_get();
        if (k == ' ') {
            run = !run;
            fire(step = 0);
            shown = ~crc;                       /* para que lo escriba con "parado" o sin él */
        } else if (k == 'p' || k == 'P' || k == KEY_ESC) {
            break;
        }
    }

    frame();
    for (int i = 0; i < 64; i++)
        REG16(ZXV_PALETTES + 2 * i) = saved[i];
    REG8(ZXV_V_BORDER) = border;
    REG8(ZXV_V_REMAP) = remap;
    REG8(ZXV_V_FLAGS) = flags;
    zxv_set_page(0);
    zxv_gfx_draw_page(0);
}

/* --- La pantalla ------------------------------------------------------------------ */

static void labels(void)
{
    zxv_cls(BLANCO);
    zxv_text_colors(BLANCO, AZUL);
    zxv_text_at(0, 0);
    zxv_print_str(TR(" ZX-V: board test", " ZX-V: prueba de la placa"));
    for (int i = es ? 25 : 17; i < 32; i++)
        zxv_print_char(' ');
    at(0, 2, AZUL, TR("Video", "Vídeo"));
    at(0, 3, AZUL, TR("Screen", "Monitor"));
    at(0, 4, AZUL, TR("Clock", "Reloj"));
    at(0, 5, AZUL, TR("SD card", "Tarjeta"));
    at(0, 6, AZUL, TR("Version", "Versión"));
    at(0, 7, AZUL, TR("Key click", "Clic"));
    at(10, 7, NEGRO, REG32(ZXV_SV_PREFS) & ZXV_PREF_NO_CLICK ? TR("off", "no") : TR("on", "sí"));
    at(0, 8, AZUL, TR("Keyboard", "Teclado"));
    at(0, 9, AZUL, TR("Gamepad 1", "Mando 1"));
    at(0, 10, AZUL, TR("Gamepad 2", "Mando 2"));
    at(0, 11, AZUL, TR("(keys)", "(teclas)"));
    at(10, 11, NEGRO, "< > v ^ 1 2 3 S s");
    at(0, 12, AZUL, TR("Mouse", "Ratón"));
    at(0, 14, NEGRO, TR("1 beeper  2 AY left  3 AY right", "1 beeper  2 AY izq.  3 AY der."));
    at(0, 15, NEGRO, TR("T: card and clock again", "T: otra vez tarjeta y reloj"));
    at(0, 16, NEGRO, TR("P: the palettes", "P: las paletas"));
    at(0, 17, NEGRO, TR("MODE: changes the video mode;", "MODO: cambia el modo de vídeo;"));
    at(0, 18, NEGRO, TR("  the Video line follows it.", "  la línea Vídeo lo sigue."));
    at(0, 19, NEGRO, TR("RESET: restarts the machine.", "RESET: reinicia la máquina."));
    at(0, 20, NEGRO, TR("SD LED: blinks in the card test", "LED SD: parpadea con la tarjeta"));
    at(0, 21, NEGRO, TR("Link LED: on, steady.", "LED ENLACE: encendido, fijo."));
    at(0, 22, NEGRO, TR("Click: change it in EDIT, RESET", "Clic: cámbialo en EDIT y RESET"));
    at(0, 23, ROJO, TR("Esc: back to BASIC", "Esc: volver al BASIC"));
    char s[24], *p = s;
    uint32_t v = REG32(ZXV_SV_VERSION);
    p = num(p, (int)(v >> 16), 1);
    *p++ = '.';
    p = num(p, (int)(v & 0xFFFF), 1);
    p = cat(p, TR(", language ", ", idioma "));
    uint32_t l = zxv_lang_get();
    *p++ = (char)(l & 0xFF);
    *p++ = (char)(l >> 8);
    *p = 0;
    at(10, 6, NEGRO, s);
}

int main(void)
{
    es = zxv_lang_get() == ZXV_LANG_ES;
    uint8_t stereo = REG8(ZXV_S_AY_STEREO);
    REG8(ZXV_S_AY_STEREO) = 1;              /* ABC: A a la izquierda, C a la derecha */
    zxv_set_mode(0);
    labels();
    video();
    clock_test();
    card_test();
    zxv_mouse_t m;
    zxv_mouse_init(&m, 128, 96);
    rest(10, 8);
    at(10, 8, AZUL, TR("press a key", "pulsa una tecla"));
    for (;;) {
        zxv_wait_vblank();
        video();
        joystick(0);
        joystick(1);
        mouse(&m);
        if (ay_until && (int32_t)(zxv_time_us() - ay_until) >= 0)
            ay_off();
        int k = zxv_key_get();
        if (k < 0)
            continue;
        if (k == KEY_ESC)
            break;
        key(k);
        if (k == '1')
            beeper();
        else if (k == '2' || k == '3')
            ay(k == '3');
        else if (k == 't' || k == 'T') {
            clock_test();
            card_test();
        } else if (k == 'p' || k == 'P') {
            palette_page();
        }
    }
    ay_off();
    REG8(ZXV_S_AY_STEREO) = stereo;
    return 0;
}
