# Draws the sharing scenes without a window and reads the renderer's own
# account of what it loaded, shared, and freed. Three hundred crates of one
# prefab: with one material they share one model, one texture, and one
# material; with a colour each they share the model and the texture and
# need three hundred materials, which is more descriptor sets than one pool
# holds, so the pools grow and nothing fails.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

if (CASE STREQUAL "crates-with-one-material-share-it")
  set(SCENE assets://scenes/sharing-same.scene.yml)
else ()
  set(SCENE assets://scenes/sharing-distinct.scene.yml)
endif ()

run(--headless-renderer --window-size 1280x720 --render-scale 1 --time-step 0.05 --frames 2
        --output-dir shots --screenshot output://frame.png --scene ${SCENE})

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
expect_no_output("Could not allocate a descriptor set")

# expect_output_count(<text> <count>): how many lines of the output hold the text
function(expect_output_count TEXT COUNT)
  string(REGEX MATCHALL "${TEXT}" FOUND "${OUTPUT}")
  list(LENGTH FOUND GOT)
  if (NOT GOT EQUAL COUNT)
    fail("Expected ${COUNT} lines with '${TEXT}', there are ${GOT}")
  endif ()
endfunction()

# the model is loaded once and shared by the other 299 either way; its
# texture is read once, by the first material, whichever case
expect_output("1 models were loaded and 299 shared")
expect_output_count("Model assets://models/kit/crate.glb was freed" 1)

if (CASE STREQUAL "crates-with-one-material-share-it")
  # one material, 299 shares, freed once, when the renderer is cleaned up:
  # a material that nothing draws with stays until then; one pool
  expect_output("1 materials were made and 299 shared")
  expect_output("Material 0 is shared, 300 render objects draw with it now")
  expect_output("1 materials were freed, nothing draws with them any more")
  expect_output_count("Made descriptor pool" 1)
else ()
  # a material each, each taking the texture from the cache, none shared,
  # two pools of 256
  expect_output("300 materials were made and 0 shared")
  expect_output("300 materials were freed, nothing draws with them any more")
  expect_output("Made descriptor pool 2 for 256 materials")
  expect_output_count("Made descriptor pool" 2)
endif ()

expect_image("shots/frame.png")
