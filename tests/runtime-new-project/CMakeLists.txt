# A project the editor makes builds and runs: new-project, the command line
# of ProjectTemplate, writes one into the folder of the test, the folder is
# configured and built on its own against the runtime this build made, as a
# person builds a project, and NeonEditor runs the game put together without
# a window for a few frames, since the runtime a player gets takes no
# --frames or --screenshot (#310). Needs a graphics card with a Vulkan
# driver. On a machine without one the test is skipped.

# the command line of the generator, which is also how a project is made by
# hand until NeonEditor has --new-project
add_executable(new-project EXCLUDE_FROM_ALL new-project.cpp)
neon_warnings(new-project)
target_link_libraries(new-project PRIVATE neon-editor neon-platform neon-core)
# the engine a project is made against unless told otherwise: this checkout
target_compile_definitions(new-project PRIVATE NEON_ENGINE_DIRECTORY="${CMAKE_SOURCE_DIR}")
add_dependencies(neon-tests new-project NeonRuntime NeonEditor)

# as neon_add_application_test, for an application that is built by the
# test itself
if (NOT CMAKE_CROSSCOMPILING)
  foreach (CASE IN ITEMS a-new-project-builds-and-runs)
    add_test(
            NAME "runtime-new-project.${CASE}"
            COMMAND "${CMAKE_COMMAND}"
            "-DNEW_PROJECT=$<TARGET_FILE:new-project>"
            "-DRUNTIME_DIRECTORY=$<TARGET_FILE_DIR:NeonRuntime>"
            "-DAPPLICATION=$<TARGET_FILE:NeonEditor>"
            "-DENGINE_DIRECTORY=${CMAKE_SOURCE_DIR}"
            "-DGENERATOR=${CMAKE_GENERATOR}"
            "-DC_COMPILER=${CMAKE_C_COMPILER}"
            "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DDIRECTORY=${NEON_TESTS_DIRECTORY}/runtime-new-project/${CASE}"
            "-DCASE=${CASE}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run.cmake"
    )
    set_tests_properties("runtime-new-project.${CASE}" PROPERTIES
            LABELS "integration"
            SKIP_REGULAR_EXPRESSION "SKIPPED:"
            # a configure, a build of one file, and a run of a few frames
            TIMEOUT 300
    )
  endforeach ()
endif ()
