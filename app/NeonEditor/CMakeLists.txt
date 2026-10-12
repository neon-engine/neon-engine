# The editor: the second composition root next to NeonRuntime, on the same
# libraries, with the editor's own objects and the tool interface of
# neon-imgui on top. See docs/editor.md.
#
# The editor's objects are the library neon-editor, which the executable and
# the tests link; main.cpp is the composition root and nothing more. They
# are free of any user interface library and done on the interfaces of
# neon-core, so that the window, a command line, a test, and a service call
# the same thing. The sources are under neon/editor/<area>/, included as
# <neon/editor/...>, and are listed here plainly, one file a line, grouped
# by folder.
add_library(neon-editor STATIC
        # the runtime with the tools on top: the loop and the menu bar
        neon/editor/editor.cpp
        neon/editor/editor.hpp

        # the camera of the viewport, and how the world is told to draw
        # with it
        neon/editor/camera/editor-camera.cpp
        neon/editor/camera/editor-camera.hpp
        neon/editor/camera/editor-camera-input.hpp
        neon/editor/camera/editor-camera-submission.cpp
        neon/editor/camera/editor-camera-submission.hpp

        # the windows, drawn with the tool interface of neon-core
        neon/editor/ui/editor-log.cpp
        neon/editor/ui/editor-log.hpp
        neon/editor/ui/editor-viewport.cpp
        neon/editor/ui/editor-viewport.hpp

        # a new project written outside the engine, see docs/new-projects.md
        neon/editor/templates/new-project.hpp
        neon/editor/templates/project-template.cpp
        neon/editor/templates/project-template.hpp
        neon/editor/templates/project-template-text.cpp
        neon/editor/templates/project-template-text.hpp
        neon/editor/toolchains/build-target.hpp
        neon/editor/toolchains/toolchain.hpp
        neon/editor/toolchains/toolchain-check.cpp
        neon/editor/toolchains/toolchain-check.hpp
        neon/editor/toolchains/toolchain-file.cpp
        neon/editor/toolchains/toolchain-file.hpp
        neon/editor/toolchains/toolchain-kind.hpp
        neon/editor/toolchains/toolchain-placeholders.cpp
        neon/editor/toolchains/toolchain-placeholders.hpp
        neon/editor/toolchains/toolchain-run.cpp
        neon/editor/toolchains/toolchain-run.hpp
        neon/editor/toolchains/toolchain-step.hpp
        neon/editor/toolchains/toolchains.cpp
        neon/editor/toolchains/toolchains.hpp
)
neon_warnings(neon-editor)

target_include_directories(neon-editor
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(neon-editor
        PUBLIC neon-core
)

if (NEON_BUILD_TESTS)
  neon_add_unit_test(neon-editor neon/editor/camera/editor-camera.test.cpp)
  neon_add_unit_test(neon-editor neon/editor/ui/editor-viewport.test.cpp)
  neon_add_unit_test(neon-editor neon/editor/ui/editor-log.test.cpp)
  neon_add_unit_test(neon-editor neon/editor/templates/project-template.test.cpp)
  neon_add_unit_test(neon-editor neon/editor/templates/project-template-text.test.cpp)
endif ()

set(TARGET NeonEditor)

add_executable(${TARGET} main.cpp)

target_link_libraries(${TARGET} PRIVATE neon-core neon-editor neon-imgui neon-vulkan neon-sdl2 neon-sdl2-main neon-flecs neon-ryml neon-miniaudio neon-jolt neon-lua neon-stb neon-freetype neon-lunasvg neon-platform)
neon_warnings(${TARGET})

# What the editor ships next to it as engine://, the same as the runtime
# ships: the sources are kept with the runtime, app/NeonRuntime/engine, and
# are copied and compiled here for the editor's own folder. The editor
# opens a project folder, so no assets:// is next to it.
set(ENGINE_SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/app/NeonRuntime/engine")

setup_copy_assets(
        ${TARGET}
        "${ENGINE_SOURCE_DIRECTORY}"
        "${APP_BIN_DIRECTORY}/engine"
        PART engine
        EXCLUDE "\\.(vert|frag|glsl)$"
)

setup_compile_shaders(
        ${TARGET}
        "${ENGINE_SOURCE_DIRECTORY}/shaders"
        "${APP_BIN_DIRECTORY}/engine/shaders"
        # the shaders of elements of a user interface, and effects of cameras
        FOLDERS ui effects
)
