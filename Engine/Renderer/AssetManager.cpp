#include "Engine/Renderer/AssetManager.h"

#include <stdexcept>

namespace Engine {

Shader& AssetManager::loadShader(const std::string& name, const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath) {
    const auto existing = shaders_.find(name);
    if (existing != shaders_.end()) return *existing->second;
    auto resource = std::make_unique<Shader>(vertexPath, fragmentPath);
    Shader& result = *resource;
    shaders_.emplace(name, std::move(resource));
    return result;
}

Texture2D& AssetManager::loadTexture(const std::string& name, const std::filesystem::path& path) {
    const auto existing = textures_.find(name);
    if (existing != textures_.end()) return *existing->second;
    auto resource = Texture2D::loadPpm(path);
    Texture2D& result = *resource;
    textures_.emplace(name, std::move(resource));
    return result;
}

Mesh& AssetManager::createCube(const std::string& name) {
    const auto existing = meshes_.find(name);
    if (existing != meshes_.end()) return *existing->second;
    auto resource = Mesh::createCube();
    Mesh& result = *resource;
    meshes_.emplace(name, std::move(resource));
    return result;
}

Shader& AssetManager::shader(const std::string& name) const {
    const auto found = shaders_.find(name);
    if (found == shaders_.end()) throw std::runtime_error("Shader no cargado: " + name);
    return *found->second;
}

Texture2D& AssetManager::texture(const std::string& name) const {
    const auto found = textures_.find(name);
    if (found == textures_.end()) throw std::runtime_error("Textura no cargada: " + name);
    return *found->second;
}

Mesh& AssetManager::mesh(const std::string& name) const {
    const auto found = meshes_.find(name);
    if (found == meshes_.end()) throw std::runtime_error("Malla no cargada: " + name);
    return *found->second;
}

} // namespace Engine
