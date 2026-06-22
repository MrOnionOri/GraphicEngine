#include "Engine/Renderer/Frustum.h"

namespace Engine {

Frustum::Frustum(const glm::mat4& matrix) {
    const glm::vec4 row0{matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0]};
    const glm::vec4 row1{matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1]};
    const glm::vec4 row2{matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2]};
    const glm::vec4 row3{matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]};
    planes_ = {row3 + row0, row3 - row0, row3 + row1,
        row3 - row1, row3 + row2, row3 - row2};
    for (glm::vec4& plane : planes_) {
        const float length = glm::length(glm::vec3(plane));
        if (length > 0.0f) plane /= length;
    }
}

bool Frustum::intersectsTransformedBox(const glm::mat4& transform,
    const glm::vec3& minimum, const glm::vec3& maximum) const {
    std::array<glm::vec3, 8> corners;
    int index = 0;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                const glm::vec3 local{x ? maximum.x : minimum.x,
                    y ? maximum.y : minimum.y, z ? maximum.z : minimum.z};
                corners[index++] = glm::vec3(transform * glm::vec4(local, 1.0f));
            }
        }
    }

    for (const glm::vec4& plane : planes_) {
        bool allOutside = true;
        for (const glm::vec3& corner : corners) {
            if (glm::dot(glm::vec3(plane), corner) + plane.w >= 0.0f) {
                allOutside = false;
                break;
            }
        }
        if (allOutside) return false;
    }
    return true;
}

} // namespace Engine
