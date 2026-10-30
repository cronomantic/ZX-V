/*
 * main.cpp - zxv-tracker, el tracker de ZX-V para el PC: música para los dos
 * AY en módulos PT3 (Pro Tracker 3 y Vortex Tracker II) y efectos AYFX, los
 * que toca la biblioteca de música del SDK (zxv_music.h).
 *
 *   zxv-tracker [FICHERO.pt3 | FICHERO.afb]
 *   zxv-tracker --screenshot CAPTURA.png [--tab N] [FICHERO]
 *         (para las pruebas: abre, guarda la ventana en la captura y acaba)
 *   zxv-tracker --wav SALIDA.wav FICHERO.pt3
 *         (toca el módulo una vez, a un WAV, y acaba)
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
    QCommandLineOption tab("tab", "La pestaña (0-3) que se enseña.", "n");
    QCommandLineOption wav("wav", "Toca el módulo una vez, a un WAV, y acaba.", "fichero");
    QCommandLineOption lang("lang", "El idioma: es o en.", "idioma");
    args.addOptions({ shot, tab, wav, lang });
    args.addPositionalArgument("fichero", "Un módulo .pt3 o un banco de efectos .afb.");
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
    if (!args.positionalArguments().isEmpty() && !w.open(args.positionalArguments().first()))
        return 1;
    if (args.isSet(wav))
        return w.exportWav(args.value(wav)) ? 0 : 1;
    if (args.isSet(tab))
        w.showTab(std::clamp(args.value(tab).toInt(), 0, 3));
    if (args.isSet(shot)) {
        QString path = args.value(shot);
        QTimer::singleShot(300, &w, [&w, path] {
            QApplication::exit(w.grab().save(path) ? 0 : 1);
        });
    }
    return app.exec();
}
