set(TARGET neon-core)

add_library(${TARGET} STATIC
        # the behaviour of the user interface, and what it needs of the input
        # and of the window. New files are listed in this block
        neon/command-line/display-options.cpp
        neon/command-line/display-options.hpp
        neon/input/clipboard-context.hpp
        neon/reflection/field-numbers.cpp
        neon/reflection/field-numbers.hpp
        neon/reflection/field-text.cpp
        neon/reflection/field-text.hpp
        neon/input/input-script.cpp
        neon/input/input-script.hpp
        neon/input/key.hpp
        neon/ui/tree-ui-system-describe.cpp
        neon/ui/elements/ui-binding.hpp
        neon/ui/elements/ui-checkbox.cpp
        neon/ui/elements/ui-checkbox.hpp
        neon/ui/elements/ui-radio.cpp
        neon/ui/elements/ui-radio.hpp
        neon/ui/elements/ui-select.cpp
        neon/ui/elements/ui-select.hpp
        neon/ui/elements/ui-slider.cpp
        neon/ui/elements/ui-slider.hpp
        neon/ui/elements/ui-text-field.cpp
        neon/ui/elements/ui-text-field.hpp
        neon/ui/css/css-expression.cpp
        neon/ui/css/css-expression.hpp
        neon/ui/css/css-media.cpp
        neon/ui/css/css-media.hpp
        neon/ui/css/css-selector.cpp
        neon/ui/css/css-selector.hpp
        neon/ui/css/css-style-sheet.cpp
        neon/ui/css/css-style-sheet.hpp
        neon/ui/tree-ui-system-animation.cpp
        neon/ui/tree-ui-system-elements.cpp
        neon/ui/tree-ui-system-events.cpp
        neon/ui/tree-ui-system-host.cpp
        neon/ui/tree-ui-system-input.cpp
        neon/ui/tree-ui-system-scroll.cpp
        neon/ui/ui-animator.cpp
        neon/ui/ui-animator.hpp
        neon/ui/ui-cascade.cpp
        neon/ui/ui-cascade.hpp
        neon/ui/ui-declarations.cpp
        neon/ui/ui-declarations.hpp
        neon/ui/ui-draw-cache.hpp
        neon/ui/ui-element-host.hpp
        neon/ui/ui-fields.hpp
        neon/ui/ui-handle.hpp
        neon/ui/ui-properties.cpp
        neon/ui/ui-properties.hpp
        neon/ui/ui-scrollbars.cpp
        neon/ui/ui-scrollbars.hpp
        neon/ui/ui-style-behaviour.cpp
        neon/ui/ui-style-sheets.cpp
        neon/ui/ui-style-sheets.hpp
        neon/ui/ui-text-editor.cpp
        neon/ui/ui-text-editor.hpp
        neon/ui/ui-text-measure.cpp
        neon/ui/ui-text-measure.hpp
        neon/ui/ui-timing.cpp
        neon/ui/ui-timing.hpp
        neon/window/window-metrics.hpp

        # runtime
        neon/runtime/runtime.cpp
        neon/runtime/runtime.hpp
        neon/runtime/settings-config.hpp
        neon/runtime/frame-capture.cpp
        neon/runtime/frame-capture.hpp

        # curves
        neon/curves/bezier.hpp
        neon/curves/bezier-path.hpp
        neon/curves/cubic-bezier.hpp
        neon/curves/hanging-curve.cpp
        neon/curves/hanging-curve.hpp
        neon/curves/polyline.hpp

        # geometry built at run time
        neon/geometry/mesh-data.hpp
        neon/geometry/mesh-builder.cpp
        neon/geometry/mesh-builder.hpp
        neon/geometry/mesh-normals.cpp
        neon/geometry/mesh-normals.hpp
        neon/geometry/mesh-uvs.cpp
        neon/geometry/mesh-uvs.hpp
        neon/geometry/tube.cpp
        neon/geometry/tube.hpp
        neon/world-system/ecs/components/geometry.hpp
        neon/world-system/ecs/components/persistent.hpp
        neon/world-system/ecs/components/scene-exit.hpp
        neon/world-system/ecs/components/geometry-shape.hpp
        neon/world-system/ecs/systems/geometry-building.cpp
        neon/world-system/ecs/systems/geometry-building.hpp
        neon/world-system/ecs/components/rope.hpp
        neon/world-system/ecs/systems/rope-drawing.cpp
        neon/world-system/ecs/systems/rope-drawing.hpp

        # extensions: libraries that are loaded when the application starts
        neon/extension/extension-field.hpp
        neon/extension/extension-physics-listener.hpp
        neon/extension/extension-services.hpp
        neon/extension/extension-file.cpp
        neon/extension/extension-file.hpp
        neon/extension/extension-host.cpp
        neon/extension/extension-host.hpp
        neon/extension/extension-host-table.cpp
        neon/extension/extension-host-table.hpp
        neon/world-system/ecs/systems/extension-running.cpp
        neon/world-system/ecs/systems/extension-running.hpp
        neon/extension/extension-recipe.hpp
        neon/extension/library-loader.hpp
        neon/extension/loaded-extension.hpp
        neon/extension/native-library.hpp
        # project
        neon/project/project.hpp
        neon/project/project-file.cpp
        neon/project/project-file.hpp

        # settings
        neon/settings/settings-file.cpp
        neon/settings/settings-file.hpp

        # command line
        neon/command-line/command-line.cpp
        neon/command-line/command-line.hpp
        neon/command-line/command-line-context.hpp
        neon/command-line/command-line-options.hpp
        neon/command-line/runtime-options.cpp
        neon/command-line/runtime-options.hpp
        neon/command-line/editor-options.cpp
        neon/command-line/editor-options.hpp

        # common
        neon/common/data-buffer.hpp
        neon/common/util.hpp

        # audio
        neon/audio/audio-context.hpp
        neon/audio/audio-system.hpp
        neon/audio/headless-audio-system.cpp
        neon/audio/headless-audio-system.hpp
        neon/audio/sound-group-setting.hpp
        neon/audio/sound-info.hpp

        # data
        neon/data/data-value.cpp
        neon/data/data-value.hpp
        neon/data/document-format.hpp
        neon/data/data-reader.cpp
        neon/data/data-reader.hpp

        # reflection
        neon/reflection/entity-fields.cpp
        neon/reflection/entity-fields.hpp
        neon/reflection/field-documents.cpp
        neon/reflection/field-documents.hpp
        neon/reflection/field-value.cpp
        neon/reflection/field-value.hpp
        neon/reflection/type-builder.hpp
        neon/reflection/type-info.cpp
        neon/reflection/type-info.hpp

        # scripting: what a script declares, laid out as components of the engine
        neon/scripting/script-component-layout.cpp
        neon/scripting/script-component-layout.hpp
        neon/scripting/script-context.hpp
        neon/scripting/script-field.hpp
        neon/scripting/script-system.hpp

        # layout
        neon/layout/layout-style.hpp
        neon/layout/layout-engine.hpp
        neon/layout/flex-layout-engine.cpp
        neon/layout/flex-layout-engine.hpp

        # text
        neon/text/utf8.cpp
        neon/text/utf8.hpp
        neon/text/font-rasterizer.hpp
        neon/text/font-atlas.cpp
        neon/text/font-atlas.hpp
        neon/text/text-layout.cpp
        neon/text/text-layout.hpp

        # user interface
        neon/ui/css-values.cpp
        neon/ui/css-values.hpp
        neon/ui/ui-style.cpp
        neon/ui/ui-style.hpp
        neon/ui/ui-values.cpp
        neon/ui/ui-values.hpp
        neon/ui/ui-resources.cpp
        neon/ui/ui-resources.hpp
        neon/ui/ui-painter.cpp
        neon/ui/ui-painter.hpp
        neon/ui/ui-element.cpp
        neon/ui/ui-element.hpp
        neon/ui/ui-text.cpp
        neon/ui/ui-text.hpp
        neon/ui/elements/ui-panel.cpp
        neon/ui/elements/ui-panel.hpp
        neon/ui/elements/ui-label.cpp
        neon/ui/elements/ui-label.hpp
        neon/ui/elements/ui-image.cpp
        neon/ui/elements/ui-image.hpp
        neon/ui/elements/ui-button.cpp
        neon/ui/elements/ui-button.hpp
        neon/ui/elements/ui-bar.cpp
        neon/ui/elements/ui-bar.hpp
        neon/ui/ui-element-types.cpp
        neon/ui/ui-element-types.hpp
        neon/ui/ui-document.cpp
        neon/ui/ui-document.hpp
        neon/ui/ui-file.cpp
        neon/ui/ui-file.hpp
        neon/ui/ui-input-gate.cpp
        neon/ui/ui-input-gate.hpp
        neon/ui/ui-context.hpp
        neon/ui/ui-system.hpp
        neon/ui/tree-ui-system.cpp
        neon/ui/tree-ui-system.hpp

        # random numbers of the operating system
        neon/random/entropy-context.hpp
        neon/random/entropy-seed.cpp
        neon/random/entropy-seed.hpp

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
        neon/input/input-system.cpp
        neon/input/input-system.hpp
        neon/input/input-context.hpp
        neon/input/input-state.hpp
        neon/input/headless-input-system.cpp
        neon/input/headless-input-system.hpp

        # input maps: the actions a project plays with
        neon/input/chord.hpp
        neon/input/controller-button.hpp
        neon/input/input-action.hpp
        neon/input/input-actions.cpp
        neon/input/input-actions.hpp
        neon/input/input-map.cpp
        neon/input/input-map.hpp
        neon/input/input-map-file.cpp
        neon/input/input-map-file.hpp
        neon/input/input-map-state.hpp
        neon/input/mouse-button.hpp
        neon/input/stick.hpp
        neon/input/controller-trigger.hpp
        neon/input/sensor.hpp

        # logging
        neon/logging/logging-system.cpp
        neon/logging/logging-system.hpp
        neon/logging/logger.hpp
        neon/logging/error-count-sink.hpp
        neon/logging/spd-logger.cpp
        neon/logging/spd-logger.hpp
        neon/logging/spd-logger.cpp
        neon/logging/spd-logger.hpp
        neon/logging/logging-context.hpp
        neon/logging/log-file-target.hpp
        neon/logging/deferred-file-sink.cpp
        neon/logging/deferred-file-sink.hpp

        # common
        neon/common/color.hpp
        neon/common/color-space.cpp
        neon/common/color-space.hpp
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
        neon/render/api-version.hpp
        neon/render/tonemapper.hpp
        neon/render/render-capabilities.hpp
        neon/render/render-context.hpp
        neon/render/render-2d-context.hpp
        neon/render/mesh.hpp
        neon/render/model.cpp
        neon/render/model.hpp
        neon/render/texture-info.hpp
        neon/render/model-material.hpp
        neon/render/texture-type.hpp
        neon/render/light-source.hpp
        neon/render/sky-info.hpp
        neon/render/sky-type.hpp
        neon/render/material-info.hpp

        # physics
        neon/physics/joint-info.hpp
        neon/physics/joint-kind.hpp
        neon/physics/joint-state.hpp
        neon/physics/model-geometry.cpp
        neon/physics/model-geometry.hpp
        neon/physics/physics-context.hpp
        neon/physics/physics-system.hpp
        neon/physics/physics-types.hpp
        neon/physics/shape-cast-hit.hpp

        # world-system
        neon/world-system/world-system.hpp
        neon/world-system/ecs/entity.hpp
        neon/world-system/ecs/component-info.hpp
        neon/world-system/ecs/entity-query.hpp
        neon/world-system/ecs/entity-store.hpp
        neon/world-system/ecs/entity-system.hpp
        neon/world-system/ecs/fixed-clock.hpp
        neon/world-system/ecs/scene.hpp
        neon/world-system/ecs/scene-file/component-format.cpp
        neon/world-system/ecs/scene-file/component-format.hpp
        neon/world-system/ecs/scene-file/engine-component-formats.cpp
        neon/world-system/ecs/scene-file/physics-component-formats.cpp
        neon/world-system/ecs/scene-file/prefab-file.cpp
        neon/world-system/ecs/scene-file/prefab-file.hpp
        neon/world-system/ecs/scene-file/prefab-files.cpp
        neon/world-system/ecs/scene-file/prefab-files.hpp
        neon/world-system/ecs/scene-file/scene-file.cpp
        neon/world-system/ecs/scene-file/scene-file.hpp
        neon/world-system/ecs/entity-world.cpp
        neon/world-system/ecs/entity-world.hpp
        neon/world-system/ecs/components/camera.hpp
        neon/world-system/ecs/components/character-body.hpp
        neon/world-system/ecs/components/collider.hpp
        neon/world-system/ecs/components/joint.hpp
        neon/world-system/ecs/components/light.hpp
        neon/world-system/ecs/components/sky.hpp
        neon/world-system/ecs/components/player.hpp
        neon/world-system/ecs/components/prefab.hpp
        neon/world-system/ecs/components/renderable.hpp
        neon/world-system/ecs/components/rigid-body.hpp
        neon/world-system/ecs/components/spectator.hpp
        neon/world-system/ecs/components/sound-listener.hpp
        neon/world-system/ecs/components/sound-source.hpp
        neon/world-system/ecs/components/ui-sound-switch.hpp
        neon/world-system/ecs/components/ui-volume.hpp
        neon/world-system/ecs/systems/audio-playback.cpp
        neon/world-system/ecs/systems/audio-playback.hpp
        neon/world-system/ecs/components/trigger.hpp
        neon/world-system/ecs/systems/physics-simulation.cpp
        neon/world-system/ecs/systems/physics-simulation.hpp
        neon/world-system/ecs/systems/script-running.cpp
        neon/world-system/ecs/systems/script-running.hpp
        neon/world-system/ecs/systems/player-movement.cpp
        neon/world-system/ecs/systems/player-movement.hpp
        neon/world-system/ecs/components/ui-view.hpp
        neon/world-system/ecs/scene-file/ui-view-format.cpp
        neon/world-system/ecs/scene-file/ui-view-format.hpp
        neon/world-system/ecs/systems/ui-view-loading.cpp
        neon/world-system/ecs/systems/ui-view-loading.hpp
        neon/world-system/ecs/systems/render-submission.cpp
        neon/world-system/ecs/systems/render-submission.hpp
        neon/world-system/ecs/systems/spectator-movement.cpp
        neon/world-system/ecs/systems/spectator-movement.hpp
        neon/world-system/ecs/systems/transform-propagation.cpp
        neon/world-system/ecs/systems/transform-propagation.hpp

        # text that is shaped, and what boxes are painted with
        neon/text/text-shaper.hpp
        neon/text/glyph-atlas.cpp
        neon/text/glyph-atlas.hpp
        neon/text/text-case.cpp
        neon/text/text-case.hpp
        neon/text/shaped-text.cpp
        neon/text/shaped-text.hpp
        neon/ui/ui-paint.cpp
        neon/ui/ui-paint.hpp
        neon/ui/css-functions.cpp
        neon/ui/css-functions.hpp

        # images
        neon/image/image-pixels.cpp
        neon/image/image-pixels.hpp
        neon/image/image-decoder.hpp
        neon/image/vector-image-rasterizer.hpp
        neon/ui/ui-box-paint.cpp
        neon/ui/ui-box-paint.hpp
        neon/ui/ui-image-paint.cpp
        neon/ui/ui-image-paint.hpp
        neon/world-system/ecs/systems/ui-audio.cpp
        neon/world-system/ecs/systems/ui-audio.hpp
        neon/world-system/ecs/systems/ui-clock.cpp
        neon/world-system/ecs/systems/ui-clock.hpp

        # user interfaces on surfaces in the world
        neon/world-system/ecs/components/ui-surface-view.hpp
        neon/world-system/ecs/systems/ui-surface-loading.cpp
        neon/world-system/ecs/systems/ui-surface-loading.hpp
        neon/world-system/ecs/systems/ui-surface-pointing.cpp
        neon/world-system/ecs/systems/ui-surface-pointing.hpp
)

neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
        neon-extension-api
        glm::glm
        spdlog::spdlog
        assimp::assimp
)

if (NEON_BUILD_TESTS)
  # Mocks and fakes of the interfaces of neon-core, shared by the tests of
  # every library. They are headers, and are compiled into tests only.
  add_library(neon-testing INTERFACE
          neon/testing/fake-entity-store.hpp
          neon/testing/fake-entropy-context.hpp
          neon/testing/mock-audio-context.hpp
          neon/testing/fake-physics-context.hpp
          neon/testing/fake-font-rasterizer.hpp
          neon/testing/memory-file-system.hpp
          neon/testing/mock-entity-store.hpp
          neon/testing/mock-entity-world.hpp
          neon/testing/mock-file-system-context.hpp
          neon/testing/mock-library-loader.hpp
          neon/testing/mock-input-context.hpp
          neon/testing/recording-logging-context.hpp
          neon/testing/mock-script-context.hpp
          neon/testing/mock-input-system.hpp
          neon/testing/mock-render-2d-context.hpp
          neon/testing/mock-render-context.hpp
          neon/testing/mock-ui-system.hpp
          neon/testing/mock-render-pipeline.hpp
          neon/testing/mock-render-system.hpp
          neon/testing/mock-window-system.hpp
          neon/testing/mock-world-system.hpp
          neon/testing/recording-logger.hpp
          neon/testing/temporary-directory.hpp
          neon/testing/fake-text-shaper.hpp
          neon/testing/fake-images.hpp
  )

  target_link_libraries(neon-testing INTERFACE
          neon-core
          GTest::gmock
  )

  # Unit tests. Each one sits next to the file it tests.
  neon_add_unit_test(${TARGET} neon/ui/ui-cascade.test.cpp)
neon_add_unit_test(${TARGET} neon/ui/ui-text-measure.test.cpp)
neon_add_unit_test(${TARGET} neon/scripting/script-component-layout.test.cpp)
neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/script-running.test.cpp)
neon_add_unit_test(${TARGET} neon/command-line/display-options.test.cpp)
  neon_add_unit_test(${TARGET} neon/input/input-script.test.cpp)
  neon_add_unit_test(${TARGET} neon/reflection/field-text.test.cpp)
  neon_add_unit_test(${TARGET} neon/reflection/field-numbers.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/css/css-expression.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/css/css-media.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/css/css-selector.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/css/css-style-sheet.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-properties.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-style-behaviour.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-style-sheets.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-text-editor.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-timing.test.cpp)
  neon_add_unit_test(${TARGET} neon/window/window-metrics.test.cpp)
  neon_add_unit_test(${TARGET} neon/command-line/command-line.test.cpp)
  neon_add_unit_test(${TARGET} neon/reflection/entity-fields.test.cpp)
  neon_add_unit_test(${TARGET} neon/reflection/field-documents.test.cpp)
  neon_add_unit_test(${TARGET} neon/reflection/type-info.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/scene-file/engine-component-formats.test.cpp)
  neon_add_unit_test(${TARGET} neon/command-line/runtime-options.test.cpp)
  neon_add_unit_test(${TARGET} neon/command-line/editor-options.test.cpp)
  neon_add_unit_test(${TARGET} neon/common/color-space.test.cpp)
  neon_add_unit_test(${TARGET} neon/common/data-buffer.test.cpp)
  neon_add_unit_test(${TARGET} neon/common/rotation.test.cpp)
  neon_add_unit_test(${TARGET} neon/common/transform.test.cpp)
  neon_add_unit_test(${TARGET} neon/common/util.test.cpp)
  neon_add_unit_test(${TARGET} neon/data/data-reader.test.cpp)
  neon_add_unit_test(${TARGET} neon/filesystem/file-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/random/entropy-seed.test.cpp)
  neon_add_unit_test(${TARGET} neon/input/headless-input-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/input/input-actions.test.cpp)
  neon_add_unit_test(${TARGET} neon/input/input-map.test.cpp)
  neon_add_unit_test(${TARGET} neon/input/input-state.test.cpp)
  neon_add_unit_test(${TARGET} neon/layout/flex-layout-engine.test.cpp)
  neon_add_unit_test(${TARGET} neon/audio/headless-audio-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/logging/logging-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/logging/deferred-file-sink.test.cpp)
  neon_add_unit_test(${TARGET} neon/logging/spd-logger.test.cpp)
  neon_add_unit_test(${TARGET} neon/physics/model-geometry.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/forward-render-pipeline.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/model.test.cpp)
  neon_add_unit_test(${TARGET} neon/curves/bezier-path.test.cpp)
  neon_add_unit_test(${TARGET} neon/curves/cubic-bezier.test.cpp)
  neon_add_unit_test(${TARGET} neon/curves/hanging-curve.test.cpp)
  neon_add_unit_test(${TARGET} neon/curves/polyline.test.cpp)
  neon_add_unit_test(${TARGET} neon/geometry/mesh-builder.test.cpp)
  neon_add_unit_test(${TARGET} neon/geometry/mesh-normals.test.cpp)
  neon_add_unit_test(${TARGET} neon/geometry/tube.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/rope-drawing.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/geometry-building.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/api-version.test.cpp)
  neon_add_unit_test(${TARGET} neon/runtime/frame-capture.test.cpp)
  neon_add_unit_test(${TARGET} neon/runtime/runtime.test.cpp)
  neon_add_unit_test(${TARGET} neon/text/font-atlas.test.cpp)
  neon_add_unit_test(${TARGET} neon/text/text-layout.test.cpp)
  neon_add_unit_test(${TARGET} neon/text/utf8.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/css-values.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-document.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-element-types.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-input-gate.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-painter.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-resources.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-style.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-values.test.cpp)
  neon_add_unit_test(${TARGET} neon/window/headless-window-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/component-info.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/entity-store.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/entity-world.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/fixed-clock.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/scene-file/physics-component-formats.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/audio-playback.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/physics-simulation.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/render-submission.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/player-movement.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/spectator-movement.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/transform-propagation.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/ui-view-loading.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/ui-surface-pointing.test.cpp)
  neon_add_unit_test(${TARGET} neon/image/image-pixels.test.cpp)
  neon_add_unit_test(${TARGET} neon/text/glyph-atlas.test.cpp)
  neon_add_unit_test(${TARGET} neon/text/shaped-text.test.cpp)
  neon_add_unit_test(${TARGET} neon/text/text-case.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/css-functions.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-paint.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-box-paint.test.cpp)
  neon_add_unit_test(${TARGET} neon/ui/ui-values-fallback.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/forward-render-pipeline-cameras.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/ui-audio.test.cpp)
  neon_add_unit_test(${TARGET} neon/world-system/ecs/systems/ui-clock.test.cpp)
endif ()
