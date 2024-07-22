set(TARGET NeonFPS)

add_executable(${TARGET} main.cpp
        neon-fps-application.cpp
        neon-fps-application.hpp)

target_link_libraries(${TARGET} PRIVATE core)

file(GLOB_RECURSE ASSETS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

add_custom_target(${TARGET}_copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${BASE_APP_BIN_DIRECTORY}/${TARGET}/assets"
        COMMENT "Copying assets to ${BASE_APP_BIN_DIRECTORY}"
        DEPENDS ${ASSETS_FILES}
)

add_dependencies(${TARGET} ${TARGET}_copy_assets)
