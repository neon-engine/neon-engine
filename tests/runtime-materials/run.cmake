# Draws materials-test.scene.yml without a window and reads pixels of the
# result.
#
# The model is coloured-boxes.glb, two boxes 0.8 on every side at x = -1
# and x = 1, each a mesh with a material of its own, red and blue as the
# base colour factor of each. The camera is 5 in front of the origin, so
# 4.6 in front of the faces that look at it, drawn at 1920 by 1080: one
# unit of that plane is 312 pixels, the middle of the box at x is the
# pixel 960 + 312 x, and a row placed at y lies at 540 - 312 y.
#
# The top row is drawn with the color shader: the colour of each material
# as it is. The middle row with pbr, as a matte dielectric lit from the
# camera, which is the setup of runtime-pbr: red shows 250 11 11, the 4
# percent that is reflected white. The bottom row is given a grey of 0.5
# by the scene, which multiplies the colour of every material of the
# model, so each box is drawn at half its light.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 2
        --output-dir shots --screenshot output://frame.png --scene assets://scenes/materials-test.scene.yml)

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

# three render objects of one model, each with a material for every
# material of the file
expect_output("Created 3 render objects: 1 models were loaded or built and 2 shared")
expect_output("6 materials were made and 0 shared")

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

if (CASE STREQUAL "every-mesh-is-drawn-with-its-own-material")
  expect_pixel("the red box" 648 165 255 0 0)
  expect_pixel("the blue box" 1272 165 0 0 255)
  expect_pixel("the gap between them" 960 165 0 0 0)
elseif (CASE STREQUAL "every-material-is-lit-with-its-own-colour")
  expect_pixel("the red box, lit" 648 540 250 11 11)
  expect_pixel("the blue box, lit" 1272 540 11 11 250)
elseif (CASE STREQUAL "the-colour-of-the-scene-multiplies-every-material")
  expect_pixel("the red box at half its light" 648 915 127 0 0)
  expect_pixel("the blue box at half its light" 1272 915 0 0 127)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
