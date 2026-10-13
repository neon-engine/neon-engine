# File > Open in VS Code driven by a script without a window, with a script
# of the shell standing in for VS Code, so that nothing opens on the screen
# of whoever runs the tests: the root of the project is what opens, the
# editor does not wait, editor.yml names where VS Code is, user-editor.yml
# wins over it and a place that is not there falls back to PATH, and a
# mistake in editor.yml is reported with its line. Needs a graphics card
# with a Vulkan driver and a shell; without one the tests are skipped. See
# docs/editor.md#opening-the-code.
neon_add_application_test(editor-open-vscode NeonEditor run.cmake
        PROJECT sandbox
        CASES
        opens-the-root-of-the-project
        user-editor-yml-wins-and-a-wrong-path-falls-back
        a-mistake-in-editor-yml-is-reported
)
