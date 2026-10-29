/*
 * sound.c - El sonido de Estela, con los dos AY (TurboSound, spec §5):
 *
 *   - Primer AY: la música. Canal A la melodía, B el bajo y C la batería
 *     (bombo con tono, caja con ruido). Un pequeño reproductor de filas: una
 *     nota por fila, 1 = sigue la anterior, 0 = silencio.
 *   - Segundo AY: los efectos. Canal A los disparos y avisos (barridos de
 *     tono), B las explosiones (ruido con envolvente).
 *   - Segundo AY, canal C: muestras digitalizadas a 8 kHz por su volumen,
 *     cada una a su hora con zxv_ay_event (spec §5.4). Se mandan unos 30 ms
 *     por delante, solo las que caben en la cola, así el juego no espera nunca.
 */
#include "estela.h"

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
    uint8_t tempo, rows, loop;          /* tempo: pasos (de 1/50 s) por fila */
    const uint8_t *melody, *bass, *drums;
} song_t;

static const uint8_t title_m[32] = { 69, 1, 72, 76, 81, 1, 79, 76, 77, 1, 76, 74, 76, 1, 1, 0,
                                     72, 1, 74, 76, 79, 1, 77, 76, 74, 1, 72, 71, 72, 1, 1, 0 };
static const uint8_t title_b[32] = { 45, 0, 57, 0, 45, 0, 57, 0, 41, 0, 53, 0, 40, 0, 52, 0,
                                     48, 0, 60, 0, 43, 0, 55, 0, 38, 0, 50, 0, 40, 0, 52, 0 };
static const uint8_t title_d[32] = { 1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0,
                                     1, 0, 0, 0, 2, 0, 0, 0, 1, 0, 1, 0, 2, 0, 2, 2 };

static const uint8_t stage_m[32] = { 76, 1, 0, 76, 79, 1, 76, 0, 74, 1, 72, 1, 74, 1, 0, 0,
                                     76, 1, 0, 76, 79, 1, 81, 1, 83, 1, 81, 79, 76, 1, 1, 0 };
static const uint8_t stage_b[32] = { 40, 52, 40, 52, 40, 52, 40, 52, 36, 48, 36, 48, 38, 50, 38, 50,
                                     40, 52, 40, 52, 43, 55, 43, 55, 45, 57, 45, 57, 40, 52, 40, 52 };
static const uint8_t stage_d[32] = { 1, 0, 2, 0, 1, 1, 2, 0, 1, 0, 2, 0, 1, 1, 2, 0,
                                     1, 0, 2, 0, 1, 1, 2, 0, 1, 0, 2, 0, 1, 2, 2, 2 };

static const uint8_t boss_m[16] = { 69, 0, 70, 0, 69, 0, 68, 0, 69, 1, 72, 1, 71, 1, 70, 1 };
static const uint8_t boss_b[16] = { 45, 45, 57, 45, 45, 45, 56, 45, 45, 45, 57, 45, 44, 44, 56, 44 };
static const uint8_t boss_d[16] = { 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 2, 2 };

static const uint8_t over_m[12] = { 72, 1, 71, 1, 70, 1, 69, 1, 1, 1, 0, 0 };
static const uint8_t over_b[12] = { 48, 1, 47, 1, 46, 1, 45, 1, 1, 1, 0, 0 };

static const uint8_t clear_m[16] = { 72, 76, 79, 84, 1, 0, 79, 84, 88, 1, 1, 1, 1, 0, 0, 0 };
static const uint8_t clear_b[16] = { 48, 0, 55, 0, 60, 0, 55, 0, 60, 1, 1, 1, 1, 0, 0, 0 };

static const song_t songs[] = {
    [SONG_TITLE] = { 7, 32, 1, title_m, title_b, title_d },
    [SONG_STAGE] = { 6, 32, 1, stage_m, stage_b, stage_d },
    [SONG_BOSS] = { 5, 16, 1, boss_m, boss_b, boss_d },
    [SONG_OVER] = { 10, 12, 0, over_m, over_b, 0 },
    [SONG_CLEAR] = { 6, 16, 0, clear_m, clear_b, 0 },
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

static void music_tick(void)
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
    /* Los volúmenes: la melodía cae un poco, el bajo es fijo. */
    ay(8, mel_on ? 13 - (mel_age / 3 > 4 ? 4 : mel_age / 3) : 0);
    ay(9, bass_on ? 11 : 0);
    mel_age++;

    /* La batería en el canal C: bombo (un tono que baja) o caja (ruido). */
    int mixer = 0x38;                   /* los tres tonos, sin ruido */
    if (drum == 1 && drum_age < 4) {
        tone(0, 2, 300 + drum_age * 250);
        ay(10, 13 - drum_age * 3);
    } else if (drum == 2 && drum_age < 4) {
        mixer = 0x1C;                   /* tono A y B; C solo ruido */
        ay(6, 6);
        ay(10, 12 - drum_age * 3);
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

static int fx_a, fx_a_age;              /* el canal A del segundo AY */

static void effects_tick(void)
{
    int age = fx_a_age++;
    switch (fx_a) {
    case FX_SHOT:                       /* un barrido hacia abajo */
        if (age < 6) {
            tone(AY2, 0, 40 + age * 22);
            ay(AY2 + 8, 13 - age * 2);
            return;
        }
        break;
    case FX_HIT:
        if (age < 3) {
            tone(AY2, 0, 30 + age * 6);
            ay(AY2 + 8, 11 - age * 3);
            return;
        }
        break;
    case FX_POWER: {                    /* un arpegio que sube */
        static const uint8_t arp[] = { 72, 76, 79, 84, 79, 84, 88 };
        if (age < 21) {
            tone(AY2, 0, period[arp[age / 3] - 36]);
            ay(AY2 + 8, 12);
            return;
        }
        break;
    }
    case FX_ALARM:                      /* dos tonos alternos */
        if (age < 90) {
            tone(AY2, 0, (age / 8) & 1 ? 120 : 160);
            ay(AY2 + 8, (age % 8) < 6 ? 12 : 0);
            return;
        }
        break;
    default:
        break;
    }
    ay(AY2 + 8, 0);
    fx_a = -1;
}

/* Las explosiones: ruido con la envolvente que cae, reiniciada con R13. */
static void boom(int noise, int env)
{
    ay(AY2 + 6, noise);
    ay(AY2 + 9, 0x10);                  /* el volumen, el de la envolvente */
    ay(AY2 + 11, env & 0xFF);
    ay(AY2 + 12, env >> 8);
    ay(AY2 + 13, 0x09);                 /* \___ */
}

/* --- Las muestras digitalizadas -------------------------------------------------- */

#define DIG_LEN     4800                /* 0,6 s a 8 kHz */
#define DIG_STEP    125                 /* µs entre muestras */

static uint8_t dig_boom[DIG_LEN];
static const uint8_t *dig;
static int dig_n, dig_pos;
static uint32_t dig_t;

static unsigned queue_room(void)
{
    uint16_t head = *(volatile uint16_t *)(uintptr_t)ZXV_S_BEEP_HEAD;
    uint16_t tail = *(volatile uint16_t *)(uintptr_t)ZXV_S_BEEP_TAIL;
    return ZXV_BEEP_QUEUE_LEN - (uint16_t)(head - tail);
}

/* Una explosión grande: ruido filtrado y un retumbar grave que se apagan. */
static void make_samples(void)
{
    int lp = 0, ph = 0;
    for (int i = 0; i < DIG_LEN; i++) {
        int env = (DIG_LEN - i) * 256 / DIG_LEN;            /* 256 -> 0 */
        env = env * env / 256;
        int noise = (int)(rnd() & 255) - 128;
        lp += (noise - lp) / 3;                             /* sin los agudos */
        ph += i < 1600 ? 130 : 105;                         /* en 1/256 de paso: unos 63 y 51 Hz */
        int rumble = isin(ph >> 8) * 2;
        int s = (lp + rumble) * env / 256;                  /* unos -250..250 */
        int v = 8 + s / 32;
        dig_boom[i] = (uint8_t)(v < 0 ? 0 : v > 15 ? 15 : v);
    }
}

static void digital_frame(void)
{
    if (!dig)
        return;
    uint32_t now = zxv_time_us();
    if (dig_pos == 0)
        dig_t = now + 4000;
    if ((int32_t)(dig_t - now) < 0)
        dig_t = now + 2000;             /* se ha quedado atrás (un frame perdido) */
    while (dig_pos < dig_n && (int32_t)(dig_t - (now + 30000)) < 0 && queue_room() > 8) {
        zxv_ay_event(dig_t, AY2 + 10, muted ? 0 : dig[dig_pos]);
        dig_pos++;
        dig_t += DIG_STEP;
    }
    if (dig_pos >= dig_n && queue_room() > 8) {
        zxv_ay_event(dig_t, AY2 + 10, 0);
        dig = 0;
    }
}

static void digital_play(const uint8_t *s, int n)
{
    dig = s;
    dig_n = n;
    dig_pos = 0;
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
    ay(AY2 + 7, 0x2E);                  /* tono A, ruido B; el C, solo su volumen */
    *(volatile uint8_t *)(uintptr_t)ZXV_S_AY_STEREO = ZXV_AY_STEREO_ABC;
    fx_a = -1;
    make_samples();
}

void sound_fx(int fx)
{
    switch (fx) {
    case FX_BOOM:
        boom(20, 130);
        break;
    case FX_BIG:
        boom(28, 450);
        digital_play(dig_boom, DIG_LEN);
        break;
    case FX_DEATH:
        boom(31, 700);
        digital_play(dig_boom, DIG_LEN);
        break;
    default:
        /* El canal A: un disparo no corta un efecto más largo. */
        if (fx == FX_SHOT && fx_a >= 0 && fx_a != FX_SHOT)
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
    }
}

void sound_frame(void)
{
    static int acc;
    if (muted)
        return;
    /* La música y los efectos, a 50 pasos por segundo (se compusieron así,
     * cuando el juego iba a 50 imágenes por segundo): a 60, uno de cada seis
     * frames no da ningún paso. */
    for (acc += 50; acc >= ZXV_FRAME_HZ; acc -= ZXV_FRAME_HZ) {
        music_tick();
        effects_tick();
    }
    digital_frame();
}

void sound_off(void)
{
    song = 0;
    dig = 0;
    for (int r = 8; r <= 10; r++) {
        ay(r, 0);
        ay(AY2 + r, 0);
    }
    ay(7, 0x3F);
    ay(AY2 + 7, 0x3F);
}
