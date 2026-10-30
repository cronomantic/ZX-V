/*
 * canvas.h - Los lienzos de zxv-gfx: PixelCanvas para los sprites, los tiles
 * y las texturas, y MapCanvas para los mapas. Van dentro de un QScrollArea;
 * la rueda con Ctrl cambia el zoom y el botón central arrastra la vista.
 */
#ifndef ZXV_GFX_CANVAS_H
#define ZXV_GFX_CANVAS_H

#include <QImage>
#include <QWidget>

#include "editor.h"

class QScrollArea;

/* Lo común: el zoom, arrastrar la vista y el fondo a cuadros. */
class Canvas : public QWidget
{
    Q_OBJECT

public:
    Canvas(Editor *ed, QWidget *parent = nullptr);
    int zoom() const { return zoom_; }
    /* Cambia el zoom dejando quieto el punto at (del widget). */
    void setZoom(int zoom, QPoint at = QPoint(-1, -1));
    void zoomBy(int steps);
    /* El zoom con el que se ve entero en el sitio que hay. */
    void fit();
    bool grid() const { return grid_; }
    void setGrid(bool on);

signals:
    void zoomChanged(int zoom);
    /* Bajo el puntero: el punto (en píxeles o en celdas) y qué hay. */
    void hover(const QString &text);

protected:
    virtual QSize contentSize() const = 0;  /* en píxeles o en celdas por su lado */
    /* Lo que se dice cuando no hay nada que editar. */
    virtual QString emptyText() const = 0;
    void paintEmpty(QPainter &p);
    virtual int minZoom() const { return 1; }
    virtual int maxZoom() const { return 64; }
    QRect contentRect() const;               /* en el widget */
    QScrollArea *scrollArea() const;
    void updateSize();
    static void checker(QPainter &p, const QRect &r);
    void wheelEvent(QWheelEvent *e) override;
    void pan(QMouseEvent *e);               /* el botón central */
    void leaveEvent(QEvent *e) override;

    Editor *ed_;
    int zoom_ = 8;
    bool grid_ = true;
    QPoint panFrom_;
};

class PixelCanvas : public Canvas
{
    Q_OBJECT

public:
    PixelCanvas(Editor *ed, QWidget *parent = nullptr);

    /* La selección (en píxeles de la imagen) y lo flotante (lo pegado o lo
     * que se mueve, aún sin poner en la imagen). */
    bool hasSelection() const { return !sel_.isEmpty() || floating_; }
    QRect selection() const;
    void selectAll();
    void clearSelection();
    /* Lo seleccionado (o lo flotante, o toda la imagen). */
    gfx::Image selected() const;
    /* Vacía lo seleccionado (con el color de fondo). */
    void erase();
    /* Lo pega flotante, para moverlo con la herramienta de selección. */
    void paste(const gfx::Image &img);
    /* Pone lo flotante en la imagen, o lo tira. */
    void commit();
    void discard();
    /* Lo de la imagen entera que, con una selección, se hace solo con ella. */
    void transform(gfx::Image (*fn)(const gfx::Image &));

protected:
    QSize contentSize() const override;
    QString emptyText() const override;
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QPoint pixelAt(QPoint p) const;
    void apply(gfx::Image &im, QPoint a, QPoint b, int c) const;   /* la figura de la herramienta */
    void showHover(QPoint p);

    int button_ = -1;                   /* el botón que pinta (0 o 1), o -1 */
    QPoint start_, last_;
    gfx::Image preview_;                /* la imagen con la figura a medio trazar */
    QRect sel_;
    bool selecting_ = false;
    bool floating_ = false, moving_ = false;
    bool lifted_ = false;               /* lo flotante salió de la imagen (y no de pegar) */
    gfx::Image float_;
    QPoint floatPos_, grab_, liftFrom_;
};

class MapCanvas : public Canvas
{
    Q_OBJECT

public:
    MapCanvas(Editor *ed, QWidget *parent = nullptr);
    bool showFront() const { return front_; }
    void setShowFront(bool on);

protected:
    QSize contentSize() const override;
    QString emptyText() const override;
    int maxZoom() const override { return 8; }
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QPoint cellAt(QPoint p) const;
    uint16_t brush(int button) const;
    void showHover(QPoint p);

    bool front_ = true;                 /* marcar las celdas de delante */
    int button_ = -1;
    QPoint start_, last_;
};

/* Un tile con sus atributos, ya en colores del PC. */
QImage tileImage(const gfx::Document &doc, int side, uint16_t cell);

#endif // ZXV_GFX_CANVAS_H
