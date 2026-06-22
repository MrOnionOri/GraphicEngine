#pragma once

#include "Engine/Voxel/Block.h"
#include <array>
#include <cstdint>
#include <vector>

namespace Engine {

class Chunk {
public:
    static constexpr int Width = 16;
    static constexpr int Height = 32;
    static constexpr int Depth = 16;
    static constexpr int SectionHeight = 16;
    static constexpr int SectionCount = Height / SectionHeight;

    explicit Chunk(int seed = 1337, int chunkX = 0, int chunkZ = 0);
    BlockType block(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType block);
    void generate(int seed);
    int chunkX() const { return chunkX_; }
    int chunkZ() const { return chunkZ_; }
    const std::vector<BlockType>& blocks() const { return blocks_; }
    bool replaceBlocks(std::vector<BlockType> blocks);
    bool sectionEmpty(int sectionY) const;
    std::uint16_t sectionSolidBlocks(int sectionY) const;

private:
    std::vector<BlockType> blocks_;
    std::array<std::uint16_t, SectionCount> sectionSolidCounts_{};
    int chunkX_ = 0;
    int chunkZ_ = 0;
    static bool inBounds(int x, int y, int z);
    static std::size_t index(int x, int y, int z);
    void rebuildSectionMetadata();
};

} // namespace Engine
