/*
 * tune_test.cpp - La canción de zxv-tracker (tune.cpp) y sus ficheros.
 *
 *   tracker_tune_test               una canción con los AY y pistas de
 *                                   muestras: guardada como .zxm y leída
 *                                   otra vez, igual
 *   tracker_tune_test FICHERO...    cada PT3 (o TurboSound): pasado a la
 *                                   canción y otra vez a PT3, los mismos
 *                                   bytes que el editor de siempre; como
 *                                   .zxm, suena igual que ese PT3 (el que
 *                                   guarda el editor: lo que no tiene, ya
 *                                   lo pierde al leerlo); y ese .zxm, leído
 *                                   y guardado, igual. Cada
 *                                   MOD, S3M o XM: leído (convertido) y
 *                                   guardado, suena igual que convertido
 *                                   con zxv-music
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../tune.h"

extern "C" {
#include "music_int.h"
#include "zxm_song.h"
#include "zxv_music.h"
}

using namespace trk;

static int failures;

#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            std::printf("%s:%d: falla %s\n", __FILE__, __LINE__, #c);             \
            failures++;                                                           \
        }                                                                         \
    } while (0)

/* Dos ficheros que tocan igual (sdk/music): los AY y los canales de
 * muestras, paso a paso, hasta que el primero vuelve a empezar (o "steps").
 * -1 si igual; si no, el paso. */
static long sameSound(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b, long steps)
{
    static mus_song_t x, y;
    static zxm_t zx, zy;
    std::memset(&x, 0, sizeof x);
    std::memset(&y, 0, sizeof y);
    x.zxm = &zx;
    y.zxm = &zy;
    if (mus_song_init(&x, a.data(), uint32_t(a.size())) == ZXV_MUSIC_NONE ||
        mus_song_init(&y, b.data(), uint32_t(b.size())) == ZXV_MUSIC_NONE || x.chips != y.chips)
        return 0;
    for (long i = 0; i < steps; i++) {
        mus_ay_t p[2], q[2];
        std::memset(p, 0, sizeof p);
        std::memset(q, 0, sizeof q);
        mus_song_step(&x, p);
        if (mus_song_looped(&x))
            return -1;                  /* al volver, un PT3 puede ir desfasado (zxm_pt3.c) */
        mus_song_step(&y, q);
        for (int k = 0; k < (x.chips == 3 ? 2 : 1); k++)
            if (std::memcmp(p[k].r, q[k].r, 14) || p[k].r13 != q[k].r13)
                return i;
        const mus_smp_t *sx = mus_song_samples(&x), *sy = mus_song_samples(&y);
        if (!sx != !sy || (sx && std::memcmp(sx, sy, 12 * sizeof *sx)))
            return i;
    }
    return -1;
}

static std::vector<uint8_t> readFile(const char *path)
{
    std::vector<uint8_t> v;
    FILE *f = std::fopen(path, "rb");
    if (!f)
        return v;
    int c;
    while ((c = std::fgetc(f)) != EOF)
        v.push_back(uint8_t(c));
    std::fclose(f);
    return v;
}

static void say(const char *name, const Warnings &w)
{
    for (const std::string &s : w)
        std::printf("  %s: %s\n", name, s.c_str());
}

/* Una canción con un AY, dos pistas de muestras (un seno y un ruido), dos
 * posiciones de largos distintos y patrones repetidos. */
static Tune madeTune()
{
    Tune t;
    t.title = "prueba";
    t.author = "zxv-tracker";
    t.speed = 4;
    t.tempo = 150;
    t.smpTracks = 2;
    t.pan[1] = 40;
    t.order.assign(2, Position());
    t.order[1].rows = 32;
    Pattern &a = t.ayPatterns[0][0];
    a.rows[0].ch[0].note = 48;
    a.rows[0].ch[0].sample = 1;
    a.rows[0].ch[0].vol = 15;
    a.rows[0].noise = 5;
    a.rows[4].ch[1].note = 52;
    a.rows[4].ch[1].env = 8;
    a.rows[4].envPeriod = 0x123;
    a.rows[8].ch[2].note = 55;
    a.rows[8].ch[2].fx = 1;
    a.rows[8].ch[2].fxDelay = 2;
    a.rows[8].ch[2].fxParam = 0x40;
    for (int i = 0; i < 2; i++) {
        Wave v;
        v.len = 64;
        v.loop = i ? 64 : 0;
        for (int k = 0; k < 64; k++)
            v.data.push_back(uint8_t(i ? (k * 37 & 0xFF) : int8_t(100 * ((k & 31) < 16 ? 1 : -1))));
        t.waves.push_back(v);
        SmpInstrument ins;
        std::memset(ins.data.data(), i, 96);
        t.instruments.push_back(ins);
    }
    t.smpPatterns.assign(2, SmpPattern());
    t.smpPatterns[0].rows[0].note = 48;
    t.smpPatterns[0].rows[0].ins = 1;
    t.smpPatterns[0].rows[8].fx = { '4', 0 };
    t.smpPatterns[0].rows[8].fxp = { 0x44, 0 };
    t.smpPatterns[1].rows[2].note = 60;
    t.smpPatterns[1].rows[2].ins = 2;
    t.smpPatterns[1].rows[2].vol = 32;
    t.smpPatterns[1].rows[10].note = NOTE_CUT;
    for (Position &p : t.order) {
        p.smp[0] = 0;
        p.smp[1] = 1;
    }
    return t;
}

/* Un WAV de 16 bits mono, "rate" Hz, un seno de "hz" Hz de "frames"
 * muestras; con smpl, un bucle de loopFrom a loopTo (incluida). */
static std::vector<uint8_t> makeWav(uint32_t rate, double hz, uint32_t frames, int loopFrom = -1, int loopTo = -1)
{
    std::vector<uint8_t> v;
    auto u32 = [&](uint32_t x) {
        for (int i = 0; i < 4; i++)
            v.push_back(uint8_t(x >> 8 * i));
    };
    auto u16 = [&](uint16_t x) {
        v.push_back(uint8_t(x));
        v.push_back(uint8_t(x >> 8));
    };
    auto tag = [&](const char *t) { v.insert(v.end(), t, t + 4); };
    tag("RIFF");
    u32(0);
    tag("WAVE");
    tag("fmt ");
    u32(16);
    u16(1);
    u16(1);
    u32(rate);
    u32(rate * 2);
    u16(2);
    u16(16);
    if (loopFrom >= 0) {
        tag("smpl");
        u32(60);
        for (int i = 0; i < 7; i++)
            u32(0);
        u32(1);                         /* un bucle */
        u32(0);
        u32(0);                         /* su id */
        u32(0);                         /* hacia delante */
        u32(uint32_t(loopFrom));
        u32(uint32_t(loopTo));
        u32(0);
        u32(0);
    }
    tag("data");
    u32(frames * 2);
    for (uint32_t i = 0; i < frames; i++)
        u16(uint16_t(int16_t(std::lround(20000 * std::sin(i * 2 * M_PI * hz / rate)))));
    uint32_t n = uint32_t(v.size() - 8);
    for (int i = 0; i < 4; i++)
        v[4 + i] = uint8_t(n >> 8 * i);
    return v;
}

/* Un WAV importado: suena a su altura en un C-4 (el paso del canal), con su
 * bucle; en ADPCM se parece al original; a la mitad, la mitad de muestras y
 * a la misma altura; y su instrumento, todas las notas en ella. */
static void wavTests()
{
    Wave w;
    std::string why;
    CHECK(!loadWav(std::vector<uint8_t>(10, 0), 0x01, w, why) && !why.empty());
    std::vector<uint8_t> wav = makeWav(22050, 441, 4410, 100, 3099);
    CHECK(loadWav(wav, 0x01, w, why));
    CHECK(w.len == 3100 && w.loop == 100 && w.format == 0 && w.data.size() == 3100);
    double rate = waveRate(w, 0x01);
    std::printf("WAV: %u muestras, bucle desde %u, nota relativa %d, afinación %d: %.1f Hz\n", w.len, w.loop, w.relnote,
                w.fine, rate);
    CHECK(std::fabs(rate - 22050) < 22050 * 0.003);

    /* En la canción: un C-4 de la pista de muestras, al paso de 22.050 Hz. */
    Tune t;
    t.chips = 0;
    t.smpTracks = 1;
    t.waves.push_back(w);
    CHECK(addInstrument(t, 0) == 1 && t.instruments[0].data[0] == 0 && t.instruments[0].data[95] == 0);
    t.order[0].ay = { { -1, -1 } };
    t.order[0].smp[0] = 0;
    t.smpPatterns[0].rows[0].note = 48;
    t.smpPatterns[0].rows[0].ins = 1;
    Warnings warn;
    std::vector<uint8_t> z = saveZxm(t, warn);
    static mus_song_t s;
    static zxm_t zs;
    std::memset(&s, 0, sizeof s);
    s.zxm = &zs;
    CHECK(mus_song_init(&s, z.data(), uint32_t(z.size())) == ZXV_MUSIC_ZXM);
    mus_ay_t ay[2];
    mus_song_step(&s, ay);
    const mus_smp_t *o = mus_song_samples(&s);
    double want = 22050.0 * 65536 / 48000;
    CHECK(o && o[0].on && std::fabs(o[0].step - want) < want * 0.003);

    /* ADPCM: lo que ocupa, y descodificado se parece. */
    Wave a = w;
    encodeWave(a, 1);
    CHECK(a.format == 1 && a.data.size() == (3100 + 63) / 64 * 36);
    std::vector<int16_t> x = waveSamples(w), y = waveSamples(a);
    double err = 0, sig = 0;
    for (size_t i = 0; i < x.size(); i++) {
        err += double(x[i] - y[i]) * (x[i] - y[i]);
        sig += double(x[i]) * x[i];
    }
    std::printf("ADPCM: %zu bytes, error %.1f dB por debajo\n", a.data.size(), 10 * std::log10(sig / std::max(err, 1.0)));
    CHECK(sig / std::max(err, 1.0) > 100);      /* 20 dB */
    encodeWave(a, 0);
    CHECK(a.data.size() == 3100);

    /* A la mitad: la mitad de muestras, el bucle a la mitad, la misma altura
     * (a la mitad de frecuencia, una octava más alto). */
    Wave h = w;
    halveWave(h);
    CHECK(h.len == 1550 && h.loop == 50 && h.relnote == w.relnote - 12);
    CHECK(std::fabs(waveRate(h, 0x01) * 2 - rate) < 1);
    CHECK(waveBytes(h) == 1552 && wavesBytes(t) == 3100);
}

static void madeTests()
{
    Tune t = madeTune();
    Warnings w;
    std::vector<uint8_t> z = saveZxm(t, w);
    say("hecha", w);
    CHECK(!z.empty());
    Tune back;
    Warnings w2;
    TuneFormat f = FORMAT_NONE;
    CHECK(loadTune(z, back, w2, &f) == TUNE_OK && f == FORMAT_ZXM);
    say("hecha, leída", w2);
    CHECK(back.title == t.title && back.speed == 4 && back.tempo == 150 && back.chips == 1 && back.smpTracks == 2);
    CHECK(back.pan == t.pan && back.waves == t.waves && back.instruments == t.instruments);
    CHECK(back.order.size() == 2 && back.order[1].rows == 32);
    CHECK(back.ay[0].samples == t.ay[0].samples && back.ay[0].ornaments == t.ay[0].ornaments);
    /* El patrón del AY de la primera posición, tal cual. */
    CHECK(back.order[0].ay[0] >= 0 && back.ayPatterns[0][size_t(back.order[0].ay[0])] == t.ayPatterns[0][0]);
    /* Los de muestras, tal cual (el de 64 filas sirve para las dos posiciones). */
    CHECK(back.order[0].smp[0] >= 0 && back.smpPatterns[size_t(back.order[0].smp[0])] == t.smpPatterns[0]);
    CHECK(back.order[1].smp[1] >= 0 && back.smpPatterns[size_t(back.order[1].smp[1])] == t.smpPatterns[1]);
    std::vector<uint8_t> z2 = saveZxm(back, w2);
    CHECK(z2 == z);
    CHECK(!fitsPt3(t));
    Song s;
    Warnings w3;
    CHECK(!toPt3(t, s, w3) && !w3.empty());
    std::printf("hecha: %zu bytes; leída y guardada otra vez, %s\n", z.size(), z2 == z ? "igual" : "DISTINTA");
}

static bool pt3File(const char *name, const std::vector<uint8_t> &orig)
{
    Song s;
    if (load(orig, s) != LOAD_OK) {
        std::printf("%s: no se lee\n", name);
        return false;
    }
    bool ok = true;
    Warnings w;
    Tune t = fromPt3(s, w);
    Song back;
    /* Un TurboSound cuyas mitades no van al mismo paso: el segundo se
     * ajusta al primero (y lo dice). */
    bool fitted = !w.empty();
    if (!toPt3(t, back, w) || (save(back) != save(s) && !fitted)) {
        std::printf("%s: de la canción a PT3, distinto del editor de siempre\n", name);
        say(name, w);
        ok = false;
    }
    std::vector<uint8_t> z = saveZxm(t, w), pt3 = save(s);
    /* Un TurboSound cuyas mitades no van al mismo paso no cabe en un .zxm
     * (zxm_pt3.c, y lo dice): no es un fallo, salvo que se esperase que sí. */
    if (z.empty() && t.chips == 2 && std::string(name).find("tssync") == std::string::npos) {
        std::printf("%s: como .zxm no se puede (se esperaba); PT3 igual\n", name);
        return ok;
    }
    long d = z.empty() ? 0 : sameSound(pt3, z, 200000);
    if (d >= 0) {
        std::printf("%s: como .zxm suena distinto (paso %ld)\n", name, d);
        say(name, w);
        ok = false;
    }
    Tune t2;
    Warnings w2;
    std::vector<uint8_t> z2;
    if (loadTune(z, t2, w2) != TUNE_OK || (z2 = saveZxm(t2, w2)).empty() || sameSound(z, z2, 200000) >= 0) {
        std::printf("%s: el .zxm, leído y guardado, suena distinto\n", name);
        say(name, w2);
        ok = false;
    } else if (saveZxm(t2, w2) != z2) {
        std::printf("%s: el .zxm, leído y guardado dos veces, distinto\n", name);
        ok = false;
    }
    if (ok)
        std::printf("%s: PT3, .zxm y vuelta, igual (%zu -> %zu bytes)\n", name, orig.size(), z.size());
    return ok;
}

static bool moduleFile(const char *name, const std::vector<uint8_t> &orig)
{
    size_t n = 0;
    uint8_t *b = zxm_from_module(orig.data(), uint32_t(orig.size()), &n, nullptr, nullptr);
    if (!b) {
        std::printf("%s: no se convierte\n", name);
        return false;
    }
    std::vector<uint8_t> conv(b, b + n);
    std::free(b);
    Tune t;
    Warnings w;
    TuneFormat f = FORMAT_NONE;
    std::vector<uint8_t> z;
    if (loadTune(orig, t, w, &f) != TUNE_OK || f != FORMAT_MODULE || (z = saveZxm(t, w)).empty()) {
        std::printf("%s: no se lee\n", name);
        say(name, w);
        return false;
    }
    long d = sameSound(conv, z, 100000);
    if (d >= 0) {
        std::printf("%s: leído y guardado suena distinto (paso %ld)\n", name, d);
        say(name, w);
        return false;
    }
    std::printf("%s: leído y guardado, igual (%zu bytes)\n", name, z.size());
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        madeTests();
        wavTests();
        std::printf("%s\n", failures ? "MAL" : "bien");
        return failures ? 1 : 0;
    }
    int bad = 0;
    for (int i = 1; i < argc; i++) {
        std::vector<uint8_t> data = readFile(argv[i]);
        Song s;
        bool pt3 = load(data, s) == LOAD_OK;
        if (!(pt3 ? pt3File(argv[i], data) : moduleFile(argv[i], data)))
            bad++;
    }
    std::printf("%d de %d iguales\n", argc - 1 - bad, argc - 1);
    return bad ? 1 : 0;
}
