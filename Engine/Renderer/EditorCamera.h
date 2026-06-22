#pragma once

#include <glm/glm.hpp>

namespace Engine {

class Window;

class EditorCamera {
public:
    void update(const Window& window, float deltaTime, bool viewportActive, bool allowMovement = true,
        bool forceMouseLook = false);
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;
    const glm::vec3& position() const { return position_; }
    const glm::vec3& forward() const { return front_; }
    void setPosition(const glm::vec3& position) { position_ = position; }

private:
    glm::vec3 position_{6.0f, 6.0f, -16.0f};
    glm::vec3 front_{0.0f, 0.0f, 1.0f};
    glm::vec3 up_{0.0f, 1.0f, 0.0f};
    float movementSpeed_ = 6.0f;
    float fieldOfView_ = 60.0f;
    float yaw_ = 90.0f;
    float pitch_ = 0.0f;
    float mouseSensitivity_ = 0.12f;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool looking_ = false;
};

} // namespace Engine
