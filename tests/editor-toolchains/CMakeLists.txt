neon_add_functional_test(editor-toolchains
        SOURCES editor-toolchains.cpp
        LIBRARIES neon-editor neon-ryml neon-platform
)

# the toolchains of the sandbox are read by the test as they are in the
# repository
target_compile_definitions(editor-toolchains.test PRIVATE
        NEON_SANDBOX_DIRECTORY="${CMAKE_SOURCE_DIR}/projects/sandbox"
)
