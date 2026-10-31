/*
 * doc.h - Los gráficos de zxv-gfx: sprites, tiles, mapas, texturas y
 * pantallas, como los del BASIC de ZX-V, y su fichero .gfx (LOAD "x"
 * GRAPHICS, SAVE "x" SCREEN$). Sin Qt: lo usan también las pruebas.
 *
 * Los colores son los 16 de ZX-V (0-15); el 8 es el transparente de los
 * sprites y de las celdas de mapa con ZXV_CELL_TRANSP (en las texturas y
 * en las pantallas es un negro más, como el 0). Una pantalla puede llevar
 * su paleta: el color RGB565 de cada índice (D90).
 */
#ifndef ZXV_GFX_DOC_H
#define ZXV_GFX_DOC_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gfx {

constexpr int TRANSP = 8;
constexpr int SPRITES = 256, MAPS = 16, TEXTURES = 4, SCREENS = 256, MAX_TILES = 4096;
constexpr int MAX_SIDE = 1024;          /* lo más grande de un sprite */

/* Los atributos de una celda (como en zxv_gfx.h). */
constexpr uint16_t CELL_TILE = 0x0FFF, CELL_FLIP_H = 0x1000, CELL_FLIP_V = 0x2000,
                   CELL_TRANSP = 0x4000, CELL_PRIO = 0x8000;

/* El color i de ZX-V en 0xRRGGBB: BRIGHT-G-R-B, con 0xD7 sin brillo. */
uint32_t paletteRgb(int i);

/* RGB565 (spec §4.1): el color de siempre del índice i (ZXV_STD_RGB565); uno
 * de 8 bits por canal, con la curva de ZXV_RGB; y al revés, en 0xRRGGBB, con
 * la curva de la salida (la del emulador: los de siempre dan D7 y FF). */
uint16_t stdRgb565(int i);
uint16_t toRgb565(int r, int g, int b);
uint32_t rgb565ToRgb(uint16_t c);
using Palette = std::array<uint16_t, 16>;
/* Los 16 colores de siempre en RGB565. */
Palette standardPalette();
/* La paleta de una pantalla con estos colores RGB565 (distintos, en el orden
 * en que aparecen), como png2gfx.py: 0 si todos son de los de siempre (y pal,
 * los de siempre); 1 con la paleta en pal: los de siempre en su índice (el
 * negro en el 0), los demás en los índices libres por orden (el 8 el último)
 * y los que no se usan, de siempre; o -1 si son más de 16. */
int screenPalette(const std::vector<uint16_t> &colors, Palette &pal);

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

/* Una pantalla: la página entera de la VRAM, en modo 0 (256 x 192) o en
 * modo 1 (512 x 384), con el color del borde. Su modo no cambia. Con
 * paleta (D90), cada índice se ve con su color de palette; sin ella, con
 * los de siempre (y palette son los de siempre). */
struct Screen {
    Image img;
    int border = 0;
    bool hasPalette = false;
    Palette palette = standardPalette();

    static int width(int mode) { return mode ? 512 : 256; }
    static int height(int mode) { return mode ? 384 : 192; }
    bool empty() const { return img.empty(); }
    int mode() const { return img.w == width(1) ? 1 : 0; }
    /* El color del índice i en 0xRRGGBB, y los 16. */
    uint32_t rgb(int i) const { return hasPalette ? rgb565ToRgb(palette[i & 15]) : paletteRgb(i & 15); }
    std::array<uint32_t, 16> rgbs() const;
    /* Sin paleta: los colores de siempre. */
    void removePalette();
    bool operator==(const Screen &o) const
    {
        return img == o.img && border == o.border && hasPalette == o.hasPalette && palette == o.palette;
    }
};
/* Una pantalla nueva, toda negra. */
Screen newScreen(int mode, int border);

enum { LOAD_OK, LOAD_NOT_GFX, LOAD_DAMAGED, LOAD_UNKNOWN };

struct Document {
    std::array<Image, SPRITES> sprites;
    std::vector<Image> tiles8, tiles16;
    std::array<Map, MAPS> maps;
    std::array<Texture, TEXTURES> textures;
    std::array<Screen, SCREENS> screens;

    std::vector<Image> &tiles(int side) { return side == 8 ? tiles8 : tiles16; }
    const std::vector<Image> &tiles(int side) const { return side == 8 ? tiles8 : tiles16; }
    bool operator==(const Document &o) const
    {
        return sprites == o.sprites && tiles8 == o.tiles8 && tiles16 == o.tiles16 && maps == o.maps &&
               textures == o.textures && screens == o.screens;
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
