set(TARGET core)

add_library(${TARGET} STATIC
        core/application/application.cpp
        core/application/application.hpp
        core/application/settings-config.hpp
        core/input/input-system.hpp
        core/render/render-system.hpp
        core/render/opengl-render-system.cpp
        core/render/opengl-render-system.hpp
        core/window/window-system.hpp
        core/scene/scene.cpp
        core/scene/scene.hpp
        core/render/render-context.hpp
        core/window/sdl2-window-system.cpp
        core/window/sdl2-window-system.hpp
        core/input/sdl-2-input-system.cpp
        core/input/sdl-2-input-system.hpp
        core/window/window-context.hpp
        core/oop-system/camera/camera.cpp
        core/oop-system/camera/camera.hpp
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
        core/render/data-types.hpp
        core/input/input-context.hpp
        core/input/input-state.hpp
        core/oop-system/controllers/controller.hpp
        core/oop-system/controllers/fps-player-controller.cpp
        core/oop-system/controllers/fps-player-controller.hpp
        core/oop-system/scene-graph/node.cpp
        core/oop-system/scene-graph/node.hpp
        core/oop-system/scene-graph/render-node.cpp
        core/oop-system/scene-graph/render-node.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/application
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/oop-system
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/entity
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/input
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/render
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/scene
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/util
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/core/window
)

target_link_libraries(${TARGET} glm gl3w stb SDL2::SDL2main SDL2::SDL2-static common)