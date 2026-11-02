/* zxv.h falso: solo lo que usa el juego, para ejecutarlo en el PC (ui_test.c). */
#ifndef ZXV_H
#define ZXV_H
#include <stdint.h>
#include <stddef.h>

extern uint8_t mock_keymap[32];
extern uint32_t mock_frames;
extern uint8_t mock_border;

extern uint8_t mock_vram[2][192 * 128];
#define ZXV_VRAM0_BASE  ((uintptr_t)mock_vram[0])
#define ZXV_VRAM1_BASE  ((uintptr_t)mock_vram[1])
#define ZXV_I_KEYMAP    ((uintptr_t)mock_keymap)
#define ZXV_SV_FRAMES   ((uintptr_t)&mock_frames)
#define ZXV_V_BORDER    ((uintptr_t)&mock_border)
#define ZXV_AY2         16

#define ZXV_JOY_RIGHT   0x0001u
#define ZXV_JOY_LEFT    0x0002u
#define ZXV_JOY_DOWN    0x0004u
#define ZXV_JOY_UP      0x0008u
#define ZXV_JOY_FIRE1   0x0010u
#define ZXV_JOY_FIRE2   0x0020u
#define ZXV_JOY_START   0x0080u
#define ZXV_JOY_SELECT  0x0100u

#define ZXV_E_NOENT     (-1)
#define ZXV_E_RANGE     (-15)

#define ZXV_LANG_EN     0x6e65u
#define ZXV_LANG_ES     0x7365u
#endif
