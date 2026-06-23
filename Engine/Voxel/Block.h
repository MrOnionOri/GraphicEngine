#pragma once

#include <cstdint>

namespace Engine {

enum class BlockType : std::uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Wood,
    Leaves,
    Sand,
    CoalOre,
    Planks,
    CraftingTable,
    Cobblestone,
    Gravel,
    Bedrock,
    Glass,
    Bricks,
    IronOre,
    Furnace
};

struct BlockColor {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

struct BlockDefinition {
    const char* name = "Unknown";
    bool solid = false;
    float hardnessSeconds = 0.0f;
    BlockColor hotbarColor{};
    int faceTiles[6]{0, 0, 0, 0, 0, 0};
};

// Face index convention used by the chunk mesher:
// 0/1 = Z faces, 2/3 = X faces, 4 = bottom, 5 = top.
inline const BlockDefinition& blockDefinition(BlockType block) {
    static constexpr BlockDefinition definitions[]{
        {"Air", false, 0.0f, {0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}},
        {"Grass", true, 0.45f, {74, 185, 55, 255}, {3, 3, 3, 3, 1, 0}},
        {"Dirt", true, 0.55f, {126, 78, 42, 255}, {1, 1, 1, 1, 1, 1}},
        {"Stone", true, 1.35f, {125, 129, 132, 255}, {2, 2, 2, 2, 2, 2}},
        {"Wood", true, 1.05f, {118, 73, 36, 255}, {4, 4, 4, 4, 4, 4}},
        {"Leaves", true, 0.25f, {52, 142, 42, 255}, {5, 5, 5, 5, 5, 5}},
        {"Sand", true, 0.45f, {210, 190, 112, 255}, {6, 6, 6, 6, 6, 6}},
        {"Coal Ore", true, 1.80f, {72, 72, 72, 255}, {7, 7, 7, 7, 7, 7}},
        {"Planks", true, 0.85f, {154, 105, 54, 255}, {8, 8, 8, 8, 8, 8}},
        {"Crafting Table", true, 0.95f, {128, 82, 42, 255}, {9, 9, 9, 9, 9, 9}},
        {"Cobblestone", true, 1.15f, {105, 108, 110, 255}, {10, 10, 10, 10, 10, 10}},
        {"Gravel", true, 0.65f, {117, 112, 108, 255}, {11, 11, 11, 11, 11, 11}},
        {"Bedrock", true, 9999.0f, {54, 54, 58, 255}, {12, 12, 12, 12, 12, 12}},
        {"Glass", true, 0.35f, {155, 205, 225, 170}, {13, 13, 13, 13, 13, 13}},
        {"Bricks", true, 1.40f, {148, 67, 54, 255}, {14, 14, 14, 14, 14, 14}},
        {"Iron Ore", true, 1.90f, {143, 120, 98, 255}, {15, 15, 15, 15, 15, 15}},
        {"Furnace", true, 1.25f, {92, 88, 84, 255}, {16, 16, 16, 16, 16, 16}},
    };

    const auto index = static_cast<std::uint8_t>(block);
    if (index >= sizeof(definitions) / sizeof(definitions[0])) return definitions[0];
    return definitions[index];
}

inline constexpr int blockAtlasTileCount() { return 17; }
inline constexpr int placeableBlockCount() { return 16; }

inline BlockType placeableBlock(int slot) {
    static constexpr BlockType blocks[placeableBlockCount()]{
        BlockType::Grass, BlockType::Dirt, BlockType::Stone, BlockType::Wood,
        BlockType::Leaves, BlockType::Sand, BlockType::CoalOre, BlockType::Planks,
        BlockType::CraftingTable, BlockType::Cobblestone, BlockType::Gravel,
        BlockType::Bedrock, BlockType::Glass, BlockType::Bricks, BlockType::IronOre,
        BlockType::Furnace};
    if (slot < 0 || slot >= placeableBlockCount()) return blocks[0];
    return blocks[slot];
}

inline int placeableBlockSlot(BlockType block) {
    for (int slot = 0; slot < placeableBlockCount(); ++slot) {
        if (placeableBlock(slot) == block) return slot;
    }
    return -1;
}

inline bool isValidBlockType(BlockType block) {
    return static_cast<std::uint8_t>(block) < static_cast<std::uint8_t>(BlockType::Furnace) + 1;
}

inline bool isSolid(BlockType block) { return blockDefinition(block).solid; }
inline const char* blockName(BlockType block) { return blockDefinition(block).name; }
inline float blockHardnessSeconds(BlockType block) { return blockDefinition(block).hardnessSeconds; }
inline BlockColor blockHotbarColor(BlockType block) { return blockDefinition(block).hotbarColor; }

inline int blockTextureTile(BlockType block, int faceIndex) {
    if (faceIndex < 0 || faceIndex >= 6) return 0;
    return blockDefinition(block).faceTiles[faceIndex];
}

} // namespace Engine
