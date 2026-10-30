/*
 * importdialog.h - Qué hacer con una imagen que se importa: un sprite, una
 * hoja de sprites, tiles, un nivel (tiles y mapa) o una textura.
 */
#ifndef ZXV_GFX_IMPORTDIALOG_H
#define ZXV_GFX_IMPORTDIALOG_H

#include <QDialog>
#include <QImage>

#include "editor.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QRadioButton;
class QSpinBox;

class ImportDialog : public QDialog
{
    Q_OBJECT

public:
    ImportDialog(Editor *ed, const QImage &img, const QString &name, QWidget *parent = nullptr);
    /* Lo importa en el documento (con deshacer); da "" o qué está mal. */
    QString apply();

private:
    void updateInfo();
    Editor *ed_;
    QImage img_;
    QLabel *info_;
    QRadioButton *sprite_, *sheet_, *tiles_, *level_, *texture_;
    QSpinBox *spriteN_, *sheetN_, *sheetW_, *sheetH_, *tilesN_, *levelM_, *textureN_;
    QComboBox *tilesSide_, *levelSide_, *textureOut_;
    QCheckBox *levelFlip_, *cornerKey_;
};

#endif // ZXV_GFX_IMPORTDIALOG_H
