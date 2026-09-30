# Starts NeonRuntime with a command line that ends the run before anything is
# rendered, and looks at the exit code and at what was printed.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

# expect_refused(<message> <argument>...)
function(expect_refused MESSAGE)
  run(${ARGN})
  expect_exit_code(1)
  expect_output("${MESSAGE}")
  # the help text follows, so that the mistake can be corrected
  expect_output("Usage: NeonRuntime [options]")
endfunction()

if (CASE STREQUAL "help")
  run(--help)
  expect_exit_code(0)
  expect_output("Runs a Neon Engine project.")
  expect_output("Usage: NeonRuntime [options]")
  expect_output("--headless")
  expect_output("--screenshot-at N[,N...]")
elseif (CASE STREQUAL "unknown-option")
  expect_refused("Unknown option '--unknown'" --unknown)
elseif (CASE STREQUAL "bare-argument")
  expect_refused("'scene.yml' is not an option. Options start with two dashes" scene.yml)
elseif (CASE STREQUAL "missing-value")
  expect_refused("Option '--frames' needs a value" --headless --frames)
elseif (CASE STREQUAL "unknown-renderer")
  expect_refused("'opengl' is not a value of '--renderer'. It accepts vulkan" --renderer opengl)
elseif (CASE STREQUAL "frames-not-a-number")
  expect_refused("Option '--frames' needs a whole number above zero" --headless --frames many)
elseif (CASE STREQUAL "screenshot-without-frames")
  expect_refused(
          "Option '--screenshot' needs '--frames' or '--screenshot-at'"
          --headless --screenshot user://frame.png)
elseif (CASE STREQUAL "screenshot-at-without-screenshot")
  expect_refused("Option '--screenshot-at' needs '--screenshot'" --headless --frames 3 --screenshot-at 1,2)
elseif (CASE STREQUAL "screenshot-at-never-reached")
  expect_refused(
          "Frame 11 of '--screenshot-at' is never reached, '--frames' stops after 10"
          --headless --frames 10 --screenshot user://frame.png --screenshot-at 5,11)
elseif (CASE STREQUAL "screenshot-without-output-dir")
  expect_refused(
          "'output://frame.png' needs '--output-dir', which says where output:// is"
          --headless --frames 3 --screenshot output://frame.png)
elseif (CASE STREQUAL "time-step-not-a-number")
  expect_refused(
          "Option '--time-step' needs a number of seconds above zero, such as 0.016667"
          --headless --frames 3 --time-step fast)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()

# the run ended before anything was started, so nothing was written
file(GLOB WRITTEN "${DIRECTORY}/*")
if (WRITTEN)
  fail("Expected nothing to be written, found ${WRITTEN}")
endif ()
