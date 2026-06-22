#include "Engine/Gameplay/PlayerController.h"
#include "Engine/Core/Input.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Scene/Scene.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Engine {

namespace {
constexpr float kEyeHeight = 1.62f;
constexpr float kPlayerHeight = 1.8f;
constexpr float kPlayerRadius = 0.3f;
constexpr float kMovementSpeed = 5.0f;
constexpr float kGravity = 24.0f;
constexpr float kJumpVelocity = 8.5f;
}

void PlayerController::spawn(const glm::vec3& feetPosition) {
    feetPosition_ = feetPosition;
    spawnPosition_ = feetPosition;
    verticalVelocity_ = 0.0f;
    grounded_ = false;
    active_ = true;
}

glm::vec3 PlayerController::boundsMin() const {
    return feetPosition_ + glm::vec3{-kPlayerRadius, 0.0f, -kPlayerRadius};
}

glm::vec3 PlayerController::boundsMax() const {
    return feetPosition_ + glm::vec3{kPlayerRadius, kPlayerHeight, kPlayerRadius};
}

void PlayerController::update(const Window& window, EditorCamera& camera, const Renderer& renderer,
    const Scene& scene, float deltaTime) {
    if (!active_) return;

    glm::vec3 forward{camera.forward().x, 0.0f, camera.forward().z};
    if (glm::length(forward) < 0.001f) forward = {0.0f, 0.0f, 1.0f};
    forward = glm::normalize(forward);
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3{0.0f, 1.0f, 0.0f}));
    glm::vec3 movement{0.0f};
    if (Input::keyPressed(window, GLFW_KEY_W)) movement += forward;
    if (Input::keyPressed(window, GLFW_KEY_S)) movement -= forward;
    if (Input::keyPressed(window, GLFW_KEY_D)) movement += right;
    if (Input::keyPressed(window, GLFW_KEY_A)) movement -= right;
    if (glm::length(movement) > 1.0f) movement = glm::normalize(movement);

    const bool jumpPressed = Input::keyPressed(window, GLFW_KEY_SPACE);
    if (jumpPressed && !previousJump_ && grounded_) {
        verticalVelocity_ = kJumpVelocity;
        grounded_ = false;
    }
    previousJump_ = jumpPressed;

    const int stepCount = std::max(1, static_cast<int>(std::ceil(deltaTime / 0.016f)));
    const float step = deltaTime / static_cast<float>(stepCount);
    for (int index = 0; index < stepCount; ++index) {
        glm::vec3 candidate = feetPosition_;
        candidate.x += movement.x * kMovementSpeed * step;
        if (!collides(candidate, renderer, scene)) feetPosition_.x = candidate.x;

        candidate = feetPosition_;
        candidate.z += movement.z * kMovementSpeed * step;
        if (!collides(candidate, renderer, scene)) feetPosition_.z = candidate.z;

        verticalVelocity_ -= kGravity * step;
        candidate = feetPosition_;
        candidate.y += verticalVelocity_ * step;
        if (collides(candidate, renderer, scene)) {
            if (verticalVelocity_ < 0.0f) grounded_ = true;
            verticalVelocity_ = 0.0f;
        } else {
            feetPosition_.y = candidate.y;
            grounded_ = collides(feetPosition_ + glm::vec3{0.0f, -0.06f, 0.0f}, renderer, scene);
        }
    }

    if (feetPosition_.y < -50.0f) spawn(spawnPosition_);
    camera.setPosition(feetPosition_ + glm::vec3{0.0f, kEyeHeight, 0.0f});
}

bool PlayerController::collides(const glm::vec3& feetPosition, const Renderer& renderer,
    const Scene& scene) const {
    constexpr std::array<float, 2> horizontal{-kPlayerRadius, kPlayerRadius};
    constexpr std::array<float, 3> vertical{0.05f, kPlayerHeight * 0.5f, kPlayerHeight - 0.05f};
    for (float x : horizontal) {
        for (float z : horizontal) {
            for (float y : vertical) {
                if (renderer.isSolidAtWorld(scene, feetPosition + glm::vec3{x, y, z})) return true;
            }
        }
    }
    return false;
}

} // namespace Engine
