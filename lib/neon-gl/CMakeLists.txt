set(TARGET neon-gl)

add_library(${TARGET} STATIC
        neon/render/opengl-material.cpp
        neon/render/opengl-material.hpp
        neon/render/opengl-mesh.cpp
        neon/render/opengl-mesh.hpp
        neon/render/opengl-model.cpp
        neon/render/opengl-model.hpp
        neon/render/opengl-render-system.cpp
        neon/render/opengl-render-system.hpp
        neon/render/opengl-shader.cpp
        neon/render/opengl-shader.hpp
        neon/render/opengl-texture.cpp
        neon/render/opengl-texture.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
        glm::glm
        assimp::assimp
        glad
        stb
        neon-core
)
