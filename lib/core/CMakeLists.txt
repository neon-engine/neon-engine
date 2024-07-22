set(TARGET core)

add_library(${TARGET} STATIC
        core/application.cpp
        core/application.hpp
        core/settings-config.hpp

        core/input/input-system.hpp
        core/input/glfw-input-system.cpp
        core/input/glfw-input-system.hpp
        core/render/render-system.hpp
        core/render/opengl-render-system.cpp
        core/render/opengl-render-system.hpp
        core/window/window-system.hpp
        core/window/glfw-window-system.cpp
        core/window/glfw-window-system.hpp
        core/scene/scene.cpp
        core/scene/scene.hpp
        core/entity/spectator.cpp
        core/entity/spectator.hpp
        core/render/render-context.hpp
        core/window/sdl2-window-system.cpp
        core/window/sdl2-window-system.hpp
        core/input/sdl-2-input-system.cpp
        core/input/sdl-2-input-system.hpp
        core/window/window-context.hpp
        core/camera/camera.cpp
        core/camera/camera.hpp
        core/render/primitives/cube.cpp
        core/render/primitives/cube.hpp
        core/render/open-gl-shader.cpp
        core/render/open-gl-shader.hpp
        core/render/open-gl-texture.cpp
        core/render/open-gl-texture.hpp
        core/render/open-gl-mesh.cpp
        core/render/open-gl-mesh.hpp
        core/render/open-gl-material.cpp
        core/render/open-gl-material.hpp
        core/util.hpp
        core/private/neon-pch.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/entity
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/input
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/render
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/scene
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/window
)

target_link_libraries(${TARGET} glfw glm gl3w stb SDL2::SDL2main SDL2::SDL2-static)
target_precompile_headers(${TARGET} PRIVATE core/private/neon-pch.hpp)