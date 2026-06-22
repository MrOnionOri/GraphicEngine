#pragma once

#include <cstdint>

namespace Engine {

enum class BlockType : std::uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Wood,
    Leaves
};

inline bool isSolid(BlockType block) { return block != BlockType::Air; }

} // namespace Engine
