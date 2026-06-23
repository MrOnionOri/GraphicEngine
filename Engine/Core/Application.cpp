#include "Engine/Core/Application.h"
#include "Engine/Core/AssetPath.h"
#include "Engine/Core/Input.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>

namespace Engine {

namespace {
struct PlayerSaveData {
    bool valid = false;
    glm::vec3 feetPosition{0.0f};
    Inventory inventory = createStarterInventory();
    CraftingGrid craftingGrid{};
    CraftingTableGrid craftingTableGrid{};
    int selectedHotbarSlot = 1;
};

std::optional<int> primaryTerrainSeed(const Scene& scene) {
    for (const Entity& entity : scene.entities()) {
        if (entity.hasVoxelTerrain()) return entity.voxelTerrain().seed;
    }
    return std::nullopt;
}

std::filesystem::path playerSavePath(int seed) {
    return AssetPath::executableDirectory() / "Saves"
        / ("Player_" + std::to_string(seed) + ".txt");
}

PlayerSaveData loadPlayerSave(int seed) {
    PlayerSaveData data;
    std::ifstream stream(playerSavePath(seed));
    std::string header;
    int selectedSlot = 1;
    if (!(stream >> header)) return data;
    if (header == "GEPLAYER3") {
        if (!(stream >> data.feetPosition.x >> data.feetPosition.y >> data.feetPosition.z
            >> selectedSlot)) return data;
        for (int slot = 0; slot < InventorySlots; ++slot) {
            int itemId = 0;
            int count = 0;
            int durability = 0;
            if (!(stream >> itemId >> count >> durability)) return data;
            data.inventory.slots[slot] = InventorySlot{itemId, count, durability};
        }
        for (int slot = 0; slot < CraftingGridSlots; ++slot) {
            int itemId = 0;
            int count = 0;
            int durability = 0;
            if (!(stream >> itemId >> count >> durability)) break;
            data.craftingGrid.slots[slot] = InventorySlot{itemId, count, durability};
        }
        for (int slot = 0; slot < CraftingTableGridSlots; ++slot) {
            int itemId = 0;
            int count = 0;
            int durability = 0;
            if (!(stream >> itemId >> count >> durability)) break;
            data.craftingTableGrid.slots[slot] = InventorySlot{itemId, count, durability};
        }
        normalizeInventory(data.inventory);
        for (InventorySlot& slot : data.craftingGrid.slots) normalizeInventorySlot(slot);
        for (InventorySlot& slot : data.craftingTableGrid.slots) normalizeInventorySlot(slot);
        data.selectedHotbarSlot = std::clamp(selectedSlot, 0, InventoryHotbarSlots - 1);
        data.valid = true;
        return data;
    }
    if (header == "GEPLAYER2") {
        if (!(stream >> data.feetPosition.x >> data.feetPosition.y >> data.feetPosition.z
            >> selectedSlot)) return data;
        for (int slot = 0; slot < InventorySlots; ++slot) {
            int itemId = 0;
            int count = 0;
            if (!(stream >> itemId >> count)) return data;
            data.inventory.slots[slot] = InventorySlot{itemId, count, 0};
        }
        for (int slot = 0; slot < CraftingGridSlots; ++slot) {
            int itemId = 0;
            int count = 0;
            if (!(stream >> itemId >> count)) break;
            data.craftingGrid.slots[slot] = InventorySlot{itemId, count, 0};
        }
        for (int slot = 0; slot < CraftingTableGridSlots; ++slot) {
            int itemId = 0;
            int count = 0;
            if (!(stream >> itemId >> count)) break;
            data.craftingTableGrid.slots[slot] = InventorySlot{itemId, count, 0};
        }
        normalizeInventory(data.inventory);
        for (InventorySlot& slot : data.craftingGrid.slots) normalizeInventorySlot(slot);
        for (InventorySlot& slot : data.craftingTableGrid.slots) normalizeInventorySlot(slot);
        data.selectedHotbarSlot = std::clamp(selectedSlot, 0, InventoryHotbarSlots - 1);
        data.valid = true;
        return data;
    }
    if (header == "GEPLAYER" && (stream >> data.feetPosition.x
        >> data.feetPosition.y >> data.feetPosition.z >> selectedSlot)) {
        data.inventory = {};
        for (int slot = 0; slot < placeableBlockCount(); ++slot) {
            int count = 0;
            if (!(stream >> count)) return data;
            data.inventory.slots[slot] = InventorySlot{
                blockItemId(placeableBlock(slot)), std::clamp(count, 0, InventoryMaxStack), 0};
        }
        normalizeInventory(data.inventory);
        data.selectedHotbarSlot = std::clamp(selectedSlot - 1, 0, InventoryHotbarSlots - 1);
        data.valid = true;
    }
    return data;
}

void savePlayerState(int seed, const PlayerController& player,
    const Inventory& inventory, const CraftingGrid& craftingGrid,
    const CraftingTableGrid& craftingTableGrid, int selectedHotbarSlot) {
    if (!player.active()) return;
    const std::filesystem::path path = playerSavePath(seed);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::trunc);
    if (!stream) return;
    const glm::vec3 feet = player.feetPosition();
    stream << "GEPLAYER3\n";
    stream << feet.x << ' ' << feet.y << ' ' << feet.z << ' '
        << std::clamp(selectedHotbarSlot, 0, InventoryHotbarSlots - 1) << '\n';
    for (int slot = 0; slot < InventorySlots; ++slot) {
        const InventorySlot& item = inventory.slots[slot];
        stream << (item.empty() ? 0 : item.itemId)
            << ' ' << (item.empty() ? 0 : item.count)
            << ' ' << (item.empty() ? 0 : item.durability) << '\n';
    }
    for (int slot = 0; slot < CraftingGridSlots; ++slot) {
        const InventorySlot& item = craftingGrid.slots[slot];
        stream << (item.empty() ? 0 : item.itemId)
            << ' ' << (item.empty() ? 0 : item.count)
            << ' ' << (item.empty() ? 0 : item.durability) << '\n';
    }
    for (int slot = 0; slot < CraftingTableGridSlots; ++slot) {
        const InventorySlot& item = craftingTableGrid.slots[slot];
        stream << (item.empty() ? 0 : item.itemId)
            << ' ' << (item.empty() ? 0 : item.count)
            << ' ' << (item.empty() ? 0 : item.durability) << '\n';
    }
}
}

Application::Application()
    : window_(1280, 800, "GraphicEngine"), viewportFramebuffer_(1, 1), editor_(window_) {}

void Application::run() {
    onStart();
    bool previousF5 = false;
    bool previousF2 = false;
    bool previousF3 = false;
    bool previousF4 = false;
    bool previousF6 = false;
    bool previousE = false;
    bool previousLeftMouse = false;
    bool previousRightMouse = false;
    bool previousPlaying = false;
    bool previousEscape = false;
    Inventory inventory = createStarterInventory();
    CraftingGrid craftingGrid;
    CraftingTableGrid craftingTableGrid;
    InventorySlot furnaceInput;
    InventorySlot furnaceFuel;
    InventorySlot furnaceOutput;
    float furnaceProgress = 0.0f;
    float furnaceBurnSeconds = 0.0f;
    float furnaceBurnCapacitySeconds = 0.0f;
    InventorySlot cursorItem;
    int selectedHotbarSlot = 1;
    std::optional<int> activeSaveSeed;
    double nextAutosaveTime = 0.0;
    struct MiningState {
        std::optional<Renderer::TerrainBlockTarget> target;
        float progress = 0.0f;
    } mining;
    while (!window_.shouldClose()) {
        const float deltaTime = time_.tick();
        window_.pollEvents();
        viewportFramebuffer_.resize(editor_.viewportWidth(), editor_.viewportHeight());
        editor_.beginFrame();
        editor_.setInventory(inventory, selectedHotbarSlot);
        editor_.setCraftingGrid(craftingGrid);
        editor_.setCraftingTableGrid(craftingTableGrid);
        editor_.setFurnaceSlots(furnaceInput, furnaceFuel, furnaceOutput,
            furnaceProgress,
            furnaceBurnCapacitySeconds > 0.0f
                ? furnaceBurnSeconds / furnaceBurnCapacitySeconds : 0.0f);
        editor_.setCursorItem(cursorItem);
        editor_.draw(scene_, time_, viewportFramebuffer_.colorAttachment());
        const auto stashCursorItem = [&]() {
            if (cursorItem.empty()) return true;
            if (addToInventory(inventory, cursorItem)) {
                cursorItem = {};
                return true;
            }
            editor_.showActionMessage("Inventario lleno");
            return false;
        };
        const bool f5 = Input::keyPressed(window_, GLFW_KEY_F5);
        if (f5 && !previousF5) {
            if (editor_.playing()) {
                stashCursorItem();
                if (activeSaveSeed) savePlayerState(*activeSaveSeed, player_, inventory,
                    craftingGrid, craftingTableGrid, selectedHotbarSlot);
                activeSaveSeed.reset();
            }
            editor_.togglePlay();
        }
        previousF5 = f5;
        const bool f2 = Input::keyPressed(window_, GLFW_KEY_F2);
        if (f2 && !previousF2) editor_.toggleWireframe();
        previousF2 = f2;
        const bool f3 = Input::keyPressed(window_, GLFW_KEY_F3);
        if (f3 && !previousF3) editor_.toggleFrustumCulling();
        previousF3 = f3;
        const bool f4 = Input::keyPressed(window_, GLFW_KEY_F4);
        if (f4 && !previousF4) editor_.toggleChunkBounds();
        previousF4 = f4;
        const bool f6 = Input::keyPressed(window_, GLFW_KEY_F6);
        if (f6 && !previousF6) editor_.toggleOcclusionCulling();
        previousF6 = f6;
        const bool eKey = Input::keyPressed(window_, GLFW_KEY_E);
        if (eKey && !previousE && editor_.playing()) {
            if (editor_.inventoryOpen()) {
                if (stashCursorItem()) editor_.closeInventory();
            } else if (const auto target = renderer_.terrainTargetBlock(scene_, camera_)) {
                if (target->type == BlockType::CraftingTable)
                    editor_.openCraftingTable();
                else if (target->type == BlockType::Furnace)
                    editor_.openFurnace();
                else
                    editor_.toggleInventory();
            } else {
                editor_.toggleInventory();
            }
        }
        previousE = eKey;
        const bool escape = Input::keyPressed(window_, GLFW_KEY_ESCAPE);
        if (escape && !previousEscape) {
            if (editor_.inventoryOpen()) {
                if (stashCursorItem()) editor_.closeInventory();
            } else if (editor_.playing()) {
                stashCursorItem();
                if (activeSaveSeed) savePlayerState(*activeSaveSeed, player_, inventory,
                    craftingGrid, craftingTableGrid, selectedHotbarSlot);
                activeSaveSeed.reset();
                editor_.togglePlay();
            }
            else window_.requestClose();
        }
        previousEscape = escape;
        const bool inventoryOpen = editor_.inventoryOpen();
        const bool viewportActive = !inventoryOpen
            && (editor_.playing() || editor_.viewportHovered() || editor_.viewportFocused());
        camera_.update(window_, deltaTime, viewportActive, !editor_.playing(),
            editor_.playing() && !inventoryOpen);
        if (editor_.playing() && (!previousPlaying || !player_.active())) {
            activeSaveSeed = primaryTerrainSeed(scene_);
            bool restoredPlayer = false;
            if (activeSaveSeed) {
                const PlayerSaveData save = loadPlayerSave(*activeSaveSeed);
                if (save.valid) {
                    selectedHotbarSlot = save.selectedHotbarSlot;
                    inventory = save.inventory;
                    craftingGrid = save.craftingGrid;
                    craftingTableGrid = save.craftingTableGrid;
                    player_.spawn(save.feetPosition);
                    restoredPlayer = true;
                    editor_.showActionMessage("Partida cargada");
                }
            }
            if (!restoredPlayer) {
                if (const auto spawn = renderer_.terrainSpawnPoint(scene_)) player_.spawn(*spawn);
            }
            nextAutosaveTime = time_.elapsedTime() + 3.0;
        }
        if (editor_.playing() && !inventoryOpen)
            player_.update(window_, camera_, renderer_, scene_, deltaTime);
        const auto updateFurnace = [&]() {
            const SmeltingRecipe recipe = smeltingRecipe(furnaceInput.itemId);
            const bool canSmelt = !recipe.output.empty()
                && canSmeltIntoOutput(furnaceInput, furnaceOutput);
            if (!canSmelt) {
                furnaceProgress = 0.0f;
                return;
            }
            if (furnaceBurnSeconds <= 0.0f) {
                const float fuelSeconds = furnaceFuelSeconds(furnaceFuel.itemId);
                if (fuelSeconds <= 0.0f || furnaceFuel.empty()) return;
                --furnaceFuel.count;
                normalizeInventorySlot(furnaceFuel);
                furnaceBurnSeconds = fuelSeconds;
                furnaceBurnCapacitySeconds = fuelSeconds;
            }
            furnaceBurnSeconds = std::max(0.0f, furnaceBurnSeconds - deltaTime);
            furnaceProgress += deltaTime / std::max(0.1f, recipe.seconds);
            if (furnaceProgress >= 1.0f) {
                --furnaceInput.count;
                normalizeInventorySlot(furnaceInput);
                addSmeltingOutput(furnaceOutput, recipe.output);
                furnaceProgress = 0.0f;
            }
        };
        updateFurnace();
        if (inventoryOpen) {
            const auto interactWithSlot = [&](InventorySlot& slot) {
                if (cursorItem.empty()) {
                    cursorItem = slot;
                    slot = {};
                    return;
                }
                if (!moveAllToSlot(slot, cursorItem))
                    editor_.showActionMessage("No se pudo mover");
            };
            const auto interactWithSlotRight = [&](InventorySlot& slot) {
                if (cursorItem.empty()) {
                    if (!takeHalfFromSlot(slot, cursorItem))
                        editor_.showActionMessage("Slot vacio");
                    return;
                }
                if (!moveOneToSlot(slot, cursorItem))
                    editor_.showActionMessage("No se pudo soltar 1");
            };

            if (editor_.furnaceOpen()) {
                const auto interactWithFurnaceSlot = [&](int slotIndex, bool rightClick) {
                    InventorySlot* slot = nullptr;
                    if (slotIndex == 0) slot = &furnaceInput;
                    else if (slotIndex == 1) slot = &furnaceFuel;
                    else if (slotIndex == 2) slot = &furnaceOutput;
                    if (!slot) return;
                    if (slotIndex == 2) {
                        if (rightClick) {
                            if (!moveOneToSlot(cursorItem, *slot))
                                editor_.showActionMessage("No se pudo recoger 1");
                        } else if (!moveAllToSlot(cursorItem, *slot)) {
                            editor_.showActionMessage("No se pudo recoger");
                        }
                        return;
                    }
                    if (rightClick) interactWithSlotRight(*slot);
                    else interactWithSlot(*slot);
                };
                if (const auto furnaceSlot = editor_.consumePendingFurnaceSlot())
                    interactWithFurnaceSlot(*furnaceSlot, false);
                if (const auto furnaceSlot = editor_.consumePendingRightFurnaceSlot())
                    interactWithFurnaceSlot(*furnaceSlot, true);
            } else if (editor_.craftingTableOpen()) {
                if (const auto craftingSlot = editor_.consumePendingCraftingTableSlot()) {
                    if (*craftingSlot >= 0 && *craftingSlot < CraftingTableGridSlots)
                        interactWithSlot(craftingTableGrid.slots[*craftingSlot]);
                }
                if (const auto craftingSlot = editor_.consumePendingRightCraftingTableSlot()) {
                    if (*craftingSlot >= 0 && *craftingSlot < CraftingTableGridSlots)
                        interactWithSlotRight(craftingTableGrid.slots[*craftingSlot]);
                }
            } else {
                if (const auto craftingSlot = editor_.consumePendingCraftingSlot()) {
                    if (*craftingSlot >= 0 && *craftingSlot < CraftingGridSlots)
                        interactWithSlot(craftingGrid.slots[*craftingSlot]);
                }
                if (const auto craftingSlot = editor_.consumePendingRightCraftingSlot()) {
                    if (*craftingSlot >= 0 && *craftingSlot < CraftingGridSlots)
                        interactWithSlotRight(craftingGrid.slots[*craftingSlot]);
                }
            }
            const bool craftingOutputClicked = !editor_.furnaceOpen()
                && editor_.consumePendingCraftingOutput();
            const bool craftingOutputRightClicked = !editor_.furnaceOpen()
                && editor_.consumePendingRightCraftingOutput();
            if (!editor_.furnaceOpen() && (craftingOutputClicked || craftingOutputRightClicked)) {
                const CraftingResult result = editor_.craftingTableOpen()
                    ? craftingTableResult(craftingTableGrid) : craftingResult(craftingGrid);
                if (result.valid) {
                    if (addCraftingOutputToCursor(cursorItem, result.output)) {
                        if (editor_.craftingTableOpen())
                            consumeCraftingTableRecipe(craftingTableGrid);
                        else
                            consumeCraftingRecipe(craftingGrid);
                    } else {
                        editor_.showActionMessage("Cursor ocupado");
                    }
                }
            }
            if (const auto clickedSlot = editor_.consumePendingInventorySlot()) {
                if (*clickedSlot >= 0 && *clickedSlot < InventorySlots) {
                    if (*clickedSlot < InventoryHotbarSlots && cursorItem.empty()) {
                        selectedHotbarSlot = *clickedSlot;
                    }
                    interactWithSlot(inventory.slots[*clickedSlot]);
                }
            }
            if (const auto clickedSlot = editor_.consumePendingRightInventorySlot()) {
                if (*clickedSlot >= 0 && *clickedSlot < InventorySlots) {
                    if (*clickedSlot < InventoryHotbarSlots && cursorItem.empty()) {
                        selectedHotbarSlot = *clickedSlot;
                    }
                    interactWithSlotRight(inventory.slots[*clickedSlot]);
                }
            }
        }
        if (editor_.playing() && activeSaveSeed && player_.active()
            && time_.elapsedTime() >= nextAutosaveTime) {
            savePlayerState(*activeSaveSeed, player_, inventory, craftingGrid,
                craftingTableGrid, selectedHotbarSlot);
            nextAutosaveTime = time_.elapsedTime() + 3.0;
        }
        if (previousPlaying && !editor_.playing() && activeSaveSeed) {
            stashCursorItem();
            savePlayerState(*activeSaveSeed, player_, inventory, craftingGrid,
                craftingTableGrid, selectedHotbarSlot);
            activeSaveSeed.reset();
        }
        previousPlaying = editor_.playing();
        for (int slot = 0; slot < InventoryHotbarSlots; ++slot) {
            if (Input::keyPressed(window_, GLFW_KEY_1 + slot))
                selectedHotbarSlot = slot;
        }
        const bool leftMouse = glfwGetMouseButton(window_.nativeHandle(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool rightMouse = glfwGetMouseButton(window_.nativeHandle(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        const bool windowFocused = glfwGetWindowAttrib(window_.nativeHandle(), GLFW_FOCUSED) == GLFW_TRUE;
        const bool shiftPressed = Input::keyPressed(window_, GLFW_KEY_LEFT_SHIFT)
            || Input::keyPressed(window_, GLFW_KEY_RIGHT_SHIFT);
        const bool leftClicked = leftMouse && !previousLeftMouse;
        const bool rightClicked = rightMouse && !previousRightMouse;
        const auto showPlacementFailure = [&](Renderer::TerrainEditResult result) {
            switch (result) {
            case Renderer::TerrainEditResult::IntersectsForbiddenBounds:
                editor_.showActionMessage("Muy cerca del jugador");
                break;
            case Renderer::TerrainEditResult::TargetOccupied:
                editor_.showActionMessage("Ese espacio ya esta ocupado");
                break;
            case Renderer::TerrainEditResult::NoTarget:
                editor_.showActionMessage("No hay bloque al alcance");
                break;
            default:
                editor_.showActionMessage("No se pudo colocar");
                break;
            }
        };
        if (editor_.playing() && windowFocused && !inventoryOpen) {
            if (rightClicked || (leftClicked && shiftPressed)) {
                bool handledInteraction = false;
                if (rightClicked) {
                    const auto target = renderer_.terrainTargetBlock(scene_, camera_);
                    if (target && target->type == BlockType::CraftingTable) {
                        editor_.openCraftingTable();
                        editor_.showActionMessage("Crafting table");
                        handledInteraction = true;
                    } else if (target && target->type == BlockType::Furnace) {
                        editor_.openFurnace();
                        editor_.showActionMessage("Furnace");
                        handledInteraction = true;
                    }
                }
                if (!handledInteraction) {
                    const InventorySlot& selectedItem = inventory.slots[selectedHotbarSlot];
                    const BlockType selectedBlock = selectedHotbarBlock(inventory, selectedHotbarSlot);
                    if (selectedItem.empty()) {
                        editor_.showActionMessage("Slot vacio");
                    } else if (selectedBlock == BlockType::Air) {
                        editor_.showActionMessage("Item no colocable");
                    } else {
                        const Renderer::TerrainEditResult result = renderer_.editTerrainDetailed(
                            scene_, camera_, true, selectedBlock,
                            player_.boundsMin(), player_.boundsMax());
                        if (result == Renderer::TerrainEditResult::Success)
                            removeOneFromSlot(inventory, selectedHotbarSlot);
                        else showPlacementFailure(result);
                    }
                }
                mining = {};
                editor_.clearMiningProgress();
            } else if (leftMouse) {
                const auto target = renderer_.terrainTargetBlock(scene_, camera_);
                if (!target) {
                    mining = {};
                    editor_.clearMiningProgress();
                } else if (target->type == BlockType::Bedrock) {
                    mining = {};
                    editor_.clearMiningProgress();
                    editor_.showActionMessage("Bedrock no se puede romper");
                } else {
                    const bool sameTarget = mining.target
                        && mining.target->entityId == target->entityId
                        && mining.target->block == target->block
                        && mining.target->type == target->type;
                    if (!sameTarget) {
                        mining.target = target;
                        mining.progress = 0.0f;
                    }
                    float hardness = std::max(0.05f, blockHardnessSeconds(target->type));
                    const int pickaxeTier = selectedHotbarPickaxeTier(inventory, selectedHotbarSlot);
                    if (pickaxeTier > 0
                        && (target->type == BlockType::Stone || target->type == BlockType::CoalOre)) {
                        hardness *= pickaxeTier >= 2 ? 0.28f : 0.45f;
                    }
                    mining.progress += deltaTime / hardness;
                    editor_.setMiningProgress(target->type, mining.progress);
                    if (mining.progress >= 1.0f) {
                        if (renderer_.editTerrain(scene_, camera_, false, BlockType::Air,
                            player_.boundsMin(), player_.boundsMax())) {
                            const InventorySlot drop = blockDrop(target->type);
                            if (!drop.empty() && !addToInventory(inventory, drop))
                                editor_.showActionMessage("Inventario lleno");
                            if (damageInventorySlot(inventory, selectedHotbarSlot))
                                editor_.showActionMessage("Herramienta rota");
                        }
                        mining = {};
                        editor_.clearMiningProgress();
                    }
                }
            } else {
                mining = {};
                editor_.clearMiningProgress();
            }
        } else {
            mining = {};
            editor_.clearMiningProgress();
        }
        previousLeftMouse = leftMouse;
        previousRightMouse = rightMouse;
        onUpdate(deltaTime);

        if (window_.width() > 0 && window_.height() > 0) {
            viewportFramebuffer_.bind();
            const float viewportAspect = static_cast<float>(viewportFramebuffer_.width()) /
                static_cast<float>(viewportFramebuffer_.height());
            renderer_.setWireframe(editor_.wireframe());
            renderer_.setFrustumCulling(editor_.frustumCulling());
            renderer_.setChunkBounds(editor_.chunkBounds());
            renderer_.setOcclusionCulling(editor_.occlusionCulling());
            renderer_.render(scene_, camera_, viewportAspect, deltaTime * 1000.0f);
            const RendererStats& stats = renderer_.stats();
            editor_.setRendererStats(stats.triangles, stats.drawCalls,
                stats.visibleChunks, stats.loadedChunks, stats.pendingMeshJobs,
                stats.uploadedMeshes, stats.activeMeshWorkers, stats.activeUploadBudget,
                stats.generatedChunks, stats.pendingChunkJobs,
                stats.cpuRenderMilliseconds, stats.meshProcessingMilliseconds,
                stats.gpuRenderMilliseconds);
            if (const auto pick = editor_.consumePendingPick()) {
                const int pickX = pick->first * viewportFramebuffer_.width() /
                    std::max(1, editor_.viewportWidth());
                const int pickY = pick->second * viewportFramebuffer_.height() /
                    std::max(1, editor_.viewportHeight());
                const std::uint32_t entityId = viewportFramebuffer_.readEntityId(pickX, pickY);
                editor_.selectEntity(scene_.findEntity(entityId) ? entityId : 0);
            }
            Framebuffer::unbind(window_.width(), window_.height());
            glClearColor(0.025f, 0.028f, 0.035f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        editor_.endFrame();
        if (window_.width() > 0 && window_.height() > 0) {
            window_.swapBuffers();
        }
    }
    if (!cursorItem.empty()) {
        addToInventory(inventory, cursorItem);
        cursorItem = {};
    }
    if (activeSaveSeed) savePlayerState(*activeSaveSeed, player_, inventory,
        craftingGrid, craftingTableGrid, selectedHotbarSlot);
}

} // namespace Engine
