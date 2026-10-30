/*
 * importdialog.cpp - Importar una imagen (ver importdialog.h).
 */
#include "importdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "png.h"

static QSpinBox *spin(int lo, int hi, int value)
{
    auto *s = new QSpinBox;
    s->setRange(lo, hi);
    s->setValue(value);
    return s;
}

static QComboBox *sides()
{
    auto *c = new QComboBox;
    c->addItem("8 x 8", 8);
    c->addItem("16 x 16", 16);
    return c;
}

ImportDialog::ImportDialog(Editor *ed, const QImage &img, const QString &name, QWidget *parent)
    : QDialog(parent), ed_(ed), img_(img)
{
    setWindowTitle(tr("Importar %1").arg(name));
    auto *lay = new QVBoxLayout(this);
    info_ = new QLabel;
    info_->setWordWrap(true);
    lay->addWidget(info_);

    auto *g = new QGridLayout;
    int r = 0, i = ed->index();
    Section s = ed->section();
    g->addWidget(sprite_ = new QRadioButton(tr("Un sprite, el")), r, 0);
    g->addWidget(spriteN_ = spin(0, gfx::SPRITES - 1, s == Section::Sprites ? i : 0), r++, 1);
    g->addWidget(sheet_ = new QRadioButton(tr("Una hoja de sprites, desde el")), r, 0);
    g->addWidget(sheetN_ = spin(0, gfx::SPRITES - 1, s == Section::Sprites ? i : 0), r, 1);
    g->addWidget(new QLabel(tr("de")), r, 2);
    g->addWidget(sheetW_ = spin(1, gfx::MAX_SIDE, 16), r, 3);
    g->addWidget(new QLabel("x"), r, 4);
    g->addWidget(sheetH_ = spin(1, gfx::MAX_SIDE, 16), r++, 5);
    g->addWidget(tiles_ = new QRadioButton(tr("Tiles de")), r, 0);
    g->addWidget(tilesSide_ = sides(), r, 1);
    g->addWidget(new QLabel(tr("desde el")), r, 2);
    g->addWidget(tilesN_ = spin(0, gfx::MAX_TILES - 1, ed->isTiles() ? ed->count(s) : 0), r++, 3);
    g->addWidget(level_ = new QRadioButton(tr("Un nivel: tiles y el mapa")), r, 0);
    g->addWidget(levelM_ = spin(0, gfx::MAPS - 1, s == Section::Maps ? i : 0), r, 1);
    g->addWidget(new QLabel(tr("con tiles de")), r, 2);
    g->addWidget(levelSide_ = sides(), r, 3);
    g->addWidget(levelFlip_ = new QCheckBox(tr("Aprovechar los tiles espejados")), ++r, 1, 1, 5);
    r++;
    g->addWidget(texture_ = new QRadioButton(tr("Una textura, la")), r, 0);
    g->addWidget(textureN_ = spin(0, gfx::TEXTURES - 1, s == Section::Textures ? i : 0), r, 1);
    textureOut_ = new QComboBox;
    textureOut_->addItem(tr("se repite"), -1);
    for (int c = 0; c < 16; c++)
        textureOut_->addItem(tr("fuera, el color %1").arg(c), c);
    g->addWidget(textureOut_, r++, 2, 1, 4);
    lay->addLayout(g);
    lay->addWidget(cornerKey_ = new QCheckBox(tr("El color del píxel de arriba a la izquierda también es transparente")));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(buttons);

    /* Lo más probable según lo que se está editando y la imagen. */
    bool square = img.width() == img.height() && img.width() >= 2 && img.width() <= 256 &&
                  !(img.width() & (img.width() - 1));
    if (s == Section::Textures && square)
        texture_->setChecked(true);
    else if (s == Section::Maps)
        level_->setChecked(true);
    else if (ed->isTiles())
        tiles_->setChecked(true);
    else if (img.width() > 64 || img.height() > 64)
        sheet_->setChecked(true);
    else
        sprite_->setChecked(true);
    tilesSide_->setCurrentIndex(s == Section::Tiles16);
    levelSide_->setCurrentIndex(0);
    connect(cornerKey_, &QCheckBox::toggled, this, &ImportDialog::updateInfo);
    updateInfo();
}

void ImportDialog::updateInfo()
{
    int inexact = 0;
    gfx::fromQImage(img_, &inexact, cornerKey_->isChecked() ? img_.pixelColor(0, 0) : QColor());
    QString t = tr("%1 x %2 píxeles.").arg(img_.width()).arg(img_.height());
    if (inexact)
        t += " " + tr("%n píxel(es) no son colores de ZX-V: se usa el más cercano.", nullptr, inexact);
    info_->setText(t);
}

QString ImportDialog::apply()
{
    gfx::Image img = gfx::fromQImage(img_, nullptr, cornerKey_->isChecked() ? img_.pixelColor(0, 0) : QColor());
    gfx::Document doc = ed_->doc();
    QString err;
    Section go = ed_->section();
    int index = ed_->index();
    if (sprite_->isChecked()) {
        err = gfx::Import::sprite(doc, spriteN_->value(), img);
        go = Section::Sprites;
        index = spriteN_->value();
    } else if (sheet_->isChecked()) {
        err = gfx::Import::sheet(doc, sheetN_->value(), img, sheetW_->value(), sheetH_->value());
        go = Section::Sprites;
        index = sheetN_->value();
    } else if (tiles_->isChecked()) {
        int side = tilesSide_->currentData().toInt();
        err = gfx::Import::tiles(doc, side, tilesN_->value(), img);
        go = side == 8 ? Section::Tiles8 : Section::Tiles16;
        index = tilesN_->value();
    } else if (level_->isChecked()) {
        err = gfx::Import::level(doc, levelM_->value(), levelSide_->currentData().toInt(), img, levelFlip_->isChecked());
        go = Section::Maps;
        index = levelM_->value();
    } else {
        err = gfx::Import::texture(doc, textureN_->value(), img, textureOut_->currentData().toInt());
        go = Section::Textures;
        index = textureN_->value();
    }
    if (!err.isEmpty())
        return err;
    ed_->begin();
    ed_->doc() = doc;
    ed_->changed();
    ed_->select(go, index);
    return QString();
}
