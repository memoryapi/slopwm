#include "Window.hpp"
#include <dwmapi.h>

Window::Window(HWND hwnd) : hwnd(hwnd) {
    GetWindowRect(hwnd, &originalRect);
    originalStyle = GetWindowLong(hwnd, GWL_STYLE);
}

bool Window::restore() {
    if (!isValid()) return false;
    SetWindowPos(hwnd, nullptr, 
                 originalRect.left, originalRect.top, 
                 originalRect.right - originalRect.left, 
                 originalRect.bottom - originalRect.top, 
                 SWP_NOZORDER | SWP_NOACTIVATE);
    return true;
}

void Window::setPos(int x, int y, int width, int height, HDWP hdwp) {
    if (!isValid()) return;
    if (hdwp) {
        DeferWindowPos(hdwp, hwnd, HWND_TOP, x, y, width, height, 0);
    } else {
        SetWindowPos(hwnd, HWND_TOP, x, y, width, height, 0);
    }
}

bool Window::isValid() const {
    return IsWindow(hwnd);
}

bool Window::isManageable() const {
    if (!IsWindowVisible(hwnd)) return false;
    if (IsIconic(hwnd)) return false; // Do not manage minimized windows
    
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    
    if (exStyle & WS_EX_TOOLWINDOW) return false;
    if (!(style & WS_CAPTION)) return false; // Usually we want to manage tiled apps that have captions
    
    HWND owner = GetWindow(hwnd, GW_OWNER);
    if (owner != nullptr) return false;
    
    int cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked) {
        return false;
    }
    
    return true;
}

bool Window::focus() const {
    if (!isValid()) return false;
    
    // Sometimes Windows resists foreground changes.
    // A common hack is to attach thread input to the current foreground thread.
    HWND fgWindow = GetForegroundWindow();
    DWORD fgThread = GetWindowThreadProcessId(fgWindow, nullptr);
    DWORD myThread = GetCurrentThreadId();

    if (fgThread != myThread) {
        AttachThreadInput(myThread, fgThread, TRUE);
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        AttachThreadInput(myThread, fgThread, FALSE);
    } else {
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
    }
    return true;
}
