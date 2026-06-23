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
constexpr int StickItemId = FirstToolItemId + 2;
constexpr int StonePickaxeItemId = FirstToolItemId + 3;
constexpr int IronIngotItemId = FirstToolItemId + 4;

struct InventorySlot {
    int itemId = 0;
    int count = 0;
    int durability = 0;

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
inline InventorySlot blockDrop(BlockType block) {
    if (block == BlockType::Stone) return {blockItemId(BlockType::Cobblestone), 1, 0};
    if (block == BlockType::Bedrock) return {};
    return {blockItemId(block), 1, 0};
}
inline BlockType itemBlockType(int itemId) {
    const BlockType block = static_cast<BlockType>(itemId);
    return isValidBlockType(block) ? block : BlockType::Air;
}
inline bool itemIsPlaceableBlock(int itemId) {
    const BlockType block = itemBlockType(itemId);
    return block != BlockType::Air && placeableBlockSlot(block) >= 0;
}
inline const char* itemName(int itemId) {
    if (itemId == StickItemId) return "Stick";
    if (itemId == IronIngotItemId) return "Iron Ingot";
    if (itemId == WoodPickaxeItemId) return "Wood Pickaxe";
    if (itemId == StonePickaxeItemId) return "Stone Pickaxe";
    return blockName(itemBlockType(itemId));
}
inline BlockColor itemColor(int itemId) {
    if (itemId == StickItemId) return {196, 142, 76, 255};
    if (itemId == IronIngotItemId) return {214, 203, 184, 255};
    if (itemId == WoodPickaxeItemId) return {178, 132, 72, 255};
    if (itemId == StonePickaxeItemId) return {130, 130, 130, 255};
    return blockHotbarColor(itemBlockType(itemId));
}
inline int itemMaxStack(int itemId) {
    if (itemId == StickItemId || itemId == IronIngotItemId) return InventoryMaxStack;
    return itemId >= FirstToolItemId ? 1 : InventoryMaxStack;
}
inline int itemMaxDurability(int itemId) {
    if (itemId == WoodPickaxeItemId) return 59;
    if (itemId == StonePickaxeItemId) return 131;
    return 0;
}
inline bool itemUsesDurability(int itemId) { return itemMaxDurability(itemId) > 0; }

inline void normalizeInventorySlot(InventorySlot& slot) {
    const bool validBlock = itemIsPlaceableBlock(slot.itemId);
    const bool validCraftingItem = slot.itemId == StickItemId || slot.itemId == IronIngotItemId;
    const bool validTool = slot.itemId == WoodPickaxeItemId || slot.itemId == StonePickaxeItemId;
    if (slot.count <= 0 || (!validBlock && !validCraftingItem && !validTool)) {
        slot.itemId = 0;
        slot.count = 0;
        return;
    }
    slot.count = std::clamp(slot.count, 1, itemMaxStack(slot.itemId));
    if (itemUsesDurability(slot.itemId)) {
        if (slot.durability <= 0) slot.durability = itemMaxDurability(slot.itemId);
        slot.durability = std::clamp(slot.durability, 1, itemMaxDurability(slot.itemId));
    } else {
        slot.durability = 0;
    }
}

inline InventorySlot makeItemStack(int itemId, int count = 1) {
    InventorySlot slot{itemId, count, itemMaxDurability(itemId)};
    normalizeInventorySlot(slot);
    return slot;
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

inline bool slotHasItem(const InventorySlot& slot, int itemId) {
    return slot.itemId == itemId && slot.count > 0;
}

inline bool slotIsEmpty(const InventorySlot& slot) {
    return slot.empty();
}

struct CraftingRecipe {
    const char* name = "";
    int width = 0;
    int height = 0;
    std::array<int, CraftingTableGridSlots> pattern{};
    InventorySlot output{};
    bool requiresTable = false;
};

struct CraftingMatch {
    const CraftingRecipe* recipe = nullptr;
    int offsetX = 0;
    int offsetY = 0;
};

inline const std::array<CraftingRecipe, 6>& craftingRecipes() {
    static const std::array<CraftingRecipe, 6> recipes{{
        {"Planks", 1, 1,
            {blockItemId(BlockType::Wood), 0, 0, 0, 0, 0, 0, 0, 0},
            {blockItemId(BlockType::Planks), 4, 0}, false},
        {"Sticks", 1, 2,
            {blockItemId(BlockType::Planks), blockItemId(BlockType::Planks), 0, 0, 0, 0, 0, 0, 0},
            {StickItemId, 4, 0}, false},
        {"Crafting Table", 2, 2,
            {blockItemId(BlockType::Planks), blockItemId(BlockType::Planks),
             blockItemId(BlockType::Planks), blockItemId(BlockType::Planks), 0, 0, 0, 0, 0},
            {blockItemId(BlockType::CraftingTable), 1, 0}, false},
        {"Wood Pickaxe", 3, 3,
            {blockItemId(BlockType::Planks), blockItemId(BlockType::Planks), blockItemId(BlockType::Planks),
             0, StickItemId, 0,
             0, StickItemId, 0},
            {WoodPickaxeItemId, 1, itemMaxDurability(WoodPickaxeItemId)}, true},
        {"Stone Pickaxe", 3, 3,
            {blockItemId(BlockType::Cobblestone), blockItemId(BlockType::Cobblestone), blockItemId(BlockType::Cobblestone),
             0, StickItemId, 0,
             0, StickItemId, 0},
            {StonePickaxeItemId, 1, itemMaxDurability(StonePickaxeItemId)}, true},
        {"Furnace", 3, 3,
            {blockItemId(BlockType::Cobblestone), blockItemId(BlockType::Cobblestone), blockItemId(BlockType::Cobblestone),
             blockItemId(BlockType::Cobblestone), 0, blockItemId(BlockType::Cobblestone),
             blockItemId(BlockType::Cobblestone), blockItemId(BlockType::Cobblestone), blockItemId(BlockType::Cobblestone)},
            {blockItemId(BlockType::Furnace), 1, 0}, true},
    }};
    return recipes;
}

struct SmeltingRecipe {
    InventorySlot output{};
    float seconds = 0.0f;
};

inline SmeltingRecipe smeltingRecipe(int inputItemId) {
    if (inputItemId == blockItemId(BlockType::IronOre))
        return {{IronIngotItemId, 1, 0}, 5.0f};
    if (inputItemId == blockItemId(BlockType::Sand))
        return {{blockItemId(BlockType::Glass), 1, 0}, 4.0f};
    return {};
}

inline float furnaceFuelSeconds(int itemId) {
    if (itemId == blockItemId(BlockType::CoalOre)) return 16.0f;
    if (itemId == blockItemId(BlockType::Wood)) return 3.0f;
    if (itemId == blockItemId(BlockType::Planks)) return 2.0f;
    return 0.0f;
}

inline bool canSmeltIntoOutput(const InventorySlot& input, const InventorySlot& output) {
    if (input.empty()) return false;
    const SmeltingRecipe recipe = smeltingRecipe(input.itemId);
    if (recipe.output.empty()) return false;
    return output.empty()
        || (output.itemId == recipe.output.itemId
            && output.count + recipe.output.count <= itemMaxStack(output.itemId));
}

inline bool addSmeltingOutput(InventorySlot& destination, const InventorySlot& output) {
    if (output.empty()) return false;
    if (destination.empty()) {
        destination = output;
        normalizeInventorySlot(destination);
        return true;
    }
    if (destination.itemId != output.itemId) return false;
    if (destination.count + output.count > itemMaxStack(destination.itemId)) return false;
    destination.count += output.count;
    normalizeInventorySlot(destination);
    return true;
}

template <std::size_t SlotCount>
inline bool recipeMatchesAt(const std::array<InventorySlot, SlotCount>& slots,
    int gridWidth, int gridHeight, const CraftingRecipe& recipe, int offsetX, int offsetY) {
    if (offsetX + recipe.width > gridWidth || offsetY + recipe.height > gridHeight) return false;
    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            const int gridIndex = y * gridWidth + x;
            const bool inside = x >= offsetX && y >= offsetY
                && x < offsetX + recipe.width && y < offsetY + recipe.height;
            const int expected = inside
                ? recipe.pattern[(y - offsetY) * recipe.width + (x - offsetX)]
                : 0;
            if (expected == 0) {
                if (!slots[gridIndex].empty()) return false;
            } else if (!slotHasItem(slots[gridIndex], expected)) {
                return false;
            }
        }
    }
    return true;
}

template <std::size_t SlotCount>
inline CraftingMatch findCraftingMatch(const std::array<InventorySlot, SlotCount>& slots,
    int gridWidth, int gridHeight, bool tableOpen) {
    for (const CraftingRecipe& recipe : craftingRecipes()) {
        if (recipe.requiresTable && !tableOpen) continue;
        if (recipe.width > gridWidth || recipe.height > gridHeight) continue;
        for (int y = 0; y <= gridHeight - recipe.height; ++y) {
            for (int x = 0; x <= gridWidth - recipe.width; ++x) {
                if (recipeMatchesAt(slots, gridWidth, gridHeight, recipe, x, y))
                    return CraftingMatch{&recipe, x, y};
            }
        }
    }
    return {};
}

inline CraftingResult craftingResult(const CraftingGrid& grid) {
    const CraftingMatch match = findCraftingMatch(grid.slots, 2, 2, false);
    return match.recipe ? CraftingResult{match.recipe->output, true} : CraftingResult{};
}

inline CraftingResult craftingTableResult(const CraftingTableGrid& grid) {
    const CraftingMatch match = findCraftingMatch(grid.slots, 3, 3, true);
    return match.recipe ? CraftingResult{match.recipe->output, true} : CraftingResult{};
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

inline bool addToInventory(Inventory& inventory, const InventorySlot& source) {
    if (source.empty()) return false;
    InventorySlot item = source;
    normalizeInventorySlot(item);
    if (item.empty()) return false;
    if (itemMaxStack(item.itemId) == 1) {
        for (InventorySlot& slot : inventory.slots) {
            if (!slot.empty()) continue;
            slot = item;
            return true;
        }
        return false;
    }
    return addToInventory(inventory, item.itemId, item.count);
}

inline bool addToInventory(Inventory& inventory, BlockType block, int count = 1) {
    return addToInventory(inventory, blockItemId(block), count);
}

inline bool canMergeIntoSlot(const InventorySlot& destination, const InventorySlot& source) {
    return !source.empty()
        && (destination.empty()
            || (destination.itemId == source.itemId
                && destination.count < itemMaxStack(destination.itemId)));
}

inline bool addToSlot(InventorySlot& destination, int itemId, int count) {
    if (itemId == 0 || count <= 0) return false;
    InventorySlot probe{itemId, count};
    normalizeInventorySlot(probe);
    if (probe.empty()) return false;
    if (destination.empty()) {
        const int added = std::min(count, itemMaxStack(itemId));
        destination = InventorySlot{itemId, added};
        return added == count;
    }
    if (destination.itemId != itemId) return false;
    const int added = std::min(count, itemMaxStack(itemId) - destination.count);
    destination.count += added;
    normalizeInventorySlot(destination);
    return added == count;
}

inline bool moveAllToSlot(InventorySlot& destination, InventorySlot& source) {
    if (source.empty()) return false;
    if (destination.empty()) {
        destination = source;
        source = {};
        return true;
    }
    if (destination.itemId != source.itemId) {
        std::swap(destination, source);
        normalizeInventorySlot(destination);
        normalizeInventorySlot(source);
        return true;
    }
    const int added = std::min(source.count, itemMaxStack(source.itemId) - destination.count);
    if (added <= 0) return false;
    destination.count += added;
    source.count -= added;
    normalizeInventorySlot(destination);
    normalizeInventorySlot(source);
    return true;
}

inline bool moveOneToSlot(InventorySlot& destination, InventorySlot& source) {
    if (source.empty()) return false;
    if (destination.empty()) {
        destination = source;
        destination.count = 1;
        --source.count;
        if (source.count <= 0) source = {};
        normalizeInventorySlot(source);
        normalizeInventorySlot(destination);
        return true;
    }
    if (destination.itemId != source.itemId) return false;
    if (destination.count >= itemMaxStack(destination.itemId)) return false;
    ++destination.count;
    --source.count;
    normalizeInventorySlot(destination);
    normalizeInventorySlot(source);
    return true;
}

inline bool takeHalfFromSlot(InventorySlot& source, InventorySlot& cursor) {
    if (!cursor.empty() || source.empty()) return false;
    const int taken = (source.count + 1) / 2;
    cursor = source;
    cursor.count = taken;
    source.count -= taken;
    normalizeInventorySlot(source);
    normalizeInventorySlot(cursor);
    return true;
}

inline bool addCraftingOutputToCursor(InventorySlot& cursor, const InventorySlot& output) {
    if (output.empty()) return false;
    if (cursor.empty()) {
        cursor = output;
        normalizeInventorySlot(cursor);
        return true;
    }
    if (cursor.itemId != output.itemId) return false;
    if (cursor.count + output.count > itemMaxStack(cursor.itemId)) return false;
    cursor.count += output.count;
    normalizeInventorySlot(cursor);
    return true;
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

template <std::size_t SlotCount>
inline bool consumeCraftingMatch(std::array<InventorySlot, SlotCount>& slots,
    int gridWidth, int gridHeight, bool tableOpen) {
    const CraftingMatch match = findCraftingMatch(slots, gridWidth, gridHeight, tableOpen);
    if (!match.recipe) return false;
    bool consumed = false;
    for (int y = 0; y < match.recipe->height; ++y) {
        for (int x = 0; x < match.recipe->width; ++x) {
            const int expected = match.recipe->pattern[y * match.recipe->width + x];
            if (expected == 0) continue;
            InventorySlot& slot = slots[(match.offsetY + y) * gridWidth + match.offsetX + x];
            if (!slotHasItem(slot, expected)) return false;
            --slot.count;
            normalizeInventorySlot(slot);
            consumed = true;
        }
    }
    return consumed;
}

inline bool consumeCraftingRecipe(CraftingGrid& grid) {
    return consumeCraftingMatch(grid.slots, 2, 2, false);
}

inline bool consumeCraftingTableRecipe(CraftingTableGrid& grid) {
    return consumeCraftingMatch(grid.slots, 3, 3, true);
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

inline bool damageInventorySlot(Inventory& inventory, int slotIndex, int amount = 1) {
    if (slotIndex < 0 || slotIndex >= InventorySlots || amount <= 0) return false;
    InventorySlot& slot = inventory.slots[slotIndex];
    if (slot.empty() || !itemUsesDurability(slot.itemId)) return false;
    slot.durability -= amount;
    if (slot.durability <= 0) {
        slot = {};
        return true;
    }
    normalizeInventorySlot(slot);
    return false;
}

inline int selectedHotbarPickaxeTier(const Inventory& inventory, int hotbarSlot) {
    if (hotbarSlot < 0 || hotbarSlot >= InventoryHotbarSlots) return 0;
    const InventorySlot& slot = inventory.slots[hotbarSlot];
    if (slot.empty()) return 0;
    if (slot.itemId == WoodPickaxeItemId) return 1;
    if (slot.itemId == StonePickaxeItemId) return 2;
    return 0;
}

} // namespace Engine
