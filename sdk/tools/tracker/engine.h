/*
 * engine.h - El sonido de zxv-tracker, sin Qt: los módulos con sdk/music
 * (paso a paso, como en la máquina), los efectos AYFX encima, y los dos AY y
 * los canales de muestras de la ULA (sdk/sound/sndsynth.c, el mismo que
 * suena en la máquina) a 48 kHz.
 *
 * Dos relojes, como music.c: la canción va a su tempo (un .zxm; un PT3, a
 * 50 pasos por segundo) y los efectos y la nota que se oye al editar, a 50.
 *
 * render() lo llama la tarjeta de sonido (otro hilo); lo demás, la ventana.
 * Todo va con un cerrojo.
 */
#ifndef ZXV_TRACKER_ENGINE_H
#define ZXV_TRACKER_ENGINE_H

#include <cstdint>
#include <mutex>
#include <vector>

extern "C" {
#include "music_int.h"
#include "sndsynth.h"
#include "zxv_music.h"
}

namespace trk {

class Engine
{
public:
    static constexpr int RATE = 48000;
    static constexpr int STEP = RATE / 50;     /* muestras por paso de 1/50 s */
    static constexpr int TRACKS = 18;          /* 6 del AY y 12 de muestras */

    struct Play {
        std::vector<uint8_t> data;              /* el .pt3 (o un TurboSound) o el .zxm */
        /* Un PT3: por módulo, las filas del patrón de cada posición, para
         * saber por dónde va (un .zxm lo dice él). */
        std::vector<std::vector<int>> rows;
        int pos = 0, row = 0;                   /* desde dónde */
        bool loop = true;                       /* al acabar, por la vuelta */
    };

    struct Status {
        bool playing = false, preview = false, sfx = false;
        int chips = 1;
        int pos[2] = { 0, 0 }, row[2] = { -1, -1 };     /* un .zxm: en el 0 */
        uint8_t regs[2][14] = {};
        uint8_t smpVol[12] = {};                /* lo que suena en cada canal de muestras: 0-64 */
        long steps = 0;
    };

    Engine();

    bool play(Play p);
    void stop();
    /* Un módulo que suena mientras no se pare (la nota o el sample que se
     * edita): un PT3 en el AY "chip", o un .zxm tal cual. */
    bool preview(const std::vector<uint8_t> &data, int chip);
    void stopPreview();
    /* Un efecto AYFX suelto (los bytes de un .afx) en un canal 0-5. */
    void playSfx(const std::vector<uint8_t> &afx, int channel);
    void stopSfx();

    /* Pistas que no suenan: bit n, la pista n (0-5 los canales de los AY,
     * 6-17 los de muestras). */
    void setMute(int mask);
    void setStereo(int mode);                   /* 0 mono, 1 ABC, 2 ACB */
    void setVolume(int v);                      /* 0-255 */

    /* Muestras estéreo (izquierda, derecha) a RATE. */
    void render(int16_t *out, int frames);
    Status status();

private:
    struct Voice {
        Voice() { song.zxm = &zxm; }
        mus_song_t song{};
        zxm_t zxm{};                    // el estado de un .zxm (mus_song_t.zxm)
        std::vector<uint8_t> data;
        bool on = false;
        bool samples = false;           // sus muestras están en la memoria de sonido
    };

    bool isZxm(const Voice &v) const { return v.song.format == ZXV_MUSIC_ZXM; }
    void loadSamples(Voice &v);
    void musicStep();
    void fxStep();
    void startAt(int pos, int row);
    void advance(bool audible);
    void emitSamples(const Voice &v, bool audible);
    void silenceSamples();
    void flush();
    void write(int reg, uint8_t val);

    std::mutex mutex_;
    Voice music_, note_;
    std::vector<std::vector<int>> rows_;
    bool loop_ = true;
    int noteChip_ = 0;
    int pos_[2] = { 0, 0 }, row_[2] = { -1, -1 };
    long steps_ = 0;
    std::vector<uint8_t> sfxData_;
    afx_t sfx_ {};
    int sfxHow_ = 0;
    int sfxChannel_ = 0;
    mus_ay_t music_ay_[2] {}, note_ay_ {}, out_[2] {};
    int mute_ = 0, stereo_ = 1, volume_ = 200;

    /* La ULA: el sintetizador, su memoria de sonido y lo que se le manda en
     * cada paso (escrituras de sndlink.h, todas antes de la primera muestra). */
    sndsynth_t snd_ {};
    std::vector<uint8_t> mem_;
    std::vector<uint32_t> events_;              /* las que esperan al paso siguiente */
    std::vector<uint32_t> inflight_;            /* las del tramo de snd_ (apunta aquí) */
    uint8_t sent_[2][14] = {};
    uint16_t live_ = 0;                         /* canales de muestras que suenan */
    uint8_t smpVol_[12] = {};
    bool fresh_ = true;                         /* hay que escribir todos los registros */
    int musicLeft_ = 0, fxLeft_ = 0;            /* muestras que faltan de cada paso */
    uint32_t musicFrac_ = 0;                    /* lo que sobra del paso de la canción, en 1/1.000.000 */
};

} // namespace trk

#endif // ZXV_TRACKER_ENGINE_H
