set(TARGET neon-vulkan)

add_library(${TARGET} STATIC
        neon/render/vulkan-render-system.cpp
        neon/render/vulkan-render-system.hpp
        neon/render/vk-material.cpp
        neon/render/vk-material.hpp
        neon/render/vk-mesh.cpp
        neon/render/vk-mesh.hpp
        neon/render/vk-model.cpp
        neon/render/vk-model.hpp
        neon/render/vk-shader.cpp
        neon/render/vk-shader.hpp
        neon/render/vk-texture.cpp
        neon/render/vk-texture.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
        glm::glm
        stb
        neon-core
)
