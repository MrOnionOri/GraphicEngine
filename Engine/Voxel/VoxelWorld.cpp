#include "Engine/Voxel/VoxelWorld.h"
#include "Engine/Core/AssetPath.h"

#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>

namespace Engine {

namespace {
constexpr std::uint32_t ChunkFileMagic = 0x48434547; // GECH
constexpr std::uint32_t ChunkFileVersion = 2;
}

VoxelWorld::VoxelWorld(int seed) : seed_(seed) {
    saveDirectory_ = AssetPath::executableDirectory() / "Saves"
        / ("World_" + std::to_string(seed_));
    std::filesystem::create_directories(saveDirectory_);
}

VoxelWorld::~VoxelWorld() { saveAll(); }

std::vector<std::int64_t> VoxelWorld::ensureArea(int centerChunkX, int centerChunkZ, int radius,
    int maximumChunksToCreate) {
    std::vector<std::int64_t> created;
    struct Candidate { int x; int z; int distanceSquared; };
    std::vector<Candidate> candidates;
    for (int z = centerChunkZ - radius; z <= centerChunkZ + radius; ++z) {
        for (int x = centerChunkX - radius; x <= centerChunkX + radius; ++x) {
            const std::int64_t chunkKey = key(x, z);
            if (chunks_.find(chunkKey) != chunks_.end()) continue;
            const int offsetX = x - centerChunkX;
            const int offsetZ = z - centerChunkZ;
            candidates.push_back({x, z, offsetX * offsetX + offsetZ * offsetZ});
        }
    }
    std::sort(candidates.begin(), candidates.end(),
        [](const Candidate& left, const Candidate& right) {
            if (left.distanceSquared != right.distanceSquared)
                return left.distanceSquared < right.distanceSquared;
            if (left.z != right.z) return left.z < right.z;
            return left.x < right.x;
        });
    const int budget = std::clamp(maximumChunksToCreate, 1, 8);
    for (int index = 0; index < static_cast<int>(candidates.size()) && index < budget; ++index) {
        const Candidate& candidate = candidates[index];
        const std::int64_t chunkKey = key(candidate.x, candidate.z);
        chunks_[chunkKey] = std::make_unique<Chunk>(seed_, candidate.x, candidate.z);
        loadChunk(*chunks_[chunkKey]);
        created.push_back(chunkKey);
    }
    return created;
}

std::vector<std::pair<int, int>> VoxelWorld::missingArea(
    int centerChunkX, int centerChunkZ, int radius) const {
    struct Candidate { int x; int z; int distanceSquared; };
    std::vector<Candidate> candidates;
    for (int z = centerChunkZ - radius; z <= centerChunkZ + radius; ++z) {
        for (int x = centerChunkX - radius; x <= centerChunkX + radius; ++x) {
            if (chunks_.find(key(x, z)) != chunks_.end()) continue;
            const int offsetX = x - centerChunkX;
            const int offsetZ = z - centerChunkZ;
            candidates.push_back({x, z, offsetX * offsetX + offsetZ * offsetZ});
        }
    }
    std::sort(candidates.begin(), candidates.end(),
        [](const Candidate& left, const Candidate& right) {
            if (left.distanceSquared != right.distanceSquared)
                return left.distanceSquared < right.distanceSquared;
            if (left.z != right.z) return left.z < right.z;
            return left.x < right.x;
        });
    std::vector<std::pair<int, int>> result;
    result.reserve(candidates.size());
    for (const Candidate& candidate : candidates) result.emplace_back(candidate.x, candidate.z);
    return result;
}

bool VoxelWorld::adoptChunk(std::unique_ptr<Chunk> chunk) {
    if (!chunk) return false;
    const std::int64_t chunkKey = key(chunk->chunkX(), chunk->chunkZ());
    if (chunks_.find(chunkKey) != chunks_.end()) return false;
    loadChunk(*chunk);
    chunks_.emplace(chunkKey, std::move(chunk));
    return true;
}

BlockType VoxelWorld::block(int worldX, int worldY, int worldZ) const {
    const int chunkX = floorDiv(worldX, Chunk::Width);
    const int chunkZ = floorDiv(worldZ, Chunk::Depth);
    const Chunk* chunk = findChunk(chunkX, chunkZ);
    if (!chunk) return BlockType::Air;
    const int localX = worldX - chunkX * Chunk::Width;
    const int localZ = worldZ - chunkZ * Chunk::Depth;
    return chunk->block(localX, worldY, localZ);
}

void VoxelWorld::setBlock(int worldX, int worldY, int worldZ, BlockType type) {
    const int chunkX = floorDiv(worldX, Chunk::Width);
    const int chunkZ = floorDiv(worldZ, Chunk::Depth);
    Chunk* chunk = findChunk(chunkX, chunkZ);
    if (!chunk) return;
    const int localX = worldX - chunkX * Chunk::Width;
    const int localZ = worldZ - chunkZ * Chunk::Depth;
    if (chunk->block(localX, worldY, localZ) == type) return;
    chunk->setBlock(localX, worldY, localZ, type);
    dirtyChunks_.insert(key(chunkX, chunkZ));
}

std::vector<std::int64_t> VoxelWorld::unloadOutside(int centerChunkX, int centerChunkZ, int radius) {
    std::vector<std::int64_t> removed;
    for (auto chunk = chunks_.begin(); chunk != chunks_.end();) {
        const int x = chunk->second->chunkX();
        const int z = chunk->second->chunkZ();
        if (std::max(std::abs(x - centerChunkX), std::abs(z - centerChunkZ)) <= radius) {
            ++chunk;
            continue;
        }
        if (dirtyChunks_.erase(chunk->first) > 0) saveChunk(*chunk->second);
        removed.push_back(chunk->first);
        chunk = chunks_.erase(chunk);
    }
    return removed;
}

void VoxelWorld::saveAll() {
    for (const std::int64_t chunkKey : dirtyChunks_) {
        const auto chunk = chunks_.find(chunkKey);
        if (chunk != chunks_.end()) saveChunk(*chunk->second);
    }
    dirtyChunks_.clear();
}

bool VoxelWorld::loadChunk(Chunk& chunk) const {
    std::ifstream stream(chunkPath(chunk.chunkX(), chunk.chunkZ()), std::ios::binary);
    if (!stream) return false;
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t blockCount = 0;
    stream.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    stream.read(reinterpret_cast<char*>(&version), sizeof(version));
    stream.read(reinterpret_cast<char*>(&blockCount), sizeof(blockCount));
    if (!stream || magic != ChunkFileMagic || (version != 1 && version != ChunkFileVersion)
        || blockCount != chunk.blocks().size()) return false;
    std::vector<BlockType> blocks(blockCount);
    if (version == 1) {
        stream.read(reinterpret_cast<char*>(blocks.data()),
            static_cast<std::streamsize>(blocks.size() * sizeof(BlockType)));
    } else {
        std::uint32_t runCount = 0;
        stream.read(reinterpret_cast<char*>(&runCount), sizeof(runCount));
        std::size_t output = 0;
        for (std::uint32_t run = 0; stream && run < runCount; ++run) {
            std::uint16_t length = 0;
            BlockType type = BlockType::Air;
            stream.read(reinterpret_cast<char*>(&length), sizeof(length));
            stream.read(reinterpret_cast<char*>(&type), sizeof(type));
            if (length == 0 || !isValidBlockType(type) || output + length > blocks.size()) return false;
            std::fill_n(blocks.begin() + output, length, type);
            output += length;
        }
        if (output != blocks.size()) return false;
    }
    if (!stream) return false;
    for (const BlockType block : blocks) {
        if (!isValidBlockType(block)) return false;
    }
    return chunk.replaceBlocks(std::move(blocks));
}

void VoxelWorld::saveChunk(const Chunk& chunk) const {
    std::ofstream stream(chunkPath(chunk.chunkX(), chunk.chunkZ()),
        std::ios::binary | std::ios::trunc);
    if (!stream) return;
    const std::uint32_t blockCount = static_cast<std::uint32_t>(chunk.blocks().size());
    stream.write(reinterpret_cast<const char*>(&ChunkFileMagic), sizeof(ChunkFileMagic));
    stream.write(reinterpret_cast<const char*>(&ChunkFileVersion), sizeof(ChunkFileVersion));
    stream.write(reinterpret_cast<const char*>(&blockCount), sizeof(blockCount));
    struct Run { std::uint16_t length; BlockType type; };
    std::vector<Run> runs;
    for (const BlockType block : chunk.blocks()) {
        if (!runs.empty() && runs.back().type == block && runs.back().length < UINT16_MAX) {
            ++runs.back().length;
        } else {
            runs.push_back({1, block});
        }
    }
    const std::uint32_t runCount = static_cast<std::uint32_t>(runs.size());
    stream.write(reinterpret_cast<const char*>(&runCount), sizeof(runCount));
    for (const Run& run : runs) {
        stream.write(reinterpret_cast<const char*>(&run.length), sizeof(run.length));
        stream.write(reinterpret_cast<const char*>(&run.type), sizeof(run.type));
    }
}

std::filesystem::path VoxelWorld::chunkPath(int chunkX, int chunkZ) const {
    return saveDirectory_ / ("chunk_" + std::to_string(chunkX) + "_"
        + std::to_string(chunkZ) + ".bin");
}

Chunk* VoxelWorld::findChunk(int chunkX, int chunkZ) {
    const auto found = chunks_.find(key(chunkX, chunkZ));
    return found == chunks_.end() ? nullptr : found->second.get();
}

const Chunk* VoxelWorld::findChunk(int chunkX, int chunkZ) const {
    const auto found = chunks_.find(key(chunkX, chunkZ));
    return found == chunks_.end() ? nullptr : found->second.get();
}

std::int64_t VoxelWorld::key(int chunkX, int chunkZ) {
    const std::uint64_t high = static_cast<std::uint32_t>(chunkX);
    const std::uint64_t low = static_cast<std::uint32_t>(chunkZ);
    return static_cast<std::int64_t>((high << 32) | low);
}

int VoxelWorld::floorDiv(int value, int divisor) {
    int quotient = value / divisor;
    const int remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0))) --quotient;
    return quotient;
}

int VoxelWorld::chunkXFromKey(std::int64_t chunkKey) {
    return static_cast<std::int32_t>(static_cast<std::uint64_t>(chunkKey) >> 32);
}

int VoxelWorld::chunkZFromKey(std::int64_t chunkKey) {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(chunkKey));
}

} // namespace Engine
