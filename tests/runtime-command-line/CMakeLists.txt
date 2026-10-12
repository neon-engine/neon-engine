# Needs no graphics card: the command line is read before anything starts,
# and the runtime is started as it was built, with no project. The options
# of the editor's set are the editor's alone, see editor-command-line.
neon_add_application_test(runtime-command-line NeonRuntime run.cmake
        CASES
        help
        window-mode-not-a-mode
        unknown-option
        bare-argument
        unknown-renderer
        vulkan-version-not-a-version
        option-of-the-editor
        no-project
)
