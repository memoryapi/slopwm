#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include "Monitor.hpp"

class WindowManager {
public:
    static WindowManager& getInstance() {
        static WindowManager instance;
        return instance;
    }

    void initialize();
    void shutdown();
    bool handleHotkey(int hotkeyId);

    void onWindowCreated(HWND hwnd, bool isStartup = false);
    void onWindowDestroyed(HWND hwnd);
    void onWindowFocused(HWND hwnd);

private:
    WindowManager() = default;
    ~WindowManager() = default;

    HWINEVENTHOOK eventHook;
    std::vector<Monitor> monitors;

    Monitor* getActiveMonitor();
    Monitor* getMonitorForWindow(HWND hwnd);
    void discoverMonitors();
    void discoverWindows();

    static void CALLBACK winEventCallback(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD dwEventThread,
        DWORD dwmsEventTime
    );
};
