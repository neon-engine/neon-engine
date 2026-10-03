# Draws blockout.scene.yml without a window and reads pixels of the result.
# Nothing in the scene comes from a model file: the room is a prism with its
# faces pointing inward, the platform and the crate boxes, the ramp a ramp,
# all built by the engine in metres and textured once a metre.
#
# The colours were read from the first render that was checked by eye, with
# the room's ceiling, walls, and floor all drawn and the brick upright. They
# are what the lighting makes of brick, concrete, and wood under one light,
# and they hold as long as the shapes, the camera, and the light stay.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

if (CASE STREQUAL "the-eyes-glide-up-a-step-after-the-body-stops")
  # the player walks between the ramp and the platform up to the north
  # wall, steps to the left in front of the step, and walks up it, where it
  # stands from frame 43 on. Frames of it standing soon after, and long
  # after
  run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05
          --output-dir shots --screenshot output://frame.png --screenshot-at 43,47,80,90
          --scene assets://scenes/blockout.scene.yml
          --input "1: hold-key w 27\n28: hold-key a 10\n38: hold-key w 5")
elseif (CASE STREQUAL "the-player-is-stopped-at-the-edge-of-the-platform")
  # the player holds D for three seconds, which is to the right into the
  # edge of the platform, 0.4 high and above what the player steps up
  run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 60
          --output-dir shots --screenshot output://frame.png --screenshot-at 20,60
          --scene assets://scenes/blockout.scene.yml --input "1: hold-key d 60")
else ()
  run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 5
          --output-dir shots --screenshot output://frame.png --scene assets://scenes/blockout.scene.yml)
endif ()

expect_exit_code(0)
expect_no_output("[error]")
expect_no_output("[warning]")
expect_no_output("[critical]")

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
  expect_image("shots/frame.png")
  # the ceiling above, the far wall ahead, the floor below: all drawn, none black
  expect_pixel("the ceiling" 960 200 45 45 48)
  expect_pixel("the far wall" 960 540 63 63 65)
  expect_pixel("the left wall" 300 540 104 30 31)
  expect_pixel("the floor" 960 1000 92 92 91)
elseif (CASE STREQUAL "a-box-and-a-plane-are-textured-once-a-metre")
  expect_image("shots/frame.png")
  expect_pixel("the wooden crate" 1400 760 82 58 24)
  expect_pixel("the concrete platform" 500 900 117 117 131)
elseif (CASE STREQUAL "the-eyes-glide-up-a-step-after-the-body-stops")
  # The body stands on the step from frame 43 on, and nothing else in the
  # scene moves. The eyes stayed behind when the physics lifted the body
  # (#218) and glide up at step_smoothing: a fifth of a second later the
  # view is still changing, and two seconds later it has settled, so two
  # frames then are the same image.
  foreach (FRAME IN ITEMS 43 47 80 90)
    expect_image("shots/frame-00${FRAME}.png")
  endforeach ()
  file(SHA256 "${DIRECTORY}/shots/frame-0043.png" SOON)
  file(SHA256 "${DIRECTORY}/shots/frame-0047.png" LATER)
  file(SHA256 "${DIRECTORY}/shots/frame-0080.png" SETTLED)
  file(SHA256 "${DIRECTORY}/shots/frame-0090.png" STILL)
  if (SOON STREQUAL LATER)
    fail("Expected the eyes to glide on while the body stands, and frame 43 is the same as frame 47")
  endif ()
  if (NOT SETTLED STREQUAL STILL)
    fail("Expected the eyes to have settled, and frame 80 differs from frame 90")
  endif ()
elseif (CASE STREQUAL "the-player-is-stopped-at-the-edge-of-the-platform")
  # stopped at the edge after a second, the player stands there two seconds
  # later: the same image, byte for byte. A platform of no thickness let
  # the player ride up its edge and slide back, and the images differed
  expect_image("shots/frame-0020.png")
  expect_image("shots/frame-0060.png")
  file(SHA256 "${DIRECTORY}/shots/frame-0020.png" AT_THE_EDGE)
  file(SHA256 "${DIRECTORY}/shots/frame-0060.png" TWO_SECONDS_LATER)
  if (NOT AT_THE_EDGE STREQUAL TWO_SECONDS_LATER)
    fail("Expected the player to stand at the edge of the platform from the first second to the third, and the frames differ: ${AT_THE_EDGE} and ${TWO_SECONDS_LATER}")
  endif ()
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
