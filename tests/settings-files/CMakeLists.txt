neon_add_functional_test(settings-files
        SOURCES settings-files.cpp
        LIBRARIES neon-core neon-ryml
)

# the settings of the runtime are read by the test as they are in the repository
target_compile_definitions(settings-files.test PRIVATE
        NEON_RUNTIME_SETTINGS="${CMAKE_SOURCE_DIR}/app/NeonRuntime/assets/settings.yml"
)
