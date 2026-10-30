/*
 * editor.h - Lo que se edita en zxv-tracker y por dónde: la canción, el
 * banco de efectos, el módulo, la posición y el sample y el ornamento
 * elegidos, y deshacer (por partes: un patrón, un sample... para no copiar
 * la canción entera en cada tecla).
 */
#ifndef ZXV_TRACKER_EDITOR_H
#define ZXV_TRACKER_EDITOR_H

#include <QObject>
#include <QString>

#include <memory>
#include <vector>

#include "sfx.h"
#include "song.h"

class Editor : public QObject
{
    Q_OBJECT

public:
    /* Lo que cambia una edición (y guarda deshacer). */
    enum Part { Header, PatternPart, SamplePart, OrnamentPart, Everything };

    explicit Editor(QObject *parent = nullptr);

    trk::Song &song() { return song_; }
    trk::Module &mod() { return song_.modules[size_t(module_)]; }
    const trk::Module &mod() const { return song_.modules[size_t(module_)]; }
    int module() const { return module_; }
    int position() const { return position_; }
    int pattern() const;                        /* el de la posición */
    trk::Pattern &pat() { return mod().patterns[size_t(pattern())]; }
    int sample() const { return sample_; }
    int ornament() const { return ornament_; }

    void setModule(int m);
    void setPosition(int p);
    void setSample(int s);
    void setOrnament(int o);

    /* Antes de cambiar algo: lo guarda para deshacer. index: el patrón, el
     * sample o el ornamento (del módulo actual). */
    void begin(Part part, int index = -1);
    /* Después: avisa a las vistas. */
    void changed(Part part);
    void undo();
    void redo();
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }

    void setSong(const trk::Song &s, const QString &path);
    QString path() const { return path_; }
    bool modified() const { return modified_; }
    void setSaved(const QString &path);
    /* Las filas de cada posición, por módulo (para Engine::Play). */
    std::vector<std::vector<int>> rows() const;

    /* El banco de efectos AYFX: su propio fichero y su deshacer. */
    fx::Bank &bank() { return bank_; }
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
    void positionChanged();         /* otro módulo, otra posición */
    void sampleChanged();
    void ornamentChanged();
    void bankChanged();
    void effectChanged();
    void stateChanged();            /* deshacer, guardado... */

private:
    struct HeaderData {
        std::string title, author;
        int version, table, delay, loop;
        std::vector<int> order;
    };
    struct Step {
        Part part;
        int module, index, position;
        trk::Pattern pattern;
        trk::Sample sample;
        trk::Ornament ornament;
        HeaderData header;
        std::shared_ptr<trk::Song> song;    /* solo con Everything */
    };
    Step snapshot(Part part, int module, int index) const;
    void restore(const Step &s);
    void clamp();

    trk::Song song_;
    int module_ = 0, position_ = 0, sample_ = 1, ornament_ = 0;
    std::vector<Step> undo_, redo_;
    QString path_;
    bool modified_ = false;

    fx::Bank bank_;
    int effect_ = 0;
    std::vector<fx::Bank> bankUndo_, bankRedo_;
    QString bankPath_;
    bool bankModified_ = false;
};

#endif // ZXV_TRACKER_EDITOR_H
