#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <cstdint>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif

fs::path getExecutableDir() {
    fs::path exePath = fs::current_path(); // fallback
#if defined(_WIN32)
    std::vector<char> buf(MAX_PATH);
    DWORD size = GetModuleFileNameA(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (size > 0 && size < buf.size()) {
        exePath = fs::path(buf.data()).parent_path();
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buf(size);
    if (_NSGetExecutablePath(buf.data(), &size) == 0) {
        exePath = fs::path(buf.data()).parent_path();
    }
#elif defined(__linux__)
    char buf[1024];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (len != -1) {
        buf[len] = '\0';
        exePath = fs::path(buf).parent_path();
    }
#endif
    return exePath;
}