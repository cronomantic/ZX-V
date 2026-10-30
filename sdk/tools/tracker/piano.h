/*
 * piano.h - El teclado del PC como piano (como en Vortex Tracker y los
 * trackers de siempre) y un piano para tocar con el ratón.
 *
 *   fila de abajo:  Z S X D C V G B H N J M  (do a si) y , L .
 *   fila de arriba: Q 2 W 3 E R 5 T 6 Y 7 U  (una octava más) e I 9 O 0 P
 */
#ifndef ZXV_TRACKER_PIANO_H
#define ZXV_TRACKER_PIANO_H

#include <QWidget>

/* Los semitonos desde el do de la octava elegida, o -1 si la tecla no es
 * del piano. */
int pianoKey(int qtKey);

class Piano : public QWidget
{
    Q_OBJECT

public:
    explicit Piano(QWidget *parent = nullptr);
    void setOctave(int octave);         /* 1-8: el do de la izquierda */
    QSize sizeHint() const override;

signals:
    void noteOn(int note);              /* 0-95 */
    void noteOff();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private:
    int noteAt(QPoint p) const;
    void press(int note);

    int octave_ = 4;
    int down_ = -1;                     /* la nota que suena */
    int key_ = 0;                       /* la tecla que la toca */
};

#endif // ZXV_TRACKER_PIANO_H
