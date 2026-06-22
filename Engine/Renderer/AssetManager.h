#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Texture2D.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace Engine {

class AssetManager {
public:
    Shader& loadShader(const std::string& name, const std::filesystem::path& vertexPath,
        const std::filesystem::path& fragmentPath);
    Texture2D& loadTexture(const std::string& name, const std::filesystem::path& path);
    Mesh& createCube(const std::string& name);

    Shader& shader(const std::string& name) const;
    Texture2D& texture(const std::string& name) const;
    Mesh& mesh(const std::string& name) const;

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders_;
    std::unordered_map<std::string, std::unique_ptr<Texture2D>> textures_;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes_;
};

} // namespace Engine
