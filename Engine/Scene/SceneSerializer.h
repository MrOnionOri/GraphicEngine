#pragma once

#include <filesystem>

namespace Engine {

class Scene;

class SceneSerializer {
public:
    static void load(Scene& scene, const std::filesystem::path& path);
    static void save(const Scene& scene, const std::filesystem::path& path);
};

} // namespace Engine
