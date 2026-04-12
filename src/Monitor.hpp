#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include "Window.hpp"

struct Column {
    std::vector<std::shared_ptr<Window>> windows;
    float widthScale = 0.5f;
};

class Monitor {
public:
    Monitor(HMONITOR hmon, RECT workArea);

    HMONITOR getHMonitor() const { return hmon; }
    RECT getWorkArea() const { return workArea; }

    void addWindow(std::shared_ptr<Window> win);
    std::shared_ptr<Window> removeWindow(HWND hwnd);
    bool hasWindow(HWND hwnd) const;

    // Viewport control
    void scroll(int deltaColumn); 
    void moveFocusedWindow(int deltaColumn);
    void scrollVertical(int deltaRow);
    void moveFocusedWindowVertical(int deltaRow);

    void consumeOrExpelLeft();
    void consumeOrExpelRight();
    void consumeIntoColumn();
    void expelFromColumn();

    void cycleActiveColumnWidth();
    void cycleActiveWindowHeight();

    void toggleFullscreenOnFocused();
    bool setFocusedWindow(HWND hwnd);
    void setLayout();

    std::vector<std::shared_ptr<Window>> getWindows() const;

private:
    HMONITOR hmon;
    RECT workArea;
    std::vector<Column> columns;
    
    // Viewport relative to virtual projection coordinate
    int focusedColumn = 0;
    int focusedRow = 0;
    float viewportOffset = 0.0f;
};
