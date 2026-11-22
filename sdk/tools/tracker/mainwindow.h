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
#include "tune.h"

class Audio;
class AyResult;
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
class QProgressBar;
class QMenu;
class QSettings;
class QSpinBox;
class QTabWidget;
class SampleView;
class EnvelopeView;
class NoteMapView;
class SfxView;
class WaveView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum Tab { Patterns, Samples, Ornaments, Waves, SmpInstruments, Effects };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    /* Una canción (.zxm, .pt3, .mod, .s3m, .xm) o un banco de efectos (.afb, .afx).
     * De un módulo con más de 12 canales con notas, channels dice cuáles van
     * (el bit c, el canal c); con 0, si ask, los pregunta (ChannelsDialog) y,
     * si no, van los que más notas tocan. */
    bool open(const QString &path, uint64_t channels = 0, bool ask = true);
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
    QWidget *buildWaveTab();
    QWidget *buildSmpInsTab();
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
    void showWarnings(const QString &title, const QString &what, const trk::Warnings &w);

    void play(int pos, int row, bool patternOnly);
    void stop();
    void tick();
    void previewCell(const trk::Cell &c, int envPeriod, int chip, int channel);
    void previewSample(const trk::SmpCell &c, int track);
    void previewNote(int note, bool withOrnament);
    void previewOff();
    void playEffect();

    void refreshHeader();
    void refreshPositions();
    void refreshSample();
    void refreshOrnament();
    void refreshEffects();
    void updateState();
    void editHeader(const std::function<void()> &fn);
    void setChips(int chips);
    void setSmpTracks(int tracks);
    void addPosition();
    void removePosition();
    void clonePattern();
    void setLoop();
    void setGroupPattern(int pattern);
    void setPositionRows(int rows);
    void refreshWaves();
    void refreshSmpIns();
    void editSmpIns(const std::function<void(trk::SmpInstrument &)> &fn);
    void removeSmpIns();
    void editWave(const std::function<void(trk::Wave &)> &fn);
    void importWavs();
    void removeWave();
    void previewWave(int note);
    void resizeSample(int lines);
    void resizeOrnament(int lines);
    QString groupName(int group) const;
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
    AyResult *result_;
    OrnamentView *ornamentView_;
    SfxView *sfxView_;
    WaveView *waveView_;
    QListWidget *waves_;
    QComboBox *waveFormat_;
    QSpinBox *waveRel_, *waveFine_, *waveVol_, *wavePanV_, *waveLoopFrom_;
    QCheckBox *wavePan_, *waveLoop_;
    QLabel *waveInfo_;
    QProgressBar *soundMem_;
    Piano *wavePiano_;
    int waveSel_ = 0;
    QListWidget *smpIns_;
    QSpinBox *smpPaint_;
    NoteMapView *noteMap_;
    EnvelopeView *volEnv_, *panEnv_;
    QCheckBox *volOn_, *volSus_, *volLoop_, *panOn_, *panSus_, *panLoop_;
    QSpinBox *volSusP_, *volFrom_, *volTo_, *panSusP_, *panFrom_, *panTo_;
    QSpinBox *fade_, *vibSweep_, *vibDepth_, *vibRate_;
    QComboBox *vibType_;
    Piano *insPiano_;
    Piano *samplePiano_, *ornamentPiano_;

    QLineEdit *title_, *author_;
    QSpinBox *speed_, *tempo_, *smpTracks_, *patNum_, *patRows_;
    QComboBox *table_, *version_, *chips_, *chip_, *templates_;
    QLabel *patLabel_;
    QListWidget *positions_;
    QSpinBox *octave_, *step_, *sample_, *ornament_, *instrument_;
    QSpinBox *sampleNum_, *sampleLen_, *sampleLoop_, *ornNum_, *ornLen_, *ornLoop_;
    QLabel *sampleInfo_, *ornInfo_, *sfxInfo_;
    QListWidget *effects_;
    QAction *undo_, *redo_, *cut_, *copy_, *paste_, *selectAll_, *follow_;
    QList<QAction *> patternOnly_;
    QMenu *recentMenu_;
    QLabel *where_, *size_;
    bool refreshing_ = false;
    bool playing_ = false;
};

#endif // ZXV_TRACKER_MAINWINDOW_H
