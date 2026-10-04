# A shader an extension brings is told the time and the numbers of the game:
# a project of the test's own, put together next to a copy of the runtime,
# whose extension draws a square with a shader of its own, read pixel by
# pixel from what the runtime saved. Needs a graphics card with a Vulkan
# driver. On a machine without one the test is skipped.

set(GAME_DIRECTORY "${NEON_TESTS_DIRECTORY}/runtime-shaders/game")

neon_add_project(tinted
        SOURCES extensions/tinted/tinted.cpp
        DIRECTORY "${GAME_DIRECTORY}"
)
add_dependencies(neon-tests tinted)

# The shader of the extension is compiled as an extension outside the engine
# compiles its own: with glslang, against the declarations the shaders of
# the engine share, into the assets of the extension next to the runtime.
find_program(GLSLANG_EXECUTABLE NAMES glslang glslangValidator)
set(TINTED_SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/extensions/tinted/shaders")
set(TINTED_SHADER_OUTPUT "${GAME_DIRECTORY}/extensions/tinted/assets/shaders")
file(GLOB ENGINE_SHADER_INCLUDES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/app/NeonRuntime/shaders/vulkan/*.glsl")

set(TINTED_SHADER_OUTPUTS)
foreach (STAGE IN ITEMS vert frag)
  add_custom_command(
          OUTPUT "${TINTED_SHADER_OUTPUT}/tinted.${STAGE}.spv"
          COMMAND ${CMAKE_COMMAND} -E make_directory "${TINTED_SHADER_OUTPUT}"
          COMMAND "${GLSLANG_EXECUTABLE}" -V --quiet "-I${CMAKE_SOURCE_DIR}/app/NeonRuntime/shaders/vulkan"
          "${TINTED_SHADER_SOURCES}/tinted.${STAGE}" -o "${TINTED_SHADER_OUTPUT}/tinted.${STAGE}.spv"
          DEPENDS "${TINTED_SHADER_SOURCES}/tinted.${STAGE}" ${ENGINE_SHADER_INCLUDES}
          COMMENT "Compiling the shader tinted.${STAGE} of the test"
          VERBATIM
  )
  list(APPEND TINTED_SHADER_OUTPUTS "${TINTED_SHADER_OUTPUT}/tinted.${STAGE}.spv")
endforeach ()
add_custom_target(tinted-shaders DEPENDS ${TINTED_SHADER_OUTPUTS})
add_dependencies(tinted tinted-shaders)

if (NOT CMAKE_CROSSCOMPILING)
  foreach (CASE IN ITEMS a-shader-of-an-extension-reads-the-time-and-the-numbers-of-the-game)
    add_test(
            NAME "runtime-shaders.${CASE}"
            COMMAND "${CMAKE_COMMAND}"
            "-DAPPLICATION=${GAME_DIRECTORY}/NeonRuntime${CMAKE_EXECUTABLE_SUFFIX}"
            "-DDIRECTORY=${NEON_TESTS_DIRECTORY}/runtime-shaders/${CASE}"
            "-DCASE=${CASE}"
            "-DPIXEL_PROBE=$<TARGET_FILE:pixel-probe>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run.cmake"
    )
    set_tests_properties("runtime-shaders.${CASE}" PROPERTIES
            LABELS "integration"
            SKIP_REGULAR_EXPRESSION "SKIPPED:"
            TIMEOUT 120
    )
  endforeach ()
endif ()
