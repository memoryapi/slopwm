#pragma once
#include <windows.h>
#include <print>
#include <iostream>
#include <expected>
#include <string>
#include <string_view>
#include <cstdio>

namespace utils {

    inline void debugLog(std::string_view msg) {
        std::println("{}", msg);
    }

    template <typename... Args>
    inline void debugLog(std::format_string<Args...> fmt, Args&&... args) {
        std::println("{}", std::format(fmt, std::forward<Args>(args)...));
    }

    inline void setupConsole() {
        if (AllocConsole()) {
            HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
            if (hStdin != INVALID_HANDLE_VALUE) {
                DWORD mode = 0;
                if (GetConsoleMode(hStdin, &mode)) {
                    // Disable QuickEdit and Insert mode to prevent freezing the WM when clicking the console
                    // ENABLE_EXTENDED_FLAGS is required to modify these specific flags
                    mode &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE);
                    mode |= ENABLE_EXTENDED_FLAGS;
                    SetConsoleMode(hStdin, mode);
                }
            }

            FILE* fp;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
            debugLog("SlopWM Console Started.");
        }
    }

    struct SystemError {
        DWORD code;
        std::string message;

        static SystemError getLast() {
            DWORD err = GetLastError();
            LPSTR msgBuffer = nullptr;
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                         NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msgBuffer, 0, NULL);
            std::string msg(msgBuffer, size);
            LocalFree(msgBuffer);
            return {err, msg};
        }
    };

}
