/*
 * png.h - Imágenes del PC (PNG y las que se pegan) a gráficos de ZX-V y al
 * revés, con las mismas reglas que sdk/tools/png2gfx.py: cada píxel toma el
 * color de ZX-V más cercano, y los de alfa < 128 (o del color clave) son el
 * transparente.
 */
#ifndef ZXV_GFX_PNG_H
#define ZXV_GFX_PNG_H

#include <QCoreApplication>
#include <QImage>
#include <QRect>
#include <QString>

#include "doc.h"

namespace gfx {

/* Una imagen del PC en colores de ZX-V; inexact, los píxeles que no eran
 * uno de ellos. Con key, ese color también es el transparente. */
Image fromQImage(const QImage &q, int *inexact = nullptr, const QColor &key = QColor());
/* Y al revés; con transp, el 8 es transparente (alfa 0). */
QImage toQImage(const Image &im, bool transp);

/* Lo que se importa. Dan "" o qué está mal (y entonces no cambian nada). */
class Import
{
    Q_DECLARE_TR_FUNCTIONS(Import)

public:
    /* El sprite n: la imagen entera, o el rectángulo r. */
    static QString sprite(Document &doc, int n, const Image &img, QRect r = QRect());
    /* Una hoja: los sprites first, first + 1... de w x h, fila a fila. */
    static QString sheet(Document &doc, int first, const Image &img, int w, int h);
    /* La imagen (o el rectángulo r) en tiles de side, numerados desde first. */
    static QString tiles(Document &doc, int side, int first, const Image &img, QRect r = QRect());
    /* Un nivel: la imagen en tiles de side (los repetidos se aprovechan; con
     * flip, también los espejados) y el mapa m con su distribución. */
    static QString level(Document &doc, int m, int side, const Image &img, bool flip);
    /* La textura n; con out < 0 se repite, si no fuera es de ese color. */
    static QString texture(Document &doc, int n, const Image &img, int out);
};

/* El juego de tiles de side, de cols tiles por fila. */
QImage tileSheet(const Document &doc, int side, int cols);
/* El mapa m, con sus tiles (los que faltan, en rojo). */
QImage renderMap(const Document &doc, int m);

} // namespace gfx

#endif // ZXV_GFX_PNG_H
