/*
 * engine.cpp - El sonido de zxv-tracker: sdk/music paso a paso y los dos AY.
 */
#include "engine.h"

#include <cstring>

extern "C" {
#include "zxv_music.h"
}

namespace trk {

static void silent(mus_ay_t &ay)
{
    std::memset(&ay, 0, sizeof ay);
    ay.r[7] = 0x3F;
}

/* Lo que tapa un efecto en su canal (como music.c). */
static void overlay(mus_ay_t &ay, int ch, const afx_t &a, int how)
{
    ay.r[8 + ch] = a.vol;
    if (how == 2)
        return;
    if (a.has_tone) {
        ay.r[ch * 2] = a.tone & 0xFF;
        ay.r[ch * 2 + 1] = a.tone >> 8;
    }
    if (a.has_noise)
        ay.r[6] = a.noise;
    uint8_t bits = uint8_t(1u << ch | 8u << ch);
    ay.r[7] = uint8_t((ay.r[7] & ~bits) | ((a.mix & 1) << ch) | ((a.mix >> 1 & 1) << (ch + 3)));
}

static bool isPt3(const mus_song_t &s)
{
    return s.format == ZXV_MUSIC_PT3 || s.format == ZXV_MUSIC_TS;
}

Engine::Engine()
{
    ays_reset(&ay_[0]);
    ays_reset(&ay_[1]);
    aymix_reset(&mix_);
    silent(out_[0]);
    silent(out_[1]);
}

bool Engine::play(Play p)
{
    std::lock_guard<std::mutex> lock(mutex_);
    music_.on = false;
    music_.data = std::move(p.data);
    if (mus_song_init(&music_.song, music_.data.data(), uint32_t(music_.data.size())) == ZXV_MUSIC_NONE)
        return false;
    rows_ = std::move(p.rows);
    loop_ = p.loop;
    steps_ = 0;
    music_.on = true;
    startAt(p.pos, p.row);
    return true;
}

void Engine::stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    music_.on = false;
}

bool Engine::preview(const std::vector<uint8_t> &data, int chip)
{
    std::lock_guard<std::mutex> lock(mutex_);
    note_.on = false;
    note_.data = data;
    if (mus_song_init(&note_.song, note_.data.data(), uint32_t(note_.data.size())) == ZXV_MUSIC_NONE)
        return false;
    noteChip_ = chip & 1;
    note_.on = true;
    return true;
}

void Engine::stopPreview()
{
    std::lock_guard<std::mutex> lock(mutex_);
    note_.on = false;
}

void Engine::playSfx(const std::vector<uint8_t> &afx, int channel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sfxData_ = afx;
    sfxChannel_ = channel < 0 || channel > 5 ? 0 : channel;
    afx_start(&sfx_, sfxData_.data(), sfxData_.data() + sfxData_.size());
}

void Engine::stopSfx()
{
    std::lock_guard<std::mutex> lock(mutex_);
    sfx_.p = nullptr;
}

void Engine::setMute(int mask)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mute_ = mask;
}

void Engine::setStereo(int mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    stereo_ = mode;
}

void Engine::setVolume(int v)
{
    std::lock_guard<std::mutex> lock(mutex_);
    volume_ = v < 0 ? 0 : v > 255 ? 255 : v;
}

Engine::Status Engine::status()
{
    std::lock_guard<std::mutex> lock(mutex_);
    Status s;
    s.playing = music_.on;
    s.preview = note_.on;
    s.sfx = sfx_.p != nullptr;
    s.chips = music_.on && music_.song.chips == 3 ? 2 : 1;
    for (int k = 0; k < 2; k++) {
        s.pos[k] = pos_[k];
        s.row[k] = row_[k];
        std::memcpy(s.regs[k], out_[k].r, 14);
    }
    s.steps = steps_;
    return s;
}

/* Empieza en la posición "pos", con las filas de antes ya tocadas (sin
 * sonar: así el tempo y lo demás son los que serían). */
void Engine::startAt(int pos, int row)
{
    for (int k = 0; k < 2; k++) {
        pos_[k] = 0;
        row_[k] = -1;
    }
    if (!isPt3(music_.song))
        return;
    int n = music_.song.chips == 3 ? 2 : 1;
    for (int k = 0; k < n; k++) {
        pt3_t &s = music_.song.u.pt3[k];
        if (pos <= 0 || pos >= s.npos)
            continue;
        s.pos = uint8_t(pos);
        uint16_t pat = uint16_t(mus_w(s.m, s.n, 0x67) + mus_b(s.m, s.n, 0xC9 + pos) * 2);
        for (int i = 0; i < 3; i++)
            s.ch[i].addr = mus_w(s.m, s.n, pat + i * 2u);
        pos_[k] = pos;
    }
    for (long guard = 0; row > 0 && guard < 200000 && music_.on; guard++) {
        if (row_[0] >= row - 1 && music_.song.u.pt3[0].delay_cnt == 1)
            break;
        advance(false);
    }
    steps_ = 0;
}

/* Un paso de la música (en out_, si suena), siguiendo por dónde va. */
void Engine::advance(bool audible)
{
    bool pt3 = isPt3(music_.song);
    int n = music_.song.chips == 3 ? 2 : 1;
    bool tick[2] = { false, false };
    int before[2] = { 0, 0 };
    if (pt3)
        for (int k = 0; k < n; k++) {
            tick[k] = music_.song.u.pt3[k].delay_cnt == 1;
            before[k] = music_.song.u.pt3[k].pos;
        }
    mus_ay_t m[2];
    mus_song_step(&music_.song, m);
    if (!loop_ && mus_song_looped(&music_.song)) {
        music_.on = false;
        return;
    }
    if (pt3)
        for (int k = 0; k < n; k++) {
            if (!tick[k])
                continue;
            int pos = music_.song.u.pt3[k].pos;
            row_[k] = pos != before[k] ? 0 : row_[k] + 1;
            int len = k < int(rows_.size()) && pos < int(rows_[k].size()) ? rows_[k][pos] : 0;
            if (len > 0 && row_[k] >= len)
                row_[k] = 0;
            pos_[k] = pos;
        }
    steps_++;
    if (audible) {
        if (music_.song.chips == 3) {
            out_[0] = m[0];
            out_[1] = m[1];
        } else {
            out_[0] = m[0];
        }
    }
}

void Engine::step()
{
    silent(out_[0]);
    silent(out_[1]);
    if (music_.on)
        advance(true);
    if (note_.on) {
        mus_ay_t m[2];
        mus_song_step(&note_.song, m);
        out_[noteChip_] = m[0];
    }
    if (sfx_.p) {
        int how = afx_step(&sfx_);
        if (how)
            overlay(out_[sfxChannel_ / 3], sfxChannel_ % 3, sfx_, how);
    }
    for (int ch = 0; ch < 6; ch++)
        if (mute_ >> ch & 1)
            out_[ch / 3].r[8 + ch % 3] = 0;
    for (int k = 0; k < 2; k++) {
        for (int r = 0; r < 13; r++)
            if (fresh_ || sent_[k][r] != out_[k].r[r]) {
                ays_write(&ay_[k], unsigned(r), out_[k].r[r]);
                sent_[k][r] = out_[k].r[r];
            }
        if (out_[k].r13)
            ays_write(&ay_[k], 13, out_[k].r[13]);
    }
    fresh_ = false;
}

void Engine::render(int16_t *out, int frames)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (int i = 0; i < frames; i++) {
        if (left_ == 0) {
            step();
            left_ = STEP;
        }
        left_--;
        int32_t a[3], b[3];
        ays_step(&ay_[0], a);
        ays_step(&ay_[1], b);
        aymix_sample(&mix_, a, b, 0, unsigned(stereo_), unsigned(volume_), false, out + i * 2);
    }
}

} // namespace trk
