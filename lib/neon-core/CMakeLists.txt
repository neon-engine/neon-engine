set(TARGET neon-core)

add_library(${TARGET} STATIC
        # runtime
        neon/runtime/runtime.cpp
        neon/runtime/runtime.hpp
        neon/runtime/settings-config.hpp
        neon/runtime/frame-capture.cpp
        neon/runtime/frame-capture.hpp

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

        # data
        neon/data/data-value.cpp
        neon/data/data-value.hpp
        neon/data/document-format.hpp
        neon/data/data-reader.cpp
        neon/data/data-reader.hpp

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
        neon/world-system/ecs/scene-file/component-format.cpp
        neon/world-system/ecs/scene-file/component-format.hpp
        neon/world-system/ecs/scene-file/engine-component-formats.cpp
        neon/world-system/ecs/scene-file/scene-file.cpp
        neon/world-system/ecs/scene-file/scene-file.hpp
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
)

if (NEON_BUILD_TESTS)
  # Mocks and fakes of the interfaces of neon-core, shared by the tests of
  # every library. They are headers, and are compiled into tests only.
  add_library(neon-testing INTERFACE
          neon/testing/fake-entity-store.hpp
          neon/testing/memory-file-system.hpp
          neon/testing/mock-entity-store.hpp
          neon/testing/mock-entity-world.hpp
          neon/testing/mock-file-system-context.hpp
          neon/testing/mock-input-system.hpp
          neon/testing/mock-render-context.hpp
          neon/testing/mock-render-pipeline.hpp
          neon/testing/mock-render-system.hpp
          neon/testing/mock-window-system.hpp
          neon/testing/mock-world-system.hpp
          neon/testing/recording-logger.hpp
          neon/testing/temporary-directory.hpp
  )

  target_link_libraries(neon-testing INTERFACE
          neon-core
          GTest::gmock
  )

  # Unit tests. Each one sits next to the file it tests.
  neon_add_unit_test(${TARGET} neon/command-line/command-line.test.cpp)
  neon_add_unit_test(${TARGET} neon/command-line/runtime-options.test.cpp)
  neon_add_unit_test(${TARGET} neon/common/data-buffer.test.cpp)
  neon_add_unit_test(${TARGET} neon/common/rotation.test.cpp)
  neon_add_unit_test(${TARGET} neon/common/transform.test.cpp)
  neon_add_unit_test(${TARGET} neon/common/util.test.cpp)
  neon_add_unit_test(${TARGET} neon/filesystem/file-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/input/headless-input-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/input/input-state.test.cpp)
  neon_add_unit_test(${TARGET} neon/logging/logging-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/logging/spd-logger.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/forward-render-pipeline.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/model.test.cpp)
  neon_add_unit_test(${TARGET} neon/runtime/frame-capture.test.cpp)
  neon_add_unit_test(${TARGET} neon/runtime/runtime.test.cpp)
  neon_add_unit_test(${TARGET} neon/window/headless-window-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/component-info.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/entity-store.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/entity-world.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/render-submission.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/spectator-movement.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/transform-propagation.test.cpp)
endif ()
