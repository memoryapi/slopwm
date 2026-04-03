#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include "Window.hpp"

class Monitor {
public:
    Monitor(HMONITOR hmon, RECT workArea);

    HMONITOR getHMonitor() const { return hmon; }
    RECT getWorkArea() const { return workArea; }

    void addWindow(std::shared_ptr<Window> win);
    void removeWindow(HWND hwnd);
    bool hasWindow(HWND hwnd) const;

    // Viewport control
    void scroll(int deltaIndex); // e.g. +1 to scroll right, -1 to scroll left
    void moveFocusedWindow(int deltaIndex); // e.g. +1 to move right, -1 to move left
    void toggleFullscreenOnFocused();
    bool setFocusedWindow(HWND hwnd);
    void setLayout();

    const std::vector<std::shared_ptr<Window>>& getWindows() const { return windows; }

private:
    HMONITOR hmon;
    RECT workArea;
    std::vector<std::shared_ptr<Window>> windows;
    
    // Viewport relative to virtual projection coordinate
    int focusedIndex = 0;
    float viewportOffset = 0.0f;
};
