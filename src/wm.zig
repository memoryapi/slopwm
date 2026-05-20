const std = @import("std");
const win32 = @import("win32.zig");
const key_mod = @import("keybindings.zig");
const HWND = win32.HWND;
const RECT = win32.RECT;
const Allocator = std.mem.Allocator;

pub const PRESET_SCALES = [_]f32{ 1.0 / 3.0, 0.5, 2.0 / 3.0 };

pub fn getNextPresetScale(current_scale: f32) f32 {
    for (PRESET_SCALES) |scale| {
        if (scale > current_scale + 0.01) {
            return scale;
        }
    }
    return PRESET_SCALES[0];
}

/// Backup state of windows for restoration upon shutdown
pub const WindowState = struct {
    original_rect: RECT,
    original_style: win32.LONG,
};

pub const ColumnWindow = struct {
    hwnd: HWND,
    height_scale: f32 = 1.0,
};

pub const Column = struct {
    windows: std.ArrayList(ColumnWindow) = .empty,
    width_scale: f32 = 0.5,

    pub fn deinit(self: *Column, allocator: Allocator) void {
        self.windows.deinit(allocator);
    }

    pub fn initSingle(allocator: Allocator, hwnd: HWND, width_scale: f32) !Column {
        var col = Column{
            .windows = .empty,
            .width_scale = width_scale,
        };
        try col.windows.append(allocator, .{ .hwnd = hwnd, .height_scale = 1.0 });
        return col;
    }

    pub fn insertWindow(self: *Column, allocator: Allocator, hwnd: HWND, index: isize) !void {
        if (self.windows.items.len == 0) {
            try self.windows.append(allocator, .{ .hwnd = hwnd, .height_scale = 1.0 });
            return;
        }

        var total_h: f32 = 0.0;
        for (self.windows.items) |w| {
            total_h += w.height_scale;
        }
        const new_height = total_h / @as(f32, @floatFromInt(self.windows.items.len));

        const insert_idx = if (index < 0 or index >= self.windows.items.len)
            self.windows.items.len
        else
            @as(usize, @intCast(index));

        try self.windows.insert(allocator, insert_idx, .{ .hwnd = hwnd, .height_scale = new_height });
    }

    pub fn removeWindow(self: *Column, index: usize) ColumnWindow {
        return self.windows.orderedRemove(index);
    }

    pub fn equalizeHeights(self: *Column) void {
        if (self.windows.items.len == 0) return;
        const default_scale = 1.0 / @as(f32, @floatFromInt(self.windows.items.len));
        for (self.windows.items) |*w| {
            w.height_scale = default_scale;
        }
    }

    pub fn cycleWindowHeight(self: *Column, index: usize) void {
        if (self.windows.items.len <= 1 or index >= self.windows.items.len) return;

        var total_height_scale: f32 = 0.0;
        for (self.windows.items) |w| {
            total_height_scale += w.height_scale;
        }

        if (total_height_scale > 0.0) {
            for (self.windows.items) |*w| {
                w.height_scale /= total_height_scale;
            }
        }

        const current_prop = self.windows.items[index].height_scale;
        const target_prop = getNextPresetScale(current_prop);
        self.windows.items[index].height_scale = target_prop;

        const remaining_prop = @max(0.0, 1.0 - target_prop);
        const old_remaining_prop = @max(0.0001, 1.0 - current_prop);

        for (self.windows.items, 0..) |*w, r| {
            if (r != index) {
                const old_prop = w.height_scale;
                const new_prop = old_prop * (remaining_prop / old_remaining_prop);
                w.height_scale = new_prop;
            }
        }
    }
};

pub const Workspace = struct {
    columns: std.ArrayList(Column) = .empty,
    focused_column: usize = 0,
    focused_row: usize = 0,
    viewport_offset: f32 = 0.0,

    pub fn deinit(self: *Workspace, allocator: Allocator) void {
        for (self.columns.items) |*col| {
            col.deinit(allocator);
        }
        self.columns.deinit(allocator);
    }

    pub fn ensureFocusLimits(self: *Workspace) void {
        if (self.columns.items.len == 0) {
            self.focused_column = 0;
            self.focused_row = 0;
            return;
        }
        if (self.focused_column >= self.columns.items.len) {
            self.focused_column = self.columns.items.len - 1;
        }
        const col_wins = self.columns.items[self.focused_column].windows.items.len;
        if (col_wins == 0) {
            self.focused_row = 0;
        } else if (self.focused_row >= col_wins) {
            self.focused_row = col_wins - 1;
        }
    }

    pub fn insertColumn(self: *Workspace, allocator: Allocator, col: Column, index: isize) !void {
        const insert_idx = if (index < 0 or index > self.columns.items.len)
            self.columns.items.len
        else
            @as(usize, @intCast(index));

        try self.columns.insert(allocator, insert_idx, col);
        self.ensureFocusLimits();
    }

    pub fn removeColumn(self: *Workspace, index: usize) Column {
        const col = self.columns.orderedRemove(index);
        self.ensureFocusLimits();
        return col;
    }

    pub fn addWindowContextually(self: *Workspace, allocator: Allocator, hwnd: HWND) !void {
        if (self.columns.items.len == 0) {
            var col = Column{};
            try col.insertWindow(allocator, hwnd, -1);
            try self.columns.append(allocator, col);
            self.focused_column = 0;
            self.focused_row = 0;
        } else {
            self.focused_column += 1;
            var col = Column{};
            try col.insertWindow(allocator, hwnd, -1);
            try self.columns.insert(allocator, self.focused_column, col);
            self.focused_row = 0;
        }
        self.ensureFocusLimits();
    }

    pub fn removeWindow(self: *Workspace, allocator: Allocator, hwnd: HWND) ?ColumnWindow {
        for (self.columns.items, 0..) |*col, c| {
            for (col.windows.items, 0..) |win, r| {
                if (win.hwnd == hwnd) {
                    const w = col.removeWindow(r);
                    if (col.windows.items.len == 0) {
                        var removed_col = self.removeColumn(c);
                        removed_col.deinit(allocator);
                    }
                    self.ensureFocusLimits();
                    return w;
                }
            }
        }
        return null;
    }

    pub fn consumeOrExpelLeft(self: *Workspace, allocator: Allocator) !void {
        if (self.columns.items.len == 0) return;
        if (self.focused_column == 0 and self.columns.items[self.focused_column].windows.items.len == 1) return;

        var cur_col = &self.columns.items[self.focused_column];
        if (cur_col.windows.items.len == 1) {
            const win = cur_col.windows.items[0];
            const target_idx = self.focused_column - 1;

            var removed = self.removeColumn(self.focused_column);
            removed.deinit(allocator);

            self.focused_column = target_idx;
            var target_col = &self.columns.items[self.focused_column];
            try target_col.insertWindow(allocator, win.hwnd, -1);
            self.focused_row = target_col.windows.items.len - 1;
        } else {
            const win = cur_col.removeWindow(self.focused_row);
            const new_col = try Column.initSingle(allocator, win.hwnd, cur_col.width_scale);
            try self.columns.insert(allocator, self.focused_column, new_col);
            self.focused_row = 0;
        }
        self.ensureFocusLimits();
    }

    pub fn consumeOrExpelRight(self: *Workspace, allocator: Allocator) !void {
        if (self.columns.items.len == 0) return;
        if (self.focused_column == self.columns.items.len - 1 and self.columns.items[self.focused_column].windows.items.len == 1) return;

        var cur_col = &self.columns.items[self.focused_column];
        if (cur_col.windows.items.len == 1) {
            const win = cur_col.windows.items[0];
            const target_idx = self.focused_column; // right shifts left

            var removed = self.removeColumn(self.focused_column);
            removed.deinit(allocator);

            self.focused_column = target_idx;
            var target_col = &self.columns.items[self.focused_column];
            try target_col.insertWindow(allocator, win.hwnd, -1);
            self.focused_row = target_col.windows.items.len - 1;
        } else {
            const win = cur_col.removeWindow(self.focused_row);
            const new_col = try Column.initSingle(allocator, win.hwnd, cur_col.width_scale);
            self.focused_column += 1;
            try self.columns.insert(allocator, self.focused_column, new_col);
            self.focused_row = 0;
        }
        self.ensureFocusLimits();
    }

    pub fn consumeIntoColumn(self: *Workspace, allocator: Allocator) !void {
        if (self.columns.items.len == 0 or self.focused_column >= self.columns.items.len - 1) return;

        var right_col = &self.columns.items[self.focused_column + 1];
        const win = right_col.removeWindow(0);

        try self.columns.items[self.focused_column].insertWindow(allocator, win.hwnd, -1);

        if (right_col.windows.items.len == 0) {
            var removed = self.removeColumn(self.focused_column + 1);
            removed.deinit(allocator);
        }
        self.ensureFocusLimits();
    }

    pub fn expelFromColumn(self: *Workspace, allocator: Allocator) !void {
        var cur_col = &self.columns.items[self.focused_column];
        if (self.columns.items.len == 0 or cur_col.windows.items.len <= 1) return;

        const win = cur_col.removeWindow(cur_col.windows.items.len - 1);
        self.ensureFocusLimits();

        const new_col = try Column.initSingle(allocator, win.hwnd, cur_col.width_scale);
        try self.columns.insert(allocator, self.focused_column + 1, new_col);
    }

    pub fn scroll(self: *Workspace, delta_column: isize) void {
        if (self.columns.items.len == 0) return;
        const new_val = @as(isize, @intCast(self.focused_column)) + delta_column;
        self.focused_column = @intCast(std.math.clamp(new_val, 0, @as(isize, @intCast(self.columns.items.len - 1))));
        self.ensureFocusLimits();
    }

    pub fn scrollVertical(self: *Workspace, delta_row: isize) void {
        if (self.columns.items.len == 0) return;
        const col_wins = self.columns.items[self.focused_column].windows.items.len;
        if (col_wins == 0) return;
        const new_val = @as(isize, @intCast(self.focused_row)) + delta_row;
        self.focused_row = @intCast(std.math.clamp(new_val, 0, @as(isize, @intCast(col_wins - 1))));
    }

    pub fn moveFocusedWindow(self: *Workspace, delta_column: isize) void {
        if (self.columns.items.len == 0) return;
        const new_val = @as(isize, @intCast(self.focused_column)) + delta_column;
        if (new_val >= 0 and new_val < self.columns.items.len) {
            const new_idx = @as(usize, @intCast(new_val));
            const tmp = self.columns.items[self.focused_column];
            self.columns.items[self.focused_column] = self.columns.items[new_idx];
            self.columns.items[new_idx] = tmp;
            self.focused_column = new_idx;
        }
    }

    pub fn moveFocusedWindowVertical(self: *Workspace, delta_row: isize) void {
        if (self.columns.items.len == 0) return;
        var wins = &self.columns.items[self.focused_column].windows;
        const new_val = @as(isize, @intCast(self.focused_row)) + delta_row;
        if (new_val >= 0 and new_val < wins.items.len) {
            const new_idx = @as(usize, @intCast(new_val));
            const tmp = wins.items[self.focused_row];
            wins.items[self.focused_row] = wins.items[new_idx];
            wins.items[new_idx] = tmp;
            self.focused_row = new_idx;
        }
    }

    pub fn cycleActiveColumnWidth(self: *Workspace) void {
        if (self.columns.items.len == 0) return;
        var col = &self.columns.items[self.focused_column];
        col.width_scale = getNextPresetScale(col.width_scale);
    }

    pub fn cycleActiveWindowHeight(self: *Workspace) void {
        if (self.columns.items.len == 0) return;
        var col = &self.columns.items[self.focused_column];
        col.cycleWindowHeight(self.focused_row);
    }

    pub fn toggleFullscreenOnFocused(self: *Workspace) void {
        if (self.columns.items.len == 0) return;
        var col = &self.columns.items[self.focused_column];
        col.width_scale = if (col.width_scale > 0.5) 0.5 else 1.0;
    }

    pub fn hasWindow(self: *const Workspace, hwnd: HWND) bool {
        for (self.columns.items) |col| {
            for (col.windows.items) |win| {
                if (win.hwnd == hwnd) return true;
            }
        }
        return false;
    }

    pub fn setFocusToWindow(self: *Workspace, hwnd: HWND) bool {
        for (self.columns.items, 0..) |col, c| {
            for (col.windows.items, 0..) |win, r| {
                if (win.hwnd == hwnd) {
                    self.focused_column = c;
                    self.focused_row = r;
                    return true;
                }
            }
        }
        return false;
    }

    pub fn getFocusedWindow(self: *const Workspace) ?HWND {
        if (self.columns.items.len == 0 or self.focused_column >= self.columns.items.len) return null;
        const wins = self.columns.items[self.focused_column].windows.items;
        if (wins.len == 0 or self.focused_row >= wins.len) return null;
        return wins[self.focused_row].hwnd;
    }

    pub fn isEmpty(self: *const Workspace) bool {
        return self.columns.items.len == 0;
    }
};

pub const WorkspaceManager = struct {
    workspaces: std.ArrayList(Workspace) = .empty,
    active_workspace: usize = 0,
    previous_workspace: usize = 0,

    pub fn init(allocator: Allocator) !WorkspaceManager {
        var wm = WorkspaceManager{
            .workspaces = .empty,
        };
        try wm.workspaces.append(allocator, Workspace{});
        return wm;
    }

    pub fn deinit(self: *WorkspaceManager, allocator: Allocator) void {
        for (self.workspaces.items) |*ws| {
            ws.deinit(allocator);
        }
        self.workspaces.deinit(allocator);
    }

    pub fn cleanupEmptyWorkspaces(self: *WorkspaceManager, allocator: Allocator) void {
        var i: isize = @as(isize, @intCast(self.workspaces.items.len)) - 1;
        while (i >= 0) : (i -= 1) {
            const idx = @as(usize, @intCast(i));
            if (self.workspaces.items[idx].isEmpty() and idx != self.active_workspace) {
                var ws = self.workspaces.orderedRemove(idx);
                ws.deinit(allocator);
                if (self.active_workspace > idx) self.active_workspace -= 1;
                if (self.previous_workspace > idx) {
                    self.previous_workspace -= 1;
                } else if (self.previous_workspace == idx) {
                    self.previous_workspace = self.active_workspace;
                }
            }
        }

        if (self.workspaces.items.len == 0) {
            self.workspaces.append(allocator, Workspace{}) catch {};
            self.active_workspace = 0;
            self.previous_workspace = 0;
        } else if (!self.workspaces.items[self.workspaces.items.len - 1].isEmpty()) {
            self.workspaces.append(allocator, Workspace{}) catch {};
        }
    }

    pub fn setActiveWorkspace(self: *WorkspaceManager, allocator: Allocator, index: usize) void {
        if (index >= self.workspaces.items.len or index == self.active_workspace) return;
        self.previous_workspace = self.active_workspace;
        self.active_workspace = index;
        self.cleanupEmptyWorkspaces(allocator);
    }

    pub fn addWindowContextually(self: *WorkspaceManager, allocator: Allocator, hwnd: HWND) !void {
        try self.workspaces.items[self.active_workspace].addWindowContextually(allocator, hwnd);
        self.cleanupEmptyWorkspaces(allocator);
    }

    pub fn removeWindow(self: *WorkspaceManager, allocator: Allocator, hwnd: HWND) ?ColumnWindow {
        for (self.workspaces.items) |*ws| {
            if (ws.removeWindow(allocator, hwnd)) |w| {
                self.cleanupEmptyWorkspaces(allocator);
                return w;
            }
        }
        return null;
    }

    pub fn setFocusedWindow(self: *WorkspaceManager, allocator: Allocator, hwnd: HWND) bool {
        for (self.workspaces.items, 0..) |*ws, wi| {
            if (ws.setFocusToWindow(hwnd)) {
                if (wi != self.active_workspace) {
                    self.setActiveWorkspace(allocator, wi);
                }
                return true;
            }
        }
        return false;
    }

    pub fn focusWorkspaceDown(self: *WorkspaceManager, allocator: Allocator) void {
        if (self.active_workspace < self.workspaces.items.len - 1) {
            self.setActiveWorkspace(allocator, self.active_workspace + 1);
        }
    }

    pub fn focusWorkspaceUp(self: *WorkspaceManager, allocator: Allocator) void {
        if (self.active_workspace > 0) {
            self.setActiveWorkspace(allocator, self.active_workspace - 1);
        }
    }

    pub fn focusWorkspacePrevious(self: *WorkspaceManager, allocator: Allocator) void {
        if (self.previous_workspace < self.workspaces.items.len and self.previous_workspace != self.active_workspace) {
            self.setActiveWorkspace(allocator, self.previous_workspace);
        }
    }

    pub fn moveColumnToWorkspaceDown(self: *WorkspaceManager, allocator: Allocator) !void {
        var ws = &self.workspaces.items[self.active_workspace];
        if (ws.isEmpty()) return;

        const cur_col_idx = ws.focused_column;
        const col = ws.removeColumn(cur_col_idx);

        self.setActiveWorkspace(allocator, self.active_workspace + 1);

        var target_ws = &self.workspaces.items[self.active_workspace];
        const target_idx = if (target_ws.isEmpty()) 0 else target_ws.focused_column + 1;
        try target_ws.insertColumn(allocator, col, @intCast(target_idx));
        target_ws.focused_column = target_idx;
        target_ws.focused_row = 0;

        self.cleanupEmptyWorkspaces(allocator);
    }

    pub fn moveColumnToWorkspaceUp(self: *WorkspaceManager, allocator: Allocator) !void {
        var ws = &self.workspaces.items[self.active_workspace];
        if (ws.isEmpty() or self.active_workspace == 0) return;

        const cur_col_idx = ws.focused_column;
        const col = ws.removeColumn(cur_col_idx);

        self.setActiveWorkspace(allocator, self.active_workspace - 1);

        var target_ws = &self.workspaces.items[self.active_workspace];
        const target_idx = if (target_ws.isEmpty()) 0 else target_ws.focused_column + 1;
        try target_ws.insertColumn(allocator, col, @intCast(target_idx));
        target_ws.focused_column = target_idx;
        target_ws.focused_row = 0;

        self.cleanupEmptyWorkspaces(allocator);
    }

    pub fn hasWindow(self: *const WorkspaceManager, hwnd: HWND) bool {
        for (self.workspaces.items) |ws| {
            if (ws.hasWindow(hwnd)) return true;
        }
        return false;
    }
};

pub const Monitor = struct {
    hmon: win32.HMONITOR,
    device_name: []const u16,
    work_area: RECT,
    workspace_manager: WorkspaceManager,
    allocator: Allocator,

    pub fn init(allocator: Allocator, hmon: win32.HMONITOR, work_area: RECT, device_name: []const u16) !Monitor {
        const name_copy = try allocator.dupe(u16, device_name);
        errdefer allocator.free(name_copy);
        const wsm = try WorkspaceManager.init(allocator);
        return .{
            .hmon = hmon,
            .device_name = name_copy,
            .work_area = work_area,
            .workspace_manager = wsm,
            .allocator = allocator,
        };
    }

    pub fn deinit(self: *Monitor) void {
        self.workspace_manager.deinit(self.allocator);
        self.allocator.free(self.device_name);
    }
};

// Layout Renderer namespace/struct
pub const LayoutRenderer = struct {
    pub fn renderActiveWorkspace(monitor: *Monitor) !void {
        var wsm = &monitor.workspace_manager;
        if (wsm.workspaces.items.len == 0) return;

        var ws = &wsm.workspaces.items[wsm.active_workspace];
        const columns = ws.columns.items;
        if (columns.len == 0) return;

        const work_area = monitor.work_area;
        const mon_width = work_area.right - work_area.left;
        const mon_height = work_area.bottom - work_area.top;

        // Calculate virtual X bounds
        var col_virtual_x = try monitor.allocator.alloc(f32, columns.len);
        defer monitor.allocator.free(col_virtual_x);

        var current_virtual_x: f32 = 0.0;
        for (columns, 0..) |col, c| {
            col_virtual_x[c] = current_virtual_x;
            current_virtual_x += col.width_scale;
        }

        const focused_column = ws.focused_column;
        const focused_start = col_virtual_x[focused_column];
        const focused_width = columns[focused_column].width_scale;
        const focused_end = focused_start + focused_width;

        var viewport_offset = ws.viewport_offset;

        if (columns.len == 1) {
            viewport_offset = -(1.0 - focused_width) / 2.0;
            ws.viewport_offset = viewport_offset;
        } else {
            if (focused_start < viewport_offset) {
                viewport_offset = focused_start;
            }
            if (focused_end > viewport_offset + 1.0) {
                viewport_offset = focused_end - 1.0;
                if (focused_start < viewport_offset) {
                    viewport_offset = focused_start;
                }
            }

            const max_viewport = @max(0.0, current_virtual_x - 1.0);
            viewport_offset = std.math.clamp(viewport_offset, 0.0, max_viewport);
            ws.viewport_offset = viewport_offset;
        }

        var total_windows: usize = 0;
        for (columns) |col| {
            total_windows += col.windows.items.len;
        }

        if (total_windows == 0) return;

        const hdwp = win32.BeginDeferWindowPos(@intCast(total_windows)) orelse {
            std.log.err("BeginDeferWindowPos failed", .{});
            return error.BeginDeferWindowPosFailed;
        };
        errdefer _ = win32.EndDeferWindowPos(hdwp);

        const offscreen_x = work_area.left + 50000;
        const offscreen_y = work_area.top + 50000;

        for (columns, 0..) |col, c| {
            const wins = col.windows.items;
            const start_x = col_virtual_x[c];
            const end_x = start_x + col.width_scale;
            const in_viewport = (start_x < viewport_offset + 1.0 and end_x > viewport_offset);

            var total_height_scale: f32 = 0.0;
            for (wins) |w| {
                total_height_scale += w.height_scale;
            }

            var current_y = work_area.top;
            for (wins, 0..) |win, r| {
                if (win32.IsWindow(win.hwnd) == 0) continue;

                const fraction = win.height_scale / (if (total_height_scale > 0.0) total_height_scale else 1.0);
                var win_height = @as(i32, @intFromFloat(@round(fraction * @as(f32, @floatFromInt(mon_height)))));

                if (r == wins.len - 1) {
                    win_height = work_area.bottom - current_y;
                }

                const pixel_width = @as(i32, @intFromFloat(@round(col.width_scale * @as(f32, @floatFromInt(mon_width)))));

                const flags = if (win32.IsHungAppWindow(win.hwnd) != 0) win32.SWP_ASYNCWINDOWPOS else 0;
                if (in_viewport) {
                    const rel_start = start_x - viewport_offset;
                    const pixel_x = work_area.left + @as(i32, @intFromFloat(@round(rel_start * @as(f32, @floatFromInt(mon_width)))));
                    _ = win32.DeferWindowPos(hdwp, win.hwnd, win32.HWND_TOP, pixel_x, current_y, pixel_width, win_height, flags);
                } else {
                    _ = win32.DeferWindowPos(hdwp, win.hwnd, win32.HWND_TOP, offscreen_x, offscreen_y, pixel_width, win_height, flags);
                }
                current_y += win_height;
            }
        }

        _ = win32.EndDeferWindowPos(hdwp);
    }

    pub fn parkInactiveWorkspaces(monitor: *Monitor) !void {
        const wsm = &monitor.workspace_manager;
        const active_idx = wsm.active_workspace;

        const work_area = monitor.work_area;
        const offscreen_x = work_area.left + 50000;
        const offscreen_y = work_area.top + 50000;
        const mon_width = work_area.right - work_area.left;
        const mon_height = work_area.bottom - work_area.top;

        for (wsm.workspaces.items, 0..) |ws, wi| {
            if (wi == active_idx) continue;

            var ws_wins: usize = 0;
            for (ws.columns.items) |col| {
                ws_wins += col.windows.items.len;
            }
            if (ws_wins == 0) continue;

            const hdwp = win32.BeginDeferWindowPos(@intCast(ws_wins)) orelse continue;
            errdefer _ = win32.EndDeferWindowPos(hdwp);

            for (ws.columns.items) |col| {
                var total_height_scale: f32 = 0.0;
                for (col.windows.items) |win| {
                    total_height_scale += win.height_scale;
                }

                const pixel_width = @as(i32, @intFromFloat(@round(col.width_scale * @as(f32, @floatFromInt(mon_width)))));
                var current_y = work_area.top;

                for (col.windows.items, 0..) |win, r| {
                    if (win32.IsWindow(win.hwnd) == 0) continue;

                    const fraction = win.height_scale / (if (total_height_scale > 0.0) total_height_scale else 1.0);
                    var win_height = @as(i32, @intFromFloat(@round(fraction * @as(f32, @floatFromInt(mon_height)))));

                    if (r == col.windows.items.len - 1) {
                        win_height = work_area.bottom - current_y;
                    }

                    const flags = if (win32.IsHungAppWindow(win.hwnd) != 0) win32.SWP_ASYNCWINDOWPOS else 0;
                    _ = win32.DeferWindowPos(hdwp, win.hwnd, win32.HWND_TOP, offscreen_x, offscreen_y, pixel_width, win_height, flags);
                    current_y += win_height;
                }
            }
            _ = win32.EndDeferWindowPos(hdwp);
        }
    }
};

pub fn isWindowManageable(hwnd: HWND) bool {
    if (win32.IsWindowVisible(hwnd) == 0) return false;
    if (win32.IsIconic(hwnd) != 0) return false;

    const style = win32.GetWindowLongW(hwnd, win32.GWL_STYLE);
    const ex_style = win32.GetWindowLongW(hwnd, win32.GWL_EXSTYLE);

    if ((ex_style & @as(i32, @bitCast(win32.WS_EX_TOOLWINDOW))) != 0) return false;
    if ((style & @as(i32, @bitCast(win32.WS_CAPTION))) == 0) return false;

    const owner = win32.GetWindow(hwnd, win32.GW_OWNER);
    if (owner != null) return false;

    var cloaked: u32 = 0;
    const hr = win32.DwmGetWindowAttribute(hwnd, win32.DWMWA_CLOAKED, &cloaked, @sizeOf(u32));
    if (hr == 0 and cloaked != 0) return false;

    return true;
}

pub fn forceFocusWindow(hwnd: HWND) void {
    if (win32.IsWindow(hwnd) == 0) return;

    const fg_window = win32.GetForegroundWindow();
    const fg_thread = win32.GetWindowThreadProcessId(fg_window, null);
    const my_thread = win32.GetCurrentThreadId();

    if (fg_thread != my_thread) {
        _ = win32.AttachThreadInput(my_thread, fg_thread, win32.TRUE);
        _ = win32.SetForegroundWindow(hwnd);
        _ = win32.SetFocus(hwnd);
        _ = win32.AttachThreadInput(my_thread, fg_thread, win32.FALSE);
    } else {
        _ = win32.SetForegroundWindow(hwnd);
        _ = win32.SetFocus(hwnd);
    }
}

pub const WindowManager = struct {
    monitors: std.ArrayList(Monitor) = .empty,
    registry: std.AutoHashMap(HWND, WindowState),
    event_hook: ?win32.HWINEVENTHOOK = null,
    allocator: Allocator,

    pub fn init(allocator: Allocator) WindowManager {
        return .{
            .monitors = .empty,
            .registry = std.AutoHashMap(HWND, WindowState).init(allocator),
            .allocator = allocator,
        };
    }

    pub fn deinit(self: *WindowManager) void {
        self.monitors.deinit(self.allocator);
        self.registry.deinit();
    }

    pub fn discoverMonitors(self: *WindowManager) !void {
        const enum_proc = struct {
            fn cb(hmon: win32.HMONITOR, hdc: ?*anyopaque, rect: ?*win32.RECT, lparam: win32.LPARAM) callconv(win32.WINAPI) win32.BOOL {
                _ = hdc; _ = rect;
                const wm = @as(*WindowManager, @ptrFromInt(@as(usize, @intCast(lparam))));
                var mi = std.mem.zeroes(win32.MONITORINFOEXW);
                mi.cbSize = @sizeOf(win32.MONITORINFOEXW);
                if (win32.GetMonitorInfoW(hmon, &mi) != 0) {
                    var len: usize = 0;
                    while (len < mi.szDevice.len and mi.szDevice[len] != 0) : (len += 1) {}
                    const device_name = mi.szDevice[0..len];

                    var monitor = Monitor.init(wm.allocator, hmon, mi.rcWork, device_name) catch |err| {
                        std.log.err("Failed to init monitor: {}", .{err});
                        return win32.TRUE;
                    };
                    wm.monitors.append(wm.allocator, monitor) catch |err| {
                        monitor.deinit();
                        std.log.err("Failed to append monitor: {}", .{err});
                    };
                }
                return win32.TRUE;
            }
        }.cb;

        _ = win32.EnumDisplayMonitors(null, null, enum_proc, @intCast(@intFromPtr(self)));
    }

    pub fn syncMonitors(self: *WindowManager) !void {
        const DisplayInfo = struct {
            hmon: win32.HMONITOR,
            rect: win32.RECT,
            device_name: [32]u16,
            name_len: usize,
        };

        var active_displays = std.ArrayList(DisplayInfo).empty;
        defer active_displays.deinit(self.allocator);

        const EnumContext = struct {
            displays: *std.ArrayList(DisplayInfo),
            allocator: Allocator,
        };
        
        var ctx = EnumContext{ .displays = &active_displays, .allocator = self.allocator };

        const enum_callback = struct {
            fn cb(hmon: win32.HMONITOR, hdc: ?*anyopaque, rect: ?*win32.RECT, lparam: win32.LPARAM) callconv(win32.WINAPI) win32.BOOL {
                _ = hdc; _ = rect;
                const c = @as(*EnumContext, @ptrFromInt(@as(usize, @intCast(lparam))));
                var mi = std.mem.zeroes(win32.MONITORINFOEXW);
                mi.cbSize = @sizeOf(win32.MONITORINFOEXW);
                if (win32.GetMonitorInfoW(hmon, &mi) != 0) {
                    var len: usize = 0;
                    while (len < mi.szDevice.len and mi.szDevice[len] != 0) : (len += 1) {}
                    var name: [32]u16 = undefined;
                    std.mem.copyForwards(u16, &name, mi.szDevice[0..len]);
                    c.displays.append(c.allocator, .{
                        .hmon = hmon,
                        .rect = mi.rcWork,
                        .device_name = name,
                        .name_len = len,
                    }) catch {};
                }
                return win32.TRUE;
            }
        }.cb;

        _ = win32.EnumDisplayMonitors(null, null, enum_callback, @intCast(@intFromPtr(&ctx)));

        var new_monitors = std.ArrayList(Monitor).empty;
        errdefer {
            for (new_monitors.items) |*m| m.deinit();
            new_monitors.deinit(self.allocator);
        }

        for (active_displays.items) |display| {
            var found = false;
            var i: usize = 0;
            while (i < self.monitors.items.len) {
                var old_m = &self.monitors.items[i];
                if (std.mem.eql(u16, old_m.device_name, display.device_name[0..display.name_len])) {
                    old_m.work_area = display.rect;
                    old_m.hmon = display.hmon;
                    try new_monitors.append(self.allocator, self.monitors.orderedRemove(i));
                    found = true;
                    break;
                }
                i += 1;
            }
            if (!found) {
                const m = try Monitor.init(self.allocator, display.hmon, display.rect, display.device_name[0..display.name_len]);
                try new_monitors.append(self.allocator, m);
            }
        }

        if (new_monitors.items.len > 0) {
            var target_wsm = &new_monitors.items[0].workspace_manager;
            for (self.monitors.items) |*old_m| {
                var survives = false;
                for (active_displays.items) |display| {
                    if (std.mem.eql(u16, old_m.device_name, display.device_name[0..display.name_len])) {
                        survives = true;
                        break;
                    }
                }
                if (!survives) {
                    for (old_m.workspace_manager.workspaces.items) |*ws| {
                        if (!ws.isEmpty()) {
                            const insert_pos = if (target_wsm.workspaces.items.len > 0) target_wsm.workspaces.items.len - 1 else 0;
                            try target_wsm.workspaces.insert(self.allocator, insert_pos, ws.*);
                            ws.* = Workspace{};
                        }
                    }
                    target_wsm.cleanupEmptyWorkspaces(self.allocator);
                }
            }
        }

        for (self.monitors.items) |*old_m| {
            old_m.deinit();
        }
        self.monitors.deinit(self.allocator);
        self.monitors = new_monitors;

        for (self.monitors.items) |*m| {
            try LayoutRenderer.parkInactiveWorkspaces(m);
            try LayoutRenderer.renderActiveWorkspace(m);
        }
    }

    pub fn getActiveMonitor(self: *WindowManager) ?*Monitor {
        var pt = win32.POINT{ .x = 0, .y = 0 };
        if (win32.GetCursorPos(&pt) != 0) {
            const hmon = win32.MonitorFromPoint(pt, win32.MONITOR_DEFAULTTONEAREST);
            for (self.monitors.items) |*m| {
                if (m.hmon == hmon) return m;
            }
        }
        return if (self.monitors.items.len > 0) &self.monitors.items[0] else null;
    }

    pub fn getMonitorForWindow(self: *WindowManager, hwnd: HWND) ?*Monitor {
        if (hwnd == null) return null;
        const hmon = win32.MonitorFromWindow(hwnd, win32.MONITOR_DEFAULTTONEAREST);
        for (self.monitors.items) |*m| {
            if (m.hmon == hmon) return m;
        }
        return null;
    }

    pub fn moveFocusedWindowToNextMonitor(self: *WindowManager) !void {
        if (self.monitors.items.len <= 1) return;
        const fg = win32.GetForegroundWindow() orelse return;

        for (self.monitors.items, 0..) |*mon, i| {
            if (mon.workspace_manager.hasWindow(fg)) {
                if (mon.workspace_manager.removeWindow(self.allocator, fg)) |win| {
                    const next_idx = (i + 1) % self.monitors.items.len;
                    var next_mon = &self.monitors.items[next_idx];
                    try next_mon.workspace_manager.addWindowContextually(self.allocator, win.hwnd);

                    try LayoutRenderer.parkInactiveWorkspaces(mon);
                    try LayoutRenderer.renderActiveWorkspace(mon);
                    try LayoutRenderer.parkInactiveWorkspaces(next_mon);
                    try LayoutRenderer.renderActiveWorkspace(next_mon);

                    if (next_mon.workspace_manager.workspaces.items[next_mon.workspace_manager.active_workspace].getFocusedWindow()) |w| {
                        forceFocusWindow(w);
                    }
                }
                break;
            }
        }
    }

    pub fn handleAction(self: *WindowManager, action: key_mod.Action) !bool {
        if (action == .MoveToNextMonitor) {
            try self.moveFocusedWindowToNextMonitor();
            return true;
        }

        const fg = win32.GetForegroundWindow();
        var m = self.getMonitorForWindow(fg);
        if (m == null) {
            m = self.getActiveMonitor();
        }

        if (m) |mon| {
            var wsm = &mon.workspace_manager;
            var ws = &wsm.workspaces.items[wsm.active_workspace];

            switch (action) {
                .ScrollLeft => ws.scroll(-1),
                .ScrollRight => ws.scroll(1),
                .ScrollUp => ws.scrollVertical(-1),
                .ScrollDown => ws.scrollVertical(1),
                .MoveWindowLeft => ws.moveFocusedWindow(-1),
                .MoveWindowRight => ws.moveFocusedWindow(1),
                .MoveWindowUp => ws.moveFocusedWindowVertical(-1),
                .MoveWindowDown => ws.moveFocusedWindowVertical(1),
                .ConsumeOrExpelLeft => try ws.consumeOrExpelLeft(self.allocator),
                .ConsumeOrExpelRight => try ws.consumeOrExpelRight(self.allocator),
                .ConsumeIntoColumn => try ws.consumeIntoColumn(self.allocator),
                .ExpelFromColumn => try ws.expelFromColumn(self.allocator),
                .SwitchPresetColumnWidth => ws.cycleActiveColumnWidth(),
                .SwitchPresetWindowHeight => ws.cycleActiveWindowHeight(),
                .FocusWorkspaceDown => wsm.focusWorkspaceDown(self.allocator),
                .FocusWorkspaceUp => wsm.focusWorkspaceUp(self.allocator),
                .FocusWorkspacePrevious => wsm.focusWorkspacePrevious(self.allocator),
                .MoveColumnToWorkspaceDown => try wsm.moveColumnToWorkspaceDown(self.allocator),
                .MoveColumnToWorkspaceUp => try wsm.moveColumnToWorkspaceUp(self.allocator),
                .ToggleFullscreen => ws.toggleFullscreenOnFocused(),
                else => return false,
            }

            try LayoutRenderer.parkInactiveWorkspaces(mon);
            try LayoutRenderer.renderActiveWorkspace(mon);
            if (wsm.workspaces.items[wsm.active_workspace].getFocusedWindow()) |w| {
                forceFocusWindow(w);
            }
            return true;
        }
        return false;
    }

    pub fn onWindowCreated(self: *WindowManager, hwnd: HWND, is_startup: bool) !void {
        if (!isWindowManageable(hwnd)) return;

        for (self.monitors.items) |mon| {
            if (mon.workspace_manager.hasWindow(hwnd)) return;
        }

        var m: ?*Monitor = null;
        if (is_startup) {
            m = self.getMonitorForWindow(hwnd);
        } else {
            m = self.getActiveMonitor();
        }

        if (m == null and self.monitors.items.len > 0) {
            m = &self.monitors.items[0];
        }

        if (m) |mon| {
            std.log.info("Tracking window: {*}", .{hwnd});
            var original_rect = std.mem.zeroes(win32.RECT);
            _ = win32.GetWindowRect(hwnd, &original_rect);
            const original_style = win32.GetWindowLongW(hwnd, win32.GWL_STYLE);
            
            try self.registry.put(hwnd, .{ .original_rect = original_rect, .original_style = original_style });

            try mon.workspace_manager.addWindowContextually(self.allocator, hwnd);
            try LayoutRenderer.parkInactiveWorkspaces(mon);
            try LayoutRenderer.renderActiveWorkspace(mon);
        }
    }

    pub fn onWindowDestroyed(self: *WindowManager, hwnd: HWND) void {
        for (self.monitors.items) |*mon| {
            if (mon.workspace_manager.hasWindow(hwnd)) {
                std.log.info("Untracking window: {*}", .{hwnd});
                _ = mon.workspace_manager.removeWindow(self.allocator, hwnd);
                _ = self.registry.remove(hwnd);
                
                LayoutRenderer.parkInactiveWorkspaces(mon) catch {};
                LayoutRenderer.renderActiveWorkspace(mon) catch {};
                
                if (mon.workspace_manager.workspaces.items[mon.workspace_manager.active_workspace].getFocusedWindow()) |w| {
                    forceFocusWindow(w);
                }
                break;
            }
        }
    }

    pub fn onWindowFocused(self: *WindowManager, hwnd: HWND) void {
        if (hwnd == null) return;

        var tracked = false;
        for (self.monitors.items) |*mon| {
            if (mon.workspace_manager.hasWindow(hwnd)) {
                _ = mon.workspace_manager.setFocusedWindow(self.allocator, hwnd);
                
                LayoutRenderer.parkInactiveWorkspaces(mon) catch {};
                LayoutRenderer.renderActiveWorkspace(mon) catch {};
                tracked = true;
                break;
            }
        }

        if (!tracked) {
            self.onWindowCreated(hwnd, false) catch |err| {
                std.log.err("Error creating window on focus: {}", .{err});
            };
        }
    }

    pub fn shutdown(self: *WindowManager) void {
        if (self.event_hook) |hook| {
            _ = win32.UnhookWinEvent(hook);
            self.event_hook = null;
        }

        var it = self.registry.iterator();
        while (it.next()) |entry| {
            const hwnd = entry.key_ptr.*;
            const state = entry.value_ptr.*;
            if (win32.IsWindow(hwnd) != 0) {
                _ = win32.SetWindowLongW(hwnd, win32.GWL_STYLE, state.original_style);
                const flags = (win32.SWP_NOZORDER | win32.SWP_NOACTIVATE) | (if (win32.IsHungAppWindow(hwnd) != 0) win32.SWP_ASYNCWINDOWPOS else 0);
                _ = win32.SetWindowPos(
                    hwnd,
                    null,
                    state.original_rect.left,
                    state.original_rect.top,
                    state.original_rect.right - state.original_rect.left,
                    state.original_rect.bottom - state.original_rect.top,
                    flags,
                );
            }
        }

        for (self.monitors.items) |*m| {
            m.deinit();
        }
        self.monitors.clearAndFree(self.allocator);
        self.registry.clearAndFree();
    }
};
