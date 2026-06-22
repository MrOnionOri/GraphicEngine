#pragma once

#include <cstdint>
#include <optional>
#include <utility>

namespace Engine {

class Scene;
class Time;
class Window;

class EditorLayer {
public:
    explicit EditorLayer(const Window& window);
    ~EditorLayer();
    EditorLayer(const EditorLayer&) = delete;
    EditorLayer& operator=(const EditorLayer&) = delete;

    void beginFrame();
    void draw(Scene& scene, const Time& time, unsigned int viewportTexture);
    void endFrame();
    bool wantsMouse() const;
    bool wantsKeyboard() const;
    bool viewportHovered() const { return viewportHovered_; }
    bool viewportFocused() const { return viewportFocused_; }
    int viewportWidth() const { return viewportWidth_; }
    int viewportHeight() const { return viewportHeight_; }
    std::optional<std::pair<int, int>> consumePendingPick();
    void selectEntity(std::uint32_t entityId);

private:
    std::optional<std::uint32_t> selectedEntity_;
    bool viewportHovered_ = false;
    bool viewportFocused_ = false;
    int viewportWidth_ = 1;
    int viewportHeight_ = 1;
    bool resetLayoutRequested_ = true;
    std::optional<std::pair<int, int>> pendingPick_;
    void buildDefaultLayout(unsigned int dockspaceId);
    void drawViewport(unsigned int texture);
    void drawHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
    void drawStatistics(const Scene& scene, const Time& time);
};

} // namespace Engine
