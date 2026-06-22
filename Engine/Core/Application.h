#pragma once

#include "Engine/Core/Time.h"
#include "Engine/Core/Window.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Scene/Scene.h"

namespace Engine {

class Application {
public:
    Application();
    virtual ~Application() = default;
    void run();

protected:
    virtual void onStart() = 0;
    virtual void onUpdate(float deltaTime) {}

    Scene& scene() { return scene_; }
    Window& window() { return window_; }

private:
    Window window_;
    Renderer renderer_;
    Scene scene_;
    EditorCamera camera_;
    Time time_;
};

} // namespace Engine
