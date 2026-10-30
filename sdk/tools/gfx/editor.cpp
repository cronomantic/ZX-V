/*
 * editor.cpp - Lo que se edita en zxv-gfx (ver editor.h).
 */
#include "editor.h"

#include <algorithm>

/* Lo que se recuerda para deshacer: hasta estos pasos. */
static const size_t UNDO_STEPS = 500;

Editor::Editor(QObject *parent) : QObject(parent) {}

int Editor::count(Section s) const
{
    switch (s) {
    case Section::Sprites: return gfx::SPRITES;
    case Section::Tiles8: return int(doc_.tiles8.size());
    case Section::Tiles16: return int(doc_.tiles16.size());
    case Section::Maps: return gfx::MAPS;
    case Section::Textures: return gfx::TEXTURES;
    }
    return 0;
}

void Editor::select(Section s, int index)
{
    int n = count(s);
    index = n ? std::clamp(index, 0, n - 1) : 0;
    if (s == section_ && index == index_[int(s)])
        return;
    section_ = s;
    index_[int(s)] = index;
    emit selectionChanged();
}

gfx::Image *Editor::image()
{
    int i = index();
    switch (section_) {
    case Section::Sprites:
        return doc_.sprites[i].empty() ? nullptr : &doc_.sprites[i];
    case Section::Tiles8:
    case Section::Tiles16: {
        auto &set = doc_.tiles(tileSide());
        return i < int(set.size()) ? &set[i] : nullptr;
    }
    case Section::Textures:
        return doc_.textures[i].empty() ? nullptr : &doc_.textures[i].img;
    case Section::Maps:
        break;
    }
    return nullptr;
}

gfx::Map *Editor::map()
{
    if (section_ != Section::Maps || doc_.maps[index()].empty())
        return nullptr;
    return &doc_.maps[index()];
}

void Editor::setColor(int button, int c)
{
    if (color_[button] == c)
        return;
    color_[button] = c;
    emit colorsChanged();
}

void Editor::setTool(Tool t)
{
    if (tool_ == t)
        return;
    tool_ = t;
    emit toolChanged();
}

void Editor::setMapTile(int t)
{
    if (mapTile_ == t)
        return;
    mapTile_ = t;
    emit brushChanged();
}

void Editor::setMapFlags(int f)
{
    if (mapFlags_ == f)
        return;
    mapFlags_ = f;
    emit brushChanged();
}

void Editor::begin()
{
    undo_.push_back(doc_);
    if (undo_.size() > UNDO_STEPS)
        undo_.pop_front();
    redo_.clear();
}

void Editor::changed()
{
    modified_ = true;
    emit documentChanged();
    emit stateChanged();
}

void Editor::cancel()
{
    if (!undo_.empty() && undo_.back() == doc_)
        undo_.pop_back();
    emit stateChanged();
}

void Editor::undo()
{
    if (undo_.empty())
        return;
    redo_.push_back(doc_);
    doc_ = undo_.back();
    undo_.pop_back();
    select(section_, index());          /* lo elegido puede haber dejado de existir */
    changed();
}

void Editor::redo()
{
    if (redo_.empty())
        return;
    undo_.push_back(doc_);
    doc_ = redo_.back();
    redo_.pop_back();
    select(section_, index());
    changed();
}

void Editor::setDocument(const gfx::Document &doc, const QString &path)
{
    doc_ = doc;
    undo_.clear();
    redo_.clear();
    path_ = path;
    modified_ = false;
    for (int s = 0; s < 5; s++)
        index_[s] = 0;
    emit documentChanged();
    emit selectionChanged();
    emit stateChanged();
}

void Editor::setSaved(const QString &path)
{
    path_ = path;
    modified_ = false;
    emit stateChanged();
}
