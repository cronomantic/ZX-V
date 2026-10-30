/*
 * song.cpp - El módulo de zxv-tracker: leer un .pt3 a patrones y compilarlo
 * de vuelta, como lo toca sdk/music/pt3.c.
 */
#include "song.h"

#include <algorithm>
#include <cstring>
#include <map>

extern "C" {
#include "music_int.h"
#include "zxv_music.h"
}

namespace trk {

bool Cell::operator==(const Cell &o) const
{
    return note == o.note && sample == o.sample && env == o.env && orn == o.orn && vol == o.vol &&
           fx == o.fx && fxDelay == o.fxDelay && fxParam == o.fxParam;
}

bool Row::operator==(const Row &o) const
{
    return ch[0] == o.ch[0] && ch[1] == o.ch[1] && ch[2] == o.ch[2] && envPeriod == o.envPeriod &&
           noise == o.noise;
}

bool Pattern::empty() const
{
    for (const Row &r : rows)
        if (!r.empty())
            return false;
    return true;
}

Sample defaultSample()
{
    Sample s;
    SampleLine l;
    l.b0 = 0x01;            /* sin envolvente */
    l.b1 = 0x8F;            /* sin ruido, con tono, volumen 15 */
    s.lines.push_back(l);
    return s;
}

Ornament defaultOrnament()
{
    Ornament o;
    o.offsets.push_back(0);
    return o;
}

Module::Module()
{
    samples[1] = defaultSample();
    ornaments[0] = defaultOrnament();
}

bool Module::operator==(const Module &o) const
{
    return title == o.title && author == o.author && version == o.version && table == o.table &&
           delay == o.delay && loop == o.loop && order == o.order && patterns == o.patterns &&
           samples == o.samples && ornaments == o.ornaments;
}

std::string noteName(int note)
{
    if (note == NOTE_OFF)
        return "R--";
    if (note < 0 || note >= NOTES)
        return "---";
    static const char *names[12] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };
    return std::string(names[note % 12]) + char('1' + note / 12);
}

/* --- Leer ------------------------------------------------------------------- */

namespace {

struct Reader {
    const uint8_t *m;
    size_t n;
    bool bad = false;
    int b(size_t i)
    {
        if (i >= n) {
            bad = true;
            return 0;
        }
        return m[i];
    }
    int w(size_t i) { return b(i) | b(i + 1) << 8; }
};

/* Las órdenes de una fila de un canal (como pattern() de pt3.c), hasta su
 * nota. Deja en *skip las filas que dura si las cambia. */
size_t readEntry(Reader &r, size_t a, Cell &c, Row &row, int &skip)
{
    int fx[8] = { 0 }, nfx = 0;     /* las órdenes 1-9, en orden */
    for (int guard = 0;; a++) {
        if (++guard > 4096 || r.bad) {
            r.bad = true;
            return a;
        }
        int v = r.b(a);
        if (v >= 0xF0) {
            c.orn = v - 0xF0;
            c.sample = (r.b(++a) / 2) & 31;
            c.env = ENV_OFF;
        } else if (v >= 0xD1) {
            c.sample = v - 0xD0;
        } else if (v == 0xD0) {
            a++;
            break;
        } else if (v >= 0xC1) {
            c.vol = v - 0xC0;
        } else if (v == 0xC0) {
            c.note = NOTE_OFF;
            a++;
            break;
        } else if (v >= 0xB2) {
            c.env = v - 0xB1;
            row.envPeriod = r.b(a + 1) << 8 | r.b(a + 2);
            a += 2;
        } else if (v == 0xB1) {
            skip = r.b(++a);
            if (skip == 0)
                skip = 256;
        } else if (v == 0xB0) {
            c.env = ENV_OFF;
        } else if (v >= 0x50) {
            c.note = v - 0x50;
            a++;
            break;
        } else if (v >= 0x40) {
            c.orn = v - 0x40;
        } else if (v >= 0x20) {
            row.noise = v - 0x20;
        } else if (v >= 0x10) {
            if (v == 0x10) {
                c.env = ENV_OFF;
            } else {
                c.env = v - 0x10;
                row.envPeriod = r.b(a + 1) << 8 | r.b(a + 2);
                a += 2;
            }
            c.sample = (r.b(++a) / 2) & 31;
        } else if (v >= 1 && v <= 9 && v != 6 && v != 7) {
            if (nfx < 8)
                fx[nfx++] = v;
        }
    }
    /* Los parámetros van tras la nota, del último efecto al primero. Si hay
     * más de uno, se queda el primero (Vortex Tracker solo tiene uno). */
    for (int i = nfx - 1; i >= 0; i--) {
        int f = fx[i], cmd = 0, delay = 0, param = 0;
        switch (f) {
        case 1: {
            delay = r.b(a);
            int16_t step = int16_t(r.w(a + 1));
            a += 3;
            cmd = step >= 0 ? 1 : 2;
            param = step >= 0 ? step : -step;
            break;
        }
        case 2: {
            delay = r.b(a);
            int16_t step = int16_t(r.w(a + 3));
            a += 5;
            cmd = 3;
            param = step >= 0 ? step : -step;
            break;
        }
        case 3:
            cmd = 4;
            param = r.b(a++);
            break;
        case 4:
            cmd = 5;
            param = r.b(a++);
            break;
        case 5:
            cmd = 6;
            param = std::min(r.b(a), 15) << 4 | std::min(r.b(a + 1), 15);
            a += 2;
            break;
        case 8: {
            delay = r.b(a);
            int16_t step = int16_t(r.w(a + 1));
            a += 3;
            cmd = step >= 0 ? 9 : 10;
            param = step >= 0 ? step : -step;
            break;
        }
        case 9:
            cmd = 11;
            param = r.b(a++);
            break;
        }
        if (i == 0) {
            c.fx = cmd;
            c.fxDelay = delay;
            c.fxParam = param;
        }
    }
    return a;
}

/* Un canal de un patrón: "len" filas (o, en el A, hasta su 0). */
int readChannel(Reader &r, size_t a, int ch, Pattern &p, int len, int &skip)
{
    int row = 0;
    while (row < (ch == 0 ? MAX_ROWS : len) && !r.bad) {
        if (ch == 0 && r.b(a) == 0)
            break;
        if ((int)p.rows.size() <= row)
            p.rows.resize(row + 1);
        Cell c;
        Row &rw = p.rows[row];
        a = readEntry(r, a, c, rw, skip);
        rw.ch[ch] = c;
        row += skip;
    }
    return std::min(row, MAX_ROWS);
}

int loadModule(const uint8_t *m, size_t n, Module &mod)
{
    static const char pt[] = "ProTracker 3.", vt[] = "Vortex Tracker II";
    if (n < 0xCA || (memcmp(m, pt, 13) != 0 && memcmp(m, vt, 17) != 0))
        return LOAD_NOT_PT3;
    Reader r{ m, n };
    Module out;
    auto text = [&](size_t at) {
        std::string s(reinterpret_cast<const char *>(m + at), 32);
        for (char &c : s)
            if ((unsigned char)c < 32 || (unsigned char)c >= 127)
                c = ' ';
        s.erase(s.find_last_not_of(' ') + 1);
        return s;
    };
    out.title = text(0x1E);
    out.author = text(0x42);
    out.version = m[13] >= '0' && m[13] <= '9' ? m[13] - '0' : 6;
    out.table = m[0x63] & 3;
    out.delay = m[0x64];
    int npos = m[0x65];
    out.loop = m[0x66];
    out.order.clear();
    for (int i = 0; i < npos; i++) {
        int v = r.b(0xC9 + i);
        if (v == 0xFF || v % 3 || v / 3 >= MAX_PATTERNS)
            return LOAD_DAMAGED;
        out.order.push_back(v / 3);
    }
    if (out.order.empty() || out.loop >= npos)
        return LOAD_DAMAGED;

    int pats = r.w(0x67);
    std::vector<bool> done(MAX_PATTERNS);
    int skip[3] = { 1, 1, 1 };          /* sigue de un patrón al siguiente */
    for (int p : out.order) {
        if (done[p])
            continue;
        done[p] = true;
        Pattern &pat = out.patterns[p];
        pat.rows.clear();
        int len = readChannel(r, r.w(pats + p * 6), 0, pat, 0, skip[0]);
        if (len <= 0)
            return LOAD_DAMAGED;
        pat.rows.resize(len);
        readChannel(r, r.w(pats + p * 6 + 2), 1, pat, len, skip[1]);
        readChannel(r, r.w(pats + p * 6 + 4), 2, pat, len, skip[2]);
        pat.rows.resize(len);
        /* El periodo, solo donde hay una envolvente (la que se quita después
         * en la misma fila no deja el suyo). */
        for (Row &row : pat.rows) {
            bool env = false;
            for (const Cell &c : row.ch)
                env |= c.env >= 1 && c.env <= 14;
            if (!env)
                row.envPeriod = -1;
        }
    }
    for (int i = 0; i < SAMPLES; i++) {
        size_t a = r.w(0x69 + i * 2);
        Sample &s = out.samples[i];
        s = Sample();
        if (a == 0)
            continue;
        s.loop = r.b(a);
        int len = r.b(a + 1);
        for (int j = 0; j < len; j++) {
            SampleLine l;
            l.b0 = uint8_t(r.b(a + 2 + j * 4));
            l.b1 = uint8_t(r.b(a + 3 + j * 4));
            l.tone = int16_t(r.w(a + 4 + j * 4));
            s.lines.push_back(l);
        }
    }
    for (int i = 0; i < ORNAMENTS; i++) {
        size_t a = r.w(0xA9 + i * 2);
        Ornament &o = out.ornaments[i];
        o = Ornament();
        if (a == 0)
            continue;
        o.loop = r.b(a);
        int len = r.b(a + 1);
        for (int j = 0; j < len; j++)
            o.offsets.push_back(int8_t(r.b(a + 2 + j)));
    }
    if (r.bad)
        return LOAD_DAMAGED;
    /* Los que no hay se guardan como uno que no suena (save()): se leen
     * otra vez como que no hay. Suenan igual. */
    for (Sample &s : out.samples)
        if (s.loop == 0 && s.lines.size() == 1 && s.lines[0].b0 == 0x01 && s.lines[0].b1 == 0x90 && !s.lines[0].tone)
            s.lines.clear();
    for (int i = 1; i < ORNAMENTS; i++)
        if (out.ornaments[i] == defaultOrnament())
            out.ornaments[i] = Ornament();
    mod = std::move(out);
    return LOAD_OK;
}

} // namespace

int load(const std::vector<uint8_t> &data, Song &song)
{
    uint32_t n1, n2;
    Song out;
    if (pt3_ts_split(data.data(), uint32_t(data.size()), &n1, &n2)) {
        out.modules.resize(2);
        int e = loadModule(data.data(), n1, out.modules[0]);
        if (e == LOAD_OK)
            e = loadModule(data.data() + n1, n2, out.modules[1]);
        if (e != LOAD_OK)
            return e == LOAD_NOT_PT3 ? LOAD_DAMAGED : e;
    } else {
        int e = loadModule(data.data(), data.size(), out.modules[0]);
        if (e != LOAD_OK)
            return e;
    }
    song = std::move(out);
    return LOAD_OK;
}

/* --- Compilar --------------------------------------------------------------- */

namespace {

void put16(std::vector<uint8_t> &v, int x)
{
    v.push_back(uint8_t(x));
    v.push_back(uint8_t(x >> 8));
}

void set16(std::vector<uint8_t> &v, size_t at, int x)
{
    v[at] = uint8_t(x);
    v[at + 1] = uint8_t(x >> 8);
}

/* Una fila de un canal: las órdenes, la nota y los parámetros del efecto. */
void writeCell(std::vector<uint8_t> &v, const Cell &c, const Row &row, bool noise)
{
    static const uint8_t code[16] = { 0, 1, 1, 2, 3, 4, 5, 0, 0, 8, 8, 9, 0, 0, 0, 0 };
    int fx = c.fx >= 0 && c.fx < 16 ? code[c.fx] : 0;
    if (fx)
        v.push_back(uint8_t(fx));
    if (noise)
        v.push_back(uint8_t(0x20 + (row.noise & 31)));
    int period = row.envPeriod < 0 ? 0 : row.envPeriod;
    if (c.env >= 1 && c.env <= 14) {
        if (c.sample) {
            v.push_back(uint8_t(0x10 + c.env));
            v.push_back(uint8_t(period >> 8));
            v.push_back(uint8_t(period));
            v.push_back(uint8_t(c.sample * 2));
        } else {
            v.push_back(uint8_t(0xB1 + c.env));
            v.push_back(uint8_t(period >> 8));
            v.push_back(uint8_t(period));
        }
        if (c.orn >= 0)
            v.push_back(uint8_t(0x40 + c.orn));
    } else if (c.env == ENV_OFF) {
        if (c.sample && c.orn >= 0) {
            v.push_back(uint8_t(0xF0 + c.orn));
            v.push_back(uint8_t(c.sample * 2));
        } else if (c.sample) {
            v.push_back(0x10);
            v.push_back(uint8_t(c.sample * 2));
        } else {
            v.push_back(0xB0);
            if (c.orn >= 0)
                v.push_back(uint8_t(0x40 + c.orn));
        }
    } else {
        if (c.orn >= 0)
            v.push_back(uint8_t(0x40 + c.orn));
        if (c.sample)
            v.push_back(uint8_t(0xD0 + c.sample));
    }
    if (c.vol)
        v.push_back(uint8_t(0xC0 + c.vol));
    if (c.note == NOTE_OFF)
        v.push_back(0xC0);
    else if (c.note >= 0 && c.note < NOTES)
        v.push_back(uint8_t(0x50 + c.note));
    else
        v.push_back(0xD0);
    int p = c.fxParam;
    switch (fx) {
    case 1:
        v.push_back(uint8_t(c.fxDelay));
        put16(v, c.fx == 2 ? -p : p);
        break;
    case 2:
        v.push_back(uint8_t(c.fxDelay));
        put16(v, 0);                    /* la distancia: la calcula el reproductor */
        put16(v, p);
        break;
    case 3:
    case 4:
    case 9:
        v.push_back(uint8_t(p));
        break;
    case 5:
        v.push_back(uint8_t(p >> 4 & 15));
        v.push_back(uint8_t(p & 15));
        break;
    case 8:
        v.push_back(uint8_t(c.fxDelay));
        put16(v, c.fx == 10 ? -p : p);
        break;
    }
}

/* Un canal de un patrón: una entrada por fila con algo (y en la 0, y cada
 * 255 filas), con las filas que dura cada una. */
void writeChannel(std::vector<uint8_t> &v, const Pattern &p, int ch)
{
    int len = int(p.rows.size());
    std::vector<int> at;
    for (int r = 0; r < len; r++) {
        const Row &row = p.rows[r];
        if (r == 0 || !row.ch[ch].empty() || (ch == 1 && row.noise >= 0) || r - at.back() >= 255)
            at.push_back(r);
    }
    int skip = -1;
    for (size_t i = 0; i < at.size(); i++) {
        int s = (i + 1 < at.size() ? at[i + 1] : len) - at[i];
        if (s != skip) {
            v.push_back(0xB1);
            v.push_back(uint8_t(s));
            skip = s;
        }
        const Row &row = p.rows[at[i]];
        writeCell(v, row.ch[ch], row, ch == 1 && row.noise >= 0);
    }
    v.push_back(0);
}

} // namespace

std::vector<uint8_t> saveModule(const Module &m)
{
    std::vector<uint8_t> v;
    auto text = [&](const char *s, size_t len) {
        for (size_t i = 0; i < len; i++)
            v.push_back(s[i] ? uint8_t(s[i]) : uint8_t(' '));
    };
    auto field = [&](const std::string &s) {
        std::string t = s.substr(0, 32);
        t.resize(32, ' ');
        for (char &c : t)
            if ((unsigned char)c < 32 || (unsigned char)c >= 127)
                c = '?';
        text(t.c_str(), 32);
    };
    text("ProTracker 3.", 13);
    v.push_back(uint8_t('0' + std::clamp(m.version, 0, 9)));
    text(" compilation of ", 16);
    field(m.title);
    text(" by ", 4);
    field(m.author);
    v.push_back(' ');
    int npos = std::clamp(int(m.order.size()), 1, MAX_POSITIONS);
    v.push_back(uint8_t(m.table & 3));
    v.push_back(uint8_t(std::clamp(m.delay, 1, 255)));
    v.push_back(uint8_t(npos));
    v.push_back(uint8_t(std::clamp(m.loop, 0, npos - 1)));
    v.resize(0xC9);                     /* los punteros, después */
    int npat = 0;
    for (int i = 0; i < npos; i++) {
        int p = i < int(m.order.size()) ? std::clamp(m.order[i], 0, MAX_PATTERNS - 1) : 0;
        v.push_back(uint8_t(p * 3));
        npat = std::max(npat, p + 1);
    }
    v.push_back(0xFF);
    for (int p = 0; p < MAX_PATTERNS && p < int(m.patterns.size()); p++)
        if (!m.patterns[p].empty())
            npat = std::max(npat, p + 1);

    /* Los samples y los ornamentos, antes que los patrones (como Vortex
     * Tracker): los que no hay apuntan a uno que no suena. */
    size_t silent = v.size();
    v.insert(v.end(), { 0, 1, 0x01, 0x90, 0, 0 });
    size_t still = v.size();
    v.insert(v.end(), { 0, 1, 0 });
    /* Lo que es igual (dos samples, dos canales de este patrón o de otro) se
     * escribe una vez. */
    std::map<std::vector<uint8_t>, int> written;
    auto share = [&](const std::vector<uint8_t> &data) {
        auto it = written.find(data);
        if (it == written.end()) {
            it = written.emplace(data, int(v.size())).first;
            v.insert(v.end(), data.begin(), data.end());
        }
        return it->second;
    };
    for (int i = 0; i < SAMPLES; i++) {
        const Sample &s = m.samples[i];
        if (s.lines.empty()) {
            set16(v, 0x69 + i * 2, int(silent));
            continue;
        }
        int len = std::min(int(s.lines.size()), 255);
        std::vector<uint8_t> d = { uint8_t(std::clamp(s.loop, 0, len - 1)), uint8_t(len) };
        for (int j = 0; j < len; j++) {
            d.push_back(s.lines[j].b0);
            d.push_back(s.lines[j].b1);
            put16(d, s.lines[j].tone);
        }
        set16(v, 0x69 + i * 2, share(d));
    }
    for (int i = 0; i < ORNAMENTS; i++) {
        const Ornament &o = m.ornaments[i];
        if (o.offsets.empty()) {
            set16(v, 0xA9 + i * 2, int(still));
            continue;
        }
        int len = std::min(int(o.offsets.size()), 255);
        std::vector<uint8_t> d = { uint8_t(std::clamp(o.loop, 0, len - 1)), uint8_t(len) };
        for (int j = 0; j < len; j++)
            d.push_back(uint8_t(o.offsets[j]));
        set16(v, 0xA9 + i * 2, share(d));
    }

    size_t table = v.size();
    set16(v, 0x67, int(table));
    v.resize(table + npat * 6);
    static const Pattern blank;
    for (int p = 0; p < npat; p++) {
        const Pattern &pat = p < int(m.patterns.size()) && !m.patterns[p].rows.empty() ? m.patterns[p] : blank;
        for (int ch = 0; ch < 3; ch++) {
            std::vector<uint8_t> data;
            writeChannel(data, pat, ch);
            set16(v, table + p * 6 + ch * 2, share(data));
        }
    }
    return v;
}

std::vector<uint8_t> save(const Song &song)
{
    std::vector<uint8_t> a = saveModule(song.modules.at(0));
    if (song.modules.size() < 2)
        return a;
    std::vector<uint8_t> b = saveModule(song.modules[1]);
    size_t n1 = a.size(), n2 = b.size();
    a.insert(a.end(), b.begin(), b.end());
    /* El final de un TurboSound: "PT3!", el tamaño de cada uno y "02TS". */
    const char tail[] = "PT3!";
    a.insert(a.end(), tail, tail + 4);
    put16(a, int(n1));
    a.insert(a.end(), tail, tail + 4);
    put16(a, int(n2));
    const char ts[] = "02TS";
    a.insert(a.end(), ts, ts + 4);
    return a;
}

std::vector<uint8_t> previewModule(const Module &m, const Cell &c, int envPeriod, int channel)
{
    Module p;
    p.version = m.version;
    p.table = m.table;
    p.samples = m.samples;
    p.ornaments = m.ornaments;
    p.delay = 255;                      /* 256 filas de 255 pasos: no vuelve a empezar */
    p.order = { 0 };
    p.loop = 0;
    p.patterns.assign(1, Pattern());
    p.patterns[0].rows.assign(MAX_ROWS, Row());
    Row &r = p.patterns[0].rows[0];
    r.ch[std::clamp(channel, 0, 2)] = c;
    if (c.env >= 1 && c.env <= 14)
        r.envPeriod = envPeriod < 0 ? 0 : envPeriod;
    return saveModule(p);
}

/* --- Sonar ------------------------------------------------------------------ */

long compareSound(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b, long steps)
{
    static mus_song_t sa, sb;
    if (mus_song_init(&sa, a.data(), uint32_t(a.size())) == ZXV_MUSIC_NONE ||
        mus_song_init(&sb, b.data(), uint32_t(b.size())) == ZXV_MUSIC_NONE)
        return 0;
    if (sa.chips != sb.chips)
        return 0;
    for (long i = 0; i < steps; i++) {
        mus_ay_t x[2], y[2];
        mus_song_step(&sa, x);
        mus_song_step(&sb, y);
        for (int c = 0; c < (sa.chips == 3 ? 2 : 1); c++) {
            if (memcmp(x[c].r, y[c].r, 14) != 0 || x[c].r13 != y[c].r13)
                return i;
        }
    }
    return -1;
}

long songSteps(const std::vector<uint8_t> &data, long limit)
{
    static mus_song_t s;
    if (mus_song_init(&s, data.data(), uint32_t(data.size())) == ZXV_MUSIC_NONE)
        return 0;
    for (long i = 0; i < limit; i++) {
        mus_ay_t ay[2];
        mus_song_step(&s, ay);
        if (mus_song_looped(&s))
            return i;
    }
    return limit;
}

} // namespace trk
