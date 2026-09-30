# Starts NeonRuntime without a window, lets it render its scene, and looks at
# the exit code and at the images it saved.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

# run_headless(<argument>...)
function(run_headless)
  run(--headless --time-step 0.016667 ${ARGN})

  set(EXIT_CODE "${EXIT_CODE}" PARENT_SCOPE)
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
endfunction()

if (CASE STREQUAL "last-frame")
  run_headless(--frames 3 --output-dir shots --screenshot output://frame.png)
elseif (CASE STREQUAL "listed-frames")
  run_headless(--output-dir shots --screenshot output://today/frame.png --screenshot-at 3,1)
elseif (CASE STREQUAL "without-screenshot")
  run_headless(--frames 2)
elseif (CASE STREQUAL "screenshot-for-the-user")
  run_headless(--frames 1 --screenshot user://shots/frame.png)
elseif (CASE STREQUAL "output-folder-cannot-be-used")
  # a file is where the folder would have to be
  file(WRITE "${DIRECTORY}/taken" "a file, not a folder")
  run_headless(--frames 1 --output-dir taken --screenshot output://frame.png)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()

# a machine that cannot render at all skips the test
string(FIND "${OUTPUT}" "Failed to initialize Vulkan" NO_VULKAN)
if (NOT NO_VULKAN EQUAL -1)
  message("SKIPPED: this machine cannot start the Vulkan renderer")
  message("${OUTPUT}")
  return()
endif ()

if (CASE STREQUAL "last-frame")
  expect_exit_code(0)
  expect_output("Rendered 3 frames, stopping")
  expect_image("shots/frame.png")
  expect_no_file("shots/frame-0003.png")
elseif (CASE STREQUAL "listed-frames")
  expect_exit_code(0)
  # the run is as long as the highest frame that was asked for
  expect_output("Rendered 3 frames, stopping")
  expect_image("shots/today/frame-0001.png")
  expect_image("shots/today/frame-0003.png")
  expect_no_file("shots/today/frame-0002.png")
  expect_no_file("shots/today/frame.png")
elseif (CASE STREQUAL "without-screenshot")
  expect_exit_code(0)
  expect_output("Rendered 2 frames, stopping")
  expect_no_file("shots")
elseif (CASE STREQUAL "screenshot-for-the-user")
  expect_exit_code(0)
  # where the folder of the user is below the home folder depends on the
  # platform
  file(GLOB_RECURSE SAVED RELATIVE "${DIRECTORY}" "${DIRECTORY}/home/frame.png")
  list(LENGTH SAVED COUNT)
  if (NOT COUNT EQUAL 1 OR NOT SAVED MATCHES "neon-engine/neon-runtime/shots/frame.png$")
    fail("Expected one image in the folder of the user, below ${DIRECTORY}/home, found '${SAVED}'")
  endif ()
  expect_image("${SAVED}")
elseif (CASE STREQUAL "output-folder-cannot-be-used")
  # the run still renders, says what went wrong, and fails
  expect_exit_code(1)
  expect_output("cannot be used, output:// stays without a folder")
  expect_output("Rendered 1 frames, stopping")
  file(READ "${DIRECTORY}/taken" TAKEN)
  if (NOT TAKEN STREQUAL "a file, not a folder")
    fail("Expected the file that was in the way to be left as it was")
  endif ()
endif ()
