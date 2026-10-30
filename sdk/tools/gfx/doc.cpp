/*
 * doc.cpp - Los gráficos de zxv-gfx, sus operaciones y el fichero .gfx.
 */
#include "doc.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace gfx {

uint32_t paletteRgb(int i)
{
    uint32_t lvl = i & 8 ? 0xFF : 0xD7;
    return (i & 2 ? lvl << 16 : 0) | (i & 4 ? lvl << 8 : 0) | (i & 1 ? lvl : 0);
}

/* --- Imágenes --------------------------------------------------------------- */

Image resized(const Image &im, int w, int h, int fill)
{
    Image out(w, h, fill);
    for (int y = 0; y < std::min(h, im.h); y++)
        for (int x = 0; x < std::min(w, im.w); x++)
            out.set(x, y, im.at(x, y));
    return out;
}

Image flippedH(const Image &im)
{
    Image out(im.w, im.h, 0);
    for (int y = 0; y < im.h; y++)
        for (int x = 0; x < im.w; x++)
            out.set(im.w - 1 - x, y, im.at(x, y));
    return out;
}

Image flippedV(const Image &im)
{
    Image out(im.w, im.h, 0);
    for (int y = 0; y < im.h; y++)
        for (int x = 0; x < im.w; x++)
            out.set(x, im.h - 1 - y, im.at(x, y));
    return out;
}

Image rotatedRight(const Image &im)
{
    Image out(im.h, im.w, 0);
    for (int y = 0; y < im.h; y++)
        for (int x = 0; x < im.w; x++)
            out.set(im.h - 1 - y, x, im.at(x, y));
    return out;
}

Image rotatedLeft(const Image &im)
{
    Image out(im.h, im.w, 0);
    for (int y = 0; y < im.h; y++)
        for (int x = 0; x < im.w; x++)
            out.set(y, im.w - 1 - x, im.at(x, y));
    return out;
}

Image shifted(const Image &im, int dx, int dy)
{
    Image out(im.w, im.h, 0);
    if (im.empty())
        return out;
    for (int y = 0; y < im.h; y++)
        for (int x = 0; x < im.w; x++)
            out.set(((x + dx) % im.w + im.w) % im.w, ((y + dy) % im.h + im.h) % im.h, im.at(x, y));
    return out;
}

Image cropped(const Image &im, int x0, int y0, int w, int h, int fill)
{
    Image out(w, h, fill);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (im.inside(x0 + x, y0 + y))
                out.set(x, y, im.at(x0 + x, y0 + y));
    return out;
}

void paste(Image &dst, const Image &src, int x0, int y0, bool transp)
{
    for (int y = 0; y < src.h; y++)
        for (int x = 0; x < src.w; x++) {
            int c = src.at(x, y);
            if (!transp || c != TRANSP)
                dst.set(x0 + x, y0 + y, c);
        }
}

void drawLine(Image &im, int x0, int y0, int x1, int y1, int c)
{
    int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (;;) {
        im.set(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void drawRect(Image &im, int x0, int y0, int x1, int y1, int c, bool fill)
{
    if (x0 > x1)
        std::swap(x0, x1);
    if (y0 > y1)
        std::swap(y0, y1);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            if (fill || y == y0 || y == y1 || x == x0 || x == x1)
                im.set(x, y, c);
}

/* La elipse inscrita en el rectángulo (algoritmo del punto medio, con
 * números enteros, que vale también para lados pares). */
void drawEllipse(Image &im, int x0, int y0, int x1, int y1, int c, bool fill)
{
    if (x0 > x1)
        std::swap(x0, x1);
    if (y0 > y1)
        std::swap(y0, y1);
    long a = x1 - x0, b = y1 - y0, b1 = b & 1;
    long dx = 4 * (1 - a) * b * b, dy = 4 * (b1 + 1) * a * a, err = dx + dy + b1 * a * a, e2;
    y0 += int((b + 1) / 2);
    y1 = y0 - int(b1);
    long aa = 8 * a * a, bb = 8 * b * b;
    auto span = [&](int xa, int xb, int y) {
        if (fill) {
            for (int x = xa; x <= xb; x++)
                im.set(x, y, c);
        } else {
            im.set(xa, y, c);
            im.set(xb, y, c);
        }
    };
    do {
        span(x0, x1, y0);
        span(x0, x1, y1);
        e2 = 2 * err;
        if (e2 <= dy) {
            y0++;
            y1--;
            err += dy += aa;
        }
        if (e2 >= dx || 2 * err > dy) {
            x0++;
            x1--;
            err += dx += bb;
        }
    } while (x0 <= x1);
    while (y0 - y1 <= b) {
        span(x0 - 1, x1 + 1, y0++);
        span(x0 - 1, x1 + 1, y1--);
    }
}

void floodFill(Image &im, int x, int y, int c)
{
    if (!im.inside(x, y))
        return;
    int from = im.at(x, y);
    if (from == c)
        return;
    std::vector<std::pair<int, int>> stack{ { x, y } };
    while (!stack.empty()) {
        auto [px, py] = stack.back();
        stack.pop_back();
        if (im.at(px, py) != from)
            continue;
        int l = px, r = px;
        while (l > 0 && im.at(l - 1, py) == from)
            l--;
        while (r < im.w - 1 && im.at(r + 1, py) == from)
            r++;
        for (int i = l; i <= r; i++)
            im.set(i, py, c);
        for (int ny : { py - 1, py + 1 }) {
            if (ny < 0 || ny >= im.h)
                continue;
            for (int i = l; i <= r; i++)
                if (im.at(i, ny) == from && (i == l || im.at(i - 1, ny) != from))
                    stack.push_back({ i, ny });
        }
    }
}

Map resizedMap(const Map &m, int w, int h)
{
    Map out = m;
    out.w = w;
    out.h = h;
    out.cells.assign(size_t(w) * h, 0);
    for (int y = 0; y < std::min(h, m.h); y++)
        for (int x = 0; x < std::min(w, m.w); x++)
            out.cells[size_t(y) * w + x] = m.at(x, y);
    return out;
}

/* --- El fichero .gfx ------------------------------------------------------------
 *
 * "ZXVG", versión (1), 0, número de objetos (u16); cada objeto: tipo (u8), 0,
 * número (u16), longitud (u32) y los datos, en little-endian, con los píxeles
 * de 4 bits (nibble bajo = píxel par):
 *   1 sprite   ancho, alto (u16) y las filas de (ancho + 1) / 2 bytes
 *   2 tiles    (el número es el lado) cuántos (u16), 0 (u16) y los tiles
 *   3 mapa     ancho, alto (u16), lado, repetición (u8), 0 (u16), las celdas
 *   4 textura  log2 del lado, repetición, color de fuera, 0 y los texels
 * Es el formato de rom/src/basic/bgfx.c y de sdk/tools/png2gfx.py.
 */

namespace {

uint32_t get16(const uint8_t *p) { return p[0] | uint32_t(p[1]) << 8; }
uint32_t get32(const uint8_t *p) { return get16(p) | get16(p + 2) << 16; }

void put16(std::vector<uint8_t> &o, uint32_t v)
{
    o.push_back(uint8_t(v));
    o.push_back(uint8_t(v >> 8));
}

void put32(std::vector<uint8_t> &o, uint32_t v)
{
    put16(o, v & 0xFFFF);
    put16(o, v >> 16);
}

/* Las filas de 4 bits de un bitmap (el píxel de relleno, transparente). */
Image unpack(const uint8_t *d, int w, int h, int rowBytes)
{
    Image im(w, h, 0);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            im.set(x, y, d[y * rowBytes + x / 2] >> (4 * (x & 1)) & 15);
    return im;
}

void pack(std::vector<uint8_t> &o, const Image &im)
{
    for (int y = 0; y < im.h; y++)
        for (int x = 0; x < im.w; x += 2)
            o.push_back(uint8_t(im.at(x, y) | (x + 1 < im.w ? im.at(x + 1, y) : TRANSP) << 4));
}

} // namespace

int Document::load(const std::vector<uint8_t> &data)
{
    const uint8_t *d = data.data();
    size_t n = data.size();
    if (n < 8 || std::memcmp(d, "ZXVG", 4) || d[4] != 1)
        return LOAD_NOT_GFX;
    Document doc = *this;
    uint32_t count = get16(d + 6);
    size_t pos = 8;
    for (uint32_t i = 0; i < count; i++) {
        if (n - pos < 8)
            return LOAD_DAMAGED;
        int kind = d[pos], num = int(get16(d + pos + 2));
        uint32_t len = get32(d + pos + 4);
        const uint8_t *o = d + pos + 8;
        if (d[pos + 1] || len > n - pos - 8)
            return LOAD_DAMAGED;
        pos += 8 + len;
        switch (kind) {
        case 1: {
            uint32_t w = len >= 4 ? get16(o) : 0, h = len >= 4 ? get16(o + 2) : 0;
            if (num >= SPRITES || w < 1 || w > MAX_SIDE || h < 1 || h > MAX_SIDE || len != 4 + h * ((w + 1) / 2))
                return LOAD_DAMAGED;
            doc.sprites[num] = unpack(o + 4, int(w), int(h), int((w + 1) / 2));
            break;
        }
        case 2: {
            uint32_t c = len >= 4 ? get16(o) : 0;
            if ((num != 8 && num != 16) || c < 1 || c > MAX_TILES || len != 4 + c * num * num / 2)
                return LOAD_DAMAGED;
            auto &t = doc.tiles(num);
            t.clear();
            for (uint32_t k = 0; k < c; k++)
                t.push_back(unpack(o + 4 + k * num * num / 2, num, num, num / 2));
            break;
        }
        case 3: {
            uint32_t w = len >= 8 ? get16(o) : 0, h = len >= 8 ? get16(o + 2) : 0;
            if (num >= MAPS || w < 1 || w > 32767 || h < 1 || h > 32767 || (o[4] != 8 && o[4] != 16) || o[5] > 1 ||
                len != 8 + w * h * 2)
                return LOAD_DAMAGED;
            Map m;
            m.w = int(w);
            m.h = int(h);
            m.tile = o[4];
            m.wrap = o[5];
            m.cells.resize(size_t(w) * h);
            for (size_t k = 0; k < m.cells.size(); k++)
                m.cells[k] = uint16_t(get16(o + 8 + 2 * k));
            doc.maps[num] = m;
            break;
        }
        case 4: {
            uint32_t l2 = len >= 4 ? o[0] : 0;
            if (num >= TEXTURES || l2 < 1 || l2 > 8 || o[1] > 1 || o[2] > 15 || len != 4 + (1u << (2 * l2)))
                return LOAD_DAMAGED;
            int side = 1 << l2;
            Texture t;
            t.img = Image(side, side, 0);
            for (int k = 0; k < side * side; k++) {
                if (o[4 + k] > 15)
                    return LOAD_DAMAGED;
                t.img.px[k] = o[4 + k];
            }
            t.wrap = o[1];
            t.out = o[2];
            doc.textures[num] = t;
            break;
        }
        default:
            return LOAD_UNKNOWN;
        }
    }
    *this = doc;
    return LOAD_OK;
}

std::vector<uint8_t> Document::save() const
{
    std::vector<uint8_t> o = { 'Z', 'X', 'V', 'G', 1, 0, 0, 0 };
    int count = 0;
    auto record = [&](int kind, int num, size_t len) {
        o.push_back(uint8_t(kind));
        o.push_back(0);
        put16(o, uint32_t(num));
        put32(o, uint32_t(len));
        count++;
    };
    /* En el orden de png2gfx.py: sprites, tiles de 8 y de 16, mapas y texturas. */
    for (int n = 0; n < SPRITES; n++) {
        const Image &s = sprites[n];
        if (s.empty())
            continue;
        record(1, n, 4 + size_t(s.h) * ((s.w + 1) / 2));
        put16(o, uint32_t(s.w));
        put16(o, uint32_t(s.h));
        pack(o, s);
    }
    for (int side : { 8, 16 }) {
        const auto &t = tiles(side);
        if (t.empty())
            continue;
        record(2, side, 4 + t.size() * side * side / 2);
        put16(o, uint32_t(t.size()));
        put16(o, 0);
        for (const Image &im : t)
            pack(o, im);
    }
    for (int n = 0; n < MAPS; n++) {
        const Map &m = maps[n];
        if (m.empty())
            continue;
        record(3, n, 8 + m.cells.size() * 2);
        put16(o, uint32_t(m.w));
        put16(o, uint32_t(m.h));
        o.push_back(uint8_t(m.tile));
        o.push_back(m.wrap ? 1 : 0);
        put16(o, 0);
        for (uint16_t c : m.cells)
            put16(o, c);
    }
    for (int n = 0; n < TEXTURES; n++) {
        const Texture &t = textures[n];
        if (t.empty())
            continue;
        int l2 = 0;
        while ((1 << l2) < t.img.w)
            l2++;
        record(4, n, 4 + t.img.px.size());
        o.push_back(uint8_t(l2));
        o.push_back(t.wrap ? 1 : 0);
        o.push_back(uint8_t(t.out));
        o.push_back(0);
        o.insert(o.end(), t.img.px.begin(), t.img.px.end());
    }
    o[6] = uint8_t(count);
    o[7] = uint8_t(count >> 8);
    return o;
}

size_t Document::machineBytes() const
{
    size_t total = 0;
    for (const Image &s : sprites)
        if (!s.empty())
            total += 24 + size_t(s.h) * ((s.w + 7) / 8) * 8;     /* preparado para el motor */
    total += tiles8.size() * 32 + tiles16.size() * 128;
    for (const Map &m : maps)
        if (!m.empty())
            total += 16 + m.cells.size() * 2;
    for (const Texture &t : textures)
        if (!t.empty())
            total += 8 + t.img.px.size();
    return total;
}

} // namespace gfx
