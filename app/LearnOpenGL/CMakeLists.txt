set(TARGET LearnOpenGL)

add_subdirectory(graphics)

add_executable(${TARGET} main.cpp)

target_link_libraries(${TARGET} PRIVATE glm glfw gl3w)
target_link_libraries(${TARGET} PRIVATE graphics)

message("${APP_BIN_DIRECTORY}")
message("${BASE_APP_BIN_DIRECTORY}")

add_custom_command(
        TARGET LearnOpenGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${BASE_APP_BIN_DIRECTORY}/${TARGET}/assets"
        COMMENT "Copying shaders to ${BASE_APP_BIN_DIRECTORY}"
)
