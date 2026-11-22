/*
 * worker.h - Lo que habla con la máquina en zxv-link-gui, en su propio hilo
 * (mandar un fichero tarda; la ventana no se para): abre y cierra el puerto,
 * mira qué hay en la tarjeta, ejecuta y manda ficheros (linkcore.c), y va
 * leyendo el informe.
 */
#ifndef ZXV_LINK_WORKER_H
#define ZXV_LINK_WORKER_H

#include <QObject>
#include <QStringList>

#include "linkcore.h"

class QTimer;

class Worker : public QObject
{
    Q_OBJECT

public:
    Worker();
    ~Worker() override;

    /* Desde la ventana (otro hilo): lo que esté haciendo (y lo que esté en
     * cola) acaba enseguida, con error, hasta arm(). */
    void cancel() { l.cancel = 1; }
    void arm() { l.cancel = 0; }

    /* Lo que se pregunta antes de seguir (ask). */
    enum { AskOverwrite = 1, AskStop = 2 };

public slots:
    void open(const QString &port, bool spanish);
    void close();
    /* Lo que hay en la tarjeta con el nombre de cada fichero: checked(file,
     * LC_CARD_*, -1 no se sabe, -2 no hay tarjeta). */
    void check(const QStringList &files);
    /* files[0], el programa; los demás, lo que carga. Todos a la tarjeta (lo
     * que ya está igual, no) y a ejecutarlo. Sin permiso para sustituir lo
     * que no puso zxv-link (overwrite) o para parar un programa en marcha
     * (stop), no toca nada y lo pregunta (ask). */
    void run(const QStringList &files, bool overwrite, bool stop);
    void send(const QStringList &files, bool overwrite);
    /* Servir folder como la unidad EXT: de la máquina (mientras está
     * conectada; vacío, dejar de servirla). */
    void serve(const QString &folder, bool overwrite);
    /* Ejecutar file (dentro de folder) desde EXT:, sin nada a la tarjeta. */
    void runExt(const QString &folder, const QString &file, bool stop);

signals:
    /* Abierto (con la versión de la máquina en text) o no (con el porqué). */
    void opened(bool ok, const QString &text);
    void closed(const QString &why);
    void checked(const QString &file, int state);
    void log(const QString &text);
    void progress(const QString &name, qint64 done, qint64 total, bool skipped);
    /* Acabado: con run, text es la orden tecleada; con send, cuántos. */
    void finished(bool ok, const QString &text);
    /* No se ha tocado nada: hace falta permiso (AskOverwrite, AskStop). */
    void ask(int what, const QString &text);
    /* Se sirve EXT: (o no, con el porqué). */
    void served(bool ok, const QString &text);
    /* Lo que ha hecho la máquina en EXT: (read, write, remove...). */
    void extEvent(const QString &what, const QString &zpath);

private:
    void pump();
    bool lost(int r);
    void failed(int r);
    QString describe(const lc_info_t &info) const;
    static void onLog(void *ctx, const char *text, unsigned n);
    static void onProgress(void *ctx, const char *name, size_t done, size_t total, bool skipped);
    static void onExtEvent(void *ctx, const char *what, const char *zpath);

    lc_link_t l {};
    bool on = false;
    QTimer *timer = nullptr;
};

#endif // ZXV_LINK_WORKER_H
