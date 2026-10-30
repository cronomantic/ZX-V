/*
 * mode7.h - Una textura como suelo del modo 7, con un horizonte y una cámara
 * que se mueve con las flechas (o arrastrando con el ratón), para ver cómo
 * queda. Es una vista aproximada: el dibujo exacto es el del motor de la
 * máquina.
 */
#ifndef ZXV_GFX_MODE7_H
#define ZXV_GFX_MODE7_H

#include <QDialog>

#include "editor.h"

class Mode7View : public QWidget
{
    Q_OBJECT

public:
    Mode7View(Editor *ed, int texture, QWidget *parent = nullptr);
    QSize sizeHint() const override { return QSize(640, 480); }

protected:
    void paintEvent(QPaintEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

private:
    Editor *ed_;
    int texture_;
    double x_ = 0, y_ = 0, angle_ = 0, height_ = 32;
    QPoint drag_;
};

class Mode7Dialog : public QDialog
{
    Q_OBJECT

public:
    Mode7Dialog(Editor *ed, int texture, QWidget *parent = nullptr);
};

#endif // ZXV_GFX_MODE7_H
