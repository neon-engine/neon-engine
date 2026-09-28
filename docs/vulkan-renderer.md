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
| `WindowContext`: Vulkan surface hooks | The window system owns the window, so it is the one that can create a surface for it. They replaced `GetGlProcAddress()`, the hook OpenGL had used |
| Headless window and input systems | Implement the interfaces with no window and no devices. They live in neon-core, as they need no backend |
| File system: `user://` and `WriteBytes` | Screenshots need somewhere writable. `user://` was already planned |

## The backend library

A library of its own, `neon-vulkan`.

| Piece | Role |
|---|---|
| `VK_RenderSystem` | Implements `RenderSystem`. Owns the instance, device, and frame loop |
| `VK_Model`, `VK_Mesh` | Vertex and index buffers. `VK_Model` derives from the core `Model`, which does the loading through assimp for every renderer |
| `VK_Texture` | Image, view, and sampler |
| `VK_Shader` | Shader modules from SPIR-V |
| `VK_Material` | Pipeline, descriptor sets, and per-object data |

**Render to an image first, always.** Every frame is drawn into an offscreen
image. With a window, that image is then copied to the swapchain. Headless,
it is simply left there to be captured. Both modes share all drawing code, so
what a screenshot shows is what a window would show.

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

## Known limits

| Limit | Detail |
|---|---|
| One frame at a time | The renderer waits for a frame to finish before starting the next. Simple and correct, but it leaves speed on the table |
| The size is fixed at start | Resizing the window is not handled, which matches the rest of the engine |
| Buffers live in memory the processor writes to | Fine for the sizes in use. Copying to memory owned by the graphics card is the next step if a profile asks for it |
| No validation layers | They need the Vulkan SDK. See open questions |
| Image decoding lives in the backend | stb_image is compiled into neon-vulkan. It belongs in neon-core, where a second renderer could share it |

## Order of work

| Step | Content | State |
|---|---|---|
| 1 | Submodules, shader compilation in the build, docker images | Done |
| 2 | neon-core changes | Done |
| 3 | Backend up to the demo scene drawn into an image | Done |
| 4 | Presenting to a window, checked on a screen | Done on macOS |
| 5 | Removing the OpenGL backend | Done |
| 6 | Running on Windows, and on Linux with a real graphics card | Open |
| 7 | A setting for the Vulkan version, and capabilities the engine can ask for | Open |
| 8 | Headless as a finished feature | Later |

## Open questions

- Shipping on macOS means bundling MoltenVK with the app, until there is a
  Metal renderer. Its license allows that.
- Validation layers are useful in debug builds but need the Vulkan SDK
  installed. Enable them only when present?
