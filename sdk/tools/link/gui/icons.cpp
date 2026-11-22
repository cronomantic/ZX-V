/*
 * icons.cpp - Los iconos y el logo de zxv-link-gui (ver icons.h).
 */
#include "icons.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>

#include <functional>

/* Los de la franja del logo, y el azul de la máquina. */
static const QColor RED(0xD7, 0x00, 0x00), YELLOW(0xD7, 0xD7, 0x00), GREEN(0x00, 0xD7, 0x00),
    CYAN(0x00, 0xD7, 0xD7), BLUE(0x00, 0x00, 0xD7), GREEN_DARK(0x00, 0x9A, 0x00), CYAN_DARK(0x00, 0x9A, 0x9A);

static QColor ink()
{
    return QApplication::palette().color(QPalette::ButtonText);
}

static QColor paper()
{
    return QApplication::palette().color(QPalette::Base);
}

/* Un icono a varios tamaños, dibujado en una rejilla de 24 x 24. */
static QIcon drawn(const std::function<void(QPainter &)> &draw)
{
    QIcon ic;
    for (int s : { 16, 20, 24, 32, 48, 64 }) {
        QPixmap pm(s, s);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.scale(s / 24.0, s / 24.0);
        draw(p);
        p.end();
        ic.addPixmap(pm);
    }
    return ic;
}

static QPen pen(const QColor &c, qreal w = 1.6)
{
    return QPen(c, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
}

/* Una hoja de papel, con la esquina doblada. */
static void page(QPainter &p, qreal x, qreal y, qreal w, qreal h, const QColor &fill)
{
    QPainterPath path;
    path.moveTo(x, y);
    path.lineTo(x + w - 4, y);
    path.lineTo(x + w, y + 4);
    path.lineTo(x + w, y + h);
    path.lineTo(x, y + h);
    path.closeSubpath();
    p.setPen(pen(ink(), 1.4));
    p.setBrush(fill);
    p.drawPath(path);
}

QIcon icon(Ic kind)
{
    switch (kind) {
    case Ic::Play:
        return drawn([](QPainter &p) {
            p.setPen(pen(GREEN_DARK, 1.6));
            p.setBrush(GREEN);
            p.drawPolygon(QPolygonF({ QPointF(6.5, 3.5), QPointF(20.5, 12), QPointF(6.5, 20.5) }));
        });
    case Ic::Copy:
        return drawn([](QPainter &p) {
            page(p, 3.5, 2.5, 11, 14, paper());
            page(p, 9.5, 7.5, 11, 14, CYAN);
        });
    case Ic::Folder:
        return drawn([](QPainter &p) {
            QPainterPath path;
            path.moveTo(2.5, 5.5);
            path.lineTo(9, 5.5);
            path.lineTo(11, 8);
            path.lineTo(21.5, 8);
            path.lineTo(21.5, 19.5);
            path.lineTo(2.5, 19.5);
            path.closeSubpath();
            p.setPen(pen(ink(), 1.4));
            p.setBrush(YELLOW);
            p.drawPath(path);
        });
    case Ic::Add:
        return drawn([](QPainter &p) {
            p.setPen(pen(GREEN_DARK, 3));
            p.drawLine(QPointF(12, 4.5), QPointF(12, 19.5));
            p.drawLine(QPointF(4.5, 12), QPointF(19.5, 12));
        });
    case Ic::Refresh:
        return drawn([](QPainter &p) {
            p.setPen(pen(ink(), 2.2));
            p.setBrush(Qt::NoBrush);
            p.drawArc(QRectF(5, 5, 14, 14), 70 * 16, 290 * 16);
            p.setPen(Qt::NoPen);
            p.setBrush(ink());
            p.drawPolygon(QPolygonF({ QPointF(10.5, 1.5), QPointF(17, 5.2), QPointF(11, 9.5) }));
        });
    case Ic::Plug:
        return drawn([](QPainter &p) {
            p.setPen(pen(ink(), 2));
            p.drawLine(QPointF(9, 2.5), QPointF(9, 7.5));
            p.drawLine(QPointF(15, 2.5), QPointF(15, 7.5));
            p.drawLine(QPointF(12, 16), QPointF(12, 21.5));
            p.setPen(pen(ink(), 1.4));
            p.setBrush(CYAN);
            p.drawRoundedRect(QRectF(5.5, 7.5, 13, 9), 2.5, 2.5);
        });
    case Ic::Program:
        return drawn([](QPainter &p) {
            page(p, 4.5, 2.5, 15, 19, paper());
            const QColor stripe[] = { RED, YELLOW, GREEN, CYAN };
            p.setPen(Qt::NoPen);
            for (int i = 0; i < 4; i++) {
                p.setBrush(stripe[i]);
                p.drawRect(QRectF(6.5 + i * 2.75, 15, 2.75, 3.5));
            }
            p.setPen(pen(ink(), 1.2));
            p.drawLine(QPointF(7.5, 7.5), QPointF(14.5, 7.5));
            p.drawLine(QPointF(7.5, 10.5), QPointF(16.5, 10.5));
        });
    case Ic::File:
        return drawn([](QPainter &p) {
            page(p, 5.5, 2.5, 13, 19, paper());
            p.setPen(pen(ink(), 1.2));
            for (qreal y : { 9.0, 12.5, 16.0 })
                p.drawLine(QPointF(8.5, y), QPointF(15.5, y));
        });
    case Ic::Done:
        return drawn([](QPainter &p) {
            p.setPen(pen(GREEN_DARK, 3));
            p.drawPolyline(QPolygonF({ QPointF(4.5, 12.5), QPointF(10, 18), QPointF(19.5, 6) }));
        });
    case Ic::Same:
        return drawn([](QPainter &p) {
            p.setPen(pen(QApplication::palette().color(QPalette::PlaceholderText), 2.6));
            p.drawLine(QPointF(5.5, 9), QPointF(18.5, 9));
            p.drawLine(QPointF(5.5, 15), QPointF(18.5, 15));
        });
    case Ic::Sending:
        return drawn([](QPainter &p) {
            p.setPen(pen(CYAN_DARK, 2.6));
            p.drawLine(QPointF(3.5, 12), QPointF(18, 12));
            p.drawPolyline(QPolygonF({ QPointF(12.5, 6), QPointF(19, 12), QPointF(12.5, 18) }));
        });
    case Ic::Error:
        return drawn([](QPainter &p) {
            p.setPen(pen(RED, 3));
            p.drawLine(QPointF(6, 6), QPointF(18, 18));
            p.drawLine(QPointF(18, 6), QPointF(6, 18));
        });
    case Ic::Warning:
        return drawn([](QPainter &p) {
            p.setPen(pen(ink(), 1.4));
            p.setBrush(YELLOW);
            p.drawPolygon(QPolygonF({ QPointF(12, 3), QPointF(21.5, 20), QPointF(2.5, 20) }));
            p.setPen(pen(ink(), 2.2));
            p.drawLine(QPointF(12, 9), QPointF(12, 14));
            p.drawPoint(QPointF(12, 17.2));
        });
    case Ic::Clear:
        return drawn([](QPainter &p) {
            p.translate(12, 12);
            p.rotate(-40);
            p.setPen(pen(ink(), 1.4));
            p.setBrush(RED);
            p.drawRoundedRect(QRectF(-8.5, -4, 8, 8), 1.5, 1.5);
            p.setBrush(paper());
            p.drawRoundedRect(QRectF(-0.5, -4, 9, 8), 1.5, 1.5);
        });
    }
    return QIcon();
}

QPixmap dot(Dot kind, int size)
{
    qreal dpr = qApp->devicePixelRatio();
    QPixmap pm(QSize(size, size) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QColor c = kind == Dot::On ? GREEN : kind == Dot::Busy ? YELLOW : kind == Dot::Error ? RED : QColor(0x9A, 0x9A, 0x9A);
    p.setPen(QPen(c.darker(140), 1));
    p.setBrush(c);
    p.drawEllipse(QRectF(1, 1, size - 2, size - 2));
    return pm;
}

QPixmap stepBadge(int n, int size)
{
    qreal dpr = qApp->devicePixelRatio();
    QPixmap pm(QSize(size, size) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(BLUE);
    p.drawEllipse(QRectF(0, 0, size, size));
    QFont f = qApp->font();
    f.setBold(true);
    f.setPixelSize(size * 6 / 10);
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, QString::number(n));
    return pm;
}

/* El de docs/images/logo-light.svg: las letras, en 212 x 60, y la franja
 * debajo, de 72 a 81. */
Logo::Logo(int height, QWidget *parent) : QWidget(parent), h(height)
{
    setFixedSize(sizeHint());
    setToolTip(QStringLiteral("ZX-V"));
}

QSize Logo::sizeHint() const
{
    return QSize(h * 212 / 81 + 1, h);
}

void Logo::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(h / 81.0, h / 81.0);
    p.setPen(Qt::NoPen);
    p.setBrush(palette().color(QPalette::WindowText));
    p.drawPolygon(QPolygonF({ QPointF(0, 0), QPointF(44, 0), QPointF(44, 11), QPointF(15, 49), QPointF(44, 49),
                              QPointF(44, 60), QPointF(0, 60), QPointF(0, 49), QPointF(29, 11), QPointF(0, 11) }));
    p.drawPolygon(QPolygonF({ QPointF(56, 0), QPointF(69, 0), QPointF(78, 20), QPointF(87, 0), QPointF(100, 0),
                              QPointF(85, 30), QPointF(100, 60), QPointF(87, 60), QPointF(78, 40), QPointF(69, 60),
                              QPointF(56, 60), QPointF(71, 30) }));
    p.drawRect(QRectF(120, 25, 28, 11));
    p.drawPolygon(QPolygonF({ QPointF(168, 0), QPointF(181, 0), QPointF(190, 44), QPointF(199, 0), QPointF(212, 0),
                              QPointF(197, 60), QPointF(183, 60) }));
    const QColor stripe[] = { RED, YELLOW, GREEN, CYAN };
    for (int i = 0; i < 4; i++)
        p.fillRect(QRectF(i * 53, 72, 53, 9), stripe[i]);
}
