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
  expect_output("--vulkan-version 1.N")
  expect_output("--window-mode MODE")
  # the editor's set is not the runtime's (#310)
  expect_no_output("Editor:")
  expect_no_output("--scene PATH")
  expect_no_output("--headless-renderer")
elseif (CASE STREQUAL "window-mode-not-a-mode")
  expect_refused("Option '--window-mode' needs windowed, borderless, or fullscreen" --window-mode maximized)
elseif (CASE STREQUAL "unknown-option")
  expect_refused("Unknown option '--unknown'" --unknown)
elseif (CASE STREQUAL "bare-argument")
  expect_refused("'scene.yml' is not an option. Options start with two dashes" scene.yml)
elseif (CASE STREQUAL "unknown-renderer")
  expect_refused("'opengl' is not a value of '--renderer'. It accepts vulkan" --renderer opengl)
elseif (CASE STREQUAL "vulkan-version-not-a-version")
  expect_refused(
          "Option '--vulkan-version' needs a version of Vulkan 1, such as 1.3"
          --vulkan-version 2.0)
elseif (CASE STREQUAL "option-of-the-editor")
  # a shipped game accepts no --scene, and none of the editor's set (#310)
  expect_refused("Unknown option '--scene'" --scene assets://scenes/other.scene.yml)
  expect_refused("Unknown option '--headless-renderer'" --headless-renderer --frames 1)
elseif (CASE STREQUAL "no-project")
  # the runtime as it was built ships no game, and says what to start instead
  run()
  expect_exit_code(1)
  expect_output("assets://project.yml: there is no project here, the file cannot be read")
  expect_output("NeonRuntime runs the project that is put together next to it, and has none of its own")
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()

# the run ended before anything was started, so nothing was written
file(GLOB WRITTEN "${DIRECTORY}/*")
if (WRITTEN)
  fail("Expected nothing to be written, found ${WRITTEN}")
endif ()
