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
3. Later: run headless as a feature of its own, with no window, no display,
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
| `max_samples` | The most samples of anti-aliasing that colour and depth both have. 1 is none |
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
| Sources kept apart from the assets | They live in `app/<app>/shaders/vulkan`, written in GLSL 450. Another renderer would get a folder of its own |
| Compiled at build time | glslang turns each source into a `.spv` file placed in the app's assets folder |
| Named without an extension | A material names `assets://shaders/basic-lit` and the renderer adds `.vert.spv` and `.frag.spv` |

Scene files therefore do not change with the renderer.

Light data is held in a uniform buffer, not in individual uniforms. That
removes the limit on uniforms per shader, which is what had held the OpenGL
backend to 16 lights of each kind.

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
| `VK_Canvas` | Where the frame, or a render target, is drawn: its stages, its clear colour, its scene image, and the see-through models kept for the end of its scene. The frame and every render target own one |
| `VK_Pipelines` | The pipelines of materials, one per variant of shader, covering, and culling, and the layout they share |
| `VK_Capture` | Reads a finished frame back and writes it as a PNG |
| `VK_Model`, `VK_Mesh` | Vertex and index buffers. `VK_Model` derives from the core `Model`, which does the loading through assimp for every renderer |
| `VK_Texture` | Image, view, and sampler |
| `VK_Shader` | Shader modules from SPIR-V |
| `VK_Material` | Textures, descriptor set, per-object data, and which pipelines draw it |
| `VK_SceneImage` | The image of linear light a scene is lit in, with its depth |
| `VK_Resolve` | The resolve step, which turns a scene image into the colours of the image that is shown |
| `VK_RenderTarget` | An image that is drawn to like the frame, and read as a texture |
| `VK_Renderer2D` | What is drawn in two dimensions, on top of the resolved scene |
| `VK_SwapchainSizing`, `VK_FrameStages`, `VK_DrawOrder`, `VK_Culling` | The decisions of the renderer, kept apart from the graphics card so that they are tested |

**Render to an image first, always.** Every frame is drawn into an offscreen
image. With a window, that image is then copied to the swapchain. Headless,
it is simply left there to be captured. Both modes share all drawing code, so
what a screenshot shows is what a window would show.

## The order of a frame

| Stage | Drawn into | What happens |
|---|---|---|
| Render targets | Each target, in the order they are drawn | Each goes through the stages below on its own: a camera that draws into a texture lights a scene in a scene image of the target, a user interface on a surface draws on top. A target that shows only a user interface has no scene image |
| Scene | The scene image, `R16G16B16A16_SFLOAT`, and its depth | Opaque models as they come, then see-through ones from the farthest to the nearest, tested against depth but not writing it. The back of every triangle is left out unless the material is double-sided. Lighting and blending are in linear light |
| Resolve | The image that is shown, `R8G8B8A8_UNORM` | A triangle that covers it reads the scene image pixel by pixel, clamps it, and writes it in sRGB. The one place where light becomes the colours of a screen |
| On top | The same image | What is drawn in two dimensions: user interfaces, blended in sRGB as CSS blends them |
| Copy | The window, or a file | Byte for byte. The bytes are sRGB already |

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
that bleeds around what is bright is added to the scene image just before
it. A curve for light brighter than white replaces the clamp in
`resolve.frag`. The scene image keeps such light until then.

## Colour spaces

Colours are written the way a screen shows them, in sRGB: in image files, in
scene files, and in the style sheets of a user interface. Light adds up and
blends in linear terms, where 0.5 is half the light of 1. In sRGB, 0.5 is
about a fifth of it. The scene is therefore lit in linear light, and user
interfaces are drawn in sRGB, which is what CSS blends in.

| What | How it is read |
|---|---|
| The first texture of a material | An sRGB format, read as linear light. Its smaller copies are made by the graphics card in linear light |
| The second texture of a material | Plain bytes. It says how much a surface shines, which is a number and not a colour |
| The `color` of a material, the colour a camera clears its texture to | Turned into linear light before the shaders see it |
| `ambient`, `diffuse`, and `specular` of a light | Amounts of light, handed over as they are. 0.5 is half the light |
| Images, glyphs, and colours of a user interface | Plain bytes and sRGB numbers, blended as they are, as before and as CSS does |
| A render target | Holds sRGB colours as bytes. A model reads it through an sRGB view, as linear light. A user interface reads it through a view of plain bytes |
| The window | A format of plain bytes. A format that converts to sRGB would convert the frame a second time, and is taken only when there is nothing else, with a warning |
| A saved frame | Copied byte for byte, and sRGB |

The scene `gamma-test.scene.yml` and the tests `runtime-colours` check this
pixel by pixel: half of red over black is 188, the light of half of red, and
not 128, while half of black over white in a user interface is 127, as in a
browser.

## The command line

```bash
NeonRuntime
```

For development, a scene can be rendered without a window:

```bash
NeonRuntime --headless --frames 3 --screenshot user://screenshots/frame.png
```

| Option | Meaning |
|---|---|
| `--renderer vulkan` | Renderer to use. Vulkan is the only one so far |
| `--vulkan-version 1.N` | The highest version of Vulkan to render with. 1.3 unless it is given |
| `--headless` | No window and no input |
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

Later, lighting moved to linear light (see colour spaces above). A lit
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
| What a camera draws into a texture is clamped | Its scene is resolved into the bytes of the target, so light brighter than white, and what is later done with it, stays in the frame |
| Shaders of models write alpha 1 when opaque | A shader of its own has to do the same through `object_alpha()` in `scene-data.glsl`, or its alpha comes out in the resolve |

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
