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
| Back faces left out (#136, #194) | The back of every triangle is left out unless a material is double-sided, as the model file says or the scene's `double_sided` overrides, and what is mirrored is still drawn from the front |
| Several materials in one model (#193) | A model whose meshes use different materials is drawn with each of them, inside one render object, and the scene's `Renderable` overrides them all. See [models.md](models.md#several-materials) |
| Vertex colours (#192) | A `Vertex` carries a colour, white unless the model paints its vertices, and `pbr`, `basic-lit`, and `unlit` multiply it into the base colour. See [models.md](models.md#the-colours-of-the-vertices) |
| `VK_RenderSystem` split (#163) | The swapchain, the pipelines of materials, the canvas a frame or a render target is drawn on, and the reading back of a frame are types of their own. The render system wires them. Done ahead of the programmable pipeline (#102) |
| File system (#22) | Virtual paths with `assets://`, `user://`, `output://`, and `extensions://`. See [file-systems.md](file-systems.md) |
| Load a project (#41) | A project is a folder with a `project.yml` at its root: its name, its organization, its scenes, and the scene it starts with. The runtime runs the project next to it, and the names place `user://`. See [projects.md](projects.md) |
| The layout of a project (#147) | A style guide for the folders and names of a project, which the editor, export, and agents rely on: by kind at the top, by area inside, lowercase with dashes, the kind of a file before its format. See [project-layout.md](project-layout.md) |
| Settings from `settings.yml` (#125) | In layers: the defaults in code, `assets://settings.yml` of the project, `user://settings.yml` of the player, then the command line. The window, the user interface, the steps of the world, and the renderer. See [settings.md](settings.md) |
| Log file (#44) | Written to `user://logs/neon-engine.log`, with what was logged before the file system started. See [file-systems.md](file-systems.md#the-log-file) |
| Window modes (#22) | Windowed, borderless, and fullscreen |
| Command line (#24) | An abstraction with a set of options for each application |
| Runtime class (#25) | The core class an application is built from is `neon::Runtime` |
| Sound (#33) | Sound effects and music with miniaudio, behind an interface. Sounds have a place in the world. See [audio.md](audio.md) |
| Groups and fading of sounds (#71) | Music, effects, voices, and groups of a game, each with a volume, and fades from one piece of music to another. A settings menu sets them from its file with `UiVolume` and `UiSoundSwitch`. Merged in #139 |
| Ambience and the groups of a project (#195) | `ambience` is a fourth group, held still with the game and left alone when the music or the effects are stopped. A project declares groups of its own in `settings.yml` under `audio.groups`, and the volumes of every group are settings under `audio.volumes`, which the player's file sets on top |
| Physics described with reflection (#58) | The components of the physics are read and written through their descriptions, which gained fields that belong to a shape only, rules across fields, and layers. Merged in #140 |
| Reflection (#35) | A component is described once, next to itself. Scene recipes follow from it, and the editor and scripts will. See [reflection.md](reflection.md). Since #283 every type a component may hold has a kind: bytes, chars, shorts, longs, `Vector2`, `Vector4`, whole vectors, and their lists; the quaternion and the matrices are its second half |
| Scenes in files (#29) | A scene is a recipe, a YAML file of entities and components, made to be changed by hand. The rules every recipe shares are in [recipes.md](recipes.md), the scene's own in [scenes.md](scenes.md) |
| Entities and components (#27) | The world is made of entities, components, and systems, with Flecs behind an interface. See [entity-component-system.md](entity-component-system.md) |
| Chords, dead zones, and curves in input maps (#200) | A binding that is a list is a chord held together; an axis has a dead zone and a curve of its own, and the backends hand the sticks and the triggers over as they are. See [input.md](input.md#chords) |
| Scripts hold a controller button and push the left stick (#202) | `hold-button NAME` and `left-stick X Y` in an input script, so that every binding of a map is reached without devices. See [input.md](input.md#scripts) |
| The fixed game actions are gone (#203) | `Action::L_*` and `R_*`, the map of before a project could say, are deleted from the input state and the backends. A game reads its named actions, and the tests set keys and buttons |
| Running without a window (#28) | `--headless`, `--frames`, and `--screenshot` render a number of frames and save the last one |
| Screenshots where the caller wants them (#28) | `--output-dir` chooses the folder behind `output://` |
| Screenshots at chosen frames (#28) | `--screenshot-at` runs the game forward and saves each frame that is listed |
| Fixed time step (#28) | `--time-step` advances every frame by the same amount of time |
| Exit codes (#28) | The runtime returns a failing exit code when a screenshot could not be written or the run ended with an exception |
| Physics (#34) | Static, kinematic, and dynamic bodies, triggers, and characters that move and slide, with Jolt Physics behind an interface. The world steps at a fixed rate. See [physics.md](physics.md) |
| Unit tests (#31) | GoogleTest, with tests for the four libraries and for the runtime as a whole. See the [development guide](development.md#tests) |
| User interface (#37) | Menus and what is shown during play, from UI recipes with the properties of CSS: layout by flexbox, text, values of the game, input and focus, events. It is the first version. See [user-interface.md](user-interface.md) |
| Drawing in two dimensions (#37) | Triangles in pixels through an interface of the renderer, which is all a user interface needs of it |
| Drawing user interfaces (#39) | Text, images, shapes, and shaders, on the screen and on surfaces in the world. Merged in #18 |
| Behaviour of user interfaces (#40) | Style sheets, scrolling, typing, choosing, animation, scripts, and displays of high density. Merged in #20 |

## Next

Work that builds directly on what exists.

### The prototype game (#182)

A small, complete first-person shooter in the way of Doom and Half-Life 2,
built from Kenney's kits with NeonRuntime, recipes, and later Lua alone, no
editor. It is the test of the engine: everything a game needs and the runtime
cannot do is a gap, and every gap becomes an issue. It is what the order of
work serves from here: each feature below is built when the prototype needs
it, and the prototype shows whether the feature is enough. It is done when a
person can play it from start to end from an exported build on macOS, Linux,
and Windows.

| Step | Issue | Detail |
|---|---|---|
| A level to walk through | #183 | Import glTF, `external/kenney/` in the project, see [project-layout.md](project-layout.md#external-assets), and a first level as a scene recipe |
| Walking, looking, shooting | #117 | Input maps with states: named actions a project binds, and maps for walking and for a menu |
| The feel of the player | #217, #218, #219, #220 | Done. `run` on the click of the left stick as well as the left shift. The eyes glide over a step and a slope at `step_smoothing`. In the air the body keeps the velocity it took off with and the keys steer it by `air_control`, decided with #219 as most shooters do. `camera_offset` moves the camera from the eyes for a lean or a view from behind. See [physics.md](physics.md#the-player) |
| Enemies, pickups, doors | #146 | Prefabs: an entity described once in a `*.prefab.yml` and placed many times |
| The game's own logic | #57 | Lua, see [Scripting](#world) |
| From one level to the next | #118 | A scene manager with a loading screen and what carries over |
| How it looks | #59, #60, #149, #129 | Physically based materials, shadow mapping, skeletal animation, emissive materials |
| How it sounds | #71 | Weapon sounds, footsteps, music by area, through the groups that exist |
| Shipping it | #143, #84 | The options are owned by the runtime or the editor, see [command-line.md](command-line.md), and every platform has a release preset. Exporting a project into something that runs on a machine without the repository is the editor's to do, once there is one (#84) |

### The rest of Next

| Item | Detail |
|---|---|
| Vulkan version and capabilities (#42) | A setting for the version to ask for, and a way for the engine to learn what the graphics card can do, so that menus only offer what works |
| Run on Windows (#43) | The build works. It has not been run |
| `--headless-renderer` and `--headless` (#180) | Decided 2026-10-02. What `--headless` does today, frames rendered without a window for checks and the editor's tools, becomes `--headless-renderer`. `--headless` is kept for a dedicated server (#144): no display, no renderer, no sound, no input devices. Until the server exists it is refused with a message that points at `--headless-renderer` |

## Planned

Everything the engine needs to be complete, by area. The order to build it in
is [below](#order).

### Foundations

These come first, because everything after them is cheaper with them in place.

| Item | Detail |
|---|---|
| clang-tidy (#30) | Checks that code follows the conventions the code base already has, such as naming. Run in the build and in the editor |
| The TODOs in the repository (#46) | Listed [below](#todos-in-the-repository) |
| Throw sparingly (#179) | The engine never throws while a game runs: a problem is a result that is logged and dealt with where it happens, and the game goes on. An asset that fails to load is not drawn, a scene with a mistake loads without the entities that are wrong, a user interface that cannot be made leaves its surface empty. Whether start-up throws at all, for what keeps the engine from coming up, is part of the decision. 46 `throw` sites to go through, scenes and user interfaces first |
| Random numbers of true entropy (#160) | From the operating system's secure generator, through an interface a game asks: `getentropy`, `BCryptGenRandom`, and in the browser `crypto.getRandomValues` |
| A document for each major feature (#45) | In the way [file-systems.md](file-systems.md) describes the file system: what it does, how it is used, and what is open. The feature documents so far: [recipes.md](recipes.md), [scenes.md](scenes.md), [prefabs.md](prefabs.md), [user-interface.md](user-interface.md), [projects.md](projects.md), [settings.md](settings.md), [project-layout.md](project-layout.md), [reflection.md](reflection.md), [entity-component-system.md](entity-component-system.md), [physics.md](physics.md), [audio.md](audio.md), [vulkan-renderer.md](vulkan-renderer.md), [file-systems.md](file-systems.md), [command-line.md](command-line.md) |

### World

| Item | Detail |
|---|---|
| More of the physics (#56) | Joints, locked axes, shapes that are cast along a way, and shapes that change while a body lives. Then joined bodies kept apart (#206), motors and springs on a hinge and a slider (#207), and the state of a joint read back (#208). See [physics.md](physics.md#limits) |
| Scripting (#57) | **First step done.** Lua, run by LuaJIT: every `*.lua` under `scripts/` of the project is found and run at start, nothing names an entry script and nothing lists the files. A script declares a component with `Component:extend { fields }`, named after its file and laid out as one of the engine's, so scenes, prefabs, and reflection treat it as such; and a system with `System:extend(...)`, its hooks `ready`, `update`, `fixed_update`, `removed`, `on_trigger_enter`, `on_trigger_exit`, and `on_collision` run over every entity with the components, with the fields read and changed in place. The contracts are checked when a file loads. Sandboxed, with `world`, `input`, `scene`, `log`, and `require` inside the folder. Bytecode or embedding when exported, lists and references as fields, spawning, audio, and hot reload are open. See [scripting.md](scripting.md) |
| Native extensions (#286) | **First step done.** A folder under `extensions://`, next to the executable, with an `extension.yml` and a library for each platform, which the runtime opens at start. The boundary is one header in C, `neon-extension.h`, with versioned tables of function pointers, and `neon_add_extension` builds an extension against it alone. An extension is started and cleaned up, writes to the log, registers components that recipes and scripts know, reaches the entity store, and adds systems that run per frame and per step; `neon-extension.hpp` is the layer for C++ on top. It reads and changes the fields of any component by name, reads the input and files, spawns prefabs, asks for scenes, is told what touched what, casts rays, and reads the engine's fields in place. An extension brings assets of its own under `extensions://<name>/assets/`, scripts included, and is built without the engine through `cmake/NeonSdk.cmake`. The bench, `projects/bench`, is the first game in C++ on it, a project with an extension (#327). The Quake extension (#287) comes next. See [extensions.md](extensions.md) |
| Games in C++, and other languages (#93) | A game in C++ is an application that links the engine, as NeonRuntime does. Native libraries loaded at run time are deferred to the editor, which needs them for hot reload. Other languages come after 1.0 through the same interface as Lua. [Pallene](https://github.com/pallene-lang/pallene), typed Lua compiled to C, is a watch item for scripts that have to be fast |
| Lua with almost no overhead (#99) | Components reached without copying, no allocation between engine and script, updates over the store's arrays |
| Components from Lua (#100) | **Done with #57:** a script declares a component that takes part in scenes, reflection, and systems. Open: a sentence per field for the editor, and lists, choices, and references as fields |
| Script everything (#101) | **The shape is in (#57):** the unit is a system over components, a script is a component and a system, and events of the physics arrive as hooks. Open: what a script reaches of audio, spawning, and the user interface |
| Games in C++ and Lua with hot reload (#104) | Other languages after 1.0. C++ reloads as a library, which may need a runtime made for the editor |
| Meshes with collision, generated (#97) | **First step done:** `MeshData`, `MeshBuilder` (box, plane, ramp, prism, sphere, cylinder, quad), flat and smooth normals, textures projected once a metre, the `Geometry` component drawn by its `Renderable` and collided with through a `Collider`, a level blocked out in `blockout.scene.yml`, and the museum of `demo.scene.yml` built from shapes alone (#341). Open: concave outlines, brushes and booleans, rebuilding while the game runs. See [geometry.md](geometry.md) |
| Curves, and a rope that hangs between two bodies (#352) | A cubic Bézier curve, a path of them, flattening, length, and even steps along a curve, as templates over what a point is; a tube along a curve as a mesh and a `Geometry`; a mesh that changes after it was first drawn; a `Joint` of type `rope` and a `Rope` component that draws it, straight when taut and hanging when slack. Open: a rope that is simulated, particles on a trail, a path as a component. See [curves.md](curves.md) |
| Scenes and resources serialized to binary and text (#96) | As Godot does with resources, so that saved games are the same machinery. Not decided; to be discussed |
| A scene manager (#118) | Changes between whole scenes, with a loading screen and what carries over |
| Prefabs (#146) | **Done:** an entity, or a tree of them, described once in a `*.prefab.yml`, placed in scenes with what differs written on top, a child taken away with `~` (#228), and spawned at run time through `WorldSystem::Spawn` (#229), see [prefabs.md](prefabs.md). A prefab is not a scene. Open: a Lua binding once scripts come (#57) |
| Fewer draw calls (#169) | **First step done:** the opaque draws of a scene are kept and sorted by pipeline, material, and model, the nearest first, and those alike are one instanced call, with every object's data in one buffer the shaders read by index; the shadow pass draws the same batches; what a frame cost is logged. Open: indirect draws from a buffer, and textures by index so that a material needs no descriptor set of its own, which lets whole scenes go into a few draws |
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
| Emissive materials (#129) | **Done:** `material.emissive`, `emissive_strength`, and `emissive_texture`, a file or a render target, added after lighting by `pbr` and `basic-lit`, and read from a glTF material's `emissiveFactor`, `emissiveTexture`, and `KHR_materials_emissive_strength`, see [scenes.md](scenes.md#renderable). Open: a CRT or hologram look as a material shader, which waits for the stitching tool of [shaders.md](shaders.md#the-stitching-tool) |
| Bloom (#130) | What is brighter than white bleeds into a soft glow. The first post-processing step |
| A sky (#343) | **Done:** a `Sky` component shows the six faces of a cube or a panorama around a sphere behind the scene, for every camera, see [vulkan-renderer.md](vulkan-renderer.md#the-sky). Open: HDR panoramas and smaller copies (#347) |
| Day and night from painted skies (#346) | Several skies keyed to the time of day, blended two at a time, with the lights, a tint, and the fog on the same clock |
| A sky made by a formula (#344) | Day and night from where the sun stands, with an atmosphere that scatters its light, a moon, and stars |
| Volumetric clouds (#345) | Clouds with depth, lit by the sun, in the sky made by a formula |
| Tonemapping and exposure (#131) | **Done:** `rendering.tonemapper` as `none`, `aces`, or `agx`, and `rendering.exposure`, applied in the resolve step, see [vulkan-renderer.md](vulkan-renderer.md#tonemapping). The user interface is drawn after it and keeps its colours. Open: an exposure that adapts to the scene, which comes with bloom (#130) |
| Light cast by screens (#132) | A screen can light what is around it with the average colour of its picture, if a game chooses so. A hologram only glows |
| Projected and area lights (#133) | Lights that throw a picture, and lights from a rectangle, such as a screen or a window |
| Frustum and occlusion culling (#115) | Draw what the camera sees: bounds against the frustum, and occlusion by occluder volumes or a hierarchical depth buffer |

**Shaders are written once**, in GLSL, and SPIR-V is the one intermediate
form (#89, decided 2026-10-02). glslang produces it, and the other renderers
start from it: [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) for
MSL and HLSL, and Tint or Naga for WGSL in a browser. A second renderer adds a
step to the build, not a second set of shaders. [Slang](https://shader-slang.org)
(#110) was weighed and set aside while its Metal and WGSL targets are
experimental. A tool of our own that stitches includes and wraps material
shaders into complete stages, as Godot's compiler does, is the likely next
step and the first of the compiler tools (#83). The pipeline, the tools for
each target, and what the sources keep to are in [shaders.md](shaders.md).

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
| Surfaces that are drawn when something changed | Done (#431): drawn when anything in the user interface changed; only those whose own user interface changed, later |
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
| Exporting (#84) | Turning a project into something that can be distributed. NeonEditor implements it |
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
only the game it was exported with. The options are owned: the runtime owns
its sets, the editor owns the set that loads other content, and NeonRuntime
registers the editor's set only until NeonEditor exists. Which set each
option is in is decided in [command-line.md](command-line.md). The aim is that a player cannot
do what a game does not want with an option, a setting, or a file next to the
executable, not to stop someone who is determined. A stronger binding, a key
checked against packed assets, comes with #81.

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
| 2 | The prototype game, step by step | It decides what is needed next and shows whether a feature is enough. See [the prototype game](#the-prototype-game-182) |
| 3 | Load a project, and the rest of [Next](#next) | Loading a project is done. The rest is what the prototype asks for first |
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
| Architecture document (#45) | How the libraries, interfaces, and applications fit together, and why. Written once the architecture has settled; until then the feature documents and this roadmap are the record |
| An administrator's console (#181) | For a dedicated server, see [networking](#networking): a text console the server exposes on purpose, on a terminal or a socket, off unless the game turns it on. Who may log in, which commands come with the engine and how a game adds its own, which points at Lua, and whether it shares a console with the debug mode's terminal (#116). The shape is decided before #144 builds the server |

## Known bugs and debts

| What | Issue |
|---|---|
| Sanitizer builds hang at startup on macOS | #54 |
| The cone of a spot light is written as cosines, and a light without a `constant` is divided by zero | #342 |
| What a camera draws into a texture has no shadows: the one shadow map is fitted to the window's camera | #350 |
| A script's system cannot run over the components of the physics or the player, which are registered after the scripts are read | #351 |
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
| Which technique for global illumination (#90) | What the target hardware is, and whether the web has to be able to run it |
| How agents reach the editor (#91) | What the editor turns out to be |
