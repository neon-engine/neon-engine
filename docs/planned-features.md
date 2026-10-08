# Planned features

What Neon Engine does not do yet, and is planned to. What it does today is
listed in the [README](../README.md#features). This is a plan, not a promise:
items move as they are understood better. The [roadmap](roadmap.md) has the
plan in detail, the order of the work, and the decisions still open.

## Platforms

- Tested releases for Linux and Windows
- Native Wayland and X11 on Linux, picking the one the session uses
- The web, through WebAssembly and WebGPU
- SDL3, under consideration as a backend beside SDL2

## Rendering

- Shadows from point and spot lights, soft shadows, and contact shadows
- Clustered forward lighting, for thousands of lights, and area lights
- Ambient occlusion and screen-space reflections
- GPU particles and visual effects
- Decals
- Bloom, antialiasing (TAA, MSAA, and FXAA), and upscalers
- Quality presets from low to ultra, covering shadows, lighting, and effects
- Frustum and occlusion culling, instancing, and indirect draws
- Skinned meshes and skeletal animation
- Global illumination, voxel-based first and other kinds where they fit, and
  baked lighting from the editor
- Hybrid rendering: hardware ray tracing alongside the rasterizer, for shadows,
  reflections, and light, with a fallback where there is none
- Sky lights: ambient light and reflections taken from the sky
- Volumetric fog and volumetric clouds, with light shafts
- A programmable rendering pipeline and compute shaders
- Metal, WebGPU, and Direct3D 12 renderers, possibly through SDL3's GPU API
  beside the Vulkan renderer

## Procedural generation

- Terrain, with a level of detail that follows the camera, and collision
- Foliage scattered by rules, drawn instanced, and moved by wind
- Textures from noise and patterns
- Clouds shaped by noise and weather

## The world

- Scenes and resources saved in a binary form, and packed assets
- Assets streamed in and out by distance, and worlds with a floating origin

## Physics

- A debug view of colliders
- Crouching, and shape casts that return every hit

## Sound

- Music streamed from its file
- Sound that follows the shape of a room, with
  [Steam Audio](https://github.com/ValveSoftware/steam-audio)

## Input

- Bindings a player changes

## User interface

- Localization and rich text
- An on-screen keyboard

## Scripting and code

- Hot reload of scripts and of extensions
- Bindings through LuaJIT's FFI

## Automation and AI tools

- Setting the state of a game and starting it at a given frame
- NeonEditor driven from the command line: scenes, assets, running the game,
  the log, and exporting
- A [Model Context Protocol](https://modelcontextprotocol.io) server, built in
  as an option, offering the same as tools
- Plugins: optional features you add, such as a chat inside the editor

## Tools

- NeonEditor
- Exporting a game for players
- Networking: replication, dedicated servers, and clients
