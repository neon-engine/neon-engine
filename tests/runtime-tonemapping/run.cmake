# Draws tonemap-test.scene.yml without a window, with a tonemapper or an
# exposure given on the command line, and reads pixels of the result.
#
# The scene has no light. Every plane shows the light its material gives
# off, so what reaches the screen is the resolve step alone: the exposure
# multiplied in, the curve, and the sRGB encoding. The light of each plane
# is known, and what the curves make of it is worked out from their fits:
#
#   ACES (Narkowicz): f(x) = x (2.51 x + 0.03) / (x (2.43 x + 0.59) + 0.14)
#     white 1.0 -> 0.804 -> sRGB 232;  4.0 -> 0.973 -> 252;  0.214 -> 0.322 -> 154
#   AgX (Wrensch, minimal): inset, log2 over -12.47 to 4.03 stops, the
#     sigmoid polynomial, outset, and a 2.2 power to linear
#     white 1.0 -> 0.590 -> sRGB 202;  4.0 -> 0.853 -> 239;  0.214 -> 0.246 -> 136
#     red (1, 0, 0) -> (221, 56, 56): the inset mixes a little of each
#     channel into the others, which is what keeps a bright color from
#     turning white
#
# The planes are 1.5 wide at x = -2.7, -0.9, 0.9, and 2.7 and y = 1 and
# -1, five units in front of the camera, so their centers are at the
# pixels probed: 260.7 pixels to the unit at 1920 by 1080.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

# run_scene(<argument>...): the scene, with what the case adds
function(run_scene)
  run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 2
          --output-dir shots --screenshot output://frame.png --scene assets://scenes/tonemap-test.scene.yml
          ${ARGN})
  set(EXIT_CODE "${EXIT_CODE}" PARENT_SCOPE)
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
endfunction()

# expect_pixel(<what> <x> <y> <red> <green> <blue>): the color of a pixel,
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

function(expect_frame)
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
endfunction()

if (CASE STREQUAL "light-above-white-is-cut-off-flat-by-default")
  run_scene()
  expect_frame()
  expect_pixel("the gray plane" 256 279 128 128 128)
  expect_pixel("white given off" 725 279 255 255 255)
  expect_pixel("four times white given off, cut off" 1195 279 255 255 255)
  expect_pixel("blue given off by basic-lit" 1664 279 0 0 255)
  expect_pixel("red given off" 725 801 255 0 0)
  expect_pixel("half of white given off" 1195 801 128 128 128)
  expect_pixel("the dark between the planes" 960 540 0 0 0)
elseif (CASE STREQUAL "aces-rolls-the-light-off-towards-white")
  run_scene(--tonemapper aces)
  expect_frame()
  expect_pixel("the gray plane" 256 279 154 154 154)
  expect_pixel("white given off, a little below white" 725 279 232 232 232)
  expect_pixel("four times white given off, nearly white" 1195 279 252 252 252)
  expect_pixel("blue given off by basic-lit" 1664 279 0 0 232)
  expect_pixel("red given off" 725 801 232 0 0)
  expect_pixel("half of white given off" 1195 801 154 154 154)
  expect_pixel("the dark between the planes" 960 540 0 0 0)
elseif (CASE STREQUAL "agx-rolls-the-light-off-and-keeps-its-colors")
  run_scene(--tonemapper agx)
  expect_frame()
  expect_pixel("the gray plane" 256 279 136 136 136)
  expect_pixel("white given off, well below white" 725 279 202 202 202)
  expect_pixel("four times white given off, nearer white" 1195 279 239 239 239)
  expect_pixel("blue given off by basic-lit, a little desaturated" 1664 279 79 79 213)
  expect_pixel("red given off, a little desaturated" 725 801 221 56 56)
  expect_pixel("half of white given off" 1195 801 136 136 136)
  expect_pixel("the dark between the planes" 960 540 0 0 0)
elseif (CASE STREQUAL "exposure-multiplies-the-light-before-the-curve")
  # twice the light, and no curve: 0.214 becomes 0.428, which sRGB writes
  # as 175, and white stays cut off at white
  run_scene(--exposure 2)
  expect_frame()
  expect_pixel("the gray plane at twice the light" 256 279 175 175 175)
  expect_pixel("white given off, still white" 725 279 255 255 255)
  expect_pixel("half of white given off at twice the light" 1195 801 175 175 175)
elseif (CASE STREQUAL "an-emissive-surface-shows-in-the-dark")
  # the scene has no light at all, and the pbr and basic-lit planes that
  # give off light show nonetheless, in the color they give off
  run_scene()
  expect_frame()
  expect_pixel("red given off, in the dark" 725 801 255 0 0)
  expect_pixel("blue given off by basic-lit, in the dark" 1664 279 0 0 255)
  expect_pixel("half of white given off, in the dark" 1195 801 128 128 128)
  expect_pixel("the dark between the planes" 960 540 0 0 0)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
