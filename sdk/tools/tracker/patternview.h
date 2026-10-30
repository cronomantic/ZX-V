/*
 * patternview.h - El patrón, como en Vortex Tracker: una fila por línea con
 * el periodo de la envolvente, el ruido y los tres canales; la fila que se
 * edita, siempre en el centro.
 *
 *   fila  env  ru   nota sample env orn vol  efecto retardo parámetro
 *   00   .... ..   C-4 1F.F .... ...
 */
#ifndef ZXV_TRACKER_PATTERNVIEW_H
#define ZXV_TRACKER_PATTERNVIEW_H

#include <QWidget>

#include <vector>

#include "song.h"

class Editor;

class PatternView : public QWidget
{
    Q_OBJECT

public:
    explicit PatternView(Editor *ed, QWidget *parent = nullptr);

    int row() const { return row_; }
    void setRow(int r);
    int channel() const;                /* 0-2 el del cursor (-1 en env y ruido) */
    void setPlayRow(int r);             /* la fila que suena (-1 ninguna) */
    int octave() const { return octave_; }
    void setOctave(int o);
    int step() const { return step_; }
    void setStep(int s) { step_ = s; }
    void setHighlight(int h) { highlight_ = h > 0 ? h : 4; update(); }

    bool hasSelection() const { return selecting_; }
    void copy();
    void cut();
    void paste();
    void erase();                       /* lo seleccionado, o el campo */
    void selectAll();
    void transpose(int semitones);      /* lo seleccionado, o el canal */
    void insertRow();
    void deleteRow();

    QSize sizeHint() const override;

signals:
    void preview(const trk::Cell &cell, int envPeriod, int channel);
    void previewOff();
    void stopRequested();           /* Esc sin nada seleccionado */
    void cursorMoved();

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void focusInEvent(QFocusEvent *) override { update(); }
    void focusOutEvent(QFocusEvent *) override;
    bool focusNextPrevChild(bool) override { return false; }   /* el tabulador cambia de canal */

private:
    enum Kind { Env, Noise, Note, Sample, EnvType, Orn, Vol, Fx, Delay, Param };
    struct Stop {
        int x;                          /* en caracteres */
        Kind kind;
        int ch;                         /* el canal (-1 en env y ruido) */
        int digit;
    };

    int rows() const;
    int track(const Stop &s) const { return s.ch + 1; }   /* 0 env y ruido, 1-3 los canales */
    void moveTo(int row, int stop, bool select);
    bool enter(QKeyEvent *e);
    void clearField();
    void advance(bool multi);
    QRect selection() const;            /* columnas: pistas 0-3; filas */
    int stopAt(int x) const;
    int rowAt(int y) const;

    Editor *ed_;
    std::vector<Stop> stops_;
    int row_ = 0, stop_ = 2;
    int playRow_ = -1;
    int octave_ = 4, step_ = 1, highlight_ = 4;
    bool selecting_ = false;
    int anchorRow_ = 0, anchorTrack_ = 0;
    int previewKey_ = 0;
    int cw_ = 8, lh_ = 16, top_ = 0;    /* el ancho de un carácter, el alto de una fila, la cabecera */
};

#endif // ZXV_TRACKER_PATTERNVIEW_H
