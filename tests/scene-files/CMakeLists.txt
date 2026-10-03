neon_add_functional_test(scene-files
        SOURCES scene-files.cpp
        LIBRARIES neon-core neon-flecs neon-ryml
)

# a small scene, read by the test as it is in the repository. The scene of
# the runtime needs the physics, the scripts, and the user interface, which
# this test does not have; the application tests run it
target_compile_definitions(scene-files.test PRIVATE
        NEON_SMALL_SCENE="${CMAKE_CURRENT_SOURCE_DIR}/small.scene.yml"
)
