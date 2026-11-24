/*
 * main.cpp - zxv-link-gui, la carga remota de ZX-V con ventana (D101): el
 * mismo proyecto que zxv-link (la consola), con la misma conversación
 * (linkcore.c). Por la tira J5 de la placa, con un adaptador USB-serie de
 * 3,3 V, o por Wi-Fi con una Pico 2 W: ejecutar un fichero del PC directo a
 * la memoria, la tarjeta, la ROM y EXT: como un explorador (D112) y el
 * informe de la máquina.
 *
 *   zxv-link-gui [PROGRAMA]
 *         (con un programa, lo pone en la ventana, listo para ejecutarlo
 *         del PC; se conecta con la última máquina)
 *   zxv-link-gui --screenshot CAPTURA.png [PROGRAMA]
 *         (para las pruebas: guarda la ventana en la captura y acaba, sin
 *         conectar)
 *
 * Habla el idioma del sistema si es el español y si no, el inglés; con
 * --lang es o --lang en, ese.
 *
 * MIT, como el resto del SDK.
 */
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QTimer>
#include <QTranslator>

#include "linkcore.h"
#include "window.h"
#include "zxv_version.h"

/* Los textos del propio Qt (los botones de los diálogos...) en el idioma del
 * programa: junto a él (windeployqt los deja en translations/qt_es.qm) o los
 * del sistema. */
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
    app.setApplicationName(QStringLiteral("zxv-link-gui"));
    app.setApplicationVersion(QStringLiteral(ZXV_VERSION));
    app.setOrganizationName(QStringLiteral("ZX-V"));

    QCommandLineParser args;
    args.addHelpOption();
    args.addVersionOption();
    QCommandLineOption shot("screenshot", "Guarda la ventana en una imagen y acaba.", "fichero");
    QCommandLineOption lang("lang", "El idioma: es o en.", "idioma");
    args.addOptions({ shot, lang });
    args.addPositionalArgument("programa", "Un programa, listo para ejecutarlo en la placa.");
    args.process(app);

    QString code = args.isSet(lang) ? args.value(lang) : QLocale::system().name().left(2);
    bool spanish = code == "es";
    lc_spanish = spanish;
    QTranslator tr;
    if (!spanish && tr.load(QStringLiteral(":/i18n/zxv-link-gui_en.qm")))
        app.installTranslator(&tr);
    else if (spanish)
        installQtTranslation(app, QStringLiteral("es"));

    QApplication::setWindowIcon(QIcon(QStringLiteral(":/zxv.png")));
    Window w(spanish);
    if (!args.positionalArguments().isEmpty())
        w.setProgram(args.positionalArguments().first());
    w.show();
    if (args.isSet(shot)) {
        QString path = args.value(shot);
        QTimer::singleShot(300, &w, [&w, path] {
            QApplication::exit(w.grab().save(path) ? 0 : 1);
        });
    } else {
        QTimer::singleShot(0, &w, [&w] { w.autoConnect(); });
    }
    return app.exec();
}
