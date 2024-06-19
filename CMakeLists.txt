cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# General build settings
project(ProjectNeon)
set(CMAKE_CXX_STANDARD 20)

string(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_CMAKE_SYSTEM_NAME)

# Echo system/build information
message("system name: ${CMAKE_SYSTEM_NAME}")
message("source directory: ${CMAKE_SOURCE_DIR}")
message("binary directory: ${CMAKE_BINARY_DIR}")
message("build type: ${CMAKE_BUILD_TYPE}")
message("architecture: ${CMAKE_SYSTEM_PROCESSOR}")

# Set output directories for anyone not using CLion
set(TARGET_DIR_FOR_CMAKE_FILES cmake-build-${CMAKE_BUILD_TYPE})
string(TOLOWER ${TARGET_DIR_FOR_CMAKE_FILES} TARGET_DIR_FOR_CMAKE_FILES)

string(TOLOWER ${CMAKE_BUILD_TYPE} LOWER_CMAKE_BUILD_TYPE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${LOWER_CMAKE_BUILD_TYPE})

message("cmake binary directory: ${CMAKE_BINARY_DIR}")
message("runtime output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

#find_package(Vulkan REQUIRED)

add_subdirectory(external/gl3w)
add_subdirectory(external/glfw)
add_subdirectory(external/glm)

# List of targets to generate build files
set(APP_TARGETS
        LearnOpenGL
        NeonFPS
)


foreach (APP IN LISTS APP_TARGETS)
    message("generating build target ${APP}")

    set(BASE_APP_BIN_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${LOWER_CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    set(APP_BIN_DIRECTORY "${BASE_APP_BIN_DIRECTORY}/${APP}")

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
