/*
 * channels.cpp - Elegir los canales de un módulo de más de 12 (channels.h).
 */
#include "channels.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ChannelsDialog::ChannelsDialog(const QString &name, const trk::ModuleChannels &ch, QWidget *parent)
    : QDialog(parent), ch_(ch), boxes_(size_t(ch.count), nullptr)
{
    setWindowTitle(tr("Elegir los canales"));
    int n = 0;
    for (int c = 0; c < ch.count; c++)
        n += int(ch.used >> c & 1);

    auto *lay = new QVBoxLayout(this);
    auto *intro = new QLabel(tr("%1 tiene %2 canales con notas, y ZX-V tiene 12 canales de muestras. Marca los que "
                                "van, hasta 12. Vienen marcados los que más notas tocan en la canción; un bajo o un "
                                "fondo con pocas notas largas puede importar más que una batería que toca muchas.")
                                 .arg(name)
                                 .arg(n));
    intro->setWordWrap(true);
    lay->addWidget(intro);

    /* En columnas de 8, como los canales del tracker. */
    auto *grid = new QGridLayout;
    int k = 0;
    for (int c = 0; c < ch.count; c++) {
        if (!(ch.used >> c & 1))
            continue;
        uint32_t notes = ch.notes[c < 32 ? c : 31];
        auto *b = new QCheckBox(notes == 1 ? tr("Canal %1: 1 nota").arg(c + 1)
                                           : tr("Canal %1: %2 notas").arg(c + 1).arg(notes));
        b->setChecked(ch.picked >> c & 1);
        connect(b, &QCheckBox::toggled, this, &ChannelsDialog::update);
        grid->addWidget(b, k % 8, k / 8);
        boxes_[size_t(c)] = b;
        k++;
    }
    lay->addLayout(grid);

    count_ = new QLabel;
    lay->addWidget(count_);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    ok_ = buttons->button(QDialogButtonBox::Ok);
    ok_->setText(tr("Convertir"));
    auto *busiest = buttons->addButton(tr("Los que más tocan"), QDialogButtonBox::ResetRole);
    connect(busiest, &QPushButton::clicked, this, [this] { setChannels(ch_.picked); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(buttons);
    update();
}

uint64_t ChannelsDialog::channels() const
{
    uint64_t set = 0;
    for (size_t c = 0; c < boxes_.size(); c++)
        if (boxes_[c] && boxes_[c]->isChecked())
            set |= uint64_t(1) << c;
    return set;
}

void ChannelsDialog::setChannels(uint64_t set)
{
    for (size_t c = 0; c < boxes_.size(); c++)
        if (boxes_[c]) {
            boxes_[c]->blockSignals(true);
            boxes_[c]->setChecked(set >> c & 1);
            boxes_[c]->blockSignals(false);
        }
    update();
}

/* Con 12 marcados, los demás no se pueden marcar; sin ninguno, no se convierte. */
void ChannelsDialog::update()
{
    int n = 0;
    for (QCheckBox *b : boxes_)
        n += b && b->isChecked();
    for (QCheckBox *b : boxes_)
        if (b)
            b->setEnabled(b->isChecked() || n < 12);
    count_->setText(tr("Marcados: %1 de 12").arg(n));
    ok_->setEnabled(n > 0);
}
