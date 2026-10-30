/*
 * main.cpp - zxv-gfx, el editor de gráficos de ZX-V para el PC: sprites,
 * tiles, mapas y texturas en ficheros .gfx (los de LOAD "x" GRAPHICS).
 *
 *   zxv-gfx [FICHERO.gfx]
 *   zxv-gfx --screenshot CAPTURA.png [--section N] [FICHERO.gfx]
 *         (para las pruebas: abre, guarda la ventana en la captura y acaba)
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
    app.setApplicationName(QStringLiteral("zxv-gfx"));
    app.setApplicationVersion(QStringLiteral(ZXV_VERSION));
    app.setOrganizationName(QStringLiteral("ZX-V"));

    QCommandLineParser args;
    args.addHelpOption();
    args.addVersionOption();
    QCommandLineOption shot("screenshot", "Guarda la ventana en una imagen y acaba.", "fichero");
    QCommandLineOption section("section", "La pestaña (0-4) que se enseña.", "n");
    QCommandLineOption lang("lang", "El idioma: es o en.", "idioma");
    args.addOptions({ shot, section, lang });
    args.addPositionalArgument("fichero", "Un fichero .gfx.");
    args.process(app);

    QString code = args.isSet(lang) ? args.value(lang) : QLocale::system().name().left(2);
    QTranslator tr;
    if (code != "es" && tr.load(QStringLiteral(":/i18n/zxv-gfx_en.qm")))
        app.installTranslator(&tr);
    else if (code == "es")
        installQtTranslation(app, QStringLiteral("es"));

    MainWindow w;
    w.show();
    if (!args.positionalArguments().isEmpty() && !w.open(args.positionalArguments().first()))
        return 1;
    if (args.isSet(section))
        w.showSection(Section(std::clamp(args.value(section).toInt(), 0, 4)));
    if (args.isSet(shot)) {
        QString path = args.value(shot);
        QTimer::singleShot(300, &w, [&w, path] {
            QApplication::exit(w.grab().save(path) ? 0 : 1);
        });
    }
    return app.exec();
}
