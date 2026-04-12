#include "Monitor.hpp"
#include <algorithm>
#include <cmath>
#include <array>
#include "Utils.hpp"

constexpr std::array<float, 3> PRESET_SCALES = { 1.0f / 3.0f, 0.5f, 2.0f / 3.0f };

static float getNextPresetScale(float currentScale) {
    for (float scale : PRESET_SCALES) {
        if (scale > currentScale + 0.01f) {
            return scale;
        }
    }
    return PRESET_SCALES[0];
}

Monitor::Monitor(HMONITOR hmon, RECT workArea) : hmon(hmon), workArea(workArea) {
    workspaces.push_back(Workspace{});
}

void Monitor::addWindow(std::shared_ptr<Window> win) {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (!hasWindow(win->getHwnd())) {
        win->setHeightScale(1.0f);
        Column newCol;
        newCol.windows.push_back(win);
        
        if (columns.empty()) {
            columns.push_back(newCol);
            focusedColumn = 0;
            focusedRow = 0;
        } else {
            focusedColumn++;
            columns.insert(columns.begin() + focusedColumn, newCol);
            focusedRow = 0;
        }
        setLayout();
        columns[focusedColumn].windows[focusedRow]->focus();
        cleanupEmptyWorkspaces();
    }
}


void Monitor::scroll(int deltaColumn) {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty()) return;
    int newCol = std::clamp<int>(focusedColumn + deltaColumn, 0, static_cast<int>(columns.size()) - 1);
    if (newCol != focusedColumn) {
        focusedColumn = newCol;
        focusedRow = std::clamp<int>(focusedRow, 0, static_cast<int>(columns[focusedColumn].windows.size()) - 1);
        setLayout();
        columns[focusedColumn].windows[focusedRow]->focus();
    }
}

void Monitor::moveFocusedWindow(int deltaColumn) {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty()) return;
    int newCol = focusedColumn + deltaColumn;
    if (newCol >= 0 && newCol < static_cast<int>(columns.size())) {
        std::swap(columns[focusedColumn], columns[newCol]);
        focusedColumn = newCol;
        setLayout();
        columns[focusedColumn].windows[focusedRow]->focus();
    }
}

void Monitor::scrollVertical(int deltaRow) {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty()) return;
    auto& col = columns[focusedColumn];
    int newRow = std::clamp<int>(focusedRow + deltaRow, 0, static_cast<int>(col.windows.size()) - 1);
    if (newRow != focusedRow) {
        focusedRow = newRow;
        setLayout(); // Might not change layout visually but ensures focus logic is clean
        col.windows[focusedRow]->focus();
    }
}

void Monitor::moveFocusedWindowVertical(int deltaRow) {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty()) return;
    auto& col = columns[focusedColumn];
    int newRow = focusedRow + deltaRow;
    if (newRow >= 0 && newRow < static_cast<int>(col.windows.size())) {
        std::swap(col.windows[focusedRow], col.windows[newRow]);
        focusedRow = newRow;
        setLayout();
        col.windows[focusedRow]->focus();
    }
}

void Monitor::consumeOrExpelLeft() {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty() || (focusedColumn == 0 && columns[focusedColumn].windows.size() == 1)) return;
    
    auto& curCol = columns[focusedColumn];
    if (curCol.windows.size() == 1) {
        // Consume: Move this only window to the bottom of the left column
        auto win = curCol.windows[0];
        columns.erase(columns.begin() + focusedColumn);
        focusedColumn--;
        
        auto& targetCol = columns[focusedColumn];
        float totalH = 0.0f;
        for (const auto& w : targetCol.windows) totalH += w->getHeightScale();
        win->setHeightScale(totalH / targetCol.windows.size());
        
        targetCol.windows.push_back(win);
        focusedRow = static_cast<int>(targetCol.windows.size()) - 1;
    } else {
        // Expel: Extract focused window into a new column to the left
        auto win = curCol.windows[focusedRow];
        curCol.windows.erase(curCol.windows.begin() + focusedRow);
        
        win->setHeightScale(1.0f);
        
        Column newCol;
        newCol.widthScale = curCol.widthScale;
        newCol.windows.push_back(win);
        columns.insert(columns.begin() + focusedColumn, newCol);
        // focusedColumn stays on the new column (which is now at old index)
        focusedRow = 0;
    }
    setLayout();
    columns[focusedColumn].windows[focusedRow]->focus();
}

void Monitor::consumeOrExpelRight() {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty() || (focusedColumn == static_cast<int>(columns.size()) - 1 && columns[focusedColumn].windows.size() == 1)) return;

    auto& curCol = columns[focusedColumn];
    if (curCol.windows.size() == 1) {
        // Consume: Move this into right column
        auto win = curCol.windows[0];
        columns.erase(columns.begin() + focusedColumn);
        // We deleted current column, so the old right column is now at focusedColumn
        auto& targetCol = columns[focusedColumn];
        float totalH = 0.0f;
        for (const auto& w : targetCol.windows) totalH += w->getHeightScale();
        win->setHeightScale(totalH / targetCol.windows.size());
        
        targetCol.windows.push_back(win);
        focusedRow = static_cast<int>(targetCol.windows.size()) - 1;
    } else {
        // Expel: Extract focused window into new column to right
        auto win = curCol.windows[focusedRow];
        curCol.windows.erase(curCol.windows.begin() + focusedRow);
        
        win->setHeightScale(1.0f);
        
        Column newCol;
        newCol.widthScale = curCol.widthScale;
        newCol.windows.push_back(win);
        focusedColumn++;
        columns.insert(columns.begin() + focusedColumn, newCol);
        focusedRow = 0;
    }
    setLayout();
    columns[focusedColumn].windows[focusedRow]->focus();
}

void Monitor::consumeIntoColumn() {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty() || focusedColumn >= static_cast<int>(columns.size()) - 1) return;
    
    // Take top window from right column
    auto& rightCol = columns[focusedColumn + 1];
    auto win = rightCol.windows.front();
    rightCol.windows.erase(rightCol.windows.begin());
    
    auto& targetCol = columns[focusedColumn];
    float totalH = 0.0f;
    for (const auto& w : targetCol.windows) totalH += w->getHeightScale();
    win->setHeightScale(totalH / targetCol.windows.size());
    
    targetCol.windows.push_back(win);
    
    if (rightCol.windows.empty()) {
        columns.erase(columns.begin() + focusedColumn + 1);
    }
    setLayout();
    // Don't change focus usually, just let it animate/re-layout
}

void Monitor::expelFromColumn() {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty() || columns[focusedColumn].windows.size() <= 1) return;
    
    // Expel bottom window to a new column on the right
    auto& curCol = columns[focusedColumn];
    auto win = curCol.windows.back();
    curCol.windows.pop_back();
    
    // If the focused row was the bottom one, adjust it
    if (focusedRow >= static_cast<int>(curCol.windows.size())) {
        focusedRow = static_cast<int>(curCol.windows.size()) - 1;
    }

    win->setHeightScale(1.0f);

    Column newCol;
    newCol.widthScale = curCol.widthScale;
    newCol.windows.push_back(win);
    
    columns.insert(columns.begin() + focusedColumn + 1, newCol);
    setLayout();
    columns[focusedColumn].windows[focusedRow]->focus();
}

void Monitor::cycleActiveColumnWidth() {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty() || focusedColumn < 0 || focusedColumn >= static_cast<int>(columns.size())) return;
    auto& col = columns[focusedColumn];
    col.widthScale = getNextPresetScale(col.widthScale);
    setLayout();
}

void Monitor::cycleActiveWindowHeight() {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty() || focusedColumn < 0 || focusedColumn >= static_cast<int>(columns.size())) return;
    auto& col = columns[focusedColumn];
    if (col.windows.size() <= 1) return; // Cannot re-proportion a single window

    if (focusedRow < 0 || focusedRow >= static_cast<int>(col.windows.size())) return;
    
    // Normalize current height scales to exactly 1.0 so we can math out available space safely
    float totalHeightScale = 0.0f;
    for (const auto& win : col.windows) {
        totalHeightScale += win->getHeightScale();
    }
    
    if (totalHeightScale > 0.0f) {
        for (const auto& win : col.windows) {
            win->setHeightScale(win->getHeightScale() / totalHeightScale);
        }
    }
    
    auto win = col.windows[focusedRow];
    float currentProp = win->getHeightScale();
    float targetProp = getNextPresetScale(currentProp);
    
    win->setHeightScale(targetProp);
    
    float remainingProp = (std::max)(0.0f, 1.0f - targetProp);
    float oldRemainingProp = (std::max)(0.0001f, 1.0f - currentProp);
    
    for (int r = 0; r < static_cast<int>(col.windows.size()); ++r) {
        if (r != focusedRow) {
            float oldProp = col.windows[r]->getHeightScale();
            float newProp = oldProp * (remainingProp / oldRemainingProp);
            col.windows[r]->setHeightScale(newProp);
        }
    }
    
    setLayout();
}

void Monitor::toggleFullscreenOnFocused() {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty() || focusedColumn < 0 || focusedColumn >= static_cast<int>(columns.size())) return;
    auto& col = columns[focusedColumn];
    col.widthScale = (col.widthScale > 0.5f) ? 0.5f : 1.0f;
    setLayout();
}


void Monitor::setLayout() {
    auto& ws = workspaces[activeWorkspace];
    auto& columns = ws.columns;
    auto& focusedColumn = ws.focusedColumn;
    auto& focusedRow = ws.focusedRow;
    auto& viewportOffset = ws.viewportOffset;

    if (columns.empty()) return;

    int monWidth = workArea.right - workArea.left;
    int monHeight = workArea.bottom - workArea.top;

    std::vector<float> colVirtualX(columns.size());
    float currentVirtualX = 0.0f;
    for (size_t c = 0; c < columns.size(); ++c) {
        colVirtualX[c] = currentVirtualX;
        currentVirtualX += columns[c].widthScale;
    }

    float focusedStart = colVirtualX[focusedColumn];
    float focusedWidth = columns[focusedColumn].widthScale;
    float focusedEnd = focusedStart + focusedWidth;

    if (focusedStart < viewportOffset) {
        viewportOffset = focusedStart;
    }
    if (focusedEnd > viewportOffset + 1.0f) {
        viewportOffset = focusedEnd - 1.0f;
        if (focusedStart < viewportOffset) {
            viewportOffset = focusedStart; 
        }
    }

    float maxViewport = (std::max)(0.0f, currentVirtualX - 1.0f);
    viewportOffset = std::clamp(viewportOffset, 0.0f, maxViewport);

    // Count total windows
    int totalWindows = 0;
    for (const auto& col : columns) {
        totalWindows += col.windows.size();
    }

    HDWP hdwp = BeginDeferWindowPos(totalWindows);
    if (!hdwp) {
        utils::debugLog("BeginDeferWindowPos failed");
        return;
    }

    int offscreenX = workArea.left + 50000;
    int offscreenY = workArea.top + 50000;

    for (int c = 0; c < static_cast<int>(columns.size()); ++c) {
        auto& col = columns[c];
        float startX = colVirtualX[c];
        float endX = startX + col.widthScale;
        
        bool inViewport = (startX < viewportOffset + 1.0f && endX > viewportOffset);
        
        float totalHeightScale = 0.0f;
        for (const auto& win : col.windows) {
            totalHeightScale += win->getHeightScale();
        }

        int currentY = workArea.top;
        
        for (int r = 0; r < static_cast<int>(col.windows.size()); ++r) {
            auto& win = col.windows[r];
            if (!win->isValid()) continue;

            float fraction = win->getHeightScale() / (totalHeightScale > 0.0f ? totalHeightScale : 1.0f);
            int winHeight = static_cast<int>(std::round(fraction * monHeight));
            
            if (r == static_cast<int>(col.windows.size()) - 1) {
                winHeight = workArea.bottom - currentY;
            }

            if (inViewport) {
                float relStart = startX - viewportOffset;
                int pixelX = workArea.left + static_cast<int>(std::round(relStart * monWidth));
                int pixelWidth = static_cast<int>(std::round(col.widthScale * monWidth));
                
                win->setPos(pixelX, currentY, pixelWidth, winHeight, hdwp);
            } else {
                win->setPos(offscreenX, offscreenY, monWidth / 2, monHeight, hdwp);
            }
            currentY += winHeight;
        }
    }

    EndDeferWindowPos(hdwp);
}

bool Monitor::hasWindow(HWND hwnd) const {
    for (const auto& ws : workspaces) {
        for (const auto& col : ws.columns) {
            for (const auto& win : col.windows) {
                if (win->getHwnd() == hwnd) return true;
            }
        }
    }
    return false;
}

std::shared_ptr<Window> Monitor::removeWindow(HWND hwnd) {
    for (size_t wi = 0; wi < workspaces.size(); ++wi) {
        auto& ws = workspaces[wi];
        for (size_t c = 0; c < ws.columns.size(); ++c) {
            auto& col = ws.columns[c];
            for (size_t r = 0; r < col.windows.size(); ++r) {
                if (col.windows[r]->getHwnd() == hwnd) {
                    auto win = col.windows[r];
                    col.windows.erase(col.windows.begin() + r);
                    
                    if (col.windows.empty()) {
                        ws.columns.erase(ws.columns.begin() + c);
                        if (ws.focusedColumn >= static_cast<int>(ws.columns.size())) {
                            ws.focusedColumn = (std::max)(0, static_cast<int>(ws.columns.size()) - 1);
                        }
                    } else if (ws.focusedRow >= static_cast<int>(col.windows.size())) {
                        ws.focusedRow = static_cast<int>(col.windows.size()) - 1;
                    }
                    
                    if (wi == activeWorkspace) {
                        setLayout();
                        if (!ws.columns.empty()) {
                            ws.columns[ws.focusedColumn].windows[ws.focusedRow]->focus();
                        }
                    }
                    cleanupEmptyWorkspaces();
                    return win;
                }
            }
        }
    }
    return nullptr;
}

bool Monitor::setFocusedWindow(HWND hwnd) {
    for (size_t wi = 0; wi < workspaces.size(); ++wi) {
        auto& ws = workspaces[wi];
        for (size_t c = 0; c < ws.columns.size(); ++c) {
            for (size_t r = 0; r < ws.columns[c].windows.size(); ++r) {
                if (ws.columns[c].windows[r]->getHwnd() == hwnd) {
                    if (static_cast<int>(wi) != activeWorkspace) {
                        setActiveWorkspace(static_cast<int>(wi), false);
                    }
                    auto& newWs = workspaces[activeWorkspace];
                    newWs.focusedColumn = static_cast<int>(c);
                    newWs.focusedRow = static_cast<int>(r);
                    setLayout();
                    // Do not call ->focus() again! The OS already gave it focus to trigger this event!
                    return true;
                }
            }
        }
    }
    return false;
}

std::vector<std::shared_ptr<Window>> Monitor::getWindows() const {
    std::vector<std::shared_ptr<Window>> allWins;
    for (const auto& ws : workspaces) {
        for (const auto& col : ws.columns) {
            for (const auto& win : col.windows) {
                if (win->isValid()) {
                    allWins.push_back(win);
                }
            }
        }
    }
    return allWins;
}

void Monitor::parkWorkspaceOffscreen(int wsIndex) {
    if (wsIndex < 0 || wsIndex >= static_cast<int>(workspaces.size())) return;
    HDWP hdwp = BeginDeferWindowPos(100);
    int offscreenX = 50000;
    int offscreenY = 50000;
    int monWidth = workArea.right - workArea.left;
    int monHeight = workArea.bottom - workArea.top;

    for (auto& col : workspaces[wsIndex].columns) {
        for (auto& win : col.windows) {
            win->setPos(offscreenX, offscreenY, monWidth / 2, monHeight, hdwp);
        }
    }
    EndDeferWindowPos(hdwp);
}

void Monitor::setActiveWorkspace(int index, bool focusWindow) {
    if (index < 0 || index >= static_cast<int>(workspaces.size()) || index == activeWorkspace) return;
    
    parkWorkspaceOffscreen(activeWorkspace);
    
    previousWorkspace = activeWorkspace;
    activeWorkspace = index;
    cleanupEmptyWorkspaces(); 
    
    setLayout();
    if (focusWindow) {
        auto& ws = workspaces[activeWorkspace];
        if (!ws.columns.empty()) {
            ws.columns[ws.focusedColumn].windows[ws.focusedRow]->focus();
        }
    }
}

void Monitor::cleanupEmptyWorkspaces() {
    for (int i = static_cast<int>(workspaces.size()) - 1; i >= 0; --i) {
        if (workspaces[i].columns.empty() && i != activeWorkspace) {
            workspaces.erase(workspaces.begin() + i);
            if (activeWorkspace > i) activeWorkspace--;
            if (previousWorkspace > i) previousWorkspace--;
            else if (previousWorkspace == i) previousWorkspace = activeWorkspace; 
        }
    }
    
    if (workspaces.empty()) {
        workspaces.push_back(Workspace{});
        activeWorkspace = 0;
        previousWorkspace = 0;
    } else if (!workspaces.back().columns.empty()) {
        workspaces.push_back(Workspace{});
    }
}

void Monitor::focusWorkspaceUp() {
    if (activeWorkspace > 0) {
        setActiveWorkspace(activeWorkspace - 1);
    }
}

void Monitor::focusWorkspaceDown() {
    if (activeWorkspace < static_cast<int>(workspaces.size()) - 1) {
        setActiveWorkspace(activeWorkspace + 1);
    }
}

void Monitor::focusWorkspacePrevious() {
    if (previousWorkspace >= 0 && previousWorkspace < static_cast<int>(workspaces.size()) && previousWorkspace != activeWorkspace) {
        setActiveWorkspace(previousWorkspace);
    }
}

void Monitor::moveColumnToWorkspaceUp() {
    auto& ws = workspaces[activeWorkspace];
    if (ws.columns.empty() || activeWorkspace == 0) return;
    
    auto col = std::move(ws.columns[ws.focusedColumn]);
    ws.columns.erase(ws.columns.begin() + ws.focusedColumn);
    
    if (ws.focusedColumn >= static_cast<int>(ws.columns.size())) ws.focusedColumn = (std::max)(0, static_cast<int>(ws.columns.size()) - 1);
    ws.focusedRow = 0;
    
    setActiveWorkspace(activeWorkspace - 1, false);
    
    auto& targetWs = workspaces[activeWorkspace];
    if (targetWs.columns.empty()) {
        targetWs.columns.push_back(std::move(col));
        targetWs.focusedColumn = 0;
    } else {
        targetWs.focusedColumn++;
        targetWs.columns.insert(targetWs.columns.begin() + targetWs.focusedColumn, std::move(col));
    }
    targetWs.focusedRow = 0;
    
    setLayout();
    targetWs.columns[targetWs.focusedColumn].windows[targetWs.focusedRow]->focus();
    cleanupEmptyWorkspaces();
}

void Monitor::moveColumnToWorkspaceDown() {
    if (workspaces[activeWorkspace].columns.empty()) return;
    
    // We grab ws locally. Erasing doesn't invalidate it because no reallocation occurs.
    auto& ws = workspaces[activeWorkspace];
    auto col = std::move(ws.columns[ws.focusedColumn]);
    ws.columns.erase(ws.columns.begin() + ws.focusedColumn);
    
    if (ws.focusedColumn >= static_cast<int>(ws.columns.size())) ws.focusedColumn = (std::max)(0, static_cast<int>(ws.columns.size()) - 1);
    ws.focusedRow = 0;
    
    setActiveWorkspace(activeWorkspace + 1, false);
    
    auto& targetWs = workspaces[activeWorkspace];
    if (targetWs.columns.empty()) {
        targetWs.columns.push_back(std::move(col));
        targetWs.focusedColumn = 0;
    } else {
        targetWs.focusedColumn++;
        targetWs.columns.insert(targetWs.columns.begin() + targetWs.focusedColumn, std::move(col));
    }
    targetWs.focusedRow = 0;
    
    setLayout();
    targetWs.columns[targetWs.focusedColumn].windows[targetWs.focusedRow]->focus();
    cleanupEmptyWorkspaces();
}
