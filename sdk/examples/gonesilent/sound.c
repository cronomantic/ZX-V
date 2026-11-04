/*
 * sound.c - El sonido de Gone Silent, con la biblioteca del SDK (zxv_music.h):
 *
 *   - La música, en el primer AY: gonesilent-title.pt3 (el título y la
 *     introducción) y gonesilent-end.pt3 (el final).
 *   - Los efectos, en el segundo AY, y el ambiente (el viento de las
 *     tormentas, el zumbido de la planta), en el tercer canal del primero
 *     mientras no suena la música: el banco AYFX gonesilent.afb, en el orden
 *     de FX_SHOT... (gonesilent.h).
 *
 * Con la música, el estéreo ABC; jugando, mono, para que los efectos y el
 * ambiente no suenen solo por un lado.
 *
 * Los hace mksound.py y se pueden retocar con zxv-tracker. El programa los
 * lleva dentro y, si en la carpeta desde la que se carga hay alguno con el
 * mismo nombre que valga, usa ese.
 */
#include "zxv_music.h"

#include "gonesilent.h"

/* Los de dentro del programa. */
#define EMBED(sym, file)                                                        \
    __asm__(".section .rodata." #sym ", \"a\"\n.balign 4\n" #sym ":\n"           \
            ".incbin \"examples/gonesilent/" file "\"\n" #sym "_end:\n.previous\n"); \
    extern const uint8_t sym[], sym##_end[]
EMBED(title_pt3, "gonesilent-title.pt3");
EMBED(end_pt3, "gonesilent-end.pt3");
EMBED(fx_afb, "gonesilent.afb");

#define AMBIENT     2                   /* el canal del ambiente: el C del primer AY */

enum { F_TITLE, F_END, F_BANK, FILES };

static struct {
    const char *file;
    const uint8_t *data, *end;
    uint32_t size;
} files[FILES] = {
    [F_TITLE] = { "gonesilent-title.pt3", title_pt3, title_pt3_end, 0 },
    [F_END] = { "gonesilent-end.pt3", end_pt3, end_pt3_end, 0 },
    [F_BANK] = { "gonesilent.afb", fx_afb, fx_afb_end, 0 },
};

/* Los que se cargan de la tarjeta, uno detrás de otro. */
static uint32_t card[8 * 1024 / 4];

static int song_now = SONG_NONE, ambient_now = -1;
static uint8_t saved_stereo;            /* el de antes, para dejarlo al salir */

void sound_init(void)
{
    saved_stereo = *(volatile uint8_t *)(uintptr_t)ZXV_S_AY_STEREO;
    uint8_t *at = (uint8_t *)card, *end = (uint8_t *)card + sizeof(card);
    for (int i = 0; i < FILES; i++) {
        files[i].size = (uint32_t)(files[i].end - files[i].data);
        int n = zxv_file_load(files[i].file, at, (uint32_t)(end - at));
        zxv_music_info_t info;
        if (n <= 0 || (i != F_BANK && zxv_music_info(at, (uint32_t)n, &info) != ZXV_MUSIC_PT3))
            continue;                   /* no está, o no vale: el de dentro */
        files[i].data = at;
        files[i].size = (uint32_t)n;
        at += (n + 3) & ~3;
    }
    if (zxv_sfx_bank(files[F_BANK].data, files[F_BANK].size) <= 0)
        zxv_sfx_bank(fx_afb, (uint32_t)(fx_afb_end - fx_afb));
    zxv_sfx_channels(7 << 3);           /* los efectos, en el segundo AY */
}

static void stereo(int mode) { *(volatile uint8_t *)(uintptr_t)ZXV_S_AY_STEREO = (uint8_t)mode; }

void sound_song(int s)
{
    if (s == song_now)
        return;
    song_now = s;
    if (s == SONG_NONE) {
        zxv_music_stop();
        stereo(ZXV_AY_STEREO_MONO);
        return;
    }
    stereo(ZXV_AY_STEREO_ABC);
    sound_ambient(-1);                  /* la música usa su canal */
    const int f = s == SONG_TITLE ? F_TITLE : F_END;
    zxv_music_play(files[f].data, files[f].size, ZXV_MUSIC_LOOP);
}

void sound_fx(int fx) { zxv_sfx_play(fx, ZXV_SFX_AUTO); }

void sound_ambient(int fx)
{
    if (fx < 0) {
        if (ambient_now >= 0)
            zxv_sfx_stop(AMBIENT);
        ambient_now = -1;
        return;
    }
    /* Vuelve a empezar al acabar: un ambiente sin cortes. */
    if (fx != ambient_now || !zxv_sfx_playing(AMBIENT)) {
        zxv_sfx_play(fx, AMBIENT);
        ambient_now = fx;
    }
}

void sound_frame(void) { zxv_music_update(); }

void sound_done(void)
{
    zxv_music_stop();
    zxv_sfx_stop(ZXV_SFX_AUTO);
    zxv_music_update();
    stereo(saved_stereo);
}
