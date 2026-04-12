#include "Keybindings.hpp"
#include "Utils.hpp"

// Win32HotkeyRegistrar Implementation

Win32HotkeyRegistrar::Win32HotkeyRegistrar(HWND hwnd) : m_hwnd(hwnd) {}

Win32HotkeyRegistrar::~Win32HotkeyRegistrar() { unregisterAll(); }

void Win32HotkeyRegistrar::registerHotkey(int id, const KeyCombination &combo) {
  if (RegisterHotKey(m_hwnd, id, combo.modifiers, combo.key)) {
    m_registeredIds.push_back(id);
  } else {
    utils::debugLog("Failed to register hotkey with ID {}", id);
  }
}

void Win32HotkeyRegistrar::unregisterAll() {
  for (int id : m_registeredIds) {
    UnregisterHotKey(m_hwnd, id);
  }
  m_registeredIds.clear();
}

// KeybindingManager Implementation

KeybindingManager::KeybindingManager(
    std::unique_ptr<IHotkeyRegistrar> registrar)
    : m_registrar(std::move(registrar)), m_nextId(1) {}

void KeybindingManager::addBinding(const KeyCombination &combo, Action action) {
  int id = m_nextId++;
  m_bindings[id] = action;
  m_registrar->registerHotkey(id, combo);
}

Action KeybindingManager::getActionForId(int id) const {
  auto it = m_bindings.find(id);
  if (it != m_bindings.end()) {
    return it->second;
  }
  return Action::Unknown;
}

void KeybindingManager::initializeDefaults() {
  clear();

  // Default Modifiers
  UINT modDefault = MOD_ALT | MOD_SHIFT | MOD_NOREPEAT;
  UINT modMove = MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT;

  // Arrow keys
  addBinding({modDefault, VK_LEFT}, Action::ScrollLeft);
  addBinding({modDefault, VK_RIGHT}, Action::ScrollRight);
  addBinding({modDefault, VK_UP}, Action::ScrollUp);
  addBinding({modDefault, VK_DOWN}, Action::ScrollDown);
  addBinding({modMove, VK_LEFT}, Action::MoveWindowLeft);
  addBinding({modMove, VK_RIGHT}, Action::MoveWindowRight);
  addBinding({modMove, VK_UP}, Action::MoveWindowUp);
  addBinding({modMove, VK_DOWN}, Action::MoveWindowDown);

  // Vim keys
  addBinding({modDefault, 'H'}, Action::ScrollLeft);
  addBinding({modDefault, 'L'}, Action::ScrollRight);
  addBinding({modDefault, 'K'}, Action::ScrollUp);
  addBinding({modDefault, 'J'}, Action::ScrollDown);
  addBinding({modMove, 'H'}, Action::MoveWindowLeft);
  addBinding({modMove, 'L'}, Action::MoveWindowRight);
  addBinding({modMove, 'K'}, Action::MoveWindowUp);
  addBinding({modMove, 'J'}, Action::MoveWindowDown);

  // Niri columns keys
  addBinding({modMove, VK_OEM_COMMA}, Action::ConsumeOrExpelLeft);
  addBinding({modMove, VK_OEM_PERIOD}, Action::ConsumeOrExpelRight);
  addBinding({modDefault, VK_OEM_COMMA}, Action::ConsumeIntoColumn);
  addBinding({modDefault, VK_OEM_PERIOD}, Action::ExpelFromColumn);

  // Other actions
  addBinding({modDefault, 'E'}, Action::Exit);
  addBinding({modDefault, 'F'}, Action::ToggleFullscreen);
  addBinding({modDefault, 'M'}, Action::MoveToNextMonitor);
}

void KeybindingManager::clear() {
  m_bindings.clear();
  m_registrar->unregisterAll();
  m_nextId = 1;
}
