add_library(core STATIC
        application.cpp
        window-manager.cpp
        window-manager.hpp
        input-manager.cpp
        input-manager.hpp
        renderer.cpp
        renderer.hpp
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
