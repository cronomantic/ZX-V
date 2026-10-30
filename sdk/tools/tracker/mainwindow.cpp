/*
 * mainwindow.cpp - La ventana de zxv-tracker.
 */
#include "mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

#include "audio.h"
#include "editor.h"
#include "lines.h"
#include "patternview.h"
#include "piano.h"
#include "song.h"
#include "zxv_version.h"

using namespace trk;

static QSettings *makeSettings(QObject *parent)
{
    /* Con ZXV_HOME (las pruebas), en un .ini allí y no en el sistema. */
    QByteArray home = qgetenv("ZXV_HOME");
    if (!home.isEmpty())
        return new QSettings(QString::fromLocal8Bit(home) + "/zxv-tracker.ini", QSettings::IniFormat, parent);
    return new QSettings(QStringLiteral("ZX-V"), QStringLiteral("zxv-tracker"), parent);
}

static QByteArray readAll(const QString &path, bool *ok)
{
    QFile f(path);
    *ok = f.open(QIODevice::ReadOnly);
    return *ok ? f.readAll() : QByteArray();
}

static bool writeAll(const QString &path, const std::vector<uint8_t> &data)
{
    QSaveFile f(path);
    return f.open(QIODevice::WriteOnly) &&
           f.write(reinterpret_cast<const char *>(data.data()), qint64(data.size())) == qint64(data.size()) && f.commit();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ed_(new Editor(this)), settings_(makeSettings(this)), audio_(new Audio(&engine_))
{
    setAcceptDrops(true);
    tabs_ = new QTabWidget;
    tabs_->addTab(buildPatternTab(), tr("Patrones"));
    tabs_->addTab(buildSampleTab(), tr("Samples"));
    tabs_->addTab(buildOrnamentTab(), tr("Ornamentos"));
    tabs_->addTab(buildEffectsTab(), tr("Efectos AYFX"));
    setCentralWidget(tabs_);
    buildActions();

    where_ = new QLabel;
    size_ = new QLabel;
    statusBar()->addPermanentWidget(where_);
    statusBar()->addPermanentWidget(size_);

    connect(ed_, &Editor::songChanged, this, [this] {
        refreshHeader();
        refreshPositions();
        refreshSample();
        refreshOrnament();
        updateState();
    });
    connect(ed_, &Editor::positionChanged, this, [this] {
        refreshHeader();
        refreshPositions();
        updateMute();
    });
    connect(ed_, &Editor::sampleChanged, this, &MainWindow::refreshSample);
    connect(ed_, &Editor::ornamentChanged, this, &MainWindow::refreshOrnament);
    connect(ed_, &Editor::bankChanged, this, &MainWindow::refreshEffects);
    connect(ed_, &Editor::effectChanged, this, &MainWindow::refreshEffects);
    connect(ed_, &Editor::stateChanged, this, &MainWindow::updateState);
    connect(tabs_, &QTabWidget::currentChanged, this, [this] {
        engine_.stopPreview();
        updateState();
        QWidget *focus[] = { pattern_, sampleView_, ornamentView_, sfxView_ };
        focus[std::clamp(tabs_->currentIndex(), 0, 3)]->setFocus();
    });

    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::tick);
    timer->start(25);

    octave_->setValue(settings_->value("edit/octave", 4).toInt());
    step_->setValue(settings_->value("edit/step", 1).toInt());
    follow_->setChecked(settings_->value("edit/follow", true).toBool());
    restoreGeometry(settings_->value("window/geometry").toByteArray());
    if (settings_->value("window/geometry").isNull())
        resize(1100, 720);

    newSong();
    newBank();
    rebuildRecent();
    if (!audio_->open())
        message(tr("Sin sonido: %1").arg(QString::fromStdString(audio_->error())));
    pattern_->setFocus();
}

MainWindow::~MainWindow()
{
    audio_->close();
}

QAction *MainWindow::action(const QString &text, const QKeySequence &key, const std::function<void()> &fn)
{
    auto *a = new QAction(text, this);
    if (!key.isEmpty())
        a->setShortcut(key);
    connect(a, &QAction::triggered, this, fn);
    addAction(a);
    return a;
}

/* --- Los patrones ----------------------------------------------------------- */

QWidget *MainWindow::buildPatternTab()
{
    auto *w = new QWidget;
    auto *lay = new QHBoxLayout(w);

    auto *side = new QVBoxLayout;
    auto *head = new QGroupBox(tr("Módulo"));
    auto *form = new QFormLayout(head);
    title_ = new QLineEdit;
    title_->setMaxLength(32);
    author_ = new QLineEdit;
    author_->setMaxLength(32);
    tempo_ = new QSpinBox;
    tempo_->setRange(1, 255);
    tempo_->setToolTip(tr("Pasos de 1/50 s por fila: más, más lento"));
    table_ = new QComboBox;
    for (int i = 0; i < 4; i++)
        table_->addItem(QString::number(i));
    table_->setToolTip(tr("La tabla de notas de Pro Tracker 3 (la 2 es la de siempre de Vortex Tracker)"));
    version_ = new QComboBox;
    for (int i = 0; i < 10; i++)
        version_->addItem(QStringLiteral("3.%1").arg(i));
    version_->setToolTip(tr("La versión de Pro Tracker 3: cambia algunos efectos y las tablas (Vortex Tracker usa la 3.6)"));
    turbo_ = new QCheckBox(tr("TurboSound: dos AY"));
    turbo_->setToolTip(tr("Dos módulos a la vez, uno en cada AY (ZX-V tiene dos)"));
    module_ = new QComboBox;
    module_->addItems({ tr("Módulo 1 (AY 1)"), tr("Módulo 2 (AY 2)") });
    form->addRow(tr("Título"), title_);
    form->addRow(tr("Autor"), author_);
    form->addRow(tr("Tempo"), tempo_);
    form->addRow(tr("Tabla"), table_);
    form->addRow(tr("Versión"), version_);
    form->addRow(turbo_);
    form->addRow(module_);
    side->addWidget(head);

    connect(title_, &QLineEdit::editingFinished, this, [this] {
        if (title_->text().toStdString() != ed_->mod().title)
            editHeader([this] { ed_->mod().title = title_->text().toLatin1().toStdString(); });
    });
    connect(author_, &QLineEdit::editingFinished, this, [this] {
        if (author_->text().toStdString() != ed_->mod().author)
            editHeader([this] { ed_->mod().author = author_->text().toLatin1().toStdString(); });
    });
    connect(tempo_, &QSpinBox::valueChanged, this, [this](int v) { editHeader([this, v] { ed_->mod().delay = v; }); });
    connect(table_, &QComboBox::currentIndexChanged, this, [this](int v) { editHeader([this, v] { ed_->mod().table = v; }); });
    connect(version_, &QComboBox::currentIndexChanged, this,
            [this](int v) { editHeader([this, v] { ed_->mod().version = v; }); });
    connect(turbo_, &QCheckBox::toggled, this, [this](bool on) {
        if (!refreshing_)
            setTurboSound(on);
    });
    connect(module_, &QComboBox::currentIndexChanged, this, [this](int m) {
        if (!refreshing_)
            ed_->setModule(m);
    });

    auto *posBox = new QGroupBox(tr("Posiciones"));
    auto *pv = new QVBoxLayout(posBox);
    positions_ = new QListWidget;
    positions_->setToolTip(tr("El orden de los patrones. L: la posición a la que vuelve al acabar."));
    pv->addWidget(positions_, 1);
    auto *pb = new QHBoxLayout;
    auto button = [&](const QString &text, const QString &tip, const std::function<void()> &fn) {
        auto *b = new QToolButton;
        b->setText(text);
        b->setToolTip(tip);
        connect(b, &QToolButton::clicked, this, fn);
        pb->addWidget(b);
    };
    button("+", tr("Añadir una posición detrás (con el mismo patrón)"), [this] { addPosition(); });
    button("-", tr("Quitar la posición"), [this] { removePosition(); });
    button(tr("Copia"), tr("Una copia del patrón, en un patrón libre, en esta posición"), [this] { clonePattern(); });
    button("L", tr("Que vuelva aquí al acabar"), [this] { setLoop(); });
    pv->addLayout(pb);
    auto *pf = new QFormLayout;
    patNum_ = new QSpinBox;
    patNum_->setRange(0, MAX_PATTERNS - 1);
    patNum_->setToolTip(tr("El patrón que suena en esta posición"));
    patRows_ = new QSpinBox;
    patRows_->setRange(1, MAX_ROWS);
    patRows_->setToolTip(tr("Las filas del patrón"));
    pf->addRow(tr("Patrón"), patNum_);
    pf->addRow(tr("Filas"), patRows_);
    pv->addLayout(pf);
    side->addWidget(posBox, 1);

    connect(positions_, &QListWidget::currentRowChanged, this, [this](int r) {
        if (!refreshing_ && r >= 0)
            ed_->setPosition(r);
    });
    connect(patNum_, &QSpinBox::valueChanged, this, [this](int v) {
        if (refreshing_)
            return;
        editHeader([this, v] { ed_->mod().order[size_t(ed_->position())] = v; });
    });
    connect(patRows_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            setPatternRows(v);
    });

    auto *sideW = new QWidget;
    sideW->setLayout(side);
    sideW->setMaximumWidth(260);
    lay->addWidget(sideW);
    pattern_ = new PatternView(ed_);
    lay->addWidget(pattern_, 1);
    connect(pattern_, &PatternView::preview, this, &MainWindow::previewCell);
    connect(pattern_, &PatternView::previewOff, this, &MainWindow::previewOff);
    connect(pattern_, &PatternView::stopRequested, this, &MainWindow::stop);
    connect(pattern_, &PatternView::cursorMoved, this, &MainWindow::updateMute);
    return w;
}

void MainWindow::editHeader(const std::function<void()> &fn)
{
    if (refreshing_)
        return;
    ed_->begin(Editor::Header);
    fn();
    ed_->changed(Editor::Header);
}

void MainWindow::refreshHeader()
{
    refreshing_ = true;
    const Module &m = ed_->mod();
    if (!title_->hasFocus())
        title_->setText(QString::fromLatin1(m.title.c_str()));
    if (!author_->hasFocus())
        author_->setText(QString::fromLatin1(m.author.c_str()));
    tempo_->setValue(m.delay);
    table_->setCurrentIndex(m.table & 3);
    version_->setCurrentIndex(std::clamp(m.version, 0, 9));
    bool ts = ed_->song().modules.size() > 1;
    turbo_->setChecked(ts);
    module_->setEnabled(ts);
    module_->setCurrentIndex(ed_->module());
    patNum_->setValue(ed_->pattern());
    patRows_->setValue(int(ed_->pat().rows.size()));
    refreshing_ = false;
}

void MainWindow::refreshPositions()
{
    refreshing_ = true;
    const Module &m = ed_->mod();
    if (positions_->count() != int(m.order.size())) {
        positions_->clear();
        for (size_t i = 0; i < m.order.size(); i++)
            positions_->addItem(QString());
    }
    QFont mono = pattern_->font();
    for (int i = 0; i < int(m.order.size()); i++) {
        QListWidgetItem *it = positions_->item(i);
        it->setFont(mono);
        int p = m.order[size_t(i)];
        int rows = int(m.patterns[size_t(p)].rows.size());
        it->setText(tr("%1  patrón %2  (%3)%4")
                        .arg(i, 2, 10, QChar('0'))
                        .arg(p, 2, 10, QChar('0'))
                        .arg(rows)
                        .arg(i == m.loop ? "  L" : ""));
    }
    positions_->setCurrentRow(ed_->position());
    refreshing_ = false;
}

void MainWindow::setTurboSound(bool on)
{
    ed_->begin(Editor::Everything);
    Song &s = ed_->song();
    if (on && s.modules.size() < 2) {
        Module m;
        m.title = s.modules[0].title;
        m.author = s.modules[0].author;
        m.delay = s.modules[0].delay;
        m.table = s.modules[0].table;
        m.version = s.modules[0].version;
        m.order.assign(s.modules[0].order.size(), 0);
        s.modules.push_back(m);
    } else if (!on && s.modules.size() > 1) {
        if (QMessageBox::question(this, tr("TurboSound"), tr("¿Quitar el módulo del segundo AY?")) != QMessageBox::Yes) {
            ed_->undo();
            refreshHeader();
            return;
        }
        s.modules.resize(1);
    }
    ed_->changed(Editor::Everything);
    ed_->setModule(on ? 1 : 0);
    refreshHeader();
}

void MainWindow::addPosition()
{
    if (int(ed_->mod().order.size()) >= MAX_POSITIONS)
        return;
    int p = ed_->position();
    editHeader([this, p] {
        std::vector<int> &o = ed_->mod().order;
        o.insert(o.begin() + p + 1, o[size_t(p)]);
    });
    ed_->setPosition(p + 1);
}

void MainWindow::removePosition()
{
    if (ed_->mod().order.size() <= 1)
        return;
    int p = ed_->position();
    editHeader([this, p] {
        Module &m = ed_->mod();
        m.order.erase(m.order.begin() + p);
        if (m.loop > p || m.loop >= int(m.order.size()))
            m.loop = std::max(0, m.loop - 1);
    });
    ed_->setPosition(std::min(p, int(ed_->mod().order.size()) - 1));
}

void MainWindow::clonePattern()
{
    Module &m = ed_->mod();
    std::vector<bool> used(MAX_PATTERNS);
    for (int p : m.order)
        used[size_t(p)] = true;
    int free = -1;
    for (int i = 0; i < MAX_PATTERNS && free < 0; i++)
        if (!used[size_t(i)] && m.patterns[size_t(i)].empty())
            free = i;
    if (free < 0) {
        message(tr("No queda ningún patrón libre."));
        return;
    }
    ed_->begin(Editor::Everything);
    m.patterns[size_t(free)] = m.patterns[size_t(ed_->pattern())];
    m.order[size_t(ed_->position())] = free;
    ed_->changed(Editor::Everything);
    message(tr("El patrón %1, copiado en el %2").arg(ed_->pattern()).arg(free));
}

void MainWindow::setLoop()
{
    int p = ed_->position();
    editHeader([this, p] { ed_->mod().loop = p; });
}

void MainWindow::setPatternRows(int rows)
{
    ed_->begin(Editor::PatternPart, ed_->pattern());
    ed_->pat().rows.resize(size_t(std::clamp(rows, 1, MAX_ROWS)));
    ed_->changed(Editor::PatternPart);
}

/* --- Samples y ornamentos ----------------------------------------------- */

QWidget *MainWindow::buildSampleTab()
{
    auto *w = new QWidget;
    auto *lay = new QVBoxLayout(w);
    auto *top = new QHBoxLayout;
    sampleNum_ = new QSpinBox;
    sampleNum_->setRange(1, SAMPLES - 1);
    sampleLen_ = new QSpinBox;
    sampleLen_->setRange(0, MAX_LINES);
    sampleLoop_ = new QSpinBox;
    sampleLoop_->setRange(0, MAX_LINES - 1);
    top->addWidget(new QLabel(tr("Sample")));
    top->addWidget(sampleNum_);
    top->addSpacing(16);
    top->addWidget(new QLabel(tr("Líneas")));
    top->addWidget(sampleLen_);
    top->addWidget(new QLabel(tr("Vuelta")));
    top->addWidget(sampleLoop_);
    sampleInfo_ = new QLabel;
    top->addSpacing(16);
    top->addWidget(sampleInfo_, 1);
    lay->addLayout(top);
    sampleView_ = new SampleView(ed_);
    lay->addWidget(sampleView_, 1);
    samplePiano_ = new Piano;
    lay->addWidget(samplePiano_);

    connect(sampleNum_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            ed_->setSample(v);
    });
    connect(sampleLen_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            resizeSample(v);
    });
    connect(sampleLoop_, &QSpinBox::valueChanged, this, [this](int v) {
        if (refreshing_)
            return;
        ed_->begin(Editor::SamplePart, ed_->sample());
        Sample &s = ed_->mod().samples[size_t(ed_->sample())];
        s.loop = std::clamp(v, 0, std::max(0, int(s.lines.size()) - 1));
        ed_->changed(Editor::SamplePart);
    });
    connect(samplePiano_, &Piano::noteOn, this, [this](int n) { previewNote(n, false); });
    connect(samplePiano_, &Piano::noteOff, this, &MainWindow::previewOff);
    return w;
}

QWidget *MainWindow::buildOrnamentTab()
{
    auto *w = new QWidget;
    auto *lay = new QVBoxLayout(w);
    auto *top = new QHBoxLayout;
    ornNum_ = new QSpinBox;
    ornNum_->setRange(0, ORNAMENTS - 1);
    ornLen_ = new QSpinBox;
    ornLen_->setRange(0, MAX_LINES);
    ornLoop_ = new QSpinBox;
    ornLoop_->setRange(0, MAX_LINES - 1);
    top->addWidget(new QLabel(tr("Ornamento")));
    top->addWidget(ornNum_);
    top->addSpacing(16);
    top->addWidget(new QLabel(tr("Líneas")));
    top->addWidget(ornLen_);
    top->addWidget(new QLabel(tr("Vuelta")));
    top->addWidget(ornLoop_);
    ornInfo_ = new QLabel;
    top->addSpacing(16);
    top->addWidget(ornInfo_, 1);
    lay->addLayout(top);
    ornamentView_ = new OrnamentView(ed_);
    lay->addWidget(ornamentView_, 1);
    ornamentPiano_ = new Piano;
    lay->addWidget(ornamentPiano_);

    connect(ornNum_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            ed_->setOrnament(v);
    });
    connect(ornLen_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            resizeOrnament(v);
    });
    connect(ornLoop_, &QSpinBox::valueChanged, this, [this](int v) {
        if (refreshing_)
            return;
        ed_->begin(Editor::OrnamentPart, ed_->ornament());
        Ornament &o = ed_->mod().ornaments[size_t(ed_->ornament())];
        o.loop = std::clamp(v, 0, std::max(0, int(o.offsets.size()) - 1));
        ed_->changed(Editor::OrnamentPart);
    });
    connect(ornamentPiano_, &Piano::noteOn, this, [this](int n) { previewNote(n, true); });
    connect(ornamentPiano_, &Piano::noteOff, this, &MainWindow::previewOff);
    return w;
}

void MainWindow::resizeSample(int lines)
{
    ed_->begin(Editor::SamplePart, ed_->sample());
    Sample &s = ed_->mod().samples[size_t(ed_->sample())];
    SampleLine last = s.lines.empty() ? defaultSample().lines[0] : s.lines.back();
    s.lines.resize(size_t(std::clamp(lines, 0, MAX_LINES)), last);
    s.loop = std::clamp(s.loop, 0, std::max(0, int(s.lines.size()) - 1));
    ed_->changed(Editor::SamplePart);
}

void MainWindow::resizeOrnament(int lines)
{
    ed_->begin(Editor::OrnamentPart, ed_->ornament());
    Ornament &o = ed_->mod().ornaments[size_t(ed_->ornament())];
    o.offsets.resize(size_t(std::clamp(lines, 0, MAX_LINES)), 0);
    o.loop = std::clamp(o.loop, 0, std::max(0, int(o.offsets.size()) - 1));
    ed_->changed(Editor::OrnamentPart);
}

void MainWindow::refreshSample()
{
    refreshing_ = true;
    const Sample &s = ed_->mod().samples[size_t(ed_->sample())];
    sampleNum_->setValue(ed_->sample());
    sample_->setValue(ed_->sample());
    sampleLen_->setValue(int(s.lines.size()));
    sampleLoop_->setValue(s.loop);
    sampleLoop_->setEnabled(!s.lines.empty());
    sampleInfo_->setText(s.lines.empty() ? tr("Este sample no tiene nada: dale líneas (o pulsa Insert).")
                                         : tr("Toca con el piano de abajo o con el teclado al pulsarlo."));
    refreshing_ = false;
    sampleView_->update();
}

void MainWindow::refreshOrnament()
{
    refreshing_ = true;
    const Ornament &o = ed_->mod().ornaments[size_t(ed_->ornament())];
    ornNum_->setValue(ed_->ornament());
    ornament_->setValue(ed_->ornament());
    ornLen_->setValue(int(o.offsets.size()));
    ornLoop_->setValue(o.loop);
    ornLoop_->setEnabled(!o.offsets.empty());
    ornInfo_->setText(o.offsets.empty() ? tr("Este ornamento no tiene nada: dale líneas (o pulsa Insert).")
                                        : tr("Suena con el sample %1.").arg(ed_->sample()));
    refreshing_ = false;
    ornamentView_->update();
}

/* --- Los efectos AYFX ------------------------------------------------------ */

QWidget *MainWindow::buildEffectsTab()
{
    auto *w = new QWidget;
    auto *lay = new QHBoxLayout(w);
    auto *side = new QVBoxLayout;
    auto *files = new QGridLayout;
    auto fileButton = [&](const QString &text, const QString &tip, const std::function<void()> &fn) {
        auto *b = new QPushButton(text);
        b->setToolTip(tip);
        connect(b, &QPushButton::clicked, this, fn);
        int n = files->count();
        files->addWidget(b, n / 2, n % 2);
    };
    fileButton(tr("Nuevo"), tr("Un banco de efectos nuevo"), [this] {
        if (maybeSaveBank())
            newBank();
    });
    fileButton(tr("Abrir..."), tr("Un banco (.afb) o un efecto suelto (.afx)"), [this] {
        if (!maybeSaveBank())
            return;
        QString path = QFileDialog::getOpenFileName(this, tr("Abrir efectos"), settings_->value("files/sfxdir").toString(),
                                                    tr("Efectos AYFX (*.afb *.afx);;Todos los ficheros (*)"));
        if (!path.isEmpty()) {
            settings_->setValue("files/sfxdir", QFileInfo(path).absolutePath());
            openBank(path);
        }
    });
    fileButton(tr("Guardar"), tr("El banco, en su fichero .afb"), [this] {
        ed_->bankPath().isEmpty() ? saveBankAs() : saveBank(ed_->bankPath());
    });
    fileButton(tr("Guardar como..."), QString(), [this] { saveBankAs(); });
    side->addLayout(files);
    effects_ = new QListWidget;
    effects_->setToolTip(tr("Los efectos del banco: doble clic para cambiar el nombre, Intro para oírlo"));
    side->addWidget(effects_, 1);
    auto *eb = new QHBoxLayout;
    auto effButton = [&](const QString &text, const QString &tip, const std::function<void()> &fn) {
        auto *b = new QToolButton;
        b->setText(text);
        b->setToolTip(tip);
        connect(b, &QToolButton::clicked, this, fn);
        eb->addWidget(b);
    };
    effButton("+", tr("Un efecto nuevo"), [this] {
        if (int(ed_->bank().effects.size()) >= fx::MAX_EFFECTS)
            return;
        ed_->beginBank();
        fx::Effect e;
        e.name = tr("efecto %1").arg(ed_->bank().effects.size()).toStdString();
        for (int i = 0; i < 8; i++) {
            fx::Frame f;
            f.volume = 15 - i * 2;
            e.frames.push_back(f);
        }
        ed_->bank().effects.push_back(e);
        ed_->endBank();
        ed_->setEffect(int(ed_->bank().effects.size()) - 1);
    });
    effButton(tr("Copia"), tr("Una copia de este efecto, al final"), [this] {
        if (int(ed_->bank().effects.size()) >= fx::MAX_EFFECTS)
            return;
        ed_->beginBank();
        fx::Effect e = ed_->bank().effects[size_t(ed_->effect())];
        ed_->bank().effects.push_back(e);
        ed_->endBank();
        ed_->setEffect(int(ed_->bank().effects.size()) - 1);
    });
    effButton("-", tr("Quitar este efecto"), [this] {
        if (ed_->bank().effects.size() <= 1)
            return;
        ed_->beginBank();
        ed_->bank().effects.erase(ed_->bank().effects.begin() + ed_->effect());
        ed_->endBank();
    });
    effButton(tr("Subir"), tr("Un puesto antes (el número cambia)"), [this] {
        int e = ed_->effect();
        if (e <= 0)
            return;
        ed_->beginBank();
        std::swap(ed_->bank().effects[size_t(e)], ed_->bank().effects[size_t(e - 1)]);
        ed_->endBank();
        ed_->setEffect(e - 1);
    });
    effButton(tr("Bajar"), tr("Un puesto después"), [this] {
        int e = ed_->effect();
        if (e + 1 >= int(ed_->bank().effects.size()))
            return;
        ed_->beginBank();
        std::swap(ed_->bank().effects[size_t(e)], ed_->bank().effects[size_t(e + 1)]);
        ed_->endBank();
        ed_->setEffect(e + 1);
    });
    side->addLayout(eb);
    auto *io = new QHBoxLayout;
    auto *imp = new QPushButton(tr("Importar .afx..."));
    auto *exp = new QPushButton(tr("Exportar .afx..."));
    connect(imp, &QPushButton::clicked, this, &MainWindow::importAfx);
    connect(exp, &QPushButton::clicked, this, &MainWindow::exportAfx);
    io->addWidget(imp);
    io->addWidget(exp);
    side->addLayout(io);
    auto *sideW = new QWidget;
    sideW->setLayout(side);
    sideW->setMaximumWidth(320);
    lay->addWidget(sideW);

    auto *center = new QVBoxLayout;
    auto *bar = new QHBoxLayout;
    auto *playB = new QPushButton(tr("Tocar (Intro)"));
    connect(playB, &QPushButton::clicked, this, &MainWindow::playEffect);
    bar->addWidget(playB);
    sfxInfo_ = new QLabel;
    bar->addWidget(sfxInfo_, 1);
    center->addLayout(bar);
    sfxView_ = new SfxView(ed_);
    center->addWidget(sfxView_, 1);
    lay->addLayout(center, 1);

    connect(effects_, &QListWidget::currentRowChanged, this, [this](int r) {
        if (!refreshing_ && r >= 0)
            ed_->setEffect(r);
    });
    connect(effects_, &QListWidget::itemChanged, this, [this](QListWidgetItem *it) {
        if (refreshing_)
            return;
        int r = effects_->row(it);
        QString name = it->text().section(':', 1).trimmed();
        if (r < 0 || name.toStdString() == ed_->bank().effects[size_t(r)].name)
            return;
        ed_->beginBank();
        ed_->bank().effects[size_t(r)].name = name.toLatin1().left(32).toStdString();
        ed_->endBank();
    });
    connect(effects_, &QListWidget::itemActivated, this, [this] { playEffect(); });
    connect(sfxView_, &LineGrid::activated, this, &MainWindow::playEffect);
    return w;
}

void MainWindow::refreshEffects()
{
    refreshing_ = true;
    const fx::Bank &b = ed_->bank();
    if (effects_->count() != int(b.effects.size())) {
        effects_->clear();
        for (size_t i = 0; i < b.effects.size(); i++) {
            auto *it = new QListWidgetItem;
            it->setFlags(it->flags() | Qt::ItemIsEditable);
            effects_->addItem(it);
        }
    }
    for (int i = 0; i < int(b.effects.size()); i++)
        effects_->item(i)->setText(QStringLiteral("%1: %2").arg(i, 3, 10, QChar('0')).arg(
            QString::fromLatin1(b.effects[size_t(i)].name.c_str())));
    effects_->setCurrentRow(ed_->effect());
    const fx::Effect &e = b.effects[size_t(ed_->effect())];
    sfxInfo_->setText(tr("%1 frames, %2 s, %3 bytes")
                          .arg(e.frames.size())
                          .arg(double(e.frames.size()) / 50.0, 0, 'f', 2)
                          .arg(fx::compile(e).size()));
    refreshing_ = false;
    sfxView_->update();
}

void MainWindow::playEffect()
{
    const fx::Bank &b = ed_->bank();
    if (b.effects.empty())
        return;
    engine_.playSfx(fx::compile(b.effects[size_t(ed_->effect())]), 0);
}

/* --- Menús y barra --------------------------------------------------------- */

void MainWindow::buildActions()
{
    QMenu *file = menuBar()->addMenu(tr("&Archivo"));
    file->addAction(action(tr("&Nuevo"), QKeySequence::New, [this] {
        if (maybeSave())
            newSong();
    }));
    file->addAction(action(tr("&Abrir..."), QKeySequence::Open, [this] { openDialog(); }));
    recentMenu_ = file->addMenu(tr("Abrir &reciente"));
    file->addAction(action(tr("&Guardar"), QKeySequence::Save, [this] {
        ed_->path().isEmpty() ? saveAs() : save(ed_->path());
    }));
    file->addAction(action(tr("Guardar &como..."), QKeySequence::SaveAs, [this] { saveAs(); }));
    file->addSeparator();
    file->addAction(action(tr("&Exportar como WAV..."), QKeySequence(), [this] { exportWavDialog(); }));
    file->addSeparator();
    file->addAction(action(tr("&Salir"), QKeySequence::Quit, [this] { close(); }));

    QMenu *edit = menuBar()->addMenu(tr("&Editar"));
    undo_ = action(tr("&Deshacer"), QKeySequence::Undo, [this] {
        tabs_->currentIndex() == Effects ? ed_->undoBank() : ed_->undo();
    });
    redo_ = action(tr("&Rehacer"), QKeySequence::Redo, [this] {
        tabs_->currentIndex() == Effects ? ed_->redoBank() : ed_->redo();
    });
    edit->addAction(undo_);
    edit->addAction(redo_);
    edit->addSeparator();
    cut_ = action(tr("Cor&tar"), QKeySequence::Cut, [this] { pattern_->cut(); });
    copy_ = action(tr("&Copiar"), QKeySequence::Copy, [this] { pattern_->copy(); });
    paste_ = action(tr("&Pegar"), QKeySequence::Paste, [this] { pattern_->paste(); });
    selectAll_ = action(tr("Seleccionar &todo"), QKeySequence::SelectAll, [this] { pattern_->selectAll(); });
    edit->addActions({ cut_, copy_, paste_, selectAll_ });
    edit->addSeparator();
    QAction *up = action(tr("Subir un semitono"), QKeySequence(Qt::CTRL | Qt::Key_Up), [this] { pattern_->transpose(1); });
    QAction *down = action(tr("Bajar un semitono"), QKeySequence(Qt::CTRL | Qt::Key_Down), [this] { pattern_->transpose(-1); });
    QAction *up12 = action(tr("Subir una octava"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up),
                           [this] { pattern_->transpose(12); });
    QAction *down12 = action(tr("Bajar una octava"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down),
                             [this] { pattern_->transpose(-12); });
    edit->addActions({ up, down, up12, down12 });
    edit->addSeparator();
    QAction *ins = action(tr("Insertar una fila (Insert)"), QKeySequence(), [this] { pattern_->insertRow(); });
    QAction *del = action(tr("Quitar la fila (Retroceso)"), QKeySequence(), [this] { pattern_->deleteRow(); });
    edit->addActions({ ins, del });
    patternOnly_ = { cut_, copy_, paste_, selectAll_, up, down, up12, down12, ins, del };

    QMenu *playMenu = menuBar()->addMenu(tr("&Tocar"));
    QAction *song = action(tr("La &canción"), QKeySequence(Qt::Key_F5), [this] { play(0, 0, false); });
    QAction *pat = action(tr("El &patrón"), QKeySequence(Qt::Key_F6), [this] { play(ed_->position(), 0, true); });
    QAction *from = action(tr("Desde el &cursor"), QKeySequence(Qt::Key_F7),
                           [this] { play(ed_->position(), pattern_->row(), false); });
    QAction *stopA = action(tr("&Parar"), QKeySequence(Qt::Key_F8), [this] { stop(); });
    follow_ = action(tr("&Seguir lo que suena"), QKeySequence(Qt::Key_ScrollLock), [] {});
    follow_->setCheckable(true);
    playMenu->addActions({ song, pat, from, stopA });
    playMenu->addSeparator();
    playMenu->addAction(follow_);
    QAction *octUp = action(tr("Octava más alta"), QKeySequence(Qt::KeypadModifier | Qt::Key_Asterisk),
                            [this] { octave_->setValue(octave_->value() + 1); });
    QAction *octDown = action(tr("Octava más baja"), QKeySequence(Qt::KeypadModifier | Qt::Key_Slash),
                              [this] { octave_->setValue(octave_->value() - 1); });
    playMenu->addSeparator();
    playMenu->addActions({ octUp, octDown });

    QMenu *help = menuBar()->addMenu(tr("A&yuda"));
    help->addAction(action(tr("&Teclas"), QKeySequence(Qt::Key_F1), [this] { keysHelp(); }));
    help->addAction(action(tr("&Acerca de zxv-tracker"), QKeySequence(), [this] { about(); }));

    QToolBar *tb = addToolBar(tr("Tocar"));
    tb->setObjectName("play");
    tb->addActions({ song, pat, from, stopA });
    tb->addAction(follow_);
    tb->addSeparator();
    auto spin = [&](const QString &label, int lo, int hi, const QString &tip) {
        auto *s = new QSpinBox;
        s->setRange(lo, hi);
        s->setToolTip(tip);
        s->setFocusPolicy(Qt::ClickFocus);
        tb->addWidget(new QLabel(QStringLiteral(" %1 ").arg(label)));
        tb->addWidget(s);
        return s;
    };
    octave_ = spin(tr("Octava"), 1, 8, tr("La octava de las notas que se escriben (* y / del teclado numérico)"));
    step_ = spin(tr("Paso"), 0, 16, tr("Las filas que baja el cursor al escribir"));
    sample_ = spin(tr("Sample"), 1, SAMPLES - 1, tr("El sample de las notas que se escriben"));
    ornament_ = spin(tr("Ornamento"), 0, ORNAMENTS - 1, tr("El ornamento que se edita (y el que se oye en su pestaña)"));
    connect(octave_, &QSpinBox::valueChanged, this, [this](int v) {
        pattern_->setOctave(v);
        samplePiano_->setOctave(v);
        ornamentPiano_->setOctave(v);
        settings_->setValue("edit/octave", v);
    });
    connect(step_, &QSpinBox::valueChanged, this, [this](int v) {
        pattern_->setStep(v);
        settings_->setValue("edit/step", v);
    });
    connect(sample_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            ed_->setSample(v);
    });
    connect(ornament_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            ed_->setOrnament(v);
    });
    connect(follow_, &QAction::toggled, this, [this](bool on) { settings_->setValue("edit/follow", on); });
    tb->addSeparator();
    tb->addWidget(new QLabel(tr(" Silenciar ")));
    for (int i = 0; i < 3; i++) {
        mute_[i] = new QAction(QString(QChar('A' + i)), this);
        mute_[i]->setCheckable(true);
        mute_[i]->setToolTip(tr("Silenciar el canal %1").arg(QChar('A' + i)));
        connect(mute_[i], &QAction::toggled, this, [this] { updateMute(); });
        tb->addAction(mute_[i]);
    }
}

void MainWindow::updateMute()
{
    int mask = 0;
    for (int i = 0; i < 3; i++)
        if (mute_[i]->isChecked())
            mask |= 1 << (ed_->module() * 3 + i);
    engine_.setMute(mask);
}

void MainWindow::updateState()
{
    QString name = ed_->path().isEmpty() ? tr("Sin nombre") : QFileInfo(ed_->path()).fileName();
    setWindowTitle(tr("%1%2 - Tracker de ZX-V").arg(name, ed_->modified() ? "*" : ""));
    bool sfx = tabs_->currentIndex() == Effects;
    undo_->setEnabled(sfx ? ed_->canUndoBank() : ed_->canUndo());
    redo_->setEnabled(sfx ? ed_->canRedoBank() : ed_->canRedo());
    for (QAction *a : patternOnly_)
        a->setEnabled(tabs_->currentIndex() == Patterns);
    size_->setText(tr("PT3: %1 bytes").arg(trk::save(ed_->song()).size()));
    tabs_->setTabText(Effects, tr("Efectos AYFX") + (ed_->bankModified() ? "*" : ""));
}

void MainWindow::message(const QString &text)
{
    statusBar()->showMessage(text, 5000);
}

/* --- Tocar ---------------------------------------------------------------- */

void MainWindow::play(int pos, int row, bool patternOnly)
{
    Engine::Play p;
    if (patternOnly) {
        Song s = ed_->song();
        for (Module &m : s.modules) {
            int pat = pos < int(m.order.size()) ? m.order[size_t(pos)] : 0;
            m.order = { pat };
            m.loop = 0;
        }
        p.data = trk::save(s);
        for (const Module &m : s.modules)
            p.rows.push_back({ int(m.patterns[size_t(m.order[0])].rows.size()) });
        p.pos = 0;
    } else {
        p.data = trk::save(ed_->song());
        p.rows = ed_->rows();
        p.pos = pos;
    }
    p.row = row;
    engine_.stopPreview();
    playing_ = engine_.play(p);
}

void MainWindow::stop()
{
    engine_.stop();
    engine_.stopSfx();
    engine_.stopPreview();
    playing_ = false;
    pattern_->setPlayRow(-1);
}

void MainWindow::tick()
{
    Engine::Status s = engine_.status();
    if (!s.playing) {
        if (playing_) {
            playing_ = false;
            pattern_->setPlayRow(-1);
        }
        where_->setText(tr("Posición %1, fila %2").arg(ed_->position()).arg(pattern_->row()));
        return;
    }
    int m = std::min(ed_->module(), s.chips - 1);
    int pos = s.pos[m], row = std::max(0, s.row[m]);
    where_->setText(tr("Suena: posición %1, fila %2").arg(pos).arg(row));
    if (follow_->isChecked() && s.row[m] >= 0) {
        if (ed_->mod().order.size() > size_t(pos) && pos != ed_->position() && !refreshing_)
            ed_->setPosition(pos);
        pattern_->setRow(row);
    }
    pattern_->setPlayRow(pos == ed_->position() ? row : -1);
}

void MainWindow::previewCell(const Cell &c, int envPeriod, int channel)
{
    if (engine_.status().playing)
        return;
    engine_.preview(previewModule(ed_->mod(), c, envPeriod, channel), ed_->module());
}

void MainWindow::previewNote(int note, bool withOrnament)
{
    Cell c;
    c.note = note;
    c.sample = ed_->sample();
    if (withOrnament)
        c.orn = ed_->ornament();
    engine_.preview(previewModule(ed_->mod(), c, -1, 0), ed_->module());
}

void MainWindow::previewOff()
{
    engine_.stopPreview();
}

/* --- Ficheros -------------------------------------------------------------- */

void MainWindow::newSong()
{
    stop();
    Song s;
    ed_->setSong(s, QString());
    ed_->setSample(1);
    ed_->setOrnament(0);
}

bool MainWindow::open(const QString &path)
{
    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "afb" || ext == "afx")
        return openBank(path);
    bool ok;
    QByteArray data = readAll(path, &ok);
    if (!ok) {
        QMessageBox::warning(this, tr("Abrir"), tr("No se puede abrir %1.").arg(QDir::toNativeSeparators(path)));
        return false;
    }
    std::vector<uint8_t> bytes(data.begin(), data.end());
    Song s;
    int r = load(bytes, s);
    if (r != LOAD_OK) {
        QString why = r == LOAD_NOT_PT3 ? tr("no es un módulo PT3 (Pro Tracker 3 o Vortex Tracker II)")
                                        : tr("está cortado o dañado");
        QMessageBox::warning(this, tr("Abrir"), tr("%1: %2.").arg(QFileInfo(path).fileName(), why));
        return false;
    }
    stop();
    ed_->setSong(s, QFileInfo(path).absoluteFilePath());
    addRecent(ed_->path());
    message(tr("Abierto %1").arg(QDir::toNativeSeparators(path)));
    /* Si tiene órdenes que el editor no sabe poner (dos efectos en una fila,
     * una envolvente que se quita en la misma fila...), guardado no sonaría
     * igual: se avisa. */
    long steps = songSteps(bytes, 200000) * 2 + 200;
    long diff = compareSound(bytes, trk::save(s), steps);
    if (diff >= 0) {
        auto *box = new QMessageBox(QMessageBox::Information, tr("Abrir"),
                                    tr("%1 tiene órdenes que este editor no tiene (como dos efectos en la misma fila). "
                                       "Se puede editar, pero al guardarlo sonará distinto desde el segundo %2.")
                                        .arg(QFileInfo(path).fileName())
                                        .arg(double(diff) / 50.0, 0, 'f', 1),
                                    QMessageBox::Ok, this);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
    }
    return true;
}

void MainWindow::openDialog()
{
    if (!maybeSave())
        return;
    QString path = QFileDialog::getOpenFileName(this, tr("Abrir"), settings_->value("files/dir").toString(),
                                                tr("Módulos PT3 (*.pt3);;Efectos AYFX (*.afb *.afx);;Todos los ficheros (*)"));
    if (path.isEmpty())
        return;
    settings_->setValue("files/dir", QFileInfo(path).absolutePath());
    open(path);
}

bool MainWindow::save(const QString &path)
{
    std::vector<uint8_t> data = trk::save(ed_->song());
    if (!writeAll(path, data)) {
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
    QString path = QFileDialog::getSaveFileName(this, tr("Guardar como"), dir, tr("Módulos PT3 (*.pt3)"));
    if (path.isEmpty())
        return false;
    if (QFileInfo(path).suffix().isEmpty())
        path += ".pt3";
    settings_->setValue("files/dir", QFileInfo(path).absolutePath());
    return save(path);
}

bool MainWindow::maybeSave()
{
    if (!ed_->modified())
        return true;
    auto r = QMessageBox::question(this, tr("Cambios sin guardar"), tr("¿Guardar los cambios del módulo?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Save)
        return ed_->path().isEmpty() ? saveAs() : save(ed_->path());
    return r == QMessageBox::Discard;
}

bool MainWindow::exportWav(const QString &path)
{
    Engine e;
    Engine::Play p;
    p.data = trk::save(ed_->song());
    p.rows = ed_->rows();
    p.loop = false;
    if (!e.play(p))
        return false;
    std::vector<int16_t> pcm;
    std::vector<int16_t> buf(size_t(Engine::STEP) * 2);
    for (long i = 0; i < 50L * 60 * 30 && e.status().playing; i++) {
        e.render(buf.data(), Engine::STEP);
        pcm.insert(pcm.end(), buf.begin(), buf.end());
    }
    std::vector<uint8_t> wav;
    auto u32 = [&](uint32_t v) {
        for (int i = 0; i < 4; i++)
            wav.push_back(uint8_t(v >> (8 * i)));
    };
    auto u16 = [&](uint16_t v) {
        wav.push_back(uint8_t(v));
        wav.push_back(uint8_t(v >> 8));
    };
    uint32_t bytes = uint32_t(pcm.size() * 2);
    wav.insert(wav.end(), { 'R', 'I', 'F', 'F' });
    u32(36 + bytes);
    wav.insert(wav.end(), { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
    u32(16);
    u16(1);
    u16(2);
    u32(Engine::RATE);
    u32(Engine::RATE * 4);
    u16(4);
    u16(16);
    wav.insert(wav.end(), { 'd', 'a', 't', 'a' });
    u32(bytes);
    for (int16_t s : pcm)
        u16(uint16_t(s));
    return writeAll(path, wav);
}

void MainWindow::exportWavDialog()
{
    QString base = ed_->path().isEmpty() ? settings_->value("files/dir").toString()
                                         : QFileInfo(ed_->path()).path() + "/" + QFileInfo(ed_->path()).completeBaseName() + ".wav";
    QString path = QFileDialog::getSaveFileName(this, tr("Exportar como WAV"), base, tr("Sonido WAV (*.wav)"));
    if (path.isEmpty())
        return;
    if (QFileInfo(path).suffix().isEmpty())
        path += ".wav";
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool ok = exportWav(path);
    QApplication::restoreOverrideCursor();
    if (!ok)
        QMessageBox::warning(this, tr("Exportar"), tr("No se ha podido guardar %1.").arg(QDir::toNativeSeparators(path)));
    else
        message(tr("Exportado %1").arg(QDir::toNativeSeparators(path)));
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
            bool sfx = p.endsWith(".afb", Qt::CaseInsensitive) || p.endsWith(".afx", Qt::CaseInsensitive);
            if (sfx ? maybeSaveBank() : maybeSave())
                open(p);
        });
    recentMenu_->setEnabled(!list.isEmpty());
}

/* Los efectos tienen su propio fichero. */

void MainWindow::newBank()
{
    fx::Bank b;
    fx::Effect e;
    e.name = tr("efecto 0").toStdString();
    for (int i = 0; i < 8; i++) {
        fx::Frame f;
        f.volume = 15 - i * 2;
        f.tone = 0x100 + i * 0x20;
        e.frames.push_back(f);
    }
    b.effects.push_back(e);
    ed_->setBank(b, QString());
}

bool MainWindow::openBank(const QString &path)
{
    bool ok;
    QByteArray data = readAll(path, &ok);
    std::vector<uint8_t> bytes(data.begin(), data.end());
    fx::Bank b;
    if (ok && path.endsWith(".afx", Qt::CaseInsensitive)) {
        fx::Effect e;
        e.name = QFileInfo(path).completeBaseName().toLatin1().left(32).toStdString();
        ok = fx::decompile(bytes, e);
        b.effects.push_back(e);
    } else if (ok) {
        ok = fx::loadBank(bytes, b);
    }
    if (!ok) {
        QMessageBox::warning(this, tr("Abrir efectos"),
                             tr("%1 no es un banco de efectos AYFX.").arg(QDir::toNativeSeparators(path)));
        return false;
    }
    bool afx = path.endsWith(".afx", Qt::CaseInsensitive);
    ed_->setBank(b, afx ? QString() : QFileInfo(path).absoluteFilePath());
    addRecent(QFileInfo(path).absoluteFilePath());
    showTab(Effects);
    message(tr("Abierto %1 (%2 efectos)").arg(QDir::toNativeSeparators(path)).arg(b.effects.size()));
    return true;
}

bool MainWindow::saveBank(const QString &path)
{
    std::vector<uint8_t> data = fx::saveBank(ed_->bank());
    if (!writeAll(path, data)) {
        QMessageBox::warning(this, tr("Guardar"), tr("No se ha podido guardar %1.").arg(QDir::toNativeSeparators(path)));
        return false;
    }
    ed_->setBankSaved(QFileInfo(path).absoluteFilePath());
    addRecent(ed_->bankPath());
    message(tr("Guardado %1 (%2 bytes)").arg(QDir::toNativeSeparators(path)).arg(data.size()));
    return true;
}

bool MainWindow::saveBankAs()
{
    QString dir = ed_->bankPath().isEmpty() ? settings_->value("files/sfxdir").toString() : ed_->bankPath();
    QString path = QFileDialog::getSaveFileName(this, tr("Guardar los efectos"), dir, tr("Bancos de efectos AYFX (*.afb)"));
    if (path.isEmpty())
        return false;
    if (QFileInfo(path).suffix().isEmpty())
        path += ".afb";
    settings_->setValue("files/sfxdir", QFileInfo(path).absolutePath());
    return saveBank(path);
}

bool MainWindow::maybeSaveBank()
{
    if (!ed_->bankModified())
        return true;
    auto r = QMessageBox::question(this, tr("Cambios sin guardar"), tr("¿Guardar los cambios de los efectos?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Save)
        return ed_->bankPath().isEmpty() ? saveBankAs() : saveBank(ed_->bankPath());
    return r == QMessageBox::Discard;
}

void MainWindow::importAfx()
{
    QStringList paths = QFileDialog::getOpenFileNames(this, tr("Importar efectos"), settings_->value("files/sfxdir").toString(),
                                                      tr("Efectos AYFX (*.afx)"));
    if (paths.isEmpty())
        return;
    ed_->beginBank();
    int added = 0;
    for (const QString &path : paths) {
        bool ok;
        QByteArray data = readAll(path, &ok);
        fx::Effect e;
        e.name = QFileInfo(path).completeBaseName().toLatin1().left(32).toStdString();
        if (ok && int(ed_->bank().effects.size()) < fx::MAX_EFFECTS &&
            fx::decompile(std::vector<uint8_t>(data.begin(), data.end()), e)) {
            ed_->bank().effects.push_back(e);
            added++;
        }
    }
    ed_->endBank();
    ed_->setEffect(int(ed_->bank().effects.size()) - 1);
    message(tr("%n efecto(s) importado(s)", nullptr, added));
}

void MainWindow::exportAfx()
{
    const fx::Effect &e = ed_->bank().effects[size_t(ed_->effect())];
    QString name = QString::fromLatin1(e.name.c_str()).trimmed();
    QString dir = settings_->value("files/sfxdir").toString() + "/" + (name.isEmpty() ? QString("efecto") : name) + ".afx";
    QString path = QFileDialog::getSaveFileName(this, tr("Exportar el efecto"), dir, tr("Efectos AYFX (*.afx)"));
    if (path.isEmpty())
        return;
    if (QFileInfo(path).suffix().isEmpty())
        path += ".afx";
    if (!writeAll(path, fx::compile(e)))
        QMessageBox::warning(this, tr("Exportar"), tr("No se ha podido guardar %1.").arg(QDir::toNativeSeparators(path)));
    else
        message(tr("Exportado %1").arg(QDir::toNativeSeparators(path)));
}

/* --- La ventana ------------------------------------------------------------ */

void MainWindow::showTab(int tab)
{
    tabs_->setCurrentIndex(std::clamp(tab, 0, 3));
}

void MainWindow::keysHelp()
{
    QMessageBox::information(
        this, tr("Teclas"),
        tr("<p><b>Notas</b> (en la columna de la nota): Z S X D C V G B H N J M es una octava y "
           "Q 2 W 3 E R 5 T 6 Y 7 U la siguiente; 1 o A, el silencio (R--). La octava, con * y / del teclado "
           "numérico.</p>"
           "<p><b>Lo demás</b>, en hexadecimal: el sample (1-9 y A-V), la envolvente (1-E su forma, F sin ella), "
           "el ornamento, el volumen y el efecto: 1 y 2 el tono baja y sube, 3 portamento, 4 y 5 la posición en el "
           "sample y en el ornamento, 6 vibrato, 9 y A la envolvente baja y sube, B el tempo.</p>"
           "<p><b>Moverse</b>: flechas, Re Pág y Av Pág, Inicio y Fin; el tabulador, de canal en canal. "
           "Con mayúsculas, se selecciona. Supr borra, Insert mete una fila y Retroceso la quita.</p>"
           "<p><b>Tocar</b>: F5 la canción, F6 el patrón, F7 desde el cursor; F8 o Esc paran.</p>"));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("Acerca de zxv-tracker"),
                       tr("<p><b>Tracker de ZX-V</b> %1</p>"
                          "<p>Música para los dos AY de ZX-V en módulos PT3 (los de Pro Tracker 3 y Vortex "
                          "Tracker II) y efectos AYFX: los que toca la biblioteca de música del SDK "
                          "(zxv_music.h). Suena con el mismo código que la máquina.</p>"
                          "<p>Software libre con la licencia MIT: el código está en el SDK de ZX-V "
                          "(sdk/tools/tracker), en <a href=\"https://github.com/cronomantic/ZX-V\">"
                          "github.com/cronomantic/ZX-V</a>.</p>")
                           .arg(QStringLiteral(ZXV_VERSION)));
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (!maybeSave() || !maybeSaveBank()) {
        e->ignore();
        return;
    }
    stop();
    settings_->setValue("window/geometry", saveGeometry());
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
    bool sfx = path.endsWith(".afb", Qt::CaseInsensitive) || path.endsWith(".afx", Qt::CaseInsensitive);
    if (sfx ? maybeSaveBank() : maybeSave())
        open(path);
}
