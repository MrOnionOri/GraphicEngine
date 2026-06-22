#include "Engine/Core/Application.h"
#include "Engine/Core/Input.h"

namespace Engine {

Application::Application() : window_(1100, 720, "GraphicEngine") {}

void Application::run() {
    onStart();
    while (!window_.shouldClose()) {
        const float deltaTime = time_.tick();
        window_.pollEvents();
        if (Input::keyPressed(window_, GLFW_KEY_ESCAPE)) window_.requestClose();
        camera_.update(window_, deltaTime);
        onUpdate(deltaTime);

        if (window_.width() > 0 && window_.height() > 0) {
            renderer_.render(scene_, camera_, window_.aspectRatio());
            window_.swapBuffers();
        }
    }
}

} // namespace Engine
