set(TARGET core)

add_library(${TARGET} STATIC
        core/application.cpp
        core/application.hpp
        base/render-system.hpp
        base/input-system.hpp
        core/settings-config.hpp
        base/window-system.hpp
        core/glfw-window-system.cpp
        core/glfw-window-system.hpp
        core/glfw-input-system.cpp
        core/glfw-input-system.hpp
        core/opengl-render-system.cpp
        core/opengl-render-system.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/base
)

target_link_libraries(${TARGET} glfw glm gl3w)