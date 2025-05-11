set(TARGET NeonRuntime)

add_executable(${TARGET} main.cpp
        neon-runtime-app.cpp
        neon-runtime-app.hpp)

target_link_libraries(${TARGET} PRIVATE neon-core neon-gl neon-sdl2 neon-vulkan)

setup_copy_assets(
        ${TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${APP_BIN_DIRECTORY}/assets"
)
