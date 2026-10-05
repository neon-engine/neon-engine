#!/usr/bin/env python3
"""Writes the scenes of tests/runtime-sharing: crates that share one material,
and crates of distinct colours that need more descriptor sets than one pool
holds. Run from the repository root; the scenes are committed."""
import random

ROOT = "tests/game/assets/scenes"
COUNT = 300

def header(name, what):
    return [f"# {what} Written by tools/make-sharing-scenes.py for", "# tests/runtime-sharing; do not edit by hand.",
            f"scene: {name}", "version: 1", "", "entities:",
            "  - name: camera", "    components:", "      Transform:", "        position: [0, 6, 22]", "        rotation: [-14, 0, 0]", "      Camera: Default",
            "  - name: sun", "    components:", "      Transform: Default", "      Light:", "        type: direction", "        direction: [-0.3, -1, -0.2]",
            "        ambient: [0.35, 0.35, 0.4]", "        diffuse: [0.6, 0.6, 0.55]", "        specular: [0.2, 0.2, 0.2]"]

def crate(n, x, y, z, color=None):
    lines = [f"  - name: crate-{n}", "    prefab: assets://prefabs/sharing-crate.prefab.yml", "    components:",
             "      Transform:", f"        position: [{x:.2f}, {y:.2f}, {z:.2f}]"]
    if color is not None:
        lines += ["      Renderable:", "        material:", f"          color: [{color[0]:.3f}, {color[1]:.3f}, {color[2]:.3f}]"]
    return lines

random.seed(3)
same = header("sharing-same", f"{COUNT} crates with one and the same material, drawn with one.")
distinct = header("sharing-distinct", f"{COUNT} crates of distinct colours, one material each, more than one pool holds.")
for n in range(COUNT):
    x = (n % 20 - 9.5) * 1.1
    y = (n // 20) * 1.1
    same += crate(n, x, y, 0)
    distinct += crate(n, x, y, 0, (random.uniform(0.2, 1), random.uniform(0.2, 1), random.uniform(0.2, 1)))
open(f"{ROOT}/sharing-same.scene.yml", "w").write("\n".join(same) + "\n")
open(f"{ROOT}/sharing-distinct.scene.yml", "w").write("\n".join(distinct) + "\n")
print("written")
