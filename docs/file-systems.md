# File system considerations

This note records how Neon Engine locates files today, what a fuller file
system layer would need to do, and the options that were weighed. It exists so
the decision can be revisited later without repeating the research.

**Current decision:** the engine has its own file system abstraction, with one
implementation built on SDL2. Moving to a virtual file system such as PhysFS is
not scheduled. The abstraction exists so that such a move stays possible.

## What exists today

Resource paths carry a scheme prefix, in the style of Godot.

| Scheme | Points at | Status |
|---|---|---|
| `assets://` | `<directory of executable>/assets` | Implemented |
| `user://` | A folder of the current user, for saves, settings, and anything else the app writes | Implemented, read and write |

File access sits behind an interface, following the same split as windowing
and rendering.

| Piece | Location | Role |
|---|---|---|
| `FileSystemContext` | neon-core | The interface consumers depend on: `Exists`, `ReadBytes`, `ReadText`, `WriteBytes`, `WriteText` |
| `FileSystem` | neon-core | Base class for backends. Owns the lifecycle, the path rules, and the conversion to native paths |
| `SDL2_FileSystem` | neon-sdl2 | The implementation. Finds the executable's folder and reads files through SDL2 |

The model, shader, and texture loaders read file contents through the
interface. None of them opens a file itself. Usage rules are in the
[development guide](development.md#file-system-and-resource-paths).

Engine code does not use `std::filesystem` or operating system calls for file
access. Those details belong to the backend.

### The interface

| Function | Purpose |
|---|---|
| `Exists(path)` | Whether the file can be opened for reading |
| `ReadBytes(path, contents)` | Reads a whole file as raw bytes |
| `ReadText(path, contents)` | Reads a whole file as text |

It is intentionally small. It holds what the three loaders need and nothing
speculative. Functions are added when a feature requires them.

### Portable paths

Cross-compatibility is the philosophy of the project. Content that refers to
files, such as a scene file, has to work unchanged on macOS, Linux, and
Windows. The file system therefore enforces one form of path everywhere.

| Decision | Reason |
|---|---|
| Virtual paths use forward slashes on every platform | One spelling for all three. Windows accepts it too, but the engine does not rely on that |
| Backslashes are rejected, not converted | A scene file written with backslashes would otherwise spread and only be corrected silently. Rejecting keeps files in one canonical form |
| `..` is rejected | A path cannot leave the folder of its scheme, which matters once scene files and mods supply paths |
| Paths without a scheme are rejected | Absolute and working-directory-relative paths are tied to one machine |
| Characters and name endings that any platform forbids are rejected everywhere | A file that cannot exist on Windows should not be usable on Linux either |
| Letter case has to match the name on disk, on every platform | Windows and macOS ignore case and Linux does not, which is a classic source of bugs that only appear after moving to another platform |
| Rules are checked on every platform | A problem shows up on the author's machine, not later on someone else's |

The full rule table is in the
[development guide](development.md#path-rules).

Native paths are an implementation detail of a backend. The interface cannot
return one. `FileSystem::Locate` produces them and is protected. The SDL2
backend learns the platform's separator from the base path SDL2 reports, so it
contains no platform checks of its own.

Names reserved by Windows are not checked. See open questions.

### How letter case is enforced

The operating system cannot be asked whether a file exists, because on
Windows and macOS it answers yes for any casing. `Locate` walks the path one
name at a time. At each step it lists the parent folder through the backend's
`ListDirectory` hook and looks for an entry that is byte-for-byte equal.

| Outcome | Result |
|---|---|
| An entry matches exactly | Continue with the next name |
| An entry matches only when case is ignored | Error naming the correct spelling, file treated as missing |
| Nothing matches | File treated as missing, no error |

Trade-offs that were accepted:

- **Cost.** Opening a file lists one folder per name in its path. That is
  small next to reading and decoding an asset. Listings are not cached. A
  cache is the obvious step if profiling ever shows it, and it would need to
  be cleared when an editor changes files.
- **SDL2 cannot list folders.** That arrives with SDL3. The SDL2 backend uses
  `std::filesystem` for this one task. It is confined to the backend, so the
  rule that engine code stays off `std::filesystem` holds.
- **Case folding in the error message is ASCII only.** It decides how helpful
  the message is, not whether a path is accepted. Acceptance is always an
  exact comparison.

### How it is wired

`main.cpp` creates `SDL2_FileSystem` right after logging and passes it to the
render system as a `FileSystemContext`. The render system hands it to each
loader it constructs. It is cleaned up last, after the systems that depend on
it have shut down.

### History

The first version was a set of free functions in neon-core that used
`std::filesystem` and asked each operating system for the executable's
location. It was replaced by the abstraction so that the implementation can
change without touching callers.

### Why not `argv[0]`

This was considered for locating the executable. `argv[0]` holds whatever the
launcher typed, so it is only a usable path in some cases.

| How the app is started | `argv[0]` contains | Usable |
|---|---|---|
| By path, such as `./bin/NeonRuntime` | That relative path | Yes, if resolved against the working directory at once |
| From an IDE | Usually the absolute path | Yes |
| Found through `PATH` | The bare name, with no folder | No |
| Through a symlink | The symlink's location | No, wrong folder |
| By another program | Anything that program chose | Not guaranteed |

SDL2's base path call, which the current implementation uses, is correct in
every row.

## What the current implementation does not cover

| Need | Why it matters |
|---|---|
| Packed assets | Shipping thousands of loose files is slow to install and easy to tamper with. Engines normally ship a few archives |
| Layering | Mods, patches, and downloadable content work by mounting a second source over the first, so a newer file shadows an older one |
| Listing folders | There is no way to enumerate files, which an asset browser or a scan for scene files would need |
| Streaming | Files are read whole into memory. Large audio or video would need to be read in pieces |
| Asynchronous loading | Large assets should load without stalling the frame. This is a separate concern from where files live |

## Options

| Option | Provides | Notes |
|---|---|---|
| SDL2, through `SDL2_FileSystem` | The executable's folder, a per-user folder, and file reading | **In use.** Serves `assets://` from loose files. Its per-user path call can supply the location behind `user://` |
| [PhysFS](https://icculus.org/physfs/) | A virtual file system: mounted folders and archives in one tree, one write folder, lookups for the executable and per-user folders | The usual choice for game engines. zlib license, C API, builds with CMake |
| [whereami](https://github.com/gpakosz/whereami) | The executable's path only | Not needed. SDL2 already supplies the executable's folder |
| [platform_folders](https://github.com/sago007/PlatformFolders) | The per-user folders on each OS | Not needed while SDL2 is the backend, since SDL2 covers this |
| `std::filesystem` | Path handling and file operations | Deliberately not used by engine code, so the implementation stays replaceable. A backend could be built on it, but it knows nothing about executable or user folders |

## PhysFS in more detail

PhysFS is the closest match to the Godot model, so it is the main candidate if
loose files are ever outgrown.

**What it does**

- Mounts any number of folders and archives into a single virtual tree. Later
  mounts can shadow earlier ones.
- Names exactly one folder as the write target. All writes land there, which
  is the behaviour wanted from `user://`.
- Reports the executable's folder and a per-user, per-application folder.
- Reads zip and 7z, plus several classic game formats, without extra code.

**What it does not do**

It is a virtual file system and nothing more. Every read blocks until it
finishes. It has no asynchronous I/O, threading, or networking. An asynchronous
loader would be built separately and could sit on top of it.

**Custom archive formats**

PhysFS accepts user-defined formats. An archiver is a set of callbacks
registered once at startup, after which files in that format mount like any
other archive.

| Callback | Purpose |
|---|---|
| Open archive | Recognise the format and read its index |
| Enumerate | List the entries in a folder |
| Open for reading | Return a stream for one entry |
| Stat | Report size, type, and timestamps |
| Close archive | Release resources |
| Write, append, remove, make folder | Optional. A read-only format declines them |

Separately, a custom byte source can be supplied, or an archive mounted
straight from memory. That allows an archive to be embedded in another file,
fetched over a network, or decrypted while it is read.

A custom format is worth the effort mainly for encryption, a particular
compression scheme, or a layout tuned for fast loading. Zip covers the rest.

## Cost of adopting a virtual file system

The expensive part is already done. A file inside an archive has no path the
operating system can open, so loaders must read contents through the file
system instead of handing a path to a library. They do.

| Loader | Reads through |
|---|---|
| Models | An assimp I/O handler backed by `FileSystemContext` |
| Textures | `ReadBytes`, decoded by stb_image from memory |
| Shaders | `ReadText` |

Adopting PhysFS would therefore mean one new backend library implementing
`FileSystem`, and a change to which implementation `main.cpp` constructs. The
loaders would not change.

Nothing in the interface assumes files live on disk. An earlier version had a
`ResolvePath` function that returned a native path. It was removed, because a
backend serving archives could not have honoured it.

## How it fits the architecture

| Piece | Location | Role |
|---|---|---|
| File system interface | neon-core | Exists today |
| SDL2 implementation | neon-sdl2 | Exists today. Loose files on disk |
| PhysFS implementation | Its own backend library | Would hold mounts, archives, and any custom archiver |
| Composition | `main.cpp` | Constructs one implementation and injects it |

## When to revisit

Keep the SDL2 implementation until one of these becomes a real requirement.

- Assets are to be shipped packed.
- Mods or patches need layered mounts.
- Loading has to happen off the main thread.

The first two are the point at which PhysFS pays for itself.

## Open questions

- `user://` is per application, placed by the `organization` and
  `application` names in `SettingsConfig`. Is a second, shared scheme needed
  for settings that belong to the engine?
- Should the log file move under `user://`? It is currently relative to the
  working directory.
- Is a custom archive format needed at all, or is zip enough?
- Should editor builds read loose files while shipped builds read archives?
- Should the build reject an assets folder that holds two names differing
  only in case? Linux can store both, Windows and macOS cannot.
- Should folder listings be cached once asset counts grow?
- Should names reserved by Windows be rejected by the path rules?
