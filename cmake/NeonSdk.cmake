# What a game or an extension that is built on its own includes, without the
# engine's build: the header of extensions, neon_add_extension, and
# neon_add_project. See docs/extensions.md.
#
#     cmake_minimum_required(VERSION 3.18)
#     project(my-game C CXX)
#     include(<the engine>/cmake/NeonSdk.cmake)
#     neon_add_project(my-game SOURCES extensions/my-game/my-game.cpp)
#
# Nothing of the engine is compiled. The game is put together with a runtime
# that is built already, in NEON_RUNTIME_DIRECTORY.
#
# For another platform than the one that builds, the build is given the
# toolchain file of the engine for it, as any build of CMake is:
#
#     cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<the engine>/cmake/toolchains/windows-x64-llvm-mingw.cmake

# what neon_add_extension needs: an option of the linker by the language that
# is linked
cmake_minimum_required(VERSION 3.18)

# an extension in C++ uses designated initializers and the like
if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/NeonExtensions.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/NeonProjects.cmake")
