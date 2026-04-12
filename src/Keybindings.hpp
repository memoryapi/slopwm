#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>

enum class Action {
    ScrollLeft,
    ScrollRight,
    ScrollUp,
    ScrollDown,
    MoveWindowLeft,
    MoveWindowRight,
    MoveWindowUp,
    MoveWindowDown,
    ConsumeOrExpelLeft,
    ConsumeOrExpelRight,
    ConsumeIntoColumn,
    ExpelFromColumn,
    SwitchPresetColumnWidth,
    SwitchPresetWindowHeight,
    ToggleFullscreen,
    MoveToNextMonitor,
    Exit,
    Unknown
};

struct KeyCombination {
    UINT modifiers; // WIN32 modifiers like MOD_ALT, MOD_SHIFT
    UINT key;       // WIN32 Virtual Keys like VK_LEFT, 'H'

    bool operator==(const KeyCombination& other) const {
        return modifiers == other.modifiers && key == other.key;
    }
};

struct KeyCombinationHash {
    std::size_t operator()(const KeyCombination& k) const noexcept {
        return std::hash<UINT>()(k.modifiers) ^ (std::hash<UINT>()(k.key) << 1);
    }
};

class IHotkeyRegistrar {
public:
    virtual ~IHotkeyRegistrar() = default;
    virtual void registerHotkey(int id, const KeyCombination& combo) = 0;
    virtual void unregisterAll() = 0;
};

class Win32HotkeyRegistrar : public IHotkeyRegistrar {
public:
    explicit Win32HotkeyRegistrar(HWND hwnd = NULL);
    ~Win32HotkeyRegistrar() override;

    void registerHotkey(int id, const KeyCombination& combo) override;
    void unregisterAll() override;

private:
    HWND m_hwnd;
    std::vector<int> m_registeredIds;
};

class KeybindingManager {
public:
    explicit KeybindingManager(std::unique_ptr<IHotkeyRegistrar> registrar);
    ~KeybindingManager() = default;

    void addBinding(const KeyCombination& combo, Action action);
    Action getActionForId(int id) const;
    void initializeDefaults();
    void clear();

private:
    std::unique_ptr<IHotkeyRegistrar> m_registrar;
    std::unordered_map<int, Action> m_bindings;
    int m_nextId;
};
