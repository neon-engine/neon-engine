# Needs no graphics card: the command line is read before anything starts.
neon_add_application_test(runtime-command-line NeonRuntime run.cmake
        CASES
        help
        unknown-option
        bare-argument
        missing-value
        unknown-renderer
        frames-not-a-number
        screenshot-without-frames
        screenshot-at-without-screenshot
        screenshot-at-never-reached
        screenshot-without-output-dir
        time-step-not-a-number
)
