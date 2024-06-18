set(TARGET LearnOpenGL)

add_subdirectory(graphics)

add_executable(${TARGET} main.cpp)

target_link_libraries(${TARGET} PRIVATE glm glfw gl3w)
target_link_libraries(${TARGET} PRIVATE graphics)

file(GLOB_RECURSE ASSETS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${BASE_APP_BIN_DIRECTORY}/${TARGET}/assets"
        COMMENT "Copying shaders to ${BASE_APP_BIN_DIRECTORY}"
        DEPENDS ${ASSETS_FILES}
)

add_dependencies(LearnOpenGL copy_assets)
