/*
 * sfx_test.cpp - Los efectos AYFX de zxv-tracker: leer y volver a guardar.
 *
 *   tracker_sfx_test                  bancos al azar: guardar y leer da lo mismo
 *   tracker_sfx_test FICHERO.afb...   cada uno, leído y guardado otra vez,
 *                                     suena igual (sdk/music/afx.c)
 */
#include <cstdio>
#include <random>

#include "../sfx.h"

extern "C" {
#include "music_int.h"
}

using namespace fx;

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

/* Lo que pone en el AY un efecto, paso a paso (como music.c). */
static std::vector<int> sound(const uint8_t *p, const uint8_t *end)
{
    std::vector<int> out;
    afx_t a;
    afx_start(&a, p, end);
    for (int i = 0; i < 100000; i++) {
        int how = afx_step(&a);
        if (!how)
            break;
        out.push_back(how);
        out.push_back(a.vol);
        if (how == 2)
            break;
        out.push_back(a.has_tone ? a.tone : -1);
        out.push_back(a.has_noise ? a.noise : -1);
        out.push_back(a.mix);
    }
    return out;
}

int main(int argc, char **argv)
{
    int bad = 0;
    if (argc < 2) {
        std::mt19937 rng(7);
        for (int t = 0; t < 500; t++) {
            Bank b;
            int n = 1 + int(rng() % 40);
            for (int i = 0; i < n; i++) {
                Effect e;
                e.name = rng() % 3 ? "efecto " + std::to_string(i) : "";
                int len = int(rng() % 60);
                for (int j = 0; j < len; j++) {
                    Frame f;
                    f.volume = int(rng() % 16);
                    f.toneOn = rng() % 2;
                    f.noiseOn = rng() % 2;
                    f.tone = int(rng() % 4) ? int(rng() % 0x1000) : (j ? 0 : 0);
                    f.noise = int(rng() % 32);
                    e.frames.push_back(f);
                }
                b.effects.push_back(e);
            }
            Bank back;
            if (!loadBank(saveBank(b), back) || !(back == b)) {
                std::printf("banco %d: no se lee igual\n", t);
                bad++;
            }
        }
        std::printf("%s\n", bad ? "MAL" : "bien");
        return bad ? 1 : 0;
    }
    for (int i = 1; i < argc; i++) {
        std::vector<uint8_t> d = readFile(argv[i]);
        Bank b;
        if (!loadBank(d, b)) {
            std::printf("%s: no se lee\n", argv[i]);
            bad++;
            continue;
        }
        std::vector<uint8_t> again = saveBank(b);
        int same = 0, names = 0;
        for (size_t k = 0; k < b.effects.size(); k++) {
            int c1 = 0, c2 = 0;
            const uint8_t *p1 = afx_find(d.data(), uint32_t(d.size()), int(k), &c1);
            const uint8_t *p2 = afx_find(again.data(), uint32_t(again.size()), int(k), &c2);
            if (p1 && p2 && sound(p1, d.data() + d.size()) == sound(p2, again.data() + again.size()))
                same++;
            names += !b.effects[k].name.empty();
        }
        Bank b2;
        bool stable = loadBank(again, b2) && b2 == b && saveBank(b2) == again;
        std::printf("%s: %zu efectos (%d con nombre), %d suenan igual, %zu -> %zu bytes%s\n", argv[i],
                    b.effects.size(), names, same, d.size(), again.size(), stable ? "" : ", no se vuelve a leer igual");
        if (same != int(b.effects.size()) || !stable)
            bad++;
    }
    return bad ? 1 : 0;
}
