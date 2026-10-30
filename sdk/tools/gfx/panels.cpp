/*
 * panels.cpp - Los paneles de zxv-gfx (ver panels.h).
 */
#include "panels.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

#include "canvas.h"
#include "png.h"

static void drawChecker(QPainter &p, const QRect &r, int cell)
{
    p.fillRect(r, QColor(0xC0, 0xC0, 0xC0));
    for (int y = r.top(); y <= r.bottom(); y += cell)
        for (int x = r.left() + ((y - r.top()) / cell & 1) * cell; x <= r.right(); x += 2 * cell)
            p.fillRect(QRect(x, y, cell, cell) & r, QColor(0xE8, 0xE8, 0xE8));
}

QPixmap thumbnail(const QImage &img, int size, bool checker)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    if (img.isNull() || img.width() == 0)
        return pm;
    /* A un número entero de aumentos si cabe, sin suavizar. */
    double k = std::min(double(size) / img.width(), double(size) / img.height());
    if (k >= 1)
        k = std::floor(k);
    int w = std::max(1, int(img.width() * k)), h = std::max(1, int(img.height() * k));
    QRect r((size - w) / 2, (size - h) / 2, w, h);
    QPainter p(&pm);
    if (checker)
        drawChecker(p, r, 4);
    p.drawImage(r, img);
    return pm;
}

static QPixmap emptyIcon(int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setPen(QPen(QColor(0x90, 0x90, 0x90), 1, Qt::DashLine));
    p.drawRect(2, 2, size - 5, size - 5);
    return pm;
}

/* --- La paleta ------------------------------------------------------------------ */

static const int SW = 26;               /* el lado de cada color */

PalettePanel::PalettePanel(Editor *ed, QWidget *parent) : QWidget(parent), ed_(ed)
{
    connect(ed_, &Editor::colorsChanged, this, QOverload<>::of(&QWidget::update));
    setMinimumSize(sizeHint());
    setToolTip(tr("Botón izquierdo: el color del botón izquierdo; botón derecho: el del derecho"));
}

QSize PalettePanel::sizeHint() const
{
    return QSize(8 * SW + 16, 2 * SW + 56);
}

QRect PalettePanel::swatch(int c) const
{
    return QRect(8 + (c & 7) * SW, 8 + (c >> 3) * SW, SW - 2, SW - 2);
}

void PalettePanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    for (int c = 0; c < 16; c++) {
        QRect r = swatch(c);
        if (c == gfx::TRANSP)
            drawChecker(p, r, 6);
        else
            p.fillRect(r, QColor(gfx::paletteRgb(c)));
        p.setPen(QColor(0x60, 0x60, 0x60));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0, 0, -1, -1));
        for (int b = 0; b < 2; b++) {
            if (ed_->color(b) != c)
                continue;
            QColor mark = (c == 0 || c == 1 || c == 8 || c == 9) ? Qt::white : Qt::black;
            QPolygon tri = b == 0 ? QPolygon({ r.topLeft(), r.topLeft() + QPoint(10, 0), r.topLeft() + QPoint(0, 10) })
                                  : QPolygon({ r.bottomRight(), r.bottomRight() - QPoint(10, 0),
                                               r.bottomRight() - QPoint(0, 10) });
            p.setPen(Qt::NoPen);
            p.setBrush(mark);
            p.drawPolygon(tri);
        }
    }
    /* Los dos colores elegidos. */
    int y = 8 + 2 * SW + 8;
    for (int b = 0; b < 2; b++) {
        QRect r(8 + b * (4 * SW), y, 34, 24);
        int c = ed_->color(b);
        if (c == gfx::TRANSP)
            drawChecker(p, r, 6);
        else
            p.fillRect(r, QColor(gfx::paletteRgb(c)));
        p.setPen(palette().color(QPalette::WindowText));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0, 0, -1, -1));
        p.drawText(QRect(r.right() + 6, y, 4 * SW - 40, 24), Qt::AlignVCenter,
                   (b ? tr("Derecho: %1") : tr("Izquierdo: %1")).arg(c));
    }
}

void PalettePanel::mousePressEvent(QMouseEvent *e)
{
    for (int c = 0; c < 16; c++)
        if (swatch(c).contains(e->position().toPoint())) {
            ed_->setColor(e->button() == Qt::RightButton ? 1 : 0, c);
            return;
        }
}

/* --- La lista de objetos ---------------------------------------------------------- */

static const int ICON = 48;

ObjectList::ObjectList(Editor *ed, QWidget *parent) : QListWidget(parent), ed_(ed)
{
    setViewMode(QListView::IconMode);
    setIconSize(QSize(ICON, ICON));
    setGridSize(QSize(ICON + 16, ICON + 22));
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(300);
    connect(timer, &QTimer::timeout, this, &ObjectList::refresh);
    connect(ed_, &Editor::documentChanged, this, [this, timer] {
        if (ed_->section() != shown_ || ed_->count(shown_) != shownCount_)
            rebuild();
        else if (timer->isActive())
            return;                     /* muchos cambios seguidos: al final */
        else
            refresh();
        timer->start();
    });
    connect(ed_, &Editor::selectionChanged, this, [this] {
        if (ed_->section() != shown_ || ed_->count(shown_) != shownCount_)
            rebuild();
        QSignalBlocker block(this);
        setCurrentRow(ed_->index());
        if (currentItem())
            scrollToItem(currentItem());
    });
    connect(this, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0)
            ed_->select(row);
    });
    connect(this, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *it) {
        if (it->data(Qt::UserRole).toBool())
            emit createRequested();
    });
    rebuild();
}

void ObjectList::rebuild()
{
    QSignalBlocker block(this);
    shown_ = ed_->section();
    shownCount_ = ed_->count(shown_);
    clear();
    for (int i = 0; i < shownCount_; i++)
        addItem(new QListWidgetItem(QString::number(i)));
    refresh();
    setCurrentRow(ed_->index());
}

void ObjectList::refresh()
{
    const gfx::Document &doc = ed_->doc();
    for (int i = 0; i < count() && i < ed_->count(shown_); i++) {
        QImage img;
        bool transp = false;
        switch (shown_) {
        case Section::Sprites:
            if (!doc.sprites[i].empty())
                img = gfx::toQImage(doc.sprites[i], true);
            transp = true;
            break;
        case Section::Tiles8:
        case Section::Tiles16:
            img = gfx::toQImage(doc.tiles(shown_ == Section::Tiles8 ? 8 : 16)[i], false);
            break;
        case Section::Maps:
            if (!doc.maps[i].empty())
                img = gfx::renderMap(doc, i);
            break;
        case Section::Textures:
            if (!doc.textures[i].empty())
                img = gfx::toQImage(doc.textures[i].img, false);
            break;
        }
        QListWidgetItem *it = item(i);
        it->setIcon(img.isNull() ? emptyIcon(ICON) : thumbnail(img, ICON, transp));
        it->setData(Qt::UserRole, img.isNull());
        it->setToolTip(img.isNull() ? tr("Vacío: doble clic para crearlo")
                                    : tr("%1 x %2").arg(img.width()).arg(img.height()));
    }
}

/* --- Las propiedades -------------------------------------------------------------- */

PropertiesPanel::PropertiesPanel(Editor *ed, QWidget *parent) : QWidget(parent), ed_(ed)
{
    auto *lay = new QVBoxLayout(this);
    title_ = new QLabel;
    QFont f = title_->font();
    f.setBold(true);
    title_->setFont(f);
    lay->addWidget(title_);
    pages_ = new QStackedWidget;
    lay->addWidget(pages_);

    auto spin = [](int lo, int hi) {
        auto *s = new QSpinBox;
        s->setRange(lo, hi);
        s->setKeyboardTracking(false);
        return s;
    };
    auto page = [this]() {
        auto *w = new QWidget;
        auto *form = new QFormLayout(w);
        form->setContentsMargins(0, 0, 0, 0);
        pages_->addWidget(w);
        return form;
    };

    /* Sprites. */
    QFormLayout *fs = page();
    fs->addRow(tr("Ancho"), spriteW_ = spin(1, gfx::MAX_SIDE));
    fs->addRow(tr("Alto"), spriteH_ = spin(1, gfx::MAX_SIDE));
    /* Tiles. */
    QFormLayout *ft = page();
    ft->addRow(tr("Tiles"), tilesN_ = spin(0, gfx::MAX_TILES));
    /* Mapas. */
    QFormLayout *fm = page();
    fm->addRow(tr("Ancho"), mapW_ = spin(1, 32767));
    fm->addRow(tr("Alto"), mapH_ = spin(1, 32767));
    mapTile_ = new QComboBox;
    mapTile_->addItem(tr("de 8 x 8"), 8);
    mapTile_->addItem(tr("de 16 x 16"), 16);
    fm->addRow(tr("Tiles"), mapTile_);
    fm->addRow(QString(), mapWrap_ = new QCheckBox(tr("Se repite")));
    /* Texturas. */
    QFormLayout *fx = page();
    texSide_ = new QComboBox;
    for (int s = 2; s <= 256; s *= 2)
        texSide_->addItem(QString("%1 x %1").arg(s), s);
    fx->addRow(tr("Lado"), texSide_);
    fx->addRow(QString(), texWrap_ = new QCheckBox(tr("Se repite")));
    texOut_ = new QComboBox;
    for (int c = 0; c < 16; c++) {
        QPixmap pm(16, 12);
        pm.fill(QColor(gfx::paletteRgb(c)));
        texOut_->addItem(QIcon(pm), QString::number(c), c);
    }
    fx->addRow(tr("Color de fuera"), texOut_);
    /* Sin objeto. */
    page();

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(create_ = new QPushButton);
    buttons->addWidget(delete_ = new QPushButton);
    lay->addLayout(buttons);
    lay->addStretch();

    for (QSpinBox *s : { spriteW_, spriteH_, tilesN_, mapW_, mapH_ })
        connect(s, &QSpinBox::valueChanged, this, &PropertiesPanel::applySize);
    for (QComboBox *c : { mapTile_, texSide_, texOut_ })
        connect(c, &QComboBox::currentIndexChanged, this, &PropertiesPanel::applySize);
    for (QCheckBox *c : { mapWrap_, texWrap_ })
        connect(c, &QCheckBox::toggled, this, &PropertiesPanel::applySize);
    connect(create_, &QPushButton::clicked, this, &PropertiesPanel::createObject);
    connect(delete_, &QPushButton::clicked, this, &PropertiesPanel::deleteObject);
    connect(ed_, &Editor::documentChanged, this, &PropertiesPanel::refresh);
    connect(ed_, &Editor::selectionChanged, this, &PropertiesPanel::refresh);
    refresh();
}

void PropertiesPanel::refresh()
{
    updating_ = true;
    gfx::Document &doc = ed_->doc();
    int i = ed_->index();
    QString name;
    bool exists = true;
    switch (ed_->section()) {
    case Section::Sprites:
        name = tr("Sprite %1").arg(i);
        exists = !doc.sprites[i].empty();
        if (exists) {
            spriteW_->setValue(doc.sprites[i].w);
            spriteH_->setValue(doc.sprites[i].h);
        }
        pages_->setCurrentIndex(exists ? 0 : 4);
        create_->setText(tr("Crear"));
        delete_->setText(tr("Borrar"));
        break;
    case Section::Tiles8:
    case Section::Tiles16: {
        int side = ed_->tileSide(), n = int(doc.tiles(side).size());
        name = n ? tr("Tile %1 de %2 x %2").arg(i).arg(side) : tr("Tiles de %1 x %1").arg(side);
        tilesN_->setValue(n);
        pages_->setCurrentIndex(1);
        create_->setText(tr("Añadir un tile"));
        delete_->setText(tr("Quitar este tile"));
        exists = n > 0;
        break;
    }
    case Section::Maps:
        name = tr("Mapa %1").arg(i);
        exists = !doc.maps[i].empty();
        if (exists) {
            const gfx::Map &m = doc.maps[i];
            mapW_->setValue(m.w);
            mapH_->setValue(m.h);
            mapTile_->setCurrentIndex(m.tile == 16);
            mapWrap_->setChecked(m.wrap);
        }
        pages_->setCurrentIndex(exists ? 2 : 4);
        create_->setText(tr("Crear"));
        delete_->setText(tr("Borrar"));
        break;
    case Section::Textures:
        name = tr("Textura %1").arg(i);
        exists = !doc.textures[i].empty();
        if (exists) {
            const gfx::Texture &t = doc.textures[i];
            texSide_->setCurrentIndex(texSide_->findData(t.img.w));
            texWrap_->setChecked(t.wrap);
            texOut_->setCurrentIndex(t.out);
            texOut_->setEnabled(!t.wrap);
        }
        pages_->setCurrentIndex(exists ? 3 : 4);
        create_->setText(tr("Crear"));
        delete_->setText(tr("Borrar"));
        break;
    }
    title_->setText(name);
    create_->setVisible(ed_->isTiles() || !exists);
    delete_->setVisible(exists);
    updating_ = false;
}

void PropertiesPanel::applySize()
{
    if (updating_)
        return;
    gfx::Document &doc = ed_->doc();
    int i = ed_->index();
    gfx::Document before = doc;
    switch (ed_->section()) {
    case Section::Sprites:
        if (!doc.sprites[i].empty())
            doc.sprites[i] = gfx::resized(doc.sprites[i], spriteW_->value(), spriteH_->value(), gfx::TRANSP);
        break;
    case Section::Tiles8:
    case Section::Tiles16: {
        int side = ed_->tileSide();
        doc.tiles(side).resize(size_t(tilesN_->value()), gfx::Image(side, side, 0));
        break;
    }
    case Section::Maps:
        if (!doc.maps[i].empty()) {
            gfx::Map &m = doc.maps[i];
            m = gfx::resizedMap(m, mapW_->value(), mapH_->value());
            m.tile = mapTile_->currentData().toInt();
            m.wrap = mapWrap_->isChecked();
        }
        break;
    case Section::Textures:
        if (!doc.textures[i].empty()) {
            gfx::Texture &t = doc.textures[i];
            int side = texSide_->currentData().toInt();
            t.img = gfx::resized(t.img, side, side, 0);
            t.wrap = texWrap_->isChecked();
            t.out = texOut_->currentData().toInt();
        }
        break;
    }
    if (doc == before)
        return;
    /* El documento de antes, para deshacer. */
    gfx::Document after = doc;
    doc = before;
    ed_->begin();
    doc = after;
    ed_->select(ed_->index());
    ed_->changed();
}

void PropertiesPanel::createObject()
{
    gfx::Document &doc = ed_->doc();
    int i = ed_->index();
    ed_->begin();
    switch (ed_->section()) {
    case Section::Sprites:
        doc.sprites[i] = gfx::Image(16, 16, gfx::TRANSP);
        break;
    case Section::Tiles8:
    case Section::Tiles16: {
        int side = ed_->tileSide();
        auto &set = doc.tiles(side);
        if (int(set.size()) >= gfx::MAX_TILES) {
            ed_->cancel();
            return;
        }
        set.push_back(gfx::Image(side, side, 0));
        ed_->changed();
        ed_->select(int(set.size()) - 1);
        return;
    }
    case Section::Maps: {
        gfx::Map m;
        m.w = 32;
        m.h = 24;
        m.cells.assign(size_t(m.w) * m.h, 0);
        doc.maps[i] = m;
        break;
    }
    case Section::Textures:
        doc.textures[i] = gfx::Texture();
        doc.textures[i].img = gfx::Image(32, 32, 0);
        break;
    }
    ed_->changed();
}

void PropertiesPanel::deleteObject()
{
    gfx::Document &doc = ed_->doc();
    int i = ed_->index();
    ed_->begin();
    switch (ed_->section()) {
    case Section::Sprites:
        doc.sprites[i] = gfx::Image();
        break;
    case Section::Tiles8:
    case Section::Tiles16: {
        /* Los mapas siguen apuntando a los mismos tiles: los de detrás bajan
         * uno, y las celdas con este pasan al 0. */
        int side = ed_->tileSide();
        auto &set = doc.tiles(side);
        if (i >= int(set.size())) {
            ed_->cancel();
            return;
        }
        set.erase(set.begin() + i);
        for (gfx::Map &m : doc.maps)
            if (!m.empty() && m.tile == side)
                for (uint16_t &c : m.cells) {
                    int t = c & gfx::CELL_TILE;
                    if (t == i)
                        c = 0;
                    else if (t > i)
                        c = uint16_t(c - 1);
                }
        ed_->changed();
        ed_->select(i);
        return;
    }
    case Section::Maps:
        doc.maps[i] = gfx::Map();
        break;
    case Section::Textures:
        doc.textures[i] = gfx::Texture();
        break;
    }
    ed_->changed();
}

/* --- La vista previa ------------------------------------------------------------- */

PreviewPanel::PreviewPanel(Editor *ed, QWidget *parent) : QWidget(parent), ed_(ed)
{
    auto *lay = new QVBoxLayout(this);
    controls_ = new QWidget;
    auto *form = new QFormLayout(controls_);
    form->setContentsMargins(0, 0, 0, 0);
    form->addRow(QString(), play_ = new QCheckBox(tr("Animar: este sprite y los siguientes")));
    frames_ = new QSpinBox;
    frames_->setRange(1, 64);
    frames_->setValue(4);
    form->addRow(tr("Imágenes"), frames_);
    delay_ = new QSpinBox;
    delay_->setRange(1, 60);
    delay_->setValue(8);
    delay_->setSuffix(tr(" /60 s"));
    form->addRow(tr("Cada una"), delay_);
    lay->addWidget(controls_);
    scale_ = new QSpinBox;
    scale_->setRange(1, 8);
    scale_->setValue(2);
    scale_->setSuffix("x");
    auto *row = new QHBoxLayout;
    row->addWidget(new QLabel(tr("Aumento")));
    row->addWidget(scale_);
    row->addStretch();
    lay->addLayout(row);
    mode7_ = new QPushButton(tr("Ver como suelo del modo 7..."));
    lay->addWidget(mode7_);
    lay->addStretch(1);
    setMinimumHeight(220);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this] {
        step_ = (step_ + 1) % frames_->value();
        update();
    });
    auto restart = [this] {
        step_ = 0;
        timer_->setInterval(delay_->value() * 1000 / 60);
        if (play_->isChecked() && ed_->section() == Section::Sprites)
            timer_->start();
        else
            timer_->stop();
        update();
    };
    connect(play_, &QCheckBox::toggled, this, restart);
    connect(delay_, &QSpinBox::valueChanged, this, restart);
    connect(frames_, &QSpinBox::valueChanged, this, restart);
    connect(scale_, &QSpinBox::valueChanged, this, QOverload<>::of(&QWidget::update));
    connect(mode7_, &QPushButton::clicked, this, &PreviewPanel::mode7Requested);
    connect(ed_, &Editor::documentChanged, this, QOverload<>::of(&QWidget::update));
    connect(ed_, &Editor::selectionChanged, this, [this, restart] {
        controls_->setVisible(ed_->section() == Section::Sprites);
        mode7_->setVisible(ed_->section() == Section::Textures);
        restart();
    });
    controls_->setVisible(true);
    mode7_->setVisible(false);
}

QRect PreviewPanel::area() const
{
    int top = mode7_->isVisible() ? mode7_->geometry().bottom() : scale_->geometry().bottom();
    return QRect(8, top + 8, width() - 16, height() - top - 16);
}

void PreviewPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QRect a = area();
    if (a.height() < 8)
        return;
    p.fillRect(a, QColor(0x50, 0x50, 0x50));
    p.setClipRect(a);
    const gfx::Document &doc = ed_->doc();
    int z = scale_->value(), i = ed_->index();
    QImage img;
    bool transp = false;
    int reps = 1;
    switch (ed_->section()) {
    case Section::Sprites: {
        int n = (i + (timer_->isActive() ? step_ : 0)) % gfx::SPRITES;
        if (!doc.sprites[n].empty())
            img = gfx::toQImage(doc.sprites[n], true);
        transp = true;
        break;
    }
    case Section::Tiles8:
    case Section::Tiles16: {
        const auto &set = doc.tiles(ed_->tileSide());
        if (i < int(set.size()))
            img = gfx::toQImage(set[i], false);
        reps = 3;                       /* para ver si casa con sus vecinos */
        break;
    }
    case Section::Maps:
        if (!doc.maps[i].empty())
            img = gfx::renderMap(doc, i);
        break;
    case Section::Textures:
        if (!doc.textures[i].empty())
            img = gfx::toQImage(doc.textures[i].img, false);
        reps = 2;
        break;
    }
    if (img.isNull())
        return;
    int w = img.width() * z, h = img.height() * z;
    QPoint o(a.left() + std::max(0, (a.width() - w * reps) / 2), a.top() + std::max(0, (a.height() - h * reps) / 2));
    for (int y = 0; y < reps; y++)
        for (int x = 0; x < reps; x++) {
            QRect r(o + QPoint(x * w, y * h), QSize(w, h));
            if (transp)
                drawChecker(p, r, 8);
            p.drawImage(r, img);
        }
}

/* --- Los tiles de los mapas ---------------------------------------------------------- */

TilePicker::TilePicker(Editor *ed, QWidget *parent) : QWidget(parent), ed_(ed)
{
    auto *lay = new QVBoxLayout(this);
    auto *grid = new QGridLayout;
    static const char *const names[4] = { QT_TR_NOOP("Espejo H"), QT_TR_NOOP("Espejo V"), QT_TR_NOOP("Transparente"),
                                          QT_TR_NOOP("Delante") };
    for (int k = 0; k < 4; k++) {
        flags_[k] = new QCheckBox(tr(names[k]));
        grid->addWidget(flags_[k], k / 2, k % 2);
        connect(flags_[k], &QCheckBox::toggled, this, [this, k](bool on) {
            if (updating_)
                return;
            int bit = gfx::CELL_FLIP_H << k;
            ed_->setMapFlags(on ? ed_->mapFlags() | bit : ed_->mapFlags() & ~bit);
        });
    }
    flags_[0]->setToolTip(tr("Los tiles se pintan espejados en horizontal"));
    flags_[1]->setToolTip(tr("Los tiles se pintan espejados en vertical"));
    flags_[2]->setToolTip(tr("El color 8 del tile deja ver lo de debajo"));
    flags_[3]->setToolTip(tr("La celda se dibuja delante de los sprites (prioridad)"));
    lay->addLayout(grid);
    list_ = new QListWidget;
    list_->setViewMode(QListView::IconMode);
    list_->setIconSize(QSize(32, 32));
    list_->setGridSize(QSize(40, 52));
    list_->setResizeMode(QListView::Adjust);
    list_->setMovement(QListView::Static);
    list_->setUniformItemSizes(true);
    lay->addWidget(list_);
    connect(list_, &QListWidget::currentRowChanged, this, [this](int row) {
        if (!updating_ && row >= 0)
            ed_->setMapTile(row);
    });
    connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *it) {
        gfx::Map *m = ed_->map();
        emit editTile(m ? m->tile : 8, list_->row(it));
    });
    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(300);
    connect(timer, &QTimer::timeout, this, &TilePicker::rebuild);
    connect(ed_, &Editor::documentChanged, timer, QOverload<>::of(&QTimer::start));
    connect(ed_, &Editor::selectionChanged, this, &TilePicker::rebuild);
    connect(ed_, &Editor::brushChanged, this, [this] {
        updating_ = true;
        for (int k = 0; k < 4; k++)
            flags_[k]->setChecked(ed_->mapFlags() & (gfx::CELL_FLIP_H << k));
        list_->setCurrentRow(ed_->mapTile());
        updating_ = false;
    });
    rebuild();
}

void TilePicker::rebuild()
{
    if (ed_->section() != Section::Maps)
        return;
    updating_ = true;
    gfx::Map *m = ed_->map();
    int side = m ? m->tile : 8;
    const auto &set = ed_->doc().tiles(side);
    if (list_->count() != int(set.size())) {
        list_->clear();
        for (size_t i = 0; i < set.size(); i++)
            list_->addItem(new QListWidgetItem(QString::number(i)));
    }
    for (size_t i = 0; i < set.size(); i++)
        list_->item(int(i))->setIcon(thumbnail(gfx::toQImage(set[i], false), 32, false));
    list_->setCurrentRow(ed_->mapTile());
    updating_ = false;
}
