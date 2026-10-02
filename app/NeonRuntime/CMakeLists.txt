set(TARGET NeonRuntime)

add_executable(${TARGET} main.cpp
        neon-runtime.cpp
        neon-runtime.hpp)

target_link_libraries(${TARGET} PRIVATE neon-core neon-vulkan neon-sdl2 neon-flecs neon-ryml neon-miniaudio neon-jolt neon-stb neon-freetype neon-lunasvg neon-platform)

setup_copy_assets(
        ${TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${APP_BIN_DIRECTORY}/assets"
)
neon_warnings(${TARGET})

setup_compile_shaders(
        ${TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan"
        "${APP_BIN_DIRECTORY}/assets/shaders"
        # the shaders of elements of a user interface
        FOLDERS ui
)
