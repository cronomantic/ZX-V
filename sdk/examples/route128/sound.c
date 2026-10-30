/*
 * sound.c - El sonido de Route 128, con los dos AY (TurboSound, spec §5):
 *
 *   - Primer AY: la música, con la biblioteca del SDK (zxv_music.h). Son
 *     módulos de Pro Tracker 3 que se editan con zxv-tracker:
 *     route128-race.pt3 (la carrera), -title (el título), -goal (la meta) y
 *     -over (se acabó el tiempo). El programa los lleva dentro y, si en la
 *     carpeta desde la que se carga hay alguno, usa ese.
 *   - Segundo AY: el coche. Canales A y B, el motor (dos tonos, uno una
 *     octava más grave, que siguen a las vueltas); C, el ruido del derrape y
 *     de la cuneta, y encima los golpes y los avisos.
 *
 * El coche va a pasos de un cincuentavo de segundo (sound_step): lo que duran
 * los efectos, en esos pasos. La música lleva su propio reloj.
 */
#include "zxv_music.h"

#include "route128.h"

#define AY2         ZXV_AY2

/* Periodos del AY para las notas MIDI 36 (do 2) a 96 (do 7). */
static const uint16_t period[61] = {
    1695, 1600, 1510, 1425, 1345, 1270, 1198, 1131, 1068, 1008, 951, 898, 847, 800, 755, 713,
    673, 635, 599, 566, 534, 504, 476, 449, 424, 400, 377, 356, 336, 317, 300, 283, 267, 252,
    238, 224, 212, 200, 189, 178, 168, 159, 150, 141, 133, 126, 119, 112, 106, 100, 94, 89, 84,
    79, 75, 71, 67, 63, 59, 56, 53,
};

/* --- Los registros del segundo AY, sin repetir escrituras -------------------- */

static uint8_t cache[32];
static int muted;

static void ay(int r, int v)
{
    if (cache[r] != (uint8_t)v || r == AY2 + 13) {     /* R13 reinicia la envolvente */
        cache[r] = (uint8_t)v;
        zxv_ay_write(r, v);
    }
}

static void tone(int base, int ch, int per)
{
    ay(base + ch * 2, per & 0xFF);
    ay(base + ch * 2 + 1, (per >> 8) & 0x0F);
}

/* --- La música --------------------------------------------------------------- */

/* Los módulos de dentro del programa. */
#define EMBED(sym, file)                                                        \
    __asm__(".section .rodata." #sym ", \"a\"\n.balign 4\n" #sym ":\n"           \
            ".incbin \"examples/route128/" file "\"\n" #sym "_end:\n.previous\n"); \
    extern const uint8_t sym[], sym##_end[]
EMBED(race_pt3, "route128-race.pt3");
EMBED(title_pt3, "route128-title.pt3");
EMBED(goal_pt3, "route128-goal.pt3");
EMBED(over_pt3, "route128-over.pt3");

static struct {
    const char *file;
    const uint8_t *data;
    uint32_t size;
} songs[] = {
    [SONG_RACE] = { "route128-race.pt3", race_pt3, 0 },
    [SONG_TITLE] = { "route128-title.pt3", title_pt3, 0 },
    [SONG_GOAL] = { "route128-goal.pt3", goal_pt3, 0 },
    [SONG_OVER] = { "route128-over.pt3", over_pt3, 0 },
};

static int music_on = 1;

/* El informe de un módulo de la tarjeta que no vale; vuelve al BASIC. */
static void bad_song(const char *file)
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
 * dentro. Uno que no cabe o no es música da un informe y vuelve al BASIC. */
static void load_songs(void)
{
    static const uint8_t *const ends[] = {
        [SONG_RACE] = race_pt3_end, [SONG_TITLE] = title_pt3_end,
        [SONG_GOAL] = goal_pt3_end, [SONG_OVER] = over_pt3_end,
    };
    uint8_t *at = MUSIC_MEM;
    for (int s = SONG_RACE; s <= SONG_OVER; s++) {
        songs[s].size = (uint32_t)(ends[s] - songs[s].data);
        int n = zxv_file_load(songs[s].file, at, (uint32_t)(MUSIC_MEM + MUSIC_ROOM - at));
        zxv_music_info_t info;
        if (n == ZXV_E_RANGE || (n >= 0 && zxv_music_info(at, (uint32_t)n, &info) != ZXV_MUSIC_PT3))
            bad_song(songs[s].file);
        if (n >= 0) {
            songs[s].data = at;
            songs[s].size = (uint32_t)n;
            at += (n + 3) & ~3;
        }
    }
}

void sound_song(int s)
{
    if (s == SONG_NONE)
        zxv_music_stop();
    else
        zxv_music_play(songs[s].data, songs[s].size, s == SONG_RACE || s == SONG_TITLE ? ZXV_MUSIC_LOOP : 0);
    zxv_music_pause(muted || !music_on);
}

void sound_music(int on)
{
    music_on = on;
    zxv_music_pause(muted || !music_on);
}

int sound_music_on(void) { return music_on; }

/* --- El coche ------------------------------------------------------------------ */

static int eng_rpm, eng_skid, eng_off;
static int fx, fx_t;

void sound_engine(int rpm, int skid, int offroad)
{
    eng_rpm = rpm;
    eng_skid = skid;
    eng_off = offroad;
}

void sound_fx(int f)
{
    fx = f + 1;
    fx_t = 0;
}

static void car_frame(void)
{
    /* El motor: de unos 40 Hz al ralentí a 230 a tope. */
    int p = 2800 - eng_rpm * 23 / 10;
    if (p < 480)
        p = 480;
    p += (int)(rnd() & 7);              /* un poco de aspereza */
    tone(AY2, 0, p);
    tone(AY2, 1, p * 2 + 9);
    int v = eng_rpm > 0 ? 10 + eng_rpm / 400 : 7;
    int mixer = 0x3C;                   /* tonos A y B; C callado */
    int cv = 0;
    if (fx) {
        int t = fx_t++;
        switch (fx - 1) {
        case FX_CRASH:                  /* un estruendo que se apaga */
            ay(AY2 + 6, 20 + (t & 7));
            cv = 15 - t / 5;
            mixer = 0x18 | 0x04;
            if (cv <= 0)
                fx = 0;
            break;
        case FX_SHIFT:                  /* el chasquido de la palanca */
            ay(AY2 + 6, 2);
            cv = 11 - t * 4;
            mixer = 0x18 | 0x04;
            if (cv <= 0)
                fx = 0;
            break;
        case FX_BUMP:
            ay(AY2 + 6, 12);
            cv = 13 - t * 2;
            mixer = 0x18 | 0x04;
            if (cv <= 0)
                fx = 0;
            break;
        case FX_CHECKPOINT: {           /* un arpegio que sube */
            static const uint8_t arp[4] = { 72, 76, 79, 84 };
            int n = arp[(t / 4) & 3] + (t / 16) * 12;
            if (n > 96)
                n = 96;
            tone(AY2, 2, period[n - 36]);
            cv = 13;
            mixer = 0x38;
            if (t > 40)
                fx = 0;
            break;
        }
        case FX_BEEP:
        case FX_GO:
            tone(AY2, 2, fx - 1 == FX_GO ? period[84 - 36] : period[72 - 36]);
            cv = 14;
            mixer = 0x38;
            if (t > (fx - 1 == FX_GO ? 30 : 12))
                fx = 0;
            break;
        }
    } else if (eng_off) {               /* la cuneta: un traqueteo grave */
        ay(AY2 + 6, 28);
        cv = 6 + (int)(rnd() % (unsigned)(eng_off / 2 + 1));
        mixer = 0x18 | 0x04;
    } else if (eng_skid) {              /* el derrape: un siseo agudo */
        ay(AY2 + 6, 3);
        cv = eng_skid;
        mixer = 0x18 | 0x04;
    }
    ay(AY2 + 7, mixer);
    ay(AY2 + 8, muted ? 0 : v);
    ay(AY2 + 9, muted ? 0 : v - 3);
    ay(AY2 + 10, muted ? 0 : (cv > 15 ? 15 : cv < 0 ? 0 : cv));
}

/* --- Todo ------------------------------------------------------------------------- */

void sound_init(void)
{
    *(volatile uint8_t *)(uintptr_t)ZXV_S_AY_STEREO = ZXV_AY_STEREO_ACB;
    for (int r = 0; r < 32; r++)
        cache[r] = 0xFF;
    for (int r = 0; r < 14; r++)
        ay(AY2 + r, r == 7 ? 0x3F : 0);
    load_songs();
    fx = 0;
    eng_rpm = -1;
}

/* Un paso del coche: los efectos van a 50 por segundo, como se escribieron,
 * a cualquier ritmo de la pantalla. */
static void sound_step(void)
{
    if (eng_rpm < 0 && !fx) {           /* sin coche (el título): callado */
        ay(AY2 + 8, 0);
        ay(AY2 + 9, 0);
        ay(AY2 + 10, 0);
        return;
    }
    car_frame();
}

void sound_frame(void)
{
    static int acc;
    for (acc += 50; acc >= ZXV_FRAME_HZ; acc -= ZXV_FRAME_HZ)
        sound_step();
    zxv_music_update();
}

void sound_mute(int on)
{
    muted = on;
    zxv_music_pause(muted || !music_on);
}

/* Todo callado, para volver al BASIC: la música sale por delante (hasta
 * ZXV_MUSIC_AHEAD_US), así que se espera a que salga su silencio. */
void sound_off(void)
{
    zxv_music_stop();
    for (int i = 0; i < 4; i++) {
        zxv_music_update();
        zxv_wait_vblank();
    }
    for (int r = 8; r < 11; r++) {
        zxv_ay_write(r, 0);
        ay(AY2 + r, 0);
    }
    zxv_ay_write(7, 0x3F);
    ay(AY2 + 7, 0x3F);
}
