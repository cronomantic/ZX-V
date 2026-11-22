/*
 * window.cpp - La ventana de zxv-link-gui (ver window.h).
 */
#include "window.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <cstdlib>

#include "linkcore.h"
#include "serial.h"
#include "worker.h"
#include "zxv_version.h"

#define RECENT_MAX      10
#define FOLDER_MAX      64

/* Las columnas del paso 3. */
enum { COL_NAME, COL_SIZE, COL_CARD };

static QLabel *title(const QString &text)
{
    auto *l = new QLabel(text);
    QFont f = l->font();
    f.setBold(true);
    l->setFont(f);
    return l;
}

static QLabel *hint(const QString &text)
{
    auto *l = new QLabel(text);
    l->setWordWrap(true);
    QPalette pal = l->palette();
    pal.setColor(QPalette::WindowText, pal.color(QPalette::PlaceholderText));
    l->setPalette(pal);
    return l;
}

static QFrame *line()
{
    auto *f = new QFrame;
    f->setFrameShape(QFrame::HLine);
    f->setFrameShadow(QFrame::Sunken);
    return f;
}

Window::Window(bool spanish)
    : spanish(spanish), locale(spanish ? QLocale::Spanish : QLocale::English),
      settings(QStringLiteral("ZX-V"), QStringLiteral("zxv-link-gui"))
{
    setWindowTitle(tr("ZX-V: la placa"));
    setAcceptDrops(true);

    /* La cabecera: el logo y para qué es. */
    auto *head = new QHBoxLayout;
    head->addWidget(new Logo(34));
    head->addSpacing(14);
    auto *headText = new QVBoxLayout;
    auto *name = new QLabel(tr("Carga remota"));
    QFont big = name->font();
    big.setPointSizeF(big.pointSizeF() * 1.5);
    big.setBold(true);
    name->setFont(big);
    headText->addWidget(name);
    auto *what = hint(tr("Programas del PC a la placa, por la tira J5"));
    what->setWordWrap(false);
    headText->addWidget(what);
    head->addLayout(headText);
    head->addStretch(1);
    auto *version = hint(QStringLiteral("zxv-link-gui " ZXV_VERSION));
    head->addWidget(version, 0, Qt::AlignBottom);

    /* 1. La placa. */
    ports = new QComboBox;
    ports->setMinimumContentsLength(12);
    ports->setToolTip(tr("El puerto serie del adaptador USB-serie en la tira J5 de la placa: al elegirlo, se conecta"));
    refreshButton = new QToolButton;
    refreshButton->setIcon(icon(Ic::Refresh));
    refreshButton->setToolTip(tr("Vuelve a mirar qué puertos serie hay (al enchufar el adaptador)"));
    dotLabel = new QLabel;
    state = new QLabel;
    state->setTextInteractionFlags(Qt::TextSelectableByMouse);
    state->setWordWrap(true);
    plugButton = new QToolButton;
    plugButton->setIcon(icon(Ic::Plug));
    plugButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto *board = new QHBoxLayout;
    board->addWidget(ports);
    board->addWidget(refreshButton);
    board->addSpacing(10);
    board->addWidget(dotLabel);
    board->addSpacing(2);
    board->addWidget(state, 1);
    board->addWidget(plugButton);

    /* 2. El programa. */
    program = new QLineEdit;
    program->setClearButtonEnabled(true);
    program->setPlaceholderText(tr("Elige un programa (.zxv, .bas, .zvb, .tap) o arrástralo a la ventana"));
    chooseButton = new QPushButton(icon(Ic::Folder), tr("Elegir..."));
    auto *progRow = new QHBoxLayout;
    progRow->addWidget(program, 1);
    progRow->addWidget(chooseButton);
    auto *progBox = new QVBoxLayout;
    progBox->addLayout(progRow);
    progBox->addWidget(hint(tr("También puedes arrastrarlo a la ventana o pulsar uno de los recientes. "
                               "Solo lo pone aquí: nada se manda hasta pulsar un botón del paso 4.")));

    /* 3. Desde dónde lo carga la placa: la unidad EXT: (una carpeta elegida a
     * propósito, que se sirve mientras la placa está conectada) o la
     * tarjeta (con lo que va con él). */
    fromExt = new QRadioButton(tr("EXT:"));
    fromCard = new QRadioButton(tr("La tarjeta"));
    auto *fromGroup = new QButtonGroup(this);
    fromGroup->addButton(fromExt);
    fromGroup->addButton(fromCard);
    auto *extRow = new QHBoxLayout;
    extRow->addWidget(fromExt);
    extRow->addWidget(hint(tr("la carpeta compartida, sin copiar nada a la tarjeta")), 1);
    auto *cardRow = new QHBoxLayout;
    cardRow->addWidget(fromCard);
    cardRow->addWidget(hint(tr("copia antes el programa y sus ficheros")), 1);

    extPath = new QLineEdit;
    extPath->setReadOnly(true);
    extPath->setToolTip(tr("La carpeta que es la unidad EXT: de la máquina mientras la placa está conectada: la "
                           "máquina puede leer y escribir en ella"));
    extChoose = new QPushButton(icon(Ic::Folder), tr("Elegir..."));
    auto *extPathRow = new QHBoxLayout;
    extPathRow->addWidget(new QLabel(tr("EXT: es")));
    extPathRow->addWidget(extPath, 1);
    extPathRow->addWidget(extChoose);
    extState = hint(QString());
    extWarn = new QLabel;
    extWarn->setWordWrap(true);
    extCopy = new QPushButton(icon(Ic::Copy), tr("Copiarlo a esa carpeta"));
    extUseCard = new QPushButton(tr("Ejecutar desde la tarjeta"));
    auto *extWarnRow = new QHBoxLayout;
    extWarnRow->addWidget(extCopy);
    extWarnRow->addWidget(extUseCard);
    extWarnRow->addStretch(1);
    extWarnBox = new QWidget;
    auto *extWarnLayout = new QVBoxLayout(extWarnBox);
    extWarnLayout->setContentsMargins(0, 0, 0, 0);
    auto *warnRow = new QHBoxLayout;
    auto *warnIcon = new QLabel;
    warnIcon->setPixmap(icon(Ic::Warning).pixmap(18, 18));
    warnRow->addWidget(warnIcon, 0, Qt::AlignTop);
    warnRow->addWidget(extWarn, 1);
    extWarnLayout->addLayout(warnRow);
    extWarnLayout->addLayout(extWarnRow);
    extOverwrite = new QCheckBox(tr("Dejar que la máquina sustituya o borre ficheros que no escribió ella"));
    extBox = new QFrame;
    static_cast<QFrame *>(extBox)->setFrameShape(QFrame::StyledPanel);
    auto *extLayout = new QVBoxLayout(extBox);
    extLayout->addLayout(extPathRow);
    extLayout->addWidget(extState);
    extLayout->addWidget(extWarnBox);
    extLayout->addWidget(extOverwrite);

    files = new QTreeWidget;
    files->setColumnCount(3);
    files->setHeaderLabels({ tr("Fichero"), tr("Tamaño"), tr("En la tarjeta") });
    files->setRootIsDecorated(false);
    files->setUniformRowHeights(true);
    files->setMinimumHeight(110);
    files->header()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    files->header()->setSectionResizeMode(COL_SIZE, QHeaderView::ResizeToContents);
    files->header()->setSectionResizeMode(COL_CARD, QHeaderView::ResizeToContents);
    files->header()->setStretchLastSection(false);
    files->setToolTip(tr("Los ficheros de la carpeta del programa que puede cargar (gráficos, música, sonidos, "
                         "datos...) y los que añadas: los marcados van a la raíz de la tarjeta"));
    addButton = new QPushButton(icon(Ic::Add), tr("Añadir ficheros..."));
    auto *filesRow = new QHBoxLayout;
    filesRow->addWidget(addButton);
    filesRow->addStretch(1);
    cardBox = new QWidget;
    auto *filesBox = new QVBoxLayout(cardBox);
    filesBox->setContentsMargins(0, 0, 0, 0);
    filesBox->addWidget(files, 1);
    filesBox->addLayout(filesRow);
    auto *sourceBox = new QVBoxLayout;
    sourceBox->addLayout(extRow);
    sourceBox->addLayout(cardRow);
    sourceBox->addWidget(extBox);
    sourceBox->addWidget(cardBox, 1);

    /* 4. Los dos botones que mandan. */
    runButton = new QPushButton(icon(Ic::Play), tr("Ejecutar en la placa"));
    runButton->setToolTip(tr("Manda el programa y los ficheros marcados (lo que ya está igual, no) y lo ejecuta"));
    copyButton = new QPushButton(icon(Ic::Copy), tr("Solo copiar a la tarjeta"));
    copyButton->setToolTip(tr("Manda el programa (si hay) y los ficheros marcados, sin ejecutar nada"));
    QFont bold = runButton->font();
    bold.setBold(true);
    runButton->setFont(bold);
    runButton->setDefault(true);
    for (QPushButton *b : { runButton, copyButton }) {
        b->setIconSize(QSize(24, 24));
        b->setMinimumHeight(40);
        b->setMinimumWidth(220);
    }
    /* Mientras manda: parar (lo que se esté esperando acaba enseguida). */
    stopButton = new QPushButton(icon(Ic::Error), tr("Parar"));
    stopButton->setToolTip(tr("Deja de mandar (lo que ya está en la tarjeta se queda)"));
    stopButton->setIconSize(QSize(24, 24));
    stopButton->setMinimumHeight(40);
    stopButton->hide();
    auto *goRow = new QHBoxLayout;
    goRow->addWidget(runButton);
    goRow->addWidget(copyButton);
    goRow->addWidget(stopButton);
    goRow->addStretch(1);

    /* 5. El resultado. */
    results = new QTreeWidget;
    results->setColumnCount(1);
    results->setHeaderHidden(true);
    results->setRootIsDecorated(false);
    results->setUniformRowHeights(true);
    results->setMinimumHeight(80);
    results->setSelectionMode(QAbstractItemView::NoSelection);
    bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setTextVisible(false);
    bar->setMaximumHeight(8);
    auto *resultBox = new QVBoxLayout;
    resultBox->addWidget(results, 1);
    resultBox->addWidget(bar);

    auto *top = new QWidget;
    topLayout = new QVBoxLayout(top);
    topLayout->addLayout(head);
    topLayout->addWidget(line());
    topLayout->addWidget(step(1, tr("Placa"), board));
    topLayout->addWidget(line());
    topLayout->addWidget(step(2, tr("Programa"), progBox));
    topLayout->addWidget(line());
    step3 = step(3, tr("Desde dónde lo carga la placa"), sourceBox);
    topLayout->addWidget(step3, 2);
    topLayout->addWidget(line());
    topLayout->addWidget(step(4, QString(), goRow));
    topLayout->addWidget(line());
    topLayout->addWidget(step(5, tr("Resultado"), resultBox), 1);

    /* Debajo: los recientes y el informe. */
    recents = new QListWidget;
    recents->setToolTip(tr("Un clic: lo pone en el paso 2"));
    recents->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    recents->setTextElideMode(Qt::ElideMiddle);
    auto *recentBox = new QWidget;
    auto *recentLayout = new QVBoxLayout(recentBox);
    recentLayout->setContentsMargins(0, 0, 0, 0);
    recentLayout->addWidget(title(tr("Recientes")));
    recentLayout->addWidget(recents, 1);

    logView = new QPlainTextEdit;
    logView->setReadOnly(true);
    logView->setMaximumBlockCount(5000);
    logView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    logView->setToolTip(tr("Lo que cuenta la máquina cada 5 segundos: el enlace, el sonido, la tarjeta, el USB y el "
                           "reloj"));
    clearButton = new QPushButton(icon(Ic::Clear), tr("Limpiar"));
    auto *logHead = new QHBoxLayout;
    logHead->addWidget(title(tr("Informe de la máquina")));
    logHead->addStretch(1);
    logHead->addWidget(clearButton);
    auto *logBox = new QWidget;
    auto *logLayout = new QVBoxLayout(logBox);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->addLayout(logHead);
    logLayout->addWidget(logView, 1);

    auto *bottom = new QSplitter;
    bottom->addWidget(recentBox);
    bottom->addWidget(logBox);
    bottom->setStretchFactor(0, 2);
    bottom->setStretchFactor(1, 3);
    bottom->setContentsMargins(9, 0, 9, 9);

    auto *split = new QSplitter(Qt::Vertical);
    split->addWidget(top);
    split->addWidget(bottom);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);
    split->setChildrenCollapsible(false);
    setCentralWidget(split);
    resize(920, 940);
    split->setSizes({ 660, 240 });

    /* El hilo que habla con la máquina. */
    worker = new Worker;
    worker->moveToThread(&thread);
    connect(&thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Window::wantOpen, worker, &Worker::open);
    connect(this, &Window::wantClose, worker, &Worker::close);
    connect(this, &Window::wantCheck, worker, &Worker::check);
    connect(this, &Window::wantRun, worker, &Worker::run);
    connect(this, &Window::wantSend, worker, &Worker::send);
    connect(this, &Window::wantServe, worker, &Worker::serve);
    connect(this, &Window::wantRunExt, worker, &Worker::runExt);
    connect(worker, &Worker::served, this, [this](bool ok, const QString &text) {
        extState->setText(ok ? tr("Se sirve como EXT: mientras la placa está conectada.")
                             : text.isEmpty() ? tr("Se servirá como EXT: al conectar la placa.")
                                              : tr("No se puede servir: %1").arg(text));
    });
    connect(worker, &Worker::extEvent, this, [this](const QString &what, const QString &zpath) {
        QString verb = what == "read" ? tr("ha leído") : what == "write" ? tr("ha escrito")
                     : what == "remove" ? tr("ha borrado") : what == "rename" ? tr("ha renombrado a")
                     : tr("ha creado la carpeta");
        resultLine(QStringLiteral("ext:") + what + zpath, what == "read" ? Ic::File : Ic::Done,
                   tr("EXT: %1 %2").arg(verb, zpath));
    });
    connect(worker, &Worker::opened, this, [this](bool ok, const QString &text) {
        if (!ok) {
            setLink(Link::Error, text);
            return;
        }
        setLink(Link::On, tr("Conectada: %1").arg(text));
        settings.setValue(QStringLiteral("port"), ports->currentText());
        checkFiles();
        serveExt();
    });
    connect(worker, &Worker::closed, this, [this](const QString &why) {
        if (why.isEmpty())
            setLink(Link::Off, tr("Sin conectar"));
        else
            setLink(Link::Error, why);
    });
    connect(worker, &Worker::checked, this, &Window::setChecked);
    connect(worker, &Worker::ask, this, &Window::askUser);
    connect(worker, &Worker::log, this, [this](const QString &text) {
        logView->moveCursor(QTextCursor::End);
        logView->insertPlainText(text);
        logView->moveCursor(QTextCursor::End);
    });
    connect(worker, &Worker::progress, this, [this](const QString &name, qint64 done, qint64 total, bool skipped) {
        if (skipped)
            resultLine(name, Ic::Same, tr("%1: ya estaba igual en la tarjeta, no se manda").arg(name));
        else if (done < total)
            resultLine(name, Ic::Sending, tr("%1: mandando, %2 de %3 bytes").arg(name, bytes(done), bytes(total)));
        else
            resultLine(name, Ic::Done, tr("%1: mandado (%2 bytes)").arg(name, bytes(total)));
        bar->setValue(total ? int(done * 100 / total) : 100);
        /* Ya está en la tarjeta: en el paso 3, también. */
        if (skipped || done == total)
            for (int i = 0; i < files->topLevelItemCount(); i++) {
                QString f = files->topLevelItem(i)->data(COL_NAME, Qt::UserRole).toString();
                if (!f.isEmpty() && QFileInfo(f).fileName() == name)
                    setChecked(f, 1);
            }
    });
    connect(worker, &Worker::finished, this, [this](bool ok, const QString &text) {
        busy = false;
        worker->arm();                  /* tras Parar: el informe sigue llegando */
        if (!ok) {
            for (auto it = resultItems.begin(); it != resultItems.end(); ++it)
                if (it.value()->data(0, Qt::UserRole).toBool())
                    resultLine(it.key(), Ic::Error, tr("%1: no se ha mandado").arg(it.key()));
            resultLine(QStringLiteral("\x01"), Ic::Error, tr("Error: %1").arg(text));
            results->topLevelItem(results->topLevelItemCount() - 1)->setForeground(0, QColor(0xD7, 0, 0));
        } else if (mode == Mode::Run) {
            resultLine(QStringLiteral("\x01"), Ic::Play, tr("%1: en marcha").arg(text));
        } else {
            resultLine(QStringLiteral("\x01"), Ic::Done, text);
        }
        bar->setValue(ok ? 100 : 0);
        statusBar()->showMessage(ok ? (mode == Mode::Run ? tr("En marcha en la placa") : text) : text,
                                 ok ? 6000 : 15000);
        updateButtons();
    });
    thread.start();

    typing = new QTimer(this);
    typing->setSingleShot(true);
    typing->setInterval(400);
    connect(typing, &QTimer::timeout, this, &Window::rebuildFiles);
    connect(program, &QLineEdit::textChanged, typing, qOverload<>(&QTimer::start));
    connect(ports, &QComboBox::activated, this, [this] { connectTo(ports->currentText()); });
    connect(refreshButton, &QToolButton::clicked, this, &Window::refreshPorts);
    connect(plugButton, &QToolButton::clicked, this, &Window::toggleConnection);
    connect(chooseButton, &QPushButton::clicked, this, &Window::chooseProgram);
    connect(addButton, &QPushButton::clicked, this, &Window::addFiles);
    connect(files, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *it) {
        QString f = it->data(COL_NAME, Qt::UserRole).toString();
        if (f.isEmpty())
            return;
        if (it->checkState(COL_NAME) == Qt::Checked)
            unchecked.remove(f);
        else
            unchecked.insert(f);
        updateButtons();
    });
    connect(runButton, &QPushButton::clicked, this, [this] { go(Mode::Run); });
    connect(copyButton, &QPushButton::clicked, this, [this] { go(Mode::Copy); });
    connect(stopButton, &QPushButton::clicked, this, [this] {
        stopButton->setEnabled(false);
        stopButton->setText(tr("Parando..."));
        worker->cancel();
    });
    connect(clearButton, &QPushButton::clicked, logView, &QPlainTextEdit::clear);
    connect(recents, &QListWidget::itemClicked, this, [this](QListWidgetItem *it) {
        QString f = it->data(Qt::UserRole).toString();
        if (QFileInfo(f).isFile())
            setProgram(f);
        else
            statusBar()->showMessage(tr("%1 ya no existe").arg(QDir::toNativeSeparators(f)), 8000);
    });

    connect(extChoose, &QPushButton::clicked, this, &Window::chooseExtFolder);
    connect(extCopy, &QPushButton::clicked, this, &Window::copyToExt);
    connect(extUseCard, &QPushButton::clicked, this, [this] { fromCard->setChecked(true); });
    connect(fromExt, &QRadioButton::toggled, this, [this](bool on) {
        settings.setValue(QStringLiteral("from"), on ? QStringLiteral("ext") : QStringLiteral("card"));
        updateExt();
    });
    connect(extOverwrite, &QCheckBox::toggled, this, [this](bool on) {
        settings.setValue(QStringLiteral("extOverwrite"), on);
        serveExt();
    });

    extPath->setText(QDir::toNativeSeparators(extFolder()));
    {
        QSignalBlocker b1(fromExt), b2(extOverwrite);
        extOverwrite->setChecked(settings.value(QStringLiteral("extOverwrite"), false).toBool());
        (settings.value(QStringLiteral("from"), QStringLiteral("ext")).toString() == "card" ? fromCard : fromExt)
            ->setChecked(true);
    }
    extState->setText(tr("Se servirá como EXT: al conectar la placa."));
    setLink(Link::Off, tr("Sin conectar"));
    refreshPorts();
    refreshRecents();
    rebuildFiles();
}

bool Window::fromExtDrive() const
{
    return fromExt->isChecked();
}

/* La carpeta de EXT:: la elegida, o Documentos/ZX-V-EXT (no dentro de
 * Documentos/ZX-V, que es la tarjeta del emulador). Nunca la del programa:
 * la máquina puede crear ficheros en ella. */
QString Window::extFolder() const
{
    QString f = settings.value(QStringLiteral("extFolder")).toString();
    if (f.isEmpty())
        f = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QStringLiteral("/ZX-V-EXT");
    return QFileInfo(f).absoluteFilePath();
}

bool Window::insideExt(const QString &file) const
{
    char cmd[400];
    return !file.isEmpty() && lc_ext_command(extFolder().toLocal8Bit().constData(),
                                             QFileInfo(file).absoluteFilePath().toLocal8Bit().constData(), cmd,
                                             sizeof cmd) == 0;
}

void Window::serveExt()
{
    if (link == Link::On)
        emit wantServe(extFolder(), extOverwrite->isChecked());
}

void Window::chooseExtFolder()
{
    QString f = QFileDialog::getExistingDirectory(this, tr("La carpeta de la unidad EXT:"), extFolder());
    if (f.isEmpty())
        return;
    settings.setValue(QStringLiteral("extFolder"), QFileInfo(f).absoluteFilePath());
    extPath->setText(QDir::toNativeSeparators(extFolder()));
    serveExt();
    updateExt();
}

/* Lo del paso 3 que depende de desde dónde, del programa y de la carpeta. */
void Window::updateExt()
{
    bool ext = fromExtDrive();
    cardBox->setVisible(!ext);
    topLayout->setStretchFactor(step3, ext ? 0 : 2);    /* sin la lista, el alto para el resultado */
    QString prog = programFile();
    bool outside = ext && !prog.isEmpty() && !insideExt(prog);
    extWarnBox->setVisible(outside);
    if (outside)
        extWarn->setText(tr("El programa no está dentro de %1: para ejecutarlo desde EXT:, cópialo a esa carpeta "
                            "(o elige otra que lo contenga); o ejecútalo desde la tarjeta.")
                             .arg(QDir::toNativeSeparators(extFolder())));
    updateButtons();
}

/* El programa y los ficheros que carga, a la raíz de la carpeta de EXT:
 * (preguntando antes de sustituir algo), y desde ahí el programa. */
void Window::copyToExt()
{
    QString prog = programFile(), dest = extFolder();
    if (prog.isEmpty())
        return;
    QDir().mkpath(dest);
    QStringList list { prog };
    list += chosenFiles();
    QStringList clash;
    for (const QString &f : list) {
        QFileInfo to(dest + QLatin1Char('/') + QFileInfo(f).fileName());
        if (to.exists())
            clash << to.fileName();
    }
    if (!clash.isEmpty() &&
        QMessageBox::question(this, tr("Copiar a la carpeta de EXT:"),
                              tr("En %1 ya hay: %2.\n\n¿Sustituirlos?")
                                  .arg(QDir::toNativeSeparators(dest), clash.join(QStringLiteral(", "))),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    for (const QString &f : list) {
        QString to = dest + QLatin1Char('/') + QFileInfo(f).fileName();
        QFile::remove(to);
        if (!QFile::copy(f, to)) {
            statusBar()->showMessage(tr("No se puede copiar %1").arg(QDir::toNativeSeparators(f)), 8000);
            return;
        }
    }
    setProgram(dest + QLatin1Char('/') + QFileInfo(prog).fileName());
    statusBar()->showMessage(tr("Copiado a %1: ahora el programa es esa copia").arg(QDir::toNativeSeparators(dest)),
                             8000);
}

Window::~Window()
{
    thread.quit();
    thread.wait();
}

/* Un paso: su número, su título y lo suyo. */
QWidget *Window::step(int n, const QString &text, QLayout *content)
{
    auto *w = new QWidget;
    auto *g = new QGridLayout(w);
    g->setContentsMargins(0, 2, 0, 2);
    g->setHorizontalSpacing(12);
    auto *badge = new QLabel;
    badge->setPixmap(stepBadge(n));
    g->addWidget(badge, 0, 0, 2, 1, Qt::AlignTop);
    if (text.isEmpty()) {
        g->addLayout(content, 0, 1, 2, 1);
    } else {
        g->addWidget(title(text), 0, 1);
        g->addLayout(content, 1, 1);
        g->setRowStretch(1, 1);
    }
    g->setColumnStretch(1, 1);
    return w;
}

bool Window::isProgram(const QString &file)
{
    static const QStringList ext { "zxv", "bas", "txt", "zvb", "tap", "tzx" };
    return ext.contains(QFileInfo(file).suffix().toLower());
}

QString Window::bytes(qint64 n) const
{
    return locale.toString(n);
}

/* El del paso 2, si es un programa que existe. */
QString Window::programFile() const
{
    QFileInfo fi(program->text().trimmed());
    return fi.isFile() && isProgram(fi.filePath()) ? fi.absoluteFilePath() : QString();
}

void Window::setProgram(const QString &file)
{
    {
        QSignalBlocker block(program);
        program->setText(QDir::toNativeSeparators(QFileInfo(file).absoluteFilePath()));
    }
    typing->stop();
    settings.setValue(QStringLiteral("dir"), QFileInfo(file).absolutePath());
    rebuildFiles();
    statusBar()->showMessage(tr("Puesto: nada se manda hasta pulsar un botón del paso 4"), 6000);
}

void Window::autoConnect()
{
    QString port = settings.value(QStringLiteral("port")).toString();
    if (!port.isEmpty() && ports->findText(port, Qt::MatchFixedString) >= 0)
        connectTo(ports->itemText(ports->findText(port, Qt::MatchFixedString)));
}

void Window::refreshPorts()
{
    char list[4096];
    serial_list(list, sizeof list);
    QString keep = ports->currentText().isEmpty() ? settings.value(QStringLiteral("port")).toString()
                                                  : ports->currentText();
    ports->clear();
    for (const QString &p : QString::fromLocal8Bit(list).split(QLatin1Char('\n'), Qt::SkipEmptyParts))
        ports->addItem(p.trimmed());
    int i = ports->findText(keep, Qt::MatchFixedString);
    ports->setCurrentIndex(i >= 0 ? i : ports->count() ? 0 : -1);
    if (!ports->count() && link != Link::On)
        setLink(Link::Off, tr("No hay puertos serie: enchufa el adaptador USB-serie de J5 y pulsa el botón de "
                              "buscar"));
    updateButtons();
}

void Window::connectTo(const QString &port)
{
    if (busy || port.isEmpty())
        return;
    setLink(Link::Connecting, tr("Conectando con %1...").arg(port));
    emit wantOpen(port, spanish);
}

void Window::toggleConnection()
{
    if (busy)
        return;
    if (link == Link::On || link == Link::Connecting)
        emit wantClose();
    else
        connectTo(ports->currentText());
}

void Window::setLink(Link l, const QString &text)
{
    link = l;
    dotLabel->setPixmap(dot(l == Link::On ? Dot::On : l == Link::Connecting ? Dot::Busy
                            : l == Link::Error                         ? Dot::Error
                                                                       : Dot::Off));
    state->setText(text);
    bool on = l == Link::On || l == Link::Connecting;
    plugButton->setText(on ? tr("Desconectar") : tr("Conectar"));
    plugButton->setToolTip(on ? tr("Suelta el puerto (para usarlo con otro programa, como la extensión de VS Code)")
                              : tr("Conecta por el puerto elegido"));
    if (l != Link::On)
        for (int i = 0; i < files->topLevelItemCount(); i++) {
            QTreeWidgetItem *it = files->topLevelItem(i);
            if (!it->data(COL_NAME, Qt::UserRole).toString().isEmpty()) {
                it->setIcon(COL_CARD, QIcon());
                it->setText(COL_CARD, tr("se mira al conectar"));
            }
        }
    updateButtons();
}

void Window::chooseProgram()
{
    QString dir = settings.value(QStringLiteral("dir")).toString();
    QString f = QFileDialog::getOpenFileName(this, tr("El programa"), dir,
                                             tr("Programas de ZX-V (*.zxv *.bas *.zvb *.tap *.tzx *.txt)"));
    if (!f.isEmpty())
        setProgram(f);
}

void Window::addFiles()
{
    QString dir = settings.value(QStringLiteral("dir")).toString();
    QStringList f = QFileDialog::getOpenFileNames(this, tr("Ficheros que van a la tarjeta"), dir);
    if (f.isEmpty())
        return;
    settings.setValue(QStringLiteral("dir"), QFileInfo(f.first()).absolutePath());
    addExtras(f);
}

void Window::addExtras(const QStringList &list)
{
    for (const QString &f : list) {
        QString abs = QFileInfo(f).absoluteFilePath();
        if (!extras.contains(abs) && abs != programFile())
            extras << abs;
        unchecked.remove(abs);
    }
    rebuildFiles();
}

/* El paso 3: los ficheros de la carpeta del programa que puede cargar, y
 * los añadidos. Con otro programa, se empieza de nuevo. */
void Window::rebuildFiles()
{
    QString prog = programFile();
    if (prog != shown) {
        if (!shown.isEmpty()) {         /* los añadidos sin programa se quedan */
            extras.clear();
            unchecked.clear();
        }
        shown = prog;
    }
    QStringList list;
    if (!prog.isEmpty()) {
        QByteArray p = prog.toLocal8Bit();
        char *found[FOLDER_MAX];
        int n = lc_folder_files(p.constData(), found, FOLDER_MAX);
        for (int i = 0; i < n; i++) {
            list << QFileInfo(QString::fromLocal8Bit(found[i])).absoluteFilePath();
            std::free(found[i]);
        }
    }
    for (const QString &f : extras)
        if (!list.contains(f))
            list << f;

    QSignalBlocker block(files);
    files->clear();
    for (const QString &f : list) {
        QFileInfo fi(f);
        auto *it = new QTreeWidgetItem(files);
        it->setIcon(COL_NAME, icon(Ic::File));
        it->setText(COL_NAME, fi.fileName());
        it->setToolTip(COL_NAME, QDir::toNativeSeparators(f));
        it->setData(COL_NAME, Qt::UserRole, f);
        it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        it->setCheckState(COL_NAME, unchecked.contains(f) ? Qt::Unchecked : Qt::Checked);
        it->setText(COL_SIZE, bytes(fi.size()));
        it->setTextAlignment(COL_SIZE, Qt::AlignRight | Qt::AlignVCenter);
        it->setText(COL_CARD, link == Link::On ? tr("mirando...") : tr("se mira al conectar"));
    }
    if (list.isEmpty()) {
        auto *it = new QTreeWidgetItem(files);
        it->setText(COL_NAME, prog.isEmpty() ? tr("Elige un programa en el paso 2, o añade ficheros sueltos")
                                             : tr("Ninguno: en su carpeta no hay nada que pueda cargar"));
        it->setFlags(Qt::NoItemFlags);
        it->setFirstColumnSpanned(true);
    }
    checkFiles();
    updateExt();
}

/* Qué hay ya en la tarjeta (con la placa conectada). */
void Window::checkFiles()
{
    if (link != Link::On || busy)
        return;
    QStringList list;
    for (int i = 0; i < files->topLevelItemCount(); i++) {
        QString f = files->topLevelItem(i)->data(COL_NAME, Qt::UserRole).toString();
        if (!f.isEmpty())
            list << f;
    }
    if (!list.isEmpty())
        emit wantCheck(list);
}

/* Lo que hay en la tarjeta con su nombre (LC_CARD_*, -1 no se sabe, -2 no
 * hay tarjeta). */
void Window::setChecked(const QString &file, int state)
{
    for (int i = 0; i < files->topLevelItemCount(); i++) {
        QTreeWidgetItem *it = files->topLevelItem(i);
        if (it->data(COL_NAME, Qt::UserRole).toString() != file)
            continue;
        QSignalBlocker block(files);
        switch (state) {
        case LC_CARD_SAME:
            it->setIcon(COL_CARD, icon(Ic::Same));
            it->setText(COL_CARD, tr("ya está igual: no se manda"));
            break;
        case LC_CARD_ABSENT:
            it->setIcon(COL_CARD, icon(Ic::Sending));
            it->setText(COL_CARD, tr("no está: se manda"));
            break;
        case LC_CARD_OURS:
            it->setIcon(COL_CARD, icon(Ic::Sending));
            it->setText(COL_CARD, tr("ha cambiado: se manda"));
            break;
        case LC_CARD_OTHER:
            it->setIcon(COL_CARD, icon(Ic::Warning));
            it->setText(COL_CARD, tr("hay otro distinto, que no puso zxv-link: se preguntará"));
            break;
        case -2:
            it->setIcon(COL_CARD, icon(Ic::Error));
            it->setText(COL_CARD, tr("no hay tarjeta SD en la placa"));
            break;
        default:
            it->setIcon(COL_CARD, QIcon());
            it->setText(COL_CARD, tr("no se sabe"));
            break;
        }
    }
}

QStringList Window::chosenFiles() const
{
    QStringList list;
    for (int i = 0; i < files->topLevelItemCount(); i++) {
        QTreeWidgetItem *it = files->topLevelItem(i);
        QString f = it->data(COL_NAME, Qt::UserRole).toString();
        if (!f.isEmpty() && it->checkState(COL_NAME) == Qt::Checked)
            list << f;
    }
    return list;
}

/* Los botones del paso 4. Si no está conectada, antes se conecta (las
 * peticiones van al hilo en orden). */
void Window::go(Mode m)
{
    if (busy)
        return;
    QString prog = programFile();
    QStringList list = chosenFiles();
    if (!prog.isEmpty())
        list.prepend(prog);
    if (list.isEmpty() || (m == Mode::Run && prog.isEmpty()))
        return;
    bool ext = m == Mode::Run && fromExtDrive();
    if (ext && !insideExt(prog)) {
        updateExt();
        statusBar()->showMessage(tr("El programa no está dentro de la carpeta de EXT:"), 8000);
        return;
    }
    if (link != Link::On) {
        if (ports->currentText().isEmpty()) {
            statusBar()->showMessage(tr("No hay puerto: enchufa el adaptador USB-serie de J5"), 8000);
            return;
        }
        connectTo(ports->currentText());
    }
    mode = m;
    busy = true;
    results->clear();
    resultItems.clear();
    if (ext) {
        QString n = QFileInfo(prog).fileName();
        resultLine(n, Ic::File, tr("%1: se carga desde EXT:").arg(n));
    } else {
        for (const QString &f : list) {
            QString n = QFileInfo(f).fileName();
            resultLine(n, Ic::File, tr("%1: espera").arg(n));
            resultItems.value(n)->setData(0, Qt::UserRole, true);     /* aún sin mandar */
        }
    }
    bar->setValue(0);
    if (!prog.isEmpty())
        addRecent(prog);
    updateButtons();
    pending = list;
    allowOverwrite = allowStop = false;
    launch();
}

/* Al hilo, con lo que se ha permitido hasta ahora. */
void Window::launch()
{
    worker->arm();
    if (mode == Mode::Run && fromExtDrive())
        emit wantRunExt(extFolder(), pending.first(), allowStop);
    else if (mode == Mode::Run)
        emit wantRun(pending, allowOverwrite, allowStop);
    else
        emit wantSend(pending, allowOverwrite);
}

/* No se ha tocado nada: hace falta permiso para sustituir lo que no puso
 * zxv-link o para parar el programa en marcha. Con él, otra vez; sin él,
 * nada. */
void Window::askUser(int what, const QString &text)
{
    QString question = what == Worker::AskOverwrite
                           ? tr("%1.\n\nSi lo sustituyes, lo que había en la tarjeta se pierde. ¿Sustituirlo?").arg(text)
                           : tr("Hay un programa en marcha en la placa. Para ejecutar el nuevo, la máquina se reinicia "
                                "entera, como con Ctrl+Alt+Supr: lo que haya en su memoria sin guardar se pierde.\n\n"
                                "¿Reiniciarla?");
    auto yes = QMessageBox::question(this, what == Worker::AskOverwrite ? tr("Sustituir en la tarjeta")
                                                                         : tr("Parar el programa en marcha"),
                                     question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (yes == QMessageBox::Yes) {
        if (what == Worker::AskOverwrite)
            allowOverwrite = true;
        else
            allowStop = true;
        launch();
        return;
    }
    busy = false;
    for (auto it = resultItems.begin(); it != resultItems.end(); ++it)
        if (it.value()->data(0, Qt::UserRole).toBool())
            resultLine(it.key(), Ic::Same, tr("%1: no se ha mandado").arg(it.key()));
    resultLine(QStringLiteral("\x01"), Ic::Same, tr("No se ha tocado nada"));
    updateButtons();
}

/* Una línea del paso 5 (la de ese fichero, si ya está). */
void Window::resultLine(const QString &key, Ic ic, const QString &text)
{
    QTreeWidgetItem *it = resultItems.value(key);
    if (!it) {
        it = new QTreeWidgetItem(results);
        resultItems.insert(key, it);
    }
    it->setIcon(0, icon(ic));
    it->setText(0, text);
    it->setData(0, Qt::UserRole, false);
    results->scrollToItem(it);
}

void Window::addRecent(const QString &file)
{
    QString abs = QFileInfo(file).absoluteFilePath();
    QStringList list = settings.value(QStringLiteral("recent")).toStringList();
    list.removeAll(abs);
    list.prepend(abs);
    while (list.size() > RECENT_MAX)
        list.removeLast();
    settings.setValue(QStringLiteral("recent"), list);
    refreshRecents();
}

void Window::refreshRecents()
{
    recents->clear();
    for (const QString &f : settings.value(QStringLiteral("recent")).toStringList()) {
        auto *it = new QListWidgetItem(icon(Ic::Program), QFileInfo(f).fileName() + QStringLiteral("   ") +
                                                              QDir::toNativeSeparators(QFileInfo(f).path()));
        it->setData(Qt::UserRole, f);
        it->setToolTip(QDir::toNativeSeparators(f));
        recents->addItem(it);
    }
}

void Window::updateButtons()
{
    bool prog = !programFile().isEmpty();
    runButton->setEnabled(!busy && prog && (!fromExtDrive() || insideExt(programFile())));
    copyButton->setEnabled(!busy && (prog || !chosenFiles().isEmpty()));
    stopButton->setVisible(busy);
    if (!busy) {
        stopButton->setEnabled(true);
        stopButton->setText(tr("Parar"));
    }
    for (QWidget *w : std::initializer_list<QWidget *> { ports, refreshButton, plugButton, program, chooseButton,
                                                         addButton, files, recents })
        w->setEnabled(!busy);
    if (!ports->count())
        plugButton->setEnabled(false);
}

void Window::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls() && !busy)
        e->acceptProposedAction();
}

/* Arrastrar solo pone: el primer programa en el paso 2 y lo demás en el 3. */
void Window::dropEvent(QDropEvent *e)
{
    QString prog;
    QStringList rest;
    for (const QUrl &u : e->mimeData()->urls()) {
        if (!u.isLocalFile() || !QFileInfo(u.toLocalFile()).isFile())
            continue;
        if (prog.isEmpty() && isProgram(u.toLocalFile()))
            prog = u.toLocalFile();
        else
            rest << u.toLocalFile();
    }
    if (!prog.isEmpty())
        setProgram(prog);
    if (!rest.isEmpty())
        addExtras(rest);
    e->acceptProposedAction();
}

void Window::closeEvent(QCloseEvent *e)
{
    emit wantClose();
    e->accept();
}
