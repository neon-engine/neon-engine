# Helpers for the scripts of neon_add_application_test, see NeonTests.cmake.

# Empties the folder of the test, so that nothing of an earlier run is taken
# for the result of this one.
file(REMOVE_RECURSE "${DIRECTORY}")
file(MAKE_DIRECTORY "${DIRECTORY}")

# run(<argument>...)
#
# Starts the application in the folder of the test. Sets EXIT_CODE, and OUTPUT
# to everything it printed.
#
# The application is given RUN_TIMEOUT seconds, 60 unless the script says
# otherwise before calling run(), and is killed after that: a check of a few
# frames that takes minutes has hung, and the application cannot be trusted
# to notice that itself. EXIT_CODE is then "timeout".
#
# The folder of the user is sent into the folder of the test, so that user://
# does not end up in the home folder of whoever runs the tests. Linux follows
# XDG_DATA_HOME and macOS CFFIXED_USER_HOME. Windows takes no hint.
if (NOT DEFINED RUN_TIMEOUT)
  set(RUN_TIMEOUT 60)
endif ()

function(run)
  execute_process(
          COMMAND "${CMAKE_COMMAND}" -E env
          "XDG_DATA_HOME=${DIRECTORY}/home/data"
          "CFFIXED_USER_HOME=${DIRECTORY}/home"
          "${APPLICATION}" ${ARGN}
          WORKING_DIRECTORY "${DIRECTORY}"
          TIMEOUT "${RUN_TIMEOUT}"
          RESULT_VARIABLE RESULT
          OUTPUT_VARIABLE PRINTED
          ERROR_VARIABLE PRINTED_AS_ERROR
  )

  if (RESULT MATCHES "timeout")
    set(RESULT "timeout")
    string(APPEND PRINTED_AS_ERROR "\n(killed after ${RUN_TIMEOUT} seconds)")
  endif ()

  set(EXIT_CODE "${RESULT}" PARENT_SCOPE)
  set(OUTPUT "${PRINTED}${PRINTED_AS_ERROR}" PARENT_SCOPE)
endfunction()

function(fail MESSAGE)
  message(FATAL_ERROR "${MESSAGE}\n\nExit code: ${EXIT_CODE}\nOutput:\n${OUTPUT}")
endfunction()

function(expect_exit_code EXPECTED)
  if (NOT EXIT_CODE STREQUAL EXPECTED)
    fail("Expected the exit code ${EXPECTED}")
  endif ()
endfunction()

function(expect_output TEXT)
  string(FIND "${OUTPUT}" "${TEXT}" FOUND)
  if (FOUND EQUAL -1)
    fail("Expected the output to hold '${TEXT}'")
  endif ()
endfunction()

function(expect_no_output TEXT)
  string(FIND "${OUTPUT}" "${TEXT}" FOUND)
  if (NOT FOUND EQUAL -1)
    fail("Expected the output not to hold '${TEXT}'")
  endif ()
endfunction()

function(expect_no_file PATH)
  if (EXISTS "${DIRECTORY}/${PATH}")
    fail("Expected no file ${PATH}")
  endif ()
endfunction()

# Expects a PNG image of more than a few bytes.
function(expect_image PATH)
  if (NOT EXISTS "${DIRECTORY}/${PATH}")
    fail("Expected the image ${PATH}")
  endif ()

  file(READ "${DIRECTORY}/${PATH}" SIGNATURE LIMIT 8 HEX)
  if (NOT SIGNATURE STREQUAL "89504e470d0a1a0a")
    fail("Expected ${PATH} to be a PNG image, it starts with ${SIGNATURE}")
  endif ()

  file(SIZE "${DIRECTORY}/${PATH}" SIZE)
  if (SIZE LESS 100)
    fail("Expected ${PATH} to hold an image, it has ${SIZE} bytes")
  endif ()
endfunction()
