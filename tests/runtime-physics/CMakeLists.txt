# Needs a graphics card with a Vulkan driver. On a machine without one the
# tests are skipped, they do not fail.
neon_add_application_test(runtime-physics NeonRuntime run.cmake
        CASES
        moves-and-comes-to-rest
        same-at-every-frame-rate
        joints-move
)
