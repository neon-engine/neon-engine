neon_add_functional_test(input-maps
        SOURCES input-maps.cpp
        LIBRARIES neon-core neon-ryml
)

# the input map of the runtime is read by the test as it is in the repository
target_compile_definitions(input-maps.test PRIVATE
        NEON_RUNTIME_ASSETS="${CMAKE_SOURCE_DIR}/app/NeonRuntime/assets"
)
