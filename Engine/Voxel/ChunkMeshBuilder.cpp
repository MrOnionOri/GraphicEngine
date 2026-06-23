#include "Engine/Voxel/ChunkMeshBuilder.h"
#include "Engine/Voxel/VoxelWorld.h"

#include <vector>

namespace Engine {

namespace {
struct FaceCell {
    int tile = 0;
    bool positive = false;
    bool visible = false;

    bool matches(const FaceCell& other) const {
        return visible == other.visible && (!visible
            || (tile == other.tile && positive == other.positive));
    }
};

int faceIndex(int axis, bool positive) {
    constexpr int negativeFaces[3]{2, 4, 1};
    constexpr int positiveFaces[3]{3, 5, 0};
    return positive ? positiveFaces[axis] : negativeFaces[axis];
}
}

namespace {
constexpr int SnapshotWidth = Chunk::Width + 2;
constexpr int SnapshotHeight = Chunk::Height + 2;
constexpr int SnapshotDepth = Chunk::Depth + 2;
}

BlockType ChunkMeshSnapshot::block(int x, int y, int z) const {
    if (x < -1 || x > Chunk::Width || y < -1 || y > Chunk::Height
        || z < -1 || z > Chunk::Depth) return BlockType::Air;
    const std::size_t index = static_cast<std::size_t>((x + 1)
        + SnapshotWidth * ((z + 1) + SnapshotDepth * (y + 1)));
    return blocks[index];
}

ChunkMeshSnapshot ChunkMeshBuilder::capture(const Chunk& chunk, const VoxelWorld& world) {
    ChunkMeshSnapshot snapshot;
    snapshot.blocks.resize(SnapshotWidth * SnapshotHeight * SnapshotDepth, BlockType::Air);
    const auto writeSnapshot = [&](int x, int y, int z, BlockType block) {
                const std::size_t index = static_cast<std::size_t>((x + 1)
                    + SnapshotWidth * ((z + 1) + SnapshotDepth * (y + 1)));
        snapshot.blocks[index] = block;
    };

    // Empty 16-block sections remain initialized as air and cost no per-block copies.
    for (int sectionY = 0; sectionY < Chunk::SectionCount; ++sectionY) {
        if (chunk.sectionEmpty(sectionY)) continue;
        const int firstY = sectionY * Chunk::SectionHeight;
        for (int y = firstY; y < firstY + Chunk::SectionHeight; ++y) {
            for (int z = 0; z < Chunk::Depth; ++z) {
                for (int x = 0; x < Chunk::Width; ++x) {
                    writeSnapshot(x, y, z, chunk.block(x, y, z));
                }
            }
        }
    }

    // Only the four neighboring boundary planes are needed to remove faces shared
    // between chunks. Corner samples never participate in a face comparison.
    const int baseX = chunk.chunkX() * Chunk::Width;
    const int baseZ = chunk.chunkZ() * Chunk::Depth;
    for (int y = 0; y < Chunk::Height; ++y) {
        for (int z = 0; z < Chunk::Depth; ++z) {
            writeSnapshot(-1, y, z, world.block(baseX - 1, y, baseZ + z));
            writeSnapshot(Chunk::Width, y, z, world.block(baseX + Chunk::Width, y, baseZ + z));
        }
        for (int x = 0; x < Chunk::Width; ++x) {
            writeSnapshot(x, y, -1, world.block(baseX + x, y, baseZ - 1));
            writeSnapshot(x, y, Chunk::Depth, world.block(baseX + x, y, baseZ + Chunk::Depth));
        }
    }
    return snapshot;
}

ChunkMeshData ChunkMeshBuilder::build(const Chunk& chunk, const VoxelWorld& world) {
    return build(capture(chunk, world));
}

ChunkMeshData ChunkMeshBuilder::build(const ChunkMeshSnapshot& snapshot) {
    ChunkMeshData result;
    result.vertices.reserve(2048);
    result.indices.reserve(3072);
    constexpr int dimensions[3]{Chunk::Width, Chunk::Height, Chunk::Depth};

    for (int axis = 0; axis < 3; ++axis) {
        const int u = (axis + 1) % 3;
        const int v = (axis + 2) % 3;
        glm::ivec3 position{0};
        glm::ivec3 neighborOffset{0};
        neighborOffset[axis] = 1;
        std::vector<FaceCell> mask(static_cast<std::size_t>(dimensions[u] * dimensions[v]));

        for (position[axis] = -1; position[axis] < dimensions[axis];) {
            int maskIndex = 0;
            for (position[v] = 0; position[v] < dimensions[v]; ++position[v]) {
                for (position[u] = 0; position[u] < dimensions[u]; ++position[u]) {
                    const BlockType first = snapshot.block(position.x, position.y, position.z);
                    const glm::ivec3 adjacent = position + neighborOffset;
                    const BlockType second = snapshot.block(adjacent.x, adjacent.y, adjacent.z);
                    const bool firstSolid = isSolid(first);
                    const bool secondSolid = isSolid(second);
                    FaceCell& cell = mask[maskIndex++];
                    cell.visible = firstSolid != secondSolid;
                    if (cell.visible) {
                        cell.positive = firstSolid;
                        const BlockType surfaceBlock = firstSolid ? first : second;
                        cell.tile = blockTextureTile(surfaceBlock, faceIndex(axis, cell.positive));
                    }
                }
            }

            ++position[axis];
            maskIndex = 0;
            for (int j = 0; j < dimensions[v]; ++j) {
                for (int i = 0; i < dimensions[u];) {
                    const FaceCell cell = mask[maskIndex];
                    if (!cell.visible) {
                        ++i;
                        ++maskIndex;
                        continue;
                    }

                    int width = 1;
                    while (i + width < dimensions[u]
                        && mask[maskIndex + width].matches(cell)) ++width;
                    int height = 1;
                    bool canGrow = true;
                    while (j + height < dimensions[v] && canGrow) {
                        for (int offset = 0; offset < width; ++offset) {
                            if (!mask[maskIndex + offset + height * dimensions[u]].matches(cell)) {
                                canGrow = false;
                                break;
                            }
                        }
                        if (canGrow) ++height;
                    }

                    position[u] = i;
                    position[v] = j;
                    glm::ivec3 du{0};
                    glm::ivec3 dv{0};
                    du[u] = width;
                    dv[v] = height;
                    glm::vec3 normal{0.0f};
                    normal[axis] = cell.positive ? 1.0f : -1.0f;
                    const glm::vec3 origin = glm::vec3(position);
                    const glm::vec3 uVector = glm::vec3(du);
                    const glm::vec3 vVector = glm::vec3(dv);
                    const float tile = static_cast<float>(cell.tile);
                    const unsigned int firstVertex = static_cast<unsigned int>(result.vertices.size());
                    glm::vec2 uvOrigin{0.0f, 0.0f};
                    glm::vec2 uvAlongU{static_cast<float>(width), 0.0f};
                    glm::vec2 uvAlongV{0.0f, static_cast<float>(height)};
                    if (axis == 0) {
                        // On X-facing walls, U is world Y and V is world Z. Swap
                        // texture axes so grass remains above dirt instead of sideways.
                        uvAlongU = {0.0f, static_cast<float>(width)};
                        uvAlongV = {static_cast<float>(height), 0.0f};
                    }

                    if (cell.positive) {
                        result.vertices.push_back({origin, normal, uvOrigin, tile});
                        result.vertices.push_back({origin + uVector, normal, uvAlongU, tile});
                        result.vertices.push_back({origin + uVector + vVector, normal,
                            uvAlongU + uvAlongV, tile});
                        result.vertices.push_back({origin + vVector, normal, uvAlongV, tile});
                    } else {
                        result.vertices.push_back({origin, normal, uvOrigin, tile});
                        result.vertices.push_back({origin + vVector, normal, uvAlongV, tile});
                        result.vertices.push_back({origin + uVector + vVector, normal,
                            uvAlongU + uvAlongV, tile});
                        result.vertices.push_back({origin + uVector, normal, uvAlongU, tile});
                    }
                    result.indices.insert(result.indices.end(), {firstVertex, firstVertex + 1,
                        firstVertex + 2, firstVertex + 2, firstVertex + 3, firstVertex});

                    for (int clearY = 0; clearY < height; ++clearY) {
                        for (int clearX = 0; clearX < width; ++clearX) {
                            mask[maskIndex + clearX + clearY * dimensions[u]].visible = false;
                        }
                    }
                    i += width;
                    maskIndex += width;
                }
            }
        }
    }
    return result;
}

} // namespace Engine
