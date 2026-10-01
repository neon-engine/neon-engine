# Which side of a surface the runtime draws, read pixel by pixel from what it
# saved: the back is left out unless the material is double-sided, and what
# a transform mirrors is still drawn from the front. Needs a graphics card
# with a Vulkan driver. On a machine without one the tests are skipped, they
# do not fail.

neon_add_application_test(runtime-culling NeonRuntime run.cmake
        TOOLS
        pixel-probe
        CASES
        the-back-is-left-out
        a-double-sided-material-shows-its-back
        what-is-mirrored-is-drawn-from-the-front
)
