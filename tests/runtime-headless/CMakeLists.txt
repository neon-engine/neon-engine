# Needs a graphics card with a Vulkan driver. On a machine without one the
# tests are skipped, they do not fail.
neon_add_application_test(runtime-headless NeonRuntime run.cmake
        CASES
        window-size-and-render-scale
        settings-menu-with-input
        ui-scale
        last-frame
        listed-frames
        without-screenshot
        screenshot-for-the-user
        output-folder-cannot-be-used
        user-interface
        user-interface-of-a-scene
        user-interface-that-is-missing
        pause-menu
        input-map-of-the-project
        hold-of-an-action-the-map-does-not-have
        vulkan-version
        capabilities
        vulkan-version-below-what-is-needed
)
