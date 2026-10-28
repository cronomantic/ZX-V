/*
 * hello.c - Primer programa nativo de ZX-V.
 *
 * Dibuja un degradado con los 15 colores, saluda, toca una nota en el AY y
 * espera una tecla. Saluda en el idioma de la máquina (LANGUAGE). Devuelve 0:
 * ha acabado bien (otro número sería un error, el informe W del BASIC).
 */
#include "zxv_api.h"

#define VRAM0   ((volatile uint8_t *)(uintptr_t)ZXV_VRAM0_BASE)

/* Escribe directamente en la VRAM (modo 0: 128 bytes por línea, nibble bajo
 * = píxel par). La página visible no se toca mientras se está enviando a la
 * pantalla (spec §4.8): se espera al VBLANK y a que acabe la transferencia. */
static void bands(void)
{
    zxv_wait_vblank();
    while (*(volatile uint32_t *)(uintptr_t)ZXV_SV_SYSFLAGS & ZXV_SYSF_LINK_BUSY)
        ;
    for (int y = 64; y < 192; y++) {
        for (int x = 0; x < 256; x += 2) {
            uint8_t c = (uint8_t)(((x + y) / 16) % 15 + 1);
            VRAM0[y * 128 + x / 2] = (uint8_t)(c | (c << 4));
        }
    }
}

static void note(void)
{
    zxv_ay_write(0, 168);           /* La: 659 Hz en el canal A */
    zxv_ay_write(1, 0);
    zxv_ay_write(7, 0x3E);
    zxv_ay_write(8, 0x10);
    zxv_ay_write(11, 0x00);
    zxv_ay_write(12, 0x0C);         /* envolvente de ~0,5 s */
    zxv_ay_write(13, 0x09);
}

int main(void)
{
    zxv_cls(7);
    if (zxv_lang_get() == ZXV_LANG_ES)
        zxv_print_str("¡Hola desde un programa .ZXV!\n\nPulsa una tecla para volver.");
    else
        zxv_print_str("Hello from a .ZXV program!\n\nPress a key to go back.");
    bands();
    note();

    while (zxv_key_get() < 0)
        zxv_wait_vblank();
    return 0;
}
