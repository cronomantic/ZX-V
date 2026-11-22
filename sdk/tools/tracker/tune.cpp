/*
 * tune.cpp - La canción de zxv-tracker y sus ficheros (ver tune.h).
 *
 * Los AY pasan por el PT3 en los dos sentidos, con el código que ya está
 * probado: al guardar el .zxm, cada AY se compila a un PT3 (song.cpp) y
 * zxms_from_pt3 lo convierte (sdk/tools/music/zxm_pt3.c, comprobado paso a
 * paso con pt3.c); al leer un .zxm, sus casillas del AY se pasan a las de
 * Vortex Tracker con lo inverso de lo que hace zxm_pt3.c.
 */
#include "tune.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <tuple>

extern "C" {
#include "smpsynth.h"
#include "zxm_song.h"
}

namespace trk {

AyBank::AyBank()
{
    samples[1] = defaultSample();
    ornaments[0] = defaultOrnament();
}

bool Tune::operator==(const Tune &o) const
{
    return title == o.title && author == o.author && speed == o.speed && tempo == o.tempo && loop == o.loop &&
           chips == o.chips && smpTracks == o.smpTracks && flags == o.flags && pan == o.pan && ay == o.ay &&
           order == o.order && ayPatterns == o.ayPatterns && smpPatterns == o.smpPatterns &&
           instruments == o.instruments && waves == o.waves;
}

namespace {

/* Lo que dice un conversor de C, a w (una vez cada cosa). */
void say(void *ctx, const char *msg)
{
    Warnings &w = *static_cast<Warnings *>(ctx);
    if (std::find(w.begin(), w.end(), msg) == w.end())
        w.emplace_back(msg);
}

void warn(Warnings &w, const std::string &msg) { say(&w, msg.c_str()); }

/* Un zxms_t que se libera solo. */
struct Zxms {
    zxms_t s;
    Zxms() { zxms_init(&s); }
    ~Zxms() { zxms_free(&s); }
    Zxms(const Zxms &) = delete;
    Zxms &operator=(const Zxms &) = delete;
};

/* --- Del PT3 (song.h) a la canción y al revés ---------------------------------- */

/* El patrón p de un módulo con justo "rows" filas. */
Pattern sized(const std::vector<Pattern> &pool, int p, int rows)
{
    Pattern out = p >= 0 && p < int(pool.size()) ? pool[size_t(p)] : Pattern();
    out.rows.resize(size_t(rows));
    return out;
}

/* Los módulos del PT3 de la canción, sin mirar si se puede (las muestras y
 * el tempo los pone saveZxm aparte). */
bool buildPt3(const Tune &t, Song &s, Warnings &w)
{
    s.modules.assign(size_t(std::max(t.chips, 1)), Module());
    for (int c = 0; c < int(s.modules.size()); c++) {
        Module &m = s.modules[size_t(c)];
        const AyBank &b = t.ay[size_t(c)];
        m.title = c ? b.title : t.title;
        m.author = c ? b.author : t.author;
        m.version = b.version;
        m.table = b.table;
        m.delay = std::clamp(t.speed, 1, 255);
        m.loop = t.loop;
        m.samples = b.samples;
        m.ornaments = b.ornaments;
        /* Los patrones como están, con sus números; donde una posición pide
         * otro largo, uno nuevo (en un hueco libre). */
        const std::vector<Pattern> &pool = t.ayPatterns[size_t(c)];
        m.patterns = pool;
        m.patterns.resize(MAX_PATTERNS);
        std::vector<bool> taken(MAX_PATTERNS, false);
        for (const Position &p : t.order)
            if (p.ay[size_t(c)] >= 0 && p.ay[size_t(c)] < MAX_PATTERNS &&
                int(m.patterns[size_t(p.ay[size_t(c)])].rows.size()) == p.rows)
                taken[size_t(p.ay[size_t(c)])] = true;
        std::map<std::pair<int, int>, int> made;
        m.order.clear();
        for (const Position &p : t.order) {
            int k = p.ay[size_t(c)];
            if (k >= 0 && k < MAX_PATTERNS && int(m.patterns[size_t(k)].rows.size()) == p.rows) {
                m.order.push_back(k);
                continue;
            }
            auto it = made.find({ k, p.rows });
            if (it == made.end()) {
                int free = 0;
                while (free < MAX_PATTERNS && (taken[size_t(free)] || !m.patterns[size_t(free)].empty() ||
                                               int(m.patterns[size_t(free)].rows.size()) != 64))
                    free++;
                if (free >= MAX_PATTERNS) {
                    warn(w, "el PT3 no admite más de 85 patrones");
                    return false;
                }
                m.patterns[size_t(free)] = sized(pool, k, p.rows);
                taken[size_t(free)] = true;
                it = made.emplace(std::make_pair(k, p.rows), free).first;
            }
            m.order.push_back(it->second);
        }
        if (m.order.size() > MAX_POSITIONS) {
            warn(w, "el PT3 no admite más de 255 posiciones");
            return false;
        }
    }
    return true;
}

/* --- Del .zxm a la canción ----------------------------------------------------- */

int le16(const uint8_t *p) { return p[0] | p[1] << 8; }

/* Un bloque de instrumento del AY del .zxm: un sample del PT3. */
Sample blobSample(const zxms_blob_t &b)
{
    Sample s;
    if (!b.data || b.size < 2)
        return s;
    s.loop = b.data[0];
    for (int j = 0; j < b.data[1] && 2 + j * 4 + 3 < b.size; j++) {
        SampleLine l;
        l.b0 = b.data[2 + j * 4];
        l.b1 = b.data[3 + j * 4];
        l.tone = int16_t(le16(b.data + 4 + j * 4));
        s.lines.push_back(l);
    }
    return s;
}

Ornament blobOrnament(const zxms_blob_t &b)
{
    Ornament o;
    if (!b.data || b.size < 2)
        return o;
    o.loop = b.data[0];
    for (int j = 0; j < b.data[1] && 2 + j < b.size; j++)
        o.offsets.push_back(int8_t(b.data[2 + j]));
    return o;
}

/* Una casilla de una pista del AY a la de Vortex Tracker (lo que hace
 * zxm_pt3.c, al revés), con los números de los samples y los ornamentos
 * de su AY. */
void ayCell(const zxms_cell_t &z, Cell &c, Row &row, const std::map<int, int> &ins, const std::map<int, int> &orn,
            Warnings &w)
{
    if (z.has & ZXM_C_NOTE)
        c.note = z.note >= 1 && z.note <= 96 ? z.note - 1 : NOTE_OFF;
    if (z.has & ZXM_C_INS) {
        auto it = ins.find(z.ins);
        c.sample = it != ins.end() ? it->second : 0;
    }
    if (z.has & ZXM_C_VOL)
        c.vol = std::clamp(int(z.vol), 0, 15);
    if (z.has & ZXM_C_ORN) {
        auto it = orn.find(z.orn);
        c.orn = it != orn.end() ? it->second : 0;
    }
    if (z.has & ZXM_C_ENV) {
        int shape = z.env & 15;
        if (!shape || (z.env & ZXM_ENV_SET_ONLY) || shape > 14) {
            c.env = ENV_OFF;
            if (shape && (z.env & ZXM_ENV_SET_ONLY))
                warn(w, "una envolvente puesta sin usarla en su canal (Vortex Tracker no lo tiene): se quita");
        } else {
            c.env = shape;
            row.envPeriod = z.env_period;
        }
    }
    /* Los efectos: el ruido es de la fila; de los demás, Vortex Tracker
     * tiene uno (el primero, que el .zxm pone el último). */
    int kept = 0;
    for (int k = 1; k >= 0; k--) {
        if (!(z.has & (ZXM_C_FX1 << k)))
            continue;
        const uint8_t *p = z.fxp[k];
        int16_t step = int16_t(p[1] | p[2] << 8);
        int fx = 0, delay = 0, param = 0;
        switch (z.fx[k]) {
        case 'n':
            row.noise = p[0] & 31;
            continue;
        case 'g':
            fx = step >= 0 ? 1 : 2;
            delay = p[0];
            param = std::abs(int(step));
            break;
        case 'p':
            fx = 3;
            delay = p[0];
            param = std::abs(int(step));
            break;
        case '9':
            fx = 4;
            param = p[0];
            break;
        case 'o':
            fx = 5;
            param = p[0];
            break;
        case 'i':
            fx = 6;
            param = std::min<int>(p[0], 15) << 4 | std::min<int>(p[1], 15);
            break;
        case 'e':
            fx = step >= 0 ? 9 : 10;
            delay = p[0];
            param = std::abs(int(step));
            break;
        case 't':
            fx = 11;
            param = p[0];
            break;
        default:
            warn(w, std::string("un efecto del AY que Vortex Tracker no tiene: ") + char(z.fx[k]));
            continue;
        }
        if (kept++) {
            warn(w, "una casilla del AY con dos efectos: se queda uno");
            continue;
        }
        c.fx = fx;
        c.fxDelay = delay;
        c.fxParam = param;
    }
}

/* Los números de los instrumentos (o los ornamentos) que usa un AY en el
 * .zxm y los suyos en la canción: los mismos si caben (hasta "max"); si no,
 * de "first" en adelante, por orden. */
std::map<int, int> numbers(const std::vector<int> &used, int first, int max, const char *what, Warnings &w)
{
    std::map<int, int> m;
    bool fits = std::all_of(used.begin(), used.end(), [&](int i) { return i <= max; });
    int next = first;
    for (int i : used) {
        if (m.count(i))
            continue;
        if (fits) {
            m[i] = i;
        } else if (next <= max) {
            m[i] = next++;
        } else {
            m[i] = first;
            warn(w, std::string("un AY con más ") + what + " de los que caben: los de más, el primero");
        }
    }
    return m;
}

void fromZxms(const zxms_t &z, Tune &t, Warnings &w)
{
    t = Tune();
    t.title = z.title;
    t.author = z.author;
    t.speed = z.speed ? z.speed : 256;
    t.tempo = z.tempo;
    t.loop = z.loop;
    t.chips = z.ay_tracks / 3;
    t.smpTracks = z.smp_tracks;
    t.flags = z.flags & ~0x70;
    std::copy(z.pan, z.pan + 12, t.pan.begin());
    t.order.assign(size_t(z.npos), Position());
    for (int p = 0; p < z.npos; p++)
        t.order[size_t(p)].rows = z.pos[p].rows;

    /* Los AY: por grupos de tres pistas; cada combinación de patrones (y de
     * filas), un patrón de la canción. */
    for (int c = 0; c < t.chips; c++) {
        AyBank &b = t.ay[size_t(c)];
        int ay2 = z.ay2 >= 0 ? z.ay2 : (z.ay_table & 3) | (z.flags & 0x70);
        int vt = c ? ay2 : (z.ay_table & 3) | (z.flags & 0x70);
        b.table = vt & 3;
        b.version = ZXM_F_PT3VER(vt);
        b.title = t.title;
        b.author = t.author;
        std::vector<int> ins = { 1 }, orn = { 0 };      /* los de partida */
        for (int p = 0; p < z.npos; p++)
            for (int k = 0; k < 3; k++) {
                int pat = z.pos[p].pat[c * 3 + k];
                if (pat < 0)
                    continue;
                const zxms_pattern_t &zp = z.pat[pat];
                for (int r = 0; r < zp.rows; r++) {
                    if (zp.cells[r].has & ZXM_C_INS)
                        ins.push_back(zp.cells[r].ins);
                    if (zp.cells[r].has & ZXM_C_ORN)
                        orn.push_back(zp.cells[r].orn);
                }
            }
        std::map<int, int> insMap = numbers(ins, 1, SAMPLES - 1, "samples", w);
        std::map<int, int> ornMap = numbers(orn, 0, ORNAMENTS - 1, "ornamentos", w);
        for (auto [from, to] : insMap)
            if (from < z.n_ayins)
                b.samples[size_t(to)] = blobSample(z.ayins[from]);
        for (auto [from, to] : ornMap)
            if (from < z.n_orns)
                b.ornaments[size_t(to)] = blobOrnament(z.orns[from]);
        std::vector<Pattern> &pool = t.ayPatterns[size_t(c)];
        pool.clear();
        std::map<std::tuple<int, int, int, int>, int> made;
        for (int p = 0; p < z.npos; p++) {
            const zxms_position_t &zp = z.pos[p];
            auto key = std::make_tuple(zp.pat[c * 3], zp.pat[c * 3 + 1], zp.pat[c * 3 + 2], zp.rows);
            auto it = made.find(key);
            if (it == made.end()) {
                Pattern pat;
                pat.rows.assign(size_t(zp.rows), Row());
                for (int k = 0; k < 3; k++) {
                    int zi = zp.pat[c * 3 + k];
                    if (zi < 0)
                        continue;
                    for (int r = 0; r < zp.rows && r < z.pat[zi].rows; r++)
                        ayCell(z.pat[zi].cells[r], pat.rows[size_t(r)].ch[k], pat.rows[size_t(r)], insMap, ornMap,
                               w);
                }
                it = made.emplace(key, int(pool.size())).first;
                pool.push_back(std::move(pat));
            }
            t.order[size_t(p)].ay[size_t(c)] = it->second;
        }
        if (pool.empty())
            pool.resize(1);
    }

    /* Las pistas de muestras: los patrones del .zxm, tal cual. */
    t.smpPatterns.clear();
    std::map<int, int> made;
    for (int p = 0; p < z.npos; p++)
        for (int k = 0; k < t.smpTracks; k++) {
            int zi = z.pos[p].pat[z.ay_tracks + k];
            if (zi < 0)
                continue;
            auto it = made.find(zi);
            if (it == made.end()) {
                const zxms_pattern_t &zp = z.pat[zi];
                SmpPattern sp;
                sp.rows.assign(size_t(std::max(zp.rows, 1)), SmpCell());
                for (int r = 0; r < zp.rows; r++) {
                    const zxms_cell_t &zc = zp.cells[r];
                    SmpCell &c = sp.rows[size_t(r)];
                    if (zc.has & ZXM_C_NOTE)
                        c.note = zc.note == ZXM_NOTE_OFF ? NOTE_OFF
                                 : zc.note == ZXM_NOTE_CUT ? NOTE_CUT
                                 : zc.note >= 1 && zc.note <= 96 ? zc.note - 1
                                                                 : NOTE_NONE;
                    if (zc.has & ZXM_C_INS)
                        c.ins = zc.ins;
                    if (zc.has & ZXM_C_VOL)
                        c.vol = zc.vol;
                    for (int f = 0; f < 2; f++)
                        if (zc.has & (ZXM_C_FX1 << f)) {
                            c.fx[size_t(f)] = zc.fx[f];
                            c.fxp[size_t(f)] = zc.fxp[f][0];
                        }
                }
                it = made.emplace(zi, int(t.smpPatterns.size())).first;
                t.smpPatterns.push_back(std::move(sp));
            }
            t.order[size_t(p)].smp[size_t(k)] = it->second;
        }
    if (t.smpPatterns.empty())
        t.smpPatterns.resize(1);
    for (int i = 0; i < z.n_smpins; i++) {
        SmpInstrument ins;
        std::memcpy(ins.data.data(), z.smpins[i], SINS_SIZE);
        t.instruments.push_back(ins);
    }
    for (int i = 0; i < z.n_samples; i++) {
        const zxms_sample_t &s = z.samples[i];
        Wave v;
        v.len = s.len;
        v.loop = s.loop;
        v.format = s.format;
        v.relnote = s.relnote;
        v.fine = s.fine;
        v.vol = s.vol;
        v.pan = s.pan;
        v.hasPan = s.flags & ZXM_SF_PAN;
        v.data.assign(s.data, s.data + s.size);
        t.waves.push_back(std::move(v));
    }
}

} // namespace

/* --- Lo de tune.h --------------------------------------------------------------- */

Tune fromPt3(const Song &s, Warnings &w)
{
    Tune t;
    const Module &m0 = s.modules.at(0);
    t.title = m0.title;
    t.author = m0.author;
    t.speed = m0.delay;
    t.loop = m0.loop;
    t.chips = int(std::min<size_t>(s.modules.size(), 2));
    size_t npos = m0.order.size();
    if (t.chips == 2)
        npos = std::max(npos, s.modules[1].order.size());
    t.order.assign(npos, Position());
    for (int c = 0; c < t.chips; c++) {
        const Module &m = s.modules[size_t(c)];
        AyBank &b = t.ay[size_t(c)];
        b.samples = m.samples;
        b.ornaments = m.ornaments;
        b.version = m.version;
        b.table = m.table;
        b.title = m.title;
        b.author = m.author;
        t.ayPatterns[size_t(c)] = m.patterns;
        for (size_t p = 0; p < npos; p++)
            t.order[p].ay[size_t(c)] = p < m.order.size() ? m.order[p] : -1;
    }
    /* Las filas de cada posición: las del patrón del primer AY. En un
     * TurboSound hecho en Vortex Tracker las dos mitades van igual; si no,
     * el segundo se ajusta al primero (como en el .zxm). */
    auto rowsOf = [&](int c, int pat) {
        return pat >= 0 && pat < int(t.ayPatterns[size_t(c)].size())
                   ? std::clamp(int(t.ayPatterns[size_t(c)][size_t(pat)].rows.size()), 1, MAX_ROWS)
                   : -1;
    };
    for (size_t p = 0; p < npos; p++) {
        Position &pos = t.order[p];
        int r0 = rowsOf(0, pos.ay[0]), r1 = t.chips == 2 ? rowsOf(1, pos.ay[1]) : -1;
        pos.rows = r0 > 0 ? r0 : r1 > 0 ? r1 : 64;
        if (r0 > 0 && r1 > 0 && r0 != r1)
            warn(w, "los dos módulos de este TurboSound no van al mismo paso: el segundo se ajusta al primero");
    }
    return t;
}

bool fitsPt3(const Tune &t)
{
    Warnings w;
    Song s;
    return t.chips > 0 && t.smpTracks == 0 && t.tempo == 125 && buildPt3(t, s, w);
}

bool toPt3(const Tune &t, Song &s, Warnings &w)
{
    if (t.chips == 0) {
        warn(w, "la canción no usa los AY");
        return false;
    }
    if (t.smpTracks > 0) {
        warn(w, "el PT3 no tiene pistas de muestras");
        return false;
    }
    if (t.tempo != 125) {
        warn(w, "el PT3 va siempre a 50 pasos por segundo (el tempo 125)");
        return false;
    }
    return buildPt3(t, s, w);
}

std::vector<uint8_t> saveZxm(const Tune &t, Warnings &w)
{
    Zxms z;
    zxms_t &s = z.s;
    if (t.chips > 0) {
        Song pt3;
        if (!buildPt3(t, pt3, w))
            return {};
        std::vector<uint8_t> b = save(pt3);
        zxms_free(&s);
        if (zxms_from_pt3(&s, b.data(), b.size(), say, &w)) {
            warn(w, "los AY no se pueden pasar al .zxm");
            return {};
        }
        if (s.npos != int(t.order.size())) {
            warn(w, "los AY no dan las mismas posiciones");
            return {};
        }
    } else {
        s.npos = int(std::min<size_t>(t.order.size(), ZXMS_POSITIONS));
        for (int p = 0; p < s.npos; p++)
            for (int k = 0; k < ZXMS_TRACKS; k++)
                s.pos[p].pat[k] = -1;
    }
    std::snprintf(s.title, sizeof s.title, "%s", t.title.c_str());
    std::snprintf(s.author, sizeof s.author, "%s", t.author.c_str());
    s.speed = std::clamp(t.speed, 1, 255);
    s.tempo = std::clamp(t.tempo, 32, 255);
    s.loop = std::clamp(t.loop, 0, s.npos - 1);
    s.flags = (s.flags & 0x70) | (t.flags & ~0x70);
    std::copy(t.pan.begin(), t.pan.end(), s.pan);
    for (int p = 0; p < s.npos; p++)
        s.pos[p].rows = std::clamp(t.order[size_t(p)].rows, 1, MAX_ROWS);

    /* Las pistas de muestras. */
    s.smp_tracks = std::clamp(t.smpTracks, 0, SMP_TRACKS);
    std::map<int, int> made;
    for (int p = 0; p < s.npos; p++)
        for (int k = 0; k < s.smp_tracks; k++) {
            int i = t.order[size_t(p)].smp[size_t(k)];
            if (i < 0 || i >= int(t.smpPatterns.size())) {
                s.pos[p].pat[s.ay_tracks + k] = -1;
                continue;
            }
            auto it = made.find(i);
            if (it == made.end()) {
                const SmpPattern &sp = t.smpPatterns[size_t(i)];
                int zi = zxms_add_pattern(&s, 1, int(sp.rows.size()));
                if (zi < 0)
                    return {};
                for (size_t r = 0; r < sp.rows.size(); r++) {
                    const SmpCell &c = sp.rows[r];
                    zxms_cell_t &zc = s.pat[zi].cells[r];
                    if (c.note != NOTE_NONE) {
                        zc.has |= ZXM_C_NOTE;
                        zc.note = uint8_t(c.note == NOTE_OFF   ? ZXM_NOTE_OFF
                                          : c.note == NOTE_CUT ? ZXM_NOTE_CUT
                                                               : std::clamp(c.note, 0, 95) + 1);
                    }
                    if (c.ins) {
                        zc.has |= ZXM_C_INS;
                        zc.ins = uint8_t(c.ins);
                    }
                    if (c.vol >= 0) {
                        zc.has |= ZXM_C_VOL;
                        zc.vol = uint8_t(std::min(c.vol, 64));
                    }
                    for (int f = 0; f < 2; f++)
                        if (c.fx[size_t(f)]) {
                            zc.has |= uint8_t(ZXM_C_FX1 << f);
                            zc.fx[f] = c.fx[size_t(f)];
                            zc.fxp[f][0] = c.fxp[size_t(f)];
                        }
                }
                it = made.emplace(i, zi).first;
            }
            s.pos[p].pat[s.ay_tracks + k] = it->second;
        }
    for (const SmpInstrument &ins : t.instruments) {
        if (s.n_smpins >= 128) {
            warn(w, "más de 128 instrumentos de muestras: sobran los últimos");
            break;
        }
        auto *p = static_cast<uint8_t(*)[ZXM_SINS_SIZE]>(
            std::realloc(s.smpins, size_t(s.n_smpins + 1) * ZXM_SINS_SIZE));
        if (!p)
            return {};
        s.smpins = p;
        std::memcpy(s.smpins[s.n_smpins++], ins.data.data(), ZXM_SINS_SIZE);
    }
    for (const Wave &v : t.waves) {
        zxms_sample_t m {};
        m.len = v.len;
        m.loop = v.loop;
        m.format = uint8_t(v.format);
        m.relnote = int8_t(v.relnote);
        m.fine = int8_t(v.fine);
        m.vol = uint8_t(v.vol);
        m.pan = uint8_t(v.pan);
        m.flags = v.hasPan ? ZXM_SF_PAN : 0;
        m.data = const_cast<uint8_t *>(v.data.data());
        m.size = uint32_t(v.data.size());
        if (zxms_add_sample(&s, &m) < 0) {
            warn(w, "más de 255 muestras: sobran las últimas");
            break;
        }
    }
    zxms_dedup(&s);
    size_t n = 0;
    uint8_t *b = zxms_save(&s, &n);
    if (!b)
        return {};
    std::vector<uint8_t> out(b, b + n);
    std::free(b);
    return out;
}

bool ModuleChannels::tooMany() const
{
    int n = 0;
    for (int c = 0; c < count && c < 64; c++)
        n += int(used >> c & 1);
    return n > 12;
}

ModuleChannels moduleChannels(const std::vector<uint8_t> &data)
{
    ModuleChannels ch;
    if (zxm_conv_kind(data.data(), uint32_t(std::min<size_t>(data.size(), ZXM_CONV_HEAD)), uint32_t(data.size())))
        ch.count = zxm_module_channels(data.data(), uint32_t(data.size()), &ch.used, &ch.picked, ch.notes);
    return ch;
}

int loadTune(const std::vector<uint8_t> &data, Tune &t, Warnings &w, TuneFormat *format, uint64_t channels)
{
    if (data.size() >= 8 && !std::memcmp(data.data(), ZXM_MAGIC, 8)) {
        Zxms z;
        zxms_free(&z.s);
        if (zxms_load(&z.s, data.data(), data.size()))
            return TUNE_DAMAGED;
        fromZxms(z.s, t, w);
        if (format)
            *format = FORMAT_ZXM;
        return TUNE_OK;
    }
    Song s;
    int e = load(data, s);
    if (e == LOAD_OK) {
        t = fromPt3(s, w);
        if (format)
            *format = FORMAT_PT3;
        return TUNE_OK;
    }
    if (zxm_conv_kind(data.data(), uint32_t(std::min<size_t>(data.size(), ZXM_CONV_HEAD)), uint32_t(data.size()))) {
        size_t n = 0;
        uint8_t *b = zxm_from_module_ch(data.data(), uint32_t(data.size()), UINT32_MAX, channels, &n, say, &w);
        if (!b)
            return TUNE_DAMAGED;
        Zxms z;
        zxms_free(&z.s);
        int r = zxms_load(&z.s, b, n);
        std::free(b);
        if (r)
            return TUNE_DAMAGED;
        fromZxms(z.s, t, w);
        if (format)
            *format = FORMAT_MODULE;
        return TUNE_OK;
    }
    return e == LOAD_DAMAGED ? TUNE_DAMAGED : TUNE_UNKNOWN;
}

/* --- Las muestras --------------------------------------------------------------- */

uint32_t waveBytes(const Wave &w)
{
    return uint32_t((w.data.size() + 3) & ~size_t(3));
}

uint32_t wavesBytes(const Tune &t)
{
    uint32_t n = 0;
    for (const Wave &w : t.waves)
        n += waveBytes(w);
    return n;
}

namespace {

uint32_t le32(const uint8_t *p) { return p[0] | p[1] << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
int le16s(const uint8_t *p) { return int16_t(p[0] | p[1] << 8); }

/* Lo que suena un C-4 de una muestra sin afinar: 8.363 Hz (el tono lineal
 * y el reloj del S3M) o 8.287 Hz (el del Amiga PAL, el MOD). */
double baseRate(int flags)
{
    return (flags & ZXM_F_LINEAR) || (flags & ZXM_F_S3MCLOCK) ? 8363.0 : 14187578.0 / 1712.0;
}

/* La nota relativa y la afinación para que un C-4 suene a "rate" Hz. */
void tuneTo(Wave &w, double rate, int flags)
{
    double t = 12.0 * std::log2(rate / baseRate(flags));
    int rel = int(std::lround(t));
    int fine = int(std::lround((t - rel) * 128.0));
    w.relnote = std::clamp(rel, -96, 95);
    w.fine = std::clamp(fine, -128, 127);
}

} // namespace

double waveRate(const Wave &w, int flags)
{
    return baseRate(flags) * std::pow(2.0, (w.relnote + w.fine / 128.0) / 12.0);
}

bool loadWav(const std::vector<uint8_t> &d, int flags, Wave &w, std::string &why)
{
    size_t n = d.size();
    if (n < 12 || std::memcmp(d.data(), "RIFF", 4) || std::memcmp(d.data() + 8, "WAVE", 4)) {
        why = "no es un WAV";
        return false;
    }
    int fmt = 0, chans = 0, bits = 0;
    uint32_t rate = 0, data = 0, dataLen = 0, ls = 0, le = 0;
    bool loop = false;
    for (size_t pos = 12; pos + 8 <= n;) {
        uint32_t len = le32(&d[pos + 4]);
        const uint8_t *c = &d[pos + 8];
        size_t avail = n - pos - 8;
        if (!std::memcmp(&d[pos], "fmt ", 4) && len >= 16 && avail >= 16) {
            fmt = d[pos + 8] | d[pos + 9] << 8;
            if (fmt == 0xFFFE && len >= 26 && avail >= 26)     /* WAVE_FORMAT_EXTENSIBLE: su subformato */
                fmt = c[24] | c[25] << 8;
            chans = c[2] | c[3] << 8;
            rate = le32(c + 4);
            bits = c[14] | c[15] << 8;
        } else if (!std::memcmp(&d[pos], "data", 4)) {
            data = uint32_t(pos + 8);
            dataLen = uint32_t(std::min<size_t>(len, avail));
        } else if (!std::memcmp(&d[pos], "smpl", 4) && len >= 60 && avail >= 60 && le32(c + 28)) {
            ls = le32(c + 36 + 8);              /* el primer bucle: desde y hasta (incluida) */
            le = le32(c + 36 + 12);
            loop = true;
        }
        if (len > n)
            break;
        pos += 8 + len + (len & 1);
    }
    if (fmt != 1 || chans < 1 || chans > 2 || (bits != 8 && bits != 16) || !rate || !data) {
        why = "solo WAV sin comprimir, de 8 o 16 bits, mono o estéreo";
        return false;
    }
    uint32_t fsz = uint32_t(chans * bits / 8), frames = dataLen / fsz;
    if (!frames) {
        why = "el WAV no tiene sonido";
        return false;
    }
    /* Cabe en la memoria de sonido, como mucho. */
    frames = std::min(frames, SOUND_MEM);
    std::vector<int16_t> s(frames);
    for (uint32_t i = 0; i < frames; i++) {
        const uint8_t *q = &d[data + i * fsz];
        int v = bits == 8 ? (chans == 2 ? ((q[0] + q[1]) - 256) * 128 : (q[0] - 128) * 256)
                          : (chans == 2 ? (le16s(q) + le16s(q + 2)) / 2 : le16s(q));
        s[i] = int16_t(std::clamp(v, -32768, 32767));
    }
    w = Wave();
    w.len = frames;
    w.loop = frames;
    if (loop && ls < frames && le >= ls) {
        if (le + 1 < frames) {
            s.resize(le + 1);
            w.len = le + 1;
        }
        w.loop = ls;
    }
    tuneTo(w, rate, flags);
    w.data.resize(w.len);
    for (uint32_t i = 0; i < w.len; i++)
        w.data[i] = uint8_t(int8_t(std::clamp((s[i] + 128) >> 8, -128, 127)));
    return true;
}

std::vector<int16_t> waveSamples(const Wave &w)
{
    std::vector<int16_t> s(w.len);
    if (w.format == 0) {
        for (uint32_t i = 0; i < w.len && i < w.data.size(); i++)
            s[i] = int16_t(int8_t(w.data[i]) * 256);
        return s;
    }
    /* El ADPCM de la ULA: bloques de 64 muestras, la primera en la cabecera. */
    static const int8_t idx[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
    static const uint16_t steps[89] = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
        107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724,
        796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
        4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500,
        20350, 22385, 24623, 27086, 29794, 32767,
    };
    for (uint32_t b = 0; b * 64 < w.len; b++) {
        size_t at = size_t(b) * 36;
        if (at + 36 > w.data.size())
            break;
        const uint8_t *p = &w.data[at];
        int pred = int16_t(p[0] | p[1] << 8), index = std::min<int>(p[2], 88);
        for (uint32_t j = 0; j < 64 && b * 64 + j < w.len; j++) {
            if (j) {
                unsigned nib = p[4 + (j - 1) / 2] >> ((j - 1) & 1) * 4 & 15;
                int step = steps[index], diff = step >> 3;
                if (nib & 4)
                    diff += step;
                if (nib & 2)
                    diff += step >> 1;
                if (nib & 1)
                    diff += step >> 2;
                pred = std::clamp(nib & 8 ? pred - diff : pred + diff, -32768, 32767);
                index = std::clamp(index + idx[nib], 0, 88);
            }
            s[b * 64 + j] = int16_t(pred);
        }
    }
    return s;
}

void encodeWave(Wave &w, int format)
{
    if (format == w.format)
        return;
    std::vector<int16_t> s = waveSamples(w);
    w.format = format;
    if (format == 0) {
        w.data.resize(w.len);
        for (uint32_t i = 0; i < w.len; i++)
            w.data[i] = uint8_t(int8_t(std::clamp((s[i] + 128) >> 8, -128, 127)));
    } else {
        w.data.assign((w.len + 63) / 64 * 36, 0);
        if (w.len)
            smp_adpcm_encode(s.data(), w.len, w.data.data());
    }
}

void halveWave(Wave &w)
{
    if (w.len < 2)
        return;
    int format = w.format;
    std::vector<int16_t> s = waveSamples(w), h((w.len + 1) / 2);
    for (size_t i = 0; i < h.size(); i++)
        h[i] = int16_t((s[i * 2] + (i * 2 + 1 < s.size() ? s[i * 2 + 1] : s[i * 2])) / 2);
    bool looped = w.loop < w.len;
    w.len = uint32_t(h.size());
    w.loop = looped ? w.loop / 2 : w.len;
    w.relnote = std::max(-96, w.relnote - 12);
    w.format = 0;
    w.data.resize(w.len);
    for (uint32_t i = 0; i < w.len; i++)
        w.data[i] = uint8_t(int8_t(std::clamp((h[i] + 128) >> 8, -128, 127)));
    if (format)
        encodeWave(w, format);
}

int addInstrument(Tune &t, int wave)
{
    if (t.instruments.size() >= 128)
        return 0;
    SmpInstrument ins;
    std::memset(ins.data.data() + ZXM_SI_MAP, wave, 96);
    t.instruments.push_back(ins);
    return int(t.instruments.size());
}

} // namespace trk
