set(TARGET neon-core)

add_library(${TARGET} STATIC
        # runtime
        neon/runtime/runtime.cpp
        neon/runtime/runtime.hpp
        neon/runtime/settings-config.hpp

        # command line
        neon/command-line/command-line.cpp
        neon/command-line/command-line.hpp
        neon/command-line/command-line-context.hpp
        neon/command-line/command-line-options.hpp
        neon/command-line/runtime-options.cpp
        neon/command-line/runtime-options.hpp

        # common
        neon/common/data-buffer.hpp
        neon/common/util.hpp

        # filesystem
        neon/filesystem/file-system.cpp
        neon/filesystem/file-system.hpp
        neon/filesystem/file-system-context.hpp

        # window
        neon/window/window-system.hpp
        neon/window/window-context.hpp
        neon/window/headless-window-system.cpp
        neon/window/headless-window-system.hpp

        # input
        neon/input/input-system.hpp
        neon/input/input-context.hpp
        neon/input/input-state.hpp
        neon/input/headless-input-system.cpp
        neon/input/headless-input-system.hpp

        # logging
        neon/logging/logging-system.cpp
        neon/logging/logging-system.hpp
        neon/logging/logger.hpp
        neon/logging/logging-system.cpp
        neon/logging/logging-system.hpp
        neon/logging/logging-system.cpp
        neon/logging/logging-system.hpp
        neon/logging/logger.hpp
        neon/logging/spd-logger.cpp
        neon/logging/spd-logger.hpp
        neon/logging/spd-logger.cpp
        neon/logging/spd-logger.hpp
        neon/logging/logging-context.hpp

        # common
        neon/common/color.hpp
        neon/common/transform.hpp
        neon/common/rotation.hpp

        # render
        neon/render/camera-info.hpp
        neon/render/render-target.hpp
        neon/render/render-info.hpp
        neon/render/render-object-ref.hpp
        neon/render/vertex.hpp
        neon/render/mesh.hpp
        neon/render/render-pipeline.hpp
        neon/render/forward-render-pipeline.cpp
        neon/render/forward-render-pipeline.hpp
        neon/render/render-system.hpp
        neon/render/render-context.hpp
        neon/render/mesh.hpp
        neon/render/model.cpp
        neon/render/model.hpp
        neon/render/texture-info.hpp
        neon/render/texture-type.hpp
        neon/render/light-source.hpp
        neon/render/material-info.hpp

        # world-system
        neon/world-system/world-system.hpp
        neon/world-system/ecs/entity.hpp
        neon/world-system/ecs/component-info.hpp
        neon/world-system/ecs/entity-query.hpp
        neon/world-system/ecs/entity-store.hpp
        neon/world-system/ecs/entity-system.hpp
        neon/world-system/ecs/scene.hpp
        neon/world-system/ecs/entity-world.cpp
        neon/world-system/ecs/entity-world.hpp
        neon/world-system/ecs/components/camera.hpp
        neon/world-system/ecs/components/light.hpp
        neon/world-system/ecs/components/renderable.hpp
        neon/world-system/ecs/components/spectator.hpp
        neon/world-system/ecs/systems/render-submission.cpp
        neon/world-system/ecs/systems/render-submission.hpp
        neon/world-system/ecs/systems/spectator-movement.cpp
        neon/world-system/ecs/systems/spectator-movement.hpp
        neon/world-system/ecs/systems/transform-propagation.cpp
        neon/world-system/ecs/systems/transform-propagation.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
        glm::glm
        spdlog::spdlog
        assimp::assimp
        ryml
)
