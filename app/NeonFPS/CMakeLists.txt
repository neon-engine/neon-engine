set(TARGET NeonFPS)

add_executable(${TARGET} main.cpp
        neon-fps-application.cpp
        neon-fps-application.hpp)

target_link_libraries(${TARGET} PRIVATE core)

setup_copy_assets(
        ${TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${APP_BIN_DIRECTORY}/assets"
)
