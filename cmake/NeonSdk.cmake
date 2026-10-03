# What a game or an extension that is built on its own includes, without the
# engine's build: the header of extensions, neon_add_extension, and
# neon_add_project. See docs/extensions.md.
#
#     cmake_minimum_required(VERSION 3.16)
#     project(my-game C CXX)
#     include(<the engine>/cmake/NeonSdk.cmake)
#     neon_add_project(my-game SOURCES extensions/my-game/my-game.cpp)
#
# Nothing of the engine is compiled. The game is put together with a runtime
# that is built already, in NEON_RUNTIME_DIRECTORY.

# an extension in C++ uses designated initializers and the like
if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/NeonExtensions.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/NeonProjects.cmake")
