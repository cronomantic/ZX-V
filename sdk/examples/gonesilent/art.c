/*
 * art.c - Los sprites de las criaturas de Gone Silent: los de gonesilent.gfx
 * (que hace mkcreature.py y se puede retocar con zxv-gfx), comprimidos por
 * mkspr.py en build/gonesilent.spr. Van dentro del programa y se dibujan
 * desde ahí mismo, sin descomprimirlos (render.c).
 *
 * En cada postura (SPR_WALK_A...), cinco ángulos: de frente, 3/4 de frente,
 * de lado (con la cámara a su derecha), 3/4 de espaldas y de espaldas; los
 * otros tres son esos en espejo. La muerte, tres sprites sin ángulo. Y la
 * incubadora (SPR_MOTHER...), más grande y solo de frente. Y los objetos que
 * se cogen (SPR_CARD...).
 */
#include "gonesilent.h"

__asm__(".section .rodata.creature_spr, \"a\"\n.balign 4\ncreature_spr:\n"
        ".incbin \"build/gonesilent.spr\"\n.previous\n");
extern const uint8_t creature_spr[];

#define SPRITES     43

static const uint8_t *sprites[SPRITES];

void art_init(void)
{
    const uint8_t *f = creature_spr;
    int n = f[0] | f[1] << 8;
    for (int k = 0; k < SPRITES && k < n; k++) {
        const uint8_t *p = f + 2 + 4 * k;
        uint32_t at = p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
        sprites[k] = at ? f + at : 0;
    }
}

const uint8_t *art_sprite(int n) { return n >= 0 && n < SPRITES ? sprites[n] : 0; }

void art_icon(int n, int x, int y, int step, const uint8_t remap[16])
{
    const uint8_t *s = art_sprite(n);
    if (!s)
        return;
    for (int r = 0; r < s[1]; r += step) {
        const uint8_t *p = s + (s[2 + 2 * r] | s[3 + 2 * r] << 8);
        for (int g = *p++; g > 0; g--) {
            int x0 = p[0], len = p[1];
            p += 2;
            for (int k = 0; k < len; k++)
                if ((x0 + k) % step == 0)
                    rect(x + (x0 + k) / step, y + r / step, 1, 1, remap[(p[k >> 1] >> ((k & 1) * 4)) & 15]);
            p += (len + 1) >> 1;
        }
    }
}
