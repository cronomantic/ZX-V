/*
 * channels.h - Elegir los canales de un MOD, un S3M o un XM de más de 12
 * canales con notas: ZX-V tiene 12 canales de muestras. Vienen marcados los
 * que elige el conversor (los que más notas tocan); se pueden marcar otros,
 * hasta 12.
 */
#ifndef ZXV_TRACKER_CHANNELS_H
#define ZXV_TRACKER_CHANNELS_H

#include <QDialog>

#include <cstdint>
#include <vector>

#include "tune.h"

class QCheckBox;
class QLabel;
class QPushButton;

class ChannelsDialog : public QDialog
{
    Q_OBJECT

public:
    ChannelsDialog(const QString &name, const trk::ModuleChannels &ch, QWidget *parent = nullptr);
    /* Los canales marcados: el bit c, el canal c del módulo (desde 0). */
    uint64_t channels() const;
    /* Marca esos (para las pruebas). */
    void setChannels(uint64_t set);

private:
    void update();

    trk::ModuleChannels ch_;
    std::vector<QCheckBox *> boxes_;    /* la del canal c, o nullptr si no tiene nada */
    QLabel *count_;
    QPushButton *ok_;
};

#endif
