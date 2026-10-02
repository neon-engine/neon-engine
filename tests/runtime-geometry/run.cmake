# Draws blockout.scene.yml without a window and reads pixels of the result.
# Nothing in the scene comes from a model file: the room is a prism with its
# faces pointing inward, the platform a plane, the crate a box, the ramp a
# ramp, all built by the engine in metres and textured once a metre.
#
# The colours were read from the first render that was checked by eye, with
# the room's ceiling, walls, and floor all drawn and the brick upright. They
# are what the lighting makes of brick, concrete, and wood under one light,
# and they hold as long as the shapes, the camera, and the light stay.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 5
        --output-dir shots --screenshot output://frame.png --scene assets://scenes/blockout.scene.yml)

expect_exit_code(0)
expect_no_output("[error]")
expect_no_output("[warning]")
expect_no_output("[critical]")
expect_image("shots/frame.png")

# expect_pixel(<what> <x> <y> <red> <green> <blue>): the colour of a pixel,
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
endfunction()

if (CASE STREQUAL "a-room-built-from-a-prism-is-seen-from-within")
  # the ceiling above, the far wall ahead, the floor below: all drawn, none black
  expect_pixel("the ceiling" 960 200 45 45 48)
  expect_pixel("the far wall" 960 540 63 63 65)
  expect_pixel("the left wall" 300 540 104 30 31)
  expect_pixel("the floor" 960 1000 92 92 91)
elseif (CASE STREQUAL "a-box-and-a-plane-are-textured-once-a-metre")
  expect_pixel("the wooden crate" 1400 760 82 58 24)
  expect_pixel("the concrete platform" 500 900 117 117 131)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
