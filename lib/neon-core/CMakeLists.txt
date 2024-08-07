set(TARGET core)

add_library(${TARGET} STATIC
        neon/core/application/application.cpp
        neon/core/application/application.hpp
        neon/core/application/settings-config.hpp
        neon/core/common/data-buffer.hpp
        neon/core/common/util.hpp
        neon/core/input/input-system.hpp
        neon/core/render/render-system.hpp
        neon/core/render/opengl-render-system.cpp
        neon/core/render/opengl-render-system.hpp
        neon/core/window/window-system.hpp
        neon/core/scene/scene.cpp
        neon/core/scene/scene.hpp
        neon/core/render/render-context.hpp
        neon/core/window/sdl2-window-system.cpp
        neon/core/window/sdl2-window-system.hpp
        neon/core/input/sdl-2-input-system.cpp
        neon/core/input/sdl-2-input-system.hpp
        neon/core/window/window-context.hpp
        neon/core/oop-system/camera/camera.cpp
        neon/core/oop-system/camera/camera.hpp
        neon/core/render/open-gl-shader.cpp
        neon/core/render/open-gl-shader.hpp
        neon/core/render/open-gl-texture.cpp
        neon/core/render/open-gl-texture.hpp
        neon/core/render/open-gl-mesh.cpp
        neon/core/render/open-gl-mesh.hpp
        neon/core/render/open-gl-material.cpp
        neon/core/render/open-gl-material.hpp
        neon/core/render/data-types.hpp
        neon/core/input/input-context.hpp
        neon/core/input/input-state.hpp
        neon/core/oop-system/controllers/controller.hpp
        neon/core/oop-system/controllers/fps-player-controller.cpp
        neon/core/oop-system/controllers/fps-player-controller.hpp
        neon/core/oop-system/scene-graph/node.cpp
        neon/core/oop-system/scene-graph/node.hpp
        neon/core/oop-system/scene-graph/render-node.cpp
        neon/core/oop-system/scene-graph/render-node.hpp
        neon/core/oop-system/scene-graph/scene-graph.cpp
        neon/core/oop-system/scene-graph/scene-graph.hpp
        neon/core/oop-system/scene-graph/camera-node.cpp
        neon/core/oop-system/scene-graph/camera-node.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET} glm gl3w stb SDL2::SDL2main SDL2::SDL2-static)