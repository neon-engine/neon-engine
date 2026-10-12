# The editor's tools driven by a script without a window: the Log switched
# through the View menu with a screenshot of each state, Quit chosen through
# the File menu, an expectation that fails, a script that cannot be read, and
# the tools drawn for a screenshot with no script at all. Needs a graphics
# card with a Vulkan driver; without one the tests are skipped. See
# docs/editor.md#scripted-runs.
neon_add_application_test(editor-scripts NeonEditor run.cmake
        PROJECT sandbox
        CASES
        view-log-switches-the-log
        file-quit-ends-the-run
        a-failed-expectation-names-the-step
        a-script-that-cannot-be-read-stops-the-start
        tools-are-drawn-for-a-screenshot
)
