#!/usr/bin/env python3
"""Writes what the scenes of tests/game show that the engine does not build
itself: a kit of pieces as models, three textures, and a sound. Everything
is made from the numbers below, so nothing here was drawn, recorded, or
taken from anywhere, and all of it is small: the whole of it is a few
kilobytes.

models/kit/    Pieces a level is put together from, each a GLB of a few
               boxes, in metres, standing on its origin: a wall, a floor
               tile, a column, a crate, a wall with a doorway, a target, a
               supply crate, a blaster, and a small target. Every piece
               takes its colours from one image, `colormap.png` next to
               them, which it names as a glTF names a texture, so that the
               tests see models that share a texture
               (tests/runtime-prototype, tests/runtime-sharing).

textures/      brick.png, concrete.png, and wood.png, 32 pixels square,
               each a pattern that shows which way up it lies and how often
               it repeats (tests/runtime-geometry).

sounds/        tone.wav, half a second of a sine wave that loops without a
               click, for the scenes that play a sound.

Needs nothing but Python. Run from the root of the repository:

    python3 tools/make-test-game-assets.py
"""

import json
import math
import struct
import zlib
from pathlib import Path

ASSETS = Path(__file__).resolve().parent.parent / "tests/game/assets"


def write_png(path, width, height, pixel):
    """Writes an image of red, green, and blue, `pixel(x, y)` from the top left."""

    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    rows = b""
    for y in range(height):
        rows += b"\x00" + b"".join(bytes(pixel(x, y)) for x in range(width))

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(rows, 9))
        + chunk(b"IEND", b"")
    )


# The kit. The colours of the pieces, each a pixel of colormap.png, which is
# four pixels wide and two high.

COLOURS = {
    "wall": (150, 155, 190),
    "frame": (85, 90, 112),
    "crate": (45, 50, 140),
    "plate": (170, 170, 178),
    "red": (220, 80, 55),
    "metal": (60, 60, 70),
    "supply": (110, 130, 90),
    "floor": (160, 162, 185),
}
COLOUR_NAMES = list(COLOURS)
COLORMAP_WIDTH = 4
COLORMAP_HEIGHT = 2

# Each piece as boxes: the colour, then the least and the most of x, y, and z.
PIECES = {
    "wall": [("wall", (-0.1, 0.0, -0.5), (0.1, 1.0, 0.5))],
    # a tile has no thickness to speak of: its top lies at the height of its origin
    "floor-square": [("floor", (-0.5, -0.02, -0.5), (0.5, 0.0, 0.5))],
    "column": [("wall", (-0.1, 0.0, -0.1), (0.1, 1.0, 0.1))],
    "crate": [("crate", (-0.25, 0.0, -0.25), (0.25, 0.5, 0.25))],
    # two posts and the lintel over them
    "wall-doorway": [
        ("frame", (-0.15, 0.0, -0.5), (0.15, 1.0, -0.3)),
        ("frame", (-0.15, 0.0, 0.3), (0.15, 1.0, 0.5)),
        ("frame", (-0.15, 0.8, -0.3), (0.15, 1.0, 0.3)),
    ],
    # a plate with a bull's eye that stands out of both its faces
    "target": [
        ("plate", (-0.05, 0.0, -0.25), (0.05, 0.5, 0.25)),
        ("red", (-0.062, 0.17, -0.08), (0.062, 0.33, 0.08)),
    ],
    "supply-crate": [("supply", (-0.52, -0.02, -0.5), (0.27, 0.21, 0.5))],
    # a barrel and a grip
    "blaster": [
        ("metal", (-0.05, 0.0, -0.37), (0.05, 0.15, 0.43)),
        ("metal", (-0.04, -0.27, -0.3), (0.04, 0.0, -0.15)),
    ],
    "target-small": [("red", (-0.05, -0.1, -0.1), (0.05, 0.1, 0.1))],
}

# each face of a box as its outward normal and its four corners, anticlockwise
# seen from outside; 0 is the least of an axis and 1 the most
FACES = [
    ((0.0, 0.0, 1.0), ((0, 0, 1), (1, 0, 1), (1, 1, 1), (0, 1, 1))),
    ((0.0, 0.0, -1.0), ((1, 0, 0), (0, 0, 0), (0, 1, 0), (1, 1, 0))),
    ((1.0, 0.0, 0.0), ((1, 0, 1), (1, 0, 0), (1, 1, 0), (1, 1, 1))),
    ((-1.0, 0.0, 0.0), ((0, 0, 0), (0, 0, 1), (0, 1, 1), (0, 1, 0))),
    ((0.0, 1.0, 0.0), ((0, 1, 1), (1, 1, 1), (1, 1, 0), (0, 1, 0))),
    ((0.0, -1.0, 0.0), ((0, 0, 0), (1, 0, 0), (1, 0, 1), (0, 0, 1))),
]


def padded(chunk, filling):
    return chunk + filling * (-len(chunk) % 4)


def write_piece(path, name, boxes):
    positions = []
    normals = []
    coordinates = []
    indices = []
    for colour, least, most in boxes:
        # the middle of the pixel of the colour, so that every corner of
        # the box reads that pixel and nothing of its neighbours
        number = COLOUR_NAMES.index(colour)
        u = (number % COLORMAP_WIDTH + 0.5) / COLORMAP_WIDTH
        v = (number // COLORMAP_WIDTH + 0.5) / COLORMAP_HEIGHT
        for normal, corners in FACES:
            first = len(positions)
            for corner in corners:
                positions.append(tuple(most[axis] if corner[axis] else least[axis] for axis in range(3)))
                normals.append(normal)
                coordinates.append((u, v))
            indices += [first, first + 1, first + 2, first, first + 2, first + 3]

    parts = [
        b"".join(struct.pack("<3f", *position) for position in positions),
        b"".join(struct.pack("<3f", *normal) for normal in normals),
        b"".join(struct.pack("<2f", *coordinate) for coordinate in coordinates),
        padded(struct.pack(f"<{len(indices)}H", *indices), b"\x00"),
    ]
    views = []
    offset = 0
    for part, target in zip(parts, (34962, 34962, 34962, 34963)):
        views.append({"buffer": 0, "byteOffset": offset, "byteLength": len(part), "target": target})
        offset += len(part)
    buffer = b"".join(parts)

    count = len(positions)
    document = {
        "asset": {"version": "2.0", "generator": "tools/make-test-game-assets.py"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"name": name, "mesh": 0}],
        "meshes": [{
            "name": name,
            "primitives": [{
                "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
                "indices": 3,
                "material": 0,
            }],
        }],
        "materials": [{
            "name": "colormap",
            "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}, "metallicFactor": 0.0, "roughnessFactor": 1.0},
        }],
        "textures": [{"source": 0, "sampler": 0}],
        # nearest, so that a pixel of the image is one colour to its edge
        "samplers": [{"magFilter": 9728, "minFilter": 9728}],
        "images": [{"uri": "colormap.png"}],
        "accessors": [
            {
                "bufferView": 0, "componentType": 5126, "count": count, "type": "VEC3",
                "min": [min(position[axis] for position in positions) for axis in range(3)],
                "max": [max(position[axis] for position in positions) for axis in range(3)],
            },
            {"bufferView": 1, "componentType": 5126, "count": count, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": count, "type": "VEC2"},
            {"bufferView": 3, "componentType": 5123, "count": len(indices), "type": "SCALAR"},
        ],
        "bufferViews": views,
        "buffers": [{"byteLength": len(buffer)}],
    }

    text = padded(json.dumps(document, separators=(",", ":")).encode(), b" ")
    binary = padded(buffer, b"\x00")
    length = 12 + 8 + len(text) + 8 + len(binary)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        struct.pack("<4sII", b"glTF", 2, length)
        + struct.pack("<I4s", len(text), b"JSON") + text
        + struct.pack("<I4s", len(binary), b"BIN\x00") + binary
    )


kit = ASSETS / "models/kit"
write_png(kit / "colormap.png", COLORMAP_WIDTH, COLORMAP_HEIGHT,
          lambda x, y: COLOURS[COLOUR_NAMES[y * COLORMAP_WIDTH + x]])
for piece, piece_boxes in PIECES.items():
    write_piece(kit / f"{piece}.glb", piece, piece_boxes)


# The textures, 32 pixels square, one repeat of each.

SIZE = 32


def brick(x, y):
    # four courses of bricks, every other one set off by half a brick, with
    # mortar of one pixel between them
    course = y // 8
    along = (x + (8 if course % 2 else 0)) % 16
    if y % 8 == 7 or along == 15:
        return 190, 185, 175
    return 165, 70, 50


def concrete(x, y):
    # grey, with a darker line along the left and the top of every repeat
    if x == 0 or y == 0:
        return 110, 110, 112
    return 160, 160, 162


def wood(x, y):
    # four planks side by side, each a shade of its own, with a dark gap
    if x % 8 == 7:
        return 70, 45, 25
    return ((150, 105, 60), (165, 115, 65), (140, 95, 55), (158, 110, 62))[x // 8]


write_png(ASSETS / "textures/brick.png", SIZE, SIZE, brick)
write_png(ASSETS / "textures/concrete.png", SIZE, SIZE, concrete)
write_png(ASSETS / "textures/wood.png", SIZE, SIZE, wood)


# The sound: half a second at 220 Hz, which is a whole number of waves, so
# that the end meets the start when it loops. One channel of 16 bits at
# 22,050 samples a second.

RATE = 22050
SAMPLES = RATE // 2
samples = b"".join(
    struct.pack("<h", round(8000 * math.sin(2 * math.pi * 220 * sample / RATE))) for sample in range(SAMPLES)
)
sound = ASSETS / "sounds/tone.wav"
sound.parent.mkdir(parents=True, exist_ok=True)
sound.write_bytes(
    b"RIFF" + struct.pack("<I", 36 + len(samples)) + b"WAVE"
    + b"fmt " + struct.pack("<IHHIIHH", 16, 1, 1, RATE, RATE * 2, 2, 16)
    + b"data" + struct.pack("<I", len(samples)) + samples
)

print(f"wrote the kit, the textures, and the sound of {ASSETS}")
