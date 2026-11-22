/*
 * ayresult.cpp - Lo que suena con un instrumento del AY, y las plantillas
 * (ver ayresult.h).
 */
#include "ayresult.h"

#include <QFontDatabase>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "editor.h"

extern "C" {
#include "music_int.h"
#include "zxv_music.h"
}

using namespace trk;

namespace {

const QColor Bg(0x1d, 0x1f, 0x26), Off(0x2c, 0x30, 0x3a), Label(0x9a, 0xa0, 0xb0), Line(0x3c, 0x40, 0x4c);
const QColor VolC(0x5f, 0xa8, 0xe8), ToneC(0x9e, 0xd8, 0x7a), NoiseC(0xe6, 0xcf, 0x6a), EnvC(0xc0, 0x8c, 0xe8);

SampleLine makeLine(bool tone, bool noise, bool env, int vol, int toneShift = 0, bool toneAcc = false)
{
    SampleLine l;
    l.setToneOn(tone);
    l.setNoiseOn(noise);
    l.setEnvOn(env);
    l.setVolume(vol);
    l.tone = int16_t(toneShift);
    l.setToneAcc(toneAcc);
    return l;
}

Sample make(std::vector<SampleLine> lines, int loop)
{
    Sample s;
    s.lines = std::move(lines);
    s.loop = loop;
    return s;
}

} // namespace

std::vector<std::pair<QString, Sample>> AyResult::templates()
{
    std::vector<std::pair<QString, Sample>> t;
    t.emplace_back(tr("Nota plana"), make({ makeLine(true, false, false, 15) }, 0));
    {
        std::vector<SampleLine> v;
        for (int vol : { 15, 13, 11, 10, 9, 8 })
            v.push_back(makeLine(true, false, false, vol));
        t.emplace_back(tr("Ataque y caída"), make(v, int(v.size()) - 1));
    }
    {
        std::vector<SampleLine> v;
        for (int vol = 15; vol >= 0; vol--)
            v.push_back(makeLine(true, false, false, vol));
        t.emplace_back(tr("Pulsación (cae hasta callar)"), make(v, int(v.size()) - 1));
    }
    {
        std::vector<SampleLine> v;
        for (int vol : { 15, 14, 12, 10, 8, 6, 3, 0 })
            v.push_back(makeLine(true, false, false, vol, 0x20, true));   /* el periodo sube: el tono baja */
        t.emplace_back(tr("Bombo"), make(v, int(v.size()) - 1));
    }
    {
        std::vector<SampleLine> v = { makeLine(true, true, false, 15), makeLine(true, true, false, 13) };
        for (int vol : { 12, 10, 8, 6, 4, 2, 0 })
            v.push_back(makeLine(false, true, false, vol));
        t.emplace_back(tr("Caja"), make(v, int(v.size()) - 1));
    }
    {
        std::vector<SampleLine> v;
        for (int vol : { 12, 8, 4, 0 })
            v.push_back(makeLine(false, true, false, vol));
        t.emplace_back(tr("Charles"), make(v, int(v.size()) - 1));
    }
    {
        std::vector<SampleLine> v;
        for (int d : { 1, 1, -1, -1, -1, -1, 1, 1 })
            v.push_back(makeLine(true, false, false, 15, d, true));
        t.emplace_back(tr("Vibrato"), make(v, 0));
    }
    t.emplace_back(tr("Con envolvente (la forma, en el patrón)"), make({ makeLine(true, false, true, 15) }, 0));
    return t;
}

/* --- Lo que suena -------------------------------------------------------------- */

AyResult::AyResult(Editor *ed, QWidget *parent) : QWidget(parent), ed_(ed)
{
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(std::max(9, f.pointSize() - 1));
    setFont(f);
    setToolTip(tr("Lo que suena al tocar un C-4 sostenido con este instrumento, paso a paso (1/50 s), como lo hace "
                  "el reproductor: el volumen (en morado, con la envolvente), el tono en semitonos desde el de la "
                  "nota y, abajo, cuándo suena el ruido."));
    connect(ed_, &Editor::songChanged, this, [this] {
        compute();
        update();
    });
    connect(ed_, &Editor::sampleChanged, this, [this] {
        compute();
        update();
    });
    compute();
}

QSize AyResult::sizeHint() const
{
    return QSize(600, 130);
}

QSize AyResult::minimumSizeHint() const
{
    return QSize(200, 90);
}

/* Una nota sostenida con el instrumento, en el reproductor de verdad, y la
 * misma con uno plano: el periodo de la nota, para medir el tono. */
void AyResult::compute()
{
    steps_.clear();
    const AyBank &b = ed_->bank();
    Module m;
    m.samples = b.samples;
    m.ornaments = b.ornaments;
    m.version = b.version;
    m.table = b.table;
    Cell c;
    c.note = 36;                        /* C-4 */
    c.sample = ed_->sample();
    c.vol = 15;
    const Sample &s = b.samples[size_t(ed_->sample())];
    if (s.lines.empty())
        return;
    int n = std::clamp(int(s.lines.size()) * 2 + 16, 32, 160);
    std::vector<uint8_t> data = previewModule(m, c, -1, 0);
    Module flat = m;
    flat.samples[1] = defaultSample();
    flat.samples[1].lines[0].setToneOn(true);
    Cell fc = c;
    fc.sample = 1;
    std::vector<uint8_t> ref = previewModule(flat, fc, -1, 0);
    static mus_song_t song, base;
    std::memset(&song, 0, sizeof song);
    std::memset(&base, 0, sizeof base);
    if (mus_song_init(&song, data.data(), uint32_t(data.size())) == ZXV_MUSIC_NONE ||
        mus_song_init(&base, ref.data(), uint32_t(ref.size())) == ZXV_MUSIC_NONE)
        return;
    mus_ay_t ay[2], br[2];
    std::memset(br, 0, sizeof br);
    mus_song_step(&base, br);
    int note = br[0].r[0] | (br[0].r[1] & 15) << 8;
    for (int i = 0; i < n; i++) {
        std::memset(ay, 0, sizeof ay);
        mus_song_step(&song, ay);
        const uint8_t *r = ay[0].r;
        int period = r[0] | (r[1] & 15) << 8;
        Step st;
        st.vol = r[8] & 16 ? 16 : r[8] & 15;
        st.tone = !(r[7] & 1);
        st.noise = !(r[7] & 8);
        st.semis = period > 0 && note > 0 ? 12.0 * std::log2(double(note) / period) : 0.0;
        steps_.push_back(st);
    }
}

void AyResult::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Bg);
    QFontMetrics fm(font());
    int lw = fm.horizontalAdvance(QStringLiteral("Volumen  ")) + 12;
    int top = fm.height() + 6, h = height() - top - 14;
    p.setPen(Label);
    p.drawText(6, fm.ascent() + 2, tr("Lo que suena (un C-4 sostenido)"));
    if (steps_.empty() || h < 20)
        return;
    int n = int(steps_.size());
    double w = double(width() - lw - 8) / n;
    int volH = h * 2 / 3, mid = top + volH / 2;
    p.setPen(Label);
    p.drawText(QRect(6, top, lw - 8, volH), Qt::AlignLeft | Qt::AlignVCenter, tr("Volumen\ny tono"));
    p.drawText(QRect(6, top + volH, lw - 8, h - volH + 10), Qt::AlignLeft | Qt::AlignVCenter, tr("Ruido"));
    p.fillRect(lw, top, int(w * n), volH, Off);
    /* El volumen, en barras (la envolvente, entera y en morado). */
    for (int i = 0; i < n; i++) {
        int x = lw + int(i * w), bw = std::max(1, int((i + 1) * w) - int(i * w) - 1);
        int v = steps_[size_t(i)].vol;
        int bh = (v > 15 ? 15 : v) * volH / 15;
        p.fillRect(x, top + volH - bh, bw, bh, v > 15 ? EnvC : VolC.darker(120));
        if (steps_[size_t(i)].noise)
            p.fillRect(x, top + volH + 4, bw, std::max(3, h - volH - 6), NoiseC);
    }
    /* El tono: una raya, en semitonos (de -12 a +12), solo donde suena. */
    p.setPen(QPen(ToneC, 2));
    double scaleY = volH / 2.0 / 12.0;
    for (int i = 0; i + 1 < n; i++) {
        const Step &a = steps_[size_t(i)], &b = steps_[size_t(i + 1)];
        if (!a.tone)
            continue;
        double ya = mid - std::clamp(a.semis, -12.0, 12.0) * scaleY;
        double yb = mid - std::clamp(b.tone ? b.semis : a.semis, -12.0, 12.0) * scaleY;
        p.drawLine(QPointF(lw + (i + 0.5) * w, ya), QPointF(lw + (i + 1.5) * w, yb));
    }
    p.setPen(QPen(Line, 1, Qt::DashLine));
    p.drawLine(lw, mid, lw + int(w * n), mid);
}
