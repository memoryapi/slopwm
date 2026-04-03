#include "Monitor.hpp"
#include <algorithm>
#include <cmath>
#include "Utils.hpp"

Monitor::Monitor(HMONITOR hmon, RECT workArea) : hmon(hmon), workArea(workArea) {
}

void Monitor::addWindow(std::shared_ptr<Window> win) {
    if (!hasWindow(win->getHwnd())) {
        if (windows.empty()) {
            windows.push_back(win);
            focusedIndex = 0;
        } else {
            focusedIndex++;
            windows.insert(windows.begin() + focusedIndex, win);
        }
        setLayout();
        win->focus();
    }
}

std::shared_ptr<Window> Monitor::removeWindow(HWND hwnd) {
    auto it = std::find_if(windows.begin(), windows.end(), [hwnd](const std::shared_ptr<Window>& w) {
        return w->getHwnd() == hwnd;
    });
    if (it != windows.end()) {
        std::shared_ptr<Window> removed = *it;
        windows.erase(it);
        // clamp focusedIndex
        if (windows.empty()) {
            focusedIndex = 0;
            viewportOffset = 0.0f;
        } else {
            focusedIndex = std::clamp<int>(focusedIndex, 0, static_cast<int>(windows.size()) - 1);
            setLayout();
            windows[focusedIndex]->focus();
        }
        return removed;
    }
    return nullptr;
}

bool Monitor::hasWindow(HWND hwnd) const {
    return std::any_of(windows.begin(), windows.end(), [hwnd](const auto& w) {
        return w->getHwnd() == hwnd;
    });
}

void Monitor::scroll(int deltaIndex) {
    if (windows.empty()) return;
    
    int newIndex = std::clamp<int>(focusedIndex + deltaIndex, 0, static_cast<int>(windows.size()) - 1);
    if (newIndex != focusedIndex) {
        focusedIndex = newIndex;
        setLayout();
        windows[focusedIndex]->focus();
    }
}

void Monitor::moveFocusedWindow(int deltaIndex) {
    if (windows.empty()) return;

    int newIndex = focusedIndex + deltaIndex;
    if (newIndex >= 0 && newIndex < static_cast<int>(windows.size())) {
        std::swap(windows[focusedIndex], windows[newIndex]);
        focusedIndex = newIndex;
        setLayout();
        windows[focusedIndex]->focus();
    }
}

void Monitor::toggleFullscreenOnFocused() {
    if (windows.empty() || focusedIndex < 0 || focusedIndex >= static_cast<int>(windows.size())) return;
    windows[focusedIndex]->toggleFullscreen();
    setLayout();
}

bool Monitor::setFocusedWindow(HWND hwnd) {
    if (windows.empty()) return false;
    
    // Early exit if the window is already focused
    if (focusedIndex >= 0 && focusedIndex < static_cast<int>(windows.size())) {
        if (windows[focusedIndex]->getHwnd() == hwnd) return true;
    }

    for (size_t i = 0; i < windows.size(); ++i) {
        if (windows[i]->getHwnd() == hwnd) {
            focusedIndex = static_cast<int>(i);
            setLayout();
            return true;
        }
    }
    return false;
}

void Monitor::setLayout() {
    if (windows.empty()) return;

    int monWidth = workArea.right - workArea.left;
    int monHeight = workArea.bottom - workArea.top;

    // 1. Calculate cumulative layout projection
    std::vector<float> winVirtualX(windows.size());
    float currentVirtualX = 0.0f;
    for (size_t i = 0; i < windows.size(); ++i) {
        winVirtualX[i] = currentVirtualX;
        currentVirtualX += windows[i]->getWidthScale();
    }

    // 2. Bound viewportOffset using the focused window
    float focusedStart = winVirtualX[focusedIndex];
    float focusedWidth = windows[focusedIndex]->getWidthScale();
    float focusedEnd = focusedStart + focusedWidth;

    // Viewport is [viewportOffset, viewportOffset + 1.0f]
    // Condition A: the window's left edge should not be before the viewport's left edge
    if (focusedStart < viewportOffset) {
        viewportOffset = focusedStart;
    }
    // Condition B: the window's right edge should not be after the viewport's right edge
    if (focusedEnd > viewportOffset + 1.0f) {
        viewportOffset = focusedEnd - 1.0f;
        // Re-enforce Condition A if the window itself is wider than the viewport
        if (focusedStart < viewportOffset) {
            viewportOffset = focusedStart; 
        }
    }

    // Attempt to clamp to edges if we don't have enough windows to scroll past
    float maxViewport = (std::max)(0.0f, currentVirtualX - 1.0f);
    viewportOffset = std::clamp(viewportOffset, 0.0f, maxViewport);

    HDWP hdwp = BeginDeferWindowPos(static_cast<int>(windows.size()));
    if (!hdwp) {
        utils::debugLog("BeginDeferWindowPos failed");
        return;
    }

    int offscreenX = workArea.left + 50000;
    int offscreenY = workArea.top + 50000;

    for (int i = 0; i < (int)windows.size(); ++i) {
        auto& win = windows[i];
        if (!win->isValid()) continue;

        float startX = winVirtualX[i];
        float endX = startX + win->getWidthScale();

        // Check if window overlaps with viewport [viewportOffset, viewportOffset + 1.0f]
        // Epsilon is helpful but theoretically if startX < viewportOffset + 1.0 and endX > viewportOffset it overlaps.
        if (startX < viewportOffset + 1.0f && endX > viewportOffset) {
            // Window is in viewport
            float relStart = startX - viewportOffset;
            int pixelX = workArea.left + static_cast<int>(std::round(relStart * monWidth));
            int pixelWidth = static_cast<int>(std::round(win->getWidthScale() * monWidth));
            
            win->setPos(pixelX, workArea.top, pixelWidth, monHeight, hdwp);
        } else {
            // Off-screen
            win->setPos(offscreenX, offscreenY, monWidth / 2, monHeight, hdwp);
        }
    }

    EndDeferWindowPos(hdwp);
}
