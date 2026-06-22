#pragma once

#include "Engine/Core/Window.h"

namespace Engine {

class Input {
public:
    static bool keyPressed(const Window& window, int key) {
        return glfwGetKey(window.nativeHandle(), key) == GLFW_PRESS;
    }
};

} // namespace Engine
