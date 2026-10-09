# The shadow of the direction light, read pixel by pixel from what the
# runtime saved: a box on a floor darkens the floor beside it by the light
# it keeps off, and a light that casts no shadow leaves the floor lit, as
# does the setting that switches the shadows off, whose frame is the very
# frame of that light. A smaller map with hard edges shadows the floor the
# same. Needs a graphics card with a Vulkan driver. On a machine without
# one the tests are skipped, they do not fail.
neon_add_application_test(runtime-shadows NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        a-box-shadows-the-floor-beside-it
        a-light-that-casts-no-shadow-leaves-the-floor-lit
        shadows-off-leaves-the-floor-lit
        shadows-off-is-the-frame-of-a-light-that-casts-none
        a-small-map-with-hard-edges-shadows-the-floor-too
)
