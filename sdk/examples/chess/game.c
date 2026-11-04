/*
 * game.c - ZXV Chess, ajedrez para ZX-V: el menú, la partida, el dibujo y el sonido.
 *
 * Las reglas y el jugador artificial están en chess.c (que no sabe nada de
 * ZX-V y se prueba en el PC); aquí solo hay interfaz.
 *
 * Pantalla (modo 0, 256 x 192): el tablero ocupa 192 x 192 (casillas de
 * 24 x 24) y a su derecha hay un panel de 8 columnas de texto.
 *
 * Mandos: cursores / WASD / cruceta mueven; Intro, Espacio o FIRE1 eligen y
 * mueven; Esc o FIRE2 anulan; U deshace; N nueva partida; M menú.
 *
 * Ratón: el puntero (que dibujamos nosotros) aparece al mover el ratón y se
 * esconde si se vuelve al teclado. En el menú, pasar por encima resalta y el
 * botón izquierdo elige; en la partida el cursor sigue al puntero, el botón
 * izquierdo elige y mueve, el derecho anula, y las ayudas del panel (Esc, U, N,
 * M) son botones. La rueda cambia el nivel en el menú.
 */
#include "zxv_gfxfile.h"
#include "chess.h"

#define REG8(a)     (*(volatile uint8_t *)(uintptr_t)(a))

#define CELL        24
#define PANEL_X     192
#define PANEL_COL   24                          /* primera columna de texto del panel (PANEL_X / 8) */
#define PANEL_TXT   25                          /* donde empieza su texto: deja una columna de margen */
#define TEXT_COLS   32                          /* columnas de texto de la pantalla (0 a 31) */

/* Los tableros, todos con colores sin brillo (1 a 7); el primero de la lista es
 * el de arranque. Cada uno lleva los colores de resalte que mejor se separan de
 * sus casillas y entre sí: la última jugada y la pieza elegida. Casillas oscuras
 * primero (a1 es oscura). */
typedef struct {
    const char *es, *en;
    uint8_t dark, light, last, select;
} board_t;

static const board_t BOARDS[4] = {
    { "Azul-Cian", "Blue-Cyan", 1, 5, 4, 6 },
    { "Magenta-Amarillo", "Magenta-Yellow", 3, 6, 4, 5 },
    { "Magenta-Cian", "Magenta-Cyan", 3, 5, 4, 6 },
    { "Magenta-Verde", "Magenta-Green", 3, 4, 5, 6 },
};
#define N_BOARDS    4

static int board_sel;                           /* el tablero elegido (índice en BOARDS) */

#define C_DARK      (BOARDS[board_sel].dark)    /* casillas oscuras */
#define C_LIGHT     (BOARDS[board_sel].light)   /* y claras */
#define C_LAST      (BOARDS[board_sel].last)    /* la última jugada */
#define C_SELECT    (BOARDS[board_sel].select)  /* la pieza elegida */
#define C_CHECK     10                          /* el rey en jaque (rojo brillante) */

/* Tintas de texto compartidas por el panel del juego y el menú principal. */
#define C_TITLE_ZXV     10                      /* "ZXV": rojo brillante */
#define C_TITLE_CHESS   11                      /* "CHESS": magenta brillante */
#define C_KEY           10                      /* la primera letra de los botones del panel: rojo brillante */
#define INK_HILITE      14                      /* lo marcado con el ratón o el cursor: amarillo brillante */

/* --- Textos en español y en inglés ------------------------------------------ */

static int es;
#define T(a, b)     (es ? (a) : (b))

/* --- Las piezas ------------------------------------------------------------------ */

/* Los gráficos son los de chess.gfx: los sprites 0 a 5, las piezas blancas
 * (peón, caballo, alfil, torre, dama y rey), de 24 x 24; del 6 al 11, las
 * negras, y el 12, el puntero del ratón. El Makefile lo hace con png2gfx.py a
 * partir de art/pieces.png y art/pointer.png, que se editan con cualquier
 * programa de dibujo. El programa lo lleva dentro y, si en la carpeta desde la
 * que se carga hay otro chess.gfx (de zxv-gfx o gfxedit, por ejemplo), usa ese. */
#ifndef GFX_DATA                                /* el arnés del PC (test/ui_test.c) trae el suyo */
#define GFX_SLACK   8192                        /* sitio detrás para uno de la tarjeta más grande */
#define STR_(x)     #x
#define STR(x)      STR_(x)
__asm__(".section .data.chess_gfx, \"aw\"\n"
        ".balign 4\n"
        "chess_gfx:\n"
        ".incbin \"build/chess.gfx\"\n"
        "chess_gfx_end:\n"
        ".space " STR(GFX_SLACK) "\n"
        ".previous\n");
extern uint8_t chess_gfx[], chess_gfx_end[];
#define GFX_DATA    chess_gfx
#define GFX_SIZE    ((uint32_t)(chess_gfx_end - chess_gfx))
#endif

#define SPR_POINTER 12                          /* el sprite del puntero, tras las 12 piezas */

static const void *piece_spr[12];               /* 0-5 blancas, 6-11 negras: peón...rey */
static const void *ptr_spr;
static uint32_t pool[(12 * 584 + 2048) / 4];    /* los sprites preparados (una pieza, 584 bytes) */
static uint32_t pool_used;

static void bad_gfx(const char *why_es, const char *why_en)
{
    static char msg[48];
    const char *a = "chess.gfx: ", *b = T(why_es, why_en);
    int n = 0;

    while (*a)
        msg[n++] = *a++;
    while (*b && n < (int)sizeof msg - 1)
        msg[n++] = *b++;
    msg[n] = 0;
    zxv_report_error(0, msg);                   /* vuelve al BASIC con el informe */
}

/* El sprite num del fichero, preparado; con w, tiene que medir w x h. */
static const void *load_sprite(const zxv_gfxfile *f, int num, int w, int h)
{
    static uint32_t pix[1024 / 4];              /* sus píxeles, en líneas alineadas */
    zxv_bitmap b;
    uint32_t n = zxv_gfxf_sprite(f, num, &b, pix, sizeof pix);

    if (!n || n > sizeof pix || (w && (b.w != w || b.h != h)))
        bad_gfx("piezas 0-11, de 24x24; puntero 12", "pieces 0-11, 24x24; pointer 12");
    uint32_t need = (zxv_spr_prepare(&b, 0, 0, b.w, b.h, 0) + 3) & ~3u;
    if (pool_used + need > sizeof pool)
        bad_gfx("puntero demasiado grande", "pointer too big");
    void *p = (uint8_t *)pool + pool_used;
    pool_used += need;
    zxv_spr_prepare(&b, 0, 0, b.w, b.h, p);
    return p;
}

static void make_pieces(void)
{
    zxv_gfxfile f;
    uint32_t size = GFX_SIZE;

#ifdef GFX_SLACK
    int n = zxv_file_load("chess.gfx", GFX_DATA, size + GFX_SLACK);  /* el de la tarjeta, si hay */
    if (n == ZXV_E_RANGE)
        bad_gfx("demasiado grande", "too big");
    if (n >= 0)
        size = (uint32_t)n;
#endif
    if (!zxv_gfxf_open(&f, GFX_DATA, size))
        bad_gfx("no vale", "not valid");
    pool_used = 0;
    for (int i = 0; i < 12; i++)
        piece_spr[i] = load_sprite(&f, i, CELL, CELL);
    ptr_spr = load_sprite(&f, SPR_POINTER, 0, 0);
}

static int piece_index(int pc)                  /* pc: PAWN..KING, negativo las negras */
{
    return pc < 0 ? 6 + (-pc - 1) : pc - 1;
}

static void draw_piece(int pc, int x, int y)
{
    zxv_spr_draw(piece_spr[piece_index(pc)], x, y, 0, 0);
}

/* --- Sonido: efectos sencillos en el segundo AY ------------------------------- */

#define AY_PERIOD(hz)   ((1773448 + 8 * (hz)) / (16 * (hz)))

/* Cada sonido: pares (Hz, frames), terminado en 0. */
static const uint16_t SND_SELECT[] = { 660, 1, 0 };
static const uint16_t SND_MOVE[] = { 392, 2, 0 };
static const uint16_t SND_CAPTURE[] = { 262, 2, 196, 3, 0 };
static const uint16_t SND_CASTLE[] = { 330, 2, 392, 3, 0 };
static const uint16_t SND_CHECK[] = { 523, 3, 659, 5, 0 };
static const uint16_t SND_WIN[] = { 523, 5, 659, 5, 784, 5, 1047, 14, 0 };
static const uint16_t SND_DRAW[] = { 392, 6, 330, 6, 262, 12, 0 };
static const uint16_t SND_NO[] = { 110, 4, 0 };

static const uint16_t *snd_next;
static int snd_left;

static void snd_note(int hz)
{
    if (hz) {
        int p = AY_PERIOD(hz);
        zxv_ay_write(ZXV_AY2 + 0, p & 0xFF);
        zxv_ay_write(ZXV_AY2 + 1, p >> 8);
        zxv_ay_write(ZXV_AY2 + 7, 0x3E);        /* solo el tono del canal A */
        zxv_ay_write(ZXV_AY2 + 8, 10);
    } else {
        zxv_ay_write(ZXV_AY2 + 8, 0);
    }
}

static void sound(const uint16_t *s)
{
    snd_next = s;
    snd_left = 0;
}

static void sound_tick(void)
{
    if (snd_left > 0 && --snd_left > 0)
        return;
    if (snd_next && snd_next[0]) {
        snd_note(snd_next[0]);
        snd_left = snd_next[1];
        snd_next += 2;
    } else {
        snd_next = 0;
        snd_note(0);
    }
}

/* --- La partida ---------------------------------------------------------------- */

#define HIST_MAX    400

static Pos hist[HIST_MAX];                      /* hist[0] es la posición inicial */
static int nhist;

enum { MODE_WHITE, MODE_BLACK, MODE_TWO };      /* tú con blancas / con negras / dos jugadores */

static int mode = MODE_WHITE;
static int level = 1;                           /* 1 a 4 */
static int ai_side;                             /* WHITE, BLACK o 0 */
static int flipped;                             /* el tablero se ve desde las negras */
static int status;                              /* ST_... de la posición actual */
static int game_active;                         /* hay una partida empezada */

static int cursor;                              /* casilla del cursor */
static int sel = NO_SQ;                         /* casilla elegida */
static Move sel_moves[40];                      /* las jugadas legales de la pieza elegida */
static int nsel;
static int promo_on;                            /* se está eligiendo la corona */
static int promo_idx;                           /* 0 dama, 1 torre, 2 alfil, 3 caballo */
static int thinking;                            /* la máquina va a pensar */
static int confirm_new;                         /* frames que quedan para confirmar con otra N */
static int dirty = 1;                           /* hay que redibujar */

static const uint8_t PROMO_PIECE[4] = { QUEEN, ROOK, BISHOP, KNIGHT };

static const struct { int depth; uint32_t cap; int noise; } LEVELS[4] = {
    { 1, 0, 100 },                              /* principiante: mira una jugada, con despistes */
    { 2, 0, 30 },                               /* fácil */
    { 4, 40000, 0 },                            /* medio */
    { 6, 100000, 0 },                           /* fuerte */
};

static const Pos *cur(void) { return &hist[nhist - 1]; }

static void push(const Pos *p)
{
    if (nhist == HIST_MAX) {                    /* lleno: se olvidan las dos más viejas */
        for (int i = 2; i < HIST_MAX; i++)
            hist[i - 2] = hist[i];
        nhist -= 2;
    }
    hist[nhist++] = *p;
}

static void new_game(int m)
{
    mode = m;
    ai_side = m == MODE_WHITE ? BLACK : m == MODE_BLACK ? WHITE : 0;
    flipped = m == MODE_BLACK;
    pos_init(&hist[0]);
    nhist = 1;
    status = ST_PLAY;
    sel = NO_SQ;
    nsel = 0;
    promo_on = 0;
    thinking = 0;
    confirm_new = 0;
    cursor = SQ(4, flipped ? 6 : 1);
    game_active = 1;
    dirty = 1;
}

static int over(void) { return status >= ST_MATE; }

static void select_square(int sq)
{
    sel = sq;
    nsel = 0;
    if (sq == NO_SQ)
        return;
    Move all[MAX_MOVES];
    int n = legal_moves(cur(), all);
    for (int i = 0; i < n && nsel < 40; i++)
        if (all[i].from == sq)
            sel_moves[nsel++] = all[i];
}

static int is_target(int sq)
{
    for (int i = 0; i < nsel; i++)
        if (sel_moves[i].to == sq)
            return 1;
    return 0;
}

static void apply_move(Move m)
{
    Pos n = *cur();
    int castle = m.flags & FL_CASTLE;
    int capture = m.flags & FL_CAP;

    make_move(&n, m);
    push(&n);
    select_square(NO_SQ);
    status = pos_status(cur());
    dirty = 1;

    if (status == ST_MATE)
        sound(SND_WIN);
    else if (over())
        sound(SND_DRAW);
    else if (status == ST_CHECK)
        sound(SND_CHECK);
    else if (capture)
        sound(SND_CAPTURE);
    else if (castle)
        sound(SND_CASTLE);
    else
        sound(SND_MOVE);
}

static void undo(void)
{
    if (nhist <= 1)
        return;
    nhist--;
    if (ai_side && cur()->side == ai_side && nhist > 1)
        nhist--;                                /* se deshace también la respuesta */
    select_square(NO_SQ);
    promo_on = 0;
    status = pos_status(cur());
    dirty = 1;
    sound(SND_SELECT);
}

static void ai_move(void)
{
    SearchOpts o;
    SearchResult r;

    o.depth = LEVELS[level - 1].depth;
    o.node_cap = LEVELS[level - 1].cap;
    o.noise = LEVELS[level - 1].noise;
    o.seed = *(volatile uint32_t *)(uintptr_t)ZXV_SV_FRAMES * 2654435761u + (uint32_t)nhist;
    search(cur(), &o, &r);
    thinking = 0;
    if (r.found)
        apply_move(r.best);
}

/* --- Entrada ------------------------------------------------------------------- */

#define K_RIGHT 0x4F
#define K_LEFT  0x50
#define K_DOWN  0x51
#define K_UP    0x52
#define K_ENTER 0x28
#define K_ESC   0x29
#define K_BKSP  0x2A
#define K_SPACE 0x2C
#define K_A     0x04
#define K_D     0x07
#define K_S     0x16
#define K_W     0x1A
#define K_M     0x10
#define K_N     0x11
#define K_U     0x18

enum { B_OK = 1, B_BACK = 2, B_UNDO = 4, B_NEW = 8, B_MENU = 16, B_CLICK = 32, B_RCLICK = 64 };
#define B_KEYS  (B_OK | B_BACK | B_UNDO | B_NEW | B_MENU)

typedef struct {
    int dx, dy;                                 /* movimiento (con repetición) */
    unsigned ev;                                /* B_...: pulsaciones nuevas */
    int mx, my;                                 /* el puntero del ratón */
    int moved;                                  /* el ratón se ha movido */
    int wheel;                                  /* la rueda desde el frame anterior */
} input_t;

static zxv_mouse_t mouse;
static int mouse_vis;                           /* el puntero se ve (desde que se mueve el ratón) */

static int held(unsigned hid)
{
    const volatile uint8_t *map = (const volatile uint8_t *)(uintptr_t)ZXV_I_KEYMAP;
    return (map[hid >> 3] >> (hid & 7)) & 1;
}

static void input_read(input_t *in)
{
    static unsigned prev;
    static uint8_t cnt[4];
    uint32_t joy = (zxv_joy_read(0) | zxv_joy_read(1)) & 0x1FFu;
    int dir[4] = {
        held(K_LEFT) || held(K_A) || (joy & ZXV_JOY_LEFT),
        held(K_RIGHT) || held(K_D) || (joy & ZXV_JOY_RIGHT),
        held(K_UP) || held(K_W) || (joy & ZXV_JOY_UP),
        held(K_DOWN) || held(K_S) || (joy & ZXV_JOY_DOWN),
    };
    int fire[4];

    for (int i = 0; i < 4; i++) {
        if (dir[i]) {                           /* la primera vez, y desde el frame 14 cada 5 */
            if (++cnt[i] == 19)
                cnt[i] = 14;
            fire[i] = cnt[i] == 1 || cnt[i] == 14;
        } else {
            cnt[i] = 0;
            fire[i] = 0;
        }
    }
    in->dx = fire[1] - fire[0];
    in->dy = fire[3] - fire[2];

    unsigned now = 0;
    if (held(K_ENTER) || held(K_SPACE) || (joy & ZXV_JOY_FIRE1))
        now |= B_OK;
    if (held(K_ESC) || held(K_BKSP) || (joy & ZXV_JOY_FIRE2))
        now |= B_BACK;
    if (held(K_U) || (joy & ZXV_JOY_SELECT))
        now |= B_UNDO;
    if (held(K_N))
        now |= B_NEW;
    if (held(K_M) || (joy & ZXV_JOY_START))
        now |= B_MENU;
    in->ev = now & ~prev;
    prev = now;

    /* El ratón. */
    static int pmx = -1, pmy, pwheel;
    static unsigned pbtn;
    zxv_mouse_update(&mouse, 256, 192);
    if (pmx < 0) {
        pmx = mouse.x;
        pmy = mouse.y;
    }
    in->mx = mouse.x;
    in->my = mouse.y;
    in->moved = mouse.x != pmx || mouse.y != pmy;
    pmx = mouse.x;
    pmy = mouse.y;
    in->wheel = mouse.wheel - pwheel;
    pwheel = mouse.wheel;
    unsigned pressed = mouse.buttons & ~pbtn;
    pbtn = mouse.buttons;
    if (pressed & 1)
        in->ev |= B_CLICK;
    if (pressed & 2)
        in->ev |= B_RCLICK;

    int was = mouse_vis;
    if (in->moved || pressed || in->wheel)
        mouse_vis = 1;
    else if (in->dx || in->dy || (in->ev & B_KEYS))
        mouse_vis = 0;                          /* se vuelve al teclado o al mando */
    if (mouse_vis != was || (mouse_vis && in->moved))
        dirty = 1;

    while (zxv_key_get() >= 0)                  /* vacía la cola de teclas */
        ;
}

/* --- Dibujo -------------------------------------------------------------------- */

static int visible;                             /* la página que se ve */

static void draw_pointer(void)
{
    if (mouse_vis)
        zxv_spr_draw(ptr_spr, mouse.x, mouse.y, 0, 0);
}

/* La casilla que hay bajo un punto de la pantalla, o NO_SQ. */
static int sq_at(int x, int y)
{
    if (x < 0 || y < 0 || x >= 8 * CELL || y >= 8 * CELL)
        return NO_SQ;
    int f = x / CELL, r = y / CELL;
    return SQ(flipped ? 7 - f : f, flipped ? r : 7 - r);
}

/* Las ayudas del panel que son botones (filas de texto). */
#define BTN_UNDO    18
#define BTN_NEW     20
#define BTN_MENU    22

static int panel_row_at(int x, int y)          /* la fila de texto bajo el puntero, o -1 */
{
    return (mouse_vis && x >= PANEL_X) ? y / 8 : -1;
}

/* La celda de la rejilla de coronación bajo un punto, o -1. */
static int promo_cell_x(int i) { return PANEL_X + 8 + (i & 1) * 26; }
static int promo_cell_y(int i) { return 116 + (i >> 1) * 26; }

static int promo_hit(int x, int y)
{
    for (int i = 0; i < 4; i++)
        if (x >= promo_cell_x(i) && x < promo_cell_x(i) + CELL
            && y >= promo_cell_y(i) && y < promo_cell_y(i) + CELL)
            return i;
    return -1;
}

static void txt(int col, int row, int ink, const char *s)
{
    zxv_text_colors(ink, 0);
    zxv_text_at(col, row);
    zxv_print_str(s);
}

static int ulen(const char *s)                  /* caracteres de una cadena UTF-8 */
{
    int n = 0;
    for (; *s; s++)
        if ((*s & 0xC0) != 0x80)
            n++;
    return n;
}

static void txt_center(int row, int ink, const char *s)
{
    txt((32 - ulen(s)) / 2, row, ink, s);
}

/* Texto del panel. Empieza en la columna pedida, pero si no cabe antes del borde derecho
 * se corre a la izquierda lo justo: un texto que pasara de la columna 31 saltaría a la
 * línea siguiente, encima del tablero. */
static void ptxt(int col, int row, int ink, const char *s)
{
    int len = ulen(s);

    if (col + len > TEXT_COLS)
        col = TEXT_COLS - len;
    txt(col, row, ink, s);
}

static void frame(int x, int y, int w, int h, int c, int t)
{
    zxv_gfx_rect(x, y, w, t, c);
    zxv_gfx_rect(x, y + h - t, w, t, c);
    zxv_gfx_rect(x, y + t, t, h - 2 * t, c);
    zxv_gfx_rect(x + w - t, y + t, t, h - 2 * t, c);
}

/* El cursor: un marco negro por fuera y blanco por dentro, que se ve sobre cualquier
 * color de casilla (y deja ver el anillo de las capturas). */
static void cursor_frame(int x, int y, int w, int h)
{
    frame(x, y, w, h, 0, 1);
    frame(x + 1, y + 1, w - 2, h - 2, 15, 1);
}

static int sq_x(int sq) { return (flipped ? 7 - SQ_FILE(sq) : SQ_FILE(sq)) * CELL; }
static int sq_y(int sq) { return (flipped ? SQ_RANK(sq) : 7 - SQ_RANK(sq)) * CELL; }

static void draw_board(void)
{
    const Pos *p = cur();
    int king_sq = NO_SQ;

    if (status == ST_CHECK || status == ST_MATE)
        king_sq = p->ksq[p->side > 0 ? 0 : 1];

    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            int sq = SQ(f, r);
            int x = sq_x(sq), y = sq_y(sq);
            int light = ((f + r) & 1) != 0;
            int col = light ? C_LIGHT : C_DARK;

            if (sq == p->last_from || sq == p->last_to)
                col = C_LAST;
            if (sq == sel)
                col = C_SELECT;
            if (sq == king_sq)
                col = C_CHECK;
            zxv_gfx_rect(x, y, CELL, CELL, col);

            int pc = p->sq[sq];
            if (pc)
                draw_piece(pc, x, y);
            if (sel != NO_SQ && is_target(sq)) {
                if (pc) {                       /* captura: un marco blanco y negro */
                    frame(x, y, CELL, CELL, 0, 2);
                    frame(x + 2, y + 2, CELL - 4, CELL - 4, 15, 1);
                } else {                        /* jugada: un punto blanco y negro */
                    zxv_gfx_circle(x + CELL / 2, y + CELL / 2, 5, 0, 1);
                    zxv_gfx_circle(x + CELL / 2, y + CELL / 2, 3, 15, 1);
                }
            }
        }
    }
    cursor_frame(sq_x(cursor), sq_y(cursor), CELL, CELL);
}

static const char *level_name(int l)
{
    switch (l) {
    case 1: return T("Principiante", "Beginner");
    case 2: return T("Fácil", "Easy");
    case 3: return T("Medio", "Medium");
    default: return T("Fuerte", "Strong");
    }
}

/* Un botón del panel: la primera letra (su tecla) en rojo y el resto en gris, o en amarillo
 * si el ratón está encima. */
static void panel_button(int row, int hover, const char *s)
{
    int len = ulen(s), col = PANEL_TXT;
    char key[2] = { s[0], 0 };

    if (col + len > TEXT_COLS)
        col = TEXT_COLS - len;
    txt(col, row, C_KEY, key);
    txt(col + 1, row, hover ? INK_HILITE : 7, s + 1);
}

static void draw_panel(void)
{
    const Pos *p = cur();
    char buf[12];

    zxv_gfx_rect(PANEL_X, 0, 256 - PANEL_X, 192, 0);
    ptxt(26, 0, C_TITLE_ZXV, "zxv");            /* el título, centrado, en dos líneas */
    ptxt(PANEL_TXT, 1, C_TITLE_CHESS, "CHESS");

    if (!over()) {
        ptxt(26, 3, 7, T("Turno", "Turn"));
        /* el rey del indicador va sobre el fondo del panel, sin casilla detrás (transparente) */
        draw_piece(p->side > 0 ? KING : -KING, PANEL_X + 20, 32);
        ptxt(PANEL_TXT, 7, 15, p->side > 0 ? T("Blancas", "White") : T("Negras", "Black"));
    }

    if (confirm_new) {
        ptxt(PANEL_TXT, 9, 14, T("Seguro?", "Sure?"));
        ptxt(PANEL_TXT, 10, 14, T("N = si", "N = yes"));
    } else if (thinking) {
        ptxt(PANEL_TXT, 9, 14, T("Pensar", "Think"));
    } else {
        switch (status) {
        case ST_CHECK:
            ptxt(PANEL_TXT, 9, 10, T("¡JAQUE!", "CHECK!"));
            break;
        case ST_MATE:
            ptxt(PANEL_TXT, 3, 10, T("¡MATE!", "MATE!"));
            zxv_gfx_rect(PANEL_X + 20, 32, CELL, CELL, C_LIGHT);
            draw_piece(p->side > 0 ? -KING : KING, PANEL_X + 20, 32);
            ptxt(PANEL_TXT, 7, 15, T("Ganan", "Winner:"));
            ptxt(PANEL_TXT, 8, 15, p->side > 0 ? T("negras", "Black") : T("blancas", "White"));
            break;
        case ST_STALEMATE:
            ptxt(PANEL_TXT, 3, 14, T("TABLAS", "DRAW"));
            ptxt(PANEL_TXT, 5, 7, T("Rey", "Stale-"));
            ptxt(PANEL_TXT, 6, 7, T("ahogado", "mate"));
            break;
        case ST_FIFTY:
            ptxt(PANEL_TXT, 3, 14, T("TABLAS", "DRAW"));
            ptxt(PANEL_TXT, 5, 7, T("Regla de", "50 move"));
            ptxt(PANEL_TXT, 6, 7, T("50 jug.", "rule"));
            break;
        case ST_MATERIAL:
            ptxt(PANEL_TXT, 3, 14, T("TABLAS", "DRAW"));
            ptxt(PANEL_TXT, 5, 7, T("Faltan", "Not"));
            ptxt(PANEL_TXT, 6, 7, T("piezas", "enough"));
            break;
        default:
            break;
        }
    }

    if (mode == MODE_TWO) {
        ptxt(PANEL_TXT, 11, 11, T("2 jug.", "2 pl."));
    } else {
        ptxt(PANEL_TXT, 11, 11, T("Nivel", "Level"));
        buf[0] = (char)('0' + level);
        buf[1] = 0;
        ptxt(PANEL_TXT + 6, 11, 11, buf);
    }

    if (promo_on) {
        ptxt(PANEL_TXT, 13, 14, T("Corona", "Promote"));
        for (int i = 0; i < 4; i++) {
            int x = promo_cell_x(i);
            int y = promo_cell_y(i);
            zxv_gfx_rect(x, y, CELL, CELL, ((i & 1) ^ (i >> 1)) ? C_LIGHT : C_DARK);
            draw_piece(p->side * PROMO_PIECE[i], x, y);
            if (i == promo_idx)
                cursor_frame(x - 2, y - 2, CELL + 4, CELL + 4);
        }
    } else {
        buf[0] = (char)('a' + SQ_FILE(cursor));
        buf[1] = (char)('1' + SQ_RANK(cursor));
        buf[2] = 0;
        ptxt(27, 13, 11, buf);
        int hr = panel_row_at(mouse.x, mouse.y);   /* el botón bajo el puntero, en amarillo */
        panel_button(BTN_UNDO, hr == BTN_UNDO, T("U atras", "U undo"));
        panel_button(BTN_NEW, hr == BTN_NEW, T("N nueva", "N new"));
        panel_button(BTN_MENU, hr == BTN_MENU, "M menu");
    }
}

static void draw_game(void)
{
    draw_board();
    draw_panel();
    draw_pointer();
}

/* Dibuja en la página oculta y la enseña (sin parpadeos). */
static void present(void (*draw)(void))
{
    int hidden = 1 - visible;

    zxv_gfx_draw_page(hidden);
    draw();
    zxv_set_page(hidden);
    zxv_wait_vblank();
    visible = hidden;
}

/* --- El menú ------------------------------------------------------------------- */

enum { M_CONTINUE, M_WHITE, M_BLACK, M_TWO, M_QUIT, M_COUNT };

#define MENU_ROW0   7                           /* fila de texto de la primera opción */
#define LEVEL_ROW   18                          /* el selector de nivel (dos filas) */
#define BOARD_ROW   21                          /* y el de tablero (dos filas) */

static int menu_items[M_COUNT];
static int menu_n;
static int menu_pos;                            /* 0..menu_n-1 las opciones; menu_n, el selector de nivel; menu_n+1, el de tablero */
static int focus_mouse;                         /* ese foco lo puso el ratón: los selectores no muestran '>' */

static void menu_build(void)
{
    menu_n = 0;
    if (game_active && !over())
        menu_items[menu_n++] = M_CONTINUE;
    menu_items[menu_n++] = M_WHITE;
    menu_items[menu_n++] = M_BLACK;
    menu_items[menu_n++] = M_TWO;
    menu_items[menu_n++] = M_QUIT;
}

static const char *menu_label(int item)
{
    switch (item) {
    case M_CONTINUE: return T("Continuar partida", "Resume game");
    case M_WHITE: return T("Jugar con blancas", "Play as White");
    case M_BLACK: return T("Jugar con negras", "Play as Black");
    case M_TWO: return T("Dos jugadores", "Two players");
    default: return T("Salir", "Quit");
    }
}

/* La opción del menú bajo un punto de la pantalla, o -1. */
static int menu_hit(int x, int y)
{
    for (int i = 0; i < menu_n; i++) {
        int len = ulen(menu_label(menu_items[i]));
        int x0 = ((32 - len) / 2) * 8 - 16, x1 = ((32 - len) / 2 + len) * 8 + 8;
        int y0 = (MENU_ROW0 + i * 2) * 8 - 4, y1 = y0 + 16;
        if (x >= x0 && x < x1 && y >= y0 && y < y1)
            return i;
    }
    return -1;
}

static int on_selector(int y, int row)          /* ¿está y sobre las dos filas de un selector? */
{
    return y >= row * 8 - 4 && y < (row + 2) * 8 + 4;
}

/* "< Nivel 3 >": el texto de un selector. */
static void selector_text(char *buf, const char *label, int value)
{
    int n = 0;

    buf[n++] = '<';
    buf[n++] = ' ';
    for (; *label; label++)
        buf[n++] = *label;
    buf[n++] = (char)('0' + value);
    buf[n++] = ' ';
    buf[n++] = '>';
    buf[n] = 0;
}

/* --- Texto en doble altura ---------------------------------------------------------- */

#define SCRATCH_ROW 22      /* filas de texto 22 y 23: zona de trabajo, en la página oculta */

/* Escribe `s` (solo ASCII) en (x, y) con la tinta `ink` y al doble de alto. El SDK no tiene
 * texto escalado, así que se escribe con la fuente de la ROM en la zona de trabajo (página de
 * dibujo, que no se ve), se leen sus píxeles de la memoria de vídeo y se redibuja cada línea
 * dos veces, con tramos de color. Al final la zona de trabajo vuelve a negro. */
static void big_text(int x, int y, int ink, const char *s)
{
    int len = 0;
    while (s[len])
        len++;
    const volatile uint8_t *vram =
        (const volatile uint8_t *)(uintptr_t)((1 - visible) ? ZXV_VRAM1_BASE : ZXV_VRAM0_BASE);

    zxv_text_colors(15, 0);
    zxv_text_at(0, SCRATCH_ROW);
    zxv_print_str(s);
    for (int r = 0; r < 8; r++) {
        int sy = SCRATCH_ROW * 8 + r, run = -1;
        for (int px = 0; px <= len * 8; px++) {
            int on = 0;
            if (px < len * 8) {
                uint8_t v = vram[sy * 128 + px / 2];            /* modo 0: 128 bytes por línea, 2 píxeles por byte */
                on = ((px & 1) ? v >> 4 : v & 15) != 0;
            }
            if (on && run < 0) {
                run = px;
            } else if (!on && run >= 0) {
                zxv_gfx_rect(x + run, y + 2 * r, px - run, 2, ink);
                run = -1;
            }
        }
    }
    zxv_gfx_rect(0, SCRATCH_ROW * 8, 256, 16, 0);
}

/* "Z X V" en rojo y debajo "C H E S S" en magenta (los colores del título del panel), con la
 * misma letra pero al doble de alto, centrados. */
static void draw_title(void)
{
    static const char l1[] = "Z X V", l2[] = "C H E S S";

    big_text((256 - 8 * (int)(sizeof l1 - 1)) / 2, 2, C_TITLE_ZXV, l1);
    big_text((256 - 8 * (int)(sizeof l2 - 1)) / 2, 18, C_TITLE_CHESS, l2);
}

static void draw_menu(void)
{
    char buf[40];

    zxv_gfx_rect(0, 0, 256, 192, 0);
    draw_title();

    for (int i = 0; i < menu_n; i++) {
        const char *s = menu_label(menu_items[i]);
        int row = MENU_ROW0 + i * 2;
        int col = (32 - ulen(s)) / 2;
        if (i == menu_pos) {
            txt(col - 2, row, INK_HILITE, ">");
            txt(col, row, INK_HILITE, s);
        } else {
            txt(col, row, 7, s);
        }
    }

    int focus_lvl = menu_pos == menu_n;
    int ink_lvl = focus_lvl ? 11 : 3;           /* magenta; más claro si es el que tiene el foco */
    selector_text(buf, T("Nivel ", "Level "), level);
    if (focus_lvl && !focus_mouse)              /* el '>' solo con el teclado; al pasar el ratón no */
        txt((32 - ulen(buf)) / 2 - 2, LEVEL_ROW, 14, ">");
    txt_center(LEVEL_ROW, ink_lvl, buf);
    txt_center(LEVEL_ROW + 1, ink_lvl, level_name(level));

    int focus = menu_pos == menu_n + 1;
    int ink = focus ? 13 : 5;                   /* cian; más claro si es el que tiene el foco */
    selector_text(buf, T("Tablero ", "Board "), board_sel + 1);
    if (focus && !focus_mouse)                  /* el '>' solo con el teclado; al pasar el ratón no */
        txt((32 - ulen(buf)) / 2 - 2, BOARD_ROW, 14, ">");
    txt_center(BOARD_ROW, ink, buf);
    txt_center(BOARD_ROW + 1, ink, T(BOARDS[board_sel].es, BOARDS[board_sel].en));
    draw_pointer();
}

/* Devuelve 1 para jugar (partida nueva o continuada) y 0 para salir. */
static int menu(void)
{
    input_t in;

    menu_build();
    menu_pos = 0;
    focus_mouse = 0;
    input_read(&in);                            /* que lo pulsado hasta ahora no cuente */
    for (;;) {
        input_read(&in);
        unsigned ev = in.ev;
        int level_d = 0, board_d = 0;           /* cambios pedidos al nivel y al tablero */

        if (in.moved || (ev & B_CLICK)) {
            int h = menu_hit(in.mx, in.my);
            if (h < 0 && on_selector(in.my, LEVEL_ROW))
                h = menu_n;                     /* el puntero enfoca el selector de nivel */
            if (h < 0 && on_selector(in.my, BOARD_ROW))
                h = menu_n + 1;                 /* o el de tablero */
            if (h >= 0) {
                int fm = h >= menu_n;           /* el ratón resalta los selectores, pero sin '>' */
                if (fm != focus_mouse || h != menu_pos) {
                    focus_mouse = fm;
                    if (h != menu_pos) {
                        menu_pos = h;
                        sound(SND_SELECT);
                    }
                    dirty = 1;
                }
            }
            if (ev & B_CLICK) {
                if (h >= 0 && h < menu_n)
                    ev |= B_OK;
                else if (on_selector(in.my, BOARD_ROW))
                    board_d = in.mx < 128 ? -1 : 1;     /* "<" y ">" del selector */
                else if (on_selector(in.my, LEVEL_ROW))
                    level_d = in.mx < 128 ? -1 : 1;
            }
        }
        if (in.wheel) {                         /* la rueda cambia el selector sobre el que está el puntero */
            int d = in.wheel > 0 ? 1 : -1;
            if (on_selector(in.my, BOARD_ROW))
                board_d = d;
            else
                level_d = d;
        }
        if (in.dy) {
            menu_pos = (menu_pos + in.dy + menu_n + 2) % (menu_n + 2);
            focus_mouse = 0;                    /* el foco vuelve al teclado: el selector sí lleva '>' */
            sound(SND_SELECT);
            dirty = 1;
        }
        if (in.dx) {                            /* izquierda y derecha: el tablero si está enfocado; si no, el nivel */
            if (menu_pos == menu_n + 1)
                board_d = in.dx;
            else
                level_d = in.dx;
        }
        if (level_d) {
            level += level_d;
            level = level < 1 ? 1 : level > 4 ? 4 : level;
            sound(SND_SELECT);
            dirty = 1;
        }
        if (board_d) {
            board_sel += board_d;
            board_sel = board_sel < 0 ? 0 : board_sel > N_BOARDS - 1 ? N_BOARDS - 1 : board_sel;
            sound(SND_SELECT);
            dirty = 1;
        }
        if (ev & B_BACK)
            return 0;
        if (ev & B_OK) {
            if (menu_pos == menu_n + 1) {       /* Intro en el selector de tablero: el siguiente */
                board_sel = (board_sel + 1) % N_BOARDS;
                sound(SND_SELECT);
                dirty = 1;
            } else if (menu_pos == menu_n) {    /* e Intro en el de nivel: el siguiente */
                level = level % 4 + 1;
                sound(SND_SELECT);
                dirty = 1;
            } else {
                switch (menu_items[menu_pos]) {
                case M_QUIT: return 0;
                case M_CONTINUE: break;
                case M_WHITE: new_game(MODE_WHITE); break;
                case M_BLACK: new_game(MODE_BLACK); break;
                default: new_game(MODE_TWO); break;
                }
                dirty = 1;
                sound(SND_MOVE);
                return 1;
            }
        }
        sound_tick();
        if (dirty) {
            present(draw_menu);
            dirty = 0;
        } else {
            zxv_wait_vblank();
        }
    }
}

/* --- Jugar --------------------------------------------------------------------- */

static void move_cursor(int dx, int dy)
{
    int f = SQ_FILE(cursor), r = SQ_RANK(cursor);

    f += flipped ? -dx : dx;
    r += flipped ? dy : -dy;
    f = f < 0 ? 0 : f > 7 ? 7 : f;
    r = r < 0 ? 0 : r > 7 ? 7 : r;
    cursor = SQ(f, r);
    dirty = 1;
}

static void click(void)
{
    const Pos *p = cur();
    int pc = p->sq[cursor];

    if (sel != NO_SQ && is_target(cursor)) {
        Move first = { 0, 0, 0, 0 };
        for (int i = 0; i < nsel; i++) {
            if (sel_moves[i].to == cursor) {
                first = sel_moves[i];
                break;
            }
        }
        if (first.promo) {                      /* hay que elegir la corona */
            promo_on = 1;
            promo_idx = 0;
            dirty = 1;
        } else {
            apply_move(first);
        }
        return;
    }
    if (pc && (pc > 0) == (p->side > 0)) {      /* una pieza propia: se elige */
        select_square(cursor == sel ? NO_SQ : cursor);
        sound(SND_SELECT);
    } else if (sel != NO_SQ) {
        select_square(NO_SQ);
        sound(SND_NO);
    } else {
        sound(SND_NO);
    }
    dirty = 1;
}

/* El ratón en la partida: traduce el puntero y los botones a lo mismo que harían
 * el cursor y las teclas. */
static void mouse_game(const input_t *in, unsigned *ev)
{
    if (in->ev & B_RCLICK)
        *ev |= B_BACK;
    if (promo_on) {                             /* la rejilla de coronación */
        int hit = promo_hit(in->mx, in->my);
        if (hit >= 0) {
            if (hit != promo_idx && (in->moved || (in->ev & B_CLICK))) {
                promo_idx = hit;
                dirty = 1;
            }
            if (in->ev & B_CLICK)
                *ev |= B_OK;
        }
        return;
    }
    if (in->mx < PANEL_X) {                     /* el tablero: el cursor sigue al puntero */
        int sq = sq_at(in->mx, in->my);
        if (sq != NO_SQ && sq != cursor && (in->moved || (in->ev & B_CLICK))) {
            cursor = sq;
            dirty = 1;
        }
        if ((in->ev & B_CLICK) && sq != NO_SQ)
            *ev |= B_OK;
    } else if (in->ev & B_CLICK) {              /* el panel: sus ayudas son botones */
        switch (in->my / 8) {
        case BTN_UNDO: *ev |= B_UNDO; break;
        case BTN_NEW: *ev |= B_NEW; break;
        case BTN_MENU: *ev |= B_MENU; break;
        default: break;
        }
    }
}

/* Una partida. Vuelve cuando se pide el menú. */
static void play(void)
{
    input_t in;

    input_read(&in);
    dirty = 1;
    for (;;) {
        input_read(&in);
        unsigned ev = in.ev;
        mouse_game(&in, &ev);

        if (ev & B_MENU)
            return;
        if (confirm_new && --confirm_new == 0)
            dirty = 1;                          /* se acabó el tiempo para confirmar */
        if (ev & B_NEW) {
            if (confirm_new || over() || nhist <= 1) {
                new_game(mode);
                confirm_new = 0;
                sound(SND_MOVE);
            } else {
                confirm_new = 150;              /* tres segundos para pulsar N otra vez */
                dirty = 1;
                sound(SND_NO);
            }
        }

        int ai_turn = ai_side && !over() && cur()->side == ai_side;
        if (ai_turn) {
            if (!thinking) {                    /* primero se enseña "Pensando" */
                thinking = 1;
                dirty = 1;
            } else if (!dirty && !snd_next) {   /* y se deja acabar el sonido: pensando no se llama a sound_tick */
                ai_move();
                input_read(&in);                /* lo pulsado mientras pensaba no cuenta */
            }
        } else {
            if (ev & B_UNDO)
                undo();
            if (promo_on) {
                if (in.dx || in.dy) {
                    promo_idx ^= in.dx ? 1 : 2;
                    dirty = 1;
                }
                if (ev & B_BACK) {
                    promo_on = 0;
                    dirty = 1;
                } else if (ev & B_OK) {
                    Move m;
                    if (find_move(cur(), sel, cursor, PROMO_PIECE[promo_idx], &m)) {
                        promo_on = 0;
                        apply_move(m);
                    }
                }
            } else {
                if (in.dx || in.dy)
                    move_cursor(in.dx, in.dy);
                if (ev & B_BACK) {
                    if (over()) {
                        return;
                    }
                    if (sel != NO_SQ) {
                        select_square(NO_SQ);
                        dirty = 1;
                    }
                }
                if ((ev & B_OK) && !over())
                    click();
            }
        }

        sound_tick();
        if (dirty) {
            present(draw_game);
            dirty = 0;
        } else {
            zxv_wait_vblank();
        }
    }
}

int main(void)
{
    es = zxv_lang_get() != ZXV_LANG_EN;
    make_pieces();
    zxv_mouse_init(&mouse, 128, 96);
    uint8_t border = REG8(ZXV_V_BORDER);     /* el del BASIC, para volver */
    REG8(ZXV_V_BORDER) = 0;
    zxv_set_page(0);
    zxv_gfx_draw_page(0);
    visible = 0;

    while (menu())
        play();

    /* Todo recogido para volver al BASIC. */
    zxv_ay_write(ZXV_AY2 + 8, 0);
    zxv_gfx_draw_page(0);
    zxv_set_page(0);
    zxv_text_colors(0, 7);
    REG8(ZXV_V_BORDER) = border;
    return 0;
}
