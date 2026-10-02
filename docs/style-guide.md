# Style guide

This guide describes the conventions the code follows today. It was written
by reading the code, not the other way around. Where the code disagrees with
itself, the guide names the form that is used most, and lists the rest under
[open points](#open-points).

Most of it comes from the
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
Some of it is borrowed from other languages, mainly C#. The tables say where a
rule differs from Google, and where it comes from.

Two tools check part of it, see [tooling](#tooling).

| File | Holds |
|---|---|
| [.clang-tidy](../.clang-tidy) | The naming rules, and checks for common mistakes |
| [.clang-format](../.clang-format) | The layout |
| [.editorconfig](../.editorconfig) | Indentation, line endings, and the final newline, for every editor |

## Naming

| What | Form | Example | Google | Borrowed from |
|---|---|---|---|---|
| Class, struct | `PascalCase` | `CommandLine`, `SettingsConfig` | Same | |
| Class of a backend, or one of several implementations | Prefix, underscore, `PascalCase` | `SDL2_WindowSystem`, `VK_Device`, `Headless_InputSystem`, `Forward_RenderPipeline` | Differs, no underscores | |
| Method | `PascalCase` | `GetDrawableSize()`, `CleanUp()` | Same | C# |
| Function outside a class | `snake_case` | `get_file_extension()`, `rate_device_type()` | Differs, `PascalCase` | C++ standard library |
| Private or protected member | `_snake_case` | `_logger`, `_window_flags` | Differs, `snake_case_` | C# |
| Public field of a struct | `snake_case` | `window_mode`, `max_frames` | Same | |
| Local variable, parameter | `snake_case` | `native_path`, `delta_time` | Same | |
| Constant | `snake_case` | `assets_scheme`, `frame_time` | Differs, `kPascalCase` | |
| Enum | `PascalCase` | `WindowMode` | Same | |
| Enumerator | `PascalCase` | `WindowMode::Borderless` | Differs, `kPascalCase` | C# |
| Namespace | `snake_case`, and there is one | `neon` | Same | |
| Template parameter | `PascalCase` | `T`, `Args` | Same | |
| Macro | `UPPER_CASE` | `STB_IMAGE_IMPLEMENTATION` | Same | |
| Header guard | File name in `UPPER_CASE`, then `_HPP` | `FILE_SYSTEM_HPP` | Differs, the guard holds the path | |
| File | `kebab-case`, `.hpp` and `.cpp` | `file-system-context.hpp` | Differs, `snake_case` and `.h`, `.cc` | |
| Folder of a library | `kebab-case` | `lib/neon-core`, `neon/command-line` | | |
| Folder of an application, CMake target of one | `PascalCase` | `app/NeonRuntime` | | |

### Backend classes

A class that implements an interface for one backend carries the backend in
its name. An editor that checks names is told to accept it:

```cpp
// ReSharper disable once CppInconsistentNaming
class SDL2_WindowSystem final : public WindowSystem
```

clang-tidy accepts a name of two `PascalCase` parts with one underscore
between them, so no comment is needed for it.

The file of such a class is named like any other: `sdl2-window-system.hpp`,
`vk-render-system.hpp`.

### Systems and contexts

| Name | Is | Example |
|---|---|---|
| `XContext` | What the rest of the engine sees of X. An interface | `WindowContext`, `FileSystemContext` |
| `XSystem` | The base class of the backends of X. Derives from `XContext` and adds the lifecycle: `Initialize()`, `CleanUp()`, and `Update()` where needed | `WindowSystem`, `FileSystem` |
| `Prefix_XSystem` | One backend | `SDL2_WindowSystem`, `Headless_WindowSystem` |

Code that uses a system takes the context. Only `main.cpp` and the runtime
hold the system.

## Layout

| Rule | Example | Google |
|---|---|---|
| Two spaces, no tabs | | Same |
| Lines stay under 120 characters | 1 line of 7057 is longer, 92 are longer than 100 | Differs, 80 |
| Braces of a namespace, class, function, and control statement go on a line of their own | See below | Differs, same line |
| `else` and `catch` follow the closing brace, and their brace goes on the next line | `} else` | Differs |
| The contents of a namespace are indented | | Differs |
| A namespace closes with its name | `} // neon` | Differs, `// namespace neon` |
| `public:`, `protected:`, `private:` are level with `class` | | Differs, one space in |
| `case` is indented in its `switch`, and its block has braces on lines of their own | | |
| Every `if`, `for`, and `while` has braces | | Same in effect |
| A guard clause may stay on one line | `if (_destroyed) { return; }` | Differs |
| An empty body stays on one line | `void Update() {}` | Same |
| `*` and `&` bind to the name | `Node *parent`, `const std::string &path` | Differs, to the type |
| No space after `template` | `template<typename T>` | Differs |
| Arguments or parameters that do not fit go one on each line, indented once. The closing bracket stays on the last one | See below | Differs, packed and aligned |
| One empty line between functions, also between declarations in a class | | |
| A file ends with a newline, lines end with LF | | Same |

```cpp
namespace neon
{
  void SDL2_WindowSystem::SetWindowFocus(const bool focus)
  {
    if (focus)
    {
      SDL_SetWindowGrab(_window, SDL_TRUE);
    } else
    {
      SDL_SetWindowGrab(_window, SDL_FALSE);
    }
  }
} // neon
```

```cpp
_window = SDL_CreateWindow(
  _settings_config.title.c_str(),
  SDL_WINDOWPOS_CENTERED,
  SDL_WINDOWPOS_CENTERED,
  _settings_config.width,
  _settings_config.height,
  _window_flags);
```

A constructor puts its initialisers on the next line, starting with the
colon, and each further one below the first:

```cpp
explicit VK_RenderSystem(
  WindowContext *window_context,
  FileSystemContext *file_system_context,
  const SettingsConfig &settings_config,
  const std::shared_ptr<Logger> &logger)
  : RenderSystem(window_context, file_system_context, settings_config, kMax_Render_Objects, logger),
    _model_refs(kMax_Render_Objects),
    _material_refs(kMax_Render_Objects) {}
```

### Order inside a class

| Order | What |
|---|---|
| 1 | Private members and private methods, without writing `private:` |
| 2 | `protected:` |
| 3 | `public:` |

This differs from Google, which starts with `public:`. `private:` is written
only when something private has to follow the public part.

### Helpers of a file go in an anonymous namespace, with a comment

A function, a constant, or a type that only one `.cpp` uses goes in an
anonymous namespace at the top of that file, as Google does, so that it has
internal linkage and stays out of the header. The namespace opens with a
comment that says what it holds and for whom, since a bare `namespace {`
tells a reader nothing:

```cpp
namespace neon
{
  // Helpers of SettingsFile: one reader per part of the file.
  namespace
  {
    ...
  }
```

A helper that needs the object is a private member. Decided 2026-10-02,
after a day under the opposite rule: moving every helper into its class made
the headers longer without making anything easier to find.

## Language use

| Rule | Example | Google |
|---|---|---|
| C++20 | | Same |
| `[[nodiscard]]` on functions that only return a value | `[[nodiscard]] bool IsRunning() const` | Not required |
| `override` without `virtual` | `void Initialize() override;` | Same |
| `final` on a class nothing derives from | `class CommandLine final` | Allowed |
| `explicit` on constructors, also those with several parameters | `explicit FileSystem(const SettingsConfig &, const std::shared_ptr<Logger> &)` | Same |
| `const` on a parameter passed by value, in the definition. The declaration leaves it out | `SetWindowFocus(bool focus)` declared, `SetWindowFocus(const bool focus)` defined | Differs, discouraged |
| `const` on locals that do not change | `const auto current_frame = ...` | Encouraged |
| `auto` where the type is on the same line or is long | `const auto bear = new RenderNode(...)` | Same |
| `if` with an initialiser when the variable is only needed there | `if (std::string error; !Apply(..., error))` | |
| Designated initialisers for the engine's own structs | `SettingsConfig{.width = 1920, .height = 1080}` | Same |
| `enum class`, never a plain `enum` | | Same |
| `static_cast` and its relatives, never a C cast | | Same |
| `nullptr`, never `NULL` or `0` | | Same |
| A parameter that is not used has no name | `void SetWindowFocus(bool) {}` | Same |
| Default values of members are written where the member is declared | `bool _should_close = false;` | Same |
| A function without a class in a `.cpp` file goes into a namespace without a name, inside `neon` | | Same |

### Interfaces

An interface is a class with pure virtual functions and a destructor that is
protected and not virtual. Nothing can be deleted through an interface. The
object is owned, and destroyed, by whoever created it with its real type.

```cpp
class FileSystemContext
{
protected:
  ~FileSystemContext() = default;

public:
  virtual bool Exists(const std::string &path) = 0;
};
```

Google asks for a public virtual destructor instead.

### Passing dependencies

| What | Passed as |
|---|---|
| A system or a context | Pointer, through the constructor: `WindowContext *window_context` |
| A logger | `const std::shared_ptr<Logger> &logger` |
| Settings | `const SettingsConfig &settings_config`, and the class keeps a copy |

### Errors

**While a game runs, the engine never throws.** A problem is a result — a
`bool`, a struct, a list of messages — that the caller handles where it
happens, and the game goes on: a scene with a mistake is loaded without what
was wrong, a user interface that cannot be made leaves its entity showing
nothing, an asset that cannot be read is not drawn. Every such problem is
logged as an error, and the runtime turns the count of errors into its exit
code, so that a script or a test learns that something went wrong even though
the run went on (`LoggingSystem::CountErrors`, `Runtime::HasFailed`). A file
never ends a game (#179).

| Situation | What the code does | Google |
|---|---|---|
| Something that can fail while the game runs: reading a file, a recipe with a mistake, a user interface that cannot be made | Logs the reason as an error, returns `false` or a result, and goes on with what it has | Same in spirit |
| A system cannot start, in `Initialize()`: no window, no graphics device, no folder for the executable | Logs with `Critical`, then throws `std::runtime_error`, which `main.cpp` catches. Whether these become results too is open (#179) | Differs, no exceptions |
| A programming mistake, such as an id that holds nothing or a component that was never registered | Throws, `std::out_of_range` in `DataBuffer`, `std::runtime_error` in the store. To be decided with scripting, which makes these reachable at run time (#179) | Differs |

`main.cpp` is the only place that catches.

### Logging

Every class that logs holds a `std::shared_ptr<Logger>`. The message is a
format string in the style of `std::format`. It starts with a capital letter
and has no full stop at the end.

```cpp
_logger->Info("SDL version: {}.{}.{}", major, minor, patch);
```

Nothing but `main.cpp` writes to `std::cout` or `std::cerr`, and it does so
only before the logger exists.

## Includes

| Rule | Example |
|---|---|
| A `.cpp` file includes its own header first | `#include "sdl2-window-system.hpp"` |
| A header of the engine in another folder or library: angle brackets and the full path from `neon/` | `#include <neon/logging/logger.hpp>` |
| A header in the same folder: quotes and the file name | `#include "vk-device.hpp"` |
| A header of the standard library or of a third-party library: angle brackets | `#include <vector>`, `#include <glm/glm.hpp>` |
| Standard library headers come before the headers of the engine | |

How often each form is used for the engine's own headers:

| Library | `<neon/...>` | `"neon/..."` | `"file.hpp"` |
|---|---|---|---|
| neon-core | 19 | 28 | 54 |
| neon-sdl2 | 9 | 0 | 3 |
| neon-vulkan | 12 | 0 | 18 |
| NeonRuntime | 9 | 2 | 2 |
| All | 49 | 30 | 77 |

27 of the 77 are a `.cpp` file including its own header. Google asks for the
path from the root of the project in quotes, and for a fixed order of groups.

## Comments

| Rule | Example |
|---|---|
| `///` in front of a class, function, or member that needs explaining. Plain, short sentences that end with a full stop | `/// Whether a file can be opened for reading.` |
| The first line says what it is or does. Details follow after an empty `///` line | |
| Names of code inside a comment go in backticks, a function gets `()` | `` /// Native folder behind `assets://` `` |
| `//` inside a function says why, not what. It starts with a lower-case letter and has no full stop | `// the renderer presents its own frames` |
| No `/* */` comments, no `@param` or `\brief` tags | |
| A TODO names the issue | `// TODO [issues/4] create a NodeFactory class` |
| A warning of the editor is switched off for one line, with the name of the inspection | `// ReSharper disable once CppInconsistentNaming` |

Google uses `//` for everything, and writes TODOs as `TODO(name)`.

## Architecture rules

These are rules of design, but they decide what a file may contain, so they
are checked in review like the rest.

| Rule | Detail |
|---|---|
| A third-party library is used in one backend library | SDL2 in neon-sdl2. Vulkan, volk, and stb in neon-vulkan |
| neon-core and the applications use interfaces | They name `WindowContext`, not `SDL2_WindowSystem`. `main.cpp` is the exception, it creates the backends and hands them on |
| No type of a backend appears in an interface | The Vulkan surface is passed as `void *` for that reason |
| No file access outside a backend | No `std::filesystem`, `fopen`, or `std::ifstream` in neon-core or an application. See the [development guide](development.md#file-system-and-resource-paths) |
| Files are named by virtual paths with forward slashes | `assets://models/cube.obj` |
| Everything works the same on macOS, Linux, and Windows | A check for the platform is only found inside a backend |
| Each `CMakeLists.txt` lists its files by hand | Headers and sources, grouped by folder. No `file(GLOB)` for sources |

Where neon-core does not follow the first rule yet:

| Library | Used in neon-core by |
|---|---|
| spdlog | `logging/spd-logger.hpp`, `logging/logging-system.hpp` and their sources |
| assimp | `render/model.hpp`, `render/model.cpp` |
| rapidyaml | `world-system/world-system.hpp` |
| glm | The math types of the interfaces |

## Project structure

The repository follows much of the canonical project structure of
[P1204R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html).

| P1204R0 | This repository | Follows |
|---|---|---|
| The source folder is named after the project and sits inside it | `lib/neon-core/neon/` | Yes, with one name for all libraries, see below |
| Headers and sources sit next to each other, no `include/` and `src/` | `file-system.hpp` next to `file-system.cpp` | Yes |
| `.hpp` and `.cpp` | | Yes |
| Includes name the project | `<neon/window/window-system.hpp>` | Partly, see [includes](#includes) |
| Includes use angle brackets | | Partly |
| The namespace is named after the project | `neon` | Yes |
| Folders inside the source folder order the code into components, and need not be namespaces | `neon/render/`, `neon/window/`, all in `neon::` | Yes |
| File names use letters, digits, and one kind of separator | `-`, as in `file-system.hpp` | Yes |
| A unit test sits next to what it tests, as `name.test.cpp` | `command-line.test.cpp` next to `command-line.cpp` | Yes, see below |
| Tests of the whole go into `tests/` | None exist | |
| A library project is named `lib<name>`, and its namespace is the name without `lib` | `neon-core`, with the namespace `neon` | No |
| A library and an executable are separate projects, each with a root of its own | Three libraries under `lib/` and the applications under `app/` share one repository and one build | No, by choice |
| The sources of a project sit in a folder named after it | `app/NeonRuntime/main.cpp`, with no folder in between | No, for applications |

All three libraries use `neon/` as their source folder. A header is included
the same way whichever library it is in, and a class can move between
libraries without its users changing. The price is that two libraries must
not have a file of the same path.

Third-party code lives in `external/` as git submodules. No tool and no rule
of this guide applies to it.

The assets of a project, such as `app/NeonRuntime/assets`, follow a guide of
their own: [project-layout.md](project-layout.md).

### One type per file

Every class and struct at namespace scope has a file of its own, named after
it: `VK_Canvas` is in `vk-canvas.hpp`, and `SoundFade` in `sound-fade.hpp`.
A simple struct, one that holds data and at most a few inline helpers, is a
`.hpp` alone, without a `.cpp`. More files, and each one about one thing,
which is found by its name.

Two kinds of type stay where they are:

- A type nested inside a class as a detail that only the class uses, such
  as the `State` a backend keeps behind a pointer, or a private `Entry` of
  a map. It is part of the class.
- An enum that only one type reads, which goes next to that type. One that
  several types share gets a file of its own.

The headers that still hold several types are listed in #168, and are split
one area at a time.

### Unit tests

A unit test is a file named `name.test.cpp` next to the file it tests. The
test framework is being added at the time of writing. A test file follows
this guide like any other file.

### CMake

| Rule | Example |
|---|---|
| A library or application sets `TARGET` first, and uses `${TARGET}` after that | `set(TARGET neon-sdl2)` |
| Two spaces inside `if` and `foreach` | |
| Arguments that continue on the next line are indented by eight spaces | |
| Commands are lower-case, variables are upper-case | `set(APP_BIN_DIRECTORY ...)` |
| A setting that exists for one platform or one library carries a comment that says why | |
| Functions of the build live in `cmake/*.cmake`, scripts run with `-P` in `cmake/scripts/` | |
| Every library and application calls `neon_warnings(${TARGET})` right after it is declared. The test functions do it themselves | See [compiler warnings](#compiler-warnings) |
| A header of a library in `external/` that the engine includes is marked `SYSTEM`, so that the warnings stop at it | `target_include_directories(stb SYSTEM INTERFACE ...)` |

## Git

| Rule | Example |
|---|---|
| The subject says what the commit does, in the imperative | `Add the options every runtime understands` |
| It is a plain sentence without a full stop, a prefix, or a tag | Not `feat: options` |
| One commit for each feature or fix | |
| Work happens on a branch named `feature/<name>` | `feature/command-line` |
| A pull request is reviewed and merged by the owner | |

## Tooling

### Compiler warnings

Every target of the engine is compiled with `-Wall -Wextra -Werror`: the
libraries under `lib/`, the applications under `app/`, and every test. A
warning in them fails the build. The libraries under `external/` are built as
they come, their warnings are not ours to fix, and a header of theirs that the
engine includes is marked as a system header, so that the warnings stop at it.
The flags are set by `neon_warnings(<target>)` in
[cmake/Warnings.cmake](../cmake/Warnings.cmake), which every library and
application calls, and which the test functions call for every test.

| Switched off | Reason |
|---|---|
| `-Wunused-parameter` | An interface gives its methods a default body that does nothing, and the names of the parameters there document the method. clang-tidy's `readability-named-parameter` is off for the same reason |
| `-Wmissing-designated-field-initializers` | A designated initializer leaves out the fields that keep their default on purpose |

A warning is fixed where it points, and not switched off. An argument a
backend has no use for has no name, or is marked `[[maybe_unused]]` when its
name says something. A narrowing that is meant is written as a
`static_cast`, so that it reads as meant.

### clang-tidy and clang-format

Both tools come with LLVM 20, the compiler of the project. On macOS they are
found in the folder of Homebrew's LLVM, elsewhere on `PATH`. A target says so
and fails when its tool is missing.

```bash
cmake --preset macos-arm64-debug
cmake --build build/macos-arm64-debug --target tidy
cmake --build build/macos-arm64-debug --target format-check
```

| Target | Does |
|---|---|
| `tidy` | Runs clang-tidy over every `.cpp` file of the engine and its applications. Headers are checked through the files that include them |
| `format-check` | Lists what clang-format would change. It changes nothing |

Neither runs in a normal build. The targets are defined in
[cmake/ClangTools.cmake](../cmake/ClangTools.cmake).

| Option | Default | Effect |
|---|---|---|
| `-DNEON_CLANG_TIDY=ON` | Off | Runs clang-tidy on the engine's own files every time they are compiled. Builds take several times as long |
| `-DNEON_TIDY_WARNINGS_AS_ERRORS=ON` | Off | The `tidy` target fails on any finding |

One file is checked with
`cmake --build build/<preset> --target tidy-lib-neon-core-neon-runtime-runtime-cpp`,
the path with every character that is not a letter or digit replaced by `-`.

CLion and VS Code with clangd read both files by themselves and show the
same findings while typing.

### What clang-tidy checks

| Group | Checks |
|---|---|
| `readability-identifier-naming` | The [naming](#naming) table, except files, folders, and header guards |
| `bugprone-*` | Mistakes such as a value used after a move, or an unchecked `std::optional` |
| `performance-*` | Copies and allocations that are not needed |
| `modernize-*` | `override`, `nullptr`, `[[nodiscard]]`, and other newer forms |
| `readability-*` | Braces around every body, no `else` after `return`, and similar |
| `cppcoreguidelines-*` | Variables and members without a value |
| `misc-*`, `google-*` | What is left of them after the list below |

| Switched off | Reason |
|---|---|
| `modernize-use-trailing-return-type` | Return types are written in front |
| `readability-uppercase-literal-suffix` | Literals are written `1.0f` |
| `google-readability-namespace-comments` | A namespace closes with `} // neon` |
| `google-readability-todo` | A TODO names an issue, not a person |
| `google-runtime-int` | `int`, `long`, and `unsigned int` are used as they are |
| `readability-named-parameter` | A parameter that is not used has no name |
| `readability-qualified-auto` | Pointers are held in a plain `const auto` |
| `*-non-private-member-variables-in-classes` | Base classes share protected members such as `_logger` |
| `cppcoreguidelines-prefer-member-initializer` | Most constructors assign in their body, see open points |
| `cppcoreguidelines-special-member-functions` | Interfaces declare a protected destructor and nothing else |
| `cppcoreguidelines-pro-bounds-*`, `-pro-type-union-access`, `-pro-type-reinterpret-cast`, `-pro-type-vararg`, `-owning-memory`, `-no-malloc` | They fire on every call into SDL2, Vulkan, assimp, and glm |
| `*-avoid-c-arrays` | `argv`, and arrays whose layout a shader shares |
| `modernize-use-designated-initializers` | Asks for the field names of every Vulkan structure |
| `readability-implicit-bool-conversion` | Flags and results of C functions are tested as they are |
| `misc-misplaced-const` | Vulkan handles are pointers behind a type name |
| `misc-include-cleaner` | 656 findings. Wants a header for every name, also those that arrive through `<glm/glm.hpp>` or `<SDL.h>` |
| `*-magic-numbers`, `readability-identifier-length`, `bugprone-easily-swappable-parameters`, `performance-enum-size` | More findings than a reviewer would ask to change |

Findings are limited to `lib/neon-*` and `app/`. Nothing under `external/` or
a build folder is reported.

### What clang-format does

It reproduces the [layout](#layout). It keeps the line breaks it finds and
does not enforce the line length, because it would otherwise join and split
lines by rules that are not those of the code. With a limit of 120 it would
change 640 lines, without one it changes 196.

It does not sort includes and does not write the comment that closes a
namespace.

## Open points

### Where the code disagrees with itself

The guide and the tools follow the column "Used most". The rest is for the
owner to decide.

| Topic | Used most | Also found |
|---|---|---|
| Constants | `snake_case`: `assets_scheme`, `user_scheme`, `frame_time`, `forbidden_characters`, `help_option`, and the six names in `runtime-options.cpp` | `k` and words with underscores, 6 times: `kAction_Size`, `kAxis_Size`, `kMax_Point_Lights`, `kMax_Spot_Lights`, `kMax_Render_Objects`, `kMax_Scenes_Per_Frame`. The settings of CLion in `.idea/codeStyles` ask for this form. clang-tidy reports the six |
| Methods | `PascalCase` | `World_Forward()` and `World_Up()` in `transform.hpp`. The settings of CLion allow it, clang-tidy accepts it |
| Enumerators | `PascalCase` | `Ui_Up` and six like it, and `COUNT` twice, in `input-state.hpp`. clang-tidy accepts both |
| Prefix of a class | For a backend: `SDL2_`, `VK_` | For one of several implementations inside neon-core: `Headless_`, `Forward_`, `Spd_`. Is the underscore meant for both? |
| Own headers in another folder | `<neon/...>`, 49 times | `"neon/..."`, 30 times, 28 of them in neon-core |
| `*` and `&` | At the name, 521 times | At the type, 26 times: `WindowContext* _context`, `const std::string& format` |
| Braces | On a line of their own, 647 times | At the end of the line, 9 times: `sdl2-window-system.hpp`, `sdl2-input-system.hpp`, `camera-node.hpp`, `light-node.hpp`, `neon-runtime.hpp`, `util.hpp`, and a loop in `model.cpp` |
| Contents of a namespace | Indented | Not indented in `camera-node.hpp` and `light-node.hpp` |
| Closing of a namespace | `} // neon`, 75 times | A bare `}`, 9 times |
| Header guard | The file name | 9 that do not match it. 3 are `SDL_2_...` for `sdl2-...`. 6 carry the name the file had before: `WINDOW_INFO_HPP` in `settings-config.hpp`, `LIGHT_HPP`, `TEXTURE_HPP`, `SCENE_HPP`, `PLAYER_NODE_HPP`, `SDL_2_WINDOW_CONTEXT_HPP` in `window-context.hpp` |
| `#endif` | `#endif //NAME`, 58 times | Bare, 3 times. With a space, once |
| Members set by a constructor | Assigned in the body | An initialiser list, in the newer classes. Google and clang-tidy prefer the list |
| `[[nodiscard]]` on a getter | With, 19 | Without, 15 |
| `//` comments | Lower-case first letter, 106 | Upper-case, 27, mostly where the comment is several sentences |
| Float literals | `1.0f`, 166 | `1.f`, 13. `.5f`, 2 |
| Namespace | Everything in `neon` | `SettingsConfig`, `RenderingApi`, and `WindowMode` are in none. `NodeFactory` is in `node`. `NeonRuntime` is in none, which may be meant |

### What no tool checks

| Rule | Why not |
|---|---|
| File and folder names | clang-tidy does not look at them |
| One type per file | Nothing counts the types of a header. A script over the headers could, see #168 |
| Header guards | The check that exists wants the path in the guard. Guards without a prefix, such as `LOGGER_HPP` and `MESH_HPP`, can also collide with those of a library. `NEON_` in front, or `#pragma once`, would end that |
| The kind of include, and the order | clang-format can sort, but not by these rules |
| Line length | See [what clang-format does](#what-clang-format-does) |
| `const` on parameters passed by value | No check for definitions only |
| Comments | Form and language are a matter of review |
| Third-party libraries only in backends | It depends on the folder. A script that searches neon-core for includes could do it |
| No file access outside a backend | The same |
| Files listed by hand in CMake | neon-core lists the logging files three times and `mesh.hpp` twice, which CMake accepts |
| Commit messages | |

### What clang-format would still change

196 of 7057 lines, in 34 of 87 files.

| Cause | Lines, about |
|---|---|
| The files under "Braces" and "Contents of a namespace" above | 45 |
| `*` and `&` at the type | 30 |
| Line breaks that clang-format places differently: the brace that closes a designated initialiser, as in `scene-manager.cpp` and `runtime-options.cpp`, `{}` after the initialisers of a constructor, a matrix written as four rows, conditions and `<<` that continue on the next line | 120 |

The first two are inconsistencies the owner may want fixed. The others are
places where clang-format cannot be taught the form the code uses. Running
it over the code is therefore a decision for later, best made when no other
branch is open.

### Findings of clang-tidy

43 remain with the checks above. None was changed in the code.

| Check | Count | Example |
|---|---|---|
| `cppcoreguidelines-init-variables` | 10 | `main.cpp:73`: `window_system` has no value until the `if` below it. Also `int width, height;` before a call that fills them |
| `readability-identifier-naming` | 6 | The six constants with `k`, see above |
| `cppcoreguidelines-pro-type-member-init` | 4 | `settings-config.hpp:25`: `width`, `height`, and `selected_api` have no default value. Also `LightSource`, `RenderInfo`, `TextureInfo` |
| `readability-isolate-declaration` | 3 | `sdl2-window-system.cpp:162`: `int width, height;` |
| `readability-redundant-member-init` | 3 | `vk-model.hpp:14`: `std::vector<VK_Mesh> _meshes{};` |
| `performance-inefficient-vector-operation` | 2 | `vk-device.cpp:94`: `push_back` in a loop without `reserve` |
| `cppcoreguidelines-avoid-const-or-ref-data-members` | 2 | `render-context.hpp:22`: `const int width` makes `RenderResolution` impossible to assign |
| `bugprone-exception-escape` | 2 | `main.cpp:23`: what is created before the `try` can throw out of `main` |
| `bugprone-unchecked-optional-access` | 1 | `vk-render-system.cpp:849`: `GetRenderResolution()` reads `_render_resolution`, which is empty before `Initialize()` |
| `bugprone-suspicious-memory-comparison` | 1 | `vk-render-system.cpp:1004`: `memcmp` on `VK_SceneData`. Floats that are equal can differ in their bytes, the cost is a scene stored once more than needed |
| `bugprone-implicit-widening-of-multiplication-result` | 1 | `settings-config.hpp:40`: `1048576 * 5` is calculated as `int` |
| `bugprone-multi-level-implicit-pointer-conversion` | 1 | `vk-device.cpp:77`: `&_surface` passed as `void *` |
| `modernize-use-nodiscard` | 1 | `vk-device.hpp:31`: `LoadLibrary()` |
| `modernize-return-braced-init-list` | 1 | `model.cpp:223` |
| `modernize-min-max-use-initializer-list` | 1 | `model.cpp:227`: `std::max` inside `std::max` |
| `misc-const-correctness` | 1 | `vk-model.cpp:55` |
| `misc-unused-parameters` | 1 | `node.cpp:67`: `delta_time` in `Node::Update` |
| `readability-use-anyofallof` | 1 | `vk-device.cpp:43` |
| `performance-inefficient-string-concatenation` | 1 | `command-line.cpp:137` |

None of them is a bug that shows today. Two are worth a look first:
`GetRenderResolution()` before `Initialize()`, and the settings without
default values.

One thing clang-tidy did not report: `runtime.cpp` ends the line
`_world_system = world_system,` with a comma instead of a semicolon. The
result is the same, since the next line is an assignment too.
