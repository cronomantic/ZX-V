/*
 * editor.cpp - La canción, el banco de efectos y deshacer de zxv-tracker.
 */
#include "editor.h"

#include <algorithm>

using namespace trk;

static constexpr size_t UNDO_STEPS = 500;

Editor::Editor(QObject *parent) : QObject(parent)
{
    bank_.effects.resize(1);
}

int Editor::pattern() const
{
    const Module &m = mod();
    if (m.order.empty())
        return 0;
    return std::clamp(m.order[size_t(std::clamp(position_, 0, int(m.order.size()) - 1))], 0, MAX_PATTERNS - 1);
}

void Editor::clamp()
{
    module_ = std::clamp(module_, 0, int(song_.modules.size()) - 1);
    position_ = std::clamp(position_, 0, std::max(0, int(mod().order.size()) - 1));
    sample_ = std::clamp(sample_, 1, SAMPLES - 1);
    ornament_ = std::clamp(ornament_, 0, ORNAMENTS - 1);
    effect_ = std::clamp(effect_, 0, std::max(0, int(bank_.effects.size()) - 1));
}

void Editor::setModule(int m)
{
    m = std::clamp(m, 0, int(song_.modules.size()) - 1);
    if (m == module_)
        return;
    module_ = m;
    clamp();
    emit positionChanged();
}

void Editor::setPosition(int p)
{
    p = std::clamp(p, 0, std::max(0, int(mod().order.size()) - 1));
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

Editor::Step Editor::snapshot(Part part, int module, int index) const
{
    Step s{ part, module, index, position_, {}, {}, {}, {}, nullptr };
    const Module &m = song_.modules[size_t(module)];
    switch (part) {
    case PatternPart:
        s.pattern = m.patterns[size_t(index)];
        break;
    case SamplePart:
        s.sample = m.samples[size_t(index)];
        break;
    case OrnamentPart:
        s.ornament = m.ornaments[size_t(index)];
        break;
    case Header:
        s.header = { m.title, m.author, m.version, m.table, m.delay, m.loop, m.order };
        break;
    case Everything:
        s.song = std::make_shared<Song>(song_);
        break;
    }
    return s;
}

void Editor::restore(const Step &s)
{
    if (s.part == Everything) {
        song_ = *s.song;
    } else {
        Module &m = song_.modules[size_t(s.module)];
        switch (s.part) {
        case PatternPart:
            m.patterns[size_t(s.index)] = s.pattern;
            break;
        case SamplePart:
            m.samples[size_t(s.index)] = s.sample;
            break;
        case OrnamentPart:
            m.ornaments[size_t(s.index)] = s.ornament;
            break;
        case Header:
            m.title = s.header.title;
            m.author = s.header.author;
            m.version = s.header.version;
            m.table = s.header.table;
            m.delay = s.header.delay;
            m.loop = s.header.loop;
            m.order = s.header.order;
            break;
        default:
            break;
        }
        module_ = s.module;
    }
    position_ = s.position;
    if (s.part == SamplePart)
        sample_ = std::max(1, s.index);
    if (s.part == OrnamentPart)
        ornament_ = s.index;
    clamp();
}

void Editor::begin(Part part, int index)
{
    undo_.push_back(snapshot(part, module_, index));
    if (undo_.size() > UNDO_STEPS)
        undo_.erase(undo_.begin());
    redo_.clear();
}

void Editor::changed(Part)
{
    modified_ = true;
    emit songChanged();
    emit stateChanged();
}

void Editor::undo()
{
    if (undo_.empty())
        return;
    Step s = undo_.back();
    undo_.pop_back();
    redo_.push_back(snapshot(s.part, s.part == Everything ? module_ : s.module, s.index));
    redo_.back().position = position_;
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
    undo_.push_back(snapshot(s.part, s.part == Everything ? module_ : s.module, s.index));
    undo_.back().position = position_;
    restore(s);
    modified_ = true;
    emit positionChanged();
    emit songChanged();
    emit sampleChanged();
    emit ornamentChanged();
    emit stateChanged();
}

void Editor::setSong(const Song &s, const QString &path)
{
    song_ = s;
    path_ = path;
    modified_ = false;
    undo_.clear();
    redo_.clear();
    module_ = position_ = 0;
    clamp();
    emit positionChanged();
    emit songChanged();
    emit sampleChanged();
    emit ornamentChanged();
    emit stateChanged();
}

void Editor::setSaved(const QString &path)
{
    path_ = path;
    modified_ = false;
    emit stateChanged();
}

std::vector<std::vector<int>> Editor::rows() const
{
    std::vector<std::vector<int>> r;
    for (const Module &m : song_.modules) {
        std::vector<int> v;
        for (int p : m.order)
            v.push_back(int(m.patterns[size_t(std::clamp(p, 0, MAX_PATTERNS - 1))].rows.size()));
        r.push_back(v);
    }
    return r;
}

/* --- Los efectos ------------------------------------------------------------ */

void Editor::setEffect(int e)
{
    e = std::clamp(e, 0, std::max(0, int(bank_.effects.size()) - 1));
    if (e == effect_)
        return;
    effect_ = e;
    emit effectChanged();
}

void Editor::beginBank()
{
    bankUndo_.push_back(bank_);
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
    bankRedo_.push_back(bank_);
    bank_ = bankUndo_.back();
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
    bankUndo_.push_back(bank_);
    bank_ = bankRedo_.back();
    bankRedo_.pop_back();
    bankModified_ = true;
    clamp();
    emit bankChanged();
    emit effectChanged();
    emit stateChanged();
}

void Editor::setBank(const fx::Bank &b, const QString &path)
{
    bank_ = b;
    if (bank_.effects.empty())
        bank_.effects.resize(1);
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
