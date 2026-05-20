const std = @import("std");
pub const WINAPI = std.builtin.CallingConvention.winapi;

// Core Win32 Types
pub const HWND = ?*anyopaque;
pub const HINSTANCE = ?*anyopaque;
pub const HMONITOR = ?*anyopaque;
pub const HWINEVENTHOOK = ?*anyopaque;
pub const HDWP = ?*anyopaque;
pub const HMENU = ?*anyopaque;
pub const HICON = ?*anyopaque;
pub const HMODULE = ?*anyopaque;
pub const HBRUSH = ?*anyopaque;
pub const HCURSOR = ?*anyopaque;

pub const LRESULT = isize;
pub const WPARAM = usize;
pub const LPARAM = isize;
pub const DWORD = u32;
pub const WORD = u16;
pub const UINT = u32;
pub const LONG = i32;
pub const WCHAR = u16;
pub const LPCWSTR = [*]align(1) const WCHAR;
pub const LPWSTR = [*]align(1) WCHAR;
pub const BOOL = i32;
pub const HRESULT = i32;

pub const TRUE: BOOL = 1;
pub const FALSE: BOOL = 0;

// Structures
pub const RECT = extern struct {
    left: LONG,
    top: LONG,
    right: LONG,
    bottom: LONG,
};

pub const POINT = extern struct {
    x: LONG,
    y: LONG,
};

pub const MSG = extern struct {
    hwnd: HWND,
    message: UINT,
    wParam: WPARAM,
    lParam: LPARAM,
    time: DWORD,
    pt: POINT,
    lPrivate: DWORD,
};

pub const MONITORINFOEXW = extern struct {
    cbSize: DWORD = @sizeOf(MONITORINFOEXW),
    rcMonitor: RECT,
    rcWork: RECT,
    dwFlags: DWORD,
    szDevice: [32]WCHAR,
};

pub const MONITORINFO = extern struct {
    cbSize: DWORD = @sizeOf(MONITORINFO),
    rcMonitor: RECT,
    rcWork: RECT,
    dwFlags: DWORD,
};

pub const WNDCLASSW = extern struct {
    style: UINT,
    lpfnWndProc: WNDPROC,
    cbClsExtra: i32 = 0,
    cbWndExtra: i32 = 0,
    hInstance: HINSTANCE,
    hIcon: HICON = null,
    hCursor: HCURSOR = null,
    hbrBackground: HBRUSH = null,
    lpszMenuName: ?LPCWSTR = null,
    lpszClassName: LPCWSTR,
};

pub const NOTIFYICONDATAW = extern struct {
    cbSize: DWORD = @sizeOf(NOTIFYICONDATAW),
    hWnd: HWND,
    uID: UINT,
    uFlags: UINT,
    uCallbackMessage: UINT,
    hIcon: HICON,
    szTip: [128]WCHAR,
    dwState: DWORD = 0,
    dwStateMask: DWORD = 0,
    szInfo: [256]WCHAR = std.mem.zeroes([256]WCHAR),
    union_u: extern union {
        uTimeout: UINT,
        uVersion: UINT,
    } = .{ .uVersion = 0 },
    szInfoTitle: [64]WCHAR = std.mem.zeroes([64]WCHAR),
    dwInfoFlags: DWORD = 0,
    guidItem: [16]u8 = std.mem.zeroes([16]u8),
    hBalloonIcon: HICON = null,
};

// Callback Types
pub const WNDPROC = *const fn (HWND, UINT, WPARAM, LPARAM) callconv(WINAPI) LRESULT;
pub const WINEVENTPROC = *const fn (HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) callconv(WINAPI) void;
pub const MONITORENUMPROC = *const fn (HMONITOR, ?*anyopaque, ?*RECT, LPARAM) callconv(WINAPI) BOOL;
pub const WNDENUMPROC = *const fn (HWND, LPARAM) callconv(WINAPI) BOOL;

// Constants
pub const GWL_STYLE: i32 = -16;
pub const GWL_EXSTYLE: i32 = -20;
pub const GWLP_USERDATA: i32 = -21;

pub const WS_EX_TOOLWINDOW: u32 = 0x00000080;
pub const WS_CAPTION: u32 = 0x00C00000;

pub const GW_OWNER: UINT = 4;

pub const DWMWA_CLOAKED: DWORD = 14;

pub const MOD_ALT: UINT = 0x0001;
pub const MOD_CONTROL: UINT = 0x0002;
pub const MOD_SHIFT: UINT = 0x0004;
pub const MOD_NOREPEAT: UINT = 0x4000;

pub const VK_LEFT: UINT = 0x25;
pub const VK_UP: UINT = 0x26;
pub const VK_RIGHT: UINT = 0x27;
pub const VK_DOWN: UINT = 0x28;
pub const VK_TAB: UINT = 0x09;
pub const VK_PRIOR: UINT = 0x21; // PgUp
pub const VK_NEXT: UINT = 0x22;  // PgDn
pub const VK_OEM_COMMA: UINT = 0xBC;
pub const VK_OEM_PERIOD: UINT = 0xBE;

pub const WM_USER: UINT = 0x0400;
pub const WM_COMMAND: UINT = 0x0111;
pub const WM_HOTKEY: UINT = 0x0312;
pub const WM_DISPLAYCHANGE: UINT = 0x007E;
pub const WM_TRAYICON: UINT = WM_USER + 1;
pub const WM_RBUTTONUP: UINT = 0x0205;
pub const WM_LBUTTONDBLCLK: UINT = 0x0203;

pub const NIF_MESSAGE: UINT = 0x00000001;
pub const NIF_ICON: UINT = 0x00000002;
pub const NIF_TIP: UINT = 0x00000004;
pub const NIF_INFO: UINT = 0x00000010;

pub const NIM_ADD: DWORD = 0x00000000;
pub const NIM_MODIFY: DWORD = 0x00000001;
pub const NIM_DELETE: DWORD = 0x00000002;

pub const NIIF_INFO: DWORD = 0x00000001;

pub const TPM_RIGHTBUTTON: UINT = 0x0002;
pub const TPM_BOTTOMALIGN: UINT = 0x0020;

pub const MF_STRING: UINT = 0x00000000;
pub const MF_CHECKED: UINT = 0x00000008;
pub const MF_SEPARATOR: UINT = 0x00000800;

pub const SW_HIDE: i32 = 0;
pub const SW_SHOW: i32 = 5;

pub const SWP_NOZORDER: UINT = 0x0004;
pub const SWP_NOACTIVATE: UINT = 0x0010;
pub const SWP_ASYNCWINDOWPOS: UINT = 0x4000;
pub const HWND_TOP: HWND = @ptrFromInt(0);

pub const MONITOR_DEFAULTTONEAREST: DWORD = 2;

// WinEventHook Constants
pub const WINEVENT_OUTOFCONTEXT: DWORD = 0x0000;
pub const WINEVENT_SKIPOWNPROCESS: DWORD = 0x0002;

pub const EVENT_SYSTEM_FOREGROUND: DWORD = 0x0003;
pub const EVENT_SYSTEM_MINIMIZESTART: DWORD = 0x0016;
pub const EVENT_SYSTEM_MINIMIZEEND: DWORD = 0x0017;
pub const EVENT_OBJECT_CREATE: DWORD = 0x8000;
pub const EVENT_OBJECT_DESTROY: DWORD = 0x8001;
pub const EVENT_OBJECT_SHOW: DWORD = 0x8002;
pub const EVENT_OBJECT_HIDE: DWORD = 0x8003;
pub const EVENT_OBJECT_NAMECHANGE: DWORD = 0x800C;
pub const EVENT_OBJECT_UNCLOAKED: DWORD = 0x8018;

pub const OBJID_WINDOW: LONG = 0;
pub const CHILDID_SELF: LONG = 0;

pub const IDI_APP_ICON: UINT = 101;

// Extern Functions
pub extern "user32" fn SetWinEventHook(
    eventMin: DWORD,
    eventMax: DWORD,
    hmodWinEventProc: HMODULE,
    pfnWinEventProc: WINEVENTPROC,
    idProcess: DWORD,
    idThread: DWORD,
    dwFlags: DWORD,
) callconv(WINAPI) HWINEVENTHOOK;

pub extern "user32" fn UnhookWinEvent(hWinEventHook: HWINEVENTHOOK) callconv(WINAPI) BOOL;
pub extern "user32" fn GetForegroundWindow() callconv(WINAPI) HWND;
pub extern "user32" fn GetWindowThreadProcessId(hwnd: HWND, lpdwProcessId: ?*DWORD) callconv(WINAPI) DWORD;
pub extern "kernel32" fn GetCurrentThreadId() callconv(WINAPI) DWORD;
pub extern "user32" fn AttachThreadInput(idAttach: DWORD, idAttachTo: DWORD, fAttach: BOOL) callconv(WINAPI) BOOL;
pub extern "user32" fn SetForegroundWindow(hwnd: HWND) callconv(WINAPI) BOOL;
pub extern "user32" fn SetFocus(hwnd: HWND) callconv(WINAPI) HWND;
pub extern "user32" fn GetWindowRect(hwnd: HWND, lpRect: *RECT) callconv(WINAPI) BOOL;

// 64-bit safety declarations for Window Long APIs
pub extern "user32" fn GetWindowLongW(hwnd: HWND, nIndex: i32) callconv(WINAPI) LONG;
pub extern "user32" fn SetWindowLongW(hwnd: HWND, nIndex: i32, dwNewLong: LONG) callconv(WINAPI) LONG;
pub extern "user32" fn GetWindowLongPtrW(hwnd: HWND, nIndex: i32) callconv(WINAPI) isize;
pub extern "user32" fn SetWindowLongPtrW(hwnd: HWND, nIndex: i32, dwNewLong: isize) callconv(WINAPI) isize;

pub extern "user32" fn SetWindowPos(
    hwnd: HWND,
    hWndInsertAfter: HWND,
    x: i32,
    y: i32,
    cx: i32,
    cy: i32,
    uFlags: UINT,
) callconv(WINAPI) BOOL;

pub extern "user32" fn BeginDeferWindowPos(nNumWindows: i32) callconv(WINAPI) HDWP;
pub extern "user32" fn DeferWindowPos(
    hWinPosInfo: HDWP,
    hwnd: HWND,
    hWndInsertAfter: HWND,
    x: i32,
    y: i32,
    cx: i32,
    cy: i32,
    uFlags: UINT,
) callconv(WINAPI) HDWP;

pub extern "user32" fn EndDeferWindowPos(hWinPosInfo: HDWP) callconv(WINAPI) BOOL;
pub extern "user32" fn IsWindow(hwnd: HWND) callconv(WINAPI) BOOL;
pub extern "user32" fn IsWindowVisible(hwnd: HWND) callconv(WINAPI) BOOL;
pub extern "user32" fn IsIconic(hwnd: HWND) callconv(WINAPI) BOOL;
pub extern "user32" fn IsHungAppWindow(hwnd: HWND) callconv(WINAPI) BOOL;
pub extern "user32" fn GetWindow(hwnd: HWND, uCmd: UINT) callconv(WINAPI) HWND;
pub extern "user32" fn GetWindowTextW(hwnd: HWND, lpString: LPWSTR, nMaxCount: i32) callconv(WINAPI) i32;
pub extern "user32" fn GetClassNameW(hwnd: HWND, lpClassName: LPWSTR, nMaxCount: i32) callconv(WINAPI) i32;

pub extern "user32" fn EnumDisplayMonitors(
    hdc: ?*anyopaque,
    lprcClip: ?*RECT,
    lpfnEnum: MONITORENUMPROC,
    dwData: LPARAM,
) callconv(WINAPI) BOOL;

pub extern "user32" fn GetMonitorInfoW(hMonitor: HMONITOR, lpmi: ?*anyopaque) callconv(WINAPI) BOOL;
pub extern "user32" fn EnumWindows(lpEnumFunc: WNDENUMPROC, lParam: LPARAM) callconv(WINAPI) BOOL;
pub extern "user32" fn RegisterClassW(lpWndClass: *const WNDCLASSW) callconv(WINAPI) WORD;

pub extern "user32" fn CreateWindowExW(
    dwExStyle: DWORD,
    lpClassName: LPCWSTR,
    lpWindowName: ?LPCWSTR,
    dwStyle: DWORD,
    x: i32,
    y: i32,
    nWidth: i32,
    nHeight: i32,
    hWndParent: HWND,
    hMenu: HMENU,
    hInstance: HINSTANCE,
    lpParam: ?*anyopaque,
) callconv(WINAPI) HWND;

pub extern "user32" fn DestroyWindow(hwnd: HWND) callconv(WINAPI) BOOL;
pub extern "user32" fn DefWindowProcW(hwnd: HWND, Msg: UINT, wParam: WPARAM, lParam: LPARAM) callconv(WINAPI) LRESULT;
pub extern "user32" fn PostQuitMessage(nExitCode: i32) callconv(WINAPI) void;
pub extern "user32" fn CreatePopupMenu() callconv(WINAPI) HMENU;
pub extern "user32" fn AppendMenuW(hMenu: HMENU, uFlags: UINT, uIDNewItem: usize, lpNewItem: ?LPCWSTR) callconv(WINAPI) BOOL;
pub extern "user32" fn TrackPopupMenu(
    hMenu: HMENU,
    uFlags: UINT,
    x: i32,
    y: i32,
    nReserved: i32,
    hwnd: HWND,
    prcRect: ?*RECT,
) callconv(WINAPI) BOOL;

pub extern "user32" fn DestroyMenu(hMenu: HMENU) callconv(WINAPI) BOOL;
pub extern "user32" fn GetCursorPos(lpPoint: *POINT) callconv(WINAPI) BOOL;
pub extern "user32" fn MonitorFromPoint(pt: POINT, dwFlags: DWORD) callconv(WINAPI) HMONITOR;
pub extern "user32" fn MonitorFromWindow(hwnd: HWND, dwFlags: DWORD) callconv(WINAPI) HMONITOR;

pub extern "user32" fn RegisterHotKey(hwnd: HWND, id: i32, fsModifiers: UINT, vk: UINT) callconv(WINAPI) BOOL;
pub extern "user32" fn UnregisterHotKey(hwnd: HWND, id: i32) callconv(WINAPI) BOOL;

pub extern "user32" fn TranslateMessage(lpMsg: *const MSG) callconv(WINAPI) BOOL;
pub extern "user32" fn DispatchMessageW(lpMsg: *const MSG) callconv(WINAPI) LRESULT;
pub extern "user32" fn GetMessageW(lpMsg: *MSG, hwnd: HWND, wMsgFilterMin: UINT, wMsgFilterMax: UINT) callconv(WINAPI) BOOL;
pub extern "user32" fn ShowWindow(hwnd: HWND, nCmdShow: i32) callconv(WINAPI) BOOL;

pub const HANDLE = ?*anyopaque;
pub const INVALID_HANDLE_VALUE = @as(HANDLE, @ptrFromInt(@as(usize, @bitCast(@as(isize, -1)))));

pub extern "kernel32" fn GetStdHandle(nStdHandle: DWORD) callconv(WINAPI) HANDLE;
pub extern "kernel32" fn GetConsoleMode(hConsoleHandle: HANDLE, lpMode: *DWORD) callconv(WINAPI) BOOL;
pub extern "kernel32" fn SetConsoleMode(hConsoleHandle: HANDLE, dwMode: DWORD) callconv(WINAPI) BOOL;
pub extern "kernel32" fn GetModuleHandleW(lpModuleName: ?LPCWSTR) callconv(WINAPI) HINSTANCE;

pub extern "kernel32" fn GetConsoleWindow() callconv(WINAPI) HWND;
pub extern "kernel32" fn AllocConsole() callconv(WINAPI) BOOL;
pub extern "kernel32" fn GetLastError() callconv(WINAPI) DWORD;
pub extern "kernel32" fn FormatMessageW(
    dwFlags: DWORD,
    lpSource: ?*anyopaque,
    dwMessageId: DWORD,
    dwLanguageId: DWORD,
    lpBuffer: LPWSTR,
    nSize: DWORD,
    Arguments: ?*anyopaque,
) callconv(WINAPI) DWORD;

pub extern "kernel32" fn LocalFree(hMem: ?*anyopaque) callconv(WINAPI) ?*anyopaque;

pub extern "shell32" fn Shell_NotifyIconW(dwMessage: DWORD, lpData: *NOTIFYICONDATAW) callconv(WINAPI) BOOL;
pub extern "dwmapi" fn DwmGetWindowAttribute(
    hwnd: HWND,
    dwAttribute: DWORD,
    pvAttribute: ?*anyopaque,
    cbAttribute: DWORD,
) callconv(WINAPI) HRESULT;
pub extern "user32" fn LoadIconW(hInstance: HINSTANCE, lpIconName: LPCWSTR) callconv(WINAPI) HICON;
