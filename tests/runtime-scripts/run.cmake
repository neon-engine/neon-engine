# Draws scripting-demo.scene.yml without a window and reads pixels of what
# it saved with pixel-probe. The scene is a green quad facing the camera
# with a Spinner, the component scripts/spinner.lua declares, which
# turns it about y at 180 degrees a second. At a time step of 0.05 s the
# first frame shows the quad turned by 9 degrees, still green in the middle
# of the frame; the tenth frame shows it turned by 90, edge on, so the
# middle shows the black behind it. No hash is recorded: the colors are
# plain and read within 2 of each channel.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 10
        --output-dir shots --screenshot output://frame.png --screenshot-at 1,10
        --scene assets://scenes/scripting-demo.scene.yml)

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

# the scripts were found and what they declare is said: the Spinner this
# scene uses, the one script of the game of the tests
expect_output("Scripts under assets:// declare 1 components and 1 systems")

# expect_pixel(<what> <x> <y> <red> <green> <blue>): the color of a pixel of
# the image FRAME, each channel within 2 of what is given
function(expect_pixel WHAT X Y RED GREEN BLUE)
  execute_process(
          COMMAND "${PIXEL_PROBE}" "${DIRECTORY}/${FRAME}" "${X},${Y}"
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

set(FRAME "shots/frame-0001.png")
expect_image("${FRAME}")
expect_pixel("the card, turned a little, in the first frame" 960 540 0 255 0)
expect_pixel("the black beside the card" 960 100 0 0 0)

set(FRAME "shots/frame-0010.png")
expect_image("${FRAME}")
expect_pixel("the black where the card stands edge on" 960 540 0 0 0)
