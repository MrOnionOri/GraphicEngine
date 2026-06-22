#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Scene/Scene.h"

#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <vector>

namespace Engine {

Renderer::Renderer()
{
    assets_.loadShader("VoxelLit", "Assets/Shaders/Voxel.vert", "Assets/Shaders/Voxel.frag");
    assets_.loadTexture("Grid", "Assets/Textures/Grid.ppm");
    assets_.createCube("Cube");
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.055f, 0.075f, 0.10f, 1.0f);
}

void Renderer::render(const Scene& scene, const EditorCamera& camera, float aspectRatio) {
    const GLfloat clearColor[]{0.055f, 0.075f, 0.10f, 1.0f};
    const GLuint clearEntity[]{0};
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferuiv(GL_COLOR, 1, clearEntity);
    glClear(GL_DEPTH_BUFFER_BIT);
    Shader& shader = assets_.shader("VoxelLit");
    shader.bind();
    shader.setMat4("uViewProjection", camera.projectionMatrix(aspectRatio) * camera.viewMatrix());

    glm::vec3 lightDirection{-0.5f, -1.0f, -0.35f};
    glm::vec3 lightColor{1.0f};
    float lightIntensity = 1.0f;
    for (const Entity& entity : scene.entities()) {
        if (!entity.hasDirectionalLight()) continue;
        const DirectionalLightComponent& light = entity.directionalLight();
        lightDirection = light.direction;
        lightColor = light.color;
        lightIntensity = light.intensity;
        break;
    }
    shader.setVec3("uLightDirection", lightDirection);
    shader.setVec3("uLightColor", lightColor);
    shader.setFloat("uLightIntensity", lightIntensity);
    shader.setInt("uTexture", 0);

    for (const Entity& entity : scene.entities()) {
        if (!entity.hasVoxelGrid()) continue;
        shader.setUInt("uEntityId", entity.id());
        const MaterialComponent defaultMaterial;
        const MaterialComponent& material = entity.hasMaterial() ? entity.material() : defaultMaterial;
        shader.setVec3("uBaseColor", material.baseColor);
        assets_.texture(material.textureAsset).bind(0);
        const VoxelGridComponent& grid = entity.voxelGrid();
        std::vector<glm::mat4> instances;
        instances.reserve(static_cast<std::size_t>(grid.width * grid.height * grid.depth));
        const glm::mat4 root = scene.worldTransform(entity);
        const glm::vec3 center{
            (grid.width - 1) * grid.spacing * 0.5f,
            (grid.height - 1) * grid.spacing * 0.5f,
            0.0f
        };
        for (int x = 0; x < grid.width; ++x) {
            for (int y = 0; y < grid.height; ++y) {
                for (int z = 0; z < grid.depth; ++z) {
                    const glm::vec3 local{x * grid.spacing, y * grid.spacing, z * grid.spacing};
                    instances.push_back(root * glm::translate(glm::mat4(1.0f), local - center));
                }
            }
        }
        Mesh& mesh = assets_.mesh("Cube");
        mesh.uploadInstances(instances);
        mesh.drawInstanced();
    }

    for (const Entity& entity : scene.entities()) {
        if (!entity.hasMeshRenderer()) continue;
        shader.setUInt("uEntityId", entity.id());
        const MaterialComponent defaultMaterial;
        const MaterialComponent& material = entity.hasMaterial() ? entity.material() : defaultMaterial;
        shader.setVec3("uBaseColor", material.baseColor);
        assets_.texture(material.textureAsset).bind(0);
        Mesh& mesh = assets_.mesh(entity.meshRenderer().meshAsset);
        const std::vector<glm::mat4> instances{scene.worldTransform(entity)};
        mesh.uploadInstances(instances);
        mesh.drawInstanced();
    }

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) throw std::runtime_error("OpenGL error: " + std::to_string(error));
}

} // namespace Engine
