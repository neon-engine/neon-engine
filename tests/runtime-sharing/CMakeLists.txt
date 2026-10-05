# What the renderer holds once for the entities that share it, read from
# the log of a run without a window: models, textures, and materials are
# made once and shared, freed once, and the descriptor pools grow past one.
# Needs a graphics card with a Vulkan driver. On a machine without one the
# tests are skipped, they do not fail.

neon_add_application_test(runtime-sharing NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        crates-with-one-material-share-it
        crates-of-distinct-colours-fill-more-than-one-pool
)
