set(TARGET LearnOpenGL)

add_executable(${TARGET} main.cpp
        graphics/shader.cpp
        graphics/triangle.cpp
        graphics/plane.cpp
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

add_custom_command(
        TARGET LearnOpenGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${BASE_APP_BIN_DIRECTORY}/${TARGET}/assets"
        COMMENT "Copying shaders to ${BASE_APP_BIN_DIRECTORY}"
)
