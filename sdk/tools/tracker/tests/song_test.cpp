/*
 * song_test.cpp - El módulo de zxv-tracker: leer y volver a compilar.
 *
 *   tracker_song_test                 canciones al azar: compilar, leer y
 *                                     compilar otra vez da lo mismo
 *   tracker_song_test FICHERO.pt3...  cada uno, leído y compilado otra vez,
 *                                     suena igual (sdk/music) que el original
 *   tracker_song_test --estable FICHERO.pt3...
 *                                     solo que, leído otra vez, da lo mismo
 *                                     (los que tienen órdenes que el editor
 *                                     no tiene no suenan igual)
 */
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>

#include "../song.h"

using namespace trk;

static int failures;

#define CHECK(c)                                                                  \
    do {                                                                          \
        if (!(c)) {                                                               \
            std::printf("%s:%d: falla %s\n", __FILE__, __LINE__, #c);             \
            failures++;                                                           \
        }                                                                         \
    } while (0)

static std::mt19937 rng(2026);

static int rnd(int n) { return int(rng() % unsigned(n)); }

static Cell randomCell()
{
    Cell c;
    if (rnd(3) == 0)
        return c;
    int k = rnd(10);
    c.note = k < 6 ? rnd(NOTES) : k < 7 ? NOTE_OFF : NOTE_NONE;
    if (rnd(3) == 0)
        c.sample = 1 + rnd(31);
    if (rnd(4) == 0)
        c.env = 1 + rnd(15);
    if (rnd(4) == 0)
        c.orn = rnd(16);
    if (rnd(3) == 0)
        c.vol = 1 + rnd(15);
    if (rnd(4) == 0) {
        static const int fx[] = { 1, 2, 3, 4, 5, 6, 9, 10, 11 };
        c.fx = fx[rnd(9)];
        bool delay = c.fx <= 3 || c.fx == 9 || c.fx == 10;
        c.fxDelay = delay ? rnd(16) : 0;
        c.fxParam = rnd(256);
        if ((c.fx == 2 || c.fx == 10) && !c.fxParam)
            c.fxParam = 1;              /* -0 es +0: se lee como 1 o 9 */
    }
    return c;
}

static Module randomModule()
{
    Module m;
    m.title = "Titulo " + std::to_string(rnd(1000));
    m.author = rnd(2) ? "zxv" : "";
    m.version = rnd(10);
    m.table = rnd(4);
    m.delay = 1 + rnd(20);
    int npos = 1 + rnd(12);
    m.order.clear();
    int npat = 1 + rnd(6);
    for (int i = 0; i < npos; i++)
        m.order.push_back(rnd(npat) * (rnd(8) ? 1 : 13));
    m.loop = rnd(npos);
    for (int p : m.order) {
        Pattern &pat = m.patterns[p];
        int len = rnd(10) ? 1 + rnd(64) : 1 + rnd(MAX_ROWS);
        pat.rows.assign(len, Row());
        for (Row &r : pat.rows) {
            if (rnd(2))
                continue;
            for (Cell &c : r.ch)
                c = randomCell();
            for (const Cell &c : r.ch)
                if (c.env >= 1 && c.env <= 14)
                    r.envPeriod = rnd(0x10000);
            if (rnd(6) == 0)
                r.noise = rnd(32);
        }
    }
    for (int i = 0; i < SAMPLES; i++) {
        if (rnd(3) == 0)
            continue;
        Sample &s = m.samples[i];
        s.lines.clear();
        int len = 1 + rnd(MAX_LINES);
        for (int j = 0; j < len; j++) {
            SampleLine l;
            l.b0 = uint8_t(rnd(256));
            l.b1 = uint8_t(rnd(256));
            l.tone = int16_t(rnd(65536) - 32768);
            s.lines.push_back(l);
        }
        if (len == 1 && s.lines[0].b0 == 0x01 && s.lines[0].b1 == 0x90 && !s.lines[0].tone)
            s.lines[0].b1 = 0x91;           /* el que no suena se lee como ninguno */
        s.loop = rnd(len);
    }
    for (int i = 1; i < ORNAMENTS; i++) {
        if (rnd(2) == 0)
            continue;
        Ornament &o = m.ornaments[i];
        int len = 2 + rnd(MAX_LINES - 1);
        for (int j = 0; j < len; j++)
            o.offsets.push_back(rnd(256) - 128);
        o.loop = rnd(len);
    }
    return m;
}

static void describe(const Song &s, const Song &back)
{
    for (size_t mi = 0; mi < s.modules.size(); mi++) {
        const Module &a = s.modules[mi], &b = back.modules[mi];
        for (int p = 0; p < MAX_PATTERNS; p++)
            if (!(a.patterns[p] == b.patterns[p])) {
                std::printf("  módulo %zu, patrón %d (%zu / %zu filas)\n", mi, p, a.patterns[p].rows.size(),
                            b.patterns[p].rows.size());
                for (size_t r = 0; r < a.patterns[p].rows.size() && r < b.patterns[p].rows.size(); r++)
                    if (a.patterns[p].rows[r] != b.patterns[p].rows[r]) {
                        for (int ch = 0; ch < 3; ch++) {
                            const Cell &x = a.patterns[p].rows[r].ch[ch], &y = b.patterns[p].rows[r].ch[ch];
                            std::printf("   fila %zu canal %d: %d %d %d %d %d %d/%d/%d -> %d %d %d %d %d %d/%d/%d\n",
                                        r, ch, x.note, x.sample, x.env, x.orn, x.vol, x.fx, x.fxDelay,
                                        x.fxParam, y.note, y.sample, y.env, y.orn, y.vol, y.fx, y.fxDelay,
                                        y.fxParam);
                        }
                        break;
                    }
                break;
            }
        if (!(a.samples == b.samples))
            std::printf("  módulo %zu: los samples\n", mi);
        if (!(a.ornaments == b.ornaments))
            std::printf("  módulo %zu: los ornamentos\n", mi);
        if (a.order != b.order || a.loop != b.loop || a.title != b.title || a.author != b.author)
            std::printf("  módulo %zu: la cabecera\n", mi);
    }
}

static void randomSongs()
{
    for (int t = 0; t < 300; t++) {
        Song s;
        s.modules[0] = randomModule();
        if (t % 5 == 4)
            s.modules.push_back(randomModule());
        std::vector<uint8_t> bytes = save(s);
        Song back;
        int e = load(bytes, back);
        CHECK(e == LOAD_OK);
        if (e != LOAD_OK)
            continue;
        if (!(back == s)) {
            std::printf("canción %d: no se lee igual\n", t);
            failures++;
            describe(s, back);
            if (failures > 5)
                return;
            continue;
        }
        CHECK(save(back) == bytes);
        CHECK(compareSound(bytes, save(back), 3000) == -1);
    }
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

int main(int argc, char **argv)
{
    if (argc < 2) {
        randomSongs();
        std::printf("%s\n", failures ? "MAL" : "bien");
        return failures ? 1 : 0;
    }
    int bad = 0;
    bool stableOnly = std::string(argv[1]) == "--estable";
    for (int i = stableOnly ? 2 : 1; i < argc; i++) {
        std::vector<uint8_t> orig = readFile(argv[i]);
        Song s;
        int e = load(orig, s);
        if (e != LOAD_OK) {
            std::printf("%s: no se lee (%d)\n", argv[i], e);
            bad++;
            continue;
        }
        std::vector<uint8_t> again = save(s);
        long steps = songSteps(orig, 100000) * 2 + 200;
        long diff = compareSound(orig, again, steps);
        Song s2;
        bool same = load(again, s2) == LOAD_OK && s2 == s && save(s2) == again;
        if (!same && getenv("DESCRIBE"))
            describe(s, s2);
        std::printf("%s: %ld pasos, %zu -> %zu bytes, %s%s\n", argv[i], steps, orig.size(), again.size(),
                    diff < 0 ? "suena igual" : ("distinto desde el paso " + std::to_string(diff)).c_str(),
                    same ? "" : ", y no se vuelve a leer igual");
        if ((diff >= 0 && !stableOnly) || !same)
            bad++;
    }
    return bad ? 1 : 0;
}
