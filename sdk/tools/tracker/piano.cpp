/*
 * piano.cpp - El teclado como piano y el piano de tres octavas.
 */
#include "piano.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

#include "song.h"

int pianoKey(int k)
{
    switch (k) {
    case Qt::Key_Z: return 0;
    case Qt::Key_S: return 1;
    case Qt::Key_X: return 2;
    case Qt::Key_D: return 3;
    case Qt::Key_C: return 4;
    case Qt::Key_V: return 5;
    case Qt::Key_G: return 6;
    case Qt::Key_B: return 7;
    case Qt::Key_H: return 8;
    case Qt::Key_N: return 9;
    case Qt::Key_J: return 10;
    case Qt::Key_M: return 11;
    case Qt::Key_Comma: return 12;
    case Qt::Key_L: return 13;
    case Qt::Key_Period: return 14;
    case Qt::Key_Q: return 12;
    case Qt::Key_2: return 13;
    case Qt::Key_W: return 14;
    case Qt::Key_3: return 15;
    case Qt::Key_E: return 16;
    case Qt::Key_R: return 17;
    case Qt::Key_5: return 18;
    case Qt::Key_T: return 19;
    case Qt::Key_6: return 20;
    case Qt::Key_Y: return 21;
    case Qt::Key_7: return 22;
    case Qt::Key_U: return 23;
    case Qt::Key_I: return 24;
    case Qt::Key_9: return 25;
    case Qt::Key_O: return 26;
    case Qt::Key_0: return 27;
    case Qt::Key_P: return 28;
    default: return -1;
    }
}

static const int OCTAVES = 3;
static const bool black[12] = { false, true, false, true, false, false, true, false, true, false, true, false };

Piano::Piano(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(48);
    setToolTip(tr("Toca con el ratón o con el teclado (Z, S, X... y Q, 2, W...)"));
}

void Piano::setOctave(int octave)
{
    octave_ = std::clamp(octave, 1, 8);
    update();
}

QSize Piano::sizeHint() const
{
    return QSize(OCTAVES * 7 * 18, 64);
}

/* La nota bajo el ratón: las negras tapan la mitad de arriba. */
int Piano::noteAt(QPoint p) const
{
    double ww = double(width()) / (OCTAVES * 7);
    int white = int(p.x() / ww);
    if (white < 0 || white >= OCTAVES * 7)
        return -1;
    static const int whites[7] = { 0, 2, 4, 5, 7, 9, 11 };
    int note = white / 7 * 12 + whites[white % 7];
    if (p.y() < height() * 6 / 10) {
        double fx = p.x() / ww - white;
        int n = white % 7;
        if (fx > 0.65 && n != 2 && n != 6)
            note++;
        else if (fx < 0.35 && n != 0 && n != 3)
            note--;
    }
    int base = (octave_ - 1) * 12;
    return std::clamp(base + note, 0, trk::NOTES - 1);
}

void Piano::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    double ww = double(width()) / (OCTAVES * 7);
    int base = (octave_ - 1) * 12;
    static const int whites[7] = { 0, 2, 4, 5, 7, 9, 11 };
    for (int i = 0; i < OCTAVES * 7; i++) {
        QRectF r(i * ww, 0, ww, height() - 1);
        int note = base + i / 7 * 12 + whites[i % 7];
        p.fillRect(r, note == down_ ? QColor(0x9f, 0xb8, 0xff) : Qt::white);
        p.setPen(Qt::gray);
        p.drawRect(r);
        if (i % 7 == 0) {
            p.setPen(Qt::darkGray);
            p.drawText(r.adjusted(0, 0, 0, -3), Qt::AlignHCenter | Qt::AlignBottom,
                       QString::fromStdString(trk::noteName(note)).remove('-'));
        }
    }
    for (int i = 0; i < OCTAVES * 12; i++) {
        if (!black[i % 12])
            continue;
        int white = i / 12 * 7 + (i % 12 < 5 ? (i % 12) / 2 : (i % 12 + 1) / 2);
        QRectF r((white + 0.65) * ww, 0, ww * 0.7, height() * 6 / 10);
        p.fillRect(r, base + i == down_ ? QColor(0x50, 0x70, 0xd0) : QColor(0x20, 0x20, 0x20));
    }
    if (hasFocus()) {
        p.setPen(QPen(palette().highlight().color(), 2));
        p.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}

void Piano::press(int note)
{
    if (note == down_)
        return;
    down_ = note;
    update();
    if (note >= 0)
        emit noteOn(note);
    else
        emit noteOff();
}

void Piano::mousePressEvent(QMouseEvent *e)
{
    setFocus();
    press(noteAt(e->pos()));
}

void Piano::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
        press(noteAt(e->pos()));
}

void Piano::mouseReleaseEvent(QMouseEvent *)
{
    press(-1);
}

void Piano::keyPressEvent(QKeyEvent *e)
{
    int k = e->modifiers() & (Qt::ControlModifier | Qt::AltModifier) ? -1 : pianoKey(e->key());
    if (k < 0) {
        QWidget::keyPressEvent(e);
        return;
    }
    if (e->isAutoRepeat())
        return;
    key_ = e->key();
    press(std::clamp((octave_ - 1) * 12 + k, 0, trk::NOTES - 1));
}

/* Como en el patrón: suena mientras la tecla esté pulsada, y se para al
 * soltar cualquiera del piano o al perder el foco. */
void Piano::keyReleaseEvent(QKeyEvent *e)
{
    if (!e->isAutoRepeat() && key_ && (e->key() == key_ || e->key() == 0 || pianoKey(e->key()) >= 0)) {
        key_ = 0;
        press(-1);
        return;
    }
    QWidget::keyReleaseEvent(e);
}

void Piano::focusOutEvent(QFocusEvent *e)
{
    key_ = 0;
    press(-1);
    QWidget::focusOutEvent(e);
}
