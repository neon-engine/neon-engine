set(TARGET neon-vulkan)

add_library(${TARGET} STATIC
        neon/render/vk-device.cpp
        neon/render/vk-device.hpp
        neon/render/vk-material.cpp
        neon/render/vk-material.hpp
        neon/render/vk-mesh.cpp
        neon/render/vk-mesh.hpp
        neon/render/vk-model.cpp
        neon/render/vk-model.hpp
        neon/render/vk-renderer-2d.cpp
        neon/render/vk-renderer-2d.hpp
        neon/render/vk-shader.cpp
        neon/render/vk-shader.hpp
        neon/render/vk-shader-data.hpp
        neon/render/vk-texture.cpp
        neon/render/vk-texture.hpp
        neon/render/vk-render-system.cpp
        neon/render/vk-render-system.hpp
)

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
  neon_add_unit_test(${TARGET} neon/render/vk-mesh.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-model.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-renderer-2d.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-shader.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-shader-data.test.cpp)
  neon_add_unit_test(${TARGET} neon/render/vk-texture.test.cpp)
endif ()
