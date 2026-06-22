#pragma once

#include "Engine/Scene/Entity.h"
#include <vector>
#include <glm/mat4x4.hpp>

namespace Engine {

class Scene {
public:
    Entity& createEntity(const std::string& name = "Entity");
    Entity& createEntityWithId(std::uint32_t id, const std::string& name);
    Entity* findEntity(std::uint32_t id);
    const Entity* findEntity(std::uint32_t id) const;
    void setParent(std::uint32_t childId, std::optional<std::uint32_t> parentId);
    glm::mat4 worldTransform(const Entity& entity) const;
    void clear();
    const std::vector<Entity>& entities() const { return entities_; }

private:
    std::vector<Entity> entities_;
    std::uint32_t nextId_ = 1;
};

} // namespace Engine
