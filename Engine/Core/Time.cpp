#include "Engine/Core/Time.h"

#include <GLFW/glfw3.h>
#include <algorithm>

namespace Engine {

Time::Time() : previousTime_(glfwGetTime()) {}

float Time::tick() {
    const double now = glfwGetTime();
    deltaTime_ = static_cast<float>(std::min(now - previousTime_, 0.1));
    previousTime_ = now;
    elapsedTime_ = now;
    return deltaTime_;
}

} // namespace Engine
