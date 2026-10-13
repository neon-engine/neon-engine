# The editor: the second composition root next to NeonRuntime, on the same
# shared start-up, neon-runtime, with the editor's own objects and the tool
# interface of neon-imgui on top. See docs/editor.md.
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

        # a script that drives the tools by their labels, and what runs it
        # frame by frame, see docs/editor.md#scripted-runs
        neon/editor/scripts/editor-script.cpp
        neon/editor/scripts/editor-script.hpp
        neon/editor/scripts/editor-script-loading.cpp
        neon/editor/scripts/editor-script-loading.hpp
        neon/editor/scripts/editor-script-runner.cpp
        neon/editor/scripts/editor-script-runner.hpp

        # the windows, drawn with the tool interface of neon-core, and the
        # registry the View menu shows and hides them from
        neon/editor/ui/editor-log.cpp
        neon/editor/ui/editor-log.hpp
        neon/editor/ui/editor-viewport.cpp
        neon/editor/ui/editor-viewport.hpp
        neon/editor/ui/editor-window.hpp
        neon/editor/ui/editor-windows.cpp
        neon/editor/ui/editor-windows.hpp

        # the editor's own folder, editor://, with its settings and the
        # layout of its windows, see docs/editor.md
        neon/editor/settings/editor-folder.cpp
        neon/editor/settings/editor-folder.hpp
        neon/editor/settings/editor-settings.hpp
        neon/editor/settings/editor-settings-file.cpp
        neon/editor/settings/editor-settings-file.hpp

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
        neon/editor/toolchains/toolchain-files.cpp
        neon/editor/toolchains/toolchain-files.hpp
        neon/editor/toolchains/toolchain-kind.hpp
        neon/editor/toolchains/toolchain-placeholders.cpp
        neon/editor/toolchains/toolchain-placeholders.hpp
        neon/editor/toolchains/toolchain-run.cpp
        neon/editor/toolchains/toolchain-run.hpp
        neon/editor/toolchains/toolchain-step.hpp
        neon/editor/toolchains/toolchains.cpp
        neon/editor/toolchains/toolchains.hpp

        # what opens the code of the project in another program, see
        # docs/editor.md
        neon/editor/tools/external-editor.cpp
        neon/editor/tools/external-editor.hpp
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
  neon_add_unit_test(neon-editor neon/editor/ui/editor-windows.test.cpp)
  neon_add_unit_test(neon-editor neon/editor/settings/editor-settings-file.test.cpp LIBRARIES neon-ryml)
  neon_add_unit_test(neon-editor neon/editor/settings/editor-folder.test.cpp LIBRARIES neon-ryml)
  neon_add_unit_test(neon-editor neon/editor/scripts/editor-script.test.cpp LIBRARIES neon-ryml)
  neon_add_unit_test(neon-editor neon/editor/scripts/editor-script-loading.test.cpp LIBRARIES neon-ryml)
  neon_add_unit_test(neon-editor neon/editor/scripts/editor-script-runner.test.cpp LIBRARIES neon-ryml)
  neon_add_unit_test(neon-editor neon/editor/templates/project-template.test.cpp)
  neon_add_unit_test(neon-editor neon/editor/templates/project-template-text.test.cpp)
  neon_add_unit_test(neon-editor neon/editor/toolchains/toolchain-files.test.cpp LIBRARIES neon-ryml)
  neon_add_unit_test(neon-editor neon/editor/tools/external-editor.test.cpp)
endif ()

set(TARGET NeonEditor)

# The composition root, and the editor's choices on the shared start-up in
# EditorHooks, which is where the tools of neon-imgui are made, so that
# neon-editor stays free of them. neon-sdl2-main is what the platform
# starts the application by, and has to be linked into the executable.
add_executable(${TARGET} main.cpp
        editor-hooks.cpp
        editor-hooks.hpp)

target_link_libraries(${TARGET} PRIVATE neon-runtime neon-editor neon-imgui neon-sdl2-main)
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
