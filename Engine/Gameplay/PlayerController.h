#pragma once

#include <glm/glm.hpp>

namespace Engine {

class EditorCamera;
class Renderer;
class Scene;
class Window;

class PlayerController {
public:
    void spawn(const glm::vec3& feetPosition);
    void update(const Window& window, EditorCamera& camera, const Renderer& renderer,
        const Scene& scene, float deltaTime);
    bool active() const { return active_; }
    glm::vec3 boundsMin() const;
    glm::vec3 boundsMax() const;

private:
    glm::vec3 feetPosition_{0.0f};
    glm::vec3 spawnPosition_{0.0f};
    float verticalVelocity_ = 0.0f;
    bool grounded_ = false;
    bool previousJump_ = false;
    bool active_ = false;

    bool collides(const glm::vec3& feetPosition, const Renderer& renderer, const Scene& scene) const;
};

} // namespace Engine
