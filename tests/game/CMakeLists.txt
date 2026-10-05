# The game of the tests: a project that holds every scene the tests start
# the runtime with, and what those scenes show. It is put together once,
# next to a copy of the runtime, where the tests are run, and a test names it
# with PROJECT, see neon_add_application_test. Nothing of it is in the
# assets of the runtime, so a game does not carry the scenes of the tests.
#
# What its scenes show is built by the engine as Geometry, or is a small
# file written by a script of tools/, which the file itself names.
neon_add_project(test-game DIRECTORY "${NEON_TESTS_DIRECTORY}/game")
