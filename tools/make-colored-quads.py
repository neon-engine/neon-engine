#!/usr/bin/env python3
"""Writes tests/game/assets/models/colored-quads.glb: a model whose
color is painted on its vertices, for tests/runtime-vertex-colors.

Three quads, 0.8 wide and 0.8 high, facing +z at x = -1, 0, and 1, one
red, one green, one blue, in one mesh with one material. The material has
no texture, is a matte dielectric, and is marked doubleSided, as a kit
exported from Unity marks every material. The colors are in linear light,
as glTF keeps COLOR_0.

Run from the root of the repository:

    python3 tools/make-colored-quads.py
"""

import json
import struct
from pathlib import Path

QUADS = [(-1.0, (1.0, 0.0, 0.0)), (0.0, (0.0, 1.0, 0.0)), (1.0, (0.0, 0.0, 1.0))]
HALF = 0.4

positions = []
normals = []
colors = []
indices = []
for center, color in QUADS:
    first = len(positions)
    # counterclockwise seen from +z, which is the front
    for x, y in ((-HALF, -HALF), (HALF, -HALF), (HALF, HALF), (-HALF, HALF)):
        positions.append((center + x, y, 0.0))
        normals.append((0.0, 0.0, 1.0))
        colors.append(color)
    indices += [first, first + 1, first + 2, first, first + 2, first + 3]


def floats(triples):
    return b"".join(struct.pack("<3f", *triple) for triple in triples)


def padded(chunk, filling):
    return chunk + filling * (-len(chunk) % 4)


position_bytes = floats(positions)
normal_bytes = floats(normals)
color_bytes = floats(colors)
index_bytes = b"".join(struct.pack("<H", index) for index in indices)
buffer = position_bytes + normal_bytes + color_bytes + padded(index_bytes, b"\0")

views = []
accessors = []
offset = 0
for data, count, kind, component in (
    (position_bytes, len(positions), "VEC3", 5126),
    (normal_bytes, len(normals), "VEC3", 5126),
    (color_bytes, len(colors), "VEC3", 5126),
    (index_bytes, len(indices), "SCALAR", 5123),
):
    views.append({"buffer": 0, "byteOffset": offset, "byteLength": len(data)})
    accessor = {"bufferView": len(views) - 1, "componentType": component, "count": count, "type": kind}
    if kind == "VEC3" and data is position_bytes:
        accessor["min"] = [min(p[i] for p in positions) for i in range(3)]
        accessor["max"] = [max(p[i] for p in positions) for i in range(3)]
    accessors.append(accessor)
    offset += len(padded(data, b"\0"))

gltf = {
    "asset": {"version": "2.0", "generator": "tools/make-colored-quads.py of Neon Engine"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{"mesh": 0, "name": "colored-quads"}],
    "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1, "COLOR_0": 2}, "indices": 3, "material": 0}]}],
    "materials": [{"name": "painted", "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 1.0}, "doubleSided": True}],
    "buffers": [{"byteLength": len(buffer)}],
    "bufferViews": views,
    "accessors": accessors,
}

json_chunk = padded(json.dumps(gltf, separators=(",", ":")).encode(), b" ")
binary_chunk = padded(buffer, b"\0")
length = 12 + 8 + len(json_chunk) + 8 + len(binary_chunk)
glb = (
    struct.pack("<III", 0x46546C67, 2, length)
    + struct.pack("<II", len(json_chunk), 0x4E4F534A) + json_chunk
    + struct.pack("<II", len(binary_chunk), 0x004E4942) + binary_chunk
)

target = Path(__file__).resolve().parent.parent / "tests/game/assets/models/colored-quads.glb"
target.write_bytes(glb)
print(f"wrote {target} ({len(glb)} bytes)")
