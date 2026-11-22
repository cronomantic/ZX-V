/*
 * patternview.cpp - Los patrones de una posición: pintarlos, moverse,
 * escribir notas y valores, seleccionar, copiar y pegar.
 */
#include "patternview.h"

#include <QFontDatabase>
#include <QKeyEvent>
#include <QPainter>

#include <algorithm>

#include "editor.h"
#include "piano.h"

using namespace trk;

/* Dónde va cada cosa, en caracteres: la fila a la izquierda, fija; los
 * grupos, uno detrás de otro. Un AY: el periodo de la envolvente, el ruido y
 * los tres canales; una pista de muestras: nota, instrumento, volumen y dos
 * efectos. */
static const int NUM_W = 4, MX = 6;
static const int AY_ENV = 0, AY_NOISE = 5, AY_CH = 8, CH_W = 15, AY_W = AY_CH + 3 * CH_W;
static const int S_INS = 4, S_VOL = 7, S_FX = 10, S_FX_W = 4, SMP_W = 17;
static const int GAP = 2;
static const char SAMPLE_DIGITS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

/* Lo copiado: filas de varias pistas, cada una de su clase. */
namespace {

enum ClipKind { CLIP_ENV, CLIP_CH, CLIP_SMP };

struct ClipTrack {
    ClipKind kind;
    std::vector<Row> ay;                /* la env y el ruido, o el canal en ch[0] (con su periodo) */
    std::vector<SmpCell> smp;
};

struct {
    std::vector<ClipTrack> tracks;
    int rows = 0;
} clip;

struct Colors {
    QColor bg{ 0x1d, 0x1f, 0x26 }, beat{ 0x25, 0x28, 0x32 }, bar{ 0x2d, 0x31, 0x40 }, line{ 0x33, 0x3a, 0x58 };
    QColor play{ 0x55, 0x30, 0x30 }, sel{ 0x2f, 0x52, 0x3a }, cursor{ 0x6f, 0x7f, 0xd8 };
    QColor dim{ 0x55, 0x5a, 0x68 }, num{ 0x80, 0x86, 0x96 }, numBeat{ 0xc8, 0xcc, 0xd8 };
    QColor note{ 0xee, 0xee, 0xee }, off{ 0xe0, 0x9a, 0x60 }, smp{ 0xe6, 0xcf, 0x6a };
    QColor env{ 0x7c, 0xc4, 0xd8 }, vol{ 0x9e, 0xd8, 0x7a }, fx{ 0xd8, 0x8c, 0xd8 }, sep{ 0x3c, 0x40, 0x4c };
    QColor muted{ 0xd0, 0x50, 0x50 };
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

QString hex(int v, int digits) { return QString::number(v, 16).toUpper().rightJustified(digits, '0'); }

/* Las notas de las pistas de muestras, como en el XM: C-0 a B-7. */
QString smpNoteName(int note)
{
    if (note == NOTE_OFF)
        return QStringLiteral("===");
    if (note == NOTE_CUT)
        return QStringLiteral("^^^");
    if (note < 0 || note >= NOTES)
        return QStringLiteral("---");
    static const char *names[12] = { "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" };
    return QString::fromLatin1(names[note % 12]) + QChar('0' + note / 12);
}

const Row EMPTY_ROW;
const SmpCell EMPTY_CELL;

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
    layout();
    connect(ed_, &Editor::songChanged, this, [this] {
        layout();
        row_ = std::clamp(row_, 0, rows() - 1);
        update();
    });
    connect(ed_, &Editor::positionChanged, this, [this] {
        layout();
        row_ = std::clamp(row_, 0, rows() - 1);
        selecting_ = false;
        update();
    });
}

/* Las columnas de los grupos que tiene la canción. Si cambian, el cursor
 * se queda en la misma pista si sigue ahí. */
void PatternView::layout()
{
    int oldGroup = stops_.empty() ? 0 : stops_[size_t(stop_)].group;
    int oldKind = stops_.empty() ? Note : stops_[size_t(stop_)].kind;
    int oldCh = stops_.empty() ? 0 : stops_[size_t(stop_)].ch;
    int oldDigit = stops_.empty() ? 0 : stops_[size_t(stop_)].digit;
    stops_.clear();
    tracks_.clear();
    int x = 0;
    const Tune &t = ed_->tune();
    for (int c = 0; c < t.chips; c++) {
        int tr = int(tracks_.size());
        tracks_.push_back({ c, -1, x + AY_ENV, x + AY_NOISE + 2, -1 });
        for (int d = 0; d < 4; d++)
            stops_.push_back({ x + AY_ENV + d, Env, c, -1, tr, d });
        for (int d = 0; d < 2; d++)
            stops_.push_back({ x + AY_NOISE + d, Noise, c, -1, tr, d });
        for (int ch = 0; ch < 3; ch++) {
            int b = x + AY_CH + ch * CH_W;
            tr = int(tracks_.size());
            tracks_.push_back({ c, ch, b, b + 13, c * 3 + ch });
            stops_.push_back({ b, Note, c, ch, tr, 0 });
            stops_.push_back({ b + 4, Sample, c, ch, tr, 0 });
            stops_.push_back({ b + 5, EnvType, c, ch, tr, 0 });
            stops_.push_back({ b + 6, Orn, c, ch, tr, 0 });
            stops_.push_back({ b + 7, Vol, c, ch, tr, 0 });
            stops_.push_back({ b + 9, Fx, c, ch, tr, 0 });
            stops_.push_back({ b + 10, Delay, c, ch, tr, 0 });
            stops_.push_back({ b + 11, Param, c, ch, tr, 0 });
            stops_.push_back({ b + 12, Param, c, ch, tr, 1 });
        }
        x += AY_W + GAP;
    }
    for (int k = 0; k < t.smpTracks; k++) {
        int g = Editor::SMP_GROUP + k, tr = int(tracks_.size());
        tracks_.push_back({ g, -1, x, x + SMP_W, 6 + k });
        stops_.push_back({ x, SNote, g, -1, tr, 0 });
        for (int d = 0; d < 2; d++)
            stops_.push_back({ x + S_INS + d, SIns, g, -1, tr, d });
        for (int d = 0; d < 2; d++)
            stops_.push_back({ x + S_VOL + d, SVol, g, -1, tr, d });
        for (int f = 0; f < 2; f++) {
            int b = x + S_FX + f * S_FX_W;
            stops_.push_back({ b, SFx, g, -1, tr, f });
            stops_.push_back({ b + 1, SParam, g, -1, tr, f * 2 });
            stops_.push_back({ b + 2, SParam, g, -1, tr, f * 2 + 1 });
        }
        x += SMP_W + GAP;
    }
    width_ = x;
    /* El cursor, donde estaba (o lo más cerca). */
    stop_ = 0;
    for (size_t i = 0; i < stops_.size(); i++) {
        const Stop &s = stops_[i];
        if (s.group == oldGroup && s.ch == oldCh && s.kind == oldKind && s.digit == oldDigit) {
            stop_ = int(i);
            break;
        }
        if (s.group == oldGroup && (s.kind == Note || s.kind == SNote) && (s.ch == oldCh || s.ch < 0) && !stop_)
            stop_ = int(i);
    }
    anchorTrack_ = std::min(anchorTrack_, std::max(0, int(tracks_.size()) - 1));
    scrollToCursor();
}

QSize PatternView::sizeHint() const
{
    return QSize((NUM_W + std::min(width_, AY_W + GAP + 2 * (SMP_W + GAP))) * cw_ + 2 * MX, lh_ * 33 + top_);
}

int PatternView::rows() const
{
    return std::clamp(ed_->rows(), 1, MAX_ROWS);
}

int PatternView::group() const
{
    return stops_.empty() ? 0 : stops_[size_t(stop_)].group;
}

int PatternView::channel() const
{
    return stops_.empty() ? -1 : stops_[size_t(stop_)].ch;
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

void PatternView::setMuted(int mask)
{
    if (mask != muted_) {
        muted_ = mask;
        update();
        emit mutedChanged(muted_);
    }
}

int PatternView::contentX() const
{
    return MX + NUM_W * cw_;
}

void PatternView::scrollToCursor()
{
    if (stops_.empty()) {
        scroll_ = 0;
        return;
    }
    int visible = std::max(1, (width() - contentX() - MX) / std::max(1, cw_));
    const Track &t = tracks_[size_t(stops_[size_t(stop_)].track)];
    if (t.x0 < scroll_)
        scroll_ = t.x0;
    else if (t.x1 > scroll_ + visible)
        scroll_ = std::max(0, std::min(t.x0, t.x1 - visible));
    scroll_ = std::clamp(scroll_, 0, std::max(0, width_ - visible));
    if (t.x0 < scroll_)
        scroll_ = t.x0;
}

void PatternView::moveTo(int row, int stop, bool select)
{
    if (stops_.empty())
        return;
    if (select && !selecting_) {
        anchorRow_ = row_;
        anchorTrack_ = stops_[size_t(stop_)].track;
        selecting_ = true;
    } else if (!select) {
        selecting_ = false;
    }
    row_ = std::clamp(row, 0, rows() - 1);
    stop_ = std::clamp(stop, 0, int(stops_.size()) - 1);
    scrollToCursor();
    update();
    emit cursorMoved();
}

QRect PatternView::selection() const
{
    int t = stops_.empty() ? 0 : stops_[size_t(stop_)].track;
    if (!selecting_)
        return QRect(t, row_, 1, 1);
    int t0 = std::min(t, anchorTrack_), t1 = std::max(t, anchorTrack_);
    int r0 = std::min(row_, anchorRow_), r1 = std::max(row_, anchorRow_);
    return QRect(t0, r0, t1 - t0 + 1, r1 - r0 + 1);
}

int PatternView::firstStop(int track) const
{
    for (size_t i = 0; i < stops_.size(); i++)
        if (stops_[i].track == track)
            return int(i);
    return 0;
}

/* --- Pintar ----------------------------------------------------------------- */

void PatternView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), C.bg);
    QFontMetrics fm(font());
    int asc = fm.ascent();
    if (stops_.empty()) {
        p.setPen(C.num);
        p.drawText(rect().adjusted(MX, top_, -MX, 0), Qt::AlignTop | Qt::TextWordWrap,
                   tr("La canción no tiene pistas: ponle un AY o pistas de muestras."));
        return;
    }
    const Tune &t = ed_->tune();
    int n = rows();
    int lines = (height() - top_) / lh_ + 1;
    int center = (height() - top_) / lh_ / 2;
    int cx = contentX();
    auto colX = [&](int x) { return cx + (x - scroll_) * cw_; };
    auto put = [&](int x, int y, const QString &s, const QColor &c) {
        p.setPen(c);
        p.drawText(colX(x), y + asc, s);
    };

    /* La cabecera: el nombre de cada pista (en rojo, si está callada). */
    p.fillRect(0, 0, width(), top_ - 2, C.beat);
    p.setClipRect(cx, 0, width() - cx, height());
    for (const Track &tk : tracks_) {
        bool off = tk.mute >= 0 && (muted_ >> tk.mute & 1);
        QColor col = off ? C.muted : C.numBeat;
        if (tk.group < Editor::SMP_GROUP && tk.ch < 0) {
            put(tk.x0, 2, tr("Env."), C.num);
            put(tk.x0 + AY_NOISE, 2, tr("Ru"), C.num);
        } else if (tk.group < Editor::SMP_GROUP) {
            QString name = QString(QChar('A' + tk.ch));
            if (t.chips > 1)
                name = tr("AY%1 %2").arg(tk.group + 1).arg(name);
            if (off)
                name += QStringLiteral(" ×");
            put(tk.x0 + 5 - int(name.size()) / 2, 2, name, col);
        } else {
            QString name = tr("M%1").arg(tk.group - Editor::SMP_GROUP + 1);
            if (off)
                name += QStringLiteral(" ×");
            put(tk.x0 + SMP_W / 2 - int(name.size()) / 2, 2, name, col);
        }
    }
    p.setClipping(false);

    QRect sel = selection();
    int numW = n > 100 ? 3 : 2;
    const Pattern *ay[2] = { ed_->ayPattern(0), ed_->ayPattern(1) };
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
        p.setPen(r % highlight_ == 0 ? C.numBeat : C.num);
        p.drawText(MX, y + asc, QString::number(r).rightJustified(numW, '0'));

        p.setClipRect(cx, y, width() - cx, lh_);
        if (selecting_ && r >= sel.top() && r <= sel.bottom()) {
            int x0 = tracks_[size_t(sel.left())].x0, x1 = tracks_[size_t(sel.right())].x1;
            p.fillRect(colX(x0) - 2, y, (x1 - x0) * cw_ + 4, lh_, C.sel);
        }
        if (r == row_) {
            const Stop &s = stops_[size_t(stop_)];
            int w = s.kind == Note || s.kind == SNote ? 3 : 1;
            QColor cc = hasFocus() ? C.cursor : C.cursor.darker(170);
            p.fillRect(colX(s.x) - 1, y, w * cw_ + 2, lh_, cc);
        }
        for (const Track &tk : tracks_) {
            if (tk.group < Editor::SMP_GROUP) {
                const Pattern *pat = ay[tk.group];
                const Row &row = pat && r < int(pat->rows.size()) ? pat->rows[size_t(r)] : EMPTY_ROW;
                QColor none = pat ? C.dim : C.dim.darker(140);
                if (tk.ch < 0) {
                    put(tk.x0 + AY_ENV, y, row.envPeriod < 0 ? "...." : hex(row.envPeriod, 4),
                        row.envPeriod < 0 ? none : C.env);
                    put(tk.x0 + AY_NOISE, y, row.noise < 0 ? ".." : hex(row.noise, 2), row.noise < 0 ? none : C.env);
                    continue;
                }
                const Cell &cell = row.ch[tk.ch];
                int b = tk.x0;
                QString note = QString::fromStdString(noteName(cell.note));
                put(b, y, note, cell.note == NOTE_NONE ? none : cell.note == NOTE_OFF ? C.off : C.note);
                put(b + 4, y, cell.sample ? QString(QChar(SAMPLE_DIGITS[cell.sample & 31])) : ".",
                    cell.sample ? C.smp : none);
                put(b + 5, y, cell.env ? hex(cell.env, 1) : ".", cell.env ? C.env : none);
                put(b + 6, y, cell.orn >= 0 ? hex(cell.orn, 1) : ".", cell.orn >= 0 ? C.env : none);
                put(b + 7, y, cell.vol ? hex(cell.vol, 1) : ".", cell.vol ? C.vol : none);
                if (cell.fx) {
                    put(b + 9, y, hex(cell.fx, 1), C.fx);
                    put(b + 10, y, hex(cell.fxDelay & 15, 1), C.fx);
                    put(b + 11, y, hex(cell.fxParam & 0xFF, 2), C.fx);
                } else {
                    put(b + 9, y, "....", none);
                }
            } else {
                const SmpPattern *pat = ed_->smpPattern(tk.group - Editor::SMP_GROUP);
                const SmpCell &c = pat && r < int(pat->rows.size()) ? pat->rows[size_t(r)] : EMPTY_CELL;
                QColor none = pat ? C.dim : C.dim.darker(140);
                int b = tk.x0;
                put(b, y, smpNoteName(c.note),
                    c.note == NOTE_NONE ? none : c.note < 0 ? C.off : C.note);
                put(b + S_INS, y, c.ins ? hex(c.ins, 2) : "..", c.ins ? C.smp : none);
                put(b + S_VOL, y, c.vol >= 0 ? hex(c.vol, 2) : "..", c.vol >= 0 ? C.vol : none);
                for (int f = 0; f < 2; f++) {
                    int fx = b + S_FX + f * S_FX_W;
                    if (c.fx[size_t(f)]) {
                        put(fx, y, QString(QChar(c.fx[size_t(f)])), C.fx);
                        put(fx + 1, y, hex(c.fxp[size_t(f)], 2), C.fx);
                    } else {
                        put(fx, y, "...", none);
                    }
                }
            }
        }
        p.setClipping(false);
    }

    /* Las rayas entre las pistas. */
    p.setClipRect(cx, 0, width() - cx, height());
    p.setPen(C.sep);
    for (const Track &tk : tracks_) {
        int x = colX(tk.x0) - cw_ / 2 - (tk.ch < 0 && tk.group < Editor::SMP_GROUP ? 0 : 0);
        p.drawLine(x, 0, x, height());
    }
}

/* --- Teclado ------------------------------------------------------------------ */

void PatternView::advance(bool multi)
{
    const Stop &s = stops_[size_t(stop_)];
    if (multi && stop_ + 1 < int(stops_.size()) && stops_[size_t(stop_ + 1)].kind == s.kind &&
        stops_[size_t(stop_ + 1)].track == s.track &&
        (s.kind != SParam || stops_[size_t(stop_ + 1)].digit / 2 == s.digit / 2)) {
        stop_++;
        update();
        return;
    }
    if (multi)
        stop_ -= s.kind == SParam ? s.digit % 2 : s.digit;
    moveTo(std::min(row_ + step_, rows() - 1), stop_, false);
}

bool PatternView::enter(QKeyEvent *e)
{
    if (stops_.empty() || row_ >= rows())
        return false;
    const Stop &s = stops_[size_t(stop_)];
    return s.group < Editor::SMP_GROUP ? enterAy(e, s) : enterSmp(e, s);
}

bool PatternView::enterAy(QKeyEvent *e, const Stop &s)
{
    int chip = s.group;
    if (s.kind == Note) {
        int k = pianoKey(e->key());
        bool off = e->key() == Qt::Key_1 || e->key() == Qt::Key_A;
        if (k < 0 && !off)
            return false;
        if (e->isAutoRepeat())
            return true;
        ed_->beginGroup(chip);
        Row &row = ed_->ayEdit(chip).rows[size_t(row_)];
        Cell &c = row.ch[s.ch];
        if (off) {
            c.note = NOTE_OFF;
        } else {
            c.note = std::clamp((octave_ - 1) * 12 + k, 0, NOTES - 1);
            c.sample = ed_->sample();
            previewKey_ = e->key();
            emit preview(c, row.envPeriod, chip, s.ch);
        }
        ed_->changed(Editor::AyPatternPart);
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

    ed_->beginGroup(chip);
    Row &row = ed_->ayEdit(chip).rows[size_t(row_)];
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
    ed_->changed(Editor::AyPatternPart);
    advance(multi);
    return true;
}

bool PatternView::enterSmp(QKeyEvent *e, const Stop &s)
{
    int track = s.group - Editor::SMP_GROUP;
    if (s.kind == SNote) {
        int k = pianoKey(e->key());
        bool off = e->key() == Qt::Key_1 || e->key() == Qt::Key_A;
        if (k < 0 && !off)
            return false;
        if (e->isAutoRepeat())
            return true;
        ed_->beginGroup(s.group);
        SmpCell &c = ed_->smpEdit(track).rows[size_t(row_)];
        if (off) {
            c.note = NOTE_OFF;
        } else {
            c.note = std::clamp(octave_ * 12 + k, 0, NOTES - 1);
            c.ins = ed_->instrument();
            previewKey_ = e->key();
            emit previewSample(c, track);
        }
        ed_->changed(Editor::SmpPatternPart);
        advance(false);
        return true;
    }
    QString t = e->text().toUpper();
    if (t.isEmpty())
        return false;
    QChar ch = t[0];
    int v = hexValue(ch);
    if (s.kind == SFx) {
        /* El efecto: su letra, como en el MOD, el S3M y el XM (0-9, A-Z). */
        if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z')))
            return false;
    } else if (v < 0) {
        return false;
    }
    ed_->beginGroup(s.group);
    SmpCell &c = ed_->smpEdit(track).rows[size_t(row_)];
    bool multi = false;
    switch (s.kind) {
    case SIns: {
        int val = s.digit == 0 ? (v << 4 | (c.ins & 15)) : ((c.ins & 0xF0) | v);
        c.ins = std::min(val, 128);
        multi = true;
        break;
    }
    case SVol: {
        int old = c.vol < 0 ? 0 : c.vol;
        int val = s.digit == 0 ? (v << 4 | (old & 15)) : ((old & 0xF0) | v);
        c.vol = std::min(val, 64);
        multi = true;
        break;
    }
    case SFx:
        c.fx[size_t(s.digit)] = uint8_t(ch.unicode());
        break;
    case SParam: {
        int f = s.digit / 2, val = c.fxp[size_t(f)];
        c.fxp[size_t(f)] = uint8_t(s.digit % 2 == 0 ? (v << 4 | (val & 15)) : ((val & 0xF0) | v));
        multi = true;
        break;
    }
    default:
        break;
    }
    ed_->changed(Editor::SmpPatternPart);
    advance(multi);
    return true;
}

void PatternView::clearField()
{
    if (stops_.empty())
        return;
    const Stop &s = stops_[size_t(stop_)];
    ed_->beginGroup(s.group);
    if (s.group < Editor::SMP_GROUP) {
        Row &row = ed_->ayEdit(s.group).rows[size_t(row_)];
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
        default: break;
        }
        ed_->changed(Editor::AyPatternPart);
    } else {
        SmpCell &c = ed_->smpEdit(s.group - Editor::SMP_GROUP).rows[size_t(row_)];
        switch (s.kind) {
        case SNote: c.note = NOTE_NONE; c.ins = 0; break;
        case SIns: c.ins = 0; break;
        case SVol: c.vol = -1; break;
        case SFx:
        case SParam: {
            int f = s.kind == SFx ? s.digit : s.digit / 2;
            c.fx[size_t(f)] = 0;
            c.fxp[size_t(f)] = 0;
            break;
        }
        default: break;
        }
        ed_->changed(Editor::SmpPatternPart);
    }
    int back = s.kind == Param || s.kind == Env || s.kind == Noise || s.kind == SIns || s.kind == SVol ? s.digit
             : s.kind == SParam                                                                     ? s.digit % 2
                                                                                                    : 0;
    moveTo(std::min(row_ + step_, rows() - 1), stop_ - back, false);
}

void PatternView::keyPressEvent(QKeyEvent *e)
{
    bool shift = e->modifiers() & Qt::ShiftModifier;
    bool ctrl = e->modifiers() & Qt::ControlModifier;
    int track = stops_.empty() ? 0 : stops_[size_t(stop_)].track;
    switch (e->key()) {
    case Qt::Key_Up: moveTo(row_ - 1, stop_, shift); return;
    case Qt::Key_Down: moveTo(row_ + 1, stop_, shift); return;
    case Qt::Key_PageUp: moveTo(row_ - 16, stop_, shift); return;
    case Qt::Key_PageDown: moveTo(row_ + 16, stop_, shift); return;
    case Qt::Key_Home: moveTo(0, ctrl ? 0 : stop_, shift); return;
    case Qt::Key_End: moveTo(rows() - 1, stop_, shift); return;
    case Qt::Key_Left: moveTo(row_, stop_ - 1, shift); return;
    case Qt::Key_Right: moveTo(row_, stop_ + 1, shift); return;
    case Qt::Key_Tab:
        moveTo(row_, firstStop(track + 1 < int(tracks_.size()) ? track + 1 : 0), false);
        return;
    case Qt::Key_Backtab:
        moveTo(row_, firstStop(track > 0 ? track - 1 : int(tracks_.size()) - 1), false);
        return;
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
    int cx = (x - contentX()) / std::max(1, cw_) + scroll_;
    int best = 0;
    for (size_t i = 0; i < stops_.size(); i++)
        if (stops_[i].x <= cx)
            best = int(i);
    return best;
}

void PatternView::mousePressEvent(QMouseEvent *e)
{
    setFocus();
    if (e->button() != Qt::LeftButton || stops_.empty())
        return;
    int x = int(e->position().x()), y = int(e->position().y());
    if (y < top_) {
        /* La cabecera: callar (o no) la pista. */
        int cx = (x - contentX()) / std::max(1, cw_) + scroll_;
        for (const Track &t : tracks_)
            if (t.mute >= 0 && cx >= t.x0 && cx < t.x1)
                setMuted(muted_ ^ (1 << t.mute));
        return;
    }
    moveTo(rowAt(y), stopAt(x), e->modifiers() & Qt::ShiftModifier);
}

void PatternView::mouseMoveEvent(QMouseEvent *e)
{
    if (!(e->buttons() & Qt::LeftButton) || stops_.empty() || e->position().y() < top_)
        return;
    int r = rowAt(int(e->position().y()));
    int s = stopAt(int(e->position().x()));
    if (r != row_ || stops_[size_t(s)].track != stops_[size_t(stop_)].track)
        moveTo(r, s, true);
}

void PatternView::wheelEvent(QWheelEvent *e)
{
    int d = e->angleDelta().y() / 40;
    if (d)
        moveTo(row_ - d, stop_, false);
    int h = e->angleDelta().x() / 40;
    if (h) {
        scroll_ = std::clamp(scroll_ - h, 0, std::max(0, width_ - 1));
        update();
    }
}

/* --- Bloques ------------------------------------------------------------------ */

void PatternView::copy()
{
    if (stops_.empty())
        return;
    QRect s = selection();
    clip.tracks.clear();
    clip.rows = s.height();
    for (int t = s.left(); t <= s.right(); t++) {
        const Track &tr = tracks_[size_t(t)];
        ClipTrack ct;
        if (tr.group < Editor::SMP_GROUP) {
            ct.kind = tr.ch < 0 ? CLIP_ENV : CLIP_CH;
            const Pattern *pat = ed_->ayPattern(tr.group);
            for (int r = s.top(); r <= s.bottom(); r++) {
                const Row &src = pat && r < int(pat->rows.size()) ? pat->rows[size_t(r)] : EMPTY_ROW;
                Row row;
                row.envPeriod = src.envPeriod;
                if (tr.ch < 0)
                    row.noise = src.noise;
                else
                    row.ch[0] = src.ch[tr.ch];
                ct.ay.push_back(row);
            }
        } else {
            ct.kind = CLIP_SMP;
            const SmpPattern *pat = ed_->smpPattern(tr.group - Editor::SMP_GROUP);
            for (int r = s.top(); r <= s.bottom(); r++)
                ct.smp.push_back(pat && r < int(pat->rows.size()) ? pat->rows[size_t(r)] : EMPTY_CELL);
        }
        clip.tracks.push_back(std::move(ct));
    }
}

void PatternView::erase()
{
    if (stops_.empty())
        return;
    if (!selecting_) {
        clearField();
        return;
    }
    QRect s = selection();
    ed_->begin(Editor::Everything);
    for (int t = s.left(); t <= s.right(); t++) {
        const Track &tr = tracks_[size_t(t)];
        if ((tr.group < Editor::SMP_GROUP && !ed_->ayPattern(tr.group)) ||
            (tr.group >= Editor::SMP_GROUP && !ed_->smpPattern(tr.group - Editor::SMP_GROUP)))
            continue;                   /* sin patrón: no hay nada que borrar */
        ed_->beginGroup(tr.group, false);
        for (int r = s.top(); r <= s.bottom(); r++) {
            if (tr.group < Editor::SMP_GROUP) {
                Row &row = ed_->ayEdit(tr.group).rows[size_t(r)];
                if (tr.ch < 0) {
                    row.envPeriod = -1;
                    row.noise = -1;
                } else {
                    row.ch[tr.ch] = Cell();
                }
            } else {
                ed_->smpEdit(tr.group - Editor::SMP_GROUP).rows[size_t(r)] = SmpCell();
            }
        }
    }
    ed_->changed(Editor::Everything);
}

void PatternView::cut()
{
    copy();
    bool was = selecting_;
    selecting_ = true;
    if (!was && !stops_.empty()) {
        anchorRow_ = row_;
        anchorTrack_ = stops_[size_t(stop_)].track;
    }
    erase();
    selecting_ = was;
}

void PatternView::paste()
{
    if (clip.tracks.empty() || stops_.empty())
        return;
    ed_->begin(Editor::Everything);
    int t0 = stops_[size_t(stop_)].track;
    for (size_t i = 0; i < clip.tracks.size(); i++) {
        int d = t0 + int(i);
        if (d >= int(tracks_.size()))
            break;
        const Track &tr = tracks_[size_t(d)];
        const ClipTrack &src = clip.tracks[i];
        ClipKind kind = tr.group >= Editor::SMP_GROUP ? CLIP_SMP : tr.ch < 0 ? CLIP_ENV : CLIP_CH;
        if (kind != src.kind)
            continue;                   /* cada cosa, en las de su clase */
        ed_->beginGroup(tr.group, false);
        for (int r = 0; r < clip.rows && row_ + r < rows(); r++) {
            size_t at = size_t(row_ + r);
            if (kind == CLIP_SMP) {
                ed_->smpEdit(tr.group - Editor::SMP_GROUP).rows[at] = src.smp[size_t(r)];
                continue;
            }
            Row &dst = ed_->ayEdit(tr.group).rows[at];
            const Row &s = src.ay[size_t(r)];
            if (kind == CLIP_ENV) {
                dst.envPeriod = s.envPeriod;
                dst.noise = s.noise;
            } else {
                dst.ch[tr.ch] = s.ch[0];
                if (s.ch[0].env >= 1 && s.ch[0].env <= 14 && dst.envPeriod < 0)
                    dst.envPeriod = s.envPeriod;
            }
        }
    }
    ed_->changed(Editor::Everything);
    moveTo(std::min(row_ + clip.rows, rows() - 1), stop_, false);
}

void PatternView::selectAll()
{
    if (stops_.empty())
        return;
    anchorRow_ = 0;
    anchorTrack_ = 0;
    selecting_ = true;
    row_ = rows() - 1;
    stop_ = int(stops_.size()) - 1;
    scrollToCursor();
    update();
}

void PatternView::transpose(int semitones)
{
    if (stops_.empty())
        return;
    QRect s = selection();
    if (!selecting_)
        s = QRect(stops_[size_t(stop_)].track, 0, 1, rows());
    ed_->begin(Editor::Everything);
    for (int t = s.left(); t <= s.right(); t++) {
        const Track &tr = tracks_[size_t(t)];
        bool ay = tr.group < Editor::SMP_GROUP;
        if ((ay && (tr.ch < 0 || !ed_->ayPattern(tr.group))) || (!ay && !ed_->smpPattern(tr.group - Editor::SMP_GROUP)))
            continue;
        ed_->beginGroup(tr.group, false);
        for (int r = s.top(); r <= s.bottom(); r++) {
            int &note = ay ? ed_->ayEdit(tr.group).rows[size_t(r)].ch[tr.ch].note
                           : ed_->smpEdit(tr.group - Editor::SMP_GROUP).rows[size_t(r)].note;
            if (note >= 0)
                note = std::clamp(note + semitones, 0, NOTES - 1);
        }
    }
    ed_->changed(Editor::Everything);
}

/* Meter o quitar una fila en el patrón del grupo del cursor: las demás
 * bajan o suben, y el patrón sigue con sus filas. */
void PatternView::insertRow()
{
    if (stops_.empty())
        return;
    int g = stops_[size_t(stop_)].group;
    ed_->beginGroup(g);
    if (g < Editor::SMP_GROUP) {
        std::vector<Row> &v = ed_->ayEdit(g).rows;
        v.insert(v.begin() + row_, Row());
        v.pop_back();
        ed_->changed(Editor::AyPatternPart);
    } else {
        std::vector<SmpCell> &v = ed_->smpEdit(g - Editor::SMP_GROUP).rows;
        v.insert(v.begin() + row_, SmpCell());
        v.pop_back();
        ed_->changed(Editor::SmpPatternPart);
    }
}

void PatternView::deleteRow()
{
    if (stops_.empty())
        return;
    int g = stops_[size_t(stop_)].group;
    ed_->beginGroup(g);
    if (g < Editor::SMP_GROUP) {
        std::vector<Row> &v = ed_->ayEdit(g).rows;
        v.erase(v.begin() + row_);
        v.push_back(Row());
        ed_->changed(Editor::AyPatternPart);
    } else {
        std::vector<SmpCell> &v = ed_->smpEdit(g - Editor::SMP_GROUP).rows;
        v.erase(v.begin() + row_);
        v.push_back(SmpCell());
        ed_->changed(Editor::SmpPatternPart);
    }
}
