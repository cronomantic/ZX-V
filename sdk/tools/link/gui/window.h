/*
 * window.h - La ventana de zxv-link-gui (D112), de arriba abajo:
 *
 *   - La máquina: un puerto del adaptador en la tira J5 o una ZX-V de la red
 *     (elegirla conecta), su estado y Emparejar (las de la red).
 *   - Ejecutar un fichero del PC directo a la memoria: no se copia a la
 *     tarjeta; la máquina lo lee del PC (por EXT:). Lo que cargue después es
 *     cosa del programa.
 *   - La máquina como un explorador: la tarjeta (SD:), la ROM (solo leer) y
 *     EXT: (la carpeta del PC que se le sirve, elegida a propósito). En cada
 *     una: ejecutar, mandar del PC, traer al PC, carpeta nueva, renombrar y
 *     borrar (sustituir y borrar, preguntando), y arrastrar a la lista.
 *   - Abajo, cómo va y el informe de la máquina, plegado.
 *
 * Actualizar el firmware, en el menú Máquina.
 */
#ifndef ZXV_LINK_WINDOW_H
#define ZXV_LINK_WINDOW_H

#include <QLocale>
#include <QMainWindow>
#include <QSet>
#include <QSettings>
#include <QThread>
#include <QVariantList>

#include "icons.h"

class QAction;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QMessageBox;
class Worker;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(bool spanish);
    ~Window() override;

    /* Un programa (.zxv, .bas, .zvb, .tap, .tzx, .txt). */
    static bool isProgram(const QString &file);

    /* El fichero del PC para ejecutar directo a la memoria (el de la línea de
     * órdenes, uno arrastrado o uno reciente): solo lo pone. */
    void setProgram(const QString &file);

    /* Conectar con la última máquina, si está. */
    void autoConnect();

signals:
    void wantOpen(const QString &port, bool spanish);
    void wantClose();
    void wantServe(const QString &folder, bool overwrite);
    void wantUpdate(const QString &file);
    void wantNetPorts();
    void wantPair(const QString &pcName);
    void wantList(const QString &path);
    void wantPut(const QStringList &files, const QString &dir);
    void wantGet(const QStringList &paths, const QString &local);
    void wantRemove(const QStringList &paths);
    void wantRename(const QString &from, const QString &to);
    void wantMkdir(const QString &path);
    void wantRunPath(const QString &path, bool stop);
    void wantRunPc(const QString &file, bool stop);

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void closeEvent(QCloseEvent *e) override;
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    enum class Link { Off, Connecting, On, Unpaired, Error };
    enum class Op { None, RunPc, RunPath, Put, Get, Remove, Rename, Mkdir, Update };
    enum Drive { SD, ROM, EXT, DRIVES };

    /* La máquina. */
    void refreshPorts();
    QString currentPort() const;
    bool isNet(const QString &port) const;
    void connectTo(const QString &port);
    void toggleConnection();
    void setLink(Link link, const QString &text);
    void pairClicked();
    void startPair();
    void askPair(const QString &machine);
    void showPairCode(quint32 code, const QString &machine);
    void updateFirmware();
    void updateLog(const QString &text);

    /* Ejecutar del PC. */
    void chooseProgram();
    void runPc();
    QString pcFile() const;
    void addRecent(const QString &file);
    void refreshRecents();

    /* El explorador. */
    void setDrive(int d);
    QString drivePrefix(int d) const;
    QString machinePath(const QString &name = QString()) const;
    QString extFolder() const;
    QString extLocal(const QString &name = QString()) const;
    void chooseExtFolder();
    void serveExt();
    void refreshList();
    void showEntries(const QVariantList &entries);
    void placeholder(const QString &text);
    QStringList selectedNames(bool *dirs = nullptr) const;
    bool existsHere(const QString &name) const;
    void openItem(QTreeWidgetItem *it);
    void goUp();
    void runSelected();
    void putFiles(const QStringList &files);
    void choosePut();
    void getSelected();
    void newFolder();
    void renameSelected();
    void deleteSelected();
    bool validName(const QString &name);

    /* Lo que se hace y cómo va. */
    void begin(Op op, const QString &text);
    void say(Ic ic, const QString &text, bool error = false);
    void askUser(int what, const QString &text);
    void updateButtons();
    QString bytes(qint64 n) const;

    bool spanish;
    QLocale locale;
    QSettings settings;
    QThread thread;
    Worker *worker;
    Link link = Link::Off;
    Op op = Op::None;
    bool busy = false;
    bool pairWanted = false;            /* Emparejar pulsado: al abrir la máquina, empezar */
    QString runTarget;                  /* lo que se ejecuta (si hay que preguntar y repetir) */
    QString machine;                    /* lo que dice la máquina al conectar */
    QSet<QString> netPorts;             /* las máquinas de la red de la lista (D106) */
    QString updateLine;                 /* lo que falta de una línea del informe (actualizando) */
    QMessageBox *pairBox = nullptr;     /* el código, mientras se empareja */
    int drive = SD;
    QString paths[DRIVES];              /* la carpeta de cada unidad: "/", "/juegos"... */
    QVariantList entries;               /* lo que hay en la carpeta que se ve (SD: y ROM:) */
    bool listing = false;

    QComboBox *ports;
    QToolButton *refreshButton, *plugButton;
    QPushButton *pairButton;
    QLabel *dotLabel, *state;
    QComboBox *pcProgram;
    QPushButton *pcChoose, *pcRun;
    QToolButton *driveButtons[DRIVES];
    QLabel *pathLabel;
    QPushButton *extChoose;
    QAction *actRun, *actPut, *actGet, *actMkdir, *actRename, *actDelete, *actUp, *actRefresh;
    QAction *menuPair, *menuUpdate, *menuSearch;
    QTreeWidget *list;
    QLabel *statusIcon, *status;
    QProgressBar *bar;
    QPushButton *stopButton;
    QToolButton *logToggle;
    QWidget *logBox;
    QPlainTextEdit *logView;
};

#endif // ZXV_LINK_WINDOW_H
