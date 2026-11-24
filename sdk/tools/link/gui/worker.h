/*
 * worker.h - Lo que habla con la máquina en zxv-link-gui, en su propio hilo
 * (mandar un fichero tarda; la ventana no se para): abre y cierra el puerto,
 * ejecuta, la tarjeta y la ROM como un explorador (D112: listar, mandar,
 * traer, borrar, renombrar, carpetas; linkcore.c) y va leyendo el informe.
 */
#ifndef ZXV_LINK_WORKER_H
#define ZXV_LINK_WORKER_H

#include <QObject>
#include <QStringList>
#include <QVariantList>

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
    /* Actualizar el firmware de la máquina con file, un zxv-update.bin (D105):
     * unos minutos; lo que cuenta la máquina llega por log. Al acabar, se la
     * vuelve a saludar (opened, que vuelve a servir EXT:). */
    void update(const QString &file);
    /* --- La máquina como un explorador (D112). Rutas con su unidad:
     * "SD:/juegos", "ROM:/". Cada cosa acaba con finished (salvo list). */
    /* Lo que hay en path: listed(path, [{name, size, dir}...]) o listFailed. */
    void list(const QString &path);
    /* Ficheros del PC a la carpeta dir de la tarjeta (sustituyendo: quien
     * llama pregunta antes). */
    void put(const QStringList &files, const QString &dir);
    /* De la máquina a la carpeta local del PC (sustituyendo). */
    void get(const QStringList &paths, const QString &local);
    /* Borrar (una carpeta, con lo que tenga dentro), renombrar, crear. */
    void remove(const QStringList &paths);
    void rename(const QString &from, const QString &to);
    void mkdir(const QString &path);
    /* Ejecutar lo que está en la máquina (SD:, ROM:, EXT:), o un fichero
     * del PC directo a la memoria. Con un programa en marcha, sin stop,
     * ask(AskStop). */
    void runPath(const QString &path, bool stop);
    void runPc(const QString &file, bool stop);

    /* Las ZX-V de la red que contestan (D106): netPorts, una por línea; las de
     * otra versión del protocolo de la red, en other: "puerto	qué hacer". */
    void listNet();
    /* Emparejar con la máquina de la red abierta (needPair): pairCode con el
     * código y, al acabar, pairDone (y, si sale bien, opened). */
    void pair(const QString &pcName);

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
    void netPorts(const QString &lines, const QString &other);
    void listed(const QString &path, const QVariantList &entries);
    void listFailed(const QString &path, const QString &why);
    /* Abierta una máquina de la red que no está emparejada con este PC. */
    void needPair(const QString &machine);
    void pairCode(quint32 code, const QString &machine);
    void pairDone(bool ok, const QString &text);

private:
    void pump();
    bool lost(int r);
    void failed(int r);
    QString describe(const lc_info_t &info) const;
    int removeTree(const QByteArray &path, int depth);
    bool ready();
    static void onLog(void *ctx, const char *text, unsigned n);
    static void onProgress(void *ctx, const char *name, size_t done, size_t total, bool skipped);
    static void onExtEvent(void *ctx, const char *what, const char *zpath);

    lc_link_t l {};
    bool on = false;
    QTimer *timer = nullptr;
};

#endif // ZXV_LINK_WORKER_H
