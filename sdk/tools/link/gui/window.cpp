/*
 * window.cpp - La ventana de zxv-link-gui (ver window.h).
 */
#include "window.h"

#include <QAction>
#include <algorithm>
#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "linkcore.h"
#include "serial.h"
#include "worker.h"
#include "zxv_version.h"

#define RECENT_MAX      10

enum { COL_NAME, COL_SIZE };
enum { ROLE_NAME = Qt::UserRole, ROLE_DIR, ROLE_UP };

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
    setWindowTitle(tr("ZX-V: carga remota"));
    setAcceptDrops(true);
    for (QString &p : paths)
        p = QStringLiteral("/");

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
    auto *what = hint(tr("Programas y ficheros entre el PC y la máquina, por J5 o Wi-Fi"));
    what->setWordWrap(false);
    headText->addWidget(what);
    head->addLayout(headText);
    head->addStretch(1);
    auto *version = hint(QStringLiteral("zxv-link-gui " ZXV_VERSION));
    version->setWordWrap(false);
    head->addWidget(version, 0, Qt::AlignBottom);

    /* La máquina. */
    ports = new QComboBox;
    ports->setMinimumContentsLength(28);
    ports->setToolTip(tr("El adaptador USB-serie en la tira J5, o una ZX-V de la red: al elegirla, se conecta"));
    refreshButton = new QToolButton;
    refreshButton->setIcon(icon(Ic::Refresh));
    refreshButton->setToolTip(tr("Vuelve a buscar los puertos serie y las ZX-V de la red"));
    dotLabel = new QLabel;
    state = new QLabel;
    state->setTextInteractionFlags(Qt::TextSelectableByMouse);
    state->setWordWrap(true);
    pairButton = new QPushButton(icon(Ic::Pair), tr("Emparejar..."));
    pairButton->setToolTip(tr("Empareja este PC con la ZX-V de la red elegida (con EDIT > Wi-Fi > Emparejar un PC "
                              "abierto en ella): hasta entonces, no acepta nada de él"));
    plugButton = new QToolButton;
    plugButton->setIcon(icon(Ic::Plug));
    plugButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto *machineRow = new QHBoxLayout;
    auto *machineLabel = new QLabel(tr("Máquina"));
    QFont bold = machineLabel->font();
    bold.setBold(true);
    machineLabel->setFont(bold);
    machineRow->addWidget(machineLabel);
    machineRow->addWidget(ports, 1);
    machineRow->addWidget(refreshButton);
    machineRow->addWidget(pairButton);
    machineRow->addWidget(plugButton);
    auto *stateRow = new QHBoxLayout;
    stateRow->addSpacing(machineLabel->sizeHint().width() + 6);
    stateRow->addWidget(dotLabel);
    stateRow->addWidget(state, 1);

    /* Ejecutar un fichero del PC, directo a la memoria. */
    pcProgram = new QComboBox;
    pcProgram->setEditable(true);
    pcProgram->setInsertPolicy(QComboBox::NoInsert);
    pcProgram->lineEdit()->setPlaceholderText(tr("Un programa del PC (.zxv, .bas, .zvb, .tap): elígelo, arrástralo o "
                                                 "toma uno reciente"));
    pcProgram->lineEdit()->setClearButtonEnabled(true);
    pcChoose = new QPushButton(icon(Ic::Folder), tr("Elegir..."));
    pcRun = new QPushButton(icon(Ic::Play), tr("Ejecutar"));
    pcRun->setFont(bold);
    pcRun->setIconSize(QSize(22, 22));
    pcRun->setMinimumHeight(34);
    pcRun->setDefault(true);
    pcRun->setToolTip(tr("La máquina lo carga de este PC y lo ejecuta: no se copia a la tarjeta"));
    auto *pcRow = new QHBoxLayout;
    pcRow->addWidget(pcProgram, 1);
    pcRow->addWidget(pcChoose);
    pcRow->addWidget(pcRun);
    auto *pcBox = new QGroupBox(tr("Ejecutar un fichero del PC, directo a la memoria"));
    auto *pcLayout = new QVBoxLayout(pcBox);
    pcLayout->addLayout(pcRow);
    pcLayout->addWidget(hint(tr("No se copia a la tarjeta. Si el programa carga otros ficheros, tienen que estar en "
                                "la máquina.")));

    /* La máquina como un explorador. */
    const Ic driveIcons[DRIVES] = { Ic::Card, Ic::Rom, Ic::Ext };
    const QString driveNames[DRIVES] = { tr("Tarjeta"), tr("ROM (solo leer)"), tr("EXT: (carpeta del PC)") };
    const QString driveTips[DRIVES] = {
        tr("La tarjeta SD de la máquina"),
        tr("El disco ROM de la máquina: se puede ejecutar y traer, no cambiar"),
        tr("La carpeta de este PC que la máquina ve como EXT: mientras está conectada: puede leer y escribir en ella")
    };
    auto *driveGroup = new QButtonGroup(this);
    auto *driveRow = new QHBoxLayout;
    for (int d = 0; d < DRIVES; d++) {
        driveButtons[d] = new QToolButton;
        driveButtons[d]->setIcon(icon(driveIcons[d]));
        driveButtons[d]->setText(driveNames[d]);
        driveButtons[d]->setToolTip(driveTips[d]);
        driveButtons[d]->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        driveButtons[d]->setCheckable(true);
        driveButtons[d]->setAutoRaise(true);
        driveGroup->addButton(driveButtons[d], d);
        driveRow->addWidget(driveButtons[d]);
    }
    driveRow->addStretch(1);
    pathLabel = new QLabel;
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    extChoose = new QPushButton(icon(Ic::Folder), tr("Carpeta..."));
    extChoose->setToolTip(tr("Elige la carpeta del PC que es la unidad EXT: (una a propósito: la máquina puede "
                             "escribir en ella)"));
    driveRow->addWidget(extChoose);

    auto *tools = new QToolBar;
    tools->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tools->setIconSize(QSize(18, 18));
    actRun = tools->addAction(icon(Ic::Play), tr("Ejecutar"));
    actRun->setToolTip(tr("Ejecuta el programa elegido en la máquina (entrando antes en su carpeta)"));
    tools->addSeparator();
    actPut = tools->addAction(icon(Ic::Upload), tr("Mandar..."));
    actPut->setToolTip(tr("Manda ficheros del PC a esta carpeta (también se pueden arrastrar a la lista)"));
    actGet = tools->addAction(icon(Ic::Download), tr("Traer..."));
    actGet->setToolTip(tr("Copia lo elegido a una carpeta del PC"));
    tools->addSeparator();
    actMkdir = tools->addAction(icon(Ic::NewFolder), tr("Carpeta nueva..."));
    actRename = tools->addAction(icon(Ic::Rename), tr("Renombrar..."));
    actRename->setShortcut(Qt::Key_F2);
    actDelete = tools->addAction(icon(Ic::Delete), tr("Borrar"));
    actDelete->setShortcut(QKeySequence::Delete);
    actUp = new QAction(icon(Ic::Up), tr("Arriba"), this);
    actUp->setToolTip(tr("La carpeta de arriba"));
    actUp->setShortcut(Qt::Key_Backspace);
    actRefresh = new QAction(icon(Ic::Refresh), tr("Volver a leer"), this);
    actRefresh->setToolTip(tr("Vuelve a leer la carpeta"));
    actRefresh->setShortcut(QKeySequence::Refresh);
    auto *upButton = new QToolButton;
    upButton->setDefaultAction(actUp);
    upButton->setAutoRaise(true);
    auto *rereadButton = new QToolButton;
    rereadButton->setDefaultAction(actRefresh);
    rereadButton->setAutoRaise(true);
    auto *pathRow = new QHBoxLayout;
    pathRow->addWidget(upButton);
    pathRow->addWidget(pathLabel, 1);
    pathRow->addWidget(rereadButton);
    for (QAction *a : { actRename, actDelete, actUp, actRefresh })
        a->setShortcutContext(Qt::WidgetWithChildrenShortcut);

    list = new QTreeWidget;
    list->setColumnCount(2);
    list->setHeaderLabels({ tr("Nombre"), tr("Tamaño") });
    list->setRootIsDecorated(false);
    list->setUniformRowHeights(true);
    list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list->setMinimumHeight(220);
    list->header()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    list->header()->setSectionResizeMode(COL_SIZE, QHeaderView::ResizeToContents);
    list->header()->setStretchLastSection(false);
    list->setContextMenuPolicy(Qt::ActionsContextMenu);
    list->addActions({ actRun, actGet, actRename, actDelete });
    list->addActions({ actUp, actRefresh });
    list->installEventFilter(this);
    list->viewport()->installEventFilter(this);
    auto *explorer = new QGroupBox(tr("En la máquina"));
    auto *explorerLayout = new QVBoxLayout(explorer);
    explorerLayout->addLayout(driveRow);
    explorerLayout->addWidget(tools);
    explorerLayout->addLayout(pathRow);
    explorerLayout->addWidget(list, 1);
    explorerLayout->addWidget(hint(tr("Arrastra ficheros del PC a la lista para mandarlos a esta carpeta. Borrar y "
                                      "sustituir preguntan antes.")));

    /* Abajo: cómo va, y el informe de la máquina (plegado). */
    statusIcon = new QLabel;
    status = new QLabel;
    status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    status->setWordWrap(true);
    stopButton = new QPushButton(icon(Ic::Error), tr("Parar"));
    stopButton->setToolTip(tr("Deja lo que se está haciendo (lo que ya se ha mandado se queda)"));
    stopButton->hide();
    bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setTextVisible(false);
    bar->setMaximumHeight(6);
    bar->hide();
    auto *statusRow = new QHBoxLayout;
    statusRow->addWidget(statusIcon);
    statusRow->addWidget(status, 1);
    statusRow->addWidget(stopButton);

    logToggle = new QToolButton;
    logToggle->setText(tr("Informe de la máquina"));
    logToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    logToggle->setArrowType(Qt::RightArrow);
    logToggle->setCheckable(true);
    logToggle->setAutoRaise(true);
    logToggle->setToolTip(tr("Lo que cuenta la máquina cada 5 segundos: el enlace, el sonido, la tarjeta, el USB, el "
                             "reloj y la Wi-Fi"));
    logView = new QPlainTextEdit;
    logView->setReadOnly(true);
    logView->setMaximumBlockCount(5000);
    logView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    logView->setMinimumHeight(140);
    auto *clearButton = new QPushButton(icon(Ic::Clear), tr("Limpiar"));
    logBox = new QWidget;
    auto *logLayout = new QVBoxLayout(logBox);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->addWidget(logView, 1);
    auto *logButtons = new QHBoxLayout;
    logButtons->addStretch(1);
    logButtons->addWidget(clearButton);
    logLayout->addLayout(logButtons);
    logBox->hide();

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->addLayout(head);
    layout->addWidget(line());
    layout->addLayout(machineRow);
    layout->addLayout(stateRow);
    layout->addWidget(pcBox);
    layout->addWidget(explorer, 3);
    layout->addLayout(statusRow);
    layout->addWidget(bar);
    layout->addWidget(logToggle, 0, Qt::AlignLeft);
    layout->addWidget(logBox, 2);
    setCentralWidget(central);
    resize(900, 820);

    /* El menú: lo que se usa poco. */
    QMenu *machineMenu = menuBar()->addMenu(tr("&Máquina"));
    menuSearch = machineMenu->addAction(icon(Ic::Refresh), tr("Buscar otra vez"));
    menuPair = machineMenu->addAction(icon(Ic::Pair), tr("Emparejar..."));
    machineMenu->addSeparator();
    menuUpdate = machineMenu->addAction(icon(Ic::Update), tr("Actualizar firmware..."));
    machineMenu->addSeparator();
    machineMenu->addAction(tr("Salir"), this, &QWidget::close);

    /* El hilo que habla con la máquina. */
    worker = new Worker;
    worker->moveToThread(&thread);
    connect(&thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Window::wantOpen, worker, &Worker::open);
    connect(this, &Window::wantClose, worker, &Worker::close);
    connect(this, &Window::wantServe, worker, &Worker::serve);
    connect(this, &Window::wantUpdate, worker, &Worker::update);
    connect(this, &Window::wantNetPorts, worker, &Worker::listNet);
    connect(this, &Window::wantPair, worker, &Worker::pair);
    connect(this, &Window::wantList, worker, &Worker::list);
    connect(this, &Window::wantPut, worker, &Worker::put);
    connect(this, &Window::wantGet, worker, &Worker::get);
    connect(this, &Window::wantRemove, worker, &Worker::remove);
    connect(this, &Window::wantRename, worker, &Worker::rename);
    connect(this, &Window::wantMkdir, worker, &Worker::mkdir);
    connect(this, &Window::wantRunPath, worker, &Worker::runPath);
    connect(this, &Window::wantRunPc, worker, &Worker::runPc);
    connect(worker, &Worker::netPorts, this, [this](const QString &lines, const QString &other) {
        /* Las de otra versión del protocolo de la red, a la vista pero sin
         * poder elegirlas, con el porqué (qué actualizar) al pasar por encima;
         * si se actualizan, la próxima vez ya se pueden. */
        auto *model = qobject_cast<QStandardItemModel *>(ports->model());
        auto put = [this, model](const QString &port, const QString &why) {
            int open = port.indexOf(QLatin1Char('('));
            QString label = open > 0 ? tr("%1 · Wi-Fi %2").arg(port.left(open).trimmed(), port.mid(open))
                                     : tr("%1 · Wi-Fi").arg(port);
            if (!why.isEmpty())
                label = tr("%1 · no compatible").arg(label);
            int i = ports->findData(port, Qt::UserRole, Qt::MatchFixedString);
            if (i < 0) {
                ports->addItem(icon(Ic::Pair), label, port);
                i = ports->count() - 1;
            }
            ports->setItemText(i, label);
            ports->setItemData(i, why.isEmpty() ? QVariant() : QVariant(why), Qt::ToolTipRole);
            if (model && model->item(i))
                model->item(i)->setEnabled(why.isEmpty());
        };
        for (const QString &p : lines.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            QString port = p.trimmed();
            netPorts.insert(port.toUpper());
            put(port, QString());
        }
        for (const QString &p : other.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            QString port = p.section(QLatin1Char('\t'), 0, 0).trimmed();
            netPorts.remove(port.toUpper());
            put(port, p.section(QLatin1Char('\t'), 1));
        }
        QString keep = settings.value(QStringLiteral("port")).toString();
        if (link == Link::Off && !keep.isEmpty() && currentPort() != keep) {
            int i = ports->findData(keep, Qt::UserRole, Qt::MatchFixedString);
            if (i >= 0 && (!model || !model->item(i) || model->item(i)->isEnabled()))
                ports->setCurrentIndex(i);
        }
        updateButtons();
    });
    connect(worker, &Worker::needPair, this, [this](const QString &m) {
        setLink(Link::Unpaired, tr("%1: sin emparejar con este PC").arg(m));
        if (pairWanted)
            startPair();
        else
            askPair(m);
    });
    connect(worker, &Worker::pairCode, this, &Window::showPairCode);
    connect(worker, &Worker::pairDone, this, [this](bool ok, const QString &text) {
        if (pairBox) {
            pairBox->hide();
            pairBox->deleteLater();
            pairBox = nullptr;
        }
        worker->arm();
        pairWanted = false;
        if (ok) {
            say(Ic::Done, tr("Emparejada: este PC ya puede usarla"));
        } else {
            say(Ic::Error, text, true);
            if (link == Link::Unpaired)
                emit wantClose();
        }
    });
    connect(worker, &Worker::served, this, [this](bool ok, const QString &text) {
        if (!ok && !text.isEmpty())
            say(Ic::Warning, tr("La carpeta de EXT: no se puede servir: %1").arg(text), true);
    });
    connect(worker, &Worker::extEvent, this, [this](const QString &what, const QString &zpath) {
        if (what != QLatin1String("read") && drive == EXT)
            refreshList();              /* la máquina ha cambiado algo en EXT: */
        Q_UNUSED(zpath);
    });
    connect(worker, &Worker::opened, this, [this](bool ok, const QString &text) {
        if (!ok) {
            setLink(Link::Error, text);
            return;
        }
        machine = text;
        setLink(Link::On, tr("Conectada: %1").arg(text));
        settings.setValue(QStringLiteral("port"), currentPort());
        serveExt();
        refreshList();
        if (pairWanted)
            startPair();
    });
    connect(worker, &Worker::closed, this, [this](const QString &why) {
        if (why.isEmpty())
            setLink(Link::Off, tr("Sin conectar"));
        else
            setLink(Link::Error, why);
        refreshList();
    });
    connect(worker, &Worker::ask, this, &Window::askUser);
    connect(worker, &Worker::log, this, [this](const QString &text) {
        if (busy && op == Op::Update)
            updateLog(text);
        logView->moveCursor(QTextCursor::End);
        logView->insertPlainText(text);
        logView->moveCursor(QTextCursor::End);
    });
    connect(worker, &Worker::progress, this, [this](const QString &name, qint64 done, qint64 total, bool skipped) {
        if (skipped) {
            say(Ic::Same, tr("%1: ya estaba igual, no se manda").arg(name));
            return;
        }
        QString verb = op == Op::Get ? tr("trayendo") : tr("mandando");
        say(Ic::Sending, tr("%1: %2, %3 de %4 bytes").arg(name, verb, bytes(done), bytes(total)));
        bar->setValue(total ? int(done * 100 / total) : 100);
    });
    connect(worker, &Worker::listed, this, [this](const QString &path, const QVariantList &e) {
        if (path != machinePath() || drive == EXT)
            return;                     /* de otra carpeta: ya no se ve */
        listing = false;
        showEntries(e);
    });
    connect(worker, &Worker::listFailed, this, [this](const QString &path, const QString &why) {
        if (path != machinePath() || drive == EXT)
            return;
        listing = false;
        entries.clear();
        placeholder(why);
    });
    connect(worker, &Worker::finished, this, [this](bool ok, const QString &text) {
        Op was = op;
        busy = false;
        op = Op::None;
        worker->arm();                  /* tras Parar: el informe sigue llegando */
        bar->hide();
        if (!ok)
            say(Ic::Error, text, true);
        else if (was == Op::RunPc)
            say(Ic::Play, tr("%1: en marcha en la máquina (cargado desde este PC)").arg(text));
        else if (was == Op::RunPath)
            say(Ic::Play, tr("En marcha: %1").arg(text));
        else
            say(Ic::Done, text);
        if (was == Op::Put || was == Op::Remove || was == Op::Rename || was == Op::Mkdir)
            refreshList();
        updateButtons();
    });
    thread.start();

    connect(ports, &QComboBox::activated, this, [this] { connectTo(currentPort()); });
    connect(refreshButton, &QToolButton::clicked, this, &Window::refreshPorts);
    connect(menuSearch, &QAction::triggered, this, &Window::refreshPorts);
    connect(plugButton, &QToolButton::clicked, this, &Window::toggleConnection);
    connect(pairButton, &QPushButton::clicked, this, &Window::pairClicked);
    connect(menuPair, &QAction::triggered, this, &Window::pairClicked);
    connect(menuUpdate, &QAction::triggered, this, &Window::updateFirmware);
    connect(pcChoose, &QPushButton::clicked, this, &Window::chooseProgram);
    connect(pcRun, &QPushButton::clicked, this, &Window::runPc);
    connect(pcProgram, &QComboBox::currentTextChanged, this, [this] { updateButtons(); });
    connect(driveGroup, &QButtonGroup::idClicked, this, &Window::setDrive);
    connect(extChoose, &QPushButton::clicked, this, &Window::chooseExtFolder);
    connect(actRun, &QAction::triggered, this, &Window::runSelected);
    connect(actPut, &QAction::triggered, this, &Window::choosePut);
    connect(actGet, &QAction::triggered, this, &Window::getSelected);
    connect(actMkdir, &QAction::triggered, this, &Window::newFolder);
    connect(actRename, &QAction::triggered, this, &Window::renameSelected);
    connect(actDelete, &QAction::triggered, this, &Window::deleteSelected);
    connect(actUp, &QAction::triggered, this, &Window::goUp);
    connect(actRefresh, &QAction::triggered, this, &Window::refreshList);
    connect(list, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *it) { openItem(it); });
    connect(list, &QTreeWidget::itemSelectionChanged, this, &Window::updateButtons);
    connect(stopButton, &QPushButton::clicked, this, [this] {
        stopButton->setEnabled(false);
        stopButton->setText(tr("Parando..."));
        worker->cancel();
    });
    connect(clearButton, &QPushButton::clicked, logView, &QPlainTextEdit::clear);
    connect(logToggle, &QToolButton::toggled, this, [this](bool on) {
        logToggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
        logBox->setVisible(on);
        settings.setValue(QStringLiteral("log"), on);
    });

    refreshRecents();
    refreshPorts();
    setLink(Link::Off, tr("Sin conectar"));
    logToggle->setChecked(settings.value(QStringLiteral("log"), false).toBool());
    int d = settings.value(QStringLiteral("drive"), int(SD)).toInt();
    setDrive(d >= 0 && d < DRIVES ? d : int(SD));
    say(Ic::File, tr("Elige la máquina arriba: al elegirla, se conecta."));
}

Window::~Window()
{
    thread.quit();
    thread.wait();
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

/* --- La máquina ------------------------------------------------------------- */

QString Window::currentPort() const
{
    return ports->currentData().toString();
}

bool Window::isNet(const QString &port) const
{
    return netPorts.contains(port.trimmed().toUpper());
}

void Window::autoConnect()
{
    QString port = settings.value(QStringLiteral("port")).toString();
    int i = port.isEmpty() ? -1 : ports->findData(port, Qt::UserRole, Qt::MatchFixedString);
    if (i >= 0) {
        ports->setCurrentIndex(i);
        connectTo(port);
    }
}

void Window::refreshPorts()
{
    char buf[4096];
    serial_list(buf, sizeof buf);
    emit wantNetPorts();                /* las ZX-V de la red, en el hilo: tardan un segundo y medio */
    QString keep = currentPort().isEmpty() ? settings.value(QStringLiteral("port")).toString() : currentPort();
    ports->clear();
    netPorts.clear();
    for (const QString &p : QString::fromLocal8Bit(buf).split(QLatin1Char('\n'), Qt::SkipEmptyParts))
        ports->addItem(icon(Ic::Plug), tr("%1 · cable (J5)").arg(p.trimmed()), p.trimmed());
    int i = ports->findData(keep, Qt::UserRole, Qt::MatchFixedString);
    ports->setCurrentIndex(i >= 0 ? i : ports->count() ? 0 : -1);
    if (!ports->count() && link == Link::Off)
        say(Ic::Warning, tr("Buscando... Si no aparece ninguna: enchufa el adaptador USB-serie de J5, o enciende la "
                            "Wi-Fi de la máquina, y pulsa buscar."));
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
    if (link == Link::On || link == Link::Connecting || link == Link::Unpaired)
        emit wantClose();
    else
        connectTo(currentPort());
}

void Window::setLink(Link l, const QString &text)
{
    link = l;
    dotLabel->setPixmap(dot(l == Link::On ? Dot::On : l == Link::Connecting || l == Link::Unpaired ? Dot::Busy
                            : l == Link::Error                                                     ? Dot::Error
                                                                                                   : Dot::Off));
    state->setText(text);
    bool on = l != Link::Off && l != Link::Error;
    plugButton->setText(on ? tr("Desconectar") : tr("Conectar"));
    plugButton->setToolTip(on ? tr("Suelta la máquina (el puerto, para usarlo con otro programa)")
                              : tr("Conecta con la máquina elegida"));
    if (l != Link::On) {
        entries.clear();
        listing = false;
    }
    updateButtons();
}

/* Emparejar: si la máquina no está abierta, antes se abre (y luego, al
 * abrirse, se empieza). */
void Window::pairClicked()
{
    if (busy || !isNet(currentPort()))
        return;
    pairWanted = true;
    if (link == Link::On || link == Link::Unpaired)
        startPair();
    else
        connectTo(currentPort());
}

void Window::startPair()
{
    pairWanted = false;
    auto yes = QMessageBox::information(this, tr("Emparejar"),
                                        tr("En la máquina, abre EDIT > Wi-Fi > Emparejar un PC y, cuando espere a un "
                                           "PC, pulsa aquí Aceptar. Los dos enseñarán el mismo código de seis cifras."),
                                        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
    if (yes != QMessageBox::Ok) {
        if (link == Link::Unpaired)
            emit wantClose();
        return;
    }
    worker->arm();
    QString name = QSysInfo::machineHostName();
    emit wantPair(name.isEmpty() ? QStringLiteral("PC") : name);
}

/* Una ZX-V de la red sin emparejar, abierta sin pulsar Emparejar. */
void Window::askPair(const QString &m)
{
    auto yes = QMessageBox::question(this, tr("Emparejar"),
                                     tr("%1 no está emparejada con este PC: hasta que lo esté, no acepta nada de él."
                                        "\n\n¿Emparejarla ahora?")
                                         .arg(m),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (yes == QMessageBox::Yes)
        startPair();
    else
        emit wantClose();
}

/* El código: el usuario mira que la máquina enseña el mismo y acepta allí. */
void Window::showPairCode(quint32 code, const QString &m)
{
    pairBox = new QMessageBox(QMessageBox::Information, tr("Emparejar"),
                              tr("Código: %1 %2\n\nComprueba que %3 enseña el mismo código y acepta allí (Enter). "
                                 "Si no coincide, recházalo en la máquina: alguien podría estar en medio.")
                                  .arg(code / 1000, 3, 10, QLatin1Char('0'))
                                  .arg(code % 1000, 3, 10, QLatin1Char('0'))
                                  .arg(m),
                              QMessageBox::Cancel, this);
    QFont f = pairBox->font();
    f.setPointSizeF(f.pointSizeF() * 1.2);
    pairBox->setFont(f);
    connect(pairBox, &QMessageBox::rejected, this, [this] { worker->cancel(); });
    pairBox->show();
}

/* Actualizar el firmware (D105): el fichero, las versiones, la pregunta y,
 * mientras tanto, los pasos que cuenta la máquina. */
void Window::updateFirmware()
{
    if (busy || link != Link::On)
        return;
    if (isNet(currentPort())) {
        QMessageBox::information(this, tr("Actualizar firmware"),
                                 tr("Por la red no se puede: al reiniciarse para actualizar, la máquina solo escucha "
                                    "a J5. Hazlo con el adaptador USB-serie en J5, o desde la tarjeta (EDIT > "
                                    "Actualizar firmware)."));
        return;
    }
    QString dir = settings.value(QStringLiteral("updateDir"),
                                 QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
    QString f = QFileDialog::getOpenFileName(this, tr("El fichero de actualización"), dir,
                                             tr("Actualización de ZX-V (zxv-update.bin *.bin)"));
    if (f.isEmpty())
        return;
    settings.setValue(QStringLiteral("updateDir"), QFileInfo(f).path());
    lc_link_t probe {};
    char version[17];
    QByteArray path = QFileInfo(f).absoluteFilePath().toLocal8Bit();
    lc_spanish = spanish;
    if (lc_update_version(&probe, path.constData(), version) < 0) {
        QMessageBox::warning(this, tr("Actualizar firmware"), QString::fromUtf8(probe.error));
        return;
    }
    QString question = tr("La máquina: %1\nEl fichero: ZX-V %2\n\n¿Actualizar el firmware de las dos Pico? La "
                          "máquina se reinicia (lo que haya en su memoria sin guardar se pierde) y tarda dos o tres "
                          "minutos. No la apagues ni desenchufes el adaptador mientras tanto.")
                           .arg(machine, QString::fromUtf8(version));
    if (QMessageBox::question(this, tr("Actualizar firmware"), question, QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
        return;
    updateLine.clear();
    begin(Op::Update, tr("Actualizando con %1 (ZX-V %2)...").arg(QFileInfo(f).fileName(), QString::fromUtf8(version)));
    emit wantUpdate(f);
}

/* Lo que cuenta la máquina mientras actualiza: el paso, a la línea de
 * estado; el porcentaje, a la barra. */
void Window::updateLog(const QString &text)
{
    updateLine += text;
    int nl;
    while ((nl = updateLine.indexOf(QLatin1Char('\n'))) >= 0) {
        QString l = updateLine.left(nl).trimmed();
        updateLine.remove(0, nl + 1);
        int colon = l.indexOf(QStringLiteral(": "));
        QString head = colon > 0 ? l.left(colon) : QString();
        if (head != QStringLiteral("Actualizar") && head != QStringLiteral("Update"))
            continue;
        QString rest = l.mid(colon + 2);
        if (rest.endsWith(QStringLiteral(" %"))) {
            bar->setValue(rest.chopped(2).toInt());
            continue;
        }
        say(rest.endsWith(QStringLiteral(": error")) ? Ic::Error : Ic::Sending, rest);
    }
}

/* --- Ejecutar del PC ------------------------------------------------------- */

void Window::setProgram(const QString &file)
{
    pcProgram->setEditText(QDir::toNativeSeparators(QFileInfo(file).absoluteFilePath()));
    settings.setValue(QStringLiteral("dir"), QFileInfo(file).absolutePath());
    updateButtons();
}

QString Window::pcFile() const
{
    QFileInfo fi(pcProgram->currentText().trimmed());
    return fi.isFile() ? fi.absoluteFilePath() : QString();
}

void Window::chooseProgram()
{
    QString dir = settings.value(QStringLiteral("dir")).toString();
    QString f = QFileDialog::getOpenFileName(this, tr("El programa"), dir,
                                             tr("Programas de ZX-V (*.zxv *.bas *.zvb *.tap *.tzx *.txt)"));
    if (!f.isEmpty())
        setProgram(f);
}

void Window::runPc()
{
    QString f = pcFile();
    if (busy || f.isEmpty())
        return;
    if (link != Link::On) {
        say(Ic::Warning, tr("Antes, conecta con la máquina (arriba)."), true);
        return;
    }
    addRecent(f);
    runTarget = f;
    begin(Op::RunPc, tr("%1: cargándolo desde este PC...").arg(QFileInfo(f).fileName()));
    emit wantRunPc(f, false);
}

void Window::addRecent(const QString &file)
{
    QString abs = QFileInfo(file).absoluteFilePath();
    QStringList l = settings.value(QStringLiteral("recent")).toStringList();
    l.removeAll(abs);
    l.prepend(abs);
    while (l.size() > RECENT_MAX)
        l.removeLast();
    settings.setValue(QStringLiteral("recent"), l);
    refreshRecents();
}

void Window::refreshRecents()
{
    QString keep = pcProgram->currentText();
    pcProgram->clear();
    for (const QString &f : settings.value(QStringLiteral("recent")).toStringList())
        pcProgram->addItem(icon(Ic::Program), QDir::toNativeSeparators(f));
    pcProgram->setEditText(keep);
}

/* --- El explorador ----------------------------------------------------------- */

QString Window::drivePrefix(int d) const
{
    return d == SD ? QStringLiteral("SD:") : d == ROM ? QStringLiteral("ROM:") : QStringLiteral("EXT:");
}

/* La ruta en la máquina de la carpeta que se ve (o de name, dentro). */
QString Window::machinePath(const QString &name) const
{
    QString p = paths[drive];
    if (!name.isEmpty())
        p = (p.endsWith(QLatin1Char('/')) ? p : p + QLatin1Char('/')) + name;
    return drivePrefix(drive) + p;
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

/* En el PC, la carpeta de EXT: que se ve (o name, dentro). */
QString Window::extLocal(const QString &name) const
{
    QString p = extFolder() + paths[EXT];
    return name.isEmpty() ? QDir::cleanPath(p) : QDir::cleanPath(p + QLatin1Char('/') + name);
}

void Window::serveExt()
{
    if (link == Link::On)
        emit wantServe(extFolder(), false);
}

void Window::chooseExtFolder()
{
    QString f = QFileDialog::getExistingDirectory(this, tr("La carpeta de la unidad EXT:"), extFolder());
    if (f.isEmpty())
        return;
    settings.setValue(QStringLiteral("extFolder"), QFileInfo(f).absoluteFilePath());
    paths[EXT] = QStringLiteral("/");
    serveExt();
    refreshList();
}

void Window::setDrive(int d)
{
    drive = d;
    driveButtons[d]->setChecked(true);
    settings.setValue(QStringLiteral("drive"), d);
    extChoose->setVisible(d == EXT);
    refreshList();
}

void Window::placeholder(const QString &text)
{
    list->clear();
    auto *it = new QTreeWidgetItem(list);
    it->setText(COL_NAME, text);
    it->setFlags(Qt::NoItemFlags);
    it->setFirstColumnSpanned(true);
    updateButtons();
}

void Window::refreshList()
{
    pathLabel->setText(drive == EXT ? tr("%1  (en el PC: %2)").arg(machinePath(), QDir::toNativeSeparators(extLocal()))
                                    : machinePath());
    if (drive == EXT) {
        QDir dir(extLocal());
        if (!dir.exists()) {
            if (paths[EXT] != QLatin1String("/")) {
                paths[EXT] = QStringLiteral("/");
                refreshList();
                return;
            }
            entries.clear();
            placeholder(tr("La carpeta %1 aún no existe: se crea al conectar con la máquina.")
                            .arg(QDir::toNativeSeparators(extFolder())));
            return;
        }
        QVariantList e;
        const QFileInfoList infos = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                                      QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo &fi : infos) {
            if (fi.fileName().startsWith(QLatin1Char('.')))
                continue;
            QVariantMap m;
            m.insert(QStringLiteral("name"), fi.fileName());
            m.insert(QStringLiteral("size"), fi.size());
            m.insert(QStringLiteral("dir"), fi.isDir());
            e << m;
        }
        showEntries(e);
        return;
    }
    if (link != Link::On) {
        entries.clear();
        placeholder(drive == SD ? tr("Conecta con la máquina para ver su tarjeta.")
                                : tr("Conecta con la máquina para ver su disco ROM."));
        return;
    }
    listing = true;
    placeholder(tr("Leyendo..."));
    emit wantList(machinePath());
}

void Window::showEntries(const QVariantList &e)
{
    entries = e;
    std::sort(entries.begin(), entries.end(), [](const QVariant &a, const QVariant &b) {
        QVariantMap x = a.toMap(), y = b.toMap();
        if (x.value("dir").toBool() != y.value("dir").toBool())
            return x.value("dir").toBool();
        return x.value("name").toString().compare(y.value("name").toString(), Qt::CaseInsensitive) < 0;
    });
    list->clear();
    if (paths[drive] != QLatin1String("/")) {
        auto *up = new QTreeWidgetItem(list);
        up->setIcon(COL_NAME, icon(Ic::Up));
        up->setText(COL_NAME, QStringLiteral(".."));
        up->setData(COL_NAME, ROLE_UP, true);
        up->setToolTip(COL_NAME, tr("La carpeta de arriba"));
    }
    for (const QVariant &v : entries) {
        QVariantMap m = v.toMap();
        QString name = m.value("name").toString();
        bool dir = m.value("dir").toBool();
        auto *it = new QTreeWidgetItem(list);
        it->setIcon(COL_NAME, icon(dir ? Ic::Folder : isProgram(name) ? Ic::Program : Ic::File));
        it->setText(COL_NAME, name);
        it->setData(COL_NAME, ROLE_NAME, name);
        it->setData(COL_NAME, ROLE_DIR, dir);
        if (!dir) {
            it->setText(COL_SIZE, bytes(m.value("size").toLongLong()));
            it->setTextAlignment(COL_SIZE, Qt::AlignRight | Qt::AlignVCenter);
        }
    }
    if (entries.isEmpty() && paths[drive] == QLatin1String("/")) {
        auto *it = new QTreeWidgetItem(list);
        it->setText(COL_NAME, drive == EXT ? tr("Vacía: arrastra aquí ficheros del PC.") : tr("Vacía."));
        it->setFlags(Qt::NoItemFlags);
        it->setFirstColumnSpanned(true);
    }
    updateButtons();
}

/* Los nombres elegidos (sin ".."); dirs: si alguno es una carpeta. */
QStringList Window::selectedNames(bool *dirs) const
{
    QStringList out;
    if (dirs)
        *dirs = false;
    for (QTreeWidgetItem *it : list->selectedItems()) {
        QString n = it->data(COL_NAME, ROLE_NAME).toString();
        if (n.isEmpty())
            continue;
        out << n;
        if (dirs && it->data(COL_NAME, ROLE_DIR).toBool())
            *dirs = true;
    }
    return out;
}

bool Window::existsHere(const QString &name) const
{
    if (drive == EXT)
        return QFileInfo::exists(extLocal(name));
    for (const QVariant &v : entries)
        if (v.toMap().value("name").toString().compare(name, Qt::CaseInsensitive) == 0)
            return true;            /* FAT no distingue mayúsculas */
    return false;
}

void Window::openItem(QTreeWidgetItem *it)
{
    if (!it || busy)
        return;
    if (it->data(COL_NAME, ROLE_UP).toBool()) {
        goUp();
        return;
    }
    QString n = it->data(COL_NAME, ROLE_NAME).toString();
    if (n.isEmpty())
        return;
    if (it->data(COL_NAME, ROLE_DIR).toBool()) {
        QString p = paths[drive];
        paths[drive] = (p.endsWith(QLatin1Char('/')) ? p : p + QLatin1Char('/')) + n;
        refreshList();
        return;
    }
    if (isProgram(n)) {
        list->setCurrentItem(it);
        runSelected();
    }
}

void Window::goUp()
{
    QString p = paths[drive];
    if (p == QLatin1String("/"))
        return;
    int slash = p.lastIndexOf(QLatin1Char('/'));
    paths[drive] = slash <= 0 ? QStringLiteral("/") : p.left(slash);
    refreshList();
}

void Window::runSelected()
{
    bool dirs;
    QStringList n = selectedNames(&dirs);
    if (busy || n.size() != 1 || dirs)
        return;
    if (link != Link::On) {
        say(Ic::Warning, tr("Antes, conecta con la máquina (arriba)."), true);
        return;
    }
    runTarget = machinePath(n.first());
    begin(Op::RunPath, tr("Ejecutando %1...").arg(runTarget));
    emit wantRunPath(runTarget, false);
}

bool Window::validName(const QString &name)
{
    static const QString bad = QStringLiteral("\\/:*?\"<>|");
    bool ok = !name.trimmed().isEmpty() && name.size() <= 63 && !name.endsWith(QLatin1Char('.')) &&
              !name.endsWith(QLatin1Char(' ')) && name != QLatin1String("..");
    for (QChar c : name)
        if (bad.contains(c) || c.unicode() < 0x20 || c.unicode() > 0x7E)
            ok = false;
    if (!ok)
        QMessageBox::warning(this, tr("Nombre"),
                             tr("«%1» no vale como nombre en la máquina: de 1 a 63 letras sin acentos, sin \\ / : * ? "
                                "\" < > | y sin acabar en punto o espacio.")
                                 .arg(name));
    return ok;
}

/* Mandar ficheros del PC a la carpeta que se ve, preguntando antes de
 * sustituir lo que haya con su nombre. */
void Window::putFiles(const QStringList &files)
{
    if (busy || files.isEmpty())
        return;
    if (drive == ROM) {
        say(Ic::Warning, tr("En la ROM no se puede escribir."), true);
        return;
    }
    if (drive != EXT && link != Link::On) {
        say(Ic::Warning, tr("Antes, conecta con la máquina (arriba)."), true);
        return;
    }
    QStringList clash, ok;
    for (const QString &f : files) {
        QFileInfo fi(f);
        if (!fi.isFile())
            continue;
        if (drive != EXT && !validName(fi.fileName()))
            return;
        ok << fi.absoluteFilePath();
        if (existsHere(fi.fileName()))
            clash << fi.fileName();
    }
    if (ok.isEmpty())
        return;
    if (!clash.isEmpty() &&
        QMessageBox::question(this, tr("Sustituir"),
                              tr("En %1 ya hay: %2.\n\nSi los sustituyes, lo que había se pierde. ¿Sustituirlos?")
                                  .arg(machinePath(), clash.join(QStringLiteral(", "))),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    settings.setValue(QStringLiteral("dir"), QFileInfo(ok.first()).absolutePath());
    if (drive == EXT) {
        QDir().mkpath(extLocal());
        for (const QString &f : ok) {
            QString to = extLocal(QFileInfo(f).fileName());
            QFile::remove(to);
            if (!QFile::copy(f, to)) {
                say(Ic::Error, tr("No se puede copiar %1").arg(QDir::toNativeSeparators(f)), true);
                refreshList();
                return;
            }
        }
        say(Ic::Done, tr("%n fichero(s) copiado(s) a %1", nullptr, int(ok.size())).arg(machinePath()));
        refreshList();
        return;
    }
    begin(Op::Put, tr("Mandando %n fichero(s) a %1...", nullptr, int(ok.size())).arg(machinePath()));
    emit wantPut(ok, machinePath());
}

void Window::choosePut()
{
    QString dir = settings.value(QStringLiteral("dir")).toString();
    QStringList f = QFileDialog::getOpenFileNames(this, tr("Ficheros que se mandan a %1").arg(machinePath()), dir);
    putFiles(f);
}

/* Traer lo elegido a una carpeta del PC, preguntando antes de sustituir. */
void Window::getSelected()
{
    bool dirs;
    QStringList n = selectedNames(&dirs);
    if (busy || n.isEmpty())
        return;
    if (dirs) {
        say(Ic::Warning, tr("Las carpetas no se traen: entra en ella y elige sus ficheros."), true);
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, tr("La carpeta del PC adonde se traen"),
                                                    settings.value(QStringLiteral("getDir"),
                                                                   settings.value(QStringLiteral("dir"))).toString());
    if (dir.isEmpty())
        return;
    settings.setValue(QStringLiteral("getDir"), dir);
    QStringList clash;
    for (const QString &name : n)
        if (QFileInfo::exists(QDir(dir).filePath(name)))
            clash << name;
    if (!clash.isEmpty() &&
        QMessageBox::question(this, tr("Sustituir"),
                              tr("En %1 ya hay: %2.\n\nSi los sustituyes, lo que había se pierde. ¿Sustituirlos?")
                                  .arg(QDir::toNativeSeparators(dir), clash.join(QStringLiteral(", "))),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    if (drive == EXT) {
        for (const QString &name : n) {
            QString to = QDir(dir).filePath(name);
            QFile::remove(to);
            if (!QFile::copy(extLocal(name), to)) {
                say(Ic::Error, tr("No se puede copiar %1").arg(name), true);
                return;
            }
        }
        say(Ic::Done, tr("%n fichero(s) copiado(s) a %1", nullptr, int(n.size())).arg(QDir::toNativeSeparators(dir)));
        return;
    }
    QStringList full;
    for (const QString &name : n)
        full << machinePath(name);
    begin(Op::Get, tr("Trayendo %n fichero(s)...", nullptr, int(n.size())));
    emit wantGet(full, dir);
}

void Window::newFolder()
{
    if (busy || drive == ROM || (drive != EXT && link != Link::On))
        return;
    QString name = QInputDialog::getText(this, tr("Carpeta nueva"), tr("Nombre de la carpeta nueva en %1:").arg(machinePath()));
    if (name.isEmpty() || !validName(name))
        return;
    if (existsHere(name)) {
        say(Ic::Warning, tr("Ya hay algo con ese nombre."), true);
        return;
    }
    if (drive == EXT) {
        if (QDir().mkpath(extLocal(name)))
            say(Ic::Done, tr("Carpeta creada: %1").arg(name));
        else
            say(Ic::Error, tr("No se puede crear %1").arg(name), true);
        refreshList();
        return;
    }
    begin(Op::Mkdir, tr("Creando %1...").arg(name));
    emit wantMkdir(machinePath(name));
}

void Window::renameSelected()
{
    QStringList n = selectedNames();
    if (busy || n.size() != 1 || drive == ROM || (drive != EXT && link != Link::On))
        return;
    bool ok;
    QString to = QInputDialog::getText(this, tr("Renombrar"), tr("Nombre nuevo de %1:").arg(n.first()),
                                       QLineEdit::Normal, n.first(), &ok);
    if (!ok || to.isEmpty() || to == n.first() || !validName(to))
        return;
    if (existsHere(to) && to.compare(n.first(), Qt::CaseInsensitive) != 0) {
        say(Ic::Warning, tr("Ya hay algo con ese nombre."), true);
        return;
    }
    if (drive == EXT) {
        if (QDir(extLocal()).rename(n.first(), to))
            say(Ic::Done, tr("Renombrado: %1").arg(to));
        else
            say(Ic::Error, tr("No se puede renombrar %1").arg(n.first()), true);
        refreshList();
        return;
    }
    begin(Op::Rename, tr("Renombrando %1...").arg(n.first()));
    emit wantRename(machinePath(n.first()), machinePath(to));
}

void Window::deleteSelected()
{
    bool dirs;
    QStringList n = selectedNames(&dirs);
    if (busy || n.isEmpty() || drive == ROM || (drive != EXT && link != Link::On))
        return;
    QString what = n.size() == 1 ? QStringLiteral("«%1»").arg(n.first()) : tr("%n elemento(s)", nullptr, int(n.size()));
    QString text = drive == EXT
                       ? tr("¿Borrar %1 de la carpeta de EXT:? Va a la papelera del PC.").arg(what)
                       : tr("¿Borrar %1 de %2? No se puede deshacer: en la tarjeta no hay papelera.")
                             .arg(what, drivePrefix(drive));
    if (dirs)
        text += QStringLiteral("\n\n") + tr("Las carpetas se borran con todo lo que tienen dentro.");
    if (QMessageBox::question(this, tr("Borrar"), text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) !=
        QMessageBox::Yes)
        return;
    if (drive == EXT) {
        for (const QString &name : n)
            if (!QFile::moveToTrash(extLocal(name))) {
                say(Ic::Error, tr("No se puede borrar %1").arg(name), true);
                refreshList();
                return;
            }
        say(Ic::Done, tr("A la papelera: %1").arg(what));
        refreshList();
        return;
    }
    QStringList full;
    for (const QString &name : n)
        full << machinePath(name);
    begin(Op::Remove, tr("Borrando %1...").arg(what));
    emit wantRemove(full);
}

/* --- Lo que se hace y cómo va ------------------------------------------------ */

void Window::begin(Op o, const QString &text)
{
    op = o;
    busy = true;
    worker->arm();
    bar->setValue(0);
    bar->setVisible(o == Op::Put || o == Op::Get || o == Op::Update);
    say(Ic::Sending, text);
    updateButtons();
}

void Window::say(Ic ic, const QString &text, bool error)
{
    statusIcon->setPixmap(icon(ic).pixmap(18, 18));
    status->setText(text);
    QPalette pal = status->palette();
    pal.setColor(QPalette::WindowText, error ? QColor(0xD7, 0, 0) : palette().color(QPalette::WindowText));
    status->setPalette(pal);
}

/* Con un programa en marcha: preguntar y, con permiso, otra vez. */
void Window::askUser(int what, const QString &text)
{
    Q_UNUSED(what);
    Q_UNUSED(text);
    auto yes = QMessageBox::question(this, tr("Parar el programa en marcha"),
                                     tr("Hay un programa en marcha en la máquina. Para ejecutar este, la máquina se "
                                        "reinicia entera, como con Ctrl+Alt+Supr: lo que haya en su memoria sin "
                                        "guardar se pierde.\n\n¿Reiniciarla?"),
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (yes != QMessageBox::Yes) {
        busy = false;
        op = Op::None;
        say(Ic::Same, tr("No se ha hecho nada"));
        updateButtons();
        return;
    }
    worker->arm();
    if (op == Op::RunPc)
        emit wantRunPc(runTarget, true);
    else
        emit wantRunPath(runTarget, true);
}

void Window::updateButtons()
{
    bool on = link == Link::On;
    bool canWrite = !busy && drive != ROM && (drive == EXT || on);
    bool dirs;
    QStringList sel = selectedNames(&dirs);
    bool canSee = drive == EXT || on;
    pcRun->setEnabled(!busy && on && !pcFile().isEmpty());
    pcChoose->setEnabled(!busy);
    actRun->setEnabled(!busy && on && sel.size() == 1 && !dirs && isProgram(sel.first()));
    actPut->setEnabled(canWrite);
    actGet->setEnabled(!busy && canSee && !sel.isEmpty() && !dirs);
    actMkdir->setEnabled(canWrite);
    actRename->setEnabled(canWrite && sel.size() == 1);
    actDelete->setEnabled(canWrite && !sel.isEmpty());
    actUp->setEnabled(!busy && paths[drive] != QLatin1String("/"));
    actRefresh->setEnabled(!busy && canSee);
    for (QToolButton *b : driveButtons)
        b->setEnabled(!busy);
    extChoose->setEnabled(!busy);
    bool net = isNet(currentPort());
    pairButton->setVisible(net);
    pairButton->setEnabled(!busy);
    menuPair->setEnabled(!busy && net);
    menuUpdate->setEnabled(!busy && on);
    for (QWidget *w : std::initializer_list<QWidget *> { ports, refreshButton, plugButton })
        w->setEnabled(!busy && ports->count() > 0);
    refreshButton->setEnabled(!busy);
    stopButton->setVisible(busy);
    if (!busy) {
        stopButton->setEnabled(true);
        stopButton->setText(tr("Parar"));
    }
}

/* --- Arrastrar ----------------------------------------------------------------- */

void Window::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls() && !busy)
        e->acceptProposedAction();
}

/* A la lista: se mandan a esa carpeta. A otro sitio: un programa, para
 * ejecutarlo del PC. */
void Window::dropEvent(QDropEvent *e)
{
    QStringList files;
    for (const QUrl &u : e->mimeData()->urls())
        if (u.isLocalFile() && QFileInfo(u.toLocalFile()).isFile())
            files << u.toLocalFile();
    e->acceptProposedAction();
    if (files.isEmpty())
        return;
    QPoint at = list->mapFrom(this, e->position().toPoint());
    if (list->rect().contains(at)) {
        putFiles(files);
        return;
    }
    for (const QString &f : files)
        if (isProgram(f)) {
            setProgram(f);
            return;
        }
    say(Ic::Warning, tr("Para mandarlos a la máquina, arrástralos a la lista de abajo."), true);
}

bool Window::eventFilter(QObject *o, QEvent *e)
{
    if ((o == list || o == list->viewport()) && e->type() == QEvent::KeyPress) {
        auto *k = static_cast<QKeyEvent *>(e);
        if (k->key() == Qt::Key_Return || k->key() == Qt::Key_Enter) {
            openItem(list->currentItem());
            return true;
        }
    }
    return QMainWindow::eventFilter(o, e);
}

void Window::closeEvent(QCloseEvent *e)
{
    /* Actualizando, cerrar corta la actualización: la máquina deja de recibir
     * el fichero (como si fallara la tarjeta). */
    if (busy && op == Op::Update &&
        QMessageBox::question(this, tr("Actualizar firmware"),
                              tr("La máquina se está actualizando y lee el fichero de este PC. Si cierras, la "
                                 "actualización se corta, la máquina lo dice en su pantalla y habrá que repetirla."
                                 "\n\n¿Cerrar de todos modos?"),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        e->ignore();
        return;
    }
    if (busy)
        worker->cancel();               /* lo que esté haciendo acaba enseguida: si no, el hilo no termina */
    emit wantClose();
    e->accept();
}
