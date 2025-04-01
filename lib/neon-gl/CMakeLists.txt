set(TARGET neon-gl)

add_library(${TARGET} STATIC
        neon/render/gl-material.cpp
        neon/render/gl-material.hpp
        neon/render/gl-mesh.cpp
        neon/render/gl-mesh.hpp
        neon/render/gl-model.cpp
        neon/render/gl-model.hpp
        neon/render/gl-render-system.cpp
        neon/render/gl-render-system.hpp
        neon/render/gl-shader.cpp
        neon/render/gl-shader.hpp
        neon/render/gl-texture.cpp
        neon/render/gl-texture.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
        glm::glm
        glad
        stb
        neon-core
)
