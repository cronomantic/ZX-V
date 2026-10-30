/*
 * mainwindow.h - La ventana de zxv-gfx: las pestañas de cada clase de
 * objeto, el lienzo, los paneles, los menús y los ficheros.
 */
#ifndef ZXV_GFX_MAINWINDOW_H
#define ZXV_GFX_MAINWINDOW_H

#include <QMainWindow>

#include "editor.h"

class MapCanvas;
class PixelCanvas;
class QActionGroup;
class QDockWidget;
class QLabel;
class QMenu;
class QScrollArea;
class QSettings;
class QStackedWidget;
class QTabBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    bool open(const QString &path);
    void showSection(Section s);

protected:
    void closeEvent(QCloseEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    void buildActions();
    void buildDocks();
    QAction *action(const QString &text, const QKeySequence &key, const std::function<void()> &fn);
    bool save(const QString &path);
    bool saveAs();
    bool maybeSave();
    void openDialog();
    void importImage(const QString &path);
    void importDialog();
    void exportImage();
    void addRecent(const QString &path);
    void rebuildRecent();
    void updateState();
    void updateSection();
    void copy(bool cut);
    void pasteClipboard();
    void transform(gfx::Image (*fn)(const gfx::Image &));
    void about();
    void message(const QString &text);

    Editor *ed_;
    QSettings *settings_;
    QTabBar *tabs_;
    QStackedWidget *stack_;
    PixelCanvas *pixel_;
    MapCanvas *map_;
    QScrollArea *pixelArea_, *mapArea_;
    QDockWidget *objectsDock_, *paletteDock_, *propsDock_, *previewDock_, *tilesDock_;
    QActionGroup *tools_;
    QAction *undo_, *redo_, *cut_, *copy_, *paste_, *erase_, *selectAll_, *grid_, *front_, *mode7_;
    QList<QAction *> imageOnly_;         /* lo que no vale en los mapas */
    QMenu *recentMenu_;
    QLabel *hover_, *zoom_, *memory_;
};

#endif // ZXV_GFX_MAINWINDOW_H
