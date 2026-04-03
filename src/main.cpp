#include "Utils.hpp"
#include "WindowManager.hpp"
#include "TrayManager.hpp"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                   LPSTR /*pCmdLine*/, int /*nCmdShow*/) {
  auto &wm = WindowManager::getInstance();

  // Attempt to attach a console so we can see std::println outputs
  if (AllocConsole()) {
    FILE *fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    utils::debugLog("SlopWM Console Started.");
  }

  wm.initialize();

  TrayManager tray(hInstance);

  // Register Hotkeys
  // 1: Alt + Shift + Left
  RegisterHotKey(NULL, 1, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, VK_LEFT);
  // 2: Alt + Shift + Right
  RegisterHotKey(NULL, 2, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, VK_RIGHT);
  // 4: Alt + Ctrl + Shift + Left (Move Window Left)
  RegisterHotKey(NULL, 4, MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
                 VK_LEFT);
  // 5: Alt + Ctrl + Shift + Right (Move Window Right)
  RegisterHotKey(NULL, 5, MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
                 VK_RIGHT);

  // Vim keys
  // 11: Alt + Shift + H
  RegisterHotKey(NULL, 11, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'H');
  // 12: Alt + Shift + L
  RegisterHotKey(NULL, 12, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'L');
  // 14: Alt + Ctrl + Shift + H (Move Window Left)
  RegisterHotKey(NULL, 14, MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
                 'H');
  // 15: Alt + Ctrl + Shift + L (Move Window Right)
  RegisterHotKey(NULL, 15, MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
                 'L');

  // 3: Alt + Shift + E
  RegisterHotKey(NULL, 3, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'E');
  // 6: Alt + Shift + F (Toggle Fullscreen Column)
  RegisterHotKey(NULL, 6, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'F');
  // 7: Alt + Shift + M (Move window to next monitor)
  RegisterHotKey(NULL, 7, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'M');

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (msg.message == WM_HOTKEY) {
      int id = static_cast<int>(msg.wParam);
      if (id == 3) {
        // Exit hotkey
        utils::debugLog("Exit hotkey pressed.");
        break;
      } else {
        wm.handleHotkey(id);
      }
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  wm.shutdown();
  return 0;
}
