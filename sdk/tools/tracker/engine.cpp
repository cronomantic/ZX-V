/*
 * engine.cpp - El sonido de zxv-tracker: sdk/music paso a paso, los dos AY
 * y los canales de muestras.
 */
#include "engine.h"

#include <algorithm>
#include <climits>
#include <cstring>

extern "C" {
#include "zxm_format.h"
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

static void put32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++)
        p[i] = uint8_t(v >> 8 * i);
}

Engine::Engine() : mem_(SMP_SOUND_MEM)
{
    sndsynth_memory(&snd_, mem_.data());
    sndsynth_reset(&snd_);
    silent(music_ay_[0]);
    silent(music_ay_[1]);
    silent(note_ay_);
    silent(out_[0]);
    silent(out_[1]);
}

/* Las muestras de un .zxm a la memoria de sonido, desde el principio, y sus
 * entradas en la tabla desde la 0 (como music.c, con zxv_music_samples(0, 0)). */
void Engine::loadSamples(Voice &v)
{
    silenceSamples();
    const zxm_t &z = v.zxm;
    uint32_t size = std::min<uint32_t>(z.o[ZXM_O_DATA_SIZE], SMP_MEM);
    if (size && z.o[ZXM_O_DATA] + size <= z.n)
        std::memcpy(mem_.data(), z.m + z.o[ZXM_O_DATA], size);
    for (int i = 0; i < z.n_samples && i < SMP_SLOTS; i++) {
        const uint8_t *s = z.m + z.o[ZXM_O_SAMPLES] + uint32_t(i) * ZXM_SMP_SIZE;
        uint8_t *e = mem_.data() + SMP_MEM + i * SMP_SLOT_SIZE;
        put32(e, mus_w(z.m, z.n, uint32_t(s - z.m)) | uint32_t(mus_w(z.m, z.n, uint32_t(s - z.m) + 2)) << 16 |
                     uint32_t(s[ZXM_S_FORMAT]) << 24);
        put32(e + 4, mus_w(z.m, z.n, uint32_t(s - z.m) + 4) | uint32_t(mus_w(z.m, z.n, uint32_t(s - z.m) + 6)) << 16);
        put32(e + 8, mus_w(z.m, z.n, uint32_t(s - z.m) + 8) | uint32_t(mus_w(z.m, z.n, uint32_t(s - z.m) + 10)) << 16);
    }
    v.samples = true;
}

bool Engine::play(Play p)
{
    std::lock_guard<std::mutex> lock(mutex_);
    music_.on = false;
    note_.on = false;
    music_.data = std::move(p.data);
    if (mus_song_init(&music_.song, music_.data.data(), uint32_t(music_.data.size())) == ZXV_MUSIC_NONE)
        return false;
    if (isZxm(music_))
        loadSamples(music_);
    else
        silenceSamples();
    rows_ = std::move(p.rows);
    loop_ = p.loop;
    steps_ = 0;
    music_.on = true;
    startAt(p.pos, p.row);
    musicLeft_ = 0;
    musicFrac_ = 0;
    return true;
}

void Engine::stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    music_.on = false;
    silent(music_ay_[0]);
    silent(music_ay_[1]);
    silenceSamples();
    flush();
}

bool Engine::preview(const std::vector<uint8_t> &data, int chip)
{
    std::lock_guard<std::mutex> lock(mutex_);
    note_.on = false;
    note_.data = data;
    if (mus_song_init(&note_.song, note_.data.data(), uint32_t(note_.data.size())) == ZXV_MUSIC_NONE)
        return false;
    if (isZxm(note_))
        loadSamples(note_);
    noteChip_ = chip & 1;
    note_.on = true;
    return true;
}

void Engine::stopPreview()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool samples = note_.on && isZxm(note_);
    note_.on = false;
    silent(note_ay_);
    if (samples && !music_.on)
        silenceSamples();
    flush();
}

void Engine::playSfx(const std::vector<uint8_t> &afx, int channel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sfxData_ = afx;
    sfxChannel_ = channel < 0 || channel > 5 ? 0 : channel;
    afx_start(&sfx_, sfxData_.data(), sfxData_.data() + sfxData_.size());
    sfxHow_ = 0;
}

void Engine::stopSfx()
{
    std::lock_guard<std::mutex> lock(mutex_);
    sfx_.p = nullptr;
    sfxHow_ = 0;
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
    fresh_ = true;
}

void Engine::setVolume(int v)
{
    std::lock_guard<std::mutex> lock(mutex_);
    volume_ = v < 0 ? 0 : v > 255 ? 255 : v;
    fresh_ = true;
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
    std::memcpy(s.smpVol, smpVol_, sizeof smpVol_);
    s.steps = steps_;
    return s;
}

/* Empieza en la posición "pos" y la fila "row". Un .zxm salta allí; un PT3
 * toca sin sonar las filas de antes de la posición (así el tempo y lo demás
 * son los que serían). */
void Engine::startAt(int pos, int row)
{
    for (int k = 0; k < 2; k++) {
        pos_[k] = 0;
        row_[k] = -1;
    }
    if (isZxm(music_)) {
        if (pos > 0 || row > 0) {
            music_.zxm.jump_pos = int16_t(pos < music_.zxm.npos ? pos : 0);
            music_.zxm.jump_row = int16_t(row);
        }
        pos_[0] = pos;
        return;
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

/* Un paso de la música (en music_ay_, si suena), siguiendo por dónde va. */
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
    silent(m[0]);
    silent(m[1]);
    mus_song_step(&music_.song, m);
    if (!loop_ && mus_song_looped(&music_.song)) {
        music_.on = false;
        silent(music_ay_[0]);
        silent(music_ay_[1]);
        silenceSamples();
        return;
    }
    if (pt3) {
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
    } else if (isZxm(music_) && music_.zxm.tick == 0) {
        pos_[0] = music_.zxm.pos;
        row_[0] = music_.zxm.row - 1;
    }
    steps_++;
    if (audible) {
        music_ay_[0] = m[0];
        music_ay_[1] = music_.song.chips == 3 ? m[1] : music_ay_[1];
        if (music_.song.chips != 3)
            silent(music_ay_[1]);
    }
}

void Engine::write(int reg, uint8_t val)
{
    events_.push_back(SND_EVENT(0, reg, val));
}

/* Lo que pide la canción a sus canales de muestras en este paso (como
 * music.c): todo lo de cada canal que usa, y a la vez las notas nuevas. */
void Engine::emitSamples(const Voice &v, bool audible)
{
    const mus_smp_t *o = audible ? mus_song_samples(&v.song) : nullptr;
    uint32_t key = 0;
    for (int ch = 0; ch < 12; ch++) {
        int base = SND_REG_SMP + ch * SND_SMP_CH_REGS;
        if (!o || !o[ch].on) {
            if (live_ >> ch & 1)
                write(base + SMP_R_CTRL, SMP_STOP);
            live_ &= uint16_t(~(1u << ch));
            smpVol_[ch] = 0;
            continue;
        }
        const mus_smp_t &c = o[ch];
        if (c.key) {
            write(base + SMP_R_SLOT, c.slot);
            write(base + SMP_R_OFFSET, c.offset);
            key |= 1u << ch;
        }
        uint8_t vol = mute_ >> (6 + ch) & 1 ? 0 : c.vol;
        write(base + SMP_R_VOL, vol);
        write(base + SMP_R_PAN, c.pan);
        for (int i = 0; i < 3; i++)
            write(base + SMP_R_STEP + i, uint8_t(c.step >> 8 * i));
        write(base + SMP_R_CTRL, uint8_t(c.ctrl | (c.stop ? SMP_STOP : 0)));
        live_ |= uint16_t(1u << ch);
        smpVol_[ch] = c.stop ? 0 : vol;
    }
    if (key) {
        write(SND_REG_SMPKEY_HI, uint8_t(key >> 8));
        write(SND_REG_SMPKEY, uint8_t(key));
    }
}

void Engine::silenceSamples()
{
    for (int ch = 0; ch < 12; ch++)
        if (live_ >> ch & 1)
            write(SND_REG_SMP + ch * SND_SMP_CH_REGS + SMP_R_CTRL, SMP_STOP);
    live_ = 0;
    std::memset(smpVol_, 0, sizeof smpVol_);
}

/* Los AY de este paso (la canción, la nota, el efecto encima y los canales
 * callados) y todo lo que haya que mandar, a la ULA. */
void Engine::flush()
{
    silent(out_[0]);
    silent(out_[1]);
    if (music_.on) {
        out_[0] = music_ay_[0];
        out_[1] = music_ay_[1];
    }
    if (note_.on) {
        if (isZxm(note_)) {
            out_[0] = note_ay_;
        } else {
            out_[noteChip_] = note_ay_;
            out_[noteChip_ ^ 1].r13 = 0;
        }
    }
    if (sfxHow_)
        overlay(out_[sfxChannel_ / 3], sfxChannel_ % 3, sfx_, sfxHow_);
    for (int ch = 0; ch < 6; ch++)
        if (mute_ >> ch & 1)
            out_[ch / 3].r[8 + ch % 3] = 0;
    if (fresh_) {
        write(SND_REG_STEREO, uint8_t(stereo_));
        write(SND_REG_VOLUME, uint8_t(volume_));
        write(SND_REG_AFLAGS, 0);
        write(SND_REG_SMPVOL, 255);             /* como la máquina al empezar */
    }
    for (int k = 0; k < 2; k++) {
        for (int r = 0; r < 13; r++)
            if (fresh_ || sent_[k][r] != out_[k].r[r]) {
                write(k * 16 + r, out_[k].r[r]);
                sent_[k][r] = out_[k].r[r];
            }
        if (out_[k].r13)
            write(k * 16 + 13, out_[k].r[13]);
    }
    /* R13, solo en el paso que lo escribe. */
    music_ay_[0].r13 = music_ay_[1].r13 = note_ay_.r13 = 0;
    fresh_ = false;
    /* sndsynth_begin se queda con el tramo: sus escrituras se aplican al
     * sonar la primera muestra. Las que aún no (otro flush antes de sonar),
     * delante de las nuevas. */
    std::vector<uint32_t> next;
    if (snd_.ev < snd_.f.events)
        next.assign(inflight_.begin() + snd_.ev, inflight_.begin() + snd_.f.events);
    next.insert(next.end(), events_.begin(), events_.end());
    inflight_.swap(next);
    events_.clear();
    snd_span_t f {};
    f.ay = inflight_.data();
    f.events = unsigned(inflight_.size());
    sndsynth_begin(&snd_, &f, 0);
}

void Engine::musicStep()
{
    advance(true);
    if (isZxm(music_))
        emitSamples(music_, music_.on);
    flush();
    uint64_t us = uint64_t(music_.on ? mus_song_step_us(&music_.song) : 20000u) * RATE + musicFrac_;
    musicLeft_ = int(us / 1000000u);
    musicFrac_ = uint32_t(us % 1000000u);
    if (musicLeft_ < 1)
        musicLeft_ = 1;
}

void Engine::fxStep()
{
    if (note_.on) {
        mus_ay_t m[2];
        silent(m[0]);
        silent(m[1]);
        mus_song_step(&note_.song, m);
        note_ay_ = m[0];
        if (isZxm(note_) && !music_.on)
            emitSamples(note_, true);
    }
    sfxHow_ = sfx_.p ? afx_step(&sfx_) : 0;
    flush();
    fxLeft_ = STEP;
}

void Engine::render(int16_t *out, int frames)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (int i = 0; i < frames;) {
        /* Los dos relojes: el de la canción y el de los efectos (como
         * music.c). Si coinciden, primero la canción. */
        if (music_.on && musicLeft_ <= 0)
            musicStep();
        if (fxLeft_ <= 0)
            fxStep();
        int run = std::min(frames - i, fxLeft_);
        if (music_.on)
            run = std::min(run, musicLeft_);
        uint32_t buf[256];
        run = std::min(run, 256);
        sndsynth_render(&snd_, unsigned(run), buf);
        for (int k = 0; k < run; k++) {
            out[(i + k) * 2] = int16_t(buf[k] & 0xFFFF);
            out[(i + k) * 2 + 1] = int16_t(buf[k] >> 16);
        }
        if (music_.on)
            musicLeft_ -= run;
        fxLeft_ -= run;
        i += run;
    }
}

} // namespace trk
