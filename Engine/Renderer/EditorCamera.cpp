#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Core/Input.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Engine {

void EditorCamera::update(const Window& window, float deltaTime) {
    const float distance = movementSpeed_ * deltaTime;
    const glm::vec3 right = glm::normalize(glm::cross(front_, up_));
    if (Input::keyPressed(window, GLFW_KEY_W)) position_ += front_ * distance;
    if (Input::keyPressed(window, GLFW_KEY_S)) position_ -= front_ * distance;
    if (Input::keyPressed(window, GLFW_KEY_A)) position_ -= right * distance;
    if (Input::keyPressed(window, GLFW_KEY_D)) position_ += right * distance;
    if (Input::keyPressed(window, GLFW_KEY_UP)) position_ += up_ * distance;
    if (Input::keyPressed(window, GLFW_KEY_DOWN)) position_ -= up_ * distance;

    GLFWwindow* handle = window.nativeHandle();
    const bool wantsToLook = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (wantsToLook && !looking_) {
        looking_ = true;
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(handle, &lastMouseX_, &lastMouseY_);
    } else if (!wantsToLook && looking_) {
        looking_ = false;
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (looking_) {
        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(handle, &mouseX, &mouseY);
        yaw_ += static_cast<float>(mouseX - lastMouseX_) * mouseSensitivity_;
        pitch_ -= static_cast<float>(mouseY - lastMouseY_) * mouseSensitivity_;
        pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;

        const float yawRadians = glm::radians(yaw_);
        const float pitchRadians = glm::radians(pitch_);
        front_ = glm::normalize(glm::vec3{
            std::cos(yawRadians) * std::cos(pitchRadians),
            std::sin(pitchRadians),
            std::sin(yawRadians) * std::cos(pitchRadians)
        });
    }
}

glm::mat4 EditorCamera::viewMatrix() const { return glm::lookAt(position_, position_ + front_, up_); }

glm::mat4 EditorCamera::projectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fieldOfView_), aspectRatio, 0.1f, 500.0f);
}

} // namespace Engine
