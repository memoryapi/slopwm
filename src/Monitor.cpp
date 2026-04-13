#include "Monitor.hpp"
#include "LayoutRenderer.hpp"

Monitor::Monitor(HMONITOR hmon, RECT workArea) : hmon(hmon), workArea(workArea) {}

void Monitor::updateLayout() {
    LayoutRenderer::parkInactiveWorkspaces(workspaceManager, workArea);
    LayoutRenderer::renderActiveWorkspace(workspaceManager, workArea);
}

void Monitor::addWindow(std::shared_ptr<Window> win) {
    workspaceManager.addWindowContextually(win);
    updateLayout();
    auto w = workspaceManager.getActiveWorkspace().getFocusedWindow();
    if (w) w->focus();
}

std::shared_ptr<Window> Monitor::removeWindow(HWND hwnd) {
    auto win = workspaceManager.removeWindow(hwnd);
    if (win) {
        updateLayout();
        auto w = workspaceManager.getActiveWorkspace().getFocusedWindow();
        if (w) w->focus();
    }
    return win;
}

bool Monitor::hasWindow(HWND hwnd) const {
    return workspaceManager.hasWindow(hwnd);
}

bool Monitor::setFocusedWindow(HWND hwnd) {
    bool changed = workspaceManager.setFocusedWindow(hwnd);
    if (changed) updateLayout();
    return changed;
}

std::vector<std::shared_ptr<Window>> Monitor::getWindows() const {
    return workspaceManager.getAllWindows();
}
