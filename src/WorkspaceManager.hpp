#pragma once
#include <vector>
#include <memory>
#include "Workspace.hpp"

class WorkspaceManager {
public:
    WorkspaceManager();

    std::vector<Workspace>& getWorkspaces() { return workspaces; }
    const std::vector<Workspace>& getWorkspaces() const { return workspaces; }

    int getActiveIndex() const { return activeWorkspace; }
    Workspace& getActiveWorkspace() { return workspaces[activeWorkspace]; }
    const Workspace& getActiveWorkspace() const { return workspaces[activeWorkspace]; }

    void addWindowContextually(std::shared_ptr<Window> win);
    std::shared_ptr<Window> removeWindow(HWND hwnd);
    bool setFocusedWindow(HWND hwnd);

    void focusWorkspaceDown();
    void focusWorkspaceUp();
    void focusWorkspacePrevious();

    void moveColumnToWorkspaceDown();
    void moveColumnToWorkspaceUp();

    bool hasWindow(HWND hwnd) const;
    std::vector<std::shared_ptr<Window>> getAllWindows() const;

private:
    std::vector<Workspace> workspaces;
    int activeWorkspace = 0;
    int previousWorkspace = 0;

    void setActiveWorkspace(int index);
    void cleanupEmptyWorkspaces();
};
