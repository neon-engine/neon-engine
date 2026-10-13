# Starts NeonEditor on the sandbox without a window, with a script that
# chooses File > Open in VS Code, and looks at what was logged, what was
# saved, and what the stand-in for VS Code was started with. The toolchain
# files are written into the editor's own folder, editor://, which run()
# sends into the folder of the test, where ToolchainFiles reads them.

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/scripts/run-application.cmake")

if (CMAKE_HOST_WIN32)
  message("SKIPPED: the stand-in for VS Code is a script of the shell")
  return()
endif ()

# the editor's own folder, as the file system places it on each platform
# under the home of the test, see docs/editor.md#the-editors-folder
set(EDITOR_MACOS "${DIRECTORY}/home/Library/Application Support/neon-engine/neon-editor")
set(EDITOR_LINUX "${DIRECTORY}/home/data/neon-engine/neon-editor")

# write_editor_file(<name> <content>): a file of editor://, on either platform
function(write_editor_file NAME CONTENT)
  file(WRITE "${EDITOR_MACOS}/${NAME}" "${CONTENT}")
  file(WRITE "${EDITOR_LINUX}/${NAME}" "${CONTENT}")
endfunction()

# write_stand_in(<folder>): a program named code that records where it was
# started and with what in opened.txt, then stays up for a while, as VS Code
# does, so that an editor that waited for it would take that long
function(write_stand_in FOLDER)
  file(WRITE "${FOLDER}/code" "#!/bin/sh\npwd > \"${DIRECTORY}/opened.txt\"\necho \"$@\" >> \"${DIRECTORY}/opened.txt\"\nsleep 15\n")
  file(CHMOD "${FOLDER}/code" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endfunction()

# run_script(<script>): runs the editor with the script of that name from
# this folder, and sets SECONDS to how long the run took
function(run_script SCRIPT)
  file(COPY "${CMAKE_CURRENT_LIST_DIR}/${SCRIPT}" DESTINATION "${DIRECTORY}/shots")
  string(TIMESTAMP STARTED "%s")
  run(--headless-renderer --window-size 1280x800 --render-scale 1 --time-step 0.016667
          --output-dir shots --editor-script "output://${SCRIPT}")
  string(TIMESTAMP ENDED "%s")
  math(EXPR SECONDS "${ENDED} - ${STARTED}")
  set(SECONDS "${SECONDS}" PARENT_SCOPE)
  set(EXIT_CODE "${EXIT_CODE}" PARENT_SCOPE)
  set(OUTPUT "${OUTPUT}" PARENT_SCOPE)
endfunction()

function(skip_without_vulkan)
  string(FIND "${OUTPUT}" "Failed to initialize Vulkan" NO_VULKAN)
  if (NOT NO_VULKAN EQUAL -1)
    message("SKIPPED: this machine cannot start the Vulkan renderer")
    message("${OUTPUT}")
    return()
  endif ()
endfunction()

# The stand-in was started in the root of the project, with `.` alone. The
# editor does not wait for it, so it may end before the stand-in has written
# its file: that is waited for, ten seconds at most.
function(expect_opened_the_root)
  foreach (TRY RANGE 100)
    if (EXISTS "${DIRECTORY}/opened.txt")
      file(STRINGS "${DIRECTORY}/opened.txt" OPENED)
      list(LENGTH OPENED LINES)
      if (LINES GREATER_EQUAL 2)
        break()
      endif ()
    endif ()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.1)
  endforeach ()
  if (NOT EXISTS "${DIRECTORY}/opened.txt")
    fail("Expected the stand-in for VS Code to have been started")
  endif ()
  file(STRINGS "${DIRECTORY}/opened.txt" OPENED)
  list(GET OPENED 0 STARTED_IN)
  list(GET OPENED 1 ARGUMENTS)
  get_filename_component(STARTED_IN "${STARTED_IN}" REALPATH)
  get_filename_component(ROOT "${PROJECT_DIRECTORY}" REALPATH)
  if (NOT STARTED_IN STREQUAL ROOT)
    fail("Expected VS Code to be started in ${ROOT}, the root of the project, it was started in ${STARTED_IN}")
  endif ()
  if (NOT ARGUMENTS STREQUAL ".")
    fail("Expected VS Code to be given '.', it was given '${ARGUMENTS}'")
  endif ()
endfunction()

# The editor did not wait for the stand-in, which stays up 15 seconds.
function(expect_did_not_wait)
  if (NOT SECONDS LESS 15)
    fail("Expected the editor not to wait for VS Code, the run took ${SECONDS} seconds")
  endif ()
endfunction()

if (CASE STREQUAL "opens-the-root-of-the-project")
  write_stand_in("${DIRECTORY}/vscode")
  write_editor_file(editor.yml "version: 1\neditors:\n  vscode: ${DIRECTORY}/vscode/code\n")
  run_script(open-in-vscode.editor-script.yml)
  skip_without_vulkan()
  expect_exit_code(0)
  expect_no_output("[error]")
  expect_no_output("[warning]")
  expect_output("Reading the toolchains from editor://editor.yml")
  expect_output("'drag from File to File' on line 11, is done")
  expect_output("'expect item Open in VS Code present' on line 12, is done")
  expect_output("'click Open in VS Code' on line 14, is done")
  expect_output("VS Code is ${DIRECTORY}/vscode/code, as the toolchain files say")
  expect_output("Opening ${PROJECT_DIRECTORY} in VS Code")
  expect_output("The script is done, all 9 steps of it")
  expect_opened_the_root()
  expect_did_not_wait()
  expect_image("shots/file-menu.png")
  expect_image("shots/opened.png")
elseif (CASE STREQUAL "user-editor-yml-wins-and-a-wrong-path-falls-back")
  write_stand_in("${DIRECTORY}/on-path")
  write_editor_file(editor.yml "version: 1\neditors:\n  vscode: ${DIRECTORY}/vscode/code\n")
  write_editor_file(user-editor.yml "version: 1\neditors:\n  vscode: /nowhere/code\n")
  # the stand-in first on PATH, then only the folders of the system, which
  # hold sleep for the stand-in and no VS Code
  set(RUN_ENVIRONMENT "PATH=${DIRECTORY}/on-path:/usr/bin:/bin")
  run_script(open-in-vscode.editor-script.yml)
  skip_without_vulkan()
  expect_exit_code(0)
  expect_no_output("[error]")
  expect_output("Reading the toolchains from editor://user-editor.yml")
  expect_output("The toolchain files name VS Code as /nowhere/code, which is not there; the usual places are looked in instead")
  expect_output("VS Code was found at ${DIRECTORY}/on-path/code")
  expect_output("Opening ${PROJECT_DIRECTORY} in VS Code")
  expect_opened_the_root()
  expect_image("shots/opened.png")
elseif (CASE STREQUAL "a-mistake-in-editor-yml-is-reported")
  write_stand_in("${DIRECTORY}/vscode")
  write_editor_file(editor.yml "version: 1\neditors:\n  vscode: [a]\n")
  write_editor_file(user-editor.yml "version: 1\neditors:\n  vscode: ${DIRECTORY}/vscode/code\n")
  run_script(mistake.editor-script.yml)
  skip_without_vulkan()
  # the mistake is an error in the log, and a run with an error in its log
  # ends with exit code 1 (Runtime::HasFailed), though the script was done
  expect_exit_code(1)
  expect_output("The script is done, all 7 steps of it")
  expect_output("[error] editor://editor.yml:3")
  expect_output("'expect text 'editor://editor.yml:3'' on line 8, is done")
  expect_output("'menu File > Open in VS Code' on line 9, is done")
  expect_output("VS Code is ${DIRECTORY}/vscode/code, as the toolchain files say")
  expect_output("Opening ${PROJECT_DIRECTORY} in VS Code")
  expect_opened_the_root()
  expect_image("shots/mistake.png")
else ()
  message(FATAL_ERROR "There is no case '${CASE}'")
endif ()
