add_library(graphics STATIC
        shader.cpp
        triangle.cpp
        plane.cpp
)

target_link_libraries(graphics PRIVATE gl3w)

add_custom_command(
        TARGET graphics POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "${BASE_APP_BIN_DIRECTORY}/${TARGET}/shaders"
        COMMENT "Copying shaders to ${BASE_APP_BIN_DIRECTORY}"
)
