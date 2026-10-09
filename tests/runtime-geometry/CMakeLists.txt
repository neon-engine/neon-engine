# A level blocked out from shapes the engine builds, read pixel by pixel from
# what the runtime saved. Needs a graphics card with a Vulkan driver. On a
# machine without one the tests are skipped, they do not fail.
neon_add_application_test(runtime-geometry NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        a-room-built-from-a-prism-is-seen-from-within
        a-box-and-a-plane-are-textured-once-a-meter
        the-eyes-glide-up-a-step-after-the-body-stops
        the-player-is-stopped-at-the-edge-of-the-platform
)
