# Needs a graphics card with a Vulkan driver. On a machine without one the
# tests are skipped, they do not fail.
neon_add_application_test(runtime-headless NeonRuntime run.cmake
        PROJECT test-game
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
        scene-that-is-missing
        pause-menu
        title-screen-starts-the-level
        input-map-of-the-engine-by-default
        hold-of-an-action-the-map-does-not-have
        vulkan-version
        capabilities
        vulkan-version-below-what-is-needed
        log-entity
)
