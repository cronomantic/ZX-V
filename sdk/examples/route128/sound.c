/*
 * sound.c - El sonido de Route 128, con los dos AY (TurboSound, spec §5):
 *
 *   - Primer AY: la música. Canal A la melodía, B el bajo y C la batería
 *     (bombo con un barrido de tono, caja con ruido). Una nota por fila:
 *     1 sigue la anterior, 0 calla.
 *   - Segundo AY: el coche. Canales A y B, el motor (dos tonos, uno una
 *     octava más grave, que siguen a las vueltas); C, el ruido del derrape y
 *     de la cuneta, y encima los golpes y los avisos.
 */
#include "route128.h"

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

/* La carrera: do, la menor, fa y sol, en corcheas. */
static const uint8_t race_m[64] = {
    76, 1, 79, 81, 1, 79, 76, 1, 74, 1, 76, 79, 1, 0, 0, 0,
    72, 1, 74, 76, 1, 79, 81, 1, 79, 1, 76, 74, 1, 0, 0, 0,
    76, 1, 79, 81, 1, 84, 83, 81, 79, 1, 76, 79, 1, 81, 1, 0,
    84, 1, 83, 81, 79, 1, 76, 74, 72, 1, 1, 1, 1, 0, 74, 76,
};
static const uint8_t race_b[64] = {
    48, 0, 55, 0, 60, 0, 55, 0, 45, 0, 52, 0, 57, 0, 52, 0,
    41, 0, 48, 0, 53, 0, 48, 0, 43, 0, 50, 0, 55, 0, 50, 0,
    48, 0, 55, 0, 60, 0, 55, 0, 45, 0, 52, 0, 57, 0, 52, 0,
    41, 0, 48, 0, 53, 0, 48, 0, 43, 0, 50, 0, 43, 47, 50, 55,
};
static const uint8_t race_d[16] = { 1, 0, 3, 0, 2, 0, 3, 1, 1, 0, 3, 0, 2, 0, 3, 3 };

static const uint8_t title_m[32] = {
    72, 1, 76, 1, 79, 1, 84, 1, 83, 1, 79, 1, 76, 1, 1, 0,
    74, 1, 77, 1, 81, 1, 79, 1, 77, 1, 76, 1, 74, 1, 1, 0,
};
static const uint8_t title_b[32] = {
    48, 0, 1, 0, 55, 0, 1, 0, 45, 0, 1, 0, 52, 0, 1, 0,
    41, 0, 1, 0, 48, 0, 1, 0, 43, 0, 1, 0, 50, 0, 1, 0,
};
static const uint8_t title_d[16] = { 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0 };

static const uint8_t goal_m[24] = { 72, 76, 79, 84, 1, 0, 79, 84, 1, 1, 1, 0, 81, 84, 88, 1, 1, 1, 91, 1, 1, 1, 1, 0 };
static const uint8_t goal_b[24] = { 48, 0, 0, 52, 0, 0, 55, 0, 0, 60, 0, 0, 53, 0, 0, 57, 0, 0, 55, 0, 0, 1, 1, 0 };
static const uint8_t goal_d[8] = { 1, 0, 2, 0, 1, 1, 2, 0 };

static const uint8_t over_m[16] = { 72, 1, 71, 1, 70, 1, 69, 1, 1, 1, 1, 1, 1, 1, 1, 0 };
static const uint8_t over_b[16] = { 48, 1, 47, 1, 46, 1, 45, 1, 1, 1, 1, 1, 1, 1, 1, 0 };
static const uint8_t over_d[8] = { 0 };

static const song_t songs[] = {
    [SONG_RACE] = { 7, 64, 1, race_m, race_b, race_d },
    [SONG_TITLE] = { 8, 32, 1, title_m, title_b, title_d },
    [SONG_GOAL] = { 6, 24, 0, goal_m, goal_b, goal_d },
    [SONG_OVER] = { 10, 16, 0, over_m, over_b, over_d },
};

static const song_t *song;
static int row, tick, drum_len;
static int mel_vol, bass_vol, drum_t, drum_kind;

void sound_song(int s)
{
    song = s == SONG_NONE ? 0 : &songs[s];
    row = 0;
    tick = 0;
    mel_vol = bass_vol = 0;
    drum_len = song == &songs[SONG_RACE] ? 16 : song == &songs[SONG_TITLE] ? 16 : 8;
}

static void music_frame(void)
{
    if (!song) {
        ay(8, 0);
        ay(9, 0);
        ay(10, 0);
        return;
    }
    if (tick == 0) {
        int m = song->melody[row], b = song->bass[row], d = song->drums[row % drum_len];
        if (m > 1) {
            note(0, m);
            mel_vol = 13;
        } else if (m == 0) {
            mel_vol = 0;
        }
        if (b > 1) {
            note(1, b);
            bass_vol = 12;
        } else if (b == 0) {
            bass_vol = bass_vol > 8 ? 8 : bass_vol;
        }
        if (d) {
            drum_kind = d;
            drum_t = 0;
        }
        if (++row >= song->rows) {
            if (song->loop)
                row = 0;
            else {
                song = 0;
                return;
            }
        }
    }
    if (++tick >= song->tempo)
        tick = 0;
    if (mel_vol > 9 && (tick & 1))
        mel_vol--;                      /* la nota se apaga un poco */
    /* La batería, en el canal C: el bombo baja de tono; la caja y el charles son ruido. */
    int mixer = 0x38;                   /* tonos A y B, sin ruido */
    int dv = 0;
    if (drum_kind == 1 && drum_t < 6) {
        tone(0, 2, 600 + drum_t * 300);
        dv = 15 - drum_t * 2;
        mixer = 0x38 & ~0x00;           /* el tono de C */
    } else if (drum_kind == 2 && drum_t < 7) {
        ay(6, 8);
        dv = 13 - drum_t * 2;
        mixer = 0x18 | 0x04;            /* ruido en C, sin su tono */
    } else if (drum_kind == 3 && drum_t < 2) {
        ay(6, 2);
        dv = 8;
        mixer = 0x18 | 0x04;
    } else {
        mixer = 0x3C;
    }
    drum_t++;
    ay(7, mixer);
    ay(8, muted ? 0 : mel_vol);
    ay(9, muted ? 0 : bass_vol);
    ay(10, muted ? 0 : dv);
}

/* --- El coche -------------------------------------------------------------------- */

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
    for (int r = 0; r < 14; r++) {
        ay(r, r == 7 ? 0x3F : 0);
        ay(AY2 + r, r == 7 ? 0x3F : 0);
    }
    song = 0;
    fx = 0;
    eng_rpm = -1;
}

void sound_frame(void)
{
    music_frame();
    if (eng_rpm < 0 && !fx) {           /* sin coche (el título): callado */
        ay(AY2 + 8, 0);
        ay(AY2 + 9, 0);
        ay(AY2 + 10, 0);
        return;
    }
    car_frame();
}

void sound_mute(int on) { muted = on; }

void sound_off(void)
{
    for (int r = 8; r < 11; r++) {
        ay(r, 0);
        ay(AY2 + r, 0);
    }
    ay(7, 0x3F);
    ay(AY2 + 7, 0x3F);
}
