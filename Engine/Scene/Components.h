#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace Engine {

struct TagComponent {
    std::string name = "Entity";
};

struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 matrix() const {
        glm::mat4 result = glm::translate(glm::mat4(1.0f), position);
        result = glm::rotate(result, rotation.x, {1.0f, 0.0f, 0.0f});
        result = glm::rotate(result, rotation.y, {0.0f, 1.0f, 0.0f});
        result = glm::rotate(result, rotation.z, {0.0f, 0.0f, 1.0f});
        return glm::scale(result, scale);
    }
};

struct VoxelGridComponent {
    int width = 5;
    int height = 5;
    int depth = 5;
    float spacing = 2.0f;
};

struct VoxelTerrainComponent {
    int seed = 1337;
    int viewRadius = 1;
    int meshWorkers = 0;
    int meshUploadsPerFrame = 2;
    int meshQueueLimit = 16;
    bool adaptiveScheduling = true;
    float targetFrameMilliseconds = 8.33f;
    int chunkLoadsPerFrame = 2;
};

struct MaterialComponent {
    glm::vec3 baseColor{0.20f, 0.65f, 0.95f};
    std::string textureAsset = "Grid";
};

struct MeshRendererComponent {
    std::string meshAsset = "Cube";
};

struct DirectionalLightComponent {
    glm::vec3 direction{-0.5f, -1.0f, -0.35f};
    glm::vec3 color{1.0f, 0.95f, 0.85f};
    float intensity = 1.0f;
};

} // namespace Engine
