/*
 * worker.cpp - Lo que habla con la máquina en zxv-link-gui (ver worker.h).
 */
#include "worker.h"

#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <vector>

Worker::Worker()
{
    l.ctx = this;
    l.on_log = onLog;
    l.on_progress = onProgress;
}

Worker::~Worker()
{
    if (on)
        lc_close(&l);
}

void Worker::onLog(void *ctx, const char *text, unsigned n)
{
    emit static_cast<Worker *>(ctx)->log(QString::fromUtf8(text, int(n)));
}

void Worker::onProgress(void *ctx, const char *name, size_t done, size_t total, bool skipped)
{
    emit static_cast<Worker *>(ctx)->progress(QString::fromUtf8(name), qint64(done), qint64(total), skipped);
}

void Worker::onExtEvent(void *ctx, const char *what, const char *zpath)
{
    emit static_cast<Worker *>(ctx)->extEvent(QString::fromUtf8(what), QString::fromUtf8(zpath));
}

QString Worker::describe(const lc_info_t &info) const
{
    return tr("%1 · protocolo %2 · %3")
        .arg(QString::fromUtf8(info.firmware))
        .arg(info.protocol)
        .arg(info.waiting ? tr("el BASIC espera una orden") : tr("con un programa en marcha"));
}

void Worker::open(const QString &port, bool spanish)
{
    lc_spanish = spanish;
    if (on)
        close();
    if (!timer) {
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Worker::pump);
    }
    if (lc_open(&l, port.toUtf8().constData(), nullptr) < 0) {
        emit opened(false, QString::fromUtf8(l.error));
        return;
    }
    lc_info_t info;
    if (lc_hello(&l, &info) < 0) {
        emit opened(false, QString::fromUtf8(l.error));
        lc_close(&l);
        return;
    }
    on = true;
    timer->start(0);                    /* lc_pump espera hasta 20 ms: lo que pida EXT:, enseguida */
    emit opened(true, describe(info));
}

void Worker::close()
{
    if (timer)
        timer->stop();
    if (on)
        lc_close(&l);
    on = false;
    emit closed(QString());
}

/* Si el puerto ha fallado (el adaptador se ha desenchufado), se cierra. */
bool Worker::lost(int r)
{
    if (r != LC_E_SERIAL)
        return false;
    QString why = QString::fromUtf8(l.error);
    timer->stop();
    lc_close(&l);
    on = false;
    emit closed(why);
    return true;
}

/* El error de run o send. Si la máquina no contesta, puede haberse
 * reiniciado entera: se la vuelve a saludar y, si contesta, se dice. */
void Worker::failed(int r)
{
    QString why = QString::fromUtf8(l.error);
    if (r == LC_E_NOANSWER && on) {
        lc_info_t info;
        l.cancel = 0;
        if (lc_hello(&l, &info) == 0) {
            emit opened(true, describe(info));
            why += QLatin1Char(' ') + tr("La placa se había reiniciado: ya contesta otra vez.");
        }
    }
    emit finished(false, why);
    lost(r);
}

/* El informe, entre una cosa y otra. */
void Worker::pump()
{
    if (on)
        lost(lc_pump(&l, 20) < 0 ? LC_E_SERIAL : 0);
}

void Worker::serve(const QString &folder, bool overwrite)
{
    if (!on)
        return;
    if (folder.isEmpty()) {
        lc_unserve(&l);
        emit served(false, QString());
        return;
    }
    QDir().mkpath(folder);
    if (lc_serve(&l, QFileInfo(folder).absoluteFilePath().toLocal8Bit().constData(), overwrite) < 0) {
        emit served(false, QString::fromUtf8(l.error));
        return;
    }
    l.ext->ctx = this;
    l.ext->on_event = onExtEvent;
    emit served(true, QDir::toNativeSeparators(folder));
}

void Worker::runExt(const QString &folder, const QString &file, bool stop)
{
    if (!on) {
        emit finished(false, tr("La placa no está conectada."));
        return;
    }
    QByteArray f = QFileInfo(folder).absoluteFilePath().toLocal8Bit(), p = QFileInfo(file).absoluteFilePath().toLocal8Bit();
    l.allow_stop = stop;
    int r = lc_run_ext(&l, f.constData(), p.constData());
    l.allow_stop = false;
    if (r == LC_E_RUNNING) {
        emit ask(AskStop, QString::fromUtf8(l.error));
        return;
    }
    if (r < 0) {
        failed(r);
        return;
    }
    char cmd[400];
    lc_ext_command(f.constData(), p.constData(), cmd, sizeof cmd);
    emit finished(true, QString::fromUtf8(cmd));
}

void Worker::check(const QStringList &files)
{
    int stop = 0;                       /* sin respuesta o sin tarjeta: no se pregunta por cada uno */
    for (const QString &f : files) {
        if (!on || stop) {
            emit checked(f, stop == ZXV_E_NOCARD ? -2 : -1);
            continue;
        }
        QByteArray path = QFileInfo(f).absoluteFilePath().toLocal8Bit();
        int r = lc_card_state(&l, path.constData(), lc_base_name(path.constData()));
        emit checked(f, r >= 0 ? r : r == ZXV_E_NOCARD ? -2 : -1);
        if (r == LC_E_NOANSWER || r == ZXV_E_NOCARD || r == LC_E_CANCEL)
            stop = r;
        lost(r);
    }
}

void Worker::run(const QStringList &files, bool overwrite, bool stop)
{
    if (!on) {
        emit finished(false, tr("La placa no está conectada."));
        return;
    }
    std::vector<QByteArray> paths;
    for (const QString &f : files)
        paths.push_back(QFileInfo(f).absoluteFilePath().toLocal8Bit());
    std::vector<const char *> list;
    for (const QByteArray &p : paths)
        list.push_back(p.constData());
    l.overwrite = overwrite;
    l.allow_stop = stop;
    int r = lc_run(&l, list.data(), int(list.size()));
    l.overwrite = l.allow_stop = false;
    if (r == LC_E_EXISTS || r == LC_E_RUNNING) {
        emit ask(r == LC_E_EXISTS ? AskOverwrite : AskStop, QString::fromUtf8(l.error));
        return;
    }
    if (r < 0) {
        failed(r);
        return;
    }
    char cmd[200];
    lc_load_command(list[0], cmd, sizeof cmd);
    emit finished(true, QString::fromUtf8(cmd));
}

void Worker::send(const QStringList &files, bool overwrite)
{
    if (!on) {
        emit finished(false, tr("La placa no está conectada."));
        return;
    }
    std::vector<QByteArray> paths;
    for (const QString &f : files)
        paths.push_back(QFileInfo(f).absoluteFilePath().toLocal8Bit());
    std::vector<const char *> list;
    for (const QByteArray &p : paths)
        list.push_back(p.constData());
    l.overwrite = overwrite;
    int r = lc_copy(&l, list.data(), int(list.size()));
    l.overwrite = false;
    if (r == LC_E_EXISTS) {
        emit ask(AskOverwrite, QString::fromUtf8(l.error));
        return;
    }
    if (r < 0) {
        failed(r);
        return;
    }
    emit finished(true, tr("%n fichero(s) en la tarjeta", nullptr, int(files.size())));
}
