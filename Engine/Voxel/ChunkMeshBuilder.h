#pragma once

#include "Engine/Renderer/Mesh.h"
#include "Engine/Voxel/Chunk.h"

namespace Engine {

struct ChunkMeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct ChunkMeshSnapshot {
    std::vector<BlockType> blocks;
    BlockType block(int x, int y, int z) const;
};

class ChunkMeshBuilder {
public:
    static ChunkMeshData build(const Chunk& chunk, const class VoxelWorld& world);
    static ChunkMeshSnapshot capture(const Chunk& chunk, const class VoxelWorld& world);
    static ChunkMeshData build(const ChunkMeshSnapshot& snapshot);
};

} // namespace Engine
