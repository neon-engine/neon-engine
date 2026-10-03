#!/usr/bin/env python3
"""Writes the images of the skies in app/NeonRuntime/assets/textures/sky.

Two skies, each as the six faces of a cube and as one panorama, so that a
Sky of type box and one of type sphere can be compared:

day-*.png   A day painted from a formula: blue that pales towards the
            horizon, ground below it, and a sun ahead and to the right of
            a camera that was not turned. For sky-box.scene.yml and
            sky-sphere.scene.yml, which show the same picture.

museum-panorama.png
            The same day with the sun where the direction light of the
            museum, demo.scene.yml, comes from, so that the shadows there
            fall away from the sun that is seen.

test-*.png  One plain colour for every direction, for tests/runtime-sky,
            which reads pixels: right red, left green, top blue, bottom
            brown, front yellow, back purple.

The faces are laid out as docs/scenes.md says a Sky expects them: `front`
is seen along negative z, `right` along positive x, `top` along positive y,
the four sides upright, the lower edge of `top` on the upper edge of
`front`. The middle of a panorama is seen along negative z.

Needs nothing but Python. Run from the root of the repository:

    python3 tools/make-sky-images.py
"""

import math
import struct
import zlib
from pathlib import Path

FOLDER = Path("app/NeonRuntime/assets/textures/sky")

# the faces in the order of the names, and the direction a place on each is
# seen in: across and down from -1 to 1, as the image is looked at
FACES = [
    ("right", lambda across, down: (1.0, -down, across)),
    ("left", lambda across, down: (-1.0, -down, -across)),
    ("top", lambda across, down: (across, 1.0, -down)),
    ("bottom", lambda across, down: (across, -1.0, down)),
    ("front", lambda across, down: (across, -down, -1.0)),
    ("back", lambda across, down: (-across, -down, 1.0)),
]

TEST_COLOURS = {
    "right": (220, 40, 40),
    "left": (40, 180, 40),
    "top": (60, 90, 230),
    "bottom": (120, 80, 40),
    "front": (240, 220, 60),
    "back": (150, 60, 200),
}

# ahead and to the right of a camera that was not turned, 27 degrees up
SUN = (0.5, 0.45, -0.74)

# where the light of the museum comes from: the opposite of the direction
# of its Light, [0.6, -1, -0.25]
MUSEUM_SUN = (-0.6, 1.0, 0.25)


def normalized(direction):
    length = math.sqrt(sum(part * part for part in direction))
    return tuple(part / length for part in direction)


def mixed(low, high, amount):
    amount = min(max(amount, 0.0), 1.0)
    return tuple(a + (b - a) * amount for a, b in zip(low, high))


def day(direction, sun=SUN):
    """The colour of the day sky in a direction, as a screen shows it, with
    the sun standing in the direction `sun`."""
    x, y, z = normalized(direction)
    sun = normalized(sun)

    horizon = (0.80, 0.88, 0.95)
    zenith = (0.16, 0.42, 0.84)
    ground = (0.34, 0.31, 0.27)
    far_ground = (0.62, 0.64, 0.62)

    if y >= 0.0:
        colour = mixed(horizon, zenith, math.sqrt(y))
    else:
        colour = mixed(far_ground, ground, math.sqrt(-y) * 2.0)

    # the sun: a disc of 3 degrees, and a glow that fades around it
    towards = max(x * sun[0] + y * sun[1] + z * sun[2], -1.0)
    angle = math.degrees(math.acos(min(towards, 1.0)))
    if y >= 0.0:
        glow = math.exp(-angle / 12.0) * 0.55
        colour = mixed(colour, (1.0, 0.95, 0.80), glow)
        colour = mixed(colour, (1.0, 1.0, 0.94), 1.0 - (angle - 2.5) / 0.5)

    return tuple(round(min(max(part, 0.0), 1.0) * 255.0) for part in colour)


def test(direction):
    """One plain colour for the face of the cube a direction falls on."""
    x, y, z = direction
    largest = max(abs(x), abs(y), abs(z))
    if largest == abs(y):
        return TEST_COLOURS["top" if y > 0.0 else "bottom"]
    if largest == abs(x):
        return TEST_COLOURS["right" if x > 0.0 else "left"]
    return TEST_COLOURS["front" if z < 0.0 else "back"]


def test_panorama(across, down):
    """The same colours as bands of a panorama: the top and the bottom
    quarter, and four columns between them."""
    if down < 0.25:
        return TEST_COLOURS["top"]
    if down >= 0.75:
        return TEST_COLOURS["bottom"]
    if 0.375 <= across < 0.625:
        return TEST_COLOURS["front"]
    if 0.625 <= across < 0.875:
        return TEST_COLOURS["right"]
    if 0.125 <= across < 0.375:
        return TEST_COLOURS["left"]
    return TEST_COLOURS["back"]


def write_png(path, width, height, colour_at):
    """Writes an image of red, green, and blue; colour_at(column, row)
    gives a pixel, rows counted from the top."""

    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    rows = bytearray()
    above = bytes(width * 3)
    for row in range(height):
        line = bytearray()
        for column in range(width):
            line.extend(colour_at(column, row))
        # each row as its difference from the one above, which packs a
        # gradient well
        rows.append(2)
        rows.extend((value - upper) & 0xFF for value, upper in zip(line, above))
        above = bytes(line)

    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", header)
        + chunk(b"IDAT", zlib.compress(bytes(rows), 9))
        + chunk(b"IEND", b""))
    print(f"{path}: {width} by {height}, {path.stat().st_size} bytes")


def write_faces(prefix, side, colour_of):
    for name, direction_at in FACES:
        def colour_at(column, row, direction_at=direction_at):
            across = (column + 0.5) / side * 2.0 - 1.0
            down = (row + 0.5) / side * 2.0 - 1.0
            return colour_of(direction_at(across, down))

        write_png(FOLDER / f"{prefix}-{name}.png", side, side, colour_at)


def panorama_direction(across, down):
    """The direction a place of a panorama is seen in, both from 0 to 1."""
    around = (across - 0.5) * 2.0 * math.pi
    from_up = down * math.pi
    return (math.sin(from_up) * math.sin(around), math.cos(from_up), -math.sin(from_up) * math.cos(around))


def main():
    FOLDER.mkdir(parents=True, exist_ok=True)

    write_faces("day", 512, day)
    write_png(
        FOLDER / "day-panorama.png", 2048, 1024,
        lambda column, row: day(panorama_direction((column + 0.5) / 2048, (row + 0.5) / 1024)))

    write_png(
        FOLDER / "museum-panorama.png", 2048, 1024,
        lambda column, row: day(panorama_direction((column + 0.5) / 2048, (row + 0.5) / 1024), MUSEUM_SUN))

    write_faces("test", 16, test)
    write_png(
        FOLDER / "test-panorama.png", 64, 32,
        lambda column, row: test_panorama((column + 0.5) / 64, (row + 0.5) / 32))


if __name__ == "__main__":
    main()
