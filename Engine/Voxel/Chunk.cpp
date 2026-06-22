#include "Engine/Voxel/Chunk.h"

#include <glm/common.hpp>
#include <algorithm>
#include <cmath>
#include <utility>
#include <cstdint>

namespace Engine {

namespace {
std::uint32_t mixBits(std::uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}

float randomValue(int x, int y, int z, int seed) {
    std::uint32_t hash = mixBits(static_cast<std::uint32_t>(x) ^ 0x9e3779b9u);
    hash = mixBits(hash ^ static_cast<std::uint32_t>(y) * 0x85ebca6bu);
    hash = mixBits(hash ^ static_cast<std::uint32_t>(z) * 0xc2b2ae35u);
    hash = mixBits(hash ^ static_cast<std::uint32_t>(seed));
    return static_cast<float>(hash & 0x00ffffffu) / static_cast<float>(0x00ffffffu);
}

float smooth(float value) { return value * value * (3.0f - 2.0f * value); }

float valueNoise2D(float x, float z, int seed) {
    const int x0 = static_cast<int>(std::floor(x));
    const int z0 = static_cast<int>(std::floor(z));
    const float tx = smooth(x - static_cast<float>(x0));
    const float tz = smooth(z - static_cast<float>(z0));
    const float a = glm::mix(randomValue(x0, 0, z0, seed),
        randomValue(x0 + 1, 0, z0, seed), tx);
    const float b = glm::mix(randomValue(x0, 0, z0 + 1, seed),
        randomValue(x0 + 1, 0, z0 + 1, seed), tx);
    return glm::mix(a, b, tz);
}

float valueNoise3D(float x, float y, float z, int seed) {
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int z0 = static_cast<int>(std::floor(z));
    const float tx = smooth(x - static_cast<float>(x0));
    const float ty = smooth(y - static_cast<float>(y0));
    const float tz = smooth(z - static_cast<float>(z0));
    float layers[2]{};
    for (int dz = 0; dz < 2; ++dz) {
        const float lower = glm::mix(randomValue(x0, y0, z0 + dz, seed),
            randomValue(x0 + 1, y0, z0 + dz, seed), tx);
        const float upper = glm::mix(randomValue(x0, y0 + 1, z0 + dz, seed),
            randomValue(x0 + 1, y0 + 1, z0 + dz, seed), tx);
        layers[dz] = glm::mix(lower, upper, ty);
    }
    return glm::mix(layers[0], layers[1], tz);
}

float fractalNoise2D(float x, float z, int seed, int octaves) {
    float result = 0.0f;
    float amplitude = 1.0f;
    float totalAmplitude = 0.0f;
    for (int octave = 0; octave < octaves; ++octave) {
        result += valueNoise2D(x, z, seed + octave * 1013) * amplitude;
        totalAmplitude += amplitude;
        x *= 2.03f;
        z *= 2.03f;
        amplitude *= 0.5f;
    }
    return result / totalAmplitude;
}

int terrainHeight(int worldX, int worldZ, int seed) {
    const float continent = fractalNoise2D(worldX * 0.025f, worldZ * 0.025f, seed, 4);
    const float hills = fractalNoise2D(worldX * 0.085f, worldZ * 0.085f, seed + 7919, 3);
    return std::clamp(static_cast<int>(std::round(
        3.0f + continent * 15.0f + hills * 7.0f)), 5, Chunk::Height - 3);
}

int floorDivide(int value, int divisor) {
    int result = value / divisor;
    if (value < 0 && value % divisor != 0) --result;
    return result;
}
}

Chunk::Chunk(int seed, int chunkX, int chunkZ)
    : blocks_(Width * Height * Depth, BlockType::Air), chunkX_(chunkX), chunkZ_(chunkZ) {
    generate(seed);
}

BlockType Chunk::block(int x, int y, int z) const {
    if (!inBounds(x, y, z)) return BlockType::Air;
    return blocks_[index(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType blockType) {
    if (!inBounds(x, y, z)) return;
    BlockType& current = blocks_[index(x, y, z)];
    if (current == blockType) return;
    std::uint16_t& solidCount = sectionSolidCounts_[y / SectionHeight];
    if (isSolid(current)) --solidCount;
    if (isSolid(blockType)) ++solidCount;
    current = blockType;
}

void Chunk::generate(int seed) {
    std::fill(blocks_.begin(), blocks_.end(), BlockType::Air);
    sectionSolidCounts_.fill(0);
    for (int x = 0; x < Width; ++x) {
        for (int z = 0; z < Depth; ++z) {
            const int worldX = chunkX_ * Width + x;
            const int worldZ = chunkZ_ * Depth + z;
            const int surface = terrainHeight(worldX, worldZ, seed);
            for (int y = 0; y <= surface; ++y) {
                if (y > 1 && y < surface - 3) {
                    const float broadCave = valueNoise3D(worldX * 0.075f, y * 0.095f,
                        worldZ * 0.075f, seed + 15485863);
                    const float caveDetail = valueNoise3D(worldX * 0.15f, y * 0.19f,
                        worldZ * 0.15f, seed + 32452843);
                    if (broadCave * 0.72f + caveDetail * 0.28f > 0.69f) continue;
                }
                BlockType type = BlockType::Stone;
                if (y == surface) type = BlockType::Grass;
                else if (y >= surface - 3) type = BlockType::Dirt;
                setBlock(x, y, z, type);
            }
        }
    }

    // Tree candidates live on a world-space grid, so trunks and canopies continue
    // deterministically across chunk borders regardless of generation order.
    constexpr int TreeCellSize = 7;
    const int baseX = chunkX_ * Width;
    const int baseZ = chunkZ_ * Depth;
    const auto placeWorldBlock = [&](int worldX, int y, int worldZ, BlockType type,
        bool replaceSolid) {
        const int localX = worldX - baseX;
        const int localZ = worldZ - baseZ;
        if (localX < 0 || localX >= Width || localZ < 0 || localZ >= Depth
            || y < 0 || y >= Height) return;
        if (!replaceSolid && block(localX, y, localZ) != BlockType::Air) return;
        setBlock(localX, y, localZ, type);
    };
    const int firstCellX = floorDivide(baseX - 2, TreeCellSize);
    const int lastCellX = floorDivide(baseX + Width + 1, TreeCellSize);
    const int firstCellZ = floorDivide(baseZ - 2, TreeCellSize);
    const int lastCellZ = floorDivide(baseZ + Depth + 1, TreeCellSize);
    for (int cellZ = firstCellZ; cellZ <= lastCellZ; ++cellZ) {
        for (int cellX = firstCellX; cellX <= lastCellX; ++cellX) {
            if (randomValue(cellX, 17, cellZ, seed + 49999) < 0.42f) continue;
            const int rootX = cellX * TreeCellSize + 1 + static_cast<int>(
                randomValue(cellX, 29, cellZ, seed + 70001) * 5.0f);
            const int rootZ = cellZ * TreeCellSize + 1 + static_cast<int>(
                randomValue(cellX, 43, cellZ, seed + 90001) * 5.0f);
            const int localRootX = rootX - baseX;
            const int localRootZ = rootZ - baseZ;
            // Keep the complete radius-two canopy in its owning chunk. This avoids
            // rendering a neighbor's leaves before that neighbor has generated the trunk.
            if (localRootX < 2 || localRootX >= Width - 2
                || localRootZ < 2 || localRootZ >= Depth - 2) continue;
            const int surface = terrainHeight(rootX, rootZ, seed);
            const int neighborMinimum = std::min({terrainHeight(rootX - 1, rootZ, seed),
                terrainHeight(rootX + 1, rootZ, seed), terrainHeight(rootX, rootZ - 1, seed),
                terrainHeight(rootX, rootZ + 1, seed)});
            const int neighborMaximum = std::max({terrainHeight(rootX - 1, rootZ, seed),
                terrainHeight(rootX + 1, rootZ, seed), terrainHeight(rootX, rootZ - 1, seed),
                terrainHeight(rootX, rootZ + 1, seed)});
            if (neighborMaximum - neighborMinimum > 2 || surface + 7 >= Height) continue;
            const int trunkHeight = 4 + (randomValue(cellX, 61, cellZ, seed + 110003) > 0.7f ? 1 : 0);
            for (int y = 1; y <= trunkHeight; ++y)
                placeWorldBlock(rootX, surface + y, rootZ, BlockType::Wood, true);
            const int crownY = surface + trunkHeight;
            for (int layer = -1; layer <= 1; ++layer) {
                const int radius = layer == 1 ? 1 : 2;
                for (int dz = -radius; dz <= radius; ++dz) {
                    for (int dx = -radius; dx <= radius; ++dx) {
                        if (radius == 2 && std::abs(dx) == 2 && std::abs(dz) == 2) continue;
                        if (dx == 0 && dz == 0 && layer <= 0) continue;
                        placeWorldBlock(rootX + dx, crownY + layer, rootZ + dz,
                            BlockType::Leaves, false);
                    }
                }
            }
            placeWorldBlock(rootX, crownY + 2, rootZ, BlockType::Leaves, false);
        }
    }
}

bool Chunk::replaceBlocks(std::vector<BlockType> blocks) {
    if (blocks.size() != blocks_.size()) return false;
    blocks_ = std::move(blocks);
    rebuildSectionMetadata();
    return true;
}

bool Chunk::sectionEmpty(int sectionY) const {
    return sectionY < 0 || sectionY >= SectionCount || sectionSolidCounts_[sectionY] == 0;
}

std::uint16_t Chunk::sectionSolidBlocks(int sectionY) const {
    return sectionY < 0 || sectionY >= SectionCount ? 0 : sectionSolidCounts_[sectionY];
}

void Chunk::rebuildSectionMetadata() {
    sectionSolidCounts_.fill(0);
    for (int y = 0; y < Height; ++y) {
        std::uint16_t& count = sectionSolidCounts_[y / SectionHeight];
        for (int z = 0; z < Depth; ++z) {
            for (int x = 0; x < Width; ++x) {
                if (isSolid(blocks_[index(x, y, z)])) ++count;
            }
        }
    }
}

bool Chunk::inBounds(int x, int y, int z) {
    return x >= 0 && x < Width && y >= 0 && y < Height && z >= 0 && z < Depth;
}

std::size_t Chunk::index(int x, int y, int z) {
    return static_cast<std::size_t>(x + Width * (z + Depth * y));
}

} // namespace Engine
