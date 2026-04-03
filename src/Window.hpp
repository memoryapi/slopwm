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

    float getWidthScale() const { return widthScale; }
    void toggleFullscreen() { 
        widthScale = (widthScale > 0.5f) ? 0.5f : 1.0f; 
    }

private:
    HWND hwnd;
    float widthScale = 0.5f;
    // Store original state to restore when WM exits
    RECT originalRect;
    LONG originalStyle;
};
