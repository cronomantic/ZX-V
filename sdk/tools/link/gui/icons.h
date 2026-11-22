/*
 * icons.h - Los iconos de zxv-link-gui y el logo de ZX-V, dibujados aquí con
 * los colores de la máquina (los de la franja del logo), sin Qt SVG.
 */
#ifndef ZXV_LINK_ICONS_H
#define ZXV_LINK_ICONS_H

#include <QIcon>
#include <QPixmap>
#include <QWidget>

enum class Ic {
    Play,           /* ejecutar */
    Copy,           /* solo copiar */
    Folder,         /* elegir */
    Add,            /* añadir */
    Refresh,        /* buscar los puertos */
    Plug,           /* conectar / desconectar */
    Program,        /* un programa (los recientes) */
    File,           /* un fichero que va con él */
    Done,           /* mandado */
    Same,           /* ya estaba igual */
    Sending,        /* mandándose */
    Error,          /* falló */
    Clear,          /* limpiar */
    Warning         /* cuidado: se preguntará */
};
QIcon icon(Ic kind);

/* El punto del estado de la placa. */
enum class Dot { Off, Busy, On, Error };
QPixmap dot(Dot kind, int size = 12);

/* El número de cada paso, en un círculo azul. */
QPixmap stepBadge(int n, int size = 24);

/* El logo de ZX-V (las letras, en el color del texto, y la franja). */
class Logo : public QWidget
{
public:
    explicit Logo(int height, QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;

private:
    int h;
};

#endif // ZXV_LINK_ICONS_H
