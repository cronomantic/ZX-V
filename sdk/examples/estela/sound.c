/*
 * sound.c - El sonido de Estela, con los dos AY (TurboSound, spec §5):
 *
 *   - Primer AY: la música, con la biblioteca del SDK (zxv_music.h). Son
 *     módulos de Pro Tracker 3 que se editan con zxv-tracker: estela-title,
 *     -stage1, -stage2 y -stage3 (las fases), -boss (los jefes), -over (el
 *     final de la partida) y -clear (fase superada), con .pt3. El programa
 *     los lleva dentro y, si en la carpeta desde la que se carga hay alguno,
 *     usa ese.
 *   - Segundo AY: los efectos. Canal A los disparos y avisos (barridos de
 *     tono), B las explosiones (ruido con envolvente).
 *   - Segundo AY, canal C: muestras digitalizadas a 8 kHz por su volumen,
 *     cada una a su hora con zxv_ay_event (spec §5.4). Van por la misma cola
 *     que la música, que tiene que ir en orden de hora: se mandan por delante,
 *     detrás de lo que ya tiene la música, y la música solo hasta la última.
 */
#include "zxv_music.h"

#include "estela.h"

#define AY2         ZXV_AY2

/* Periodos del AY para las notas MIDI 36 (do 2) a 96 (do 7). */
static const uint16_t period[61] = {
    1695, 1600, 1510, 1425, 1345, 1270, 1198, 1131, 1068, 1008, 951, 898, 847, 800, 755, 713,
    673, 635, 599, 566, 534, 504, 476, 449, 424, 400, 377, 356, 336, 317, 300, 283, 267, 252,
    238, 224, 212, 200, 189, 178, 168, 159, 150, 141, 133, 126, 119, 112, 106, 100, 94, 89, 84,
    79, 75, 71, 67, 63, 59, 56, 53,
};

/* --- Los registros del segundo AY, sin repetir escrituras --------------------- */

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

/* --- La música ------------------------------------------------------------------- */

/* Los módulos de dentro del programa. */
#define EMBED(sym, file)                                                        \
    __asm__(".section .rodata." #sym ", \"a\"\n.balign 4\n" #sym ":\n"           \
            ".incbin \"examples/estela/" file "\"\n" #sym "_end:\n.previous\n");  \
    extern const uint8_t sym[], sym##_end[]
EMBED(title_pt3, "estela-title.pt3");
EMBED(stage1_pt3, "estela-stage1.pt3");
EMBED(stage2_pt3, "estela-stage2.pt3");
EMBED(stage3_pt3, "estela-stage3.pt3");
EMBED(boss_pt3, "estela-boss.pt3");
EMBED(over_pt3, "estela-over.pt3");
EMBED(clear_pt3, "estela-clear.pt3");

static struct {
    const char *file;
    const uint8_t *data, *end;
    uint8_t loop;
} songs[SONGS] = {
    [SONG_TITLE] = { "estela-title.pt3", title_pt3, title_pt3_end, 1 },
    [SONG_STAGE] = { "estela-stage1.pt3", stage1_pt3, stage1_pt3_end, 1 },
    [SONG_STAGE_2] = { "estela-stage2.pt3", stage2_pt3, stage2_pt3_end, 1 },
    [SONG_STAGE_3] = { "estela-stage3.pt3", stage3_pt3, stage3_pt3_end, 1 },
    [SONG_BOSS] = { "estela-boss.pt3", boss_pt3, boss_pt3_end, 1 },
    [SONG_OVER] = { "estela-over.pt3", over_pt3, over_pt3_end, 0 },
    [SONG_CLEAR] = { "estela-clear.pt3", clear_pt3, clear_pt3_end, 0 },
};

static uint8_t card[24 * 1024];         /* los de la tarjeta, uno detrás de otro */

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
static int music_on = 1;

/* Uno de la tarjeta que no cabe o no es música da un informe y vuelve al BASIC. */
static void load_songs(void)
{
    uint8_t *at = card;
    for (int s = SONG_TITLE; s < SONGS; s++) {
        int n = zxv_file_load(songs[s].file, at, (uint32_t)(card + sizeof(card) - at));
        zxv_music_info_t info;
        if (n == ZXV_E_RANGE || (n >= 0 && zxv_music_info(at, (uint32_t)n, &info) != ZXV_MUSIC_PT3))
            bad_song(songs[s].file);
        if (n >= 0) {
            songs[s].data = at;
            songs[s].end = at + n;
            at += (n + 3) & ~3;
        }
    }
}

void sound_song(int s)
{
    if (s == SONG_NONE)
        zxv_music_stop();
    else
        zxv_music_play(songs[s].data, (uint32_t)(songs[s].end - songs[s].data), songs[s].loop ? ZXV_MUSIC_LOOP : 0);
    zxv_music_pause(muted || !music_on);
}

void sound_music(int on)
{
    music_on = on;
    zxv_music_pause(muted || !music_on);
}

int sound_music_on(void) { return music_on; }

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

/* Las muestras que caben en la cola, hasta ZXV_MUSIC_AHEAD_US por delante, y
 * la música en medio, en orden de hora: hasta cada muestra, la música que
 * va antes. Mientras suenan, la música no va más allá de la última. */
static void digital_frame(void)
{
    if (!dig) {
        zxv_music_update();
        return;
    }
    uint32_t now = zxv_time_us();
    if (dig_pos == 0 || (int32_t)(dig_t - now) < 0)
        dig_t = now + ZXV_MUSIC_AHEAD_US;  /* detrás de lo que ya tiene la música */
    while (dig_pos < dig_n && (int32_t)(dig_t - (now + ZXV_MUSIC_AHEAD_US)) <= 0 && queue_room() > 32) {
        zxv_music_update_until(dig_t);
        zxv_ay_event(dig_t, AY2 + 10, muted ? 0 : dig[dig_pos]);
        dig_pos++;
        dig_t += DIG_STEP;
    }
    zxv_music_update_until(dig_t);
    if (dig_pos >= dig_n && queue_room() > 32) {
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
    for (int r = 0; r < 32; r++)
        cache[r] = 0xFF;
    for (int r = 0; r < 13; r++)
        ay(AY2 + r, 0);
    ay(AY2 + 7, 0x2E);                  /* tono A, ruido B; el C, solo su volumen */
    *(volatile uint8_t *)(uintptr_t)ZXV_S_AY_STEREO = ZXV_AY_STEREO_ABC;
    fx_a = -1;
    make_samples();
    load_songs();
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
    zxv_music_pause(muted || !music_on);
    if (on) {
        ay(AY2 + 8, 0);
        ay(AY2 + 9, 0);
    }
}

void sound_frame(void)
{
    static int acc;
    /* Los efectos, a 50 pasos por segundo (se hicieron así, cuando el juego
     * iba a 50 imágenes por segundo): a 60, uno de cada seis frames no da
     * ningún paso. La música lleva su propio reloj. */
    if (!muted)
        for (acc += 50; acc >= ZXV_FRAME_HZ; acc -= ZXV_FRAME_HZ)
            effects_tick();
    digital_frame();
}

/* Todo callado, para volver al BASIC: la música y las muestras salen por
 * delante (hasta ZXV_MUSIC_AHEAD_US), así que se espera a que salgan. */
void sound_off(void)
{
    zxv_music_stop();
    dig = 0;
    for (int i = 0; i < 4; i++) {
        zxv_music_update();
        zxv_wait_vblank();
    }
    for (int r = 8; r <= 10; r++) {
        zxv_ay_write(r, 0);
        ay(AY2 + r, 0);
    }
    zxv_ay_write(7, 0x3F);
    ay(AY2 + 7, 0x3F);
}
