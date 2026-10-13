# The editor's own folder, editor://, on a real file system: NeonEditor is
# started on the sandbox without a window, and the log says where the folder
# was placed, whether the editor's settings were read from it, and what was
# refused. Nothing of the folder is written without a window, since the
# layout and the window are the tools'. Needs a graphics card with a Vulkan
# driver, as every run of the editor does.
neon_add_application_test(editor-folder NeonEditor run.cmake
        PROJECT sandbox
        CASES
        places-the-folder
        reads-the-settings
        refuses-a-mistake
)
