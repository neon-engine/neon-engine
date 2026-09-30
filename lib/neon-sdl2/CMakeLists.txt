set(TARGET neon-sdl2)

add_library(${TARGET} STATIC
        neon/filesystem/sdl2-file-system.cpp
        neon/filesystem/sdl2-file-system.hpp
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
)

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/filesystem/sdl2-file-system.test.cpp)

  # assets:// is the folder `assets` next to the executable, which for a test
  # is in the build tree. What the test expects to find there is written when
  # the project is configured.
  set(TEST_ASSETS "${NEON_UNIT_TEST_DIRECTORY}/assets")
  file(REMOVE_RECURSE "${TEST_ASSETS}")
  file(WRITE "${TEST_ASSETS}/readme.txt" "at the top\n")
  file(WRITE "${TEST_ASSETS}/empty.txt" "")
  file(WRITE "${TEST_ASSETS}/models/cube.obj" "a cube\n")
  file(WRITE "${TEST_ASSETS}/models/props/chair.obj" "a chair\n")
  file(WRITE "${TEST_ASSETS}/Textures/Wood-Oak.PNG" "a texture\n")
endif ()
