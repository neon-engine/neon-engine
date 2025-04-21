cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# General build settings
project(ProjectNeon)
set(CMAKE_CXX_STANDARD 20)

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
find_package(Vulkan)
find_package(glad REQUIRED)
find_package(stb REQUIRED)

# Add external subdirectories
add_subdirectory(external/glm)
add_subdirectory(external/sdl2)
add_subdirectory(external/assimp)
add_subdirectory(external/jolt-physics/Build)
add_subdirectory(external/spdlog)
add_subdirectory(external/rapidyaml)

# Additional libraries
add_subdirectory(lib/neon-core)
add_subdirectory(lib/neon-gl)
add_subdirectory(lib/neon-sdl2)

# Load additional cmake
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CopyAssets.cmake")

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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")

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
