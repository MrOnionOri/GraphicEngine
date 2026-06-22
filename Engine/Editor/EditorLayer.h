#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include "Engine/Voxel/Block.h"

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
    bool playing() const { return playing_; }
    void togglePlay() { playing_ = !playing_; viewportFocusRequested_ = playing_; }
    bool wireframe() const { return wireframe_; }
    void toggleWireframe() { wireframe_ = !wireframe_; }
    bool frustumCulling() const { return frustumCulling_; }
    void toggleFrustumCulling() { frustumCulling_ = !frustumCulling_; }
    bool chunkBounds() const { return chunkBounds_; }
    void toggleChunkBounds() { chunkBounds_ = !chunkBounds_; }
    bool occlusionCulling() const { return occlusionCulling_; }
    void toggleOcclusionCulling() { occlusionCulling_ = !occlusionCulling_; }
    void setRendererStats(std::uint64_t triangles, std::uint32_t drawCalls,
        std::uint32_t visibleChunks, std::uint32_t loadedChunks, std::uint32_t pendingJobs,
        std::uint32_t uploadedMeshes, std::uint32_t activeWorkers,
        std::uint32_t activeUploadBudget, std::uint32_t generatedChunks,
        std::uint32_t pendingChunkJobs, float cpuRenderMs, float meshProcessingMs,
        float gpuRenderMs);
    std::optional<std::pair<int, int>> consumePendingPick();
    void selectEntity(std::uint32_t entityId);
    void setSelectedBlock(BlockType block) { selectedBlock_ = block; }

private:
    std::optional<std::uint32_t> selectedEntity_;
    bool viewportHovered_ = false;
    bool viewportFocused_ = false;
    int viewportWidth_ = 1;
    int viewportHeight_ = 1;
    bool resetLayoutRequested_ = true;
    bool playing_ = false;
    bool viewportFocusRequested_ = false;
    bool wireframe_ = false;
    bool frustumCulling_ = true;
    bool chunkBounds_ = false;
    bool occlusionCulling_ = true;
    BlockType selectedBlock_ = BlockType::Dirt;
    std::uint64_t renderedTriangles_ = 0;
    std::uint32_t drawCalls_ = 0;
    std::uint32_t visibleChunks_ = 0;
    std::uint32_t loadedChunks_ = 0;
    std::uint32_t pendingJobs_ = 0;
    std::uint32_t uploadedMeshes_ = 0;
    std::uint32_t activeWorkers_ = 0;
    std::uint32_t activeUploadBudget_ = 0;
    std::uint32_t generatedChunks_ = 0;
    std::uint32_t pendingChunkJobs_ = 0;
    float cpuRenderMilliseconds_ = 0.0f;
    float meshProcessingMilliseconds_ = 0.0f;
    float gpuRenderMilliseconds_ = 0.0f;
    std::optional<std::pair<int, int>> pendingPick_;
    void buildDefaultLayout(unsigned int dockspaceId);
    void drawViewport(unsigned int texture);
    void drawHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
    void drawStatistics(const Scene& scene, const Time& time);
};

} // namespace Engine
