set(TARGET core)

add_library(${TARGET} STATIC
        core/application.cpp
        core/application.hpp
        core/render-system.cpp
        core/render-system.hpp
        core/input-system.cpp
        core/input-system.hpp
        core/window-info.hpp
        core/window-system.cpp
        core/window-system.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

target_link_libraries(${TARGET} glfw glm gl3w)