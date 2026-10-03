# Geometry built at run time

This note records how the engine builds meshes from numbers, in place of a
model from a file: for blocking out a level in a recipe, for tools that edit
levels in the editor, and for importers that read a map format and hand the
engine what it holds. What a model file brings is in
[models.md](models.md).

**Current decision:** a mesh is a plain structure of vertices and triangle
indices, `MeshData`, that anything may fill. `MeshBuilder` fills it from
shapes in metres — a box, a plane, a ramp, a prism from an outline — with
flat normals and texture coordinates projected so that a texture repeats
once a metre whatever the size of the surface. A `Geometry` component builds
one when a scene loads; its `Renderable` draws it with its shader and
textures, and a `Collider` without a model takes the same shape, so that what
is drawn is what collides. The renderer draws a built mesh as it is, in
metres, without the centring and scaling a file gets.

## In a recipe

```yaml
- name: room
  components:
    Transform: Default
    Geometry:
      shape: prism
      size: [0, 3, 0]
      outline: [-4, -4, 4, -4, 4, 4, -4, 4]
      inside: true
    Renderable:
      shader: assets://shaders/pbr
      textures: [assets://textures/brick.png]
    RigidBody:
      kind: static
    Collider:
      shape: mesh
```

The scene of the runtime that is built this way is
[blockout.scene.yml](../app/NeonRuntime/assets/scenes/blockout.scene.yml):
a room seen from within, a platform, a crate, a ramp, and a step, with nothing from a model file.

**Geometry**

| Name | Holds | Default |
|---|---|---|
| `shape` | `box`, `plane`, `ramp`, or `prism` | `box` |
| `size` | A box or a ramp in x, y, and z; a plane in x and z; the height of a prism in y. Metres | `[1, 1, 1]` |
| `segments` | How many quads a plane is cut into along each side. A plane alone | `1` |
| `outline` | The outline of a prism on the ground, as pairs of x and z, at least three, going round either way. A prism alone | None |
| `texels_per_metre` | How often a texture repeats over one metre of surface | `1` |
| `smooth` | Whether the normals are smoothed over shared vertices, for a plane that is to look round | `false` |
| `inside` | Whether the faces point inward, for a box or a prism that is a room seen from within | `false` |

A `Renderable` with a `Geometry` leaves `model` out. A `Collider` of kind
`mesh` or `convex_hull` with no `model` takes the entity's `Geometry`; one
that names a model reads the model, as before.

## The shapes

| Shape | What it is | Faces |
|---|---|---|
| Box | `size` in three axes, centred on the entity | 6 quads, outward |
| Plane | `size` in x and z, facing up, cut into `segments²` quads. For ground: a floor, a terrain. Not for a platform with an edge a character can meet, which is a box, see [physics.md](physics.md#shapes) | Up only; no thickness, so a platform is seen from above and not from below |
| Ramp | A wedge of `size` that rises along z, its low edge at the front (positive z) and its full height at the back | The slope, the back, the bottom, two triangular sides |
| Prism | An outline on the ground pulled up by `size.y`, floor and ceiling included | Walls, ceiling, floor. The outline has to be convex: the floor and the ceiling are fans from its first point |

Every face is wound anticlockwise seen from outside, which is what back-face
culling expects, and carries the flat normal of its face. `inside` turns every
face round, for a room: the outside is then culled, as the inside of a crate
is. The order of a prism's outline does not matter; the builder orients it by
its signed area.

## Textures on built geometry

A built mesh has no texture coordinates of its own, so they are projected: a
vertex takes the two coordinates across the axis its normal points most along
— a wall facing z takes x and y, a floor x and z — times `texels_per_metre`.
A texture then repeats once a metre, or as often as asked, on every surface
alike, and a wall of twenty metres is not one stretched brick. The signs are
chosen so that a texture reads upright on every wall seen from its front, and
north-up on a floor; the check in `tests/runtime-geometry` renders the room
and reads it pixel by pixel.

## In code

| Piece | Location | Role |
|---|---|---|
| `MeshData` | neon-core, `neon/geometry/mesh-data.hpp` | Vertices and triangle indices, nothing else. A vertex has a colour, white from the builder, that a tool or an importer sets to paint a mesh by vertex (#192) |
| `MeshBuilder` | `neon/geometry/mesh-builder.hpp` | `AddBox`, `AddPlane`, `AddRamp`, `AddPrism`, `AddQuad`, `InsideOut`, `Build` |
| `ComputeFlatNormals`, `ComputeSmoothNormals` | `neon/geometry/mesh-normals.hpp` | Flat splits shared vertices so every edge is hard; smooth averages by area |
| `ProjectUvs` | `neon/geometry/mesh-uvs.hpp` | The projection above |
| `Geometry`, `GeometryShape` | `neon/world-system/ecs/components/` | The component, described with reflection |
| `GeometryBuilding` | `neon/world-system/ecs/systems/` | Builds the mesh once into `RenderInfo::mesh`; `Build()` is what the physics calls too |
| `RenderInfo::mesh` | `neon/render/render-info.hpp` | A shared `MeshData` the renderer draws in place of `model_path` |
| `VK_Model` from a mesh | neon-vulkan | Uploads the mesh as it is, with a `ModelFit` of `None`, whatever a file would get |

A tool or an importer fills a `MeshData` by hand or through the builder, puts
it on a `Renderable` as `render_info.mesh`, and hands its points and
triangles to a `Collider` through the same path the component takes. It never
talks to Vulkan or to Jolt.

## Open questions

- **Concave outlines.** The fan triangulation of a prism's floor and ceiling
  is right for convex outlines only; an L-shaped room needs ear clipping, or
  is two prisms today.
- **Boolean operations and brushes.** A doorway cut out of a wall, as a map
  editor in the way of Quake's does with brushes, is the next step for a
  block-out tool. An importer of such a map format would produce one
  `MeshData` per brush or per material.
- **Welding and smoothing across pieces.** Two boxes that meet keep their own
  vertices; a terrain that wants smooth shading across tiles needs the plane's
  shared vertices and `smooth`, or a merge of pieces.
- **Changing a Geometry while the game runs.** The mesh is built once, when
  the entity is first drawn; a changed component is not rebuilt yet. A tool
  that edits a level live needs the rebuild and the renderer's object remade.
- **Painting a block-out by vertex.** A `Vertex` has a colour and the
  shaders show it (#192), but a `Geometry` has no way to set one yet; a
  second texture set and tangents for normal maps (#106) are open too.
