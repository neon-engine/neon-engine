# Unit tests and functional tests.
#
# A unit test lives next to the file it tests and is named after it, with
# `.test` in front of the extension: command-line.cpp is tested by
# command-line.test.cpp. Each one is built into an executable of its own,
# which runs all of its tests when started without arguments. This follows
# P1204R0, https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html
#
# Nothing here is part of the default build. The target `neon-tests` builds
# every test. The global property NEON_TEST_TARGETS lists the test
# executables, so that the style tools can check them.

include(GoogleTest)

# Builds every test. Test executables are added to it as they are declared.
add_custom_target(neon-tests)

# Where the test executables are written, each into a folder of its own.
# PROJECT_BINARY_DIR is used because the project redefines CMAKE_BINARY_DIR.
set(NEON_TESTS_DIRECTORY "${PROJECT_BINARY_DIR}/tests")

# neon_add_unit_test(<library> <file.test.cpp> [LIBRARIES <target>...])
#
# Declares the unit test of one source file of a library. The test links the
# library, the shared mocks and fakes, and GoogleTest with GoogleMock.
# LIBRARIES names what the test needs beyond that.
#
# NEON_UNIT_TEST_TARGET holds the name of the target afterwards, and
# NEON_UNIT_TEST_DIRECTORY the folder its executable is written to.
function(neon_add_unit_test LIBRARY SOURCE)
  cmake_parse_arguments(PARSE_ARGV 2 TEST "" "" "LIBRARIES")

  if (NOT SOURCE MATCHES "\\.test\\.cpp$")
    message(FATAL_ERROR "The unit test '${SOURCE}' has to end with .test.cpp")
  endif ()

  get_filename_component(NAME "${SOURCE}" NAME)
  string(REGEX REPLACE "\\.test\\.cpp$" "" NAME "${NAME}")

  set(TEST_TARGET "${LIBRARY}.${NAME}.test")
  set(TEST_DIRECTORY "${NEON_TESTS_DIRECTORY}/${TEST_TARGET}")

  add_executable(${TEST_TARGET} EXCLUDE_FROM_ALL "${SOURCE}")

  # GoogleTest comes first, so that its main() is the one that is found. SDL
  # brings one of its own on Windows, in SDL2main.
  target_link_libraries(${TEST_TARGET} PRIVATE
          GTest::gmock_main
          ${LIBRARY}
          neon-testing
          ${TEST_LIBRARIES}
  )

  set_target_properties(${TEST_TARGET} PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${TEST_DIRECTORY}"
  )

  add_dependencies(neon-tests ${TEST_TARGET})
  set_property(GLOBAL APPEND PROPERTY NEON_TEST_TARGETS ${TEST_TARGET})

  # The tests are listed when ctest runs, and not while building. A test that
  # was cross-compiled cannot be started on the machine that built it.
  gtest_discover_tests(${TEST_TARGET}
          WORKING_DIRECTORY "${TEST_DIRECTORY}"
          DISCOVERY_MODE PRE_TEST
          # The first start of a binary that was just built is slow on macOS,
          # while the system checks it, which is longer than the 5 seconds
          # given otherwise.
          DISCOVERY_TIMEOUT 60
          PROPERTIES LABELS "unit"
  )

  set(NEON_UNIT_TEST_TARGET "${TEST_TARGET}" PARENT_SCOPE)
  set(NEON_UNIT_TEST_DIRECTORY "${TEST_DIRECTORY}" PARENT_SCOPE)
endfunction()

# neon_add_functional_test(<name> SOURCES <file.cpp>... LIBRARIES <target>...)
#
# Declares a test that needs more than one library. These live in tests/,
# each in a folder of its own. The test links what LIBRARIES names, the shared
# mocks and fakes, and GoogleTest with GoogleMock.
function(neon_add_functional_test NAME)
  cmake_parse_arguments(PARSE_ARGV 1 TEST "" "" "SOURCES;LIBRARIES")

  set(TEST_TARGET "${NAME}.test")
  set(TEST_DIRECTORY "${NEON_TESTS_DIRECTORY}/${TEST_TARGET}")

  add_executable(${TEST_TARGET} EXCLUDE_FROM_ALL ${TEST_SOURCES})

  target_link_libraries(${TEST_TARGET} PRIVATE
          GTest::gmock_main
          ${TEST_LIBRARIES}
          neon-testing
  )

  set_target_properties(${TEST_TARGET} PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${TEST_DIRECTORY}"
  )

  add_dependencies(neon-tests ${TEST_TARGET})
  set_property(GLOBAL APPEND PROPERTY NEON_TEST_TARGETS ${TEST_TARGET})

  gtest_discover_tests(${TEST_TARGET}
          WORKING_DIRECTORY "${TEST_DIRECTORY}"
          DISCOVERY_MODE PRE_TEST
          # The first start of a binary that was just built is slow on macOS,
          # while the system checks it, which is longer than the 5 seconds
          # given otherwise.
          DISCOVERY_TIMEOUT 60
          PROPERTIES LABELS "integration"
  )
endfunction()

# neon_add_application_test(<name> <application> <script.cmake> [TOOLS <target>...] [CASES <case>...])
#
# Declares tests that start an application as a user would, and look at what
# it printed, what it wrote, and the exit code. The script is run by CMake,
# once for every case, with these variables set:
#   APPLICATION  the executable
#   DIRECTORY    a folder for what the run writes, empty at the start
#   CASE         the name of the case
# and for each of the TOOLS, the executable under its name in capitals with
# underscores, as PIXEL_PROBE for pixel-probe.
#
# A script that prints a line starting with `SKIPPED:` marks its test as
# skipped, for a machine that cannot do what the test needs.
#
# Nothing is declared when cross-compiling, since the application cannot be
# started on the machine that built it.
function(neon_add_application_test NAME APPLICATION SCRIPT)
  cmake_parse_arguments(PARSE_ARGV 3 TEST "" "" "CASES;TOOLS")

  add_dependencies(neon-tests ${APPLICATION})

  set(TOOL_DEFINITIONS)
  foreach (TOOL IN LISTS TEST_TOOLS)
    string(TOUPPER "${TOOL}" TOOL_VARIABLE)
    string(REPLACE "-" "_" TOOL_VARIABLE "${TOOL_VARIABLE}")
    list(APPEND TOOL_DEFINITIONS "-D${TOOL_VARIABLE}=$<TARGET_FILE:${TOOL}>")
  endforeach ()

  if (CMAKE_CROSSCOMPILING)
    return()
  endif ()

  foreach (CASE IN LISTS TEST_CASES)
    add_test(
            NAME "${NAME}.${CASE}"
            COMMAND "${CMAKE_COMMAND}"
            "-DAPPLICATION=$<TARGET_FILE:${APPLICATION}>"
            "-DDIRECTORY=${NEON_TESTS_DIRECTORY}/${NAME}/${CASE}"
            "-DCASE=${CASE}"
            ${TOOL_DEFINITIONS}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}"
    )

    set_tests_properties("${NAME}.${CASE}" PROPERTIES
            LABELS "integration"
            SKIP_REGULAR_EXPRESSION "SKIPPED:"
            # A check runs a handful of frames. Each run has a limit of its
            # own in run-application.cmake; this is the net under it.
            TIMEOUT 120
    )
  endforeach ()
endfunction()
