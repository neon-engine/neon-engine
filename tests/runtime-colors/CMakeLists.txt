# The colors the runtime draws, read pixel by pixel from what it saved: the
# world lit and blended in linear light, textures shown as they are, and user
# interfaces blended as CSS blends them. Needs a graphics card with a Vulkan
# driver. On a machine without one the tests are skipped, they do not fail.

add_executable(pixel-probe pixel-probe.cpp)
neon_warnings(pixel-probe)
target_link_libraries(pixel-probe PRIVATE stb)
add_dependencies(neon-tests pixel-probe)

neon_add_application_test(runtime-colors NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        half-red-over-black-is-half-the-light
        see-through-planes-blend-in-linear-light
        an-opaque-plane-ignores-the-alpha-of-its-color
        a-texture-is-shown-as-it-is
        a-surface-blends-as-css
        the-screen-blends-as-css
)
