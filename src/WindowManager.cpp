#include "WindowManager.hpp"
#include "Utils.hpp"
#include <algorithm>

void CALLBACK WindowManager::winEventCallback(
    HWINEVENTHOOK /*hWinEventHook*/, DWORD event, HWND hwnd,
    LONG idObject, LONG idChild, DWORD /*dwEventThread*/, DWORD /*dwmsEventTime*/) {
    
    if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF) return;
    if (hwnd == nullptr) return;

    auto& wm = WindowManager::getInstance();

    switch (event) {
        case EVENT_OBJECT_CREATE:
        case EVENT_OBJECT_SHOW:
        case EVENT_OBJECT_UNCLOAKED:
        case EVENT_OBJECT_NAMECHANGE:
            wm.onWindowCreated(hwnd);
            break;
        case EVENT_SYSTEM_MINIMIZEEND:
            wm.onWindowCreated(hwnd);
            break;
        case EVENT_OBJECT_DESTROY:
        case EVENT_OBJECT_HIDE:
            wm.onWindowDestroyed(hwnd);
            break;
        case EVENT_SYSTEM_MINIMIZESTART:
            wm.onWindowDestroyed(hwnd);
            break;
        case EVENT_SYSTEM_FOREGROUND:
            wm.onWindowFocused(hwnd);
            break;
    }
}

void WindowManager::initialize() {
    discoverMonitors();
    discoverWindows();

    eventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_OBJECT_UNCLOAKED,
        nullptr,
        winEventCallback,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    if (!eventHook) {
        utils::debugLog("Failed to set win event hook: {}", utils::SystemError::getLast().message);
    }
}

void WindowManager::shutdown() {
    if (eventHook) {
        UnhookWinEvent(eventHook);
        eventHook = nullptr;
    }

    // Restore all windows
    for (auto& mon : monitors) {
        for (const auto& win : mon.getWindows()) {
            win->restore();
        }
    }
    monitors.clear();
}

bool WindowManager::handleAction(Action action) {
    HWND fg = GetForegroundWindow();
    Monitor* m = getMonitorForWindow(fg);
    
    // If no foreground window is managed, fallback to cursor monitor
    if (!m) {
        m = getActiveMonitor();
    }

    if (m) {
        if (action == Action::ScrollLeft) {
            m->scroll(-1);
            return true;
        } else if (action == Action::ScrollRight) {
            m->scroll(1);
            return true;
        } else if (action == Action::ScrollUp) {
            m->scrollVertical(-1);
            return true;
        } else if (action == Action::ScrollDown) {
            m->scrollVertical(1);
            return true;
        } else if (action == Action::MoveWindowLeft) {
            m->moveFocusedWindow(-1);
            return true;
        } else if (action == Action::MoveWindowRight) {
            m->moveFocusedWindow(1);
            return true;
        } else if (action == Action::MoveWindowUp) {
            m->moveFocusedWindowVertical(-1);
            return true;
        } else if (action == Action::MoveWindowDown) {
            m->moveFocusedWindowVertical(1);
            return true;
        } else if (action == Action::ConsumeOrExpelLeft) {
            m->consumeOrExpelLeft();
            return true;
        } else if (action == Action::ConsumeOrExpelRight) {
            m->consumeOrExpelRight();
            return true;
        } else if (action == Action::ConsumeIntoColumn) {
            m->consumeIntoColumn();
            return true;
        } else if (action == Action::ExpelFromColumn) {
            m->expelFromColumn();
            return true;
        } else if (action == Action::SwitchPresetColumnWidth) {
            m->cycleActiveColumnWidth();
            return true;
        } else if (action == Action::SwitchPresetWindowHeight) {
            m->cycleActiveWindowHeight();
            return true;
        } else if (action == Action::FocusWorkspaceDown) {
            m->focusWorkspaceDown();
            return true;
        } else if (action == Action::FocusWorkspaceUp) {
            m->focusWorkspaceUp();
            return true;
        } else if (action == Action::FocusWorkspacePrevious) {
            m->focusWorkspacePrevious();
            return true;
        } else if (action == Action::MoveColumnToWorkspaceDown) {
            m->moveColumnToWorkspaceDown();
            return true;
        } else if (action == Action::MoveColumnToWorkspaceUp) {
            m->moveColumnToWorkspaceUp();
            return true;
        } else if (action == Action::ToggleFullscreen) {
            m->toggleFullscreenOnFocused();
            return true;
        }
    }
    
    // Window manager level hotkeys
    if (action == Action::MoveToNextMonitor) {
        moveFocusedWindowToNextMonitor();
        return true;
    }

    return false;
}

Monitor* WindowManager::getActiveMonitor() {
    POINT pt;
    if (GetCursorPos(&pt)) {
        HMONITOR hmon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        for (auto& m : monitors) {
            if (m.getHMonitor() == hmon) return &m;
        }
    }
    return monitors.empty() ? nullptr : &monitors[0];
}

Monitor* WindowManager::getMonitorForWindow(HWND hwnd) {
    if (!hwnd) return nullptr;
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    for (auto& m : monitors) {
        if (m.getHMonitor() == hmon) return &m;
    }
    return nullptr;
}

void WindowManager::discoverMonitors() {
    monitors.clear();
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hmon, HDC, LPRECT /*rect*/, LPARAM lParam) -> BOOL {
        auto* wm = reinterpret_cast<WindowManager*>(lParam);
        
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfo(hmon, &mi)) {
            wm->monitors.emplace_back(hmon, mi.rcWork);
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(this));
}

void WindowManager::discoverWindows() {
    EnumWindows([](HWND hwnd, LPARAM) -> BOOL {
        WindowManager::getInstance().onWindowCreated(hwnd, true);
        return TRUE;
    }, 0);
}

void WindowManager::onWindowCreated(HWND hwnd, bool isStartup) {
    auto win = std::make_shared<Window>(hwnd);
    if (!win->isManageable()) return;

    for (const auto& mon : monitors) {
        if (mon.hasWindow(hwnd)) return; // Already tracking
    }

    Monitor* m = nullptr;
    if (isStartup) {
        m = getMonitorForWindow(hwnd);
    } else {
        m = getActiveMonitor();
    }
    
    if (!m && !monitors.empty()) m = &monitors[0];

    if (m) {
        utils::debugLog("Tracking window: {:p}", reinterpret_cast<void*>(hwnd));
        m->addWindow(win);
    }
}

void WindowManager::onWindowDestroyed(HWND hwnd) {
    for (auto& m : monitors) {
        if (m.hasWindow(hwnd)) {
            utils::debugLog("Untracking window: {:p}", reinterpret_cast<void*>(hwnd));
            m.removeWindow(hwnd);
            break;
        }
    }
}

void WindowManager::onWindowFocused(HWND hwnd) {
    if (!hwnd) return;
    
    // Sync viewport to focused window if user natively focused it (Alt+Tab, Mouse, TaskView)
    bool tracked = false;
    for (auto& m : monitors) {
        if (m.hasWindow(hwnd)) {
            m.setFocusedWindow(hwnd);
            tracked = true;
            break;
        }
    }

    // If a window gets foreground but wasn't tracked, try tracking it now.
    // This catches apps like Firefox that start unmanageable (cloaked or missing styles)
    // but eventually become manageable and steal foreground.
    if (!tracked) {
        onWindowCreated(hwnd, false);
    }
}

void WindowManager::moveFocusedWindowToNextMonitor() {
    if (monitors.size() <= 1) return;

    HWND fg = GetForegroundWindow();
    if (!fg) return;

    for (size_t i = 0; i < monitors.size(); ++i) {
        if (monitors[i].hasWindow(fg)) {
            // Found the monitor this window belongs to
            auto win = monitors[i].removeWindow(fg);
            if (win) {
                // Move to next monitor
                size_t nextIndex = (i + 1) % monitors.size();
                monitors[nextIndex].addWindow(win);
            }
            break;
        }
    }
}
