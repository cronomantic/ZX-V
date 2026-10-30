/*
 * lines.h - Los editores de líneas de zxv-tracker: los samples, los
 * ornamentos y los efectos AYFX. Una línea por paso, con columnas que se
 * escriben con el teclado, interruptores que se cambian con un clic y una
 * barra de volumen que se pinta arrastrando el ratón.
 */
#ifndef ZXV_TRACKER_LINES_H
#define ZXV_TRACKER_LINES_H

#include <QWidget>

#include <vector>

class Editor;

class LineGrid : public QWidget
{
    Q_OBJECT

public:
    explicit LineGrid(QWidget *parent = nullptr);
    int line() const { return line_; }
    void setLine(int l);
    void insertLine();
    void removeLine();
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void cursorMoved();
    void activated();               /* Intro */

protected:
    enum Kind { Toggle, Sign, Digit, Value, Bar };
    struct Stop {
        int x, w;
        Kind kind;
        int id;                         /* la columna, para la subclase */
    };

    virtual int count() const = 0;
    virtual int loopLine() const { return -1; }
    virtual void drawLine(QPainter &p, int line, int y) = 0;
    /* Escribe en la columna (id) de la línea; false si la tecla no vale. */
    virtual bool type(int line, int id, QChar c) = 0;
    virtual void toggle(int line, int id) = 0;
    virtual int bar(int line) const = 0;
    virtual void setBar(int line, int v) = 0;
    virtual void begin() = 0;           /* antes de cambiar (deshacer) */
    virtual void end() = 0;             /* después */
    virtual void insertAt(int line) = 0;
    virtual void removeAt(int line) = 0;

    void put(QPainter &p, int x, int y, const QString &s, const QColor &c);
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void resizeEvent(QResizeEvent *) override { scrollToCursor(); }
    void focusInEvent(QFocusEvent *) override { update(); }
    void focusOutEvent(QFocusEvent *) override { update(); }

    std::vector<Stop> stops_;
    std::vector<std::pair<int, QString>> header_;
    int barX_ = -1;                     /* la barra del volumen: 15 caracteres desde aquí */
    int numW_ = 2;                      /* las cifras del número de línea */
    int cw_ = 8, lh_ = 16, top_ = 0;

    static const QColor Dim, Text, On, Hi;

private:
    int lineAt(int y) const;
    int barValue(double cx) const;
    int stopAt(int x) const;
    void scrollToCursor();
    void move(int line, int stop);

    int line_ = 0, stop_ = 0, first_ = 0;
    bool dragging_ = false;
};

/* Un sample: tono, ruido y envolvente, sus desplazamientos y el volumen. */
class SampleView : public LineGrid
{
    Q_OBJECT
public:
    explicit SampleView(Editor *ed, QWidget *parent = nullptr);

protected:
    int count() const override;
    int loopLine() const override;
    void drawLine(QPainter &p, int line, int y) override;
    bool type(int line, int id, QChar c) override;
    void toggle(int line, int id) override;
    int bar(int line) const override;
    void setBar(int line, int v) override;
    void begin() override;
    void end() override;
    void insertAt(int line) override;
    void removeAt(int line) override;

private:
    Editor *ed_;
};

/* Un ornamento: un desplazamiento de nota por paso. */
class OrnamentView : public LineGrid
{
    Q_OBJECT
public:
    explicit OrnamentView(Editor *ed, QWidget *parent = nullptr);

protected:
    int count() const override;
    int loopLine() const override;
    void drawLine(QPainter &p, int line, int y) override;
    bool type(int line, int id, QChar c) override;
    void toggle(int line, int id) override;
    int bar(int line) const override;
    void setBar(int line, int v) override;
    void begin() override;
    void end() override;
    void insertAt(int line) override;
    void removeAt(int line) override;

private:
    Editor *ed_;
};

/* Un efecto AYFX: tono, ruido, sus periodos y el volumen de cada frame. */
class SfxView : public LineGrid
{
    Q_OBJECT
public:
    explicit SfxView(Editor *ed, QWidget *parent = nullptr);

protected:
    int count() const override;
    void drawLine(QPainter &p, int line, int y) override;
    bool type(int line, int id, QChar c) override;
    void toggle(int line, int id) override;
    int bar(int line) const override;
    void setBar(int line, int v) override;
    void begin() override;
    void end() override;
    void insertAt(int line) override;
    void removeAt(int line) override;

private:
    Editor *ed_;
};

#endif // ZXV_TRACKER_LINES_H
