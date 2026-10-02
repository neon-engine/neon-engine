# Starts NeonRuntime without a window with the scene of the physics, and
# compares the images it saved. An image is the whole of what the physics led
# to: where every body is, and how it is turned.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

# run_scene(<scene> <folder> <time step> <frame>...)
#
# Saves the frames that are listed into a folder of their own.
function(run_scene SCENE FOLDER TIME_STEP)
  list(JOIN ARGN "," FRAMES)

  run(--headless-renderer
          --scene assets://scenes/${SCENE}.scene.yml
          --time-step ${TIME_STEP}
          --output-dir ${FOLDER}
          --screenshot output://frame.png
          --screenshot-at ${FRAMES})

  set(EXIT_CODE "${EXIT_CODE}" PARENT_SCOPE)
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
endfunction()

# run_physics(<folder> <time step> <frame>...): the scene of the physics
function(run_physics FOLDER TIME_STEP)
  run_scene(physics "${FOLDER}" "${TIME_STEP}" ${ARGN})
  set(EXIT_CODE "${EXIT_CODE}" PARENT_SCOPE)
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
endfunction()

# Whether every run so far could render. A machine that cannot skips the
# test.
function(skip_without_renderer)
  string(FIND "${OUTPUT}" "Failed to initialize Vulkan" NO_VULKAN)
  if (NOT NO_VULKAN EQUAL -1)
    message("SKIPPED: this machine cannot start the Vulkan renderer")
    message("${OUTPUT}")
    set(SKIPPED TRUE PARENT_SCOPE)
  endif ()
endfunction()

function(expect_same_image FIRST SECOND)
  expect_image("${FIRST}")
  expect_image("${SECOND}")

  file(SHA256 "${DIRECTORY}/${FIRST}" FIRST_HASH)
  file(SHA256 "${DIRECTORY}/${SECOND}" SECOND_HASH)
  if (NOT FIRST_HASH STREQUAL SECOND_HASH)
    fail("Expected ${FIRST} and ${SECOND} to be the same image")
  endif ()
endfunction()

function(expect_other_image FIRST SECOND)
  expect_image("${FIRST}")
  expect_image("${SECOND}")

  file(SHA256 "${DIRECTORY}/${FIRST}" FIRST_HASH)
  file(SHA256 "${DIRECTORY}/${SECOND}" SECOND_HASH)
  if (FIRST_HASH STREQUAL SECOND_HASH)
    fail("Expected ${FIRST} and ${SECOND} to be different images")
  endif ()
endfunction()

if (CASE STREQUAL "moves-and-comes-to-rest")
  run_physics(first 0.016667 1 60 120)
  skip_without_renderer()
  if (SKIPPED)
    return()
  endif ()
  expect_exit_code(0)
  expect_no_output("[error]")

  # what falls is somewhere else a second later
  expect_other_image("first/frame-0001.png" "first/frame-0060.png")
  expect_other_image("first/frame-0060.png" "first/frame-0120.png")

  # and the same run leads to the same images
  run_physics(second 0.016667 1 60 120)
  expect_exit_code(0)
  expect_same_image("first/frame-0001.png" "second/frame-0001.png")
  expect_same_image("first/frame-0060.png" "second/frame-0060.png")
  expect_same_image("first/frame-0120.png" "second/frame-0120.png")
elseif (CASE STREQUAL "same-at-every-frame-rate")
  # One and two seconds into the scene, at 30, 60, 120, and 240 frames per
  # second. The time steps are a little above the length of the frames, so
  # that the last step of a second is not a matter of rounding.
  run_physics(at-60 0.016667 60 120)
  skip_without_renderer()
  if (SKIPPED)
    return()
  endif ()
  expect_exit_code(0)

  run_physics(at-30 0.033334 30 60)
  expect_exit_code(0)
  run_physics(at-120 0.0083335 120 240)
  expect_exit_code(0)
  run_physics(at-240 0.00416675 240 480)
  expect_exit_code(0)

  expect_same_image("at-60/frame-0060.png" "at-30/frame-0030.png")
  expect_same_image("at-60/frame-0060.png" "at-120/frame-0120.png")
  expect_same_image("at-60/frame-0060.png" "at-240/frame-0240.png")

  expect_same_image("at-60/frame-0120.png" "at-30/frame-0060.png")
  expect_same_image("at-60/frame-0120.png" "at-120/frame-0240.png")
  expect_same_image("at-60/frame-0120.png" "at-240/frame-0480.png")

  # something moved in between
  expect_other_image("at-60/frame-0060.png" "at-60/frame-0120.png")
elseif (CASE STREQUAL "joints-move")
  # The scene of the joints: a door swings open, a pendulum swings, a sled
  # slides, and a glued pair tips as one. Something is somewhere else after
  # one and two seconds, and the same run leads to the same images.
  run_scene(joints first 0.016667 1 60 120)
  skip_without_renderer()
  if (SKIPPED)
    return()
  endif ()
  expect_exit_code(0)
  expect_no_output("[error]")
  expect_no_output("[warning]")

  expect_other_image("first/frame-0001.png" "first/frame-0060.png")
  expect_other_image("first/frame-0060.png" "first/frame-0120.png")

  run_scene(joints second 0.016667 1 60 120)
  expect_exit_code(0)
  expect_same_image("first/frame-0001.png" "second/frame-0001.png")
  expect_same_image("first/frame-0060.png" "second/frame-0060.png")
  expect_same_image("first/frame-0120.png" "second/frame-0120.png")
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
