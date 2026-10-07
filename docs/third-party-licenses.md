# Third-party licenses

This note says, for every third-party library in the repository, whether its
license allows the library to be linked statically into a program that is
distributed, and what a game that is handed on has to carry. The list itself,
with versions and where each license text is, is [CREDITS.md](../CREDITS.md)
at the root of the repository; the two are kept in step.

It was written by reading the license files in `external/`. It is not legal
advice. Have it checked before a game is sold.

**In short:** everything NeonRuntime links today may be linked statically,
into an open or a closed program. Most of it asks that its copyright notice
and license text go with the program, and FreeType asks to be named in the
credits.

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
| MIT No Attribution | Allowed | Nothing |
| "Old MIT", of HarfBuzz | Allowed | Include the copyright notice and the license text |
| The FreeType License | Allowed | Include the license text, and say in the credits that the program uses FreeType |
| SIL Open Font License 1.1 | Does not apply, it covers fonts | The copyright notice and the license go with the font, also when it is embedded. A font is not sold by itself. A font that is changed takes another name where one is reserved |

"Include" means that the text ships with the game, in a file next to it or on
a screen in it.

## Linked into NeonRuntime

| Library | Version | License | Static linking | Used for |
|---|---|---|---|---|
| [SDL2](https://github.com/libsdl-org/SDL) | 2.30.9 | zlib | Allowed | Window, input, files |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | MIT, or the Happy Bunny License | Allowed. Header only | Vectors and matrices |
| [spdlog](https://github.com/gabime/spdlog) | 1.15.3 | MIT | Allowed | Logging |
| [assimp](https://github.com/assimp/assimp) | 5.4.2 | BSD 3 clauses | Allowed | Loading models |
| [stb](https://github.com/nothings/stb) | 013ac3b | MIT, or public domain | Allowed. Header only | Reading and writing images |
| [rapidyaml](https://github.com/biojppm/rapidyaml) | 0.8.0 | MIT | Allowed | Reading YAML |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | 1.4.357 | Apache 2.0, or MIT | Allowed. Header only | Declarations of Vulkan |
| [volk](https://github.com/zeux/volk) | 1.4.357 | MIT | Allowed | Finding the Vulkan library when the program starts |
| [LuaJIT](https://github.com/LuaJIT/LuaJIT) | c6ffc14, v2.1 of 2026-09-08 | MIT | Allowed | The scripts |
| [Flecs](https://github.com/SanderMertens/flecs) | 4.1.6 | MIT | Allowed | Entities and components |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | 5.2.0 | MIT | Allowed | Physics |
| [miniaudio](https://github.com/mackron/miniaudio) | 0.11.25 | Public domain, or MIT No Attribution | Allowed | Sound |
| [FreeType](https://github.com/freetype/freetype) | 2.14.3 | The FreeType License, or GPL 2 | Allowed, under the FreeType License | Drawing the glyphs of fonts |
| [HarfBuzz](https://github.com/harfbuzz/harfbuzz) | 14.5.0 | "Old MIT" | Allowed | Shaping text |
| [LunaSVG](https://github.com/sammycage/lunasvg) | 3.5.0 | MIT | Allowed | Drawing SVG images |

For GLM, stb, and miniaudio the MIT license is the one to pick. The Happy
Bunny License asks for something that cannot be checked, and public domain is
not recognized the same way everywhere. FreeType is taken under its own
license, never under the GPL.

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
| LunaSVG | PlutoVG | MIT | Allowed |
| LunaSVG | A rasterizer of FreeType, inside PlutoVG | The FreeType License | Allowed |
| LunaSVG | stb, inside PlutoVG | MIT, or public domain | Allowed |

assimp is temporary, see the [roadmap](roadmap.md). Most of this table leaves
with it.

## In the repository, and not part of NeonRuntime

| Library | Version | License | Static linking | Meant for |
|---|---|---|---|---|
| [GoogleTest](https://github.com/google/googletest) | 1.18.0 | BSD 3 clauses | Allowed | Tests. It is linked into the test programs only, and is not part of a game |
| [Steam Audio](https://github.com/ValveSoftware/steam-audio) | 4.6.0 | Apache 2.0 | Allowed. See below | Spatial audio. Not built yet |

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
| The fonts in `app/NeonRuntime/engine/fonts`: Inter and Noto Sans Arabic | SIL Open Font License 1.1 | Keep the licence with the fonts, also when they are embedded in the engine. [CREDITS.md](../CREDITS.md) lists them |
| Everything else: the scenes, and the models, textures, and sounds of `projects/` and `tests/game` | The engine's own | Built by the engine as `Geometry`, or written by a script of `tools/` from numbers. No model, texture, or sound in the repository was taken from elsewhere |

## What a distributed game needs

1. A file with the copyright notices and license texts of everything in
   "Linked into NeonRuntime" and what those bring along, except where the
   table above says nothing is asked.
2. The same for the libraries of the toolchain on Windows.
3. The credits of the assets that are used.
4. MoltenVK and its license, on macOS.

Collecting these by hand does not last. Exporting a game should write the
file, which makes it a task for NeonEditor, and the engine has to carry the
notices of what is embedded in it, see #435.

## Open points

- stb and LuaJIT are pinned to commits, not to releases. A release is easier
  to name in a list of licenses.
- This list and [CREDITS.md](../CREDITS.md) are kept by hand. A check that
  fails when `external/` holds a library that is not listed would keep them
  true.
- The license of the engine itself is not decided (#95). Once it is, a game
  that is built with the engine has to include what that license asks too.
