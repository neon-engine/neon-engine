cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# General build settings
project(neon-engine)
set(CMAKE_CXX_STANDARD 20)

# Written to the build folder as compile_commands.json. clang-tidy and editors
# read from it how each file is compiled.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Echo system/build information
message("system name: ${CMAKE_SYSTEM_NAME}")
message("architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message("build type: ${CMAKE_BUILD_TYPE}")
message("source directory: ${CMAKE_SOURCE_DIR}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -U_FORTIFY_SOURCE")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -U_FORTIFY_SOURCE -fno-inline")

  message("CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")
  message("CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
endif ()
# TODO set release flags here

# Set output directories for anyone not using CLion

# Set the build directory
set(CMAKE_BINARY_DIR build/${CMAKE_BUILD_TYPE})
string(TOLOWER ${CMAKE_BINARY_DIR} CMAKE_BINARY_DIR)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/${CMAKE_BINARY_DIR})

# bin/ holds the applications alone, each in its own folder under its
# platform, with what it needs next to it. Everything else that is built,
# the tests, their tools, and whatever a library of external/ makes, stays
# under the build folder, where CMake puts it when nothing says otherwise.
set(NEON_BIN_DIRECTORY bin/${CMAKE_BUILD_TYPE})
string(TOLOWER ${NEON_BIN_DIRECTORY} NEON_BIN_DIRECTORY)
set(NEON_BIN_DIRECTORY ${CMAKE_SOURCE_DIR}/${NEON_BIN_DIRECTORY})

message("cmake binary directory: ${CMAKE_BINARY_DIR}")
message("applications directory: ${NEON_BIN_DIRECTORY}")

# Add cmake folder where we place our own custom Find*.cmake files
# And locate custom packages within our file structure
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/find")

# The warnings every target of the engine is built with
include("${CMAKE_CURRENT_LIST_DIR}/cmake/Warnings.cmake")

# Native extensions, what a platform is called for them, and how one is built
include("${CMAKE_CURRENT_LIST_DIR}/cmake/NeonExtensions.cmake")


# Find packages installed on the toolchain/sysroot
find_package(stb REQUIRED)

# Add external subdirectories

# Build every dependency as a static library so the executables run without
# shipping shared objects alongside them. assimp flips BUILD_SHARED_LIBS on by
# default, and that would otherwise leak into every subdirectory after it.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glm)
add_subdirectory(external/sdl2)

# assimp defaults to -Werror, which breaks on new warnings whenever the
# compiler is upgraded (clang 20 added -Wnontrivial-memcall, for example).
# Its warnings are not ours to fix, so build it without -Werror.
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
# Always use assimp's bundled zlib. Picking up a system zlib adds its include
# directory with -I, which on macOS with Homebrew LLVM puts the SDK's C headers
# ahead of libc++'s and breaks every <cstdio>/<cmath> include.
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
# assimp builds its own tests by default, a GoogleTest and an executable
# named `unit` that lands at the root of bin/, next to nothing of ours
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/assimp)
if(APPLE AND TARGET zlibstatic)
  # assimp's bundled zlib 1.2.13 treats every Apple target as classic Mac OS
  # and defines fdopen as NULL, which clashes with the SDK's declaration.
  # Predefining the macro as itself makes zlib skip that define.
  target_compile_definitions(zlibstatic PRIVATE fdopen=fdopen)
endif()
include("${CMAKE_CURRENT_LIST_DIR}/cmake/JoltPhysics.cmake")
add_subdirectory(external/spdlog)

# Vulkan. The engine builds against the headers alone and volk finds the
# Vulkan library when the app starts, so no Vulkan SDK is needed to build,
# not even when cross-compiling.
add_subdirectory(external/vulkan-headers)
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
add_subdirectory(external/volk)
target_link_libraries(volk PUBLIC Vulkan::Headers)

# Flecs, the entity component system behind neon-flecs. Only the static
# library is built.
set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(external/flecs)

add_subdirectory(external/rapidyaml)
if(MINGW)
  # c4core only detects MinGW under GCC. Under clang it misses it and rapidyaml
  # then includes <alloca.h>, which mingw-w64 does not ship.
  target_compile_definitions(ryml PUBLIC C4_MINGW)
endif()

# Tests. They are never part of the default build or of an application. The
# target neon-tests builds them, and ctest runs them.
option(NEON_BUILD_TESTS "Make the unit tests and functional tests available to build" ON)
if (NEON_BUILD_TESTS)
  enable_testing()

  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  # only matters to the compiler of Microsoft, where GoogleTest would
  # otherwise link another C runtime than the engine
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_subdirectory(external/googletest EXCLUDE_FROM_ALL)

  include("${CMAKE_CURRENT_LIST_DIR}/cmake/NeonTests.cmake")
endif ()

# Additional libraries
add_subdirectory(lib/neon-core)
add_subdirectory(lib/neon-vulkan)
add_subdirectory(lib/neon-sdl2)
add_subdirectory(lib/neon-flecs)
add_subdirectory(lib/neon-ryml)
add_subdirectory(lib/neon-miniaudio)
add_subdirectory(lib/neon-jolt)
add_subdirectory(lib/neon-lua)
add_subdirectory(lib/neon-stb)
add_subdirectory(lib/neon-freetype)
add_subdirectory(lib/neon-lunasvg)
add_subdirectory(lib/neon-platform)

# Load additional cmake
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CopyAssets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CompileShaders.cmake")

set(APP_TARGETS
        NeonRuntime
)

string(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_CMAKE_SYSTEM_NAME)

# where neon_add_extension puts an extension unless it is told otherwise:
# next to NeonRuntime
set(NEON_EXTENSIONS_DIRECTORY
        "${NEON_BIN_DIRECTORY}/${LOWER_CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}/NeonRuntime/extensions")

foreach (APP IN LISTS APP_TARGETS)
  message("generating build target ${APP}")

  set(BASE_APP_BIN_DIRECTORY "${NEON_BIN_DIRECTORY}/${LOWER_CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
  set(APP_BIN_DIRECTORY "${BASE_APP_BIN_DIRECTORY}/${APP}")

  # ensure binaries can run anywhere, assuming they are ABI compatible that is
  # todo consider downgrading GCC so this is not necessary
  # Apple has no static C++ runtime to link. It ships with the system instead.
  # The flag is the linker's alone: given to the compiler as well, clang
  # reports it as unused, which -Werror turns into a failed build
  if (NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
  endif ()

  add_subdirectory(app/${APP})

  # Each executable is in its own folder. A shared library of its own would
  # go next to it, which is where Windows looks for a DLL first; the engine
  # and everything it links are static, so there is none today.
  set_target_properties(
          ${APP}
          PROPERTIES
          ARCHIVE_OUTPUT_DIRECTORY "${APP_BIN_DIRECTORY}/lib"
          LIBRARY_OUTPUT_DIRECTORY "${APP_BIN_DIRECTORY}"
          RUNTIME_OUTPUT_DIRECTORY "${APP_BIN_DIRECTORY}"
  )
endforeach ()

# Projects with code of their own, each a folder of projects/ with a
# CMakeLists.txt that calls neon_add_project. They are found here, so that a
# new one needs no edit of this file, and built on request alone.
include("${CMAKE_CURRENT_LIST_DIR}/cmake/NeonProjects.cmake")
file(GLOB NEON_PROJECT_LISTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/projects/*/CMakeLists.txt")
foreach (PROJECT_LIST IN LISTS NEON_PROJECT_LISTS)
  get_filename_component(PROJECT_FOLDER "${PROJECT_LIST}" DIRECTORY)
  get_filename_component(PROJECT_NAME_OF_FOLDER "${PROJECT_FOLDER}" NAME)
  message("found the project ${PROJECT_NAME_OF_FOLDER}")
  add_subdirectory("${PROJECT_FOLDER}")
endforeach ()

# Functional tests come last, they start the applications declared above
if (NEON_BUILD_TESTS)
  add_subdirectory(tests)
endif ()

# Checks against the style guide. They run on request only
include("${CMAKE_CURRENT_LIST_DIR}/cmake/ClangTools.cmake")
set(CHECKED_TARGETS neon-core neon-vulkan neon-sdl2 neon-flecs neon-platform ${APP_TARGETS})
if (NEON_BUILD_TESTS)
  get_property(NEON_TEST_TARGETS GLOBAL PROPERTY NEON_TEST_TARGETS)
  list(APPEND CHECKED_TARGETS neon-testing ${NEON_TEST_TARGETS})
endif ()
setup_clang_tools(${CHECKED_TARGETS})
