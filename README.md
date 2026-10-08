<p align=center>
<img src="./docs/logo/main-logo.png" width=320>
</p>

<h1 align=center>Neon Engine</h1>

<p align=center><i>a[N]other [E]xtensible [O]pen-source e[N]gine</i></p>

> **Note:** Neon Engine is a working title, and the name is subject to change.
> The engine is in development and has not been released.

## Description

Neon Engine is a 3D game engine written in C++20, with a Vulkan renderer,
[Jolt Physics](https://github.com/jrouwe/JoltPhysics), entities and components on
[Flecs](https://github.com/SanderMertens/flecs), and scripting in Lua on
[LuaJIT](https://github.com/LuaJIT/LuaJIT).

A game is a project folder that NeonRuntime, the player, loads and runs, much as
Godot does. Scenes, prefabs, user interfaces, and settings are YAML files you
can write by hand, and when one is wrong the error names the file and the line.
Games can extend the engine with native extensions in C++, built on their own
against a prebuilt runtime, without compiling the engine.

Every part of the engine can be driven from the command line. A game can run
without a window, take its input from a script, and save the frames it draws.
That makes automated tests and continuous integration straightforward, and it
also lets the engine be used from scripts and AI tools if you want to work that
way.

Special thanks to [Akusha](https://vgen.co/Akusha/portfolio) for the logo.

## Getting started

- [Quick start](./docs/quick-start.md): clone, build, and walk through the
  museum, a scene with a hall for each part of the engine, in a few commands.
- [Development guide](./docs/development.md): building on every platform,
  the presets, the build options, and the tests, in depth.

## Features

What the engine does today. What is coming is in
[planned features](./docs/planned-features.md).

| Area | Overview | Docs |
|---|---|---|
| Platforms | macOS on Apple silicon; Linux and Windows x64 built in Docker | [development](./docs/development.md) |
| Rendering | Vulkan, PBR, HDR, cascaded shadows, glTF models, procedural meshes, post-processing, headless rendering | [renderer](./docs/vulkan-renderer.md), [models](./docs/models.md), [geometry](./docs/geometry.md) |
| World | Entities and components on Flecs, scenes and prefabs in YAML, object pools | [ECS](./docs/entity-component-system.md), [scenes](./docs/scenes.md), [prefabs](./docs/prefabs.md) |
| Physics | Jolt rigid bodies, joints, triggers, and a character controller | [physics](./docs/physics.md) |
| Sound | Positional sound and music on miniaudio | [audio](./docs/audio.md) |
| Input | Keyboard, mouse, and gamepads, with an action map | [input](./docs/input.md) |
| User interface | YAML and CSS, HarfBuzz text, SVG, gamepad navigation, screens in the world | [user interface](./docs/user-interface.md) |
| Scripting and code | Lua on LuaJIT, and native C++ extensions | [scripting](./docs/scripting.md), [extensions](./docs/extensions.md) |
| Settings | Graphics settings that change while the game runs | [settings](./docs/settings.md) |
| Automation | Windowless runs, scripted input, and frame capture from the command line | [command line](./docs/command-line.md) |

## Documentation

- [Planned features](./docs/planned-features.md): what is coming
- [Roadmap](./docs/roadmap.md): the plan in detail, and the decisions behind it
- [Style guide](./docs/style-guide.md): how the code is written
- [Everything else](./docs), one note per part of the engine

## Contributing

Pull requests from the public are not accepted for now, while the engine takes
shape. That may change later. In the meantime, everyone is free to clone the
engine and make versions of their own, under the terms of the license.

## License

Neon Engine is released under the [MIT license](./LICENSE). A game made with it
may be sold, modified, and distributed, provided the copyright notice and the
license text are included. A credit noting that the game was made with Neon
Engine is appreciated, though not required.

The libraries the engine uses, and what their licenses ask for, are listed in
[third-party-licenses.md](./docs/third-party-licenses.md).

## Supporting the project

Neon Engine is free to use: anyone may build it from source at no cost.
Prebuilt, tested builds are planned to be offered for purchase, and will be the
most direct way to fund the engine's continued development. If you build Neon
Engine yourself and find it valuable, please consider buying a build once they
are available. Ways to sponsor the project will be listed here once they are
set up.
