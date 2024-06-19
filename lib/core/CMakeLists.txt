add_library(core STATIC
        application.cpp
#        application.hpp
#        window-manager.hpp
#        input-manager.hpp
#        renderer.hpp
#        impl/glfw-window-manager.cpp
        impl/gl-renderer.cpp
)

target_include_directories(core
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../impl
)
