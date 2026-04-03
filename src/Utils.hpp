#pragma once
#include <windows.h>
#include <print>
#include <iostream>
#include <expected>
#include <string>
#include <string_view>

namespace utils {

    inline void debugLog(std::string_view msg) {
        std::println("{}", msg);
    }

    template <typename... Args>
    inline void debugLog(std::format_string<Args...> fmt, Args&&... args) {
        std::println("{}", std::format(fmt, std::forward<Args>(args)...));
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
