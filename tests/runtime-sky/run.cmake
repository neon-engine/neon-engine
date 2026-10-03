# Draws a scene under a sky without a window and reads pixels of the result:
# sky-test-box.scene.yml, sky-test-sphere.scene.yml, and
# sky-test-turned.scene.yml.
#
# Every direction of the sky has a plain colour of its own, as
# tools/make-sky-images.py writes them:
#
#   right 220 40 40, left 40 180 40, top 60 90 230, bottom 120 80 40,
#   front 240 220 60, back 150 60 200
#
# The images hold sRGB colours, are read as linear light, and are written
# back as sRGB by the resolve, so a plain colour comes out as it went in.
#
# The camera stands at the origin and is not turned, drawn at 1920 by 1080
# with a vertical field of view of 120 degrees. A pixel x from the middle
# column and y above the middle row is seen in the direction
#
#   (x / 960 × tan 60° × 16/9,  y / 540 × tan 60°,  -1)
#
# so the middle of the picture is seen straight ahead, 20 pixels from the
# left and the right edge 72 degrees to the side, which is on the left and
# the right face of the cube, and 20 pixels from the top and the bottom
# edge 59 degrees up and down, which is on the top and the bottom face. In
# the panorama these fall in the middle of the bands of the same colours.
#
# A sky turned by 90 degrees against the clock seen from above shows ahead
# what was to the right, to the right what was behind, and to the left what
# was ahead.
#
# Two quads of 0.2 stand 2 in front of the camera, 0.5 to either side of
# the middle, which is 78 pixels from the middle column. The right one is
# opaque magenta and covers the sky. The left one is half see-through red,
# blended over the sky in linear light:
#
#   over front 240 220 60:  0.5 × (1, 0, 0) + 0.5 × (0.871, 0.716, 0.045) → 248 161 41
#   over right 220 40 40:   0.5 × (1, 0, 0) + 0.5 × (0.716, 0.021, 0.021) → 238 26 26

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

if (CASE STREQUAL "a-box-shows-each-face-where-it-is-seen")
  set(SCENE assets://scenes/sky-test-box.scene.yml)
elseif (CASE STREQUAL "a-sphere-shows-its-panorama-around-the-camera")
  set(SCENE assets://scenes/sky-test-sphere.scene.yml)
elseif (CASE STREQUAL "a-sky-that-is-turned-shows-another-side")
  set(SCENE assets://scenes/sky-test-turned.scene.yml)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()

run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 2
        --output-dir shots --screenshot output://frame.png --scene ${SCENE})

# a machine that cannot render at all skips the test
string(FIND "${OUTPUT}" "Failed to initialize Vulkan" NO_VULKAN)
if (NOT NO_VULKAN EQUAL -1)
  message("SKIPPED: this machine cannot start the Vulkan renderer")
  message("${OUTPUT}")
  return()
endif ()

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

  message("${WHAT} at ${X},${Y} is ${GOT_RED} ${GOT_GREEN} ${GOT_BLUE}")
endfunction()

# above and below, the same however the sky is turned around what is up
expect_pixel("the top of the sky" 960 20 60 90 230)
expect_pixel("the bottom of the sky" 960 1060 120 80 40)

# an opaque model covers the sky
expect_pixel("the opaque quad" 1038 540 255 0 255)

if (CASE STREQUAL "a-sky-that-is-turned-shows-another-side")
  expect_pixel("ahead, what was to the right" 960 540 220 40 40)
  expect_pixel("to the right, what was behind" 1900 540 150 60 200)
  expect_pixel("to the left, what was ahead" 20 540 240 220 60)
  expect_pixel("the see-through quad over the sky" 882 540 238 26 26)
else ()
  expect_pixel("the front of the sky" 960 540 240 220 60)
  expect_pixel("the right of the sky" 1900 540 220 40 40)
  expect_pixel("the left of the sky" 20 540 40 180 40)
  expect_pixel("the see-through quad over the sky" 882 540 248 161 41)
endif ()
