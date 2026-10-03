# What the resolve step makes of light, read pixel by pixel from what the
# runtime saved: the clamp of the default, the two tonemappers, the
# exposure, and a surface that gives off light in the dark. Needs a graphics
# card with a Vulkan driver. On a machine without one the tests are
# skipped, they do not fail.
neon_add_application_test(runtime-tonemapping NeonRuntime run.cmake
        TOOLS
        pixel-probe
        CASES
        light-above-white-is-cut-off-flat-by-default
        aces-rolls-the-light-off-towards-white
        agx-rolls-the-light-off-and-keeps-its-colours
        exposure-multiplies-the-light-before-the-curve
        an-emissive-surface-shows-in-the-dark
)
