/*
 * ayresult.h - Lo que suena de verdad al tocar una nota con un instrumento
 * del AY (un «sample» del PT3): el volumen y el tono, paso a paso, como los
 * calcula el reproductor (sdk/music). Y las plantillas, instrumentos hechos
 * para empezar.
 */
#ifndef ZXV_TRACKER_AYRESULT_H
#define ZXV_TRACKER_AYRESULT_H

#include <QString>
#include <QWidget>

#include <utility>
#include <vector>

#include "song.h"

class Editor;

class AyResult : public QWidget
{
    Q_OBJECT

public:
    explicit AyResult(Editor *ed, QWidget *parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /* Instrumentos hechos, para empezar: su nombre y sus líneas. */
    static std::vector<std::pair<QString, trk::Sample>> templates();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    struct Step {
        int vol;                        /* 0-15, o 16 con la envolvente */
        double semis;                   /* el tono, en semitonos desde el de la nota */
        bool tone, noise;
    };
    void compute();

    Editor *ed_;
    std::vector<Step> steps_;
};

#endif // ZXV_TRACKER_AYRESULT_H
