#include "Workspace.hpp"
#include <algorithm>
#include <array>

constexpr std::array<float, 3> PRESET_COL_SCALES = { 1.0f / 3.0f, 0.5f, 2.0f / 3.0f };

float Workspace::getNextPresetScale(float currentScale) {
    for (float scale : PRESET_COL_SCALES) {
        if (scale > currentScale + 0.01f) {
            return scale;
        }
    }
    return PRESET_COL_SCALES[0];
}

void Workspace::setFocusedColumn(int c) { 
    focusedColumn = c; 
    ensureFocusLimits(); 
}

void Workspace::setFocusedRow(int r) { 
    focusedRow = r; 
    ensureFocusLimits(); 
}

void Workspace::setViewportOffset(float offset) {
    viewportOffset = offset;
}

void Workspace::ensureFocusLimits() {
    if (columns.empty()) {
        focusedColumn = 0;
        focusedRow = 0;
        return;
    }
    focusedColumn = std::clamp<int>(focusedColumn, 0, static_cast<int>(columns.size()) - 1);
    focusedRow = std::clamp<int>(focusedRow, 0, static_cast<int>(columns[focusedColumn].getWindows().size()) - 1);
}

void Workspace::insertColumn(Column col, int index) {
    if (index < 0 || index > static_cast<int>(columns.size())) {
        columns.push_back(std::move(col));
    } else {
        columns.insert(columns.begin() + index, std::move(col));
    }
    ensureFocusLimits();
}

bool Workspace::removeColumn(int index) {
    if (index < 0 || index >= static_cast<int>(columns.size())) return false;
    columns.erase(columns.begin() + index);
    ensureFocusLimits();
    return true;
}

void Workspace::addWindowContextually(std::shared_ptr<Window> win) {
    if (columns.empty()) {
        columns.push_back(Column(win));
        focusedColumn = 0;
        focusedRow = 0;
    } else {
        focusedColumn++;
        columns.insert(columns.begin() + focusedColumn, Column(win));
        focusedRow = 0;
    }
}

std::shared_ptr<Window> Workspace::removeWindow(HWND hwnd) {
    for (int c = 0; c < static_cast<int>(columns.size()); ++c) {
        auto& col = columns[c];
        auto& wins = col.getWindows();
        for (int r = 0; r < static_cast<int>(wins.size()); ++r) {
            if (wins[r]->getHwnd() == hwnd) {
                auto w = wins[r];
                col.removeWindow(r);
                if (col.getWindows().empty()) {
                    removeColumn(c);
                }
                ensureFocusLimits();
                return w;
            }
        }
    }
    return nullptr;
}

void Workspace::consumeOrExpelLeft() {
    if (columns.empty() || (focusedColumn == 0 && columns[focusedColumn].getWindows().size() == 1)) return;
    
    auto& curCol = columns[focusedColumn];
    if (curCol.getWindows().size() == 1) {
        auto win = curCol.getWindows()[0];
        int targetIdx = focusedColumn - 1;
        
        removeColumn(focusedColumn);
        
        focusedColumn = targetIdx;
        auto& targetCol = columns[focusedColumn];
        targetCol.insertWindow(win); 
        focusedRow = static_cast<int>(targetCol.getWindows().size()) - 1;
    } else {
        auto win = curCol.removeWindow(focusedRow);
        Column newCol(win, curCol.getWidthScale());
        columns.insert(columns.begin() + focusedColumn, std::move(newCol));
        focusedRow = 0;
    }
}

void Workspace::consumeOrExpelRight() {
    if (columns.empty() || (focusedColumn == static_cast<int>(columns.size()) - 1 && columns[focusedColumn].getWindows().size() == 1)) return;

    auto& curCol = columns[focusedColumn];
    if (curCol.getWindows().size() == 1) {
        auto win = curCol.getWindows()[0];
        int targetIdx = focusedColumn; // Because the column on our right shifts leftwards into our index.
        
        removeColumn(focusedColumn);
        
        focusedColumn = targetIdx;
        auto& targetCol = columns[focusedColumn];
        targetCol.insertWindow(win);
        focusedRow = static_cast<int>(targetCol.getWindows().size()) - 1;
    } else {
        auto win = curCol.removeWindow(focusedRow);
        Column newCol(win, curCol.getWidthScale());
        focusedColumn++;
        columns.insert(columns.begin() + focusedColumn, std::move(newCol));
        focusedRow = 0;
    }
}

void Workspace::consumeIntoColumn() {
    if (columns.empty() || focusedColumn >= static_cast<int>(columns.size()) - 1) return;
    
    auto& rightCol = columns[focusedColumn + 1];
    auto win = rightCol.removeWindow(0);
    
    columns[focusedColumn].insertWindow(win);
    
    if (rightCol.getWindows().empty()) {
        removeColumn(focusedColumn + 1);
    }
}

void Workspace::expelFromColumn() {
    if (columns.empty() || columns[focusedColumn].getWindows().size() <= 1) return;
    
    auto& curCol = columns[focusedColumn];
    auto win = curCol.removeWindow(static_cast<int>(curCol.getWindows().size()) - 1);
    ensureFocusLimits();

    Column newCol(win, curCol.getWidthScale());
    columns.insert(columns.begin() + focusedColumn + 1, std::move(newCol));
}

void Workspace::scroll(int deltaColumn) {
    if (columns.empty()) return;
    focusedColumn = std::clamp<int>(focusedColumn + deltaColumn, 0, static_cast<int>(columns.size()) - 1);
    ensureFocusLimits();
}

void Workspace::scrollVertical(int deltaRow) {
    if (columns.empty()) return;
    focusedRow = std::clamp<int>(focusedRow + deltaRow, 0, static_cast<int>(columns[focusedColumn].getWindows().size()) - 1);
}

void Workspace::moveFocusedWindow(int deltaColumn) {
    if (columns.empty()) return;
    int newCol = focusedColumn + deltaColumn;
    if (newCol >= 0 && newCol < static_cast<int>(columns.size())) {
        std::swap(columns[focusedColumn], columns[newCol]);
        focusedColumn = newCol;
    }
}

void Workspace::moveFocusedWindowVertical(int deltaRow) {
    if (columns.empty()) return;
    auto& wins = columns[focusedColumn].getWindows();
    int newRow = focusedRow + deltaRow;
    if (newRow >= 0 && newRow < static_cast<int>(wins.size())) {
        std::swap(wins[focusedRow], wins[newRow]);
        focusedRow = newRow;
    }
}

void Workspace::cycleActiveColumnWidth() {
    if (columns.empty()) return;
    columns[focusedColumn].setWidthScale(getNextPresetScale(columns[focusedColumn].getWidthScale()));
}

void Workspace::cycleActiveWindowHeight() {
    if (columns.empty()) return;
    columns[focusedColumn].cycleWindowHeight(focusedRow);
}

void Workspace::toggleFullscreenOnFocused() {
    if (columns.empty()) return;
    columns[focusedColumn].setWidthScale(columns[focusedColumn].getWidthScale() > 0.5f ? 0.5f : 1.0f);
}

bool Workspace::hasWindow(HWND hwnd) const {
    for (const auto& col : columns) {
        for (const auto& win : col.getWindows()) {
            if (win->getHwnd() == hwnd) return true;
        }
    }
    return false;
}

bool Workspace::setFocusToWindow(HWND hwnd) {
    for (int c = 0; c < static_cast<int>(columns.size()); ++c) {
        auto& wins = columns[c].getWindows();
        for (int r = 0; r < static_cast<int>(wins.size()); ++r) {
            if (wins[r]->getHwnd() == hwnd) {
                focusedColumn = c;
                focusedRow = r;
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<Window> Workspace::getFocusedWindow() const {
    if (columns.empty() || focusedColumn >= static_cast<int>(columns.size())) return nullptr;
    const auto& wins = columns[focusedColumn].getWindows();
    if (focusedRow >= static_cast<int>(wins.size())) return nullptr;
    return wins[focusedRow];
}
