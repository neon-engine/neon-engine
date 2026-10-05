# The sky, read pixel by pixel from what the runtime saved: the six faces of
# a box and the panorama of a sphere each show where they are seen, a sky
# that is turned shows another side, an opaque model covers the sky, and a
# see-through one is blended over it. Needs a graphics card with a Vulkan
# driver. On a machine without one the tests are skipped, they do not fail.
neon_add_application_test(runtime-sky NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        a-box-shows-each-face-where-it-is-seen
        a-sphere-shows-its-panorama-around-the-camera
        a-sky-that-is-turned-shows-another-side
)
