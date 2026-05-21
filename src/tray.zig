const std = @import("std");
const win32 = @import("win32").everything;
const wm_mod = @import("wm.zig");
const WindowManager = wm_mod.WindowManager;

const IDI_TRAYICON = 1;
const ID_TRAY_TOGGLE_CONSOLE = 1001;
const ID_TRAY_EXIT = 1002;
const IDI_APP_ICON = 101;
const WM_TRAYICON = win32.WM_USER + 1;

fn copyToUtf16Buf(dest: []u16, source: []const u8) void {
    @memset(dest, 0);
    const view = std.unicode.Utf8View.init(source) catch return;
    var it = view.iterator();
    var idx: usize = 0;
    while (it.nextCodepoint()) |codepoint| {
        const units = if (codepoint <= 0xFFFF) @as(usize, 1) else @as(usize, 2);
        if (idx + units >= dest.len) break;

        if (units == 1) {
            dest[idx] = @intCast(codepoint);
            idx += 1;
        } else {
            const high = @as(u16, @intCast(((codepoint - 0x10000) >> 10) + 0xD800));
            const low = @as(u16, @intCast(((codepoint - 0x10000) & 0x3FF) + 0xDC00));
            dest[idx] = high;
            dest[idx + 1] = low;
            idx += 2;
        }
    }
    dest[idx] = 0;
}

pub const TrayManager = struct {
    hidden_hwnd: ?win32.HWND = null,
    nid: win32.NOTIFYICONDATAW = undefined,
    h_inst: win32.HINSTANCE,
    is_console_visible: bool = false,
    wm: *WindowManager,

    pub fn init(h_inst: win32.HINSTANCE, wm: *WindowManager) !*TrayManager {
        const allocator = wm.allocator;
        const self = try allocator.create(TrayManager);
        errdefer allocator.destroy(self);

        self.* = .{
            .h_inst = h_inst,
            .wm = wm,
        };

        // Hide console window by default
        const h_console = win32.GetConsoleWindow();
        if (h_console) |h| {
            _ = win32.ShowWindow(h, win32.SW_HIDE);
            self.is_console_visible = false;
        }

        const class_name = std.unicode.utf8ToUtf16LeStringLiteral("SlopWMTrayHiddenWindowClass");
        const window_name = std.unicode.utf8ToUtf16LeStringLiteral("SlopWMTrayWindow");

        const wc = win32.WNDCLASSW{
            .style = .{},
            .lpfnWndProc = trayWindowProc,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = h_inst,
            .hIcon = null,
            .hCursor = null,
            .hbrBackground = null,
            .lpszMenuName = null,
            .lpszClassName = class_name,
        };

        _ = win32.RegisterClassW(&wc);

        self.hidden_hwnd = win32.CreateWindowExW(
            .{},
            class_name,
            window_name,
            .{},
            0,
            0,
            0,
            0,
            null,
            null,
            h_inst,
            null,
        ) orelse return error.CreateWindowFailed;

        _ = win32.SetWindowLongPtrW(self.hidden_hwnd.?, win32.GWLP_USERDATA, @intCast(@intFromPtr(self)));

        // Setup NOTIFYICONDATAW
        self.nid = std.mem.zeroes(win32.NOTIFYICONDATAW);
        self.nid.cbSize = @sizeOf(win32.NOTIFYICONDATAW);
        self.nid.hWnd = self.hidden_hwnd;
        self.nid.uID = IDI_TRAYICON;
        self.nid.uFlags = .{
            .ICON = 1,
            .MESSAGE = 1,
            .TIP = 1,
        };
        self.nid.uCallbackMessage = WM_TRAYICON;
        self.nid.hIcon = win32.LoadIconW(h_inst, @ptrFromInt(@as(usize, IDI_APP_ICON)));
        if (self.nid.hIcon == null) {
            // Fallback to default application icon if resource load fails
            self.nid.hIcon = win32.LoadIconW(null, @ptrFromInt(@as(usize, 32512))); // IDI_APPLICATION
        }
        copyToUtf16Buf(&self.nid.szTip, "SlopWM");

        _ = win32.Shell_NotifyIconW(win32.NIM_ADD, &self.nid);

        self.showNotification("SlopWM", "Window manager is running in the background. Double click icon to open console.");

        return self;
    }

    pub fn deinit(self: *TrayManager) void {
        _ = win32.Shell_NotifyIconW(win32.NIM_DELETE, &self.nid);
        if (self.hidden_hwnd) |hwnd| {
            _ = win32.DestroyWindow(hwnd);
        }
        const allocator = self.wm.allocator;
        allocator.destroy(self);
    }

    pub fn showNotification(self: *TrayManager, title: []const u8, message: []const u8) void {
        self.nid.uFlags.INFO = 1;
        copyToUtf16Buf(&self.nid.szInfoTitle, title);
        copyToUtf16Buf(&self.nid.szInfo, message);
        self.nid.dwInfoFlags = win32.NIIF_INFO;

        _ = win32.Shell_NotifyIconW(win32.NIM_MODIFY, &self.nid);

        self.nid.uFlags.INFO = 0;
    }

    pub fn toggleConsole(self: *TrayManager) void {
        const h_console = win32.GetConsoleWindow();
        if (h_console) |h| {
            self.is_console_visible = !self.is_console_visible;
            _ = win32.ShowWindow(h, if (self.is_console_visible) win32.SW_SHOW else win32.SW_HIDE);
            if (self.is_console_visible) {
                _ = win32.SetForegroundWindow(h);
            }
        }
    }

    pub fn showContextMenu(self: *TrayManager) void {
        var pt = win32.POINT{ .x = 0, .y = 0 };
        _ = win32.GetCursorPos(&pt);

        const h_menu = win32.CreatePopupMenu() orelse return;
        defer _ = win32.DestroyMenu(h_menu);

        var console_flags = win32.MENU_ITEM_FLAGS{};
        if (self.is_console_visible) {
            console_flags.CHECKED = 1;
        }

        const show_console_text = std.unicode.utf8ToUtf16LeStringLiteral("Show Debug Console");
        const exit_text = std.unicode.utf8ToUtf16LeStringLiteral("Exit SlopWM");

        _ = win32.AppendMenuW(h_menu, console_flags, ID_TRAY_TOGGLE_CONSOLE, show_console_text);
        _ = win32.AppendMenuW(h_menu, .{ .SEPARATOR = 1 }, 0, null);
        _ = win32.AppendMenuW(h_menu, .{}, ID_TRAY_EXIT, exit_text);

        _ = win32.SetForegroundWindow(self.hidden_hwnd.?);
        const tpm_flags = win32.TRACK_POPUP_MENU_FLAGS{
            .RIGHTBUTTON = 1,
            .BOTTOMALIGN = 1,
        };
        _ = win32.TrackPopupMenu(h_menu, tpm_flags, pt.x, pt.y, 0, self.hidden_hwnd.?, null);
    }
};

fn trayWindowProc(
    hwnd: win32.HWND,
    msg: u32,
    wparam: win32.WPARAM,
    lparam: win32.LPARAM,
) callconv(.winapi) win32.LRESULT {
    const ptr_val = win32.GetWindowLongPtrW(hwnd, win32.GWLP_USERDATA);
    if (ptr_val != 0) {
        const self = @as(*TrayManager, @ptrFromInt(@as(usize, @intCast(ptr_val))));
        switch (msg) {
            WM_TRAYICON => {
                const event = @as(u32, @intCast(lparam));
                if (event == win32.WM_RBUTTONUP) {
                    self.showContextMenu();
                } else if (event == win32.WM_LBUTTONDBLCLK) {
                    self.toggleConsole();
                }
                return 0;
            },
            win32.WM_DISPLAYCHANGE => {
                self.wm.syncMonitors() catch |err| {
                    std.log.err("Failed to sync monitors on display change: {}", .{err});
                };
                return 0;
            },
            win32.WM_COMMAND => {
                const wm_id = @as(u16, @intCast(wparam & 0xFFFF));
                if (wm_id == ID_TRAY_TOGGLE_CONSOLE) {
                    self.toggleConsole();
                } else if (wm_id == ID_TRAY_EXIT) {
                    win32.PostQuitMessage(0);
                }
                return 0;
            },
            else => {},
        }
    }
    return win32.DefWindowProcW(hwnd, msg, wparam, lparam);
}
