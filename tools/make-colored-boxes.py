#!/usr/bin/env python3
"""Writes tests/game/assets/models/colored-boxes.glb: a model whose
meshes use different materials, for tests/runtime-materials.

Two boxes, 0.8 on every side, one at x = -1 and one at x = 1, each a mesh
of its own on a node of its own, with a material of its own: the left one
red and the right one blue, as the baseColorFactor of each material. The
materials have no texture and are matte dielectrics, so that what the pbr
shader makes of each color is a known number. The colors are in linear
light, as glTF keeps a factor.

Run from the root of the repository:

    python3 tools/make-colored-boxes.py
"""

import json
import struct
from pathlib import Path

BOXES = [("red", -1.0, (1.0, 0.0, 0.0, 1.0)), ("blue", 1.0, (0.0, 0.0, 1.0, 1.0))]
HALF = 0.4

# each face as its outward normal and its four corners, counterclockwise seen
# from outside
FACES = [
    ((0.0, 0.0, 1.0), ((-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1))),
    ((0.0, 0.0, -1.0), ((1, -1, -1), (-1, -1, -1), (-1, 1, -1), (1, 1, -1))),
    ((1.0, 0.0, 0.0), ((1, -1, 1), (1, -1, -1), (1, 1, -1), (1, 1, 1))),
    ((-1.0, 0.0, 0.0), ((-1, -1, -1), (-1, -1, 1), (-1, 1, 1), (-1, 1, -1))),
    ((0.0, 1.0, 0.0), ((-1, 1, 1), (1, 1, 1), (1, 1, -1), (-1, 1, -1))),
    ((0.0, -1.0, 0.0), ((-1, -1, -1), (1, -1, -1), (1, -1, 1), (-1, -1, 1))),
]


def floats(triples):
    return b"".join(struct.pack("<3f", *triple) for triple in triples)


def padded(chunk, filling):
    return chunk + filling * (-len(chunk) % 4)


buffer = b""
views = []
accessors = []
meshes = []
nodes = []
materials = []

for name, center, color in BOXES:
    positions = []
    normals = []
    indices = []
    for normal, corners in FACES:
        first = len(positions)
        for x, y, z in corners:
            positions.append((center + x * HALF, y * HALF, z * HALF))
            normals.append(normal)
        indices += [first, first + 1, first + 2, first, first + 2, first + 3]

    position_bytes = floats(positions)
    normal_bytes = floats(normals)
    index_bytes = b"".join(struct.pack("<H", index) for index in indices)

    attributes = {}
    for data, count, kind, component, attribute in (
        (position_bytes, len(positions), "VEC3", 5126, "POSITION"),
        (normal_bytes, len(normals), "VEC3", 5126, "NORMAL"),
        (index_bytes, len(indices), "SCALAR", 5123, None),
    ):
        views.append({"buffer": 0, "byteOffset": len(buffer), "byteLength": len(data)})
        buffer += padded(data, b"\0")
        accessor = {"bufferView": len(views) - 1, "componentType": component, "count": count, "type": kind}
        if attribute == "POSITION":
            accessor["min"] = [min(p[i] for p in positions) for i in range(3)]
            accessor["max"] = [max(p[i] for p in positions) for i in range(3)]
        accessors.append(accessor)
        if attribute is not None:
            attributes[attribute] = len(accessors) - 1

    materials.append({
        "name": name,
        "pbrMetallicRoughness": {"baseColorFactor": list(color), "metallicFactor": 0.0, "roughnessFactor": 1.0},
    })
    meshes.append({
        "name": f"{name}-box",
        "primitives": [{"attributes": attributes, "indices": len(accessors) - 1, "material": len(materials) - 1}],
    })
    nodes.append({"mesh": len(meshes) - 1, "name": f"{name}-box"})

gltf = {
    "asset": {"version": "2.0", "generator": "tools/make-colored-boxes.py of Neon Engine"},
    "scene": 0,
    "scenes": [{"nodes": list(range(len(nodes)))}],
    "nodes": nodes,
    "meshes": meshes,
    "materials": materials,
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

target = Path(__file__).resolve().parent.parent / "tests/game/assets/models/colored-boxes.glb"
target.write_bytes(glb)
print(f"wrote {target} ({len(glb)} bytes)")
