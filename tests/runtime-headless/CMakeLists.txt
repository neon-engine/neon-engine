# Needs a graphics card with a Vulkan driver. On a machine without one the
# tests are skipped, they do not fail.
neon_add_application_test(runtime-headless NeonRuntime run.cmake
        CASES
        last-frame
        listed-frames
        without-screenshot
        screenshot-for-the-user
        output-folder-cannot-be-used
)
