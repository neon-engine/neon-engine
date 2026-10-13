# Starts NeonEditor on the sandbox without a window, which places editor://
# in a folder of the test (see run-application.cmake), and looks at what the
# log says of the editor's folder and its settings file. The folder is
# found where the log says it is, so that the test holds on every platform.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

# run_editor(): a run of two frames, which the editor's folder is placed in
function(run_editor)
  run(--headless-renderer --time-step 0.05 --frames 2)
  set(EXIT_CODE "${EXIT_CODE}" PARENT_SCOPE)
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
endfunction()

# the folder the log says editor:// is, into EDITOR_DIRECTORY
function(find_editor_directory)
  string(REGEX MATCH "editor:// is ([^\n\r]+)" MATCHED "${OUTPUT}")
  if (NOT MATCHED)
    fail("Expected the log to say where editor:// is")
  endif ()
  set(EDITOR_DIRECTORY "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

if (CASE STREQUAL "places-the-folder")
  run_editor()
  expect_exit_code(0)
  expect_output("editor:// is ")
  expect_output("neon-engine")
  expect_output("neon-editor")
  expect_output("There is no editor://editor-settings.yml yet, the editor starts with its defaults")
  expect_no_output("[error]")

  # the folder is there, apart from the project's user://, and without a
  # window nothing was written into it
  find_editor_directory()
  if (NOT IS_DIRECTORY "${EDITOR_DIRECTORY}")
    fail("Expected the folder ${EDITOR_DIRECTORY}")
  endif ()
  string(REGEX MATCH "user:// is ([^\n\r]+)" MATCHED "${OUTPUT}")
  if (CMAKE_MATCH_1 STREQUAL EDITOR_DIRECTORY)
    fail("Expected editor:// apart from user://, both are ${EDITOR_DIRECTORY}")
  endif ()
  if (EXISTS "${EDITOR_DIRECTORY}/editor-settings.yml" OR EXISTS "${EDITOR_DIRECTORY}/layout.ini")
    fail("Expected nothing written into ${EDITOR_DIRECTORY} without a window")
  endif ()
elseif (CASE STREQUAL "reads-the-settings")
  # the first run places the folder, the file goes in, the second reads it
  run_editor()
  expect_exit_code(0)
  find_editor_directory()
  file(WRITE "${EDITOR_DIRECTORY}/editor-settings.yml"
          "version: 1\nwindow:\n  width: 1234\n  height: 567\n  mode: borderless\nopen_windows: [Log]\n")

  run_editor()
  expect_exit_code(0)
  expect_output("The editor's settings were read from editor://editor-settings.yml: a window of 1234 by 567 points, borderless")
  expect_no_output("[error]")
elseif (CASE STREQUAL "refuses-a-mistake")
  run_editor()
  expect_exit_code(0)
  find_editor_directory()
  file(WRITE "${EDITOR_DIRECTORY}/editor-settings.yml" "version: 1\nwindow:\n  width: 1234\n  colour: red\n")

  # every problem with its line, the defaults in place of the file, and the
  # errors in the exit code, as every error of a run is
  run_editor()
  expect_exit_code(1)
  expect_output("editor://editor-settings.yml:4: ")
  expect_output("colour")
  expect_output("editor://editor-settings.yml is refused, the editor starts with its defaults and leaves the file as it is")
  file(READ "${EDITOR_DIRECTORY}/editor-settings.yml" KEPT)
  if (NOT KEPT STREQUAL "version: 1\nwindow:\n  width: 1234\n  colour: red\n")
    fail("Expected the refused file to be left as it was, it holds:\n${KEPT}")
  endif ()
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
