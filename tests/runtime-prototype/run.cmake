# Draws the scene prototype.scene.yml without a window, and reads pixels of
# what it saved with pixel-probe. The player stands in the corridor and looks
# through the doorway into the room, drawn at 1920 by 1080, so the corridor
# floor is at the bottom of the frame, its walls at the sides, the frame of
# the doorway in the middle, and the black behind the walls at the top.
#
# The pieces are those of the kit of the tests, models/kit, a few boxes each,
# which tools/make-test-game-assets.py writes. The pixels were read from the
# first frame drawn with them that was checked by eye. The direction light
# casts shadows (#60): the walls and the crate shadow the floor, and none of
# the pixels below lies in a shadow. A hash of the frame is not checked,
# since another graphics card rounds a little differently; the pixels are,
# within 2 of each channel.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

if (CASE STREQUAL "the-player-walks-into-the-room")
  # the player holds W for a second, which is four metres at the walking
  # speed, from the middle of the corridor to the doorway
  run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05 --frames 60
          --output-dir shots --screenshot output://frame.png --screenshot-at 1,60
          --scene assets://scenes/prototype.scene.yml --input "1: hold-key w 20")
elseif (CASE STREQUAL "the-kit-pieces-are-loaded-once")
  # two frames: the first creates the render objects, the second shows that
  # nothing is loaded again
  run(--headless-renderer --window-size 320x180 --render-scale 1 --time-step 0.05 --frames 2
          --scene assets://scenes/prototype.scene.yml)
elseif (CASE STREQUAL "the-player-keeps-its-speed-through-a-jump")
  # the player walks for a third of a second, a metre and a bit, jumps in
  # the last frame of it, and releases W in the air. Once more without the
  # jump, to compare
  run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05
          --output-dir jump --screenshot output://frame.png --screenshot-at 5,16,60
          --scene assets://scenes/prototype.scene.yml --input "1: hold-key w 6\n6: hold jump 1")
  set(JUMP_EXIT_CODE "${EXIT_CODE}")
  set(JUMP_OUTPUT "${OUTPUT}")
  run(--headless-renderer --window-size 1920x1080 --render-scale 1 --time-step 0.05
          --output-dir walk --screenshot output://frame.png --screenshot-at 60
          --scene assets://scenes/prototype.scene.yml --input "1: hold-key w 6")
  if (NOT JUMP_EXIT_CODE STREQUAL 0)
    set(EXIT_CODE "${JUMP_EXIT_CODE}")
    set(OUTPUT "${JUMP_OUTPUT}")
  endif ()
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
  # The colours are those of the kit's colormap, lit by the daylight and the
  # lamp: the floor tile and the walls are the grey-blue of the kit, the
  # frame of the doorway its darker grey. Nothing is behind the walls, so
  # the top of the frame is black.
  set(FRAME "shots/frame.png")
  expect_image("${FRAME}")
  expect_pixel("the black behind the walls" 960 100 0 0 0)
  expect_pixel("the floor of the corridor" 960 1040 160 159 176)
  expect_pixel("the west wall of the corridor" 300 500 112 115 144)
  expect_pixel("the frame of the doorway" 750 700 58 61 80)
elseif (CASE STREQUAL "the-player-keeps-its-speed-through-a-jump")
  # The body kept the velocity it took off with (#219): once it has landed
  # it stands nearer the doorway than where W was released, so the frame
  # three seconds in is not the one of the walk alone, and it is a
  # different view from the one before the jump.
  expect_image("jump/frame-0005.png")
  expect_image("jump/frame-0060.png")
  expect_image("walk/frame-0060.png")
  file(SHA256 "${DIRECTORY}/jump/frame-0005.png" BEFORE)
  file(SHA256 "${DIRECTORY}/jump/frame-0060.png" AFTER)
  file(SHA256 "${DIRECTORY}/walk/frame-0060.png" WITHOUT)
  if (BEFORE STREQUAL AFTER)
    fail("Expected the player to have moved through the jump, and frame 5 is the same as frame 60")
  endif ()
  if (WITHOUT STREQUAL AFTER)
    fail("Expected the jump to carry the player on after W was released, and frame 60 is the same without it")
  endif ()

  # It landed in the doorway, four metres from where it started, which is
  # where a second of walking takes it: the same pixels as after the walk,
  # read at the same frame, since the walker's shadow moves with time.
  set(FRAME "jump/frame-0060.png")
  expect_pixel("the sky above the room after the jump" 960 160 0 0 0)
  expect_pixel("the north wall of the room after the jump" 700 500 136 133 153)
  expect_pixel("the target on the north wall after the jump" 960 580 204 69 43)
  expect_pixel("the walker to the right after the jump" 1180 700 46 87 165)
  expect_pixel("the floor of the room after the jump" 1000 1000 178 173 184)

  # halfway through the jump the eyes are above the walls, and the black
  # behind them has taken the place of the north wall
  set(FRAME "jump/frame-0016.png")
  expect_image("${FRAME}")
  expect_pixel("the black above the walls in the air" 700 500 0 0 0)
elseif (CASE STREQUAL "the-player-walks-into-the-room")
  # In the first frame the player stands where the scene put it, with the
  # black behind the walls above the doorway. A second later it stands in
  # the doorway: the lintel is above it where the black was, and through
  # the doorway the north wall of the room fills the middle of the frame
  # where the west post of the doorway was.
  set(FRAME "shots/frame-0001.png")
  expect_image("${FRAME}")
  expect_pixel("the black above the doorway before the walk" 960 160 0 0 0)
  expect_pixel("the west post of the doorway before the walk" 750 700 58 61 80)

  # Inside the room: the doorway is behind the player now, the target on the
  # north wall is ahead, the walker to the right. Before the colliders of
  # the doorway stood beside it (#190) the posts stopped the walker under the
  # lintel, which is what the old check saw. The target has a body since
  # #231, and the walker passes east of it before it reaches the wall, so
  # the frame is the one from before.
  set(FRAME "shots/frame-0060.png")
  expect_image("${FRAME}")
  expect_pixel("the sky above the room after the walk" 960 160 0 0 0)
  expect_pixel("the north wall of the room after the walk" 700 500 136 133 153)
  expect_pixel("the target on the north wall after the walk" 960 580 204 69 43)
  expect_pixel("the walker to the right after the walk" 1180 700 46 87 165)
  expect_pixel("the floor of the room after the walk" 1000 1000 178 173 184)

  file(SHA256 "${DIRECTORY}/shots/frame-0001.png" BEFORE)
  file(SHA256 "${DIRECTORY}/shots/frame-0060.png" AFTER)
  if (BEFORE STREQUAL AFTER)
    fail("Expected the frame to change while the player walked, and frame 1 is the same as frame 60")
  endif ()
elseif (CASE STREQUAL "the-kit-pieces-are-loaded-once")
  # expect_output_count(<text> <count>): how many lines of the output hold
  # the text
  function(expect_output_count TEXT COUNT)
    string(REGEX MATCHALL "[^\n]*${TEXT}[^\n]*" LINES "${OUTPUT}")
    list(LENGTH LINES FOUND)
    if (NOT FOUND EQUAL COUNT)
      fail("Expected ${COUNT} lines of the output to hold '${TEXT}', ${FOUND} do")
    endif ()
  endfunction()

  # The level places 17 prefabs of four kinds and draws a few more pieces
  # written out in full. Every model is read once for every path, and
  # every texture once, however many render objects draw them: the walls
  # share one wall.glb, and every piece of the kit shares its
  # colormap. One piece, what the walker looks like, is a shape the engine builds, and is no
  # model. The numbers are those of prototype.scene.yml.
  # the 11 shared placements take a shared material too
  expect_output("Created 21 render objects: 9 models were loaded and 11 shared")
  expect_output("10 materials were made and 11 shared")
  expect_output_count("Created render object [0-9]+ from assets://models/kit/wall.glb" 8)
  expect_output_count("Initializing texture from assets://models/kit/colormap.png" 1)
  expect_output("Model assets://models/kit/wall.glb is shared, 8 render objects draw it now")

  # and freed once, when the last render object that drew it was destroyed
  expect_output_count("Model assets://models/kit/wall.glb was freed, nothing draws it any more" 1)
  expect_output_count("Texture assets://models/kit/colormap.png\\|color was freed" 1)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
