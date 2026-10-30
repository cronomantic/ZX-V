/*
 * sfx.h - Los efectos de sonido AYFX (los del AY Sound FX Editor de Shiru,
 * que toca sdk/music con zxv_sfx_play), sin Qt.
 *
 * Un efecto es una lista de frames, uno por paso de 1/50 s: el volumen, si
 * suenan el tono y el ruido, y sus periodos. Un banco (.afb) tiene hasta
 * 256 efectos, con su nombre; un .afx, uno solo.
 */
#ifndef ZXV_TRACKER_SFX_H
#define ZXV_TRACKER_SFX_H

#include <cstdint>
#include <string>
#include <vector>

namespace fx {

struct Frame {
    int volume = 15;            /* 0-15 */
    bool toneOn = true, noiseOn = false;
    int tone = 0x100;           /* 0-0xFFF */
    int noise = 0;              /* 0-31 */
    bool operator==(const Frame &o) const
    {
        return volume == o.volume && toneOn == o.toneOn && noiseOn == o.noiseOn && tone == o.tone &&
               noise == o.noise;
    }
};

struct Effect {
    std::string name;
    std::vector<Frame> frames;
    bool operator==(const Effect &o) const { return name == o.name && frames == o.frames; }
};

struct Bank {
    std::vector<Effect> effects;
    bool operator==(const Bank &o) const { return effects == o.effects; }
};

constexpr int MAX_EFFECTS = 256;

/* Los bytes de un efecto (un .afx), con su final. */
std::vector<uint8_t> compile(const Effect &e);
/* Un .afx; false si no lo es. */
bool decompile(const std::vector<uint8_t> &data, Effect &e);

std::vector<uint8_t> saveBank(const Bank &b);
bool loadBank(const std::vector<uint8_t> &data, Bank &b);

} // namespace fx

#endif // ZXV_TRACKER_SFX_H
