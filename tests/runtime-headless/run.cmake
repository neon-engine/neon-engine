# Starts NeonRuntime without a window, lets it render its scene, and looks at
# the exit code and at the images it saved.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

# run_headless(<argument>...)
function(run_headless)
  run(--headless-renderer --time-step 0.016667 ${ARGN})

  set(EXIT_CODE "${EXIT_CODE}" PARENT_SCOPE)
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
endfunction()

# expect_image_size(<path> <width> <height>): what the header of the PNG says
function(expect_image_size PATH WIDTH HEIGHT)
  expect_image("${PATH}")

  file(READ "${DIRECTORY}/${PATH}" HEADER OFFSET 16 LIMIT 8 HEX)
  math(EXPR WIDTH_HEX "${WIDTH}" OUTPUT_FORMAT HEXADECIMAL)
  math(EXPR HEIGHT_HEX "${HEIGHT}" OUTPUT_FORMAT HEXADECIMAL)
  string(SUBSTRING "${WIDTH_HEX}" 2 -1 WIDTH_HEX)
  string(SUBSTRING "${HEIGHT_HEX}" 2 -1 HEIGHT_HEX)
  string(REPEAT "0" 8 ZEROS)
  string(SUBSTRING "${ZEROS}${WIDTH_HEX}" 0 -1 PADDED)
  string(LENGTH "${WIDTH_HEX}" WIDTH_LENGTH)
  math(EXPR CUT "8 - ${WIDTH_LENGTH}")
  string(SUBSTRING "${ZEROS}" 0 ${CUT} WIDTH_PAD)
  string(LENGTH "${HEIGHT_HEX}" HEIGHT_LENGTH)
  math(EXPR CUT "8 - ${HEIGHT_LENGTH}")
  string(SUBSTRING "${ZEROS}" 0 ${CUT} HEIGHT_PAD)

  if (NOT HEADER STREQUAL "${WIDTH_PAD}${WIDTH_HEX}${HEIGHT_PAD}${HEIGHT_HEX}")
    fail("Expected ${PATH} to be ${WIDTH} by ${HEIGHT} pixels, its header says ${HEADER}")
  endif ()
endfunction()

if (CASE STREQUAL "window-size-and-render-scale")
  # 640 by 360 points at two pixels a point is 1280 by 720 pixels
  run_headless(--frames 2 --output-dir shots --screenshot output://frame.png --window-size 640x360 --render-scale 2
          --ui assets://ui/hud.ui.yml)
elseif (CASE STREQUAL "settings-menu-with-input")
  # a name is typed, and the dropdown of the texture filtering is opened. The steps
  # are on lines of their own, since a semicolon is a list to CMake
  set(SCRIPT "20: pointer 1000 321\n21: click\n23: text Lovelace\n30: pointer 1000 439\n31: click")
  run_headless(--output-dir shots --screenshot output://frame.png --screenshot-at 20,40
          --scene assets://scenes/settings-demo.scene.yml --input "${SCRIPT}")
elseif (CASE STREQUAL "quality-preset-of-the-project")
  # pause, down to Settings, accept; the dropdown of the quality is opened
  # with the pointer, and down twice from high is the project's potato,
  # which accept chooses
  set(SCRIPT "5: hold pause\n8: hold ui-down\n11: hold ui-accept\n20: pointer 1000 246\n21: click\n25: hold ui-down\n28: hold ui-down\n31: hold ui-accept")
  run_headless(--output-dir shots --screenshot output://frame.png --screenshot-at 18,40
          --scene assets://scenes/hud-demo.scene.yml --input "${SCRIPT}")
elseif (CASE STREQUAL "quality-of-the-command-line-from-the-project")
  run_headless(--frames 1 --quality potato)
elseif (CASE STREQUAL "ui-scale")
  run_headless(--frames 2 --output-dir shots --screenshot output://plain.png --ui assets://ui/hud.ui.yml)
  set(PLAIN_EXIT_CODE "${EXIT_CODE}")
  run_headless(--frames 2 --output-dir shots --screenshot output://large.png --ui assets://ui/hud.ui.yml --ui-scale 1.5)
elseif (CASE STREQUAL "last-frame")
  run_headless(--frames 3 --output-dir shots --screenshot output://frame.png)
elseif (CASE STREQUAL "listed-frames")
  run_headless(--output-dir shots --screenshot output://today/frame.png --screenshot-at 3,1)
elseif (CASE STREQUAL "without-screenshot")
  run_headless(--frames 2)
elseif (CASE STREQUAL "screenshot-for-the-user")
  run_headless(--frames 1 --screenshot user://shots/frame.png)
elseif (CASE STREQUAL "output-folder-cannot-be-used")
  # a file is where the folder would have to be
  file(WRITE "${DIRECTORY}/taken" "a file, not a folder")
  run_headless(--frames 1 --output-dir taken --screenshot output://frame.png)
elseif (CASE STREQUAL "user-interface")
  # the same frame without and with a user interface on top of it
  run_headless(--frames 2 --output-dir shots --screenshot output://plain.png)
  set(PLAIN_EXIT_CODE "${EXIT_CODE}")
  run_headless(--frames 2 --output-dir shots --screenshot output://frame.png --ui assets://ui/hud.ui.yml)
elseif (CASE STREQUAL "user-interface-of-a-scene")
  run_headless(--frames 2 --output-dir shots --screenshot output://frame.png
          --scene assets://scenes/hud-demo.scene.yml)
elseif (CASE STREQUAL "user-interface-that-is-missing")
  run_headless(--frames 2 --output-dir shots --screenshot output://frame.png --ui assets://ui/missing.ui.yml)
elseif (CASE STREQUAL "scene-that-is-missing")
  # many frames are asked for, and none is drawn
  run_headless(--frames 300 --output-dir shots --screenshot output://frame.png
          --scene assets://scenes/missing.scene.yml)
elseif (CASE STREQUAL "pause-menu")
  # pause is pressed, the menu is closed with cancel, and pressed again
  set(SCRIPT "5: hold pause\n20: hold ui-cancel\n30: hold pause")
  run_headless(--output-dir shots --screenshot output://frame.png --screenshot-at 3,10,25,40
          --scene assets://scenes/hud-demo.scene.yml --input "${SCRIPT}")
elseif (CASE STREQUAL "title-screen-starts-the-level")
  # Start has the focus; accept chooses it, which asks for the level
  run_headless(--frames 12 --output-dir shots --screenshot output://frame.png --screenshot-at 3,12
          --scene assets://scenes/title.scene.yml --input "5: hold ui-accept")
elseif (CASE STREQUAL "input-map-of-the-engine-by-default")
  run_headless(--frames 1)
elseif (CASE STREQUAL "hold-of-an-action-the-map-does-not-have")
  run_headless(--frames 1 --input "1: hold fly")
elseif (CASE STREQUAL "vulkan-version")
  run_headless(--frames 1 --vulkan-version 1.1)
elseif (CASE STREQUAL "capabilities")
  run_headless(--frames 1)
elseif (CASE STREQUAL "vulkan-version-below-what-is-needed")
  run_headless(--frames 1 --vulkan-version 1.0)
elseif (CASE STREQUAL "log-entity")
  # a crate that falls, a character that walks, and an entity the scene does
  # not have
  run_headless(--frames 3 --scene assets://scenes/physics.scene.yml --log-entity crates/upper,walker,ghost)
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()

# Refused before a driver is looked for, so it is the same on a machine
# that cannot render
if (CASE STREQUAL "vulkan-version-below-what-is-needed")
  expect_exit_code(1)
  expect_output("Vulkan 1.0 was asked for, but the engine needs Vulkan 1.1 at least")
  return()
endif ()

# the script is checked against the input map before anything is rendered
if (CASE STREQUAL "hold-of-an-action-the-map-does-not-have")
  expect_exit_code(1)
  expect_output("'hold fly' names an action the input map does not have. It has: move, look, jump, run, pause")
  return()
endif ()

# a machine that cannot render at all skips the test
string(FIND "${OUTPUT}" "Failed to initialize Vulkan" NO_VULKAN)
if (NOT NO_VULKAN EQUAL -1)
  message("SKIPPED: this machine cannot start the Vulkan renderer")
  message("${OUTPUT}")
  return()
endif ()

if (CASE STREQUAL "window-size-and-render-scale")
  expect_exit_code(0)
  expect_no_output("[error]")
  expect_no_output("[warning]")
  expect_image_size("shots/frame.png" 1280 720)
elseif (CASE STREQUAL "log-entity")
  expect_exit_code(0)
  expect_output("Rendered 3 frames, stopping")
  expect_no_output("[error]")

  # every frame says where each of them is drawn and where the physics has
  # its body, the crate a RigidBody and the walker a CharacterBody
  foreach (FRAME 1 2 3)
    foreach (ENTITY crates/upper walker)
      string(REGEX MATCH "Frame ${FRAME}: ${ENTITY} is at \\[[-0-9.]+, [-0-9.]+, [-0-9.]+\\], its body at \\[[-0-9.]+, [-0-9.]+, [-0-9.]+\\]"
              LINE "${OUTPUT}")
      if (NOT LINE)
        fail("Expected a line that says where ${ENTITY} and its body are in frame ${FRAME}")
      endif ()
    endforeach ()
  endforeach ()

  # In the first frame they are drawn where the scene put them, while the
  # physics has taken its steps of the frame already: what is drawn is
  # blended between the last two steps
  expect_output("Frame 1: crates/upper is at [-0.100, 3.500, 0.000], its body at [")
  expect_output("Frame 1: walker is at [-6.000, 1.000, 4.000], its body at [")

  # and the crate falls
  string(REGEX MATCH "Frame 3: crates/upper is at [^\n]*, its body at \\[-0\\.100, ([0-9.]+), 0\\.000\\]"
          LINE "${OUTPUT}")
  if (NOT LINE OR NOT CMAKE_MATCH_1 LESS 3.5)
    fail("Expected the body of the crate to have fallen below 3.5 in frame 3, the log says '${LINE}'")
  endif ()

  # what is not there is said once, and not in every frame
  string(REGEX MATCHALL "there is no entity ghost to log the place of" MISSING "${OUTPUT}")
  list(LENGTH MISSING COUNT)
  if (NOT COUNT EQUAL 1)
    fail("Expected the missing entity to be said once, it was said ${COUNT} times")
  endif ()
  expect_output("Frame 1: there is no entity ghost to log the place of. It is logged once it is there")
  expect_no_output("ghost is at")
elseif (CASE STREQUAL "settings-menu-with-input")
  expect_exit_code(0)
  expect_output("Loading the user interface from engine://ui/settings.ui.yml")
  expect_output("Rendered 40 frames, stopping")
  expect_no_output("[error]")
  expect_no_output("[warning]")
  expect_image_size("shots/frame-0020.png" 1920 1080)
  expect_image_size("shots/frame-0040.png" 1920 1080)

  # the frame with the text typed and the list open differs from the one before
  file(SHA256 "${DIRECTORY}/shots/frame-0020.png" BEFORE)
  file(SHA256 "${DIRECTORY}/shots/frame-0040.png" AFTER)
  if (BEFORE STREQUAL AFTER)
    fail("Expected the input to change the frame")
  endif ()
elseif (CASE STREQUAL "quality-preset-of-the-project")
  expect_exit_code(0)
  expect_output("Rendered 40 frames, stopping")
  expect_no_output("[error]")
  expect_no_output("[warning]")
  expect_output("Loading the user interface from engine://ui/settings.ui.yml")

  # the menu offers the preset of the project after the engine's, and
  # choosing it sets the values the project wrote and the engine's defaults
  # for the rest
  expect_output("The menu offers the presets low, medium, high, ultra, potato, custom")
  expect_output("The menu set quality to potato")
  expect_output("The menu set anisotropy to 1")
  expect_output("The menu set texture_scale to 0.25")
  expect_output("The menu set shadow_map_size to 512")
  expect_output("The menu set shadow_cascades to 1")
  expect_output("The menu set shadow_distance to 20")
  expect_output("Textures are read with anisotropic filtering of 1x")
  expect_image("shots/frame-0018.png")
  expect_image("shots/frame-0040.png")

  # the shadows behind the menu change with the preset
  file(SHA256 "${DIRECTORY}/shots/frame-0018.png" BEFORE)
  file(SHA256 "${DIRECTORY}/shots/frame-0040.png" AFTER)
  if (BEFORE STREQUAL AFTER)
    fail("Expected the preset to change the frame")
  endif ()
elseif (CASE STREQUAL "quality-of-the-command-line-from-the-project")
  expect_exit_code(0)
  expect_output("Textures are read with anisotropic filtering of 1x")
  expect_no_output("[error]")
elseif (CASE STREQUAL "title-screen-starts-the-level")
  expect_exit_code(0)
  expect_output("Rendered 12 frames, stopping")
  expect_no_output("[error]")
  expect_no_output("[warning]")
  expect_output("Loading the user interface from assets://ui/title.ui.yml")
  expect_output("The scene assets://scenes/prototype.scene.yml was asked for by the user interface")
  expect_output("Changing the scene to assets://scenes/prototype.scene.yml")
  expect_output("Unloading the user interface of assets://ui/title.ui.yml")
  expect_output("Loading the scene from assets://scenes/prototype.scene.yml")
  expect_image("shots/frame-0003.png")
  expect_image("shots/frame-0012.png")

  # the title screen, then the level
  file(SHA256 "${DIRECTORY}/shots/frame-0003.png" TITLE)
  file(SHA256 "${DIRECTORY}/shots/frame-0012.png" LEVEL)
  if (TITLE STREQUAL LEVEL)
    fail("Expected Start to change the scene")
  endif ()
elseif (CASE STREQUAL "pause-menu")
  expect_exit_code(0)
  expect_output("Rendered 40 frames, stopping")
  expect_no_output("[error]")
  expect_no_output("[warning]")
  expect_output("Loading the user interface from engine://ui/pause.ui.yml")
  expect_output("Unloading the user interface of engine://ui/pause.ui.yml")
  expect_image("shots/frame-0003.png")
  expect_image("shots/frame-0010.png")
  expect_image("shots/frame-0025.png")
  expect_image("shots/frame-0040.png")

  # the menu is in the frame after pause, gone after cancel, and back
  # after the next press. The world stands still under it, so the frame
  # without the menu is the one from before
  file(SHA256 "${DIRECTORY}/shots/frame-0003.png" BEFORE)
  file(SHA256 "${DIRECTORY}/shots/frame-0010.png" SHOWN)
  file(SHA256 "${DIRECTORY}/shots/frame-0025.png" CLOSED)
  file(SHA256 "${DIRECTORY}/shots/frame-0040.png" AGAIN)
  if (BEFORE STREQUAL SHOWN)
    fail("Expected pause to show the menu")
  endif ()
  if (NOT CLOSED STREQUAL BEFORE)
    fail("Expected the frame after the menu was closed to be the one from before it, with the world still")
  endif ()
  if (AGAIN STREQUAL CLOSED)
    fail("Expected the second press to show the menu again")
  endif ()
elseif (CASE STREQUAL "input-map-of-the-engine-by-default")
  expect_exit_code(0)
  expect_output("Reading the input map from engine://input/default.input.yml")
  expect_no_output("[error]")
elseif (CASE STREQUAL "ui-scale")
  expect_exit_code(0)
  if (NOT PLAIN_EXIT_CODE STREQUAL "0")
    fail("Expected the run without --ui-scale to end with the exit code 0")
  endif ()
  expect_no_output("[error]")
  expect_image_size("shots/plain.png" 1920 1080)
  expect_image_size("shots/large.png" 1920 1080)

  file(SHA256 "${DIRECTORY}/shots/plain.png" PLAIN)
  file(SHA256 "${DIRECTORY}/shots/large.png" LARGE)
  if (PLAIN STREQUAL LARGE)
    fail("Expected --ui-scale to change the frame")
  endif ()
elseif (CASE STREQUAL "last-frame")
  expect_exit_code(0)
  expect_output("Rendered 3 frames, stopping")
  expect_image("shots/frame.png")
  expect_no_file("shots/frame-0003.png")
elseif (CASE STREQUAL "listed-frames")
  expect_exit_code(0)
  # the run is as long as the highest frame that was asked for
  expect_output("Rendered 3 frames, stopping")
  expect_image("shots/today/frame-0001.png")
  expect_image("shots/today/frame-0003.png")
  expect_no_file("shots/today/frame-0002.png")
  expect_no_file("shots/today/frame.png")
elseif (CASE STREQUAL "without-screenshot")
  expect_exit_code(0)
  expect_output("Rendered 2 frames, stopping")
  expect_no_file("shots")
elseif (CASE STREQUAL "screenshot-for-the-user")
  expect_exit_code(0)
  # where the folder of the user is below the home folder depends on the
  # platform
  file(GLOB_RECURSE SAVED RELATIVE "${DIRECTORY}" "${DIRECTORY}/home/frame.png")
  list(LENGTH SAVED COUNT)
  if (NOT COUNT EQUAL 1 OR NOT SAVED MATCHES "neon-engine/neon-test-game/shots/frame.png$")
    fail("Expected one image in the folder of the user, below ${DIRECTORY}/home, found '${SAVED}'")
  endif ()
  expect_image("${SAVED}")
elseif (CASE STREQUAL "user-interface")
  expect_exit_code(0)
  if (NOT PLAIN_EXIT_CODE STREQUAL "0")
    fail("Expected the run without a user interface to end with the exit code 0")
  endif ()
  expect_output("Loading the user interface from assets://ui/hud.ui.yml")
  expect_output("Loaded the font engine://fonts/inter/Inter-Regular.ttf")
  expect_no_output("[error]")
  expect_no_output("[warning]")
  expect_image("shots/plain.png")
  expect_image("shots/frame.png")

  # what is drawn on top is part of the frame that is saved
  file(SHA256 "${DIRECTORY}/shots/plain.png" PLAIN)
  file(SHA256 "${DIRECTORY}/shots/frame.png" WITH_USER_INTERFACE)
  if (PLAIN STREQUAL WITH_USER_INTERFACE)
    fail("Expected the frame with a user interface to differ from the one without")
  endif ()
elseif (CASE STREQUAL "user-interface-of-a-scene")
  expect_exit_code(0)
  expect_output("Loading the scene from assets://scenes/hud-demo.scene.yml")
  expect_output("Loading the user interface from assets://ui/hud.ui.yml")
  expect_no_output("[error]")
  expect_image("shots/frame.png")
elseif (CASE STREQUAL "user-interface-that-is-missing")
  # the run goes on without it and renders, says why, and fails all the same
  expect_exit_code(1)
  expect_output("assets://ui/missing.ui.yml: the file cannot be read")
  expect_output("The user interface assets://ui/missing.ui.yml cannot be used, nothing is shown from the start")
  expect_image("shots/frame.png")
elseif (CASE STREQUAL "scene-that-is-missing")
  # the run stops at once rather than show nothing, says why, and fails
  expect_exit_code(1)
  expect_output("The scene assets://scenes/missing.scene.yml cannot be read: there is no such file")
  expect_output("There is no scene to run, so the application stops")
  expect_no_output("Rendered 300 frames")
  expect_no_file("shots/frame.png")
elseif (CASE STREQUAL "vulkan-version")
  # what is asked for is the most that is used, and the log says what was
  expect_exit_code(0)
  expect_output(" in Vulkan 1.1. Vulkan 1.1 was asked for")
elseif (CASE STREQUAL "capabilities")
  # what the graphics card can do is logged, and without a window there
  # are no present modes to choose from
  expect_exit_code(0)
  expect_output("Textures up to ")
  expect_output("Present modes: none without a window")
  expect_output("Scene image of R16G16B16A16_SFLOAT: yes, sRGB textures: yes")
elseif (CASE STREQUAL "output-folder-cannot-be-used")
  # the run still renders, says what went wrong, and fails
  expect_exit_code(1)
  expect_output("cannot be used, output:// stays without a folder")
  expect_output("Rendered 1 frames, stopping")
  file(READ "${DIRECTORY}/taken" TAKEN)
  if (NOT TAKEN STREQUAL "a file, not a folder")
    fail("Expected the file that was in the way to be left as it was")
  endif ()
endif ()
