set(TARGET neon-sdl2)

add_library(${TARGET} STATIC
        neon/input/sdl2-clipboard.cpp
        neon/input/sdl2-clipboard.hpp
        neon/filesystem/sdl2-file-system.cpp
        neon/filesystem/sdl2-file-system.hpp
        neon/extension/sdl2-library-loader.cpp
        neon/extension/sdl2-library-loader.hpp
        neon/extension/sdl2-native-library.cpp
        neon/extension/sdl2-native-library.hpp
        neon/window/sdl2-window-system.cpp
        neon/window/sdl2-window-system.hpp
        neon/input/sdl2-input-system.cpp
        neon/input/sdl2-input-system.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# what this platform is called where a library is named for it, which is
# what the build names the libraries of extensions by, see NeonExtensions.cmake
target_compile_definitions(${TARGET} PRIVATE NEON_PLATFORM="${NEON_PLATFORM}")

target_link_libraries(${TARGET}
        neon-core
        SDL2::SDL2-static
)

# The entry point of an application, apart from the library: SDL2main is what
# the platform starts the application by, WinMain on Windows, and it calls a
# main() that the application defines. An application links this next to
# neon-sdl2 and includes neon/application/sdl2-main.hpp where its main() is.
# A test links neon-sdl2 alone: it has the main() of GoogleTest, and none for
# SDL2main to call, which fails to link for Windows.
add_library(neon-sdl2-main INTERFACE
        main/neon/application/sdl2-main.hpp
)

target_include_directories(neon-sdl2-main
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/main
)

target_link_libraries(neon-sdl2-main
        INTERFACE
        ${TARGET}
        SDL2::SDL2main
)

if (NEON_BUILD_TESTS)
  # these start SDL with its video driver that draws nowhere, and are
  # skipped where SDL was built without it
  neon_add_unit_test(${TARGET} neon/input/sdl2-input-system.test.cpp)
  neon_add_unit_test(${TARGET} neon/input/sdl2-clipboard.test.cpp)
  neon_add_unit_test(${TARGET} neon/window/sdl2-window-system.test.cpp)

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
