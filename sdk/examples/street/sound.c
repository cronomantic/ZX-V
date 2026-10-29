/*
 * sound.c - El sonido de Street Justice, con los dos AY (TurboSound, spec §5):
 *
 *   - Primer AY: la música. Canal A la melodía, B el bajo y C la batería
 *     (bombo con tono, caja con ruido). Una nota por fila: 1 = sigue la
 *     anterior, 0 = silencio.
 *   - Segundo AY: los efectos. Canal A los tonos (daño, comida, saltos, el
 *     aviso de seguir), B los golpes (ruido con envolvente) y C el silbido
 *     de los puñetazos al aire (ruido que se apaga).
 */
#include "street.h"

#define AY2         ZXV_AY2

/* Periodos del AY para las notas MIDI 36 (do 2) a 96 (do 7). */
static const uint16_t period[61] = {
    1695, 1600, 1510, 1425, 1345, 1270, 1198, 1131, 1068, 1008, 951, 898, 847, 800, 755, 713,
    673, 635, 599, 566, 534, 504, 476, 449, 424, 400, 377, 356, 336, 317, 300, 283, 267, 252,
    238, 224, 212, 200, 189, 178, 168, 159, 150, 141, 133, 126, 119, 112, 106, 100, 94, 89, 84,
    79, 75, 71, 67, 63, 59, 56, 53,
};

/* --- Los registros, sin repetir escrituras ------------------------------------- */

static uint8_t cache[32];
static int muted;

static void ay(int r, int v)
{
    if (cache[r] != (uint8_t)v || r == 13 || r == AY2 + 13) {   /* R13 reinicia la envolvente */
        cache[r] = (uint8_t)v;
        zxv_ay_write(r, v);
    }
}

static void tone(int base, int ch, int per)
{
    ay(base + ch * 2, per & 0xFF);
    ay(base + ch * 2 + 1, (per >> 8) & 0x0F);
}

static void note(int ch, int n)
{
    if (n < 36)
        n = 36;
    if (n > 96)
        n = 96;
    tone(0, ch, period[n - 36]);
}

/* --- La música ----------------------------------------------------------------- */

typedef struct {
    uint8_t tempo, rows, loop;
    const uint8_t *melody, *bass, *drums;
} song_t;

static const uint8_t title_m[32] = { 69, 0, 72, 0, 74, 1, 72, 69, 0, 67, 69, 1, 1, 0, 0, 0,
                                     69, 0, 72, 0, 74, 1, 76, 1, 79, 1, 76, 74, 72, 1, 69, 0 };
static const uint8_t title_b[32] = { 45, 57, 45, 0, 45, 57, 43, 55, 41, 53, 41, 0, 41, 53, 43, 55,
                                     45, 57, 45, 0, 45, 57, 43, 55, 41, 53, 41, 53, 43, 55, 44, 56 };
static const uint8_t title_d[32] = { 1, 0, 2, 0, 1, 1, 2, 0, 1, 0, 2, 0, 1, 1, 2, 2,
                                     1, 0, 2, 0, 1, 1, 2, 0, 1, 0, 2, 1, 1, 2, 2, 2 };

static const uint8_t street_m[32] = { 64, 1, 67, 1, 69, 1, 67, 64, 62, 1, 64, 1, 1, 0, 59, 0,
                                      64, 1, 67, 1, 71, 1, 69, 67, 69, 1, 1, 67, 64, 1, 0, 0 };
static const uint8_t street_b[32] = { 40, 40, 52, 40, 40, 52, 40, 52, 38, 38, 50, 38, 38, 50, 38, 50,
                                      36, 36, 48, 36, 36, 48, 36, 48, 35, 35, 47, 35, 35, 47, 38, 50 };
static const uint8_t street_d[32] = { 1, 0, 2, 0, 1, 0, 2, 1, 1, 0, 2, 0, 1, 0, 2, 0,
                                      1, 0, 2, 0, 1, 0, 2, 1, 1, 0, 2, 0, 1, 2, 2, 2 };

static const uint8_t market_m[32] = { 74, 1, 78, 81, 1, 78, 76, 1, 74, 1, 71, 1, 74, 1, 0, 0,
                                      76, 1, 79, 83, 1, 81, 79, 78, 76, 1, 74, 1, 76, 0, 78, 0 };
static const uint8_t market_b[32] = { 50, 0, 62, 0, 50, 0, 62, 0, 47, 0, 59, 0, 47, 0, 59, 0,
                                      43, 0, 55, 0, 45, 0, 57, 0, 50, 0, 62, 0, 45, 0, 57, 57 };
static const uint8_t market_d[32] = { 1, 0, 2, 0, 1, 1, 2, 0, 1, 0, 2, 0, 1, 1, 2, 0,
                                      1, 0, 2, 0, 1, 1, 2, 0, 1, 0, 2, 0, 1, 1, 2, 2 };

static const uint8_t roofs_m[32] = { 69, 0, 69, 72, 0, 69, 76, 1, 74, 1, 72, 1, 71, 1, 72, 71,
                                     69, 0, 69, 72, 0, 69, 77, 1, 76, 1, 74, 72, 71, 1, 68, 1 };
static const uint8_t roofs_b[32] = { 45, 45, 57, 45, 45, 45, 57, 45, 41, 41, 53, 41, 40, 40, 52, 40,
                                     45, 45, 57, 45, 45, 45, 57, 45, 38, 38, 50, 38, 40, 40, 52, 44 };
static const uint8_t roofs_d[32] = { 1, 0, 2, 1, 0, 1, 2, 0, 1, 0, 2, 1, 0, 1, 2, 0,
                                     1, 0, 2, 1, 0, 1, 2, 0, 1, 0, 2, 1, 1, 2, 2, 2 };

static const uint8_t boss_m[16] = { 76, 75, 76, 0, 72, 0, 76, 0, 77, 76, 77, 0, 71, 0, 77, 0 };
static const uint8_t boss_b[16] = { 40, 40, 52, 40, 40, 40, 52, 40, 41, 41, 53, 41, 39, 39, 51, 39 };
static const uint8_t boss_d[16] = { 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 2, 2 };

static const uint8_t over_m[12] = { 72, 1, 69, 1, 67, 1, 64, 1, 1, 1, 0, 0 };
static const uint8_t over_b[12] = { 45, 1, 41, 1, 43, 1, 40, 1, 1, 1, 0, 0 };

static const uint8_t clear_m[16] = { 67, 72, 76, 79, 1, 0, 76, 79, 84, 1, 1, 1, 1, 0, 0, 0 };
static const uint8_t clear_b[16] = { 48, 0, 52, 0, 55, 0, 52, 0, 48, 1, 1, 1, 1, 0, 0, 0 };

static const song_t songs[] = {
    [SONG_TITLE] = { 7, 32, 1, title_m, title_b, title_d },
    [SONG_STREET] = { 6, 32, 1, street_m, street_b, street_d },
    [SONG_MARKET] = { 6, 32, 1, market_m, market_b, market_d },
    [SONG_ROOFS] = { 5, 32, 1, roofs_m, roofs_b, roofs_d },
    [SONG_BOSS] = { 5, 16, 1, boss_m, boss_b, boss_d },
    [SONG_OVER] = { 10, 12, 0, over_m, over_b, 0 },
    [SONG_CLEAR] = { 6, 16, 0, clear_m, clear_b, 0 },
};

static const song_t *song;
static int song_id, row, tick, mel_age, mel_on, bass_on, drum, drum_age;

void sound_song(int s)
{
    if (s == song_id && song && song->loop)
        return;                         /* ya suena */
    song_id = s;
    song = s == SONG_NONE ? 0 : &songs[s];
    row = 0;
    tick = 0;
    mel_on = bass_on = 0;
    drum = 0;
}

static void music_frame(void)
{
    if (song && tick == 0) {
        int m = song->melody[row], b = song->bass[row];
        if (m > 1) {
            note(0, m);
            mel_on = 1;
            mel_age = 0;
        } else if (m == 0) {
            mel_on = 0;
        }
        if (b > 1) {
            note(1, b);
            bass_on = 1;
        } else if (b == 0) {
            bass_on = 0;
        }
        if (song->drums && song->drums[row]) {
            drum = song->drums[row];
            drum_age = 0;
        }
    }
    ay(8, mel_on ? 12 - (mel_age / 3 > 4 ? 4 : mel_age / 3) : 0);
    ay(9, bass_on ? 12 - (tick > 2 ? 2 : tick) : 0);   /* el bajo, con golpe */
    mel_age++;

    int mixer = 0x38;
    if (drum == 1 && drum_age < 4) {
        tone(0, 2, 280 + drum_age * 260);
        ay(10, 13 - drum_age * 3);
    } else if (drum == 2 && drum_age < 3) {
        mixer = 0x1C;
        ay(6, 5);
        ay(10, 11 - drum_age * 3);
    } else {
        ay(10, 0);
        drum = 0;
    }
    ay(7, mixer);
    drum_age++;

    if (song && ++tick >= song->tempo) {
        tick = 0;
        if (++row >= song->rows) {
            if (song->loop) {
                row = 0;
            } else {
                song = 0;
                song_id = SONG_NONE;
                mel_on = bass_on = 0;
            }
        }
    }
}

/* --- Los efectos -------------------------------------------------------------- */

static int fx_a = -1, fx_a_age, swing_age = 99;

static void effects_frame(void)
{
    int age = fx_a_age++;
    int done = 1;
    switch (fx_a) {
    case FX_HURT:                       /* un tono que cae */
        if (age < 10) {
            tone(AY2, 0, 180 + age * 60);
            ay(AY2 + 8, 13 - age);
            done = 0;
        }
        break;
    case FX_JUMP:                       /* uno que sube */
        if (age < 6) {
            tone(AY2, 0, 300 - age * 40);
            ay(AY2 + 8, 9 - age);
            done = 0;
        }
        break;
    case FX_EAT:
    case FX_LIFE: {                     /* arpegios */
        static const uint8_t eat[] = { 72, 76, 79, 84 };
        static const uint8_t life[] = { 72, 76, 79, 84, 79, 84, 88, 91 };
        int n = fx_a == FX_EAT ? 4 : 8;
        if (age < n * 3) {
            tone(AY2, 0, period[(fx_a == FX_EAT ? eat : life)[age / 3] - 36]);
            ay(AY2 + 8, 12);
            done = 0;
        }
        break;
    }
    case FX_GO:                         /* dos pitidos, dos veces */
        if (age < 64) {
            tone(AY2, 0, (age / 8) & 1 ? 84 : 112);
            ay(AY2 + 8, (age % 32) < 14 && (age % 8) < 6 ? 12 : 0);
            done = 0;
        }
        break;
    default:
        break;
    }
    if (done) {
        ay(AY2 + 8, 0);
        fx_a = -1;
    }
    /* El silbido, en el canal C. */
    if (swing_age < 5) {
        ay(AY2 + 10, 9 - swing_age * 2);
        swing_age++;
    } else {
        ay(AY2 + 10, 0);
    }
}

/* Los golpes: ruido con la envolvente que cae, reiniciada con R13. */
static void thump(int noise, int env)
{
    ay(AY2 + 6, noise);
    ay(AY2 + 9, 0x10);
    ay(AY2 + 11, env & 0xFF);
    ay(AY2 + 12, env >> 8);
    ay(AY2 + 13, 0x09);                 /* \___ */
}

/* --- Lo de fuera ------------------------------------------------------------------ */

void sound_init(void)
{
    for (int r = 0; r < 32; r++) {
        cache[r] = 0xFF;
        if ((r & 15) < 14 && (r & 15) != 13)
            ay(r, 0);
    }
    ay(7, 0x38);
    ay(AY2 + 7, 0x0E);                  /* tono A; ruido B y C */
    *(volatile uint8_t *)(uintptr_t)ZXV_S_AY_STEREO = ZXV_AY_STEREO_ABC;
    fx_a = -1;
    song_id = SONG_NONE;
}

void sound_fx(int fx)
{
    if (muted)
        return;
    switch (fx) {
    case FX_SWING:
        ay(AY2 + 6, 2);
        swing_age = 0;
        break;
    case FX_HIT:
        thump(12, 140);
        break;
    case FX_HEAVY:
        thump(26, 600);
        break;
    case FX_BREAK:
        thump(5, 380);
        break;
    default:
        /* El canal A: un salto no corta un efecto más importante. */
        if (fx == FX_JUMP && fx_a >= 0)
            return;
        fx_a = fx;
        fx_a_age = 0;
        break;
    }
}

void sound_mute(int on)
{
    muted = on;
    if (on) {
        ay(8, 0);
        ay(9, 0);
        ay(10, 0);
        ay(AY2 + 8, 0);
        ay(AY2 + 9, 0);
        ay(AY2 + 10, 0);
    }
}

/* La música y los efectos se escribieron a 50 pasos por segundo: van a ese
 * paso, sea cual sea ZXV_FRAME_HZ. */
void sound_frame(void)
{
    static int acc;
    if (muted)
        return;
    for (acc += 50; acc >= ZXV_FRAME_HZ; acc -= ZXV_FRAME_HZ) {
        music_frame();
        effects_frame();
    }
}

void sound_off(void)
{
    song = 0;
    song_id = SONG_NONE;
    for (int r = 8; r <= 10; r++) {
        ay(r, 0);
        ay(AY2 + r, 0);
    }
    ay(7, 0x3F);
    ay(AY2 + 7, 0x3F);
}
