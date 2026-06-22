#pragma once

#include "Engine/Scene/Components.h"
#include <cstdint>
#include <optional>

namespace Engine {

class Entity {
public:
    Entity(std::uint32_t id, std::string name) : id_(id), tag_{std::move(name)} {}

    std::uint32_t id() const { return id_; }
    std::optional<std::uint32_t> parentId() const { return parentId_; }
    TagComponent& tag() { return tag_; }
    const TagComponent& tag() const { return tag_; }
    TransformComponent& transform() { return transform_; }
    const TransformComponent& transform() const { return transform_; }

    VoxelGridComponent& addVoxelGrid(const VoxelGridComponent& component = {}) {
        voxelGrid_ = component;
        return *voxelGrid_;
    }
    bool hasVoxelGrid() const { return voxelGrid_.has_value(); }
    const VoxelGridComponent& voxelGrid() const { return voxelGrid_.value(); }

    MaterialComponent& addMaterial(const MaterialComponent& component = {}) {
        material_ = component;
        return *material_;
    }
    bool hasMaterial() const { return material_.has_value(); }
    const MaterialComponent& material() const { return material_.value(); }

    MeshRendererComponent& addMeshRenderer(const MeshRendererComponent& component = {}) {
        meshRenderer_ = component;
        return *meshRenderer_;
    }
    bool hasMeshRenderer() const { return meshRenderer_.has_value(); }
    const MeshRendererComponent& meshRenderer() const { return meshRenderer_.value(); }

    DirectionalLightComponent& addDirectionalLight(const DirectionalLightComponent& component = {}) {
        directionalLight_ = component;
        return *directionalLight_;
    }
    bool hasDirectionalLight() const { return directionalLight_.has_value(); }
    const DirectionalLightComponent& directionalLight() const { return directionalLight_.value(); }

private:
    friend class Scene;
    std::uint32_t id_ = 0;
    std::optional<std::uint32_t> parentId_;
    TagComponent tag_;
    TransformComponent transform_;
    std::optional<VoxelGridComponent> voxelGrid_;
    std::optional<MaterialComponent> material_;
    std::optional<MeshRendererComponent> meshRenderer_;
    std::optional<DirectionalLightComponent> directionalLight_;
};

} // namespace Engine
