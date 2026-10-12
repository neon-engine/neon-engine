set(TARGET neon-imgui)

# Dear ImGui, the docking branch, built from its submodule as a static
# library: the library itself, its SDL2 and Vulkan backends, and the helpers
# that take std::string. It has no build of its own. The Vulkan backend finds
# Vulkan through volk, as the renderer does, so that no Vulkan library is
# linked.
set(IMGUI_DIRECTORY "${CMAKE_SOURCE_DIR}/external/imgui")
add_library(imgui STATIC EXCLUDE_FROM_ALL
        "${IMGUI_DIRECTORY}/imgui.cpp"
        "${IMGUI_DIRECTORY}/imgui_demo.cpp"
        "${IMGUI_DIRECTORY}/imgui_draw.cpp"
        "${IMGUI_DIRECTORY}/imgui_tables.cpp"
        "${IMGUI_DIRECTORY}/imgui_widgets.cpp"
        "${IMGUI_DIRECTORY}/backends/imgui_impl_sdl2.cpp"
        "${IMGUI_DIRECTORY}/backends/imgui_impl_vulkan.cpp"
        "${IMGUI_DIRECTORY}/misc/cpp/imgui_stdlib.cpp"
)
target_include_directories(imgui SYSTEM PUBLIC
        "${IMGUI_DIRECTORY}"
        "${IMGUI_DIRECTORY}/backends"
        "${IMGUI_DIRECTORY}/misc/cpp"
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)
target_link_libraries(imgui PUBLIC SDL2::SDL2-static volk)

add_library(${TARGET} STATIC
        neon/tool-ui/imgui-tool-ui-system.cpp
        neon/tool-ui/imgui-tool-ui-system.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Dear ImGui is private. What includes the header of this library sees no
# declaration of it and cannot call it. The window and the input of SDL2
# and the renderer of Vulkan are what it draws with, so those backends are
# public: an application hands them over.
target_link_libraries(${TARGET}
        PUBLIC neon-core neon-sdl2 neon-vulkan
        PRIVATE imgui
)
