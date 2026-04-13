#include "TrayManager.hpp"
#include "WindowManager.hpp"
#include "resource.h"
#include <string>

#define WM_TRAYICON (WM_USER + 1)
#define IDI_TRAYICON 1
#define ID_TRAY_TOGGLE_CONSOLE 1001
#define ID_TRAY_EXIT 1002

LRESULT CALLBACK TrayManager::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                         LPARAM lParam) {
  if (uMsg == WM_TRAYICON) {
    TrayManager *manager =
        reinterpret_cast<TrayManager *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager) {
      manager->handleTrayMessage(lParam);
    }
    return 0;
  } else if (uMsg == WM_DISPLAYCHANGE) {
    WindowManager::getInstance().syncMonitors();
    return 0;
  } else if (uMsg == WM_COMMAND) {
    TrayManager *manager =
        reinterpret_cast<TrayManager *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (manager) {
      int wmId = LOWORD(wParam);
      if (wmId == ID_TRAY_TOGGLE_CONSOLE) {
        manager->toggleConsole();
      } else if (wmId == ID_TRAY_EXIT) {
        manager->exitApp();
      }
    }
    return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

TrayManager::TrayManager(HINSTANCE hInstance) : hInst(hInstance) {
  // Hide console by default
  HWND hConsole = GetConsoleWindow();
  if (hConsole) {
    ShowWindow(hConsole, SW_HIDE);
    isConsoleVisible = false;
  }

  const wchar_t CLASS_NAME[] = L"SlopWMTrayHiddenWindowClass";

  WNDCLASSW wc = {};
  wc.lpfnWndProc = TrayManager::WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;

  RegisterClassW(&wc);

  hiddenHwnd = CreateWindowExW(0, CLASS_NAME, L"SlopWMTrayWindow", 0, 0, 0, 0,
                               0, NULL, NULL, hInstance, NULL);

  SetWindowLongPtr(hiddenHwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  nid.cbSize = sizeof(NOTIFYICONDATAW);
  nid.hWnd = hiddenHwnd;
  nid.uID = IDI_TRAYICON;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_TRAYICON;
  nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)); // Use custom icon
  wcscpy_s(nid.szTip, L"SlopWM");

  Shell_NotifyIconW(NIM_ADD, &nid);

  // Show startup notification
  showNotification(L"SlopWM", L"Window manager is running in the background. "
                              L"Double click icon to open console.");
}

TrayManager::~TrayManager() {
  Shell_NotifyIconW(NIM_DELETE, &nid);
  if (hiddenHwnd) {
    DestroyWindow(hiddenHwnd);
  }
}

void TrayManager::showNotification(const wchar_t *title,
                                   const wchar_t *message) {
  nid.uFlags |= NIF_INFO;
  wcscpy_s(nid.szInfoTitle, title);
  wcscpy_s(nid.szInfo, message);
  nid.dwInfoFlags = NIIF_INFO;
  Shell_NotifyIconW(NIM_MODIFY, &nid);
  // Remove NIF_INFO flag so subsequent updates don't keep modifying the balloon
  nid.uFlags &= ~NIF_INFO;
}

void TrayManager::handleTrayMessage(LPARAM lParam) {
  if (LOWORD(lParam) == WM_RBUTTONUP) {
    showContextMenu();
  } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
    toggleConsole();
  }
}

void TrayManager::showContextMenu() {
  POINT pt;
  GetCursorPos(&pt);

  HMENU hMenu = CreatePopupMenu();
  UINT consoleFlags = MF_STRING;
  if (isConsoleVisible)
    consoleFlags |= MF_CHECKED;

  AppendMenuW(hMenu, consoleFlags, ID_TRAY_TOGGLE_CONSOLE,
              L"Show Debug Console");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit SlopWM");

  // SetForegroundWindow is required for the popup menu to disappear correctly
  // when clicking away
  SetForegroundWindow(hiddenHwnd);
  TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0,
                 hiddenHwnd, NULL);
  DestroyMenu(hMenu);
}

void TrayManager::toggleConsole() {
  HWND hConsole = GetConsoleWindow();
  if (hConsole) {
    isConsoleVisible = !isConsoleVisible;
    ShowWindow(hConsole, isConsoleVisible ? SW_SHOW : SW_HIDE);
    if (isConsoleVisible) {
      SetForegroundWindow(hConsole);
    }
  }
}

void TrayManager::exitApp() { PostQuitMessage(0); }
