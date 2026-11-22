/*
 * instrview.cpp - Las envolventes y el mapa de notas de los instrumentos de
 * las pistas de muestras (ver instrview.h).
 */
#include "instrview.h"

#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

#include "editor.h"

extern "C" {
#include "zxm_format.h"
}

using namespace trk;

/* --- Los bytes del instrumento ----------------------------------------------- */

namespace ins {

static int get16(const SmpInstrument &i, int at) { return i.data[size_t(at)] | i.data[size_t(at + 1)] << 8; }

static void set16(SmpInstrument &i, int at, int v)
{
    i.data[size_t(at)] = uint8_t(v);
    i.data[size_t(at + 1)] = uint8_t(v >> 8);
}

Env env(const SmpInstrument &i, int which)
{
    int pts = which ? ZXM_SI_PENV : ZXM_SI_VENV, cnt = which ? ZXM_SI_PENV_N : ZXM_SI_VENV_N;
    uint8_t f = i.data[ZXM_SI_FLAGS];
    Env e;
    e.n = std::min<int>(i.data[size_t(cnt)], POINTS);
    e.sustain = i.data[size_t(cnt + 1)];
    e.loopFrom = i.data[size_t(cnt + 2)];
    e.loopTo = i.data[size_t(cnt + 3)];
    e.on = f & (which ? ZXM_SIF_PENV : ZXM_SIF_VENV);
    e.sus = f & (which ? ZXM_SIF_PSUS : ZXM_SIF_VSUS);
    e.loop = f & (which ? ZXM_SIF_PLOOP : ZXM_SIF_VLOOP);
    for (int k = 0; k < POINTS; k++) {
        e.tick[size_t(k)] = get16(i, pts + 4 * k);
        e.value[size_t(k)] = get16(i, pts + 4 * k + 2);
    }
    return e;
}

void setEnv(SmpInstrument &i, int which, const Env &e)
{
    int pts = which ? ZXM_SI_PENV : ZXM_SI_VENV, cnt = which ? ZXM_SI_PENV_N : ZXM_SI_VENV_N;
    for (int k = 0; k < POINTS; k++) {
        set16(i, pts + 4 * k, k < e.n ? e.tick[size_t(k)] : 0);
        set16(i, pts + 4 * k + 2, k < e.n ? e.value[size_t(k)] : 0);
    }
    i.data[size_t(cnt)] = uint8_t(e.n);
    i.data[size_t(cnt + 1)] = uint8_t(e.sustain);
    i.data[size_t(cnt + 2)] = uint8_t(e.loopFrom);
    i.data[size_t(cnt + 3)] = uint8_t(e.loopTo);
    uint8_t on = which ? ZXM_SIF_PENV : ZXM_SIF_VENV, sus = which ? ZXM_SIF_PSUS : ZXM_SIF_VSUS,
            loop = which ? ZXM_SIF_PLOOP : ZXM_SIF_VLOOP;
    uint8_t &f = i.data[ZXM_SI_FLAGS];
    f = uint8_t((f & ~(on | sus | loop)) | (e.on ? on : 0) | (e.sus ? sus : 0) | (e.loop ? loop : 0));
}

int fade(const SmpInstrument &i) { return get16(i, ZXM_SI_FADE); }
void setFade(SmpInstrument &i, int v) { set16(i, ZXM_SI_FADE, v); }

std::array<int, 4> vibrato(const SmpInstrument &i)
{
    return { i.data[ZXM_SI_VIBRATO], i.data[ZXM_SI_VIBRATO + 1], i.data[ZXM_SI_VIBRATO + 2], i.data[ZXM_SI_VIBRATO + 3] };
}

void setVibrato(SmpInstrument &i, const std::array<int, 4> &v)
{
    for (int k = 0; k < 4; k++)
        i.data[size_t(ZXM_SI_VIBRATO + k)] = uint8_t(v[size_t(k)]);
}

} // namespace ins

namespace {
const QColor Bg(0x1d, 0x1f, 0x26), Grid(0x2c, 0x30, 0x3a), Mid(0x3c, 0x40, 0x4c), Label(0x9a, 0xa0, 0xb0);
const QColor LineC(0x5f, 0xa8, 0xe8), PointC(0xee, 0xee, 0xee), SusC(0x9e, 0xd8, 0x7a), LoopC(0xe0, 0x9a, 0x60);
const QColor LoopBg(0x2d, 0x28, 0x24), OffC(0x55, 0x5a, 0x68);
const int L = 34, R = 8, T = 8, B = 18;
} // namespace

/* --- La envolvente --------------------------------------------------------------- */

EnvelopeView::EnvelopeView(Editor *ed, int which, QWidget *parent) : QWidget(parent), ed_(ed), which_(which)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(std::max(8, f.pointSize() - 2));
    setFont(f);
    setToolTip(which ? tr("El panorama del instrumento mientras suena la nota (arriba, a la derecha; 32, el centro). "
                          "Arrastra los puntos; un clic añade uno, el botón derecho lo quita.")
                     : tr("El volumen del instrumento mientras suena la nota. Arrastra los puntos; un clic añade "
                          "uno, el botón derecho lo quita."));
    connect(ed_, &Editor::songChanged, this, qOverload<>(&QWidget::update));
}

QSize EnvelopeView::sizeHint() const
{
    return QSize(500, 140);
}

QSize EnvelopeView::minimumSizeHint() const
{
    return QSize(200, 90);
}

SmpInstrument *EnvelopeView::instrument() const
{
    Tune &t = ed_->tune();
    int i = ed_->instrument() - 1;
    return i >= 0 && i < int(t.instruments.size()) ? &t.instruments[size_t(i)] : nullptr;
}

/* Los ticks que se ven: los de la envolvente y un poco más (64 por lo menos). */
int EnvelopeView::maxTick() const
{
    const SmpInstrument *i = instrument();
    int m = 0;
    if (i) {
        ins::Env e = ins::env(*i, which_);
        for (int k = 0; k < e.n; k++)
            m = std::max(m, e.tick[size_t(k)]);
    }
    return std::max(64, (m + m / 4 + 16) / 16 * 16);
}

QPoint EnvelopeView::toScreen(int tick, int value) const
{
    int w = width() - L - R, h = height() - T - B;
    return QPoint(L + tick * w / maxTick(), T + (64 - value) * h / 64);
}

QPoint EnvelopeView::pointPos(int i) const
{
    const SmpInstrument *in = instrument();
    if (!in)
        return QPoint();
    ins::Env e = ins::env(*in, which_);
    return i < e.n ? toScreen(e.tick[size_t(i)], e.value[size_t(i)]) : QPoint();
}

int EnvelopeView::pointAt(QPoint p) const
{
    const SmpInstrument *in = instrument();
    if (!in)
        return -1;
    ins::Env e = ins::env(*in, which_);
    for (int k = 0; k < e.n; k++)
        if ((toScreen(e.tick[size_t(k)], e.value[size_t(k)]) - p).manhattanLength() <= 8)
            return k;
    return -1;
}

void EnvelopeView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Bg);
    QFontMetrics fm(font());
    const SmpInstrument *in = instrument();
    int mt = maxTick();
    /* La rejilla: los valores y los ticks. */
    for (int v = 0; v <= 64; v += 16) {
        QPoint a = toScreen(0, v), b = toScreen(mt, v);
        p.setPen(which_ && v == 32 ? Mid : Grid);
        p.drawLine(a, b);
        p.setPen(Label);
        p.drawText(QRect(0, a.y() - fm.height() / 2, L - 4, fm.height()), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(v));
    }
    int step = mt <= 64 ? 8 : mt <= 160 ? 16 : 32;
    for (int t = 0; t <= mt; t += step) {
        QPoint a = toScreen(t, 64), b = toScreen(t, 0);
        p.setPen(Grid);
        p.drawLine(a, b);
        p.setPen(Label);
        p.drawText(QRect(a.x() - 20, b.y() + 2, 40, B - 2), Qt::AlignCenter, QString::number(t));
    }
    if (!in) {
        p.setPen(Label);
        p.drawText(rect(), Qt::AlignCenter, tr("Sin instrumento"));
        return;
    }
    ins::Env e = ins::env(*in, which_);
    if (e.n == 0) {
        p.setPen(Label);
        p.drawText(rect().adjusted(L, T, -R, -B), Qt::AlignCenter, tr("Un clic para poner el primer punto"));
        return;
    }
    /* El bucle, una banda; el sostener, una raya. */
    if (e.loop && e.loopFrom < e.n && e.loopTo < e.n && e.loopFrom <= e.loopTo) {
        QPoint a = toScreen(e.tick[size_t(e.loopFrom)], 64), b = toScreen(e.tick[size_t(e.loopTo)], 0);
        p.fillRect(QRect(a, b).normalized().adjusted(0, 0, 1, 0), LoopBg);
        p.setPen(LoopC);
        p.drawText(a.x() + 3, a.y() + fm.ascent(), tr("bucle"));
    }
    if (e.sus && e.sustain < e.n) {
        QPoint a = toScreen(e.tick[size_t(e.sustain)], 64), b = toScreen(e.tick[size_t(e.sustain)], 0);
        p.setPen(QPen(SusC, 1, Qt::DashLine));
        p.drawLine(a, b);
        p.setPen(SusC);
        p.drawText(a.x() + 3, b.y() - 3, tr("sostener"));
    }
    QColor line = e.on ? LineC : OffC;
    p.setPen(QPen(line, 2));
    for (int k = 0; k + 1 < e.n; k++)
        p.drawLine(toScreen(e.tick[size_t(k)], e.value[size_t(k)]),
                   toScreen(e.tick[size_t(k + 1)], e.value[size_t(k + 1)]));
    for (int k = 0; k < e.n; k++) {
        QPoint c = toScreen(e.tick[size_t(k)], e.value[size_t(k)]);
        p.fillRect(c.x() - 3, c.y() - 3, 7, 7, e.on ? PointC : OffC);
    }
    if (!e.on) {
        p.setPen(Label);
        p.drawText(rect().adjusted(L + 6, T, -R, -B), Qt::AlignTop | Qt::AlignRight, tr("apagada"));
    }
}

void EnvelopeView::mousePressEvent(QMouseEvent *ev)
{
    SmpInstrument *in = instrument();
    if (!in)
        return;
    QPoint pos = ev->position().toPoint();
    ins::Env e = ins::env(*in, which_);
    int k = pointAt(pos);
    if (ev->button() == Qt::RightButton) {
        if (k < 0)
            return;
        ed_->begin(Editor::Everything);
        for (int j = k; j + 1 < e.n; j++) {
            e.tick[size_t(j)] = e.tick[size_t(j + 1)];
            e.value[size_t(j)] = e.value[size_t(j + 1)];
        }
        e.n--;
        if (k == 0 && e.n > 0)
            e.tick[0] = 0;                  /* el primero, siempre al principio */
        for (int *x : { &e.sustain, &e.loopFrom, &e.loopTo })
            if (*x > k || *x >= e.n)
                *x = std::max(0, *x - 1);
        ins::setEnv(*in, which_, e);
        ed_->changed(Editor::Everything);
        return;
    }
    if (ev->button() != Qt::LeftButton)
        return;
    ed_->begin(Editor::Everything);
    if (k < 0) {
        /* Un punto nuevo, en su sitio (en orden de tick). */
        if (e.n >= ins::POINTS) {
            ed_->undo();
            return;
        }
        int w = width() - L - R, h = height() - T - B;
        int tick = e.n == 0 ? 0 : std::max(0, (pos.x() - L) * maxTick() / std::max(1, w));
        int value = std::clamp(64 - (pos.y() - T) * 64 / std::max(1, h), 0, 64);
        k = 0;
        while (k < e.n && e.tick[size_t(k)] < tick)
            k++;
        if (k < e.n && e.tick[size_t(k)] == tick) {
            e.value[size_t(k)] = value;
        } else {
            if (k == 0 && e.n > 0)
                tick = 0, k = 0;
            for (int j = e.n; j > k; j--) {
                e.tick[size_t(j)] = e.tick[size_t(j - 1)];
                e.value[size_t(j)] = e.value[size_t(j - 1)];
            }
            e.tick[size_t(k)] = tick;
            e.value[size_t(k)] = value;
            e.n++;
            for (int *x : { &e.sustain, &e.loopFrom, &e.loopTo })
                if (*x >= k && e.n > 1)
                    ++*x;
            e.sustain = std::min(e.sustain, e.n - 1);
            e.loopFrom = std::min(e.loopFrom, e.n - 1);
            e.loopTo = std::min(e.loopTo, e.n - 1);
        }
        e.on = true;                        /* se pone para oírla */
        ins::setEnv(*in, which_, e);
    }
    drag_ = k;
    ed_->changed(Editor::Everything);
}

void EnvelopeView::mouseMoveEvent(QMouseEvent *ev)
{
    SmpInstrument *in = instrument();
    if (!in || drag_ < 0 || !(ev->buttons() & Qt::LeftButton))
        return;
    ins::Env e = ins::env(*in, which_);
    if (drag_ >= e.n)
        return;
    QPoint pos = ev->position().toPoint();
    int w = width() - L - R, h = height() - T - B;
    int tick = (pos.x() - L) * maxTick() / std::max(1, w);
    int lo = drag_ == 0 ? 0 : e.tick[size_t(drag_ - 1)] + 1;
    int hi = drag_ == 0 ? 0 : drag_ + 1 < e.n ? e.tick[size_t(drag_ + 1)] - 1 : 65535;
    e.tick[size_t(drag_)] = std::clamp(tick, lo, std::max(lo, hi));
    e.value[size_t(drag_)] = std::clamp(64 - (pos.y() - T) * 64 / std::max(1, h), 0, 64);
    ins::setEnv(*in, which_, e);
    ed_->changed(Editor::Everything);
}

/* --- El mapa de notas ------------------------------------------------------------ */

NoteMapView::NoteMapView(Editor *ed, QWidget *parent) : QWidget(parent), ed_(ed)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(std::max(8, f.pointSize() - 2));
    setFont(f);
    setToolTip(tr("La muestra que suena con cada nota (el número, sobre la primera de cada tramo). Un clic o "
                  "arrastrar pone la muestra elegida."));
    connect(ed_, &Editor::songChanged, this, qOverload<>(&QWidget::update));
}

QSize NoteMapView::sizeHint() const
{
    return QSize(96 * 10 + 8, 64);
}

QSize NoteMapView::minimumSizeHint() const
{
    return QSize(96 * 4 + 8, 56);
}

QRect NoteMapView::keyRect(int note) const
{
    int w = width() - 8, lh = QFontMetrics(font()).height();
    int x0 = 4 + note * w / 96, x1 = 4 + (note + 1) * w / 96;
    return QRect(x0, lh + 4, std::max(1, x1 - x0), height() - 2 * lh - 8);
}

int NoteMapView::noteAt(QPoint p) const
{
    int w = std::max(1, width() - 8);
    return std::clamp((p.x() - 4) * 96 / w, 0, 95);
}

void NoteMapView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Bg);
    QFontMetrics fm(font());
    Tune &t = ed_->tune();
    int idx = ed_->instrument() - 1;
    if (idx < 0 || idx >= int(t.instruments.size())) {
        p.setPen(Label);
        p.drawText(rect(), Qt::AlignCenter, tr("Sin instrumento: haz uno con «Nuevo» o importa un WAV"));
        return;
    }
    const SmpInstrument &in = t.instruments[size_t(idx)];
    int prev = -1;
    for (int n = 0; n < 96; n++) {
        QRect r = keyRect(n);
        int s = in.data[size_t(ZXM_SI_MAP + n)];
        bool black = (0x54A >> (n % 12)) & 1;       /* C# D# F# G# A# */
        QColor c = s == 255 || s >= int(t.waves.size()) ? OffC : QColor::fromHsv((s * 47) % 360, 120, black ? 150 : 210);
        p.fillRect(r.adjusted(0, 0, -1, 0), c);
        if (s != prev) {
            p.setPen(Label);
            p.drawText(r.x(), fm.ascent() + 2, s == 255 ? QStringLiteral("--") : QString::number(s));
            p.setPen(Bg);
            p.drawLine(r.x(), r.y(), r.x(), r.bottom());
            prev = s;
        }
        if (n % 12 == 0) {
            p.setPen(Label);
            p.drawText(r.x(), r.bottom() + fm.ascent() + 2, QStringLiteral("C-%1").arg(n / 12));
        }
    }
}

void NoteMapView::paintNote(int note, bool first)
{
    Tune &t = ed_->tune();
    int idx = ed_->instrument() - 1;
    if (idx < 0 || idx >= int(t.instruments.size()))
        return;
    if (first)
        ed_->begin(Editor::Everything);
    int from = last_ < 0 || first ? note : last_;
    for (int n = std::min(from, note); n <= std::max(from, note); n++)
        t.instruments[size_t(idx)].data[size_t(ZXM_SI_MAP + n)] = uint8_t(paint_);
    last_ = note;
    ed_->changed(Editor::Everything);
}

void NoteMapView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        paintNote(noteAt(e->position().toPoint()), true);
}

void NoteMapView::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
        paintNote(noteAt(e->position().toPoint()), false);
}
