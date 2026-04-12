#include "Monitor.hpp"
#include <algorithm>
#include <cmath>
#include "Utils.hpp"

Monitor::Monitor(HMONITOR hmon, RECT workArea) : hmon(hmon), workArea(workArea) {}

void Monitor::addWindow(std::shared_ptr<Window> win) {
    if (!hasWindow(win->getHwnd())) {
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
    }
}

std::shared_ptr<Window> Monitor::removeWindow(HWND hwnd) {
    for (size_t c = 0; c < columns.size(); ++c) {
        auto& col = columns[c];
        auto it = std::find_if(col.windows.begin(), col.windows.end(), [hwnd](const std::shared_ptr<Window>& w) {
            return w->getHwnd() == hwnd;
        });
        
        if (it != col.windows.end()) {
            std::shared_ptr<Window> removed = *it;
            col.windows.erase(it);
            
            if (col.windows.empty()) {
                columns.erase(columns.begin() + c);
                // Adjust focused column
                if (columns.empty()) {
                    focusedColumn = 0;
                    focusedRow = 0;
                    viewportOffset = 0.0f;
                    return removed;
                } else {
                    focusedColumn = std::clamp<int>(focusedColumn, 0, static_cast<int>(columns.size()) - 1);
                }
            } else if (static_cast<int>(c) == focusedColumn) {
                // Adjust focused row if in the same column
                focusedRow = std::clamp<int>(focusedRow, 0, static_cast<int>(columns[focusedColumn].windows.size()) - 1);
            }
            
            setLayout();
            columns[focusedColumn].windows[focusedRow]->focus();
            return removed;
        }
    }
    return nullptr;
}

bool Monitor::hasWindow(HWND hwnd) const {
    for (const auto& col : columns) {
        for (const auto& win : col.windows) {
            if (win->getHwnd() == hwnd) return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<Window>> Monitor::getWindows() const {
    std::vector<std::shared_ptr<Window>> all;
    for (const auto& col : columns) {
        for (const auto& win : col.windows) {
            all.push_back(win);
        }
    }
    return all;
}

void Monitor::scroll(int deltaColumn) {
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
    if (columns.empty() || focusedColumn == 0 && columns[focusedColumn].windows.size() == 1) return;
    
    auto& curCol = columns[focusedColumn];
    if (curCol.windows.size() == 1) {
        // Consume: Move this only window to the bottom of the left column
        auto win = curCol.windows[0];
        columns.erase(columns.begin() + focusedColumn);
        focusedColumn--;
        columns[focusedColumn].windows.push_back(win);
        focusedRow = static_cast<int>(columns[focusedColumn].windows.size()) - 1;
    } else {
        // Expel: Extract focused window into a new column to the left
        auto win = curCol.windows[focusedRow];
        curCol.windows.erase(curCol.windows.begin() + focusedRow);
        
        Column newCol;
        // The expelled column inherits width scale logic or resets. Let's inherit:
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
    if (columns.empty() || (focusedColumn == static_cast<int>(columns.size()) - 1 && columns[focusedColumn].windows.size() == 1)) return;

    auto& curCol = columns[focusedColumn];
    if (curCol.windows.size() == 1) {
        // Consume: Move this into right column
        auto win = curCol.windows[0];
        columns.erase(columns.begin() + focusedColumn);
        // We deleted current column, so the old right column is now at focusedColumn
        // We append to bottom:
        columns[focusedColumn].windows.push_back(win);
        focusedRow = static_cast<int>(columns[focusedColumn].windows.size()) - 1;
    } else {
        // Expel: Extract focused window into new column to right
        auto win = curCol.windows[focusedRow];
        curCol.windows.erase(curCol.windows.begin() + focusedRow);
        
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
    if (columns.empty() || focusedColumn >= static_cast<int>(columns.size()) - 1) return;
    
    // Take top window from right column
    auto& rightCol = columns[focusedColumn + 1];
    auto win = rightCol.windows.front();
    rightCol.windows.erase(rightCol.windows.begin());
    
    columns[focusedColumn].windows.push_back(win);
    
    if (rightCol.windows.empty()) {
        columns.erase(columns.begin() + focusedColumn + 1);
    }
    setLayout();
    // Don't change focus usually, just let it animate/re-layout
}

void Monitor::expelFromColumn() {
    if (columns.empty() || columns[focusedColumn].windows.size() <= 1) return;
    
    // Expel bottom window to a new column on the right
    auto& curCol = columns[focusedColumn];
    auto win = curCol.windows.back();
    curCol.windows.pop_back();
    
    // If the focused row was the bottom one, adjust it
    if (focusedRow >= static_cast<int>(curCol.windows.size())) {
        focusedRow = static_cast<int>(curCol.windows.size()) - 1;
    }

    Column newCol;
    newCol.widthScale = curCol.widthScale;
    newCol.windows.push_back(win);
    
    columns.insert(columns.begin() + focusedColumn + 1, newCol);
    setLayout();
    columns[focusedColumn].windows[focusedRow]->focus();
}

void Monitor::toggleFullscreenOnFocused() {
    if (columns.empty() || focusedColumn < 0 || focusedColumn >= static_cast<int>(columns.size())) return;
    auto& col = columns[focusedColumn];
    col.widthScale = (col.widthScale > 0.5f) ? 0.5f : 1.0f;
    setLayout();
}

bool Monitor::setFocusedWindow(HWND hwnd) {
    if (columns.empty()) return false;
    
    // Early exit if the window is already focused
    if (focusedColumn >= 0 && focusedColumn < static_cast<int>(columns.size())) {
        auto& col = columns[focusedColumn];
        if (focusedRow >= 0 && focusedRow < static_cast<int>(col.windows.size())) {
            if (col.windows[focusedRow]->getHwnd() == hwnd) return true;
        }
    }

    for (size_t c = 0; c < columns.size(); ++c) {
        for (size_t r = 0; r < columns[c].windows.size(); ++r) {
            if (columns[c].windows[r]->getHwnd() == hwnd) {
                focusedColumn = static_cast<int>(c);
                focusedRow = static_cast<int>(r);
                setLayout();
                return true;
            }
        }
    }
    return false;
}

void Monitor::setLayout() {
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
        
        int winHeight = monHeight / col.windows.size();
        
        for (int r = 0; r < static_cast<int>(col.windows.size()); ++r) {
            auto& win = col.windows[r];
            if (!win->isValid()) continue;

            if (inViewport) {
                float relStart = startX - viewportOffset;
                int pixelX = workArea.left + static_cast<int>(std::round(relStart * monWidth));
                int pixelWidth = static_cast<int>(std::round(col.widthScale * monWidth));
                int pixelY = workArea.top + (r * winHeight);
                
                win->setPos(pixelX, pixelY, pixelWidth, winHeight, hdwp);
            } else {
                win->setPos(offscreenX, offscreenY, monWidth / 2, monHeight, hdwp);
            }
        }
    }

    EndDeferWindowPos(hdwp);
}
