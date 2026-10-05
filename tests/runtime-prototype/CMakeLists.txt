# The first level of the prototype game, drawn without a window and read
# pixel by pixel from what the runtime saved: the kit pieces of a GLB are
# loaded with their textures, placed, and lit, and the player walks through
# it by a script of input. Needs a graphics card with a Vulkan driver. On a
# machine without one the tests are skipped, they do not fail.

neon_add_application_test(runtime-prototype NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        the-level-is-drawn-from-the-kit-pieces
        the-player-walks-into-the-room
        the-kit-pieces-are-loaded-once
        the-player-keeps-its-speed-through-a-jump
)
