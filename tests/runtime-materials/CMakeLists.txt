# A model whose meshes use different materials, drawn with each of them,
# read pixel by pixel from what the runtime saved. Needs a graphics card
# with a Vulkan driver. On a machine without one the tests are skipped,
# they do not fail.
neon_add_application_test(runtime-materials NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        every-mesh-is-drawn-with-its-own-material
        every-material-is-lit-with-its-own-colour
        the-colour-of-the-scene-multiplies-every-material
)
