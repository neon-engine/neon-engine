# Draws the scene spawn-test.scene.yml without a window, with and without
# the target of the prototype spawned at its origin through --spawn, and
# reads the middle of what it saved with pixel-probe. The scene holds a
# camera that looks at the origin and a light, nothing else, so the middle
# of the frame is black until the target is spawned there.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

set(SCENE --headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 5
        --output-dir shots --screenshot output://frame.png --scene assets://scenes/spawn-test.scene.yml)

if (CASE STREQUAL "a-prefab-that-cannot-be-read-spawns-nothing")
  run(${SCENE} --spawn assets://prefabs/nope.prefab.yml)
elseif (CASE STREQUAL "a-target-spawned-from-its-prefab-is-drawn")
  run(${SCENE} --spawn assets://prefabs/target.prefab.yml)
else ()
  run(${SCENE})
endif ()

# a machine that cannot render at all skips the test
string(FIND "${OUTPUT}" "Failed to initialize Vulkan" NO_VULKAN)
if (NOT NO_VULKAN EQUAL -1)
  message("SKIPPED: this machine cannot start the Vulkan renderer")
  message("${OUTPUT}")
  return()
endif ()

# expect_pixel(<what> <x> <y> <red> <green> <blue>): the color of a pixel,
# each channel within 2 of what is given, which leaves room for rounding by
# the graphics card
function(expect_pixel WHAT X Y RED GREEN BLUE)
  execute_process(
          COMMAND "${PIXEL_PROBE}" "${DIRECTORY}/shots/frame.png" "${X},${Y}"
          RESULT_VARIABLE RESULT
          OUTPUT_VARIABLE PRINTED
          ERROR_VARIABLE PROBLEM
  )
  if (NOT RESULT EQUAL 0)
    fail("Could not read the pixel ${X},${Y}: ${PROBLEM}")
  endif ()

  string(STRIP "${PRINTED}" PRINTED)
  string(REPLACE " " ";" PARTS "${PRINTED}")
  list(GET PARTS 1 GOT_RED)
  list(GET PARTS 2 GOT_GREEN)
  list(GET PARTS 3 GOT_BLUE)

  foreach (CHANNEL IN ITEMS RED GREEN BLUE)
    math(EXPR DIFFERENCE "${GOT_${CHANNEL}} - ${${CHANNEL}}")
    if (DIFFERENCE GREATER 2 OR DIFFERENCE LESS -2)
      fail("Expected ${WHAT} at ${X},${Y} to be ${RED} ${GREEN} ${BLUE}, it is ${GOT_RED} ${GOT_GREEN} ${GOT_BLUE}")
    endif ()
  endforeach ()

  message("${WHAT} at ${X},${Y} is ${GOT_RED} ${GOT_GREEN} ${GOT_BLUE}")
endfunction()

if (CASE STREQUAL "a-prefab-that-cannot-be-read-spawns-nothing")
  # the run goes on without it, and the exit code says that it could not
  expect_exit_code(1)
  expect_output("Spawning assets://prefabs/nope.prefab.yml as the command line asked")
  expect_output("'prefab' of the spawned entity is assets://prefabs/nope.prefab.yml, which cannot be read")
  expect_output("The prefab assets://prefabs/nope.prefab.yml has 1 problem, nothing is spawned")
  expect_image("shots/frame.png")
  expect_pixel("the black where nothing was spawned" 960 540 0 0 0)
  return()
endif ()

expect_exit_code(0)
expect_no_output("[error]")
expect_no_output("[warning]")
expect_no_output("[critical]")
expect_image("shots/frame.png")

if (CASE STREQUAL "the-scene-alone-shows-nothing")
  expect_pixel("the black of the empty scene" 960 540 0 0 0)
elseif (CASE STREQUAL "a-target-spawned-from-its-prefab-is-drawn")
  # the target of the kit, seen face on in the colors of the kit's
  # colormap, lit by the daylight: the red of its bull's eye in the middle,
  # the gray of its plate around it, and black where nothing is
  expect_output("Spawning assets://prefabs/target.prefab.yml as the command line asked")
  expect_pixel("the red bull's eye of the spawned target" 960 540 146 50 33)
  expect_pixel("the plate of the spawned target above it" 960 450 112 112 117)
  expect_pixel("the plate of the spawned target to its left" 800 540 112 112 117)
  expect_pixel("the black above the spawned target" 960 300 0 0 0)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
