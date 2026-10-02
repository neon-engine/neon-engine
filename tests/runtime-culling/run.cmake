# Draws the scene culling-test.scene.yml without a window, and reads pixels
# of what it saved with pixel-probe. Where each quad is on the screen follows
# from the scene: a camera 5 in front of quads of 1 by 1, drawn at 1920 by
# 1080, where 1 at the depth of the quads is about 260 pixels.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 5
        --output-dir shots --screenshot output://frame.png --scene assets://scenes/culling-test.scene.yml)

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

if (CASE STREQUAL "the-back-is-left-out")
  expect_pixel("the quad seen from the front" 177 331 0 255 0)
  expect_pixel("where the quad seen from the back is" 699 331 0 0 0)
elseif (CASE STREQUAL "a-double-sided-material-shows-its-back")
  expect_pixel("the double-sided quad seen from the back" 1220 331 0 0 255)
elseif (CASE STREQUAL "what-is-mirrored-is-drawn-from-the-front")
  # Mirroring turns the triangles round. Without taking that into account
  # the mirrored quad seen from the front would be left out, and the one
  # seen from the back drawn.
  expect_pixel("the mirrored quad seen from the front" 1742 331 255 255 0)
  expect_pixel("where the mirrored quad seen from the back is" 960 853 0 0 0)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
