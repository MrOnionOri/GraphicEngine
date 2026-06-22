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
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 imageSize{std::max(1.0f, available.x), std::max(1.0f, available.y)};
    const ImVec2 imageOrigin = ImGui::GetCursorScreenPos();
    viewportWidth_ = static_cast<int>(imageSize.x);
    viewportHeight_ = static_cast<int>(imageSize.y);
    viewportHovered_ = ImGui::IsWindowHovered();
    viewportFocused_ = ImGui::IsWindowFocused();
    ImGui::Image(ImTextureRef(static_cast<ImTextureID>(texture)), imageSize,
        ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetMousePos();
        const int x = static_cast<int>(mouse.x - imageOrigin.x);
        const int y = static_cast<int>(imageSize.y - (mouse.y - imageOrigin.y));
        pendingPick_ = std::pair<int, int>{x, y};
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
    ImGui::Text("Entities: %zu", scene.entities().size());
    ImGui::End();
}

} // namespace Engine
