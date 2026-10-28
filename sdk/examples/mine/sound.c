/*
 * sound.c - El sonido de la mina, con los dos AY (TurboSound, spec §5):
 *
 *   - Primer AY: la música del título y los finales. Canal A la melodía, B
 *     el bajo y C la batería (bombo con tono, caja con ruido). Una nota por
 *     fila: 1 = sigue la anterior, 0 = silencio.
 *   - Segundo AY: los efectos. Canal A los avisos (diamante, salida,
 *     tiempo), B los golpes con ruido (cavar, piedras, explosiones) y C los
 *     tintineos (diamantes que caen, el tamiz, la cuenta del tiempo).
 */
#include "mine.h"

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

static int pnote(int n)
{
    return period[(n < 36 ? 36 : n > 96 ? 96 : n) - 36];
}

/* --- La música ----------------------------------------------------------------- */

typedef struct {
    uint8_t tempo, rows, loop;
    const uint8_t *melody, *bass, *drums;
} song_t;

/* El título: una copla de mineros en la menor. */
static const uint8_t title_m[64] = {
    69, 1, 72, 1, 76, 1, 74, 72, 71, 1, 72, 74, 76, 1, 1, 0,
    77, 1, 76, 74, 72, 1, 69, 1, 71, 1, 68, 1, 69, 1, 1, 0,
    76, 1, 79, 1, 81, 1, 79, 77, 76, 1, 74, 1, 72, 1, 1, 0,
    74, 1, 72, 71, 72, 1, 69, 1, 68, 1, 71, 1, 69, 1, 1, 0,
};
static const uint8_t title_b[64] = {
    45, 0, 52, 0, 45, 0, 52, 0, 43, 0, 50, 0, 43, 0, 50, 0,
    41, 0, 48, 0, 41, 0, 48, 0, 40, 0, 47, 0, 45, 0, 52, 0,
    48, 0, 55, 0, 48, 0, 55, 0, 43, 0, 50, 0, 45, 0, 52, 0,
    38, 0, 45, 0, 41, 0, 48, 0, 40, 0, 44, 0, 45, 0, 45, 0,
};
static const uint8_t title_d[16] = { 1, 0, 2, 0, 0, 1, 2, 0, 1, 0, 2, 0, 0, 1, 2, 2 };

static const uint8_t clear_m[16] = { 72, 1, 76, 1, 79, 1, 84, 1, 83, 84, 1, 1, 1, 0, 0, 0 };
static const uint8_t clear_b[16] = { 48, 0, 52, 0, 55, 0, 48, 0, 55, 48, 1, 1, 1, 0, 0, 0 };

static const uint8_t over_m[16] = { 69, 1, 68, 1, 67, 1, 66, 1, 65, 1, 1, 1, 1, 1, 0, 0 };
static const uint8_t over_b[16] = { 45, 1, 44, 1, 43, 1, 42, 1, 41, 1, 1, 1, 1, 1, 0, 0 };

static const uint8_t win_m[32] = { 72, 76, 79, 76, 72, 76, 79, 84, 81, 1, 79, 1, 77, 1, 76, 1,
                                   74, 77, 81, 77, 74, 77, 81, 86, 84, 1, 1, 1, 1, 1, 0, 0 };
static const uint8_t win_b[32] = { 48, 0, 55, 0, 48, 0, 55, 0, 53, 0, 53, 0, 52, 0, 52, 0,
                                   50, 0, 57, 0, 55, 0, 43, 0, 48, 1, 1, 1, 1, 1, 0, 0 };
static const uint8_t win_d[16] = { 1, 0, 2, 0, 1, 0, 2, 0, 1, 0, 2, 0, 1, 2, 2, 2 };

static const song_t songs[] = {
    [SONG_TITLE] = { 7, 64, 1, title_m, title_b, title_d },
    [SONG_CLEAR] = { 5, 16, 0, clear_m, clear_b, 0 },
    [SONG_OVER] = { 9, 16, 0, over_m, over_b, 0 },
    [SONG_WIN] = { 6, 32, 0, win_m, win_b, win_d },
};

static const song_t *song;
static int row, tick, mel_age, mel_on, bass_on, drum, drum_age;

void sound_song(int s)
{
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
            tone(0, 0, pnote(m));
            mel_on = 1;
            mel_age = 0;
        } else if (m == 0) {
            mel_on = 0;
        }
        if (b > 1) {
            tone(0, 1, pnote(b));
            bass_on = 1;
        } else if (b == 0) {
            bass_on = 0;
        }
        if (song->drums && song->drums[row & 15]) {
            drum = song->drums[row & 15];
            drum_age = 0;
        }
    }
    /* La melodía con un poco de vibrato y caída; el bajo, fijo. */
    ay(8, mel_on ? 12 - (mel_age / 4 > 4 ? 4 : mel_age / 4) : 0);
    ay(9, bass_on ? 10 : 0);
    mel_age++;

    int mixer = 0x38;
    if (drum == 1 && drum_age < 4) {
        tone(0, 2, 400 + drum_age * 300);
        ay(10, 12 - drum_age * 3);
    } else if (drum == 2 && drum_age < 3) {
        mixer = 0x1C;                   /* el C, solo ruido */
        ay(6, 4);
        ay(10, 10 - drum_age * 3);
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
                mel_on = bass_on = 0;
            }
        }
    }
}

/* --- Los efectos -------------------------------------------------------------- */

static int fa = -1, fa_age;             /* canal A: avisos */
static int fb = -1, fb_age, fb_pri;     /* canal B: ruido */
static int fc = -1, fc_age, fc_arg;     /* canal C: tintineos */

static void effects_frame(void)
{
    /* A: los avisos. */
    int age = fa_age++, v = 0;
    switch (fa) {
    case FX_DIAMOND: {                  /* dos notas que suben */
        static const uint8_t n[] = { 88, 93 };
        if (age < 6) {
            tone(AY2, 0, pnote(n[age / 3]));
            v = 13 - age;
        }
        break;
    }
    case FX_OPEN:
    case FX_LIFE: {                     /* un arpegio que sube */
        static const uint8_t n[] = { 72, 76, 79, 84, 79, 84, 88, 91 };
        if (age < 32) {
            tone(AY2, 0, pnote(n[age / 4] + (fa == FX_LIFE ? 5 : 0)));
            v = 13 - (age & 3);
        }
        break;
    }
    case FX_TICK:                       /* el tiempo se acaba */
        if (age < 3) {
            tone(AY2, 0, 60);
            v = 12;
        }
        break;
    case FX_BORN:                       /* un barrido hacia arriba */
        if (age < 20) {
            tone(AY2, 0, 500 - age * 22);
            v = 11 - age / 3;
        }
        break;
    case FX_MAGIC:
        if (age < 24) {
            tone(AY2, 0, pnote(84 + ((age * 5) % 12)));
            v = 12 - age / 3;
        }
        break;
    default:
        break;
    }
    ay(AY2 + 8, v > 0 ? v : 0);
    if (!v)
        fa = -1;

    /* B: ruido, a volumen fijo que cae (las explosiones, con la envolvente). */
    age = fb_age++;
    v = 0;
    switch (fb) {
    case FX_DIG:
        if (age < 2) {
            ay(AY2 + 6, 3);
            v = 7 - age * 3;
        }
        break;
    case FX_PUSH:
        if (age < 3) {
            ay(AY2 + 6, 20);
            v = 9 - age * 3;
        }
        break;
    case FX_LAND:
        if (age < 5) {
            ay(AY2 + 6, 24 + age);
            v = 13 - age * 3;
        }
        break;
    case FX_BOOM:
        if (age < 45)
            v = -1;                     /* lo lleva la envolvente */
        break;
    default:
        break;
    }
    if (v >= 0)
        ay(AY2 + 9, v);
    if (!v) {
        fb = -1;
        fb_pri = 0;
    }

    /* C: tintineos. */
    age = fc_age++;
    v = 0;
    switch (fc) {
    case FX_TING:
        if (age < 8) {
            tone(AY2, 2, 42 + (age & 1) * 3);
            v = 12 - age;
        }
        break;
    case FX_SIEVE:
        if (age < 10) {
            tone(AY2, 2, 30 + ((age * 7) & 15));
            v = 11 - age;
        }
        break;
    case FX_BONUS:
        if (age < 2) {
            tone(AY2, 2, 40 + fc_arg);
            v = 10;
        }
        break;
    default:
        break;
    }
    ay(AY2 + 10, v > 0 ? v : 0);
    if (!v)
        fc = -1;
}

void sound_fx(int fx)
{
    switch (fx) {
    case FX_DIG:
    case FX_PUSH:
    case FX_LAND:
    case FX_BOOM: {
        static const uint8_t pri[] = { [FX_DIG] = 1, [FX_PUSH] = 2, [FX_LAND] = 3, [FX_BOOM] = 4 };
        if (fb >= 0 && pri[fx] < fb_pri)
            return;
        fb = fx;
        fb_age = 0;
        fb_pri = pri[fx];
        if (fx == FX_BOOM) {
            ay(AY2 + 6, 28);
            ay(AY2 + 9, 0x10);          /* el volumen, el de la envolvente */
            ay(AY2 + 11, 0x40);
            ay(AY2 + 12, 0x06);
            ay(AY2 + 13, 0x09);         /* \___ */
        }
        break;
    }
    case FX_TING:
    case FX_SIEVE:
        if (fc == FX_BONUS)
            return;
        fc = fx;
        fc_age = 0;
        break;
    case FX_BONUS:
        fc = fx;
        fc_age = 0;
        fc_arg = (fc_arg + 3) & 31;
        break;
    default:
        /* A: un diamante no corta un aviso más largo. */
        if (fx == FX_DIAMOND && fa >= 0 && fa != FX_DIAMOND)
            return;
        fa = fx;
        fa_age = 0;
        break;
    }
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
    ay(AY2 + 7, 0x2A);                  /* tono A y C, ruido B */
    *(volatile uint8_t *)(uintptr_t)ZXV_S_AY_STEREO = ZXV_AY_STEREO_ABC;
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
        fa = fb = fc = -1;
        fb_pri = 0;
    }
}

void sound_frame(void)
{
    if (muted)
        return;
    music_frame();
    effects_frame();
}

void sound_off(void)
{
    song = 0;
    for (int r = 8; r <= 10; r++) {
        ay(r, 0);
        ay(AY2 + r, 0);
    }
    ay(7, 0x3F);
    ay(AY2 + 7, 0x3F);
}
