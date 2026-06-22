#include "Engine/Core/Application.h"
#include "Engine/Core/Input.h"

#include <algorithm>

namespace Engine {

Application::Application()
    : window_(1280, 800, "GraphicEngine"), viewportFramebuffer_(1, 1), editor_(window_) {}

void Application::run() {
    onStart();
    bool previousF5 = false;
    bool previousF2 = false;
    bool previousF3 = false;
    bool previousF4 = false;
    bool previousF6 = false;
    bool previousLeftMouse = false;
    bool previousRightMouse = false;
    bool previousPlaying = false;
    bool previousEscape = false;
    BlockType selectedBlock = BlockType::Dirt;
    while (!window_.shouldClose()) {
        const float deltaTime = time_.tick();
        window_.pollEvents();
        viewportFramebuffer_.resize(editor_.viewportWidth(), editor_.viewportHeight());
        editor_.beginFrame();
        editor_.setSelectedBlock(selectedBlock);
        editor_.draw(scene_, time_, viewportFramebuffer_.colorAttachment());
        const bool f5 = Input::keyPressed(window_, GLFW_KEY_F5);
        if (f5 && !previousF5) editor_.togglePlay();
        previousF5 = f5;
        const bool f2 = Input::keyPressed(window_, GLFW_KEY_F2);
        if (f2 && !previousF2) editor_.toggleWireframe();
        previousF2 = f2;
        const bool f3 = Input::keyPressed(window_, GLFW_KEY_F3);
        if (f3 && !previousF3) editor_.toggleFrustumCulling();
        previousF3 = f3;
        const bool f4 = Input::keyPressed(window_, GLFW_KEY_F4);
        if (f4 && !previousF4) editor_.toggleChunkBounds();
        previousF4 = f4;
        const bool f6 = Input::keyPressed(window_, GLFW_KEY_F6);
        if (f6 && !previousF6) editor_.toggleOcclusionCulling();
        previousF6 = f6;
        const bool escape = Input::keyPressed(window_, GLFW_KEY_ESCAPE);
        if (escape && !previousEscape) {
            if (editor_.playing()) editor_.togglePlay();
            else window_.requestClose();
        }
        previousEscape = escape;
        const bool viewportActive = editor_.playing() || editor_.viewportHovered() || editor_.viewportFocused();
        camera_.update(window_, deltaTime, viewportActive, !editor_.playing(), editor_.playing());
        if (editor_.playing() && (!previousPlaying || !player_.active())) {
            if (const auto spawn = renderer_.terrainSpawnPoint(scene_)) player_.spawn(*spawn);
        }
        if (editor_.playing()) player_.update(window_, camera_, renderer_, scene_, deltaTime);
        previousPlaying = editor_.playing();
        if (Input::keyPressed(window_, GLFW_KEY_1)) selectedBlock = BlockType::Grass;
        if (Input::keyPressed(window_, GLFW_KEY_2)) selectedBlock = BlockType::Dirt;
        if (Input::keyPressed(window_, GLFW_KEY_3)) selectedBlock = BlockType::Stone;
        if (Input::keyPressed(window_, GLFW_KEY_4)) selectedBlock = BlockType::Wood;
        if (Input::keyPressed(window_, GLFW_KEY_5)) selectedBlock = BlockType::Leaves;
        const bool leftMouse = glfwGetMouseButton(window_.nativeHandle(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool rightMouse = glfwGetMouseButton(window_.nativeHandle(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        const bool windowFocused = glfwGetWindowAttrib(window_.nativeHandle(), GLFW_FOCUSED) == GLFW_TRUE;
        const bool shiftPressed = Input::keyPressed(window_, GLFW_KEY_LEFT_SHIFT)
            || Input::keyPressed(window_, GLFW_KEY_RIGHT_SHIFT);
        const bool leftClicked = leftMouse && !previousLeftMouse;
        const bool rightClicked = rightMouse && !previousRightMouse;
        if (editor_.playing() && windowFocused && (leftClicked || rightClicked)) {
            const bool placeBlock = rightClicked || (leftClicked && shiftPressed);
            renderer_.editTerrain(scene_, camera_, placeBlock, selectedBlock,
                player_.boundsMin(), player_.boundsMax());
        }
        previousLeftMouse = leftMouse;
        previousRightMouse = rightMouse;
        onUpdate(deltaTime);

        if (window_.width() > 0 && window_.height() > 0) {
            viewportFramebuffer_.bind();
            const float viewportAspect = static_cast<float>(viewportFramebuffer_.width()) /
                static_cast<float>(viewportFramebuffer_.height());
            renderer_.setWireframe(editor_.wireframe());
            renderer_.setFrustumCulling(editor_.frustumCulling());
            renderer_.setChunkBounds(editor_.chunkBounds());
            renderer_.setOcclusionCulling(editor_.occlusionCulling());
            renderer_.render(scene_, camera_, viewportAspect, deltaTime * 1000.0f);
            const RendererStats& stats = renderer_.stats();
            editor_.setRendererStats(stats.triangles, stats.drawCalls,
                stats.visibleChunks, stats.loadedChunks, stats.pendingMeshJobs,
                stats.uploadedMeshes, stats.activeMeshWorkers, stats.activeUploadBudget,
                stats.generatedChunks, stats.pendingChunkJobs,
                stats.cpuRenderMilliseconds, stats.meshProcessingMilliseconds,
                stats.gpuRenderMilliseconds);
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
