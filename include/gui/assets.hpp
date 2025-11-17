#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include "extern/nanosvgrast.h"
#include "SFML/Graphics.hpp"

/**
 * @return Path to this executable. fs::current_path() as fallback.
 */
fs::path get_executable_dir();

/**
 * @param asset_path relative path within the 'assets' folder next to this executable
 * @return SFML texture rasterized from the svg
 */
sf::Texture load_svg(const fs::path& asset_path);
