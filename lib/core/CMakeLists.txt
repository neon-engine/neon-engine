set(TARGET core)

add_library(${TARGET} STATIC
        core/application.cpp
        core/application.hpp
        core/input-manager.hpp
        core/renderer.hpp
        core/window-manager.hpp
        private/opengl-renderer.cpp
        private/opengl-renderer.hpp
        private/glfw-window-manager.cpp
        private/glfw-window-manager.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)