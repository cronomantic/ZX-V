/*
 * store.h - Los gráficos del editor: sprites, tiles, mapas y texturas en
 * una zona de memoria, y su lectura y escritura en el formato .gfx del
 * BASIC (LOAD "x" GRAPHICS). Sin la API de la máquina: también se compila
 * en el PC para las pruebas.
 *
 * Los sprites, los tiles y las texturas se guardan como bitmaps de 4 bits
 * (nibble bajo = píxel par, filas alineadas a 4 bytes), así que el motor
 * gráfico los dibuja tal cual. Un juego de tiles es un bitmap del ancho de
 * un tile y tan alto como todos sus tiles, uno debajo de otro.
 */
#ifndef GFXEDIT_STORE_H
#define GFXEDIT_STORE_H

#include <stddef.h>
#include <stdint.h>

/* De zxvlib.c (o de la biblioteca de C, en el PC). */
void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *d, const void *s, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);

#define TRANSP          8               /* el color transparente */

#define N_SPRITES       256
#define N_MAPS          16
#define N_TEXTURES      4
#define MAX_TILES       4096

/* Los objetos, por su hueco. */
#define SLOT_SPRITE     0               /* + número (0-255) */
#define SLOT_TILES8     256
#define SLOT_TILES16    257
#define SLOT_MAP        258             /* + número (0-15) */
#define SLOT_TEX        274             /* + número (0-3) */
#define N_SLOTS         278

#define POOL_SIZE       (112u * 1024u)

typedef struct {
    uint32_t off, len;                  /* en la zona; len 0: no existe */
    uint16_t w, h;                      /* sprite y textura: píxeles; tiles: lado y
                                         * tiles; mapa: celdas */
    uint8_t a, b, c;                    /* mapa: lado del tile y si se repite;
                                         * textura: log2 del lado, si se repite y
                                         * el color de fuera */
} obj_t;

extern obj_t objs[N_SLOTS];

/* Bytes por fila de un bitmap de w píxeles: múltiplo de 4. */
static inline uint32_t stride_of(uint32_t w) { return (w + 7) / 8 * 4; }

uint8_t *obj_data(int slot);
int obj_exists(int slot);
uint32_t pool_free(void);
/* Cambia el tamaño de los datos (lo que había se queda al principio y lo
 * nuevo, a 0). 0, o -1 si no cabe. */
int obj_resize(int slot, uint32_t len);
void obj_delete(int slot);
void store_clear(void);

/* El hueco de los tiles de lado s (8 o 16). */
static inline int tiles_slot(int s) { return s == 8 ? SLOT_TILES8 : SLOT_TILES16; }

/* Crean el objeto (o le cambian el tamaño, conservando lo de arriba a la
 * izquierda; lo nuevo, transparente o a 0). 0, o -1 si no cabe. */
int sprite_size(int n, int w, int h);
int tiles_count(int s, int count);
int map_size(int m, int w, int h, int tile);
int texture_size(int t, int log2);

/* Un bitmap: un sprite, una textura o un tile. */
typedef struct {
    uint8_t *pix;
    uint32_t stride;
    int w, h;
} img_t;

int obj_image(int slot, int tile, img_t *im);
static inline int img_get(const img_t *im, int x, int y)
{
    uint8_t b = im->pix[y * im->stride + x / 2];
    return x & 1 ? b >> 4 : b & 15;
}
static inline void img_set(img_t *im, int x, int y, int c)
{
    uint8_t *p = &im->pix[y * im->stride + x / 2];
    *p = x & 1 ? (uint8_t)((*p & 0x0F) | c << 4) : (uint8_t)((*p & 0xF0) | (c & 15));
}

/* El fichero .gfx, por partes: io lee (o escribe) n bytes y da cuántos.
 * gfx_load añade los objetos del fichero (sustituyen a los del mismo
 * número, como LOAD ... GRAPHICS) y da 0 o un código: */
typedef int (*gfx_io_t)(void *buf, uint32_t n);
#define GFX_BAD         (-1)            /* no es un fichero de gráficos o está dañado */
#define GFX_FULL        (-2)            /* no cabe en la memoria */
int gfx_load(gfx_io_t read);
/* Escribe todo (con write NULL, solo calcula); en *size, los bytes del
 * fichero. 0, o -1 si falla una escritura. */
int gfx_save(gfx_io_t write, uint32_t *size);

/* --- Deshacer ---------------------------------------------------------------- */

/* Guarda cómo está un objeto entero (o, con len, solo esos bytes) antes
 * de cambiarlo; lo más antiguo se olvida si no cabe. */
void undo_push(int slot, uint32_t off, uint32_t len);
static inline void undo_push_all(int slot) { undo_push(slot, 0, 0xFFFFFFFFu); }
/* Lo deshace; da el hueco, o -1 si no hay nada. */
int undo_pop(void);
void undo_clear(void);

#endif
