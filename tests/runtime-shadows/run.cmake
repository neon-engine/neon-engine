# Draws shadow-test.scene.yml without a window and reads pixels of the
# result, and then the same scene with a light that casts no shadow.
#
# The floor and the box are white, drawn with basic-lit and no shininess,
# and the one light comes from above at 45 degrees with an ambient of 0.2
# and a diffuse of 0.8. The floor faces up, so where the light reaches it
# it shows
#
#   0.2 + 0.8 × cos 45° = 0.2 + 0.566 = 0.766 → sRGB 227
#
# and in the shadow of the box the ambient light alone,
#
#   0.2 → sRGB 124.
#
# The box is 1 metre high and the light falls at 45 degrees from the side
# of positive x, so its shadow lies on the floor from x = -0.5 to x = -1.5.
# The camera is 5 above and 5 in front of the box, looking down at 45
# degrees, drawn at 1920 by 1080 with a vertical field of view of 45
# degrees; the places on the floor below project to the pixels probed:
# x = -1 in the shadow to 776,540, x = -3 and x = 2 in the light to 407,540
# and 1329,540, and the top of the box, which faces up like the floor, to
# 960,395.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

if (CASE STREQUAL "a-box-shadows-the-floor-beside-it")
  set(SCENE assets://scenes/shadow-test.scene.yml)
elseif (CASE STREQUAL "a-light-that-casts-no-shadow-leaves-the-floor-lit")
  set(SCENE assets://scenes/shadow-test-unshadowed.scene.yml)
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

expect_pixel("the floor in the light, far from the box" 407 540 227 227 227)
expect_pixel("the floor in the light, on the side of the light" 1329 540 227 227 227)
expect_pixel("the top of the box" 960 395 227 227 227)

if (CASE STREQUAL "a-box-shadows-the-floor-beside-it")
  # the ambient light alone: what the diffuse light added is kept off
  expect_pixel("the floor in the shadow of the box" 776 540 124 124 124)
else ()
  expect_pixel("the floor beside the box, with no shadow" 776 540 227 227 227)
endif ()
