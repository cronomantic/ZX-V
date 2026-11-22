/*
 * patternview.h - Los patrones de una posición, uno al lado del otro: cada
 * AY como en Vortex Tracker (el periodo de la envolvente, el ruido y los tres
 * canales) y luego las pistas de muestras (nota, instrumento, volumen y dos
 * efectos). La fila que se edita, siempre en el centro; si no caben todas,
 * se desplaza a lo ancho para que se vea el cursor.
 *
 *   fila  env  ru   nota sample env orn vol  efecto retardo parámetro  ...
 *   00   .... ..   C-4 1F.F .... ...                                  C-4 01 40 A04 ...
 *
 * Un clic en el nombre de una pista (A, B, C, M1...) la silencia.
 */
#ifndef ZXV_TRACKER_PATTERNVIEW_H
#define ZXV_TRACKER_PATTERNVIEW_H

#include <QWidget>

#include <vector>

#include "tune.h"

class Editor;

class PatternView : public QWidget
{
    Q_OBJECT

public:
    explicit PatternView(Editor *ed, QWidget *parent = nullptr);

    int row() const { return row_; }
    void setRow(int r);
    int group() const;                  /* el del cursor: 0-1 un AY, 2- una pista de muestras */
    int channel() const;                /* 0-2 el canal del AY del cursor (-1 en env y ruido, y en las de muestras) */
    void setPlayRow(int r);             /* la fila que suena (-1 ninguna) */
    int octave() const { return octave_; }
    void setOctave(int o);
    int step() const { return step_; }
    void setStep(int s) { step_ = s; }
    void setHighlight(int h) { highlight_ = h > 0 ? h : 4; update(); }
    /* Las pistas silenciadas: bit n, el canal n de los AY (0-5) o la pista
     * de muestras n - 6. */
    int muted() const { return muted_; }
    void setMuted(int mask);

    bool hasSelection() const { return selecting_; }
    void copy();
    void cut();
    void paste();
    void erase();                       /* lo seleccionado, o el campo */
    void selectAll();
    void transpose(int semitones);      /* lo seleccionado, o la pista */
    void insertRow();                   /* en el grupo del cursor */
    void deleteRow();

    QSize sizeHint() const override;

signals:
    void preview(const trk::Cell &cell, int envPeriod, int chip, int channel);
    void previewSample(const trk::SmpCell &cell, int track);
    void previewOff();
    void stopRequested();           /* Esc sin nada seleccionado */
    void cursorMoved();
    void mutedChanged(int mask);

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void focusInEvent(QFocusEvent *) override { update(); }
    void focusOutEvent(QFocusEvent *) override;
    void resizeEvent(QResizeEvent *) override { scrollToCursor(); }
    bool focusNextPrevChild(bool) override { return false; }   /* el tabulador cambia de pista */

private:
    enum Kind { Env, Noise, Note, Sample, EnvType, Orn, Vol, Fx, Delay, Param, SNote, SIns, SVol, SFx, SParam };
    struct Stop {
        int x;                          /* en caracteres, desde la primera columna de las pistas */
        Kind kind;
        int group;                      /* 0-1 un AY, 2- una pista de muestras */
        int ch;                         /* el canal del AY (-1 en env y ruido, y en las de muestras) */
        int track;                      /* la pista: las de selección, copiar y pegar */
        int digit;
    };
    /* Una pista (para seleccionar, copiar y pegar): la env y el ruido de un
     * AY, un canal de un AY o una pista de muestras. */
    struct Track {
        int group, ch;                  /* ch: -1 env y ruido (o la pista de muestras), 0-2 el canal */
        int x0, x1;                     /* de dónde a dónde, en caracteres */
        int mute;                       /* su bit en muted_, o -1 */
    };

    void layout();
    int rows() const;
    void moveTo(int row, int stop, bool select);
    bool enter(QKeyEvent *e);
    bool enterAy(QKeyEvent *e, const Stop &s);
    bool enterSmp(QKeyEvent *e, const Stop &s);
    void clearField();
    void advance(bool multi);
    QRect selection() const;            /* columnas: pistas (tracks_); filas */
    int stopAt(int x) const;
    int rowAt(int y) const;
    int firstStop(int track) const;
    void scrollToCursor();
    int contentX() const;               /* dónde empiezan las pistas, en píxeles */

    Editor *ed_;
    std::vector<Stop> stops_;
    std::vector<Track> tracks_;
    int width_ = 0;                     /* el ancho de todas las pistas, en caracteres */
    int row_ = 0, stop_ = 0;
    int playRow_ = -1;
    int octave_ = 4, step_ = 1, highlight_ = 4;
    bool selecting_ = false;
    int anchorRow_ = 0, anchorTrack_ = 0;
    int previewKey_ = 0;
    int muted_ = 0;
    int scroll_ = 0;                    /* caracteres de las pistas que quedan a la izquierda */
    int cw_ = 8, lh_ = 16, top_ = 0;    /* el ancho de un carácter, el alto de una fila, la cabecera */
};

#endif // ZXV_TRACKER_PATTERNVIEW_H
