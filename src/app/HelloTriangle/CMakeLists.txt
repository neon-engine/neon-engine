set(TARGET HelloTriangle)

add_executable(${TARGET} main.cpp)

target_link_libraries(${TARGET} PRIVATE
        SDL2::SDL2main
        SDL2::SDL2-static
        Vulkan::Vulkan
)

setup_copy_assets(
        ${TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${APP_BIN_DIRECTORY}/assets"
        TRUE
)
