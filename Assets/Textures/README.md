# Included voxel atlas

`Blocks.ppm` is the runtime atlas packed from `BlockTiles/*.ppm`. It contains original 16x16 tiles: grass top, dirt,
stone, grass side, tree bark, leaves, sand, coal ore, planks, crafting table, cobblestone, gravel,
bedrock, glass, bricks, and iron ore. `BlocksOriginalV2.png` is the compact
preview for the first texture pass. The other PNG files preserve the generated source material.

Edit the individual `Assets/Textures/BlockTiles/*.ppm` files to change block textures. Run
`python Tools/GenerateBlockAtlas.py` from the project root after editing any tile.

The atlas and tiles were made specifically for GraphicEngine. They are not derived from Minecraft assets.
