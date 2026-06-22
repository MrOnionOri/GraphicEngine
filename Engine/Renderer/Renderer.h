#pragma once

#include "Engine/Renderer/AssetManager.h"

namespace Engine {

class EditorCamera;
class Scene;

class Renderer {
public:
    Renderer();
    void render(const Scene& scene, const EditorCamera& camera, float aspectRatio);

private:
    AssetManager assets_;
};

} // namespace Engine
