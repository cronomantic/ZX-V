/*
 * doc.h - Los gráficos de zxv-gfx: sprites, tiles, mapas y texturas, como
 * los del BASIC de ZX-V, y su fichero .gfx (LOAD "x" GRAPHICS). Sin Qt: lo
 * usan también las pruebas.
 *
 * Los colores son los 16 de ZX-V (0-15); el 8 es el transparente de los
 * sprites y de las celdas de mapa con ZXV_CELL_TRANSP.
 */
#ifndef ZXV_GFX_DOC_H
#define ZXV_GFX_DOC_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gfx {

constexpr int TRANSP = 8;
constexpr int SPRITES = 256, MAPS = 16, TEXTURES = 4, MAX_TILES = 4096;
constexpr int MAX_SIDE = 1024;          /* lo más grande de un sprite */

/* Los atributos de una celda (como en zxv_gfx.h). */
constexpr uint16_t CELL_TILE = 0x0FFF, CELL_FLIP_H = 0x1000, CELL_FLIP_V = 0x2000,
                   CELL_TRANSP = 0x4000, CELL_PRIO = 0x8000;

/* El color i de ZX-V en 0xRRGGBB: BRIGHT-G-R-B, con 0xD7 sin brillo. */
uint32_t paletteRgb(int i);

/* Un bitmap: un píxel por byte. Vacío (w == 0) es que no existe. */
struct Image {
    int w = 0, h = 0;
    std::vector<uint8_t> px;

    Image() = default;
    Image(int width, int height, int fill) : w(width), h(height), px(size_t(width) * height, uint8_t(fill)) {}
    bool empty() const { return w == 0; }
    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
    int at(int x, int y) const { return px[size_t(y) * w + x]; }
    void set(int x, int y, int c) { if (inside(x, y)) px[size_t(y) * w + x] = uint8_t(c); }
    bool operator==(const Image &o) const { return w == o.w && h == o.h && px == o.px; }
};

/* Lo de arriba a la izquierda se queda; lo nuevo, del color fill. */
Image resized(const Image &im, int w, int h, int fill);
Image flippedH(const Image &im);
Image flippedV(const Image &im);
Image rotatedRight(const Image &im);
Image rotatedLeft(const Image &im);
/* Lo que sale por un lado entra por el otro. */
Image shifted(const Image &im, int dx, int dy);
Image cropped(const Image &im, int x, int y, int w, int h, int fill);
/* Pega src en (x, y); con transp, sin sus píxeles del 8. */
void paste(Image &dst, const Image &src, int x, int y, bool transp);

/* Herramientas de dibujo. */
void drawLine(Image &im, int x0, int y0, int x1, int y1, int c);
void drawRect(Image &im, int x0, int y0, int x1, int y1, int c, bool fill);
void drawEllipse(Image &im, int x0, int y0, int x1, int y1, int c, bool fill);
void floodFill(Image &im, int x, int y, int c);

struct Map {
    int w = 0, h = 0;                   /* en celdas; 0 es que no existe */
    int tile = 8;                       /* el lado de los tiles: 8 o 16 */
    bool wrap = false;
    std::vector<uint16_t> cells;

    bool empty() const { return w == 0; }
    uint16_t at(int x, int y) const { return cells[size_t(y) * w + x]; }
    void set(int x, int y, uint16_t v) { if (x >= 0 && y >= 0 && x < w && y < h) cells[size_t(y) * w + x] = v; }
    bool operator==(const Map &o) const
    {
        return w == o.w && h == o.h && tile == o.tile && wrap == o.wrap && cells == o.cells;
    }
};
Map resizedMap(const Map &m, int w, int h);

struct Texture {
    Image img;                          /* cuadrada, de lado potencia de 2 (2-256) */
    bool wrap = true;
    int out = 0;                        /* el color de fuera, si no se repite */

    bool empty() const { return img.empty(); }
    bool operator==(const Texture &o) const { return img == o.img && wrap == o.wrap && out == o.out; }
};

enum { LOAD_OK, LOAD_NOT_GFX, LOAD_DAMAGED, LOAD_UNKNOWN };

struct Document {
    std::array<Image, SPRITES> sprites;
    std::vector<Image> tiles8, tiles16;
    std::array<Map, MAPS> maps;
    std::array<Texture, TEXTURES> textures;

    std::vector<Image> &tiles(int side) { return side == 8 ? tiles8 : tiles16; }
    const std::vector<Image> &tiles(int side) const { return side == 8 ? tiles8 : tiles16; }
    bool operator==(const Document &o) const
    {
        return sprites == o.sprites && tiles8 == o.tiles8 && tiles16 == o.tiles16 && maps == o.maps &&
               textures == o.textures;
    }

    /* El fichero .gfx. load añade lo del fichero (sustituye lo del mismo
     * número, como LOAD ... GRAPHICS) y da LOAD_OK o qué está mal; si falla,
     * no cambia nada. */
    int load(const std::vector<uint8_t> &data);
    std::vector<uint8_t> save() const;
    /* Lo que ocupa en el almacén del BASIC de la máquina (aproximado). */
    size_t machineBytes() const;
};

} // namespace gfx

#endif // ZXV_GFX_DOC_H
