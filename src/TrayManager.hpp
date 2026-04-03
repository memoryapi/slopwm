#pragma once
#include <windows.h>
#include <shellapi.h>

class TrayManager {
public:
    TrayManager(HINSTANCE hInstance);
    ~TrayManager();

    void showNotification(const wchar_t* title, const wchar_t* message);

private:
    HWND hiddenHwnd = nullptr;
    NOTIFYICONDATAW nid = {};
    HINSTANCE hInst = nullptr;
    bool isConsoleVisible = false;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void handleTrayMessage(LPARAM lParam);
    void showContextMenu();
    void toggleConsole();
    void exitApp();
};
