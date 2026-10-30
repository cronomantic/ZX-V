/*
 * sound.c - El sonido de Umbra, con los dos AY (TurboSound, spec §5) y la
 * biblioteca de música del SDK (zxv_music.h):
 *
 *   - Primer AY: la música, módulos de Pro Tracker 3 que se editan con
 *     zxv-tracker: umbra-title.pt3 (el título), -maze (el laberinto),
 *     -clear (nivel superado) y -over (fin de la partida).
 *   - Segundo AY: los efectos, del banco AYFX umbra.afb (también se edita
 *     con zxv-tracker), por su número (FX_RUNE... en umbra.h). El canal A,
 *     lo que pasa (las runas, los frascos, los golpes, la puerta); el B, el
 *     latido de la lámpara que se apaga; el C, los pasos y el susurro de las
 *     sombras que se acercan.
 *
 * El programa lo lleva todo dentro y, si en la carpeta desde la que se carga
 * hay alguno de estos ficheros, usa ese.
 */
#include "zxv_music.h"

#include "umbra.h"

enum { CH_EVENT = 3, CH_HEART, CH_STEP };       /* los canales del segundo AY */

/* Lo de dentro del programa. */
#define EMBED(sym, file)                                                        \
    __asm__(".section .rodata." #sym ", \"a\"\n.balign 4\n" #sym ":\n"           \
            ".incbin \"examples/umbra/" file "\"\n" #sym "_end:\n.previous\n");   \
    extern const uint8_t sym[], sym##_end[]
EMBED(title_pt3, "umbra-title.pt3");
EMBED(maze_pt3, "umbra-maze.pt3");
EMBED(clear_pt3, "umbra-clear.pt3");
EMBED(over_pt3, "umbra-over.pt3");
EMBED(fx_afb, "umbra.afb");

static struct {
    const char *file;
    const uint8_t *data, *end;
    uint32_t size;
} files[SONGS + 1] = {
    [SONG_TITLE] = { "umbra-title.pt3", title_pt3, title_pt3_end, 0 },
    [SONG_MAZE] = { "umbra-maze.pt3", maze_pt3, maze_pt3_end, 0 },
    [SONG_CLEAR] = { "umbra-clear.pt3", clear_pt3, clear_pt3_end, 0 },
    [SONG_OVER] = { "umbra-over.pt3", over_pt3, over_pt3_end, 0 },
    [SONGS] = { "umbra.afb", fx_afb, fx_afb_end, 0 },  /* los efectos */
};

static int music_on = 1, muted;
static int heart, whisper, heart_t, whisper_t;

/* El informe de un fichero de la tarjeta que no vale; vuelve al BASIC. */
static void bad_file(const char *file)
{
    static char msg[48];
    int n = 0;
    while (*file)
        msg[n++] = *file++;
    for (const char *s = T(" no vale", " is not valid"); *s;)
        msg[n++] = *s++;
    msg[n] = 0;
    zxv_report_error(0, msg);
}

/* Los de la tarjeta, uno detrás de otro en MUSIC_MEM; si no hay, los de
 * dentro. Uno que no cabe o no vale da un informe y vuelve al BASIC. */
static void load_files(void)
{
    uint8_t *at = MUSIC_MEM;
    for (int s = SONG_TITLE; s <= SONGS; s++) {
        files[s].size = (uint32_t)(files[s].end - files[s].data);
        int n = zxv_file_load(files[s].file, at, (uint32_t)(MUSIC_MEM + MUSIC_ROOM - at));
        if (n == ZXV_E_RANGE)
            bad_file(files[s].file);
        if (n < 0)
            continue;
        zxv_music_info_t info;
        if (s < SONGS ? zxv_music_info(at, (uint32_t)n, &info) != ZXV_MUSIC_PT3
                      : zxv_sfx_bank(at, (uint32_t)n) < FX_COUNT)
            bad_file(files[s].file);
        files[s].data = at;
        files[s].size = (uint32_t)n;
        at += (n + 3) & ~3;
    }
    if (zxv_sfx_bank(files[SONGS].data, files[SONGS].size) < FX_COUNT)
        bad_file(files[SONGS].file);
}

void sound_song(int s)
{
    if (s == SONG_NONE)
        zxv_music_stop();
    else
        zxv_music_play(files[s].data, files[s].size, s == SONG_TITLE || s == SONG_MAZE ? ZXV_MUSIC_LOOP : 0);
    zxv_music_pause(muted || !music_on);
}

void sound_music(int on)
{
    music_on = on;
    zxv_music_pause(muted || !music_on);
}

int sound_music_on(void) { return music_on; }

void sound_fx(int fx)
{
    if (!muted)
        zxv_sfx_play(fx, fx == FX_STEP ? CH_STEP : CH_EVENT);
}

void sound_ambient(int h, int w)
{
    heart = h;
    whisper = w;
}

void sound_init(void)
{
    *(volatile uint8_t *)(uintptr_t)ZXV_S_AY_STEREO = ZXV_AY_STEREO_ACB;
    load_files();
    heart = whisper = 0;
}

void sound_frame(void)
{
    /* El latido: más deprisa cuanto menos aceite queda. */
    if (heart && !muted) {
        static const uint8_t period[4] = { 0, 60, 42, 27 };     /* en frames */
        if (++heart_t >= period[heart > 3 ? 3 : heart]) {
            heart_t = 0;
            zxv_sfx_play(FX_HEART, CH_HEART);
        }
    } else {
        heart_t = 0;
    }
    /* El susurro: uno detrás de otro, más fuerte cuanto más cerca. */
    if (whisper && !muted && ++whisper_t >= 12 && !zxv_sfx_playing(CH_STEP)) {
        whisper_t = 0;
        zxv_sfx_play(FX_WHISPER_1 + (whisper > 3 ? 3 : whisper) - 1, CH_STEP);
    }
    zxv_music_update();
}

void sound_mute(int on)
{
    muted = on;
    if (on)
        zxv_sfx_stop(ZXV_SFX_AUTO);
    zxv_music_pause(muted || !music_on);
}

/* Todo callado, para volver al BASIC: la música sale por delante (hasta
 * ZXV_MUSIC_AHEAD_US), así que se espera a que salga su silencio. */
void sound_off(void)
{
    zxv_music_stop();
    zxv_sfx_stop(ZXV_SFX_AUTO);
    for (int i = 0; i < 4; i++) {
        zxv_music_update();
        zxv_wait_vblank();
    }
    for (int r = 8; r < 11; r++) {
        zxv_ay_write(r, 0);
        zxv_ay_write(ZXV_AY2 + r, 0);
    }
    zxv_ay_write(7, 0x3F);
    zxv_ay_write(ZXV_AY2 + 7, 0x3F);
}
