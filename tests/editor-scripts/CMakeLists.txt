# The editor's tools driven by a script without a window: the editor opens
# with the viewport alone and the Log is switched through the View menu and
# its shortcut with a screenshot of each state, the Log comes back where it
# was, a mistake in the editor's settings is in the Log with its line, the
# Log and the window's size survive a restart, --window-size wins over the
# editor's settings for its run, Quit chosen through the File menu, an
# expectation that fails, a script that cannot be read, and the tools drawn
# for a screenshot with no script at all. Needs a graphics card with a
# Vulkan driver; without one the tests are skipped. See
# docs/editor.md#scripted-runs.
neon_add_application_test(editor-scripts NeonEditor run.cmake
        PROJECT sandbox
        CASES
        view-log-switches-the-log
        the-shortcut-switches-the-log
        the-log-comes-back-where-it-was
        a-mistake-in-the-settings-is-in-the-log
        the-log-and-the-window-survive-a-restart
        the-window-size-of-the-command-line-wins
        file-quit-ends-the-run
        a-failed-expectation-names-the-step
        a-script-that-cannot-be-read-stops-the-start
        tools-are-drawn-for-a-screenshot
)
