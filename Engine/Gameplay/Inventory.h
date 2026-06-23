#pragma once

#include "Engine/Voxel/Block.h"

#include <array>
#include <algorithm>

namespace Engine {

constexpr int InventoryHotbarSlots = 9;
constexpr int InventoryBackpackSlots = 27;
constexpr int InventorySlots = InventoryHotbarSlots + InventoryBackpackSlots;
constexpr int InventoryMaxStack = 64;
constexpr int CraftingGridSlots = 4;
constexpr int CraftingTableGridSlots = 9;
constexpr int FirstToolItemId = 1000;
constexpr int WoodPickaxeItemId = FirstToolItemId + 1;

struct InventorySlot {
    int itemId = 0;
    int count = 0;

    bool empty() const { return count <= 0 || itemId == 0; }
};

struct Inventory {
    std::array<InventorySlot, InventorySlots> slots{};
};

struct CraftingGrid {
    std::array<InventorySlot, CraftingGridSlots> slots{};
};

struct CraftingTableGrid {
    std::array<InventorySlot, CraftingTableGridSlots> slots{};
};

struct CraftingResult {
    InventorySlot output{};
    bool valid = false;
};

inline int blockItemId(BlockType block) { return static_cast<int>(block); }
inline BlockType itemBlockType(int itemId) {
    const BlockType block = static_cast<BlockType>(itemId);
    return isValidBlockType(block) ? block : BlockType::Air;
}
inline bool itemIsPlaceableBlock(int itemId) {
    const BlockType block = itemBlockType(itemId);
    return block != BlockType::Air && placeableBlockSlot(block) >= 0;
}
inline const char* itemName(int itemId) {
    if (itemId == WoodPickaxeItemId) return "Wood Pickaxe";
    return blockName(itemBlockType(itemId));
}
inline BlockColor itemColor(int itemId) {
    if (itemId == WoodPickaxeItemId) return {178, 132, 72, 255};
    return blockHotbarColor(itemBlockType(itemId));
}
inline int itemMaxStack(int itemId) {
    return itemId >= FirstToolItemId ? 1 : InventoryMaxStack;
}

inline void normalizeInventorySlot(InventorySlot& slot) {
    const bool validBlock = itemIsPlaceableBlock(slot.itemId);
    const bool validTool = slot.itemId == WoodPickaxeItemId;
    if (slot.count <= 0 || (!validBlock && !validTool)) {
        slot.itemId = 0;
        slot.count = 0;
        return;
    }
    slot.count = std::clamp(slot.count, 1, itemMaxStack(slot.itemId));
}

inline void normalizeInventory(Inventory& inventory) {
    for (InventorySlot& slot : inventory.slots) normalizeInventorySlot(slot);
}

inline Inventory createStarterInventory() {
    Inventory inventory;
    for (int slot = 0; slot < placeableBlockCount(); ++slot) {
        inventory.slots[slot] = InventorySlot{blockItemId(placeableBlock(slot)), 32};
    }
    return inventory;
}

inline CraftingResult craftingResult(const CraftingGrid& grid) {
    int woodCount = 0;
    int plankCount = 0;
    int nonEmptyCount = 0;
    for (const InventorySlot& slot : grid.slots) {
        if (slot.empty()) continue;
        ++nonEmptyCount;
        if (slot.itemId == blockItemId(BlockType::Wood) && slot.count == 1) ++woodCount;
        if (slot.itemId == blockItemId(BlockType::Planks) && slot.count == 1) ++plankCount;
    }
    if (nonEmptyCount == 1 && woodCount == 1)
        return {InventorySlot{blockItemId(BlockType::Planks), 4}, true};
    if (nonEmptyCount == 4 && plankCount == 4)
        return {InventorySlot{blockItemId(BlockType::CraftingTable), 1}, true};
    return {};
}

inline CraftingResult craftingTableResult(const CraftingTableGrid& grid) {
    int plankCount = 0;
    int nonEmptyCount = 0;
    for (const InventorySlot& slot : grid.slots) {
        if (slot.empty()) continue;
        ++nonEmptyCount;
        if (slot.itemId == blockItemId(BlockType::Planks) && slot.count == 1) ++plankCount;
    }
    if (nonEmptyCount == 4 && plankCount == 4)
        return {InventorySlot{WoodPickaxeItemId, 1}, true};
    return {};
}

inline bool addToInventory(Inventory& inventory, int itemId, int count = 1) {
    if (itemId == 0 || count <= 0) return false;
    InventorySlot probe{itemId, count};
    normalizeInventorySlot(probe);
    if (probe.empty()) return false;
    int remaining = count;
    for (InventorySlot& slot : inventory.slots) {
        if (slot.itemId != itemId || slot.count >= itemMaxStack(itemId)) continue;
        const int added = std::min(remaining, itemMaxStack(itemId) - slot.count);
        slot.count += added;
        remaining -= added;
        if (remaining == 0) return true;
    }
    for (InventorySlot& slot : inventory.slots) {
        if (!slot.empty()) continue;
        const int added = std::min(remaining, itemMaxStack(itemId));
        slot.itemId = itemId;
        slot.count = added;
        remaining -= added;
        if (remaining == 0) return true;
    }
    return false;
}

inline bool addToInventory(Inventory& inventory, BlockType block, int count = 1) {
    return addToInventory(inventory, blockItemId(block), count);
}

inline bool removeOneFromSlot(Inventory& inventory, int slotIndex) {
    if (slotIndex < 0 || slotIndex >= InventorySlots) return false;
    InventorySlot& slot = inventory.slots[slotIndex];
    if (slot.empty()) return false;
    --slot.count;
    normalizeInventorySlot(slot);
    return true;
}

inline bool removeOneFromCraftingGrid(CraftingGrid& grid, int itemId) {
    for (InventorySlot& slot : grid.slots) {
        if (slot.itemId != itemId || slot.count <= 0) continue;
        --slot.count;
        normalizeInventorySlot(slot);
        return true;
    }
    return false;
}

inline bool removeOneFromCraftingTableGrid(CraftingTableGrid& grid, int itemId) {
    for (InventorySlot& slot : grid.slots) {
        if (slot.itemId != itemId || slot.count <= 0) continue;
        --slot.count;
        normalizeInventorySlot(slot);
        return true;
    }
    return false;
}

inline BlockType selectedHotbarBlock(const Inventory& inventory, int hotbarSlot) {
    if (hotbarSlot < 0 || hotbarSlot >= InventoryHotbarSlots) return BlockType::Air;
    const InventorySlot& slot = inventory.slots[hotbarSlot];
    return slot.empty() ? BlockType::Air : itemBlockType(slot.itemId);
}

inline bool selectedHotbarHasWoodPickaxe(const Inventory& inventory, int hotbarSlot) {
    if (hotbarSlot < 0 || hotbarSlot >= InventoryHotbarSlots) return false;
    return inventory.slots[hotbarSlot].itemId == WoodPickaxeItemId
        && !inventory.slots[hotbarSlot].empty();
}

} // namespace Engine
