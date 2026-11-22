/*
 * editor.cpp - La canción, el banco de efectos y deshacer de zxv-tracker.
 */
#include "editor.h"

#include <algorithm>

using namespace trk;

static constexpr size_t UNDO_STEPS = 500;

Editor::Editor(QObject *parent) : QObject(parent)
{
    bankFx_.effects.resize(1);
}

int Editor::patternOf(int group) const
{
    if (!hasGroup(group))
        return -1;
    const Position &p = pos();
    return group < SMP_GROUP ? p.ay[size_t(group)] : p.smp[size_t(group - SMP_GROUP)];
}

const Pattern *Editor::ayPattern(int chip) const
{
    int p = patternOf(chip);
    const std::vector<Pattern> &pool = tune_.ayPatterns[size_t(chip & 1)];
    return p >= 0 && p < int(pool.size()) ? &pool[size_t(p)] : nullptr;
}

const SmpPattern *Editor::smpPattern(int track) const
{
    int p = patternOf(SMP_GROUP + track);
    return p >= 0 && p < int(tune_.smpPatterns.size()) ? &tune_.smpPatterns[size_t(p)] : nullptr;
}

void Editor::beginGroup(int group, bool record)
{
    int p = patternOf(group);
    bool ay = group < SMP_GROUP;
    size_t pool = ay ? tune_.ayPatterns[size_t(group)].size() : tune_.smpPatterns.size();
    if (p < 0 || p >= int(pool)) {
        /* Sin patrón: uno nuevo, en el primer hueco vacío que no use nadie. */
        if (record)
            begin(Everything);
        std::vector<bool> used(std::max<size_t>(pool, 1));
        for (const Position &o : tune_.order) {
            if (ay && o.ay[size_t(group)] >= 0 && o.ay[size_t(group)] < int(pool))
                used[size_t(o.ay[size_t(group)])] = true;
            if (!ay)
                for (int s : o.smp)
                    if (s >= 0 && s < int(pool))
                        used[size_t(s)] = true;
        }
        int free = -1;
        for (size_t i = 0; i < pool && free < 0; i++)
            if (!used[i] && (ay ? tune_.ayPatterns[size_t(group)][i].empty() : tune_.smpPatterns[i] == SmpPattern()))
                free = int(i);
        if (free < 0) {
            free = int(pool);
            if (ay)
                tune_.ayPatterns[size_t(group)].emplace_back();
            else
                tune_.smpPatterns.emplace_back();
        }
        if (ay)
            pos().ay[size_t(group)] = free;
        else
            pos().smp[size_t(group - SMP_GROUP)] = free;
        p = free;
    } else if (record) {
        begin(ay ? AyPatternPart : SmpPatternPart, ay ? group : group - SMP_GROUP, p);
    }
    /* Con las filas de la posición, por lo menos. */
    size_t rows = size_t(pos().rows);
    if (ay) {
        Pattern &pat = tune_.ayPatterns[size_t(group)][size_t(p)];
        if (pat.rows.size() < rows)
            pat.rows.resize(rows);
    } else {
        SmpPattern &pat = tune_.smpPatterns[size_t(p)];
        if (pat.rows.size() < rows)
            pat.rows.resize(rows);
    }
}

Pattern &Editor::ayEdit(int chip)
{
    return tune_.ayPatterns[size_t(chip & 1)][size_t(patternOf(chip))];
}

SmpPattern &Editor::smpEdit(int track)
{
    return tune_.smpPatterns[size_t(patternOf(SMP_GROUP + track))];
}

void Editor::clamp()
{
    if (tune_.order.empty())
        tune_.order.resize(1);
    chip_ = std::clamp(chip_, 0, std::max(0, tune_.chips - 1));
    position_ = std::clamp(position_, 0, int(tune_.order.size()) - 1);
    sample_ = std::clamp(sample_, 1, SAMPLES - 1);
    ornament_ = std::clamp(ornament_, 0, ORNAMENTS - 1);
    instrument_ = std::clamp(instrument_, 1, 128);
    effect_ = std::clamp(effect_, 0, std::max(0, int(bankFx_.effects.size()) - 1));
}

void Editor::setChip(int c)
{
    c = std::clamp(c, 0, std::max(0, tune_.chips - 1));
    if (c == chip_)
        return;
    chip_ = c;
    emit positionChanged();
    emit sampleChanged();
    emit ornamentChanged();
}

void Editor::setPosition(int p)
{
    p = std::clamp(p, 0, int(tune_.order.size()) - 1);
    if (p == position_)
        return;
    position_ = p;
    emit positionChanged();
}

void Editor::setSample(int s)
{
    s = std::clamp(s, 1, SAMPLES - 1);
    if (s == sample_)
        return;
    sample_ = s;
    emit sampleChanged();
}

void Editor::setOrnament(int o)
{
    o = std::clamp(o, 0, ORNAMENTS - 1);
    if (o == ornament_)
        return;
    ornament_ = o;
    emit ornamentChanged();
}

void Editor::setInstrument(int i)
{
    instrument_ = std::clamp(i, 1, 128);
}

Editor::Step Editor::snapshot(Part part, int a, int index) const
{
    Step s{ part, a, index, position_, chip_, {}, {}, {}, {}, {}, nullptr };
    switch (part) {
    case AyPatternPart:
        s.pattern = tune_.ayPatterns[size_t(a)][size_t(index)];
        break;
    case SmpPatternPart:
        s.smpPattern = tune_.smpPatterns[size_t(index)];
        break;
    case SamplePart:
        s.sample = tune_.ay[size_t(a)].samples[size_t(index)];
        break;
    case OrnamentPart:
        s.ornament = tune_.ay[size_t(a)].ornaments[size_t(index)];
        break;
    case Header:
        s.header = { tune_.title, tune_.author, tune_.speed, tune_.tempo, tune_.loop, tune_.chips, tune_.smpTracks,
                     tune_.flags, tune_.pan, tune_.order, { tune_.ay[0].version, tune_.ay[1].version },
                     { tune_.ay[0].table, tune_.ay[1].table } };
        break;
    case Everything:
        s.tune = std::make_shared<Tune>(tune_);
        break;
    }
    return s;
}

void Editor::restore(const Step &s)
{
    switch (s.part) {
    case Everything:
        tune_ = *s.tune;
        break;
    case AyPatternPart:
        tune_.ayPatterns[size_t(s.a)][size_t(s.index)] = s.pattern;
        break;
    case SmpPatternPart:
        tune_.smpPatterns[size_t(s.index)] = s.smpPattern;
        break;
    case SamplePart:
        tune_.ay[size_t(s.a)].samples[size_t(s.index)] = s.sample;
        break;
    case OrnamentPart:
        tune_.ay[size_t(s.a)].ornaments[size_t(s.index)] = s.ornament;
        break;
    case Header: {
        const HeaderData &h = s.header;
        tune_.title = h.title;
        tune_.author = h.author;
        tune_.speed = h.speed;
        tune_.tempo = h.tempo;
        tune_.loop = h.loop;
        tune_.chips = h.chips;
        tune_.smpTracks = h.smpTracks;
        tune_.flags = h.flags;
        tune_.pan = h.pan;
        tune_.order = h.order;
        for (int c = 0; c < 2; c++) {
            tune_.ay[size_t(c)].version = h.version[c];
            tune_.ay[size_t(c)].table = h.table[c];
        }
        break;
    }
    }
    position_ = s.position;
    chip_ = s.chip;
    if (s.part == SamplePart)
        sample_ = std::max(1, s.index);
    if (s.part == OrnamentPart)
        ornament_ = s.index;
    clamp();
}

void Editor::begin(Part part, int a, int index)
{
    undo_.push_back(snapshot(part, a, index));
    if (undo_.size() > UNDO_STEPS)
        undo_.erase(undo_.begin());
    redo_.clear();
}

void Editor::changed(Part)
{
    modified_ = true;
    clamp();
    emit songChanged();
    emit stateChanged();
}

void Editor::undo()
{
    if (undo_.empty())
        return;
    Step s = undo_.back();
    undo_.pop_back();
    redo_.push_back(snapshot(s.part, s.a, s.index));
    redo_.back().position = position_;
    redo_.back().chip = chip_;
    restore(s);
    modified_ = true;
    emit positionChanged();
    emit songChanged();
    emit sampleChanged();
    emit ornamentChanged();
    emit stateChanged();
}

void Editor::redo()
{
    if (redo_.empty())
        return;
    Step s = redo_.back();
    redo_.pop_back();
    undo_.push_back(snapshot(s.part, s.a, s.index));
    undo_.back().position = position_;
    undo_.back().chip = chip_;
    restore(s);
    modified_ = true;
    emit positionChanged();
    emit songChanged();
    emit sampleChanged();
    emit ornamentChanged();
    emit stateChanged();
}

void Editor::setTune(const Tune &t, const QString &path, TuneFormat format)
{
    tune_ = t;
    path_ = path;
    format_ = format;
    modified_ = false;
    undo_.clear();
    redo_.clear();
    chip_ = position_ = 0;
    clamp();
    emit positionChanged();
    emit songChanged();
    emit sampleChanged();
    emit ornamentChanged();
    emit stateChanged();
}

void Editor::setSaved(const QString &path, TuneFormat format)
{
    path_ = path;
    format_ = format;
    modified_ = false;
    emit stateChanged();
}

/* --- Los efectos ------------------------------------------------------------ */

void Editor::setEffect(int e)
{
    e = std::clamp(e, 0, std::max(0, int(bankFx_.effects.size()) - 1));
    if (e == effect_)
        return;
    effect_ = e;
    emit effectChanged();
}

void Editor::beginBank()
{
    bankUndo_.push_back(bankFx_);
    if (bankUndo_.size() > UNDO_STEPS)
        bankUndo_.erase(bankUndo_.begin());
    bankRedo_.clear();
}

void Editor::endBank()
{
    bankModified_ = true;
    clamp();
    emit bankChanged();
    emit stateChanged();
}

void Editor::undoBank()
{
    if (bankUndo_.empty())
        return;
    bankRedo_.push_back(bankFx_);
    bankFx_ = bankUndo_.back();
    bankUndo_.pop_back();
    bankModified_ = true;
    clamp();
    emit bankChanged();
    emit effectChanged();
    emit stateChanged();
}

void Editor::redoBank()
{
    if (bankRedo_.empty())
        return;
    bankUndo_.push_back(bankFx_);
    bankFx_ = bankRedo_.back();
    bankRedo_.pop_back();
    bankModified_ = true;
    clamp();
    emit bankChanged();
    emit effectChanged();
    emit stateChanged();
}

void Editor::setBank(const fx::Bank &b, const QString &path)
{
    bankFx_ = b;
    if (bankFx_.effects.empty())
        bankFx_.effects.resize(1);
    bankPath_ = path;
    bankModified_ = false;
    bankUndo_.clear();
    bankRedo_.clear();
    effect_ = 0;
    emit bankChanged();
    emit effectChanged();
    emit stateChanged();
}

void Editor::setBankSaved(const QString &path)
{
    bankPath_ = path;
    bankModified_ = false;
    emit stateChanged();
}
