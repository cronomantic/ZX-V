/*
 * editor.h - Lo que se edita en zxv-tracker y por dónde: la canción (tune.h),
 * el banco de efectos, la posición, el AY cuyos samples y ornamentos se ven,
 * el sample, el ornamento y el instrumento elegidos, y deshacer (por partes:
 * un patrón, un sample... para no copiar la canción entera en cada tecla).
 *
 * Los grupos de una posición: 0 y 1 los AY (sus tres canales, con la
 * envolvente y el ruido), 2 en adelante las pistas de muestras.
 */
#ifndef ZXV_TRACKER_EDITOR_H
#define ZXV_TRACKER_EDITOR_H

#include <QObject>
#include <QString>

#include <memory>
#include <vector>

#include "sfx.h"
#include "tune.h"

class Editor : public QObject
{
    Q_OBJECT

public:
    /* Lo que cambia una edición (y guarda deshacer). */
    enum Part { Header, AyPatternPart, SmpPatternPart, SamplePart, OrnamentPart, Everything };
    static constexpr int SMP_GROUP = 2;     /* el grupo de la primera pista de muestras */

    explicit Editor(QObject *parent = nullptr);

    trk::Tune &tune() { return tune_; }
    const trk::Tune &tune() const { return tune_; }
    int chip() const { return chip_; }
    trk::AyBank &bank() { return tune_.ay[size_t(chip_)]; }
    const trk::AyBank &bank() const { return tune_.ay[size_t(chip_)]; }
    int position() const { return position_; }
    trk::Position &pos() { return tune_.order[size_t(position_)]; }
    const trk::Position &pos() const { return tune_.order[size_t(position_)]; }
    int rows() const { return pos().rows; }
    int sample() const { return sample_; }
    int ornament() const { return ornament_; }
    int instrument() const { return instrument_; }

    /* Los grupos de la canción (los AY que usa y sus pistas de muestras). */
    int groups() const { return SMP_GROUP + tune_.smpTracks; }
    bool hasGroup(int g) const { return g < SMP_GROUP ? g < tune_.chips : g - SMP_GROUP < tune_.smpTracks; }
    /* El patrón de un grupo en la posición, o -1. */
    int patternOf(int group) const;
    const trk::Pattern *ayPattern(int chip) const;
    const trk::SmpPattern *smpPattern(int track) const;
    /* Antes de cambiar el patrón de un grupo: si no tiene, uno nuevo (vacío),
     * y con las filas de la posición; lo guarda para deshacer (con record;
     * sin él, quien llama ya ha hecho begin(Everything): lo que cambia varios
     * grupos a la vez). Luego se cambia con ayEdit o smpEdit, y changed(). */
    void beginGroup(int group, bool record = true);
    trk::Pattern &ayEdit(int chip);
    trk::SmpPattern &smpEdit(int track);

    void setChip(int c);
    void setPosition(int p);
    void setSample(int s);
    void setOrnament(int o);
    void setInstrument(int i);

    /* Antes de cambiar algo: lo guarda para deshacer. a: el AY (samples,
     * ornamentos, patrones del AY) o la pista; index: el patrón, el sample
     * o el ornamento. */
    void begin(Part part, int a = 0, int index = -1);
    /* Después: avisa a las vistas. */
    void changed(Part part);
    void undo();
    void redo();
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }

    void setTune(const trk::Tune &t, const QString &path, trk::TuneFormat format);
    QString path() const { return path_; }
    trk::TuneFormat format() const { return format_; }
    bool modified() const { return modified_; }
    void setSaved(const QString &path, trk::TuneFormat format);

    /* El banco de efectos AYFX: su propio fichero y su deshacer. */
    fx::Bank &fxBank() { return bankFx_; }
    int effect() const { return effect_; }
    void setEffect(int e);
    void beginBank();
    void endBank();
    void undoBank();
    void redoBank();
    bool canUndoBank() const { return !bankUndo_.empty(); }
    bool canRedoBank() const { return !bankRedo_.empty(); }
    void setBank(const fx::Bank &b, const QString &path);
    QString bankPath() const { return bankPath_; }
    bool bankModified() const { return bankModified_; }
    void setBankSaved(const QString &path);

signals:
    void songChanged();             /* cualquier cambio: las vistas se repintan */
    void positionChanged();         /* otra posición, otro AY */
    void sampleChanged();
    void ornamentChanged();
    void bankChanged();
    void effectChanged();
    void stateChanged();            /* deshacer, guardado... */

private:
    /* Lo de la canción que no son patrones, samples ni muestras. */
    struct HeaderData {
        std::string title, author;
        int speed, tempo, loop, chips, smpTracks, flags;
        std::array<uint8_t, trk::SMP_TRACKS> pan;
        std::vector<trk::Position> order;
        int version[2], table[2];
    };
    struct Step {
        Part part;
        int a, index, position, chip;
        trk::Pattern pattern;
        trk::SmpPattern smpPattern;
        trk::Sample sample;
        trk::Ornament ornament;
        HeaderData header;
        std::shared_ptr<trk::Tune> tune;    /* solo con Everything */
    };
    Step snapshot(Part part, int a, int index) const;
    void restore(const Step &s);
    void clamp();

    trk::Tune tune_;
    int chip_ = 0, position_ = 0, sample_ = 1, ornament_ = 0, instrument_ = 1;
    std::vector<Step> undo_, redo_;
    QString path_;
    trk::TuneFormat format_ = trk::FORMAT_NONE;
    bool modified_ = false;

    fx::Bank bankFx_;
    int effect_ = 0;
    std::vector<fx::Bank> bankUndo_, bankRedo_;
    QString bankPath_;
    bool bankModified_ = false;
};

#endif // ZXV_TRACKER_EDITOR_H
