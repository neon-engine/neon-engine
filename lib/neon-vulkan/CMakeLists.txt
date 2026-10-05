set(TARGET neon-vulkan)

add_library(${TARGET} STATIC
        neon/render/vk-api-version.cpp
        neon/render/vk-api-version.hpp
        neon/render/vk-capabilities.cpp
        neon/render/vk-capabilities.hpp
        neon/render/vk-device-answers.hpp
        neon/render/vk-device.cpp
        neon/render/vk-device.hpp
        neon/render/vk-material.cpp
        neon/render/vk-material.hpp
        neon/render/vk-mesh.cpp
        neon/render/vk-mesh.hpp
        neon/render/vk-model.cpp
        neon/render/vk-model.hpp
        neon/render/vk-material-cache.cpp
        neon/render/vk-material-cache.hpp
        neon/render/vk-model-cache.cpp
        neon/render/vk-model-cache.hpp
        neon/render/vk-renderer-2d.cpp
        neon/render/vk-renderer-2d.hpp
        neon/render/vk-shader.cpp
        neon/render/vk-shader.hpp
        neon/render/vk-shader-data.hpp
        neon/render/vk-texture.cpp
        neon/render/vk-texture.hpp
        neon/render/vk-texture-cache.cpp
        neon/render/vk-texture-cache.hpp
        neon/render/vk-render-system.cpp
        neon/render/vk-render-system.hpp
        neon/render/vk-shader-values.cpp
        neon/render/vk-shader-values.hpp
        neon/render/vk-render-target.cpp
        neon/render/vk-render-target.hpp
        neon/render/vk-swapchain-sizing.cpp
        neon/render/vk-swapchain-sizing.hpp
        neon/render/vk-culling.cpp
        neon/render/vk-culling.hpp
        neon/render/vk-draw.hpp
        neon/render/vk-draw-batch.hpp
        neon/render/vk-draw-order.cpp
        neon/render/vk-draw-order.hpp
        neon/render/vk-draw-queue.cpp
        neon/render/vk-draw-queue.hpp
        neon/render/vk-frame-stages.cpp
        neon/render/vk-frame-stages.hpp
        neon/render/vk-resolve.cpp
        neon/render/vk-present-mode.cpp
        neon/render/vk-effects.cpp
        neon/render/vk-effect-images.cpp
        neon/render/vk-resolve.hpp
        neon/render/vk-samplers.cpp
        neon/render/vk-samplers.hpp
        neon/render/vk-sampling.hpp
        neon/render/vk-scene-image.cpp
        neon/render/vk-scene-image.hpp
        neon/render/vk-shadow-batch.hpp
        neon/render/vk-shadow-cascades.hpp
        neon/render/vk-shadow-casting.cpp
        neon/render/vk-shadow-casting.hpp
        neon/render/vk-sky.cpp
        neon/render/vk-sky.hpp
        neon/render/vk-sky-draw.hpp
        neon/render/vk-sky-values.hpp
        neon/render/vk-shadow-fit.cpp
        neon/render/vk-shadow-fit.hpp
        neon/render/vk-shadow-map.cpp
        neon/render/vk-shadow-map.hpp
        neon/render/vk-surface-format.cpp
        neon/render/vk-surface-format.hpp
        neon/render/vk-swapchain.cpp
        neon/render/vk-swapchain.hpp
        neon/render/vk-pipelines.cpp
        neon/render/vk-pipelines.hpp
        neon/render/vk-canvas.cpp
        neon/render/vk-canvas.hpp
        neon/render/vk-canvas-shared.hpp
        neon/render/vk-capture.cpp
        neon/render/vk-capture.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
        glm::glm
        stb
        volk
        neon-core
        ${CMAKE_DL_LIBS}
)

if (NEON_BUILD_TESTS)
  # Only what needs no graphics card: what is worked out before the first
  # call to Vulkan, and what happens when a file cannot be used.
  neon_add_unit_test(${TARGET} neon/render/vk-material.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-material-cache.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-mesh.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-model.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-renderer-2d.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-shader.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-shader-data.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-texture.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-shader-values.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-render-target.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-pipelines.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-resolve.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-present-mode.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-effects.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-samplers.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-sky.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-swapchain-sizing.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-culling.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-shadow-fit.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-draw-order.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-draw-queue.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-shadow-casting.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-frame-stages.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-surface-format.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-api-version.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-capabilities.test.cpp)
endif ()
