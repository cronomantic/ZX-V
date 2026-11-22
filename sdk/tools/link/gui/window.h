/*
 * window.h - La ventana de zxv-link-gui, en pasos de arriba abajo:
 *
 *   1. La placa: el puerto del adaptador en la tira J5 (elegirlo conecta).
 *   2. El programa: elegirlo, arrastrarlo o pulsar un reciente solo lo pone
 *      ahí; nada se manda hasta pulsar un botón.
 *   3. Desde dónde lo carga la placa: la unidad EXT: (una carpeta elegida a
 *      propósito, nunca la del programa, que se sirve mientras la placa está
 *      conectada; el programa tiene que estar dentro) o la tarjeta (con lo
 *      que va con él: los ficheros de su carpeta que puede cargar, marcados,
 *      y los que se añadan; lo que ya está igual no se manda).
 *   4. Los dos botones que mandan: ejecutar en la placa, o solo copiar.
 *   5. El resultado, fichero a fichero.
 *
 * Debajo, los recientes y el informe de la máquina.
 */
#ifndef ZXV_LINK_WINDOW_H
#define ZXV_LINK_WINDOW_H

#include <QHash>
#include <QLocale>
#include <QMainWindow>
#include <QSet>
#include <QSettings>
#include <QThread>

#include "icons.h"

class QCheckBox;
class QComboBox;
class QVBoxLayout;
class QRadioButton;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QTimer;
class Worker;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(bool spanish);
    ~Window() override;

    /* Un programa (.zxv, .bas, .zvb, .tap, .tzx, .txt). */
    static bool isProgram(const QString &file);

    /* El programa del paso 2 (el de la línea de órdenes, uno arrastrado o
     * uno reciente): solo lo pone, con lo que va con él. */
    void setProgram(const QString &file);

    /* Conectar por el último puerto, si está. */
    void autoConnect();

signals:
    void wantOpen(const QString &port, bool spanish);
    void wantClose();
    void wantCheck(const QStringList &files);
    void wantRun(const QStringList &files, bool overwrite, bool stop);
    void wantSend(const QStringList &files, bool overwrite);
    void wantServe(const QString &folder, bool overwrite);
    void wantRunExt(const QString &folder, const QString &file, bool stop);

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private:
    enum class Mode { Run, Copy };
    enum class Link { Off, Connecting, On, Error };

    QWidget *step(int n, const QString &title, QLayout *content);
    void refreshPorts();
    void connectTo(const QString &port);
    void toggleConnection();
    void setLink(Link link, const QString &text);
    void chooseProgram();
    void addFiles();
    void addExtras(const QStringList &files);
    void rebuildFiles();
    void checkFiles();
    void setChecked(const QString &file, int state);
    QStringList chosenFiles() const;
    void go(Mode mode);
    void launch();
    void askUser(int what, const QString &text);
    void resultLine(const QString &key, Ic icon, const QString &text);
    void addRecent(const QString &file);
    void refreshRecents();
    void updateButtons();
    QString bytes(qint64 n) const;
    QString programFile() const;
    bool fromExtDrive() const;
    QString extFolder() const;
    bool insideExt(const QString &file) const;
    void serveExt();
    void chooseExtFolder();
    void updateExt();
    void copyToExt();

    bool spanish;
    QLocale locale;
    QSettings settings;
    QThread thread;
    Worker *worker;
    Link link = Link::Off;
    bool busy = false;
    Mode mode = Mode::Run;
    QStringList extras;                 /* los añadidos a mano */
    QSet<QString> unchecked;            /* los desmarcados */
    QHash<QString, QTreeWidgetItem *> resultItems;
    QTimer *typing;
    QString shown;                      /* el programa de la lista del paso 3 */
    QStringList pending;                /* lo que se manda (y, si hace falta permiso, otra vez) */
    bool allowOverwrite = false, allowStop = false;

    QComboBox *ports;
    QToolButton *refreshButton, *plugButton;
    QLabel *dotLabel, *state;
    QLineEdit *program;
    QPushButton *chooseButton, *addButton, *runButton, *copyButton, *stopButton, *clearButton;
    QTreeWidget *files, *results;
    QRadioButton *fromExt, *fromCard;
    QWidget *extBox, *cardBox, *extWarnBox, *step3;
    QVBoxLayout *topLayout;
    QLineEdit *extPath;
    QPushButton *extChoose, *extCopy, *extUseCard;
    QLabel *extState, *extWarn;
    QCheckBox *extOverwrite;
    QProgressBar *bar;
    QListWidget *recents;
    QPlainTextEdit *logView;
};

#endif // ZXV_LINK_WINDOW_H
