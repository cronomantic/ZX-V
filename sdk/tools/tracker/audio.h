/*
 * audio.h - La tarjeta de sonido de zxv-tracker (SDL2): pide las muestras a
 * un Engine.
 */
#ifndef ZXV_TRACKER_AUDIO_H
#define ZXV_TRACKER_AUDIO_H

#include <cstdint>
#include <string>

#include "engine.h"

class Audio
{
public:
    explicit Audio(trk::Engine *engine);
    ~Audio();
    /* Abre la salida de sonido; false (y error()) si no hay. */
    bool open();
    void close();
    bool isOpen() const { return device_ != 0; }
    std::string error() const { return error_; }

private:
    static void callback(void *user, uint8_t *stream, int len);

    trk::Engine *engine_;
    uint32_t device_ = 0;
    std::string error_;
};

#endif // ZXV_TRACKER_AUDIO_H
