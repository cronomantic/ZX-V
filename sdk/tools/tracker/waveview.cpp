/*
 * waveview.cpp - La onda de una muestra y su bucle (ver waveview.h).
 */
#include "waveview.h"

#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

#include "editor.h"

using namespace trk;

namespace {
const QColor Bg(0x1d, 0x1f, 0x26), Mid(0x3c, 0x40, 0x4c), WaveC(0x5f, 0xa8, 0xe8), LoopC(0xe0, 0x9a, 0x60);
const QColor LoopBg(0x2d, 0x28, 0x24), Label(0x9a, 0xa0, 0xb0);
}

WaveView::WaveView(Editor *ed, QWidget *parent) : QWidget(parent), ed_(ed)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(std::max(9, f.pointSize() - 1));
    setFont(f);
    setToolTip(tr("La onda de la muestra. Lo que se repite mientras suena la nota (el bucle), en naranja: "
                  "un clic pone dónde empieza."));
    connect(ed_, &Editor::songChanged, this, &WaveView::reload);
}

QSize WaveView::sizeHint() const
{
    return QSize(600, 220);
}

QSize WaveView::minimumSizeHint() const
{
    return QSize(200, 100);
}

void WaveView::setWave(int w)
{
    wave_ = w;
    reload();
}

void WaveView::reload()
{
    const Tune &t = ed_->tune();
    samples_ = wave_ >= 0 && wave_ < int(t.waves.size()) ? waveSamples(t.waves[size_t(wave_)]) : std::vector<int16_t>();
    update();
}

uint32_t WaveView::sampleAt(int x) const
{
    if (samples_.empty())
        return 0;
    double f = std::clamp(double(x - 4) / std::max(1, width() - 8), 0.0, 1.0);
    return std::min(uint32_t(f * samples_.size()), uint32_t(samples_.size()) - 1);
}

void WaveView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Bg);
    if (samples_.empty()) {
        p.setPen(Label);
        p.drawText(rect().adjusted(8, 8, -8, -8), Qt::AlignCenter | Qt::TextWordWrap,
                   tr("No hay ninguna muestra: impórtala de un WAV."));
        return;
    }
    const Wave &w = ed_->tune().waves[size_t(wave_)];
    int n = int(samples_.size()), x0 = 4, ww = std::max(1, width() - 8), mid = height() / 2, half = height() / 2 - 6;
    bool looped = w.loop < w.len;
    if (looped) {
        int lx = x0 + int(double(w.loop) * ww / n);
        p.fillRect(lx, 0, x0 + ww - lx, height(), LoopBg);
    }
    p.fillRect(x0, mid, ww, 1, Mid);
    /* Cada columna, de la más baja a la más alta de sus muestras (y de la
     * última de la anterior: la onda, seguida). */
    int prev = samples_[0];
    for (int x = 0; x < ww; x++) {
        int a = int(double(x) * n / ww), b = std::max(a + 1, int(double(x + 1) * n / ww));
        int lo = prev, hi = prev;
        for (int i = a; i < b && i < n; i++) {
            lo = std::min<int>(lo, samples_[size_t(i)]);
            hi = std::max<int>(hi, samples_[size_t(i)]);
        }
        prev = samples_[size_t(std::min(b, n) - 1)];
        bool inLoop = looped && uint32_t(a) >= w.loop;
        p.fillRect(x0 + x, mid - hi * half / 32768, 1, std::max(1, (hi - lo) * half / 32768), inLoop ? LoopC : WaveC);
    }
    if (looped) {
        int lx = x0 + int(double(w.loop) * ww / n);
        p.fillRect(lx, 0, 2, height(), LoopC);
        p.setPen(LoopC);
        p.drawText(lx + 4, QFontMetrics(font()).ascent() + 2, tr("bucle desde %1").arg(w.loop));
    }
}

void WaveView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && !samples_.empty())
        emit loopChanged(sampleAt(int(e->position().x())), true);
}

void WaveView::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) && !samples_.empty())
        emit loopChanged(sampleAt(int(e->position().x())), false);
}
