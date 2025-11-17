#include "gui/assets.hpp"

#include <stdio.h>
#include <string.h>
#include <math.h>

// NanoSVG implementation
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "extern/nanosvgrast.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <cstdint>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif


fs::path get_executable_dir() {
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


sf::Texture load_svg(const fs::path& asset_path) {
    fs::path file = get_executable_dir() / "assets" / asset_path;

    // Parse SVG
    NSVGimage* image = nsvgParseFromFile(file.c_str(), "px", 96.0f);
    if (!image) {
        throw std::runtime_error("Failed to parse SVG: " + file.string());
    }

    // Rasterize to a buffer
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    constexpr int render_size = 128;
    std::vector<unsigned char> bitmap(render_size * render_size * 4);
    nsvgRasterize(rast, image, 0, 0, (float)render_size / image->width,
                    bitmap.data(), render_size, render_size, render_size * 4);
    nsvgDelete(image);
    nsvgDeleteRasterizer(rast);

    // Create texture
    sf::Image img(sf::Vector2u(render_size, render_size), bitmap.data());
    sf::Texture tex;
    if(!tex.loadFromImage(img)){
        throw std::runtime_error("Failed to create image from rasterized svg: " + file.string());
    }
    tex.setSmooth(true);

    return tex;
}

