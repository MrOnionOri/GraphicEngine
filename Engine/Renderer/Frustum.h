#pragma once

#include <glm/glm.hpp>
#include <array>

namespace Engine {

class Frustum {
public:
    explicit Frustum(const glm::mat4& viewProjection);
    bool intersectsTransformedBox(const glm::mat4& transform,
        const glm::vec3& localMinimum, const glm::vec3& localMaximum) const;

private:
    std::array<glm::vec4, 6> planes_;
};

} // namespace Engine
