# Vulkan renderer

This note is the plan for the Vulkan render backend. It records the decisions
taken, why, and the order of work. It is updated as the work proceeds.

**Status:** Vulkan is the engine's renderer. The OpenGL backend it replaced
has been removed. See [Current state](#current-state).

## Goals

1. Render in a window on macOS, Linux, and Windows, drawing at least what the
   earlier OpenGL backend drew. **This is the priority.**
2. Follow the engine's architecture. The backend sits behind the neon-core
   render interfaces. Nothing outside the backend sees a Vulkan type.
3. Later: a headless renderer as a feature of its own, with no window, no display,
   and no input devices, to make rendering testable in CI.

The pieces that headless rendering needs already exist, because they were
built first and are what the renderer is checked with during development.
Turning them into a finished feature is deferred.

Metal is a separate backend on its own branch, after this one.

## Why OpenGL was removed

The engine started with an OpenGL backend. It was removed once Vulkan drew the
same scenes.

| Reason | Detail |
|---|---|
| A dead end on macOS | Apple stopped at OpenGL 4.1 and deprecated it |
| Its limits were already felt | The lit shader exceeded Apple's uniform limit and had to be cut from 256 lights of each kind to 16 |
| Every shader had to be written twice | Once in GLSL 330 for OpenGL and once in GLSL 450 for Vulkan |
| It had bugs of its own | See [What changed in the picture](#what-changed-in-the-picture) |
| One renderer is less to maintain | One library, one set of shaders, no glad |

What was given up: a machine without a Vulkan driver cannot run the engine.
On macOS that means MoltenVK has to be present, until there is a Metal
renderer.

## Why Vulkan before Metal

| Reason | Detail |
|---|---|
| Headless | Vulkan can render to an image with no surface at all |
| Reach | One backend covers Linux and Windows natively, and macOS through MoltenVK |

## Dependencies

| Dependency | How it is supplied | Purpose |
|---|---|---|
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | Git submodule | The API declarations |
| [volk](https://github.com/zeux/volk) | Git submodule | Loads the Vulkan library at run time and resolves its functions |
| glslang | Build tool, installed on the build machine | Compiles shader sources to SPIR-V during the build |
| A Vulkan driver | Present on the machine that runs the app | See below |

Both submodules are pinned to the same Vulkan SDK release.

**Why volk instead of linking the Vulkan loader.** Linking needs the loader's
import library on the build machine, for every target. With volk the engine
needs only headers to build, which keeps the Windows cross-compile free of a
Vulkan SDK. The loader is found when the app starts.

**Drivers at run time**

| Platform | Driver |
|---|---|
| macOS | MoltenVK, with the Vulkan loader. `brew install molten-vk vulkan-loader` |
| Linux | The GPU vendor's driver, or Mesa lavapipe for software rendering with no GPU, which is what the Docker image uses |
| Windows | Ships with the GPU driver |

## Versions of Vulkan

The engine needs **Vulkan 1.1**. Frames are turned the right way up with a
viewport of negative height, which Vulkan 1.0 allows only with an extension
and 1.1 has as part of it. Nothing of 1.2 or later is used.

Which version is rendered with is the lowest of three:

| Who | What it says |
|---|---|
| The settings | `SettingsConfig::vulkan_version`, 1.3 unless `--vulkan-version` says otherwise |
| The loader of the driver | The highest version an instance can be made for. A loader of Vulkan 1.0 cannot say, and counts as 1.0 |
| The graphics card | The version in its properties. MoltenVK reports no more than the instance asked for |

A graphics card below 1.1 is passed over for one that has it. The log says
what was chosen:

```
Rendering with Apple M5 Max in Vulkan 1.3. Vulkan 1.3 was asked for, and the graphics card offers 1.3.357
```

The renderer stops, and says why, when any of the three is below 1.1:

| Below 1.1 | Message |
|---|---|
| What was asked for | `Vulkan 1.0 was asked for, but the engine needs Vulkan 1.1 at least`. Said before a driver is looked for |
| The loader | `The Vulkan driver offers Vulkan 1.0, but the engine needs Vulkan 1.1 at least. Is the graphics driver up to date?` |
| Every graphics card | `No graphics card offers Vulkan 1.1, which the engine needs at least.`, followed by the card that was found and its version |

`VK_ApiVersion` works the version out, apart from the device, so that its
unit tests need no graphics card.

## What the graphics card can do

`RenderSystem::GetCapabilities()` returns a
[`RenderCapabilities`](../lib/neon-core/neon/render/render-capabilities.hpp),
which holds no type of Vulkan. It is filled in when the renderer starts and
does not change. A settings menu reads it to offer only what works.

| Field | What it holds |
|---|---|
| `api_version` | The version that is rendered with |
| `device_api_version` | The version the graphics card offers, with its patch |
| `device_name` | The name of the graphics card |
| `max_texture_size` | The widest and highest texture, in pixels |
| `max_samples` | The most samples of anti-aliasing that color and depth both have. 1 is none |
| `max_anisotropy` | The most anisotropic filtering, or 0 when the graphics card has none |
| `present_modes` | The choices of vertical sync: `Immediate`, `Mailbox`, `Fifo`, `FifoRelaxed`, in that order. Empty without a window |
| `has_scene_format` | Whether the scene image of `R16G16B16A16_SFLOAT` can be drawn into, blended, and read |
| `has_srgb_textures` | Whether textures of `R8G8B8A8_SRGB` can be read and filtered |
| `has_mutable_format_views` | Whether an image can be seen as sRGB and as plain bytes, which render targets need |

They are logged when the renderer starts:

```
Textures up to 32768 pixels wide, anti-aliasing up to 8 samples, anisotropic filtering up to 16
Present modes: none without a window
Scene image of R16G16B16A16_SFLOAT: yes, sRGB textures: yes, views of another format: yes
```

Without the scene format or sRGB textures nothing could be drawn, and the
renderer stops with a message that names the format. Without views of
another format only render targets are lost, and a warning says so.

`VK_Capabilities` turns what Vulkan says into the struct, from values, so
that its unit tests need no graphics card.

## Shaders

Vulkan consumes SPIR-V, not GLSL text.

| Decision | Detail |
|---|---|
| Sources kept with what they ship as | They live in `app/NeonRuntime/engine/shaders`, written in GLSL 450, where what they are compiled to ships, `engine://shaders/`. Another renderer would get a folder of its own |
| Compiled at build time | glslang turns each source into a `.spv` file placed in the app's assets folder |
| Named without an extension | A material names `engine://shaders/basic-lit` and the renderer adds `.vert.spv` and `.frag.spv` |

Scene recipes therefore do not change with the renderer.
How the same sources reach Metal, DirectX 12, and WebGPU, and the tools that
do it, is in [shaders.md](shaders.md).

### What the shaders bind

Every texture is bound apart from the sampler it is read through, as a
`texture2D` and a `sampler`, and a shader puts them together where it reads:
`texture(sampler2D(diffuse_texture, diffuse_sampler), tex_coord)`. WGSL has
no combined image sampler, and the tools that split one make up bindings of
their own; written apart, every target binds what the source says
([shaders.md](shaders.md#what-the-sources-keep-to), #212). The layouts are
constants of the backend, `VK_Pipelines::kBindings`, `VK_Resolve::kBindings`,
`VK_Sky::kBindings`, and `VK_Renderer2D::kBindings`, and tests hold them to what the shaders
declare.

| Shaders | Set | Binding | Type | What |
|---|---|---|---|---|
| Models: `pbr`, `basic-lit`, `unlit`, `color` | 0 | 0 | Uniform buffer, dynamic | `SceneData`: the camera and the lights, `scene-data.glsl` |
| | 0 | 1 | Storage buffer | `ObjectBuffer`: every object of the frame, `ObjectData` each, read by `gl_InstanceIndex`, `scene-data.glsl` |
| | 0 | 2 | Sampled image | The first texture: the colors of the surface |
| | 0 | 3 | Sampled image | The second texture: the metallic-roughness map, or the specular map |
| | 0 | 4 | Sampled image | The third texture: what the surface gives off, see [emissive surfaces](#emissive-surfaces) |
| | 0 | 5 | Sampler | What the first texture is read through |
| | 0 | 6 | Sampler | What the second texture is read through |
| | 0 | 7 | Sampler | What the third texture is read through |
| | 0 | 8 | Sampled image, an array | The shadow map of the direction light, a layer a cascade, `shadows.glsl` |
| | 0 | 9 | Sampler | What the shadow map is compared through |
| | 0 | 10 | Sampled image | The lightmap of the material, see [lightmaps](#lightmaps), `lightmap.glsl` |
| | 0 | 11 | Sampler | What the lightmap is read through |
| Shadow pass: `shadow` | 0 | 0, 1 | As above | The same set as the models; the vertex half reads the matrix of the cascade a push constant names from `SceneData` and the object from the `ObjectBuffer`, the fragment half does nothing |

The scene is bound by an offset into its buffer, which the dynamic binding
gives, since a frame has a few. The objects are one buffer of the frame
bound whole: a draw names its first object with `firstInstance`, which the
vertex half reads as `gl_InstanceIndex`, and the objects a batch draws
follow it. The vertex half hands the index to the fragment half as a flat
varying, since a fragment has no instance of its own. A shader says which
one `object` is before it includes `scene-data.glsl`, as the shaders that
ship do.
| Resolve: `resolve` | 0 | 0 | Sampled image | The scene image |
| | 0 | 1 | Sampler | What it is read through, pixel by pixel |
| | 0 | 2 | Uniform buffer | `ResolveData`: the tonemapper and the exposure of the run, see [tonemapping](#tonemapping) |
| The sky: `sky-box`, `sky-sphere` | 0 | 0 | Sampled image | The images of the sky: a cube for the one, a panorama for the other |
| | 0 | 1 | Sampler | What they are read through |
| | push constant | | | `Sky`: how a pixel becomes a direction, and how bright the sky is, `sky.glsl` |
| User interface: `flat`, `ui/*` | 0 | 0 | Sampled image | The texture of the call, `ui-shader.glsl` |
| | 0 | 1 | Sampler | What it is read through |
| | 1 | 0 | Storage buffer | The shapes of the frame |
| | 2 | 0 | Uniform buffer, dynamic | `Values`: what the shader of an element is given, see [user-interface.md](user-interface.md#shaders-of-elements) |
| | push constant | | | `Frame`: what both halves are told about the call, `ui-frame.glsl` |

A model has a sampler for each of its textures, and not one for the set,
because a material may read a render target, whose edge is drawn on past
it, next to a texture from a file, which starts again past its edge.

The samplers are made once by the render system, in `VK_Samplers`, one for
every way a texture is read, and a `VK_Texture` says which way it is read
(`VK_Sampling`) in place of owning a sampler of its own. There are six
ways, which is every sampler the renderer ever made:

| Way | Filter | Smaller copies | Past the edge | From the side | Read that way |
|---|---|---|---|---|---|
| `AnisotropicRepeat` | Linear | Blended | Starts again | `rendering.anisotropy` samples, 8 unless set, when the graphics card can | The textures of a model |
| `AnisotropicClamp` | Linear | Blended | The edge is drawn on | `rendering.anisotropy` samples | A render target, shown by a model or a user interface; an image of a user interface read from a file or from memory |
| `LinearRepeat` | Linear | Blended | Starts again | No | An image of a user interface made with its smaller copies, that repeats |
| `LinearClamp` | Linear | Blended | The edge is drawn on | No | An image of a user interface made with its smaller copies |
| `NearestClamp` | Nearest | The image alone | The edge is drawn on | No | `image_rendering: pixelated`, and the scene image in the resolve, which reads whole pixels |
| `ShadowCompare` | Linear, compared | The image alone | A border of white: lit | No | The shadow map, see [Shadows](#shadows). A depth is compared with the four texels around it and the answers are blended |

How many samples the two that read from the side take is a quality a game
sets (#463): `rendering.anisotropy` of the settings, `--anisotropy` on the
command line, and `RenderContext::SetAnisotropy()` (an extension's
`world.SetAnisotropy()`) while the game runs: 1 for none, 2, 4, 8, or 16,
held to what the graphics card allows, which the log says. A change is made
at the start of the next frame: the frame before is waited for, the two
samplers are made again, and the descriptor sets that hold them, those of
the materials and of the textures of the user interface, are written again.
The other four are not touched.

The size a texture read from a file is kept at is a quality a game sets
(#464): `rendering.texture_scale` of the settings, `--texture-scale` on the
command line, and `RenderContext::SetTextureScale()` while the game runs: 1,
0.5, 0.25, or 0.125. Each halving is a level of the smaller copies the
texture has anyway: the image is decoded, halved on the processor that many
times (colors averaged as light, as the graphics card averages them, see
`HalveImage()`), and uploaded, and the graphics card makes the rest of its
smaller copies as always. No side is halved below 32 pixels, so a small
texture keeps its detail. The texture cache keeps a texture under its scale
as well, so a change holds for the textures read from then on, and a scene
that is loaded next reads its textures at the new size; what is held stays
as it is until nothing reads it. Images of the user interface, fonts,
render targets, and images made while the game runs are not scaled.

Light data is held in a uniform buffer, not in individual uniforms. That
removes the limit on uniforms per shader, which is what had held the OpenGL
backend to 16 lights of each kind.

### The shaders that ship

| Shader | Lights | What it draws |
|---|---|---|
| `pbr` | Yes | Physically based, with the metallic-roughness model of glTF (#59): Lambert for the scattered light and Cook-Torrance for the reflected, with GGX, Smith-GGX as Schlick approximates it, and Schlick's Fresnel. The material's `color`, the color of the vertex, and the texture multiplied are the base color, `metallic` and `roughness` say what the surface is, and the second texture is a glTF metallic-roughness map (green roughness, blue metallic). The shader for every surface of a game |
| `basic-lit` | Yes | Blinn-Phong with `shininess`, the shader before `pbr`. The color of the vertex tints the diffuse color. Kept for the scenes that use it; nothing new should |
| `unlit` | No | The texture, times the color of the vertex, as it is |
| `color` | No | The color as it is |
| `flat` | No | For what is drawn in two dimensions |
| `sky-box`, `sky-sphere` | No | The sky of a scene, see [The sky](#the-sky). Drawn by the renderer for a `Sky`, not named by a material |

How `pbr` reads a light: `diffuse` is the light's radiance - the color a
white, matte, non-metal surface facing it shows; `ambient` lights the diffuse
color evenly from every side; `specular` is not read, since a surface's
highlight follows from its roughness and its metalness, not from the light.
A roughness below 0.045 is raised to it, so that a mirror does not become a
spike no pixel hits. The defaults, `metallic: 0` and `roughness: 0.5`,
differ from glTF's 1 and 1 on purpose: without an environment to reflect
(#64, #90) a metal is dark, and most surfaces of a game are not metals.
They hold for a surface whose model file says nothing of either, such as a
`Geometry`. A glTF file always says, and its `metallicFactor` and
`roughnessFactor` are taken when the scene leaves the numbers out, and
multiplied by them when it writes them, see
[models.md](models.md#what-of-a-material-is-read).

`tests/runtime-pbr` checks the shader against numbers worked out by hand:
flat planes face the camera and one white light shines from it, so that every
angle in the shading is known - a white matte dielectric shows 0.963 of the
light (sRGB 250), a white matte metal 0.080 (sRGB 80), a red matte dielectric
red with the 4 percent it reflects in white (250, 11, 11).

Both lit shaders take the shadow of the direction light into account, see
[Shadows](#shadows).

Both lit shaders add what a surface gives off after the lighting, see
[emissive surfaces](#emissive-surfaces).

What `pbr` does not do yet: normal maps, occlusion, image based lighting
from an environment (#64). The ubershader of #106 adds them as variants.

### Lightmaps

Light that was worked out ahead of time and kept in a texture (#240). It is
not a shadow map: a shadow map is drawn again in every frame from where the
light is, and follows what moves; a lightmap is made once, for what stands
still, and costs one read of a texture when it is drawn.

| Piece | Is |
|---|---|
| `Vertex::lightmap_coords` | A second set of coordinates, from 0 to 1, apart from those of the textures: a lightmap lies over a surface once, where a texture repeats. Attribute 4 of every model |
| `material.lightmap` | The texture, a path of the file system or `image://<name>`. Read as colors, in sRGB, without smaller copies, which would blend the light of one surface of an atlas into its neighbor's, and without repeating |
| `material.lightmap_strength` | What it is multiplied by, in linear light, so that baked light can be brighter than eight bits hold |
| `ObjectData::lightmap` | The strength, and whether a lightmap is bound; a material without one binds plain white and the shaders leave it out |
| `lightmap.glsl` | `has_lightmap` and `baked_light`, included by the shaders that read it |

| Shader | Does with it |
|---|---|
| `unlit` | Multiplies it into what it shows: baked light is the one light it knows. It is how a level whose light is all baked is drawn |
| `basic-lit`, `pbr` | Add it to the light of the lights, on the diffuse color, as ambient light is |
| `color` | Nothing |

The bindings come after those of the shadow map, so that none a shader named
before has moved. Where the coordinates come from is whoever makes the mesh:
an extension hands them over, see [extensions.md](extensions.md#what-an-extension-draws).
A model file's second set of coordinates is not read yet, and nothing in the
engine makes a lightmap; both belong to #240.

### Emissive surfaces

A material can give off light of its own (#129): `material.emissive`, a
color written in sRGB as `color` is, `material.emissive_strength`, what
it is multiplied by in linear light, and `material.emissive_texture`, a
texture or a render target the color multiplies. See
[scenes.md](scenes.md#renderable) for the fields and
[models.md](models.md#what-of-a-material-is-read) for what a glTF file
contributes. `VK_Material` hands the color, with the strength multiplied
in, to the shaders as `ObjectData.emissive`, with its `w` saying whether a
texture is bound; the texture is the third of the material's bindings, and
plain white when there is none.

`pbr` and `basic-lit` add the result after everything the lights do, so a
surface that gives off light shows in a room without any, and `unlit` and
`color` ignore it. The light goes into the scene image as it is, which
holds floats: an `emissive_strength` of 4 makes a surface four times
brighter than white, which only the resolve step then decides about. Without
a tonemapper such light is cut off at white, and it does not glow around its
edges and lights nothing nearby: that is bloom (#130) and light cast by
screens (#132). `tests/runtime-tonemapping` draws planes that give off
light in a scene without any.

## Changes to neon-core

The interfaces were shaped around OpenGL in a few places. These are the
changes that were made, all backend neutral.

| Change | Reason |
|---|---|
| `RenderingApi::Vulkan` | Select the backend |
| `RenderSystem::FinishFrame()` | Vulkan has to submit and present its work at the end of a frame |
| `RenderSystem::CaptureFrame(path)` | Save the last rendered frame as an image through the file system |
| `RenderSystem::GetCapabilities()` | Tell the rest of the engine what the graphics card can do, see above |
| `WindowContext`: Vulkan surface hooks | The window system owns the window, so it is the one that can create a surface for it. They replaced `GetGlProcAddress()`, the hook OpenGL had used |
| Headless window and input systems | Implement the interfaces with no window and no devices. They live in neon-core, as they need no backend |
| File system: `user://` and `WriteBytes` | Screenshots need somewhere writable. `user://` was already planned |

## The backend library

A library of its own, `neon-vulkan`.

| Piece | Role |
|---|---|
| `VK_RenderSystem` | Implements `RenderSystem` and `Render2DContext`. Owns the device, the render passes, the frame images, the buffers of shader data, and the frame loop, and hands the work to the types below |
| `VK_Device` | The instance, the graphics card, the logical device and its queue, and the helpers for buffers and images |
| `VK_Swapchain` | The images of a window, made again when the window changes, and the copy of a finished frame into them |
| `VK_Canvas` | Where the frame, or a render target, is drawn: its stages, its clear color, its scene image, and the see-through models kept for the end of its scene. The frame and every render target own one |
| `VK_Pipelines` | The pipelines of materials, one per variant of shader, covering, and culling, and the layout they share |
| `VK_Capture` | Reads a finished frame back and writes it as a PNG |
| `VK_Model`, `VK_Mesh` | Vertex and index buffers. `VK_Model` derives from the core `Model`, which does the loading through assimp for every renderer. One vertex layout for every model, `Vertex` of neon-core field for field: position, normal, texture coordinates, and a color, bound by `VK_Pipelines` |
| `VK_Texture` | Image and view, and which way it is read |
| `VK_ModelCache`, `VK_TextureCache` | What the render objects draw, held once each: a model for every path and fit, a texture for every image, counted and freed when the last object that drew it goes. See [what is shared](#what-is-shared) |
| `VK_Samplers` | The five samplers every texture is read through, one for each way of reading, made once and shared |
| `VK_Shader` | Shader modules from SPIR-V |
| `VK_Material` | Textures, descriptor set, per-object data, and which pipelines draw it. A render object holds one for each material of its model that a mesh uses |
| `VK_SceneImage` | The image of linear light a scene is lit in, with its depth |
| `VK_ShadowMap` | The depth of the scene as the direction light sees it, with the pass that draws it |
| `VK_Sky` | The sky of a scene: its two pipelines, the images of the skies that are drawn, and how a pixel becomes the direction it is seen in |
| `VK_Resolve` | The resolve step, which turns a scene image into the colors of the image that is shown |
| `VK_RenderTarget` | An image that is drawn to like the frame, and read as a texture |
| `VK_Renderer2D` | What is drawn in two dimensions, on top of the resolved scene |
| `VK_SwapchainSizing`, `VK_FrameStages`, `VK_DrawOrder`, `VK_Culling`, `VK_ShadowFit` | The decisions of the renderer, kept apart from the graphics card so that they are tested |

### What is shared

A render object is one entity that is drawn. It draws with a material for
each material of its model that a mesh uses (#193): one for a model with one
material, and for a mesh a `Geometry` built. Each is a draw of its own, the
meshes that use the material with the pipeline, the descriptor set, and an
entry of object data of that material, so that the color factor of every
material of the file reaches the shader; draws alike are batched as every
draw is, see [The order of a frame](#the-order-of-a-frame), a see-through
material is kept for the end of the scene, and the shadow pass draws the
whole model of an object once when all of its materials cast alike, and
the meshes of each material with its covering when they do not. The scene's `Renderable`
overrides every material, and its `textures` the first alone, see
[models.md](models.md#several-materials). What a render object draws with
is held once (#241), its materials included: two render objects whose
material of a model's material is the same draw with one material, one
descriptor set, one set of textures.

| What | Held once for every | By | Freed |
|---|---|---|---|
| A model from a file: its vertex and index buffers | Path and `fit` | `VK_ModelCache` | When the last render object that drew it is destroyed, and at clean-up |
| A model file that could not be read: that it failed | Path and `fit` | `VK_ModelCache` | At clean-up, see below |
| A texture: an image file, or an image a model carries | Image path and how it is kept (colors or numbers, smaller copies, repeating), and the model's path for an image it carries, since two models may each call theirs `*0` | `VK_TextureCache` | When the last material that read it is cleaned up, and at clean-up |
| A material: the shader, the textures, the color and the rest of `MaterialInfo`, over a model | Everything it is made from, as `VK_MaterialCache::KeyOf` writes it | `VK_MaterialCache` | When nothing draws with it any more and what is unused is freed, see below, and at clean-up |
| A mesh a `Geometry` built | Nothing: it belongs to its entity | The model cache, uncounted | With its render object |
| A render target a material shows | The target | The target | With the target |

A material that nothing draws with any more is not freed at once. It stays,
with its textures, until what is unused is asked to be freed
(`RenderContext::FreeUnused`), which happens at three times:

| When | Who asks |
|---|---|
| A scene took the place of another, after its first frame was drawn | `EntityWorld`. What both scenes show is held by the new one by then, and is not read again |
| A game that changes its levels within one scene says a level is over | The game: `free_unused` of an extension, `World::FreeUnused()` |
| There is no room for another material | The renderer itself |

A model file that cannot be read is read once, and the error is said once:
"Error importing model …" from assimp, and "Could not initialize model …"
from the cache (#248, #507). The cache then remembers the path and fit as
failed, and every render object that names it after that draws nothing and
says nothing, and is neither looked for in the file system nor handed to
assimp again. Before this, the bench, which spawns sixty crates a second
that named a missing model, logged thousands of errors a second. What
failed is remembered until clean-up, and not forgotten when a scene takes
the place of another: the model cache keeps nothing by scene, since a model
is freed as soon as nothing draws it and `FreeUnused` does not reach it, and
the physics remembers a model it could not read for as long as it runs as
well, so the two never disagree about whether a model is there. A file that
was put right while the game runs is read at the next start.
`VK_ModelCache::Failures()` says how many paths and fits failed.

It is how a game with levels loads: what a level showed stays until the
level is over. An entity that shows one texture after another, a face, a
button, a weapon with several skins, goes back and forth between materials
that are held, and nothing is read or sent to the graphics card for a
texture that was shown before. What is freed is what was given back before
the frame in which it is freed, at the start of a frame, so no draw waits
with it.

An entity is drawn with what its `Renderable` says now. The description of
`Renderable` counts a version up whenever a field of it is written through
it, by a script, an extension, or a document (`TypeInfo::written`), and
`RenderSubmission` tells the renderer when the version is not the one it
drew: one comparison of two numbers for each entity in a frame, and nothing
for what was not written. `UpdateRenderObject` takes the materials of what
the entity looks like now before it gives back those it had, so what both
share is never let go of, and keeps the id and, for a mesh that was built,
the mesh. What writes a field in place, through its address, is not
noticed; it counts `RenderInfo::version` up itself. `preload` of a
`Renderable` takes the material for each texture it names when the entity
is first drawn, and holds it with the entity.

`VK_ModelCache` and `VK_MaterialCache` are a `DataBuffer`, which gives
every model or material the id a render object keeps, with a key and a
count on top. `DataBuffer` stays the store of what is not shared, the
render objects, the render targets, and the images of the user interface,
so the counting lives beside it and not in it. The texture cache is a map
by key, since a material holds a `VK_Texture` and not an id.

A material's descriptor set comes from a pool, and a pool holds a fixed
number of sets, 256. When the last pool is full another is made, and a
material remembers the pool its set came from, so that a game may have
as many distinct materials as it likes. Before this, the one pool of 4096
sets ran dry in a scene that spawned crates without end, and every crate
past it failed with "Could not allocate a descriptor set". The frame
summary says how many materials were made and how many shared, and
tests/runtime-sharing reads it: three hundred crates of one material make
one, three hundred of distinct colors make three hundred and two pools.

So the 17 walls, floors, columns, and targets the prototype level places
from four prefabs read four GLB files and one colormap, not seventeen of
each, and a scene that writes the same model on two entities in full gets
the same. `CreateRenderObject()` takes a reference and
`DestroyRenderObject()` gives it back. The log says at the end of the
frame that created render objects how many models were loaded and how many
were shared, and how many materials were made and how many shared, once a
scene. Textures are not counted there, since one is only ever read through
a material; each model and texture that is shared or freed is named at the
level of debugging.

The images of the user interface, loaded through `LoadTexture()`, are held
by `UiResources` once for every path already and are not in the cache.
Drawing the objects that share a model with one call is instancing (#169),
which is not done.

**Render to an image first, always.** Every frame is drawn into an offscreen
image. With a window, that image is then copied to the swapchain. Headless,
it is simply left there to be captured. Both modes share all drawing code, so
what a screenshot shows is what a window would show.

## The order of a frame

| Stage | Drawn into | What happens |
|---|---|---|
| Shadow pass | The shadow map, `D32_SFLOAT`, `rendering.shadow_map_size` texels a side, 2048 unless the settings say otherwise, a layer a cascade | The opaque models of the first scene of the frame whose direction light casts, as the light sees them, depth alone, once into every cascade. Recorded apart and run before everything below, so that every scene of the frame reads the finished map. Left out when no light casts, and when `rendering.shadows` is off. See [Shadows](#shadows) |
| Render targets | Each target, in the order they are drawn | Each goes through the stages below on its own: a camera that draws into a texture lights a scene in a scene image of the target, a user interface on a surface draws on top. A target that shows only a user interface has no scene image |
| Scene | The scene image, `R16G16B16A16_SFLOAT`, and its depth | Opaque models in the order that costs the least, see below, then the sky wherever none of them is, see [The sky](#the-sky), then see-through ones from the farthest to the nearest, tested against depth but not writing it. The back of every triangle is left out unless the material is double-sided. Lighting and blending are in linear light. An opaque model replaces what is behind it and leaves the alpha of the scene image at 1, whatever its shader wrote |
| Effects on the light | Two pictures as the scene image, in turn | The `effects` of the camera, each a triangle that covers the picture: the first reads the scene image, and each one after it what the one before wrote. Left out, with the pictures, for a camera that names none. See [the effects of a camera](#the-effects-of-a-camera) |
| Resolve | The image that is shown, `R8G8B8A8_UNORM` | A triangle that covers it reads the scene image pixel by pixel, multiplies the exposure in, maps it through the tonemapper, and writes it in sRGB. The one place where light becomes the colors of a screen, see [tonemapping](#tonemapping) |
| Effects on the screen | Two pictures as the image that is shown, in turn, and then that image | The `screen_effects` of the camera. The resolve then writes into the first of the pictures, and the last effect writes into the image that is shown, where the resolve would. Left out for a camera that names none |
| On top | The same image | What is drawn in two dimensions: user interfaces, blended in sRGB as CSS blends them |
| Copy | The window, or a file | Byte for byte. The bytes are sRGB already |

A model that is drawn is kept, not drawn: `VK_DrawQueue` holds the draws
of a scene until the scene ends, which is when something is drawn on top
or the frame finishes, and then puts the opaque ones in order: by pipeline,
so that one is bound when it changes and not for every object; by material
within it, for the descriptor set; by model within that; and the nearest
first among those alike, so that what is hidden is not shaded. Draws alike
in all three are one call, `vkCmdDrawIndexed` with as many instances, their
objects side by side in the buffer of the frame from the first instance on.
Two thousand crates of one prefab are one draw. The shadow pass has batches
of its own, since it writes depth alone and the material of an object parts
nothing there: the casters are ordered by the pipeline of the pass, the
model, and its meshes, their objects are written into the buffer once more
in that order, and a model is one call a cascade however many materials its
objects are drawn with in the scene. An object of several materials casts
its whole model with one of its draws when all of them cast alike
(`VK_ShadowCasting`). The pass reads the scene and the objects alone, which
every descriptor set binds the same, so it binds one set for a camera and
its lights. See-through draws keep the order they
came in, behind the opaque ones, and the canvas sorts them from the farthest
to the nearest as before, by the middle of the box around each model in the
world: a mesh may lie far from the origin its entity places it by. The order is worked out apart from the graphics
card, in `VK_DrawQueue`, which is tested; what a frame cost, objects, draws,
pipelines and sets bound, is said at the level of debugging every three
hundred frames. Indirect draws from a buffer, and textures by index so that
materials need no set each, are the next steps of #169.

The first model of a frame begins the scene, and the first triangle drawn in
two dimensions resolves it. A model that comes after that would cover what is
on top, and is left out with a warning. The runtime draws the world before
its user interfaces, so this does not happen.

Which side of a triangle is the front is part of a pipeline, and a model
whose transform mirrors it turns its triangles round. A material gets a
second pipeline for that, with the other side as its front, when the first
mirrored object is drawn with it, and the draw picks it by the sign of the
determinant of the model's transform. The pipeline is shared by every
material with the same shader, covering, and culling, so the first mirrored
object of such a variant pays for it once. `VK_Culling` holds these rules,
apart from the graphics card, so that they are tested.

The resolve is where what changes how light looks on a screen goes. Light
that bleeds around what is bright (#130) is added to the scene image just
before it. The scene image keeps such light until then.

## The effects of a camera

A camera names shaders that are run over the whole picture it drew, in two
lists, see [scenes.md](scenes.md):

```yaml
Camera:
  effects:
    - extensions://quake/shaders/under-water
  screen_effects:
    - engine://shaders/effects/scan-lines
```

| List | Run | The picture holds | For |
|---|---|---|---|
| `effects` | After the scene is drawn, before the resolve | Linear light, with room above white, as half floats | What changes the light: a view that waves, a vignette, a color laid over everything |
| `screen_effects` | After the resolve, before what is drawn on top | The colors a screen is given, sRGB encoded, as bytes | What is about the picture that is shown: the rows of a monitor, a palette, a pattern of dots |

An effect is a fragment shader alone, named as a shader is and read from
the file with `.frag.spv` added. The engine brings the vertex half,
`effect.vert`. It starts with `effect.glsl`, which gives it:

| Name | What it is |
|---|---|
| `frame_coord` | Where the pixel is in the picture: 0, 0 at the top left, 1, 1 at the bottom right |
| `read_frame(at)` | The picture at a place of it, as it is before this effect, read smoothly, with its edge drawn on past it |
| `frame_size()` | The size of the picture in pixels |
| `scene.time`, `scene.numbers[0..7]` | What the shaders of a material read under the same names: the time of the world, and the numbers of the game, see [extensions.md](extensions.md#the-shaders-an-extension-brings) |
| `frag_color` | What the effect writes |

```glsl
#version 450
#extension GL_GOOGLE_include_directive : require
#include "effect.glsl"

void main()
{
    vec2 swayed = frame_coord + 0.004 * sin(frame_coord.yx * 16.0 + scene.time.x);
    frag_color = read_frame(swayed);
}
```

| Decision | Why |
|---|---|
| The lists are on the camera | A camera that draws into a texture has effects of its own, and the camera of the window others. A game turns an effect on by writing the field, as it writes any field |
| Two lists | An effect on light has to come before the tonemapper, which is where light ends. One that counts rows or picks from a palette wants the picture as it is shown |
| An effect is no material | It is bound to a layout of its own, three bindings, and never to an object. A shader of a material is drawn into the scene as before and does not know that an effect follows |
| The user interface is drawn after both | A menu is not waved with the water behind it |
| A camera without effects costs nothing | The pictures the effects are run between are made when a camera first names one. Each effect is one pass over the picture |
| The alpha of the picture is multiplied into its colors | As everywhere after the scene, see [color spaces](#color-spaces). An effect hands on what it does not change |

An effect that cannot be read is said once and left out; the others are run.
The engine ships two, in `engine://shaders/effects`: `vignette`, for
`effects`, and `scan-lines`, for `screen_effects`. The monitor of hall 10 of
the museum shows both.

Not there yet: an effect cannot read the depth of the scene, has no
pictures of its own to draw into (a blur in several steps), and is told
nothing beyond the time and the numbers of the game.

## Vertical sync and the window

How frames reach the screen is one setting, `rendering.vsync`, `--vsync` on
the command line, and one function while the application runs:

| Vertical sync | Present mode | What it means |
|---|---|---|
| On, as it starts | FIFO | A frame for every refresh of the screen: nothing is torn, and no more frames are drawn than the screen shows |
| Off | Immediate, where the driver offers it | A frame is shown as soon as it is done, however many a second that is, and may be torn |
| Off, and the driver has no immediate | FIFO | It stays on, and the log says so |

The mode that was taken is said in the log, `Presenting with fifo, vertical
sync on`, next to `Present modes:`, which lists what the driver offers.
Mailbox and FIFO relaxed are not taken (#356): the two above are what every
platform of the engine has. `VK_PresentMode` chooses, apart from the
graphics card, so that it is tested.

Apart from the sync there is a limit, `rendering.max_fps`, `--max-fps` on
the command line: the most frames a second, a number from 30 to 300, or 0
for as many as can be drawn, which is how it starts. The two hold together,
and whichever allows fewer frames is what is seen: a limit of 120 on a
screen of 60 with the sync on shows 60. The window keeps it, not the
renderer: a frame that was done early waits out the rest of its time
before the next is measured, asleep for most of it and looking at the clock
for the last two milliseconds, since a sleep ends late. `FrameLimit` holds
the numbers, apart from a window, so that they are tested. A run with a
fixed time step (`--time-step`) and one without a window are not held
back.

Four things change how the window is shown while the application runs. A
menu of video settings calls them, the one of the runtime or one a game
brings:

| What | In C++ | For an extension |
|---|---|---|
| Vertical sync | `RenderContext::SetVerticalSync(bool)`, `GetVerticalSync()` | `set_vertical_sync`, `get_vertical_sync` |
| The most frames a second, 30 to 300, or 0 for no limit | `WindowContext::SetFrameLimit(int)`, `GetFrameLimit()` | `set_frame_limit`, `get_frame_limit` |
| The mode of the window: windowed, borderless, fullscreen | `WindowContext::SetWindowMode(WindowMode)`, `GetWindowMode()` | `set_window_mode`, `get_window_mode` |
| The size of the window, in points | `WindowContext::SetWindowSize(width, height)`, `GetWindowSize()` | `set_window_size`, `get_window_size` |
| The sizes the display offers | `WindowContext::GetDisplaySizes()` | `list_display_sizes` |

| Decision | Why |
|---|---|
| The swapchain is made again before the next frame | A present mode belongs to a swapchain. The one before is handed over as `oldSwapchain`, and the frame is drawn at the size the window has by then, as it is when a window is resized by hand |
| The renderer is not told of a new size | It asks the window for its size in every frame, and follows: the frame images, the projection, and the user interface with it. A mode or a size that changes is one more way a window gets another size |
| A size is in points | It is what a window is counted in. On a display of high density the frame has more pixels than that, see `get_view_size` |
| Fullscreen takes the size the display offers that is nearest | A display shows only the sizes it has. `GetDisplaySizes()` lists them for a menu to choose from |
| Borderless keeps the size it is told | It covers its display whatever the size, and is that size when it is a window again |
| A limit beyond 30 to 300 is held to the nearest, where a setting that says so is refused | A slider of a menu is not to be able to go wrong; a file that says 500 is a mistake its writer is to be told of |
| Nothing is written to a settings file | These change what is shown now. What a player chose is kept by whoever offers the choice: a game's own menu keeps it with its other settings |

The settings menu of the runtime changes all of these while the game runs,
along with the other graphics, and keeps what the player chose with Apply,
see [user-interface.md](user-interface.md#the-graphics-of-the-settings-menu)
(#356).

## Tonemapping

The scene image holds light, which has no top: a surface that gives off
light, a strong light on a white wall, and later bloom all make values
above 1. A screen shows nothing above white, so the resolve step decides
what becomes of such light (#131). Two settings say how, read from
`settings.yml` and the command line, see [settings.md](settings.md) and
[command-line.md](command-line.md):

| Setting | What it does | Default |
|---|---|---|
| `rendering.exposure` | How bright the scene is taken to be. The light is multiplied by it before the curve, so 2 doubles everything and 0.5 halves it. A number above zero | 1 |
| `rendering.tonemapper` | The curve: `none`, `aces`, or `agx` | `none` |

The default keeps every frame as it was: at an exposure of 1 and without a
curve, what is at most white is written as it is and what is above is cut
off flat at white, as before. A game chooses a curve when it has light
above white to show.

| Curve | What it is | What it does with white |
|---|---|---|
| `none` | A clamp | 1.0 stays white; 4.0 is white too, cut off flat |
| `aces` | The ACES filmic curve, as Krzysztof Narkowicz fits it (2016): one rational function, `x (2.51 x + 0.03) / (x (2.43 x + 0.59) + 0.14)`. Cheap, and close to the reference ACES transform for most of its range; the fit does not carry the hue shifts of the reference, which bends bright colors towards yellow | 1.0 comes out at 0.80 of white, sRGB 232; 4.0 at 0.97, sRGB 252; the light keeps a little contrast all the way up |
| `agx` | AgX in the minimal form of Benjamin Wrensch (2023), after Troy Sobotka's: a small mix of the three channels into each other, a log encoding over sixteen and a half stops, a sigmoid fitted by a polynomial, the mix undone, and a 2.2 power back to linear light. The mix keeps a bright color from turning white or yellow | 1.0 comes out at 0.59 of white, sRGB 202; 4.0 at 0.85, sRGB 239; white is reached about four stops above 1. A game that chooses AgX raises its exposure, or its lights, to match |

`resolve.frag` holds both fits, named after their authors, and the test
`tests/runtime-tonemapping` holds the numbers above: planes that give off
known light are read pixel by pixel under each curve, and under an exposure
of 2 without one. The numbers are worked out from the fits in the test's
script.

The curve is applied to the straight color, with alpha taken out, as the
clamp was, and the two matrices of AgX are written by columns in the
shader, as GLSL lays a matrix out. The settings reach the shader in one
uniform buffer, `ResolveData`, written once at start, and not as a push
constant, since the sources keep push constants for what changes every
draw, see [shaders.md](shaders.md#what-the-sources-keep-to).

What is not tonemapped:

- The user interface. It is drawn after the resolve, on the image that is
  shown, and keeps the colors of its style sheets: a button is the color
  CSS says, whatever the scene behind it does.
- A render target, in a way: a camera that draws into a texture goes
  through the same resolve, so the picture it makes is tonemapped once
  into the target's bytes, and once more when a model that shows it is
  resolved with the frame. With `none` that changes nothing. A game that
  shows a camera view on a screen in the world and tonemaps may see the
  picture a little flatter than the world around it.

What is left for later: an exposure that adapts to the scene, as eyes do,
needs the average brightness of the frame, which is a reduction over the
scene image before the resolve, and a speed at which the eye follows. It
comes with bloom (#130), which needs the same pass over the image. A
white point or a look of a game's own, such as the full ACES transform
with its reference rendering transform, can be added as more values of
`rendering.tonemapper` without touching anything but the shader and the
setting.

## Shadows

The direction light casts shadows (#60): what stands in it keeps its light
off what is behind. A `Light` says whether it does with `casts_shadows`,
true unless written, see [scenes.md](scenes.md). Point and spot lights
cast nothing yet.

| Part | What it is |
|---|---|
| The map | One depth image in whole floats with four layers, `VK_ShadowMap`, a layer a cascade, of `rendering.shadow_map_size` texels along each side, 2048 unless the [settings](settings.md) say otherwise, drawn once a frame and read by every scene of it as an array. It is left all lit when it is made, so that a frame in which nothing casts reads it and lights everything. `RenderContext::SetShadowMapSize()` makes it again at another size while the game runs, at the start of the next frame as the frame's images are made again when the window changes its size: the render pass stays, so the pipelines of the pass do, and every material's set is written again with the new view |
| The switch | `rendering.shadows`, `RenderContext::SetShadowsEnabled()`: off, no light is fitted and nothing casts, so the pass is not recorded and the shaders skip the comparison, which is the frame of a light with `casts_shadows: false` byte for byte. The other shadow settings are kept for when it is on again |
| The pass | Before the scene, in commands of its own that are submitted first. Every opaque model of the first scene that casts is drawn with the depth-only `shadow` shader through a variant of `VK_Pipelines`, once into each cascade's layer, the cascade named by a push constant, culled as its material is: a plane seen from one side casts from that side alone, and a double-sided material from both. In batches of its own, by model and not by material, so two thousand crates are one draw a cascade whatever their materials are. See-through models cast nothing for now |
| The fit | `VK_ShadowFit`: cascades, as Godot, Unity, and Unreal fit theirs. What the camera sees up to `rendering.shadow_distance`, 120 meters unless the [settings](settings.md) say otherwise, is cut into `rendering.shadow_cascades` slices along the view, four by default, split halfway between even and logarithmic so that the near slice is thin; each slice gets a box around the sphere that holds its corners, seen along the light without perspective, reaching back towards the light by the distance so that what casts into the slice from above it is in the map. A sphere gives the box one size however the camera turns, and the box moves in whole texels of the map, so that the edge of a shadow stays where it is while the camera moves by less than one and does not shimmer. The shaders pick the cascade by the point's distance along the view and read its layer; past the last one everything is lit |
| The bias | Two parts. The pipeline of the pass pushes a caster back along the slope of its surface by two texels of depth (`depthBiasSlopeFactor`), which covers the texels the comparison reaches across on a surface that slopes away from the light. The shaders move the compared depth by 0.0002 of the depth of the box, about 1.6 centimeters, which covers a surface that faces the light. With whole floats the constant bias of the pipeline is too fine to be of use, which is why that part is in the shaders |
| The comparison | `shadows.glsl`, bound to every lit shader: the place in the map and the depth of the point, compared through a sampler that compares (`VK_Sampling::ShadowCompare`), which blends the answers of the four texels around the place. With `rendering.shadow_filter` of `pcf` nine such comparisons one texel apart are averaged, which softens the edge of a shadow over about three texels; with `none` the place is compared once, and the edge is about a texel wide. The scene data tells the shader how many comparisons across, 1 or 3, in `shadow.x`, and 0 for a light that casts none. Past the edge of the map, and further from the light than the box reaches, everything is lit |
| What it dims | The diffuse and the specular light of the direction light, never its ambient. A point in full shadow shows the ambient light alone |

Why culling as the material is, and a bias, rather than drawing the backs
of casters into the map: the engine draws planes with one side, as the
floors of `tests/runtime-shadows` and the platforms of the built geometry
are, and those have no back to draw. A bias along the slope leaves a
caster where it is, to within two texels.

`tests/runtime-shadows` checks the numbers: a white box on a white floor
under one light at 45 degrees, where the floor beside the box shows the
ambient light alone, and the same scene with `casts_shadows: false` shows
the floor lit, as it does with `--shadows off`, whose frame is the frame
of that light byte for byte; and the smallest map with `--shadow-filter
none` shadows the floor the same. The frames of every scene whose light
does not cast are byte for byte what they were before the pass existed,
since the shaders skip the comparison for such a light.

What is open:

| Open | Detail |
|---|---|
| Between cascades | A point picks one cascade, so the edge of a shadow changes its softness where one cascade hands over to the next. Blending the two over a band of depth hides the seam, and is later |
| Point and spot lights | A spot light needs a map with perspective, a point light six of them in a cube, or a map of two paraboloids. The scene data and the bindings have room, the shaders do not read any yet |
| Soft shadows | The filter gives a fixed softness of three texels, or one without it. Percentage-closer soft shadows, which widen with the distance between the caster and the receiver, or a Poisson disc, are later |
| One map a frame | The map is fitted around the camera of the frame, the one the window shows, and holds what that scene casts. What a camera draws into a texture is left unshadowed: it is drawn before the frame and picks a cascade by its own depth, so it cannot read a map fitted to another camera. Before, the first camera of a frame took the map, which was the camera of a texture, and the window lost its shadows. A map per camera, or per light, is later (#350) |
| The bias per cascade | One bias serves every cascade, in texels of each, so the far cascades push a caster back further in meters than the near one. A bias scaled to the cascade, and normal offset, are later |

## The sky

A scene with a `Sky` (#343, see [scenes.md](scenes.md)) shows it behind its
models, for every camera of the frame: the one of the window and those
that draw into a texture.

| Part | What it is |
|---|---|
| The way in | `RenderSubmission` hands the `SkyInfo` of the scene to the pipeline in every frame, as it does the lights, and `Forward_RenderPipeline` calls `RenderContext::DrawSky()` once for every camera, with its view and its projection. A renderer that leaves `DrawSky()` as it is shows what the frame is cleared to |
| The draw | One triangle that covers the scene image, at the far end of the depth, which is what the depth is cleared to. It is tested against the depth as less or equal and writes none: it passes where no model was drawn and nowhere else. The canvas keeps it until its opaque models are drawn and draws it then, so that only the pixels left over are shaded, and before the see-through models, which blend over it |
| The direction | `VK_Sky::ValuesOf()`: the view without where the camera stands, times the projection, undone, and turned back by the `rotation` of the sky. The fragment shader takes its place on the screen through that matrix to the direction it is seen in, for every pixel (`sky.glsl`). The camera therefore turns in the sky and never moves through it, whatever `far` is |
| A box | The six images as the layers of one cube image, `VK_Texture::InitializeWithFaces()`, read by the direction. A cube counts z the other way round than the world does, so `front`, seen along negative z, is the layer of positive z, and `sky-box.frag` reads with z turned round; the faces are then seen from the inside as they were painted, not mirrored. The graphics card blends across the edges of the faces, so no seam shows |
| A sphere | The panorama as one image. `sky-sphere.frag` turns the direction into a place across, by the angle around what is up, and a place down, by the angle from straight up. The image starts again past its left and right edge, and half a pixel is kept from its top and bottom |
| The images | sRGB colors read as linear light, as the first texture of a material is, multiplied by `brightness`, and written into the scene image with alpha 1. They are loaded the first time a sky is drawn and held until a frame goes by that does not draw it, so a scene that is left gives its sky back. A sky that cannot be loaded says why once, and the frame shows what it is cleared to |

`tests/runtime-sky` checks where each face and each part of a panorama is
seen, a sky that is turned, a model in front of the sky, and a see-through
one blended over it, pixel by pixel.

What is open:

| Open | Detail |
|---|---|
| Light from the sky | The sky lights nothing. Image based lighting from it is #64 |
| Brighter than white | The images are 8 bits a channel, so nothing in a sky is brighter than white for the [tonemapper](#tonemapping) to map. HDR panoramas are #347 |
| Smaller copies | The images are read at their full size, so a panorama much larger than the screen shimmers as the camera turns. A level picked from the place in the image would draw a seam where a panorama starts again, so it has to be picked from the direction (#347) |
| Day and night | Blending between painted skies by the time of day is #346, a sky made by a formula with an atmosphere #344, volumetric clouds #345 |

## Color spaces

Colors are written the way a screen shows them, in sRGB: in image files, in
scene recipes, and in the style sheets of a user interface. Light adds up and
blends in linear terms, where 0.5 is half the light of 1. In sRGB, 0.5 is
about a fifth of it. The scene is therefore lit in linear light, and user
interfaces are drawn in sRGB, which is what CSS blends in.

| What | How it is read |
|---|---|
| The first texture of a material | An sRGB format, read as linear light. Its smaller copies are made by the graphics card in linear light |
| The images of a sky | An sRGB format, read as linear light, without smaller copies |
| The second texture of a material | Plain bytes. It says how much a surface shines, which is a number and not a color |
| The `color` of a material, the color a camera clears its texture to | Turned into linear light before the shaders see it |
| The `emissive` of a material, and its texture | As the color and the first texture: written in sRGB, turned into linear light, and multiplied by `emissive_strength` in linear light, which is how it goes above white |
| `ambient`, `diffuse`, and `specular` of a light | Amounts of light, handed over as they are. 0.5 is half the light |
| Images, glyphs, and colors of a user interface | Plain bytes and sRGB numbers, blended as they are, as before and as CSS does |
| A render target | Holds sRGB colors as bytes. A model reads it through an sRGB view, as linear light. A user interface reads it through a view of plain bytes |
| The window | A format of plain bytes. A format that converts to sRGB would convert the frame a second time, and is taken only when there is nothing else, with a warning |
| A saved frame | Copied byte for byte, and sRGB |

The scene `gamma-test.scene.yml` and the tests `runtime-colors` check this
pixel by pixel: half of red over black is 188, the light of half of red, and
not 128, while half of black over white in a user interface is 127, as in a
browser.

## The command line

```bash
NeonRuntime
```

For development, a scene can be rendered without a window:

```bash
NeonRuntime --headless-renderer --frames 3 --screenshot user://screenshots/frame.png
```

| Option | Meaning |
|---|---|
| `--renderer vulkan` | Renderer to use. Vulkan is the only one so far |
| `--vulkan-version 1.N` | The highest version of Vulkan to render with. 1.3 unless it is given |
| `--headless-renderer` | No window and no input devices, the frames are drawn off-screen. `--headless` is something else, a dedicated server (#144) that is not there yet |
| `--frames N` | Render N frames, then exit |
| `--screenshot PATH` | Save the last frame to a virtual path before exiting |

The app logs where the image was written.

## Current state

| Part | State |
|---|---|
| Dependencies, shader compilation in the build, containers | Done |
| neon-core changes | Done |
| Backend: device, meshes, models, textures, shaders, materials, lights | Done |
| Drawing the demo scene into an image | Done, checked on macOS and Linux |
| Presenting to a window | Done, checked on a screen on macOS |
| Removing the OpenGL backend | Done |
| Windows | Compiles. Not run |

**How it was checked.** The demo scene was rendered without a window and the
saved image inspected. On macOS it was also run in a window.

| Platform | Driver | Without a window | In a window |
|---|---|---|---|
| macOS arm64 | MoltenVK on the Apple GPU | Scene drawn correctly | Scene drawn correctly |
| Linux, in the container | Mesa lavapipe, on the CPU | The same image | Not run |
| Windows | | Not run | Not run |

Two unrelated drivers producing the same picture is good evidence that the
renderer follows the rules of Vulkan and does not lean on one driver.

## What changed in the picture

The Vulkan renderer draws what the OpenGL shaders were written to draw. The
OpenGL backend fell short of that in two places, so scenes look slightly
different from how they did.

| Subject | The removed OpenGL backend | Vulkan backend |
|---|---|---|
| Position of the camera | The shader has a `view_position` uniform that is never set, so highlights are computed as if the camera stood at the origin | Taken from the view matrix |
| Highlights of the direction light | Set under the name `dirLight.specular`, which the shader does not have. They stay black | Set |
| Number of lights | 16 point and 16 spot lights, the most that fit Apple's uniform limit | 64 of each, held in a uniform buffer |

Later, lighting moved to linear light (see color spaces above). A lit
surface turned brighter in its middle tones, and light falls off more
softly, which is what light does. Scenes whose lights were set to look
right before may want weaker `ambient` and `diffuse` values.

## Known limits

| Limit | Detail |
|---|---|
| One frame at a time | The renderer waits for a frame to finish before starting the next. Simple and correct, but it leaves speed on the table |
| Buffers live in memory the processor writes to | Fine for the sizes in use. Copying to memory owned by the graphics card is the next step if a profile asks for it |
| No validation layers | They need the Vulkan SDK. See open questions |
| Image decoding lives in the backend | stb_image is compiled into neon-vulkan. It belongs in neon-core, where a second renderer could share it |
| What a camera draws into a texture is resolved into bytes | Its scene goes through the resolve, with the tonemapper of the run, into the bytes of the target, so light brighter than white, and what is later done with it, stays in the frame. See [tonemapping](#tonemapping) |
| An opaque material writes alpha 1 over an opaque clear | The pipeline of an opaque material keeps the larger of the alpha the shader wrote and the alpha of the scene image (`VK_BLEND_OP_MAX`), since Vulkan has no blend factor that writes a constant. Over a frame, which is cleared opaque, that is 1 whatever the shader wrote. A texture a camera clears to a see-through color relies on the shader, and the shaders of the engine write 1 through `object_alpha()` in `scene-data.glsl` for that case |

## Order of work

| Step | Content | State |
|---|---|---|
| 1 | Submodules, shader compilation in the build, docker images | Done |
| 2 | neon-core changes | Done |
| 3 | Backend up to the demo scene drawn into an image | Done |
| 4 | Presenting to a window, checked on a screen | Done on macOS |
| 5 | Removing the OpenGL backend | Done |
| 6 | Running on Windows, and on Linux with a real graphics card | Open |
| 7 | A setting for the Vulkan version, and capabilities the engine can ask for | Done |
| 8 | Headless as a finished feature | Later |

## Open questions

- Shipping on macOS means bundling MoltenVK with the app, until there is a
  Metal renderer. Its license allows that.
- Validation layers are useful in debug builds but need the Vulkan SDK
  installed. Enable them only when present?
