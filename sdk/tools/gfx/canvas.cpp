/*
 * canvas.cpp - Los lienzos de zxv-gfx (ver canvas.h).
 */
#include "canvas.h"

#include <QHash>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <algorithm>

#include "png.h"

static const int MARGIN = 8;            /* alrededor de la imagen */
static const QColor BACK(0x50, 0x50, 0x50);

/* --- Canvas -------------------------------------------------------------------- */

Canvas::Canvas(Editor *ed, QWidget *parent) : QWidget(parent), ed_(ed)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    connect(ed_, &Editor::documentChanged, this, [this] {
        updateSize();
        update();
    });
    connect(ed_, &Editor::selectionChanged, this, [this] {
        updateSize();
        update();
    });
}

QRect Canvas::contentRect() const
{
    QSize s = contentSize() * zoom_;
    return QRect(MARGIN, MARGIN, s.width(), s.height());
}

QScrollArea *Canvas::scrollArea() const
{
    for (QWidget *w = parentWidget(); w; w = w->parentWidget())
        if (auto *a = qobject_cast<QScrollArea *>(w))
            return a;
    return nullptr;
}

void Canvas::updateSize()
{
    QSize s = contentSize() * zoom_;
    if (s.isEmpty())
        setFixedSize(460, 120);         /* el sitio para decir que está vacío */
    else
        setFixedSize(s.width() + 2 * MARGIN, s.height() + 2 * MARGIN);
}

void Canvas::paintEmpty(QPainter &p)
{
    p.setPen(QColor(0xE0, 0xE0, 0xE0));
    p.drawText(rect().adjusted(16, 0, -16, 0), Qt::AlignCenter | Qt::TextWordWrap, emptyText());
}

void Canvas::setZoom(int zoom, QPoint at)
{
    zoom = std::clamp(zoom, minZoom(), maxZoom());
    if (zoom == zoom_)
        return;
    QScrollArea *area = scrollArea();
    if (at.x() < 0 && area)
        at = mapFrom(area->viewport(), area->viewport()->rect().center());
    /* El punto de la imagen bajo at, y dónde está en la vista. */
    QPointF img = QPointF(at - QPoint(MARGIN, MARGIN)) / zoom_;
    QPoint inView = area ? mapTo(area->viewport(), at) : QPoint();
    zoom_ = zoom;
    updateSize();
    if (area) {
        QPoint now = QPoint(MARGIN, MARGIN) + (img * zoom_).toPoint();
        area->horizontalScrollBar()->setValue(now.x() - inView.x());
        area->verticalScrollBar()->setValue(now.y() - inView.y());
    }
    update();
    emit zoomChanged(zoom_);
}

void Canvas::zoomBy(int steps)
{
    static const int levels[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64 };
    int i = 0;
    while (i < 11 && levels[i] < zoom_)
        i++;
    i = std::clamp(i + steps, 0, 11);
    setZoom(levels[i]);
}

void Canvas::fit()
{
    QScrollArea *area = scrollArea();
    QSize c = contentSize();
    if (!area || c.isEmpty())
        return;
    QSize v = area->viewport()->size() - QSize(2 * MARGIN, 2 * MARGIN);
    int z = std::min(v.width() / c.width(), v.height() / c.height());
    setZoom(std::clamp(z, minZoom(), std::min(maxZoom(), 32)));
}

void Canvas::setGrid(bool on)
{
    grid_ = on;
    update();
}

void Canvas::checker(QPainter &p, const QRect &r)
{
    p.fillRect(r, QColor(0xC0, 0xC0, 0xC0));
    for (int y = r.top(); y <= r.bottom(); y += 8)
        for (int x = r.left() + ((y - r.top()) / 8 & 1) * 8; x <= r.right(); x += 16)
            p.fillRect(QRect(x, y, 8, 8) & r, QColor(0xE8, 0xE8, 0xE8));
}

void Canvas::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        int steps = e->angleDelta().y() > 0 ? 1 : -1;
        static const int levels[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64 };
        int i = 0;
        while (i < 11 && levels[i] < zoom_)
            i++;
        setZoom(levels[std::clamp(i + steps, 0, 11)], e->position().toPoint());
        e->accept();
    } else {
        e->ignore();
    }
}

void Canvas::pan(QMouseEvent *e)
{
    QScrollArea *area = scrollArea();
    if (!area)
        return;
    QPoint d = e->globalPosition().toPoint() - panFrom_;
    panFrom_ = e->globalPosition().toPoint();
    area->horizontalScrollBar()->setValue(area->horizontalScrollBar()->value() - d.x());
    area->verticalScrollBar()->setValue(area->verticalScrollBar()->value() - d.y());
}

void Canvas::leaveEvent(QEvent *)
{
    emit hover(QString());
}

/* --- PixelCanvas ----------------------------------------------------------------- */

PixelCanvas::PixelCanvas(Editor *ed, QWidget *parent) : Canvas(ed, parent)
{
    connect(ed_, &Editor::selectionChanged, this, [this] {
        commit();
        sel_ = QRect();
        preview_ = gfx::Image();
        button_ = -1;
    });
    connect(ed_, &Editor::toolChanged, this, [this] {
        if (ed_->tool() != Tool::Select)
            commit();
    });
    updateSize();
}

QString PixelCanvas::emptyText() const
{
    if (ed_->isTiles())
        return tr("Aún no hay tiles de %1 x %1: añade uno en Propiedades, o importa una imagen.").arg(ed_->tileSide());
    if (ed_->section() == Section::Textures)
        return tr("La textura %1 está vacía: créala en Propiedades, o importa una imagen.").arg(ed_->index());
    return tr("El sprite %1 está vacío: créalo en Propiedades (o con doble clic en la lista), pega una imagen o "
              "importa una.").arg(ed_->index());
}

QSize PixelCanvas::contentSize() const
{
    gfx::Image *im = const_cast<Editor *>(ed_)->image();
    return im ? QSize(im->w, im->h) : QSize(0, 0);
}

QPoint PixelCanvas::pixelAt(QPoint p) const
{
    QPoint q = p - QPoint(MARGIN, MARGIN);
    return QPoint(q.x() >= 0 ? q.x() / zoom_ : -1 - (-q.x() - 1) / zoom_,
                  q.y() >= 0 ? q.y() / zoom_ : -1 - (-q.y() - 1) / zoom_);
}

void PixelCanvas::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), BACK);
    gfx::Image *im = ed_->image();
    if (!im) {
        paintEmpty(p);
        return;
    }
    const gfx::Image &show = preview_.empty() ? *im : preview_;
    QRect r = contentRect();
    bool transp = ed_->transparentImage();
    if (transp)
        checker(p, r);
    p.drawImage(r, gfx::toQImage(show, transp));
    if (floating_) {
        QRect fr(r.topLeft() + floatPos_ * zoom_, QSize(float_.w, float_.h) * zoom_);
        p.drawImage(fr, gfx::toQImage(float_, transp));
    }
    if (grid_ && zoom_ >= 6) {
        p.setPen(QColor(0, 0, 0, 80));
        for (int x = 1; x < im->w; x++)
            p.drawLine(r.left() + x * zoom_, r.top(), r.left() + x * zoom_, r.bottom());
        for (int y = 1; y < im->h; y++)
            p.drawLine(r.left(), r.top() + y * zoom_, r.right(), r.top() + y * zoom_);
    }
    QRect s = selection();
    if (!s.isEmpty()) {
        QRect sr(r.topLeft() + s.topLeft() * zoom_, s.size() * zoom_);
        p.setPen(QPen(Qt::white, 1));
        p.drawRect(sr.adjusted(0, 0, -1, -1));
        p.setPen(QPen(Qt::black, 1, Qt::DashLine));
        p.drawRect(sr.adjusted(0, 0, -1, -1));
    }
}

void PixelCanvas::apply(gfx::Image &im, QPoint a, QPoint b, int c) const
{
    switch (ed_->tool()) {
    case Tool::Line: gfx::drawLine(im, a.x(), a.y(), b.x(), b.y(), c); break;
    case Tool::Rect: gfx::drawRect(im, a.x(), a.y(), b.x(), b.y(), c, false); break;
    case Tool::FilledRect: gfx::drawRect(im, a.x(), a.y(), b.x(), b.y(), c, true); break;
    case Tool::Ellipse: gfx::drawEllipse(im, a.x(), a.y(), b.x(), b.y(), c, false); break;
    case Tool::FilledEllipse: gfx::drawEllipse(im, a.x(), a.y(), b.x(), b.y(), c, true); break;
    default: break;
    }
}

void PixelCanvas::showHover(QPoint p)
{
    gfx::Image *im = ed_->image();
    QPoint px = pixelAt(p);
    if (!im || !im->inside(px.x(), px.y())) {
        emit hover(QString());
        return;
    }
    QString t = tr("x %1, y %2: color %3").arg(px.x()).arg(px.y()).arg(im->at(px.x(), px.y()));
    if (button_ >= 0 && ed_->tool() != Tool::Pen)
        t += tr("   (%1 x %2)").arg(std::abs(px.x() - start_.x()) + 1).arg(std::abs(px.y() - start_.y()) + 1);
    emit hover(t);
}

void PixelCanvas::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::MiddleButton) {
        panFrom_ = e->globalPosition().toPoint();
        return;
    }
    gfx::Image *im = ed_->image();
    int b = e->button() == Qt::LeftButton ? 0 : e->button() == Qt::RightButton ? 1 : -1;
    if (!im || b < 0 || button_ >= 0)
        return;
    QPoint px = pixelAt(e->position().toPoint());
    int c = ed_->color(b);

    if (ed_->tool() == Tool::Select) {
        if (floating_ && QRect(floatPos_, QSize(float_.w, float_.h)).contains(px)) {
            moving_ = true;
            grab_ = px - floatPos_;
        } else if (!floating_ && sel_.contains(px)) {
            /* Se levanta lo seleccionado para moverlo. */
            ed_->begin();
            float_ = gfx::cropped(*im, sel_.x(), sel_.y(), sel_.width(), sel_.height(), ed_->background());
            gfx::drawRect(*im, sel_.left(), sel_.top(), sel_.right(), sel_.bottom(), ed_->background(), true);
            floatPos_ = liftFrom_ = sel_.topLeft();
            lifted_ = true;
            floating_ = moving_ = true;
            grab_ = px - floatPos_;
            sel_ = QRect();
        } else {
            commit();
            selecting_ = true;
            start_ = QPoint(std::clamp(px.x(), 0, im->w - 1), std::clamp(px.y(), 0, im->h - 1));
            sel_ = QRect(start_, QSize(1, 1));
        }
        button_ = b;
        update();
        return;
    }
    if (!im->inside(px.x(), px.y()) && ed_->tool() != Tool::Line && ed_->tool() != Tool::Rect &&
        ed_->tool() != Tool::FilledRect && ed_->tool() != Tool::Ellipse && ed_->tool() != Tool::FilledEllipse)
        return;
    commit();
    button_ = b;
    start_ = last_ = px;
    switch (ed_->tool()) {
    case Tool::Pen:
        ed_->begin();
        im->set(px.x(), px.y(), c);
        break;
    case Tool::Fill:
        ed_->begin();
        gfx::floodFill(*im, px.x(), px.y(), c);
        ed_->changed();
        button_ = -1;
        break;
    case Tool::Pick:
        ed_->setColor(b, im->at(px.x(), px.y()));
        button_ = -1;
        break;
    default:
        preview_ = *im;
        apply(preview_, start_, px, c);
        break;
    }
    update();
}

void PixelCanvas::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::MiddleButton) {
        pan(e);
        return;
    }
    QPoint pos = e->position().toPoint();
    showHover(pos);
    gfx::Image *im = ed_->image();
    if (button_ < 0 || !im)
        return;
    QPoint px = pixelAt(pos);
    if (ed_->tool() == Tool::Select) {
        if (moving_) {
            floatPos_ = px - grab_;
        } else if (selecting_) {
            QPoint q(std::clamp(px.x(), 0, im->w - 1), std::clamp(px.y(), 0, im->h - 1));
            sel_ = QRect(start_, q).normalized();
        }
        update();
        return;
    }
    if (px == last_)
        return;
    int c = ed_->color(button_);
    if (ed_->tool() == Tool::Pen) {
        gfx::drawLine(*im, last_.x(), last_.y(), px.x(), px.y(), c);
    } else if (!preview_.empty()) {
        preview_ = *im;
        apply(preview_, start_, px, c);
    }
    last_ = px;
    update();
}

void PixelCanvas::mouseReleaseEvent(QMouseEvent *e)
{
    int b = e->button() == Qt::LeftButton ? 0 : e->button() == Qt::RightButton ? 1 : -1;
    if (b != button_ || b < 0)
        return;
    button_ = -1;
    gfx::Image *im = ed_->image();
    if (ed_->tool() == Tool::Select) {
        moving_ = selecting_ = false;
        if (sel_.width() == 1 && sel_.height() == 1 && !floating_)
            sel_ = QRect();             /* un clic quita la selección */
        update();
        return;
    }
    if (!im)
        return;
    if (ed_->tool() == Tool::Pen) {
        ed_->changed();
    } else if (!preview_.empty()) {
        ed_->begin();
        *im = preview_;
        preview_ = gfx::Image();
        ed_->changed();
    }
}

QRect PixelCanvas::selection() const
{
    return floating_ ? QRect(floatPos_, QSize(float_.w, float_.h)) : sel_;
}

void PixelCanvas::selectAll()
{
    commit();
    if (gfx::Image *im = ed_->image())
        sel_ = QRect(0, 0, im->w, im->h);
    update();
}

void PixelCanvas::clearSelection()
{
    commit();
    sel_ = QRect();
    update();
}

gfx::Image PixelCanvas::selected() const
{
    if (floating_)
        return float_;
    gfx::Image *im = const_cast<Editor *>(ed_)->image();
    if (!im)
        return gfx::Image();
    if (!sel_.isEmpty())
        return gfx::cropped(*im, sel_.x(), sel_.y(), sel_.width(), sel_.height(), ed_->background());
    return *im;
}

void PixelCanvas::erase()
{
    gfx::Image *im = ed_->image();
    if (!im)
        return;
    if (floating_) {
        floating_ = false;
        ed_->changed();
    } else {
        QRect r = sel_.isEmpty() ? QRect(0, 0, im->w, im->h) : sel_;
        ed_->begin();
        gfx::drawRect(*im, r.left(), r.top(), r.right(), r.bottom(), ed_->background(), true);
        ed_->changed();
    }
    update();
}

void PixelCanvas::paste(const gfx::Image &img)
{
    if (!ed_->image() || img.empty())
        return;
    commit();
    ed_->begin();
    float_ = img;
    QPoint at = sel_.isEmpty() ? QPoint(0, 0) : sel_.topLeft();
    if (QScrollArea *area = scrollArea())
        if (sel_.isEmpty()) {
            QPoint v = mapFrom(area->viewport(), QPoint(0, 0));
            at = QPoint(std::max(0, pixelAt(v).x()), std::max(0, pixelAt(v).y()));
        }
    floatPos_ = at;
    floating_ = true;
    lifted_ = false;
    sel_ = QRect();
    ed_->setTool(Tool::Select);
    update();
}

void PixelCanvas::commit()
{
    if (!floating_)
        return;
    floating_ = moving_ = false;
    if (gfx::Image *im = ed_->image()) {
        gfx::paste(*im, float_, floatPos_.x(), floatPos_.y(), ed_->transparentImage());
        sel_ = QRect(floatPos_, QSize(float_.w, float_.h)) & QRect(0, 0, im->w, im->h);
    }
    ed_->changed();
    update();
}

void PixelCanvas::discard()
{
    if (!floating_)
        return;
    floating_ = moving_ = false;
    /* Lo levantado vuelve a su sitio; lo pegado, nada: no ha cambiado nada. */
    if (lifted_)
        if (gfx::Image *im = ed_->image())
            gfx::paste(*im, float_, liftFrom_.x(), liftFrom_.y(), false);
    ed_->cancel();
    update();
}

void PixelCanvas::transform(gfx::Image (*fn)(const gfx::Image &))
{
    gfx::Image *im = ed_->image();
    if (!im)
        return;
    if (!floating_ && !sel_.isEmpty() && sel_ != QRect(0, 0, im->w, im->h)) {
        /* Con una selección, solo ella: se levanta y se cambia. */
        ed_->begin();
        float_ = gfx::cropped(*im, sel_.x(), sel_.y(), sel_.width(), sel_.height(), ed_->background());
        gfx::drawRect(*im, sel_.left(), sel_.top(), sel_.right(), sel_.bottom(), ed_->background(), true);
        floatPos_ = liftFrom_ = sel_.topLeft();
        lifted_ = true;
        floating_ = true;
        sel_ = QRect();
    }
    if (floating_) {
        float_ = fn(float_);
        update();
        return;
    }
    gfx::Image t = fn(*im);
    if ((t.w != im->w || t.h != im->h) && ed_->section() != Section::Sprites)
        return;                         /* los tiles y las texturas no cambian de tamaño */
    ed_->begin();
    *im = t;
    sel_ = QRect();
    ed_->changed();
}

/* --- MapCanvas -------------------------------------------------------------------- */

QImage tileImage(const gfx::Document &doc, int side, uint16_t cell)
{
    const auto &set = doc.tiles(side);
    int t = cell & gfx::CELL_TILE;
    if (t >= int(set.size())) {
        QImage q(side, side, QImage::Format_ARGB32);
        q.fill(0xFFFFFFFF);
        for (int i = 0; i < side; i++) {
            q.setPixel(i, i, 0xFFD70000);
            q.setPixel(side - 1 - i, i, 0xFFD70000);
        }
        return q;
    }
    QImage q = gfx::toQImage(set[t], cell & gfx::CELL_TRANSP);
    if (cell & (gfx::CELL_FLIP_H | gfx::CELL_FLIP_V))
        q = q.mirrored(cell & gfx::CELL_FLIP_H, cell & gfx::CELL_FLIP_V);
    return q;
}

MapCanvas::MapCanvas(Editor *ed, QWidget *parent) : Canvas(ed, parent)
{
    zoom_ = 2;
    updateSize();
}

void MapCanvas::setShowFront(bool on)
{
    front_ = on;
    update();
}

QString MapCanvas::emptyText() const
{
    return tr("El mapa %1 está vacío: créalo en Propiedades, o importa un nivel desde una imagen.").arg(ed_->index());
}

QSize MapCanvas::contentSize() const
{
    gfx::Map *m = const_cast<Editor *>(ed_)->map();
    return m ? QSize(m->w * m->tile, m->h * m->tile) : QSize(0, 0);
}

QPoint MapCanvas::cellAt(QPoint p) const
{
    gfx::Map *m = const_cast<Editor *>(ed_)->map();
    int s = (m ? m->tile : 8) * zoom_;
    QPoint q = p - QPoint(MARGIN, MARGIN);
    return QPoint(q.x() >= 0 ? q.x() / s : -1, q.y() >= 0 ? q.y() / s : -1);
}

uint16_t MapCanvas::brush(int button) const
{
    return button == 0 ? uint16_t(ed_->mapTile() | ed_->mapFlags()) : 0;
}

void MapCanvas::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.fillRect(rect(), BACK);
    gfx::Map *m = ed_->map();
    if (!m) {
        paintEmpty(p);
        return;
    }
    int s = m->tile, cs = s * zoom_;
    QRect r = contentRect();
    QRect vis = e->rect() & r;
    int x0 = (vis.left() - r.left()) / cs, x1 = std::min(m->w - 1, (vis.right() - r.left()) / cs);
    int y0 = (vis.top() - r.top()) / cs, y1 = std::min(m->h - 1, (vis.bottom() - r.top()) / cs);
    QHash<uint16_t, QImage> cache;
    for (int cy = y0; cy <= y1; cy++)
        for (int cx = x0; cx <= x1; cx++) {
            uint16_t v = m->at(cx, cy);
            QRect cr(r.left() + cx * cs, r.top() + cy * cs, cs, cs);
            /* Lo que se está pintando con una caja, encima. */
            if (button_ >= 0 && (ed_->tool() == Tool::Rect || ed_->tool() == Tool::FilledRect) &&
                QRect(start_, last_).normalized().contains(cx, cy))
                v = brush(button_);
            auto hit = cache.find(v & ~gfx::CELL_PRIO);
            if (hit == cache.end())
                hit = cache.insert(v & ~gfx::CELL_PRIO, tileImage(ed_->doc(), s, v));
            if (v & gfx::CELL_TRANSP)
                checker(p, cr);
            p.drawImage(cr, *hit);
            if (front_ && (v & gfx::CELL_PRIO)) {
                QPolygon tri({ cr.topRight() + QPoint(1, 0), cr.topRight() + QPoint(1 - cs / 3, 0),
                               cr.topRight() + QPoint(1, cs / 3) });
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0xFF, 0xD7, 0));
                p.drawPolygon(tri);
            }
        }
    if (grid_) {
        p.setPen(QColor(255, 255, 255, 60));
        for (int x = x0 + 1; x <= x1; x++)
            p.drawLine(r.left() + x * cs, vis.top(), r.left() + x * cs, vis.bottom());
        for (int y = y0 + 1; y <= y1; y++)
            p.drawLine(vis.left(), r.top() + y * cs, vis.right(), r.top() + y * cs);
    }
}

void MapCanvas::showHover(QPoint pos)
{
    gfx::Map *m = ed_->map();
    QPoint c = cellAt(pos);
    if (!m || c.x() < 0 || c.y() < 0 || c.x() >= m->w || c.y() >= m->h) {
        emit hover(QString());
        return;
    }
    uint16_t v = m->at(c.x(), c.y());
    QString t = tr("x %1, y %2: tile %3").arg(c.x()).arg(c.y()).arg(v & gfx::CELL_TILE);
    if (v & gfx::CELL_FLIP_H)
        t += tr(", espejo H");
    if (v & gfx::CELL_FLIP_V)
        t += tr(", espejo V");
    if (v & gfx::CELL_TRANSP)
        t += tr(", transparente");
    if (v & gfx::CELL_PRIO)
        t += tr(", delante");
    emit hover(t);
}

void MapCanvas::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::MiddleButton) {
        panFrom_ = e->globalPosition().toPoint();
        return;
    }
    gfx::Map *m = ed_->map();
    int b = e->button() == Qt::LeftButton ? 0 : e->button() == Qt::RightButton ? 1 : -1;
    QPoint c = cellAt(e->position().toPoint());
    if (!m || b < 0 || button_ >= 0 || c.x() < 0 || c.y() < 0 || c.x() >= m->w || c.y() >= m->h)
        return;
    button_ = b;
    start_ = last_ = c;
    switch (ed_->tool()) {
    case Tool::Pick: {
        uint16_t v = m->at(c.x(), c.y());
        ed_->setMapTile(v & gfx::CELL_TILE);
        ed_->setMapFlags(v & ~gfx::CELL_TILE);
        button_ = -1;
        break;
    }
    case Tool::Fill: {
        /* Rellena las celdas iguales que se tocan. */
        uint16_t from = m->at(c.x(), c.y()), to = brush(b);
        button_ = -1;
        if (from == to)
            break;
        ed_->begin();
        std::vector<QPoint> stack{ c };
        while (!stack.empty()) {
            QPoint q = stack.back();
            stack.pop_back();
            if (q.x() < 0 || q.y() < 0 || q.x() >= m->w || q.y() >= m->h || m->at(q.x(), q.y()) != from)
                continue;
            m->set(q.x(), q.y(), to);
            stack.push_back(q + QPoint(1, 0));
            stack.push_back(q - QPoint(1, 0));
            stack.push_back(q + QPoint(0, 1));
            stack.push_back(q - QPoint(0, 1));
        }
        ed_->changed();
        break;
    }
    case Tool::Rect:
    case Tool::FilledRect:
        break;
    default:                            /* el lápiz (y lo que no sirve en los mapas) */
        ed_->begin();
        m->set(c.x(), c.y(), brush(b));
        break;
    }
    update();
}

void MapCanvas::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::MiddleButton) {
        pan(e);
        return;
    }
    QPoint pos = e->position().toPoint();
    showHover(pos);
    gfx::Map *m = ed_->map();
    if (button_ < 0 || !m)
        return;
    QPoint c = cellAt(pos);
    c = QPoint(std::clamp(c.x(), 0, m->w - 1), std::clamp(c.y(), 0, m->h - 1));
    if (c == last_)
        return;
    if (ed_->tool() != Tool::Rect && ed_->tool() != Tool::FilledRect) {
        /* Todas las celdas entre la anterior y esta. */
        int n = std::max(std::abs(c.x() - last_.x()), std::abs(c.y() - last_.y()));
        for (int i = 1; i <= n; i++)
            m->set(last_.x() + (c.x() - last_.x()) * i / n, last_.y() + (c.y() - last_.y()) * i / n, brush(button_));
    }
    last_ = c;
    update();
}

void MapCanvas::mouseReleaseEvent(QMouseEvent *e)
{
    int b = e->button() == Qt::LeftButton ? 0 : e->button() == Qt::RightButton ? 1 : -1;
    if (b != button_ || b < 0)
        return;
    button_ = -1;
    gfx::Map *m = ed_->map();
    if (!m)
        return;
    if (ed_->tool() == Tool::Rect || ed_->tool() == Tool::FilledRect) {
        ed_->begin();
        QRect r = QRect(start_, last_).normalized();
        for (int y = r.top(); y <= r.bottom(); y++)
            for (int x = r.left(); x <= r.right(); x++)
                m->set(x, y, brush(b));
    }
    ed_->changed();
}
