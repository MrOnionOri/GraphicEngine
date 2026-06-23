# Resource packs

GraphicEngine includes an original voxel atlas. You can override it locally without
changing tracked project assets.

Create `ResourcePacks/Active/Blocks.ppm` using ASCII PPM (`P3`). The atlas must contain one row
of ten equally sized square tiles in this order:

1. Grass top
2. Dirt
3. Stone
4. Grass side
5. Tree bark
6. Leaves
7. Sand
8. Coal ore
9. Planks
10. Crafting table

The active directory is ignored by Git intentionally. Restart GraphicEngine after changing the
pack. When running the compiled executable directly, the same directory can be placed beside the
executable.

Do not commit copyrighted game textures to this repository. Users may provide their own local
pack when they have permission to use its assets.
