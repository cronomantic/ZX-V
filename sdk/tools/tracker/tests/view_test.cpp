/*
 * view_test.cpp - Los editores de zxv-tracker con el teclado y el ratón de
 * QTest (sin pantalla): escribir notas y valores en el patrón (en los AY y
 * en las pistas de muestras), deshacer, copiar y pegar, transponer, y los
 * samples y los efectos.
 */
#include <QtTest>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>

#include "../channels.h"
#include "../editor.h"
#include "../ayresult.h"
#include "../waveview.h"
#include "../instrview.h"

extern "C" {
#include "zxm_format.h"
}
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
        const Pattern &pat = ed.tune().ayPatterns[0][0];

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

        /* Lo que se ha escrito, guardado (como PT3) y leído, sigue ahí. */
        Song pt3, back;
        Warnings w;
        QVERIFY(toPt3(ed.tune(), pt3, w));
        QCOMPARE(load(save(pt3), back), LOAD_OK);
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
        const Pattern &pat = ed.tune().ayPatterns[0][0];
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

    /* Una pista de muestras: sin patrón hasta que se escribe en ella; la
     * nota con el instrumento elegido, el volumen, un efecto y su
     * parámetro; deshacer; y guardado como .zxm y leído, sigue ahí. */
    void sampleTrack()
    {
        Editor ed;
        Tune t;
        t.smpTracks = 1;
        ed.setTune(t, QString(), FORMAT_NONE);
        ed.setInstrument(3);
        PatternView pv(&ed);
        pv.resize(900, 500);
        pv.show();
        pv.setOctave(4);
        for (int i = 0; i < 3; i++)
            QTest::keyClick(&pv, Qt::Key_Tab);      /* A, B, C, M1 */
        QCOMPARE(pv.group(), int(Editor::SMP_GROUP));
        QCOMPARE(ed.tune().order[0].smp[0], -1);
        QTest::keyClick(&pv, Qt::Key_Z);            /* C-4 */
        int p = ed.tune().order[0].smp[0];
        QVERIFY(p >= 0);
        const SmpCell &c = ed.tune().smpPatterns[size_t(p)].rows[0];
        QCOMPARE(c.note, 48);
        QCOMPARE(c.ins, 3);
        QTest::keyClick(&pv, Qt::Key_Up);
        for (int i = 0; i < 3; i++)
            QTest::keyClick(&pv, Qt::Key_Right);    /* el instrumento (2) y el volumen */
        keys(&pv, "20");
        QCOMPARE(ed.tune().smpPatterns[size_t(p)].rows[0].vol, 32);
        QTest::keyClick(&pv, Qt::Key_Up);
        QTest::keyClick(&pv, Qt::Key_Right);
        QTest::keyClick(&pv, Qt::Key_Right);        /* el efecto 1 */
        keys(&pv, "a");
        QCOMPARE(int(ed.tune().smpPatterns[size_t(p)].rows[0].fx[0]), int('A'));
        QTest::keyClick(&pv, Qt::Key_Up);
        QTest::keyClick(&pv, Qt::Key_Right);        /* su parámetro */
        keys(&pv, "0F");
        QCOMPARE(int(ed.tune().smpPatterns[size_t(p)].rows[0].fxp[0]), 0x0F);
        ed.undo();
        QCOMPARE(int(ed.tune().smpPatterns[size_t(p)].rows[0].fxp[0]), 0x00);
        ed.redo();

        Warnings w;
        Tune back;
        QCOMPARE(loadTune(saveZxm(ed.tune(), w), back, w), int(TUNE_OK));
        QCOMPARE(back.smpTracks, 1);
        const SmpCell &b = back.smpPatterns[size_t(back.order[0].smp[0])].rows[0];
        QCOMPARE(b.note, 48);
        QCOMPARE(b.ins, 3);
        QCOMPARE(b.vol, 32);
        QCOMPARE(int(b.fx[0]), int('A'));
        QCOMPARE(int(b.fxp[0]), 0x0F);
    }

    /* Las plantillas de los instrumentos del AY: todas con líneas y su
     * vuelta dentro; puesta una, se oye (lo que suena no está callado). */
    void templates()
    {
        auto all = AyResult::templates();
        QVERIFY(all.size() >= 6);
        for (const auto &tp : all) {
            QVERIFY(!tp.second.lines.empty());
            QVERIFY(tp.second.loop >= 0 && tp.second.loop < int(tp.second.lines.size()));
        }
        Editor ed;
        ed.setSample(5);
        ed.bank().samples[5] = all[1].second;      /* ataque y caída */
        AyResult r(&ed);
        r.resize(600, 150);
        r.show();
        QImage img = r.grab().toImage();
        QVERIFY(!img.isNull());
    }

    /* La onda de una muestra: un clic en medio pone ahí el bucle. */
    void wave()
    {
        Editor ed;
        Tune t;
        Wave w;
        w.len = w.loop = 1000;
        for (int i = 0; i < 1000; i++)
            w.data.push_back(uint8_t(int8_t(i % 100 - 50)));
        t.waves.push_back(w);
        ed.setTune(t, QString(), FORMAT_NONE);
        WaveView v(&ed);
        v.resize(504, 200);
        v.show();
        v.setWave(0);
        uint32_t got = 0;
        bool start = false;
        connect(&v, &WaveView::loopChanged, this, [&](uint32_t from, bool s) {
            got = from;
            start = s;
        });
        QTest::mouseClick(&v, Qt::LeftButton, Qt::NoModifier, QPoint(4 + 248, 100));
        QVERIFY(start);
        QVERIFY(got >= 490 && got <= 510);
    }

    /* La envolvente: un clic añade un punto, arrastrarlo lo mueve, el botón
     * derecho lo quita; deshacer vuelve atrás. */
    void envelope()
    {
        Editor ed;
        Tune t;
        t.instruments.resize(1);
        ed.setTune(t, QString(), FORMAT_NONE);
        EnvelopeView v(&ed, 0);
        v.resize(34 + 8 + 640, 8 + 18 + 128);  /* 10 px por tick, 2 px por valor */
        v.show();
        auto env = [&] { return ins::env(ed.tune().instruments[0], 0); };
        QCOMPARE(env().n, 0);
        QTest::mouseClick(&v, Qt::LeftButton, Qt::NoModifier, QPoint(34 + 300, 8));
        QCOMPARE(env().n, 1);
        QCOMPARE(env().tick[0], 0);             /* el primero, siempre en el tick 0 */
        QCOMPARE(env().value[0], 64);
        QVERIFY(env().on);
        QTest::mouseClick(&v, Qt::LeftButton, Qt::NoModifier, QPoint(34 + 320, 8 + 128));
        QCOMPARE(env().n, 2);
        QCOMPARE(env().tick[1], 32);
        QCOMPARE(env().value[1], 0);
        /* Arrastra el segundo punto a (16, 32). */
        QPoint p = v.pointPos(1);
        QTest::mousePress(&v, Qt::LeftButton, Qt::NoModifier, p);
        QMouseEvent mv(QEvent::MouseMove, QPointF(34 + 160, 8 + 64), v.mapToGlobal(QPointF(34 + 160, 8 + 64)),
                       Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&v, &mv);
        QTest::mouseRelease(&v, Qt::LeftButton, Qt::NoModifier, QPoint(34 + 160, 8 + 64));
        QCOMPARE(env().tick[1], 16);
        QCOMPARE(env().value[1], 32);
        /* Uno en medio: queda en orden. */
        QTest::mouseClick(&v, Qt::LeftButton, Qt::NoModifier, QPoint(34 + 80, 8 + 32));
        QCOMPARE(env().n, 3);
        QCOMPARE(env().tick[1], 8);
        QCOMPARE(env().tick[2], 16);
        QTest::mouseClick(&v, Qt::RightButton, Qt::NoModifier, v.pointPos(1));
        QCOMPARE(env().n, 2);
        QCOMPARE(env().tick[1], 16);
        ed.undo();
        QCOMPARE(env().n, 3);
    }

    /* El mapa de notas: arrastrar pinta todas las notas por las que pasa. */
    void noteMap()
    {
        Editor ed;
        Tune t;
        t.instruments.resize(1);
        t.waves.resize(3);
        ed.setTune(t, QString(), FORMAT_NONE);
        NoteMapView v(&ed);
        v.resize(96 * 8 + 8, 64);
        v.show();
        v.setPaint(2);
        QRect a = v.keyRect(12), b = v.keyRect(23);
        QTest::mousePress(&v, Qt::LeftButton, Qt::NoModifier, a.center());
        QMouseEvent mv(QEvent::MouseMove, QPointF(b.center()), v.mapToGlobal(QPointF(b.center())), Qt::NoButton,
                       Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&v, &mv);
        QTest::mouseRelease(&v, Qt::LeftButton, Qt::NoModifier, b.center());
        const auto &d = ed.tune().instruments[0].data;
        QCOMPARE(int(d[ZXM_SI_MAP + 11]), 0);
        for (int n = 12; n <= 23; n++)
            QCOMPARE(int(d[size_t(ZXM_SI_MAP + n)]), 2);
        QCOMPARE(int(d[ZXM_SI_MAP + 24]), 0);
        ed.undo();
        QCOMPARE(int(ed.tune().instruments[0].data[ZXM_SI_MAP + 12]), 0);
    }

    void samples()
    {
        Editor ed;
        ed.setSample(1);
        SampleView sv(&ed);
        sv.resize(600, 400);
        sv.show();
        Sample &s = ed.bank().samples[1];
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
        QCOMPARE(ed.fxBank().effects[0].frames[0].tone, 0x123);
        QTest::keyClick(&v, Qt::Key_Delete);        /* quita la línea 1 */
        QCOMPARE(int(ed.fxBank().effects[0].frames.size()), 2);
        ed.undoBank();
        QCOMPARE(int(ed.fxBank().effects[0].frames.size()), 3);
        fx::Bank back;
        QVERIFY(fx::loadBank(fx::saveBank(ed.fxBank()), back));
        QCOMPARE(back.effects[0].frames[0].tone, 0x123);
    }

    /* Elegir los canales de un módulo de 14 con notas (el 3 y el 10 con
     * menos): vienen los que elige el conversor; con 12 marcados, los demás
     * no se pueden marcar; sin ninguno, no se convierte. Con ZXV_SHOT, la
     * ventana a ese PNG. */
    void channels()
    {
        ModuleChannels ch;
        ch.count = 16;
        ch.used = 0x3FFF;
        for (int c = 0; c < 14; c++)
            ch.notes[c] = c == 3 || c == 10 ? 40 : 200 + c;
        ch.picked = ch.used & ~((uint64_t(1) << 3) | (uint64_t(1) << 10));
        QVERIFY(ch.tooMany());
        ChannelsDialog d(QStringLiteral("prueba.xm"), ch);
        d.show();
        QCOMPARE(d.channels(), ch.picked);
        QList<QCheckBox *> boxes = d.findChildren<QCheckBox *>();
        QCOMPARE(int(boxes.size()), 14);
        QCheckBox *four = boxes[3];         /* el canal 4 (el 3), sin marcar */
        QVERIFY(!four->isChecked() && !four->isEnabled());
        boxes[0]->setChecked(false);
        QVERIFY(four->isEnabled());
        four->setChecked(true);
        QCOMPARE(d.channels(), (ch.picked & ~uint64_t(1)) | (uint64_t(1) << 3));
        QVERIFY(!boxes[0]->isEnabled());
        QPushButton *ok = d.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
        d.setChannels(0);
        QVERIFY(!ok->isEnabled());
        d.setChannels(1);
        QVERIFY(ok->isEnabled());
        d.setChannels(ch.picked);
        QByteArray shot = qgetenv("ZXV_SHOT");
        if (!shot.isEmpty())
            QVERIFY(d.grab().save(QString::fromLocal8Bit(shot)));
        ModuleChannels few;
        few.count = 16;
        few.used = 0x0FFF;
        QVERIFY(!few.tooMany());
    }
};

QTEST_MAIN(ViewTest)
#include "view_test.moc"
