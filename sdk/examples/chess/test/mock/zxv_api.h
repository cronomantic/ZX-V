#ifndef ZXV_API_H
#define ZXV_API_H
#include "zxv.h"
typedef struct {
    int x, y;
    int wheel;
    uint8_t buttons;
    uint16_t rx, ry;
    uint8_t rw;
} zxv_mouse_t;
void zxv_mouse_init(zxv_mouse_t *p, int x, int y);
void zxv_mouse_update(zxv_mouse_t *p, int w, int h);
void zxv_wait_vblank(void);
void zxv_set_page(int page);
void zxv_cls(int color);
void zxv_print_str(const char *s);
void zxv_text_colors(int ink, int paper);
void zxv_text_at(int col, int row);
int zxv_key_get(void);
uint32_t zxv_joy_read(int n);
void zxv_ay_write(int reg, int val);
uint32_t zxv_lang_get(void);
int zxv_file_load(const char *name, void *addr, uint32_t max);
void zxv_report_error(int code, const char *msg);
#endif
