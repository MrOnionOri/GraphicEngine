#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Core/AssetPath.h"
#include "Engine/Scene/Scene.h"

#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Engine {

namespace {
void expect(std::istream& stream, const char* expected) {
    std::string token;
    stream >> token;
    if (token != expected) throw std::runtime_error("Escena invalida: se esperaba " + std::string(expected));
}
}

void SceneSerializer::load(Scene& scene, const std::filesystem::path& path) {
    std::ifstream stream(AssetPath::resolve(path));
    if (!stream) throw std::runtime_error("No se pudo abrir la escena");
    expect(stream, "GESCENE");
    int version = 0;
    stream >> version;
    if (version < 1 || version > 5) throw std::runtime_error("Version de escena no compatible");

    scene.clear();
    std::vector<std::pair<std::uint32_t, std::uint32_t>> parents;
    std::string token;
    while (stream >> token) {
        if (token != "ENTITY") throw std::runtime_error("Escena invalida: se esperaba ENTITY");
        std::uint32_t id = 0;
        std::uint32_t parentId = 0;
        std::string name;
        stream >> id >> std::quoted(name) >> parentId;
        Entity& entity = scene.createEntityWithId(id, name);

        expect(stream, "TRANSFORM");
        auto& transform = entity.transform();
        stream >> transform.position.x >> transform.position.y >> transform.position.z
               >> transform.rotation.x >> transform.rotation.y >> transform.rotation.z
               >> transform.scale.x >> transform.scale.y >> transform.scale.z;

        stream >> token;
        if (token == "VOXEL") {
            VoxelGridComponent component;
            stream >> component.width >> component.height >> component.depth >> component.spacing;
            entity.addVoxelGrid(component);
        } else if (token != "NO_VOXEL") throw std::runtime_error("Componente voxel invalido");

        stream >> token;
        if (token == "TERRAIN") {
            VoxelTerrainComponent component;
            stream >> component.seed >> component.viewRadius;
            if (version >= 2) stream >> component.meshWorkers >> component.meshUploadsPerFrame
                >> component.meshQueueLimit;
            if (version >= 3) stream >> component.adaptiveScheduling
                >> component.targetFrameMilliseconds;
            if (version >= 4) stream >> component.chunkLoadsPerFrame;
            if (version >= 5) {
                int profile = 1;
                stream >> profile;
                component.performanceProfile = static_cast<VoxelTerrainComponent::PerformanceProfile>(
                    std::clamp(profile, 0, 3));
            }
            entity.addVoxelTerrain(component);
        } else if (token != "NO_TERRAIN") throw std::runtime_error("Terreno voxel invalido");

        stream >> token;
        if (token == "MESH") {
            MeshRendererComponent component;
            stream >> std::quoted(component.meshAsset);
            entity.addMeshRenderer(component);
        } else if (token != "NO_MESH") throw std::runtime_error("Componente mesh invalido");

        stream >> token;
        if (token == "MATERIAL") {
            MaterialComponent component;
            stream >> component.baseColor.r >> component.baseColor.g >> component.baseColor.b
                   >> std::quoted(component.textureAsset);
            entity.addMaterial(component);
        } else if (token != "NO_MATERIAL") throw std::runtime_error("Material invalido");

        stream >> token;
        if (token == "LIGHT") {
            DirectionalLightComponent component;
            stream >> component.direction.x >> component.direction.y >> component.direction.z
                   >> component.color.r >> component.color.g >> component.color.b >> component.intensity;
            entity.addDirectionalLight(component);
        } else if (token != "NO_LIGHT") throw std::runtime_error("Luz invalida");

        expect(stream, "END");
        if (parentId != 0) parents.emplace_back(id, parentId);
    }
    for (const auto& relationship : parents) scene.setParent(relationship.first, relationship.second);
}

void SceneSerializer::save(const Scene& scene, const std::filesystem::path& path) {
    std::ofstream stream(path);
    if (!stream) throw std::runtime_error("No se pudo guardar la escena: " + path.string());
    stream << "GESCENE 5\n";
    for (const Entity& entity : scene.entities()) {
        stream << "ENTITY " << entity.id() << ' ' << std::quoted(entity.tag().name) << ' '
               << entity.parentId().value_or(0) << '\n';
        const auto& transform = entity.transform();
        stream << "TRANSFORM " << transform.position.x << ' ' << transform.position.y << ' '
               << transform.position.z << ' ' << transform.rotation.x << ' ' << transform.rotation.y << ' '
               << transform.rotation.z << ' ' << transform.scale.x << ' ' << transform.scale.y << ' '
               << transform.scale.z << '\n';
        if (entity.hasVoxelGrid()) {
            const auto& voxel = entity.voxelGrid();
            stream << "VOXEL " << voxel.width << ' ' << voxel.height << ' ' << voxel.depth << ' '
                   << voxel.spacing << '\n';
        } else stream << "NO_VOXEL\n";
        if (entity.hasVoxelTerrain()) stream << "TERRAIN " << entity.voxelTerrain().seed << ' '
            << entity.voxelTerrain().viewRadius << ' ' << entity.voxelTerrain().meshWorkers << ' '
            << entity.voxelTerrain().meshUploadsPerFrame << ' '
            << entity.voxelTerrain().meshQueueLimit << ' '
            << entity.voxelTerrain().adaptiveScheduling << ' '
            << entity.voxelTerrain().targetFrameMilliseconds << ' '
            << entity.voxelTerrain().chunkLoadsPerFrame << ' '
            << static_cast<int>(entity.voxelTerrain().performanceProfile) << '\n';
        else stream << "NO_TERRAIN\n";
        if (entity.hasMeshRenderer()) stream << "MESH " << std::quoted(entity.meshRenderer().meshAsset) << '\n';
        else stream << "NO_MESH\n";
        if (entity.hasMaterial()) {
            const auto& material = entity.material();
            stream << "MATERIAL " << material.baseColor.r << ' ' << material.baseColor.g << ' '
                   << material.baseColor.b << ' ' << std::quoted(material.textureAsset) << '\n';
        } else stream << "NO_MATERIAL\n";
        if (entity.hasDirectionalLight()) {
            const auto& light = entity.directionalLight();
            stream << "LIGHT " << light.direction.x << ' ' << light.direction.y << ' ' << light.direction.z << ' '
                   << light.color.r << ' ' << light.color.g << ' ' << light.color.b << ' ' << light.intensity << '\n';
        } else stream << "NO_LIGHT\n";
        stream << "END\n";
    }
}

} // namespace Engine
