#pragma once

#include <filesystem>

namespace Engine::AssetPath {

std::filesystem::path resolve(const std::filesystem::path& path);

} // namespace Engine::AssetPath
