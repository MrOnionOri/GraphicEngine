from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ATLAS = ROOT / "Assets" / "Textures" / "Blocks.ppm"
TILE_SIZE = 16


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
    pixels = []
    cursor = 0
    for _y in range(height):
        row = []
        for _x in range(width):
            row.append((values[cursor], values[cursor + 1], values[cursor + 2]))
            cursor += 3
        pixels.append(row)
    return width, height, pixels


def write_ppm(path, width, height, pixels):
    lines = [
        "P3",
        "# Original GraphicEngine atlas: grass top, dirt, stone, grass side, bark, leaves, sand, coal ore, planks, crafting table",
        f"{width} {height}",
        "255",
    ]
    for row in pixels:
        values = []
        for red, green, blue in row:
            values.extend((str(red), str(green), str(blue)))
        lines.append(" ".join(values))
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def clamp(value):
    return max(0, min(255, int(value)))


def noise(x, y, seed):
    value = (x * 374761393 + y * 668265263 + seed * 1442695041) & 0xFFFFFFFF
    value ^= value >> 13
    value = (value * 1274126177) & 0xFFFFFFFF
    value ^= value >> 16
    return value & 255


def sand_pixel(x, y):
    grain = noise(x, y, 701) - 128
    wave = 10 if (x + y * 2) % 7 < 2 else -3
    return (
        clamp(206 + grain * 0.10 + wave),
        clamp(188 + grain * 0.08 + wave),
        clamp(112 + grain * 0.05),
    )


def coal_ore_pixel(x, y):
    grain = noise(x, y, 953) - 128
    base = clamp(95 + grain * 0.10)
    vein = noise(x // 2, y // 2, 1201)
    if vein > 188 or abs((x - 7) + (y - 8)) < 2:
        coal = clamp(22 + grain * 0.04)
        return coal, coal, coal + 2
    return base, base, base


def planks_pixel(x, y):
    grain = noise(x, y, 1601) - 128
    plank_band = y // 4
    seam = y % 4 == 0
    knot = noise(x // 3, y // 2, 1709) > 238
    red = 151 + grain * 0.08 + plank_band * 5
    green = 101 + grain * 0.06 + plank_band * 3
    blue = 48 + grain * 0.04
    if seam:
        red -= 38
        green -= 26
        blue -= 12
    if knot:
        red -= 28
        green -= 20
        blue -= 10
    return clamp(red), clamp(green), clamp(blue)


def crafting_table_pixel(x, y):
    border = x < 2 or y < 2 or x > 13 or y > 13
    seam = x == 7 or x == 8 or y == 7 or y == 8
    grain = noise(x, y, 2003) - 128
    red = 127 + grain * 0.07
    green = 82 + grain * 0.05
    blue = 41 + grain * 0.04
    if border:
        red -= 36
        green -= 24
        blue -= 12
    if seam:
        red += 28
        green += 18
        blue += 8
    if (3 <= x <= 5 and 3 <= y <= 5) or (10 <= x <= 12 and 10 <= y <= 12):
        red += 34
        green += 24
        blue += 12
    return clamp(red), clamp(green), clamp(blue)


def main():
    width, height, pixels = read_ppm(ATLAS)
    if height != TILE_SIZE or width < TILE_SIZE * 6:
        raise RuntimeError("Blocks.ppm must contain at least six 16x16 tiles")
    tiles = width // TILE_SIZE
    output_tiles = 10
    output = [[(0, 0, 0) for _ in range(output_tiles * TILE_SIZE)] for _ in range(TILE_SIZE)]

    for y in range(TILE_SIZE):
        for x in range(min(6, tiles) * TILE_SIZE):
            output[y][x] = pixels[y][x]
        for local_x in range(TILE_SIZE):
            output[y][6 * TILE_SIZE + local_x] = sand_pixel(local_x, y)
            output[y][7 * TILE_SIZE + local_x] = coal_ore_pixel(local_x, y)
            output[y][8 * TILE_SIZE + local_x] = planks_pixel(local_x, y)
            output[y][9 * TILE_SIZE + local_x] = crafting_table_pixel(local_x, y)

    write_ppm(ATLAS, output_tiles * TILE_SIZE, TILE_SIZE, output)


if __name__ == "__main__":
    main()
