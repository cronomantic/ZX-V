/*
 * asmhello.S - Un programa nativo en ensamblador RISC-V, sin C.
 *
 * Limpia la pantalla, saluda en el idioma de la máquina (la variable del
 * sistema LANGUAGE) y mueve una estrella con el mando 1 (leyendo su
 * registro, JOY0) o con los cursores (la cola de teclas). Cualquier otra
 * tecla, o el fuego 1, vuelve al BASIC con 0: ha acabado bien.
 *
 * Enseña: ZXV_CALL para llamar a la API (zxv.h), leer un registro y una
 * variable del sistema, y guardar ra y s0-s2 como una función de C.
 */
#include "zxv.h"

#define COLS        32                  /* el texto del modo 0: 32 × 24 */
#define ROWS        24
#define STEP        4                   /* la estrella se mueve cada 4 imágenes */

/* Las teclas de los cursores en la cola: 0xF000 | su código HID (spec §6.4). */
#define K_RIGHT     0xF04F
#define K_LEFT      0xF050
#define K_DOWN      0xF051
#define K_UP        0xF052

    .section .rodata
msg_es:
    .asciz "Hola desde ensamblador.\n\nMueve la estrella con el mando\no con los cursores. Otra tecla\n(o el fuego) vuelve al BASIC."
msg_en:
    .asciz "Hello from assembler.\n\nMove the star with the gamepad\nor the cursor keys. Any other\nkey (or fire) returns to BASIC."

    .text
    .globl main
main:
    addi sp, sp, -16                    # ra, s0 (columna), s1 (fila), s2 (imágenes)
    sw ra, 12(sp)
    sw s0, 8(sp)
    sw s1, 4(sp)
    sw s2, 0(sp)

    li a0, 7
    ZXV_CALL(ZXV_API_CLS)               # cls(7)

    /* El idioma: la variable LANGUAGE es ZXV_LANG('e','s') = 'e' | 's' << 8
     * en castellano (ZXV_LANG no vale aquí: lleva conversiones de C). */
    li t1, ZXV_SV_LANG
    lw t1, 0(t1)
    li t2, 0x7365
    la a0, msg_en
    bne t1, t2, 1f
    la a0, msg_es
1:  ZXV_CALL(ZXV_API_PRINT_STR)

    li s0, COLS / 2
    li s1, 16
    li s2, 0
    call star_on

loop:
    ZXV_CALL(ZXV_API_WAIT_VBLANK)
    ZXV_CALL(ZXV_API_KEY_GET)           # a0 = tecla, o -1
    bltz a0, pad
    li t1, K_RIGHT
    beq a0, t1, right
    li t1, K_LEFT
    beq a0, t1, left
    li t1, K_DOWN
    beq a0, t1, down
    li t1, K_UP
    beq a0, t1, up
    j done                              # otra tecla: se acabó

pad:
    li t1, ZXV_I_JOY0                   # el registro del mando 1 (u16)
    lhu t1, 0(t1)
    andi t2, t1, ZXV_JOY_FIRE1
    bnez t2, done
    addi s2, s2, 1                      # las direcciones, cada STEP imágenes
    li t2, STEP
    bltu s2, t2, loop
    li s2, 0
    andi t2, t1, ZXV_JOY_RIGHT
    bnez t2, right
    andi t2, t1, ZXV_JOY_LEFT
    bnez t2, left
    andi t2, t1, ZXV_JOY_DOWN
    bnez t2, down
    andi t2, t1, ZXV_JOY_UP
    bnez t2, up
    j loop

right:
    li t1, COLS - 1
    bgeu s0, t1, loop
    call star_off
    addi s0, s0, 1
    j moved
left:
    beqz s0, loop
    call star_off
    addi s0, s0, -1
    j moved
down:
    li t1, ROWS - 1
    bgeu s1, t1, loop
    call star_off
    addi s1, s1, 1
    j moved
up:
    li t1, 7                            # sin subir al texto
    bleu s1, t1, loop
    call star_off
    addi s1, s1, -1
moved:
    call star_on
    j loop

done:
    li a0, 0                            # main devuelve 0
    lw s2, 0(sp)
    lw s1, 4(sp)
    lw s0, 8(sp)
    lw ra, 12(sp)
    addi sp, sp, 16
    ret

/* La estrella (o un espacio) en la columna s0, fila s1. Llaman a la API:
 * guardan ra. */
star_on:
    li a1, '*'
    j put
star_off:
    li a1, ' '
put:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw a1, 8(sp)
    mv a0, s0
    mv a1, s1
    ZXV_CALL(ZXV_API_TEXT_AT)           # text_at(columna, fila)
    lw a0, 8(sp)
    ZXV_CALL(ZXV_API_PRINT_CHAR)        # print_char(c)
    lw ra, 12(sp)
    addi sp, sp, 16
    ret
