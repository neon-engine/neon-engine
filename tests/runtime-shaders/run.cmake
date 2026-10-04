# Starts the game of the test without a window: the runtime with the
# extension tinted next to it, which draws a square three metres in front of
# the camera with a shader of its own. The left half of the square is the
# colour the extension set as numbers for its shader; the right half is
# green when the shader is told that time runs.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

run(--headless-renderer --window-size 1280x720 --render-scale 1 --time-step 0.05 --frames 5
        --tonemapper none --output-dir shots --screenshot output://frame.png)

expect_exit_code(0)
expect_no_output("[error]")
expect_no_output("[critical]")
expect_output("The square is drawn with the shader of the extension")

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

# the numbers at place 3 are 1, 0.5, 0 in linear light, which a screen
# writes as 255 188 0
expect_pixel("the numbers of the game, on the left of the square" 560 360 255 188 0)

# the fifth frame: the world has run for a quarter of a second
expect_pixel("the time that runs, on the right of the square" 720 360 0 255 0)
