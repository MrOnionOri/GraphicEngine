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
    drawViewport(viewportTexture);
    drawHierarchy(scene);
    drawInspector(scene);
    drawStatistics(scene, time);
}

void EditorLayer::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool EditorLayer::wantsMouse() const { return ImGui::GetIO().WantCaptureMouse; }
bool EditorLayer::wantsKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

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
        const ImVec2 center{imageOrigin.x + imageSize.x * 0.5f, imageOrigin.y + imageSize.y * 0.5f};
        const ImU32 color = IM_COL32(255, 255, 255, 210);
        drawList->AddLine({center.x - 7.0f, center.y}, {center.x + 7.0f, center.y}, color, 1.5f);
        drawList->AddLine({center.x, center.y - 7.0f}, {center.x, center.y + 7.0f}, color, 1.5f);

        constexpr float slotSize = 46.0f;
        constexpr float gap = 5.0f;
        const float hotbarWidth = slotSize * 5.0f + gap * 4.0f;
        const float hotbarX = imageOrigin.x + (imageSize.x - hotbarWidth) * 0.5f;
        const float hotbarY = imageOrigin.y + imageSize.y - slotSize - 14.0f;
        const BlockType blocks[5]{BlockType::Grass, BlockType::Dirt, BlockType::Stone,
            BlockType::Wood, BlockType::Leaves};
        const ImU32 blockColors[5]{
            IM_COL32(74, 185, 55, 255), IM_COL32(126, 78, 42, 255),
            IM_COL32(125, 129, 132, 255), IM_COL32(118, 73, 36, 255),
            IM_COL32(52, 142, 42, 255)};
        for (int slot = 0; slot < 5; ++slot) {
            const ImVec2 minimum{hotbarX + slot * (slotSize + gap), hotbarY};
            const ImVec2 maximum{minimum.x + slotSize, minimum.y + slotSize};
            drawList->AddRectFilled(minimum, maximum, IM_COL32(18, 20, 23, 220), 3.0f);
            drawList->AddRectFilled({minimum.x + 7.0f, minimum.y + 7.0f},
                {maximum.x - 7.0f, maximum.y - 7.0f}, blockColors[slot], 2.0f);
            const bool selected = selectedBlock_ == blocks[slot];
            drawList->AddRect(minimum, maximum,
                selected ? IM_COL32(255, 238, 120, 255) : IM_COL32(145, 150, 160, 210),
                3.0f, 0, selected ? 3.0f : 1.0f);
            drawList->AddText({minimum.x + 4.0f, minimum.y + 2.0f},
                IM_COL32(255, 255, 255, 255), std::to_string(slot + 1).c_str());
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

std::optional<std::pair<int, int>> EditorLayer::consumePendingPick() {
    auto result = pendingPick_;
    pendingPick_.reset();
    return result;
}

void EditorLayer::selectEntity(std::uint32_t entityId) {
    if (entityId == 0) selectedEntity_.reset();
    else selectedEntity_ = entityId;
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
