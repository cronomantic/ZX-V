/*
 * sound.c - El sonido de The Keep, con la biblioteca del SDK (zxv_music.h):
 *
 *   - La música, en el primer AY: módulos de Pro Tracker 3 que se editan con
 *     zxv-tracker. keep-title.pt3 (el título), keep-tower.pt3 (las plantas),
 *     keep-boss.pt3 (el hechicero), keep-win.pt3 (la victoria) y
 *     keep-over.pt3 (el final de la partida).
 *   - Los efectos, en el segundo AY: el banco AYFX keep.afb, también de
 *     zxv-tracker (o del AY Sound FX Editor), en el orden de FX_SWING...
 *     (keep.h).
 *
 * El programa los lleva dentro y, si en la carpeta desde la que se carga hay
 * alguno con el mismo nombre, usa ese.
 */
#include "zxv_music.h"

#include "keep.h"

/* Los de dentro del programa. */
#define EMBED(sym, file)                                                        \
    __asm__(".section .rodata." #sym ", \"a\"\n.balign 4\n" #sym ":\n"           \
            ".incbin \"examples/keep/" file "\"\n" #sym "_end:\n.previous\n");   \
    extern const uint8_t sym[], sym##_end[]
EMBED(title_pt3, "keep-title.pt3");
EMBED(tower_pt3, "keep-tower.pt3");
EMBED(boss_pt3, "keep-boss.pt3");
EMBED(win_pt3, "keep-win.pt3");
EMBED(over_pt3, "keep-over.pt3");
EMBED(fx_afb, "keep.afb");

static struct {
    const char *file;
    const uint8_t *data, *end;
    uint32_t size;
} songs[] = {
    [SONG_TITLE] = { "keep-title.pt3", title_pt3, title_pt3_end, 0 },
    [SONG_TOWER] = { "keep-tower.pt3", tower_pt3, tower_pt3_end, 0 },
    [SONG_BOSS] = { "keep-boss.pt3", boss_pt3, boss_pt3_end, 0 },
    [SONG_WIN] = { "keep-win.pt3", win_pt3, win_pt3_end, 0 },
    [SONG_OVER] = { "keep-over.pt3", over_pt3, over_pt3_end, 0 },
};

/* Lo que se carga de la tarjeta, uno detrás de otro. */
static uint32_t card[48 * 1024 / 4];

static int music_on = 1, muted, song_now;

/* El informe de un fichero de la tarjeta que no vale; vuelve al BASIC. */
static void bad_file(const char *file)
{
    static char msg[48];
    append(append(msg, file), T(" no vale", " is not valid"));
    zxv_report_error(0, msg);
}

void sound_init(void)
{
    uint8_t *at = (uint8_t *)card, *end = (uint8_t *)card + sizeof(card);
    for (int s = SONG_TITLE; s <= SONG_OVER; s++) {
        songs[s].size = (uint32_t)(songs[s].end - songs[s].data);
        int n = zxv_file_load(songs[s].file, at, (uint32_t)(end - at));
        zxv_music_info_t info;
        if (n == ZXV_E_RANGE || (n >= 0 && zxv_music_info(at, (uint32_t)n, &info) != ZXV_MUSIC_PT3))
            bad_file(songs[s].file);
        if (n >= 0) {
            songs[s].data = at;
            songs[s].size = (uint32_t)n;
            at += (n + 3) & ~3;
        }
    }
    const uint8_t *bank = fx_afb;
    uint32_t size = (uint32_t)(fx_afb_end - fx_afb);
    int n = zxv_file_load("keep.afb", at, (uint32_t)(end - at));
    if (n == ZXV_E_RANGE)
        bad_file("keep.afb");
    if (n >= 0) {
        bank = at;
        size = (uint32_t)n;
    }
    if (zxv_sfx_bank(bank, size) <= 0)
        bad_file("keep.afb");
    zxv_sfx_channels(7 << 3);           /* los efectos, en el segundo AY */
    *(volatile uint8_t *)(uintptr_t)ZXV_S_AY_STEREO = ZXV_AY_STEREO_ACB;
    song_now = SONG_NONE;
}

void sound_song(int s)
{
    if (s == song_now)
        return;
    song_now = s;
    if (s == SONG_NONE)
        zxv_music_stop();
    else
        zxv_music_play(songs[s].data, songs[s].size,
                       s == SONG_WIN || s == SONG_OVER ? 0 : ZXV_MUSIC_LOOP);
    zxv_music_pause(muted || !music_on);
}

int sound_song_now(void) { return song_now; }

void sound_fx(int fx)
{
    if (!muted)
        zxv_sfx_play(fx, ZXV_SFX_AUTO);
}

void sound_frame(void) { zxv_music_update(); }

void sound_music(int on)
{
    music_on = on;
    zxv_music_pause(muted || !music_on);
}

int sound_music_on(void) { return music_on; }

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
