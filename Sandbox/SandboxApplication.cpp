#include "Sandbox/SandboxApplication.h"
#include "Engine/Scene/SceneSerializer.h"

void SandboxApplication::onStart() {
    Engine::SceneSerializer::load(scene(), "Assets/Scenes/Sandbox.gescene");
}

void SandboxApplication::onUpdate(float) {
    // Aquí vivirá la lógica propia del juego o editor, separada del motor.
}
