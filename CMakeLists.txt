cmake_minimum_required(VERSION 3.20)
project(SlopWM VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# We are building a Windows desktop application.
# Set WIN32 so it uses WinMain as the entry point and doesn't implicitly look for main().
add_executable(slopwm WIN32
    src/main.cpp
    src/WindowManager.cpp
    src/Keybindings.cpp
    src/Monitor.cpp
    src/Column.cpp
    src/Workspace.cpp
    src/WorkspaceManager.cpp
    src/LayoutRenderer.cpp
    src/Window.cpp
    src/TrayManager.cpp
    res/resource.rc
)

target_include_directories(slopwm PRIVATE src res)

# Add strict warnings (MSVC)
if(MSVC)
    target_compile_options(slopwm PRIVATE /W4)
endif()

# Link Windows libraries
target_link_libraries(slopwm PRIVATE 
    User32.lib 
    Gdi32.lib 
    Dwmapi.lib
    Shell32.lib
)
