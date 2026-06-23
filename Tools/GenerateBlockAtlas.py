from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TILE_SIZE = 16
TILES_DIR = ROOT / "Assets" / "Textures" / "BlockTiles"
ATLAS = ROOT / "Assets" / "Textures" / "Blocks.ppm"

# Keep this order in sync with Engine/Voxel/Block.h tile indices.
TILES = [
    "00_grass_top.ppm",
    "01_dirt.ppm",
    "02_stone.ppm",
    "03_grass_side.ppm",
    "04_wood.ppm",
    "05_leaves.ppm",
    "06_sand.ppm",
    "07_coal_ore.ppm",
    "08_planks.ppm",
    "09_crafting_table.ppm",
    "10_cobblestone.ppm",
    "11_gravel.ppm",
    "12_bedrock.ppm",
    "13_glass.ppm",
    "14_bricks.ppm",
    "15_iron_ore.ppm",
    "16_furnace.ppm",
]


def read_ppm(path):
    tokens = []
    for line in path.read_text(encoding="ascii").splitlines():
        if "#" in line:
            line = line[:line.index("#")]
        tokens.extend(line.split())
    if not tokens or tokens[0] != "P3":
        raise RuntimeError(f"{path} is not an ASCII P3 PPM")
    width = int(tokens[1])
    height = int(tokens[2])
    maximum = int(tokens[3])
    values = [int(value) for value in tokens[4:]]
    if maximum != 255 or len(values) != width * height * 3:
        raise RuntimeError(f"{path} has an unexpected PPM layout")
    if width != TILE_SIZE or height != TILE_SIZE:
        raise RuntimeError(f"{path} must be exactly {TILE_SIZE}x{TILE_SIZE}")
    pixels = []
    cursor = 0
    for _y in range(height):
        row = []
        for _x in range(width):
            row.append((values[cursor], values[cursor + 1], values[cursor + 2]))
            cursor += 3
        pixels.append(row)
    return pixels


def write_ppm(path, width, height, pixels):
    lines = [
        "P3",
        "# GraphicEngine block atlas packed from Assets/Textures/BlockTiles/*.ppm",
        f"{width} {height}",
        "255",
    ]
    for row in pixels:
        values = []
        for red, green, blue in row:
            values.extend((str(red), str(green), str(blue)))
        lines.append(" ".join(values))
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def main():
    tiles = []
    for tile_name in TILES:
        tile_path = TILES_DIR / tile_name
        if not tile_path.exists():
            raise RuntimeError(f"Missing block tile: {tile_path}")
        tiles.append(read_ppm(tile_path))

    width = len(tiles) * TILE_SIZE
    height = TILE_SIZE
    atlas_pixels = [[(0, 0, 0) for _ in range(width)] for _ in range(height)]
    for tile_index, tile in enumerate(tiles):
        x_offset = tile_index * TILE_SIZE
        for y in range(TILE_SIZE):
            for x in range(TILE_SIZE):
                atlas_pixels[y][x_offset + x] = tile[y][x]

    write_ppm(ATLAS, width, height, atlas_pixels)
    print(f"Packed {len(tiles)} block tiles into {ATLAS}")


if __name__ == "__main__":
    main()
