# Roadmap

Where Neon Engine is heading, what is decided, and what is still open. It is
a plan, not a promise. Items move as they are understood better.

## The shape of the product

Neon Engine is built as two applications on one engine, in the way Godot is.

| Application | What it is | State |
|---|---|---|
| **NeonRuntime** | What gets distributed with a game. It loads a project and runs it. The project's assets ship next to it, not inside it | Exists. Runs a scene that is still written in code |
| **NeonEditor** | Where games are made. A special kind of runtime, with everything the runtime has plus tools of its own | Not started |

Principles that hold for both:

- **One engine, reached through interfaces.** Windowing, input, rendering, and
  file access sit behind interfaces in neon-core. The editor extends the
  runtime by adding to what is there, not by changing it.
- **Content works everywhere.** A project made on one platform runs on the
  others unchanged. See the path rules in the
  [development guide](development.md#path-rules).
- **The runtime is separate from the game.** Assets, and later game code in
  libraries of its own, are loaded by the runtime and not compiled into it.

## Done

| Area | What exists |
|---|---|
| Platforms | Builds on macOS, Linux, and Windows with one toolchain |
| Renderer | Vulkan, on all three. See [vulkan-renderer.md](vulkan-renderer.md) |
| File system | Virtual paths with `assets://` and `user://`. See [file-systems.md](file-systems.md) |
| Window modes | Windowed, borderless, and fullscreen |
| Command line | An abstraction with a set of options for each application |
| Runtime class | The core class an application is built from is `neon::Runtime` |

## Next

Work that builds directly on what exists.

| Item | Detail |
|---|---|
| Load a project | The runtime takes a project to run. Scenes come from files, such as `demo.scene.yml`, which nothing reads yet |
| Vulkan version and capabilities | A setting for the version to ask for, and a way for the engine to learn what the graphics card can do, so that menus only offer what works |
| Run on Windows | The build works. It has not been run |
| Log file under `user://` | It is still written relative to the working directory |

## Later

### NeonEditor

| Part | Detail |
|---|---|
| Editing | Scenes, assets, and settings of a project |
| Running a game from the editor | With options the runtime alone does not have, for debugging |
| Exporting | Turning a project into something that can be distributed |
| Command line | The options of the runtime, plus a set only the editor has |
| Runtime class | The core class an application is built from is `neon::Runtime` |
| Agent support | See below |

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

**Proposed: the Model Context Protocol.** It is an open protocol for giving
an agent tools, and the products named above already speak it. The editor
would run a server that offers its abilities as tools. One implementation
then serves every agent, and a protocol of our own is not needed.

| Tool an agent could be given | Built on |
|---|---|
| Read and change a scene | The scene graph |
| List, import, and inspect assets | The file system |
| Run the game and stop it | The runtime |
| Look at what is on screen | Rendering without a window and saving the frame, which exists |
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
| A renderer for the web | Browsers have no Vulkan. They offer WebGPU and WebGL 2 |
| A main loop the browser drives | The browser calls the engine once per frame. The engine cannot loop by itself |
| Assets served with the page | A file system backend that reads what was packed |
| A fourth toolchain | Emscripten |

Threads are limited in browsers, which will matter for physics.

### Renderers

Vulkan is the renderer today. Which one comes second is open, and the web
weighs on it.

| Option | Covers | Cost |
|---|---|---|
| **Metal** | macOS without MoltenVK | A renderer for one platform |
| **WebGPU** | The web. Through Dawn or wgpu also macOS, Windows, and Linux | A large dependency, and only the features every platform has |
| **WebGL 2** | The web, including older browsers | An older design. The removed OpenGL renderer would be a starting point |

WebGPU would answer both the web and the dependency on MoltenVK, which would
make a Metal renderer unnecessary. This is to be decided before either is
started.

**Shaders are written once.** They are compiled to SPIR-V, and
[SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) turns that into
what Metal, DirectX, and OpenGL want. A second renderer adds a step to the
build, not a second set of shaders. [Slang](https://shader-slang.org) is the
alternative if shaders grow numerous.

### Other

| Item | Detail |
|---|---|
| Replace assimp | It is temporary. It is large, and a game should load models that were prepared ahead of time |
| Packed assets | Shipping archives instead of loose files. See [file-systems.md](file-systems.md) |
| Headless as a feature | It works and is used to check the renderer. Making it a supported way to run is deferred |
| Physics | Jolt is in the tree and builds. Nothing uses it yet |
| Audio | Not started |
| Scripting | Bindings for Lua, Rust, and Python are planned |

## Open decisions

| Decision | Depends on |
|---|---|
| The second renderer: Metal, WebGPU, or WebGL 2 | How much web builds matter, and how much control is wanted on desktop |
| Shaders in GLSL or in Slang | How many shaders there will be. Cheap to change now, expensive later |
| How agents reach the editor | What the editor turns out to be |
| What a project is on disk | Needed before the runtime can load one |
| How game code is loaded | Libraries loaded at run time, scripts, or both |
