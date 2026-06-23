#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include "Engine/Gameplay/Inventory.h"
#include "Engine/Voxel/Block.h"
#include <imgui.h>

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
    void togglePlay() {
        playing_ = !playing_;
        viewportFocusRequested_ = playing_;
        if (!playing_) closeInventory();
    }
    bool inventoryOpen() const { return inventoryOpen_; }
    bool craftingTableOpen() const { return craftingTableOpen_; }
    bool furnaceOpen() const { return furnaceOpen_; }
    void toggleInventory() {
        inventoryOpen_ = playing_ && !inventoryOpen_;
        craftingTableOpen_ = false;
        furnaceOpen_ = false;
    }
    void openCraftingTable() {
        inventoryOpen_ = playing_;
        craftingTableOpen_ = inventoryOpen_;
        furnaceOpen_ = false;
    }
    void openFurnace() {
        inventoryOpen_ = playing_;
        furnaceOpen_ = inventoryOpen_;
        craftingTableOpen_ = false;
    }
    void closeInventory() { inventoryOpen_ = false; craftingTableOpen_ = false; furnaceOpen_ = false; }
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
    void setInventory(const Inventory& inventory, int selectedHotbarSlot) {
        inventory_ = inventory;
        selectedHotbarSlot_ = selectedHotbarSlot;
    }
    void setCraftingGrid(const CraftingGrid& craftingGrid) { craftingGrid_ = craftingGrid; }
    void setCraftingTableGrid(const CraftingTableGrid& grid) { craftingTableGrid_ = grid; }
    void setFurnaceSlots(const InventorySlot& input, const InventorySlot& fuel,
        const InventorySlot& output, float progress, float burnRatio) {
        furnaceInput_ = input;
        furnaceFuel_ = fuel;
        furnaceOutput_ = output;
        furnaceProgress_ = progress;
        furnaceBurnRatio_ = burnRatio;
    }
    void setCursorItem(const InventorySlot& cursorItem) { cursorItem_ = cursorItem; }
    std::optional<int> consumePendingInventorySlot();
    std::optional<int> consumePendingCraftingSlot();
    std::optional<int> consumePendingCraftingTableSlot();
    bool consumePendingCraftingOutput();
    std::optional<int> consumePendingRightInventorySlot();
    std::optional<int> consumePendingRightCraftingSlot();
    std::optional<int> consumePendingRightCraftingTableSlot();
    bool consumePendingRightCraftingOutput();
    std::optional<int> consumePendingFurnaceSlot();
    std::optional<int> consumePendingRightFurnaceSlot();
    void setMiningProgress(BlockType block, float progress);
    void clearMiningProgress();
    void showActionMessage(std::string message);

private:
    std::optional<std::uint32_t> selectedEntity_;
    bool viewportHovered_ = false;
    bool viewportFocused_ = false;
    int viewportWidth_ = 1;
    int viewportHeight_ = 1;
    bool resetLayoutRequested_ = true;
    bool playing_ = false;
    bool inventoryOpen_ = false;
    bool craftingTableOpen_ = false;
    bool furnaceOpen_ = false;
    bool viewportFocusRequested_ = false;
    bool wireframe_ = false;
    bool frustumCulling_ = true;
    bool chunkBounds_ = false;
    bool occlusionCulling_ = true;
    Inventory inventory_{};
    CraftingGrid craftingGrid_{};
    CraftingTableGrid craftingTableGrid_{};
    InventorySlot furnaceInput_{};
    InventorySlot furnaceFuel_{};
    InventorySlot furnaceOutput_{};
    float furnaceProgress_ = 0.0f;
    float furnaceBurnRatio_ = 0.0f;
    InventorySlot cursorItem_{};
    int selectedHotbarSlot_ = 0;
    std::optional<BlockType> miningBlock_;
    float miningProgress_ = 0.0f;
    std::string actionMessage_;
    float actionMessageSeconds_ = 0.0f;
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
    std::optional<int> pendingInventorySlot_;
    std::optional<int> pendingCraftingSlot_;
    std::optional<int> pendingCraftingTableSlot_;
    bool pendingCraftingOutput_ = false;
    std::optional<int> pendingRightInventorySlot_;
    std::optional<int> pendingRightCraftingSlot_;
    std::optional<int> pendingRightCraftingTableSlot_;
    bool pendingRightCraftingOutput_ = false;
    std::optional<int> pendingFurnaceSlot_;
    std::optional<int> pendingRightFurnaceSlot_;
    void buildDefaultLayout(unsigned int dockspaceId);
    void drawViewport(unsigned int texture);
    void drawInventoryOverlay(ImDrawList* drawList, const ImVec2& imageOrigin,
        const ImVec2& imageSize);
    void drawHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
    void drawStatistics(const Scene& scene, const Time& time);
};

} // namespace Engine
