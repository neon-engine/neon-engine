# Roadmap

Where Neon Engine is heading, what is decided, and what is still open. It is
a plan, not a promise. Items move as they are understood better.

The goal is a complete engine: rendering, physics, audio, scripting, user
interfaces, and an editor to make games with. All of it is version 1.0.

Every item here is an issue on the [v1.0 milestone](https://git.traparwave.net/neon-engine/neon-engine/milestone/1), where the
work is tracked: what is done is a closed issue that names its pull request,
what is open is an open issue, and what is still to be decided is labelled
`type: decision`. The tables below name the issue of each item as #N.

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
| Platforms (#21) | Builds on macOS, Linux, and Windows with one toolchain |
| Renderer (#23) | Vulkan, on all three. See [vulkan-renderer.md](vulkan-renderer.md) |
| Resizing the window (#68) | The swapchain, and everything of the size of the frame, is made again when the window changes size |
| Linear light (#67) | The world is lit and blended in linear light in a floating-point scene image, and a resolve step turns it into the colours of the screen. User interfaces blend in sRGB, as CSS does. See [vulkan-renderer.md](vulkan-renderer.md#colour-spaces) |
| See-through materials (#67) | `alpha_mode: blend`, drawn after what is opaque, from the farthest to the nearest. The first of the alpha modes of #106 |
| Back faces left out (#136) | The back of every triangle is left out unless a material is `double_sided`, and what is mirrored is still drawn from the front |
| `VK_RenderSystem` split (#163) | The swapchain, the pipelines of materials, the canvas a frame or a render target is drawn on, and the reading back of a frame are types of their own. The render system wires them. Done ahead of the programmable pipeline (#102) |
| File system (#22) | Virtual paths with `assets://`, `user://`, and `output://`. See [file-systems.md](file-systems.md) |
| Log file (#44) | Written to `user://logs/neon-engine.log`, with what was logged before the file system started. See [file-systems.md](file-systems.md#the-log-file) |
| Window modes (#22) | Windowed, borderless, and fullscreen |
| Command line (#24) | An abstraction with a set of options for each application |
| Runtime class (#25) | The core class an application is built from is `neon::Runtime` |
| Sound (#33) | Sound effects and music with miniaudio, behind an interface. Sounds have a place in the world. See [audio.md](audio.md) |
| Groups and fading of sounds (#71) | Music, effects, voices, and groups of a game, each with a volume, and fades from one piece of music to another. A settings menu sets them from its file with `UiVolume` and `UiSoundSwitch`. Merged in #139 |
| Physics described with reflection (#58) | The components of the physics are read and written through their descriptions, which gained fields that belong to a shape only, rules across fields, and layers. Merged in #140 |
| Reflection (#35) | A component is described once, next to itself. Scene files follow from it, and the editor and scripts will. See [reflection.md](reflection.md) |
| Scenes in files (#29) | A scene is a YAML file of entities and components, made to be changed by hand. See [scenes.md](scenes.md) |
| Entities and components (#27) | The world is made of entities, components, and systems, with Flecs behind an interface. See [entity-component-system.md](entity-component-system.md) |
| Running without a window (#28) | `--headless`, `--frames`, and `--screenshot` render a number of frames and save the last one |
| Screenshots where the caller wants them (#28) | `--output-dir` chooses the folder behind `output://` |
| Screenshots at chosen frames (#28) | `--screenshot-at` runs the game forward and saves each frame that is listed |
| Fixed time step (#28) | `--time-step` advances every frame by the same amount of time |
| Exit codes (#28) | The runtime returns a failing exit code when a screenshot could not be written or the run ended with an exception |
| Physics (#34) | Static, kinematic, and dynamic bodies, triggers, and characters that move and slide, with Jolt Physics behind an interface. The world steps at a fixed rate. See [physics.md](physics.md) |
| Unit tests (#31) | GoogleTest, with tests for the four libraries and for the runtime as a whole. See the [development guide](development.md#tests) |
| User interface (#37) | Menus and what is shown during play, from YAML files with the properties of CSS: layout by flexbox, text, values of the game, input and focus, events. It is the first version. See [user-interface.md](user-interface.md) |
| Drawing in two dimensions (#37) | Triangles in pixels through an interface of the renderer, which is all a user interface needs of it |
| Drawing user interfaces (#39) | Text, images, shapes, and shaders, on the screen and on surfaces in the world. Merged in #18 |
| Behaviour of user interfaces (#40) | Style sheets, scrolling, typing, choosing, animation, scripts, and displays of high density. Merged in #20 |

## Next

Work that builds directly on what exists.

| Item | Detail |
|---|---|
| Load a project (#41) | The runtime takes a project to run. It takes a scene today, with `--scene`. What a project is on disk is open |
| Settings from `settings.yml` (#125) | The runtime reads its settings from `assets://settings.yml`, then what the player changed from `user://settings.yml`, then the command line. Today they are defaults in code and command-line options. The file names the folder of `user://`, which is fixed at `neon-engine/neon-runtime` today |
| Vulkan version and capabilities (#42) | A setting for the version to ask for, and a way for the engine to learn what the graphics card can do, so that menus only offer what works |
| Run on Windows (#43) | The build works. It has not been run |

## Planned

Everything the engine needs to be complete, by area. The order to build it in
is [below](#order).

### Foundations

These come first, because everything after them is cheaper with them in place.

| Item | Detail |
|---|---|
| clang-tidy (#30) | Checks that code follows the conventions the code base already has, such as naming. Run in the build and in the editor |
| The TODOs in the repository (#46) | Listed [below](#todos-in-the-repository) |
| Architecture document (#45) | How the libraries, interfaces, and applications fit together, and why |
| Random numbers of true entropy (#160) | From the operating system's secure generator, through an interface a game asks: `getentropy`, `BCryptGenRandom`, and in the browser `crypto.getRandomValues` |
| How a project is organised (#147) | A style guide for the folders and names of a project, which the editor, export, and agents rely on |
| A document for each major feature (#45) | In the way [file-systems.md](file-systems.md) describes the file system: what it does, how it is used, and what is open |

### World

| Item | Detail |
|---|---|
| More of the physics (#56) | Joints, locked axes, shapes that are cast along a way, and shapes that change while a body lives. See [physics.md](physics.md#limits) |
| Scripting (#57) | Lua first. Game code is loaded by the runtime, not compiled into it. Bindings for other languages can follow the same interface |
| Lua with almost no overhead (#99) | Components reached without copying, no allocation between engine and script, updates over the store's arrays |
| Components from Lua (#100) | A script declares a component that takes part in scenes, reflection, and systems |
| Script everything (#101) | As Godot does, in a design that fits an ECS: the unit is a system over components |
| Games in C++ and Lua with hot reload (#104) | Other languages after 1.0. C++ reloads as a library, which may need a runtime made for the editor |
| Meshes with collision, generated (#97) | Geometry built at run time and in the editor, with its collider, for level editing |
| Scenes and resources serialized to binary and text (#96) | As Godot does with resources, so that saved games are the same machinery. Not decided; to be discussed |
| A scene manager (#118) | Changes between whole scenes, with a loading screen and what carries over |
| Prefabs (#146) | An entity, or a tree of them, described once in a `*.prefab.yml`, placed in scenes with what differs written on top, and made at run time. A prefab is not a scene |
| Assets streamed by distance (#114) | Loaded as the player comes near and released as the player leaves, in the background |
| Input maps with states (#117) | Named actions bound by a project, and maps for walking, driving, swimming, a menu |

Both attach to entities and components, which exist. Physics is components
and a system. A script declares components and systems of its own.

### Rendering

| Item | Detail |
|---|---|
| Physically based materials (#59) | Metalness and roughness, as glTF describes them |
| Shadow mapping (#60) | For directional, point, and spot lights |
| Deferred rendering (#61) | A second pipeline next to the forward one, for scenes with many lights |
| Compute shaders (#62) | Used by the engine, and offered to games through the render interfaces |
| Light mapping (#63) | Lighting computed ahead of time for what does not move. Baked by the editor |
| Global illumination (#64) | Which technique is open. It builds on deferred rendering and compute shaders |
| Metal renderer (#65) | macOS without MoltenVK |
| WebGPU renderer (#66) | The web. Through Dawn or wgpu also the desktop platforms |
| Programmable rendering pipeline (#102) | A game describes passes, targets, and order in place of the fixed forward pipeline |
| Compute in the pipeline (#107) | Compute passes next to draw passes, with shared buffers and images |
| PBR and an ubershader out of the box (#106) | One shader that covers a traditionally rendered game, chosen by variants |
| Skeletal animation (#149) | Skinned meshes, skeletons, and animations, read from glTF, with skinning on the graphics card and animations that blend |
| Emissive materials (#129) | Screens and holograms that glow. A screen shows a render target and makes it emissive; a CRT look is a material shader |
| Bloom (#130) | What is brighter than white bleeds into a soft glow. The first post-processing step |
| Tonemapping and exposure (#131) | Light above white mapped into what a screen shows, in place of clipping. The user interface is drawn after it and keeps its colours |
| Light cast by screens (#132) | A screen can light what is around it with the average colour of its picture, if a game chooses so. A hologram only glows |
| Projected and area lights (#133) | Lights that throw a picture, and lights from a rectangle, such as a screen or a window |
| Frustum and occlusion culling (#115) | Draw what the camera sees: bounds against the frustum, and occlusion by occluder volumes or a hierarchical depth buffer |

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
| Spatial audio with Steam Audio (#70) | Sounds that are shaped by the room they are in. Sounds already have a place in the world, with miniaudio |

### User interface

The framework exists, see [user-interface.md](user-interface.md). What is
left:

| Item | Detail |
|---|---|
| Yoga as the layout engine (#77) | The engine places elements with an implementation of flexbox of its own. Yoga is tested against the specification by many more users |
| Animation, themes, localisation, rich text (#75) | |
| A library with a language of its own (#78) | RmlUi is the candidate. The interface for drawing in two dimensions takes what it hands over |
| Text that mixes directions (#74) | The Unicode Bidirectional Algorithm. SheenBidi is the candidate |
| A keyboard on the screen (#158) | For typing with a controller: moved over with the D-pad and stick, made from a `*.ui.yml` file, and the platform's own keyboard where there is one |
| WebP, colour emoji, variable fonts, fonts as several distances (#76) | Each needs a library, or a font to try it with |
| `filter`, `backdrop_filter`, opacity of a group (#76) | Built from render targets, which are there |
| Surfaces that are drawn when something changed | Every surface is drawn in every frame |
| A ray that finds where the player points on a screen in the world (#73) | Done. The ray meets the square of the entity, and needs no collider |

This is separate from the interface of the editor, which uses Dear ImGui.
Dear ImGui suits tools. It is not meant for what players see.

### Running without a window

Rendering without a window, saving chosen frames to a chosen folder, and a
fixed time step exist. See the
[development guide](development.md#running-without-a-window). What is planned
makes them a supported way to run a game, for automated checks and for agents.

| Item | Detail |
|---|---|
| Setting the state of a game (#79) | Options to start from a given state, such as a scene, a saved game, or values of the game's own. Waits for scenes that load from files |
| Input and test scripts in Lua (#80) | Relative frames and waits, loops, comments, and checks along the way, for longer runs. Waits on Lua (#57) |
| A debug mode of the runtime (#116) | The loop as a function that is stepped, and a terminal or a file of commands: step, send input, take a screenshot, read values |

### NeonEditor

| Part | Detail |
|---|---|
| Interface (#82) | Built with [Dear ImGui](https://github.com/ocornut/imgui) |
| Editing (#82) | Scenes, assets, and settings of a project |
| Running a game from the editor (#82) | With options the runtime alone does not have, for debugging |
| Exporting (#84) | Turning a project into something that can be distributed |
| Compiler tools (#83) | The tools that prepare a project are packaged with the editor: compiling shaders, preparing models and textures, and baking light maps |
| Command line (#24) | The options of the runtime, plus a set only the editor has |
| Agent support (#85) | A Model Context Protocol server. See below |
| Import through assimp, run without it (#98) | The editor converts anything assimp reads to glTF or a format of the engine's; the runtime reads only that. An animated model becomes a skinned mesh, its skeleton, and each animation in a file of its own |
| Interface beyond Dear ImGui (#103) | A stretch goal: the editor's interface behind an abstraction |

### Exporting games

The editor can export in two ways.

| Way | How | When |
|---|---|---|
| With precompiled runtimes | The editor takes a NeonRuntime that was built ahead of time for the target and puts the project next to it | The normal case. Needs no compiler |
| By compiling | The editor builds the runtime for the target, inside a container | When the runtime itself was changed |
| By compiling the game in (#105) | The game and the runtime become one executable, harder to decompile and optimised as a whole | When the developer chooses |


**A shipped runtime (#143)** keeps only the options a player needs, and loads
only the game it was exported with. Options it does not keep are left out
when it is built. Which ones those are is proposed in
[command-line.md](command-line.md). The aim is that a player cannot do what a
game does not want with an option, a setting, or a file next to the
executable, not to stop someone who is determined. Lower priority, but for
1.0.

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

### Networking

A stretch goal for 1.0 (#144): wanted in it, and the first to move out if
time runs short.

| Item | Detail |
|---|---|
| Real time, with a server | A server that has the last word, clients that predict their own moves and are corrected, interpolation, and lag compensation |
| Lockstep, for strategy | Only inputs are sent, and every machine simulates the same world. Needs the fixed time step, seeded random numbers (#80), and deterministic physics |
| Peer to peer | One player hosts, or all are equal, through NAT traversal or a relay |
| Replication of the ECS | Which entities and components go to whom, as changes. Builds on reflection |
| A dedicated server | A runtime without a window, renderer, or audio, built for servers |
| A transport behind an interface | UDP with what has to arrive arriving. Which library is open |

### Other

| Item | Detail |
|---|---|
| Replace assimp (#69) | It is temporary. It is large, and a game should load models that were prepared ahead of time |
| Packed assets (#81) | Shipping archives instead of loose files. See [file-systems.md](file-systems.md) |

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
| 10 | User interface framework | The first version is done. It draws through an interface of its own, which a change to the renderer of scenes leaves alone |
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

## Backlog

Wanted, and not for 1.0.

| Item | Detail |
|---|---|
| Virtual coordinates with a floating origin (#113) | The world is loaded and unloaded around the player and shifted back to the origin without the player noticing, for worlds that may as well be endless |
| Fewer draw calls (#169) | Opaque draws sorted by pipeline and material, instancing for objects that share a model, per-object data in one buffer, and indirect draws. Measured first, on a scene of thousands of objects |

## Known bugs and debts

| What | Issue |
|---|---|
| Sanitizer builds hang at startup on macOS | #54 |
| clang-tidy findings in the test code, clang-format violations | #52, #53 |
| Pointing at a user interface on a surface in the world needs the ray cast | #73 |
| Two raw `new` remain in the Jolt backend, for the factory and for characters | #148 |
| Reflection cannot describe a field of type `double` | #151 |
| A `SoundSource` without a `Transform` is never heard, and nothing says so | #152 |
| The log file may not open on Windows under a user folder with letters outside ASCII | #156 |
| An opaque 3D shader that writes an alpha below 1 comes out brighter | #157 |
| 55 headers hold several types, against the rule of one type per file | #168 |
| The settings menu cannot be used all the way through with a controller | #159 |

## Open decisions

| Decision | Depends on |
|---|---|
| When the Metal and WebGPU renderers are started (#87) | How much is to be ported, against how much is to be built three times |
| Whether WebGPU also serves the desktop (#88) | If it does on macOS, the Metal renderer has less to justify it |
| Shaders in GLSL or in Slang (#89) | How many shaders there will be. Cheap to change now, expensive later |
| Which technique for global illumination (#90) | What the target hardware is, and whether the web has to be able to run it |
| How agents reach the editor (#91) | What the editor turns out to be |
| What a project is on disk (#92) | Needed before the runtime can load one |
| How game code is loaded (#93) | Scripts are decided. Whether libraries loaded at run time are offered as well is open |
| YAML, JSON, or BSON for scenes and user interfaces (#94) | YAML is used and can be changed by hand; JSON with BSON for shipping was weighed |
| The license of the engine (#95) | MIT was removed; what replaces it is not decided |
