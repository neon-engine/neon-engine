# Draws the scene gamma-test.scene.yml without a window, and reads pixels of
# what it saved with pixel-probe. Where each thing is on the screen follows
# from the scene: a camera 5 in front of planes at a known place and size,
# drawn at 1920 by 1080.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 5
        --output-dir shots --screenshot output://frame.png --scene assets://scenes/gamma-test.scene.yml)

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

if (CASE STREQUAL "half-red-over-black-is-half-the-light")
  # Half the light of red is 0.5, which sRGB writes as 188. Blending the
  # sRGB numbers instead would give 128.
  expect_pixel("half of red over black" 206 279 188 0 0)
  expect_pixel("black" 800 450 0 0 0)
  expect_pixel("white" 1820 480 255 255 255)
elseif (CASE STREQUAL "see-through-planes-blend-in-linear-light")
  # Half of blue over white is the light 0.5 0.5 1. Half of orange, whose
  # green is the light 0.214, over that is 0.75 0.357 0.5. The orange plane
  # is written first in the scene and is nearer, so the blue one has to be
  # drawn before it.
  expect_pixel("half of blue over white" 1250 300 188 188 255)
  expect_pixel("half of orange over half of blue over white" 1450 300 225 161 188)
  expect_pixel("half of orange over white" 1700 300 255 204 188)
  expect_pixel("half of orange over black" 1580 70 188 92 0)
elseif (CASE STREQUAL "an-opaque-plane-ignores-the-alpha-of-its-colour")
  # Were its alpha of 0.25 taken as the scene image's, the resolve would
  # divide the green by it, clamp it, and multiply it back in: 64, not 255.
  expect_pixel("the opaque green plane with an alpha of 0.25" 960 827 0 255 0)
elseif (CASE STREQUAL "a-texture-is-shown-as-it-is")
  # read as linear light and written as sRGB again, the colours of the file
  expect_pixel("the left top of the texture" 386 723 128 128 128)
  expect_pixel("the right top of the texture" 594 723 255 128 0)
  expect_pixel("the left bottom of the texture" 386 931 32 64 96)
  expect_pixel("the right bottom of the texture" 594 931 200 30 150)
elseif (CASE STREQUAL "a-surface-blends-as-css")
  # A user interface on a surface blends in sRGB, as CSS does, and the
  # model that shows it reads it as light and gives it back as it was.
  # Blending in linear light would give 255 188 188 and 188.
  expect_pixel("half of red over white on the surface" 1313 826 255 127 127)
  expect_pixel("half of white over black on the surface" 1547 826 128 128 128)
  expect_pixel("white on the surface" 1190 650 255 255 255)
elseif (CASE STREQUAL "the-screen-blends-as-css")
  # Blending in linear light would give 188 and 137.
  expect_pixel("half of black over the white plane" 1070 160 127 127 127)
  expect_pixel("a quarter of white over the black plane" 550 200 64 64 64)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
