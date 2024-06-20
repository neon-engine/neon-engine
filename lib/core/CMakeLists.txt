set(TARGET core)

add_library(${TARGET} STATIC
        core/application.cpp
        core/application.hpp
        core/render-system.cpp
        core/render-system.hpp
        core/input-system.cpp
        core/input-system.hpp
        core/settings-config.hpp
        core/window-system.cpp
        core/window-system.hpp
        private/glfw-window-system.cpp
        private/glfw-window-system.hpp
        private/glfw-input-system.cpp
        private/glfw-input-system.hpp
        private/opengl-render-system.cpp
        private/opengl-render-system.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

target_link_libraries(${TARGET} glfw glm gl3w)