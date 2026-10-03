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
  expect_output("--headless-renderer")
  expect_output("--headless ")
  expect_output("--screenshot-at N[,N...]")
  expect_output("--vulkan-version 1.N")
  expect_output("--window-mode MODE")
  expect_output("Editor:")
  expect_output("--scene PATH")
elseif (CASE STREQUAL "window-mode-not-a-mode")
  expect_refused("Option '--window-mode' needs windowed, borderless, or fullscreen" --window-mode maximised)
elseif (CASE STREQUAL "headless-is-not-a-server")
  # --headless is kept for the dedicated server of #144, which does not exist
  # yet, so it is understood and refused with a pointer to the renderer
  expect_refused(
          "'--headless' is for a dedicated server, which does not exist yet (#144). To render without a window, use '--headless-renderer'"
          --headless --frames 3)
elseif (CASE STREQUAL "unknown-option")
  expect_refused("Unknown option '--unknown'" --unknown)
elseif (CASE STREQUAL "bare-argument")
  expect_refused("'scene.yml' is not an option. Options start with two dashes" scene.yml)
elseif (CASE STREQUAL "missing-value")
  expect_refused("Option '--frames' needs a value" --headless-renderer --frames)
elseif (CASE STREQUAL "unknown-renderer")
  expect_refused("'opengl' is not a value of '--renderer'. It accepts vulkan" --renderer opengl)
elseif (CASE STREQUAL "frames-not-a-number")
  expect_refused("Option '--frames' needs a whole number above zero" --headless-renderer --frames many)
elseif (CASE STREQUAL "screenshot-without-frames")
  expect_refused(
          "Option '--screenshot' needs '--frames' or '--screenshot-at'"
          --headless-renderer --screenshot user://frame.png)
elseif (CASE STREQUAL "screenshot-at-without-screenshot")
  expect_refused("Option '--screenshot-at' needs '--screenshot'" --headless-renderer --frames 3 --screenshot-at 1,2)
elseif (CASE STREQUAL "screenshot-at-never-reached")
  expect_refused(
          "Frame 11 of '--screenshot-at' is never reached, '--frames' stops after 10"
          --headless-renderer --frames 10 --screenshot user://frame.png --screenshot-at 5,11)
elseif (CASE STREQUAL "screenshot-without-output-dir")
  expect_refused(
          "'output://frame.png' needs '--output-dir', which says where output:// is"
          --headless-renderer --frames 3 --screenshot output://frame.png)
elseif (CASE STREQUAL "time-step-not-a-number")
  expect_refused(
          "Option '--time-step' needs a number of seconds above zero, such as 0.016667"
          --headless-renderer --frames 3 --time-step fast)
elseif (CASE STREQUAL "vulkan-version-not-a-version")
  expect_refused(
          "Option '--vulkan-version' needs a version of Vulkan 1, such as 1.3"
          --headless-renderer --frames 3 --vulkan-version 2.0)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()

# the run ended before anything was started, so nothing was written
file(GLOB WRITTEN "${DIRECTORY}/*")
if (WRITTEN)
  fail("Expected nothing to be written, found ${WRITTEN}")
endif ()
