set(TARGET neon-sdl2)

add_library(${TARGET} STATIC
        neon/window/sdl2-window-system.cpp
        neon/window/sdl2-window-system.hpp
        neon/input/sdl2-input-system.cpp
        neon/input/sdl2-input-system.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
        neon-core
        SDL2::SDL2main
        SDL2::SDL2-static
        glad
)
