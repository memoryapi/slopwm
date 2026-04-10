#include "Utils.hpp"
#include "WindowManager.hpp"
#include "TrayManager.hpp"
#include "Keybindings.hpp"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                   LPSTR /*pCmdLine*/, int /*nCmdShow*/) {
  auto &wm = WindowManager::getInstance();

  // Attempt to allocate a console so we can see std::println outputs
  utils::setupConsole();

  wm.initialize();

  TrayManager tray(hInstance);

  KeybindingManager keybindings(std::make_unique<Win32HotkeyRegistrar>());
  keybindings.initializeDefaults();

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (msg.message == WM_HOTKEY) {
      int id = static_cast<int>(msg.wParam);
      Action action = keybindings.getActionForId(id);

      if (action == Action::Exit) {
        utils::debugLog("Exit hotkey pressed.");
        break;
      } else if (action != Action::Unknown) {
        wm.handleAction(action);
      }
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  wm.shutdown();
  return 0;
}
