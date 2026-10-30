/*
 * song.h - Lo que edita zxv-tracker: un módulo PT3 (o dos, un TurboSound)
 * con sus patrones, samples y ornamentos, sin Qt.
 *
 * Los patrones son filas de tres canales, como en Vortex Tracker II: nota,
 * sample, envolvente, ornamento, volumen y un efecto; y, por fila, el
 * periodo de la envolvente y el ruido. load() pasa los bytes de un .pt3 a
 * eso y save() lo compila de vuelta a un .pt3 que tocan sdk/music, Vortex
 * Tracker y los reproductores del Spectrum.
 *
 * Los efectos tienen los números de Vortex Tracker:
 *   1 y 2  el tono baja / sube (cada "retardo" pasos, "parámetro")
 *   3      portamento hasta la nota
 *   4 y 5  la posición en el sample / en el ornamento
 *   6      vibrato: XY, X pasos suena e Y no
 *   9 y A  la envolvente baja / sube
 *   B      el tempo
 */
#ifndef ZXV_TRACKER_SONG_H
#define ZXV_TRACKER_SONG_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace trk {

constexpr int MAX_ROWS = 256;           /* filas por patrón */
constexpr int MAX_PATTERNS = 85;        /* patrón × 3 cabe en un byte */
constexpr int MAX_POSITIONS = 255;
constexpr int SAMPLES = 32;             /* el 0 no se puede elegir en los patrones */
constexpr int ORNAMENTS = 16;
constexpr int MAX_LINES = 64;           /* líneas de un sample o un ornamento */
constexpr int NOTES = 96;

constexpr int NOTE_NONE = -1, NOTE_OFF = -2;
constexpr int ENV_NONE = 0, ENV_OFF = 15;

struct Cell {
    int note = NOTE_NONE;   /* 0-95 (C-1 a B-8), NOTE_OFF o nada */
    int sample = 0;         /* 1-31, 0 nada */
    int env = ENV_NONE;     /* 1-14 la forma de la envolvente, 15 sin ella */
    int orn = -1;           /* 0-15, -1 nada */
    int vol = 0;            /* 1-15, 0 nada */
    int fx = 0;             /* el efecto (los números de arriba), 0 nada */
    int fxDelay = 0;
    int fxParam = 0;
    bool empty() const
    {
        return note == NOTE_NONE && !sample && env == ENV_NONE && orn < 0 && !vol && !fx;
    }
    bool operator==(const Cell &o) const;
    bool operator!=(const Cell &o) const { return !(*this == o); }
};

struct Row {
    Cell ch[3];
    int envPeriod = -1;     /* 0-0xFFFF, con una envolvente en la fila */
    int noise = -1;         /* 0-31, -1 nada */
    bool empty() const { return ch[0].empty() && ch[1].empty() && ch[2].empty() && envPeriod < 0 && noise < 0; }
    bool operator==(const Row &o) const;
    bool operator!=(const Row &o) const { return !(*this == o); }
};

struct Pattern {
    std::vector<Row> rows = std::vector<Row>(64);
    bool empty() const;
    bool operator==(const Pattern &o) const { return rows == o.rows; }
};

/* Una línea de un sample: los cuatro bytes del PT3, y lo que dicen. */
struct SampleLine {
    uint8_t b0 = 0x01, b1 = 0x90;
    int16_t tone = 0;

    bool toneOn() const { return !(b1 & 0x10); }
    bool noiseOn() const { return !(b1 & 0x80); }
    bool envOn() const { return !(b0 & 0x01); }
    bool toneAcc() const { return b1 & 0x40; }
    bool shiftAcc() const { return b1 & 0x20; }
    int volume() const { return b1 & 15; }
    int volSlide() const { return b0 & 0x80 ? (b0 & 0x40 ? 1 : -1) : 0; }
    /* El desplazamiento del ruido (con el ruido) o de la envolvente: -16..15. */
    int shift() const { int v = b0 >> 1 & 31; return v >= 16 ? v - 32 : v; }

    void setToneOn(bool on) { b1 = on ? b1 & ~0x10 : b1 | 0x10; }
    void setNoiseOn(bool on) { b1 = on ? b1 & ~0x80 : b1 | 0x80; }
    void setEnvOn(bool on) { b0 = on ? b0 & ~0x01 : b0 | 0x01; }
    void setToneAcc(bool on) { b1 = on ? b1 | 0x40 : b1 & ~0x40; }
    void setShiftAcc(bool on) { b1 = on ? b1 | 0x20 : b1 & ~0x20; }
    void setVolume(int v) { b1 = uint8_t((b1 & 0xF0) | (v & 15)); }
    void setVolSlide(int s) { b0 = uint8_t((b0 & 0x3F) | (s ? 0x80 : 0) | (s > 0 ? 0x40 : 0)); }
    void setShift(int v) { b0 = uint8_t((b0 & 0xC1) | (v & 31) << 1); }
    bool operator==(const SampleLine &o) const { return b0 == o.b0 && b1 == o.b1 && tone == o.tone; }
};

struct Sample {
    int loop = 0;
    std::vector<SampleLine> lines;      /* vacío: no hay */
    bool operator==(const Sample &o) const { return loop == o.loop && lines == o.lines; }
};

struct Ornament {
    int loop = 0;
    std::vector<int> offsets;           /* en semitonos; vacío: no hay */
    bool operator==(const Ornament &o) const { return loop == o.loop && offsets == o.offsets; }
};

struct Module {
    std::string title, author;
    int version = 6;        /* el 3.x de Pro Tracker: cambia algunos efectos */
    int table = 2;          /* la tabla de notas, 0-3 */
    int delay = 3;          /* el tempo: pasos de 1/50 s por fila */
    int loop = 0;           /* la posición a la que vuelve */
    std::vector<int> order = { 0 };     /* las posiciones: un patrón cada una */
    std::vector<Pattern> patterns = std::vector<Pattern>(MAX_PATTERNS);
    std::array<Sample, SAMPLES> samples;
    std::array<Ornament, ORNAMENTS> ornaments;

    Module();
    bool operator==(const Module &o) const;
};

/* Un módulo, o dos (TurboSound: uno en cada AY, a la vez). */
struct Song {
    std::vector<Module> modules = std::vector<Module>(1);
    bool operator==(const Song &o) const { return modules == o.modules; }
};

enum { LOAD_OK, LOAD_NOT_PT3, LOAD_DAMAGED };

/* Un .pt3 (también un TurboSound). Si no es LOAD_OK, song no cambia. */
int load(const std::vector<uint8_t> &data, Song &song);
std::vector<uint8_t> save(const Song &song);
std::vector<uint8_t> saveModule(const Module &m);

/* Toca los dos ficheros (con sdk/music) y da el primer paso de 1/50 s en que
 * los registros del AY no son iguales, o -1 si lo son los "steps" primeros. */
long compareSound(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b, long steps);

/* Los pasos que dura una vuelta a la canción (hasta que vuelve a su
 * posición de vuelta), como mucho "limit". */
long songSteps(const std::vector<uint8_t> &data, long limit);

/* Las notas como en Vortex Tracker: "C-4", "C#4"... */
std::string noteName(int note);

/* Un módulo para oír una nota mientras se edita: la fila "c" en el canal
 * "channel", con los samples y ornamentos de "m", que suena hasta que se
 * pare. */
std::vector<uint8_t> previewModule(const Module &m, const Cell &c, int envPeriod, int channel);

/* Una muestra y un ornamento que suenan: los de un módulo nuevo. */
Sample defaultSample();
Ornament defaultOrnament();

} // namespace trk

#endif // ZXV_TRACKER_SONG_H
