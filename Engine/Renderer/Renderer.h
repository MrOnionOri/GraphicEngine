#pragma once

#include "Engine/Renderer/AssetManager.h"
#include "Engine/Core/ThreadPool.h"
#include "Engine/Voxel/VoxelWorld.h"
#include "Engine/Voxel/ChunkMeshBuilder.h"
#include <cstdint>
#include <deque>
#include <future>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Engine {

class EditorCamera;
class Scene;

struct RendererStats {
    std::uint64_t triangles = 0;
    std::uint32_t drawCalls = 0;
    std::uint32_t visibleChunks = 0;
    std::uint32_t loadedChunks = 0;
    std::uint32_t pendingMeshJobs = 0;
    std::uint32_t uploadedMeshes = 0;
    std::uint32_t activeMeshWorkers = 0;
    std::uint32_t activeUploadBudget = 0;
    std::uint32_t generatedChunks = 0;
    std::uint32_t pendingChunkJobs = 0;
    float cpuRenderMilliseconds = 0.0f;
    float meshProcessingMilliseconds = 0.0f;
    float gpuRenderMilliseconds = 0.0f;
};

class Renderer {
public:
    enum class TerrainEditResult {
        Success,
        NoTarget,
        MissingWorld,
        TargetOccupied,
        IntersectsForbiddenBounds
    };

    struct TerrainBlockTarget {
        std::uint32_t entityId = 0;
        glm::ivec3 block{0};
        BlockType type = BlockType::Air;
    };

    Renderer();
    ~Renderer();
    void render(const Scene& scene, const EditorCamera& camera, float aspectRatio,
        float frameMilliseconds);
    bool editTerrain(const Scene& scene, const EditorCamera& camera, bool placeBlock,
        BlockType placedBlock, const glm::vec3& forbiddenMin, const glm::vec3& forbiddenMax);
    TerrainEditResult editTerrainDetailed(const Scene& scene, const EditorCamera& camera,
        bool placeBlock, BlockType placedBlock, const glm::vec3& forbiddenMin,
        const glm::vec3& forbiddenMax);
    std::optional<TerrainBlockTarget> terrainTargetBlock(const Scene& scene,
        const EditorCamera& camera) const;
    bool isSolidAtWorld(const Scene& scene, const glm::vec3& worldPosition) const;
    std::optional<glm::vec3> terrainSpawnPoint(const Scene& scene) const;
    void setWireframe(bool enabled) { wireframe_ = enabled; }
    void setFrustumCulling(bool enabled) { frustumCulling_ = enabled; }
    void setChunkBounds(bool enabled) { chunkBounds_ = enabled; }
    void setOcclusionCulling(bool enabled) { occlusionCulling_ = enabled; }
    const RendererStats& stats() const { return stats_; }

private:
    struct TerrainRayHit {
        std::uint32_t entityId = 0;
        glm::ivec3 block{0};
        glm::ivec3 adjacent{0};
    };
    struct ChunkOcclusionState {
        GLuint queries[2]{0, 0};
        bool issued[2]{false, false};
        bool visible = true;
        std::uint8_t consecutiveOccludedFrames = 0;
    };

    struct MeshJobResult {
        std::uint32_t entityId = 0;
        int seed = 0;
        std::int64_t chunkKey = 0;
        std::uint64_t revision = 0;
        ChunkMeshData meshData;
    };

    struct ChunkJobResult {
        std::uint32_t entityId = 0;
        int seed = 0;
        std::int64_t chunkKey = 0;
        std::unique_ptr<Chunk> chunk;
    };

    struct TerrainWorldRenderData {
        int seed = 0;
        std::unique_ptr<VoxelWorld> world;
        std::unordered_map<std::int64_t, std::unique_ptr<Mesh>> meshes;
        std::unordered_map<std::int64_t, std::uint64_t> revisions;
        std::unordered_set<std::int64_t> dirtyMeshes;
        std::unordered_set<std::int64_t> pendingMeshes;
        std::unordered_map<std::int64_t, ChunkOcclusionState> occlusion;
        std::unordered_set<std::int64_t> pendingChunks;
        int centerChunkX = 0;
        int centerChunkZ = 0;
        int activeRadius = 1;
    };

    AssetManager assets_;
    ThreadPool meshThreadPool_;
    std::unordered_map<std::uint32_t, TerrainWorldRenderData> terrainWorlds_;
    std::vector<std::future<MeshJobResult>> meshJobs_;
    std::vector<std::future<ChunkJobResult>> chunkJobs_;
    std::deque<MeshJobResult> completedMeshJobs_;
    RendererStats stats_;
    bool wireframe_ = false;
    bool frustumCulling_ = true;
    bool chunkBounds_ = false;
    bool occlusionCulling_ = true;
    std::uint64_t frameIndex_ = 0;
    std::size_t meshUploadsPerFrame_ = 2;
    std::size_t adaptiveWorkerBudget_ = 1;
    std::size_t desiredWorkerBudget_ = 1;
    std::uint32_t schedulerCooldown_ = 0;
    GLuint gpuTimerQueries_[3]{0, 0, 0};
    bool gpuTimerIssued_[3]{false, false, false};
    float lastGpuRenderMilliseconds_ = 0.0f;
    glm::vec3 lastOcclusionCameraPosition_{0.0f};
    glm::vec3 lastOcclusionCameraForward_{0.0f};
    bool hasLastOcclusionCamera_ = false;
    void markTerrainMeshDirty(std::uint32_t entityId, int chunkX, int chunkZ,
        bool forceNewRevision = false);
    void processFinishedMeshJobs();
    void processFinishedChunkJobs();
    void scheduleChunkJobs(std::uint32_t entityId, int maximumOutstanding);
    void scheduleTerrainMeshJobs(std::uint32_t entityId, int centerChunkX, int centerChunkZ,
        int requestedWorkers, int requestedQueueLimit);
    std::optional<TerrainRayHit> raycastTerrain(const Scene& scene,
        const EditorCamera& camera, float maximumDistance) const;
    void decayLeavesAround(std::uint32_t entityId, VoxelWorld& world,
        const glm::ivec3& removedWood);
};

} // namespace Engine
