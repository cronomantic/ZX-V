/*
 * lines.cpp - Los editores de samples, ornamentos y efectos AYFX.
 */
#include "lines.h"

#include <QFontDatabase>
#include <QKeyEvent>
#include <QPainter>

#include <algorithm>

#include "editor.h"

using namespace trk;

const QColor LineGrid::Dim(0x55, 0x5a, 0x68);
const QColor LineGrid::Text(0xdd, 0xdd, 0xdd);
const QColor LineGrid::On(0x9e, 0xd8, 0x7a);
const QColor LineGrid::Hi(0xe6, 0xcf, 0x6a);

static const QColor Bg(0x1d, 0x1f, 0x26), Beat(0x25, 0x28, 0x32), Cur(0x33, 0x3a, 0x58), Cursor(0x6f, 0x7f, 0xd8);
static const QColor Loop(0xe0, 0x9a, 0x60), BarOn(0x5f, 0xa8, 0xe8), BarOff(0x2c, 0x30, 0x3a);

static int hexValue(QChar c)
{
    c = c.toUpper();
    if (c >= '0' && c <= '9')
        return c.unicode() - '0';
    if (c >= 'A' && c <= 'F')
        return c.unicode() - 'A' + 10;
    return -1;
}

static QString hex(int v, int digits)
{
    return QString::number(v, 16).toUpper().rightJustified(digits, '0').right(digits);
}

LineGrid::LineGrid(QWidget *parent) : QWidget(parent)
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
}

QSize LineGrid::sizeHint() const
{
    int w = 0;
    for (const Stop &s : stops_)
        w = std::max(w, s.x + s.w);
    if (barX_ >= 0)
        w = std::max(w, barX_ + 15);
    return QSize((w + 2) * cw_ + 12, top_ + lh_ * 24);
}

QSize LineGrid::minimumSizeHint() const
{
    return QSize(sizeHint().width(), top_ + lh_ * 6);
}

void LineGrid::put(QPainter &p, int x, int y, const QString &s, const QColor &c)
{
    p.setPen(c);
    p.drawText(6 + x * cw_, y + QFontMetrics(font()).ascent(), s);
}

void LineGrid::setLine(int l)
{
    move(l, stop_);
}

void LineGrid::move(int line, int stop)
{
    line_ = std::clamp(line, 0, std::max(0, count() - 1));
    stop_ = std::clamp(stop, 0, int(stops_.size()) - 1);
    scrollToCursor();
    update();
    emit cursorMoved();
}

void LineGrid::scrollToCursor()
{
    int vis = std::max(1, (height() - top_) / lh_);
    if (line_ < first_)
        first_ = line_;
    if (line_ >= first_ + vis)
        first_ = line_ - vis + 1;
    first_ = std::clamp(first_, 0, std::max(0, count() - vis));
}

void LineGrid::insertLine()
{
    begin();
    insertAt(std::min(line_ + 1, count()));
    end();
    move(line_ + 1, stop_);
}

void LineGrid::removeLine()
{
    if (count() <= 1)
        return;
    begin();
    removeAt(line_);
    end();
    move(line_, stop_);
}

void LineGrid::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Bg);
    p.fillRect(0, 0, width(), top_ - 2, Beat);
    for (const auto &h : header_)
        put(p, h.first, 2, h.second, LineGrid::Dim.lighter(150));
    int n = count();
    int vis = (height() - top_) / lh_ + 1;
    for (int i = 0; i < vis; i++) {
        int l = first_ + i;
        if (l >= n)
            break;
        int y = top_ + i * lh_;
        p.fillRect(0, y, width(), lh_, l == line_ ? Cur : l % 4 == 0 ? Beat : Bg);
        if (l == line_ && !stops_.empty()) {
            const Stop &s = stops_[size_t(stop_)];
            if (s.kind != Bar)
                p.fillRect(6 + s.x * cw_ - 1, y, s.w * cw_ + 2, lh_, hasFocus() ? Cursor : Cursor.darker(170));
        }
        bool loop = l == loopLine();
        put(p, 0, y, QString::number(l).rightJustified(numW_, '0'), loop ? Loop : LineGrid::Dim.lighter(140));
        if (loop)
            put(p, numW_, y, "L", Loop);
        if (barX_ >= 0) {
            int v = bar(l);
            for (int k = 0; k < 15; k++)
                p.fillRect(6 + (barX_ + k) * cw_ + 1, y + 2, cw_ - 2, lh_ - 4, k < v ? BarOn : BarOff);
            if (l == line_ && stops_[size_t(stop_)].kind == Bar) {
                p.setPen(hasFocus() ? Cursor : Cursor.darker(170));
                p.drawRect(6 + barX_ * cw_, y, 15 * cw_, lh_ - 1);
            }
        }
        drawLine(p, l, y);
    }
}

/* La casilla k de la barra pone el volumen k + 1; a su izquierda, 0. */
int LineGrid::barValue(double cx) const
{
    return cx < barX_ ? 0 : std::clamp(int(cx - barX_) + 1, 0, 15);
}

int LineGrid::lineAt(int y) const
{
    return first_ + (y < top_ ? 0 : (y - top_) / lh_);
}

int LineGrid::stopAt(int x) const
{
    int cx = (x - 6) / std::max(1, cw_);
    int best = 0;
    for (size_t i = 0; i < stops_.size(); i++)
        if (stops_[i].x <= cx)
            best = int(i);
    return best;
}

void LineGrid::keyPressEvent(QKeyEvent *e)
{
    int vis = std::max(1, (height() - top_) / lh_);
    switch (e->key()) {
    case Qt::Key_Up: move(line_ - 1, stop_); return;
    case Qt::Key_Down: move(line_ + 1, stop_); return;
    case Qt::Key_PageUp: move(line_ - vis, stop_); return;
    case Qt::Key_PageDown: move(line_ + vis, stop_); return;
    case Qt::Key_Home: move(0, stop_); return;
    case Qt::Key_End: move(count() - 1, stop_); return;
    case Qt::Key_Left: move(line_, stop_ - 1); return;
    case Qt::Key_Right: move(line_, stop_ + 1); return;
    case Qt::Key_Insert: insertLine(); return;
    case Qt::Key_Delete: removeLine(); return;
    case Qt::Key_Return:
    case Qt::Key_Enter: emit activated(); return;
    default:
        break;
    }
    if (stops_.empty() || count() == 0 || (e->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
        QWidget::keyPressEvent(e);
        return;
    }
    const Stop &s = stops_[size_t(stop_)];
    QString t = e->text();
    if (t.isEmpty()) {
        QWidget::keyPressEvent(e);
        return;
    }
    QChar c = t[0];
    if (s.kind == Toggle && c == ' ') {
        begin();
        toggle(line_, s.id);
        end();
        move(line_ + 1, stop_);
        return;
    }
    if (s.kind == Bar) {
        int v = hexValue(c);
        if (v < 0 && c != '+' && c != '-') {
            QWidget::keyPressEvent(e);
            return;
        }
        begin();
        setBar(line_, v >= 0 ? v : std::clamp(bar(line_) + (c == '+' ? 1 : -1), 0, 15));
        end();
        if (v >= 0)
            move(line_ + 1, stop_);
        return;
    }
    begin();
    if (!type(line_, s.id, c)) {
        end();
        QWidget::keyPressEvent(e);
        return;
    }
    end();
    /* Una cifra de un número de varias: a la siguiente, y tras la última, a
     * la primera de la línea de abajo. */
    if (s.kind == Digit) {
        if (stop_ + 1 < int(stops_.size()) && stops_[size_t(stop_ + 1)].kind == Digit &&
            stops_[size_t(stop_ + 1)].x == s.x + 1) {
            move(line_, stop_ + 1);
            return;
        }
        int first = stop_;
        while (first > 0 && stops_[size_t(first - 1)].kind == Digit && stops_[size_t(first - 1)].x == stops_[size_t(first)].x - 1)
            first--;
        move(line_ + 1, first);
        return;
    }
    move(line_ + 1, stop_);
}

void LineGrid::mousePressEvent(QMouseEvent *e)
{
    setFocus();
    if (e->button() != Qt::LeftButton)
        return;
    int l = lineAt(int(e->position().y()));
    if (l >= count())
        return;
    double cx = (e->position().x() - 6) / std::max(1, cw_);
    if (barX_ >= 0 && cx >= barX_ - 0.5 && cx < barX_ + 15) {
        begin();
        setBar(l, barValue(cx));
        end();
        dragging_ = true;
        for (size_t i = 0; i < stops_.size(); i++)
            if (stops_[i].kind == Bar)
                stop_ = int(i);
        move(l, stop_);
        return;
    }
    int s = stopAt(int(e->position().x()));
    move(l, s);
    if (stops_[size_t(s)].kind == Toggle) {
        begin();
        toggle(l, stops_[size_t(s)].id);
        end();
    }
}

void LineGrid::mouseMoveEvent(QMouseEvent *e)
{
    if (!dragging_ || !(e->buttons() & Qt::LeftButton))
        return;
    int l = std::clamp(lineAt(int(e->position().y())), 0, count() - 1);
    double cx = (e->position().x() - 6) / std::max(1, cw_);
    setBar(l, barValue(cx));
    end();
    move(l, stop_);
}

void LineGrid::mouseReleaseEvent(QMouseEvent *)
{
    dragging_ = false;
}

void LineGrid::wheelEvent(QWheelEvent *e)
{
    int vis = std::max(1, (height() - top_) / lh_);
    first_ = std::clamp(first_ - e->angleDelta().y() / 40, 0, std::max(0, count() - vis));
    update();
}

/* --- Los samples ---------------------------------------------------------- */

namespace {
enum { S_T, S_N, S_E, S_TSIGN, S_T0, S_T1, S_T2, S_TACC, S_SSIGN, S_S0, S_S1, S_SACC, S_VOL, S_SLIDE, S_BAR };
}

SampleView::SampleView(Editor *ed, QWidget *parent) : LineGrid(parent), ed_(ed)
{
    stops_ = { { 4, 1, Toggle, S_T },  { 6, 1, Toggle, S_N },   { 8, 1, Toggle, S_E },   { 11, 1, Sign, S_TSIGN },
               { 12, 1, Digit, S_T0 }, { 13, 1, Digit, S_T1 },  { 14, 1, Digit, S_T2 },  { 15, 1, Toggle, S_TACC },
               { 18, 1, Sign, S_SSIGN }, { 19, 1, Digit, S_S0 }, { 20, 1, Digit, S_S1 }, { 21, 1, Toggle, S_SACC },
               { 24, 1, Value, S_VOL }, { 25, 1, Sign, S_SLIDE }, { 27, 15, Bar, S_BAR } };
    barX_ = 27;
    header_ = { { 4, "T" }, { 6, "N" }, { 8, "E" }, { 11, tr("Tono") }, { 18, tr("R/E") }, { 24, tr("Vol") } };
    setToolTip(tr("T, N, E: tono, ruido y envolvente (espacio o clic). Tono: su desplazamiento (^ lo acumula). "
                  "R/E: el del ruido o, sin ruido, el de la envolvente. Vol: el volumen (+ o - lo desliza). "
                  "Insert y Supr añaden y quitan líneas."));
    connect(ed_, &Editor::songChanged, this, [this] { setLine(line()); });
    connect(ed_, &Editor::sampleChanged, this, [this] { setLine(line()); });
}

int SampleView::count() const
{
    return int(ed_->mod().samples[size_t(ed_->sample())].lines.size());
}

int SampleView::loopLine() const
{
    const Sample &s = ed_->mod().samples[size_t(ed_->sample())];
    return s.lines.empty() ? -1 : s.loop;
}

void SampleView::drawLine(QPainter &p, int l, int y)
{
    const SampleLine &s = ed_->mod().samples[size_t(ed_->sample())].lines[size_t(l)];
    put(p, 4, y, s.toneOn() ? "T" : "t", s.toneOn() ? On : Dim);
    put(p, 6, y, s.noiseOn() ? "N" : "n", s.noiseOn() ? On : Dim);
    put(p, 8, y, s.envOn() ? "E" : "e", s.envOn() ? On : Dim);
    int t = s.tone;
    put(p, 11, y, (t < 0 ? "-" : "+") + hex(std::abs(t) & 0xFFF, 3), t ? Text : Dim);
    put(p, 15, y, s.toneAcc() ? "^" : ".", s.toneAcc() ? Hi : Dim);
    int sh = s.shift();
    put(p, 18, y, (sh < 0 ? "-" : "+") + hex(std::abs(sh), 2), sh ? Text : Dim);
    put(p, 21, y, s.shiftAcc() ? "^" : ".", s.shiftAcc() ? Hi : Dim);
    put(p, 24, y, hex(s.volume(), 1), Text);
    put(p, 25, y, s.volSlide() > 0 ? "+" : s.volSlide() < 0 ? "-" : ".", s.volSlide() ? Hi : Dim);
}

bool SampleView::type(int l, int id, QChar c)
{
    SampleLine &s = ed_->mod().samples[size_t(ed_->sample())].lines[size_t(l)];
    int v = hexValue(c);
    switch (id) {
    case S_TSIGN:
        if (c != '+' && c != '-' && c != ' ')
            return false;
        if ((c == '-') != (s.tone < 0) || c == ' ')
            s.tone = int16_t(-s.tone);
        return true;
    case S_T0:
    case S_T1:
    case S_T2: {
        if (v < 0)
            return false;
        int a = std::abs(int(s.tone)) & 0xFFF, sh = (2 - (id - S_T0)) * 4;
        a = (a & ~(0xF << sh)) | v << sh;
        s.tone = int16_t(s.tone < 0 ? -a : a);
        return true;
    }
    case S_SSIGN:
        if (c != '+' && c != '-' && c != ' ')
            return false;
        if ((c == '-') != (s.shift() < 0) || c == ' ')
            s.setShift(s.shift() < 0 ? std::min(-s.shift(), 15) : -s.shift());
        return true;
    case S_S0:
    case S_S1: {
        if (v < 0 || (id == S_S0 && v > 1))
            return false;
        int a = std::abs(s.shift());
        a = id == S_S0 ? (v << 4 | (a & 15)) : ((a & 0x10) | v);
        s.setShift(s.shift() < 0 ? -std::min(a, 16) : std::min(a, 15));
        return true;
    }
    case S_VOL:
        if (v < 0)
            return false;
        s.setVolume(v);
        return true;
    case S_SLIDE:
        if (c == '+')
            s.setVolSlide(1);
        else if (c == '-')
            s.setVolSlide(-1);
        else if (c == ' ' || c == '.' || c == '0')
            s.setVolSlide(s.volSlide() == 0 ? 1 : s.volSlide() > 0 ? -1 : 0);
        else
            return false;
        return true;
    case S_TACC:
    case S_SACC:
        if (c != '^')
            return false;
        toggle(l, id);
        return true;
    }
    return false;
}

void SampleView::toggle(int l, int id)
{
    SampleLine &s = ed_->mod().samples[size_t(ed_->sample())].lines[size_t(l)];
    switch (id) {
    case S_T: s.setToneOn(!s.toneOn()); break;
    case S_N: s.setNoiseOn(!s.noiseOn()); break;
    case S_E: s.setEnvOn(!s.envOn()); break;
    case S_TACC: s.setToneAcc(!s.toneAcc()); break;
    case S_SACC: s.setShiftAcc(!s.shiftAcc()); break;
    }
}

int SampleView::bar(int l) const
{
    return ed_->mod().samples[size_t(ed_->sample())].lines[size_t(l)].volume();
}

void SampleView::setBar(int l, int v)
{
    ed_->mod().samples[size_t(ed_->sample())].lines[size_t(l)].setVolume(v);
}

void SampleView::begin()
{
    ed_->begin(Editor::SamplePart, ed_->sample());
}

void SampleView::end()
{
    ed_->changed(Editor::SamplePart);
}

void SampleView::insertAt(int l)
{
    Sample &s = ed_->mod().samples[size_t(ed_->sample())];
    if (int(s.lines.size()) >= MAX_LINES)
        return;
    SampleLine copy = s.lines.empty() ? defaultSample().lines[0] : s.lines[size_t(std::max(0, l - 1))];
    s.lines.insert(s.lines.begin() + l, copy);
    if (s.loop >= l && l > 0 && s.loop + 1 < int(s.lines.size()))
        s.loop++;
}

void SampleView::removeAt(int l)
{
    Sample &s = ed_->mod().samples[size_t(ed_->sample())];
    s.lines.erase(s.lines.begin() + l);
    if (s.loop > l || s.loop >= int(s.lines.size()))
        s.loop = std::max(0, s.loop - 1);
}

/* --- Los ornamentos ------------------------------------------------------- */

namespace {
enum { O_SIGN, O_D0, O_D1 };
const int GRAPH_X = 10, GRAPH_W = 25;   /* de -12 a +12 */
}

OrnamentView::OrnamentView(Editor *ed, QWidget *parent) : LineGrid(parent), ed_(ed)
{
    stops_ = { { 4, 1, Sign, O_SIGN }, { 5, 1, Digit, O_D0 }, { 6, 1, Digit, O_D1 } };
    header_ = { { 4, tr("Semitonos") } };
    setToolTip(tr("Lo que se suma a la nota en cada paso, en semitonos (+ o - cambia el signo). "
                  "Insert y Supr añaden y quitan líneas."));
    connect(ed_, &Editor::songChanged, this, [this] { setLine(line()); });
    connect(ed_, &Editor::ornamentChanged, this, [this] { setLine(line()); });
}

int OrnamentView::count() const
{
    return int(ed_->mod().ornaments[size_t(ed_->ornament())].offsets.size());
}

int OrnamentView::loopLine() const
{
    const Ornament &o = ed_->mod().ornaments[size_t(ed_->ornament())];
    return o.offsets.empty() ? -1 : o.loop;
}

void OrnamentView::drawLine(QPainter &p, int l, int y)
{
    int v = ed_->mod().ornaments[size_t(ed_->ornament())].offsets[size_t(l)];
    put(p, 4, y, (v < 0 ? "-" : "+") + QString::number(std::abs(v)).rightJustified(2, '0'), v ? Text : Dim);
    int mid = 6 + (GRAPH_X + GRAPH_W / 2) * cw_ + cw_ / 2;
    p.fillRect(mid, y, 1, lh_, Dim);
    int c = std::clamp(v, -12, 12);
    int x0 = std::min(mid, mid + c * cw_), w = std::abs(c * cw_);
    p.fillRect(x0, y + 3, std::max(w, 2), lh_ - 6, QColor(0x5f, 0xa8, 0xe8));
    if (v != c)
        put(p, c > 0 ? GRAPH_X + GRAPH_W + 1 : GRAPH_X - 2, y, c > 0 ? ">" : "<", Hi);
}

bool OrnamentView::type(int l, int id, QChar c)
{
    int &v = ed_->mod().ornaments[size_t(ed_->ornament())].offsets[size_t(l)];
    if (c == '+' || c == '-' || (id == O_SIGN && c == ' ')) {
        if (c == ' ' || (c == '-') != (v < 0))
            v = -v;
        return true;
    }
    if (!c.isDigit() || id == O_SIGN)
        return false;
    int a = std::abs(v), d = c.unicode() - '0';
    a = id == O_D0 ? d * 10 + a % 10 : a / 10 * 10 + d;
    a = std::min(a, 95);
    v = v < 0 ? -a : a;
    return true;
}

void OrnamentView::toggle(int, int) {}

int OrnamentView::bar(int) const
{
    return 0;
}

void OrnamentView::setBar(int, int) {}

void OrnamentView::begin()
{
    ed_->begin(Editor::OrnamentPart, ed_->ornament());
}

void OrnamentView::end()
{
    ed_->changed(Editor::OrnamentPart);
}

void OrnamentView::insertAt(int l)
{
    Ornament &o = ed_->mod().ornaments[size_t(ed_->ornament())];
    if (int(o.offsets.size()) >= MAX_LINES)
        return;
    o.offsets.insert(o.offsets.begin() + l, o.offsets.empty() ? 0 : o.offsets[size_t(std::max(0, l - 1))]);
}

void OrnamentView::removeAt(int l)
{
    Ornament &o = ed_->mod().ornaments[size_t(ed_->ornament())];
    o.offsets.erase(o.offsets.begin() + l);
    if (o.loop > l || o.loop >= int(o.offsets.size()))
        o.loop = std::max(0, o.loop - 1);
}

/* --- Los efectos AYFX ----------------------------------------------------- */

namespace {
enum { F_T, F_N, F_T0, F_T1, F_T2, F_N0, F_N1, F_VOL, F_BAR };
}

SfxView::SfxView(Editor *ed, QWidget *parent) : LineGrid(parent), ed_(ed)
{
    numW_ = 3;
    stops_ = { { 5, 1, Toggle, F_T },  { 7, 1, Toggle, F_N },  { 10, 1, Digit, F_T0 }, { 11, 1, Digit, F_T1 },
               { 12, 1, Digit, F_T2 }, { 15, 1, Digit, F_N0 }, { 16, 1, Digit, F_N1 }, { 19, 1, Value, F_VOL },
               { 21, 15, Bar, F_BAR } };
    barX_ = 21;
    header_ = { { 5, "T" }, { 7, "N" }, { 10, tr("Tono") }, { 15, tr("Ru") }, { 19, tr("Vol") } };
    setToolTip(tr("Un frame por paso de 1/50 s. T y N: tono y ruido (espacio o clic); sus periodos; "
                  "el volumen. Insert y Supr añaden y quitan frames."));
    connect(ed_, &Editor::bankChanged, this, [this] { setLine(line()); });
    connect(ed_, &Editor::effectChanged, this, [this] { setLine(line()); });
}

int SfxView::count() const
{
    const fx::Bank &b = ed_->bank();
    return b.effects.empty() ? 0 : int(b.effects[size_t(ed_->effect())].frames.size());
}

void SfxView::drawLine(QPainter &p, int l, int y)
{
    const fx::Frame &f = ed_->bank().effects[size_t(ed_->effect())].frames[size_t(l)];
    put(p, 5, y, f.toneOn ? "T" : "t", f.toneOn ? On : Dim);
    put(p, 7, y, f.noiseOn ? "N" : "n", f.noiseOn ? On : Dim);
    put(p, 10, y, hex(f.tone, 3), f.toneOn ? Text : Dim);
    put(p, 15, y, hex(f.noise, 2), f.noiseOn ? Text : Dim);
    put(p, 19, y, hex(f.volume, 1), Text);
}

bool SfxView::type(int l, int id, QChar c)
{
    fx::Frame &f = ed_->bank().effects[size_t(ed_->effect())].frames[size_t(l)];
    int v = hexValue(c);
    if (v < 0)
        return false;
    switch (id) {
    case F_T0:
    case F_T1:
    case F_T2: {
        int sh = (2 - (id - F_T0)) * 4;
        f.tone = (f.tone & ~(0xF << sh)) | v << sh;
        return true;
    }
    case F_N0:
        if (v > 1)
            return false;
        f.noise = (v << 4) | (f.noise & 15);
        return true;
    case F_N1:
        f.noise = (f.noise & 0x10) | v;
        return true;
    case F_VOL:
        f.volume = v;
        return true;
    }
    return false;
}

void SfxView::toggle(int l, int id)
{
    fx::Frame &f = ed_->bank().effects[size_t(ed_->effect())].frames[size_t(l)];
    if (id == F_T)
        f.toneOn = !f.toneOn;
    else if (id == F_N)
        f.noiseOn = !f.noiseOn;
}

int SfxView::bar(int l) const
{
    return ed_->bank().effects[size_t(ed_->effect())].frames[size_t(l)].volume;
}

void SfxView::setBar(int l, int v)
{
    ed_->bank().effects[size_t(ed_->effect())].frames[size_t(l)].volume = v;
}

void SfxView::begin()
{
    ed_->beginBank();
}

void SfxView::end()
{
    ed_->endBank();
}

void SfxView::insertAt(int l)
{
    std::vector<fx::Frame> &v = ed_->bank().effects[size_t(ed_->effect())].frames;
    v.insert(v.begin() + l, v.empty() ? fx::Frame() : v[size_t(std::max(0, l - 1))]);
}

void SfxView::removeAt(int l)
{
    std::vector<fx::Frame> &v = ed_->bank().effects[size_t(ed_->effect())].frames;
    v.erase(v.begin() + l);
}
