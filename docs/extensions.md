# Extensions

How an application is extended from outside: by a folder next to it, which
brings a library the application loads when it starts. It is to Neon what
GDExtension is to Godot. What is not a permanent feature of the engine hooks
in this way: a game's systems in C++, a loader for another engine's files, an
interpreter for another engine's scripts (#286, #287).

**Where this stands.** An extension is found, its library is opened, and it
is started and cleaned up. It writes to the log, registers components of its
own, which recipes and scripts then know, reaches the entity store, and adds
systems that run with the engine's own, in every frame and in every step. It
reads and changes the fields of any component by their names, reads the
actions of the input and files, places prefabs, and asks for another scene.
It is written in C against one header, or in C++ against a second one on
top, and is told what touched what by the physics and casts rays. See
[what comes next](#what-comes-next) for what is still open.

## Where extensions live

`extensions://` is the folder `extensions` next to the executable, as
`assets://` is the folder `assets`, see [file-systems.md](file-systems.md).
Every folder in it is an extension:

```
NeonRuntime
assets/
extensions/
  quake/
    extension.yml
    quake-macos-arm64.dylib
    quake-linux-x86_64.so
    quake-windows-x86_64.dll
    assets/
      scenes/e1m1.scene.yml
      prefabs/ scripts/ ui/
      palette.lmp
```

| Decision | Reason |
|---|---|
| Nothing lists the extensions; every folder that is there is one | As with the scripts of a project: what is there is used. A development run and a shipped game find them the same way |
| A game without the folder has no extensions, which is not an error | Most games have none |
| One folder for each extension, not loose libraries next to the executable | An extension brings more than a library: its recipe, and whatever else is its own |
| What an extension brings besides its code is in `assets/` of its folder, laid out as the assets of a project are | `extensions://quake/assets/scenes/e1m1.scene.yml` is written wherever a path is: in `project.yml`, in a scene, in `--scene`, in code. Two extensions cannot collide, and removing the folder removes everything of it |
| The scripts under an extension's `assets/` are read after the project's | An extension brings Lua as it brings scenes: a component in Lua next to its twin in C++, or an extension that is Lua alone and has no library |
| The libraries of every platform lie in one folder, each named for its platform | The folder is the same on every platform, and the exporter (#84) takes the libraries of the target |
| The scheme is read-only | As `assets://` |

A shipped runtime is to load only the extensions it was exported with (#143).
Until the exporter exists, everything that is found is loaded.

## The recipe

`extension.yml` says what the extension is.

```yaml
version: 1
name: quake

libraries:
  macos-arm64: quake-macos-arm64.dylib
  linux-x86_64: quake-linux-x86_64.so
  windows-x86_64: quake-windows-x86_64.dll
```

| Name | Holds | |
|---|---|---|
| `version` | The version of this layout, `1` | Required |
| `name` | What the extension is called: a plain name, the same as its folder | Required |
| `libraries` | The library for each platform, as a file in the folder of the extension | Left out by an extension that brings no library |

The platforms are `macos-arm64`, `macos-x86_64`, `linux-x86_64`,
`linux-arm64`, `windows-x86_64`, and `windows-arm64`. A name or a platform
that is not known is an error, so that one that was misspelled does not go
unnoticed.

## What happens when the application starts

| Step | What happens |
|---|---|
| 1 | The folders of `extensions://` are listed, and taken in the order of their names |
| 2 | The recipe of each is read |
| 3 | The library the recipe names for the platform is opened |
| 4 | Its function `neon_extension_initialize` is called with what the application offers, and the extension fills in what it brings |
| 5 | When the world comes up, `register_components` of each is called, with every other system registering its own. The extensions come before the scripts, so that a script finds their components |
| 6 | Once every component is registered and before the scene is read, `start` of each is called |
| 7 | When the application ends, `clean_up` of each extension is called, the last that was started first, and its library is closed |

Steps 1 to 4 happen once the log has its file and before any other system
comes up. Steps 5 and 6 are the system `ExtensionRunning`, which the
application adds to its world.

An extension with a problem is reported as an error and left out, and the
rest are started. The run goes on, and its exit code says that something
failed, as with every error.

| Problem | Message |
|---|---|
| The folder has no recipe | `extensions://quake has no extension.yml, which says what the extension is. It is left out` |
| The recipe has a mistake | The mistake with its line, then `The extension 'quake' is left out until its recipe is corrected` |
| No library for the platform | `The extension 'quake' has no library for this platform, linux-arm64. It is left out` |
| The library cannot be opened | `The library extensions://quake/quake-linux-x86_64.so of the extension 'quake' cannot be opened: ` and what the platform said |
| The library is no extension | `The library … exports no function 'neon_extension_initialize', so it is no extension. It is left out` |
| The extension returned 0 | `The extension 'quake' did not start. It is left out`, after what the extension logged itself |
| The extension is newer than the application | `The extension 'quake' was built with version 3 of the interface of extensions, and this application has version 1. It is left out` |

## The boundary is C

Everything an extension and the application see of each other is in one
header, [neon-extension.h](../lib/neon-extension-api/neon/extension/neon-extension.h):
plain types, structs, and pointers to functions.

| Decision | Reason |
|---|---|
| A C interface, not the C++ headers of the engine | Classes, `std::string`, `std::function`, and exceptions are laid out differently by every compiler, standard library, and build setting. Through C, an extension need not be built with what the application was built with, and an extension of last year still loads. It is what Godot, Quake's own game libraries, and audio plugins do |
| Two tables of function pointers: `NeonExtensionHost` from the application, `NeonExtension` from the extension | An extension links nothing of the engine, so nothing has to be exported from the executable. Everything it can call, it was handed |
| Both tables begin with the version they were built with, and a version only ever adds fields at the end | An extension built with an older version finds everything where it was. The application reads no field of an extension past what its version holds |
| An extension that is newer than the application is refused | It may rely on what the application does not have |
| C++ is a layer on top, in a header of its own, `neon-extension.hpp` | It is to the tables what godot-cpp is to GDExtension: `world.Get<T>()`, a class for a system, a query over types. It is compiled into the extension, so nothing of it crosses. See [an extension in C++](#an-extension-in-c) |

An extension in C:

```c
#include <neon/extension/neon-extension.h>

NEON_EXTENSION_EXPORT int neon_extension_initialize(const NeonExtensionHost *host, NeonExtension *extension)
{
  extension->abi_version = NEON_EXTENSION_ABI_VERSION;

  host->log(host->context, NEON_LOG_INFO, "Hello from an extension in C");
  return 1;
}
```

In C++ the function is `extern "C"`, see
[polite.cpp](../tests/extensions/polite/polite.cpp), which also keeps
something of its own and is told to clean up.

| Version | Adds to what the application offers | Adds to what an extension brings |
|---|---|---|
| 1 | `log` | `clean_up` |
| 2 | `register_component`, `find_component`, `create_entity`, `destroy_entity`, `is_alive`, `find_entity`, `set_parent`, `get_parent`, `set_component`, `get_component`, `has_component`, `remove_component`, `create_query`, `each` | `register_components`, `start` |
| 3 | `register_system` | |
| 4 | `find_field`, `get_field`, `set_field`, `get_field_text`, `set_field_text`, `is_action_down`, `was_action_pressed`, `action_axis`, `action_axis2`, `action_axis3`, `file_exists`, `read_file`, `spawn`, `load_scene` | |
| 5 | `listen_to_physics`, `cast_ray` | |
| 6 | `get_field_layout`, `set_ui_number`, `set_ui_text`, `spawn_at` | |
| 7 | `add_component`, `set_field_texts`, `set_image`, `set_mesh`, and `name`, the name of the extension | |
| 8 | `set_mesh_lightmap` | |
| 9 | `set_sound` | |
| 10 | `write_file`, `list_files`, `request_quit` | |
| 11 | `set_shader_numbers` | |
| 12 | `ui_show`, `ui_close`, `ui_find`, `ui_create`, `ui_remove`, `ui_set_field`, `ui_set_style`, `ui_set_visible`, `ui_listen`, `ui_unlisten`, `get_view_size` | |
| 13 | `set_group_volume`, `get_group_volume` | |
| 14 | `free_unused` | |
| 15 | `set_component_enabled`, `is_component_enabled` | |
| 16 | `pool_add`, `pool_acquire`, `pool_release`, `pool_count`, `pool_free_count` | |
| 17 | `set_window_mode`, `get_window_mode`, `set_window_size`, `get_window_size`, `list_display_sizes`, `set_vertical_sync`, `get_vertical_sync`, `set_frame_limit`, `get_frame_limit` | |

## Components

A component of an extension is a struct of plain fields, described to the
application field by field. From the description the application makes what
it makes for a component a script declares: how the store keeps it, a
description for the editor and for scripts, and how a recipe reads and writes
it. A scene then holds it as it holds any other:

```yaml
- name: wheel
  components:
    Spinner:
      speed: 45
```

```c
typedef struct Spinner
{
  float speed;
  NeonVector3 axis;
  int32_t turns;
  bool enabled;
} Spinner;

static const NeonFieldDescription spinner_fields[] = {
  {"speed", NEON_FIELD_FLOAT, offsetof(Spinner, speed), {90.0}, "How far it turns in a second, in degrees"},
  {"axis", NEON_FIELD_VECTOR3, offsetof(Spinner, axis), {0.0, 1.0, 0.0}, "What it turns around"},
  {"turns", NEON_FIELD_INTEGER, offsetof(Spinner, turns), {0.0}, 0},
  {"enabled", NEON_FIELD_BOOLEAN, offsetof(Spinner, enabled), {1.0}, 0},
};

static void register_components(void *context)
{
  const NeonComponentDescription description = {"Spinner", "Turns what carries it", sizeof(Spinner), spinner_fields, 4};
  spinner = host->register_component(host->context, &description);
}
```

The whole of it is [spinner.c](../tests/extensions/spinner/spinner.c), which
also creates an entity, sets the component, and runs a query.

| Decision | Reason |
|---|---|
| The fields lie as a script's do: one after the other, each at the alignment of what it holds | It is how a compiler lays out the struct, so the extension reads its component as a struct and the engine reads it by its description. `ScriptComponentLayout` is used for both, which was written to be neutral to the language |
| The description carries `sizeof` the struct and `offsetof` every field, and the application checks them against where it puts the fields | A struct that holds something that is not described, or has its fields in another order, would otherwise be read as wrong values and nothing else. It is refused with the byte where the two differ |
| The struct holds the described fields alone | What a component keeps for itself, a pointer or a handle, has no place yet. It comes with a kind for it, or with a hook for a component that is removed |
| Numbers, booleans, vectors, colours, and quaternions; no text and no lists | Text and lists are `std::string` and `std::vector` in the engine, which do not cross the boundary. They come as kinds of their own, reached through functions |
| A default is up to four numbers | Enough for every kind there is. It is what a recipe leaves out |
| A name that is taken is refused, whoever took it | The engine, another extension, or the same one twice. A script that declares the name afterwards is refused by the scripts in the same way |
| Components are registered in `register_components` alone | It is when every system registers its own, before any query is made. Later the store would hold entities that queries have already been built for |

The store is reached from `start` on, until the world is cleaned up. A call
at another time, or one that cannot be done (no component, an entity that is
not alive, no query), is an error in the log under the name of the extension
and returns 0.

A component of the engine is found by its name, `find_component(host->context, "Transform")`,
and can be queried for, but how it lies in memory is the engine's own. It is
read and changed through its fields, see below.

## The services of the engine

What an extension reaches besides the store. Like the store, they are for
`start` and what runs after it; files are read at any time.

| Service | Functions | Notes |
|---|---|---|
| The fields of any component | `find_field`, `get_field`, `set_field`, `get_field_text`, `set_field_text` | By the names a recipe writes them with: `find_field(host->context, "Transform", "position")`, once, in `start`. It works for a component of the engine, of a script, and of another extension alike, through the same description the scene file and Lua use |
| Input | `is_action_down`, `was_action_pressed`, `action_axis`, `action_axis2`, `action_axis3` | The actions of the input map, less what the user interface used, see [input.md](input.md) |
| Files | `file_exists`, `read_file`, `write_file`, `list_files` | Virtual paths, under every rule of [file-systems.md](file-systems.md). An extension's own files are `extensions://<name>/…`. What it writes goes under `user://`, see [what an extension keeps](#what-an-extension-keeps) |
| The world | `spawn`, `spawn_at`, `load_scene` | A prefab under a parent or at the top, see [prefabs.md](prefabs.md), with `spawn_at` at a position and a rotation written on top of its `Transform`; another scene when the frame is done |
| The user interface | `set_ui_number`, `set_ui_text`, and the `ui_…` functions | The values its files show as `{name}`, see [user-interface.md](user-interface.md); and its files and elements themselves, see [what an extension shows on the screen](#what-an-extension-shows-on-the-screen) |
| The view | `get_view_size` | The size in pixels of what the camera of the window draws to |
| Components of the engine | `add_component` | Gives an entity a component by its name, with what its fields start with, as `Renderable: Default` in a recipe. An extension has no struct for the engine's components, so this is how an entity it creates gets a `Transform` or a `Renderable`; the fields are then set with `set_field`, and a list of text, such as `textures`, with `set_field_texts` |
| What is drawn | `set_image`, `set_mesh` | A picture and a mesh the extension made, see [what an extension draws](#what-an-extension-draws) |
| Physics | `listen_to_physics`, `cast_ray` | What began and ended to touch in a frame, before `update`, as the hooks of a script are told; and the first thing a ray hits. A body itself is a component, `RigidBody`, `CharacterBody`, or `Trigger` with a `Collider`, and is moved through its fields, see [physics.md](physics.md) |
| The application | `request_quit` | Asks it to close once the frame is done, see [what an extension keeps](#what-an-extension-keeps) |
| Audio | none of its own | A sound is a component and is played through its fields, see [audio.md](audio.md) |

| Decision | Reason |
|---|---|
| The engine's components are reached by the names of their fields, not as structs | `Transform` is a C++ type with a layout of the compiler's. Its description is already what a recipe, the editor, and Lua go through, so an extension sees the same names and the same checks, and a component that changes its layout breaks no extension |
| A field is found once and named by a number afterwards | The lookup by name happens in `start`, not in every frame |
| A value crosses as up to four numbers, or as text | Every kind an extension can hold is made of them: a vector, a colour as r, g, b, a, a quaternion as x, y, z, w, a boolean as 0 or 1. A choice crosses as its word. Lists and matrices do not cross yet |
| A field checks what it is given | As when a recipe is read: a value the field does not take is an error in the log and changes nothing |
| `read_file` hands the bytes to a function of the extension | Nothing is allocated on one side and freed on the other. The extension copies what it keeps |
| Reading a field with `get_field` costs a call and a conversion | It is for the handful of entities a system touches, and for a field that is worked out when it is read, such as the rotation of a `Transform` in degrees |
| `get_field_layout` says where a field lies, and the extension reads it in place | For a system that runs over thousands of entities: a query that names `Transform` hands over its column, and the position of entity `i` is at `column + i * stride + offset`. The layout is asked of the application that runs, in `start`, so nothing of it is compiled into the extension, and an engine that moves the field breaks no extension. What is written in place is not checked |

## What an extension draws

An extension that reads a format of its own, a level, a model, a picture of
another engine, shows what it read by handing the engine a mesh and
pictures. Nothing of it is in a file the engine could open.

```cpp
const std::string picture = world.SetImage("wall", 64, 64, pixels);   // image://quake/wall

const Entity wall = world.CreateEntity("wall");
world.AddComponent(wall, "Transform");
world.AddComponent(wall, "Renderable");
world.SetText(wall, world.FindField("Renderable", "shader"), "assets://shaders/basic-lit");
world.SetTexts(wall, world.FindField("Renderable", "textures"), {picture});
world.SetMesh(wall, corners, indices);
```

| Decision | Reason |
|---|---|
| A mesh is given to the `Renderable` of an entity, in place of a model file | It is the path a `Geometry` goes, and a rope: `RenderInfo::mesh`. The shader, the material, and everything else of the `Renderable` stay what they are, and an entity that is drawn already is drawn with the new mesh from the next frame |
| A corner is as the engine keeps its own: position, normal, where in the texture, and a colour; in metres, y up, triangles anticlockwise from outside | The corners cross as they lie, without a copy of each number. Another engine's units and axes are the extension's to convert |
| A picture is four bytes a pixel, set under a name, and read by a material as the texture `image://<extension>/<name>` | A texture is named by a path everywhere in the engine, in a recipe, a prefab, a field; this makes a picture from memory one more thing a path can name, as `surface://` names what a camera drew. The name of the extension stands in front, so two extensions cannot take each other's |
| A picture that is set again under its name is drawn anew, from the next frame on, when it keeps its size | A game works some pictures out while it runs: the light of a level that flickers, a map that fills in. The renderer writes the new pixels into the texture that is there, so no material is made again and nothing that names the picture has to know. It waits for the frame before to finish first, so a picture is set again a few times a second, not a few hundred. Another size is refused: what was made keeps the size it was made with |
| `image://` is no scheme of the file system | Nothing reads or lists it as a file. It is the renderer's, `RenderContext::SetImage`. What the path of a texture names, a file, a `surface://`, or an `image://`, is said in one place, `TextureSource` of neon-core, which every renderer and the user interface ask |

## The shaders an extension brings

An extension that draws what the shaders of the engine do not, water that
swims, a sky that drifts, a texture read pixel by pixel, brings shaders of
its own. A material names one as it names any other, by a path without an
ending, and the renderer reads the two compiled files next to it:

```cpp
world.SetText(wall, world.FindField("Renderable", "shader"), "extensions://quake/assets/shaders/liquid");
// reads extensions://quake/assets/shaders/liquid.vert.spv and liquid.frag.spv
```

The extension compiles them when it is built, with `glslang`, the compiler
the engine compiles its own with, and with the folder of the engine's
shaders to include from (`app/NeonRuntime/shaders/vulkan`):

```sh
glslang -V -I<engine>/app/NeonRuntime/shaders/vulkan liquid.frag -o <game>/extensions/quake/assets/shaders/liquid.frag.spv
```

A shader that is run over the whole picture of a camera, an effect, is
brought the same way, as a fragment shader alone, and named in `effects` or
`screen_effects` of a `Camera`: see
[the effects of a camera](vulkan-renderer.md#the-effects-of-a-camera).

An entity is drawn with what its `Renderable` says now: setting `textures`,
`shader`, or a field of `material` of an entity that is drawn already shows
at the next frame. A texture that was shown stays loaded, so an entity goes
back and forth between textures at no cost. An extension that changes its
levels itself, within one scene, says when a level is over, so that what
only that level showed is freed:

```cpp
world.FreeUnused();   // once the next level was shown
```

| What such a shader may rely on | |
|---|---|
| `scene-data.glsl` | The declarations every shader of the engine starts with: `scene`, with the view, the projection, where the camera stands, and the lights; `objects[]`, with the matrix, the colour, and the material of what is drawn. A shader includes it and reads what it needs |
| The corners | Position, normal, texture coordinates, colour, and lightmap coordinates, at locations 0 to 4, as `unlit.vert` reads them |
| The textures | The first texture of the material at binding 2, read through the sampler at binding 5, as `unlit.frag` does; a lightmap through `lightmap.glsl` |
| `scene.time` | `x` is the seconds the world has run, `y` how long its last frame took. The time stands still while the world does, as while a game is paused |
| `scene.numbers[0..7]` | Eight places of four numbers each for what the game tells its shaders, set with `SetShaderNumbers` |

```cpp
world.SetShaderNumbers(0, {fog_density, 0.0f, 0.0f, 0.0f});   // scene.numbers[0].x in every shader
```

| Decision | Reason |
|---|---|
| The time is in what every shader is told of a scene | A shader that moves something needs it, and nothing an object carries changes by itself. It is the time of the world, so what a shader moves stands still with the game |
| A game has eight places of four numbers, by place and not by name | A shader reads a place without looking anything up, and a block of numbers costs nothing to hand over. What a place means is between a game and its own shaders. Two extensions that both bring shaders agree on their places; names are for when that hurts |
| The numbers are of the scene, not of an object | What differs from one object to the next is its material. These are what holds for everything that is drawn: fog, a wind, the light of a storm |
| They stand behind the lights in `SceneData` | A shader that was compiled before reads what it read, where it read it |

## What an extension plays

A sound an extension reads out of an archive of its own is in no file the
engine could open either. The extension hands over the bytes as a file would
hold them, WAV, FLAC, MP3, or Ogg Vorbis, and a `SoundSource` plays them by
the path it gets back.

```cpp
const std::string sound = world.SetSound("doors/open", bytes);   // sound://quake/doors/open

const Entity door = world.CreateEntity("door sound");
world.AddComponent(door, "Transform");
world.AddComponent(door, "SoundSource");
world.SetText(door, world.FindField("SoundSource", "sound"), sound);
```

| Decision | Reason |
|---|---|
| The bytes of a sound file are set under a name, and played as the sound `sound://<extension>/<name>` | It is what `image://` is for a picture: a sound is named by a path everywhere, and this makes one from memory one more thing a path can name. Everything else of a sound, its place, its group, whether it loops, stays with `SoundSource` |
| The bytes are those of a file, not samples | The audio decodes a file already, so a WAV out of an archive plays as it is, with its rate and its channels, and the extension needs no decoder |
| A name that is set again names the new bytes for the sounds made from then on | A sound that plays keeps what it was made from |
| `sound://` is no scheme of the file system | Nothing reads or lists it as a file. It is the audio's, `AudioContext::SetSound`, and it is written down in one place, `SoundMemory` of neon-core |

The test [tests/runtime-extensions](../tests/runtime-extensions) does this
with a picture of four colours on a square, and reads the pixels of what the
runtime drew.

Light that was worked out ahead, as a level of another engine carries it, is
a second set of coordinates and a picture: `SetMeshLightmap(entity, coordinates)`
gives every corner of the mesh its place in the lightmap, and the field
`material.lightmap` of the `Renderable` names the picture, see
[vulkan-renderer.md](vulkan-renderer.md#lightmaps). The coordinates are
handed over apart from the corners, so that a corner is what it was in
version 7.

What an extension draws can be collided with as it is: an entity with a
`Collider` of the shape `mesh` or `convex_hull` that names no model takes the
mesh that was handed to its `Renderable`, as it would take a `Geometry`, see
[physics.md](physics.md). So a level is given a `RigidBody` of the kind
`static` and such a `Collider` with `add_component` and `set_field_text`, and
what is seen is what is walked on.

Not yet: letting a picture go again.

## What an extension keeps

A game keeps what is the player's, a saved game, the settings of its menu,
under `user://`, and finds it there the next time. Its own menu has a quit as
well, which asks the application to close.

```cpp
world.WriteFile("user://saves/slot-1.sav", bytes);        // the folder is created

for (const std::string &name : world.ListFiles("user://saves"))   // slot-1.sav, slot-2.sav
{
  std::vector<std::uint8_t> saved;
  world.ReadFile("user://saves/" + name, saved);
}

world.RequestQuit();   // the frame is finished, then everything is cleaned up
```

| Decision | Reason |
|---|---|
| An extension writes under `user://` alone | It is the one place a game may write, on every platform. `output://`, which the file system writes as well, is for what a run hands back to whoever started it, and is not the game's |
| A path that is empty, is under another scheme, or has `..` in it is refused, with the reason in the log of the extension | The file system would refuse most of them too, in a log of its own. Who wrote the path reads why in the log that carries the name of the extension |
| `write_file` writes a whole file and replaces one that is there; folders on the way are created | It is `WriteBytes` of the file system, which a screenshot and a scene that is saved go through. A file may hold nothing |
| `list_files` gives the names of the files directly in a folder, in the order of their names, to a function of the extension | As `read_file` hands over its bytes: nothing is allocated on one side and freed on the other. Folders, and what is in them, are left out, so a folder of saved games lists the saved games. Any folder that can be read can be listed |
| A folder that is not there lists as nothing, without an error | `user://saves` is not there before the first game was saved |
| `request_quit` tells the window to close, as the button `quit` of the pause menu does | The application leaves its loop once the frame is done and cleans up as it always does. An extension that ends the process itself skips that: the log is cut short, and what the audio and the renderer hold is never let go of |

## What an extension shows on the screen

A file of the user interface says ahead what is shown, and an extension sets
the values it shows as `{name}`. What a file cannot say ahead, a picture at a
place that changes with the game, a row for every saved game, a menu that is
shown and closed, an extension does with the elements themselves: the part
of `UiContext` a game needs, see
[from code and scripts](user-interface.md#from-code-and-scripts).

```cpp
world.ShowUi("extensions://quake/assets/ui/hud.ui.yml");

const UiElement face = world.CreateUi(
  "type: label\n"
  "name: face\n"
  "text: 100\n",
  world.FindUi("status-bar"));               // or under the top of the topmost file

world.SetUiField(face, "text", "75");        // a field, from text as a file writes it
world.SetUiStyle(face, "left", "112px");     // a property, as CSS writes both
world.SetUiVisible(face, false);

const UiListener listening = world.ListenToUi(world.FindUi("new-game"), "click", [&world](const UiEvent &event)
{
  world.CloseUi("extensions://quake/assets/ui/menu.ui.yml");
});

int width = 0;
int height = 0;
world.GetViewSize(width, height);            // pixels of what the window's camera draws to

world.RemoveUi(face);                        // `face` names nothing from here on
```

How the window is shown, for the video settings of a menu an extension
brings. Each takes effect at once, and the game keeps what the player chose
with its other settings:

```cpp
world.SetWindowMode(2);                      // 0 a window, 1 without borders, 2 the whole display
world.SetWindowSize(1920, 1080);             // in points
for (const auto &[width, height] : world.ListDisplaySizes()) { /* what the display offers, largest first */ }
world.SetVerticalSync(false);                // frames as fast as they are done
world.SetFrameLimit(144);                    // at most 144 a second, from 30 to 300; 0 for no limit
```

See [vulkan-renderer.md](vulkan-renderer.md#vertical-sync-and-the-window).

How loud a [group of sounds](audio.md#groups) is, which a menu of settings
an extension brings sets with a slider:

```cpp
world.SetGroupVolume("music", 0.4f);         // 1 as the sounds are, 0 for silence
float music = world.GetGroupVolume("music"); // 0 for a group that is not there
```

In C the same are `ui_show`, `ui_close`, `ui_find`, `ui_create`, `ui_remove`,
`ui_set_field`, `ui_set_style`, `ui_set_visible`, `ui_listen`, `ui_unlisten`,
and `get_view_size` of the table, see
[sign.cpp](../tests/extensions/sign/sign.cpp) for all of them at work.

| Decision | Reason |
|---|---|
| An element is named by a number of 64 bits, `NeonUiElement`, 0 for none | It is the handle of the engine, `UiHandle`, as it is: a number that is given once and never again. The name of an element that was removed, or whose file was closed, names nothing, and a call that is handed it does nothing, returns 0, and says so in the log. Nothing dangles |
| A file is shown and closed by its path | `ui_show` is `UiContext::Load`, which the runtime shows `--ui PATH` and its pause menu with, and `ui_close` is `Unload`. The host keeps which file of the extension is which, so no number of a file crosses, and an extension closes only what it showed itself. A file that is shown already stays as it is |
| `ui_create` takes YAML, as the files are written | It is `UiContext::Create`: one format for an element, in a file and from code, with the same checks and the same messages. A parent of 0 is the element at the top of the topmost file, so a game that shows one file needs to find nothing first |
| A field is set from text, whatever it holds | `SetField` of the engine takes a text, a number, or a flag, by the kind of the field. The host asks the description of the element, `DescribeElement`, what the field holds and reads the text as that, as an inspector does: `75` for `value` of a bar, `true` for `checked`. One function crosses in place of one for every kind, and a text that is no such value is refused with the reason |
| The style is set as CSS writes it, by `ui_set_style` | It is `UiContext::Set`: the property counts as written on the element itself, on top of its file and its sheets, and an empty value takes it back. It is how an element is placed: `left`, `top`, `width` |
| A listener is a function and a pointer of the extension, and gets the target, the name of the event, and where the pointer is | It is `UiContext::On`, so an event that goes up reaches what listens above its target. The name is valid during the call. More of an event, the key, the wheel, can be appended to `NeonUiEvent` when something needs it |
| What an extension listens with is taken away before its library is closed | The user interface outlives the extensions, and would call into a library that is gone. The host keeps what each extension listens with, as it keeps what is told of the physics, and `ui_unlisten` takes away only what is the extension's own |
| The elements are reached from `start` on, like the store | Before that the world is not up, and a call says so in the log |
| `get_view_size` gives what the renderer draws to, in pixels | It is `RenderContext::GetRenderResolution`, which follows the window when it is resized and is the frame of a run without a window. The user interface on the window is laid out in it, divided by its scale, see [points, pixels, and density](user-interface.md#points-pixels-and-density) |
| An `image` of the user interface shows a picture of `set_image`, as `src: image://<extension>/<name>` | The status bar of a game is pictures out of its own archives. The user interface asks the renderer for the pixels of the name the first time the image is drawn, makes a texture of them, and keeps it; a picture that is handed over later is looked for again until it is there, as a surface is. A picture that is set again is not drawn anew by the user interface: the element is given another name |

## Systems

A system is a name and up to three functions: `update` once per frame,
`fixed_update` once per step of the world, and `interpolate` once per frame
before it is drawn, as `EntitySystem` of the engine has them, see
[entity-component-system.md](entity-component-system.md). It is added with
`register_system` while the extension is started or in `register_components`,
and makes its queries in `start`.

| Decision | Reason |
|---|---|
| The systems of an extension run in the order they were added, the extensions in the order of their names, before the scripts and before the physics | As a game's systems do: what they ask for in a step is part of that step of the physics |
| No system is added once the world runs | The order of the systems is settled before the first frame |
| A system is handed a pointer of its own, `user` | It is how a system in C keeps its state, and how the layer for C++ finds its object |

## An extension in C++

[neon-extension.hpp](../lib/neon-extension-api/neon/extension/neon-extension.hpp)
turns the tables into classes. A whole game's worth of code looks like this,
see [tumbler.cpp](../tests/extensions/tumbler/tumbler.cpp):

```cpp
#include <neon/extension/neon-extension.hpp>

using namespace neon::extension;

struct Spinner
{
  float speed = 90.0f;
  float angle = 0.0f;
};

class Spinning final : public System
{
  Query<Spinner> _spinners;

public:
  void Start(World &world) override { _spinners = world.CreateQuery<Spinner>(); }

  void FixedUpdate(World &world, const double step) override
  {
    _spinners.Each([step](Entity, Spinner &spinner) { spinner.angle += spinner.speed * static_cast<float>(step); });
  }
};

class Game final : public Extension
{
public:
  bool Initialize(World &world) override
  {
    AddSystem<Spinning>("Spinning");
    return true;
  }

  void RegisterComponents(World &world) override
  {
    world.RegisterComponent<Spinner>("Spinner", "Turns what carries it", {
      Field("speed", &Spinner::speed, "How far it turns in a second, in degrees"),
      Field("angle", &Spinner::angle),
    });
  }
};

NEON_EXTENSION(Game)
```

| Piece | Is |
|---|---|
| `Extension` | The class of the extension: `Initialize`, `RegisterComponents`, `Start`, `CleanUp`, and `AddSystem<S>(name, arguments…)` |
| `System` | `Start`, `OnPhysicsEvent`, `Update`, `FixedUpdate`, `Interpolate` |
| `World` | What the application offers: the log, `RegisterComponent<T>`, entities, `Set<T>`, `Get<T>`, `Has<T>`, `Remove<T>`, `CreateQuery<Ts…>`, `FindField` with `GetNumber`, `GetVector3`, `GetText` and their `Set…`, `IsActionDown`, `WasActionPressed`, `ActionAxis2`, `ReadFile`, `Spawn`, `SpawnAt`, `LoadScene`, `CastRay`, `SetUiNumber`, `SetUiText`, `ShowUi`, `CloseUi`, `FindUi`, `CreateUi`, `RemoveUi`, `SetUiField`, `SetUiStyle`, `SetUiVisible`, `ListenToUi`, `UnlistenToUi`, `GetViewSize`, `SetEnabled`, `IsEnabled`, `AddToPool`, `AcquireFromPool`, `ReleaseToPool`, `CountInPool`, `CountFreeInPool`, `SetWindowMode`, `GetWindowMode`, `SetWindowSize`, `GetWindowSize`, `ListDisplaySizes`, `SetVerticalSync`, `GetVerticalSync`, `FreeUnused`, `SetGroupVolume`, `GetGroupVolume`, `AddComponent`, `SetTexts`, `SetImage`, `SetMesh`, `SetMeshLightmap`, and `CreateBlockQuery` with `PlaceField<T>` for a field of the engine in place |
| `Query<Ts…>` | `Each([](Entity, Ts &…) { … })` |
| `Field(name, &T::member, description)` | A field from the member itself: its kind from its type, its offset from where it lies, and its default from what `T{}` holds, so a default is written once, in the struct |
| `NEON_EXTENSION(Class)` | The function the application starts the extension by |

A type of the extension's own that lies in memory as one of the kinds, a
`glm::vec3` for one, is made known by a specialization of `FieldKindOf`. An
exception must not leave a function the application calls.

## A game in C++

A project with code of its own has its assets and its extension side by
side, see [projects.md](projects.md#a-project-with-code-in-c). The bench,
[projects/bench](../projects/bench/README.md), is the first: five components,
five systems, and the same work in Lua next to it, all inside its extension.

## Building an extension

An extension is built **without the engine**: it is compiled against the two
headers and links nothing, so its build knows nothing of the engine's code
and takes as long as its own files do. This is how a game in C++ is worked
on.

```cmake
cmake_minimum_required(VERSION 3.18)
project(quake C CXX)
include(<the engine>/cmake/NeonSdk.cmake)

neon_add_extension(quake
        SOURCES quake.cpp pak-file.cpp
        DIRECTORY <a NeonRuntime>/extensions
)
```

`neon_add_extension` of [NeonExtensions.cmake](../cmake/NeonExtensions.cmake)
builds one and puts it where the application finds it.

| What | Is |
|---|---|
| The target | `quake-extension` |
| The library | `extensions/quake/quake-<platform>.dylib`, `.so`, or `.dll`, in the folder `DIRECTORY` names, or next to the NeonRuntime the engine builds |
| The recipe | `extension.yml` next to the `CMakeLists.txt`, or what `RECIPE` names, copied next to the library |
| The assets | The folder `assets` next to the recipe, or what `ASSETS` names, copied to `extensions/quake/assets/` whenever the extension is built |
| What is exported | Only what is marked `NEON_EXTENSION_EXPORT`; everything else is hidden |

| Decision | Reason |
|---|---|
| `NeonSdk.cmake` is one include and compiles nothing of the engine | Someone who writes a game in C++ should not build the engine, wait for it, or think about it. The runtime is used as it was built, by the engine's build or from a release |
| Any compiler | The boundary is C. The bench builds with Apple's clang against a runtime built with LLVM's |
| For another platform, the build is given the engine's toolchain file | `-DCMAKE_TOOLCHAIN_FILE=<the engine>/cmake/toolchains/windows-x64-llvm-mingw.cmake`, in the image of [docker/windows-x64.dockerfile](../docker/windows-x64.dockerfile). The presets of the engine name it themselves; a build of its own has no presets, so it is said once when configuring, by its whole path |
| CMake 3.18 or later | An extension in C++ links its runtime statically and one in C links none, which the build says by the language that is linked |
| The engine's build can build an extension too | For the extensions of the tests, and the projects under `projects/`. The same `CMakeLists.txt` serves both |

A game is a project, an extension, or both, see
[projects.md](projects.md#a-project-with-code-in-c). For both there is
`neon_add_project`, which also puts the game together with a runtime.

## In the code

| Piece | Location | Role |
|---|---|---|
| `neon-extension.h` | neon-extension-api | The boundary |
| `neon-extension.hpp` | neon-extension-api | The layer for C++, compiled into an extension |
| `ExtensionHost` | neon-core | Finds, starts, and cleans up the extensions |
| `FillExtensionHostTable` | neon-core | The functions behind `NeonExtensionHost`: the log, components, the store, systems, and the services |
| `ExtensionServices` | neon-core | What of the engine the extensions reach besides the store, each through its interface |
| `ToNumbers`, `FromNumbers` | neon-core | A field's value as plain numbers and back, which is how it crosses |
| `ExtensionRunning` | neon-core | The system through which the extensions take part in the world |
| `ExtensionFile`, `ExtensionRecipe` | neon-core | The recipe |
| `LibraryLoader`, `NativeLibrary` | neon-core | The interfaces for opening a library of the platform at a virtual path |
| `SDL2_LibraryLoader`, `SDL2_NativeLibrary` | neon-sdl2 | The backend. The file system hands it the native path, `FileSystem::OpenLibrary`, which the rest of the engine never sees |

The tests are in [tests/extensions](../tests/extensions): thirteen small
extensions that are built next to the test and started for real, with a store
of Flecs for those that bring components, and recipes in memory.

## What comes next

| Step | What an extension can do then |
|---|---|
| More of the physics | Shapes cast and overlapped, forces and impulses on a body |
| What a component keeps for itself, text and lists as fields | A pointer or a handle with a hook for when the component is removed; `std::string` and lists reached through functions |
| Loaders | Asset loaders and schemes of the file system, meshes and textures from memory, which the Quake extension (#287) needs |

Open: extensions in Lua through the same recipe; the web build, which cannot
open a library and links extensions statically (#86); reloading a library
while the editor runs (#104).
