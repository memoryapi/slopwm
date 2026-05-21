const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "slopwm",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });
    exe.subsystem = .windows;
    const zigwin32_dep = b.dependency("zigwin32", .{});
    exe.root_module.addImport("win32", zigwin32_dep.module("win32"));

    // Link Win32 libraries
    exe.root_module.linkSystemLibrary("user32", .{});
    exe.root_module.linkSystemLibrary("shell32", .{});
    exe.root_module.linkSystemLibrary("dwmapi", .{});
    exe.root_module.linkSystemLibrary("gdi32", .{});
    exe.root_module.linkSystemLibrary("kernel32", .{});

    // Add Windows resources
    exe.root_module.addWin32ResourceFile(.{
        .file = b.path("res/resource.rc"),
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
