/*
 * mainwindow.h - La ventana de zxv-tracker: los patrones con la cabecera y
 * las posiciones, los samples, los ornamentos y los efectos AYFX; tocar,
 * los menús y los ficheros.
 */
#ifndef ZXV_TRACKER_MAINWINDOW_H
#define ZXV_TRACKER_MAINWINDOW_H

#include <QMainWindow>

#include <functional>
#include <memory>

#include "engine.h"

class Audio;
class Editor;
class LineGrid;
class OrnamentView;
class PatternView;
class Piano;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QSettings;
class QSpinBox;
class QTabWidget;
class SampleView;
class SfxView;

namespace trk {
struct Cell;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum Tab { Patterns, Samples, Ornaments, Effects };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    /* Un módulo (.pt3) o un banco de efectos (.afb, .afx). */
    bool open(const QString &path);
    void showTab(int tab);
    bool exportWav(const QString &path);

protected:
    void closeEvent(QCloseEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    QWidget *buildPatternTab();
    QWidget *buildSampleTab();
    QWidget *buildOrnamentTab();
    QWidget *buildEffectsTab();
    void buildActions();
    QAction *action(const QString &text, const QKeySequence &key, const std::function<void()> &fn);

    void newSong();
    void openDialog();
    bool save(const QString &path);
    bool saveAs();
    bool maybeSave();
    void exportWavDialog();
    void newBank();
    bool openBank(const QString &path);
    bool saveBank(const QString &path);
    bool saveBankAs();
    bool maybeSaveBank();
    void importAfx();
    void exportAfx();
    void addRecent(const QString &path);
    void rebuildRecent();

    void play(int pos, int row, bool patternOnly);
    void stop();
    void tick();
    void previewCell(const trk::Cell &c, int envPeriod, int channel);
    void previewNote(int note, bool withOrnament);
    void previewOff();
    void playEffect();
    void updateMute();

    void refreshHeader();
    void refreshPositions();
    void refreshSample();
    void refreshOrnament();
    void refreshEffects();
    void updateState();
    void editHeader(const std::function<void()> &fn);
    void setTurboSound(bool on);
    void addPosition();
    void removePosition();
    void clonePattern();
    void setLoop();
    void setPatternRows(int rows);
    void resizeSample(int lines);
    void resizeOrnament(int lines);
    void about();
    void keysHelp();
    void message(const QString &text);

    Editor *ed_;
    QSettings *settings_;
    trk::Engine engine_;
    std::unique_ptr<Audio> audio_;
    QTabWidget *tabs_;
    PatternView *pattern_;
    SampleView *sampleView_;
    OrnamentView *ornamentView_;
    SfxView *sfxView_;
    Piano *samplePiano_, *ornamentPiano_;

    QLineEdit *title_, *author_;
    QSpinBox *tempo_, *patNum_, *patRows_;
    QComboBox *table_, *version_, *module_;
    QCheckBox *turbo_;
    QListWidget *positions_;
    QSpinBox *octave_, *step_, *sample_, *ornament_;
    QSpinBox *sampleNum_, *sampleLen_, *sampleLoop_, *ornNum_, *ornLen_, *ornLoop_;
    QLabel *sampleInfo_, *ornInfo_, *sfxInfo_;
    QListWidget *effects_;
    QAction *undo_, *redo_, *cut_, *copy_, *paste_, *selectAll_, *follow_;
    QAction *mute_[3];
    QList<QAction *> patternOnly_;
    QMenu *recentMenu_;
    QLabel *where_, *size_;
    bool refreshing_ = false;
    bool playing_ = false;
};

#endif // ZXV_TRACKER_MAINWINDOW_H
