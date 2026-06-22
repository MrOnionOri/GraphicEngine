#pragma once

#include <filesystem>

namespace Engine::AssetPath {

std::filesystem::path resolve(const std::filesystem::path& path);
std::filesystem::path executableDirectory();

} // namespace Engine::AssetPath
