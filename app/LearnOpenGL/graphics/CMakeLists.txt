add_library(graphics STATIC
        shader.cpp
        texture.cpp
        triangle.cpp
        plane.cpp
)

target_include_directories(graphics PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(graphics PRIVATE gl3w)
