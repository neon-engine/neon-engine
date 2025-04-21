<h1 style="text-align: center">NEON Engine</h1>

<h2 style="text-align: center">[N]ext-gen [E]xtensible [O]pen e[N]gine</h2>

## Description

Neon Engine is a modular, high-performance hobby game engine written in C++.
It leverages the power of existing open source libraries while maintaining a clean,
extensible architecture.

What started as a hobby engine, the goal is for the engine to support multiple platforms
and provide intuitive tools for creating interactive experiences without sacrificing low-level control.
Neon Engine aims to balance flexibility with simplicity, making game development accessible
while allowing for advanced customization.

## Planned Features

- Cross-platform support
- Graphics
  - [x] Vulkan
  - [ ] OpenGL
- Other Library Integrations
  - [x] Windowing / Input
    - [SDL2](https://wiki.libsdl.org/SDL2/Introduction)
  - [ ] Physics
  - [ ] Audio
  - [x] Logging
    - [spdlog](https://github.com/gabime/spdlog)
  - [x] Texture Loading
    - [stb](https://github.com/nothings/stb)
- Other features
  - Scripting Engine
    - [ ] LUA Bindings
    - [ ] Rust Bindings
    - [ ] Python Bindings

## Development
Review the [docs](./docs) section for how to build and run the project

## Why C++?
It is 2025, what am I thinking?! Well, jokes aside, C++ has been around for a few decades, Rust still lacks libraries found within
the C++ ecosystem. Typically, libraries found on Rust cargo are just wrappers around C++ libraries anyway.
But I would be lying if I did not wish to be writing this project in rust. This is part of the reason why I am
focusing on making this engine modular. I hope that as time goes on I can swap out pieces of the engine with modules
written in Rust. I do plan on supporting Rust as a language to allow developers to code their games in.
