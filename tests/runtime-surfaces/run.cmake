# Starts NeonRuntime without a window with the user interfaces that come
# with it, and looks at the exit code, at what it said, and at the images it
# saved. What the images look like is looked at by whoever changes how they
# are drawn. Here it is made sure that they are drawn at all, and without a
# complaint.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

# run_headless(<argument>...)
function(run_headless)
  run(--headless-renderer --time-step 0.05 ${ARGN})

  set(EXIT_CODE "${EXIT_CODE}" PARENT_SCOPE)
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
endfunction()

if (CASE STREQUAL "gallery")
  run_headless(--frames 5 --output-dir shots --screenshot output://frame.png --ui assets://ui/gallery.ui.yml)
elseif (CASE STREQUAL "shaders-move-with-time")
  run_headless(--output-dir shots --screenshot output://frame.png --screenshot-at 5,15
          --ui assets://ui/gallery.ui.yml)
elseif (CASE STREQUAL "surfaces-of-a-scene")
  # the same scene without and with what is shown on surfaces in the world
  run_headless(--frames 3 --output-dir shots --screenshot output://plain.png
          --scene assets://scenes/hud-demo.scene.yml)
  set(PLAIN_EXIT_CODE "${EXIT_CODE}")
  run_headless(--frames 3 --output-dir shots --screenshot output://frame.png
          --scene assets://scenes/surface-demo.scene.yml)
elseif (CASE STREQUAL "small-text")
  run_headless(--frames 2 --output-dir shots --screenshot output://frame.png --ui assets://ui/text-sizes.ui.yml)
elseif (CASE STREQUAL "pointing-at-a-screen")
  # the player walks up to the terminal, looks at its Unlock button, and
  # presses it
  set(SCRIPT "1: hold l-up 15\n2: look -290 35\n40: hold pointer-primary 3")
  run_headless(--output-dir shots --screenshot output://frame.png --screenshot-at 2,30,41
          --scene assets://scenes/surface-demo.scene.yml --input "${SCRIPT}")
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

if (CASE STREQUAL "gallery")
  expect_output("Loading the user interface from assets://ui/gallery.ui.yml")
  expect_output("Loaded the font assets://fonts/noto/NotoSansArabic-Regular.ttf")
  expect_output("Loaded the shader assets://shaders/ui/shine.frag.spv for elements")
  expect_output("Loaded the shader assets://shaders/ui/dissolve.frag.spv for elements")
  expect_output("Loaded the shader assets://shaders/ui/cooldown.frag.spv for elements")
  expect_image("shots/frame.png")
elseif (CASE STREQUAL "shaders-move-with-time")
  expect_image("shots/frame-0005.png")
  expect_image("shots/frame-0015.png")

  # the band of light is elsewhere half a second later
  file(SHA256 "${DIRECTORY}/shots/frame-0005.png" EARLIER)
  file(SHA256 "${DIRECTORY}/shots/frame-0015.png" LATER)
  if (EARLIER STREQUAL LATER)
    fail("Expected a frame with a shader that moves to differ from the one half a second before")
  endif ()
elseif (CASE STREQUAL "surfaces-of-a-scene")
  if (NOT PLAIN_EXIT_CODE STREQUAL "0")
    fail("Expected the run without surfaces to end with the exit code 0")
  endif ()

  expect_output("Created the render target 'terminal' of 1024 by 768")
  expect_output("Loading the user interface from assets://ui/terminal.ui.yml onto the surface 'terminal'")
  expect_output("Created the render target 'security' of 512 by 512")
  expect_output("Loading the user interface from assets://ui/hud.ui.yml")
  expect_output("Destroying the render target 'terminal'")
  expect_output("Destroying the render target 'security'")
  expect_image("shots/plain.png")
  expect_image("shots/frame.png")

  file(SHA256 "${DIRECTORY}/shots/plain.png" PLAIN)
  file(SHA256 "${DIRECTORY}/shots/frame.png" WITH_SURFACES)
  if (PLAIN STREQUAL WITH_SURFACES)
    fail("Expected the frame with surfaces in the world to differ from the one without")
  endif ()
elseif (CASE STREQUAL "small-text")
  expect_output("Loaded the font assets://fonts/inter/Inter-Regular.ttf")
  expect_image("shots/frame.png")
elseif (CASE STREQUAL "pointing-at-a-screen")
  expect_image("shots/frame-0002.png")
  expect_image("shots/frame-0030.png")
  expect_image("shots/frame-0041.png")

  # the button is under the dot in the middle: it shows that it is pointed
  # at, and then that it is pressed
  file(SHA256 "${DIRECTORY}/shots/frame-0002.png" BEFORE)
  file(SHA256 "${DIRECTORY}/shots/frame-0030.png" POINTED)
  file(SHA256 "${DIRECTORY}/shots/frame-0041.png" PRESSED)
  if (BEFORE STREQUAL POINTED)
    fail("Expected looking at the terminal to change the frame")
  endif ()
  if (POINTED STREQUAL PRESSED)
    fail("Expected pressing the button of the terminal to change the frame")
  endif ()
endif ()
