#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/AssetPath.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Renderer/Frustum.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Voxel/Chunk.h"
#include "Engine/Voxel/ChunkMeshBuilder.h"
#include "Engine/Voxel/VoxelWorld.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <chrono>
#include <future>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace Engine {

namespace {
int safeMeshWorkerCapacity() {
    const unsigned int hardware = std::max(1u, std::thread::hardware_concurrency());
    return std::clamp(static_cast<int>(hardware) - 1, 1, 16);
}

int resolvedMeshWorkers(int requested) {
    const unsigned int hardware = std::max(1u, std::thread::hardware_concurrency());
    const int automatic = std::clamp(static_cast<int>(hardware) - 2, 1, 12);
    return requested <= 0 ? automatic : std::clamp(requested, 1, safeMeshWorkerCapacity());
}

std::filesystem::path blockAtlasPath() {
    const std::filesystem::path localPack =
        std::filesystem::path("ResourcePacks") / "Active" / "Blocks.ppm";
    if (std::filesystem::exists(localPack)) return localPack;
    const std::filesystem::path executablePack = AssetPath::executableDirectory() / localPack;
    if (std::filesystem::exists(executablePack)) return executablePack;
    return "Assets/Textures/Blocks.ppm";
}
}

Renderer::Renderer() : meshThreadPool_(static_cast<std::size_t>(safeMeshWorkerCapacity()))
{
    assets_.loadShader("VoxelLit", "Assets/Shaders/Voxel.vert", "Assets/Shaders/Voxel.frag");
    assets_.loadShader("TerrainLit", "Assets/Shaders/Terrain.vert", "Assets/Shaders/Terrain.frag");
    assets_.loadShader("DebugLine", "Assets/Shaders/DebugLine.vert", "Assets/Shaders/DebugLine.frag");
    assets_.loadTexture("Grid", "Assets/Textures/Grid.ppm");
    assets_.loadTexture("Blocks", blockAtlasPath());
    assets_.createCube("Cube");
    assets_.createWireCube("WireCube");
    glGenQueries(3, gpuTimerQueries_);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.055f, 0.075f, 0.10f, 1.0f);
}

Renderer::~Renderer() {
    glDeleteQueries(3, gpuTimerQueries_);
    for (auto& terrain : terrainWorlds_) {
        for (auto& pair : terrain.second.occlusion) {
            glDeleteQueries(2, pair.second.queries);
        }
    }
}

void Renderer::render(const Scene& scene, const EditorCamera& camera, float aspectRatio,
    float frameMilliseconds) {
    const auto cpuRenderStart = std::chrono::steady_clock::now();
    stats_ = {};
    const unsigned int gpuQueryIndex = static_cast<unsigned int>(frameIndex_ % 3u);
    for (unsigned int index = 0; index < 3; ++index) {
        if (!gpuTimerIssued_[index]) continue;
        GLint available = GL_FALSE;
        glGetQueryObjectiv(gpuTimerQueries_[index], GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available) continue;
        GLuint64 nanoseconds = 0;
        glGetQueryObjectui64v(gpuTimerQueries_[index], GL_QUERY_RESULT, &nanoseconds);
        lastGpuRenderMilliseconds_ = static_cast<float>(nanoseconds) / 1000000.0f;
        gpuTimerIssued_[index] = false;
    }
    const bool measureGpuFrame = !gpuTimerIssued_[gpuQueryIndex];
    if (measureGpuFrame) glBeginQuery(GL_TIME_ELAPSED, gpuTimerQueries_[gpuQueryIndex]);
    stats_.gpuRenderMilliseconds = lastGpuRenderMilliseconds_;
    std::size_t configuredUploads = 1;
    desiredWorkerBudget_ = 1;
    bool adaptiveScheduling = false;
    float targetFrameMilliseconds = 8.33f;
    for (const Entity& entity : scene.entities()) {
        if (!entity.hasVoxelTerrain()) continue;
        configuredUploads = std::max(configuredUploads, static_cast<std::size_t>(
            std::clamp(entity.voxelTerrain().meshUploadsPerFrame, 1, 8)));
        desiredWorkerBudget_ = std::max(desiredWorkerBudget_, static_cast<std::size_t>(
            resolvedMeshWorkers(entity.voxelTerrain().meshWorkers)));
        adaptiveScheduling = adaptiveScheduling || entity.voxelTerrain().adaptiveScheduling;
        targetFrameMilliseconds = std::clamp(
            entity.voxelTerrain().targetFrameMilliseconds, 4.0f, 33.33f);
    }
    if (!adaptiveScheduling) {
        adaptiveWorkerBudget_ = desiredWorkerBudget_;
        meshUploadsPerFrame_ = configuredUploads;
    } else {
        adaptiveWorkerBudget_ = std::clamp(adaptiveWorkerBudget_, std::size_t{1}, desiredWorkerBudget_);
        meshUploadsPerFrame_ = std::clamp(meshUploadsPerFrame_, std::size_t{1}, configuredUploads);
        if (schedulerCooldown_ > 0) --schedulerCooldown_;
        if (frameMilliseconds > targetFrameMilliseconds * 1.25f) {
            adaptiveWorkerBudget_ = std::max<std::size_t>(1, adaptiveWorkerBudget_ - 2);
            meshUploadsPerFrame_ = 1;
            schedulerCooldown_ = 8;
        } else if (frameMilliseconds > targetFrameMilliseconds && schedulerCooldown_ == 0) {
            adaptiveWorkerBudget_ = std::max<std::size_t>(1, adaptiveWorkerBudget_ - 1);
            meshUploadsPerFrame_ = std::max<std::size_t>(1, meshUploadsPerFrame_ - 1);
            schedulerCooldown_ = 12;
        } else if (frameMilliseconds < targetFrameMilliseconds * 0.72f
            && schedulerCooldown_ == 0) {
            adaptiveWorkerBudget_ = std::min(desiredWorkerBudget_, adaptiveWorkerBudget_ + 1);
            meshUploadsPerFrame_ = std::min(configuredUploads, meshUploadsPerFrame_ + 1);
            schedulerCooldown_ = 20;
        }
    }
    stats_.activeMeshWorkers = static_cast<std::uint32_t>(adaptiveWorkerBudget_);
    stats_.activeUploadBudget = static_cast<std::uint32_t>(meshUploadsPerFrame_);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    const GLfloat clearColor[]{0.055f, 0.075f, 0.10f, 1.0f};
    const GLuint clearEntity[]{0};
    glClearBufferfv(GL_COLOR, 0, clearColor);
    glClearBufferuiv(GL_COLOR, 1, clearEntity);
    glClear(GL_DEPTH_BUFFER_BIT);
    processFinishedChunkJobs();
    processFinishedMeshJobs();
    const glm::mat4 viewProjection = camera.projectionMatrix(aspectRatio) * camera.viewMatrix();
    const Frustum frustum(viewProjection);
    Shader& shader = assets_.shader("VoxelLit");
    shader.bind();
    shader.setMat4("uViewProjection", viewProjection);

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
        ++stats_.drawCalls;
        stats_.triangles += mesh.triangleCount() * instances.size();
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
        ++stats_.drawCalls;
        stats_.triangles += mesh.triangleCount();
    }

    Shader& terrainShader = assets_.shader("TerrainLit");
    terrainShader.bind();
    terrainShader.setMat4("uViewProjection", viewProjection);
    terrainShader.setVec3("uLightDirection", lightDirection);
    terrainShader.setVec3("uLightColor", lightColor);
    terrainShader.setFloat("uLightIntensity", lightIntensity);
    terrainShader.setInt("uTexture", 0);
    assets_.texture("Blocks").bind(0);

    struct TerrainDrawItem {
        Mesh* mesh;
        glm::mat4 transform;
        std::uint32_t entityId;
        float distanceSquared;
        ChunkOcclusionState* occlusion;
    };
    std::vector<TerrainDrawItem> terrainDrawItems;
    std::vector<glm::mat4> chunkBounds;

    for (const Entity& entity : scene.entities()) {
        if (!entity.hasVoxelTerrain()) continue;
        const int seed = entity.voxelTerrain().seed;
        auto cached = terrainWorlds_.find(entity.id());
        if (cached == terrainWorlds_.end() || cached->second.seed != seed) {
            TerrainWorldRenderData data;
            data.seed = seed;
            data.world = std::make_unique<VoxelWorld>(seed);
            terrainWorlds_[entity.id()] = std::move(data);
            cached = terrainWorlds_.find(entity.id());
        }

        const glm::mat4 terrainTransform = scene.worldTransform(entity);
        const glm::vec3 localCamera = glm::vec3(glm::inverse(terrainTransform)
            * glm::vec4(camera.position(), 1.0f));
        const int centerChunkX = VoxelWorld::floorDiv(static_cast<int>(glm::floor(localCamera.x)),
            Chunk::Width);
        const int centerChunkZ = VoxelWorld::floorDiv(static_cast<int>(glm::floor(localCamera.z)),
            Chunk::Depth);
        const int viewRadius = std::clamp(entity.voxelTerrain().viewRadius, 1, 4);
        cached->second.centerChunkX = centerChunkX;
        cached->second.centerChunkZ = centerChunkZ;
        cached->second.activeRadius = viewRadius;
        scheduleChunkJobs(entity.id(), entity.voxelTerrain().chunkLoadsPerFrame);
        const std::vector<std::int64_t> created;
        const std::vector<std::int64_t> removed =
            cached->second.world->unloadOutside(centerChunkX, centerChunkZ, viewRadius + 2);
        for (const std::int64_t chunkKey : removed) {
            cached->second.meshes.erase(chunkKey);
            cached->second.dirtyMeshes.erase(chunkKey);
            ++cached->second.revisions[chunkKey];
            const auto query = cached->second.occlusion.find(chunkKey);
            if (query != cached->second.occlusion.end()) {
                glDeleteQueries(2, query->second.queries);
                cached->second.occlusion.erase(query);
            }
        }
        std::unordered_set<std::int64_t> dirty(created.begin(), created.end());
        std::unordered_set<std::int64_t> forceDirty(created.begin(), created.end());
        for (const std::int64_t chunkKey : created) {
            const int x = VoxelWorld::chunkXFromKey(chunkKey);
            const int z = VoxelWorld::chunkZFromKey(chunkKey);
            constexpr int neighbors[4][2]{{-1,0},{1,0},{0,-1},{0,1}};
            for (const auto& offset : neighbors) {
                if (cached->second.world->findChunk(x + offset[0], z + offset[1])) {
                    const std::int64_t neighborKey = VoxelWorld::key(x + offset[0], z + offset[1]);
                    dirty.insert(neighborKey);
                    forceDirty.insert(neighborKey);
                }
            }
        }
        for (const auto& pair : cached->second.world->chunks()) {
            if (cached->second.meshes.find(pair.first) == cached->second.meshes.end()) dirty.insert(pair.first);
        }
        for (const std::int64_t chunkKey : dirty) {
            markTerrainMeshDirty(entity.id(), VoxelWorld::chunkXFromKey(chunkKey),
                VoxelWorld::chunkZFromKey(chunkKey), forceDirty.find(chunkKey) != forceDirty.end());
        }
        scheduleTerrainMeshJobs(entity.id(), centerChunkX, centerChunkZ,
            static_cast<int>(adaptiveWorkerBudget_), entity.voxelTerrain().meshQueueLimit);
        stats_.loadedChunks += static_cast<std::uint32_t>(cached->second.world->chunks().size());
        for (const auto& pair : cached->second.world->chunks()) {
            const Chunk& chunk = *pair.second;
            const glm::mat4 chunkTransform = glm::translate(terrainTransform,
                glm::vec3{chunk.chunkX() * Chunk::Width, 0.0f, chunk.chunkZ() * Chunk::Depth});
            if (chunkBounds_) {
                chunkBounds.push_back(glm::scale(glm::translate(chunkTransform,
                    glm::vec3{Chunk::Width * 0.5f, Chunk::Height * 0.5f, Chunk::Depth * 0.5f}),
                    glm::vec3{Chunk::Width, Chunk::Height, Chunk::Depth}));
            }
            const auto mesh = cached->second.meshes.find(pair.first);
            if (mesh == cached->second.meshes.end()) continue;
            if (frustumCulling_ && !frustum.intersectsTransformedBox(chunkTransform,
                glm::vec3{0.0f}, glm::vec3{Chunk::Width, Chunk::Height, Chunk::Depth})) continue;
            ChunkOcclusionState& occlusion = cached->second.occlusion[pair.first];
            if (occlusion.queries[0] == 0) glGenQueries(2, occlusion.queries);
            const glm::vec3 chunkCenter = glm::vec3(chunkTransform
                * glm::vec4(Chunk::Width * 0.5f, Chunk::Height * 0.5f,
                    Chunk::Depth * 0.5f, 1.0f));
            const glm::vec3 cameraOffset = chunkCenter - camera.position();
            terrainDrawItems.push_back({mesh->second.get(), chunkTransform, entity.id(),
                glm::dot(cameraOffset, cameraOffset), &occlusion});
        }
    }

    std::sort(terrainDrawItems.begin(), terrainDrawItems.end(),
        [](const TerrainDrawItem& left, const TerrainDrawItem& right) {
            return left.distanceSquared < right.distanceSquared;
        });

    const unsigned int queryIndex = static_cast<unsigned int>(frameIndex_ & 1u);
    const unsigned int resultIndex = queryIndex ^ 1u;
    const glm::vec3 cameraDelta = camera.position() - lastOcclusionCameraPosition_;
    const bool cameraMoved = !hasLastOcclusionCamera_
        || glm::dot(cameraDelta, cameraDelta) > 0.0001f
        || glm::dot(camera.forward(), lastOcclusionCameraForward_) < 0.99995f;
    lastOcclusionCameraPosition_ = camera.position();
    lastOcclusionCameraForward_ = camera.forward();
    hasLastOcclusionCamera_ = true;
    for (TerrainDrawItem& item : terrainDrawItems) {
        ChunkOcclusionState& state = *item.occlusion;
        if (!occlusionCulling_ || cameraMoved) {
            state.visible = true;
            state.consecutiveOccludedFrames = 0;
        }
        if (!state.issued[resultIndex]) continue;
        GLint available = GL_FALSE;
        glGetQueryObjectiv(state.queries[resultIndex], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            GLuint samplesPassed = 1;
            glGetQueryObjectuiv(state.queries[resultIndex], GL_QUERY_RESULT, &samplesPassed);
            if (samplesPassed != 0) {
                state.visible = true;
                state.consecutiveOccludedFrames = 0;
            } else if (occlusionCulling_ && !cameraMoved) {
                state.consecutiveOccludedFrames = static_cast<std::uint8_t>(
                    std::min(3, static_cast<int>(state.consecutiveOccludedFrames) + 1));
                // Three confirmations prevent a delayed result or a one-frame depth
                // disagreement from punching visible holes into the world.
                if (state.consecutiveOccludedFrames >= 3) state.visible = false;
            }
            state.issued[resultIndex] = false;
        }
    }

    const auto drawTerrain = [&](bool onlyVisible) {
        for (const TerrainDrawItem& item : terrainDrawItems) {
            if (onlyVisible && !item.occlusion->visible) continue;
            terrainShader.setUInt("uEntityId", item.entityId);
            terrainShader.setMat4("uModel", item.transform);
            item.mesh->draw();
        }
    };

    // Depth-only front-to-back pass. Queries are consumed on a later frame, never
    // waited on, so hidden chunks skip expensive color shading without a GPU stall.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    for (const TerrainDrawItem& item : terrainDrawItems) {
        terrainShader.setUInt("uEntityId", item.entityId);
        terrainShader.setMat4("uModel", item.transform);
        glBeginQuery(GL_ANY_SAMPLES_PASSED, item.occlusion->queries[queryIndex]);
        item.mesh->draw();
        glEndQuery(GL_ANY_SAMPLES_PASSED);
        item.occlusion->issued[queryIndex] = true;
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    if (wireframe_) {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawTerrain(true);

        glDisable(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        drawTerrain(true);
    }
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    for (const TerrainDrawItem& item : terrainDrawItems) {
        if (!item.occlusion->visible) continue;
        ++stats_.visibleChunks;
        ++stats_.drawCalls;
        stats_.triangles += item.mesh->triangleCount();
    }

    if (chunkBounds_ && !chunkBounds.empty()) {
        Shader& debugShader = assets_.shader("DebugLine");
        debugShader.bind();
        debugShader.setMat4("uViewProjection", viewProjection);
        debugShader.setVec3("uColor", glm::vec3{1.0f, 0.72f, 0.12f});
        Mesh& wireCube = assets_.mesh("WireCube");
        wireCube.uploadInstances(chunkBounds);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        glLineWidth(1.5f);
        wireCube.drawInstanced();
        glLineWidth(1.0f);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
    }
    if (const auto target = raycastTerrain(scene, camera, 8.0f)) {
        const Entity* terrainEntity = scene.findEntity(target->entityId);
        if (terrainEntity) {
            const glm::mat4 targetTransform = glm::scale(glm::translate(
                scene.worldTransform(*terrainEntity), glm::vec3(target->block) + glm::vec3{0.5f}),
                glm::vec3{1.006f});
            Shader& debugShader = assets_.shader("DebugLine");
            debugShader.bind();
            debugShader.setMat4("uViewProjection", viewProjection);
            debugShader.setVec3("uColor", glm::vec3{1.0f, 0.95f, 0.25f});
            Mesh& wireCube = assets_.mesh("WireCube");
            wireCube.uploadInstances({targetTransform});
            glDisable(GL_CULL_FACE);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            glLineWidth(2.0f);
            wireCube.drawInstanced();
            glLineWidth(1.0f);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
            glEnable(GL_CULL_FACE);
        }
    }
    if (measureGpuFrame) {
        glEndQuery(GL_TIME_ELAPSED);
        gpuTimerIssued_[gpuQueryIndex] = true;
    }
    ++frameIndex_;

    const GLenum error = glGetError();
    stats_.pendingMeshJobs = static_cast<std::uint32_t>(
        meshJobs_.size() + completedMeshJobs_.size());
    stats_.pendingChunkJobs = static_cast<std::uint32_t>(chunkJobs_.size());
    stats_.cpuRenderMilliseconds = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - cpuRenderStart).count();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (error != GL_NO_ERROR) throw std::runtime_error("OpenGL error: " + std::to_string(error));
}

void Renderer::markTerrainMeshDirty(std::uint32_t entityId, int chunkX, int chunkZ,
    bool forceNewRevision) {
    TerrainWorldRenderData& data = terrainWorlds_.at(entityId);
    Chunk* chunk = data.world->findChunk(chunkX, chunkZ);
    if (!chunk) return;
    const std::int64_t chunkKey = VoxelWorld::key(chunkX, chunkZ);
    const bool newlyDirty = data.dirtyMeshes.insert(chunkKey).second;
    if (newlyDirty || forceNewRevision) ++data.revisions[chunkKey];
}

void Renderer::processFinishedMeshJobs() {
    const auto processingStart = std::chrono::steady_clock::now();
    // Collect CPU results without touching GPU resources yet.
    for (auto job = meshJobs_.begin(); job != meshJobs_.end();) {
        if (job->wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            ++job;
            continue;
        }
        completedMeshJobs_.push_back(job->get());
        job = meshJobs_.erase(job);
    }

    // Uploading a mesh can force driver synchronization. A small per-frame budget
    // prevents several completed chunks from producing one large frame-time spike.
    std::size_t uploads = 0;
    while (!completedMeshJobs_.empty() && uploads < meshUploadsPerFrame_) {
        MeshJobResult result = std::move(completedMeshJobs_.front());
        completedMeshJobs_.pop_front();
        const auto terrain = terrainWorlds_.find(result.entityId);
        if (terrain == terrainWorlds_.end()) continue;
        TerrainWorldRenderData& data = terrain->second;
        if (data.seed != result.seed) continue;
        data.pendingMeshes.erase(result.chunkKey);
        const int resultChunkX = VoxelWorld::chunkXFromKey(result.chunkKey);
        const int resultChunkZ = VoxelWorld::chunkZFromKey(result.chunkKey);
        if (!data.world->findChunk(resultChunkX, resultChunkZ)) continue;
        if (data.revisions[result.chunkKey] != result.revision) continue;

        const auto mesh = data.meshes.find(result.chunkKey);
        if (mesh == data.meshes.end()) {
            data.meshes[result.chunkKey] = std::make_unique<Mesh>(
                result.meshData.vertices, result.meshData.indices);
        } else {
            mesh->second->updateGeometry(result.meshData.vertices, result.meshData.indices);
        }
        data.dirtyMeshes.erase(result.chunkKey);
        ++uploads;
        ++stats_.uploadedMeshes;
    }
    stats_.meshProcessingMilliseconds = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - processingStart).count();
}

void Renderer::processFinishedChunkJobs() {
    for (auto job = chunkJobs_.begin(); job != chunkJobs_.end();) {
        if (job->wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            ++job;
            continue;
        }
        ChunkJobResult result = job->get();
        job = chunkJobs_.erase(job);
        const auto terrain = terrainWorlds_.find(result.entityId);
        if (terrain == terrainWorlds_.end() || terrain->second.seed != result.seed) continue;
        TerrainWorldRenderData& data = terrain->second;
        data.pendingChunks.erase(result.chunkKey);
        const int chunkX = VoxelWorld::chunkXFromKey(result.chunkKey);
        const int chunkZ = VoxelWorld::chunkZFromKey(result.chunkKey);
        if (std::max(std::abs(chunkX - data.centerChunkX),
            std::abs(chunkZ - data.centerChunkZ)) > data.activeRadius + 1) continue;
        if (!data.world->adoptChunk(std::move(result.chunk))) continue;
        ++stats_.generatedChunks;
        markTerrainMeshDirty(result.entityId, chunkX, chunkZ, true);
        constexpr int Neighbors[4][2]{{-1,0},{1,0},{0,-1},{0,1}};
        for (const auto& offset : Neighbors) {
            if (data.world->findChunk(chunkX + offset[0], chunkZ + offset[1])) {
                markTerrainMeshDirty(result.entityId, chunkX + offset[0], chunkZ + offset[1], true);
            }
        }
    }
}

void Renderer::scheduleChunkJobs(std::uint32_t entityId, int maximumOutstanding) {
    TerrainWorldRenderData& data = terrainWorlds_.at(entityId);
    const std::size_t limit = static_cast<std::size_t>(std::clamp(maximumOutstanding, 1, 8));
    if (data.pendingChunks.size() >= limit) return;
    const auto missing = data.world->missingArea(
        data.centerChunkX, data.centerChunkZ, data.activeRadius);
    for (const auto& coordinates : missing) {
        if (data.pendingChunks.size() >= limit) break;
        const std::int64_t chunkKey = VoxelWorld::key(coordinates.first, coordinates.second);
        if (data.pendingChunks.find(chunkKey) != data.pendingChunks.end()) continue;
        const int seed = data.seed;
        data.pendingChunks.insert(chunkKey);
        chunkJobs_.push_back(meshThreadPool_.submit(
            [entityId, seed, chunkKey, chunkX = coordinates.first,
                chunkZ = coordinates.second]() mutable {
                ChunkJobResult result;
                result.entityId = entityId;
                result.seed = seed;
                result.chunkKey = chunkKey;
                result.chunk = std::make_unique<Chunk>(seed, chunkX, chunkZ);
                return result;
            }));
    }
}

void Renderer::scheduleTerrainMeshJobs(std::uint32_t entityId,
    int centerChunkX, int centerChunkZ, int requestedWorkers, int requestedQueueLimit) {
    const std::size_t maxConcurrentJobs = static_cast<std::size_t>(
        resolvedMeshWorkers(requestedWorkers));
    const std::size_t maxQueuedResults = static_cast<std::size_t>(
        std::clamp(requestedQueueLimit, static_cast<int>(maxConcurrentJobs), 64));
    TerrainWorldRenderData& data = terrainWorlds_.at(entityId);
    std::vector<std::int64_t> prioritizedChunks(data.dirtyMeshes.begin(), data.dirtyMeshes.end());
    std::sort(prioritizedChunks.begin(), prioritizedChunks.end(),
        [centerChunkX, centerChunkZ](std::int64_t left, std::int64_t right) {
            const int leftX = VoxelWorld::chunkXFromKey(left) - centerChunkX;
            const int leftZ = VoxelWorld::chunkZFromKey(left) - centerChunkZ;
            const int rightX = VoxelWorld::chunkXFromKey(right) - centerChunkX;
            const int rightZ = VoxelWorld::chunkZFromKey(right) - centerChunkZ;
            return leftX * leftX + leftZ * leftZ < rightX * rightX + rightZ * rightZ;
        });
    for (const std::int64_t chunkKey : prioritizedChunks) {
        if (meshJobs_.size() >= maxConcurrentJobs) break;
        if (meshJobs_.size() + completedMeshJobs_.size() >= maxQueuedResults) break;
        if (data.pendingMeshes.find(chunkKey) != data.pendingMeshes.end()) continue;
        const int chunkX = VoxelWorld::chunkXFromKey(chunkKey);
        const int chunkZ = VoxelWorld::chunkZFromKey(chunkKey);
        Chunk* chunk = data.world->findChunk(chunkX, chunkZ);
        if (!chunk) continue;
        ChunkMeshSnapshot snapshot = ChunkMeshBuilder::capture(*chunk, *data.world);
        const std::uint64_t revision = data.revisions[chunkKey];
        const int seed = data.seed;
        data.pendingMeshes.insert(chunkKey);
        meshJobs_.push_back(meshThreadPool_.submit(
            [entityId, seed, chunkKey, revision, snapshot = std::move(snapshot)]() mutable {
                MeshJobResult result;
                result.entityId = entityId;
                result.seed = seed;
                result.chunkKey = chunkKey;
                result.revision = revision;
                result.meshData = ChunkMeshBuilder::build(snapshot);
                return result;
            }));
    }
}

bool Renderer::editTerrain(const Scene& scene, const EditorCamera& camera, bool placeBlock,
    BlockType placedBlock, const glm::vec3& forbiddenMin, const glm::vec3& forbiddenMax) {
    return editTerrainDetailed(scene, camera, placeBlock, placedBlock,
        forbiddenMin, forbiddenMax) == TerrainEditResult::Success;
}

Renderer::TerrainEditResult Renderer::editTerrainDetailed(const Scene& scene,
    const EditorCamera& camera, bool placeBlock, BlockType placedBlock,
    const glm::vec3& forbiddenMin, const glm::vec3& forbiddenMax) {
    const auto hit = raycastTerrain(scene, camera, 8.0f);
    if (!hit) return TerrainEditResult::NoTarget;
    auto worldEntry = terrainWorlds_.find(hit->entityId);
    if (worldEntry == terrainWorlds_.end()) return TerrainEditResult::MissingWorld;
    const glm::ivec3 target = placeBlock ? hit->adjacent : hit->block;
    VoxelWorld& world = *worldEntry->second.world;
    const BlockType replacedBlock = world.block(target.x, target.y, target.z);
    if (placeBlock && isSolid(world.block(target.x, target.y, target.z)))
        return TerrainEditResult::TargetOccupied;
    if (placeBlock) {
        const Entity* terrainEntity = scene.findEntity(hit->entityId);
        if (!terrainEntity) return TerrainEditResult::MissingWorld;
        const glm::mat4 transform = scene.worldTransform(*terrainEntity);
        glm::vec3 blockMin{std::numeric_limits<float>::infinity()};
        glm::vec3 blockMax{-std::numeric_limits<float>::infinity()};
        for (int corner = 0; corner < 8; ++corner) {
            const glm::vec3 local = glm::vec3(target) + glm::vec3{
                static_cast<float>(corner & 1),
                static_cast<float>((corner >> 1) & 1),
                static_cast<float>((corner >> 2) & 1)};
            const glm::vec3 worldCorner = glm::vec3(transform * glm::vec4(local, 1.0f));
            blockMin = glm::min(blockMin, worldCorner);
            blockMax = glm::max(blockMax, worldCorner);
        }
        constexpr float ContactEpsilon = 0.001f;
        const bool intersectsPlayer = blockMax.x > forbiddenMin.x + ContactEpsilon
            && blockMin.x < forbiddenMax.x - ContactEpsilon
            && blockMax.y > forbiddenMin.y + ContactEpsilon
            && blockMin.y < forbiddenMax.y - ContactEpsilon
            && blockMax.z > forbiddenMin.z + ContactEpsilon
            && blockMin.z < forbiddenMax.z - ContactEpsilon;
        if (intersectsPlayer) return TerrainEditResult::IntersectsForbiddenBounds;
    }
    world.setBlock(target.x, target.y, target.z,
        placeBlock ? placedBlock : BlockType::Air);
    const int chunkX = VoxelWorld::floorDiv(target.x, Chunk::Width);
    const int chunkZ = VoxelWorld::floorDiv(target.z, Chunk::Depth);
    const int localX = target.x - chunkX * Chunk::Width;
    const int localZ = target.z - chunkZ * Chunk::Depth;
    markTerrainMeshDirty(hit->entityId, chunkX, chunkZ, true);
    if (localX == 0) markTerrainMeshDirty(hit->entityId, chunkX - 1, chunkZ, true);
    if (localX == Chunk::Width - 1) markTerrainMeshDirty(hit->entityId, chunkX + 1, chunkZ, true);
    if (localZ == 0) markTerrainMeshDirty(hit->entityId, chunkX, chunkZ - 1, true);
    if (localZ == Chunk::Depth - 1) markTerrainMeshDirty(hit->entityId, chunkX, chunkZ + 1, true);
    if (!placeBlock && replacedBlock == BlockType::Wood)
        decayLeavesAround(hit->entityId, world, target);
    return TerrainEditResult::Success;
}

std::optional<Renderer::TerrainBlockTarget> Renderer::terrainTargetBlock(
    const Scene& scene, const EditorCamera& camera) const {
    const auto hit = raycastTerrain(scene, camera, 8.0f);
    if (!hit) return std::nullopt;
    const auto worldEntry = terrainWorlds_.find(hit->entityId);
    if (worldEntry == terrainWorlds_.end()) return std::nullopt;
    const BlockType type = worldEntry->second.world->block(hit->block.x, hit->block.y, hit->block.z);
    if (!isSolid(type)) return std::nullopt;
    return TerrainBlockTarget{hit->entityId, hit->block, type};
}

void Renderer::decayLeavesAround(std::uint32_t entityId, VoxelWorld& world,
    const glm::ivec3& removedWood) {
    constexpr int SearchRadius = 8;
    constexpr int DecayRadius = 5;
    constexpr int MaximumLeafDistance = 6;
    constexpr int Diameter = SearchRadius * 2 + 1;
    constexpr std::uint8_t Unvisited = 255;
    std::vector<std::uint8_t> distance(Diameter * Diameter * Diameter, Unvisited);
    const auto indexOf = [](int x, int y, int z) {
        return static_cast<std::size_t>((x + SearchRadius)
            + Diameter * ((z + SearchRadius) + Diameter * (y + SearchRadius)));
    };
    std::deque<glm::ivec3> frontier;
    for (int y = -SearchRadius; y <= SearchRadius; ++y) {
        for (int z = -SearchRadius; z <= SearchRadius; ++z) {
            for (int x = -SearchRadius; x <= SearchRadius; ++x) {
                const glm::ivec3 offset{x, y, z};
                const glm::ivec3 position = removedWood + offset;
                if (world.block(position.x, position.y, position.z) != BlockType::Wood) continue;
                distance[indexOf(x, y, z)] = 0;
                frontier.push_back(offset);
            }
        }
    }
    constexpr glm::ivec3 Directions[6]{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    while (!frontier.empty()) {
        const glm::ivec3 offset = frontier.front();
        frontier.pop_front();
        const std::uint8_t currentDistance = distance[indexOf(offset.x, offset.y, offset.z)];
        if (currentDistance >= MaximumLeafDistance) continue;
        for (const glm::ivec3& direction : Directions) {
            const glm::ivec3 next = offset + direction;
            if (glm::any(glm::greaterThan(glm::abs(next), glm::ivec3{SearchRadius}))) continue;
            const std::size_t nextIndex = indexOf(next.x, next.y, next.z);
            if (distance[nextIndex] != Unvisited) continue;
            const glm::ivec3 position = removedWood + next;
            if (world.block(position.x, position.y, position.z) != BlockType::Leaves) continue;
            distance[nextIndex] = static_cast<std::uint8_t>(currentDistance + 1);
            frontier.push_back(next);
        }
    }

    std::unordered_set<std::int64_t> changedChunks;
    for (int y = -DecayRadius; y <= DecayRadius; ++y) {
        for (int z = -DecayRadius; z <= DecayRadius; ++z) {
            for (int x = -DecayRadius; x <= DecayRadius; ++x) {
                if (distance[indexOf(x, y, z)] != Unvisited) continue;
                const glm::ivec3 position = removedWood + glm::ivec3{x, y, z};
                if (world.block(position.x, position.y, position.z) != BlockType::Leaves) continue;
                world.setBlock(position.x, position.y, position.z, BlockType::Air);
                changedChunks.insert(VoxelWorld::key(
                    VoxelWorld::floorDiv(position.x, Chunk::Width),
                    VoxelWorld::floorDiv(position.z, Chunk::Depth)));
            }
        }
    }
    constexpr int Neighbors[5][2]{{0,0},{-1,0},{1,0},{0,-1},{0,1}};
    for (const std::int64_t changed : changedChunks) {
        const int chunkX = VoxelWorld::chunkXFromKey(changed);
        const int chunkZ = VoxelWorld::chunkZFromKey(changed);
        for (const auto& neighbor : Neighbors)
            markTerrainMeshDirty(entityId, chunkX + neighbor[0], chunkZ + neighbor[1], true);
    }
}

std::optional<Renderer::TerrainRayHit> Renderer::raycastTerrain(const Scene& scene,
    const EditorCamera& camera, float maximumDistance) const {
    constexpr float Infinity = std::numeric_limits<float>::infinity();
    for (const Entity& entity : scene.entities()) {
        if (!entity.hasVoxelTerrain()) continue;
        const auto worldEntry = terrainWorlds_.find(entity.id());
        if (worldEntry == terrainWorlds_.end()) continue;
        const glm::mat4 inverseTransform = glm::inverse(scene.worldTransform(entity));
        const glm::vec3 origin = glm::vec3(inverseTransform * glm::vec4(camera.position(), 1.0f));
        const glm::vec3 direction = glm::normalize(glm::vec3(
            inverseTransform * glm::vec4(camera.forward(), 0.0f)));
        glm::ivec3 cell = glm::ivec3(glm::floor(origin));
        glm::ivec3 previous = cell;
        glm::ivec3 step{0};
        glm::vec3 delta{Infinity};
        glm::vec3 next{Infinity};
        for (int axis = 0; axis < 3; ++axis) {
            if (direction[axis] == 0.0f) continue;
            step[axis] = direction[axis] > 0.0f ? 1 : -1;
            delta[axis] = glm::abs(1.0f / direction[axis]);
            const float boundary = step[axis] > 0
                ? static_cast<float>(cell[axis] + 1) : static_cast<float>(cell[axis]);
            next[axis] = (boundary - origin[axis]) / direction[axis];
        }
        const VoxelWorld& world = *worldEntry->second.world;
        float distance = 0.0f;
        while (distance <= maximumDistance) {
            if (isSolid(world.block(cell.x, cell.y, cell.z))) {
                return TerrainRayHit{entity.id(), cell, previous};
            }
            previous = cell;
            int axis = next.x < next.y ? (next.x < next.z ? 0 : 2)
                : (next.y < next.z ? 1 : 2);
            distance = next[axis];
            if (distance > maximumDistance) break;
            cell[axis] += step[axis];
            next[axis] += delta[axis];
        }
    }
    return std::nullopt;
}

bool Renderer::isSolidAtWorld(const Scene& scene, const glm::vec3& worldPosition) const {
    for (const Entity& entity : scene.entities()) {
        if (!entity.hasVoxelTerrain()) continue;
        const auto found = terrainWorlds_.find(entity.id());
        if (found == terrainWorlds_.end()) continue;
        const glm::vec3 local = glm::vec3(glm::inverse(scene.worldTransform(entity))
            * glm::vec4(worldPosition, 1.0f));
        const glm::ivec3 block = glm::ivec3(glm::floor(local));
        if (isSolid(found->second.world->block(block.x, block.y, block.z))) return true;
    }
    return false;
}

std::optional<glm::vec3> Renderer::terrainSpawnPoint(const Scene& scene) const {
    for (const Entity& entity : scene.entities()) {
        if (!entity.hasVoxelTerrain()) continue;
        const auto found = terrainWorlds_.find(entity.id());
        if (found == terrainWorlds_.end()) continue;
        const int x = Chunk::Width / 2;
        const int z = Chunk::Depth / 2;
        for (int y = Chunk::Height - 1; y >= 0; --y) {
            if (!isSolid(found->second.world->block(x, y, z))) continue;
            return glm::vec3(scene.worldTransform(entity)
                * glm::vec4(x + 0.5f, y + 1.01f, z + 0.5f, 1.0f));
        }
    }
    return std::nullopt;
}

} // namespace Engine
