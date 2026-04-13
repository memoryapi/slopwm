#include "WorkspaceManager.hpp"
#include <algorithm>

WorkspaceManager::WorkspaceManager() {
    workspaces.push_back(Workspace{});
}

void WorkspaceManager::cleanupEmptyWorkspaces() {
    for (int i = static_cast<int>(workspaces.size()) - 1; i >= 0; --i) {
        if (workspaces[i].isEmpty() && i != activeWorkspace) {
            workspaces.erase(workspaces.begin() + i);
            if (activeWorkspace > i) activeWorkspace--;
            if (previousWorkspace > i) previousWorkspace--;
            else if (previousWorkspace == i) previousWorkspace = activeWorkspace; 
        }
    }
    
    if (workspaces.empty()) {
        workspaces.push_back(Workspace{});
        activeWorkspace = 0;
        previousWorkspace = 0;
    } else if (!workspaces.back().isEmpty()) {
        workspaces.push_back(Workspace{});
    }
}

void WorkspaceManager::setActiveWorkspace(int index) {
    if (index < 0 || index >= static_cast<int>(workspaces.size()) || index == activeWorkspace) return;
    previousWorkspace = activeWorkspace;
    activeWorkspace = index;
    cleanupEmptyWorkspaces(); 
}

void WorkspaceManager::addWindowContextually(std::shared_ptr<Window> win) {
    workspaces[activeWorkspace].addWindowContextually(win);
    cleanupEmptyWorkspaces();
}

std::shared_ptr<Window> WorkspaceManager::removeWindow(HWND hwnd) {
    for (int wi = 0; wi < static_cast<int>(workspaces.size()); ++wi) {
        auto win = workspaces[wi].removeWindow(hwnd);
        if (win) {
            cleanupEmptyWorkspaces();
            return win;
        }
    }
    return nullptr;
}

bool WorkspaceManager::setFocusedWindow(HWND hwnd) {
    for (int wi = 0; wi < static_cast<int>(workspaces.size()); ++wi) {
        if (workspaces[wi].setFocusToWindow(hwnd)) {
            if (wi != activeWorkspace) {
                setActiveWorkspace(wi);
            }
            return true;
        }
    }
    return false;
}

void WorkspaceManager::focusWorkspaceDown() {
    if (activeWorkspace < static_cast<int>(workspaces.size()) - 1) {
        setActiveWorkspace(activeWorkspace + 1);
    }
}

void WorkspaceManager::focusWorkspaceUp() {
    if (activeWorkspace > 0) {
        setActiveWorkspace(activeWorkspace - 1);
    }
}

void WorkspaceManager::focusWorkspacePrevious() {
    if (previousWorkspace >= 0 && previousWorkspace < static_cast<int>(workspaces.size()) && previousWorkspace != activeWorkspace) {
        setActiveWorkspace(previousWorkspace);
    }
}

void WorkspaceManager::moveColumnToWorkspaceDown() {
    auto& ws = workspaces[activeWorkspace];
    if (ws.isEmpty()) return;

    auto curColIdx = ws.getFocusedColumn();
    auto col = std::move(ws.getColumns()[curColIdx]);
    ws.removeColumn(curColIdx);

    setActiveWorkspace(activeWorkspace + 1);

    auto& targetWs = workspaces[activeWorkspace];
    int targetIdx = targetWs.isEmpty() ? 0 : targetWs.getFocusedColumn() + 1;
    targetWs.insertColumn(std::move(col), targetIdx);
    targetWs.setFocusedColumn(targetIdx);
    targetWs.setFocusedRow(0);

    cleanupEmptyWorkspaces();
}

void WorkspaceManager::moveColumnToWorkspaceUp() {
    auto& ws = workspaces[activeWorkspace];
    if (ws.isEmpty() || activeWorkspace == 0) return;

    auto curColIdx = ws.getFocusedColumn();
    auto col = std::move(ws.getColumns()[curColIdx]);
    ws.removeColumn(curColIdx);

    setActiveWorkspace(activeWorkspace - 1);

    auto& targetWs = workspaces[activeWorkspace];
    int targetIdx = targetWs.isEmpty() ? 0 : targetWs.getFocusedColumn() + 1;
    targetWs.insertColumn(std::move(col), targetIdx);
    targetWs.setFocusedColumn(targetIdx);
    targetWs.setFocusedRow(0);

    cleanupEmptyWorkspaces();
}

bool WorkspaceManager::hasWindow(HWND hwnd) const {
    for (const auto& ws : workspaces) {
        if (ws.hasWindow(hwnd)) return true;
    }
    return false;
}

std::vector<std::shared_ptr<Window>> WorkspaceManager::getAllWindows() const {
    std::vector<std::shared_ptr<Window>> allWins;
    for (const auto& ws : workspaces) {
        for (const auto& col : ws.getColumns()) {
            for (const auto& win : col.getWindows()) {
                if (win->isValid()) allWins.push_back(win);
            }
        }
    }
    return allWins;
}
