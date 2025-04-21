# Development

## Environment

### OS

The following are supported

- [x] Linux
- [ ] Windows
- [ ] MacOS

### Required

- clang/llvm
  - support for C++ 20+
- lldb
- cmake
- ninja
- mingw-w64 if targeting Windows
  - [MSYS2](https://www.msys2.org/) if on a Windows environment
- GPU with OpenGL or Vulkan support (will affect what graphics backend you can use)

### Recommended

- Downloading the [Neon SDK](https://github.com/neon-engine/neon-sdk)
- CLion
  - Project was primarily developed using CLion

## How To Build

First generate build files:

```bash
cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_MAKE_PROGRAM=ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -G Ninja \
  -S <project-root> \
  -B <project-root>/build/debug/x86_64
```

If using SDK, make sure to set the correct toolchain file

```bash
  -DCMAKE_TOOLCHAIN_FILE=/opt/neon-sdk/x86_64-linux-x86_64/toolchainfile.cmake
```

Now compile:

```bash
cmake --build \
  <project-root>/build/debug/x86_64 \
  --target <app-name> -j 10
```

The `<app-name>` will largely depend on what project you are trying to compile under the apps folder.

Typically this will be NeonRuntime as of the time of writing this README.
There may be other apps such as NeonEditor or maybe other example programs.

binaries can be located under `<bin>/<release>/<os>-<arch>/<app-name>/<app-name>`
