# The shadow of the direction light, read pixel by pixel from what the
# runtime saved: a box on a floor darkens the floor beside it by the light
# it keeps off, and a light that casts no shadow leaves the floor lit.
# Needs a graphics card with a Vulkan driver. On a machine without one the
# tests are skipped, they do not fail.
neon_add_application_test(runtime-shadows NeonRuntime run.cmake
        TOOLS
        pixel-probe
        CASES
        a-box-shadows-the-floor-beside-it
        a-light-that-casts-no-shadow-leaves-the-floor-lit
)
