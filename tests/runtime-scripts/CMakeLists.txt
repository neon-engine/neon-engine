# The first script of the runtime, run without a window and read pixel by
# pixel from what the runtime saved: a Spinner declared in Lua turns a quad
# until it is edge on and gone. Needs a graphics card with a Vulkan driver.
# On a machine without one the tests are skipped, they do not fail.

neon_add_application_test(runtime-scripts NeonRuntime run.cmake
        TOOLS
        pixel-probe
        CASES
        a-script-turns-the-card-until-it-is-gone
)
