#pragma once
#include <windows.h>
#include <memory>
#include <vector>
#include <string>
#include "WorkspaceManager.hpp"
#include "Window.hpp"

class Monitor {
public:
    Monitor(HMONITOR hmon, RECT workArea, std::wstring deviceName);

    HMONITOR getHMonitor() const { return hmon; }
    void setHMonitor(HMONITOR h) { hmon = h; }
    std::wstring getDeviceName() const { return deviceName; }
    RECT getWorkArea() const { return workArea; }
    void setWorkArea(RECT r) { workArea = r; }
    
    WorkspaceManager& getWorkspaceManager() { return workspaceManager; }
    const WorkspaceManager& getWorkspaceManager() const { return workspaceManager; }

    void addWindow(std::shared_ptr<Window> win);
    std::shared_ptr<Window> removeWindow(HWND hwnd);
    bool hasWindow(HWND hwnd) const;
    bool setFocusedWindow(HWND hwnd);

    std::vector<std::shared_ptr<Window>> getWindows() const;

    void updateLayout();

private:
    HMONITOR hmon;
    std::wstring deviceName;
    RECT workArea;
    WorkspaceManager workspaceManager;
};
