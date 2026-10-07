# The sandbox: the project the engine is tried out with, one scene with a box
# on a floor. NeonRuntime ships no game of its own, so this is what a build
# of the engine runs first. See README.md.

# Configured on its own, this folder is a build of its own that puts the
# project together with a NeonRuntime that is built already:
#
#   cmake -S projects/sandbox -B build/sandbox && cmake --build build/sandbox
#
# The engine's build finds it too, and then brings these functions itself.
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
if (NOT COMMAND neon_add_project)
  project(sandbox NONE)
  include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/NeonSdk.cmake")
endif ()

# it has no code of its own, so no extension: its assets alone
neon_add_project(sandbox)
