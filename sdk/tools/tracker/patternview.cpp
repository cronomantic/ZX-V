/*
 * patternview.cpp - El patrón: pintarlo, moverse, escribir notas y valores,
 * seleccionar, copiar y pegar.
 */
#include "patternview.h"

#include <QFontDatabase>
#include <QKeyEvent>
#include <QPainter>

#include <algorithm>

#include "editor.h"
#include "piano.h"

using namespace trk;

/* Dónde va cada cosa en una línea, en caracteres. */
static const int X_ENV = 4, X_NOISE = 9, X_CH = 13, CH_W = 15, LINE = X_CH + 2 * CH_W + 13;
static const char SAMPLE_DIGITS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

/* Lo copiado: filas enteras y las pistas (0 env y ruido, 1-3 los canales). */
static struct {
    std::vector<Row> rows;
    int first = 0, last = -1;
} clip;

namespace {

struct Colors {
    QColor bg{ 0x1d, 0x1f, 0x26 }, beat{ 0x25, 0x28, 0x32 }, bar{ 0x2d, 0x31, 0x40 }, line{ 0x33, 0x3a, 0x58 };
    QColor play{ 0x55, 0x30, 0x30 }, sel{ 0x2f, 0x52, 0x3a }, cursor{ 0x6f, 0x7f, 0xd8 };
    QColor dim{ 0x55, 0x5a, 0x68 }, num{ 0x80, 0x86, 0x96 }, numBeat{ 0xc8, 0xcc, 0xd8 };
    QColor note{ 0xee, 0xee, 0xee }, off{ 0xe0, 0x9a, 0x60 }, smp{ 0xe6, 0xcf, 0x6a };
    QColor env{ 0x7c, 0xc4, 0xd8 }, vol{ 0x9e, 0xd8, 0x7a }, fx{ 0xd8, 0x8c, 0xd8 }, sep{ 0x3c, 0x40, 0x4c };
};
const Colors C;

int hexValue(QChar c)
{
    c = c.toUpper();
    if (c >= '0' && c <= '9')
        return c.unicode() - '0';
    if (c >= 'A' && c <= 'F')
        return c.unicode() - 'A' + 10;
    return -1;
}

} // namespace

PatternView::PatternView(Editor *ed, QWidget *parent) : QWidget(parent), ed_(ed)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(std::max(10, f.pointSize()));
    setFont(f);
    QFontMetrics fm(f);
    cw_ = fm.horizontalAdvance('0');
    lh_ = fm.height() + 1;
    top_ = lh_ + 4;

    stops_.push_back({ X_ENV, Env, -1, 0 });
    stops_.push_back({ X_ENV + 1, Env, -1, 1 });
    stops_.push_back({ X_ENV + 2, Env, -1, 2 });
    stops_.push_back({ X_ENV + 3, Env, -1, 3 });
    stops_.push_back({ X_NOISE, Noise, -1, 0 });
    stops_.push_back({ X_NOISE + 1, Noise, -1, 1 });
    for (int c = 0; c < 3; c++) {
        int b = X_CH + c * CH_W;
        stops_.push_back({ b, Note, c, 0 });
        stops_.push_back({ b + 4, Sample, c, 0 });
        stops_.push_back({ b + 5, EnvType, c, 0 });
        stops_.push_back({ b + 6, Orn, c, 0 });
        stops_.push_back({ b + 7, Vol, c, 0 });
        stops_.push_back({ b + 9, Fx, c, 0 });
        stops_.push_back({ b + 10, Delay, c, 0 });
        stops_.push_back({ b + 11, Param, c, 0 });
        stops_.push_back({ b + 12, Param, c, 1 });
    }
    stop_ = 6;
    connect(ed_, &Editor::songChanged, this, [this] {
        row_ = std::clamp(row_, 0, rows() - 1);
        update();
    });
    connect(ed_, &Editor::positionChanged, this, [this] {
        row_ = std::clamp(row_, 0, rows() - 1);
        selecting_ = false;
        update();
    });
}

QSize PatternView::sizeHint() const
{
    return QSize(LINE * cw_ + 12, lh_ * 33 + top_);
}

int PatternView::rows() const
{
    return std::max(1, int(ed_->mod().patterns[size_t(ed_->pattern())].rows.size()));
}

int PatternView::channel() const
{
    return stops_[size_t(stop_)].ch;
}

void PatternView::setRow(int r)
{
    r = std::clamp(r, 0, rows() - 1);
    if (r != row_) {
        row_ = r;
        update();
        emit cursorMoved();
    }
}

void PatternView::setPlayRow(int r)
{
    if (r != playRow_) {
        playRow_ = r;
        update();
    }
}

void PatternView::setOctave(int o)
{
    octave_ = std::clamp(o, 1, 8);
}

void PatternView::moveTo(int row, int stop, bool select)
{
    if (select && !selecting_) {
        anchorRow_ = row_;
        anchorTrack_ = track(stops_[size_t(stop_)]);
        selecting_ = true;
    } else if (!select) {
        selecting_ = false;
    }
    row_ = std::clamp(row, 0, rows() - 1);
    stop_ = std::clamp(stop, 0, int(stops_.size()) - 1);
    update();
    emit cursorMoved();
}

QRect PatternView::selection() const
{
    int t = track(stops_[size_t(stop_)]);
    if (!selecting_)
        return QRect(t, row_, 1, 1);
    int t0 = std::min(t, anchorTrack_), t1 = std::max(t, anchorTrack_);
    int r0 = std::min(row_, anchorRow_), r1 = std::max(row_, anchorRow_);
    return QRect(t0, r0, t1 - t0 + 1, r1 - r0 + 1);
}

/* --- Pintar ----------------------------------------------------------------- */

void PatternView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), C.bg);
    const int mx = 6;
    const Pattern &pat = ed_->mod().patterns[size_t(ed_->pattern())];
    int n = int(pat.rows.size());
    int lines = (height() - top_) / lh_ + 1;
    int center = (height() - top_) / lh_ / 2;
    QFontMetrics fm(font());
    int asc = fm.ascent();
    auto put = [&](int x, int y, const QString &s, const QColor &c) {
        p.setPen(c);
        p.drawText(mx + x * cw_, y + asc, s);
    };

    /* La cabecera. */
    p.fillRect(0, 0, width(), top_ - 2, C.beat);
    put(X_ENV, 2, tr("Env."), C.num);
    put(X_NOISE, 2, tr("Ru"), C.num);
    for (int c = 0; c < 3; c++) {
        QString name = QString(QChar('A' + c));
        if (ed_->song().modules.size() > 1)
            name = tr("AY%1 %2").arg(ed_->module() + 1).arg(name);
        put(X_CH + c * CH_W + 5 - name.size() / 2, 2, name, C.numBeat);
    }

    QRect sel = selection();
    int numW = n > 100 ? 3 : 2;
    for (int i = 0; i < lines; i++) {
        int r = row_ - center + i;
        if (r < 0 || r >= n)
            continue;
        int y = top_ + i * lh_;
        QColor bg = r == row_ ? C.line : r == playRow_ ? C.play
                  : r % (highlight_ * 4) == 0      ? C.bar
                  : r % highlight_ == 0            ? C.beat
                                                   : C.bg;
        if (r == playRow_ && r == row_)
            bg = C.play.lighter(130);
        p.fillRect(0, y, width(), lh_, bg);
        if (selecting_ && r >= sel.top() && r <= sel.bottom()) {
            int x0 = sel.left() == 0 ? X_ENV : X_CH + (sel.left() - 1) * CH_W;
            int x1 = sel.right() == 0 ? X_NOISE + 2 : X_CH + (sel.right() - 1) * CH_W + 13;
            p.fillRect(mx + x0 * cw_ - 2, y, (x1 - x0) * cw_ + 4, lh_, C.sel);
        }
        if (r == row_) {
            const Stop &s = stops_[size_t(stop_)];
            int w = s.kind == Note ? 3 : 1;
            QColor cc = hasFocus() ? C.cursor : C.cursor.darker(170);
            p.fillRect(mx + s.x * cw_ - 1, y, w * cw_ + 2, lh_, cc);
        }

        const Row &row = pat.rows[size_t(r)];
        put(0, y, QString::number(r).rightJustified(numW, '0'), r % highlight_ == 0 ? C.numBeat : C.num);
        put(X_ENV, y, row.envPeriod < 0 ? "...." : QString::number(row.envPeriod, 16).toUpper().rightJustified(4, '0'),
            row.envPeriod < 0 ? C.dim : C.env);
        put(X_NOISE, y, row.noise < 0 ? ".." : QString::number(row.noise, 16).toUpper().rightJustified(2, '0'),
            row.noise < 0 ? C.dim : C.env);
        for (int c = 0; c < 3; c++) {
            const Cell &cell = row.ch[c];
            int b = X_CH + c * CH_W;
            QString note = QString::fromStdString(noteName(cell.note));
            put(b, y, note, cell.note == NOTE_NONE ? C.dim : cell.note == NOTE_OFF ? C.off : C.note);
            put(b + 4, y, cell.sample ? QString(QChar(SAMPLE_DIGITS[cell.sample & 31])) : ".", cell.sample ? C.smp : C.dim);
            put(b + 5, y, cell.env ? QString::number(cell.env, 16).toUpper() : ".", cell.env ? C.env : C.dim);
            put(b + 6, y, cell.orn >= 0 ? QString::number(cell.orn, 16).toUpper() : ".", cell.orn >= 0 ? C.env : C.dim);
            put(b + 7, y, cell.vol ? QString::number(cell.vol, 16).toUpper() : ".", cell.vol ? C.vol : C.dim);
            if (cell.fx) {
                put(b + 9, y, QString::number(cell.fx, 16).toUpper(), C.fx);
                put(b + 10, y, QString::number(cell.fxDelay & 15, 16).toUpper(), C.fx);
                put(b + 11, y, QString::number(cell.fxParam & 0xFF, 16).toUpper().rightJustified(2, '0'), C.fx);
            } else {
                put(b + 9, y, "....", C.dim);
            }
        }
    }

    /* Las rayas entre las pistas. */
    p.setPen(C.sep);
    for (int x : { X_CH - 1, X_CH + CH_W - 1, X_CH + 2 * CH_W - 1 })
        p.drawLine(mx + x * cw_ + cw_ / 2, 0, mx + x * cw_ + cw_ / 2, height());
}

/* --- Teclado ------------------------------------------------------------------ */

void PatternView::advance(bool multi)
{
    const Stop &s = stops_[size_t(stop_)];
    if (multi && stop_ + 1 < int(stops_.size()) && stops_[size_t(stop_ + 1)].kind == s.kind &&
        stops_[size_t(stop_ + 1)].ch == s.ch) {
        stop_++;
        update();
        return;
    }
    if (multi)
        stop_ -= s.digit;
    moveTo(std::min(row_ + step_, rows() - 1), stop_, false);
}

bool PatternView::enter(QKeyEvent *e)
{
    const Stop &s = stops_[size_t(stop_)];
    Pattern &pat = ed_->pat();
    if (row_ >= int(pat.rows.size()))
        return false;
    int p = ed_->pattern();

    if (s.kind == Note) {
        int k = pianoKey(e->key());
        bool off = e->key() == Qt::Key_1 || e->key() == Qt::Key_A;
        if (k < 0 && !off)
            return false;
        if (e->isAutoRepeat())
            return true;
        ed_->begin(Editor::PatternPart, p);
        Row &row = ed_->pat().rows[size_t(row_)];
        Cell &c = row.ch[s.ch];
        if (off) {
            c.note = NOTE_OFF;
        } else {
            c.note = std::clamp((octave_ - 1) * 12 + k, 0, NOTES - 1);
            c.sample = ed_->sample();
            previewKey_ = e->key();
            emit preview(c, row.envPeriod, s.ch);
        }
        ed_->changed(Editor::PatternPart);
        advance(false);
        return true;
    }

    QString t = e->text().toUpper();
    if (t.isEmpty())
        return false;
    QChar ch = t[0];
    int v = hexValue(ch);
    if (s.kind == Sample) {
        int i = QString(SAMPLE_DIGITS).indexOf(ch);
        if (i < 0)
            return false;
        v = i;
    }
    if (v < 0)
        return false;
    if (s.kind == Noise && s.digit == 0 && v > 1)
        return false;
    if (s.kind == Fx && !(v <= 6 || v == 9 || v == 10 || v == 11))
        return false;

    ed_->begin(Editor::PatternPart, p);
    Row &row = ed_->pat().rows[size_t(row_)];
    Cell *c = s.ch >= 0 ? &row.ch[s.ch] : nullptr;
    bool multi = false;
    switch (s.kind) {
    case Env: {
        int val = row.envPeriod < 0 ? 0 : row.envPeriod;
        int sh = (3 - s.digit) * 4;
        row.envPeriod = (val & ~(0xF << sh)) | v << sh;
        multi = true;
        break;
    }
    case Noise: {
        int val = row.noise < 0 ? 0 : row.noise;
        val = s.digit == 0 ? (v << 4 | (val & 15)) : ((val & 0xF0) | v);
        row.noise = val & 31;
        multi = true;
        break;
    }
    case Sample:
        c->sample = v;
        break;
    case EnvType:
        c->env = v;
        break;
    case Orn:
        c->orn = v;
        break;
    case Vol:
        c->vol = v;
        break;
    case Fx:
        c->fx = v;
        if (!v)
            c->fxDelay = c->fxParam = 0;
        break;
    case Delay:
        c->fxDelay = v;
        break;
    case Param: {
        int val = c->fxParam & 0xFF;
        c->fxParam = s.digit == 0 ? (v << 4 | (val & 15)) : ((val & 0xF0) | v);
        multi = true;
        break;
    }
    default:
        break;
    }
    ed_->changed(Editor::PatternPart);
    advance(multi);
    return true;
}

void PatternView::clearField()
{
    const Stop &s = stops_[size_t(stop_)];
    ed_->begin(Editor::PatternPart, ed_->pattern());
    Row &row = ed_->pat().rows[size_t(row_)];
    Cell *c = s.ch >= 0 ? &row.ch[s.ch] : nullptr;
    switch (s.kind) {
    case Env: row.envPeriod = -1; break;
    case Noise: row.noise = -1; break;
    case Note: c->note = NOTE_NONE; c->sample = 0; break;
    case Sample: c->sample = 0; break;
    case EnvType: c->env = ENV_NONE; break;
    case Orn: c->orn = -1; break;
    case Vol: c->vol = 0; break;
    case Fx:
    case Delay:
    case Param: c->fx = c->fxDelay = c->fxParam = 0; break;
    }
    ed_->changed(Editor::PatternPart);
    moveTo(std::min(row_ + step_, rows() - 1), stop_ - (s.kind == Param || s.kind == Env || s.kind == Noise ? s.digit : 0), false);
}

void PatternView::keyPressEvent(QKeyEvent *e)
{
    bool shift = e->modifiers() & Qt::ShiftModifier;
    bool ctrl = e->modifiers() & Qt::ControlModifier;
    int chStop = 6 + std::max(0, channel()) * 9;     /* la nota del canal */
    switch (e->key()) {
    case Qt::Key_Up: moveTo(row_ - 1, stop_, shift); return;
    case Qt::Key_Down: moveTo(row_ + 1, stop_, shift); return;
    case Qt::Key_PageUp: moveTo(row_ - 16, stop_, shift); return;
    case Qt::Key_PageDown: moveTo(row_ + 16, stop_, shift); return;
    case Qt::Key_Home: moveTo(0, ctrl ? 0 : stop_, shift); return;
    case Qt::Key_End: moveTo(rows() - 1, stop_, shift); return;
    case Qt::Key_Left: moveTo(row_, stop_ - 1, shift); return;
    case Qt::Key_Right: moveTo(row_, stop_ + 1, shift); return;
    case Qt::Key_Tab: moveTo(row_, channel() < 0 ? 6 : channel() < 2 ? chStop + 9 : 0, false); return;
    case Qt::Key_Backtab: moveTo(row_, channel() < 0 ? 24 : channel() == 0 ? 0 : chStop - 9, false); return;
    case Qt::Key_Delete: erase(); return;
    case Qt::Key_Insert: insertRow(); return;
    case Qt::Key_Backspace: deleteRow(); return;
    case Qt::Key_Escape:
        if (selecting_) {
            selecting_ = false;
            update();
        } else {
            emit stopRequested();
        }
        return;
    default:
        if (!ctrl && !(e->modifiers() & Qt::AltModifier) && enter(e))
            return;
    }
    QWidget::keyPressEvent(e);
}

/* La nota que se oye al escribirla dura lo que la tecla. Se para al soltar
 * cualquier tecla del piano (o una que Qt no sepa cuál es, como las que
 * mandan otros programas), y al perder el foco: que no se quede sonando. */
void PatternView::keyReleaseEvent(QKeyEvent *e)
{
    if (!e->isAutoRepeat() && previewKey_ &&
        (e->key() == previewKey_ || e->key() == 0 || pianoKey(e->key()) >= 0)) {
        previewKey_ = 0;
        emit previewOff();
        return;
    }
    QWidget::keyReleaseEvent(e);
}

void PatternView::focusOutEvent(QFocusEvent *)
{
    if (previewKey_) {
        previewKey_ = 0;
        emit previewOff();
    }
    update();
}

/* --- Ratón -------------------------------------------------------------------- */

int PatternView::rowAt(int y) const
{
    int center = (height() - top_) / lh_ / 2;
    int line = y < top_ ? 0 : (y - top_) / lh_;
    return row_ + line - center;
}

int PatternView::stopAt(int x) const
{
    int cx = (x - 6) / std::max(1, cw_);
    int best = 0;
    for (size_t i = 0; i < stops_.size(); i++)
        if (stops_[i].x <= cx)
            best = int(i);
    return best;
}

void PatternView::mousePressEvent(QMouseEvent *e)
{
    setFocus();
    if (e->button() != Qt::LeftButton)
        return;
    int r = rowAt(int(e->position().y()));
    int s = stopAt(int(e->position().x()));
    moveTo(r, s, e->modifiers() & Qt::ShiftModifier);
}

void PatternView::mouseMoveEvent(QMouseEvent *e)
{
    if (!(e->buttons() & Qt::LeftButton))
        return;
    int r = rowAt(int(e->position().y()));
    int s = stopAt(int(e->position().x()));
    if (r != row_ || track(stops_[size_t(s)]) != track(stops_[size_t(stop_)]))
        moveTo(r, s, true);
}

void PatternView::wheelEvent(QWheelEvent *e)
{
    int d = e->angleDelta().y() / 40;
    if (d)
        moveTo(row_ - d, stop_, false);
}

/* --- Bloques ------------------------------------------------------------------ */

void PatternView::copy()
{
    QRect s = selection();
    const Pattern &pat = ed_->mod().patterns[size_t(ed_->pattern())];
    clip.rows.clear();
    for (int r = s.top(); r <= s.bottom() && r < int(pat.rows.size()); r++)
        clip.rows.push_back(pat.rows[size_t(r)]);
    clip.first = s.left();
    clip.last = s.right();
}

void PatternView::erase()
{
    if (!selecting_) {
        clearField();
        return;
    }
    QRect s = selection();
    ed_->begin(Editor::PatternPart, ed_->pattern());
    Pattern &pat = ed_->pat();
    for (int r = s.top(); r <= s.bottom() && r < int(pat.rows.size()); r++)
        for (int t = s.left(); t <= s.right(); t++) {
            Row &row = pat.rows[size_t(r)];
            if (t == 0) {
                row.envPeriod = -1;
                row.noise = -1;
            } else {
                row.ch[t - 1] = Cell();
            }
        }
    ed_->changed(Editor::PatternPart);
}

void PatternView::cut()
{
    copy();
    bool was = selecting_;
    selecting_ = true;
    if (!was) {
        anchorRow_ = row_;
        anchorTrack_ = track(stops_[size_t(stop_)]);
    }
    erase();
    selecting_ = was;
}

void PatternView::paste()
{
    if (clip.rows.empty())
        return;
    ed_->begin(Editor::PatternPart, ed_->pattern());
    Pattern &pat = ed_->pat();
    int t0 = track(stops_[size_t(stop_)]);
    for (size_t i = 0; i < clip.rows.size() && row_ + int(i) < int(pat.rows.size()); i++) {
        Row &dst = pat.rows[size_t(row_) + i];
        const Row &src = clip.rows[i];
        for (int t = clip.first; t <= clip.last; t++) {
            int d = t0 + (t - clip.first);
            if (d > 3)
                break;
            if (t == 0 && d == 0) {
                dst.envPeriod = src.envPeriod;
                dst.noise = src.noise;
            } else if (t > 0 && d > 0) {
                dst.ch[d - 1] = src.ch[t - 1];
                if (src.ch[t - 1].env >= 1 && src.ch[t - 1].env <= 14 && dst.envPeriod < 0)
                    dst.envPeriod = src.envPeriod;
            }
        }
    }
    ed_->changed(Editor::PatternPart);
    moveTo(std::min(row_ + int(clip.rows.size()), rows() - 1), stop_, false);
}

void PatternView::selectAll()
{
    anchorRow_ = 0;
    anchorTrack_ = 0;
    selecting_ = true;
    row_ = rows() - 1;
    stop_ = int(stops_.size()) - 1;
    update();
}

void PatternView::transpose(int semitones)
{
    QRect s = selection();
    if (!selecting_)
        s = QRect(std::max(1, track(stops_[size_t(stop_)])), 0, 1, rows());
    ed_->begin(Editor::PatternPart, ed_->pattern());
    Pattern &pat = ed_->pat();
    for (int r = s.top(); r <= s.bottom() && r < int(pat.rows.size()); r++)
        for (int t = std::max(1, s.left()); t <= s.right(); t++) {
            Cell &c = pat.rows[size_t(r)].ch[t - 1];
            if (c.note >= 0)
                c.note = std::clamp(c.note + semitones, 0, NOTES - 1);
        }
    ed_->changed(Editor::PatternPart);
}

void PatternView::insertRow()
{
    ed_->begin(Editor::PatternPart, ed_->pattern());
    Pattern &pat = ed_->pat();
    pat.rows.insert(pat.rows.begin() + row_, Row());
    pat.rows.pop_back();
    ed_->changed(Editor::PatternPart);
}

void PatternView::deleteRow()
{
    ed_->begin(Editor::PatternPart, ed_->pattern());
    Pattern &pat = ed_->pat();
    pat.rows.erase(pat.rows.begin() + row_);
    pat.rows.push_back(Row());
    ed_->changed(Editor::PatternPart);
}
