/*
 * engine_test.cpp - El sonido de zxv-tracker (engine.cpp): un .zxm con una
 * pista de muestras suena por los canales de muestras de la ULA, a su tempo,
 * dice por qué fila va, puede empezar en otra posición, y se calla al
 * silenciar su pista y al parar; y un PT3 sigue sonando en el AY.
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "../engine.h"
#include "../song.h"

extern "C" {
#include "zxm_song.h"
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

/* Un .zxm de una pista de muestras: un seno de 32 muestras en bucle, y en
 * cada una de las dos posiciones (8 filas) un C-4 en la fila 0 y un C-5 en
 * la 4. */
static std::vector<uint8_t> sineSong()
{
    zxms_t s;
    zxms_init(&s);
    s.smp_tracks = 1;
    s.speed = 3;
    int8_t wave[32];
    for (int i = 0; i < 32; i++)
        wave[i] = int8_t(std::lround(100 * std::sin(i * 2 * M_PI / 32)));
    zxms_sample_t smp {};
    smp.len = 32;
    smp.loop = 0;
    smp.vol = 64;
    smp.data = reinterpret_cast<uint8_t *>(wave);
    smp.size = 32;
    zxms_add_sample(&s, &smp);
    zxms_add_smp_instrument(&s, 0);
    int p = zxms_add_pattern(&s, 1, 8);
    for (int r : { 0, 4 }) {
        zxms_cell_t &c = s.pat[p].cells[r];
        c.has = ZXM_C_NOTE | ZXM_C_INS;
        c.note = uint8_t(r ? 61 : 49);
        c.ins = 1;
    }
    s.npos = 2;
    for (int i = 0; i < 2; i++) {
        s.pos[i].rows = 8;
        for (int t = 0; t < ZXMS_TRACKS; t++)
            s.pos[i].pat[t] = t == 0 ? p : -1;
    }
    size_t n = 0;
    uint8_t *b = zxms_save(&s, &n);
    std::vector<uint8_t> v(b, b + n);
    std::free(b);
    zxms_free(&s);
    return v;
}

/* El nivel (RMS) de "frames" muestras de la izquierda. */
static double level(Engine &e, int frames)
{
    std::vector<int16_t> buf(size_t(frames) * 2);
    e.render(buf.data(), frames);
    double sum = 0;
    for (int i = 0; i < frames; i++)
        sum += double(buf[size_t(i) * 2]) * buf[size_t(i) * 2];
    return std::sqrt(sum / frames);
}

int main()
{
    std::vector<uint8_t> zxm = sineSong();
    {
        Engine e;
        Engine::Play p;
        p.data = zxm;
        CHECK(e.play(p));
        double on = level(e, Engine::RATE / 4);
        Engine::Status s = e.status();
        std::printf("el .zxm: nivel %.0f, canal 0 a %d, fila %d\n", on, s.smpVol[0], s.row[0]);
        CHECK(on > 500);
        CHECK(s.playing && s.smpVol[0] == 64);
        /* A 125 (50 ticks por segundo) y 3 ticks por fila: en 1/4 s, la fila 4. */
        CHECK(s.pos[0] == 0 && s.row[0] == 4);
        e.setMute(1 << 6);
        level(e, Engine::STEP * 4);
        CHECK(level(e, Engine::STEP * 4) < 20);
        e.setMute(0);
        level(e, Engine::STEP * 4);
        CHECK(level(e, Engine::STEP * 4) > 500);
        e.stop();
        level(e, Engine::STEP);
        CHECK(level(e, Engine::STEP * 4) < 20 && !e.status().playing);
    }
    {
        Engine e;
        Engine::Play p;
        p.data = zxm;
        p.pos = 1;
        p.row = 2;
        CHECK(e.play(p));
        level(e, 10);
        Engine::Status s = e.status();
        std::printf("desde la posición 1, fila 2: va por %d, %d\n", s.pos[0], s.row[0]);
        CHECK(s.pos[0] == 1 && s.row[0] == 2);
    }
    {
        /* Un PT3: el módulo de un sample que suena, con un C-4. */
        Song song;
        Module &m = song.modules[0];
        m.patterns[0].rows[0].ch[0].note = 48;
        m.patterns[0].rows[0].ch[0].sample = 1;
        m.patterns[0].rows[0].ch[0].vol = 15;
        Engine e;
        Engine::Play p;
        p.data = save(song);
        p.rows = { { 64 } };
        CHECK(e.play(p));
        double on = level(e, Engine::RATE / 4);
        std::printf("el PT3: nivel %.0f\n", on);
        CHECK(on > 500 && e.status().regs[0][8] != 0);
    }
    std::printf("%s\n", failures ? "MAL" : "bien");
    return failures ? 1 : 0;
}
