neon_add_functional_test(project-files
        SOURCES project-files.cpp
        LIBRARIES neon-core neon-ryml
)

# the project of the sandbox is read by the test as it is in the repository,
# and its scenes are looked for next to it
target_compile_definitions(project-files.test PRIVATE
        NEON_SANDBOX_ASSETS="${CMAKE_SOURCE_DIR}/projects/sandbox/assets"
)
