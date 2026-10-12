# NeonEditor: the editor, the second composition root next to NeonRuntime.
# Its own objects, what a person asks the editor to do, are the library
# neon-editor, free of any user interface library and done on the
# interfaces of neon-core, so that the window, a command line, a test, and a
# service call the same thing. New files are listed by hand.
add_library(neon-editor STATIC
        neon/editor/templates/new-project.hpp
        neon/editor/templates/project-template.cpp
        neon/editor/templates/project-template.hpp
        neon/editor/templates/project-template-text.cpp
        neon/editor/templates/project-template-text.hpp
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
