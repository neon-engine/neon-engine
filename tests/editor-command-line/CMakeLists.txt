# Needs no graphics card: the command line is read before anything starts,
# and the editor is started as it was built, without a project or with an
# empty folder for one. The editor's set of options is checked here, since
# the runtime no longer has it (#310).
neon_add_application_test(editor-command-line NeonEditor run.cmake
        CASES
        help
        headless-is-not-a-server
        missing-value
        frames-not-a-number
        screenshot-without-frames
        screenshot-at-without-screenshot
        screenshot-at-never-reached
        screenshot-without-output-dir
        time-step-not-a-number
        two-folders
        no-folder
        folder-without-a-project
)
