/*
 * editor.h - Lo que se edita en zxv-gfx: el documento, el objeto elegido,
 * los colores y la herramienta, deshacer y rehacer, y el fichero.
 *
 * Cada cambio empieza con begin() (guarda cómo estaba el documento para
 * deshacerlo) y acaba con changed().
 */
#ifndef ZXV_GFX_EDITOR_H
#define ZXV_GFX_EDITOR_H

#include <QObject>
#include <QString>
#include <deque>

#include "doc.h"

/* Las clases de objeto, como las pestañas. */
enum class Section { Sprites, Tiles8, Tiles16, Maps, Textures };

enum class Tool { Pen, Line, Rect, FilledRect, Ellipse, FilledEllipse, Fill, Pick, Select };

class Editor : public QObject
{
    Q_OBJECT

public:
    explicit Editor(QObject *parent = nullptr);

    gfx::Document &doc() { return doc_; }
    const gfx::Document &doc() const { return doc_; }

    Section section() const { return section_; }
    int index() const { return index_[int(section_)]; }
    int index(Section s) const { return index_[int(s)]; }
    void select(Section s, int index);
    void select(int index) { select(section_, index); }
    /* Cuántos hay de la clase actual (los huecos de sprites, mapas y
     * texturas, o los tiles que hay). */
    int count(Section s) const;
    int tileSide() const { return section_ == Section::Tiles16 ? 16 : 8; }
    bool isTiles() const { return section_ == Section::Tiles8 || section_ == Section::Tiles16; }

    /* La imagen elegida (un sprite, un tile o una textura), o nullptr. */
    gfx::Image *image();
    gfx::Map *map();
    /* Si la imagen es de las que tienen el 8 transparente. */
    bool transparentImage() const { return section_ == Section::Sprites; }
    /* El color con el que se vacía y se amplía la imagen actual. */
    int background() const { return section_ == Section::Sprites ? gfx::TRANSP : 0; }

    int color(int button) const { return color_[button]; }   /* 0 izquierdo, 1 derecho */
    void setColor(int button, int c);
    Tool tool() const { return tool_; }
    void setTool(Tool t);
    /* Con lo que se pintan los mapas: el tile y sus atributos (CELL_...). */
    int mapTile() const { return mapTile_; }
    int mapFlags() const { return mapFlags_; }
    void setMapTile(int t);
    void setMapFlags(int f);

    /* Deshacer y rehacer (el documento entero). */
    void begin();
    void changed();
    /* Deshace lo empezado con begin() si al final no se cambió nada. */
    void cancel();
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }
    void undo();
    void redo();

    QString path() const { return path_; }
    bool modified() const { return modified_; }
    void setDocument(const gfx::Document &doc, const QString &path);
    void setSaved(const QString &path);

signals:
    void documentChanged();
    void selectionChanged();
    void colorsChanged();
    void toolChanged();
    void brushChanged();                /* el tile o los atributos de los mapas */
    void stateChanged();                /* el fichero, modificado, deshacer */

private:
    gfx::Document doc_;
    Section section_ = Section::Sprites;
    int index_[5] = {};
    int color_[2] = { 0, gfx::TRANSP };
    Tool tool_ = Tool::Pen;
    int mapTile_ = 0, mapFlags_ = 0;
    std::deque<gfx::Document> undo_, redo_;
    QString path_;
    bool modified_ = false;
};

#endif // ZXV_GFX_EDITOR_H
