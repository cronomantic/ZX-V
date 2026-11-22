/*
 * waveview.h - Una muestra de los canales de muestras, a la vista: su onda
 * entera de izquierda a derecha y, en otro color, lo que se repite (el
 * bucle). Un clic (o arrastrar) pone dónde empieza el bucle.
 */
#ifndef ZXV_TRACKER_WAVEVIEW_H
#define ZXV_TRACKER_WAVEVIEW_H

#include <QWidget>

#include <cstdint>
#include <vector>

class Editor;

class WaveView : public QWidget
{
    Q_OBJECT

public:
    explicit WaveView(Editor *ed, QWidget *parent = nullptr);
    void setWave(int w);
    int wave() const { return wave_; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    /* Dónde empieza el bucle: start, al pulsar (lo de después, arrastrando). */
    void loopChanged(uint32_t from, bool start);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

private:
    void reload();
    uint32_t sampleAt(int x) const;

    Editor *ed_;
    int wave_ = -1;
    std::vector<int16_t> samples_;
};

#endif // ZXV_TRACKER_WAVEVIEW_H
