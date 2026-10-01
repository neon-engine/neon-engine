# What the runtime draws of a user interface with a real renderer: shapes,
# images, shaders of elements, and surfaces in the world. Needs a graphics
# card with a Vulkan driver. On a machine without one the tests are skipped,
# they do not fail.
neon_add_application_test(runtime-surfaces NeonRuntime run.cmake
        CASES
        gallery
        shaders-move-with-time
        surfaces-of-a-scene
        small-text
        pointing-at-a-screen
)
