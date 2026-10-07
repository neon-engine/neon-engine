<p align=center>
<img src="./docs/logo/main-logo.png" width=320>

<h1 align=center>a[N]other [E]xtensible [O]pen-source e[N]gine</h2>
</p>

## Description

> **Note:** Neon Engine is a working title, and the name is subject to change.

Neon Engine is a modular, high-performance game engine written in C++.
It leverages the power of existing open source libraries while maintaining a clean,
extensible architecture.

What started as a hobby engine, the goal is for the engine to support multiple platforms
and provide intuitive tools for creating interactive experiences without sacrificing low-level control.
Neon Engine aims to balance flexibility with simplicity, making game development accessible
while allowing for advanced customization.

Neon Engine is made to be used by people and by AI agents alike, in whatever mix suits you.
Everything can be done by hand, and the same project can be plugged into pipelines of AI
agents: scenes, prefabs, user interfaces, and settings are text files with errors that name
the line, and a game runs without a window, takes its input from a script, and saves the
frames it draws. NeonEditor will adapt to both ways of working.

Agents drive the engine from the command line first, which costs them the fewest tokens: every
run, screenshot, and scripted play-through is a command. For organizations whose workflows are
built around the [Model Context Protocol](https://modelcontextprotocol.io), the engine will have
an MCP server built in as an option. A chat inside the editor is not planned as part of the
engine; it may come later as a plugin, should one be needed.

Special thanks to [Akusha](https://vgen.co/Akusha/portfolio) for the logo for the project

## Quick start

The fastest way to see the engine is the museum: one scene, walked through
in the first person, with a hall for each part of the engine. These steps are
for macOS on Apple silicon. Linux and Windows are built in Docker images, as
[development.md](./docs/development.md) describes.

1. Install the toolchain with [Homebrew](https://brew.sh):

   ```sh
   brew install llvm@20 cmake ninja glslang molten-vk vulkan-loader
   ```

2. Clone the repository and fetch the libraries the build uses:

   ```sh
   git clone ssh://git@git.traparwave.net:3022/neon-engine/neon-engine.git
   cd neon-engine
   git submodule update --init --recursive \
     external/glm external/sdl2 external/assimp external/jolt-physics \
     external/spdlog external/rapidyaml external/stb external/googletest \
     external/freetype external/harfbuzz external/lunasvg external/luajit \
     external/vulkan-headers external/volk external/flecs external/miniaudio
   ```

3. Configure and build. The build preset builds NeonRuntime, the museum, and
   the bench:

   ```sh
   cmake --preset macos-arm64-debug
   cmake --build --preset macos-arm64-debug
   ```

4. Run the museum:

   ```sh
   ./bin/debug/darwin-arm64/museum/NeonRuntime
   ```

| Action | Keyboard and mouse | Gamepad |
|---|---|---|
| Move | W, A, S, D | Left stick |
| Look | Mouse | Right stick |
| Jump | Space | South button |
| Run | Left Shift | Left stick press |
| Press a screen in the world | Left mouse button, aiming with the dot in the middle | |
| Pause | Escape | Start |

The halls and what each one shows are listed in
[scenes.md](./docs/scenes.md#the-museum). For a faster build, use
`macos-arm64-release` in place of `macos-arm64-debug`, and run
`./bin/release/darwin-arm64/museum/NeonRuntime`.

## Features

What the engine does today, and what is planned. Each part has a note under
[docs](./docs) that says how it works and why it was built that way.

### Platforms

- [x] macOS on Apple silicon, through MoltenVK
- [x] Builds for Linux x64 and Windows x64, in Docker images
- [ ] Tested releases for Linux and Windows
- [ ] The web, through WebAssembly and WebGPU

### Rendering ([vulkan-renderer.md](./docs/vulkan-renderer.md))

- [x] A Vulkan renderer, from version 1.1 on
- [x] Physically based materials, with lit and unlit shaders beside them
- [x] Direction, point, and spot lights
- [x] Cascaded shadow maps for the direction light
- [x] Light kept in high range, with ACES and AgX tonemapping and exposure
- [x] Lightmaps, emissive surfaces, and see-through materials
- [x] Skies from a cube of images or a panorama
- [x] Effects of a camera, as shaders a game brings
- [x] Cameras that draw into a texture
- [x] Models from glTF, with their materials, textures, and vertex colors ([models.md](./docs/models.md))
- [x] Shapes built from numbers: boxes, prisms, spheres, tubes along a curve ([geometry.md](./docs/geometry.md))
- [x] Rendering without a window, for tests and screenshots
- [x] Vertical sync, a frame limit, and window modes that change while the game runs
- [ ] Shadows from point and spot lights, soft shadows, and contact shadows
- [ ] Clustered forward lighting, for thousands of lights, and area lights
- [ ] Ambient occlusion and screen-space reflections
- [ ] Particles and visual effects, simulated on the graphics card
- [ ] Decals
- [ ] Bloom, antialiasing (TAA, MSAA, and FXAA), and upscalers
- [ ] Quality settings for weak and strong computers: texture resolution from 1x down to 1/8x, anisotropic filtering, and the quality of shadows, lighting, and effects
- [ ] Frustum and occlusion culling, instancing, and indirect draws
- [ ] Skinned meshes and skeletal animation
- [ ] Global illumination, voxel-based first and other kinds where they fit, and baked lighting from the editor
- [ ] Hybrid rendering: hardware ray tracing alongside the rasterizer, for shadows, reflections, and light, with a fallback where there is none
- [ ] Sky lights: ambient light and reflections taken from the sky
- [ ] Volumetric fog and volumetric clouds, with shafts of light
- [ ] A programmable rendering pipeline and compute shaders
- [ ] Metal, WebGPU, and DirectX 12 renderers

### Procedural generation

- [ ] Terrain, with a level of detail that follows the camera and collision
- [ ] Foliage scattered by rules, drawn instanced, and moved by wind
- [ ] Textures from noise and patterns
- [ ] Clouds shaped by noise and weather

### The world

- [x] Entities and components on [Flecs](https://github.com/SanderMertens/flecs) ([entity-component-system.md](./docs/entity-component-system.md))
- [x] Scenes and prefabs as YAML files that are written by hand, with errors that name the line ([scenes.md](./docs/scenes.md), [prefabs.md](./docs/prefabs.md))
- [x] Reflection of components, which reads, writes, and checks them ([reflection.md](./docs/reflection.md))
- [x] Spawning prefabs while the game runs, and pools of instances made ahead ([pools.md](./docs/pools.md))
- [x] Projects, settings, and file systems with paths that are the same on every platform ([projects.md](./docs/projects.md), [settings.md](./docs/settings.md), [file-systems.md](./docs/file-systems.md))
- [ ] Scenes and resources saved in a binary form, and packed assets
- [ ] Assets streamed in and out by distance, and worlds with a floating origin

### Physics ([physics.md](./docs/physics.md))

- [x] Rigid bodies, colliders, triggers, and joints on [Jolt Physics](https://github.com/jrouwe/JoltPhysics), at a fixed rate
- [x] A character that walks, jumps, and climbs steps
- [x] Collision from a model or from a built shape
- [ ] A debug view of colliders
- [ ] Crouching, and shape casts that return every hit

### Sound ([audio.md](./docs/audio.md))

- [x] Sounds with a place in the world, on [miniaudio](https://github.com/mackron/miniaudio)
- [x] Groups with volumes of their own, and fades between pieces of music
- [ ] Music streamed from its file
- [ ] Sound that follows the shape of a room, with [Steam Audio](https://github.com/ValveSoftware/steam-audio)

### Input ([input.md](./docs/input.md))

- [x] Keyboard, mouse, and gamepads on [SDL2](https://wiki.libsdl.org/SDL2/Introduction)
- [x] An input map of actions, with chords, dead zones, and sensors
- [x] Which device is in use, and which family a gamepad is of, for button prompts
- [x] Input from a script, to play a game without a player in tests
- [ ] Bindings a player changes

### User interface ([user-interface.md](./docs/user-interface.md))

- [x] Interfaces as YAML files, styled with CSS
- [x] Text shaped by [HarfBuzz](https://harfbuzz.github.io) and drawn by [FreeType](https://freetype.org), with right-to-left scripts
- [x] Images, nine-slices, atlases, and SVG through [LunaSVG](https://github.com/sammycage/lunasvg)
- [x] Layout, scrolling, typing, lists, focus with a controller, and animation
- [x] Shaders on elements, and interfaces on surfaces in the world
- [ ] Localization and rich text
- [ ] An on-screen keyboard

### Scripting and code

- [x] Scripts in Lua on [LuaJIT](https://github.com/LuaJIT/LuaJIT): components and systems declared by a file ([scripting.md](./docs/scripting.md))
- [x] Native extensions in C++ behind a C boundary, with their own components, systems, assets, and shaders ([extensions.md](./docs/extensions.md))
- [x] A game in C++ that is built on its own against a runtime that is built already
- [ ] Hot reload of scripts and of extensions
- [ ] Bindings through LuaJIT's FFI

### Automation and AI workflows

- [x] Scenes, prefabs, interfaces, and settings in text, with errors that name the file and the line
- [x] Running without a window, with frames saved as images, to check what a change did
- [x] Input from a script, to play the game without a player
- [x] A command line that runs, plays, and captures a game in one call ([command-line.md](./docs/command-line.md))
- [ ] Setting the state of a game and starting it at a given frame
- [ ] NeonEditor driven from the command line: scenes, assets, running the game, the log, and exporting
- [ ] A Model Context Protocol server built in, as an option, offering the same as tools
- [ ] Plugins: optional features you add, such as a chat inside the editor

### Tools

- [x] NeonRuntime, the player a project is run with ([command-line.md](./docs/command-line.md))
- [x] The museum, a project that shows every part of the engine: [projects/museum](./projects/museum)
- [ ] NeonEditor
- [ ] Exporting a game for players
- [ ] Networking: replication, dedicated servers, and clients

## Roadmap

Where the engine is heading is described in the [roadmap](./docs/roadmap.md).

## Style guide

How the code is written is described in the [style guide](./docs/style-guide.md).

## License

Neon Engine is released under the [MIT license](./LICENSE). A game made with it
may be sold, modified, and distributed, provided the copyright notice and the
license text are included. A credit noting that the game was made with Neon
Engine is appreciated, though not required.

## Supporting the project

Neon Engine is free to use: anyone may build it from source at no cost.
Prebuilt, tested builds are planned to be offered for purchase, and will be the
most direct way to fund the engine's continued development. If you build Neon
Engine yourself and find it valuable, please consider supporting the project
through sponsorship, or by purchasing a build once they are available.

## Licenses of third parties

The libraries the engine uses, and what their licenses ask for, are listed in
[third-party-licenses.md](./docs/third-party-licenses.md).

## Development
Review the [docs](./docs) section for how to build and run the project
