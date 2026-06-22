#include "Engine/Core/Application.h"
#include "Engine/Core/Input.h"

#include <algorithm>

namespace Engine {

Application::Application()
    : window_(1280, 800, "GraphicEngine"), viewportFramebuffer_(1, 1), editor_(window_) {}

void Application::run() {
    onStart();
    while (!window_.shouldClose()) {
        const float deltaTime = time_.tick();
        window_.pollEvents();
        viewportFramebuffer_.resize(editor_.viewportWidth(), editor_.viewportHeight());
        editor_.beginFrame();
        editor_.draw(scene_, time_, viewportFramebuffer_.colorAttachment());
        if (Input::keyPressed(window_, GLFW_KEY_ESCAPE)) window_.requestClose();
        if (editor_.viewportHovered() || editor_.viewportFocused()) camera_.update(window_, deltaTime);
        onUpdate(deltaTime);

        if (window_.width() > 0 && window_.height() > 0) {
            viewportFramebuffer_.bind();
            const float viewportAspect = static_cast<float>(viewportFramebuffer_.width()) /
                static_cast<float>(viewportFramebuffer_.height());
            renderer_.render(scene_, camera_, viewportAspect);
            if (const auto pick = editor_.consumePendingPick()) {
                const int pickX = pick->first * viewportFramebuffer_.width() /
                    std::max(1, editor_.viewportWidth());
                const int pickY = pick->second * viewportFramebuffer_.height() /
                    std::max(1, editor_.viewportHeight());
                const std::uint32_t entityId = viewportFramebuffer_.readEntityId(pickX, pickY);
                editor_.selectEntity(scene_.findEntity(entityId) ? entityId : 0);
            }
            Framebuffer::unbind(window_.width(), window_.height());
            glClearColor(0.025f, 0.028f, 0.035f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        editor_.endFrame();
        if (window_.width() > 0 && window_.height() > 0) {
            window_.swapBuffers();
        }
    }
}

} // namespace Engine
