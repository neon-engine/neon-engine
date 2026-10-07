# Credits

Everything in this repository that was not made for Neon Engine: what it is,
where it came from, the licence it is under, and where the text of that
licence is. Whether a licence allows static linking, and what it asks of a
game that is distributed, is in
[docs/third-party-licenses.md](docs/third-party-licenses.md).

The libraries are Git submodules under `external/`, each at the version its
authors published, unchanged. Each keeps its own licence file, which is the
one named here.

## Libraries that are part of NeonRuntime

Built from source and linked into the runtime, so a build of the engine holds
their code.

| Library | Version | Used for | Licence | Text |
|---|---|---|---|---|
| [SDL2](https://github.com/libsdl-org/SDL) | 2.30.9 | Window, input, gamepads, files | zlib | `external/sdl2/LICENSE.txt` |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | 1.4.357 | The declarations of Vulkan | Apache 2.0 or MIT, MIT is taken | `external/vulkan-headers/LICENSE.md`, `external/vulkan-headers/LICENSES/` |
| [volk](https://github.com/zeux/volk) | 1.4.357 | Finds the Vulkan library when the program starts | MIT | `external/volk/LICENSE.md` |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Vectors and matrices | MIT or The Happy Bunny License, MIT is taken | `external/glm/copying.txt` |
| [spdlog](https://github.com/gabime/spdlog), with [fmt](https://github.com/fmtlib/fmt) inside it | 1.15.3 | Logging | MIT, both | `external/spdlog/LICENSE` |
| [Flecs](https://github.com/SanderMertens/flecs) | 4.1.6 | Entities and components | MIT | `external/flecs/LICENSE` |
| [rapidyaml](https://github.com/biojppm/rapidyaml), with c4core, fast_float, and debugbreak inside it | 0.8.0 | Reads and writes YAML | MIT; fast_float MIT, Apache 2.0, or Boost 1.0; debugbreak BSD 2-Clause | `external/rapidyaml/LICENSE.txt`, and the folders of `external/rapidyaml/ext/c4core` |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | 5.2.0 | Physics | MIT | `external/jolt-physics/LICENSE` |
| [miniaudio](https://github.com/mackron/miniaudio) | 0.11.25 | Sound | Public domain (Unlicense) or MIT No Attribution, MIT No Attribution is taken | `external/miniaudio/LICENSE` |
| [LuaJIT](https://github.com/LuaJIT/LuaJIT), which holds parts of Lua | 2.1, commit c6ffc141 | Scripts | MIT | `external/luajit/COPYRIGHT` |
| [stb](https://github.com/nothings/stb) | commit 013ac3b | Reads and writes images | MIT or public domain, MIT is taken | `external/stb/LICENSE` |
| [FreeType](https://github.com/freetype/freetype) | 2.14.3 | Draws the glyphs of fonts | The FreeType License or GPL 2, the FreeType License is taken | `external/freetype/docs/FTL.TXT` |
| [HarfBuzz](https://github.com/harfbuzz/harfbuzz) | 14.5.0 | Shapes text | "Old MIT" | `external/harfbuzz/COPYING` |
| [LunaSVG](https://github.com/sammycage/lunasvg), with [PlutoVG](https://github.com/sammycage/plutovg) inside it | 3.5.0 | Draws SVG images | MIT, both. PlutoVG holds a rasterizer of FreeType under the FreeType License, and files of stb | `external/lunasvg/LICENSE`, `external/lunasvg/plutovg/LICENSE`, `external/lunasvg/plutovg/source/FTL.TXT` |
| [assimp](https://github.com/assimp/assimp), with what it brings along | 5.4.2 | Reads model files. To be replaced, see [docs/roadmap.md](docs/roadmap.md) | BSD 3-Clause; inside it zlib and minizip (zlib), miniz (Unlicense), clipper and utf8cpp (Boost 1.0), poly2tri (BSD 3-Clause), Open3DGC, openddlparser, pugixml, and rapidjson (MIT), stb | `external/assimp/LICENSE`, and the folders of `external/assimp/contrib` |

The FreeType License asks to be named where the program is credited: portions
of this software are copyright The FreeType Project
(https://freetype.org). All rights reserved.

SDL2 holds hidapi, which is under one of three licences to be chosen: GPL 3,
BSD, or its original licence. The BSD one is the one to take,
`external/sdl2/src/hidapi/LICENSE-bsd.txt`.

## Libraries that are in the repository and not part of NeonRuntime

| Library | Version | Used for | Licence | Text |
|---|---|---|---|---|
| [GoogleTest](https://github.com/google/googletest) | 1.18.0 | The tests. Linked into the test programs alone | BSD 3-Clause | `external/googletest/LICENSE` |
| [Steam Audio](https://github.com/ValveSoftware/steam-audio) | 4.6.0 | Nothing yet: it is not built. Meant for sound in space | Apache 2.0, with libraries of its own that `core/THIRDPARTY.md` lists | `external/steam-audio/LICENSE.md` |

## Fonts

The only assets in the repository that were taken from elsewhere. Both are
the files as they were published, whole and unchanged.

| Font | From | Licence | Text |
|---|---|---|---|
| Inter, Regular and Bold, by The Inter Project Authors | https://github.com/rsms/inter/releases/tag/v4.1 | SIL Open Font License 1.1 | [app/NeonRuntime/engine/fonts/inter/LICENSE.txt](app/NeonRuntime/engine/fonts/inter/LICENSE.txt) |
| Noto Sans Arabic, Regular, by The Noto Project Authors | https://github.com/notofonts/arabic, the file `fonts/NotoSansArabic/hinted/ttf/NotoSansArabic-Regular.ttf` of https://github.com/notofonts/notofonts.github.io | SIL Open Font License 1.1 | [app/NeonRuntime/engine/fonts/noto/LICENSE.txt](app/NeonRuntime/engine/fonts/noto/LICENSE.txt) |

## Not in the repository, and part of a build that is handed on

These come from the machine that builds or runs the engine. What each asks is
in [docs/third-party-licenses.md](docs/third-party-licenses.md#not-in-the-repository-and-part-of-what-is-distributed).

| What | Licence |
|---|---|
| [MoltenVK](https://github.com/KhronosGroup/MoltenVK), which a build for macOS ships | Apache 2.0 |
| libc++, libc++abi, libunwind, and compiler-rt, linked on Linux and Windows | Apache 2.0 with LLVM exception |
| The runtime of mingw-w64 and winpthreads, linked on Windows | Zope Public License 2.1, public domain, MIT, and BSD, by file |

## Made for the engine

Everything else, under the license of the engine, [MIT](LICENSE). No model,
texture, sound, or image in the repository was taken from elsewhere: what a scene shows is built by the engine as
`Geometry`, or is one of these files, each written by a program from
numbers.

- `app/NeonRuntime/engine/ui/panel.png`, the panel of the engine's pause menu, and its copy `tests/game/assets/ui/panel.png`
- In the museum, `projects/museum/assets/`: `textures/sky/museum-panorama.png`, written by
  `tools/make-sky-images.py` from a formula, and `sounds/hum.wav`, made from sine waves
- In the game of the tests, `tests/game/assets/`:
    - `models/kit/`, `textures/brick.png`, `textures/concrete.png`, `textures/wood.png`, and `sounds/tone.wav`,
      written by `tools/make-test-game-assets.py`
    - `models/coloured-boxes.glb` and `models/coloured-quads.glb`, written by `tools/make-coloured-boxes.py` and
      `tools/make-coloured-quads.py`
    - `textures/sky/test-*.png`, written by `tools/make-sky-images.py`
    - `models/quad.obj`, a flat square for what shows an image or a surface
    - `textures/gamma-test.png`, four colours for the scene `gamma-test.scene.yml`
    - `ui/heart.png`, `ui/shield.svg`, `ui/gem-1x.png`, `ui/gem-2x.png`, `ui/gem-4x.png`, `ui/icons.png`,
      `ui/frame.png`, and `ui/test-card.jpg`, the images of the user interfaces of the tests
