/*
 * doc_test.cpp - El documento de zxv-gfx (doc.cpp), sin Qt:
 *
 *   doc_test IN.gfx OUT.gfx   lo carga y lo vuelve a grabar (gfx_test.py
 *                             compara los dos con lo que hizo png2gfx.py)
 *   doc_test                  las operaciones de imagen, los mapas y un
 *                             fichero dañado
 */
#include <cstdio>
#include <fstream>
#include <iterator>

#include "../doc.h"

using namespace gfx;

static int fails;

#define CHECK(c, msg)                       \
    do {                                    \
        if (!(c)) {                         \
            std::printf("FALLO: %s\n", msg); \
            fails++;                        \
        }                                   \
    } while (0)

static int roundtrip(const char *in, const char *out)
{
    std::ifstream f(in, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    Document doc;
    if (doc.load(data) != LOAD_OK)
        return 1;
    std::vector<uint8_t> o = doc.save();
    std::ofstream(out, std::ios::binary).write(reinterpret_cast<const char *>(o.data()), std::streamsize(o.size()));
    return 0;
}

/* Una imagen con un color distinto en cada píxel (módulo 16). */
static Image pattern(int w, int h)
{
    Image im(w, h, 0);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            im.set(x, y, (x * 3 + y * 5) % 16);
    return im;
}

static void units()
{
    Image a = pattern(5, 3);
    CHECK(flippedH(flippedH(a)) == a && flippedV(flippedV(a)) == a, "dos espejos dejan la imagen como estaba");
    Image r = rotatedRight(a);
    CHECK(r.w == 3 && r.h == 5 && r.at(2, 0) == a.at(0, 0) && r.at(0, 4) == a.at(4, 2), "girar a la derecha");
    CHECK(rotatedLeft(r) == a, "girar a la izquierda deshace girar a la derecha");
    CHECK(rotatedRight(rotatedRight(a)) == flippedV(flippedH(a)), "dos giros son los dos espejos");
    CHECK(shifted(a, 1, 0).at(0, 0) == a.at(4, 0) && shifted(shifted(a, 2, -1), -2, 1) == a, "mover con vuelta");
    Image b = resized(a, 7, 2, TRANSP);
    CHECK(b.w == 7 && b.h == 2 && b.at(4, 1) == a.at(4, 1) && b.at(5, 0) == TRANSP, "cambiar el tamaño");
    CHECK(cropped(a, 3, 1, 4, 4, 0).at(1, 1) == a.at(4, 2) && cropped(a, 3, 1, 4, 4, 0).at(2, 0) == 0, "recortar");

    Image c(9, 9, 0);
    drawLine(c, 0, 0, 8, 4, 2);
    CHECK(c.at(0, 0) == 2 && c.at(8, 4) == 2 && c.at(4, 2) == 2, "una línea");
    Image d(9, 9, 0);
    drawRect(d, 6, 6, 2, 2, 3, false);
    CHECK(d.at(2, 2) == 3 && d.at(6, 4) == 3 && d.at(4, 4) == 0, "una caja");
    drawEllipse(d, 0, 0, 8, 8, 5, false);
    CHECK(d.at(4, 0) == 5 && d.at(0, 4) == 5 && d.at(8, 4) == 5 && d.at(4, 8) == 5 && d.at(0, 0) == 0, "una elipse");
    Image e(6, 6, 0);
    drawRect(e, 0, 0, 5, 5, 1, false);
    floodFill(e, 2, 2, 4);
    CHECK(e.at(1, 1) == 4 && e.at(4, 4) == 4 && e.at(0, 0) == 1, "rellenar dentro de una caja");
    floodFill(e, 0, 0, 1);
    CHECK(e.at(0, 0) == 1, "rellenar con el mismo color no hace nada");
    Image f(3, 3, 0);
    Image g(2, 2, TRANSP);
    g.set(1, 1, 7);
    paste(f, g, 1, 1, true);
    CHECK(f.at(1, 1) == 0 && f.at(2, 2) == 7, "pegar sin los transparentes");

    Map m;
    m.w = 2;
    m.h = 2;
    m.cells = { 1, 2, 3, 4 };
    Map m2 = resizedMap(m, 3, 1);
    CHECK(m2.cells == std::vector<uint16_t>({ 1, 2, 0 }), "cambiar el tamaño de un mapa");

    /* Un documento con de todo, y otra vez igual. */
    Document doc;
    doc.sprites[7] = pattern(5, 3);
    doc.tiles8 = { pattern(8, 8), Image(8, 8, 2) };
    doc.maps[3] = m;
    doc.maps[3].tile = 16;
    doc.textures[1].img = pattern(4, 4);
    doc.textures[1].wrap = false;
    doc.textures[1].out = 9;
    Document back;
    CHECK(back.load(doc.save()) == LOAD_OK && back == doc, "grabar y cargar");
    /* Dañado: no cambia nada. */
    std::vector<uint8_t> bad = doc.save();
    bad.resize(bad.size() - 2);
    Document keep;
    keep.sprites[0] = Image(1, 1, 3);
    Document before = keep;
    CHECK(keep.load(bad) == LOAD_DAMAGED && keep == before, "un fichero cortado no cambia nada");
}

int main(int argc, char **argv)
{
    if (argc == 3)
        return roundtrip(argv[1], argv[2]);
    units();
    std::printf(fails ? "%d fallos\n" : "todo bien\n", fails);
    return fails ? 1 : 0;
}
