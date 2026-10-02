# The first level of the prototype game, drawn without a window and read
# pixel by pixel from what the runtime saved: the kit pieces of a GLB are
# loaded with their textures, placed, and lit. Needs a graphics card with a
# Vulkan driver. On a machine without one the test is skipped, it does not
# fail.

neon_add_application_test(runtime-prototype NeonRuntime run.cmake
        TOOLS
        pixel-probe
        CASES
        the-level-is-drawn-from-the-kit-pieces
)
