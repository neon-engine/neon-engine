neon_add_functional_test(settings-files
        SOURCES settings-files.cpp
        LIBRARIES neon-core neon-ryml
)

# the settings of the sandbox are read by the test as they are in the repository
target_compile_definitions(settings-files.test PRIVATE
        NEON_SANDBOX_SETTINGS="${CMAKE_SOURCE_DIR}/projects/sandbox/assets/settings.yml"
)
