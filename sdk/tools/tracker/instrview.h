/*
 * instrview.h - Los instrumentos de las pistas de muestras, a la vista:
 *
 *   EnvelopeView  una envolvente (la del volumen o la del panorama), como en
 *                 FastTracker 2 o MilkyTracker: hasta 12 puntos (tick y valor,
 *                 0-64) que se arrastran; un clic donde no hay ninguno añade
 *                 uno, el botón derecho lo quita. El punto de sostener y el
 *                 bucle, marcados.
 *   NoteMapView   las 96 notas (C-0 a B-7) y la muestra que suena en cada
 *                 una: un clic o arrastrar las pinta con la muestra elegida.
 *
 * Los dos cambian el instrumento ed->instrument() de la canción, con
 * deshacer.
 */
#ifndef ZXV_TRACKER_INSTRVIEW_H
#define ZXV_TRACKER_INSTRVIEW_H

#include <QWidget>

#include <array>
#include <cstdint>

#include "tune.h"

class Editor;

/* Lo de un instrumento de muestras (sus 208 bytes, ZXM_SI_*). */
namespace ins {
constexpr int POINTS = 12;
struct Env {
    int n = 0, sustain = 0, loopFrom = 0, loopTo = 0;
    bool on = false, sus = false, loop = false;
    std::array<int, POINTS> tick {}, value {};
};
Env env(const trk::SmpInstrument &i, int which);    /* 0 el volumen, 1 el panorama */
void setEnv(trk::SmpInstrument &i, int which, const Env &e);
int fade(const trk::SmpInstrument &i);
void setFade(trk::SmpInstrument &i, int v);
/* El vibrato automático: forma (0 seno, 1 sierra, 2 cuadrada), barrido, profundidad, velocidad. */
std::array<int, 4> vibrato(const trk::SmpInstrument &i);
void setVibrato(trk::SmpInstrument &i, const std::array<int, 4> &v);
} // namespace ins

class EnvelopeView : public QWidget
{
    Q_OBJECT

public:
    EnvelopeView(Editor *ed, int which, QWidget *parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QPoint pointPos(int i) const;       /* dónde se pinta el punto i (para las pruebas) */

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *) override { drag_ = -1; }

private:
    trk::SmpInstrument *instrument() const;
    int maxTick() const;
    QPoint toScreen(int tick, int value) const;
    int pointAt(QPoint p) const;

    Editor *ed_;
    int which_;
    int drag_ = -1;
};

class NoteMapView : public QWidget
{
    Q_OBJECT

public:
    explicit NoteMapView(Editor *ed, QWidget *parent = nullptr);
    void setPaint(int sample) { paint_ = sample; }   /* la que se pinta (255: ninguna) */
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QRect keyRect(int note) const;

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

private:
    int noteAt(QPoint p) const;
    void paintNote(int note, bool first);

    Editor *ed_;
    int paint_ = 0, last_ = -1;
};

#endif // ZXV_TRACKER_INSTRVIEW_H
