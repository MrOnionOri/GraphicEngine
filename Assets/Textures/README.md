# Included voxel atlas

`Blocks.ppm` is the runtime 160x16 atlas. It contains ten original 16x16 tiles: grass top, dirt,
stone, grass side, tree bark, leaves, sand, coal ore, planks, and crafting table. `BlocksOriginalV2.png` is the compact
preview for the first texture pass. The other PNG files preserve the generated source material.

Run `python Tools/GenerateBlockAtlas.py` from the project root after editing the atlas generator.

The atlas was generated specifically for GraphicEngine and then downscaled with nearest-neighbor
sampling. It is not derived from Minecraft assets.
