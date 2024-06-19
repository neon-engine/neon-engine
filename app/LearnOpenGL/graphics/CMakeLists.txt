add_library(graphics STATIC
        shader.cpp
        shader.hpp
        texture.cpp
        texture.hpp
        triangle.cpp
        triangle.hpp
        plane.cpp
        plane.hpp
        cube.cpp
        cube.hpp
        camera.cpp
        camera.hpp
        graphics.hpp
)

target_include_directories(graphics PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(graphics PRIVATE gl3w glm)
