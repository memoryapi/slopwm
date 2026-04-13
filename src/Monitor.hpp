#pragma once
#include <windows.h>
#include <memory>
#include <vector>
#include "WorkspaceManager.hpp"
#include "Window.hpp"

class Monitor {
public:
    Monitor(HMONITOR hmon, RECT workArea);

    HMONITOR getHMonitor() const { return hmon; }
    RECT getWorkArea() const { return workArea; }
    
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
    RECT workArea;
    WorkspaceManager workspaceManager;
};
