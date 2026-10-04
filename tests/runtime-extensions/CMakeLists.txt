# What an extension makes is drawn: a project of the test's own, put together
# next to a copy of the runtime, whose extension paints a picture on a square,
# read pixel by pixel from what the runtime saved. Needs a graphics card with
# a Vulkan driver. On a machine without one the test is skipped.

set(GAME_DIRECTORY "${NEON_TESTS_DIRECTORY}/runtime-extensions/game")

neon_add_project(canvas
        SOURCES extensions/canvas/canvas.cpp
        DIRECTORY "${GAME_DIRECTORY}"
)
add_dependencies(neon-tests canvas)

# as neon_add_application_test, for an application that is put together and
# is no target of its own
if (NOT CMAKE_CROSSCOMPILING)
  foreach (CASE IN ITEMS a-mesh-a-picture-and-a-lightmap-of-an-extension-are-drawn)
    add_test(
            NAME "runtime-extensions.${CASE}"
            COMMAND "${CMAKE_COMMAND}"
            "-DAPPLICATION=${GAME_DIRECTORY}/NeonRuntime${CMAKE_EXECUTABLE_SUFFIX}"
            "-DDIRECTORY=${NEON_TESTS_DIRECTORY}/runtime-extensions/${CASE}"
            "-DCASE=${CASE}"
            "-DPIXEL_PROBE=$<TARGET_FILE:pixel-probe>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run.cmake"
    )
    set_tests_properties("runtime-extensions.${CASE}" PROPERTIES
            LABELS "integration"
            SKIP_REGULAR_EXPRESSION "SKIPPED:"
            TIMEOUT 120
    )
  endforeach ()
endif ()
