const std = @import("std");
const win32 = @import("win32").everything;
const wm_mod = @import("wm.zig");
const key_mod = @import("keybindings.zig");
const tray_mod = @import("tray.zig");

pub var g_wm: ?*wm_mod.WindowManager = null;

fn setupConsole() void {
    if (win32.AllocConsole() != 0) {
        const h_stdin = win32.GetStdHandle(win32.STD_INPUT_HANDLE);
        if (h_stdin != win32.INVALID_HANDLE_VALUE) {
            var mode: win32.CONSOLE_MODE = undefined;
            if (win32.GetConsoleMode(h_stdin, &mode) != 0) {
                mode.ENABLE_QUICK_EDIT_MODE = 0;
                mode.ENABLE_INSERT_MODE = 0;
                mode.ENABLE_EXTENDED_FLAGS = 1;
                _ = win32.SetConsoleMode(h_stdin, mode);
            }
        }
        std.log.info("SlopWM Console Started.", .{});
    }
}

fn winEventCallback(
    h_win_event_hook: ?win32.HWINEVENTHOOK,
    event: u32,
    hwnd: ?win32.HWND,
    id_object: i32,
    id_child: i32,
    dw_event_thread: u32,
    dwms_event_time: u32,
) callconv(.winapi) void {
    _ = h_win_event_hook;
    _ = id_child;
    _ = dw_event_thread;
    _ = dwms_event_time;

    if (id_object != @intFromEnum(win32.OBJID_WINDOW)) return;
    const h = hwnd orelse return;

    if (g_wm) |wm| {
        switch (event) {
            win32.EVENT_OBJECT_CREATE,
            win32.EVENT_OBJECT_SHOW,
            win32.EVENT_OBJECT_UNCLOAKED,
            win32.EVENT_OBJECT_NAMECHANGE,
            win32.EVENT_SYSTEM_MINIMIZEEND,
            => {
                wm.onWindowCreated(h, false) catch |err| {
                    std.log.err("Error handling window created: {}", .{err});
                };
            },
            win32.EVENT_OBJECT_DESTROY,
            win32.EVENT_OBJECT_HIDE,
            win32.EVENT_SYSTEM_MINIMIZESTART,
            => {
                wm.onWindowDestroyed(h);
            },
            win32.EVENT_SYSTEM_FOREGROUND => {
                wm.onWindowFocused(h);
            },
            else => {},
        }
    }
}

fn enumWindowsProc(hwnd: win32.HWND, lparam: win32.LPARAM) callconv(.winapi) win32.BOOL {
    _ = lparam;
    if (g_wm) |wm| {
        wm.onWindowCreated(hwnd, true) catch |err| {
            std.log.err("Error discovering window: {}", .{err});
        };
    }
    return win32.TRUE;
}

pub fn main() !void {
    // Setup gpa allocator
    var gpa = std.heap.DebugAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    setupConsole();

    var wm = wm_mod.WindowManager.init(allocator);
    defer wm.deinit();
    g_wm = &wm;

    // Discover existing monitors and windows
    try wm.syncMonitors();
    _ = win32.EnumWindows(enumWindowsProc, 0);

    // Set WinEventHook
    wm.event_hook = win32.SetWinEventHook(
        win32.EVENT_SYSTEM_FOREGROUND,
        win32.EVENT_OBJECT_UNCLOAKED,
        null,
        winEventCallback,
        0,
        0,
        win32.WINEVENT_OUTOFCONTEXT | win32.WINEVENT_SKIPOWNPROCESS,
    );

    if (wm.event_hook == null) {
        std.log.err("Failed to set win event hook. Error: {}", .{win32.GetLastError()});
        return error.SetWinEventHookFailed;
    }
    defer {
        if (wm.event_hook) |hook| {
            _ = win32.UnhookWinEvent(hook);
        }
    }

    const h_inst = win32.GetModuleHandleW(null) orelse return error.GetModuleHandleFailed;

    // Initialize tray icon
    var tray = try tray_mod.TrayManager.init(h_inst, &wm);
    defer tray.deinit();

    // Initialize keybindings
    var keybindings = key_mod.KeybindingManager.init(allocator);
    defer keybindings.deinit();
    try keybindings.initializeDefaults();

    std.log.info("SlopWM initialized successfully.", .{});

    // Run Message Loop
    var msg = std.mem.zeroes(win32.MSG);
    while (win32.GetMessageW(&msg, null, 0, 0) != 0) {
        if (msg.message == win32.WM_HOTKEY) {
            const id = @as(i32, @intCast(msg.wParam));
            const action = keybindings.getActionForId(id);

            if (action == .Exit) {
                std.log.info("Exit hotkey pressed.", .{});
                break;
            } else if (action != .Unknown) {
                _ = wm.handleAction(action) catch |err| {
                    std.log.err("Error executing action: {}", .{err});
                };
            }
        }
        _ = win32.TranslateMessage(&msg);
        _ = win32.DispatchMessageW(&msg);
    }

    std.log.info("SlopWM shutting down...", .{});
    wm.shutdown();
}
