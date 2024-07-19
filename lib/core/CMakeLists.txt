set(TARGET core)

add_library(${TARGET} STATIC
        core/application.cpp
        core/application.hpp
        core/settings-config.hpp

        core/input/input-system.hpp
        core/input/glfw-input-system.cpp
        core/input/glfw-input-system.hpp
        core/render/render-system.hpp
        core/render/opengl-render-system.cpp
        core/render/opengl-render-system.hpp

        core/window/window-system.hpp
        core/window/glfw-window-system.cpp
        core/window/glfw-window-system.hpp
        core/scene/scene.cpp
        core/scene/scene.hpp
        core/entity/spectator.cpp
        core/entity/spectator.hpp
        core/render/render-context.hpp

        core/private/neon.hpp
        core/window/sdl2-window-system.cpp
        core/window/sdl2-window-system.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/entity
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/input
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/render
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/scene
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/window
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

target_link_libraries(${TARGET} glfw glm gl3w)
target_precompile_headers(${TARGET} INTERFACE core/private/neon.hpp)
