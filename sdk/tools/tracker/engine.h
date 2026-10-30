/*
 * engine.h - El sonido de zxv-tracker, sin Qt: los módulos con sdk/music
 * (paso a paso, como en la máquina), los efectos AYFX encima y los dos AY
 * del firmware (pico/common/aysynth.c) a 48 kHz.
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
#include "aysynth.h"
#include "music_int.h"
}

namespace trk {

class Engine
{
public:
    static constexpr int RATE = 48000;
    static constexpr int STEP = RATE / 50;     /* muestras por paso de 1/50 s */

    struct Play {
        std::vector<uint8_t> data;              /* el .pt3 (o un TurboSound) */
        /* Por módulo, las filas del patrón de cada posición: para saber por
         * dónde va. */
        std::vector<std::vector<int>> rows;
        int pos = 0, row = 0;                   /* desde dónde */
        bool loop = true;                       /* al acabar, por la vuelta */
    };

    struct Status {
        bool playing = false, preview = false, sfx = false;
        int chips = 1;
        int pos[2] = { 0, 0 }, row[2] = { -1, -1 };
        uint8_t regs[2][14] = {};
        long steps = 0;
    };

    Engine();

    bool play(Play p);
    void stop();
    /* Un módulo que suena en el AY "chip" mientras no se pare (la nota o el
     * sample que se edita). */
    bool preview(const std::vector<uint8_t> &data, int chip);
    void stopPreview();
    /* Un efecto AYFX suelto (los bytes de un .afx) en un canal 0-5. */
    void playSfx(const std::vector<uint8_t> &afx, int channel);
    void stopSfx();

    /* Canales que no suenan: bit n, el canal n (0-2 el primer AY). */
    void setMute(int mask);
    void setStereo(int mode);                   /* 0 mono, 1 ABC, 2 ACB */
    void setVolume(int v);                      /* 0-255 */

    /* Muestras estéreo (izquierda, derecha) a RATE. */
    void render(int16_t *out, int frames);
    Status status();

private:
    struct Voice {
        mus_song_t song;
        std::vector<uint8_t> data;
        bool on = false;
    };

    void step();
    void startAt(int pos, int row);
    void advance(bool audible);

    std::mutex mutex_;
    Voice music_, note_;
    std::vector<std::vector<int>> rows_;
    bool loop_ = true;
    int noteChip_ = 0;
    int pos_[2] = { 0, 0 }, row_[2] = { -1, -1 };
    long steps_ = 0;
    std::vector<uint8_t> sfxData_;
    afx_t sfx_ {};
    int sfxChannel_ = 0;
    mus_ay_t out_[2] {};
    int mute_ = 0, stereo_ = 1, volume_ = 200;
    aysynth_t ay_[2];
    aymix_t mix_;
    uint8_t sent_[2][14] = {};
    bool fresh_ = true;                         /* hay que escribir todos los registros */
    int left_ = 0;                              /* muestras que faltan del paso */
};

} // namespace trk

#endif // ZXV_TRACKER_ENGINE_H
