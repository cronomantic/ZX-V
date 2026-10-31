/*
 * png.h - Imágenes del PC (PNG y las que se pegan) a gráficos de ZX-V y al
 * revés, con las mismas reglas que sdk/tools/png2gfx.py: cada píxel toma el
 * color de ZX-V más cercano, y los de alfa < 128 (o del color clave) son el
 * transparente. Las pantallas guardan sus colores: si no son los de
 * siempre, con su paleta (D90).
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
 * uno de ellos. Con key, ese color también es el transparente. Con pal (16
 * colores 0xRRGGBB, los de una pantalla), el más cercano de esos, el 8
 * incluido. */
Image fromQImage(const QImage &q, int *inexact = nullptr, const QColor &key = QColor(),
                 const uint32_t *pal = nullptr);
/* Y al revés; con transp, el 8 es transparente (alfa 0). Con pal, esos
 * colores (los de una pantalla) en vez de los de siempre. */
QImage toQImage(const Image &im, bool transp, const uint32_t *pal = nullptr);
/* Una pantalla con sus colores (sin el borde). */
QImage screenImage(const Screen &s);

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
    /* La pantalla n: de 256 x 192 (modo 0) o 512 x 384 (modo 1), o el
     * cuadro con su borde (320 x 240 o 640 x 480). Con border < 0, el borde
     * es el color de la esquina del cuadro (o el 0, sin cuadro). Guarda los
     * colores de la imagen (lo transparente, y con key ese color, es el
     * negro): si son de los de siempre, sin paleta; si no, con su paleta (16
     * colores como mucho), como png2gfx.py. */
    static QString screen(Document &doc, int n, const QImage &img, int border, const QColor &key = QColor());
    /* Si la imagen es de pantalla (con su cuadro o sin él). */
    static bool isScreen(int w, int h);
    /* Cuántos colores tendría como pantalla: 0 si son de los de siempre (no
     * lleva paleta); con más de 16, no cabe. */
    static int screenColors(const QImage &img, int border, const QColor &key = QColor());
};

/* El juego de tiles de side, de cols tiles por fila. */
QImage tileSheet(const Document &doc, int side, int cols);
/* El mapa m, con sus tiles (los que faltan, en rojo). */
QImage renderMap(const Document &doc, int m);

} // namespace gfx

#endif // ZXV_GFX_PNG_H
