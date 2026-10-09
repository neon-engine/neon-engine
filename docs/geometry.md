# Geometry built at run time

This note records how the engine builds meshes from numbers, in place of a
model from a file: for blocking out a level in a recipe, for tools that edit
levels in the editor, and for importers that read a map format and hand the
engine what it holds. What a model file brings is in
[models.md](models.md).

**Current decision:** a mesh is a plain structure of vertices and triangle
indices, `MeshData`, that anything may fill. `MeshBuilder` fills it from
shapes in meters - a box, a plane, a ramp, a prism from an outline, a
sphere, a cylinder, an upright quad, a tube along a curve - with flat normals and texture
coordinates projected so that a texture repeats once a meter whatever the
size of the surface. A `Geometry` component builds
one when a scene loads; its `Renderable` draws it with its shader and
textures, and a `Collider` without a model takes the same shape, so that what
is drawn is what collides. The renderer draws a built mesh as it is, in
meters, without the centering and scaling a file gets.

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
      shader: engine://shaders/pbr
      textures: [assets://textures/brick.png]
    RigidBody:
      kind: static
    Collider:
      shape: mesh
```

The scene of the tests that is built this way is
[blockout.scene.yml](../tests/game/assets/scenes/blockout.scene.yml):
a room seen from within, a platform, a crate, a ramp, and a step, with nothing from a model file.

**Geometry**

| Name | Holds | Default |
|---|---|---|
| `shape` | `box`, `plane`, `ramp`, `prism`, `sphere`, `cylinder`, `quad`, or `tube` | `box` |
| `size` | A box, a ramp, a sphere, or a cylinder in x, y, and z; a plane in x and z; a quad in x and y; the height of a prism in y; the thickness of a tube in x. Meters | `[1, 1, 1]` |
| `segments` | How many quads a plane is cut into along each side. A plane alone | `1` |
| `sides` | How many faces go round a sphere, a cylinder, or a tube, at least three. A sphere has half as many rings from pole to pole. A sphere, a cylinder, and a tube alone | `24` |
| `outline` | The outline of a prism on the ground, as pairs of x and z, at least three, going round either way. A prism alone | None |
| `points` | The control points of the curve a tube follows, as triples of x, y, and z: four for the first piece, three for every further one, see [curves.md](curves.md#a-path). A tube alone | None |
| `texels_per_meter` | How often a texture repeats over one meter of surface | `1` |
| `smooth` | Whether the normals are smoothed over shared vertices, for a plane that is to look round. A sphere is lit as a ball with it, a cylinder as a round column whose caps stay flat, and a tube as a round pipe | `false` |
| `inside` | Whether the faces point inward, for a box or a prism that is a room seen from within, a sphere that is a dome, or a cylinder that is a well | `false` |

A `Renderable` with a `Geometry` leaves `model` out. A `Collider` of kind
`mesh` or `convex_hull` with no `model` takes the entity's `Geometry`; one
that names a model reads the model, as before.

## The shapes

| Shape | What it is | Faces |
|---|---|---|
| Box | `size` in three axes, centered on the entity | 6 quads, outward |
| Plane | `size` in x and z, facing up, cut into `segments^2` quads. For ground: a floor, a terrain. Not for a platform with an edge a character can meet, which is a box, see [physics.md](physics.md#shapes) | Up only; no thickness, so a platform is seen from above and not from below |
| Ramp | A wedge of `size` that rises along z, its low edge at the front (positive z) and its full height at the back | The slope, the back, the bottom, two triangular sides |
| Prism | An outline on the ground pulled up by `size.y`, floor and ceiling included | Walls, ceiling, floor. The outline has to be convex: the floor and the ceiling are fans from its first point |
| Sphere | What fills `size`, centered on the entity: a ball when the three lengths are the same, an ellipsoid otherwise | `sides` round, `sides / 2` rings: a triangle at each pole, quads between. Flat faces, or with `smooth` the normals of the round surface on shared vertices |
| Cylinder | Along y, filling `size`, centered on the entity. With few `sides` it is a column of that many faces: 6 is a hexagon | `sides` quads round, and a cap at each end. `smooth` rounds the side and keeps the edges of the caps hard |
| Quad | `size` in x and y, upright, facing +z, centered on the entity | One quad seen from its front, on which a texture lies once, with its top at the top: what shows a picture, a [surface](user-interface.md#surfaces), or what a camera sees. `texels_per_meter` is not read |
| <a id="tube"></a>Tube | A circle of thickness `size.x` swept along the curve of `points`, a path of cubic Bézier pieces, see [curves.md](curves.md). The curve is cut into straight pieces that stay within a fiftieth of the thickness of it: few where it is straight, more where it bends | `sides` quads round every piece, and a cap at each end. The rings do not twist along the curve. `smooth` rounds the side. It lays its texture itself, round it and along it, `texels_per_meter` times a meter |

A pipe that rises out of the floor and turns to run along a wall:

```yaml
Geometry:
  shape: tube
  size: [0.2, 0, 0]
  sides: 12
  smooth: true
  points: [0, 0, 0,  0, 1, 0,  0, 2, 0,  1, 2, 0]
```

A sphere of `size: 1` is the `sphere` of a `Collider` with its default
radius of 0.5, and a cylinder of `size: [1, 2, 1]` its `cylinder`, so a ball
that rolls is a `Geometry` and a `Collider` that name the same shape. A
quad of `size: 1` is the square a `UiSurface` is pointed at on, see
[user-interface.md](user-interface.md#pointing-at-a-screen-in-the-world).

Every face is wound counterclockwise seen from outside, which is what back-face
culling expects, and carries the flat normal of its face. `inside` turns every
face round, for a room: the outside is then culled, as the inside of a crate
is. The order of a prism's outline does not matter; the builder orients it by
its signed area.

## Textures on built geometry

A built mesh has no texture coordinates of its own, so they are projected: a
vertex takes the two coordinates across the axis its normal points most along
 -  a wall facing z takes x and y, a floor x and z - times `texels_per_meter`.
A texture then repeats once a meter, or as often as asked, on every surface
alike, and a wall of twenty meters is not one stretched brick. The signs are
chosen so that a texture reads upright on every wall seen from its front, and
north-up on a floor; the check in `tests/runtime-geometry` renders the room
and reads it pixel by pixel.

A sphere and the side of a cylinder are projected the same way, which is
right for a plain color and shows seams with a texture, where the axis a
normal points most along changes. A quad and a tube are the shapes that lay
their texture themselves.

## In code

| Piece | Location | Role |
|---|---|---|
| `MeshData` | neon-core, `neon/geometry/mesh-data.hpp` | Vertices and triangle indices, nothing else. A vertex has a color, white from the builder, that a tool or an importer sets to paint a mesh by vertex (#192) |
| `MeshBuilder` | `neon/geometry/mesh-builder.hpp` | `AddBox`, `AddPlane`, `AddRamp`, `AddPrism`, `AddSphere`, `AddCylinder`, `AddUprightQuad`, `AddTube`, `AddQuad`, `InsideOut`, `Build` |
| `AppendTube` | `neon/geometry/tube.hpp` | A tube along a line of points, straight into a `MeshData`: what `AddTube` calls, and what writes a rope anew every frame without the builder |
| `ComputeFlatNormals`, `ComputeSmoothNormals` | `neon/geometry/mesh-normals.hpp` | Flat splits shared vertices so every edge is hard; smooth averages by area |
| `ProjectUvs` | `neon/geometry/mesh-uvs.hpp` | The projection above |
| `Geometry`, `GeometryShape` | `neon/world-system/ecs/components/` | The component, described with reflection |
| `GeometryBuilding` | `neon/world-system/ecs/systems/` | Builds the mesh once into `RenderInfo::mesh`; `Build()` is what the physics calls too |
| `RenderInfo::mesh` | `neon/render/render-info.hpp` | A shared `MeshData` the renderer draws in place of `model_path` |
| `VK_Model` from a mesh | neon-vulkan | Uploads the mesh as it is, with a `ModelFit` of `None`, whatever a file would get |

## A mesh that changes

A mesh may change after it was first drawn: whoever changed it counts
`RenderInfo::mesh_version` up, and `RenderSubmission` hands the mesh to the
renderer again through `UpdateRenderObjectMesh` before the entity is drawn.
The Vulkan renderer writes a mesh with as many vertices and triangles as
before over the old one, which is a copy into memory it already has and is
what a [rope](physics.md#ropes) costs every frame; a mesh of another size
gets new buffers. A frame is done before the next one starts, so nothing
draws from the buffers while they are written.

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
- **Changing a Geometry while the game runs.** The mesh of a `Geometry` is
  built once, when the entity is first drawn; a changed component is not
  rebuilt yet. The renderer does take a mesh that changed, see below, so
  what a tool that edits a level live still needs is the rebuild.
- **Textures on round shapes.** A sphere and a cylinder take the projection
  of the flat shapes, which shows seams. Coordinates of their own, round
  the axis and from pole to pole, are the next step once a round shape is
  to carry a texture.
- **Painting a block-out by vertex.** A `Vertex` has a color and the
  shaders show it (#192), but a `Geometry` has no way to set one yet; a
  second texture set and tangents for normal maps (#106) are open too.
