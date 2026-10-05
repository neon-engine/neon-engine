# A prefab spawned while the game plays, drawn without a window and read
# pixel by pixel from what the runtime saved. Needs a graphics card with a
# Vulkan driver. On a machine without one the tests are skipped, they do
# not fail.
neon_add_application_test(runtime-spawn NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        the-scene-alone-shows-nothing
        a-target-spawned-from-its-prefab-is-drawn
        a-prefab-that-cannot-be-read-spawns-nothing
)
