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
        neon/core/oop-system/scene-graph/node/node.cpp
        neon/core/oop-system/scene-graph/node/node.hpp
        neon/core/oop-system/scene-graph/node/render-node.cpp
        neon/core/oop-system/scene-graph/node/render-node.hpp
        neon/core/oop-system/scene-graph/node/camera-node.cpp
        neon/core/oop-system/scene-graph/node/camera-node.hpp
        neon/core/oop-system/scene-graph/node/player-node.cpp
        neon/core/oop-system/scene-graph/node/player-node.hpp
        neon/core/oop-system/scene-graph/scene-manager.cpp
        neon/core/oop-system/scene-graph/scene-manager.hpp
        neon/core/render/render-pipeline.hpp
        neon/core/render/forward-render-pipeline.cpp
        neon/core/render/forward-render-pipeline.hpp
        neon/core/logging/logging-system.cpp
        neon/core/logging/logging-system.hpp
        neon/core/logging/logger.hpp
        neon/core/logging/logging-system.cpp
        neon/core/logging/logging-system.hpp
        neon/core/logging/logging-system.cpp
        neon/core/logging/logging-system.hpp
        neon/core/logging/logger.hpp
        neon/core/logging/spd-logger.cpp
        neon/core/logging/spd-logger.hpp
        neon/core/logging/spd-logger.cpp
        neon/core/logging/spd-logger.hpp
        neon/core/logging/logging-context.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET} glm glad stb SDL2::SDL2main SDL2::SDL2-static spdlog)