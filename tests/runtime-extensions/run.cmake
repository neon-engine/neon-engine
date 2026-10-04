# Starts the game of the test without a window: the runtime with the
# extension canvas next to it, which makes a picture of four plain colours
# and a square to show it on, three metres in front of the camera. The
# picture is shown as it is, so the colours that come out are those that
# went in.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

run(--headless-renderer --window-size 1280x720 --render-scale 1 --time-step 0.05 --frames 5
        --output-dir shots --screenshot output://frame.png)

expect_exit_code(0)
expect_no_output("[error]")
expect_no_output("[critical]")
expect_output("The canvas shows image://canvas/quarters")

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
      fail("${WHAT}, the pixel ${X},${Y}, is ${GOT_RED} ${GOT_GREEN} ${GOT_BLUE}, and ${RED} ${GREEN} ${BLUE} was expected")
    endif ()
  endforeach ()
endfunction()

# the square is in the middle of the frame, a quarter of it each colour
expect_pixel("the top left of the picture" 560 280 255 0 0)
expect_pixel("the top right of the picture" 720 280 0 255 0)
expect_pixel("the bottom left of the picture" 560 440 0 0 255)
expect_pixel("the bottom right of the picture" 720 440 128 128 128)
