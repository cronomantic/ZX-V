/*
 * mode7.cpp - La vista del modo 7 (ver mode7.h).
 */
#include "mode7.h"

#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <cmath>

Mode7View::Mode7View(Editor *ed, int texture, QWidget *parent) : QWidget(parent), ed_(ed), texture_(texture)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
    connect(ed_, &Editor::documentChanged, this, QOverload<>::of(&QWidget::update));
}

void Mode7View::paintEvent(QPaintEvent *)
{
    /* Se dibuja a 320 x 240, como el modo 0 de la máquina con su borde, y se
     * amplía sin suavizar. */
    const int W = 320, H = 240, HORIZON = 80;
    QImage img(W, H, QImage::Format_RGB32);
    const gfx::Texture &t = ed_->doc().textures[texture_];
    QRgb sky = 0xFF000000 | gfx::paletteRgb(5), out = 0xFF000000 | gfx::paletteRgb(t.out);
    double ca = std::cos(angle_), sa = std::sin(angle_), focal = 160;
    int side = t.img.w;
    for (int y = 0; y < H; y++) {
        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        if (y <= HORIZON || t.empty()) {
            for (int x = 0; x < W; x++)
                line[x] = sky;
            continue;
        }
        double dist = height_ * focal / (y - HORIZON);
        for (int x = 0; x < W; x++) {
            double side_off = (x - W / 2) * dist / focal;
            double u = x_ + ca * dist - sa * side_off, v = y_ + sa * dist + ca * side_off;
            int tu = int(std::floor(u)), tv = int(std::floor(v));
            if (t.wrap) {
                tu &= side - 1;
                tv &= side - 1;
            } else if (tu < 0 || tv < 0 || tu >= side || tv >= side) {
                line[x] = out;
                continue;
            }
            line[x] = 0xFF000000 | gfx::paletteRgb(t.img.at(tu, tv));
        }
    }
    QPainter p(this);
    int z = std::max(1, std::min(width() / W, height() / H));
    QRect r((width() - W * z) / 2, (height() - H * z) / 2, W * z, H * z);
    p.fillRect(rect(), Qt::black);
    p.drawImage(r, img);
}

void Mode7View::keyPressEvent(QKeyEvent *e)
{
    double step = 2;
    switch (e->key()) {
    case Qt::Key_Left: angle_ -= 0.05; break;
    case Qt::Key_Right: angle_ += 0.05; break;
    case Qt::Key_Up:
        x_ += std::cos(angle_) * step;
        y_ += std::sin(angle_) * step;
        break;
    case Qt::Key_Down:
        x_ -= std::cos(angle_) * step;
        y_ -= std::sin(angle_) * step;
        break;
    case Qt::Key_PageUp: height_ = std::min(256.0, height_ * 1.1); break;
    case Qt::Key_PageDown: height_ = std::max(2.0, height_ / 1.1); break;
    default: QWidget::keyPressEvent(e); return;
    }
    update();
}

void Mode7View::mousePressEvent(QMouseEvent *e)
{
    drag_ = e->position().toPoint();
}

void Mode7View::mouseMoveEvent(QMouseEvent *e)
{
    QPoint d = e->position().toPoint() - drag_;
    drag_ = e->position().toPoint();
    angle_ += d.x() * 0.01;
    x_ -= std::cos(angle_) * d.y() * 0.5;
    y_ -= std::sin(angle_) * d.y() * 0.5;
    update();
}

void Mode7View::wheelEvent(QWheelEvent *e)
{
    height_ = e->angleDelta().y() > 0 ? std::max(2.0, height_ / 1.1) : std::min(256.0, height_ * 1.1);
    update();
}

Mode7Dialog::Mode7Dialog(Editor *ed, int texture, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Textura %1 en el modo 7").arg(texture));
    auto *lay = new QVBoxLayout(this);
    auto *view = new Mode7View(ed, texture);
    lay->addWidget(view, 1);
    lay->addWidget(new QLabel(tr("Flechas: avanzar y girar. RePág, AvPág o la rueda: la altura. "
                                 "También se puede arrastrar con el ratón.")));
    view->setFocus();
}
