#pragma once
#include <windows.h>
#include <optional>

class Window {
public:
    explicit Window(HWND hwnd);

    HWND getHwnd() const { return hwnd; }

    bool restore();
    void setPos(int x, int y, int width, int height, HDWP hdwp);
    
    // Checks if the window is currently valid (still alive)
    bool isValid() const;

    // Is this a window we actually want to manage?
    bool isManageable() const;

    bool focus() const;

private:
    HWND hwnd;
    // Store original state to restore when WM exits
    RECT originalRect;
    LONG originalStyle;
};
