/*
 * png.cpp - Imágenes del PC a gráficos de ZX-V y al revés (ver png.h).
 */
#include "png.h"

#include <QPainter>
#include <map>

namespace gfx {

Image fromQImage(const QImage &src, int *inexact, const QColor &key)
{
    QImage q = src.convertToFormat(QImage::Format_ARGB32);
    Image im(q.width(), q.height(), TRANSP);
    std::map<QRgb, int> cache;
    int bad = 0;
    for (int y = 0; y < q.height(); y++) {
        const QRgb *line = reinterpret_cast<const QRgb *>(q.constScanLine(y));
        for (int x = 0; x < q.width(); x++) {
            QRgb p = line[x];
            auto hit = cache.find(p);
            if (hit == cache.end()) {
                int c = TRANSP;
                bool exact = true;
                QRgb rgb = p & 0xFFFFFF;
                if (qAlpha(p) >= 128 && !(key.isValid() && rgb == (key.rgb() & 0xFFFFFF))) {
                    long best = -1;
                    for (int i = 0; i < 16; i++) {
                        if (i == TRANSP)
                            continue;
                        uint32_t pal = paletteRgb(i);
                        long dr = long(qRed(p)) - (pal >> 16 & 255), dg = long(qGreen(p)) - (pal >> 8 & 255),
                             db = long(qBlue(p)) - (pal & 255), d = dr * dr + dg * dg + db * db;
                        if (best < 0 || d < best) {
                            best = d;
                            c = i;
                        }
                    }
                    exact = best == 0;
                }
                hit = cache.emplace(p, exact ? c : c | 0x100).first;
            }
            im.set(x, y, hit->second & 15);
            bad += hit->second >> 8;
        }
    }
    if (inexact)
        *inexact = bad;
    return im;
}

QImage toQImage(const Image &im, bool transp)
{
    QImage q(im.w, im.h, QImage::Format_ARGB32);
    for (int y = 0; y < im.h; y++) {
        QRgb *line = reinterpret_cast<QRgb *>(q.scanLine(y));
        for (int x = 0; x < im.w; x++) {
            int c = im.at(x, y);
            line[x] = transp && c == TRANSP ? 0 : 0xFF000000 | paletteRgb(c);
        }
    }
    return q;
}

static QRect whole(const Image &img, QRect r)
{
    return r.isNull() ? QRect(0, 0, img.w, img.h) : r;
}

QString Import::sprite(Document &doc, int n, const Image &img, QRect r)
{
    r = whole(img, r);
    if (r.width() < 1 || r.width() > MAX_SIDE || r.height() < 1 || r.height() > MAX_SIDE)
        return tr("Un sprite es de 1 a %1 píxeles de lado.").arg(MAX_SIDE);
    doc.sprites[n] = cropped(img, r.x(), r.y(), r.width(), r.height(), TRANSP);
    return QString();
}

QString Import::sheet(Document &doc, int first, const Image &img, int w, int h)
{
    if (w < 1 || h < 1 || w > MAX_SIDE || h > MAX_SIDE || img.w % w || img.h % h)
        return tr("La imagen (%1 x %2) no es múltiplo de %3 x %4.").arg(img.w).arg(img.h).arg(w).arg(h);
    int count = (img.w / w) * (img.h / h);
    if (first + count > SPRITES)
        return tr("Son %1 sprites: desde el %2 no caben (el último es el %3).").arg(count).arg(first).arg(SPRITES - 1);
    int n = first;
    for (int sy = 0; sy < img.h / h; sy++)
        for (int sx = 0; sx < img.w / w; sx++)
            doc.sprites[n++] = cropped(img, sx * w, sy * h, w, h, TRANSP);
    return QString();
}

/* Los tiles de un rectángulo, fila a fila. */
static std::vector<Image> cut(const Image &img, int side, QRect r)
{
    std::vector<Image> out;
    for (int ty = 0; ty < r.height() / side; ty++)
        for (int tx = 0; tx < r.width() / side; tx++)
            out.push_back(cropped(img, r.x() + tx * side, r.y() + ty * side, side, side, TRANSP));
    return out;
}

QString Import::tiles(Document &doc, int side, int first, const Image &img, QRect r)
{
    r = whole(img, r);
    if (r.width() % side || r.height() % side || r.isEmpty())
        return tr("%1 x %2 no es múltiplo de %3.").arg(r.width()).arg(r.height()).arg(side);
    std::vector<Image> got = cut(img, side, r);
    if (first + int(got.size()) > MAX_TILES)
        return tr("Más de %1 tiles.").arg(MAX_TILES);
    auto &set = doc.tiles(side);
    if (int(set.size()) < first + int(got.size()))
        set.resize(size_t(first) + got.size(), Image(side, side, 0));
    for (size_t i = 0; i < got.size(); i++)
        set[size_t(first) + i] = got[i];
    return QString();
}

QString Import::level(Document &doc, int m, int side, const Image &img, bool flip)
{
    if (img.w % side || img.h % side || img.w == 0)
        return tr("%1 x %2 no es múltiplo de %3.").arg(img.w).arg(img.h).arg(side);
    std::vector<Image> set = doc.tiles(side);
    /* El primero de cada tile repetido. */
    std::map<std::vector<uint8_t>, int> known;
    for (size_t i = set.size(); i-- > 0;)
        known[set[i].px] = int(i);
    Map map;
    map.w = img.w / side;
    map.h = img.h / side;
    map.tile = side;
    for (const Image &t : cut(img, side, QRect(0, 0, img.w, img.h))) {
        int cell = -1;
        std::vector<std::pair<Image, uint16_t>> variants = { { t, 0 } };
        if (flip) {
            variants.push_back({ flippedH(t), CELL_FLIP_H });
            variants.push_back({ flippedV(t), CELL_FLIP_V });
            variants.push_back({ flippedV(flippedH(t)), CELL_FLIP_H | CELL_FLIP_V });
        }
        for (auto &[v, bits] : variants) {
            auto hit = known.find(v.px);
            if (hit != known.end()) {
                cell = hit->second | bits;
                break;
            }
        }
        if (cell < 0) {
            if (set.size() >= size_t(MAX_TILES))
                return tr("Más de %1 tiles.").arg(MAX_TILES);
            cell = int(set.size());
            known[t.px] = cell;
            set.push_back(t);
        }
        for (uint8_t c : t.px)
            if (c == TRANSP) {
                cell |= CELL_TRANSP;
                break;
            }
        map.cells.push_back(uint16_t(cell));
    }
    doc.tiles(side) = set;
    doc.maps[m] = map;
    return QString();
}

QString Import::texture(Document &doc, int n, const Image &img, int out)
{
    bool pow2 = img.w == img.h && img.w >= 2 && img.w <= 256 && !(img.w & (img.w - 1));
    if (!pow2)
        return tr("Una textura es un cuadrado de lado potencia de 2, de 2 a 256 (esta es de %1 x %2).")
            .arg(img.w).arg(img.h);
    Texture t;
    t.img = img;
    for (uint8_t &c : t.img.px)
        if (c == TRANSP)
            c = 0;
    t.wrap = out < 0;
    t.out = out < 0 ? 0 : out;
    doc.textures[n] = t;
    return QString();
}

QImage tileSheet(const Document &doc, int side, int cols)
{
    const auto &set = doc.tiles(side);
    int rows = (int(set.size()) + cols - 1) / cols;
    QImage q(cols * side, std::max(rows, 1) * side, QImage::Format_ARGB32);
    q.fill(0);
    for (size_t i = 0; i < set.size(); i++) {
        QImage t = toQImage(set[i], false);
        for (int y = 0; y < side; y++)
            for (int x = 0; x < side; x++)
                q.setPixel(int(i % cols) * side + x, int(i / cols) * side + y, t.pixel(x, y));
    }
    return q;
}

QImage renderMap(const Document &doc, int m)
{
    const Map &map = doc.maps[m];
    int s = map.tile;
    QImage q(map.w * s, map.h * s, QImage::Format_ARGB32);
    q.fill(0xFFFF0000);
    const auto &set = doc.tiles(s);
    /* Cada tile con sus atributos, una vez. */
    std::map<uint16_t, QImage> cache;
    QPainter p(&q);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    for (int cy = 0; cy < map.h; cy++)
        for (int cx = 0; cx < map.w; cx++) {
            uint16_t v = map.at(cx, cy) & ~CELL_PRIO;
            if ((v & CELL_TILE) >= int(set.size()))
                continue;
            auto hit = cache.find(v);
            if (hit == cache.end()) {
                QImage t = toQImage(set[v & CELL_TILE], v & CELL_TRANSP);
                if (v & (CELL_FLIP_H | CELL_FLIP_V))
                    t = t.mirrored(v & CELL_FLIP_H, v & CELL_FLIP_V);
                hit = cache.emplace(v, t).first;
            }
            p.drawImage(cx * s, cy * s, hit->second);
        }
    return q;
}

} // namespace gfx
