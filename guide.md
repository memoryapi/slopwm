# SlopWM Maintainer's Guide

This guide provides a comprehensive overview of the **SlopWM** codebase, a lightweight tiling window manager for Windows. It is designed to help new maintainers understand the architecture, class responsibilities, and best practices for extending the project.

---

## 1. Project Overview
SlopWM manages windows by projecting them onto a virtual horizontal axis per monitor. It uses a "viewport" approach where windows are scrolled into view rather than being resized to fit a static grid.

### Key Technologies
- **Language**: C++23
- **Platform**: Windows (Win32 API)
- **Build System**: CMake

---

## 2. Architecture & Class Reference

### `WindowManager` (Singleton)
The central orchestrator of the application. It manages the lifecycle of the application and coordinates between monitors and system events.

#### Functions:
- **`getInstance()`**: Returns the global singleton instance.
- **`initialize()`**: Initializes monitors, discovers existing windows, and sets up the global `WinEventHook`.
- **`shutdown()`**: Unhooks events and restores all managed windows to their original positions/styles.
- **`handleHotkey(int id)`**: Processes hotkey messages from `main.cpp`.
- **`onWindowCreated/Destroyed/Focused(HWND)`**: Event handlers that update the state when windows are opened, closed, or focused.
- **`moveFocusedWindowToNextMonitor()`**: Logic for moving a window between physical displays.
- **`discoverMonitors()`**: Uses `EnumDisplayMonitors` to populate the `monitors` list.
- **`discoverWindows()`**: Uses `EnumWindows` to find and track already-open windows on startup.

### `Monitor`
Represents a physical display and its associated workspace. It handles the layout logic for the windows it "owns."

#### Functions:
- **`addWindow(shared_ptr<Window>)`**: Adds a new window to the monitor's tracking list.
- **`removeWindow(HWND)`**: Removes a window and returns it (useful for monitor-to-monitor moves).
- **`scroll(int delta)`**: Shifts the viewport left or right.
- **`moveFocusedWindow(int delta)`**: Swaps the focused window's position in the horizontal stack.
- **`toggleFullscreenOnFocused()`**: Toggles the width scale of the active window.
- **`setLayout()`**: **The Core Logic.** Calculates the virtual projection of all windows and uses `DeferWindowPos` to move them into or out of the viewport.

### `Window`
A lightweight wrapper around a Win32 `HWND`. It stores the window's state and provides abstraction for Win32 calls.

#### Functions:
- **`isManageable()`**: Filters out windows that shouldn't be tiled (tooltips, taskbars, minimized windows, cloaked windows).
- **`restore()`**: Reverts the window to its pre-SlopWM size and position.
- **`setPos(...)`**: Moves/resizes the window, optionally using a `HDWP` handle for batched updates.
- **`focus()`**: Forces the window into the foreground using the `AttachThreadInput` hack to bypass Windows focus restrictions.
- **`toggleFullscreen()`**: Changes the internal `widthScale` (0.5 to 1.0).

### `TrayManager`
Manages the system tray icon, context menu, and the debug console visibility.

#### Functions:
- **`showNotification(...)`**: Displays a Windows toast notification.
- **`toggleConsole()`**: Shows/hides the console window allocated in `Utils`.
- **`handleTrayMessage(...)`**: Routes mouse clicks on the tray icon to actions like opening the menu or toggling the console.

### `utils` (Namespace)
Helper functions for debugging and system interaction.
- **`setupConsole()`**: Allocates a console and redirects `stdout` for `std::println` support.
- **`debugLog(...)`**: A type-safe wrapper for logging to the console.
- **`SystemError`**: A helper struct that converts `GetLastError()` codes into human-readable strings.

---

## 3. Core Workflows

### The "Projection" Layout
Unlike traditional TWMs that divide the screen into tiles, SlopWM assigns each window a `widthScale` (e.g., 0.5 for half-screen). 
1. All windows are laid out on a virtual X-axis.
2. A "viewport" of width 1.0 is moved along this axis to center the focused window.
3. Windows outside the `[viewportOffset, viewportOffset + 1.0]` range are moved off-screen (X=50000).

### Window Tracking
SlopWM uses `SetWinEventHook` to listen for `EVENT_OBJECT_CREATE`, `EVENT_OBJECT_DESTROY`, and `EVENT_SYSTEM_FOREGROUND`. This allows it to react instantly when the user opens a new app or switches windows via Alt+Tab.

---

## 4. Guidelines for Maintainers

### Avoiding Class Coupling
- **Use `shared_ptr`**: Windows are shared between `WindowManager` and `Monitor`. Never use raw pointers for window ownership.
- **Encapsulate Win32**: Keep `HWND` manipulations inside the `Window` class. `Monitor` should only care about *where* a window should go, not *how* to move it.
- **Message Passing**: Use `WindowManager` as a dispatcher. `main.cpp` catches hotkeys and asks `WindowManager` to handle them; `WindowManager` then delegates to the specific `Monitor`.

### Adding New Features
1. **New Hotkeys**: Register the key in `main.cpp` (WinMain), add a case in `WindowManager::handleHotkey`, and implement the logic in `Monitor`.
2. **New Layouts**: If you want to add vertical tiling, modify `Monitor::setLayout`. Consider adding a `LayoutStrategy` interface if the logic becomes complex.
3. **Filtering**: If an app is behaving badly (e.g., a popup that shouldn't be tiled), update `Window::isManageable()`.

### Win32 API Gotchas
- **Focus Stealing**: Windows often prevents apps from taking focus. Always use the `AttachThreadInput` pattern found in `Window::focus()`.
- **Z-Order**: Use `HWND_TOP` in `SetWindowPos` to ensure tiled windows don't get buried behind the desktop or other unmanaged windows.
- **DWM Cloaking**: Some windows (like UWP apps) might exist but be "cloaked." Always check `DWMWA_CLOAKED` before managing a window.

### Debugging
Run the app and right-click the tray icon to **"Show Debug Console."** All `utils::debugLog` calls will appear there in real-time.
