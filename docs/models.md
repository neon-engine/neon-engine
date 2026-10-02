# Models

This note records how Neon Engine reads a model file, what of the file
reaches the renderer and the physics, what is left out, and what is still
open.

**Current decision:** **glTF is the model format of the engine**, as `.glb`
or `.gltf`. A shipped runtime reads glTF and nothing else, with a loader of
its own; the editor imports every format [assimp](https://github.com/assimp/assimp)
reads and converts it to glTF (#98), so that assimp ships with the editor and
never with a game, where it would weigh on a web build above all (#69).

Today the runtime still reads through assimp, so `.obj` and every other
format it knows load as well, and the models made for the engine are `.obj`:
a leftover of the example code the renderer began with, not a format of the
engine. They are converted to `.glb` when the editor converts (#98), or
before, and the loader of the runtime is written against glTF alone. Until
then, what is written here about `.obj` describes the leftover, not the
plan.

## How a model is read

| Step | What happens |
|---|---|
| The file | `Model::LoadModel()` hands assimp an `IOSystem` that opens every file through `FileSystemContext::ReadBytes`: the model, and anything it names, such as the `.mtl` of an `.obj`. No native path leaves the file system |
| The scene | assimp triangulates every face and flips the texture coordinates, so that `0, 0` is the top left of an image, as the textures are uploaded. The glTF importer of assimp flips them the other way first, so a GLB ends up the same way round as an `.obj` |
| The materials | Each material of the file becomes a `ModelMaterial`: its diffuse and specular textures, and its base colour factor. See [materials](#what-of-a-material-is-read) |
| The nodes | The tree of nodes is walked from the root, with the transform of every node under those above it. Each mesh of a node is handed to the backend with that transform, which bakes it into the vertices: the renderer draws a model as one piece and has no nodes of its own |
| The meshes | A backend makes a mesh of its own from the positions, normals, texture coordinates, and indices. The Vulkan backend uploads them; the physics keeps the positions for a `mesh` or `convex_hull` collider |
| The size | A model is drawn at its own size and around its own origin, as the file says, unless the `Renderable` asks for a `fit` of `unit`, which moves it to the origin and scales it so that its longest side is 1. See [the size of a model](#the-size-of-a-model) |

A node that mirrors its mesh, with a transform whose determinant is below 0,
turns its triangles inside out. The loader reverses the winding of every
triangle of such a mesh, so that the front still faces out and culling still
shows it. A `Transform` that mirrors an entity is handled by the renderer
instead, see [culling](vulkan-renderer.md).

## The size of a model

**Decision (#190):** a model is drawn at its own size and around its own
origin, as the file says. That is what glTF means, a piece of a kit is in
metres and stands on its origin, and what the editor's conversion (#98)
will assume. A `Renderable` says otherwise with `fit`:

| `fit` | What is drawn | For |
|---|---|---|
| `none` | The model as the file says it, in its units and around its origin. The default | glTF, and every model prepared for the engine |
| `unit` | The model moved so that its middle lies at the origin, and scaled so that its longest side is 1. The `scale` of the `Transform` then gives it a size | A model that is not in metres, such as the `.obj` leftovers of the demo scenes |

Until the first change of #190, every model got what `unit` does, and a
scene had to write the longest side of every kit piece as its `scale` and
lift it by half its height. The demo scenes keep `fit: unit` on every
`.obj`, so that they draw as they did; the prototype level stands its kit
pieces on the floor at their own size.

The physics reads the same choice: a `Collider` of kind `mesh` or
`convex_hull` carries a `fit` of its own, which is to be that of the
`Renderable` that draws the model, so that the shape lies where the model
is drawn. See [physics.md](physics.md). A mesh built at run time by a
`Geometry` is always drawn as it is, see [geometry.md](geometry.md).

In the code, `ModelFit` is `neon/render/model-fit.hpp`; `Model` takes it
and `Model::ComputeNormalizationMatrix` gives the identity for `None`.

## What of a material is read

| From the file | Where it goes | Note |
|---|---|---|
| The diffuse texture (`map_Kd` of an `.obj`, `baseColorTexture` of glTF) | The first texture of the material | Shown when the `Renderable` names no `textures` of its own |
| The specular texture (`map_Ks`) | The second texture | glTF has none; its `metallicRoughnessTexture` is not read (#59) |
| The base colour factor (`baseColorFactor` of glTF) | Multiplied into `material.color` of the `Renderable` | The `Kd` of an `.obj` is not read: the scenes set the colour themselves, and a grey `Kd` would darken them |
| An image kept inside the file (`*0` in assimp, a `bufferView` image of a GLB) | The texture is made from the bytes of the image | Only PNG and JPEG, as glTF allows. Raw pixels in a file are warned about and left out |
| An image file next to the model (`Textures/colormap.png`) | A virtual path from the folder of the model, `assets://external/kenney/prototype-kit/Textures/colormap.png` | The file has to be where the model says, with the letter case the model uses |

The scene has the last word. A `Renderable` that lists `textures` shows
those and not the model's. Its `material.color` is multiplied with the
model's factor, so white, the default, shows the model as the file means it.
What else a `Renderable` says, `shininess`, `alpha_mode`, and
`double_sided`, comes from the scene only; the `doubleSided` of a glTF
material is not read, see [open questions](#open-questions).

A model with several materials is drawn with the material of its first mesh.
The renderer binds one material per entity, and a model is one entity. The
log says so, once per model, at the level of information. The kit pieces
used so far have one material each.

## What of a GLB is not used

| In the file | What happens | Planned |
|---|---|---|
| Animations | Read by assimp, not played. The log says the model has them | Skeletal animation (#149) |
| Skins and skeletons | The mesh is drawn in its bind pose | #149 |
| Cameras and lights | Left out. The log says the model has them | A scene places its own |
| Vertex colours | Read, warned about, and not shown: the vertex layout of the shaders has no colour | Open, see below |
| Metalness and roughness | Not read. `basic-lit` has shininess and a specular texture | Physically based materials (#59) |
| Tangents, a second set of texture coordinates | Not read | Normal maps come with #59 |
| `KHR_texture_transform` | assimp reads it; only a transform that is the identity has been seen | |

## The kits

The pieces of the prototype level (#182) come from Kenney's Prototype Kit
and Blaster Kit, under CC0, in
[app/NeonRuntime/assets/external/kenney](../app/NeonRuntime/assets/external/kenney).
Only the pieces a scene uses are in the repository, with the licence of each
kit and the texture every piece names, `Textures/colormap.png`. A piece is
Y-up and in metres, with its floor at `y = 0`, and is drawn the right way up
without any change. Each kit's `colormap.png` is a palette: the texture
coordinates of a piece point at a patch of one colour, so a piece that is
stretched by a `Transform` keeps its colours.

## Open questions

- **A glTF loader of the engine's own**, and assimp out of the runtime (#69,
  #98): the loader of the runtime reads `.glb` and `.gltf` through the file
  system — meshes, materials, textures, nodes, later skins and animations
  (#149) — and nothing else; the `.obj` models of the demos become `.glb`.
  When the editor converts a model it strips the animations out into glTF
  files of their own, so that an animation can be retargeted to another
  skeleton and reused (#149); the runtime reads a skeleton, a skinned mesh,
  and its animations as separate files. It writes every texture out as an
  image file next to the model too, whether the source embedded it or carried
  raw pixels, so that a texture is a file of its own that is shared, replaced,
  and compressed by the exporter.
- **`doubleSided` of glTF.** Kenney's kits mark every material double-sided,
  as their exporter does, though every piece is closed. Honouring it would
  draw the back of every triangle of every piece. The scene decides for now.
- **Vertex colours.** A kit that paints its vertices instead of a colormap
  would show white. Showing them needs a colour in the vertex layout and in
  the shaders.
- **Several materials.** A model whose meshes have different textures or
  colours needs one material per mesh in the renderer, or to be split into
  one model per material by the editor (#98).
- **The colour and the texture.** `basic-lit` shows either the colour or the
  texture of a material, by `use_textures`. glTF multiplies the two. A tinted
  texture, a red and a blue of the same piece, waits for #59.
