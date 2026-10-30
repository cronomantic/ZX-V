/*
 * view_test.cpp - Los editores de zxv-tracker con el teclado y el ratón de
 * QTest (sin pantalla): escribir notas y valores en el patrón, deshacer,
 * copiar y pegar, transponer, y los samples y los efectos.
 */
#include <QtTest>

#include "../editor.h"
#include "../lines.h"
#include "../patternview.h"

using namespace trk;

class ViewTest : public QObject
{
    Q_OBJECT

    static void keys(QWidget *w, const char *text)
    {
        for (const char *p = text; *p; p++)
            QTest::keyClick(w, *p);
    }

private slots:
    void notes()
    {
        Editor ed;
        PatternView pv(&ed);
        pv.resize(700, 500);
        pv.show();
        pv.setOctave(4);
        ed.setSample(3);
        const Pattern &pat = ed.mod().patterns[0];

        QTest::keyClick(&pv, Qt::Key_Z);            /* C-4, con el sample 3 */
        QCOMPARE(pat.rows[0].ch[0].note, 36);
        QCOMPARE(pat.rows[0].ch[0].sample, 3);
        QCOMPARE(pv.row(), 1);
        QTest::keyClick(&pv, Qt::Key_Q);            /* C-5 */
        QCOMPARE(pat.rows[1].ch[0].note, 48);
        QTest::keyClick(&pv, Qt::Key_1);            /* el silencio */
        QCOMPARE(pat.rows[2].ch[0].note, NOTE_OFF);

        /* El volumen y un efecto de la fila 0. */
        QTest::keyClick(&pv, Qt::Key_Home);
        for (int i = 0; i < 4; i++)
            QTest::keyClick(&pv, Qt::Key_Right);    /* sample, env, orn, vol */
        keys(&pv, "C");
        QCOMPARE(pat.rows[0].ch[0].vol, 12);
        QTest::keyClick(&pv, Qt::Key_Up);
        QTest::keyClick(&pv, Qt::Key_Right);        /* el efecto */
        keys(&pv, "B");
        QTest::keyClick(&pv, Qt::Key_Up);
        QTest::keyClick(&pv, Qt::Key_Right);
        QTest::keyClick(&pv, Qt::Key_Right);        /* el parámetro */
        keys(&pv, "06");
        QCOMPARE(pat.rows[0].ch[0].fx, 11);
        QCOMPARE(pat.rows[0].ch[0].fxParam, 6);
        /* Un efecto que no existe no se escribe. */
        QTest::keyClick(&pv, Qt::Key_Up);
        QTest::keyClick(&pv, Qt::Key_Left);
        QTest::keyClick(&pv, Qt::Key_Left);
        keys(&pv, "7");
        QCOMPARE(pat.rows[1].ch[0].fx, 0);

        /* El periodo de la envolvente: cuatro cifras seguidas. */
        QTest::keyClick(&pv, Qt::Key_Home, Qt::ControlModifier);
        keys(&pv, "1A2B");
        QCOMPARE(pat.rows[0].envPeriod, 0x1A2B);
        QCOMPARE(pv.row(), 1);

        /* Deshacer, de tecla en tecla. */
        ed.undo();
        QCOMPARE(pat.rows[0].envPeriod, 0x1A20);
        ed.redo();
        QCOMPARE(pat.rows[0].envPeriod, 0x1A2B);

        /* Lo que se ha escrito, guardado y leído, sigue ahí. */
        Song back;
        QCOMPARE(load(save(ed.song()), back), LOAD_OK);
        QCOMPARE(back.modules[0].patterns[0].rows[0].ch[0].note, 36);
        QCOMPARE(back.modules[0].patterns[0].rows[0].ch[0].fxParam, 6);
        QCOMPARE(back.modules[0].patterns[0].rows[2].ch[0].note, NOTE_OFF);
    }

    /* La nota que suena al escribirla se para al soltar la tecla, aunque
     * Qt no sepa cuál es (las que mandan otros programas), y al perder el
     * foco: nunca se queda sonando. */
    void preview()
    {
        Editor ed;
        PatternView pv(&ed);
        pv.resize(700, 500);
        pv.show();
        int on = 0, off = 0, stops = 0;
        connect(&pv, &PatternView::preview, this, [&] { on++; });
        connect(&pv, &PatternView::previewOff, this, [&] { off++; });
        connect(&pv, &PatternView::stopRequested, this, [&] { stops++; });
        QTest::keyPress(&pv, Qt::Key_Z);
        QCOMPARE(on, 1);
        QTest::keyRelease(&pv, Qt::Key_Z);
        QCOMPARE(off, 1);
        QTest::keyPress(&pv, Qt::Key_X);
        QKeyEvent unknown(QEvent::KeyRelease, 0, Qt::NoModifier);
        QApplication::sendEvent(&pv, &unknown);
        QCOMPARE(off, 2);
        QTest::keyPress(&pv, Qt::Key_C);
        QFocusEvent out(QEvent::FocusOut);
        QApplication::sendEvent(&pv, &out);
        QCOMPARE(off, 3);
        QTest::keyClick(&pv, Qt::Key_Escape);
        QCOMPARE(stops, 1);
    }

    void blocks()
    {
        Editor ed;
        PatternView pv(&ed);
        pv.resize(700, 500);
        pv.show();
        pv.setOctave(2);
        keys(&pv, "zxc");                           /* C-2 D-2 E-2 en el canal A */
        const Pattern &pat = ed.mod().patterns[0];
        QCOMPARE(pat.rows[2].ch[0].note, 16);

        /* Las tres filas del canal A, al B en la fila 8. */
        QTest::keyClick(&pv, Qt::Key_Home);
        QTest::keyClick(&pv, Qt::Key_Down, Qt::ShiftModifier);
        QTest::keyClick(&pv, Qt::Key_Down, Qt::ShiftModifier);
        QVERIFY(pv.hasSelection());
        pv.copy();
        QTest::keyClick(&pv, Qt::Key_Tab);
        pv.setRow(8);
        pv.paste();
        QCOMPARE(pat.rows[8].ch[1].note, 12);
        QCOMPARE(pat.rows[10].ch[1].note, 16);
        QCOMPARE(pat.rows[8].ch[0].note, NOTE_NONE);

        /* Una octava arriba todo el canal B. */
        pv.transpose(12);
        QCOMPARE(pat.rows[8].ch[1].note, 24);
        QCOMPARE(pat.rows[0].ch[0].note, 12);       /* el A, no */

        /* Meter y quitar filas. */
        pv.setRow(0);
        pv.insertRow();
        QCOMPARE(pat.rows[1].ch[0].note, 12);
        QCOMPARE(int(pat.rows.size()), 64);
        pv.deleteRow();
        QCOMPARE(pat.rows[0].ch[0].note, 12);
    }

    void samples()
    {
        Editor ed;
        ed.setSample(1);
        SampleView sv(&ed);
        sv.resize(600, 400);
        sv.show();
        Sample &s = ed.mod().samples[1];
        QCOMPARE(int(s.lines.size()), 1);
        QVERIFY(s.lines[0].toneOn());
        QTest::keyClick(&sv, Qt::Key_Space);        /* el tono, fuera */
        QVERIFY(!s.lines[0].toneOn());
        QTest::keyClick(&sv, Qt::Key_Insert);       /* otra línea */
        QCOMPARE(int(s.lines.size()), 2);
        /* El tono de la línea 1: 1A0 y el signo. */
        QCOMPARE(sv.line(), 1);
        for (int i = 0; i < 4; i++)
            QTest::keyClick(&sv, Qt::Key_Right);
        keys(&sv, "1A0");
        QCOMPARE(int(s.lines[1].tone), 0x1A0);
        QTest::keyClick(&sv, Qt::Key_Left);
        keys(&sv, "-");
        QCOMPARE(int(s.lines[1].tone), -0x1A0);
        /* Un clic en la barra: el volumen. */
        QFontMetrics fm(sv.font());
        int cw = fm.horizontalAdvance('0');
        QTest::mouseClick(&sv, Qt::LeftButton, Qt::NoModifier,
                          QPoint(6 + (27 + 5) * cw + cw / 2, fm.height() + 6 + (fm.height() + 1) * 0 + 3));
        QCOMPARE(s.lines[0].volume(), 6);
        ed.undo();
        QCOMPARE(s.lines[0].volume(), 15);
    }

    void effects()
    {
        Editor ed;
        fx::Bank b;
        b.effects.resize(1);
        b.effects[0].frames.resize(3);
        ed.setBank(b, QString());
        SfxView v(&ed);
        v.resize(500, 300);
        v.show();
        for (int i = 0; i < 2; i++)
            QTest::keyClick(&v, Qt::Key_Right);     /* el tono */
        keys(&v, "123");
        QCOMPARE(ed.bank().effects[0].frames[0].tone, 0x123);
        QTest::keyClick(&v, Qt::Key_Delete);        /* quita la línea 1 */
        QCOMPARE(int(ed.bank().effects[0].frames.size()), 2);
        ed.undoBank();
        QCOMPARE(int(ed.bank().effects[0].frames.size()), 3);
        fx::Bank back;
        QVERIFY(fx::loadBank(fx::saveBank(ed.bank()), back));
        QCOMPARE(back.effects[0].frames[0].tone, 0x123);
    }
};

QTEST_MAIN(ViewTest)
#include "view_test.moc"
