#pragma once

#include "Engine/Voxel/Chunk.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>

namespace Engine {

class VoxelWorld {
public:
    explicit VoxelWorld(int seed);
    ~VoxelWorld();
    std::vector<std::int64_t> ensureArea(int centerChunkX, int centerChunkZ, int radius,
        int maximumChunksToCreate);
    std::vector<std::pair<int, int>> missingArea(int centerChunkX, int centerChunkZ,
        int radius) const;
    bool adoptChunk(std::unique_ptr<Chunk> chunk);
    BlockType block(int worldX, int worldY, int worldZ) const;
    void setBlock(int worldX, int worldY, int worldZ, BlockType type);
    Chunk* findChunk(int chunkX, int chunkZ);
    const Chunk* findChunk(int chunkX, int chunkZ) const;
    const std::unordered_map<std::int64_t, std::unique_ptr<Chunk>>& chunks() const { return chunks_; }
    std::vector<std::int64_t> unloadOutside(int centerChunkX, int centerChunkZ, int radius);
    void saveAll();

    static std::int64_t key(int chunkX, int chunkZ);
    static int floorDiv(int value, int divisor);
    static int chunkXFromKey(std::int64_t key);
    static int chunkZFromKey(std::int64_t key);

private:
    int seed_ = 0;
    std::filesystem::path saveDirectory_;
    std::unordered_map<std::int64_t, std::unique_ptr<Chunk>> chunks_;
    std::unordered_set<std::int64_t> dirtyChunks_;
    bool loadChunk(Chunk& chunk) const;
    void saveChunk(const Chunk& chunk) const;
    std::filesystem::path chunkPath(int chunkX, int chunkZ) const;
};

} // namespace Engine
