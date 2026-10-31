/*
 * doc_test.cpp - El documento de zxv-gfx (doc.cpp), sin Qt:
 *
 *   doc_test IN.gfx OUT.gfx   lo carga y lo vuelve a grabar (gfx_test.py
 *                             compara los dos con lo que hizo png2gfx.py)
 *   doc_test                  las operaciones de imagen, los mapas, los
 *                             colores RGB565, las paletas y un fichero
 *                             dañado
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
    doc.screens[0] = newScreen(0, 5);
    doc.screens[0].img.set(0, 0, 1);
    doc.screens[0].img.set(1, 0, 8);
    doc.screens[0].img.set(255, 191, 15);
    doc.screens[200] = newScreen(1, 15);
    paste(doc.screens[200].img, pattern(512, 384), 0, 0, false);
    Document back;
    CHECK(back.load(doc.save()) == LOAD_OK && back == doc, "grabar y cargar");

    /* Una pantalla sola, como la de SAVE "x" SCREEN$: los píxeles como en la
     * VRAM (nibble bajo = píxel par) y el 8 es un color más. */
    Document one;
    one.screens[0] = doc.screens[0];
    std::vector<uint8_t> scr = one.save();
    CHECK(scr.size() == 8 + 8 + 4 + 24576 && scr[8] == 5 && scr[10] == 0 && scr[12] == 4 && scr[13] == 0x60 &&
              scr[16] == 0 && scr[17] == 5 && scr[20] == 0x81 && scr[21] == 0 && scr.back() == 0xF0,
          "una pantalla de modo 0");
    CHECK(one.machineBytes() == 0, "las pantallas no ocupan en el almacén del BASIC");
    std::vector<uint8_t> wrong = scr;
    wrong[16] = 1;                      /* modo 1 con los datos del 0 */
    Document none;
    CHECK(none.load(wrong) == LOAD_DAMAGED, "una pantalla de otra longitud");
    wrong = scr;
    wrong[16] = 2;
    CHECK(none.load(wrong) == LOAD_DAMAGED, "una pantalla de modo 2");
    wrong = scr;
    wrong[17] = 16;
    CHECK(none.load(wrong) == LOAD_DAMAGED && none == Document(), "un borde que no es un color");
    /* Los colores en RGB565 (spec §4.1): los de siempre dan D7 y FF, y de
     * RGB565 a 8 bits y vuelta, el mismo color. */
    bool same = true;
    for (int i = 0; i < 16; i++)
        same = same && rgb565ToRgb(stdRgb565(i)) == paletteRgb(i) &&
               toRgb565(int(paletteRgb(i) >> 16), int(paletteRgb(i) >> 8 & 255), int(paletteRgb(i) & 255)) ==
                   stdRgb565(i);
    CHECK(same && stdRgb565(5) == 0x06DB && stdRgb565(8) == 0 && stdRgb565(14) == 0xFFE0, "los colores de siempre");
    bool back565 = true;
    for (uint32_t c = 0; c < 65536; c++) {
        uint32_t rgb = rgb565ToRgb(uint16_t(c));
        back565 = back565 && toRgb565(int(rgb >> 16), int(rgb >> 8 & 255), int(rgb & 255)) == c;
    }
    CHECK(back565 && toRgb565(0x80, 0x40, 0x20) == 0x8204 && rgb565ToRgb(0x1234) == 0x10449F,
          "de RGB565 a 8 bits y vuelta");
    /* La paleta de una importación: los de siempre en su índice, los demás
     * en los libres (el 8 el último). */
    Palette pal;
    CHECK(screenPalette({ 0, 0xD800, 0xFFFF }, pal) == 0 && pal == standardPalette(), "con los de siempre, sin paleta");
    CHECK(screenPalette({ 0xD800, 0x1234, 0, 0x8410 }, pal) == 1 && pal[0] == 0 && pal[1] == 0x1234 &&
              pal[2] == 0xD800 && pal[3] == 0x8410 && pal[4] == stdRgb565(4) && pal[8] == 0,
          "con otros, en los índices libres");
    std::vector<uint16_t> many;
    for (int k = 0; k < 16; k++)
        many.push_back(uint16_t(0x1000 + k));
    CHECK(screenPalette(many, pal) == 1 && pal[7] == 0x1007 && pal[8] == 0x100F && pal[9] == 0x1008,
          "16 propios: el 8 el último");
    many.push_back(0x2000);
    CHECK(screenPalette(many, pal) == -1, "17 no caben");

    /* Una pantalla con paleta (D90): la marca y los 16 colores antes de los
     * píxeles; se carga igual, y sin paleta vuelve a los de siempre. */
    Document cp;
    cp.screens[2] = newScreen(1, 9);
    cp.screens[2].img.set(1, 0, 3);
    cp.screens[2].hasPalette = true;
    for (int k = 0; k < 16; k++)
        cp.screens[2].palette[size_t(k)] = uint16_t(0x0101 * k + 0x1234);
    std::vector<uint8_t> ps = cp.save();
    CHECK(ps.size() == 8 + 8 + 4 + 32 + 98304 && ps[12] == 36 && ps[13] == 0x80 && ps[16] == 1 && ps[17] == 9 &&
              ps[18] == 1 && ps[19] == 0 && ps[20] == 0x34 && ps[21] == 0x12 && ps[50] == 0x43 && ps[51] == 0x21 &&
              ps[52] == 0x30,
          "una pantalla con paleta");
    Document cp2;
    CHECK(cp2.load(ps) == LOAD_OK && cp2 == cp && cp2.screens[2].rgb(0) == 0x10449F, "y otra vez igual");
    cp.screens[2].removePalette();
    CHECK(cp.save().size() == ps.size() - 32 && cp.save()[18] == 0 && cp.screens[2].rgb(3) == 0xD700D7,
          "sin paleta, los de siempre");
    wrong = ps;
    wrong[18] = 0;                      /* con la paleta, pero sin la marca */
    CHECK(none.load(wrong) == LOAD_DAMAGED, "una paleta sin la marca");
    wrong = ps;
    wrong[18] = 3;
    CHECK(none.load(wrong) == LOAD_DAMAGED, "una marca que no existe");

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
