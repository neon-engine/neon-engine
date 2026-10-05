# The museum: one scene that shows every part of the engine a player can see,
# hear, or play with, walked from the first person. See README.md.

# Configured on its own, this folder is a build of its own that puts the
# project together with a NeonRuntime that is built already:
#
#   cmake -S projects/museum -B build/museum && cmake --build build/museum
#
# The engine's build finds it too, and then brings these functions itself.
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
if (NOT COMMAND neon_add_project)
  project(museum NONE)
  include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/NeonSdk.cmake")
endif ()

# it has no code of its own, so no extension: its assets alone
neon_add_project(museum)
