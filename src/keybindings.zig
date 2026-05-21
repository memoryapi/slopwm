const std = @import("std");
const win32 = @import("win32").everything;

pub const Action = enum {
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
    FocusWorkspaceDown,
    FocusWorkspaceUp,
    FocusWorkspacePrevious,
    MoveColumnToWorkspaceDown,
    MoveColumnToWorkspaceUp,
    ToggleFullscreen,
    MoveToNextMonitor,
    Exit,
    Unknown,
};

pub const KeyCombination = struct {
    modifiers: win32.HOT_KEY_MODIFIERS,
    key: u32,
};

pub const KeybindingManager = struct {
    bindings: std.AutoHashMap(i32, Action),
    registered_ids: std.ArrayList(i32) = .empty,
    next_id: i32 = 1,
    allocator: std.mem.Allocator,

    pub fn init(allocator: std.mem.Allocator) KeybindingManager {
        return .{
            .bindings = std.AutoHashMap(i32, Action).init(allocator),
            .registered_ids = .empty,
            .allocator = allocator,
        };
    }

    pub fn deinit(self: *KeybindingManager) void {
        self.unregisterAll();
        self.bindings.deinit();
        self.registered_ids.deinit(self.allocator);
    }

    pub fn addBinding(self: *KeybindingManager, combo: KeyCombination, action: Action) !void {
        const id = self.next_id;
        self.next_id += 1;
        
        // Passing null for HWND registers global hotkey for this thread
        if (win32.RegisterHotKey(null, id, combo.modifiers, combo.key) != 0) {
            try self.bindings.put(id, action);
            try self.registered_ids.append(self.allocator, id);
        } else {
            std.log.err("Failed to register hotkey with ID {d}. Error: {d}", .{id, win32.GetLastError()});
        }
    }

    pub fn unregisterAll(self: *KeybindingManager) void {
        for (self.registered_ids.items) |id| {
            _ = win32.UnregisterHotKey(null, id);
        }
        self.registered_ids.clearAndFree(self.allocator);
        self.bindings.clearRetainingCapacity();
        self.next_id = 1;
    }

    pub fn getActionForId(self: *const KeybindingManager, id: i32) Action {
        return self.bindings.get(id) orelse Action.Unknown;
    }

    pub fn initializeDefaults(self: *KeybindingManager) !void {
        self.unregisterAll();

        const mod_default = win32.HOT_KEY_MODIFIERS{
            .ALT = 1,
            .SHIFT = 1,
            .NOREPEAT = 1,
        };
        const mod_move = win32.HOT_KEY_MODIFIERS{
            .ALT = 1,
            .CONTROL = 1,
            .SHIFT = 1,
            .NOREPEAT = 1,
        };

        // Arrow keys
        try self.addBinding(.{ .modifiers = mod_default, .key = @intFromEnum(win32.VK_LEFT) }, .ScrollLeft);
        try self.addBinding(.{ .modifiers = mod_default, .key = @intFromEnum(win32.VK_RIGHT) }, .ScrollRight);
        try self.addBinding(.{ .modifiers = mod_default, .key = @intFromEnum(win32.VK_UP) }, .ScrollUp);
        try self.addBinding(.{ .modifiers = mod_default, .key = @intFromEnum(win32.VK_DOWN) }, .ScrollDown);

        try self.addBinding(.{ .modifiers = mod_move, .key = @intFromEnum(win32.VK_LEFT) }, .MoveWindowLeft);
        try self.addBinding(.{ .modifiers = mod_move, .key = @intFromEnum(win32.VK_RIGHT) }, .MoveWindowRight);
        try self.addBinding(.{ .modifiers = mod_move, .key = @intFromEnum(win32.VK_UP) }, .MoveWindowUp);
        try self.addBinding(.{ .modifiers = mod_move, .key = @intFromEnum(win32.VK_DOWN) }, .MoveWindowDown);

        // Vim keys
        try self.addBinding(.{ .modifiers = mod_default, .key = 'H' }, .ScrollLeft);
        try self.addBinding(.{ .modifiers = mod_default, .key = 'L' }, .ScrollRight);
        try self.addBinding(.{ .modifiers = mod_default, .key = 'K' }, .ScrollUp);
        try self.addBinding(.{ .modifiers = mod_default, .key = 'J' }, .ScrollDown);

        try self.addBinding(.{ .modifiers = mod_move, .key = 'H' }, .MoveWindowLeft);
        try self.addBinding(.{ .modifiers = mod_move, .key = 'L' }, .MoveWindowRight);
        try self.addBinding(.{ .modifiers = mod_move, .key = 'K' }, .MoveWindowUp);
        try self.addBinding(.{ .modifiers = mod_move, .key = 'J' }, .MoveWindowDown);

        // Niri columns keys
        try self.addBinding(.{ .modifiers = mod_move, .key = @intFromEnum(win32.VK_OEM_COMMA) }, .ConsumeOrExpelLeft);
        try self.addBinding(.{ .modifiers = mod_move, .key = @intFromEnum(win32.VK_OEM_PERIOD) }, .ConsumeOrExpelRight);
        try self.addBinding(.{ .modifiers = mod_default, .key = @intFromEnum(win32.VK_OEM_COMMA) }, .ConsumeIntoColumn);
        try self.addBinding(.{ .modifiers = mod_default, .key = @intFromEnum(win32.VK_OEM_PERIOD) }, .ExpelFromColumn);

        // Preset bounds keys
        try self.addBinding(.{ .modifiers = mod_default, .key = 'R' }, .SwitchPresetColumnWidth);
        try self.addBinding(.{ .modifiers = mod_move, .key = 'R' }, .SwitchPresetWindowHeight);

        // Workspaces
        try self.addBinding(.{ .modifiers = mod_default, .key = @intFromEnum(win32.VK_NEXT) }, .FocusWorkspaceDown);
        try self.addBinding(.{ .modifiers = mod_default, .key = @intFromEnum(win32.VK_PRIOR) }, .FocusWorkspaceUp);
        try self.addBinding(.{ .modifiers = mod_default, .key = 'U' }, .FocusWorkspaceDown);
        try self.addBinding(.{ .modifiers = mod_default, .key = 'I' }, .FocusWorkspaceUp);
        // try self.addBinding(.{ .modifiers = mod_default, .key = @intFromEnum(win32.VK_TAB) }, .FocusWorkspacePrevious);

        try self.addBinding(.{ .modifiers = mod_move, .key = @intFromEnum(win32.VK_NEXT) }, .MoveColumnToWorkspaceDown);
        try self.addBinding(.{ .modifiers = mod_move, .key = @intFromEnum(win32.VK_PRIOR) }, .MoveColumnToWorkspaceUp);
        try self.addBinding(.{ .modifiers = mod_move, .key = 'U' }, .MoveColumnToWorkspaceDown);
        try self.addBinding(.{ .modifiers = mod_move, .key = 'I' }, .MoveColumnToWorkspaceUp);

        // Other actions
        try self.addBinding(.{ .modifiers = mod_default, .key = 'E' }, .Exit);
        try self.addBinding(.{ .modifiers = mod_default, .key = 'F' }, .ToggleFullscreen);
        try self.addBinding(.{ .modifiers = mod_default, .key = 'M' }, .MoveToNextMonitor);
    }
};
