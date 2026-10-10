# The scripts of the runtime, run without a window: a Spinner declared in
# Lua turns a quad until it is edge on and gone, read pixel by pixel from
# what the runtime saved; controls of a user interface that call a handler
# of the scripts with `on_change` when an input script changes them; and a
# SettingsProbe that reads the settings of the game and hears of a change,
# read from the log. Needs a graphics card with a Vulkan driver. On a
# machine without one the tests are skipped, they do not fail.

neon_add_application_test(runtime-scripts NeonRuntime run.cmake
        PROJECT test-game
        TOOLS
        pixel-probe
        CASES
        a-script-turns-the-card-until-it-is-gone
        a-script-reads-and-hears-a-setting
        controls-call-their-handlers
)
