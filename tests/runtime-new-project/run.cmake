# Makes a project with new-project, builds it on its own as a person does,
# and runs it without a window: the box the extension turns is drawn and
# the extension says it is up.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

# step(<what> <command>...): runs one command of the build, within a limit,
# and fails with what it printed when it does not return 0
function(step WHAT)
  execute_process(
          COMMAND ${ARGN}
          WORKING_DIRECTORY "${DIRECTORY}"
          TIMEOUT 120
          RESULT_VARIABLE RESULT
          OUTPUT_VARIABLE PRINTED
          ERROR_VARIABLE PRINTED_AS_ERROR
  )
  if (NOT RESULT EQUAL 0)
    message(FATAL_ERROR "${WHAT} failed\n\nExit code: ${RESULT}\nOutput:\n${PRINTED}${PRINTED_AS_ERROR}")
  endif ()
  set(STEP_OUTPUT "${PRINTED}" PARENT_SCOPE)
endfunction()

# the project, in the folder of the test, against this checkout
step("Making the project" "${NEW_PROJECT}" my-game --organization neon-tests --parent "${DIRECTORY}"
        --engine "${ENGINE_DIRECTORY}")
# the folder is the last line, after what the tool logged
string(STRIP "${STEP_OUTPUT}" STEP_OUTPUT)
string(REPLACE "\n" ";" LINES "${STEP_OUTPUT}")
list(GET LINES -1 PROJECT_DIRECTORY)
string(STRIP "${PROJECT_DIRECTORY}" PROJECT_DIRECTORY)
if (NOT PROJECT_DIRECTORY STREQUAL "${DIRECTORY}/my-game")
  message(FATAL_ERROR "Expected the project in ${DIRECTORY}/my-game, new-project printed '${PROJECT_DIRECTORY}'")
endif ()
foreach (FILE IN ITEMS CMakeLists.txt CMakePresets.json README.md .gitignore assets/project.yml assets/settings.yml
        assets/scenes/start.scene.yml assets/input/game.input.yml extensions/my-game/extension.yml
        extensions/my-game/my-game.cpp)
  if (NOT EXISTS "${PROJECT_DIRECTORY}/${FILE}")
    message(FATAL_ERROR "Expected the file ${FILE} in the project")
  endif ()
endforeach ()

# Built as a person builds it, with the compiler of this build and the
# runtime it made, since the presets would take the release build of the
# engine when there is one and need the compiler of the presets.
step("Configuring the project" "${CMAKE_COMMAND}"
        -S "${PROJECT_DIRECTORY}" -B "${DIRECTORY}/build" -G "${GENERATOR}"
        "-DCMAKE_C_COMPILER=${C_COMPILER}" "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}"
        -DCMAKE_BUILD_TYPE=Debug
        "-DNEON_RUNTIME_DIRECTORY=${RUNTIME_DIRECTORY}")
step("Building the project" "${CMAKE_COMMAND}" --build "${DIRECTORY}/build")

set(APPLICATION "${DIRECTORY}/build/my-game/my-game${CMAKE_EXECUTABLE_SUFFIX}")
if (NOT EXISTS "${APPLICATION}")
  message(FATAL_ERROR "Expected the game put together at ${APPLICATION}")
endif ()

run(--headless-renderer --time-step 0.05 --frames 5 --output-dir shots --screenshot output://frame.png)

expect_exit_code(0)
expect_no_output("[error]")
expect_no_output("[critical]")
expect_output("The extension of my-game is up")
expect_image(shots/frame.png)
