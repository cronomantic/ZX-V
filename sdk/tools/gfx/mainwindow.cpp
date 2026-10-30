/*
 * mainwindow.cpp - La ventana de zxv-gfx (ver mainwindow.h).
 */
#include "mainwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QSaveFile>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabBar>
#include <QToolBar>
#include <QVBoxLayout>

#include "canvas.h"
#include "importdialog.h"
#include "mode7.h"
#include "panels.h"
#include "png.h"
#include "zxv_version.h"

/* Lo que se copia, con sus colores de ZX-V tal cual (también va como
 * imagen, para pegarlo en otros programas). */
static const char *const CLIP_MIME = "application/x-zxv-gfx-image";

static QSettings *makeSettings(QObject *parent)
{
    /* Con ZXV_HOME (las pruebas), en un .ini allí y no en el sistema. */
    QByteArray home = qgetenv("ZXV_HOME");
    if (!home.isEmpty())
        return new QSettings(QString::fromLocal8Bit(home) + "/zxv-gfx.ini", QSettings::IniFormat, parent);
    return new QSettings(QStringLiteral("ZX-V"), QStringLiteral("zxv-gfx"), parent);
}

/* Los iconos de las herramientas, dibujados aquí. */
static QIcon toolIcon(Tool t)
{
    QPixmap pm(24, 24);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor(0x20, 0x20, 0x20), 2);
    p.setPen(pen);
    switch (t) {
    case Tool::Pen:
        p.setPen(QPen(QColor(0x20, 0x20, 0x20), 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(6, 18, 18, 6);
        p.setPen(QPen(QColor(0xD7, 0, 0), 3, Qt::SolidLine, Qt::RoundCap));
        p.drawPoint(6, 18);
        break;
    case Tool::Line: p.drawLine(4, 20, 20, 4); break;
    case Tool::Rect: p.drawRect(4, 6, 16, 12); break;
    case Tool::FilledRect: p.setBrush(QColor(0x00, 0x00, 0xD7)); p.drawRect(4, 6, 16, 12); break;
    case Tool::Ellipse: p.drawEllipse(4, 5, 16, 14); break;
    case Tool::FilledEllipse: p.setBrush(QColor(0x00, 0xD7, 0x00)); p.drawEllipse(4, 5, 16, 14); break;
    case Tool::Fill:
        p.setBrush(QColor(0xD7, 0xD7, 0x00));
        p.drawPolygon(QPolygon({ QPoint(5, 11), QPoint(12, 4), QPoint(19, 11), QPoint(12, 18) }));
        p.drawLine(19, 12, 19, 20);
        break;
    case Tool::Pick:
        p.drawLine(5, 19, 14, 10);
        p.setBrush(QColor(0x20, 0x20, 0x20));
        p.drawEllipse(13, 4, 7, 7);
        break;
    case Tool::Select:
        p.setPen(QPen(QColor(0x20, 0x20, 0x20), 1.5, Qt::DashLine));
        p.drawRect(4, 5, 16, 14);
        break;
    }
    return QIcon(pm);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ed_(new Editor(this)), settings_(makeSettings(this))
{
    setAcceptDrops(true);
    auto *central = new QWidget;
    auto *lay = new QVBoxLayout(central);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    tabs_ = new QTabBar;
    tabs_->addTab(tr("Sprites"));
    tabs_->addTab(tr("Tiles de 8"));
    tabs_->addTab(tr("Tiles de 16"));
    tabs_->addTab(tr("Mapas"));
    tabs_->addTab(tr("Texturas"));
    tabs_->setExpanding(false);
    lay->addWidget(tabs_);
    stack_ = new QStackedWidget;
    auto area = [](QWidget *w) {
        auto *a = new QScrollArea;
        a->setWidget(w);
        a->setAlignment(Qt::AlignCenter);
        a->setBackgroundRole(QPalette::Dark);
        return a;
    };
    pixel_ = new PixelCanvas(ed_);
    map_ = new MapCanvas(ed_);
    stack_->addWidget(pixelArea_ = area(pixel_));
    stack_->addWidget(mapArea_ = area(map_));
    lay->addWidget(stack_, 1);
    setCentralWidget(central);

    hover_ = new QLabel;
    zoom_ = new QLabel;
    memory_ = new QLabel;
    statusBar()->addWidget(hover_, 1);
    statusBar()->addPermanentWidget(zoom_);
    statusBar()->addPermanentWidget(memory_);

    buildDocks();
    buildActions();

    connect(tabs_, &QTabBar::currentChanged, this, [this](int t) { showSection(Section(t)); });
    connect(ed_, &Editor::selectionChanged, this, &MainWindow::updateSection);
    connect(ed_, &Editor::stateChanged, this, &MainWindow::updateState);
    connect(ed_, &Editor::documentChanged, this, &MainWindow::updateState);
    for (Canvas *c : std::initializer_list<Canvas *>{ pixel_, map_ }) {
        connect(c, &Canvas::hover, hover_, &QLabel::setText);
        connect(c, &Canvas::zoomChanged, this, [this](int z) { zoom_->setText(tr("Zoom %1x").arg(z)); });
    }
    connect(ed_, &Editor::selectionChanged, this, [this] {
        /* Cada objeto nuevo, entero a la vista. */
        if (ed_->section() == Section::Maps)
            map_->fit();
        else
            pixel_->fit();
    });

    restoreGeometry(settings_->value("window/geometry").toByteArray());
    restoreState(settings_->value("window/state").toByteArray());
    if (!settings_->contains("window/geometry"))
        resize(1180, 760);
    rebuildRecent();
    updateSection();
    updateState();
}

MainWindow::~MainWindow() = default;

QAction *MainWindow::action(const QString &text, const QKeySequence &key, const std::function<void()> &fn)
{
    auto *a = new QAction(text, this);
    a->setShortcut(key);
    connect(a, &QAction::triggered, this, fn);
    addAction(a);
    return a;
}

void MainWindow::buildDocks()
{
    auto dock = [this](const QString &title, const QString &name, QWidget *w, Qt::DockWidgetArea where) {
        auto *d = new QDockWidget(title, this);
        d->setObjectName(name);
        d->setWidget(w);
        addDockWidget(where, d);
        return d;
    };
    auto *objects = new ObjectList(ed_);
    objectsDock_ = dock(tr("Objetos"), "objects", objects, Qt::LeftDockWidgetArea);
    paletteDock_ = dock(tr("Paleta"), "palette", new PalettePanel(ed_), Qt::RightDockWidgetArea);
    auto *props = new PropertiesPanel(ed_);
    propsDock_ = dock(tr("Propiedades"), "properties", props, Qt::RightDockWidgetArea);
    auto *preview = new PreviewPanel(ed_);
    previewDock_ = dock(tr("Vista previa"), "preview", preview, Qt::RightDockWidgetArea);
    auto *picker = new TilePicker(ed_);
    tilesDock_ = dock(tr("Tiles para pintar"), "tiles", picker, Qt::RightDockWidgetArea);
    connect(objects, &ObjectList::createRequested, props, &PropertiesPanel::createObject);
    connect(preview, &PreviewPanel::mode7Requested, this, [this] { mode7_->trigger(); });
    connect(picker, &TilePicker::editTile, this, [this](int side, int i) {
        showSection(side == 8 ? Section::Tiles8 : Section::Tiles16);
        ed_->select(i);
    });
}

void MainWindow::buildActions()
{
    /* Archivo. */
    QMenu *file = menuBar()->addMenu(tr("&Archivo"));
    file->addAction(action(tr("&Nuevo"), QKeySequence::New, [this] {
        if (maybeSave())
            ed_->setDocument(gfx::Document(), QString());
    }));
    file->addAction(action(tr("&Abrir..."), QKeySequence::Open, [this] { openDialog(); }));
    recentMenu_ = file->addMenu(tr("Abrir &reciente"));
    file->addAction(action(tr("&Guardar"), QKeySequence::Save, [this] {
        if (ed_->path().isEmpty())
            saveAs();
        else
            save(ed_->path());
    }));
    file->addAction(action(tr("Guardar &como..."), QKeySequence::SaveAs, [this] { saveAs(); }));
    file->addSeparator();
    file->addAction(action(tr("&Importar una imagen..."), QKeySequence(tr("Ctrl+I")), [this] { importDialog(); }));
    file->addAction(action(tr("&Exportar como PNG..."), QKeySequence(tr("Ctrl+E")), [this] { exportImage(); }));
    file->addSeparator();
    file->addAction(action(tr("&Salir"), QKeySequence::Quit, [this] { close(); }));

    /* Editar. */
    QMenu *edit = menuBar()->addMenu(tr("&Editar"));
    edit->addAction(undo_ = action(tr("&Deshacer"), QKeySequence::Undo, [this] {
        pixel_->commit();
        ed_->undo();
    }));
    edit->addAction(redo_ = action(tr("&Rehacer"), QKeySequence::Redo, [this] {
        pixel_->commit();
        ed_->redo();
    }));
    edit->addSeparator();
    edit->addAction(cut_ = action(tr("Cor&tar"), QKeySequence::Cut, [this] { copy(true); }));
    edit->addAction(copy_ = action(tr("&Copiar"), QKeySequence::Copy, [this] { copy(false); }));
    edit->addAction(paste_ = action(tr("&Pegar"), QKeySequence::Paste, [this] { pasteClipboard(); }));
    edit->addAction(erase_ = action(tr("&Vaciar"), QKeySequence::Delete, [this] { pixel_->erase(); }));
    edit->addSeparator();
    edit->addAction(selectAll_ = action(tr("Seleccionar &todo"), QKeySequence::SelectAll, [this] { pixel_->selectAll(); }));
    edit->addAction(action(tr("Dejarlo &aquí"), QKeySequence(Qt::Key_Return), [this] { pixel_->commit(); }));
    edit->addAction(action(tr("Quitar la selección"), QKeySequence(Qt::Key_Escape), [this] {
        pixel_->discard();
        pixel_->clearSelection();
    }));
    edit->addSeparator();
    QAction *fh = action(tr("Espejo &horizontal"), QKeySequence(tr("H")), [this] { transform(gfx::flippedH); });
    QAction *fv = action(tr("Espejo v&ertical"), QKeySequence(tr("V")), [this] { transform(gfx::flippedV); });
    QAction *rr = action(tr("Girar a la de&recha"), QKeySequence(tr("]")), [this] { transform(gfx::rotatedRight); });
    QAction *rl = action(tr("Girar a la i&zquierda"), QKeySequence(tr("[")), [this] { transform(gfx::rotatedLeft); });
    edit->addActions({ fh, fv, rr, rl });
    QMenu *shift = edit->addMenu(tr("&Mover"));
    QAction *sl = action(tr("A la izquierda"), QKeySequence(tr("Ctrl+Left")),
                         [this] { transform([](const gfx::Image &i) { return gfx::shifted(i, -1, 0); }); });
    QAction *sr = action(tr("A la derecha"), QKeySequence(tr("Ctrl+Right")),
                         [this] { transform([](const gfx::Image &i) { return gfx::shifted(i, 1, 0); }); });
    QAction *su = action(tr("Arriba"), QKeySequence(tr("Ctrl+Up")),
                         [this] { transform([](const gfx::Image &i) { return gfx::shifted(i, 0, -1); }); });
    QAction *sd = action(tr("Abajo"), QKeySequence(tr("Ctrl+Down")),
                         [this] { transform([](const gfx::Image &i) { return gfx::shifted(i, 0, 1); }); });
    shift->addActions({ sl, sr, su, sd });
    edit->addSeparator();
    edit->addAction(action(tr("Cambiar los dos colores"), QKeySequence(tr("X")), [this] {
        int a = ed_->color(0);
        ed_->setColor(0, ed_->color(1));
        ed_->setColor(1, a);
    }));
    imageOnly_ = { cut_, copy_, paste_, erase_, selectAll_, fh, fv, rr, rl, sl, sr, su, sd };

    /* Las herramientas. */
    QToolBar *bar = addToolBar(tr("Herramientas"));
    bar->setObjectName("tools");
    tools_ = new QActionGroup(this);
    struct ToolDef { Tool t; const char *name; const char *key; };
    static const ToolDef defs[] = {
        { Tool::Pen, QT_TR_NOOP("Lápiz"), "P" }, { Tool::Line, QT_TR_NOOP("Línea"), "L" },
        { Tool::Rect, QT_TR_NOOP("Caja"), "R" }, { Tool::FilledRect, QT_TR_NOOP("Caja llena"), "Shift+R" },
        { Tool::Ellipse, QT_TR_NOOP("Elipse"), "E" }, { Tool::FilledEllipse, QT_TR_NOOP("Elipse llena"), "Shift+E" },
        { Tool::Fill, QT_TR_NOOP("Rellenar"), "F" }, { Tool::Pick, QT_TR_NOOP("Coger un color"), "K" },
        { Tool::Select, QT_TR_NOOP("Seleccionar y mover"), "S" },
    };
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Herramientas"));
    for (const ToolDef &d : defs) {
        QAction *a = action(tr(d.name), QKeySequence(QString::fromLatin1(d.key)), [this, t = d.t] { ed_->setTool(t); });
        a->setIcon(toolIcon(d.t));
        a->setToolTip(tr("%1 (%2)").arg(tr(d.name), QString::fromLatin1(d.key)));
        a->setCheckable(true);
        a->setData(int(d.t));
        a->setChecked(d.t == ed_->tool());
        tools_->addAction(a);
        bar->addAction(a);
        toolsMenu->addAction(a);
    }
    connect(ed_, &Editor::toolChanged, this, [this] {
        for (QAction *a : tools_->actions())
            if (a->data().toInt() == int(ed_->tool()))
                a->setChecked(true);
    });

    /* Ver. */
    QMenu *view = menuBar()->addMenu(tr("&Ver"));
    auto canvas = [this]() -> Canvas * {
        return ed_->section() == Section::Maps ? static_cast<Canvas *>(map_) : static_cast<Canvas *>(pixel_);
    };
    view->addAction(action(tr("A&cercar"), QKeySequence::ZoomIn, [canvas] { canvas()->zoomBy(1); }));
    view->addAction(action(tr("A&lejar"), QKeySequence::ZoomOut, [canvas] { canvas()->zoomBy(-1); }));
    view->addAction(action(tr("Verlo &entero"), QKeySequence(tr("Ctrl+0")), [canvas] { canvas()->fit(); }));
    grid_ = action(tr("&Rejilla"), QKeySequence(tr("G")), [this] {
        pixel_->setGrid(grid_->isChecked());
        map_->setGrid(grid_->isChecked());
    });
    grid_->setCheckable(true);
    grid_->setChecked(true);
    view->addAction(grid_);
    front_ = action(tr("Marcar las celdas de &delante"), QKeySequence(), [this] { map_->setShowFront(front_->isChecked()); });
    front_->setCheckable(true);
    front_->setChecked(true);
    view->addAction(front_);
    mode7_ = action(tr("La textura en el &modo 7..."), QKeySequence(tr("Ctrl+7")), [this] {
        if (ed_->section() == Section::Textures && !ed_->doc().textures[ed_->index()].empty())
            Mode7Dialog(ed_, ed_->index(), this).exec();
    });
    view->addAction(mode7_);
    view->addSeparator();
    for (QDockWidget *d : { objectsDock_, paletteDock_, propsDock_, previewDock_, tilesDock_ })
        view->addAction(d->toggleViewAction());

    bar->addSeparator();
    bar->addAction(undo_);
    bar->addAction(redo_);

    QMenu *help = menuBar()->addMenu(tr("A&yuda"));
    help->addAction(action(tr("&Acerca de zxv-gfx"), QKeySequence(), [this] { about(); }));
}

/* --- Lo que se ve según la pestaña ------------------------------------------------ */

void MainWindow::showSection(Section s)
{
    pixel_->commit();
    ed_->select(s, ed_->index(s));
}

void MainWindow::updateSection()
{
    Section s = ed_->section();
    bool maps = s == Section::Maps;
    {
        QSignalBlocker block(tabs_);
        tabs_->setCurrentIndex(int(s));
    }
    stack_->setCurrentWidget(maps ? mapArea_ : pixelArea_);
    tilesDock_->setVisible(maps);
    paletteDock_->setVisible(!maps);
    for (QAction *a : imageOnly_)
        a->setEnabled(!maps);
    mode7_->setEnabled(s == Section::Textures);
    /* En los mapas: lápiz, caja (llena), rellenar y coger. */
    for (QAction *a : tools_->actions()) {
        Tool t = Tool(a->data().toInt());
        a->setEnabled(!maps || t == Tool::Pen || t == Tool::FilledRect || t == Tool::Fill || t == Tool::Pick);
    }
    if (maps && !tools_->checkedAction()->isEnabled())
        ed_->setTool(Tool::Pen);
    Canvas *c = maps ? static_cast<Canvas *>(map_) : static_cast<Canvas *>(pixel_);
    zoom_->setText(tr("Zoom %1x").arg(c->zoom()));
    hover_->clear();
}

void MainWindow::updateState()
{
    QString name = ed_->path().isEmpty() ? tr("Sin nombre") : QFileInfo(ed_->path()).fileName();
    setWindowTitle(tr("%1%2 - Editor de gráficos de ZX-V").arg(name, ed_->modified() ? "*" : ""));
    undo_->setEnabled(ed_->canUndo());
    redo_->setEnabled(ed_->canRedo());
    size_t kb = (ed_->doc().machineBytes() + 1023) / 1024;
    memory_->setText(tr("En la máquina, unos %1 KB").arg(kb));
}

void MainWindow::message(const QString &text)
{
    statusBar()->showMessage(text, 5000);
}

/* --- Ficheros ----------------------------------------------------------------------- */

bool MainWindow::open(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Abrir"), tr("No se puede abrir %1.").arg(QDir::toNativeSeparators(path)));
        return false;
    }
    QByteArray data = f.readAll();
    gfx::Document doc;
    int r = doc.load(std::vector<uint8_t>(data.begin(), data.end()));
    if (r != gfx::LOAD_OK) {
        QString why = r == gfx::LOAD_NOT_GFX  ? tr("no es un fichero de gráficos de ZX-V")
                      : r == gfx::LOAD_UNKNOWN ? tr("tiene objetos que no conozco (¿es de una versión más nueva?)")
                                               : tr("está cortado o dañado");
        QMessageBox::warning(this, tr("Abrir"), tr("%1: %2.").arg(QFileInfo(path).fileName(), why));
        return false;
    }
    ed_->setDocument(doc, QFileInfo(path).absoluteFilePath());
    addRecent(ed_->path());
    /* En cada pestaña, su primer objeto; y se empieza por la primera que
     * tenga algo. */
    auto first = [&doc](Section s) {
        int n = s == Section::Sprites ? gfx::SPRITES : s == Section::Maps ? gfx::MAPS : gfx::TEXTURES;
        for (int i = 0; i < n; i++)
            if (s == Section::Sprites ? !doc.sprites[i].empty()
                : s == Section::Maps  ? !doc.maps[i].empty()
                                      : !doc.textures[i].empty())
                return i;
        return -1;
    };
    Section start = Section::Sprites;
    for (Section s : { Section::Textures, Section::Maps, Section::Sprites }) {
        int i = first(s);
        ed_->select(s, std::max(i, 0));
        if (i >= 0)
            start = s;
    }
    if (first(Section::Sprites) < 0)
        start = !doc.tiles8.empty() ? Section::Tiles8 : !doc.tiles16.empty() ? Section::Tiles16 : start;
    showSection(start);
    message(tr("Abierto %1").arg(QDir::toNativeSeparators(path)));
    return true;
}

bool MainWindow::save(const QString &path)
{
    pixel_->commit();
    std::vector<uint8_t> data = ed_->doc().save();
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(reinterpret_cast<const char *>(data.data()), qint64(data.size())) != qint64(data.size()) ||
        !f.commit()) {
        QMessageBox::warning(this, tr("Guardar"), tr("No se ha podido guardar %1.").arg(QDir::toNativeSeparators(path)));
        return false;
    }
    ed_->setSaved(QFileInfo(path).absoluteFilePath());
    addRecent(ed_->path());
    message(tr("Guardado %1 (%2 bytes)").arg(QDir::toNativeSeparators(path)).arg(data.size()));
    return true;
}

bool MainWindow::saveAs()
{
    QString dir = ed_->path().isEmpty() ? settings_->value("files/dir").toString() : ed_->path();
    QString path = QFileDialog::getSaveFileName(this, tr("Guardar como"), dir, tr("Gráficos de ZX-V (*.gfx)"));
    if (path.isEmpty())
        return false;
    if (QFileInfo(path).suffix().isEmpty())
        path += ".gfx";
    settings_->setValue("files/dir", QFileInfo(path).absolutePath());
    return save(path);
}

bool MainWindow::maybeSave()
{
    pixel_->commit();
    if (!ed_->modified())
        return true;
    auto r = QMessageBox::question(this, tr("Cambios sin guardar"), tr("¿Guardar los cambios?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Save)
        return ed_->path().isEmpty() ? saveAs() : save(ed_->path());
    return r == QMessageBox::Discard;
}

void MainWindow::openDialog()
{
    if (!maybeSave())
        return;
    QString path = QFileDialog::getOpenFileName(this, tr("Abrir"), settings_->value("files/dir").toString(),
                                                tr("Gráficos de ZX-V (*.gfx);;Todos los ficheros (*)"));
    if (path.isEmpty())
        return;
    settings_->setValue("files/dir", QFileInfo(path).absolutePath());
    open(path);
}

void MainWindow::addRecent(const QString &path)
{
    QStringList list = settings_->value("files/recent").toStringList();
    list.removeAll(path);
    list.prepend(path);
    while (list.size() > 8)
        list.removeLast();
    settings_->setValue("files/recent", list);
    rebuildRecent();
}

void MainWindow::rebuildRecent()
{
    recentMenu_->clear();
    const QStringList list = settings_->value("files/recent").toStringList();
    for (const QString &p : list)
        recentMenu_->addAction(QDir::toNativeSeparators(p), this, [this, p] {
            if (maybeSave())
                open(p);
        });
    recentMenu_->setEnabled(!list.isEmpty());
}

void MainWindow::importImage(const QString &path)
{
    QImage img(path);
    if (img.isNull()) {
        QMessageBox::warning(this, tr("Importar"), tr("No sé leer la imagen %1.").arg(QDir::toNativeSeparators(path)));
        return;
    }
    pixel_->commit();
    ImportDialog dlg(ed_, img, QFileInfo(path).fileName(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    QString err = dlg.apply();
    if (!err.isEmpty())
        QMessageBox::warning(this, tr("Importar"), err);
}

void MainWindow::importDialog()
{
    QStringList filters;
    for (const QByteArray &f : QImageReader::supportedImageFormats())
        filters << "*." + QString::fromLatin1(f);
    QString path = QFileDialog::getOpenFileName(this, tr("Importar una imagen"), settings_->value("files/imageDir").toString(),
                                                tr("Imágenes (%1)").arg(filters.join(' ')));
    if (path.isEmpty())
        return;
    settings_->setValue("files/imageDir", QFileInfo(path).absolutePath());
    importImage(path);
}

void MainWindow::exportImage()
{
    pixel_->commit();
    const gfx::Document &doc = ed_->doc();
    int i = ed_->index();
    QImage img;
    QString base;
    switch (ed_->section()) {
    case Section::Sprites:
        if (!doc.sprites[i].empty())
            img = gfx::toQImage(doc.sprites[i], true);
        base = QString("sprite%1").arg(i);
        break;
    case Section::Tiles8:
    case Section::Tiles16:
        if (!doc.tiles(ed_->tileSide()).empty())
            img = gfx::tileSheet(doc, ed_->tileSide(), 16);
        base = QString("tiles%1").arg(ed_->tileSide());
        break;
    case Section::Maps:
        if (!doc.maps[i].empty())
            img = gfx::renderMap(doc, i);
        base = QString("mapa%1").arg(i);
        break;
    case Section::Textures:
        if (!doc.textures[i].empty())
            img = gfx::toQImage(doc.textures[i].img, false);
        base = QString("textura%1").arg(i);
        break;
    }
    if (img.isNull()) {
        message(tr("No hay nada que exportar aquí."));
        return;
    }
    QString dir = settings_->value("files/imageDir").toString();
    QString path = QFileDialog::getSaveFileName(this, tr("Exportar como PNG"), QDir(dir).filePath(base + ".png"),
                                                tr("Imágenes PNG (*.png)"));
    if (path.isEmpty())
        return;
    settings_->setValue("files/imageDir", QFileInfo(path).absolutePath());
    if (!img.save(path, "PNG"))
        QMessageBox::warning(this, tr("Exportar"), tr("No se ha podido guardar %1.").arg(QDir::toNativeSeparators(path)));
    else
        message(tr("Exportado %1").arg(QDir::toNativeSeparators(path)));
}

/* --- Editar --------------------------------------------------------------------------- */

void MainWindow::copy(bool cut)
{
    gfx::Image img = pixel_->selected();
    if (img.empty())
        return;
    QByteArray raw;
    raw.append(char(img.w & 255)).append(char(img.w >> 8)).append(char(img.h & 255)).append(char(img.h >> 8));
    raw.append(reinterpret_cast<const char *>(img.px.data()), qsizetype(img.px.size()));
    auto *mime = new QMimeData;
    mime->setData(CLIP_MIME, raw);
    mime->setImageData(gfx::toQImage(img, ed_->transparentImage()));
    QApplication::clipboard()->setMimeData(mime);
    if (cut)
        pixel_->erase();
    message(cut ? tr("Cortado %1 x %2").arg(img.w).arg(img.h) : tr("Copiado %1 x %2").arg(img.w).arg(img.h));
}

void MainWindow::pasteClipboard()
{
    const QMimeData *mime = QApplication::clipboard()->mimeData();
    gfx::Image img;
    if (mime && mime->hasFormat(CLIP_MIME)) {
        QByteArray raw = mime->data(CLIP_MIME);
        if (raw.size() >= 4) {
            int w = uint8_t(raw[0]) | uint8_t(raw[1]) << 8, h = uint8_t(raw[2]) | uint8_t(raw[3]) << 8;
            if (raw.size() == 4 + w * h) {
                img = gfx::Image(w, h, 0);
                std::copy(raw.begin() + 4, raw.end(), img.px.begin());
            }
        }
    } else if (mime && mime->hasImage()) {
        /* De otro programa: a los colores de ZX-V. */
        int inexact = 0;
        img = gfx::fromQImage(qvariant_cast<QImage>(mime->imageData()), &inexact);
        if (inexact)
            message(tr("%n píxel(es) no eran colores de ZX-V: se usa el más cercano.", nullptr, inexact));
    }
    if (img.empty()) {
        message(tr("No hay una imagen para pegar."));
        return;
    }
    if (!ed_->image() && ed_->section() == Section::Sprites) {
        /* En un sprite vacío: se crea del tamaño de lo pegado. */
        ed_->begin();
        ed_->doc().sprites[ed_->index()] = gfx::resized(img, img.w, img.h, gfx::TRANSP);
        ed_->changed();
        pixel_->fit();
        return;
    }
    pixel_->paste(img);
}

void MainWindow::transform(gfx::Image (*fn)(const gfx::Image &))
{
    if (ed_->section() != Section::Maps)
        pixel_->transform(fn);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("Acerca de zxv-gfx"),
                       tr("<p><b>Editor de gráficos de ZX-V</b> %1</p>"
                          "<p>Sprites, tiles, mapas y texturas para el BASIC de ZX-V (LOAD \"x\" GRAPHICS) y "
                          "para los programas del SDK, en ficheros .gfx.</p>"
                          "<p>Software libre con la licencia MIT: el código está en el SDK de ZX-V "
                          "(sdk/tools/gfx), en <a href=\"https://github.com/cronomantic/ZX-V\">"
                          "github.com/cronomantic/ZX-V</a>.</p>")
                           .arg(QStringLiteral(ZXV_VERSION)));
}

/* --- La ventana ------------------------------------------------------------------------ */

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (!maybeSave()) {
        e->ignore();
        return;
    }
    settings_->setValue("window/geometry", saveGeometry());
    settings_->setValue("window/state", saveState());
    e->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    const QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty() || !urls.first().isLocalFile())
        return;
    QString path = urls.first().toLocalFile();
    if (QFileInfo(path).suffix().compare("gfx", Qt::CaseInsensitive) == 0) {
        if (maybeSave())
            open(path);
    } else {
        importImage(path);
    }
}
