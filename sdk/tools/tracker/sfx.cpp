/*
 * sfx.cpp - Los efectos AYFX: frames y bytes (como los lee sdk/music/afx.c).
 *
 * Cada frame, un byte de opciones (bits 0-3 el volumen, bit 4 sin tono,
 * bit 5 sigue el tono, bit 6 sigue el ruido, bit 7 sin ruido) y los valores
 * que cambian; D0 20 (un ruido de 0x20) acaba el efecto. En un banco, tras
 * ese final va el nombre del efecto, con un 0 detrás, como lo guarda el AY
 * Sound FX Editor.
 */
#include "sfx.h"

namespace fx {

namespace {

/* Los frames desde "at" hasta el final; da dónde acaba (tras D0 20), o 0. */
size_t readFrames(const std::vector<uint8_t> &d, size_t at, size_t end, Effect &e)
{
    int tone = 0, noise = 0;
    e.frames.clear();
    while (at < end) {
        uint8_t b = d[at++];
        Frame f;
        f.volume = b & 15;
        f.toneOn = !(b & 0x10);
        f.noiseOn = !(b & 0x80);
        if (b & 0x20) {
            if (at + 2 > end)
                return 0;
            tone = (d[at] | d[at + 1] << 8) & 0xFFF;
            at += 2;
        }
        if (b & 0x40) {
            if (at >= end)
                return 0;
            if (d[at] >= 0x20)          /* el final */
                return at + 1;
            noise = d[at++];
        }
        f.tone = tone;
        f.noise = noise;
        e.frames.push_back(f);
    }
    return 0;
}

} // namespace

std::vector<uint8_t> compile(const Effect &e)
{
    std::vector<uint8_t> v;
    int tone = -1, noise = -1;
    for (const Frame &f : e.frames) {
        int t = f.tone & 0xFFF, n = f.noise & 31;
        /* El primer frame pone el tono y el ruido (como el AY Sound FX
         * Editor: el ruido es de los tres canales); los demás, cuando
         * cambian. */
        bool st = tone < 0 || t != tone;
        bool sn = noise < 0 || n != noise;
        v.push_back(uint8_t((f.volume & 15) | (f.toneOn ? 0 : 0x10) | (st ? 0x20 : 0) | (sn ? 0x40 : 0) |
                            (f.noiseOn ? 0 : 0x80)));
        if (st) {
            v.push_back(uint8_t(t));
            v.push_back(uint8_t(t >> 8));
            tone = t;
        }
        if (sn) {
            v.push_back(uint8_t(n));
            noise = n;
        }
    }
    v.push_back(0xD0);
    v.push_back(0x20);
    return v;
}

bool decompile(const std::vector<uint8_t> &data, Effect &e)
{
    Effect out;
    if (!readFrames(data, 0, data.size(), out))
        return false;
    out.name = e.name;
    e = out;
    return true;
}

std::vector<uint8_t> saveBank(const Bank &b)
{
    int n = int(b.effects.size());
    if (n > MAX_EFFECTS)
        n = MAX_EFFECTS;
    std::vector<uint8_t> v(1 + 2 * size_t(n));
    v[0] = uint8_t(n);                  /* 256 es 0 */
    for (int i = 0; i < n; i++) {
        int off = int(v.size()) - (2 + 2 * i);
        v[1 + 2 * i] = uint8_t(off);
        v[2 + 2 * i] = uint8_t(off >> 8);
        std::vector<uint8_t> d = compile(b.effects[i]);
        v.insert(v.end(), d.begin(), d.end());
        for (char c : b.effects[i].name)
            v.push_back(uint8_t((unsigned char)c >= 32 && (unsigned char)c < 127 ? c : '?'));
        v.push_back(0);
    }
    return v;
}

bool loadBank(const std::vector<uint8_t> &d, Bank &b)
{
    if (d.empty())
        return false;
    int n = d[0] ? d[0] : 256;
    if (d.size() < 1 + 2 * size_t(n))
        return false;
    std::vector<size_t> start(n);
    for (int i = 0; i < n; i++) {
        start[i] = 2 + 2 * size_t(i) + (d[1 + 2 * i] | d[2 + 2 * i] << 8);
        if (start[i] >= d.size())
            return false;
    }
    Bank out;
    for (int i = 0; i < n; i++) {
        /* Acaba donde empieza el siguiente que esté detrás (o el fichero). */
        size_t end = d.size();
        for (int j = 0; j < n; j++)
            if (start[j] > start[i] && start[j] < end)
                end = start[j];
        Effect e;
        size_t at = readFrames(d, start[i], end, e);
        if (!at)
            return false;
        /* El nombre: texto con un 0 detrás, antes del siguiente efecto. */
        std::string name;
        size_t k = at;
        while (k < end && d[k] >= 32 && d[k] < 127 && name.size() < 64)
            name += char(d[k++]);
        if (k < end && d[k] == 0)
            e.name = name;
        out.effects.push_back(e);
    }
    b = std::move(out);
    return true;
}

} // namespace fx
