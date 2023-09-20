set(TARGET LearnOpenGL)

add_executable(${TARGET} main.cpp
        graphics/shaders/shader.cpp
        graphics/geometry/triangle.cpp
)

target_link_libraries(${TARGET} PRIVATE glm glfw gl3w)

message("${APP_BIN_DIRECTORY}")
message("${BASE_APP_BIN_DIRECTORY}")

add_custom_command(
        TARGET LearnOpenGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/graphics/shaders"
        "${BASE_APP_BIN_DIRECTORY}/${TARGET}/shaders"
        COMMENT "Copying shaders to ${BASE_APP_BIN_DIRECTORY}"
)
