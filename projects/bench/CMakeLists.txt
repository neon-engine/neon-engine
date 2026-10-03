# The bench: scenes that measure the engine, with the same work done by a
# script in Lua, by a system in C++, and by nothing. See README.md.

# Configured on its own, this folder is a build of its own that compiles the
# extension and nothing of the engine:
#
#   cmake -S projects/bench -B build/bench && cmake --build build/bench
#
# The engine's build finds it too, and then brings these functions itself.
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
if (NOT COMMAND neon_add_project)
  project(bench C CXX)
  include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/NeonSdk.cmake")
endif ()

neon_add_project(bench
        SOURCES
        extensions/bench/bench.cpp
        extensions/bench/light-copying.hpp
        extensions/bench/counting.hpp
        extensions/bench/crate.hpp
        extensions/bench/no-copying.hpp
        extensions/bench/native-light-copy.hpp
        extensions/bench/native-no-copy.hpp
        extensions/bench/native-heavy-copy.hpp
        extensions/bench/native-spawner.hpp
        extensions/bench/heavy-copying.hpp
        extensions/bench/spawning.hpp
)
