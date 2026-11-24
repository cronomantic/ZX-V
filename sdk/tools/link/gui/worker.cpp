/*
 * worker.cpp - Lo que habla con la máquina en zxv-link-gui (ver worker.h).
 */
#include "worker.h"

#include <QDir>
#include <QElapsedTimer>
#include <QVariantMap>
#include <QFileInfo>
#include <QThread>
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
        .arg(info.updating  ? tr("actualizando el firmware")
             : info.waiting ? tr("el BASIC espera una orden")
                            : tr("con un programa en marcha"));
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
    if (l.unpaired) {                   /* de la red, sin emparejar: solo se puede emparejar */
        on = true;
        emit needPair(QString::fromUtf8(netpc_name(l.net)));
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

void Worker::update(const QString &file)
{
    if (!on) {
        emit finished(false, tr("La placa no está conectada."));
        return;
    }
    QByteArray path = QFileInfo(file).absoluteFilePath().toLocal8Bit();
    char version[17];
    int r = lc_update_version(&l, path.constData(), version);
    if (r >= 0)
        r = lc_update(&l, path.constData());
    if (r < 0) {
        failed(r);
        if (on) {
            lc_info_t info;
            l.cancel = 0;
            if (lc_hello(&l, &info) == 0)
                emit opened(true, describe(info));
        }
        return;
    }
    lc_info_t info;
    if (lc_hello(&l, &info) == 0)
        emit opened(true, describe(info));     /* la versión nueva; vuelve a servir EXT: */
    emit finished(true, tr("Firmware actualizado: ZX-V %1").arg(QString::fromUtf8(version)));
}

void Worker::listNet()
{
    char cfg[1024];
    if (!lc_config_file("zxv-link-net.txt", cfg, sizeof cfg))
        return;
    netpc_found_t found[16];
    int n = netpc_find(cfg, found, 16, 1500);
    QString lines, other;
    for (int i = 0; i < n; i++) {
        QString port = QStringLiteral("%1 (%2)").arg(QString::fromUtf8(found[i].name), QString::fromUtf8(found[i].addr));
        char why[256];
        if (lc_net_compatible(found[i].version, why, sizeof why))
            lines += port + QLatin1Char('\n');
        else
            other += port + QLatin1Char('\t') + QString::fromUtf8(found[i].firmware) + QStringLiteral(": ") +
                     QString::fromUtf8(why) + QLatin1Char('\n');
    }
    emit netPorts(lines, other);
}

void Worker::pair(const QString &pcName)
{
    if (!on || !l.net) {
        emit pairDone(false, tr("La placa no está conectada."));
        return;
    }
    uint32_t code;
    QByteArray name = pcName.left(23).toUtf8();
    if (lc_pair_begin(&l, name.constData(), &code) < 0) {
        emit pairDone(false, QString::fromUtf8(l.error));
        return;
    }
    emit pairCode(code, QString::fromUtf8(netpc_name(l.net)));
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < 120000) {
        if (l.cancel) {
            emit pairDone(false, tr("No se ha emparejado."));
            return;
        }
        int st = lc_pair_poll(&l);
        if (st == ZLN_PAIR_OK) {
            lc_info_t info;
            if (lc_hello(&l, &info) < 0) {
                emit pairDone(false, QString::fromUtf8(l.error));
                return;
            }
            emit pairDone(true, QString());
            timer->start(0);
            emit opened(true, describe(info));
            return;
        }
        if (st == ZLN_PAIR_NO) {
            emit pairDone(false, tr("La máquina ha dicho que no (o se cerró su cuadro)."));
            return;
        }
        QThread::msleep(400);
    }
    emit pairDone(false, tr("Nadie ha aceptado en la máquina en dos minutos."));
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

/* --- La máquina como un explorador (D112) ------------------------------- */

bool Worker::ready()
{
    if (on && !l.unpaired)
        return true;
    emit finished(false, tr("La placa no está conectada."));
    return false;
}

void Worker::list(const QString &path)
{
    if (!on || l.unpaired) {
        emit listFailed(path, tr("La placa no está conectada."));
        return;
    }
    lc_entry_t *e;
    QByteArray p = path.toUtf8();
    int n = lc_list(&l, p.constData(), &e);
    if (n < 0) {
        emit listFailed(path, QString::fromUtf8(l.error));
        lost(n);
        return;
    }
    QVariantList out;
    for (int i = 0; i < n; i++) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), QString::fromUtf8(e[i].name));
        m.insert(QStringLiteral("size"), qint64(e[i].size));
        m.insert(QStringLiteral("dir"), e[i].dir);
        out << m;
    }
    free(e);
    emit listed(path, out);
}

void Worker::put(const QStringList &files, const QString &dir)
{
    if (!ready())
        return;
    QString base = dir.endsWith(QLatin1Char('/')) ? dir : dir + QLatin1Char('/');
    l.overwrite = true;                 /* la ventana ya ha preguntado */
    for (const QString &f : files) {
        QByteArray src = QFileInfo(f).absoluteFilePath().toLocal8Bit();
        QByteArray dest = (base + QFileInfo(f).fileName()).toUtf8();
        int r = lc_send(&l, src.constData(), dest.constData());
        if (r < 0) {
            l.overwrite = false;
            failed(r);
            return;
        }
    }
    l.overwrite = false;
    emit finished(true, tr("%n fichero(s) mandado(s) a %1", nullptr, int(files.size())).arg(dir));
}

void Worker::get(const QStringList &paths, const QString &local)
{
    if (!ready())
        return;
    for (const QString &p : paths) {
        QByteArray src = p.toUtf8();
        QByteArray dest = QDir(local).filePath(p.section(QLatin1Char('/'), -1)).toLocal8Bit();
        int r = lc_get(&l, src.constData(), dest.constData());
        if (r < 0) {
            failed(r);
            return;
        }
    }
    emit finished(true, tr("%n fichero(s) traído(s) a %1", nullptr, int(paths.size())).arg(QDir::toNativeSeparators(local)));
}

/* Una carpeta, con lo que tenga dentro (no más hondo de 16). */
int Worker::removeTree(const QByteArray &path, int depth)
{
    int r = lc_remove(&l, path.constData());
    if (r != ZXV_E_NOTEMPTY || depth >= 16)
        return r;
    lc_entry_t *e;
    int n = lc_list(&l, path.constData(), &e);
    if (n < 0)
        return n;
    for (int i = 0; i < n && r != LC_E_CANCEL; i++) {
        QByteArray child = path + (path.endsWith('/') ? "" : "/") + QByteArray(e[i].name);
        r = e[i].dir ? removeTree(child, depth + 1) : lc_remove(&l, child.constData());
        if (r < 0) {
            free(e);
            return r;
        }
    }
    free(e);
    return lc_remove(&l, path.constData());
}

void Worker::remove(const QStringList &paths)
{
    if (!ready())
        return;
    for (const QString &p : paths) {
        int r = removeTree(p.toUtf8(), 0);
        if (r < 0) {
            failed(r);
            return;
        }
    }
    emit finished(true, tr("%n borrado(s)", nullptr, int(paths.size())));
}

void Worker::rename(const QString &from, const QString &to)
{
    if (!ready())
        return;
    int r = lc_rename(&l, from.toUtf8().constData(), to.toUtf8().constData());
    if (r < 0) {
        failed(r);
        return;
    }
    emit finished(true, tr("Renombrado: %1").arg(to.section(QLatin1Char('/'), -1)));
}

void Worker::mkdir(const QString &path)
{
    if (!ready())
        return;
    int r = lc_mkdir(&l, path.toUtf8().constData());
    if (r < 0) {
        failed(r);
        return;
    }
    emit finished(true, tr("Carpeta creada: %1").arg(path.section(QLatin1Char('/'), -1)));
}

void Worker::runPath(const QString &path, bool stop)
{
    if (!ready())
        return;
    QByteArray p = path.toUtf8();
    l.allow_stop = stop;
    int r = lc_run_path(&l, p.constData());
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
    lc_drive_command(p.constData(), cmd, sizeof cmd);
    emit finished(true, QString::fromUtf8(cmd));
}

void Worker::runPc(const QString &file, bool stop)
{
    if (!ready())
        return;
    QByteArray p = QFileInfo(file).absoluteFilePath().toLocal8Bit();
    l.allow_stop = stop;
    int r = lc_run_pc(&l, p.constData());
    l.allow_stop = false;
    if (r == LC_E_RUNNING) {
        emit ask(AskStop, QString::fromUtf8(l.error));
        return;
    }
    if (r < 0) {
        failed(r);
        return;
    }
    if (l.ext) {
        l.ext->ctx = this;
        l.ext->on_event = onExtEvent;
    }
    emit finished(true, QFileInfo(file).fileName());
}
