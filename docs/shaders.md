# Shaders

This note records how shaders are written once and reach every renderer, which
tools do the work for each target, and what the sources have to keep to for
that to stay mechanical. The Vulkan renderer's own note is
[vulkan-renderer.md](vulkan-renderer.md); the shaders that ship are listed in
its [shaders section](vulkan-renderer.md#the-shaders-that-ship).

**Current decision (#89, 2026-10-02):** shaders are written in **GLSL 450**,
once. **SPIR-V is the one intermediate form**: glslang produces it at build
time, the Vulkan renderer loads it, and every other renderer starts from it —
cross-compiled to that target's language at build time. Nothing is translated
while a game runs, and no shader source ships. Slang (#110) was weighed and
set aside: its Metal and WGSL targets are marked experimental, and those were
the two reasons to switch.

## The pipeline

```
GLSL 450 ──glslang──► SPIR-V ──┬─► Vulkan (as it is)                         #23, done
                               ├─► MoltenVK on macOS (as it is, until #65)   today
                               ├─SPIRV-Cross─► MSL ──metal──► .metallib       #65 Metal
                               ├─SPIRV-Cross─► HLSL ──dxc──► DXIL             #109 DirectX 12
                               └─Tint or Naga─► WGSL                          #66, #86 WebGPU
```

One source, one build step per renderer, one set of compiled files per
platform. The renderer picks the file for its target by name; how the targets
are named side by side is open, see below.

## The tools, by target

| Target | From SPIR-V to | Tool | Standing | Where it runs |
|---|---|---|---|---|
| **Vulkan** (#23) | SPIR-V, as it is | [glslang](https://github.com/KhronosGroup/glslang) (`glslangValidator`), the reference compiler | Mature, in use | Build time, `cmake/CompileShaders.cmake` |
| **macOS today** | SPIR-V, as it is | [MoltenVK](https://github.com/KhronosGroup/MoltenVK) runs SPIRV-Cross on every start | Mature, in use | Run time, inside the driver, until the Metal renderer |
| **Metal** (#65) | MSL | [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross), then Apple's `metal` compiler (`xcrun metal`, `metallib`) into a `.metallib` | SPIRV-Cross mature: it is what MoltenVK, Unity, and Godot use | Build time, on a Mac or with Apple's tools |
| **DirectX 12** (#109) | HLSL, then DXIL | SPIRV-Cross to HLSL (shader model 6), then [DXC](https://github.com/microsoft/DirectXShaderCompiler) | Both mature, Microsoft's own compiler | Build time; DXC runs on every platform, the llvm-mingw image for Windows builds carries it |
| **WebGPU in a browser** (#66, #86) | WGSL | [Tint](https://dawn.googlesource.com/dawn), Dawn's compiler, or [Naga](https://github.com/gfx-rs/wgpu/tree/trunk/naga), wgpu's | Production: every Chrome and Firefox tab runs one of them. SPIRV-Cross has no WGSL target | Build time, as a library of the WebGPU backend's own dependency |
| **WebGPU on a desktop** | SPIR-V, as it is | Dawn and wgpu accept SPIR-V directly | | Nothing to do |

All of it is done at build time and later by the editor's compiler tools
(#83); only what a renderer loads ships with a game.

## What the sources keep to

Every target can take what Vulkan takes, but not every target takes
everything. The sources stay within what all of them accept, so that the
cross-compilers never meet a feature they cannot express:

| Rule | Why | State |
|---|---|---|
| GLSL 450 with `#version 450`, Vulkan's dialect (`set` and `binding` layouts) | What glslang compiles to SPIR-V and SPIRV-Cross reads back | Kept |
| No 8- and 16-bit types, no 64-bit floats | WGSL has none of them; Metal has no doubles | Kept |
| `layout(set, binding)` as the one binding model | MSL argument indices, HLSL register spaces, and WGSL `@group/@binding` are all made from it by the tools; nothing is bound by name | Kept |
| Push constants small, and only for what changes every draw | WGSL has none; Tint and Naga emulate them with a uniform buffer | One push constant block today (`ui-frame.glsl`); keep it so |
| **Separate textures and samplers**, `texture2D` and `sampler`, in place of `sampler2D` | WGSL has no combined image sampler. Tint and Naga split a combined one for us, SPIRV-Cross for MSL too, but each makes up a sampler and the bindings move; written apart, every target binds what the source says | **To do** before the second renderer: the shaders use `sampler2D` today |
| Matrices column-major, `std140` for uniform buffers | What every target agrees on; the Vulkan backend lays its structures out so already (`vk-shader-data.hpp`) | Kept |
| Depth from 0 to 1, Y of the clip space as Vulkan has it | Metal and DirectX agree on depth; Vulkan's clip Y points down, theirs up, which SPIRV-Cross fixes with a flag (`--flip-vert-y`) or the renderer with its projection | Kept; the Vulkan backend uses a negative-height viewport, see [vulkan-renderer.md](vulkan-renderer.md#versions-of-vulkan) |
| No `gl_FragCoord` origin assumptions, no `gl_VertexIndex` tricks | The origin differs between Vulkan and Metal; SPIRV-Cross fixes `gl_FragCoord`, but a shader that computes with it is safer not to | Kept |
| Includes resolved before any compiler | `#include` is glslang's `GL_GOOGLE_include_directive`, a vendor extension; the stitching tool below takes it over so the sources belong to no one compiler | To do, with the tool |

## No shading language of our own

Neon does not define a shading language, and will not for 1.0. The language is
GLSL; the compilers are glslang, SPIRV-Cross, DXC, and Tint or Naga, all made
and kept by others; what is ours is the build step that calls them. A language
of our own — a syntax, a parser, an emitter for every target — is what Godot
and Unity carry and what SPIR-V as the intermediate form spares us. If one ever
comes, after 1.0, it sits in front of the same pipeline and changes nothing
behind it.

## The stitching tool

Godot's shader compiler takes a short material program and wraps it into the
complete vertex and fragment stages its renderer needs, then hands GLSL to the
same pipeline. Neon will do the same with GLSL as the syntax, in a small tool
of its own, the first of the editor's compiler tools (#83). It is a text step,
not a compiler: it pastes files together and never reads what it pastes. The
wrapping is optional, too — the UI shaders do it by hand today with
`#include "ui-shader.glsl"`, which is enough for 1.0.

1. **Includes** resolved by plain text, so the sources depend on no vendor
   extension and a shader of a game includes the engine's `scene-data.glsl`
   as a file.
2. **Wrapping**: a material of a game writes only what differs, a `fragment()`
   and perhaps a `vertex()` block, and the tool stitches it into complete
   stages with the engine's bindings, inputs, lighting, and `object_alpha()`.
   That keeps PBR (#59), shadows (#60), and emissive (#129) out of every
   game's shader, and it is what the UI shaders (`ui-shader.glsl`) do by hand
   already.
3. **Checking** against the rules above with messages that name the file and
   the line, as recipes are checked.
4. **Compiling**: glslang as a library for SPIR-V, then SPIRV-Cross and Tint
   as libraries for the other targets, DXC as a program. One tool, nothing to
   install.

It runs at build time now and inside the editor later. The runtime never sees
a source.

## How it is built today

| Piece | Location | Role |
|---|---|---|
| The sources | `app/NeonRuntime/shaders/vulkan/*.vert`, `*.frag`, `*.glsl`, `ui/*.frag` | GLSL 450, Vulkan's dialect |
| `cmake/CompileShaders.cmake` | The build | Finds `glslang` or `glslangValidator`, compiles every `.vert` and `.frag` to `assets/shaders/<name>.<stage>.spv`, copies the result next to the binary |
| `VK_Shader` | neon-vulkan | Loads `<name>.vert.spv` and `<name>.frag.spv` through the file system |
| A material's `shader` | A scene recipe | Names a shader without an extension, `assets://shaders/pbr`; the renderer adds what it needs |

## Open questions

- **Naming the compiled files side by side:** `pbr.frag.spv`, `pbr.frag.metallib`,
  `pbr.frag.dxil`, `pbr.frag.wgsl` in one folder, or a folder per target, or
  only the target's files in an export (#84). Decided with the second
  renderer.
- **When the second renderer starts** (#87): the more shaders exist before it,
  the more there is to run through the pipeline once; the pipeline itself
  does not grow with them.
- **The Metal compiler needs a Mac**, or Apple's tools. A Windows or Linux
  build that targets macOS would ship MSL text and let Metal compile it on
  first start, which Metal allows, at a cost at that start. Decided with #65.
- **Debugging**: a crash in a cross-compiled shader points at MSL or HLSL the
  author never wrote. SPIRV-Cross keeps line directives when asked; the tool
  should pass them through.
- **Slang** (#110) stays an option if its Metal and WGSL targets mature; the
  renderer sees only compiled files, so the switch would be the tool's.
