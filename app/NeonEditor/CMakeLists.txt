# NeonEditor: the editor, the second composition root next to NeonRuntime.
# Its own objects, what a person asks the editor to do, are the library
# neon-editor, free of any user interface library and done on the
# interfaces of neon-core, so that the window, a command line, a test, and a
# service call the same thing. New files are listed by hand, grouped by
# folder.
add_library(neon-editor STATIC
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
  neon_add_unit_test(neon-editor neon/editor/templates/project-template.test.cpp)
  neon_add_unit_test(neon-editor neon/editor/templates/project-template-text.test.cpp)
endif ()
