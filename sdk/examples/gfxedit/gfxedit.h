/*
 * gfxedit.h - El editor de gráficos de ZX-V: lo que comparten sus partes.
 *
 *   store.c  los gráficos en memoria, el fichero .gfx y deshacer
 *   ui.c     la pantalla: texto, botones, el puntero, los diálogos
 *   edit.c   los sprites, los tiles y las texturas
 *   map.c    los mapas de tiles
 *   main.c   las pestañas, los ficheros y el bucle
 *
 * La pantalla es la del modo 1 (512 x 384, 64 x 48 caracteres de 8 x 8): a
 * la izquierda el lienzo, a la derecha el panel y abajo el estado.
 */
#ifndef GFXEDIT_H
#define GFXEDIT_H

#include "store.h"
#include "zxv_gfx.h"

#define SW          512
#define SH          384

/* El lienzo y el panel, en píxeles y en caracteres. */
#define CX          0
#define CY          24
#define CW          320
#define CH          320
#define PCOL        41                  /* primera columna del panel */
#define PROW        3

/* Los colores de la interfaz. */
#define C_BG        7
#define C_INK       0
#define C_BTN       5
#define C_ON        1
#define C_TITLE     1

enum { TAB_SPR, TAB_T8, TAB_T16, TAB_MAP, TAB_TEX, N_TABS };

extern int es;                          /* en español */
#define TR(en, sp)  (es ? (sp) : (en))

extern int tab;
extern int dirty;                       /* hay que volver a dibujar todo */
extern int modified;                    /* hay cambios sin guardar */
extern int color_l, color_r;            /* los colores de los dos botones */
extern zxv_mouse_t mouse;
extern int press, release;              /* botones que se acaban de pulsar o soltar */

/* --- ui.c ------------------------------------------------------------------ */

char *num(char *p, int v);
char *cat(char *p, const char *s);
void text(int col, int row, int ink, int paper, const char *s);
/* El texto en w columnas, relleno con espacios. */
void field(int col, int row, int w, int ink, int paper, const char *s);
/* Un botón (id >= 0) con su texto; on lo marca. Da su ancho. */
int button(int col, int row, const char *label, int id, int on);
/* El botón bajo el puntero, o -1. */
int button_at(int x, int y);
void buttons_clear(void);
void message(const char *s);
const char *message_text(void);

/* La página en la que se dibuja y la que se ve. */
void page_begin(void);
void page_end(void);
void wait_frame(void);                  /* el frame siguiente, con la página ya mandada */
void pointer_hide(void);
void pointer_show(void);

/* Diálogos (esperan a que se conteste): */
int ask(const char *question);                      /* sí (1) o no (0) */
int input(const char *prompt, char *buf, int max);  /* 1 con Enter, 0 con Esc */
int choose_file(char *name, int max);               /* un .gfx de la tarjeta */

/* Un búfer que usa el dibujo y, fuera de él, lo demás (girar). */
#define WORK_SIZE   (8 + 160 * 20 * 8)
extern uint8_t work[WORK_SIZE];

/* Dibuja un bitmap en (x, y), ampliado z veces, desde (ox, oy), sin pasar
 * de w x h píxeles de pantalla; con transp, el 8 deja ver un damero. */
void draw_image(const img_t *im, int x, int y, int w, int h, int z, int ox, int oy, int transp);
void checker(int x, int y, int w, int h);
/* El rectángulo de un color, con el borde de otro (-1: sin borde). */
void box(int x, int y, int w, int h, int fill, int border);

/* --- edit.c y map.c -------------------------------------------------------- */

void edit_draw(void);
void edit_click(int id, int right);
void edit_mouse(void);
void edit_key(int k);
void edit_tick(void);                   /* cada frame: la animación */
int edit_slot(void);                    /* el objeto que se edita (-1 en los mapas) */

void map_draw_tab(void);
void map_click(int id, int right);
void map_mouse(void);
void map_key(int k);

/* --- Los botones ------------------------------------------------------------- */

enum {
    B_TAB = 0,                          /* + pestaña */
    B_NEW = 10, B_LOAD, B_SAVE, B_QUIT, B_UNDO,
    B_PREV = 20, B_NEXT, B_CREATE, B_DELETE, B_WMINUS, B_WPLUS, B_HMINUS, B_HPLUS,
    B_TOOL = 30,                        /* + herramienta */
    B_COLOR = 40,                       /* + color */
    B_FLIPH = 60, B_FLIPV, B_ROTATE, B_LEFT, B_RIGHT, B_UP, B_DOWN, B_CLEAR, B_COPY, B_PASTE,
    B_ZOOMOUT = 70, B_ZOOMIN, B_ANIM, B_FRMINUS, B_FRPLUS, B_SPMINUS, B_SPPLUS, B_WRAP, B_OUTMINUS,
    B_OUTPLUS, B_GRID, B_TILESIZE,
    B_FLAG = 90,                        /* + bit de la celda (0-3) */
};

enum { T_PEN, T_LINE, T_RECT, T_FRECT, T_FILL, T_PICK, N_TOOLS };

#endif
