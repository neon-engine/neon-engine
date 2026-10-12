# Starts NeonEditor with a command line that ends the run before anything is
# rendered, and looks at the exit code and at what was printed. The folder
# of the test stands in for a project folder where one is needed: nothing is
# read from it, and nothing is written into it.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

# expect_refused(<message> <argument>...)
function(expect_refused MESSAGE)
  run(${ARGN})
  expect_exit_code(1)
  expect_output("${MESSAGE}")
  # the help text follows, so that the mistake can be corrected
  expect_output("Usage: NeonEditor FOLDER [options]")
endfunction()

if (CASE STREQUAL "help")
  run(--help)
  expect_exit_code(0)
  expect_output("Edits and runs a Neon Engine project.")
  expect_output("Usage: NeonEditor FOLDER [options]")
  expect_output("The folder of the project to open")
  expect_output("--headless-renderer")
  expect_output("--headless ")
  expect_output("--screenshot-at N[,N...]")
  expect_output("--vulkan-version 1.N")
  expect_output("--window-mode MODE")
  expect_output("Editor:")
  expect_output("--scene PATH")
elseif (CASE STREQUAL "headless-is-not-a-server")
  # --headless is kept for the dedicated server of #144, which does not exist
  # yet, so it is understood and refused with a pointer to the renderer
  expect_refused(
          "'--headless' is for a dedicated server, which does not exist yet (#144). To render without a window, use '--headless-renderer'"
          "${DIRECTORY}" --headless --frames 3)
elseif (CASE STREQUAL "missing-value")
  expect_refused("Option '--frames' needs a value" "${DIRECTORY}" --headless-renderer --frames)
elseif (CASE STREQUAL "frames-not-a-number")
  expect_refused("Option '--frames' needs a whole number above zero" "${DIRECTORY}" --headless-renderer --frames many)
elseif (CASE STREQUAL "screenshot-without-frames")
  expect_refused(
          "Option '--screenshot' needs '--frames' or '--screenshot-at'"
          "${DIRECTORY}" --headless-renderer --screenshot user://frame.png)
elseif (CASE STREQUAL "screenshot-at-without-screenshot")
  expect_refused(
          "Option '--screenshot-at' needs '--screenshot'"
          "${DIRECTORY}" --headless-renderer --frames 3 --screenshot-at 1,2)
elseif (CASE STREQUAL "screenshot-at-never-reached")
  expect_refused(
          "Frame 11 of '--screenshot-at' is never reached, '--frames' stops after 10"
          "${DIRECTORY}" --headless-renderer --frames 10 --screenshot user://frame.png --screenshot-at 5,11)
elseif (CASE STREQUAL "screenshot-without-output-dir")
  expect_refused(
          "'output://frame.png' needs '--output-dir', which says where output:// is"
          "${DIRECTORY}" --headless-renderer --frames 3 --screenshot output://frame.png)
elseif (CASE STREQUAL "time-step-not-a-number")
  expect_refused(
          "Option '--time-step' needs a number of seconds above zero, such as 0.016667"
          "${DIRECTORY}" --headless-renderer --frames 3 --time-step fast)
elseif (CASE STREQUAL "two-folders")
  expect_refused("'other' is not an option, and the FOLDER was given already" "${DIRECTORY}" other)
elseif (CASE STREQUAL "no-folder")
  expect_refused("NeonEditor needs the folder of a project to open" --headless-renderer --frames 1)
elseif (CASE STREQUAL "folder-without-a-project")
  run("${DIRECTORY}" --headless-renderer --frames 1)
  expect_exit_code(1)
  expect_output("assets://project.yml: there is no project here, the file cannot be read")
  expect_output("NeonEditor opens the folder of a project, which holds assets/project.yml")
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()

# the run ended before anything was started, so nothing was written
file(GLOB WRITTEN "${DIRECTORY}/*")
if (WRITTEN)
  fail("Expected nothing to be written, found ${WRITTEN}")
endif ()
