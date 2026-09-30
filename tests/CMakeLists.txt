# Functional tests: what needs more than one library, a real backend, or the
# application itself. Each is in a folder of its own. Unit tests are not here,
# they sit next to the file they test.

add_subdirectory(world-with-flecs)
add_subdirectory(runtime-command-line)
add_subdirectory(runtime-headless)
