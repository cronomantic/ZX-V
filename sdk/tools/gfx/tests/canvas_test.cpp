/*
 * canvas_test.cpp - Los lienzos de zxv-gfx con el ratón de QTest (sin
 * pantalla): las herramientas de dibujo, seleccionar y mover, pegar,
 * deshacer y rehacer, y pintar los mapas.
 */
#include <QScrollArea>
#include <QtTest>

#include "../canvas.h"

class CanvasTest : public QObject
{
    Q_OBJECT

    Editor ed;
    QScrollArea area;
    PixelCanvas *pixel = nullptr;

    /* El centro del píxel (x, y) en el lienzo (zoom 8, margen 8). */
    static QPoint at(int x, int y) { return QPoint(8 + x * 8 + 4, 8 + y * 8 + 4); }

    void drag(QWidget *w, QPoint a, QPoint b, Qt::MouseButton button = Qt::LeftButton)
    {
        QTest::mousePress(w, button, Qt::NoModifier, a);
        QMouseEvent move(QEvent::MouseMove, b, w->mapToGlobal(b), Qt::NoButton, button, Qt::NoModifier);
        QApplication::sendEvent(w, &move);
        QTest::mouseRelease(w, button, Qt::NoModifier, b);
    }

    void fresh()
    {
        gfx::Document doc;
        doc.sprites[0] = gfx::Image(16, 16, gfx::TRANSP);
        ed.setDocument(doc, QString());
        ed.select(Section::Sprites, 0);
        pixel->setZoom(8);
        ed.setColor(0, 2);
        ed.setColor(1, 4);
    }

private slots:
    void initTestCase()
    {
        pixel = new PixelCanvas(&ed);
        area.setWidget(pixel);
        area.resize(400, 400);
        area.show();
    }

    void tools()
    {
        fresh();
        gfx::Image &im = ed.doc().sprites[0];
        ed.setTool(Tool::Pen);
        drag(pixel, at(1, 1), at(5, 1));
        QCOMPARE(im.at(1, 1), 2);
        QCOMPARE(im.at(3, 1), 2);           /* el trazo, sin huecos */
        QCOMPARE(im.at(5, 1), 2);
        ed.setTool(Tool::FilledRect);
        drag(pixel, at(2, 4), at(4, 6), Qt::RightButton);
        QCOMPARE(im.at(3, 5), 4);
        QCOMPARE(im.at(5, 5), gfx::TRANSP);
        ed.setTool(Tool::Line);
        drag(pixel, at(0, 10), at(9, 10));
        QCOMPARE(im.at(9, 10), 2);
        ed.setTool(Tool::Fill);
        QTest::mouseClick(pixel, Qt::LeftButton, Qt::NoModifier, at(3, 5));
        QCOMPARE(im.at(2, 4), 2);           /* la caja, de otro color */
        ed.setTool(Tool::Pick);
        QTest::mouseClick(pixel, Qt::RightButton, Qt::NoModifier, at(15, 15));
        QCOMPARE(ed.color(1), gfx::TRANSP);

        /* Cada trazo, un paso de deshacer. */
        ed.undo();
        QCOMPARE(im.at(2, 4), 4);
        ed.undo();
        QCOMPARE(im.at(9, 10), gfx::TRANSP);
        ed.redo();
        QCOMPARE(im.at(9, 10), 2);
    }

    void selectAndMove()
    {
        fresh();
        gfx::Image &im = ed.doc().sprites[0];
        im.set(2, 2, 5);
        im.set(3, 2, 6);
        ed.setTool(Tool::Select);
        drag(pixel, at(2, 2), at(3, 2));    /* selecciona los dos */
        QCOMPARE(pixel->selection(), QRect(2, 2, 2, 1));
        drag(pixel, at(2, 2), at(7, 9));    /* y los lleva 5 a la derecha y 7 abajo */
        pixel->commit();
        QCOMPARE(im.at(2, 2), gfx::TRANSP);
        QCOMPARE(im.at(7, 9), 5);
        QCOMPARE(im.at(8, 9), 6);
        ed.undo();                          /* mover es un paso */
        QCOMPARE(im.at(2, 2), 5);
        QCOMPARE(im.at(7, 9), gfx::TRANSP);

        /* Esc deja lo levantado donde estaba. */
        drag(pixel, at(2, 2), at(3, 2));
        drag(pixel, at(2, 2), at(10, 10));
        pixel->discard();
        QCOMPARE(im.at(2, 2), 5);
        QCOMPARE(im.at(10, 10), gfx::TRANSP);
        QVERIFY(!ed.canUndo() || ed.doc().sprites[0] == im);

        /* Girar solo lo seleccionado. */
        pixel->clearSelection();
        drag(pixel, at(2, 2), at(3, 2));
        pixel->transform(gfx::rotatedRight);
        pixel->commit();
        QCOMPARE(im.at(2, 2), 5);
        QCOMPARE(im.at(2, 3), 6);
        QCOMPARE(im.at(3, 2), gfx::TRANSP);
    }

    void pasteFloating()
    {
        fresh();
        gfx::Image &im = ed.doc().sprites[0];
        gfx::Image stamp(2, 2, 9);
        stamp.set(1, 1, gfx::TRANSP);
        pixel->paste(stamp);
        QCOMPARE(ed.tool(), Tool::Select);
        drag(pixel, at(0, 0), at(4, 4));
        pixel->commit();
        QCOMPARE(im.at(4, 4), 9);
        QCOMPARE(im.at(5, 5), gfx::TRANSP); /* el 8 de lo pegado no tapa */
        /* Pegar y tirarlo no cambia nada (ni deja nada que deshacer). */
        gfx::Image before = im;
        bool couldUndo = ed.canUndo();
        pixel->paste(stamp);
        pixel->discard();
        QVERIFY(im == before);
        QCOMPARE(ed.canUndo(), couldUndo);
    }

    void maps()
    {
        gfx::Document doc;
        doc.tiles8 = { gfx::Image(8, 8, 0), gfx::Image(8, 8, 2), gfx::Image(8, 8, 4) };
        doc.maps[1].w = 10;
        doc.maps[1].h = 6;
        doc.maps[1].cells.assign(60, 0);
        ed.setDocument(doc, QString());
        ed.select(Section::Maps, 1);
        QScrollArea a2;
        auto *map = new MapCanvas(&ed);
        a2.setWidget(map);
        a2.resize(400, 300);
        a2.show();
        map->setZoom(2);
        auto cell = [](int x, int y) { return QPoint(8 + x * 16 + 8, 8 + y * 16 + 8); };
        gfx::Map &m = ed.doc().maps[1];

        ed.setTool(Tool::Pen);
        ed.setMapTile(2);
        ed.setMapFlags(gfx::CELL_FLIP_H);
        drag(map, cell(1, 1), cell(4, 1));
        QCOMPARE(m.at(2, 1), uint16_t(2 | gfx::CELL_FLIP_H));
        QCOMPARE(m.at(4, 1), uint16_t(2 | gfx::CELL_FLIP_H));
        drag(map, cell(2, 1), cell(2, 1), Qt::RightButton);
        QCOMPARE(m.at(2, 1), uint16_t(0));
        ed.setTool(Tool::FilledRect);
        ed.setMapTile(1);
        ed.setMapFlags(0);
        drag(map, cell(6, 2), cell(8, 4));
        QCOMPARE(m.at(7, 3), uint16_t(1));
        QCOMPARE(m.at(5, 3), uint16_t(0));
        ed.setTool(Tool::Fill);
        ed.setMapTile(2);
        QTest::mouseClick(map, Qt::LeftButton, Qt::NoModifier, cell(0, 5));
        QCOMPARE(m.at(9, 0), uint16_t(2));
        QCOMPARE(m.at(7, 3), uint16_t(1));  /* la caja, de otro tile, no */
        ed.setTool(Tool::Pick);
        QTest::mouseClick(map, Qt::LeftButton, Qt::NoModifier, cell(3, 1));
        QCOMPARE(ed.mapTile(), 2);
        QCOMPARE(ed.mapFlags(), int(gfx::CELL_FLIP_H));
        ed.undo();
        QCOMPARE(m.at(9, 0), uint16_t(0));
    }
};

QTEST_MAIN(CanvasTest)
#include "canvas_test.moc"
