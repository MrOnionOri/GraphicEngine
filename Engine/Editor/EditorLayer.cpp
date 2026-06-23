#include "Engine/Editor/EditorLayer.h"
#include "Engine/Core/AssetPath.h"
#include "Engine/Core/Time.h"
#include "Engine/Core/Window.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneSerializer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <thread>

namespace Engine {

EditorLayer::EditorLayer(const Window& window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "GraphicEngineEditor.ini";
    resetLayoutRequested_ = !std::filesystem::exists(io.IniFilename);
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

EditorLayer::~EditorLayer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport();
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Layout")) {
            if (ImGui::MenuItem("Reset Default Layout")) resetLayoutRequested_ = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Game")) {
            if (ImGui::MenuItem(playing_ ? "Stop" : "Play", "F5")) togglePlay();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            if (ImGui::MenuItem("Triangle Wireframe", "F2", wireframe_)) toggleWireframe();
            if (ImGui::MenuItem("Frustum Culling", "F3", frustumCulling_)) toggleFrustumCulling();
            if (ImGui::MenuItem("Chunk Bounds", "F4", chunkBounds_)) toggleChunkBounds();
            if (ImGui::MenuItem("Occlusion Culling", "F6", occlusionCulling_)) toggleOcclusionCulling();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (resetLayoutRequested_) buildDefaultLayout(dockspaceId);
}

void EditorLayer::draw(Scene& scene, const Time& time, unsigned int viewportTexture) {
    actionMessageSeconds_ = std::max(0.0f, actionMessageSeconds_ - time.deltaTime());
    drawViewport(viewportTexture);
    drawHierarchy(scene);
    drawInspector(scene);
    drawStatistics(scene, time);
}

void EditorLayer::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool EditorLayer::wantsMouse() const { return inventoryOpen_ || ImGui::GetIO().WantCaptureMouse; }
bool EditorLayer::wantsKeyboard() const { return inventoryOpen_ || ImGui::GetIO().WantCaptureKeyboard; }

void EditorLayer::drawViewport(unsigned int texture) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Scene View");
    if (viewportFocusRequested_) {
        ImGui::SetWindowFocus();
        viewportFocusRequested_ = false;
    }
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 imageSize{std::max(1.0f, available.x), std::max(1.0f, available.y)};
    const ImVec2 imageOrigin = ImGui::GetCursorScreenPos();
    viewportWidth_ = static_cast<int>(imageSize.x);
    viewportHeight_ = static_cast<int>(imageSize.y);
    viewportHovered_ = ImGui::IsWindowHovered();
    viewportFocused_ = ImGui::IsWindowFocused();
    ImGui::Image(ImTextureRef(static_cast<ImTextureID>(texture)), imageSize,
        ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    if (!playing_ && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetMousePos();
        const int x = static_cast<int>(mouse.x - imageOrigin.x);
        const int y = static_cast<int>(imageSize.y - (mouse.y - imageOrigin.y));
        pendingPick_ = std::pair<int, int>{x, y};
    }
    if (playing_) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        constexpr float slotSize = 46.0f;
        constexpr float gap = 5.0f;
        const float hotbarWidth = slotSize * static_cast<float>(InventoryHotbarSlots)
            + gap * static_cast<float>(InventoryHotbarSlots - 1);
        const float hotbarX = imageOrigin.x + (imageSize.x - hotbarWidth) * 0.5f;
        const float hotbarY = imageOrigin.y + imageSize.y - slotSize - 14.0f;
        if (!inventoryOpen_) {
            const ImVec2 center{imageOrigin.x + imageSize.x * 0.5f, imageOrigin.y + imageSize.y * 0.5f};
            const ImU32 color = IM_COL32(255, 255, 255, 210);
            drawList->AddLine({center.x - 7.0f, center.y}, {center.x + 7.0f, center.y}, color, 1.5f);
            drawList->AddLine({center.x, center.y - 7.0f}, {center.x, center.y + 7.0f}, color, 1.5f);

            for (int slot = 0; slot < InventoryHotbarSlots; ++slot) {
                const InventorySlot& inventorySlot = inventory_.slots[slot];
                const bool empty = inventorySlot.empty();
                const ImVec2 minimum{hotbarX + slot * (slotSize + gap), hotbarY};
                const ImVec2 maximum{minimum.x + slotSize, minimum.y + slotSize};
                const BlockColor blockColor = itemColor(inventorySlot.itemId);
                drawList->AddRectFilled(minimum, maximum, IM_COL32(18, 20, 23, 220), 3.0f);
                if (!empty) {
                    drawList->AddRectFilled({minimum.x + 7.0f, minimum.y + 7.0f},
                        {maximum.x - 7.0f, maximum.y - 7.0f},
                        IM_COL32(blockColor.r, blockColor.g, blockColor.b, blockColor.a), 2.0f);
                }
                const bool selected = selectedHotbarSlot_ == slot;
                drawList->AddRect(minimum, maximum,
                    selected ? IM_COL32(255, 238, 120, 255) : IM_COL32(145, 150, 160, 210),
                    3.0f, 0, selected ? 3.0f : 1.0f);
                drawList->AddText({minimum.x + 4.0f, minimum.y + 2.0f},
                    IM_COL32(255, 255, 255, 255), std::to_string(slot + 1).c_str());
                if (!empty) {
                    const std::string countText = std::to_string(inventorySlot.count);
                    const ImVec2 countSize = ImGui::CalcTextSize(countText.c_str());
                    drawList->AddText({maximum.x - countSize.x - 5.0f, maximum.y - countSize.y - 3.0f},
                        IM_COL32(255, 255, 255, 255), countText.c_str());
                }
                if (selected && !empty) {
                    const char* name = itemName(inventorySlot.itemId);
                    const ImVec2 textSize = ImGui::CalcTextSize(name);
                    drawList->AddText({minimum.x + (slotSize - textSize.x) * 0.5f,
                        minimum.y - textSize.y - 4.0f}, IM_COL32(255, 238, 120, 255), name);
                }
            }
        }
        if (!inventoryOpen_ && miningBlock_) {
            const float barWidth = 220.0f;
            const float barHeight = 12.0f;
            const ImVec2 minimum{imageOrigin.x + (imageSize.x - barWidth) * 0.5f,
                hotbarY - 35.0f};
            const ImVec2 maximum{minimum.x + barWidth, minimum.y + barHeight};
            drawList->AddRectFilled(minimum, maximum, IM_COL32(18, 20, 23, 220), 4.0f);
            drawList->AddRectFilled(minimum, {minimum.x + barWidth * miningProgress_, maximum.y},
                IM_COL32(255, 218, 90, 245), 4.0f);
            drawList->AddRect(minimum, maximum, IM_COL32(255, 255, 255, 160), 4.0f);
            const char* name = blockName(*miningBlock_);
            const ImVec2 textSize = ImGui::CalcTextSize(name);
            drawList->AddText({minimum.x + (barWidth - textSize.x) * 0.5f,
                minimum.y - textSize.y - 4.0f}, IM_COL32(255, 238, 120, 255), name);
        }
        if (!inventoryOpen_ && actionMessageSeconds_ > 0.0f && !actionMessage_.empty()) {
            const ImVec2 textSize = ImGui::CalcTextSize(actionMessage_.c_str());
            const ImVec2 padding{10.0f, 6.0f};
            const ImVec2 minimum{imageOrigin.x + (imageSize.x - textSize.x) * 0.5f - padding.x,
                hotbarY - 82.0f};
            const ImVec2 maximum{minimum.x + textSize.x + padding.x * 2.0f,
                minimum.y + textSize.y + padding.y * 2.0f};
            drawList->AddRectFilled(minimum, maximum, IM_COL32(12, 14, 18, 230), 5.0f);
            drawList->AddRect(minimum, maximum, IM_COL32(255, 210, 90, 180), 5.0f);
            drawList->AddText({minimum.x + padding.x, minimum.y + padding.y},
                IM_COL32(255, 230, 130, 255), actionMessage_.c_str());
        }
        if (inventoryOpen_) drawInventoryOverlay(drawList, imageOrigin, imageSize);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

std::optional<std::pair<int, int>> EditorLayer::consumePendingPick() {
    auto result = pendingPick_;
    pendingPick_.reset();
    return result;
}

std::optional<int> EditorLayer::consumePendingInventorySlot() {
    auto result = pendingInventorySlot_;
    pendingInventorySlot_.reset();
    return result;
}

std::optional<int> EditorLayer::consumePendingCraftingSlot() {
    auto result = pendingCraftingSlot_;
    pendingCraftingSlot_.reset();
    return result;
}

std::optional<int> EditorLayer::consumePendingCraftingTableSlot() {
    auto result = pendingCraftingTableSlot_;
    pendingCraftingTableSlot_.reset();
    return result;
}

bool EditorLayer::consumePendingCraftingOutput() {
    const bool result = pendingCraftingOutput_;
    pendingCraftingOutput_ = false;
    return result;
}

void EditorLayer::selectEntity(std::uint32_t entityId) {
    if (entityId == 0) selectedEntity_.reset();
    else selectedEntity_ = entityId;
}

void EditorLayer::setMiningProgress(BlockType block, float progress) {
    miningBlock_ = block;
    miningProgress_ = std::clamp(progress, 0.0f, 1.0f);
}

void EditorLayer::clearMiningProgress() {
    miningBlock_.reset();
    miningProgress_ = 0.0f;
}

void EditorLayer::showActionMessage(std::string message) {
    actionMessage_ = std::move(message);
    actionMessageSeconds_ = 1.35f;
}

void EditorLayer::drawInventoryOverlay(ImDrawList* drawList, const ImVec2& imageOrigin,
    const ImVec2& imageSize) {
    constexpr float slotSize = 58.0f;
    constexpr float gap = 8.0f;
    constexpr float padding = 18.0f;
    constexpr int columns = InventoryHotbarSlots;
    constexpr float panelWidth = padding * 2.0f + slotSize * columns + gap * (columns - 1);
    constexpr float panelHeight = 520.0f;
    const ImVec2 panelMin{imageOrigin.x + (imageSize.x - panelWidth) * 0.5f,
        imageOrigin.y + (imageSize.y - panelHeight) * 0.5f};
    const ImVec2 panelMax{panelMin.x + panelWidth, panelMin.y + panelHeight};

    drawList->AddRectFilled(imageOrigin, {imageOrigin.x + imageSize.x, imageOrigin.y + imageSize.y},
        IM_COL32(0, 0, 0, 95));
    drawList->AddRectFilled(panelMin, panelMax, IM_COL32(16, 18, 22, 245), 8.0f);
    drawList->AddRect(panelMin, panelMax, IM_COL32(255, 238, 120, 190), 8.0f, 0, 2.0f);
    drawList->AddText({panelMin.x + padding, panelMin.y + 11.0f},
        IM_COL32(255, 238, 120, 255), "Inventario  (E para cerrar)");

    const auto clickInside = [](const ImVec2& minimum, const ImVec2& maximum) {
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return false;
        const ImVec2 mouse = ImGui::GetMousePos();
        return mouse.x >= minimum.x && mouse.x <= maximum.x
            && mouse.y >= minimum.y && mouse.y <= maximum.y;
    };

    const auto drawItemBox = [&](const InventorySlot& inventorySlot, const ImVec2& minimum,
        const ImVec2& maximum, bool selected, const char* label) {
        const bool empty = inventorySlot.empty();
        const BlockColor color = itemColor(inventorySlot.itemId);
        drawList->AddRectFilled(minimum, maximum, IM_COL32(28, 31, 38, 255), 5.0f);
        if (!empty) {
            drawList->AddRectFilled({minimum.x + 9.0f, minimum.y + 9.0f},
                {maximum.x - 9.0f, maximum.y - 9.0f},
                IM_COL32(color.r, color.g, color.b, color.a), 4.0f);
        }
        drawList->AddRect(minimum, maximum,
            selected ? IM_COL32(255, 238, 120, 255) : IM_COL32(120, 125, 138, 210),
            5.0f, 0, selected ? 3.0f : 1.0f);
        if (!empty) {
            const std::string count = std::to_string(inventorySlot.count);
            const ImVec2 countSize = ImGui::CalcTextSize(count.c_str());
            drawList->AddText({maximum.x - countSize.x - 5.0f, maximum.y - countSize.y - 4.0f},
                IM_COL32(255, 255, 255, 255), count.c_str());
        }
        if (label && label[0] != '\0')
            drawList->AddText({minimum.x + 5.0f, minimum.y + 3.0f},
                IM_COL32(255, 255, 255, 235), label);
    };

    drawList->AddText({panelMin.x + padding, panelMin.y + 44.0f},
        IM_COL32(210, 214, 225, 255), craftingTableOpen_ ? "Crafting Table 3x3" : "Crafting 2x2");
    const float craftX = panelMin.x + padding;
    const float craftY = panelMin.y + 68.0f;
    constexpr float craftSlotSize = 44.0f;
    constexpr float craftGap = 6.0f;
    if (craftingTableOpen_) {
        for (int slot = 0; slot < CraftingTableGridSlots; ++slot) {
            const int x = slot % 3;
            const int y = slot / 3;
            const ImVec2 minimum{craftX + x * (craftSlotSize + craftGap),
                craftY + y * (craftSlotSize + craftGap)};
            const ImVec2 maximum{minimum.x + craftSlotSize, minimum.y + craftSlotSize};
            drawItemBox(craftingTableGrid_.slots[slot], minimum, maximum, false, "");
            if (clickInside(minimum, maximum)) pendingCraftingTableSlot_ = slot;
        }
    } else {
        for (int slot = 0; slot < CraftingGridSlots; ++slot) {
            const int x = slot % 2;
            const int y = slot / 2;
            const ImVec2 minimum{craftX + x * (craftSlotSize + craftGap),
                craftY + y * (craftSlotSize + craftGap)};
            const ImVec2 maximum{minimum.x + craftSlotSize, minimum.y + craftSlotSize};
            drawItemBox(craftingGrid_.slots[slot], minimum, maximum, false, "");
            if (clickInside(minimum, maximum)) pendingCraftingSlot_ = slot;
        }
    }
    drawList->AddText({craftX + 165.0f, craftY + 48.0f}, IM_COL32(255, 238, 120, 255), "=>");
    const CraftingResult result = craftingTableOpen_
        ? craftingTableResult(craftingTableGrid_) : craftingResult(craftingGrid_);
    const ImVec2 outputMin{craftX + 205.0f, craftY + 42.0f};
    const ImVec2 outputMax{outputMin.x + craftSlotSize, outputMin.y + craftSlotSize};
    drawItemBox(result.valid ? result.output : InventorySlot{}, outputMin, outputMax, result.valid, "");
    if (result.valid && clickInside(outputMin, outputMax)) pendingCraftingOutput_ = true;

    drawList->AddText({panelMin.x + padding, panelMin.y + 164.0f},
        IM_COL32(210, 214, 225, 255), "Mochila");

    const auto drawSlot = [&](int slotIndex, int column, float row, bool hotbar) {
        const InventorySlot& inventorySlot = inventory_.slots[slotIndex];
        const ImVec2 minimum{panelMin.x + padding + column * (slotSize + gap),
            panelMin.y + 188.0f + row * (slotSize + gap)};
        const ImVec2 maximum{minimum.x + slotSize, minimum.y + slotSize};
        const bool selected = hotbar && selectedHotbarSlot_ == column;
        const std::string label = hotbar ? std::to_string(column + 1) : "";
        drawItemBox(inventorySlot, minimum, maximum, selected, label.c_str());
        if (clickInside(minimum, maximum)) pendingInventorySlot_ = slotIndex;
    };

    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < InventoryHotbarSlots; ++column) {
            drawSlot(InventoryHotbarSlots + row * InventoryHotbarSlots + column,
                column, static_cast<float>(row), false);
        }
    }
    for (int column = 0; column < InventoryHotbarSlots; ++column) {
        drawSlot(column, column, 3.55f, true);
    }

    const InventorySlot& selectedSlot = inventory_.slots[selectedHotbarSlot_];
    if (!selectedSlot.empty()) {
        const char* name = itemName(selectedSlot.itemId);
        const ImVec2 nameSize = ImGui::CalcTextSize(name);
        drawList->AddText({panelMin.x + (panelWidth - nameSize.x) * 0.5f, panelMax.y - 62.0f},
            IM_COL32(255, 238, 120, 255), name);
    }

    drawList->AddText({panelMin.x + padding, panelMax.y - 30.0f},
        IM_COL32(170, 178, 195, 255),
        "Click mochila/crafting: intercambia con hotbar activa | Output: craftea | 1-9 selecciona");
}

void EditorLayer::setRendererStats(std::uint64_t triangles, std::uint32_t drawCalls,
    std::uint32_t visibleChunks, std::uint32_t loadedChunks, std::uint32_t pendingJobs,
    std::uint32_t uploadedMeshes, std::uint32_t activeWorkers,
    std::uint32_t activeUploadBudget, std::uint32_t generatedChunks,
    std::uint32_t pendingChunkJobs, float cpuRenderMs, float meshProcessingMs, float gpuRenderMs) {
    renderedTriangles_ = triangles;
    drawCalls_ = drawCalls;
    visibleChunks_ = visibleChunks;
    loadedChunks_ = loadedChunks;
    pendingJobs_ = pendingJobs;
    uploadedMeshes_ = uploadedMeshes;
    activeWorkers_ = activeWorkers;
    activeUploadBudget_ = activeUploadBudget;
    generatedChunks_ = generatedChunks;
    pendingChunkJobs_ = pendingChunkJobs;
    cpuRenderMilliseconds_ = cpuRenderMs;
    meshProcessingMilliseconds_ = meshProcessingMs;
    gpuRenderMilliseconds_ = gpuRenderMs;
}

void EditorLayer::buildDefaultLayout(unsigned int dockspaceId) {
    resetLayoutRequested_ = false;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

    ImGuiID center = dockspaceId;
    ImGuiID left = 0;
    ImGuiID right = 0;
    ImGuiID statistics = 0;
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, &left, &center);
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.24f, &right, &center);
    ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.25f, &statistics, &right);

    ImGui::DockBuilderDockWindow("Scene Hierarchy", left);
    ImGui::DockBuilderDockWindow("Inspector", right);
    ImGui::DockBuilderDockWindow("Statistics", statistics);
    ImGui::DockBuilderDockWindow("Scene View", center);
    ImGui::DockBuilderFinish(dockspaceId);
}

void EditorLayer::drawHierarchy(Scene& scene) {
    ImGui::Begin("Scene Hierarchy");
    if (ImGui::Button("Save Scene")) {
        SceneSerializer::save(scene, AssetPath::resolve("Assets/Scenes/Sandbox.gescene"));
    }
    ImGui::Separator();

    for (const Entity& entity : scene.entities()) {
        int depth = 0;
        auto parent = entity.parentId();
        while (parent) {
            ++depth;
            const Entity* parentEntity = scene.findEntity(*parent);
            parent = parentEntity ? parentEntity->parentId() : std::nullopt;
        }
        ImGui::Indent(static_cast<float>(depth) * 14.0f);
        const bool selected = selectedEntity_ && *selectedEntity_ == entity.id();
        const std::string label = entity.tag().name + "##" + std::to_string(entity.id());
        if (ImGui::Selectable(label.c_str(), selected)) selectedEntity_ = entity.id();
        ImGui::Unindent(static_cast<float>(depth) * 14.0f);
    }
    ImGui::End();
}

void EditorLayer::drawInspector(Scene& scene) {
    ImGui::Begin("Inspector");
    Entity* entity = selectedEntity_ ? scene.findEntity(*selectedEntity_) : nullptr;
    if (!entity) {
        ImGui::TextDisabled("Select an entity in the hierarchy.");
        ImGui::End();
        return;
    }

    ImGui::Text("%s", entity->tag().name.c_str());
    ImGui::TextDisabled("Entity %u", entity->id());
    ImGui::SeparatorText("Transform");
    auto& transform = entity->transform();
    ImGui::DragFloat3("Position", &transform.position.x, 0.1f);
    ImGui::DragFloat3("Rotation", &transform.rotation.x, 0.02f);
    ImGui::DragFloat3("Scale", &transform.scale.x, 0.05f, 0.01f, 100.0f);

    if (entity->hasMaterial()) {
        ImGui::SeparatorText("Material");
        ImGui::ColorEdit3("Base color", &entity->material().baseColor.x);
        ImGui::Text("Texture: %s", entity->material().textureAsset.c_str());
    }
    if (entity->hasVoxelGrid()) {
        ImGui::SeparatorText("Voxel Grid");
        auto& grid = entity->voxelGrid();
        ImGui::DragInt("Width", &grid.width, 1.0f, 1, 64);
        ImGui::DragInt("Height", &grid.height, 1.0f, 1, 64);
        ImGui::DragInt("Depth", &grid.depth, 1.0f, 1, 64);
        ImGui::DragFloat("Spacing", &grid.spacing, 0.05f, 0.1f, 10.0f);
    }
    if (entity->hasVoxelTerrain()) {
        ImGui::SeparatorText("Voxel Terrain");
        ImGui::InputInt("Seed", &entity->voxelTerrain().seed);
        ImGui::SliderInt("View radius", &entity->voxelTerrain().viewRadius, 1, 4);
        const unsigned int hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
        const int safeMaximum = std::clamp(static_cast<int>(hardwareThreads) - 1, 1, 16);
        auto& terrain = entity->voxelTerrain();
        const char* profiles[]{"Safe", "Balanced", "Turbo", "Extreme"};
        int profileIndex = static_cast<int>(terrain.performanceProfile);
        if (ImGui::Combo("Performance profile", &profileIndex, profiles, 4)) {
            terrain.performanceProfile = static_cast<VoxelTerrainComponent::PerformanceProfile>(
                std::clamp(profileIndex, 0, 3));
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            const int reserveTwo = std::max(1, static_cast<int>(hardwareThreads) - 2);
            const int reserveOne = std::max(1, static_cast<int>(hardwareThreads) - 1);
            switch (terrain.performanceProfile) {
            case VoxelTerrainComponent::PerformanceProfile::Safe:
                terrain.meshWorkers = 1;
                terrain.meshUploadsPerFrame = 1;
                terrain.meshQueueLimit = 8;
                terrain.chunkLoadsPerFrame = 1;
                terrain.targetFrameMilliseconds = 16.67f;
                terrain.adaptiveScheduling = true;
                break;
            case VoxelTerrainComponent::PerformanceProfile::Balanced:
                terrain.meshWorkers = 0;
                terrain.meshUploadsPerFrame = 2;
                terrain.meshQueueLimit = 16;
                terrain.chunkLoadsPerFrame = 2;
                terrain.targetFrameMilliseconds = 8.33f;
                terrain.adaptiveScheduling = true;
                break;
            case VoxelTerrainComponent::PerformanceProfile::Turbo:
                terrain.meshWorkers = std::clamp(reserveTwo, 1, safeMaximum);
                terrain.meshUploadsPerFrame = 4;
                terrain.meshQueueLimit = 32;
                terrain.chunkLoadsPerFrame = 4;
                terrain.targetFrameMilliseconds = 8.33f;
                terrain.adaptiveScheduling = true;
                break;
            case VoxelTerrainComponent::PerformanceProfile::Extreme:
                terrain.meshWorkers = std::clamp(reserveOne, 1, safeMaximum);
                terrain.meshUploadsPerFrame = 6;
                terrain.meshQueueLimit = 48;
                terrain.chunkLoadsPerFrame = 6;
                terrain.targetFrameMilliseconds = 6.94f;
                terrain.adaptiveScheduling = true;
                break;
            }
        }
        ImGui::SliderInt("Mesh workers (0 = Auto)", &entity->voxelTerrain().meshWorkers,
            0, safeMaximum);
        ImGui::SliderInt("GPU uploads/frame", &entity->voxelTerrain().meshUploadsPerFrame, 1, 8);
        ImGui::SliderInt("Mesh queue limit", &entity->voxelTerrain().meshQueueLimit, 4, 64);
        ImGui::Checkbox("Adaptive scheduler", &entity->voxelTerrain().adaptiveScheduling);
        ImGui::SliderFloat("Target frame (ms)",
            &entity->voxelTerrain().targetFrameMilliseconds, 4.0f, 33.33f, "%.2f ms");
        ImGui::SliderInt("Chunk loads/frame", &entity->voxelTerrain().chunkLoadsPerFrame, 1, 8);
        const float targetFps = 1000.0f / std::clamp(
            entity->voxelTerrain().targetFrameMilliseconds, 4.0f, 33.33f);
        ImGui::TextDisabled("Target: %.0f FPS", targetFps);
        const int automaticWorkers = std::clamp(static_cast<int>(hardwareThreads) - 2, 1, 12);
        ImGui::TextDisabled("CPU: %u threads | Auto uses %d and reserves %u",
            hardwareThreads, automaticWorkers, hardwareThreads - automaticWorkers);
        ImGui::TextDisabled("Regeneration on reload");
    }
    if (entity->hasMeshRenderer()) {
        ImGui::SeparatorText("Mesh Renderer");
        ImGui::Text("Mesh: %s", entity->meshRenderer().meshAsset.c_str());
    }
    if (entity->hasDirectionalLight()) {
        ImGui::SeparatorText("Directional Light");
        auto& light = entity->directionalLight();
        ImGui::DragFloat3("Direction", &light.direction.x, 0.02f);
        ImGui::ColorEdit3("Light color", &light.color.x);
        ImGui::DragFloat("Intensity", &light.intensity, 0.02f, 0.0f, 10.0f);
    }
    ImGui::End();
}

void EditorLayer::drawStatistics(const Scene& scene, const Time& time) {
    ImGui::Begin("Statistics");
    const float milliseconds = time.deltaTime() * 1000.0f;
    const float fps = time.deltaTime() > 0.0f ? 1.0f / time.deltaTime() : 0.0f;
    ImGui::Text("Frame: %.2f ms", milliseconds);
    ImGui::Text("FPS: %.0f", fps);
    ImGui::Text("CPU render: %.2f ms", cpuRenderMilliseconds_);
    ImGui::Text("GPU render: %.2f ms", gpuRenderMilliseconds_);
    ImGui::Text("Entities: %zu", scene.entities().size());
    ImGui::Separator();
    ImGui::Text("Triangles: %llu", static_cast<unsigned long long>(renderedTriangles_));
    ImGui::Text("Draw calls: %u", drawCalls_);
    ImGui::Text("Visible chunks: %u", visibleChunks_);
    ImGui::Text("Loaded chunks: %u", loadedChunks_);
    ImGui::Text("Generated this frame: %u", generatedChunks_);
    ImGui::Text("Chunk jobs: %u", pendingChunkJobs_);
    ImGui::Text("Mesh jobs: %u", pendingJobs_);
    ImGui::Text("Mesh processing: %.2f ms", meshProcessingMilliseconds_);
    ImGui::Text("Mesh uploads: %u", uploadedMeshes_);
    ImGui::Text("Active workers: %u", activeWorkers_);
    ImGui::Text("Upload budget: %u/frame", activeUploadBudget_);
    ImGui::Text("Mode: %s", wireframe_ ? "Wireframe" : "Solid");
    ImGui::Text("Frustum culling: %s", frustumCulling_ ? "On" : "Off");
    ImGui::Text("Chunk bounds: %s", chunkBounds_ ? "On" : "Off");
    ImGui::Text("Occlusion culling: %s", occlusionCulling_ ? "On" : "Off");
    ImGui::End();
}

} // namespace Engine
