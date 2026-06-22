#include "Engine/Scene/Scene.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace Engine {

Entity& Scene::createEntity(const std::string& name) {
    return createEntityWithId(nextId_++, name);
}

Entity& Scene::createEntityWithId(std::uint32_t id, const std::string& name) {
    if (findEntity(id)) throw std::runtime_error("ID de entidad duplicado");
    entities_.emplace_back(id, name);
    nextId_ = std::max(nextId_, id + 1);
    return entities_.back();
}

Entity* Scene::findEntity(std::uint32_t id) {
    for (Entity& entity : entities_) if (entity.id() == id) return &entity;
    return nullptr;
}

const Entity* Scene::findEntity(std::uint32_t id) const {
    for (const Entity& entity : entities_) if (entity.id() == id) return &entity;
    return nullptr;
}

void Scene::setParent(std::uint32_t childId, std::optional<std::uint32_t> parentId) {
    Entity* child = findEntity(childId);
    if (!child) throw std::runtime_error("Entidad hija no encontrada");
    if (parentId && *parentId == childId) throw std::runtime_error("Una entidad no puede ser su propio padre");

    std::optional<std::uint32_t> ancestor = parentId;
    while (ancestor) {
        if (*ancestor == childId) throw std::runtime_error("La jerarquia produciria un ciclo");
        const Entity* entity = findEntity(*ancestor);
        if (!entity) throw std::runtime_error("Entidad padre no encontrada");
        ancestor = entity->parentId();
    }
    child->parentId_ = parentId;
}

glm::mat4 Scene::worldTransform(const Entity& entity) const {
    glm::mat4 result = entity.transform().matrix();
    std::optional<std::uint32_t> parentId = entity.parentId();
    std::unordered_set<std::uint32_t> visited{entity.id()};
    while (parentId) {
        if (!visited.insert(*parentId).second) throw std::runtime_error("Ciclo detectado en la jerarquia");
        const Entity* parent = findEntity(*parentId);
        if (!parent) throw std::runtime_error("Padre de entidad no encontrado");
        result = parent->transform().matrix() * result;
        parentId = parent->parentId();
    }
    return result;
}

void Scene::clear() {
    entities_.clear();
    nextId_ = 1;
}

} // namespace Engine
