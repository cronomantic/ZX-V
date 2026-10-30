/*
 * panels.h - Los paneles de zxv-gfx: la paleta, la lista de objetos, sus
 * propiedades, la vista previa y los tiles con los que se pintan los mapas.
 */
#ifndef ZXV_GFX_PANELS_H
#define ZXV_GFX_PANELS_H

#include <QListWidget>
#include <QWidget>

#include "editor.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTimer;

/* Un cuadro con el damero de los transparentes. */
QPixmap thumbnail(const QImage &img, int size, bool checker);

/* Los 16 colores: el botón izquierdo elige el color izquierdo y el derecho,
 * el derecho. */
class PalettePanel : public QWidget
{
    Q_OBJECT

public:
    explicit PalettePanel(Editor *ed, QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    QRect swatch(int c) const;
    Editor *ed_;
};

/* Los objetos de la clase actual, con su miniatura. */
class ObjectList : public QListWidget
{
    Q_OBJECT

public:
    explicit ObjectList(Editor *ed, QWidget *parent = nullptr);

signals:
    void createRequested();             /* doble clic en un hueco vacío */

private:
    void rebuild();
    void refresh();
    Editor *ed_;
    Section shown_ = Section::Maps;     /* para rehacer la lista al cambiar */
    int shownCount_ = -1;
};

/* El tamaño y lo demás del objeto elegido; crear y borrar. */
class PropertiesPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PropertiesPanel(Editor *ed, QWidget *parent = nullptr);

public slots:
    void createObject();
    void deleteObject();

private:
    void refresh();
    void applySize();
    Editor *ed_;
    QLabel *title_;
    QStackedWidget *pages_;
    QSpinBox *spriteW_, *spriteH_, *tilesN_, *mapW_, *mapH_;
    QComboBox *mapTile_, *texSide_, *texOut_;
    QCheckBox *mapWrap_, *texWrap_;
    QPushButton *create_, *delete_;
    bool updating_ = false;
};

/* El objeto a tamaño real: los sprites animados, los tiles repetidos. */
class PreviewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewPanel(Editor *ed, QWidget *parent = nullptr);

signals:
    void mode7Requested();

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    QRect area() const;
    Editor *ed_;
    QWidget *controls_;
    QCheckBox *play_;
    QSpinBox *frames_, *delay_, *scale_;
    QPushButton *mode7_;
    QTimer *timer_;
    int step_ = 0;
};

/* Los tiles del mapa, para elegir con cuál se pinta, y sus atributos. */
class TilePicker : public QWidget
{
    Q_OBJECT

public:
    explicit TilePicker(Editor *ed, QWidget *parent = nullptr);

signals:
    void editTile(int side, int index);  /* doble clic: editarlo */

private:
    void rebuild();
    Editor *ed_;
    QListWidget *list_;
    QCheckBox *flags_[4];
    bool updating_ = false;
};

#endif // ZXV_GFX_PANELS_H
