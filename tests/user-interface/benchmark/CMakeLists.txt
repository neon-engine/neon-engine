# How long a frame of the user interface takes. Not a test: it is built by
# its target name and run by hand, and prints what it measured.
add_executable(user-interface.benchmark EXCLUDE_FROM_ALL ui-benchmark.cpp)

target_link_libraries(user-interface.benchmark PRIVATE
        GTest::gmock
        neon-core
        neon-ryml
        neon-testing
)

set_target_properties(user-interface.benchmark PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${NEON_TESTS_DIRECTORY}/user-interface.benchmark"
)
