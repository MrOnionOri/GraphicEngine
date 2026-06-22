#pragma once

namespace Engine {

class Time {
public:
    Time();
    float tick();
    float deltaTime() const { return deltaTime_; }
    double elapsedTime() const { return elapsedTime_; }

private:
    double previousTime_ = 0.0;
    double elapsedTime_ = 0.0;
    float deltaTime_ = 0.0f;
};

} // namespace Engine
