cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

# General build settings
project(ProjectNeon)
set(CMAKE_CXX_STANDARD 20)

# Echo system/build information
message("system name: ${CMAKE_SYSTEM_NAME}")
message("source directory: ${CMAKE_SOURCE_DIR}")
message("binary directory: ${CMAKE_BINARY_DIR}")
message("build type: ${CMAKE_BUILD_TYPE}")
message("architecture: ${CMAKE_SYSTEM_PROCESSOR}")

# Set output directories for anyone not using CLion
set(TARGET_DIR_FOR_CMAKE_FILES cmake-build-${CMAKE_BUILD_TYPE})
string(TOLOWER ${TARGET_DIR_FOR_CMAKE_FILES} TARGET_DIR_FOR_CMAKE_FILES)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

message("cmake binary directory: ${CMAKE_BINARY_DIR}")
message("runtime output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Find locally installed packages
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(gl3w REQUIRED)
find_package(Stb REQUIRED)

# List of targets to generate build files
set(APP_TARGETS TestApp LearnOpenGL)

foreach (APP IN LISTS APP_TARGETS)
    message("generating build target ${APP}")

    add_subdirectory(app/${APP})

    set(BASE_APP_BIN_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    set(APP_BIN_DIRECTORY "${BASE_APP_BIN_DIRECTORY}/${APP}")

    # Each executable will be in its own folder
    set_target_properties(
            ${APP}
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${APP_BIN_DIRECTORY}/lib"
            LIBRARY_OUTPUT_DIRECTORY "${APP_BIN_DIRECTORY}/lib"
            RUNTIME_OUTPUT_DIRECTORY "${APP_BIN_DIRECTORY}"
    )

#    # Create shader folder
#    set(SHADER_OUTPUT_DIR ${APP_BIN_DIRECTORY}/shaders)
#    message("shaders will be compiled and output to ${SHADER_OUTPUT_DIR}")
#    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
#
#    add_custom_command(
#            TARGET ${APP}
#            PRE_BUILD
#            COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Compiling shaders..."
#    )
#
#    file(GLOB CORE_SHADERS ${PROJECT_SOURCE_DIR}/src/core/shaders/*)
#    foreach (SHADER_PATH IN LISTS CORE_SHADERS)
#        get_filename_component(SHADER ${SHADER_PATH} NAME)
#        add_custom_command(
#                TARGET ${APP}
#                COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Compiling ${SHADER} to ${SHADER}.spv"
#        )
#
#        add_custom_command(
#                TARGET ${APP}
#                COMMAND glslc ${SHADER_PATH} -o ${SHADER_OUTPUT_DIR}/${SHADER}.spv
#                BYPRODUCTS ${SHADER_OUTPUT_DIR}/${SHADER}.spv
#        )
#    endforeach ()
endforeach ()
