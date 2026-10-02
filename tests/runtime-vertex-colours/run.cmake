# Draws vertex-colours-test.scene.yml without a window and reads pixels of
# the result.
#
# The model is coloured-quads.glb, three quads 0.8 wide at x = -1, 0, and 1,
# red, green, and blue, painted on the vertices and not in a texture or a
# material colour. The camera is 5 in front of the plane the quads lie in,
# drawn at 1920 by 1080, so one unit of the plane is 287 pixels and the
# middle of the quad at x is the pixel 960 + 287 x, as in runtime-pbr.
#
# The top row is drawn with unlit: the colours as they are. The middle row
# with pbr, as a matte dielectric lit from the camera, which is the setup
# of runtime-pbr: red shows 250 11 11, the 4 percent that is reflected
# white. The bottom row is seen from behind. The material of the file says
# doubleSided, so the left model is drawn from its back too, with the quads
# in the other order; the right one is told `double_sided: never` by the
# scene, which wins over the file, and the clear colour shows.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 2
        --output-dir shots --screenshot output://frame.png --scene assets://scenes/vertex-colours-test.scene.yml)

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

if (CASE STREQUAL "the-colours-of-the-vertices-are-shown-as-they-are")
  expect_pixel("the red quad, unlit" 673 196 255 0 0)
  expect_pixel("the green quad, unlit" 960 196 0 255 0)
  expect_pixel("the blue quad, unlit" 1247 196 0 0 255)
elseif (CASE STREQUAL "the-colours-of-the-vertices-are-the-base-colour-of-the-lighting")
  expect_pixel("the red quad, lit" 673 540 250 11 11)
  expect_pixel("the green quad, lit" 960 540 11 250 11)
  expect_pixel("the blue quad, lit" 1247 540 11 11 250)
elseif (CASE STREQUAL "a-material-the-file-marks-double-sided-is-drawn-from-behind")
  expect_pixel("the blue quad, seen from behind" 300 884 0 0 255)
  expect_pixel("the green quad, seen from behind" 587 884 0 255 0)
  expect_pixel("the red quad, seen from behind" 874 884 255 0 0)
elseif (CASE STREQUAL "the-scene-overrides-the-file-with-never")
  expect_pixel("where the blue quad would be" 1046 884 0 0 0)
  expect_pixel("where the green quad would be" 1333 884 0 0 0)
  expect_pixel("where the red quad would be" 1620 884 0 0 0)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
