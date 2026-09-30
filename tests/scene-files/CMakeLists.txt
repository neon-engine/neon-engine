neon_add_functional_test(scene-files
        SOURCES scene-files.cpp
        LIBRARIES neon-core neon-flecs neon-ryml
)

# the scene of the runtime is read by the test as it is in the repository
target_compile_definitions(scene-files.test PRIVATE
        NEON_DEMO_SCENE="${CMAKE_SOURCE_DIR}/app/NeonRuntime/assets/scenes/demo.scene.yml"
)
