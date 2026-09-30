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
  # TODO set Wall/Wextra/Werror later when we no longer have git submodules and these libraries are in the toolchain instead
  #set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Werror")
  #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -fno-inline")
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

# Set the bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/${CMAKE_BUILD_TYPE})
string(TOLOWER ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CMAKE_RUNTIME_OUTPUT_DIRECTORY)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

message("cmake binary directory: ${CMAKE_BINARY_DIR}")
message("runtime output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Add cmake folder where we place our own custom Find*.cmake files
# And locate custom packages within our file structure
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/find")


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
add_subdirectory(external/assimp)
if(APPLE AND TARGET zlibstatic)
  # assimp's bundled zlib 1.2.13 treats every Apple target as classic Mac OS
  # and defines fdopen as NULL, which clashes with the SDK's declaration.
  # Predefining the macro as itself makes zlib skip that define.
  target_compile_definitions(zlibstatic PRIVATE fdopen=fdopen)
endif()
add_subdirectory(external/jolt-physics/Build)
add_subdirectory(external/spdlog)

# Vulkan. The engine builds against the headers alone and volk finds the
# Vulkan library when the app starts, so no Vulkan SDK is needed to build,
# not even when cross-compiling.
add_subdirectory(external/vulkan-headers)
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
add_subdirectory(external/volk)
target_link_libraries(volk PUBLIC Vulkan::Headers)

add_subdirectory(external/rapidyaml)
if(MINGW)
  # c4core only detects MinGW under GCC. Under clang it misses it and rapidyaml
  # then includes <alloca.h>, which mingw-w64 does not ship.
  target_compile_definitions(ryml PUBLIC C4_MINGW)
endif()

# Additional libraries
add_subdirectory(lib/neon-core)
add_subdirectory(lib/neon-vulkan)
add_subdirectory(lib/neon-sdl2)

# Load additional cmake
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CopyAssets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CompileShaders.cmake")

set(APP_TARGETS
        NeonRuntime
)

string(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_CMAKE_SYSTEM_NAME)

foreach (APP IN LISTS APP_TARGETS)
  message("generating build target ${APP}")

  set(BASE_APP_BIN_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${LOWER_CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
  set(APP_BIN_DIRECTORY "${BASE_APP_BIN_DIRECTORY}/${APP}")

  # ensure binaries can run anywhere, assuming they are ABI compatible that is
  # todo consider downgrading GCC so this is not necessary
  # Apple has no static C++ runtime to link. It ships with the system instead
  if (NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")
  endif ()

  add_subdirectory(app/${APP})

  # Each executable will be in its own folder
  set_target_properties(
          ${APP}
          PROPERTIES
          ARCHIVE_OUTPUT_DIRECTORY "${APP_BIN_DIRECTORY}/lib"
          LIBRARY_OUTPUT_DIRECTORY "${APP_BIN_DIRECTORY}/lib"
          RUNTIME_OUTPUT_DIRECTORY "${APP_BIN_DIRECTORY}"
  )
endforeach ()

# Checks against the style guide. They run on request only
include("${CMAKE_CURRENT_LIST_DIR}/cmake/ClangTools.cmake")
setup_clang_tools(neon-core neon-vulkan neon-sdl2 ${APP_TARGETS})
