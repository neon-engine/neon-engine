# Functional tests: what needs more than one library, a real backend, or the
# application itself. Each is in a folder of its own. Unit tests are not here,
# they sit next to the file they test.

add_subdirectory(world-with-flecs)
add_subdirectory(user-interface)
add_subdirectory(runtime-command-line)
add_subdirectory(runtime-headless)
add_subdirectory(physics-with-jolt)
add_subdirectory(runtime-physics)
add_subdirectory(scene-files)
add_subdirectory(text-shaping)
add_subdirectory(runtime-surfaces)
add_subdirectory(runtime-colours)
