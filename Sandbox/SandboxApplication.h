#pragma once

#include "Engine/Core/Application.h"

class SandboxApplication final : public Engine::Application {
protected:
    void onStart() override;
    void onUpdate(float deltaTime) override;
};
