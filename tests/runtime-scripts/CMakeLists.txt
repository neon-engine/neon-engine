# The first script of the runtime, run without a window and read pixel by
# pixel from what the runtime saved: a Spinner declared in Lua turns a quad
# until it is edge on and gone. And controls of a user interface that call
# a handler of the scripts with `on_change` when an input script changes
# them. Needs a graphics card with a Vulkan driver. On a machine without
# one the tests are skipped, they do not fail.

neon_add_application_test(runtime-scripts NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        a-script-turns-the-card-until-it-is-gone
        controls-call-their-handlers
)
