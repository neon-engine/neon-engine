# Third-party licenses

This note lists every third-party library in the repository, the license it
is under, and whether that license allows the library to be linked statically
into a program that is distributed.

It was written by reading the license files in `external/`. It is not legal
advice. Have it checked before a game is sold.

**In short:** everything NeonRuntime links today may be linked statically,
into an open or a closed program. The one thing to stay away from is two
decoders that come with SDL_mixer, mpg123 and game-music-emu. They are under
the LGPL and are switched off by default.

## What the licenses ask for

| License | Static linking | What has to be done when a program is distributed |
|---|---|---|
| MIT | Allowed | Include the copyright notice and the license text |
| BSD, 2 and 3 clauses | Allowed | Include the copyright notice and the license text. The name of the project is not used to promote the program |
| zlib | Allowed | Nothing in a binary. Changed source has to be marked as changed |
| Boost 1.0 | Allowed | Nothing in a binary. The license text stays with the source |
| Apache 2.0 | Allowed | Include the license text, and the `NOTICE` file when there is one. It grants the use of patents, and ends that grant for whoever sues over them |
| Apache 2.0 with LLVM exception | Allowed | Nothing for what the compiler puts into a program |
| Unlicense, public domain | Allowed | Nothing |
| LGPL 2.1 | **Only with conditions** | Whoever receives the program has to be able to replace the library. With static linking that means handing out the object files of the program, or its source. Linking dynamically avoids it |
| Creative Commons BY 4.0 | Does not apply, it covers assets | Name the author, link the license, and say what was changed |

"Include" means that the text ships with the game, in a file next to it or on
a screen in it.

## Linked into NeonRuntime

| Library | Version | License | Static linking | Used for |
|---|---|---|---|---|
| [SDL2](https://github.com/libsdl-org/SDL) | 2.30.9 | zlib | Allowed | Window, input, files |
| [GLM](https://github.com/g-truc/glm) | 0af55cc, after 1.0.1 | MIT, or the Happy Bunny License | Allowed. Header only | Vectors and matrices |
| [spdlog](https://github.com/gabime/spdlog) | 6fa3601, after 1.15.3 | MIT | Allowed | Logging |
| [assimp](https://github.com/assimp/assimp) | ddb74c2, after 5.4.2 | BSD 3 clauses | Allowed | Loading models |
| [stb](https://github.com/nothings/stb) | 013ac3b | MIT, or public domain | Allowed. Header only | Reading and writing images |
| [rapidyaml](https://github.com/biojppm/rapidyaml) | 0.8.0 | MIT | Allowed | Reading YAML |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | 1.4.357 | Apache 2.0, or MIT | Allowed. Header only | Declarations of Vulkan |
| [volk](https://github.com/zeux/volk) | 1.4.357 | MIT | Allowed | Finding the Vulkan library when the program starts |
| [LuaJIT](https://github.com/LuaJIT/LuaJIT) | c6ffc14, v2.1 of 2026-09-08 | MIT | Allowed | The scripts |

For GLM and stb the MIT license is the one to pick. The Happy Bunny License
asks for something that cannot be checked, and public domain is not recognized
the same way everywhere.

### What these libraries bring along

A library often holds code of others. This is what ends up in the program.

| Inside | Code | License | Static linking |
|---|---|---|---|
| SDL2 | hidapi | One of three, to be chosen: GPL 3, BSD, or the original hidapi license | Allowed, when BSD or the original license is chosen |
| spdlog | fmt | MIT | Allowed |
| rapidyaml | c4core | MIT | Allowed |
| rapidyaml | fast_float | One of three: MIT, Apache 2.0, or Boost 1.0 | Allowed |
| rapidyaml | debugbreak | BSD 2 clauses | Allowed |
| assimp | zlib | zlib | Allowed |
| assimp | unzip (minizip) | zlib | Allowed |
| assimp | zip (kuba--/zip, miniz) | Unlicense | Allowed |
| assimp | clipper | Boost 1.0 | Allowed |
| assimp | utf8cpp | Boost 1.0 | Allowed |
| assimp | poly2tri | BSD 3 clauses | Allowed |
| assimp | Open3DGC | MIT | Allowed |
| assimp | openddlparser | MIT | Allowed |
| assimp | pugixml | MIT | Allowed |
| assimp | rapidjson | MIT | Allowed |
| assimp | stb | MIT, or public domain | Allowed |
| assimp | Draco | Apache 2.0 | Allowed. Not built, it is switched off |

assimp is temporary, see the [roadmap](roadmap.md). Most of this table leaves
with it.

## Arriving with open pull requests

| Library | Version | License | Static linking | Used for |
|---|---|---|---|---|
| [Flecs](https://github.com/SanderMertens/flecs) | 4.1.6 | MIT | Allowed | Entities and components |
| [GoogleTest](https://github.com/google/googletest) | 1.18.0 | BSD 3 clauses | Allowed | Tests. It is linked into the test programs only, and is not part of a game |

## In the repository, and not linked yet

| Library | Version | License | Static linking | Meant for |
|---|---|---|---|---|
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | 5.2.0 | MIT | Allowed | Physics. Built, and linked by `app/JoltTest` alone |
| [SDL_mixer](https://github.com/libsdl-org/SDL_mixer) | 2.8.0 | zlib | Allowed, with the decoders it uses by default. See below | Sound and music |
| [SoLoud](https://github.com/jarikomppa/soloud) | 20200207 | zlib | Allowed | Sound and music |
| [Steam Audio](https://github.com/ValveSoftware/steam-audio) | 4.6.0 | Apache 2.0 | Allowed. See below | Spatial audio |

### SDL_mixer

SDL_mixer itself is under the zlib license. What decides is which decoders are
switched on when it is built.

| Decoder | For | License | Default | Static linking |
|---|---|---|---|---|
| minimp3 | MP3 | Public domain (CC0) | On | Allowed |
| dr_flac | FLAC | Public domain, or MIT No Attribution | On | Allowed |
| stb_vorbis | Ogg Vorbis | MIT, or public domain | On | Allowed |
| libxmp | MOD and other tracker music | MIT | On | Allowed |
| Opus, opusfile, Ogg | Opus | BSD 3 clauses | On | Allowed |
| WavPack | WavPack | BSD 3 clauses | On | Allowed |
| Timidity | MIDI | One of three, to be chosen: GPL, LGPL, or the Artistic License | On | Allowed, when the Artistic License is chosen |
| libFLAC | FLAC | BSD 3 clauses | Off | Allowed |
| Vorbis, Tremor | Ogg Vorbis | BSD 3 clauses | Off | Allowed |
| **mpg123** | MP3 | **LGPL 2.1** | Off | **Only with conditions** |
| **game-music-emu** | Music of old consoles | **LGPL 2.1** | Off | **Only with conditions** |

Leave mpg123 and game-music-emu off. minimp3 already plays MP3. If one of them
is ever wanted, it has to be a shared library that ships next to the game.

### Steam Audio

Steam Audio is under Apache 2.0 since version 4.5.2. It depends on libraries
of its own, which its `core/THIRDPARTY.md` lists.

| Code | License | Static linking |
|---|---|---|
| PFFFT | BSD-like, from FFTPACK | Allowed |
| MySOFA | BSD 3 clauses | Allowed |
| FFTS | BSD 3 clauses | Allowed |
| Google Spherical Harmonics Library | Apache 2.0 | Allowed |
| CIPIC HRTF Database | Its own terms, of the University of California | It is data, not code. Its terms are in `THIRDPARTY.md` and were not judged here |
| Intel Embree, optional | Apache 2.0 | Allowed |
| AMD RadeonRays, optional | MIT | Allowed |
| AMD TrueAudio Next, optional | MIT | Allowed |
| **Intel IPP, optional** | Intel Simplified Software License | Not open source. It may be redistributed under the terms of Intel. Leave it off unless those terms were read |

Valve distributes Steam Audio as a shared library, and that is how it is
usually used. `TRADEMARK_RIGHTS.md` in its folder says how its name and logo
may be used.

## Not in the repository, and part of what is distributed

| What | License | How it is linked | Note |
|---|---|---|---|
| libc++, libc++abi, libunwind, compiler-rt | Apache 2.0 with LLVM exception | Statically, on Linux and Windows | The exception is there so that nothing is asked of a program that holds them |
| mingw-w64 runtime and winpthreads | Zope Public License 2.1, public domain, MIT, and BSD, by file | Statically, on Windows | All of them allow it. Their notices belong into what is distributed |
| glibc | LGPL 2.1 | Dynamically, on Linux | Keep it dynamic. It is part of the system and is not shipped |
| Vulkan loader | Apache 2.0 | Loaded when the program starts | Part of the system or the graphics driver on Linux and Windows |
| [MoltenVK](https://github.com/KhronosGroup/MoltenVK) | Apache 2.0 | Loaded when the program starts | macOS has no Vulkan of its own. A game for macOS has to ship it |

## Assets

| Asset | License | What has to be done |
|---|---|---|
| Textures in `app/NeonRuntime/assets/textures`, by hansonry | Creative Commons BY 4.0 | Name the author and link the license, which [CREDITS.md](../app/NeonRuntime/assets/CREDITS.md) does |
| Models in `app/NeonRuntime/assets/models` | Not recorded | They are there to have something to draw while the engine is built, and will be removed. They are not meant to be distributed |

## What a distributed game needs

1. A file with the copyright notices and license texts of everything in
   "Linked into NeonRuntime" and what those bring along, except where the
   table above says nothing is asked.
2. The same for the libraries of the toolchain on Windows.
3. The credits of the assets that are used.
4. MoltenVK and its license, on macOS.

Collecting these by hand does not last. Exporting a game should write the
file, which makes it a task for NeonEditor.

## Open points

- The models in `assets/models` have no recorded source. They are
  placeholders and will be removed. Until then, a game must not ship them.
- GLM, spdlog, assimp, and stb are pinned to commits between releases. A
  release is easier to name in a list of licenses.
- This list is kept by hand. A check that fails when `external/` holds a
  library that is not listed here would keep it true.
- neon-core is under the MIT license, so a game that is built with the engine
  has to include the notice of the engine too.
