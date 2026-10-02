# Draws the scene prototype.scene.yml without a window, and reads pixels of
# what it saved with pixel-probe. The player stands in the corridor and looks
# through the doorway into the room, drawn at 1920 by 1080, so the corridor
# floor is at the bottom of the frame, its walls at the sides, the frame of
# the doorway in the middle, and the black behind the walls at the top.
#
# The frame these pixels were read from had the SHA-256
# 7c9da0029d64589d027bcfc4ac9a8fd038e502faf9d303b5418eb80197cf03cd, and it
# still has it since the player became a CharacterBody with a Player that
# stands on the floor: its eyes are where the Spectator flew before. The hash
# itself is not checked, since another graphics card rounds a little
# differently; the pixels are, within 2 of each channel.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

if (CASE STREQUAL "the-player-walks-into-the-room")
  # the player holds W for a second, which is four metres at the walking
  # speed, from the middle of the corridor to the doorway
  run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 60
          --output-dir shots --screenshot output://frame.png --screenshot-at 1,60
          --scene assets://scenes/prototype.scene.yml --input "1: hold-key w 20")
else ()
  run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 5
          --output-dir shots --screenshot output://frame.png --scene assets://scenes/prototype.scene.yml)
endif ()

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

# expect_pixel(<what> <x> <y> <red> <green> <blue>): the colour of a pixel of
# the image FRAME, each channel within 2 of what is given, which leaves room
# for rounding by the graphics card
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

if (CASE STREQUAL "the-level-is-drawn-from-the-kit-pieces")
  # The colours are those of Kenney's colormap, lit by the daylight and the
  # lamp: the floor tile and the walls are the grey-blue of the kit, the
  # frame of the doorway its darker grey. Nothing is behind the walls, so
  # the top of the frame is black.
  set(FRAME "shots/frame.png")
  expect_image("${FRAME}")
  expect_pixel("the black behind the walls" 960 100 0 0 0)
  expect_pixel("the floor of the corridor" 960 1040 141 144 167)
  expect_pixel("the west wall of the corridor" 300 500 126 131 160)
  expect_pixel("the frame of the doorway" 750 700 73 77 96)
elseif (CASE STREQUAL "the-player-walks-into-the-room")
  # In the first frame the player stands where the scene put it, with the
  # black behind the walls above the doorway. A second later it stands in
  # the doorway: the lintel is above it where the black was, and through
  # the doorway the north wall of the room fills the middle of the frame
  # where the west post of the doorway was.
  set(FRAME "shots/frame-0001.png")
  expect_image("${FRAME}")
  expect_pixel("the black above the doorway before the walk" 960 160 0 0 0)
  expect_pixel("the west post of the doorway before the walk" 750 700 73 77 96)

  set(FRAME "shots/frame-0060.png")
  expect_image("${FRAME}")
  expect_pixel("the lintel of the doorway after the walk" 960 160 76 78 99)
  expect_pixel("the north wall of the room after the walk" 700 500 154 153 172)
  expect_pixel("the floor of the room after the walk" 750 700 125 124 142)

  file(SHA256 "${DIRECTORY}/shots/frame-0001.png" BEFORE)
  file(SHA256 "${DIRECTORY}/shots/frame-0060.png" AFTER)
  if (BEFORE STREQUAL AFTER)
    fail("Expected the frame to change while the player walked, and frame 1 is the same as frame 60")
  endif ()
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
