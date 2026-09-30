# Roadmap

Where Neon Engine is heading, what is decided, and what is still open. It is
a plan, not a promise. Items move as they are understood better.

The goal is a complete engine: rendering, physics, audio, scripting, user
interfaces, and an editor to make games with.

## The shape of the product

Neon Engine is built as two applications on one engine, in the way Godot is.

| Application | What it is | State |
|---|---|---|
| **NeonRuntime** | What gets distributed with a game. It loads a project and runs it. The project's assets ship next to it, not inside it | Exists. Runs a scene from a file |
| **NeonEditor** | Where games are made. A special kind of runtime, with everything the runtime has plus tools of its own | Not started |

Principles that hold for both:

- **One engine, reached through interfaces.** Windowing, input, rendering, and
  file access sit behind interfaces in neon-core. The editor extends the
  runtime by adding to what is there, not by changing it.
- **Content works everywhere.** A project made on one platform runs on the
  others unchanged. See the path rules in the
  [development guide](development.md#path-rules).
- **The runtime is separate from the game.** Assets, and later game code in
  scripts and libraries of its own, are loaded by the runtime and not compiled
  into it.

## Done

| Area | What exists |
|---|---|
| Platforms | Builds on macOS, Linux, and Windows with one toolchain |
| Renderer | Vulkan, on all three. See [vulkan-renderer.md](vulkan-renderer.md) |
| File system | Virtual paths with `assets://`, `user://`, and `output://`. See [file-systems.md](file-systems.md) |
| Window modes | Windowed, borderless, and fullscreen |
| Command line | An abstraction with a set of options for each application |
| Runtime class | The core class an application is built from is `neon::Runtime` |
| Sound | Sound effects and music with miniaudio, behind an interface. Sounds have a place in the world. See [audio.md](audio.md) |
| Reflection | A component is described once, next to itself. Scene files follow from it, and the editor and scripts will. See [reflection.md](reflection.md) |
| Scenes in files | A scene is a YAML file of entities and components, made to be changed by hand. See [scenes.md](scenes.md) |
| Entities and components | The world is made of entities, components, and systems, with Flecs behind an interface. See [entity-component-system.md](entity-component-system.md) |
| Running without a window | `--headless`, `--frames`, and `--screenshot` render a number of frames and save the last one |
| Screenshots where the caller wants them | `--output-dir` chooses the folder behind `output://` |
| Screenshots at chosen frames | `--screenshot-at` runs the game forward and saves each frame that is listed |
| Fixed time step | `--time-step` advances every frame by the same amount of time |
| Exit codes | The runtime returns a failing exit code when a screenshot could not be written or the run ended with an exception |
| Physics | Static, kinematic, and dynamic bodies, triggers, and characters that move and slide, with Jolt Physics behind an interface. The world steps at a fixed rate. See [physics.md](physics.md) |
| Unit tests | GoogleTest, with tests for the four libraries and for the runtime as a whole. See the [development guide](development.md#tests) |

## Next

Work that builds directly on what exists.

| Item | Detail |
|---|---|
| Load a project | The runtime takes a project to run. It takes a scene today, with `--scene`. What a project is on disk is open |
| Vulkan version and capabilities | A setting for the version to ask for, and a way for the engine to learn what the graphics card can do, so that menus only offer what works |
| Run on Windows | The build works. It has not been run |
| Log file under `user://` | It is still written relative to the working directory |

## Planned

Everything the engine needs to be complete, by area. The order to build it in
is [below](#order).

### Foundations

These come first, because everything after them is cheaper with them in place.

| Item | Detail |
|---|---|
| clang-tidy | Checks that code follows the conventions the code base already has, such as naming. Run in the build and in the editor |
| The TODOs in the repository | Listed [below](#todos-in-the-repository) |
| Architecture document | How the libraries, interfaces, and applications fit together, and why |
| A document for each major feature | In the way [file-systems.md](file-systems.md) describes the file system: what it does, how it is used, and what is open |

### World

| Item | Detail |
|---|---|
| More of the physics | Joints, locked axes, shapes that are cast along a way, and shapes that change while a body lives. See [physics.md](physics.md#limits) |
| Scripting | Lua first. Game code is loaded by the runtime, not compiled into it. Bindings for other languages can follow the same interface |

Both attach to entities and components, which exist. Physics is components
and a system. A script declares components and systems of its own.

### Rendering

| Item | Detail |
|---|---|
| Physically based materials | Metalness and roughness, as glTF describes them |
| Shadow mapping | For directional, point, and spot lights |
| Deferred rendering | A second pipeline next to the forward one, for scenes with many lights |
| Compute shaders | Used by the engine, and offered to games through the render interfaces |
| Light mapping | Lighting computed ahead of time for what does not move. Baked by the editor |
| Global illumination | Which technique is open. It builds on deferred rendering and compute shaders |
| Metal renderer | macOS without MoltenVK |
| WebGPU renderer | The web. Through Dawn or wgpu also the desktop platforms |

**Shaders are written once.** They are compiled to SPIR-V, and
[SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) turns that into
what Metal and WebGPU want. A second renderer adds a step to the build, not a
second set of shaders. [Slang](https://shader-slang.org) is the alternative if
shaders grow numerous.

Every technique above has to be built once for each renderer. The more of them
exist before Metal and WebGPU are started, the more there is to port. The more
renderers exist, the more each new technique costs. See [order](#order).

### Audio

| Item | Detail |
|---|---|
| Spatial audio with Steam Audio | Sounds that are shaped by the room they are in. Sounds already have a place in the world, with miniaudio |
| Groups and fading | A volume for music, effects, and voices each. Fading between pieces of music |

### User interface

| Item | Detail |
|---|---|
| A framework for interfaces in games | Menus and the display shown during play. Layout, text, input, and drawing through the renderer |

This is separate from the interface of the editor, which uses Dear ImGui.
Dear ImGui suits tools. It is not meant for what players see.

### Running without a window

Rendering without a window, saving chosen frames to a chosen folder, and a
fixed time step exist. See the
[development guide](development.md#running-without-a-window). What is planned
makes them a supported way to run a game, for automated checks and for agents.

| Item | Detail |
|---|---|
| Setting the state of a game | Options to start from a given state, such as a scene, a saved game, or values of the game's own. Waits for scenes that load from files |
| Fixed random numbers | A seed given on the command line, so that a game that uses random numbers gives the same image on every run. Nothing in the engine draws random numbers yet |

### NeonEditor

| Part | Detail |
|---|---|
| Interface | Built with [Dear ImGui](https://github.com/ocornut/imgui) |
| Editing | Scenes, assets, and settings of a project |
| Running a game from the editor | With options the runtime alone does not have, for debugging |
| Exporting | Turning a project into something that can be distributed |
| Compiler tools | The tools that prepare a project are packaged with the editor: compiling shaders, preparing models and textures, and baking light maps |
| Command line | The options of the runtime, plus a set only the editor has |
| Agent support | A Model Context Protocol server. See below |

### Exporting games

The editor can export in two ways.

| Way | How | When |
|---|---|---|
| With precompiled runtimes | The editor takes a NeonRuntime that was built ahead of time for the target and puts the project next to it | The normal case. Needs no compiler |
| By compiling | The editor builds the runtime for the target, inside a container | When the runtime itself was changed |

**Containers** already build for Linux and cross-compile for Windows. See
the [development guide](development.md).

**macOS is the exception.** Two different things have to be told apart:

| Task | Can it be automated away from a Mac |
|---|---|
| Packaging a project with a precompiled macOS runtime | **Yes, from any platform.** An app bundle is a folder with a fixed layout |
| Signing and notarizing the result | **Yes, from any platform**, with [rcodesign](https://github.com/indygreg/apple-platform-rs), an open implementation of Apple's signing. It needs an Apple developer account |
| Compiling the runtime for macOS | **Only on Apple hardware.** Apple's license ties its SDK to its machines, and macOS does not run in a container |

So exporting to macOS can work from Windows and Linux, as long as the macOS
runtime was compiled somewhere. Compiling it can still be automated, on a Mac:

| Option | Detail |
|---|---|
| A hosted macOS runner | Offered by the common CI services |
| A Mac of your own as a runner | A Mac mini that builds when asked |
| macOS virtual machines on a Mac | Tools such as Tart run them on Apple Silicon, which gives clean builds the way containers do |

### Agent support in the editor

The editor is meant to be driven by AI agents as well as by hand: Claude
Code, ChatGPT, Cursor, and others.

**Decided: the Model Context Protocol.** It is an open protocol for giving
an agent tools, and the products named above already speak it. The editor
runs a server that offers its abilities as tools. One implementation then
serves every agent, and a protocol of our own is not needed.

| Tool an agent could be given | Built on |
|---|---|
| Read and change a scene | The scene graph |
| List, import, and inspect assets | The file system |
| Run the game and stop it | The runtime |
| Look at what is on screen | Rendering without a window and saving the frame, which exists |
| Put the game into a state and look at it | Setting the state and starting at a frame, which are planned |
| Read the log | The logging system |
| Export the project | The exporter |

Looking at the screen is what lets an agent check its own work. The pieces for
it are in place.

Open: whether agents talk to a running editor, to a separate process without
a window, or both.

### Web builds

Players should be able to try a demo in a browser without downloading a
client. That means building for WebAssembly.

| Needed | Detail |
|---|---|
| A renderer for the web | The WebGPU renderer. Browsers have no Vulkan |
| A main loop the browser drives | The browser calls the engine once per frame. The engine cannot loop by itself |
| Assets served with the page | A file system backend that reads what was packed |
| A fourth toolchain | Emscripten |

Threads are limited in browsers, which will matter for physics.

### Other

| Item | Detail |
|---|---|
| Replace assimp | It is temporary. It is large, and a game should load models that were prepared ahead of time |
| Packed assets | Shipping archives instead of loose files. See [file-systems.md](file-systems.md) |

## Order

A proposal. Each step builds on the ones before it.

| Step | What | Why here |
|---|---|---|
| 1 | clang-tidy, the TODOs | Small, and they protect everything that follows. Unit tests are done, and new code comes with tests |
| 2 | Architecture document | Writing it down exposes what the next steps have to change |
| 3 | Load a project, and the rest of [Next](#next) | The runtime has to run something other than a scene written in code |
| 4 | Entities and components, and scenes in files | Done |
| 5 | Scripting with Lua | Physics is done. With both, a game can be written without touching the engine |
| 6 | Running without a window, in full | Makes every later feature checkable by a script or an agent |
| 7 | Sound effects and music are done. Steam Audio is left | Independent of rendering |
| 8 | Physically based materials, shadow mapping | The largest gain in how a scene looks |
| 9 | Deferred rendering, compute shaders | What the advanced lighting builds on |
| 10 | User interface framework | Needs the renderer to be settled enough to draw through |
| 11 | NeonEditor, with its compiler tools and the agent server | Needs most of the above to have something to edit |
| 12 | Light mapping, global illumination | Light maps are baked by the editor |
| 13 | Metal and WebGPU renderers, web builds | See below |

**Where the renderers go is the main open question of the order.** Placed
last, they have the most to port, all at once. Placed early, every rendering
step after them is built three times. A middle way is to start the second
renderer after step 8, while the render interfaces are still small, and to
hold later techniques to what both can do.

A document for each feature is written with the feature, not as a step.

## TODOs in the repository

| Where | What |
|---|---|
| [CMakeLists.txt](../CMakeLists.txt) | Turn on `-Wall`, `-Wextra`, and `-Werror`, once the libraries in `external/` no longer compile as part of the project |
| [CMakeLists.txt](../CMakeLists.txt) | Set the compiler flags for release builds |
| [CMakeLists.txt](../CMakeLists.txt) | Consider whether the C++ runtime still has to be linked statically |

## Open decisions

| Decision | Depends on |
|---|---|
| When the Metal and WebGPU renderers are started | How much is to be ported, against how much is to be built three times |
| Whether WebGPU also serves the desktop | If it does on macOS, the Metal renderer has less to justify it |
| Shaders in GLSL or in Slang | How many shaders there will be. Cheap to change now, expensive later |
| Which technique for global illumination | What the target hardware is, and whether the web has to be able to run it |
| How agents reach the editor | What the editor turns out to be |
| What a project is on disk | Needed before the runtime can load one |
| How game code is loaded | Scripts are decided. Whether libraries loaded at run time are offered as well is open |
