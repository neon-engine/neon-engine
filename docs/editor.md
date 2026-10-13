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
window, with the tools in them. `--editor-script` drives the tools by a
script, see [scripted runs](#scripted-runs).

### Without a window

With `--headless-renderer` the editor draws no tools unless it is asked
to: it runs as the runtime does, frame for frame, with the scene filling
the frame, so that what it saves is what a run of the game would save.
That is how the checks render a scene on a build server, and why the
`runtime-*` tests of `tests/` start NeonEditor on the game of the tests
and read its pixels:

```sh
./bin/debug/darwin-arm64/NeonEditor/NeonEditor bin/debug/darwin-arm64/sandbox \
  --headless-renderer --frames 3 --output-dir shots --screenshot output://frame.png
```

With `--tools`, or with `--editor-script`, the tools are drawn without a
window as well: Dear ImGui draws into the headless frame, which stands for
the editor's window, at the size of the editor's own settings, see
[the editor's folder](#the-editors-folder), or the one `--window-size`
names, in points and `--render-scale` pixels for each, and the screenshot
shows the menus and the windows over the viewport. No
platform backend is behind it then; the only input the tools get is what a
script feeds them.

```sh
./bin/debug/darwin-arm64/NeonEditor/NeonEditor bin/debug/darwin-arm64/sandbox \
  --headless-renderer --window-size 1280x800 --tools --frames 3 \
  --output-dir shots --screenshot output://tools.png
```

## What it shows

| Part | What it is |
|---|---|
| Menu bar | `File` with `Quit`; `View`, which shows and hides the tool windows, see [the View menu](#the-view-menu-and-the-tool-windows); `Help`, with a window about the editor |
| Dock space | The space under the menu bar that the windows dock into. The viewport fills it, and a tool window takes a side of it the first time it is shown, until a window is dragged elsewhere |
| `Viewport` | The entry scene of the project, or the one `--scene` names, drawn by the engine into a render target of the size of the window, in pixels of the display, seen through the [editor camera](#the-editor-camera). The target is made again when the window changes its size. Always shown |
| `Log` | A tool window: the last thousand lines every logger wrote, as the console shows them, scrolled to the newest. `Ctrl+L`, the bottom quarter |

The scene is drawn through the pipeline as the runtime draws it, with one
difference: `RenderPipeline::SetFrameTarget()` has the camera of the window
draw into the viewport's target in place of the frame, see
[vulkan-renderer.md](vulkan-renderer.md). Cameras that draw into a texture
of their own, and the screens in the world, are drawn as before. The
game's own user interface, its menus and what it shows during play, is not
drawn into the viewport yet; that comes with play mode (#569).

### The View menu and the tool windows

The editor is minimal: it opens with the viewport alone filling the window,
and every tool window, the log today, the hierarchy, the inspector, the
toolchains, and the build output as they come, is hidden until it is shown
from the `View` menu or with its shortcut, and hidden again the same way
(#590). The `View` menu lists every tool window with its shortcut, and marks
the ones that are shown. The viewport is not a tool window: it is always
there, it is what the others dock around, and it is not in the menu.

A tool window is registered with `EditorWindows`, the registry of
`app/NeonEditor/neon/editor/ui/`, by its title, its shortcut, where it docks
the first time it is shown, a side of the dock space and the share it takes,
and the function that draws it. The `View` menu is drawn from the registry,
and so are the windows that are shown; `Editor::GetWindows()` is where a
window is added next to the editor's own:

```cpp
editor.GetWindows().Add({
  .title = "Inspector",
  .shortcut = "Ctrl+I",
  .side = ToolUiDockSide::Right,
  .share = 0.3f,
  .draw = [&](ToolUiContext &ui) { inspector.Draw(ui); }
});
```

A shortcut is written as the menu shows it, `Ctrl+Shift+L`: the modifiers
and one key, a letter, a digit, or F1 to F12. `Ctrl` is the command key on
macOS, as Dear ImGui reads it. The shortcut works wherever the focus is,
unless a field that has it takes the same keys.

The first time a window is ever shown it docks where its registration says,
split off from the viewport. After that it goes where it was last, docked
or floating, which the [editor's folder](#the-editors-folder) keeps between
runs along with which windows are shown, so a minimal start is the default
of a new user, not a reset at every start.

### The editor's folder

The editor keeps what is its own, and no project's, in a folder of its own:
`editor://`, placed by the editor's names, organization `neon-engine` and
application `neon-editor`, under the folder the platform keeps for an
application, see [file-systems.md](file-systems.md). It is the same
whichever project is open, and apart from the project's `user://`, which is
the game's: a run of the game from the editor writes its real settings
there. The log says where it is, in its line `editor:// is ...`.

| Platform | The editor's folder |
|---|---|
| macOS | `~/Library/Application Support/neon-engine/neon-editor/` |
| Linux | `~/.local/share/neon-engine/neon-editor/` |
| Windows | `%APPDATA%\neon-engine\neon-editor\` |

| File | Holds |
|---|---|
| `editor-settings.yml` | The editor's own settings, see below |
| `layout.ini` | The layout of the windows, Dear ImGui's own text: where every window is, what it is docked to, and how large it is. Read once the tools are up, written at the end of a run, through the file system and never through a native path in the tool interface |
| `user-editor.yml` | This machine's toolchains, see [editor-toolchains.md](editor-toolchains.md) |

`editor-settings.yml` is read as every recipe is: with the YAML reader of the
engine, every problem reported with its line, a name that is not known
refused. A file with a mistake is refused as a whole: the problems are in the
log as errors, the editor starts with its defaults, and the file is not
written back at the end, so that the mistake can be corrected. A name that
is left out keeps its default.

```yaml
version: 1
window:
  width: 1600
  height: 1000
  mode: windowed
last_project: /Users/someone/projects/museum
projects_folder: /Users/someone/projects
open_windows: [Log]
```

| Name | Holds | Default |
|---|---|---|
| `version` | The version of this layout, 1 | Required |
| `window.width`, `window.height` | The size of the editor's window in points, as it was left. Written at the end of a run while the window is windowed | 1600 by 1000 |
| `window.mode` | How the editor's window is shown: `windowed`, `borderless`, or `fullscreen`, as `window.mode` of a project's settings. The project's mode is the game's and is not taken | `windowed` |
| `last_project` | The folder of the project that was open last, as the command line gave it, a path of this machine | None |
| `projects_folder` | The folder new projects are made in, for the new-project dialog (#573) | None |
| `open_windows` | The titles of the tool windows that were shown, which are shown again | None |

`--window-size` and `--window-mode` on the command line win over the file
for one run; at the end of it the window is written as it was left, at that
size when it was not resized. Without a window, with `--headless-renderer`,
the folder is placed and the settings are read, so that a check sees the
same start. When the tools are drawn then, with `--tools` or
`--editor-script`, the headless frame stands for the window: it takes the
size of the file, the windows of `open_windows` and `layout.ini` are shown
as they were left, and both files are written at the end, so that a script
proves what a restart keeps (the `editor-scripts` tests). Without the tools
nothing is taken from the file and nothing is written: the run is the
game's.

The objects are in `app/NeonEditor/neon/editor/settings/`: `EditorSettings`
holds the values, `EditorSettingsFile` reads and writes the file, and
`EditorFolder` is the folder with its two files, which `EditorHooks` places
and reads and the `Editor` writes at the end of a run.

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

## Scripted runs

A tool window used to be proved by a checklist for the owner, since only a
window showed it (#607). A script of the editor does what the checklist
says, without a window, and says what it expects to see; a failed
expectation ends the run with exit code 1, so an `editor-*` application
test is a script, the lines it expects in the log, and the images it
expects to be saved. The owner's checklist keeps what a script cannot see:
how it feels, what high density does to it.

A script speaks of the tools by their labels, as a checklist does, and
never of the toolkit. The backend records every window, menu, menu item,
button, checkbox, field, and text it draws in a frame with its place, as a
`ToolUiItem`, and `ToolUiContext::GetItemsDrawn()` and `FindItem()` hand
them out; a step looks its label up among what the frame before drew, and
turns into pointer and key events fed through `ToolUiContext::FeedEvents()`,
the same door the events of a window take, so a click is three frames,
the pointer moved, the button down, the button up, as a hand does it. The
script runs on `ToolUiContext` alone, so it runs on any toolkit behind the
interface (#103), and in a real window as well: `--editor-script` with a
window drives the real editor, and the owner can watch it. Dear ImGui's
own test engine is not used: its license is not MIT, and it would tie the
one untestable piece of the editor to the toolkit the interface exists to
replace.

A script is a `*.editor-script.yml` file of its own, named by `--editor-script`,
rather than lines of the input script: it speaks in labels and nested maps
where the input script speaks in device events by frame number, and a game
under the viewport may still take an input script of its own. It is a map
with a `description` and a list of `steps`, each a map of one key:

```yaml
description: The editor opens with the viewport alone, and View > Log shows the Log
steps:
  - expect: {window: Viewport, shown: true}
  - expect: {window: Log, shown: false}
  - screenshot: output://viewport-alone.png
  - menu: View > Log
  - expect: {window: Log, shown: true}
  - expect: {text: The tool interface is drawn with Dear ImGui}
  - screenshot: output://log-shown.png
```

| Step | Does |
|---|---|
| `menu: View > Log` | Opens each menu of the path in turn and chooses the last item; `menu: View`, a menu alone, opens it and leaves it open, for a screenshot of its items |
| `click: Close` | Presses a button, a menu item that is shown, a checkbox, or a field, by its label |
| `type: {field: Name, text: my-game}` | Clicks the field by its label and types the text into it |
| `key: Ctrl+L` | Presses a key with what is held with it: `Ctrl`, `Shift`, `Alt`, `Cmd`, or `Shortcut` for the key the platform makes shortcuts with. The key is named as the input script names it: `l`, `enter`, `f5` |
| `pointer: {x: 200, y: 100}` | Puts the pointer there, in points of the window |
| `drag: {from: "36,614", to: "900,300"}` | Holds the button down over a label or a place written `x,y` and lets it go over another, with the pointer moved between them over a few frames. A window's title stands for the middle of the window, which does not move a docked one: a docked window is dragged by its tab, at its place |
| `wait: 2` | Lets that many frames pass |
| `screenshot: output://name.png` | Saves the frame that was just drawn, with the tools in it |
| `expect: {window: Log, shown: true}` | Whether a window of that title was drawn in this frame; `shown: false` for one that is hidden |
| `expect: {text: "Quit was chosen"}` | Whether a text drawn in this frame holds the text |
| `expect: {item: Close, present: true}` | Whether anything of that label was drawn in this frame, a text aside |

A label that is not drawn within ten frames fails the step, with what was
drawn of that kind in the message, so that a misspelled label is found at
once. The log says when each step is done, `Step 3 of the script, 'menu
View > Log' on line 7, is done`, and names the step and its line when one
fails. The run ends when the script does, with exit code 0, unless
`--frames` says more; a script that is still running when the frames run
out, or when the window closes, has not proved what it set out to prove,
and the run ends with exit code 1. A script that cannot be read stops the
start-up with the line that is wrong.

A scripted run reads and writes [the editor's folder](#the-editors-folder)
as a window does. The application tests give every run a home of its own
(`CFFIXED_USER_HOME`, `XDG_DATA_HOME`), so `editor://` is under the test's
folder there; a scripted run started by hand writes the real one.

The proof of this mechanism is `tests/editor-scripts`: the editor opens
with the viewport alone, View > Log and its shortcut show and hide the Log
with a screenshot of each, the Log dragged out of its dock comes back
there when it is shown again, a mistake in `editor-settings.yml` is in the
Log with its line, the window's size and the Log survive a restart (two
runs in one test, the second reading what the first wrote), `--window-size`
wins over the file for its run, File > Quit ends the run,
a failed expectation names its step, a script that cannot be read stops
the start, and `--tools` alone draws the tools for a screenshot. The
`RecordingToolUi` fake of `neon/testing` records items and reacts to fed
events the same way, so `EditorScriptRunner` is unit-tested without Dear
ImGui. Every window the editor gains is proved this way: write the
checklist as a script next to the test, and keep for the owner only what
the script cannot see.

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
| `PlaceApplicationFolder` | Places `editor://` by the editor's names and reads `editor-settings.yml` from it, see [the editor's folder](#the-editors-folder) |
| `AdjustSettings` | The window is the editor's: its size and its mode are those of the editor's own settings unless `--window-size` or `--window-mode` names them, whatever the project says, and it is named `NeonEditor - <project>` unless the settings name it, see [the pointer and the window](#the-pointer-and-the-window) |
| `MakeRuntime` | Makes the `Editor`, and owns it |
| `AddSystems` | Makes `ImGui_ToolUiSystem` on the backends it is handed, with the window or without one, gives it to the editor, adds `EditorCameraSubmission` to the world after placing, tells the world not to take the pointer, and reads the script `--editor-script` names through `EditorScriptLoading`; one that cannot be read stops the start-up |

Without a window, `AddSystems` makes the tools only when `--tools` or
`--editor-script` asks for them; otherwise the editor runs as the runtime
does, which is what the checks of the scene rely on.

## How it is built

| Piece | Location | Role |
|---|---|---|
| `ToolUiContext` | [neon-core](../lib/neon-core/neon/tool-ui/tool-ui-context.hpp) | The interface of the tools: what the editor draws its windows with. Windows, a menu bar and menus, a dock space, text, buttons, checkboxes, a text field, and an image of a render target, declared anew in every frame |
| `ToolUiSystem` | [neon-core](../lib/neon-core/neon/tool-ui/tool-ui-system.hpp) | The base class of its backends, with the lifecycle: `Initialize()`, `CleanUp()`, `BeginFrame()`, `EndFrame()` |
| `ImGui_ToolUiSystem` | [neon-imgui](../lib/neon-imgui/neon/tool-ui/imgui-tool-ui-system.hpp) | The backend: Dear ImGui, its docking branch, drawn through its Vulkan backend into the frame, with its SDL2 backend reading the window when there is one and nothing but fed events when there is none |
| `Editor` | [NeonEditor](../app/NeonEditor/neon/editor/editor.hpp) | The runtime with the tools on top: the loop of the editor, the menu bar, and the windows. Derives from `Runtime` and runs as it does without a tool interface |
| `EditorWindows`, `EditorWindow` | [NeonEditor](../app/NeonEditor/neon/editor/ui/) | The registry of the tool windows, which the `View` menu is drawn from, see [the View menu](#the-view-menu-and-the-tool-windows) |
| `EditorFolder`, `EditorSettings`, `EditorSettingsFile` | [NeonEditor](../app/NeonEditor/neon/editor/settings/) | The editor's own folder and its two files, see [the editor's folder](#the-editors-folder) |
| `EditorHooks` | [NeonEditor](../app/NeonEditor/editor-hooks.hpp) | The editor's choices on the shared start-up, see [how it starts](#how-it-starts) |
| `EditorViewport`, `EditorLog` | [NeonEditor](../app/NeonEditor/neon/editor/ui/) | The viewport, and the one tool window so far |
| `EditorScript`, `EditorScriptRunner`, `EditorScriptLoading` | [NeonEditor](../app/NeonEditor/neon/editor/scripts/) | A script that drives the tools by their labels, what runs it frame by frame on `ToolUiContext`, and the step of the start-up that reads it, see [scripted runs](#scripted-runs) |
| `ToolUiItem`, `ToolUiEvents` | [neon-core](../lib/neon-core/neon/tool-ui/) | What the backend records of every item it draws, and the input a script feeds it |
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
MoltenVK on macOS, and is left for #580. Dear ImGui's own `imgui.ini` is
switched off, so that nothing is written next to wherever the editor was
started: the layout goes through `ToolUiSystem::LoadLayout()` and
`SaveLayout()` as text, and the editor keeps it in `layout.ini` of
[its folder](#the-editors-folder). A window the loaded layout does not
know, one shown for the first time ever, is docked where the
`ToolUiDockLayout` says, a side split off from the window in the middle,
when it is begun; every other window goes where it was last.

## Open questions

- What of the runtime's `Runtime` class the editor should share once it
  plays the game (#569): the pause menu, the settings menu, and the holds of
  the world and the sounds are the runtime's, and the editor derives from it
  for now.
