# NeonEditor

NeonEditor is the second application of the engine, next to NeonRuntime.
Both start on the same shared start-up, `RuntimeComposition` of
`lib/neon-runtime`, see [how it starts](#how-it-starts); they consume the
engine in different ways.
The runtime plays the game that is put together next to it, and a player
gets it. The editor opens a project folder, shows what the runtime would
show in a window of its own, and carries the options that load other
content or look inside a game, which the runtime no longer has (#310).

Today the editor shows a project; editing it, playing it from the editor,
and the other views are issues of their own under #82: #569 play mode and
possession, #570 scene editing, #571 views, #572 toolchains, #573 new
projects, #574 VS Code, #575 `game.yml`, #576 the editor as a service.

## Starting it

The build presets build NeonEditor with NeonRuntime and the projects. It is
written to `bin/<build>/<platform>/NeonEditor/`, with its `engine` folder
next to it, and takes the folder of a project as its first argument:

```sh
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug
./bin/debug/darwin-arm64/NeonEditor/NeonEditor bin/debug/darwin-arm64/sandbox
./bin/debug/darwin-arm64/NeonEditor/NeonEditor projects/museum
```

The folder is the one that holds the project's `assets/` and, when it has
code of its own, `extensions/`: a project as it is put together under
`bin/`, or as it is in the source tree under `projects/`. `project.yml` is
in `assets/` for now; it moves to the root of the folder with #368, and the
editor follows. `assets://` and `extensions://` then stand for the folders
of the project, and `engine://` stays next to the editor, see
[file-systems.md](file-systems.md). `user://` is the project's: the editor
opens the project as a run of its game would, so what the editor writes
there, its log among it, is what a run of the game would write.

Without a folder the editor says so and stops; with a folder that holds no
`assets/project.yml`, it says which folder and stops. Both end with exit
code 1, as every failure at start-up does.

### Options

The editor registers all three sets of options, `RuntimeOptions`,
`DisplayOptions`, and `EditorOptions`, so that everything in
[command-line.md](command-line.md) works on it, and `--help` lists it all.
`--scene` opens another scene of the project than its entry scene;
`--frames`, `--screenshot`, and `--screenshot-at` save frames of the editor
window, with the tools in them.

### Without a window

With `--headless-renderer` the editor draws no tools: it runs as the
runtime does, frame for frame, with the scene filling the frame, so that
what it saves is what a run of the game would save. That is how the checks
render a scene on a build server, and why the `runtime-*` tests of
`tests/` start NeonEditor on the game of the tests:

```sh
./bin/debug/darwin-arm64/NeonEditor/NeonEditor bin/debug/darwin-arm64/sandbox \
  --headless-renderer --frames 3 --output-dir shots --screenshot output://frame.png
```

## What it shows

| Part | What it is |
|---|---|
| Menu bar | `File` with `Quit`; `View`, which switches the windows on and off; `Help`, with a window about the editor |
| Dock space | The space under the menu bar that the windows dock into. The viewport fills its middle and the log the bottom quarter until a window is dragged elsewhere |
| `Viewport` | The entry scene of the project, or the one `--scene` names, drawn by the engine into a render target of the size of the window, in pixels of the display, seen through the [editor camera](#the-editor-camera). The target is made again when the window changes its size |
| `Log` | The last thousand lines every logger wrote, as the console shows them, scrolled to the newest |

The scene is drawn through the pipeline as the runtime draws it, with one
difference: `RenderPipeline::SetFrameTarget()` has the camera of the window
draw into the viewport's target in place of the frame, see
[vulkan-renderer.md](vulkan-renderer.md). Cameras that draw into a texture
of their own, and the screens in the world, are drawn as before. The
game's own user interface, its menus and what it shows during play, is not
drawn into the viewport yet; that comes with play mode (#569). The layout
of the windows is not kept between runs yet.

### The pointer and the window

The pointer is the editor's: free and shown, so that the menus and the
windows are clicked and dragged. The world is told not to take it when it
starts (`EntityWorld::SetCapturesPointer(false)`), as a game does; a game
the editor plays takes it then, over the viewport, and gives it back on
leaving play mode or with escape (#569). Nothing in the editor needs escape
to free the pointer, and the pointer leaves the window as it leaves any
window: a window holds the pointer only while the input has taken it
(`SDL2_InputSystem::CenterAndHideCursor()`), never by having the focus. The game under the tools is given no input until play
mode: `ToolUiContext::WantsPointer()` and `WantsKeyboard()` say when the
tools have the devices, and the viewport reads them before it flies.

The window is the editor's, not the game's: it is always a window that can
be moved and resized, whatever `window.mode` of the project's settings or
the command line says, 1600 by 1000 points unless `--window-size` names a
size. The project's mode and size are what the game is played in. The
editor's own window size and mode belong to the editor's settings, which
are #582.

### The editor camera

The viewport looks through the editor's own camera, `EditorCamera`, which
flies through the scene as the engine's `Spectator` does. It is no entity
of the scene, so saving the scene does not carry it, and there is one for
every viewport, which #571 needs. It starts where the scene's camera of
the window is, so that the editor opens on what the game would show, and
the scene's camera stays what the game plays with.

| Do | With |
|---|---|
| Turn the view | Hold the right mouse button over the viewport. The pointer is hidden and held in the window until the button is released, wherever it went. The view turns by how far the mouse went, a fixed angle a pixel, and not at all while the mouse rests |
| Move | W, A, S, D; E or space up, Q down |
| Move faster | Shift |
| Change the speed | The wheel over the viewport, up for faster |

The keys move the camera only while the pointer is over the viewport or
it has the focus, and never while a field of the tools is typed into. The
devices are read as they are; the actions of the game's input map are the
game's. `EditorCameraSubmission`, a system the application adds to the
world after placing, hands the editor camera to the pipeline after the
world handed the scene's, which is how the viewport shows its view.

## How it starts

The start-up is the runtime's, `RuntimeComposition` of `lib/neon-runtime`,
see [development.md](development.md#how-an-application-starts):
`app/NeonEditor/main.cpp` walks the same steps as `app/NeonRuntime/main.cpp`
and is the composition root and nothing more. Where the editor differs, it
says so in `EditorHooks`, the editor's `CompositionHooks`, which lives next
to `main.cpp` since it is where the tools of neon-imgui are made, and
`neon-editor` stays free of them:

| Hook | What the editor does with it |
|---|---|
| `RegisterOptions`, `ApplyOptions` | Takes `FOLDER` as its first argument, and registers `EditorOptions` next to the runtime's two sets |
| `ChooseProjectFolder` | The folder the command line names becomes `settings.project_directory`, which the file system opens as `assets://` and `extensions://`. Without one the editor says so and stops |
| `AdviseWithoutAProject` | Says which folder holds no `assets/project.yml` |
| `AdjustSettings` | Insists on a window that can be moved and resized, 1600 by 1000 points unless `--window-size` names a size, and names it `NeonEditor - <project>` unless the settings name it, see [the pointer and the window](#the-pointer-and-the-window) |
| `MakeRuntime` | Makes the `Editor`, and owns it |
| `AddSystems` | Tells the world not to take the pointer; with a window, makes `ImGui_ToolUiSystem` on the backends it is handed, gives it to the editor, and adds `EditorCameraSubmission` to the world after placing |

Without a window, `AddSystems` makes no tools, and the editor runs as the
runtime does, which is what the checks rely on.

## How it is built

| Piece | Location | Role |
|---|---|---|
| `ToolUiContext` | [neon-core](../lib/neon-core/neon/tool-ui/tool-ui-context.hpp) | The interface of the tools: what the editor draws its windows with. Windows, a menu bar and menus, a dock space, text, buttons, checkboxes, a text field, and an image of a render target, declared anew in every frame |
| `ToolUiSystem` | [neon-core](../lib/neon-core/neon/tool-ui/tool-ui-system.hpp) | The base class of its backends, with the lifecycle: `Initialize()`, `CleanUp()`, `BeginFrame()`, `EndFrame()` |
| `ImGui_ToolUiSystem` | [neon-imgui](../lib/neon-imgui/neon/tool-ui/imgui-tool-ui-system.hpp) | The backend: Dear ImGui, its docking branch, drawn through its SDL2 and Vulkan backends |
| `Editor` | [NeonEditor](../app/NeonEditor/neon/editor/editor.hpp) | The runtime with the tools on top: the loop of the editor, the menu bar, and the windows. Derives from `Runtime` and runs as it does without a tool interface |
| `EditorHooks` | [NeonEditor](../app/NeonEditor/editor-hooks.hpp) | The editor's choices on the shared start-up, see [how it starts](#how-it-starts) |
| `EditorViewport`, `EditorLog` | [NeonEditor](../app/NeonEditor/neon/editor/ui/) | The two windows |
| `EditorCamera`, `EditorCameraInput`, `EditorCameraSubmission` | [NeonEditor](../app/NeonEditor/neon/editor/camera/) | The camera of the viewport, what moves it in a frame, and how the world is told to draw with it |
| `LogHistorySink` | [neon-core](../lib/neon-core/neon/logging/log-history-sink.hpp) | Keeps the last lines of the log for the window, through `LoggingSystem::CopyRecentLines()` |

`neon-editor`, the library of `app/NeonEditor`, holds the editor's own objects and is free of Dear ImGui:
it sees the tools through `ToolUiContext` alone. `lib/neon-imgui` is the
one library that includes Dear ImGui, as every third-party library sits
behind an interface of neon-core in a backend library of its own. Another
toolkit, or the engine's own user interface, can implement `ToolUiContext`
later (#103); the interface holds what the editor uses and nothing of the
toolkit, and grows with the editor.

### What the backend needs of the other backends

Dear ImGui's SDL2 backend reads the window and its events, and its Vulkan
backend draws with the device and into the render pass of the frame. That
stays between the backend libraries, and nothing of it reaches neon-core
or an application:

| In | What | For |
|---|---|---|
| neon-sdl2 | `SDL2_WindowSystem::GetWindow()` | The window of SDL |
| neon-sdl2 | `SDL2_EventListener`, `SDL2_InputSystem::SetEventListener()` | Every event of SDL, handed on before the input system takes it in |
| neon-vulkan | `VK_RenderSystem::GetDevice()`, `GetFramePass()` | What the pipeline of Dear ImGui is made with |
| neon-vulkan | `VK_RenderSystem::DrawIntoFrame()` | Records Dear ImGui's draws into the frame, on top of everything else of it |
| neon-vulkan | `VK_RenderSystem::GetRenderTargetView()` | The image of a render target, for the viewport |

Dear ImGui finds Vulkan through volk, as the renderer does, so no Vulkan
library is linked.

### Docking and viewports

Docking is on. Multi-viewport, windows torn off onto the desktop, is off:
it has Dear ImGui make swapchains of its own next to the renderer's, with
MoltenVK on macOS, and is left for #580. The layout of the dock space is
not written to a file between runs; `imgui.ini` is switched off, so that
nothing is written next to wherever the editor was started.

## Open questions

- Where the editor keeps its own settings and layout: a folder of its own
  under the platform's folder for applications, not the project's `user://`.
- What of the runtime's `Runtime` class the editor should share once it
  plays the game (#569): the pause menu, the settings menu, and the holds of
  the world and the sounds are the runtime's, and the editor derives from it
  for now.
