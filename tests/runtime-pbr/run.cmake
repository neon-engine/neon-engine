# Draws pbr-test.scene.yml without a window and reads pixels of the result.
#
# Three flat planes face the camera, and one white light shines from the
# camera, so that the normal, the view, the light, and the half vector all
# point the same way. Every term of the shading is then a known number:
#
#   Fresnel at normal incidence F = F0: 0.04 for a dielectric, the base
#   colour for a metal. Lambert diffuse = (1 - F)(1 - metallic) × colour.
#   GGX with roughness 1 (alpha 1) and N·H = 1 gives D = 1/π; the Smith
#   geometry term is 1 when N·V = N·L = 1; specular = D × G × F / 4.
#
#   chalk (white dielectric):  0.96 + 0.318 × 0.04 / 4 = 0.963  → sRGB 250
#   brushed metal (white):     0     + 0.318 × 1.00 / 4 = 0.0796 → sRGB 80
#   red chalk: red as chalk → 250; green and blue only the reflected
#   0.0032 → sRGB 11
#
# The planes are 1.5 wide at x = -2, 0, and 2, five units in front of the
# camera, so their centres are at the pixels probed.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 2
        --output-dir shots --screenshot output://frame.png --scene assets://scenes/pbr-test.scene.yml)

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
endfunction()

if (CASE STREQUAL "a-matte-dielectric-scatters-nearly-all-the-light")
  expect_pixel("white chalk facing the light" 386 540 250 250 250)
elseif (CASE STREQUAL "a-matte-metal-spreads-the-light-and-scatters-none")
  expect_pixel("white brushed metal facing the light" 960 540 80 80 80)
elseif (CASE STREQUAL "a-red-dielectric-reflects-white")
  expect_pixel("red chalk facing the light" 1534 540 250 11 11)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
