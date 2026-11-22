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
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

#include "audio.h"
#include "channels.h"
#include "editor.h"
#include "ayresult.h"
#include "lines.h"
#include "patternview.h"
#include "piano.h"
#include "song.h"
#include "instrview.h"
#include "waveview.h"
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

/* El módulo del PT3 de un AY (sus samples, ornamentos, versión y tabla):
 * para oír una nota con previewModule. */
static Module bankModule(const AyBank &b)
{
    Module m;
    m.samples = b.samples;
    m.ornaments = b.ornaments;
    m.version = b.version;
    m.table = b.table;
    return m;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ed_(new Editor(this)), settings_(makeSettings(this)), audio_(new Audio(&engine_))
{
    setAcceptDrops(true);
    tabs_ = new QTabWidget;
    tabs_->addTab(buildPatternTab(), tr("Patrones"));
    tabs_->addTab(buildSampleTab(), tr("Instrumentos AY"));
    tabs_->addTab(buildOrnamentTab(), tr("Ornamentos"));
    tabs_->addTab(buildWaveTab(), tr("Muestras"));
    tabs_->addTab(buildSmpInsTab(), tr("Instrumentos de muestras"));
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
        refreshWaves();
        refreshSmpIns();
        updateState();
    });
    connect(ed_, &Editor::positionChanged, this, [this] {
        refreshHeader();
        refreshPositions();
    });
    connect(ed_, &Editor::sampleChanged, this, &MainWindow::refreshSample);
    connect(ed_, &Editor::ornamentChanged, this, &MainWindow::refreshOrnament);
    connect(ed_, &Editor::bankChanged, this, &MainWindow::refreshEffects);
    connect(ed_, &Editor::effectChanged, this, &MainWindow::refreshEffects);
    connect(ed_, &Editor::stateChanged, this, &MainWindow::updateState);
    connect(tabs_, &QTabWidget::currentChanged, this, [this] {
        engine_.stopPreview();
        updateState();
        QWidget *focus[] = { pattern_, sampleView_, ornamentView_, waves_, smpIns_, sfxView_ };
        focus[std::clamp(tabs_->currentIndex(), 0, 5)]->setFocus();
    });

    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::tick);
    timer->start(25);

    octave_->setValue(settings_->value("edit/octave", 4).toInt());
    step_->setValue(settings_->value("edit/step", 1).toInt());
    follow_->setChecked(settings_->value("edit/follow", true).toBool());
    restoreGeometry(settings_->value("window/geometry").toByteArray());
    if (settings_->value("window/geometry").isNull())
        resize(1200, 760);

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
    auto *head = new QGroupBox(tr("Canción"));
    auto *form = new QFormLayout(head);
    title_ = new QLineEdit;
    title_->setMaxLength(32);
    author_ = new QLineEdit;
    author_->setMaxLength(32);
    speed_ = new QSpinBox;
    speed_->setRange(1, 255);
    speed_->setToolTip(tr("Pasos por fila: más, más lento (el «tempo» de Vortex Tracker)"));
    tempo_ = new QSpinBox;
    tempo_->setRange(32, 255);
    tempo_->setToolTip(tr("Los pasos por segundo: 125 son 50, como en el PT3 (que no tiene otro)"));
    chips_ = new QComboBox;
    chips_->addItems({ QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("2") });
    chips_->setToolTip(tr("Los AY que usa la canción: ninguno, uno, o los dos (un TurboSound)"));
    smpTracks_ = new QSpinBox;
    smpTracks_->setRange(0, SMP_TRACKS);
    smpTracks_->setToolTip(tr("Las pistas de los canales de muestras (con ellas, la canción solo se guarda en .zxm)"));
    chip_ = new QComboBox;
    chip_->addItems({ tr("AY 1"), tr("AY 2") });
    chip_->setToolTip(tr("El AY cuya tabla y versión se ven aquí, y cuyos samples y ornamentos se editan"));
    table_ = new QComboBox;
    for (int i = 0; i < 4; i++)
        table_->addItem(QString::number(i));
    table_->setToolTip(tr("La tabla de notas de Pro Tracker 3 (la 2 es la de siempre de Vortex Tracker)"));
    version_ = new QComboBox;
    for (int i = 0; i < 10; i++)
        version_->addItem(QStringLiteral("3.%1").arg(i));
    version_->setToolTip(tr("La versión de Pro Tracker 3: cambia algunos efectos y las tablas (Vortex Tracker usa la 3.6)"));
    form->addRow(tr("Título"), title_);
    form->addRow(tr("Autor"), author_);
    form->addRow(tr("Velocidad"), speed_);
    form->addRow(tr("Tempo"), tempo_);
    form->addRow(tr("AY"), chips_);
    form->addRow(tr("Pistas de muestras"), smpTracks_);
    form->addRow(chip_);
    form->addRow(tr("Tabla"), table_);
    form->addRow(tr("Versión"), version_);
    side->addWidget(head);

    connect(title_, &QLineEdit::editingFinished, this, [this] {
        if (title_->text().toStdString() != ed_->tune().title)
            editHeader([this] { ed_->tune().title = title_->text().toLatin1().toStdString(); });
    });
    connect(author_, &QLineEdit::editingFinished, this, [this] {
        if (author_->text().toStdString() != ed_->tune().author)
            editHeader([this] { ed_->tune().author = author_->text().toLatin1().toStdString(); });
    });
    connect(speed_, &QSpinBox::valueChanged, this, [this](int v) { editHeader([this, v] { ed_->tune().speed = v; }); });
    connect(tempo_, &QSpinBox::valueChanged, this, [this](int v) { editHeader([this, v] { ed_->tune().tempo = v; }); });
    connect(table_, &QComboBox::currentIndexChanged, this, [this](int v) { editHeader([this, v] { ed_->bank().table = v; }); });
    connect(version_, &QComboBox::currentIndexChanged, this,
            [this](int v) { editHeader([this, v] { ed_->bank().version = v; }); });
    connect(chips_, &QComboBox::currentIndexChanged, this, [this](int n) {
        if (!refreshing_)
            setChips(n);
    });
    connect(smpTracks_, &QSpinBox::valueChanged, this, [this](int n) {
        if (!refreshing_)
            setSmpTracks(n);
    });
    connect(chip_, &QComboBox::currentIndexChanged, this, [this](int c) {
        if (!refreshing_)
            ed_->setChip(c);
    });

    auto *posBox = new QGroupBox(tr("Posiciones"));
    auto *pv = new QVBoxLayout(posBox);
    positions_ = new QListWidget;
    positions_->setToolTip(tr("El orden: en cada posición, el patrón de cada AY y de cada pista de muestras "
                              "(-- ninguno). L: la posición a la que vuelve al acabar."));
    pv->addWidget(positions_, 1);
    auto *pb = new QHBoxLayout;
    auto button = [&](const QString &text, const QString &tip, const std::function<void()> &fn) {
        auto *b = new QToolButton;
        b->setText(text);
        b->setToolTip(tip);
        connect(b, &QToolButton::clicked, this, fn);
        pb->addWidget(b);
    };
    button("+", tr("Añadir una posición detrás (con los mismos patrones)"), [this] { addPosition(); });
    button("-", tr("Quitar la posición"), [this] { removePosition(); });
    button(tr("Copia"), tr("Una copia del patrón de la pista del cursor, en un patrón libre, en esta posición"),
           [this] { clonePattern(); });
    button("L", tr("Que vuelva aquí al acabar"), [this] { setLoop(); });
    pv->addLayout(pb);
    auto *pf = new QFormLayout;
    patLabel_ = new QLabel;
    patNum_ = new QSpinBox;
    patNum_->setRange(-1, 255);
    patNum_->setSpecialValueText(QStringLiteral("--"));
    patNum_->setToolTip(tr("El patrón de la pista del cursor en esta posición (-- ninguno)"));
    patRows_ = new QSpinBox;
    patRows_->setRange(1, MAX_ROWS);
    patRows_->setToolTip(tr("Las filas de la posición (y de sus patrones)"));
    pf->addRow(patLabel_, patNum_);
    pf->addRow(tr("Filas"), patRows_);
    pv->addLayout(pf);
    side->addWidget(posBox, 1);

    connect(positions_, &QListWidget::currentRowChanged, this, [this](int r) {
        if (!refreshing_ && r >= 0)
            ed_->setPosition(r);
    });
    connect(patNum_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            setGroupPattern(v);
    });
    connect(patRows_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            setPositionRows(v);
    });

    auto *sideW = new QWidget;
    sideW->setLayout(side);
    sideW->setMaximumWidth(300);
    lay->addWidget(sideW);
    pattern_ = new PatternView(ed_);
    lay->addWidget(pattern_, 1);
    connect(pattern_, &PatternView::preview, this, &MainWindow::previewCell);
    connect(pattern_, &PatternView::previewSample, this, &MainWindow::previewSample);
    connect(pattern_, &PatternView::previewOff, this, &MainWindow::previewOff);
    connect(pattern_, &PatternView::stopRequested, this, &MainWindow::stop);
    connect(pattern_, &PatternView::cursorMoved, this, &MainWindow::refreshHeader);
    connect(pattern_, &PatternView::mutedChanged, this, [this](int mask) { engine_.setMute(mask); });
    return w;
}

QString MainWindow::groupName(int group) const
{
    if (group < Editor::SMP_GROUP)
        return ed_->tune().chips > 1 ? tr("AY %1").arg(group + 1) : tr("AY");
    return tr("M%1").arg(group - Editor::SMP_GROUP + 1);
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
    const Tune &t = ed_->tune();
    if (!title_->hasFocus())
        title_->setText(QString::fromLatin1(t.title.c_str()));
    if (!author_->hasFocus())
        author_->setText(QString::fromLatin1(t.author.c_str()));
    speed_->setValue(t.speed);
    tempo_->setValue(t.tempo);
    chips_->setCurrentIndex(std::clamp(t.chips, 0, 2));
    smpTracks_->setValue(t.smpTracks);
    chip_->setEnabled(t.chips > 1);
    chip_->setCurrentIndex(ed_->chip());
    table_->setEnabled(t.chips > 0);
    version_->setEnabled(t.chips > 0);
    table_->setCurrentIndex(ed_->bank().table & 3);
    version_->setCurrentIndex(std::clamp(ed_->bank().version, 0, 9));
    int g = pattern_->group();
    bool has = ed_->hasGroup(g);
    patLabel_->setText(has ? tr("Patrón (%1)").arg(groupName(g)) : tr("Patrón"));
    patNum_->setEnabled(has);
    patNum_->setMaximum(g < Editor::SMP_GROUP ? MAX_PATTERNS - 1 : 255);
    patNum_->setValue(has ? ed_->patternOf(g) : -1);
    patRows_->setValue(ed_->rows());
    refreshing_ = false;
}

void MainWindow::refreshPositions()
{
    refreshing_ = true;
    const Tune &t = ed_->tune();
    if (positions_->count() != int(t.order.size())) {
        positions_->clear();
        for (size_t i = 0; i < t.order.size(); i++)
            positions_->addItem(QString());
    }
    QFont mono = pattern_->font();
    auto num = [](int p) { return p < 0 ? QStringLiteral("--") : QStringLiteral("%1").arg(p, 2, 10, QChar('0')); };
    for (int i = 0; i < int(t.order.size()); i++) {
        const Position &p = t.order[size_t(i)];
        QStringList parts;
        for (int c = 0; c < t.chips; c++)
            parts << num(p.ay[size_t(c)]);
        if (t.smpTracks) {
            QStringList smp;
            for (int k = 0; k < t.smpTracks; k++)
                smp << num(p.smp[size_t(k)]);
            parts << QStringLiteral("| ") + smp.join(' ');
        }
        QListWidgetItem *it = positions_->item(i);
        it->setFont(mono);
        it->setText(QStringLiteral("%1  %2  (%3)%4")
                        .arg(i, 2, 10, QChar('0'))
                        .arg(parts.join(' '))
                        .arg(p.rows)
                        .arg(i == t.loop ? "  L" : ""));
    }
    positions_->setCurrentRow(ed_->position());
    refreshing_ = false;
}

/* Cuántos AY usa la canción: los que se quitan dejan de sonar (se pregunta
 * si tienen algo); los nuevos empiezan con un patrón vacío en cada posición. */
void MainWindow::setChips(int chips)
{
    Tune &t = ed_->tune();
    chips = std::clamp(chips, 0, 2);
    if (chips == t.chips)
        return;
    if (chips < t.chips) {
        bool used = false;
        for (const Position &p : t.order)
            for (int c = chips; c < t.chips; c++)
                if (p.ay[size_t(c)] >= 0 && p.ay[size_t(c)] < int(t.ayPatterns[size_t(c)].size()) &&
                    !t.ayPatterns[size_t(c)][size_t(p.ay[size_t(c)])].empty())
                    used = true;
        if (used && QMessageBox::question(this, tr("AY"), tr("¿Quitar los patrones del AY que sobra?")) != QMessageBox::Yes) {
            refreshHeader();
            return;
        }
    }
    ed_->begin(Editor::Everything);
    for (int c = 0; c < 2; c++) {
        bool on = c < chips;
        if (on && c >= t.chips) {
            if (t.ayPatterns[size_t(c)].empty())
                t.ayPatterns[size_t(c)].resize(1);
            if (c == 1) {
                t.ay[1].table = t.ay[0].table;
                t.ay[1].version = t.ay[0].version;
            }
        }
        for (Position &p : t.order)
            p.ay[size_t(c)] = on ? (c < t.chips && p.ay[size_t(c)] >= 0 ? p.ay[size_t(c)] : 0) : -1;
    }
    t.chips = chips;
    ed_->changed(Editor::Everything);
    ed_->setChip(chips > 1 ? ed_->chip() : 0);
}

/* Cuántas pistas de muestras: las nuevas, sin patrón (se le pone al
 * escribir en ella); las que se quitan, igual que los AY. */
void MainWindow::setSmpTracks(int tracks)
{
    Tune &t = ed_->tune();
    tracks = std::clamp(tracks, 0, SMP_TRACKS);
    if (tracks == t.smpTracks)
        return;
    if (tracks < t.smpTracks) {
        bool used = false;
        for (const Position &p : t.order)
            for (int k = tracks; k < t.smpTracks; k++)
                if (p.smp[size_t(k)] >= 0 && p.smp[size_t(k)] < int(t.smpPatterns.size()) &&
                    !(t.smpPatterns[size_t(p.smp[size_t(k)])] == SmpPattern()))
                    used = true;
        if (used && QMessageBox::question(this, tr("Pistas de muestras"), tr("¿Quitar las pistas que sobran?")) !=
                        QMessageBox::Yes) {
            refreshHeader();
            return;
        }
    }
    ed_->begin(Editor::Everything);
    for (Position &p : t.order)
        for (int k = tracks; k < SMP_TRACKS; k++)
            p.smp[size_t(k)] = -1;
    t.smpTracks = tracks;
    ed_->changed(Editor::Everything);
}

void MainWindow::addPosition()
{
    if (int(ed_->tune().order.size()) >= MAX_TUNE_POSITIONS)
        return;
    int p = ed_->position();
    editHeader([this, p] {
        std::vector<Position> &o = ed_->tune().order;
        o.insert(o.begin() + p + 1, o[size_t(p)]);
    });
    ed_->setPosition(p + 1);
}

void MainWindow::removePosition()
{
    if (ed_->tune().order.size() <= 1)
        return;
    int p = ed_->position();
    editHeader([this, p] {
        Tune &t = ed_->tune();
        t.order.erase(t.order.begin() + p);
        if (t.loop > p || t.loop >= int(t.order.size()))
            t.loop = std::max(0, t.loop - 1);
    });
    ed_->setPosition(std::min(p, int(ed_->tune().order.size()) - 1));
}

void MainWindow::clonePattern()
{
    int g = pattern_->group(), from = ed_->patternOf(g);
    if (from < 0) {
        message(tr("La pista no tiene patrón en esta posición."));
        return;
    }
    Tune &t = ed_->tune();
    bool ay = g < Editor::SMP_GROUP;
    size_t pool = ay ? t.ayPatterns[size_t(g)].size() : t.smpPatterns.size();
    std::vector<bool> used(pool + 1);
    for (const Position &p : t.order) {
        if (ay && p.ay[size_t(g)] >= 0 && size_t(p.ay[size_t(g)]) < pool)
            used[size_t(p.ay[size_t(g)])] = true;
        if (!ay)
            for (int s : p.smp)
                if (s >= 0 && size_t(s) < pool)
                    used[size_t(s)] = true;
    }
    int free = -1;
    for (size_t i = 0; i < pool && free < 0; i++)
        if (!used[i] && (ay ? t.ayPatterns[size_t(g)][i].empty() : t.smpPatterns[i] == SmpPattern()))
            free = int(i);
    if (free < 0 && (ay ? pool < size_t(MAX_PATTERNS) : pool < 256))
        free = int(pool);
    if (free < 0) {
        message(tr("No queda ningún patrón libre."));
        return;
    }
    ed_->begin(Editor::Everything);
    if (ay) {
        if (size_t(free) >= t.ayPatterns[size_t(g)].size())
            t.ayPatterns[size_t(g)].resize(size_t(free) + 1);
        t.ayPatterns[size_t(g)][size_t(free)] = t.ayPatterns[size_t(g)][size_t(from)];
        ed_->pos().ay[size_t(g)] = free;
    } else {
        if (size_t(free) >= t.smpPatterns.size())
            t.smpPatterns.resize(size_t(free) + 1);
        t.smpPatterns[size_t(free)] = t.smpPatterns[size_t(from)];
        ed_->pos().smp[size_t(g - Editor::SMP_GROUP)] = free;
    }
    ed_->changed(Editor::Everything);
    message(tr("El patrón %1, copiado en el %2").arg(from).arg(free));
}

void MainWindow::setLoop()
{
    int p = ed_->position();
    editHeader([this, p] { ed_->tune().loop = p; });
}

/* El patrón de la pista del cursor en la posición (uno que no hay, nuevo y
 * vacío; -1, ninguno). */
void MainWindow::setGroupPattern(int pattern)
{
    int g = pattern_->group();
    if (!ed_->hasGroup(g))
        return;
    Tune &t = ed_->tune();
    ed_->begin(Editor::Everything);
    if (g < Editor::SMP_GROUP) {
        pattern = std::min(pattern, MAX_PATTERNS - 1);
        if (pattern >= int(t.ayPatterns[size_t(g)].size()))
            t.ayPatterns[size_t(g)].resize(size_t(pattern) + 1);
        ed_->pos().ay[size_t(g)] = pattern;
    } else {
        if (pattern >= int(t.smpPatterns.size()))
            t.smpPatterns.resize(size_t(pattern) + 1);
        ed_->pos().smp[size_t(g - Editor::SMP_GROUP)] = pattern;
    }
    ed_->changed(Editor::Everything);
}

/* Las filas de la posición, y las de sus patrones. */
void MainWindow::setPositionRows(int rows)
{
    rows = std::clamp(rows, 1, MAX_ROWS);
    Tune &t = ed_->tune();
    ed_->begin(Editor::Everything);
    Position &p = ed_->pos();
    p.rows = rows;
    for (int c = 0; c < t.chips; c++)
        if (p.ay[size_t(c)] >= 0 && p.ay[size_t(c)] < int(t.ayPatterns[size_t(c)].size()))
            t.ayPatterns[size_t(c)][size_t(p.ay[size_t(c)])].rows.resize(size_t(rows));
    for (int k = 0; k < t.smpTracks; k++)
        if (p.smp[size_t(k)] >= 0 && p.smp[size_t(k)] < int(t.smpPatterns.size()))
            t.smpPatterns[size_t(p.smp[size_t(k)])].rows.resize(size_t(rows));
    ed_->changed(Editor::Everything);
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
    top->addWidget(new QLabel(tr("Instrumento")));
    top->addWidget(sampleNum_);
    top->addSpacing(16);
    top->addWidget(new QLabel(tr("Líneas")));
    top->addWidget(sampleLen_);
    top->addWidget(new QLabel(tr("Vuelta")));
    top->addWidget(sampleLoop_);
    top->addSpacing(16);
    templates_ = new QComboBox;
    templates_->addItem(tr("Plantilla..."));
    for (const auto &t : AyResult::templates())
        templates_->addItem(t.first);
    templates_->setToolTip(tr("Empezar de un instrumento hecho (se puede deshacer)"));
    top->addWidget(templates_);
    sampleInfo_ = new QLabel;
    top->addSpacing(16);
    top->addWidget(sampleInfo_, 1);
    lay->addLayout(top);
    /* Las líneas y, debajo, lo que suena. */
    auto *split = new QSplitter(Qt::Vertical);
    sampleView_ = new SampleView(ed_);
    result_ = new AyResult(ed_);
    split->addWidget(sampleView_);
    split->addWidget(result_);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);
    lay->addWidget(split, 1);
    samplePiano_ = new Piano;
    lay->addWidget(samplePiano_);
    connect(templates_, &QComboBox::activated, this, [this](int i) {
        auto all = AyResult::templates();
        if (i >= 1 && i <= int(all.size())) {
            ed_->begin(Editor::SamplePart, ed_->chip(), ed_->sample());
            ed_->bank().samples[size_t(ed_->sample())] = all[size_t(i - 1)].second;
            ed_->changed(Editor::SamplePart);
            message(tr("El instrumento %1: %2").arg(ed_->sample()).arg(all[size_t(i - 1)].first));
        }
        templates_->setCurrentIndex(0);
    });

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
        ed_->begin(Editor::SamplePart, ed_->chip(), ed_->sample());
        Sample &s = ed_->bank().samples[size_t(ed_->sample())];
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
        ed_->begin(Editor::OrnamentPart, ed_->chip(), ed_->ornament());
        Ornament &o = ed_->bank().ornaments[size_t(ed_->ornament())];
        o.loop = std::clamp(v, 0, std::max(0, int(o.offsets.size()) - 1));
        ed_->changed(Editor::OrnamentPart);
    });
    connect(ornamentPiano_, &Piano::noteOn, this, [this](int n) { previewNote(n, true); });
    connect(ornamentPiano_, &Piano::noteOff, this, &MainWindow::previewOff);
    return w;
}

void MainWindow::resizeSample(int lines)
{
    ed_->begin(Editor::SamplePart, ed_->chip(), ed_->sample());
    Sample &s = ed_->bank().samples[size_t(ed_->sample())];
    SampleLine last = s.lines.empty() ? defaultSample().lines[0] : s.lines.back();
    s.lines.resize(size_t(std::clamp(lines, 0, MAX_LINES)), last);
    s.loop = std::clamp(s.loop, 0, std::max(0, int(s.lines.size()) - 1));
    ed_->changed(Editor::SamplePart);
}

void MainWindow::resizeOrnament(int lines)
{
    ed_->begin(Editor::OrnamentPart, ed_->chip(), ed_->ornament());
    Ornament &o = ed_->bank().ornaments[size_t(ed_->ornament())];
    o.offsets.resize(size_t(std::clamp(lines, 0, MAX_LINES)), 0);
    o.loop = std::clamp(o.loop, 0, std::max(0, int(o.offsets.size()) - 1));
    ed_->changed(Editor::OrnamentPart);
}

void MainWindow::refreshSample()
{
    refreshing_ = true;
    const Sample &s = ed_->bank().samples[size_t(ed_->sample())];
    sampleNum_->setValue(ed_->sample());
    sample_->setValue(ed_->sample());
    sampleLen_->setValue(int(s.lines.size()));
    sampleLoop_->setValue(s.loop);
    sampleLoop_->setEnabled(!s.lines.empty());
    QString chip = ed_->tune().chips > 1 ? tr(" (del AY %1)").arg(ed_->chip() + 1) : QString();
    sampleInfo_->setText(s.lines.empty() ? tr("Este instrumento%1 no tiene nada: elige una plantilla o dale líneas.").arg(chip)
                                         : tr("Toca con el piano de abajo o con el teclado al pulsarlo.%1").arg(chip));
    refreshing_ = false;
    sampleView_->update();
}

void MainWindow::refreshOrnament()
{
    refreshing_ = true;
    const Ornament &o = ed_->bank().ornaments[size_t(ed_->ornament())];
    ornNum_->setValue(ed_->ornament());
    ornament_->setValue(ed_->ornament());
    ornLen_->setValue(int(o.offsets.size()));
    ornLoop_->setValue(o.loop);
    ornLoop_->setEnabled(!o.offsets.empty());
    ornInfo_->setText(o.offsets.empty() ? tr("Este ornamento no tiene nada: dale líneas (o pulsa Insert).")
                                        : tr("Suena con el instrumento %1.").arg(ed_->sample()));
    refreshing_ = false;
    ornamentView_->update();
}

/* --- Las muestras (los canales de muestras) ------------------------------- */

QWidget *MainWindow::buildWaveTab()
{
    auto *w = new QWidget;
    auto *lay = new QHBoxLayout(w);

    auto *side = new QVBoxLayout;
    waves_ = new QListWidget;
    waves_->setToolTip(tr("Las muestras de la canción: el número, lo que ocupa y su formato"));
    side->addWidget(waves_, 1);
    auto *buttons = new QGridLayout;
    auto button = [&](const QString &text, const QString &tip, int row, int col, const std::function<void()> &fn) {
        auto *b = new QPushButton(text);
        b->setToolTip(tip);
        connect(b, &QPushButton::clicked, this, fn);
        buttons->addWidget(b, row, col);
    };
    button(tr("Importar WAV..."), tr("Una muestra nueva de un WAV (y un instrumento que la toca en todas las notas)"), 0, 0,
           [this] { importWavs(); });
    button(tr("Quitar"), tr("Quitar esta muestra (sus notas de los instrumentos, sin muestra)"), 0, 1,
           [this] { removeWave(); });
    button(tr("A la mitad"), tr("La mitad de muestras: ocupa la mitad y suena a la misma altura, con menos agudos"), 1, 0,
           [this] { editWave([](Wave &v) { halveWave(v); }); });
    side->addLayout(buttons);
    soundMem_ = new QProgressBar;
    soundMem_->setRange(0, int(SOUND_MEM));
    soundMem_->setToolTip(tr("Lo que ocupan las muestras de la canción en la memoria de sonido (96 KB)"));
    side->addWidget(soundMem_);
    auto *sideW = new QWidget;
    sideW->setLayout(side);
    sideW->setMaximumWidth(300);
    lay->addWidget(sideW);

    auto *main = new QVBoxLayout;
    auto *form = new QHBoxLayout;
    waveFormat_ = new QComboBox;
    waveFormat_->addItems({ tr("8 bits"), tr("ADPCM") });
    waveFormat_->setToolTip(tr("8 bits: suena como se grabó, a cualquier altura. ADPCM: ocupa algo más de la mitad, "
                               "suena como 16 bits, y no más de una octava por encima de como se grabó."));
    waveRel_ = new QSpinBox;
    waveRel_->setRange(-96, 95);
    waveRel_->setToolTip(tr("La nota relativa: los semitonos que se suben a todas las notas (al importar, para que "
                            "un C-4 suene a la altura de la grabación)"));
    waveFine_ = new QSpinBox;
    waveFine_->setRange(-128, 127);
    waveFine_->setToolTip(tr("La afinación, en 1/128 de semitono"));
    waveVol_ = new QSpinBox;
    waveVol_->setRange(0, 64);
    waveVol_->setToolTip(tr("El volumen de la muestra (0-64)"));
    wavePan_ = new QCheckBox(tr("Panorama"));
    wavePan_->setToolTip(tr("Con su panorama; si no, el de la pista"));
    wavePanV_ = new QSpinBox;
    wavePanV_->setRange(0, 255);
    wavePanV_->setToolTip(tr("0 a la izquierda, 128 en el centro, 255 a la derecha"));
    waveLoop_ = new QCheckBox(tr("Bucle desde"));
    waveLoop_->setToolTip(tr("Que se repita mientras suena la nota, desde esa muestra hasta el final"));
    waveLoopFrom_ = new QSpinBox;
    waveLoopFrom_->setRange(0, int(SOUND_MEM));
    for (auto *x : std::initializer_list<QWidget *>{ new QLabel(tr("Formato")), waveFormat_, new QLabel(tr("Nota rel.")),
                                                     waveRel_, new QLabel(tr("Afinación")), waveFine_,
                                                     new QLabel(tr("Volumen")), waveVol_, wavePan_, wavePanV_, waveLoop_,
                                                     waveLoopFrom_ })
        form->addWidget(x);
    form->addStretch(1);
    main->addLayout(form);
    waveInfo_ = new QLabel;
    main->addWidget(waveInfo_);
    waveView_ = new WaveView(ed_);
    main->addWidget(waveView_, 1);
    wavePiano_ = new Piano;
    main->addWidget(wavePiano_);
    lay->addLayout(main, 1);

    connect(waves_, &QListWidget::currentRowChanged, this, [this](int r) {
        if (!refreshing_ && r >= 0) {
            waveSel_ = r;
            refreshWaves();
        }
    });
    connect(waveFormat_, &QComboBox::currentIndexChanged, this, [this](int f) {
        if (!refreshing_)
            editWave([f](Wave &v) { encodeWave(v, f); });
    });
    connect(waveRel_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            editWave([v](Wave &x) { x.relnote = v; });
    });
    connect(waveFine_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            editWave([v](Wave &x) { x.fine = v; });
    });
    connect(waveVol_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            editWave([v](Wave &x) { x.vol = v; });
    });
    connect(wavePan_, &QCheckBox::toggled, this, [this](bool on) {
        if (!refreshing_)
            editWave([on](Wave &x) { x.hasPan = on; });
    });
    connect(wavePanV_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            editWave([v](Wave &x) { x.pan = v; });
    });
    connect(waveLoop_, &QCheckBox::toggled, this, [this](bool on) {
        if (!refreshing_)
            editWave([on](Wave &x) { x.loop = on ? std::min<uint32_t>(x.loop, x.len ? x.len - 1 : 0) : x.len; });
    });
    connect(waveLoopFrom_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            editWave([v](Wave &x) { x.loop = std::min<uint32_t>(uint32_t(v), x.len ? x.len - 1 : 0); });
    });
    connect(waveView_, &WaveView::loopChanged, this, [this](uint32_t from, bool start) {
        Tune &t = ed_->tune();
        if (waveSel_ < 0 || waveSel_ >= int(t.waves.size()))
            return;
        if (start)
            ed_->begin(Editor::Everything);
        t.waves[size_t(waveSel_)].loop = from;
        ed_->changed(Editor::Everything);
    });
    connect(wavePiano_, &Piano::noteOn, this, [this](int n) { previewWave(n); });
    connect(wavePiano_, &Piano::noteOff, this, &MainWindow::previewOff);
    return w;
}

/* Un cambio en la muestra elegida (para deshacer, la canción entera). */
void MainWindow::editWave(const std::function<void(Wave &)> &fn)
{
    Tune &t = ed_->tune();
    if (waveSel_ < 0 || waveSel_ >= int(t.waves.size()))
        return;
    ed_->begin(Editor::Everything);
    fn(t.waves[size_t(waveSel_)]);
    ed_->changed(Editor::Everything);
}

void MainWindow::refreshWaves()
{
    refreshing_ = true;
    const Tune &t = ed_->tune();
    if (waves_->count() != int(t.waves.size())) {
        waves_->clear();
        for (size_t i = 0; i < t.waves.size(); i++)
            waves_->addItem(QString());
    }
    for (int i = 0; i < int(t.waves.size()); i++) {
        const Wave &v = t.waves[size_t(i)];
        waves_->item(i)->setText(tr("%1  %2 KB  %3%4")
                                     .arg(i, 2, 10, QChar('0'))
                                     .arg(waveBytes(v) / 1024.0, 0, 'f', 1)
                                     .arg(v.format ? tr("ADPCM") : tr("8 bits"))
                                     .arg(v.loop < v.len ? tr("  bucle") : QString()));
    }
    waveSel_ = std::clamp(waveSel_, 0, std::max(0, int(t.waves.size()) - 1));
    waves_->setCurrentRow(t.waves.empty() ? -1 : waveSel_);
    bool has = !t.waves.empty();
    for (QWidget *x : std::initializer_list<QWidget *>{ waveFormat_, waveRel_, waveFine_, waveVol_, wavePan_, wavePanV_,
                                                        waveLoop_, waveLoopFrom_ })
        x->setEnabled(has);
    if (has) {
        const Wave &v = t.waves[size_t(waveSel_)];
        waveFormat_->setCurrentIndex(v.format ? 1 : 0);
        waveRel_->setValue(v.relnote);
        waveFine_->setValue(v.fine);
        waveVol_->setValue(v.vol);
        wavePan_->setChecked(v.hasPan);
        wavePanV_->setValue(v.pan);
        wavePanV_->setEnabled(v.hasPan);
        waveLoop_->setChecked(v.loop < v.len);
        waveLoopFrom_->setMaximum(std::max(0, int(v.len) - 1));
        waveLoopFrom_->setValue(int(std::min(v.loop, v.len ? v.len - 1 : 0)));
        waveLoopFrom_->setEnabled(v.loop < v.len);
        waveInfo_->setText(tr("Muestra %1: %2 muestras, un C-4 a %3 Hz, %4 bytes")
                               .arg(waveSel_)
                               .arg(v.len)
                               .arg(waveRate(v, t.flags), 0, 'f', 0)
                               .arg(waveBytes(v)));
    } else {
        waveInfo_->setText(tr("Importa un WAV: cada muestra se toca con un instrumento (se hace uno al importarla) "
                              "en las pistas de muestras."));
    }
    uint32_t used = wavesBytes(t);
    soundMem_->setValue(int(std::min(used, SOUND_MEM)));
    soundMem_->setFormat(tr("%1 de 96 KB").arg(QString::number(used / 1024.0, 'f', 1)));
    waveView_->setWave(has ? waveSel_ : -1);
    refreshing_ = false;
}

/* Los WAV, a muestras nuevas: cada una con su instrumento. Si no cabe en la
 * memoria de sonido, en ADPCM; si aún no, a la mitad (y se dice). */
void MainWindow::importWavs()
{
    QStringList paths = QFileDialog::getOpenFileNames(this, tr("Importar WAV"), settings_->value("files/wavdir").toString(),
                                                      tr("Sonido WAV (*.wav);;Todos los ficheros (*)"));
    if (paths.isEmpty())
        return;
    settings_->setValue("files/wavdir", QFileInfo(paths.first()).absolutePath());
    Tune &t = ed_->tune();
    ed_->begin(Editor::Everything);
    QStringList done, problems;
    for (const QString &path : paths) {
        bool ok;
        QByteArray data = readAll(path, &ok);
        Wave v;
        std::string why;
        QString name = QFileInfo(path).fileName();
        if (!ok || !loadWav(std::vector<uint8_t>(data.begin(), data.end()), t.flags, v, why)) {
            problems << tr("%1: %2").arg(name, ok ? QString::fromStdString(why) : tr("no se puede leer"));
            continue;
        }
        uint32_t room = SOUND_MEM - std::min(SOUND_MEM, wavesBytes(t));
        QString how;
        if (waveBytes(v) > room) {
            encodeWave(v, 1);
            how = tr(" (en ADPCM, para que quepa)");
        }
        while (waveBytes(v) > room && v.len > 64) {
            halveWave(v);
            how = tr(" (en ADPCM y con menos muestras, para que quepa)");
        }
        if (waveBytes(v) > room || t.waves.size() >= 255) {
            problems << tr("%1: no cabe en la memoria de sonido").arg(name);
            continue;
        }
        t.waves.push_back(v);
        int ins = addInstrument(t, int(t.waves.size()) - 1);
        done << tr("%1: la muestra %2, el instrumento %3%4").arg(name).arg(t.waves.size() - 1).arg(ins).arg(how);
        if (ins)
            instrument_->setValue(ins);
    }
    if (!done.isEmpty() && t.smpTracks == 0) {
        t.smpTracks = 1;                /* para poder tocarla */
        done << tr("y una pista de muestras para tocarla");
    }
    waveSel_ = int(t.waves.size()) - 1;
    ed_->changed(Editor::Everything);
    if (!problems.isEmpty() || done.size() > 1) {
        Warnings w;
        for (const QString &s : done + problems)
            w.push_back(s.toStdString());
        showWarnings(tr("Importar WAV"), tr("Importado:"), w);
    } else if (!done.isEmpty()) {
        message(done.first());
    }
}

/* Quitar la muestra elegida: las notas de los instrumentos que la tocaban se
 * quedan sin muestra, y las de detrás bajan un número. */
void MainWindow::removeWave()
{
    Tune &t = ed_->tune();
    if (waveSel_ < 0 || waveSel_ >= int(t.waves.size()))
        return;
    ed_->begin(Editor::Everything);
    t.waves.erase(t.waves.begin() + waveSel_);
    for (SmpInstrument &ins : t.instruments)
        for (int n = 0; n < 96; n++) {
            uint8_t &m = ins.data[size_t(n)];
            if (m == waveSel_)
                m = 255;
            else if (m != 255 && m > waveSel_)
                m--;
        }
    ed_->changed(Editor::Everything);
}

/* La muestra elegida, a la altura de "note" (la del piano): una canción de
 * un instrumento que la toca en todas las notas. */
void MainWindow::previewWave(int note)
{
    const Tune &t = ed_->tune();
    if (waveSel_ < 0 || waveSel_ >= int(t.waves.size()) || engine_.status().playing)
        return;
    Tune p;
    p.chips = 0;
    p.smpTracks = 1;
    p.speed = 255;
    p.flags = t.flags;
    p.waves = { t.waves[size_t(waveSel_)] };
    addInstrument(p, 0);
    p.order.assign(1, Position());
    p.order[0].rows = MAX_ROWS;
    p.order[0].ay = { { -1, -1 } };
    p.order[0].smp[0] = 0;
    p.smpPatterns.assign(1, SmpPattern());
    p.smpPatterns[0].rows.assign(MAX_ROWS, SmpCell());
    p.smpPatterns[0].rows[0].note = std::clamp(note + 12, 0, NOTES - 1);  /* el piano cuenta como los AY: C-4 es 36 */
    p.smpPatterns[0].rows[0].ins = 1;
    Warnings w;
    std::vector<uint8_t> data = saveZxm(p, w);
    if (!data.empty())
        engine_.preview(data, 0);
}

/* --- Los instrumentos de las pistas de muestras --------------------------- */

/* Las muestras que toca un instrumento, en pocas palabras: "0", "0-3", "0, 2-4". */
static QString mapSummary(const SmpInstrument &in, int waves)
{
    std::vector<int> used;
    for (int n = 0; n < 96; n++) {
        int s = in.data[size_t(n)];
        if (s < waves && std::find(used.begin(), used.end(), s) == used.end())
            used.push_back(s);
    }
    std::sort(used.begin(), used.end());
    QStringList parts;
    for (size_t i = 0; i < used.size();) {
        size_t j = i;
        while (j + 1 < used.size() && used[j + 1] == used[j] + 1)
            j++;
        parts << (j > i ? QStringLiteral("%1-%2").arg(used[i]).arg(used[j]) : QString::number(used[i]));
        i = j + 1;
    }
    return parts.isEmpty() ? QStringLiteral("--") : parts.join(QStringLiteral(", "));
}

QWidget *MainWindow::buildSmpInsTab()
{
    auto *w = new QWidget;
    auto *lay = new QHBoxLayout(w);

    auto *side = new QVBoxLayout;
    smpIns_ = new QListWidget;
    smpIns_->setToolTip(tr("Los instrumentos de las pistas de muestras: el número (el de la columna del patrón) y "
                           "las muestras que tocan"));
    side->addWidget(smpIns_, 1);
    auto *buttons = new QHBoxLayout;
    auto button = [&](const QString &text, const QString &tip, const std::function<void()> &fn) {
        auto *b = new QPushButton(text);
        b->setToolTip(tip);
        connect(b, &QPushButton::clicked, this, fn);
        buttons->addWidget(b);
    };
    button(tr("Nuevo"), tr("Un instrumento nuevo, con todas las notas en la muestra elegida abajo"), [this] {
        Tune &t = ed_->tune();
        ed_->begin(Editor::Everything);
        int n = addInstrument(t, std::max(0, smpPaint_->value()));
        ed_->changed(Editor::Everything);
        if (n)
            instrument_->setValue(n);
        else
            message(tr("Ya hay 128 instrumentos."));
    });
    button(tr("Copia"), tr("Una copia de este instrumento, al final"), [this] {
        Tune &t = ed_->tune();
        int i = ed_->instrument() - 1;
        if (i < 0 || i >= int(t.instruments.size()) || t.instruments.size() >= 128)
            return;
        ed_->begin(Editor::Everything);
        t.instruments.push_back(t.instruments[size_t(i)]);
        ed_->changed(Editor::Everything);
        instrument_->setValue(int(t.instruments.size()));
    });
    button(tr("Quitar"), tr("Quitar este instrumento: sus notas en los patrones se quedan sin instrumento, y los "
                            "de detrás bajan un número"), [this] { removeSmpIns(); });
    side->addLayout(buttons);
    auto *sideW = new QWidget;
    sideW->setLayout(side);
    sideW->setMaximumWidth(260);
    lay->addWidget(sideW);

    auto *main = new QVBoxLayout;
    auto *mapBar = new QHBoxLayout;
    mapBar->addWidget(new QLabel(tr("Mapa de notas: la muestra")));
    smpPaint_ = new QSpinBox;
    smpPaint_->setRange(-1, 254);
    smpPaint_->setSpecialValueText(tr("ninguna"));
    smpPaint_->setToolTip(tr("La muestra que se pone en las notas al pulsarlas abajo"));
    mapBar->addWidget(smpPaint_);
    auto *all = new QPushButton(tr("En todas las notas"));
    all->setToolTip(tr("Esa muestra en las 96 notas"));
    mapBar->addWidget(all);
    mapBar->addStretch(1);
    main->addLayout(mapBar);
    noteMap_ = new NoteMapView(ed_);
    main->addWidget(noteMap_);

    /* Cada envolvente: si está puesta, su punto de sostener y su bucle. */
    auto envBox = [&](int which, const QString &title, EnvelopeView *&view, QCheckBox *&on, QCheckBox *&sus,
                      QSpinBox *&susP, QCheckBox *&loop, QSpinBox *&from, QSpinBox *&to) {
        auto *box = new QGroupBox(title);
        auto *v = new QVBoxLayout(box);
        auto *row = new QHBoxLayout;
        on = new QCheckBox(tr("Puesta"));
        sus = new QCheckBox(tr("Sostener en el punto"));
        sus->setToolTip(tr("Mientras no se suelte la nota, la envolvente se para en ese punto"));
        susP = new QSpinBox;
        susP->setRange(0, ins::POINTS - 1);
        loop = new QCheckBox(tr("Bucle del punto"));
        loop->setToolTip(tr("Al llegar al segundo punto, vuelve al primero"));
        from = new QSpinBox;
        from->setRange(0, ins::POINTS - 1);
        to = new QSpinBox;
        to->setRange(0, ins::POINTS - 1);
        for (QWidget *x : std::initializer_list<QWidget *>{ on, sus, susP, loop, from, new QLabel(tr("al")), to })
            row->addWidget(x);
        row->addStretch(1);
        v->addLayout(row);
        view = new EnvelopeView(ed_, which);
        v->addWidget(view, 1);
        main->addWidget(box, 1);
        auto edit = [this, which, on, sus, susP, loop, from, to] {
            if (refreshing_)
                return;
            editSmpIns([&](SmpInstrument &in) {
                ins::Env e = ins::env(in, which);
                e.on = on->isChecked();
                e.sus = sus->isChecked();
                e.sustain = susP->value();
                e.loop = loop->isChecked();
                e.loopFrom = from->value();
                e.loopTo = std::max(from->value(), to->value());
                if (e.on && e.n == 0) {
                    /* Para empezar: el volumen que baja; el panorama, en el centro. */
                    e.n = 2;
                    e.tick = { 0, 32 };
                    e.value = { which ? 32 : 64, which ? 32 : 0 };
                }
                ins::setEnv(in, which, e);
            });
        };
        for (QCheckBox *c : { on, sus, loop })
            connect(c, &QCheckBox::toggled, this, edit);
        for (QSpinBox *s : { susP, from, to })
            connect(s, &QSpinBox::valueChanged, this, edit);
    };
    envBox(0, tr("Volumen"), volEnv_, volOn_, volSus_, volSusP_, volLoop_, volFrom_, volTo_);
    envBox(1, tr("Panorama"), panEnv_, panOn_, panSus_, panSusP_, panLoop_, panFrom_, panTo_);

    auto *bottom = new QHBoxLayout;
    fade_ = new QSpinBox;
    fade_->setRange(0, 4095);
    fade_->setToolTip(tr("Lo que baja el volumen en cada paso al soltar la nota (0: no baja; 4095: se calla en seguida)"));
    vibType_ = new QComboBox;
    vibType_->addItems({ tr("seno"), tr("sierra"), tr("cuadrada") });
    vibSweep_ = new QSpinBox;
    vibSweep_->setRange(0, 255);
    vibSweep_->setToolTip(tr("Los pasos que tarda el vibrato en llegar a su profundidad"));
    vibDepth_ = new QSpinBox;
    vibDepth_->setRange(0, 15);
    vibDepth_->setToolTip(tr("Lo que se aparta el tono (0: sin vibrato)"));
    vibRate_ = new QSpinBox;
    vibRate_->setRange(0, 63);
    vibRate_->setToolTip(tr("Lo rápido que va"));
    for (QWidget *x : std::initializer_list<QWidget *>{ new QLabel(tr("Desvanecer")), fade_, new QLabel(tr("Vibrato")),
                                                        vibType_, new QLabel(tr("barrido")), vibSweep_,
                                                        new QLabel(tr("profundidad")), vibDepth_, new QLabel(tr("velocidad")),
                                                        vibRate_ })
        bottom->addWidget(x);
    bottom->addStretch(1);
    main->addLayout(bottom);
    insPiano_ = new Piano;
    main->addWidget(insPiano_);
    lay->addLayout(main, 1);

    connect(smpIns_, &QListWidget::currentRowChanged, this, [this](int r) {
        if (!refreshing_ && r >= 0)
            instrument_->setValue(r + 1);
    });
    connect(smpPaint_, &QSpinBox::valueChanged, this, [this](int v) { noteMap_->setPaint(v < 0 ? 255 : v); });
    connect(all, &QPushButton::clicked, this, [this] {
        int s = smpPaint_->value() < 0 ? 255 : smpPaint_->value();
        editSmpIns([s](SmpInstrument &in) { std::fill(in.data.begin(), in.data.begin() + 96, uint8_t(s)); });
    });
    connect(fade_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!refreshing_)
            editSmpIns([v](SmpInstrument &in) { ins::setFade(in, v); });
    });
    auto vib = [this] {
        if (refreshing_)
            return;
        std::array<int, 4> v = { vibType_->currentIndex(), vibSweep_->value(), vibDepth_->value(), vibRate_->value() };
        editSmpIns([v](SmpInstrument &in) { ins::setVibrato(in, v); });
    };
    connect(vibType_, &QComboBox::currentIndexChanged, this, vib);
    for (QSpinBox *s : { vibSweep_, vibDepth_, vibRate_ })
        connect(s, &QSpinBox::valueChanged, this, vib);
    connect(insPiano_, &Piano::noteOn, this, [this](int n) {
        SmpCell c;
        c.note = std::clamp(n + 12, 0, NOTES - 1);     /* el piano cuenta como los AY: C-4 es 36 */
        c.ins = ed_->instrument();
        previewSample(c, 0);
    });
    connect(insPiano_, &Piano::noteOff, this, &MainWindow::previewOff);
    return w;
}

/* Un cambio en el instrumento elegido (para deshacer, la canción entera). */
void MainWindow::editSmpIns(const std::function<void(SmpInstrument &)> &fn)
{
    Tune &t = ed_->tune();
    int i = ed_->instrument() - 1;
    if (i < 0 || i >= int(t.instruments.size()))
        return;
    ed_->begin(Editor::Everything);
    fn(t.instruments[size_t(i)]);
    ed_->changed(Editor::Everything);
}

void MainWindow::refreshSmpIns()
{
    refreshing_ = true;
    const Tune &t = ed_->tune();
    if (smpIns_->count() != int(t.instruments.size())) {
        smpIns_->clear();
        for (size_t i = 0; i < t.instruments.size(); i++)
            smpIns_->addItem(QString());
    }
    for (int i = 0; i < int(t.instruments.size()); i++) {
        QString m = mapSummary(t.instruments[size_t(i)], int(t.waves.size()));
        QString num = QStringLiteral("%1").arg(i + 1, 2, 16, QChar('0'));
        bool one = !m.contains(QLatin1Char(',')) && !m.contains(QLatin1Char('-'));
        smpIns_->item(i)->setText(m == QLatin1String("--") ? tr("%1  sin muestras").arg(num)
                                  : one                    ? tr("%1  la muestra %2").arg(num, m)
                                                           : tr("%1  las muestras %2").arg(num, m));
    }
    int i = ed_->instrument() - 1;
    bool has = i >= 0 && i < int(t.instruments.size());
    smpIns_->setCurrentRow(has ? i : -1);
    for (QWidget *x : std::initializer_list<QWidget *>{ volOn_, volSus_, volSusP_, volLoop_, volFrom_, volTo_, panOn_,
                                                        panSus_, panSusP_, panLoop_, panFrom_, panTo_, fade_, vibType_,
                                                        vibSweep_, vibDepth_, vibRate_ })
        x->setEnabled(has);
    smpPaint_->setMaximum(std::max(0, int(t.waves.size()) - 1));
    if (has) {
        const SmpInstrument &in = t.instruments[size_t(i)];
        struct Row {
            int which;
            QCheckBox *on, *sus;
            QSpinBox *susP;
            QCheckBox *loop;
            QSpinBox *from, *to;
        };
        for (const Row &r : { Row{ 0, volOn_, volSus_, volSusP_, volLoop_, volFrom_, volTo_ },
                              Row{ 1, panOn_, panSus_, panSusP_, panLoop_, panFrom_, panTo_ } }) {
            ins::Env e = ins::env(in, r.which);
            r.on->setChecked(e.on);
            r.sus->setChecked(e.sus);
            r.susP->setValue(e.sustain);
            r.loop->setChecked(e.loop);
            r.from->setValue(e.loopFrom);
            r.to->setValue(e.loopTo);
        }
        fade_->setValue(ins::fade(in));
        std::array<int, 4> v = ins::vibrato(in);
        vibType_->setCurrentIndex(std::clamp(v[0], 0, 2));
        vibSweep_->setValue(v[1]);
        vibDepth_->setValue(v[2]);
        vibRate_->setValue(v[3]);
    }
    refreshing_ = false;
    noteMap_->update();
    volEnv_->update();
    panEnv_->update();
}

/* Quitar el instrumento elegido: en los patrones, sus notas se quedan sin
 * instrumento y las de los de detrás bajan un número. */
void MainWindow::removeSmpIns()
{
    Tune &t = ed_->tune();
    int k = ed_->instrument();
    if (k < 1 || k > int(t.instruments.size()))
        return;
    ed_->begin(Editor::Everything);
    t.instruments.erase(t.instruments.begin() + (k - 1));
    for (SmpPattern &p : t.smpPatterns)
        for (SmpCell &c : p.rows)
            if (c.ins == k)
                c.ins = 0;
            else if (c.ins > k)
                c.ins--;
    ed_->changed(Editor::Everything);
    instrument_->setValue(std::max(1, std::min(k, int(t.instruments.size()))));
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
        if (int(ed_->fxBank().effects.size()) >= fx::MAX_EFFECTS)
            return;
        ed_->beginBank();
        fx::Effect e;
        e.name = tr("efecto %1").arg(ed_->fxBank().effects.size()).toStdString();
        for (int i = 0; i < 8; i++) {
            fx::Frame f;
            f.volume = 15 - i * 2;
            e.frames.push_back(f);
        }
        ed_->fxBank().effects.push_back(e);
        ed_->endBank();
        ed_->setEffect(int(ed_->fxBank().effects.size()) - 1);
    });
    effButton(tr("Copia"), tr("Una copia de este efecto, al final"), [this] {
        if (int(ed_->fxBank().effects.size()) >= fx::MAX_EFFECTS)
            return;
        ed_->beginBank();
        fx::Effect e = ed_->fxBank().effects[size_t(ed_->effect())];
        ed_->fxBank().effects.push_back(e);
        ed_->endBank();
        ed_->setEffect(int(ed_->fxBank().effects.size()) - 1);
    });
    effButton("-", tr("Quitar este efecto"), [this] {
        if (ed_->fxBank().effects.size() <= 1)
            return;
        ed_->beginBank();
        ed_->fxBank().effects.erase(ed_->fxBank().effects.begin() + ed_->effect());
        ed_->endBank();
    });
    effButton(tr("Subir"), tr("Un puesto antes (el número cambia)"), [this] {
        int e = ed_->effect();
        if (e <= 0)
            return;
        ed_->beginBank();
        std::swap(ed_->fxBank().effects[size_t(e)], ed_->fxBank().effects[size_t(e - 1)]);
        ed_->endBank();
        ed_->setEffect(e - 1);
    });
    effButton(tr("Bajar"), tr("Un puesto después"), [this] {
        int e = ed_->effect();
        if (e + 1 >= int(ed_->fxBank().effects.size()))
            return;
        ed_->beginBank();
        std::swap(ed_->fxBank().effects[size_t(e)], ed_->fxBank().effects[size_t(e + 1)]);
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
        if (r < 0 || name.toStdString() == ed_->fxBank().effects[size_t(r)].name)
            return;
        ed_->beginBank();
        ed_->fxBank().effects[size_t(r)].name = name.toLatin1().left(32).toStdString();
        ed_->endBank();
    });
    connect(effects_, &QListWidget::itemActivated, this, [this] { playEffect(); });
    connect(sfxView_, &LineGrid::activated, this, &MainWindow::playEffect);
    return w;
}

void MainWindow::refreshEffects()
{
    refreshing_ = true;
    const fx::Bank &b = ed_->fxBank();
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
    const fx::Bank &b = ed_->fxBank();
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
    QAction *pat = action(tr("La &posición"), QKeySequence(Qt::Key_F6), [this] { play(ed_->position(), 0, true); });
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
    sample_ = spin(tr("Instr. AY"), 1, SAMPLES - 1, tr("El instrumento de las notas que se escriben en los AY"));
    ornament_ = spin(tr("Ornamento"), 0, ORNAMENTS - 1, tr("El ornamento que se edita (y el que se oye en su pestaña)"));
    instrument_ = spin(tr("Instr. muestras"), 1, 128, tr("El instrumento de las notas que se escriben en las pistas de muestras"));
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
    connect(instrument_, &QSpinBox::valueChanged, this, [this](int v) {
        ed_->setInstrument(v);
        refreshSmpIns();
    });
    connect(follow_, &QAction::toggled, this, [this](bool on) { settings_->setValue("edit/follow", on); });
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
    /* Lo que ocupa: en su formato (un .pt3, si sigue cabiendo en uno). */
    Warnings w;
    if (ed_->format() == FORMAT_PT3 && fitsPt3(ed_->tune())) {
        Song s;
        toPt3(ed_->tune(), s, w);
        size_->setText(QStringLiteral("   ") + tr("PT3: %1 bytes").arg(trk::save(s).size()));
    } else {
        size_->setText(QStringLiteral("   ") + tr(".zxm: %1 bytes").arg(saveZxm(ed_->tune(), w).size()));
    }
    tabs_->setTabText(Effects, tr("Efectos AYFX") + (ed_->bankModified() ? "*" : ""));
}

void MainWindow::message(const QString &text)
{
    statusBar()->showMessage(text, 5000);
}

/* Lo que se ha perdido o cambiado al leer, guardar o tocar, en una ventana
 * que no para al programa. */
void MainWindow::showWarnings(const QString &title, const QString &what, const Warnings &w)
{
    if (w.empty())
        return;
    QStringList items;
    for (const std::string &s : w)
        items << QStringLiteral("<li>%1</li>").arg(QString::fromStdString(s).toHtmlEscaped());
    auto *box = new QMessageBox(QMessageBox::Information, title,
                                QStringLiteral("<p>%1</p><ul>%2</ul>").arg(what.toHtmlEscaped(), items.join(QString())),
                                QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();
}

/* --- Tocar ---------------------------------------------------------------- */

void MainWindow::play(int pos, int row, bool patternOnly)
{
    Tune t = ed_->tune();
    if (patternOnly) {
        t.order = { t.order[size_t(std::clamp(pos, 0, int(t.order.size()) - 1))] };
        t.loop = 0;
        pos = 0;
    }
    Warnings w;
    Engine::Play p;
    p.data = saveZxm(t, w);
    if (p.data.empty()) {
        showWarnings(tr("Tocar"), tr("La canción no se puede tocar:"), w);
        return;
    }
    p.pos = pos;
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
    int pos = s.pos[0], row = std::max(0, s.row[0]);
    where_->setText(tr("Suena: posición %1, fila %2").arg(pos).arg(row));
    if (follow_->isChecked() && s.row[0] >= 0) {
        if (int(ed_->tune().order.size()) > pos && pos != ed_->position() && !refreshing_)
            ed_->setPosition(pos);
        pattern_->setRow(row);
    }
    pattern_->setPlayRow(pos == ed_->position() ? row : -1);
}

void MainWindow::previewCell(const Cell &c, int envPeriod, int chip, int channel)
{
    if (engine_.status().playing)
        return;
    chip = std::clamp(chip, 0, 1);
    engine_.preview(previewModule(bankModule(ed_->tune().ay[size_t(chip)]), c, envPeriod, channel), chip);
}

/* Una nota de una pista de muestras: una canción de una pista con esa nota,
 * con los instrumentos y las muestras de la canción. */
void MainWindow::previewSample(const SmpCell &c, int track)
{
    if (engine_.status().playing)
        return;
    const Tune &t = ed_->tune();
    Tune p;
    p.chips = 0;
    p.smpTracks = 1;
    p.speed = 255;                      /* 256 filas de 255 pasos: no vuelve a empezar */
    p.flags = t.flags;
    p.pan[0] = t.pan[size_t(std::clamp(track, 0, SMP_TRACKS - 1))];
    p.instruments = t.instruments;
    p.waves = t.waves;
    p.order.assign(1, Position());
    p.order[0].rows = MAX_ROWS;
    p.order[0].ay = { { -1, -1 } };
    p.order[0].smp[0] = 0;
    p.smpPatterns.assign(1, SmpPattern());
    p.smpPatterns[0].rows.assign(MAX_ROWS, SmpCell());
    p.smpPatterns[0].rows[0] = c;
    Warnings w;
    std::vector<uint8_t> data = saveZxm(p, w);
    if (!data.empty())
        engine_.preview(data, 0);
}

void MainWindow::previewNote(int note, bool withOrnament)
{
    Cell c;
    c.note = note;
    c.sample = ed_->sample();
    if (withOrnament)
        c.orn = ed_->ornament();
    engine_.preview(previewModule(bankModule(ed_->bank()), c, -1, 0), ed_->chip());
}

void MainWindow::previewOff()
{
    engine_.stopPreview();
}

/* --- Ficheros -------------------------------------------------------------- */

void MainWindow::newSong()
{
    stop();
    ed_->setTune(Tune(), QString(), FORMAT_NONE);
    ed_->setSample(1);
    ed_->setOrnament(0);
}

bool MainWindow::open(const QString &path, uint64_t channels, bool ask)
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
    /* De más de 12 canales con notas, cuáles van. */
    if (!channels && ask) {
        ModuleChannels ch = moduleChannels(bytes);
        if (ch.tooMany()) {
            ChannelsDialog d(QFileInfo(path).fileName(), ch, this);
            if (d.exec() != QDialog::Accepted)
                return false;
            channels = d.channels();
        }
    }
    Tune t;
    Warnings w;
    TuneFormat format = FORMAT_NONE;
    int r = loadTune(bytes, t, w, &format, channels);
    if (r != TUNE_OK) {
        QString why = r == TUNE_UNKNOWN ? tr("no es una canción que se pueda abrir (.zxm, PT3, MOD, S3M o XM)")
                                        : tr("está cortado o dañado");
        QMessageBox::warning(this, tr("Abrir"), tr("%1: %2.").arg(QFileInfo(path).fileName(), why));
        return false;
    }
    stop();
    QString name = QFileInfo(path).fileName();
    /* Un MOD, un S3M o un XM se convierten: se guardan como .zxm (otro fichero). */
    bool converted = format == FORMAT_MODULE;
    ed_->setTune(t, converted ? QString() : QFileInfo(path).absoluteFilePath(), converted ? FORMAT_NONE : format);
    if (!converted)
        addRecent(ed_->path());
    message(converted ? tr("Convertido %1: guárdalo como .zxm").arg(QDir::toNativeSeparators(path))
                      : tr("Abierto %1").arg(QDir::toNativeSeparators(path)));
    if (converted) {
        showWarnings(tr("Abrir"), tr("Al convertir %1:").arg(name), w);
        return true;
    }
    /* Un PT3 con órdenes que el editor no sabe poner (dos efectos en una
     * fila, una envolvente que se quita en la misma fila...), guardado no
     * sonaría igual: se avisa. */
    if (format == FORMAT_PT3) {
        Song s;
        Warnings w2;
        if (toPt3(t, s, w2)) {
            long steps = songSteps(bytes, 200000) * 2 + 200;
            long diff = compareSound(bytes, trk::save(s), steps);
            if (diff >= 0)
                w.push_back(tr("tiene órdenes que este editor no tiene (como dos efectos en la misma fila): se "
                               "puede editar, pero al guardarlo sonará distinto desde el segundo %1")
                                .arg(double(diff) / 50.0, 0, 'f', 1)
                                .toStdString());
        }
    }
    showWarnings(tr("Abrir"), tr("Al abrir %1:").arg(name), w);
    return true;
}

void MainWindow::openDialog()
{
    if (!maybeSave())
        return;
    QString path = QFileDialog::getOpenFileName(
        this, tr("Abrir"), settings_->value("files/dir").toString(),
        tr("Música (*.zxm *.pt3 *.mod *.s3m *.xm);;Canciones de ZX-V (*.zxm);;Módulos PT3 (*.pt3);;"
           "Módulos MOD, S3M y XM (*.mod *.s3m *.xm);;Efectos AYFX (*.afb *.afx);;Todos los ficheros (*)"));
    if (path.isEmpty())
        return;
    settings_->setValue("files/dir", QFileInfo(path).absolutePath());
    open(path);
}

/* Guardar en el formato de su extensión: un .pt3, si cabe en uno (si no, se
 * ofrece guardarlo como .zxm); cualquier otra, .zxm. */
bool MainWindow::save(const QString &path)
{
    std::vector<uint8_t> data;
    Warnings w;
    TuneFormat format = FORMAT_ZXM;
    if (QFileInfo(path).suffix().compare("pt3", Qt::CaseInsensitive) == 0) {
        Song s;
        if (!toPt3(ed_->tune(), s, w)) {
            QStringList why;
            for (const std::string &x : w)
                why << QString::fromStdString(x);
            QString zxm = QFileInfo(path).path() + "/" + QFileInfo(path).completeBaseName() + ".zxm";
            if (QMessageBox::question(this, tr("Guardar"),
                                      tr("La canción ya no cabe en un PT3 (%1). ¿Guardarla como %2?")
                                          .arg(why.join(QStringLiteral("; ")), QFileInfo(zxm).fileName())) !=
                QMessageBox::Yes)
                return false;
            return save(zxm);
        }
        data = trk::save(s);
        format = FORMAT_PT3;
    } else {
        data = saveZxm(ed_->tune(), w);
        if (data.empty()) {
            showWarnings(tr("Guardar"), tr("No se ha podido guardar %1:").arg(QDir::toNativeSeparators(path)), w);
            return false;
        }
    }
    if (!writeAll(path, data)) {
        QMessageBox::warning(this, tr("Guardar"), tr("No se ha podido guardar %1.").arg(QDir::toNativeSeparators(path)));
        return false;
    }
    ed_->setSaved(QFileInfo(path).absoluteFilePath(), format);
    addRecent(ed_->path());
    message(tr("Guardado %1 (%2 bytes)").arg(QDir::toNativeSeparators(path)).arg(data.size()));
    showWarnings(tr("Guardar"), tr("Al guardar %1:").arg(QFileInfo(path).fileName()), w);
    return true;
}

bool MainWindow::saveAs()
{
    QString zxmFilter = tr("Canciones de ZX-V (*.zxm)"), pt3Filter = tr("Módulos PT3 (*.pt3)");
    QString dir = ed_->path().isEmpty() ? settings_->value("files/dir").toString() : ed_->path();
    QString selected = ed_->format() == FORMAT_PT3 ? pt3Filter : zxmFilter;
    QString path = QFileDialog::getSaveFileName(this, tr("Guardar como"), dir, zxmFilter + ";;" + pt3Filter, &selected);
    if (path.isEmpty())
        return false;
    if (QFileInfo(path).suffix().isEmpty())
        path += selected == pt3Filter ? ".pt3" : ".zxm";
    settings_->setValue("files/dir", QFileInfo(path).absolutePath());
    return save(path);
}

bool MainWindow::maybeSave()
{
    if (!ed_->modified())
        return true;
    auto r = QMessageBox::question(this, tr("Cambios sin guardar"), tr("¿Guardar los cambios de la canción?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Save)
        return ed_->path().isEmpty() ? saveAs() : save(ed_->path());
    return r == QMessageBox::Discard;
}

bool MainWindow::exportWav(const QString &path)
{
    Engine e;
    Engine::Play p;
    Warnings w;
    p.data = saveZxm(ed_->tune(), w);
    p.loop = false;
    if (p.data.empty() || !e.play(p))
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
    std::vector<uint8_t> data = fx::saveBank(ed_->fxBank());
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
        if (ok && int(ed_->fxBank().effects.size()) < fx::MAX_EFFECTS &&
            fx::decompile(std::vector<uint8_t>(data.begin(), data.end()), e)) {
            ed_->fxBank().effects.push_back(e);
            added++;
        }
    }
    ed_->endBank();
    ed_->setEffect(int(ed_->fxBank().effects.size()) - 1);
    message(tr("%n efecto(s) importado(s)", nullptr, added));
}

void MainWindow::exportAfx()
{
    const fx::Effect &e = ed_->fxBank().effects[size_t(ed_->effect())];
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
    tabs_->setCurrentIndex(std::clamp(tab, 0, 5));
}

void MainWindow::keysHelp()
{
    QMessageBox::information(
        this, tr("Teclas"),
        tr("<p><b>Notas</b> (en la columna de la nota): Z S X D C V G B H N J M es una octava y "
           "Q 2 W 3 E R 5 T 6 Y 7 U la siguiente; 1 o A, el silencio (R-- en los AY, === en las pistas de "
           "muestras). La octava, con * y / del teclado numérico.</p>"
           "<p><b>Los AY</b>, en hexadecimal: el instrumento (1-9 y A-V), la envolvente (1-E su forma, F sin ella), "
           "el ornamento, el volumen y el efecto: 1 y 2 el tono baja y sube, 3 portamento, 4 y 5 la posición en el "
           "sample y en el ornamento, 6 vibrato, 9 y A la envolvente baja y sube, B el tempo.</p>"
           "<p><b>Las pistas de muestras</b>: el instrumento (01-80), el volumen (00-40) y dos efectos, con la "
           "letra del MOD, el S3M y el XM y su parámetro en hexadecimal.</p>"
           "<p><b>Moverse</b>: flechas, Re Pág y Av Pág, Inicio y Fin; el tabulador, de pista en pista. "
           "Con mayúsculas, se selecciona. Supr borra, Insert mete una fila y Retroceso la quita. Un clic en el "
           "nombre de una pista la silencia.</p>"
           "<p><b>Tocar</b>: F5 la canción, F6 la posición, F7 desde el cursor; F8 o Esc paran.</p>"));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("Acerca de zxv-tracker"),
                       tr("<p><b>Tracker de ZX-V</b> %1</p>"
                          "<p>Música para ZX-V: canciones .zxm con los dos AY y los canales de muestras, módulos "
                          "PT3 (los de Pro Tracker 3 y Vortex Tracker II), y efectos AYFX: los que toca la "
                          "biblioteca de música del SDK (zxv_music.h). Abre también MOD, S3M y XM. Suena con el "
                          "mismo código que la máquina.</p>"
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
    e->acceptProposedAction();
    /* Después de soltarlo: una ventana (guardar antes, los canales de un
     * módulo de más de 12) dentro del soltar deja esperando al que lo
     * arrastra (el Explorador de Windows). */
    QTimer::singleShot(0, this, [this, path] {
        bool sfx = path.endsWith(".afb", Qt::CaseInsensitive) || path.endsWith(".afx", Qt::CaseInsensitive);
        if (sfx ? maybeSaveBank() : maybeSave())
            open(path);
    });
}
