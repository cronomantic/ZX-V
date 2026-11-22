/*
 * tune.h - La canción que edita zxv-tracker, sin Qt: la de un .zxm (D98),
 * con los AY como en Vortex Tracker y hasta 12 pistas de muestras.
 *
 * - Los AY (uno, o dos: un TurboSound) van por grupos: cada AY, sus tres
 *   canales con la envolvente y el ruido de la fila, en patrones como los
 *   del PT3 (song.h: Pattern). Cada AY tiene sus samples y sus ornamentos,
 *   su versión del PT3 y su tabla de notas, como los dos módulos de un
 *   TurboSound.
 * - Cada pista de muestras tiene sus patrones: nota, instrumento, volumen
 *   y dos efectos (los del MOD, el S3M y el XM: zxm.c), por fila.
 * - Cada posición dice cuántas filas tiene y qué patrón toca cada grupo
 *   (cada AY y cada pista de muestras; -1, nada), como en Furnace.
 *
 * Se lee de un .zxm, de un PT3 o un TurboSound y, con los conversores de
 * sdk/tools/music, de un MOD, un S3M o un XM; se guarda como .zxm o, si
 * solo usa los AY, como .pt3.
 */
#ifndef ZXV_TRACKER_TUNE_H
#define ZXV_TRACKER_TUNE_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "song.h"

namespace trk {

constexpr int SMP_TRACKS = 12;
constexpr int MAX_TUNE_POSITIONS = 256;
constexpr int NOTE_CUT = -3;            /* en las pistas de muestras: cortar (^^^); NOTE_OFF, soltar */
constexpr int SINS_SIZE = 208;          /* un instrumento de muestras, como en el .zxm (ZXM_SI_*) */

/* Una fila de una pista de muestras. */
struct SmpCell {
    int note = NOTE_NONE;   /* 0-95 (C-0 a B-7), NOTE_OFF, NOTE_CUT o nada */
    int ins = 0;            /* 1-128, 0 nada */
    int vol = -1;           /* 0-64, -1 nada */
    std::array<uint8_t, 2> fx {};       /* el código (el carácter del .zxm), 0 nada */
    std::array<uint8_t, 2> fxp {};      /* su parámetro */
    bool empty() const { return note == NOTE_NONE && !ins && vol < 0 && !fx[0] && !fx[1]; }
    bool operator==(const SmpCell &o) const
    {
        return note == o.note && ins == o.ins && vol == o.vol && fx == o.fx && fxp == o.fxp;
    }
    bool operator!=(const SmpCell &o) const { return !(*this == o); }
};

struct SmpPattern {
    std::vector<SmpCell> rows = std::vector<SmpCell>(64);
    bool operator==(const SmpPattern &o) const { return rows == o.rows; }
};

/* Una muestra: lo de su entrada en la tabla del .zxm y sus datos como los
 * quiere el chip (8 bits con signo, o ADPCM en bloques de 64). */
struct Wave {
    std::string name;
    uint32_t len = 0, loop = 0;         /* en muestras; loop >= len: sin bucle */
    int format = 0;                     /* 0 8 bits, 1 ADPCM */
    int relnote = 0, fine = 0;          /* la nota relativa y la afinación (1/128 de semitono) */
    int vol = 64, pan = 128;
    bool hasPan = false;                /* trae su panorama (si no, el de la pista) */
    std::vector<uint8_t> data;
    bool operator==(const Wave &o) const
    {
        return name == o.name && len == o.len && loop == o.loop && format == o.format && relnote == o.relnote &&
               fine == o.fine && vol == o.vol && pan == o.pan && hasPan == o.hasPan && data == o.data;
    }
};

/* Un instrumento de muestras: los 208 bytes del .zxm (ZXM_SI_*: la muestra
 * de cada nota, las envolventes, el fundido y el vibrato). */
struct SmpInstrument {
    std::string name;
    std::array<uint8_t, SINS_SIZE> data {};
    bool operator==(const SmpInstrument &o) const { return name == o.name && data == o.data; }
};

/* Los samples, los ornamentos, la versión y la tabla de un AY; y el
 * título y el autor de su módulo en un TurboSound (los del segundo; los
 * del primero son los de la canción). */
struct AyBank {
    std::array<Sample, SAMPLES> samples;
    std::array<Ornament, ORNAMENTS> ornaments;
    int version = 6, table = 2;
    std::string title, author;
    AyBank();
    bool operator==(const AyBank &o) const
    {
        return samples == o.samples && ornaments == o.ornaments && version == o.version && table == o.table &&
               title == o.title && author == o.author;
    }
};

struct Position {
    int rows = 64;                              /* 1-256 */
    std::array<int, 2> ay { { 0, -1 } };        /* el patrón de cada AY (ayPatterns[k]), -1 nada */
    std::array<int, SMP_TRACKS> smp;            /* el de cada pista de muestras (smpPatterns), -1 nada */
    Position() { smp.fill(-1); }
    bool operator==(const Position &o) const { return rows == o.rows && ay == o.ay && smp == o.smp; }
};

struct Tune {
    std::string title, author;
    int speed = 3;              /* pasos por fila (el «retardo» del PT3) */
    int tempo = 125;            /* 32-255: 125, 50 pasos por segundo */
    int loop = 0;               /* la posición a la que vuelve */
    int chips = 1;              /* AY que usa: 0, 1 o 2 */
    int smpTracks = 0;          /* pistas de muestras: 0-12 */
    /* Los de las muestras: ZXM_F_LINEAR, _NOINTERP, _S3MCLOCK, _ST300, _MODFX.
     * Una canción nueva, con el tono lineal (como el XM): un C-4 suena a
     * 8.363 Hz movidos la nota relativa y la afinación de la muestra. */
    int flags = 0x01;
    std::array<uint8_t, SMP_TRACKS> pan;        /* el panorama de cada pista al empezar */
    std::array<AyBank, 2> ay;
    std::vector<Position> order = std::vector<Position>(1);
    /* Los patrones de cada AY (los números, como en su módulo del PT3) y
     * los de las pistas de muestras (todas los pueden usar). */
    std::array<std::vector<Pattern>, 2> ayPatterns { { std::vector<Pattern>(1), std::vector<Pattern>(1) } };
    std::vector<SmpPattern> smpPatterns = std::vector<SmpPattern>(1);
    std::vector<SmpInstrument> instruments;     /* el 1 es el instrumento [0]... */
    std::vector<Wave> waves;

    Tune() { pan.fill(128); }
    bool operator==(const Tune &o) const;
    bool operator!=(const Tune &o) const { return !(*this == o); }
};

/* Lo que no se ha podido pasar (o se ha cambiado al leer o guardar), una
 * vez por cosa. */
using Warnings = std::vector<std::string>;

enum { TUNE_OK, TUNE_UNKNOWN, TUNE_DAMAGED };
enum TuneFormat { FORMAT_NONE, FORMAT_ZXM, FORMAT_PT3, FORMAT_MODULE };

/* Un .zxm, un PT3 o un TurboSound, o un MOD, un S3M o un XM (convertidos:
 * en *format, cuál era; channels, los canales que van, el bit c el canal c,
 * o 0 para que los elija: de más de 12, los que más notas tocan). Si no es
 * TUNE_OK, t no cambia. */
int loadTune(const std::vector<uint8_t> &data, Tune &t, Warnings &w, TuneFormat *format = nullptr,
             uint64_t channels = 0);

/* Los canales de un MOD, un S3M o un XM, para elegir cuáles van: los del
 * fichero (count, 0 si no es uno de ellos), los que tienen algo (used), los
 * que irían sin elegirlos (picked) y las notas que toca cada uno en la
 * canción (de uno con más de 12 con algo). */
struct ModuleChannels {
    int count = 0;
    uint64_t used = 0, picked = 0;
    uint32_t notes[32] = {};
    /* Si hay que elegirlos: más de 12 con algo. */
    bool tooMany() const;
};
ModuleChannels moduleChannels(const std::vector<uint8_t> &data);

/* Un PT3 (o un TurboSound) ya leído (song.h) a la canción, tal cual (de
 * un TurboSound cuyas mitades no van al mismo paso, el segundo se ajusta
 * al primero, y lo dice w). */
Tune fromPt3(const Song &s, Warnings &w);

/* El .zxm de la canción (vacío si no se puede: lo dice w). */
std::vector<uint8_t> saveZxm(const Tune &t, Warnings &w);

/* La canción como PT3 (o TurboSound), si solo usa los AY y cabe (85
 * patrones, el tempo de 50 pasos por segundo): si no, false y lo dice w. */
bool toPt3(const Tune &t, Song &s, Warnings &w);

/* Si se puede guardar como PT3 (sin decir por qué no). */
bool fitsPt3(const Tune &t);

/* --- Las muestras ------------------------------------------------------------ */

constexpr uint32_t SOUND_MEM = 0x18000;    /* la memoria de sonido: 96 KB */

/* Lo que ocupa una muestra en la memoria de sonido (de 4 en 4 bytes). */
uint32_t waveBytes(const Wave &w);
uint32_t wavesBytes(const Tune &t);

/* Un WAV (8 o 16 bits, mono o estéreo, a cualquier frecuencia; con su bucle
 * si trae un bloque smpl) a una muestra de 8 bits, afinada para que un C-4
 * suene a su altura con los "flags" de la canción. false si no es un WAV así
 * (y por qué, en why). */
bool loadWav(const std::vector<uint8_t> &data, int flags, Wave &w, std::string &why);

/* Las muestras de una, en 16 bits (para verla, o para volver a codificarla). */
std::vector<int16_t> waveSamples(const Wave &w);
/* La misma muestra en otro formato (0 8 bits, 1 ADPCM). */
void encodeWave(Wave &w, int format);
/* La mitad de las muestras (cada dos, su media) y una octava más grave de
 * afinación: suena igual con menos agudos y ocupa la mitad. */
void halveWave(Wave &w);
/* La frecuencia a la que suena un C-4 (en Hz). */
double waveRate(const Wave &w, int flags);

/* Un instrumento de muestras con todas las notas en la muestra "wave", sin
 * envolventes; da su número (1-), o 0 si ya hay 128. */
int addInstrument(Tune &t, int wave);

} // namespace trk

#endif // ZXV_TRACKER_TUNE_H
