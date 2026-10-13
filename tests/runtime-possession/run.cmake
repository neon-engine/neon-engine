# the-character-possesses-the-cart-and-gets-off runs possession.scene.yml of
# the sandbox without a window: the character, possessed first as the first
# Possessable of the scene, walks ahead for a second, onto the plate in front
# of the cart, whose CartEntry possesses the cart in frame 13 or so. From
# then on `move`, still held until frame 20, drives the cart and not the
# character, which stands where it got in; the window is seen through the
# camera on the cart. In frame 30 `jump` is pressed, and the cart hands the
# character back in the frame after, where it stands. The frames that are
# saved show the character's view of the blue wall over the cart, the cart's
# view of its own orange top, and the character's view again. The colors
# are what the light makes of the plain colors of the scene, read from the
# first render that was checked by eye.
#
# the-player-of-a-scene-without-the-tag-is-possessed runs the blockout of
# the game of the tests, whose player carries no Possessable: it is
# possessed all the same, as the player of every scene written before the
# tag, and walks.
#
# the-cart-drives-back-and-turns, the-sandbox-possesses-nothing, and
# the-museum-player-is-possessed-once-and-walks are said where their
# results are read, below.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

if (CASE STREQUAL "the-character-possesses-the-cart-and-gets-off")
  run(--headless-renderer --window-size 1280x720 --render-scale 1 --time-step 0.05 --frames 45
          --scene assets://scenes/possession.scene.yml
          --input "1: hold-key w 20\n30: hold jump 1"
          --log-entity cart,player
          --output-dir shots --screenshot output://frame.png --screenshot-at 2,25,40)
elseif (CASE STREQUAL "the-cart-drives-back-and-turns")
  run(--headless-renderer --window-size 640x360 --render-scale 1 --time-step 0.05 --frames 80
          --scene assets://scenes/possession.scene.yml
          --input "1: hold-key w 20\n21: hold-key s 10\n31: hold-key a 10\n41: hold-key w 10\n51: hold-key d 20\n71: hold-key w 10"
          --log-entity cart,player)
elseif (CASE STREQUAL "the-sandbox-possesses-nothing")
  run(--headless-renderer --window-size 640x360 --render-scale 1 --time-step 0.05 --frames 3
          --output-dir shots --screenshot output://frame.png --screenshot-at 3)
elseif (CASE STREQUAL "the-museum-player-is-possessed-once-and-walks")
  run(--headless-renderer --window-size 320x180 --render-scale 1 --time-step 0.05 --frames 40
          --input "1: hold-key w 30" --log-entity player)
elseif (CASE STREQUAL "the-player-of-a-scene-without-the-tag-is-possessed")
  run(--headless-renderer --window-size 640x360 --render-scale 1 --time-step 0.05 --frames 10
          --scene assets://scenes/blockout.scene.yml --input "1: hold-key w 10" --log-entity player)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
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

# place_of(<frame> <variable>): where the entity was drawn in a frame, as
# a list of x, y, and z
function(place_of FRAME ENTITY VARIABLE)
  string(REGEX MATCH "Frame ${FRAME}: ${ENTITY} is at \\[([-0-9.]+), ([-0-9.]+), ([-0-9.]+)\\]" FOUND "${OUTPUT}")
  if (NOT FOUND)
    fail("Expected the log to say where ${ENTITY} is in frame ${FRAME}")
  endif ()
  set(${VARIABLE} "${CMAKE_MATCH_1};${CMAKE_MATCH_2};${CMAKE_MATCH_3}" PARENT_SCOPE)
endfunction()

# expect_pixel(<what> <frame> <x> <y> <red> <green> <blue>): the color of a
# pixel of a saved frame, each channel within 2 of what is given
function(expect_pixel WHAT FRAME X Y RED GREEN BLUE)
  set(IMAGE "shots/frame-${FRAME}.png")
  expect_image("${IMAGE}")
  execute_process(
          COMMAND "${PIXEL_PROBE}" "${DIRECTORY}/${IMAGE}" "${X},${Y}"
          RESULT_VARIABLE RESULT
          OUTPUT_VARIABLE PRINTED
          ERROR_VARIABLE PROBLEM
  )
  if (NOT RESULT EQUAL 0)
    fail("Could not read the pixel ${X},${Y} of ${IMAGE}: ${PROBLEM}")
  endif ()

  string(STRIP "${PRINTED}" PRINTED)
  string(REPLACE " " ";" PARTS "${PRINTED}")
  list(GET PARTS 1 GOT_RED)
  list(GET PARTS 2 GOT_GREEN)
  list(GET PARTS 3 GOT_BLUE)

  foreach (CHANNEL IN ITEMS RED GREEN BLUE)
    math(EXPR DIFFERENCE "${GOT_${CHANNEL}} - ${${CHANNEL}}")
    if (DIFFERENCE GREATER 2 OR DIFFERENCE LESS -2)
      fail("Expected ${WHAT} at ${X},${Y} of ${IMAGE} to be ${RED} ${GREEN} ${BLUE}, it is ${GOT_RED} ${GOT_GREEN} ${GOT_BLUE}")
    endif ()
  endforeach ()

  message("${WHAT} at ${X},${Y} of ${IMAGE} is ${GOT_RED} ${GOT_GREEN} ${GOT_BLUE}")
endfunction()

if (CASE STREQUAL "the-cart-drives-back-and-turns")
  # the cart is possessed from the plate as in the case above, and W drives
  # it ahead until frame 20; then S drives it back, A turns it to the left,
  # W drives it ahead and to the left, D turns it past where it first
  # looked, and W drives it ahead and to the right
  expect_output("Possessed 'cart'")
  place_of(21 cart AHEAD)
  place_of(31 cart BACK)
  place_of(41 cart TURNED_LEFT)
  place_of(51 cart LEFT)
  place_of(71 cart TURNED_RIGHT)
  place_of(80 cart RIGHT)
  message("The cart drove to ${AHEAD}, back to ${BACK}, turned left there, drove to ${LEFT}, turned right, and drove to ${RIGHT}")
  list(GET AHEAD 2 AHEAD_Z)
  list(GET BACK 2 BACK_Z)
  list(GET TURNED_LEFT 0 TURNED_LEFT_X)
  list(GET TURNED_LEFT 2 TURNED_LEFT_Z)
  list(GET LEFT 0 LEFT_X)
  list(GET LEFT 2 LEFT_Z)
  list(GET TURNED_RIGHT 0 TURNED_RIGHT_X)
  list(GET RIGHT 0 RIGHT_X)
  list(GET RIGHT 2 RIGHT_Z)
  if (NOT BACK_Z GREATER AHEAD_Z)
    fail("Expected S to drive the cart back from z = ${AHEAD_Z}, it is at z = ${BACK_Z}")
  endif ()
  if (NOT LEFT_X LESS TURNED_LEFT_X OR NOT LEFT_Z LESS TURNED_LEFT_Z)
    fail("Expected W after A to drive the cart ahead and to the left, from ${TURNED_LEFT} to ${LEFT}")
  endif ()
  if (NOT RIGHT_X GREATER TURNED_RIGHT_X OR NOT RIGHT_Z LESS LEFT_Z)
    fail("Expected W after D to drive the cart ahead and to the right, from ${TURNED_RIGHT} to ${RIGHT}")
  endif ()
  return()
elseif (CASE STREQUAL "the-sandbox-possesses-nothing")
  # the default scene of the sandbox has nothing to possess: the box on the
  # floor is drawn all the same
  expect_output("Nothing is possessed")
  expect_no_output("Possessed '")
  expect_pixel("the front of the box" 0003 320 200 153 83 31)
  expect_pixel("the top of the box" 0003 320 150 218 121 48)
  expect_pixel("the floor" 0003 100 300 121 121 121)
  return()
elseif (CASE STREQUAL "the-museum-player-is-possessed-once-and-walks")
  # the player of the museum, a scene written before the tag, is possessed
  # once, when the scene is read, and walks ahead with W
  string(REGEX MATCHALL "Possessed 'player'" POSSESSED "${OUTPUT}")
  list(LENGTH POSSESSED TIMES)
  if (NOT TIMES EQUAL 1)
    fail("Expected the player of the museum to be possessed once, it was ${TIMES} times")
  endif ()
  expect_no_output("Nothing is possessed")
  place_of(1 player START)
  place_of(30 player WALKED)
  list(GET START 2 START_Z)
  list(GET WALKED 2 WALKED_Z)
  message("The player of the museum walked from z = ${START_Z} to z = ${WALKED_Z}")
  if (NOT START_Z STREQUAL "4.500" OR NOT WALKED_Z LESS 0.5)
    fail("Expected the player to walk ahead more than 4 meters from z = 4.5, it went from z = ${START_Z} to z = ${WALKED_Z}")
  endif ()
  return()
endif ()

if (CASE STREQUAL "the-player-of-a-scene-without-the-tag-is-possessed")
  expect_output("Possessed 'player'")
  expect_no_output("Nothing is possessed")

  place_of(1 player START)
  place_of(10 player END)
  list(GET START 2 START_Z)
  list(GET END 2 END_Z)
  if (NOT END_Z LESS START_Z)
    fail("Expected the player to have walked ahead from z = ${START_Z}, it is at z = ${END_Z}")
  endif ()
  return()
endif ()

# the character first, then the cart, then the character again, each said
# by the world and by the hooks of the cart
expect_output("Possessed 'player'")
expect_output("Released 'player'")
expect_output("Possessed 'cart'")
expect_output("The cart is possessed: move drives it, jump gets off")
expect_output("Released 'cart'")
expect_output("The cart is left where it stands")

string(FIND "${OUTPUT}" "Possessed 'player'" PLAYER_FIRST)
string(FIND "${OUTPUT}" "Possessed 'cart'" CART)
string(FIND "${OUTPUT}" "Released 'cart'" CART_LEFT)
string(FIND "${OUTPUT}" "Possessed 'player'" PLAYER_AGAIN REVERSE)
if (NOT PLAYER_FIRST LESS CART OR NOT CART LESS CART_LEFT OR NOT CART_LEFT LESS PLAYER_AGAIN)
  fail("Expected the character to be possessed, then the cart, then the character again")
endif ()

# the cart stood until it was possessed, drove while move was held, and
# stands since; the character stood from the moment it got in
place_of(12 cart CART_BEFORE)
place_of(25 cart CART_DRIVEN)
place_of(31 cart CART_LEFT_AT)
place_of(45 cart CART_END)
place_of(20 player PLAYER_IN)
place_of(45 player PLAYER_END)
list(GET CART_BEFORE 2 CART_BEFORE_Z)
list(GET CART_DRIVEN 2 CART_DRIVEN_Z)
list(GET PLAYER_IN 2 PLAYER_IN_Z)
list(GET PLAYER_END 2 PLAYER_END_Z)
message("The cart stood at z = ${CART_BEFORE_Z}, drove to z = ${CART_DRIVEN_Z}, and the character stands at z = ${PLAYER_IN_Z}")

if (CART_BEFORE_Z LESS -6.01 OR CART_BEFORE_Z GREATER -5.99)
  fail("Expected the cart to stand at z = -6 before it was possessed, it is at ${CART_BEFORE_Z}")
endif ()
if (CART_DRIVEN_Z GREATER -8.0)
  fail("Expected the cart to have driven ahead past z = -8 while move was held, it is at ${CART_DRIVEN_Z}")
endif ()
if (NOT CART_LEFT_AT STREQUAL CART_END)
  fail("Expected the cart to stand where it was left, it was at ${CART_LEFT_AT} and is at ${CART_END}")
endif ()
if (PLAYER_IN_Z LESS -2.5 OR PLAYER_IN_Z GREATER -1.9)
  fail("Expected the character to stand on the plate at z = -2.2 while the cart drove, it is at ${PLAYER_IN_Z}")
endif ()
if (NOT PLAYER_IN_Z STREQUAL PLAYER_END_Z)
  fail("Expected the character to stand where it got in, it was at z = ${PLAYER_IN_Z} and is at ${PLAYER_END_Z}")
endif ()

# the character looks at the blue wall over the cart, the cart's camera
# looks down at the orange top of the cart on the gray floor, and the
# character looks at the wall again
expect_pixel("the blue wall seen by the character" 0002 640 360 33 51 162)
expect_pixel("the top of the cart seen by the character" 0002 640 600 162 88 33)
expect_pixel("the top of the cart seen from its camera" 0025 640 360 223 124 49)
expect_pixel("the floor beside the cart seen from its camera" 0025 300 360 124 124 124)
expect_pixel("the blue wall seen by the character again" 0040 640 360 33 51 162)
