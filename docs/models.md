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
| The nodes | The tree of nodes is walked from the root, with the transform of every node under those above it. Each mesh of a node is handed to the backend with that transform, which bakes it into the vertices: the renderer draws a model as one piece and has no nodes of its own. The model notes which material each mesh uses, see [several materials](#several-materials) |
| The meshes | `Model::ReadVertices()` makes a `Vertex` of every position, normal, texture coordinate, and vertex colour of a mesh, white where the mesh has no colours. A backend makes a mesh of its own from them and the indices: the Vulkan backend uploads them, the physics keeps the positions for a `mesh` or `convex_hull` collider |
| The size | A model is drawn at its own size and around its own origin, as the file says, unless the `Renderable` asks for a `fit` of `unit`, which moves it to the origin and scales it so that its longest side is 1. See [the size of a model](#the-size-of-a-model) |

A model is read once for every path and fit, however many entities draw
it, and its textures once for every image; the renderer holds them as long
as one entity draws them, see
[what is shared](vulkan-renderer.md#what-is-shared). The physics reads the
points of a model once for every path and fit as well, and holds one shape
for every scale a collider uses them at, see [physics.md](physics.md).

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

## The colours of the vertices

**Decision (#192):** a `Vertex` carries a colour, four floats, and there is
one vertex layout for every model. A model that paints its vertices
(`COLOR_0` of glTF, which some kits use in place of a texture) has them
read, in linear light as glTF keeps them; a model without gets white on
every vertex, and a mesh built by `MeshBuilder` too. The shaders that light,
`pbr` and `basic-lit`, and `unlit` multiply the vertex colour into the base
colour, next to the material's `color` and the texture, as glTF defines the
base colour. White changes nothing, so a model without vertex colours is
drawn as before.

The cost is sixteen bytes a vertex for every model, also those without
colours. A second layout and a second pipeline per shader for models
without would save them, and was not worth the variant: the models of a
game are small next to its textures, and one layout keeps every shader and
every pipeline the same. The alpha of a vertex colour is not read: assimp
leaves it at 0 for a colour of three components and does not say how many
the file had, so every vertex of a file gets 1, and the shaders multiply
the red, green, and blue alone. A loader of the engine's own (#69) can read
it. `tests/runtime-vertex-colours` reads a painted model pixel by pixel.

## What of a material is read

| From the file | Where it goes | Note |
|---|---|---|
| The diffuse texture (`map_Kd` of an `.obj`, `baseColorTexture` of glTF) | The first texture of the material | Shown when the `Renderable` names no `textures` of its own |
| The specular texture (`map_Ks`) | The second texture | glTF has none; its `metallicRoughnessTexture` is not read (#59) |
| The base colour factor (`baseColorFactor` of glTF) | Multiplied into `material.color` of the `Renderable` | The `Kd` of an `.obj` is not read: the scenes set the colour themselves, and a grey `Kd` would darken them |
| An image kept inside the file (`*0` in assimp, a `bufferView` image of a GLB) | The texture is made from the bytes of the image | Only PNG and JPEG, as glTF allows. Raw pixels in a file are warned about and left out |
| An image file next to the model (`Textures/colormap.png`) | A virtual path from the folder of the model, `assets://external/kenney/prototype-kit/Textures/colormap.png` | The file has to be where the model says, with the letter case the model uses |

| `doubleSided` of glTF | `ModelMaterial::double_sided` | Taken when the `Renderable` leaves `material.double_sided` at `model`, see [doubleSided](#doublesided) |

The scene has the last word. A `Renderable` that lists `textures` shows
those and not the model's. Its `material.color` is multiplied with the
model's factor, so white, the default, shows the model as the file means it.
Its `material.double_sided` is `model` unless written, which takes the
file's. What else a `Renderable` says, `shininess` and `alpha_mode`, comes
from the scene only. A model with several materials takes the scene's word
on every one of them, see [several materials](#several-materials).

## doubleSided

**Decision (#194):** the loader reads `doubleSided` of a glTF material into
`ModelMaterial::double_sided`, and a scene overrides it when it says so.
`material.double_sided` of a `Renderable` is one of three words:

| `double_sided` | What is drawn | For |
|---|---|---|
| `model` | What the file says. One side for a file that says nothing, an `.obj`, and a mesh built by a `Geometry`. The default | Every model, as the standard means it |
| `always` | Both sides of every triangle, whatever the file says | A leaf, a flag, or a pane of glass made as one surface |
| `never` | One side, whatever the file says | A closed piece whose exporter marked it double-sided out of habit |

Reading the file is correct by the standard, and costs nothing on a model
that says nothing. Kenney's kits mark every material double-sided, as their
exporter does, though every piece is closed: honouring it draws the back of
every triangle of every piece, which the depth test then discards. The
scene decides whether that matters: `never` on such a piece culls as
before, and the editor's conversion (#98) is the place to clear the flag
for a whole kit. `tests/runtime-vertex-colours` draws a model the file marks
double-sided from behind, with and without the scene's `never`.

## Several materials

**Decision (#193):** a model whose meshes use different materials is drawn
with each of them, by the renderer, inside one render object. The other
way, splitting such a model into one model per material when the editor
prepares it (#98), waits for an editor, and a character or a vehicle with
a material for its skin and one for its clothes is drawn whole until then.
Kenney's kits are one material per file, so the prototype is not touched.

The loader keeps, for every mesh in the order the backend gets them, the
index of the material the mesh uses, and the list of the materials any mesh
uses, each once, in the order of first use: `Model::GetMaterialOfMesh()`
and `Model::GetUsedMaterials()`. A renderer makes one material of its own
for each of the used materials, and draws the meshes of each with it, in
that order; a model with one material is one draw, as it always was. The
Vulkan renderer does this with a descriptor set and an entry of object data
per material, see [the render object's materials](vulkan-renderer.md#what-is-shared).

The `Renderable` of the scene stays the override of the whole model:

| In the `Renderable` | Applies to |
|---|---|
| `material.color` | Every material: it multiplies the base colour factor of each |
| `textures` | The first material alone, whose own textures it replaces. The others keep what the file names. The first material is that of the first mesh, which is the only one a model with one material has |
| `shader`, `scale_textures`, `use_textures`, `shininess`, `metallic`, `roughness`, `alpha_mode` | Every material |
| `material.double_sided` | Every material when `always` or `never`. `model` takes the `doubleSided` of each material of the file for its own meshes |

`tests/runtime-materials` draws a model of two boxes, a red and a blue one
with a material each, and reads both colours as they are, lit, and
multiplied by a colour of the scene. The model, `coloured-boxes.glb`, is
written by `tools/make-coloured-boxes.py`. A material per mesh in the file
of the scene, so that one mesh of a model can be given another texture
without touching the file, is not planned: that is what the editor's
conversion (#98) is for.

## What of a GLB is not used

| In the file | What happens | Planned |
|---|---|---|
| Animations | Read by assimp, not played. The log says the model has them | Skeletal animation (#149) |
| Skins and skeletons | The mesh is drawn in its bind pose | #149 |
| Cameras and lights | Left out. The log says the model has them | A scene places its own |
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
- **The colour and the texture.** `basic-lit` shows either the colour or the
  texture of a material, by `use_textures`. glTF multiplies the two. A tinted
  texture, a red and a blue of the same piece, waits for #59.
