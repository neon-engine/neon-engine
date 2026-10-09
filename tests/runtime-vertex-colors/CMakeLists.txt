# The colors painted on the vertices of a model, and the doubleSided of its
# material, read pixel by pixel from what the runtime saved. Needs a graphics
# card with a Vulkan driver. On a machine without one the tests are skipped,
# they do not fail.
neon_add_application_test(runtime-vertex-colors NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        the-colors-of-the-vertices-are-shown-as-they-are
        the-colors-of-the-vertices-are-the-base-color-of-the-lighting
        a-material-the-file-marks-double-sided-is-drawn-from-behind
        the-scene-overrides-the-file-with-never
)
