#include "Engine/Core/AssetPath.h"

#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace Engine::AssetPath {

std::filesystem::path executableDirectory() {
#ifdef _WIN32
    std::wstring executablePath(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, executablePath.data(),
        static_cast<DWORD>(executablePath.size()));
    if (length > 0) {
        executablePath.resize(length);
        return std::filesystem::path(executablePath).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

std::filesystem::path resolve(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) return std::filesystem::absolute(path);
#ifdef _WIN32
    const auto besideExecutable = executableDirectory() / path;
    if (std::filesystem::exists(besideExecutable)) return besideExecutable;
#endif
    throw std::runtime_error("Asset no encontrado: " + path.string());
}

} // namespace Engine::AssetPath
