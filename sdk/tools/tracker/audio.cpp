/*
 * audio.cpp - La salida de sonido de zxv-tracker con SDL2, a 48 kHz.
 */
#include "audio.h"

#include <SDL.h>

Audio::Audio(trk::Engine *engine) : engine_(engine) {}

Audio::~Audio()
{
    close();
}

bool Audio::open()
{
    if (device_)
        return true;
    if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        error_ = SDL_GetError();
        return false;
    }
    SDL_AudioSpec want{}, have{};
    want.freq = trk::Engine::RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;            /* unos 21 ms */
    want.callback = callback;
    want.userdata = this;
    device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!device_) {
        error_ = SDL_GetError();
        return false;
    }
    SDL_PauseAudioDevice(device_, 0);
    return true;
}

void Audio::close()
{
    if (device_) {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }
}

void Audio::callback(void *user, uint8_t *stream, int len)
{
    Audio *a = static_cast<Audio *>(user);
    a->engine_->render(reinterpret_cast<int16_t *>(stream), len / 4);
}
