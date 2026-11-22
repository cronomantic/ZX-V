/*
 * main.cpp - zxv-tracker, el tracker de ZX-V para el PC: canciones .zxm con
 * los dos AY y hasta 12 pistas de muestras, módulos PT3 (Pro Tracker 3 y
 * Vortex Tracker II) y efectos AYFX, los que toca la biblioteca de música
 * del SDK (zxv_music.h). Abre también MOD, S3M y XM.
 *
 *   zxv-tracker [FICHERO.zxm | .pt3 | .mod | .s3m | .xm | .afb]
 *   zxv-tracker --screenshot CAPTURA.png [--tab N] [FICHERO]
 *         (para las pruebas: abre, guarda la ventana en la captura y acaba)
 *   zxv-tracker --wav SALIDA.wav FICHERO
 *         (toca la canción una vez, a un WAV, y acaba)
 *
 * Habla el idioma del sistema si es el español y si no, el inglés; con
 * --lang es o --lang en, ese.
 */
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QTimer>
#include <QTranslator>

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "mainwindow.h"
#include "zxv_version.h"

/* Los textos del propio Qt (los botones de los diálogos, el menú de los
 * campos de texto...) en el idioma del programa: junto a él (windeployqt
 * los deja en translations/qt_es.qm) o los del sistema (qtbase_es.qm). */
static void installQtTranslation(QApplication &app, const QString &code)
{
    auto *t = new QTranslator(&app);
    const QString dirs[] = { QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("translations")),
                             QLibraryInfo::path(QLibraryInfo::TranslationsPath) };
    for (const QString &dir : dirs)
        for (const QString &name : { QStringLiteral("qt"), QStringLiteral("qtbase") })
            if (t->load(QLocale(code), name, QStringLiteral("_"), dir)) {
                app.installTranslator(t);
                return;
            }
    delete t;
}

/* "1,2,5-9" (desde 1, hasta 64): el bit de cada canal (desde 0), o 0 si no
 * vale o son más de 12. */
static uint64_t channelList(const QString &s)
{
    uint64_t set = 0;
    for (const QString &part : s.split(',')) {
        QStringList ab = part.split('-');
        bool ok1 = false, ok2 = true;
        int a = ab.value(0).toInt(&ok1), b = ab.size() == 2 ? ab[1].toInt(&ok2) : a;
        if (ab.size() > 2 || !ok1 || !ok2 || a < 1 || b < a || b > 64)
            return 0;
        for (int c = a; c <= b; c++)
            set |= uint64_t(1) << (c - 1);
    }
    int n = 0;
    for (int c = 0; c < 64; c++)
        n += int(set >> c & 1);
    return n <= 12 ? set : 0;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("zxv-tracker"));
    app.setApplicationVersion(QStringLiteral(ZXV_VERSION));
    app.setOrganizationName(QStringLiteral("ZX-V"));

    QCommandLineParser args;
    args.addHelpOption();
    args.addVersionOption();
    QCommandLineOption shot("screenshot", "Guarda la ventana en una imagen y acaba.", "fichero");
    QCommandLineOption tab("tab", "La pestaña (0-5) que se enseña.", "n");
    QCommandLineOption wav("wav", "Toca el módulo una vez, a un WAV, y acaba.", "fichero");
    QCommandLineOption lang("lang", "El idioma: es o en.", "idioma");
    QCommandLineOption chans("channels",
                             "De un MOD, S3M o XM de más de 12 canales, los que van (1,2,5-9; desde 1, hasta 12).",
                             "lista");
    args.addOptions({ shot, tab, wav, lang, chans });
    args.addPositionalArgument("fichero", "Una canción (.zxm, .pt3, .mod, .s3m o .xm) o un banco de efectos .afb.");
    args.process(app);

    QString code = args.isSet(lang) ? args.value(lang) : QLocale::system().name().left(2);
    QTranslator tr;
    if (code != "es" && tr.load(QStringLiteral(":/i18n/zxv-tracker_en.qm")))
        app.installTranslator(&tr);
    else if (code == "es")
        installQtTranslation(app, QStringLiteral("es"));

    MainWindow w;
    if (!args.isSet(wav))
        w.show();
    /* Sin preguntar los canales al tocarlo a un WAV o al guardar la ventana
     * (sin nadie delante): los de --channels o los que más notas tocan. */
    uint64_t channels = 0;
    if (args.isSet(chans) && !(channels = channelList(args.value(chans)))) {
        fprintf(stderr, "--channels: hasta 12 canales, como 1,2,5-9\n");
        return 2;
    }
    bool ask = !args.isSet(wav) && !args.isSet(shot);
    if (!args.positionalArguments().isEmpty() && !w.open(args.positionalArguments().first(), channels, ask))
        return 1;
    if (args.isSet(wav))
        return w.exportWav(args.value(wav)) ? 0 : 1;
    if (args.isSet(tab))
        w.showTab(std::clamp(args.value(tab).toInt(), 0, 5));
    if (args.isSet(shot)) {
        QString path = args.value(shot);
        QTimer::singleShot(300, &w, [&w, path] {
            QApplication::exit(w.grab().save(path) ? 0 : 1);
        });
    }
    return app.exec();
}
